#include <stdio.h>
#include <mem.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <alloc.h>
#include <dos.h>
#include <time.h>

#include <cxl\cxlwin.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"

#define MAX_EMSI_ADDR   20

#define isOPUS              5
#define isMAX_PRODUCT    0xD2
#define isITALY          0x4E

#define MAJVERSION 2
#define MINVERSION 10

#define xcrc(crc,cp) ( crctab[((crc >> 8) & 255) ^ cp] ^ (crc << 8))

char remote_system[50];
char remote_sysop[20];
char remote_program[50];

struct _alias addrs[MAX_EMSI_ADDR];

int n_getpassword(int, int, int);
int n_password(char *, char *);
int send_WaZOO(int);
void get_emsi_id (char *, int);

static void get_emsi_field (char *);

int send_Hello(Sender)
int Sender;
{
   int i;
   struct _Hello Hello;
   char *sptr;
   word crc, num_errs=0;
   long response_timer;

   memset( (char *)&Hello, 0, sizeof(struct _Hello) );

   Hello.signal         = 'o';
   Hello.hello_version  =  1;
   Hello.product        = isITALY;
   Hello.product_maj    = MAJVERSION;
   Hello.product_min    = MINVERSION;

   strncpy(Hello.my_name,system_name,58);
   Hello.my_name[59]    = '\0';

   strncpy(Hello.sysop,sysop,19);
   Hello.sysop[19]      = '\0';

   if ( remote_point &&
        remote_net == alias[assumed].net &&
        remote_node == alias[assumed].node
      ) {
      if(n_getpassword(alias[assumed].zone, alias[assumed].fakenet, remote_point))
         strncpy(Hello.my_password,remote_password,7);
      else
         Hello.my_password[0] = '\0';
   }
   else if (!remote_point) {
      if(n_getpassword(remote_zone, remote_net, remote_node))
         strncpy(Hello.my_password,remote_password,7);
      else
         Hello.my_password[0] = '\0';
   }

   Hello.reserved2[0] = '\0';

   Hello.my_zone        = alias[assumed].zone;
   Hello.my_net         = alias[assumed].net;
   Hello.my_node        = alias[assumed].node;

   Hello.tranx = time (NULL) - timezone;

   Hello.capabilities = ZED_ZAPPER|ZED_ZIPPER;
   Hello.capabilities |= Y_DIETIFNA;

   if (strstr (mdm_flags, "Hst") == NULL)
      Hello.capabilities |= DOES_IANUS;

   if (!Sender) {
      if (remote_capabilities & Hello.capabilities & DOES_IANUS)
         Hello.capabilities = DOES_IANUS;
      else if (remote_capabilities & Hello.capabilities & ZED_ZAPPER)
         Hello.capabilities = ZED_ZAPPER;
      else if (remote_capabilities & Hello.capabilities & ZED_ZIPPER)
         Hello.capabilities = ZED_ZIPPER;
      else
         Hello.capabilities = Y_DIETIFNA;
   }

   if (cur_event == -1 || !(e_ptrs[cur_event]->behavior & MAT_NOREQ))
      Hello.capabilities |= WZ_FREQ;

   XON_DISABLE ();

xmit_packet:
   SENDBYTE (0x1f);

   sptr = (char *) (&Hello);
   SENDCHARS (sptr, 128, 1);

   /*--------------------------------------------------------------------*/
   /* Calculate CRC while modem is sending its buffer                    */
   /*--------------------------------------------------------------------*/
   for (crc = i = 0; i < 128; i++)
      crc = xcrc (crc, (byte) sptr[i]);

   CLEAR_INBOUND ();

   SENDBYTE ((unsigned char) (crc >> 8));
   SENDBYTE ((unsigned char) (crc & 0xff));

   response_timer = timerset (4000);

   while (!timeup(response_timer) && CARRIER)
      {
      if (!CHAR_AVAIL ())
         {
/*         if (got_ESC ())
            {
            DTR_OFF ();
            sptr  = msgtxt[M_KBD_MSG];
            goto no_response;
            }*/
         time_release ();
         continue;
         }       

      switch (i = TIMED_READ (0))
         {
         case ACK:
            return (1);

         case '?':
         case ENQ:
            if (++num_errs == 10)
                {
                sptr = msgtxt[M_FUBAR_MSG];
                goto no_response;
                }
            goto xmit_packet;

         default:
            if (i > 0)                   /* Could just be line noise */
               {
               }
            break;
         }
      }

no_response:

   return (0);
}

/*------------------------------------------------------------------------*/
/* SEND YOOHOO                                                            */
/*------------------------------------------------------------------------*/
int send_YOOHOO(instigator)
int instigator;
{
   char a;
   long val;

   wclreol();
   wputs("\r* YooHoo");

   if (instigator) {
      remote_zone = alias[assumed].zone;
      remote_net = called_net;
      remote_node = called_node;
      remote_point = 0;
   }
   else
      wputs("/2U2\n");

   if (!send_Hello(instigator)) {
      status_line("!Not send_Hello");
      return(0);
   }

   if (instigator) {
      val=timerset(600);
      while(!timeup(val)) {
         a = (char )TIMED_READ(1);
         if(a == (char )YOOHOO)
            return(get_YOOHOO(0));
      }

      status_line("!No YOOHOO/2U2");
      modem_hangup();
      return (0);
   }

   return (1);
}

int get_YOOHOO(plan_to_send_too)
int plan_to_send_too;
{
   int i, c;
   struct _Hello Hello;
   byte *sptr, num_errs=0;
   word crc, lsb, msb;
   long timer_val, hello_timeout;
   struct tm *tim;
   struct date da;
   struct time ti;

   CLEAR_INBOUND();
   CLEAR_OUTBOUND();

   XON_DISABLE ();
   _BRK_DISABLE ();

   wclreol();
   wputs("\r* YooHoo");
   if(!plan_to_send_too)
           wputs("/2U2\n");

   SENDBYTE(ENQ);

   timer_val   = timerset(4000);

recv:
   while(1) {
      if (!CARRIER || timeup(timer_val))
         goto failed;

      if ((c = TIMED_READ(20)) == 0x1F)
         break;

      if (c >= 0) {
         hello_timeout = timerset (1000);
         while (((c = PEEKBYTE ()) >= 0) && (c != 0x1f) && (CARRIER)) {
            if (timeup(hello_timeout))
               break;
            MODEM_IN ();
         }

         if (c != 0x1f) {
            CLEAR_INBOUND ();
            SENDBYTE (ENQ);
         }
      }

   }

loop:
   sptr = (char *) (&Hello);

   hello_timeout = timerset (200);

   for (i = 0, crc = 0; i < 128; i++) {
      while (PEEKBYTE () < 0) {
         if (timeup (timer_val) || timeup (hello_timeout))
            break;
        
         if (!CARRIER)
            goto failed;

//         time_release ();
      }

      if (timeup (timer_val))
         goto failed;
      else if (timeup (hello_timeout))
         break;

      c = MODEM_IN () & 0x00FF;

      sptr[i] = (char) c;
      crc = xcrc (crc, (byte) c);

      hello_timeout = timerset (200);
   }

   if (((msb = TIMED_READ (10)) < 0) || ((lsb = TIMED_READ (10)) < 0))
      goto hello_error;

   if (((msb << 8) | lsb) == crc)
      goto process_hello;

hello_error:
   if (timeup(timer_val))
      goto failed;

   if ((num_errs++) > 10)
      goto failed;

   CLEAR_INBOUND ();
   SENDBYTE ('?');
   goto recv;

process_hello:
   Hello.my_name[42] = '\0';
   Hello.sysop[19] = '\0';
   strcpy(remote_password,Hello.my_password);
   remote_zone = Hello.my_zone;
   remote_net = Hello.my_net;
   remote_node = Hello.my_node;
   remote_point = Hello.my_point;
   remote_capabilities = (Hello.capabilities)|Y_DIETIFNA;

   if (plan_to_send_too) {
      for (i=0; i < MAX_ALIAS; i++)
         if (remote_zone && alias[i].zone == remote_zone)
            break;

      if (i == MAX_ALIAS)
         assumed = 0;
      else
         assumed = i;

      for (i=0; i < MAX_ALIAS; i++)
         if ( alias[i].fakenet == remote_net && (!remote_zone || alias[i].zone == remote_zone) )
            break;
      if (i != MAX_ALIAS)
         assumed = i;

      if (remote_point) {
         for (i=0; i < MAX_ALIAS; i++)
            if ( alias[i].net == remote_net && alias[i].node == remote_node &&
                 (!remote_zone || alias[i].zone == remote_zone)
               )
               break;
         if (i != MAX_ALIAS)
            assumed = i;
      }
   }

   if (remote_net == alias[assumed].fakenet)
      status_line("*%s (%u:%u/%u.%u)",Hello.my_name,remote_zone,alias[assumed].net,alias[assumed].node,remote_node);
   else
      status_line("*%s (%u:%u/%u.%u)",Hello.my_name,remote_zone,remote_net,remote_node,remote_point);

   strcpy(remote_system, Hello.my_name);

   if (Hello.product == isOPUS) {
      sprintf (remote_program, "Remote Uses Opus Version %d.%02d", Hello.product_maj,
      (Hello.product_min == 48) ? 0 : Hello.product_min);
   }
   else if (Hello.product <= isMAX_PRODUCT)
      sprintf (remote_program,"Remote Uses %Fs Version %d.%02d",prodcode[Hello.product], Hello.product_maj, Hello.product_min);
   else
      sprintf (remote_program,"Remote Uses Program '%02x' Version %d.%02d",Hello.product, Hello.product_maj, Hello.product_min);

   status_line("*%s", remote_program);

   status_line(":Sysop: %s",Hello.sysop);
   strcpy(remote_sysop, Hello.sysop);

   if (Hello.product == isITALY && Hello.product_maj >= MAJVERSION && Hello.product_min >= MINVERSION)
      status_line(":Tranx: %08lX / %08lX", time (NULL) - timezone, Hello.tranx);

   if ( Hello.my_point &&
        Hello.my_net == alias[assumed].net &&
        Hello.my_node == alias[assumed].node
      ) {
      if(n_getpassword(alias[assumed].zone, alias[assumed].fakenet, Hello.my_point)) {
         if(n_password(remote_password,Hello.my_password)) {
            if (plan_to_send_too) {
               modem_hangup();
               goto failed;
            }
            else
               status_line (msgtxt[M_PASSWORD_OVERRIDE]);
         }
      }
   }
   else if (!Hello.my_point) {
      if(n_getpassword(Hello.my_zone, Hello.my_net,Hello.my_node)) {
         if(n_password(remote_password,Hello.my_password)) {
            if (plan_to_send_too) {
               modem_hangup();
               goto failed;
            }
            else
               status_line (msgtxt[M_PASSWORD_OVERRIDE]);
         }
      }
   }

   if (Hello.product == isITALY && Hello.product_maj >= MAJVERSION && Hello.product_min >= MINVERSION) {
      for (i = 0; i < MAX_RESYNC; i++) {
         if (remote_net == resync[i].net && remote_node == resync[i].node && remote_zone == resync[i].zone && !remote_point)
            break;
      }

      if (i < MAX_RESYNC) {
         status_line ("+Resync clock with %d:%d/%d", remote_zone, remote_net, remote_node);
         Hello.tranx += timezone;
         tim = localtime (&Hello.tranx);
         da.da_year = tim->tm_year;
         da.da_day = tim->tm_mday;
         da.da_mon = tim->tm_mon;
         ti.ti_min = tim->tm_min;
         ti.ti_hour = tim->tm_hour;
         ti.ti_sec = tim->tm_sec;
         ti.ti_hund = 0;
         setdate (&da);
         settime (&ti);
      }
   }

   CLEAR_INBOUND();

   SENDBYTE(ACK);
   SENDBYTE(YOOHOO);

   if (plan_to_send_too) {
      for(i=0; (CARRIER && i < 10); i++) {
         if((c=TIMED_READ(5))==ENQ) {
            wclreol();
            wputs("\r* YooHoo/2U2\n");

            if(!send_Hello(0))
               return(0);
            return(1);
         }
         else {
            if(c > 0)
               CLEAR_INBOUND();
            SENDBYTE(YOOHOO);
         }
      }
      goto failed;
   }

   return(1);

failed:
   modem_hangup ();
   return(0);
}

int n_getpassword (zone, net,nodo)
int zone, net, nodo;
{
   remote_password[0] = '\0';

   if ((net != nodelist.net) || (nodo != nodelist.number)) {
      if (!get_bbs_record (zone, net, nodo)) {
         if (norm_filepath)
            filepath = norm_filepath;
         if (norm_about)
            about = norm_about;
         if (norm_request_list)
            request_list = norm_request_list;
         if (norm_availist)
            availist = norm_availist;
         if (norm_request_list)
            request_list = norm_request_list;
         if (norm_max_requests)
            max_requests = norm_max_requests;
         if (norm_max_kbytes)
            max_kbytes = norm_max_kbytes;
         return(0);
      }
      else {
         if (know_filepath)
            filepath = know_filepath;
         if (know_request_list)
            request_list = know_request_list;
         if (know_about)
            about = know_about;
         if (know_availist)
            availist = know_availist;
         if (know_request_list)
            request_list = know_request_list;
         if (know_max_requests)
            max_requests = know_max_requests;
         if (know_max_kbytes)
            max_kbytes = know_max_kbytes;
      }
   }

   if(nodelist.password[0] != '\0')
      strncpy(remote_password,nodelist.password,8);

   return(1);
}

int n_password(theirs,ours)
char *theirs, *ours;
{
   if(theirs[0])
   {
      strupr(theirs);
      strupr(ours);

      if(strnicmp(theirs,ours,8)) {
         status_line(msgtxt[M_PWD_ERROR],remote_net,remote_node,ours,theirs);

         while (!OUT_EMPTY()) {
            if (!CARRIER)
               return 0;
            time_release();
         }

         return(1);
      }

      status_line (msgtxt[M_PROTECTED_SESSION]);
      if (prot_filepath)
         filepath = prot_filepath;
      if (prot_request_list)
         request_list = prot_request_list;
      if (prot_about)
         about = prot_about;
      if (prot_availist)
         availist = prot_availist;
      if (prot_request_list)
         request_list = prot_request_list;
      if (prot_max_requests)
         max_requests = prot_max_requests;
      if (prot_max_kbytes)
         max_kbytes = prot_max_kbytes;
      strncpy (theirs, ours, 8);
      theirs[6] = '\0';
   }

   return(0);
}

void get_emsi_id (s, width)
char *s;
int width;
{
   char c;
   int i = 0;
   long t;

   t = timerset (300);
   UNBUFFER_BYTES ();

   while (CARRIER && !timeup (t)) {
      while (PEEKBYTE() == -1) {
         if (!CARRIER || timeup (t))
            return;
         time_release ();
      }

      c = (char)TIMED_READ(1);
      t = timerset (300);

      if (c == 0x0D && i)
         break;
      else if (c == 0x0D) {
         i = 0;
         continue;
      }

      s[i++] = c;

      if (i >= width)
         break;
   }

   s[i] = '\0';
}

int send_emsi_handshake (caller)
int caller;
{
   int i, m;
   unsigned int crc;
   char string[512], addr[30];
   long t;

   if (caller);

   m_print ("**EMSI_INQC816\r");

   wclreol();
   wputs("* Sending EMSI-Handshake\r\n");

   string[0] = '\0';
   strcat (string, "{EMSI}{");

   for (m = 0; alias[m].net; m++) {
      sprintf (addr, "%d:%d/%d", alias[m].zone, alias[m].net, alias[m].node);
      if (m)
         strcat (string, " ");
      strcat (string, addr);
   }

   strcat (string, "}{");

   if (remote_point && remote_net == alias[assumed].net && remote_node == alias[assumed].node) {
      if(n_getpassword(alias[assumed].zone, alias[assumed].fakenet, remote_point))
         strncpy(addr,remote_password,7);
      else
         addr[0] = '\0';
   }
   else if (!remote_point) {
      if(n_getpassword(remote_zone, remote_net, remote_node))
         strncpy(addr,remote_password,7);
      else
         addr[0] = '\0';
   }

   addr[7] = '\0';
   strcat (string, addr);
   strcat (string, "}{8N1,PUA");
   if (cur_event == -1 || !(e_ptrs[cur_event]->behavior & MAT_NOREQ))
      strcat (string, ",HRQ");
   strcat (string, "}{ZAP,ZMO}{4E}{LoraBBS}{2.10}{Beta}{IDENT}{[");
   strcat (string, system_name);
   strcat (string, "][");
   if (location != NULL)
      strcat (string, location);
   strcat (string, "][");
   strcat (string, sysop);
   strcat (string, "][");
   if (phone != NULL)
      strcat (string, phone);
   strcat (string, "][");
   sprintf (addr, "%d", speed > 9600 ? 9600 : speed);
   strcat (string, addr);
   strcat (string, "][");
   if (flags != NULL)
      strcat (string, flags);
   strcat (string, "]}");

   sprintf (addr, "{TRX#}{[%08lX]}", time (NULL) - timezone);
   strcat (string, addr);

   sprintf (addr, "EMSI_DAT%04X", strlen (string));
   for (i = crc = 0; i < strlen (addr); i++)
      crc = xcrc (crc, (byte )addr[i]);

   for (i = 0; i < strlen (string); i++)
      crc = xcrc (crc, (byte )string[i]);

   t = timerset (6000);

   do {
      m_print ("**EMSI_DAT%04X%s", strlen (string), string);
      CLEAR_INBOUND ();
      m_print ("%04X\r", crc);
      FLUSH_OUTPUT ();
newid:
      get_emsi_id (addr, 14);
      if (!strncmp (addr, "**EMSI_ACKA490", 14))
         break;
      if (!strncmp (addr, "**EMSI_HBT", 10))
         goto newid;
      if (timeup (t) || !CARRIER)
         break;
   } while (!strncmp (addr, "**EMSI_NAK", 10));

   if (timeup (t) || !CARRIER)
      return (0);

   return (1);
}

int get_emsi_handshake (caller)
int caller;
{
   int i, m;
   char string[500], *p, *a, version[40], akas[100], *from, *phone, *flags;
   char password[20], got_ident;
   long tranx, t1;
   struct tm *tim;
   struct date da;
   struct time ti;

   if (caller);

resend:
   t1 = timerset (300);

   while (CARRIER && !timeup (t1)) {
      get_emsi_id (string, 10);
      if (!strncmp (string, "**EMSI_DAT", 10))
         break;
   }

   if (timeup (t1) || !CARRIER)
      return (0);

   wclreol();
   wputs("* Receiving EMSI-Handshake\r\n");

   get_emsi_field (string);      /* fingerprint,            "EMSI"    */
   if (stricmp (string, "EMSI")) {
      CLEAR_INBOUND ();
      m_print ("**EMSI_NAKEEC3\r");
      goto resend;
   }

   get_emsi_field (string);      /* system_address_list,              */

   if (strchr (string, ' ')) {
      i = 0;
      a = string;
      while ((p = strchr (a, ' ')) != NULL) {
         *p++ = '\0';
         if (i >= MAX_EMSI_ADDR)
            break;
         if (!i)
            strcpy (akas, p);
         parse_netnode (a, &addrs[i].zone, &addrs[i].net, &addrs[i].node, &addrs[i].point);
         i++;
         a = p;
      }

      if (strlen (a))
         parse_netnode (a, &addrs[i].zone, &addrs[i].net, &addrs[i].node, &addrs[i].point);
   }
   else {
      akas[0] = 0;
      parse_netnode (string, &addrs[0].zone, &addrs[0].net, &addrs[0].node, &addrs[0].point);
   }

   get_emsi_field (string);       /* password,                         */
   strcpy (password, string);

   get_emsi_field (string);       /* link_codes,                       */

   get_emsi_field (string);       /* compatibility_codes,              */
   remote_capabilities = ZED_ZIPPER;
   if (strstr (string, "ZAP"))
      remote_capabilities = ZED_ZAPPER;
   if (!strstr (string, "HRQ"))
      remote_capabilities |= WZ_FREQ;
   get_emsi_field (string);       /* mailer_product_code,              */

   get_emsi_field (string);       /* mailer_name,                      */

   get_emsi_field (version);      /* mailer_version,                   */
   sprintf (remote_program, "Remote Uses %s Version %s", string, version);

   get_emsi_field (string);      /* mailer_serial_number:    ASCII1;  */

   tranx = 0L;
   got_ident = 0;

   while (PEEKBYTE() != '\r') {
      if (!CARRIER)
         return (0);

      if (PEEKBYTE() == '{') {
         get_emsi_field (string);
         if (!strcmp (string, "TRX#")) {
            get_emsi_field (string);
            sscanf (string, "[%08X]", &tranx);
         }
         else if (!strcmp (string, "IDENT")) {
            get_emsi_field (string);

            i = 0;
            a = string;
            if (*a == '[')
               a++;

            while ((p = strchr (a, ']')) != NULL) {
               *p++ = '\0';

               if (i == 0)
                  strcpy(remote_system, a);
               else if (i == 1)
                  from = a;
               else if (i == 2)
                  strcpy(remote_sysop, a);
               else if (i == 3)
                  phone = a;
               else if (i == 4);
               else if (i == 5)
                  flags = a;
               else if (i == 6)
                  break;

               if (*p == '[')
                  p++;
               a = p;
               i++;
            }

            got_ident = 1;
         }
      }
      else if (PEEKBYTE() != -1)
         TIMED_READ (1);

      time_release ();
   }

   if (!got_ident) {
      CLEAR_INBOUND ();
      m_print ("**EMSI_NAKEEC3\r");
      goto resend;
   }

   remote_zone = addrs[0].zone;
   remote_net = addrs[0].net;
   remote_node = addrs[0].node;
   remote_point = addrs[0].point;

   status_line("*%s (%u:%u/%u.%u)", remote_system, addrs[0].zone, addrs[0].net, addrs[0].node, addrs[0].point);
   if (akas[0])
      status_line(":Aka: %s", akas);
   status_line("*%s", remote_program);
   status_line(":Sysop: %s from %s", remote_sysop, from);
   status_line(" Phone: %s", phone);
   if (strlen (flags))
      status_line(" Flags: %s", flags);
   if (tranx)
      status_line(":Tranx: %08lX / %08lX", time (NULL) - timezone, tranx);

   CLEAR_INBOUND ();
   m_print ("**EMSI_ACKA490\r");
   m_print ("**EMSI_ACKA490\r");

   if ( addrs[0].point && addrs[0].net == alias[assumed].net && addrs[0].node == alias[assumed].node ) {
      if(n_getpassword(alias[assumed].zone, alias[assumed].fakenet, addrs[0].point)) {
         if(n_password(remote_password,password)) {
            if (!caller) {
               modem_hangup();
               return (0);
            }
            else
               status_line (msgtxt[M_PASSWORD_OVERRIDE]);
         }
      }
   }
   else if (!addrs[0].point) {
      if(n_getpassword(addrs[0].zone, addrs[0].net,addrs[0].node)) {
         if(n_password(remote_password,password)) {
            if (!caller) {
               modem_hangup();
               return (0);
            }
            else
               status_line (msgtxt[M_PASSWORD_OVERRIDE]);
         }
      }
   }

   if (tranx) {
      for (i = 0; i < MAX_RESYNC; i++) {
         for (m = 0; m < MAX_EMSI_ADDR; m++) {
            if (!addrs[m].net)
               break;
            if (addrs[m].net == resync[i].net && addrs[m].node == resync[i].node && addrs[m].zone == resync[i].zone && !addrs[m].point)
               break;
         }

         if (addrs[m].net)
            break;
      }

      if (i < MAX_RESYNC) {
         status_line ("+Resync clock with %d:%d/%d", addrs[m].zone, addrs[m].net, addrs[m].node);
         tranx += timezone;
         tim = localtime (&tranx);
         da.da_year = tim->tm_year;
         da.da_day = tim->tm_mday;
         da.da_mon = tim->tm_mon;
         ti.ti_min = tim->tm_min;
         ti.ti_hour = tim->tm_hour;
         ti.ti_sec = tim->tm_sec;
         ti.ti_hund = 0;
         setdate (&da);
         settime (&ti);
      }
   }

   return (1);
}

static void get_emsi_field (s)
char *s;
{
   char c;
   int i = 0, start = 0;
   long t;

   t = timerset (300);
   UNBUFFER_BYTES ();

   while (CARRIER && !timeup (t)) {
      while (PEEKBYTE() == -1) {
         if (!CARRIER || timeup (t))
            return;
         time_release ();
      }

      c = (char)TIMED_READ(1);
      t = timerset (300);

      if (!start && c != '{')
         continue;

      if (c == '{') {
         start = 1;
         continue;
      }

      if (c == '}' && start)
         break;

      s[i++] = c;
   }

   s[i] = '\0';
}


