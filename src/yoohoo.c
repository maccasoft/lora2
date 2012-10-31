#include <stdio.h>
#include <mem.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <alloc.h>

#include <cxl\cxlwin.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"

#define isOPUS		    5
#define isMAX_PRODUCT    0x52
#define isITALY          0x4E

#define MAJVERSION 2
#define MINVERSION 0

#define xcrc(crc,cp) ( crctab[((crc >> 8) & 255) ^ cp] ^ (crc << 8))

char *snitty_message = "Sorry, Charlie";
char remote_system[50];
char remote_sysop[20];
char remote_program[50];

int n_getpassword(int, int);
int n_password(char *, char *);

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
      if(n_getpassword(alias[assumed].fakenet, remote_point))
         strncpy(Hello.my_password,remote_password,7);
      else
         Hello.my_password[0] = '\0';
   }
   else if (!remote_point) {
      if(n_getpassword(remote_net, remote_node))
         strncpy(Hello.my_password,remote_password,7);
      else
         Hello.my_password[0] = '\0';
   }

   Hello.my_password[6] = '\0';

   Hello.my_zone        = alias[assumed].zone;
   Hello.my_net         = alias[assumed].net;
   Hello.my_node        = alias[assumed].node;

   Hello.capabilities = ZED_ZAPPER|ZED_ZIPPER;
   Hello.capabilities |= Y_DIETIFNA;

   if (strstr (mdm_flags, "Hst") == NULL)
      Hello.capabilities |= DOES_IANUS;

   if (!Sender)
   {
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
      {
      crc = xcrc (crc, (byte) sptr[i]);
      }

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

   if (!send_Hello(instigator))
   {
      status_line("!Not send_Hello");
      return(0);
   }

   if (instigator)
   {
      val=timerset(600);
      while(!timeup(val))
      {
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

   CLEAR_INBOUND();
   CLEAR_OUTBOUND();

   wclreol();
   wputs("\r* YooHoo");
   if(!plan_to_send_too)
           wputs("/2U2\n");

   SENDBYTE(ENQ);

   timer_val   = timerset(4000);

recv:
   while(1)
   {
      if (!CARRIER || timeup(timer_val))
         goto failed;

      if ((c = TIMED_READ(20)) == 0x1F)
         break;

      if (c >= 0)
      {
         hello_timeout = timerset (1000);
         while (((c = PEEKBYTE ()) >= 0) && (c != 0x1f) && (CARRIER))
         {
            if (timeup(hello_timeout))
               break;
            MODEM_IN ();
         }

         if (c != 0x1f)
         {
            CLEAR_INBOUND ();
            SENDBYTE (ENQ);
         }
      }

   }

loop:
   sptr = (char *) (&Hello);

   hello_timeout = timerset (200);

   for (i = 0, crc = 0; i < 128; i++)
   {

      while (PEEKBYTE () < 0)
      {
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
   called_zone = Hello.my_zone;
   remote_zone = Hello.my_zone;
   remote_net = Hello.my_net;
   remote_node = Hello.my_node;
   remote_point = Hello.my_point;
   remote_capabilities = (Hello.capabilities)|Y_DIETIFNA;

   if (plan_to_send_too)
   {
      for (i=0; i < MAX_ALIAS; i++)
         if (remote_zone && alias[i].zone == remote_zone)
            break;

      if (i == MAX_ALIAS)
         assumed = 0;
      else
         assumed = i;

      for (i=0; i < MAX_ALIAS; i++)
         if ( alias[i].fakenet == remote_net &&
              (!remote_zone || alias[i].zone == remote_zone)
            )
            break;
      if (i != MAX_ALIAS)
         assumed = i;

      if (remote_point)
      {
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

   if (Hello.product == isOPUS)
   {
      sprintf (remote_program, "Remote Uses Opus Version %d.%02d", Hello.product_maj,
      (Hello.product_min == 48) ? 0 : Hello.product_min);
   }
   else if (Hello.product <= isMAX_PRODUCT)
      sprintf (remote_program,"Remote Uses %s Version %d.%02d",prodcode[Hello.product], Hello.product_maj, Hello.product_min);
   else
      sprintf (remote_program,"Remote Uses Program '%02x' Version %d.%02d",Hello.product, Hello.product_maj, Hello.product_min);

   status_line("*%s", remote_program);

   status_line(":Sysop: %s",Hello.sysop);
   strcpy(remote_sysop, Hello.sysop);

   if ( Hello.my_point &&
        Hello.my_net == alias[assumed].net &&
        Hello.my_node == alias[assumed].node
      )
   {
      if(n_getpassword(alias[assumed].fakenet, Hello.my_point))
      {
         if(n_password(remote_password,Hello.my_password))
         {
            if (plan_to_send_too)
            {
               modem_hangup();
               goto failed;
            }
            else
               status_line (msgtxt[M_PASSWORD_OVERRIDE]);
         }
      }
   }
   else if (!Hello.my_point)
   {
      if(n_getpassword(Hello.my_net,Hello.my_node))
      {
         if(n_password(remote_password,Hello.my_password))
         {
            if (plan_to_send_too)
            {
               modem_hangup();
               goto failed;
            }
            else
               status_line (msgtxt[M_PASSWORD_OVERRIDE]);
         }
      }
   }

   CLEAR_INBOUND();

   SENDBYTE(ACK);
   SENDBYTE(YOOHOO);

   if (plan_to_send_too)
   {
      for(i=0; (CARRIER && i < 10); i++)
      {
         if((c=TIMED_READ(5))==ENQ)
         {
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

int n_getpassword(net,nodo)
int net, nodo;
{
   remote_password[0] = '\0';

   if((net != nodelist.net) ||
      (nodo != nodelist.number))
   {
      if(!get_bbs_record(0,net,nodo))
      {
         remote_password[0] = '\0';
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
         return(0);
      }
      else
      {
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
      }
   }

   if(nodelist.password[0] != '\0')
      strncpy(remote_password,nodelist.password,8);

   return(1);
}

int n_password(theirs,ours)
char *theirs, *ours;
{
   register int i;

   if(theirs[0])
   {
      strupr(theirs);
      strupr(ours);

      if(strnicmp(theirs,ours,6))
      {
         status_line(msgtxt[M_PWD_ERROR],remote_net,remote_node,ours,theirs);

         for(i=0;i<strlen(snitty_message);i++)
            SENDBYTE(snitty_message[i]);

         while (!OUT_EMPTY())
         {
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
      strncpy (theirs, ours, 8);
      theirs[6] = '\0';
   }

   return(0);
}

