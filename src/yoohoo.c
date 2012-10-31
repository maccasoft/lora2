#include <stdio.h>
#include <stdlib.h>
#include <mem.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <alloc.h>
#include <dos.h>
#include <time.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"

#define MAX_EMSI_ADDR   30

#define isOPUS              5
#define isMAX_PRODUCT    0xE2
#define isLORA           0x4E

#include "version.h"

#define xcrc(crc,cp) ( crctab[((crc >> 8) & 255) ^ cp] ^ (crc << 8))

extern long timeout, b_mail, b_data, elapsed;
extern int to_row, n_mail, n_data, got_maildata;
extern word serial_no;
extern char *VNUM, serial_id[3];

char remote_system[60];
char remote_sysop[36];
char remote_program[60];
char remote_location[40];

struct _alias addrs[MAX_EMSI_ADDR + 1];

void build_node_queue (int zone, int net, int node, int point, int i);
int n_getpassword(int, int, int, int);
int n_password(char *, char *);
int send_WaZOO(int);
int get_emsi_field (char *);
void m_print2(char *format, ...);
int get_bbs_local_record (int zone, int net, int node, int point);

static int n_password2(char *, char *);

int send_Hello(Sender)
int Sender;
{
   int i, v, ntt_mail = 0, ntt_data = 0;
   struct _Hello Hello;
   char *sptr;
   unsigned short crc, num_errs=0;
   long response_timer, btt_mail = 0, btt_data = 0;

   memset( (char *)&Hello, 0, sizeof(struct _Hello) );

   Hello.signal         = 'o';
   Hello.hello_version  =  1;
   Hello.product        = isLORA;
   Hello.product_maj    = MAJVERSION;
   Hello.product_min    = MINVERSION;

   strncpy(Hello.my_name,system_name,57);
   Hello.my_name[58]    = '\0';

   Hello.serial_no = serial_no;

   strncpy(Hello.sysop,sysop,19);
   Hello.sysop[19]      = '\0';

   if (remote_point && remote_zone == config->alias[assumed].zone && remote_net == config->alias[assumed].net && remote_node == config->alias[assumed].node) {
      remote_password[0] = '\0';
      if (get_bbs_local_record (config->alias[assumed].zone, config->alias[assumed].fakenet, remote_point, 0)) {
         if (nodelist.password[0] != '\0')
            strcpy (remote_password, nodelist.password);
      }
      else if (get_bbs_local_record (remote_zone, remote_net, remote_node, remote_point)) {
         if (nodelist.password[0] != '\0')
            strcpy (remote_password, nodelist.password);
      }
      strncpy(Hello.my_password,remote_password,8);
   }
   else {
      if (get_bbs_local_record (remote_zone, remote_net, remote_node, remote_point)) {
         if (nodelist.password[0] != '\0')
            strcpy (remote_password, nodelist.password);
         else
            remote_password[0] = '\0';
      }
      strncpy(Hello.my_password,remote_password,8);
   }

   Hello.reserved2[0] = '\0';

   Hello.my_zone        = config->alias[assumed].zone;
   Hello.my_net         = config->alias[assumed].net;
   Hello.my_node        = config->alias[assumed].node;
   Hello.my_point       = config->alias[assumed].point;

   Hello.tranx = time (NULL);

   if (Sender) {
      build_node_queue (call_list[next_call].zone, call_list[next_call].net, call_list[next_call].node, call_list[next_call].point, next_call);
      Hello.n_mail = call_list[next_call].n_mail;
      Hello.b_mail = call_list[next_call].b_mail;
      Hello.n_data = call_list[next_call].n_data;
      Hello.b_data = call_list[next_call].b_data;
   }
   else {
      build_node_queue (remote_zone, remote_net, remote_node, remote_point, -1);
      for (i = 0; i < max_call; i++) {
         if (remote_zone == call_list[i].zone && remote_net == call_list[i].net && remote_node == call_list[i].node && remote_point == call_list[i].point) {
            ntt_mail += call_list[i].n_mail;
            btt_mail += call_list[i].b_mail;
            ntt_data += call_list[i].n_data;
            btt_data += call_list[i].b_data;
            break;
         }
      }

      if (remote_point) {
         for (v = 0; v < MAX_ALIAS && config->alias[v].net; v++) {
            if (remote_zone == config->alias[v].zone && remote_net == config->alias[v].net && remote_node == config->alias[v].node && config->alias[v].fakenet)
               break;
         }

         if (v < MAX_ALIAS && config->alias[v].net) {
            build_node_queue (remote_zone, config->alias[v].fakenet, remote_point, 0, -1);
            for (i = 0; i < max_call; i++) {
               if (remote_zone == call_list[i].zone && config->alias[v].fakenet == call_list[i].net && remote_point == call_list[i].node && !call_list[i].point) {
                  ntt_mail += call_list[i].n_mail;
                  btt_mail += call_list[i].b_mail;
                  ntt_data += call_list[i].n_data;
                  btt_data += call_list[i].b_data;
                  break;
               }
            }
         }
      }

      Hello.n_mail = ntt_mail;
      Hello.b_mail = btt_mail;
      Hello.n_data = ntt_data;
      Hello.b_data = btt_data;
   }

   Hello.capabilities = ZED_ZAPPER|ZED_ZIPPER;
   Hello.capabilities |= Y_DIETIFNA;

   if (mdm_flags == NULL)
      Hello.capabilities |= DOES_IANUS;
   else if (config->janus && strstr (mdm_flags, "Hst") == NULL)
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
//   SENDCHARS (sptr, 128, 1);

   /*--------------------------------------------------------------------*/
   /* Calculate CRC while modem is sending its buffer                    */
   /*--------------------------------------------------------------------*/
   for (crc = i = 0; i < 128; i++) {
      crc = xcrc (crc, (byte) sptr[i]);
      SENDBYTE ( (byte) sptr[i] );
   }

   CLEAR_INBOUND ();

   SENDBYTE ((unsigned char) (crc >> 8));
   SENDBYTE ((unsigned char) (crc & 0xff));

   response_timer = timerset (4000);

   while (!timeup (response_timer) && CARRIER) {
      if (!CHAR_AVAIL ()) {
         if (local_kbd == 0x1B) {
            SET_DTR_OFF ();
            sptr  = msgtxt[M_KBD_MSG];
            goto no_response;
         }
         time_release ();
         continue;
      }

      switch (i = TIMED_READ (0)) {
         case ACK:
            return (1);

         case '?':
         case ENQ:
            if (++num_errs == 10) {
               sptr = msgtxt[M_FUBAR_MSG];
               goto no_response;
            }
            goto xmit_packet;

         default:
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

   prints (7, 65, YELLOW|_BLACK, "YooHoo");

   if (instigator) {
      remote_zone = called_zone;
      remote_net = called_net;
      remote_node = called_node;
      remote_point = 0;
   }
   else
      prints (7, 65, YELLOW|_BLACK, "YooHoo/2U2");

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
   int i, c, ne, no, pp, zz;
   struct _Hello Hello;
   char string[50], *p;
   byte *sptr, num_errs=0;
   unsigned short crc, lsb, msb;
   long timer_val, hello_timeout;
   struct tm *tim;
   struct date da;
   struct time ti;

   CLEAR_INBOUND();
   CLEAR_OUTBOUND();

   XON_DISABLE ();
   _BRK_DISABLE ();

   prints (7, 65, YELLOW|_BLACK, "YooHoo");
   if(!plan_to_send_too)
      prints (7, 65, YELLOW|_BLACK, "YooHoo/2U2");

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

         time_release ();
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
         if (remote_zone && config->alias[i].zone == remote_zone)
            break;

      if (i == MAX_ALIAS)
         assumed = 0;
      else
         assumed = i;

      for (i=0; i < MAX_ALIAS; i++)
         if ( config->alias[i].fakenet == remote_net && (!remote_zone || config->alias[i].zone == remote_zone) )
            break;
      if (i != MAX_ALIAS)
         assumed = i;

      if (remote_point) {
         for (i=0; i < MAX_ALIAS; i++)
            if ( config->alias[i].net == remote_net && config->alias[i].node == remote_node &&
                 (!remote_zone || config->alias[i].zone == remote_zone)
               )
               break;
         if (i != MAX_ALIAS)
            assumed = i;
      }
   }

   if (remote_net == config->alias[assumed].fakenet)
      status_line("*%s (%u:%u/%u.%u)",Hello.my_name,remote_zone,config->alias[assumed].net,config->alias[assumed].node,remote_node);
   else
      status_line("*%s (%u:%u/%u.%u)",Hello.my_name,remote_zone,remote_net,remote_node,remote_point);

   strcpy(remote_system, Hello.my_name);

   if (Hello.product == isOPUS) {
      sprintf (remote_program, "Opus Version %d.%02d", Hello.product_maj,
      (Hello.product_min == 48) ? 0 : Hello.product_min);
   }
   else if (Hello.product <= isMAX_PRODUCT) {
      sprintf (remote_program,"%Fs Version %d.%02d",prodcode[Hello.product], Hello.product_maj, Hello.product_min);

      if (Hello.product == isLORA) {
         sprintf (string, "/%05u", Hello.serial_no);
         strcat (remote_program, string);
      }
   }
   else
      sprintf (remote_program,"Program '%02x' Version %d.%02d",Hello.product, Hello.product_maj, Hello.product_min);

   status_line(msgtxt[M_REMOTE_USES], remote_program);

   status_line(":Sysop: %s",Hello.sysop);
   strcpy(remote_sysop, Hello.sysop);

   strcpy (string, "");
   if (remote_capabilities & WZ_FREQ)
      strcat (string, "FReqs ");
   if (remote_capabilities & ZED_ZIPPER)
      strcat (string, "ZedZIP ");
   if (remote_capabilities & ZED_ZAPPER)
      strcat (string, "ZedZAP ");
   if (remote_capabilities & DOES_IANUS)
      strcat (string, "Janus ");
   status_line (":Offer: %s", string);

   got_maildata = 0;

   if (Hello.product == isLORA) {
      if (Hello.tranx) {
         Hello.tranx -= timezone;
         status_line(":Tranx: %08lX / %08lX", time (NULL), Hello.tranx);
      }
      else
         status_line (":No transaction number presented");

      if (!plan_to_send_too)
         check_duplicate_key (Hello.serial_no);

      n_mail = Hello.n_mail;
      b_mail = Hello.b_mail;
      n_data = Hello.n_data;
      b_data = Hello.b_data;
      got_maildata = 1;
   }

   if (n_getpassword (Hello.my_zone, Hello.my_net, Hello.my_node, Hello.my_point)) {
      remote_password[8] = '\0';
      if (n_password (remote_password, Hello.my_password)) {
         if (plan_to_send_too) {
            modem_hangup ();
            goto failed;
         }
         else
            status_line (msgtxt[M_PASSWORD_OVERRIDE]);
      }
   }
   else if (Hello.my_point && Hello.my_net == config->alias[assumed].net && Hello.my_node == config->alias[assumed].node ) {
      if (n_getpassword (config->alias[assumed].zone, config->alias[assumed].fakenet, Hello.my_point, 0)) {
         remote_password[8] = '\0';
         if (n_password (remote_password, Hello.my_password)) {
            if (plan_to_send_too) {
               modem_hangup ();
               goto failed;
            }
            else
               status_line (msgtxt[M_PASSWORD_OVERRIDE]);
         }
      }
   }

   strcpy (remote_location, nodelist.city);

   if (Hello.product == isLORA && Hello.product_maj >= 2 && Hello.product_min >= 30) {
      tim = localtime (&Hello.tranx);
      da.da_year = tim->tm_year;
      da.da_day = tim->tm_mday;
      da.da_mon = tim->tm_mon;
      ti.ti_min = tim->tm_min;
      ti.ti_hour = tim->tm_hour;
      ti.ti_sec = tim->tm_sec;
      ti.ti_hund = 0;

      status_line ("+Remote clock: %2d-%02d-%02d %02d:%02d:%02d", da.da_day, da.da_mon + 1, da.da_year % 100, ti.ti_hour, ti.ti_min, ti.ti_sec);

      ne = config->alias[0].net;
      no = config->alias[0].node;
      pp = config->alias[0].point;
      zz = config->alias[0].zone;

      strcpy (string, config->resync_nodes);
      if ((p = strtok (string, " ")) != NULL)
         do {
            parse_netnode2 (p, &zz, &ne, &no, &pp);
            if (remote_net == ne && remote_node == no && remote_zone == zz && remote_point == pp)
               break;
         } while ((p = strtok (NULL, " ")) != NULL);

      if (p != NULL) {
         elapsed += Hello.tranx - time (NULL);

         status_line ("+Resync clock with %d:%d/%d.%d", remote_zone, remote_net, remote_node, remote_point);
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
            prints (7, 65, YELLOW|_BLACK, "YooHoo/2U2");

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

int n_getpassword (zone, net, nodo, point)
int zone, net, nodo, point;
{
   remote_password[0] = '\0';

   if (!get_bbs_local_record (zone, net, nodo, point))
      return (0);
   else {
      if (filepath == config->norm_filepath) {
         if (config->know_filepath[0])
            filepath = config->know_filepath;
         if (config->know_okfile[0])
            request_list = config->know_okfile;
         max_requests = config->know_max_requests;
         max_kbytes = config->know_max_kbytes;
      }

      strcpy (remote_password, nodelist.password);
      if (remote_password[0] == '\0')
         return (0);
   }

   return (1);
}

int n_password(ours,theirs)
char *ours, *theirs;
{
   strupr (theirs);
   strupr (ours);

   if (stricmp (theirs, ours)) {
      status_line(msgtxt[M_PWD_ERROR],remote_zone,remote_net,remote_node,remote_point,theirs,ours);

      while (!OUT_EMPTY()) {
         if (!CARRIER)
            return 0;
         time_release();
      }

      return(1);
   }

   if (ours[0]) {
      status_line (msgtxt[M_PROTECTED_SESSION]);
      if (config->prot_filepath[0])
         filepath = config->prot_filepath;
      if (config->prot_okfile[0])
         request_list = config->prot_okfile;
      max_requests = config->prot_max_requests;
      max_kbytes = config->prot_max_kbytes;
   }

   return(0);
}

static int n_password2(theirs,ours)
char *theirs, *ours;
{
   strupr(theirs);
   strupr(ours);

   if(stricmp(theirs,ours)) {
      while (!OUT_EMPTY()) {
         if (!CARRIER)
            return 0;
         time_release();
      }

      return(1);
   }

   if (ours[0]) {
      if (config->prot_filepath[0])
         filepath = config->prot_filepath;
      if (config->prot_okfile[0])
         request_list = config->prot_okfile;
      max_requests = config->prot_max_requests;
      max_kbytes = config->prot_max_kbytes;
   }

   return(0);
}

void get_emsi_id (s, width)
char *s;
int width;
{
   int i, m, c;

   while ((c = TIMED_READ (10)) == '*' && CARRIER)
      ;

   if (c != 'E' || !CARRIER) {
      s[0] = '\0';
      return;
   }
   if ((c = TIMED_READ (10)) != 'M' || !CARRIER) {
      s[0] = '\0';
      return;
   }
   if ((c = TIMED_READ (10)) != 'S' || !CARRIER) {
      s[0] = '\0';
      return;
   }
   if ((c = TIMED_READ (10)) != 'I' || !CARRIER) {
      s[0] = '\0';
      return;
   }
   if ((c = TIMED_READ (10)) != '_' || !CARRIER) {
      s[0] = '\0';
      return;
   }


//   if (c != '*' || !CARRIER) {
//      s[0] = '\0';
//      return;
//   }

//   while ((c = PEEKBYTE ()) == '*' && CARRIER)
//      TIMED_READ (1);

//   if (c == '\r' || c == -1 || !CARRIER) {
//      TIMED_READ (1);
//      s[0] = '\0';
//      return;
//   }

   strcpy (s, "**EMSI_");

   for (i = strlen (s); i < width; i++) {
      if ((c = TIMED_READ (10)) == -1 || !CARRIER) {
         s[0] = '\0';
         return;
      }
      if (c == 0x0D)
         break;
      s[i] = c;
   }
   s[i] = '\0';

/*
   m = 2;
   for (i = 0; i < 12; i++) {
      c = TIMED_READ (10);
      if (c == -1 || !CARRIER) {
         s[0] = '\0';
         return;
      }
      if (c == '*' && m == 2) {
         i--;
         continue;
      }
      if (c == '\r') {
         s[m] = '\0';
         return;
      }
      s[m++] = c;
      if (m >= width) {
         s[m] = '\0';
         return;
      }
   }
   s[m] = '\0';
*/
}



/*
   char c;
   int i = 0;
   long t;

   t = timerset (100);

   while (CARRIER && !timeup (t)) {
      while (PEEKBYTE() == -1) {
         if (!CARRIER || timeup (t)) {
            s[i] = '\0';
            return;
         }
      }

      c = (char)TIMED_READ (10);
      t = timerset (100);

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
*/

int send_emsi_handshake (originator)
int originator;
{
   int i, m, tries, v, ntt_mail = 0, ntt_data = 0;
   unsigned short crc;
   char string[2048], addr[60];
   long t1, t2, btt_mail = 0, btt_data = 0;

   tries = 0;
   t1 = timerset (6000);

//   if (!originator)
//      m_print ("**EMSI_INQC816\r");

   if (originator)
      prints (7, 65, YELLOW|_BLACK, "EMSI/C1");
   else
      prints (7, 65, YELLOW|_BLACK, "EMSI/C2");

   string[0] = '\0';
   strcat (string, "{EMSI}{");

   for (m = 0; m < MAX_ALIAS && config->alias[m].net; m++) {
      sprintf (addr, "%d:%d/%d.%d", config->alias[m].zone, config->alias[m].net, config->alias[m].node, config->alias[m].point);
      if (m)
         strcat (string, " ");
      strcat (string, addr);
   }

   strcat (string, "}{");
   addr[0] = '\0';

   if (remote_point && remote_zone == config->alias[assumed].zone && remote_net == config->alias[assumed].net && remote_node == config->alias[assumed].node) {
      remote_password[0] = '\0';
      if (get_bbs_local_record (config->alias[assumed].zone, config->alias[assumed].fakenet, remote_point, 0)) {
         if (nodelist.password[0] != '\0')
            strcpy (remote_password, nodelist.password);
      }
      else if (get_bbs_local_record (remote_zone, remote_net, remote_node, remote_point)) {
         if (nodelist.password[0] != '\0')
            strcpy (remote_password, nodelist.password);
      }
      strcpy(addr,remote_password);
   }
   else {
      if (get_bbs_local_record (remote_zone, remote_net, remote_node, remote_point)) {
         if (nodelist.password[0] != '\0')
            strcpy (remote_password, nodelist.password);
         else
            remote_password[0] = '\0';
      }
      strcpy(addr,remote_password);
   }

   strcat (string, strupr(addr));

   strcat (string, "}{8N1,PUA");
   if (cur_event != -1 && (e_ptrs[cur_event]->behavior & MAT_NOREQ))
      strcat (string, ",HRQ");
   strcat (string, "}{");
   if (mdm_flags == NULL)
      strcat (string, "JAN,");
   else if (config->janus && strstr (mdm_flags, "Hst") == NULL)
      strcat (string, "JAN,");
   strcat (string, "ZAP,ZMO");
   if (cur_event != -1 && (e_ptrs[cur_event]->behavior & MAT_NOREQ))
      strcat (string, ",NRQ");
#ifdef __OS2__
   strcat (string, ",ARC,XMA,FNC}{4E}{LoraBBS-OS/2}{");
#else
   strcat (string, ",ARC,XMA,FNC}{4E}{LoraBBS-DOS}{");
#endif
   strcat (string, VNUM);
   strcat (string, "}{");

   activation_key ();
   if (registered) {
      sprintf (addr, "%s%05u", serial_id[0] ? serial_id : "", serial_no);
      strcat (string, addr);
   }
   else
      strcat (string, "Demo");

   strcat (string, "}{IDENT}{[");
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

   ntt_mail = 0;
   btt_mail = 0L;
   ntt_data = 0;
   btt_data = 0L;

   if (originator) {
      build_node_queue (call_list[next_call].zone, call_list[next_call].net, call_list[next_call].node, call_list[next_call].point, next_call);
      sprintf (addr, "{MTX#}{[%d][%ld][%d][%ld]}", call_list[next_call].n_mail, call_list[next_call].b_mail, call_list[next_call].n_data, call_list[next_call].b_data);
      strcat (string, addr);
   }
   else {
      for (m = 0; m < MAX_EMSI_ADDR; m++) {
         if (addrs[m].net == 0)
            break;

         build_node_queue (addrs[m].zone, addrs[m].net, addrs[m].node, addrs[m].point, -1);
         for (i = 0; i < max_call; i++) {
            if (addrs[m].zone == call_list[i].zone && addrs[m].net == call_list[i].net && addrs[m].node == call_list[i].node && addrs[m].point == call_list[i].point) {
               ntt_mail += call_list[i].n_mail;
               btt_mail += call_list[i].b_mail;
               ntt_data += call_list[i].n_data;
               btt_data += call_list[i].b_data;
               break;
            }
         }

         if (i >= max_call && addrs[m].point) {
            for (v = 0; v < MAX_ALIAS && config->alias[v].net; v++) {
               if (addrs[m].zone == config->alias[v].zone && addrs[m].net == config->alias[v].net && addrs[m].node == config->alias[v].node && config->alias[v].fakenet)
                  break;
            }
            if (v >= MAX_ALIAS || !config->alias[v].net)
               continue;

            for (i = 0; i < MAX_EMSI_ADDR && addrs[i].net; i++) {
               if (i == m)
                  continue;
               if (addrs[m].zone == addrs[i].zone && config->alias[v].fakenet == addrs[i].net && addrs[m].point == addrs[i].node && !addrs[i].point)
                  break;
            }

            if (i < MAX_EMSI_ADDR && addrs[i].net)
               continue;

            if (v < MAX_ALIAS && config->alias[v].net) {
               build_node_queue (addrs[m].zone, config->alias[v].fakenet, addrs[m].point, 0, -1);
               for (i = 0; i < max_call; i++) {
                  if (addrs[m].zone == call_list[i].zone && config->alias[v].fakenet == call_list[i].net && addrs[m].point == call_list[i].node && !call_list[i].point) {
                     ntt_mail += call_list[i].n_mail;
                     btt_mail += call_list[i].b_mail;
                     ntt_data += call_list[i].n_data;
                     btt_data += call_list[i].b_data;
                     break;
                  }
               }
            }
         }
      }

      sprintf (addr, "{MTX#}{[%d][%ld][%d][%ld]}", ntt_mail, btt_mail, ntt_data, btt_data);
      strcat (string, addr);
   }

   sprintf (addr, "EMSI_DAT%04X", strlen (string));
   for (i = crc = 0; i < strlen (addr); i++)
      crc = xcrc (crc, (byte )addr[i]);

   for (i = 0; i < strlen (string); i++)
      crc = xcrc (crc, (byte )string[i]);

   t1 = timerset (6000);
   timeout = t1;
   to_row = 8;

   for (;;) {
      m_print2 ("**EMSI_DAT%04X%s", strlen (string), string);
      UNBUFFER_BYTES ();
      CLEAR_INBOUND ();

      m_print2 ("%04X\r", crc);
      FLUSH_OUTPUT ();

      if (tries++ > 10 || !CARRIER) {
         modem_hangup ();
         status_line ("!Trans. EMSI failure");
         return (0);
      }

      t2 = timerset (1000);

      while (!timeup (t1) && CARRIER) {
         if (timeup (t2))
            break;

         if (esc_pressed ())
            return (0);

         if (PEEKBYTE() == '*') {
            get_emsi_id (addr, 14);
            if (!strnicmp (addr, "**EMSI_REQ", 10) || !strnicmp (addr, "*EMSI_REQ", 9))
               continue;
            if (!strnicmp (addr, "**EMSI_DAT", 10) || !strnicmp (addr, "*EMSI_DAT", 9)) {
               CLEAR_INBOUND ();
               m_print2 ("**EMSI_ACKA490\r");
               break;
            }
            if (!strnicmp (addr, "**EMSI_ACKA490", 14) || !strnicmp (addr, "*EMSI_ACKA490", 13))
               break;
            if (!strnicmp (addr, "**EMSI_NAKEEC3", 14) || !strnicmp (addr, "*EMSI_NAKEEC3", 13))
               break;
         }
         else if (PEEKBYTE () != -1)
            TIMED_READ(10);
         else {
            time_release ();
            if (local_kbd == 0x1B) {
               modem_hangup ();
               status_line ("!Trans. EMSI failure");
               return (0);
            }
         }
      }

      if (!strnicmp (addr, "**EMSI_ACKA490", 14) || !strnicmp (addr, "*EMSI_ACKA490", 13))
         break;
   }

   return (1);
}

int get_emsi_handshake (originator)
int originator;
{
   int i, m, tries, zz, no, ne, pp;
   char string[2048], *p, *a, version[40], akas[600], *from, *phone, *flags;
   char password[50], got_ident, knowaka, *got;
   long tranx, t1, t2;
   struct tm *tim;
   struct date da;
   struct time ti;

   tries = 0;
   t1 = timerset (1000);
   t2 = timerset (6000);

resend:
   if (tries++ > 6) {
      modem_hangup ();
      status_line ("!EMSI Error: too may tries");
      return (0);
   }

   if (!originator) {
      m_print2 ("**EMSI_REQA77E\r");
      t1 = timerset (1000);
   }
   else {
      if (tries >= 1) {
         m_print2 ("**EMSI_NAKEEC3\r");
         t1 = timerset (1000);
      }
   }

   while (CARRIER && !timeup (t1) && !timeup (t2)) {
      if (esc_pressed ())
         return (0);

      if (PEEKBYTE () == '*') {
         get_emsi_id (string, 10);
         if (!strncmp (string, "**EMSI_DAT", 10) || !strncmp (string, "*EMSI_DAT", 9))
            break;
         if (!strncmp (string, "**EMSI_HBT", 10) || !strncmp (string, "**EMSI_ACK", 10))
            t1 = timerset (1000);
         if (!strncmp (string, "**EMSI_IRQ8E08", 10) || !strncmp (string, "*EMSI_IRQ8E08", 9))
            continue;
      }
      else if (PEEKBYTE () != -1)
         TIMED_READ(10);
      else {
         time_release ();
         if (local_kbd == 0x1B) {
            modem_hangup ();
            status_line ("!Recv. EMSI failure");
            return (0);
         }
      }
   }

   if (timeup (t2)) {
      status_line ("!EMSI Error: %s", "timeout");
      modem_hangup ();
      return (0);
   }
   else if (!CARRIER) {
      status_line ("!EMSI Error: %s", "other end hung up");
      modem_hangup ();
      return (0);
   }

   if (timeup (t1))
      goto resend;

   if (originator)
      prints (7, 65, YELLOW|_BLACK, "EMSI/C2");
   else
      prints (7, 65, YELLOW|_BLACK, "EMSI/C1");

   if (!get_emsi_field (string))      /* fingerprint,            "EMSI"    */
      goto resend;

   if (stricmp (string, "EMSI"))
      goto resend;

   if (!get_emsi_field (string))      /* system_address_list,              */
      goto resend;

   memset ((char *)&addrs, 0, sizeof (struct _alias) * MAX_EMSI_ADDR + 1);

   if (strchr (string, ' ')) {
      i = 0;
      a = string;
      while ((p = strchr (a, ' ')) != NULL) {
         *p++ = '\0';
         if (i >= MAX_EMSI_ADDR)
            break;
         if (!i) {
            strncpy (akas, p, 399);
            akas[399] = '\0';
         }
         parse_netnode (a, (int *)&addrs[i].zone, (int *)&addrs[i].net, (int *)&addrs[i].node, (int *)&addrs[i].point);
         i++;
         a = p;
      }

      if (strlen (a) && i < MAX_EMSI_ADDR)
         parse_netnode (a, (int *)&addrs[i].zone, (int *)&addrs[i].net, (int *)&addrs[i].node, (int *)&addrs[i].point);
   }
   else {
      akas[0] = '\0';
      parse_netnode (string, (int *)&addrs[0].zone, (int *)&addrs[0].net, (int *)&addrs[0].node, (int *)&addrs[0].point);
   }

   if (!get_emsi_field (string))     /* password,                         */
      goto resend;
   if (strlen (string) > 49)
      string[49] = '\0';
   strcpy (password, string);

   if (!get_emsi_field (string))     /* link_codes,                       */
      goto resend;

   if (!get_emsi_field (string))     /* compatibility_codes,              */
      goto resend;

   remote_capabilities = ZED_ZIPPER;
   if (strstr (string, "ZAP"))
      remote_capabilities |= ZED_ZAPPER;
   if (strstr (string, "JAN"))
      remote_capabilities |= DOES_IANUS;
   if (!strstr (string, "NRQ"))
      remote_capabilities |= WZ_FREQ;

   if (!get_emsi_field (string))      /* mailer_product_code,              */
      goto resend;

   if (!get_emsi_field (string))     /* mailer_name,                      */
      goto resend;

   if (!get_emsi_field (version))      /* mailer_version,                   */
      goto resend;
   sprintf (remote_program, "%s Version %s", string, version);

   if (!get_emsi_field (string))     /* mailer_serial_number:    ASCII1;  */
      goto resend;

   if (string[0] && (strlen (remote_program) + strlen (string) < 58)) {
      strcat (remote_program, "/");
      strcat (remote_program, string);
      if (!strnicmp (remote_program, "LoraBBS", 7) && originator)
         check_duplicate_key (atoi (string));
   }

   tranx = 0L;
   got_ident = 0;
   got_maildata = 0;

   while ( (char )PEEKBYTE() != '\r') {
      if (!CARRIER)
         return (0);

      if ( (char )PEEKBYTE() == '{') {
         if (!get_emsi_field (string))
            goto resend;

         if (!strcmp (string, "TRX#")) {
            if (!get_emsi_field (string))
               goto resend;
            sscanf (&string[1], "%8lX", &tranx);
            tranx -= timezone;
         }
         else if (!strcmp (string, "MTX#")) {
            if (!get_emsi_field (string))
               goto resend;

            i = 0;
            a = string;
            if (*a == '[')
               a++;

            got = a;
            while ((p = strchr (a, ']')) != NULL) {
               if ( *(p + 1) == '[' || *(p + 1) == '}' || *(p + 1) == '\0' )
                  *p++ = '\0';
               else {
                  a = ++p;
                  continue;
               }

               if (i == 0)
                  n_mail = atoi (got);
               else if (i == 1)
                  b_mail = atol (got);
               else if (i == 2)
                  n_data = atoi (got);
               else if (i == 3)
                  b_data = atol (got);

               if (*p == '[')
                  p++;
               a = p;
               got = a;
               i++;
            }

            got_maildata = 1;
         }
         else if (!strcmp (string, "IDENT")) {
            if (!get_emsi_field (string))
               goto resend;

            i = 0;
            a = string;
            if (*a == '[')
               a++;

            got = a;
            while ((p = strchr (a, ']')) != NULL) {
               if ( *(p + 1) == '[' || *(p + 1) == '}' || *(p + 1) == '\0' )
                  *p++ = '\0';
               else {
                  a = ++p;
                  continue;
               }

               if (i == 0) {
                  if (strlen (got) > 49)
                     got[49] = '\0';
                  strcpy(remote_system, got);
               }
               else if (i == 1) {
                  from = got;
                  if (strlen (got) > 39)
                     got[39] = '\0';
                  strcpy (remote_location, from);
               }
               else if (i == 2) {
                  if (strlen (got) > 35)
                     got[35] = '\0';
                  strcpy(remote_sysop, got);
               }
               else if (i == 3)
                  phone = got;
               else if (i == 4);
               else if (i == 5)
                  flags = got;
               else if (i == 6)
                  break;

               if (*p == '[')
                  p++;
               a = p;
               got = a;
               i++;
            }

            got_ident = 1;
         }
      }
      else if (PEEKBYTE() != -1)
         TIMED_READ (10);

      time_release ();
   }

   if (!got_ident)
      goto resend;

   CLEAR_INBOUND ();
   m_print2 ("**EMSI_ACKA490\r");
   m_print2 ("**EMSI_ACKA490\r");

   remote_zone = addrs[0].zone;
   remote_net = addrs[0].net;
   remote_node = addrs[0].node;
   remote_point = addrs[0].point;

   status_line("*%s (%u:%u/%u.%u)", remote_system, addrs[0].zone, addrs[0].net, addrs[0].node, addrs[0].point);
   if (akas[0])
      status_line(":Aka: %s", akas);
   status_line(msgtxt[M_REMOTE_USES], remote_program);
   status_line(":Sysop: %s from %s", remote_sysop, from);
   status_line(" Phone: %s", phone);
   if (strlen (flags))
      status_line(" Flags: %s", flags);

   strcpy (string, "");
   if (remote_capabilities & WZ_FREQ)
      strcat (string, "FReqs ");
   if (remote_capabilities & ZED_ZIPPER)
      strcat (string, "ZedZIP ");
   if (remote_capabilities & ZED_ZAPPER)
      strcat (string, "ZedZAP ");
   if (remote_capabilities & DOES_IANUS)
      strcat (string, "Janus ");
   status_line (":Offer: %s", string);

//   remote_capabilities &= ~DOES_IANUS;
   if (remote_capabilities & ZED_ZAPPER)
      remote_capabilities &= ~ZED_ZIPPER;

   if (tranx)
      status_line (":Tranx: %08lX / %08lX", time (NULL), tranx);
   else
      status_line (":No transaction number presented");

   if (!originator) {
      assumed = -1;

      for (m = 0; m < MAX_EMSI_ADDR; m++) {
         if (!addrs[m].net)
            break;

         remote_zone = addrs[m].zone;
         remote_net = addrs[m].net;
         remote_node = addrs[m].node;
         remote_point = addrs[m].point;

         if (remote_point) {
            for (i = 0; i < MAX_ALIAS; i++)
               if ( config->alias[i].net == remote_net && config->alias[i].node == remote_node && (!remote_zone || config->alias[i].zone == remote_zone) )
                  break;

            if (i < MAX_ALIAS)
               assumed = i;
         }
         else {
            for (i = 0; i < MAX_ALIAS; i++)
               if ( config->alias[i].fakenet == remote_net && (!remote_zone || config->alias[i].zone == remote_zone) )
                  break;

            if (i < MAX_ALIAS)
               assumed = i;
         }

         if (assumed != -1)
            break;
      }

      if (assumed == -1) {
         assumed = 0;
         remote_zone = addrs[0].zone;
         remote_net = addrs[0].net;
         remote_node = addrs[0].node;
         remote_point = addrs[0].point;

         for (i = 0; i < MAX_ALIAS; i++)
            if (remote_zone && config->alias[i].zone == remote_zone)
               break;

         if (i < MAX_ALIAS)
            assumed = i;
      }
   }

   knowaka = -1;

   for (i = 0; i < MAX_EMSI_ADDR; i++) {
      if (!addrs[i].net)
         break;

      remote_zone = addrs[i].zone;
      remote_net = addrs[i].net;
      remote_node = addrs[i].node;
      remote_point = addrs[i].point;

      if (n_getpassword (remote_zone, remote_net, remote_node, remote_point)) {
         if (knowaka == -1)
            knowaka = i;
         if (!n_password2 (remote_password, password))
            break;
      }
      else if (remote_point && remote_net == config->alias[assumed].net && remote_node == config->alias[assumed].node ) {
         if (n_getpassword(config->alias[assumed].zone, config->alias[assumed].fakenet, remote_point, 0)) {
            if (knowaka == -1)
               knowaka = i;
            if (!n_password2(remote_password,password))
               break;
         }
      }
   }

   if (addrs[i].net)
      knowaka = i;

   if (knowaka != -1) {
      remote_zone = addrs[knowaka].zone;
      remote_net = addrs[knowaka].net;
      remote_node = addrs[knowaka].node;
      remote_point = addrs[knowaka].point;
   }
   else {
      remote_zone = addrs[0].zone;
      remote_net = addrs[0].net;
      remote_node = addrs[0].node;
      remote_point = addrs[0].point;
   }

   if (n_getpassword (remote_zone, remote_net, remote_node, remote_point)) {
      if(n_password (remote_password, password)) {
         if (!originator) {
            modem_hangup ();
            return (0);
         }
         else
            status_line (msgtxt[M_PASSWORD_OVERRIDE]);
      }
   }
   else if (remote_point && remote_net == config->alias[assumed].net && remote_node == config->alias[assumed].node) {
      if (n_getpassword (config->alias[assumed].zone, config->alias[assumed].fakenet, remote_point, 0)) {
         if (n_password (remote_password, password)) {
            if (!originator) {
               modem_hangup ();
               return (0);
            }
            else
               status_line (msgtxt[M_PASSWORD_OVERRIDE]);
         }
      }
   }

   if (tranx) {
      tim = localtime (&tranx);
      da.da_year = tim->tm_year;
      da.da_day = tim->tm_mday;
      da.da_mon = tim->tm_mon;
      ti.ti_min = tim->tm_min;
      ti.ti_hour = tim->tm_hour;
      ti.ti_sec = tim->tm_sec;
      ti.ti_hund = 0;

      status_line ("+Remote clock: %2d-%02d-%02d %02d:%02d:%02d", da.da_day, da.da_mon + 1, da.da_year % 100, ti.ti_hour, ti.ti_min, ti.ti_sec);

      ne = config->alias[0].net;
      no = config->alias[0].node;
      pp = config->alias[0].point;
      zz = config->alias[0].zone;

      strcpy (string, config->resync_nodes);
      if ((p = strtok (string, " ")) != NULL)
         do {
            parse_netnode2 (p, &zz, &ne, &no, &pp);
            for (m = 0; m < MAX_EMSI_ADDR; m++) {
               if (!addrs[m].net)
                  break;
               if (addrs[m].net == ne && addrs[m].node == no && addrs[m].zone == zz && addrs[m].point == pp)
                  break;
            }

            if (m < MAX_EMSI_ADDR && addrs[m].net)
               break;
         } while ((p = strtok (NULL, " ")) != NULL);

      if (p != NULL) {
         elapsed += tranx - time (NULL);

         status_line ("+Resync clock with %d:%d/%d.%d", addrs[m].zone, addrs[m].net, addrs[m].node, addrs[m].point);
         setdate (&da);
         settime (&ti);
      }
   }

/*
   if (cur_event >= 0 && (e_ptrs[cur_event]->behavior & MAT_RESERV) && !originator) {
      for (m = 0; m < MAX_EMSI_ADDR; m++) {
         if (!addrs[m].net)
            break;
         if (addrs[m].point == e_ptrs[cur_event]->res_point &&
             addrs[m].net == e_ptrs[cur_event]->res_net &&
             addrs[m].node == e_ptrs[cur_event]->res_node &&
             addrs[m].zone == e_ptrs[cur_event]->res_zone && addrs[m].zone && e_ptrs[cur_event]->res_zone)
            break;
      }

      if (!addrs[m].net) {
         status_line ("!Node not allowed in this slot");
         modem_hangup ();
         return (0);
      }
   }
*/

   return (1);
}


