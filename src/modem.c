#pragma inline

#include <dos.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#define CLEAN_MODULE
#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"

static void empty_delay (void);

int wait_for_connect(to)
int to;
{
   int i;
   unsigned int brate;
   long t1;

   t1 = timerset(6000);
   brate = 0;

   do {
      if (!to && local_kbd == ' ')
      {
         local_kbd = -1;
         return (0);
      }

      if (!to && local_kbd == 0x1B)
         return (0);

      if ((i=modem_response()) <= 0)
      {
         time_release();
         if (to)
            return (-1);
      }
      else
      {
         switch(i) {
         case 1:
            brate=300;
            break;
         case 5:
            brate=1200;
            break;
         case 11:
            brate=2400;
            break;
         case 18:
            brate=4800;
            break;
         case 12:
            brate=9600;
            break;
         case 13:
            brate = 19200;
            break;
         case 14:
            brate = 38400U;
            break;
         case 15:
            brate = 14400;
            break;
         case 16:
            brate = 7200;
            break;
         case 17:
            brate = 12000;
            break;
         }

         if (brate)
         {
            rate = brate;
            if (!lock_baud)
               com_baud(brate);
            MNP_Filter();
            return (1);
         }
         else
            return (0);
      }
   } while (!timeup(t1));

   return (0);
}

int modem_response()
{
   #define MODEM_STRING_LEN 90
   int c, i=0;
   static char stringa[MODEM_STRING_LEN];
   long t1;

   mdm_flags = NULL;

   if(PEEKBYTE() == -1)
      return(-1);

   if (terminal)
      return (terminal_response ());

   for (;;) {
      if(PEEKBYTE() != -1)
         c = MODEM_IN();
      else {
         t1 = timerset(100);

         while(PEEKBYTE() == -1)
            if(timeup(t1))
               return (-1);

         if(PEEKBYTE() != -1)
            c = MODEM_IN();
      }


      if(c == 0x0D)
         break;

      if(c < 0x20)
         continue;

      if (c == '/' && mdm_flags == NULL)
      {
         c = '\0';
         mdm_flags = (char *)&stringa[i+1];
      }

      stringa[i++]=c;

      if (i >= MODEM_STRING_LEN)
         break;
   }

   stringa[i]='\0';
   if (i < 2)
      return (-1);

   if (mdm_flags)
      fancy_str (mdm_flags);

   c = stringa[12];
   stringa[12] = '\0';
   wscrollbox (4, 0, 6, 14, 1, D_UP);
   wprints (6, 2, LCYAN|_BLUE, strupr (stringa));
   stringa[12] = c;

   if(!stricmp(stringa,"CONNECT"))
      return(1);
   else if(!stricmp(stringa,"CONNECT 300"))
      return(1);
   else if(!stricmp(stringa,"CONNECT 1200"))
      return(5);
   else if(!stricmp(stringa,"CONNECT 1275"))
      return(10);
   else if(!stricmp(stringa,"CONNECT 2400"))
      return(11);
   else if(!stricmp(stringa,"CONNECT 9600"))
      return(12);
   else if(!stricmp(stringa,"CONNECT FAST"))
      return(12);
   else if(!stricmp(stringa,"CONNECT 7200"))
      return(16);
   else if(!stricmp(stringa,"CONNECT 4800"))
      return(18);
   else if(!stricmp(stringa,"CONNECT 12000"))
      return(17);
   else if(!stricmp(stringa,"CONNECT 14400"))
      return(15);
   else if(!stricmp(stringa,"CONNECT 19200"))
      return(13);
   else if(!stricmp(stringa,"CONNECT 38400"))
      return(14);
   else if(!stricmp(stringa,"OK"))
      return(0);
   else if(!stricmp(stringa,"NO CARRIER"))
   {
      status_line(":No Carrier");
      answer_flag=0;
      return(3);
   }
   else if(!stricmp(stringa,"NO DIALTONE"))
      return(6);
   else if(!stricmp(stringa,"BUSY"))
   {
      status_line(":Busy");
      return(7);
   }
   else if(!stricmp(stringa,"NO ANSWER"))
   {
      status_line(":No Answer");
      answer_flag=0;
      return(8);
   }
   else if(!stricmp(stringa,"VOICE"))
   {
      status_line(":Voice");
      answer_flag=0;
      return(9);
   }
   else if(!strnicmp(stringa,"RING",4)) {
      if (!answer_flag)
      {
         status_line(":Ring");

         local_status("Answering the phone");

         mdm_sendcmd( (answer == NULL) ? "ATA|" : answer);
         answer_flag=1;
      }
      return(-1);
   }

   return(-1);
}

int terminal_response()
{
   int c, flag;
   long t1;

   flag = 0;

   for (;;) {
      if(PEEKBYTE() != -1)
         c = MODEM_IN();
      else {
         t1 = timerset(100);

         while(PEEKBYTE() == -1)
            if(timeup(t1))
               return (-1);

         if(PEEKBYTE() != -1)
            c = MODEM_IN();
      }

      if(c == 0x0D) {
         if (flag)
            break;
         flag = 1;
      }
   }

   switch (speed) {
   case 300:
      return (1);
   case 1200:
      return (5);
   case 2400:
      return (11);
   case 4800:
      return (18);
   case 7200:
      return (16);
   case 9600:
      return (12);
   case 12000:
      return (17);
   case 14400:
      return (15);
   case 19200:
      return (13);
   case 38400U:
      return (14);
   }

   return(-1);
}

void mdm_sendcmd(value)
char *value;
{
   register int i;
   long t;

   DTR_ON();
   if (value == NULL)
      return;

   for(i=0;value[i];i++)
      switch(value[i]) {
      case '|':
         SENDBYTE('\r');
         timer(5);
         break;
      case '~':                               /* Long delay                */
         empty_delay ();                      /* wait for buffer to clear  */
         timer(10);                           /* then wait 1 second        */
         break;
      case 'v':                               /* Lower DTR                 */
         empty_delay ();                      /* wait for buffer to clear, */
         DTR_OFF();                           /* Turn off DTR              */
         t=timerset(400);
         while(!timeup(t) && CARRIER)         /* Wait for carrier drop     */
            DTR_OFF();
         break;
      case '^':                               /* Raise DTR                 */
         empty_delay ();                      /* wait for buffer to clear  */
         DTR_ON();                            /* Turn on DTR               */
         timer(5);
         break;
      case '`':                               /* Short delay               */
         timer (1);                           /* short pause, .1 second    */
         break;
      default:
         SENDBYTE(value[i]);
         break;
      }
}

void modem_hangup()
{
   long set;

   if (local_mode)
      return;

   DTR_OFF();
   timer (5);

   set = timerset(500);
   while (CARRIER && !timeup(set) && !local_mode)
   {
      DTR_OFF();
      time_release();
   }

   if (CARRIER && !local_mode)
      status_line("!Unable to drop carrier");

   CLEAR_INBOUND();
   DTR_ON();
   timer (5);

   answer_flag = 0;
   mdm_sendcmd(init);
}

void com_baud(b)
int b;
{
        int i;
        union REGS inregs, outregs;

        if (b == 0)
                return;

        switch( (unsigned)b ) {
        case 300:
                i=BAUD_300;
                break;
        case 1200:
                i=BAUD_1200;
                break;
        case 2400:
                i=BAUD_2400;
                break;
        case 4800:
                i=BAUD_4800;
                break;
        case 7200:
                i=BAUD_7200;
                break;
        case 9600:
                i=BAUD_9600;
                break;
        case 12000:
                i=BAUD_12000;
                break;
        case 14400:
                i=BAUD_14400;
                break;
        case 19200:
                i=BAUD_19200;
                break;
        case 38400U:
                i=BAUD_38400;
                break;
        default:
                status_line("!Invalid baud rate");
        }

        inregs.h.ah=0x00;
        inregs.h.al=i|NO_PARITY|STOP_1|BITS_8;
        inregs.x.dx=com_port;
        int86(0x14,&inregs,&outregs);
}

int TIMED_READ(t)
int t;
{
   long t1;
   extern long timerset ();

   if (!CHAR_AVAIL ())
      {
      t1 = timerset ((unsigned int) (t * 100));
      while (!CHAR_AVAIL ())
         {
         if (timeup (t1))
            {
            return (EOF);
            }

         if (!CARRIER)
            {
            return (EOF);
            }
         time_release ();
         }
      }
   return ((unsigned int)(MODEM_IN ()) & 0x00ff);
}

int com_install(com_port)
int com_port;
{
   if (ComInit( (byte)com_port ) != 0x1954)
   {
      MDM_DISABLE ();
      if (ComInit( (byte)com_port ) != 0x1954)
      {
         printf(msgtxt[M_DRIVER_DEAD_1]);
         printf(msgtxt[M_DRIVER_DEAD_2]);
         printf(msgtxt[M_DRIVER_DEAD_3]);
         return (0);
      }
   }

   Com_(0x0f,0);
   Com_(0x0f,handshake_mask);

   return (1);
}

char *BadChars = "\026\021\b\377\376\375\374\x1b";

void MNP_Filter ()
{
   long t, t1;
   int c;

   t1 = timerset (1000);   /* 10 second drop dead timer */
   t = timerset (100);      /* at most a one second delay */
   while (!timeup (t))
      {
      if (timeup (t1))
         break;

      if ((c = PEEKBYTE ()) != -1)
         {
         (void) TIMED_READ(0);

         /* If we get an MNP or v.42 character, eat it and wait for clear line */
         if ((c != 0) && ((strchr (BadChars, c) != NULL) || (strchr (BadChars, c&0x7f) != NULL)))
            {
            t = timerset (50);
            status_line (msgtxt[M_FILTER]);
            while (!timeup (t))
               {
               if (timeup (t1))
                  break;

               if ((c = PEEKBYTE ()) != -1)
                  {
                  (void) TIMED_READ (0);
                  t = timerset (50);
                  }
               }
            }
         }
      }
}

void FLUSH_OUTPUT ()
{
   while (CARRIER && !OUT_EMPTY())
      time_release();
}

static void empty_delay ()
{
   while (!OUT_EMPTY())
      time_release();
}
