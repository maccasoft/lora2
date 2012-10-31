
// LoraBBS Version 2.41 Free Edition
// Copyright (C) 1987-98 Marco Maccaferri
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <dos.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef __OS2__
#define INCL_DOS
#define INCL_DOSDEVIOCTL
#define ASYNC_SETBAUDRATE_XT                  0x0043
#define ASYNC_GETBAUDRATE_XT                  0x0063
#include <os2.h>
#include <conio.h>
#endif

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

int socket = 0;
short tcpip = 0;

static void empty_delay (void);
extern int blanked, to_row;
extern long timeout;

#ifdef __OS2__
#define ioc(x,y)       ((x<<8)|y)

#define FIONBIO         ioc('f', 126)
#define FIONREAD        ioc('f', 127)
#define MSG_PEEK        0x2

struct in_addr {
	unsigned long  s_addr;
};

typedef struct _XT_BAUD_RATE
{
   unsigned long  current_baud;
   unsigned char  current_fract;
   unsigned long  min_baud;
   unsigned char  min_fract;
   unsigned long  max_baud;
   unsigned char  max_fract;
} XT_BAUD_RATE;
struct sockaddr_in {
	short          sin_family;
	unsigned short sin_port;
	struct in_addr sin_addr;
	char           sin_zero[8];
};

struct sockaddr {
	unsigned short sa_family;              /* address family */
	char           sa_data[14];            /* up to 14 bytes of direct address */
};

int _pascal sock_init( void );

int __syscall send( int, char *, int, int );
int __syscall recv( int, char *, int, int );
int __syscall ioctl( int, int, char *, int );
int __syscall getpeername( int, struct sockaddr *, int * );
int __syscall soclose( int );

char *tcpip_name (char *a)
{
#if defined (__OCC__) || defined (__TCPIP__)
	int namelen;
	struct sockaddr_in client;

	namelen = sizeof (client);
	getpeername (socket, (struct sockaddr *)&client, &namelen);
	sprintf (a, "%ld.%ld.%ld.%ld", (client.sin_addr.s_addr & 0xFFL), (client.sin_addr.s_addr & 0xFF00L) >> 8, (client.sin_addr.s_addr & 0xFF0000L) >> 16, (client.sin_addr.s_addr & 0xFF000000L) >> 24);
#endif

	return (a);
}

#else

char *tcpip_name (char *a)
{
	return (a);
}
#endif

void initialize_modem (void)
{
	mdm_sendcmd (init);
	if (init == config->init) {
		mdm_sendcmd (config->init2);
		mdm_sendcmd (config->init3);
	}

	if (config->modem_OK_errorlevel) {
		#define MODEM_STRING_LEN 90
		int c, i=0,mdm_err=0;
		static char stringa[MODEM_STRING_LEN];
		long t1,t2;
		stringa[0]=0;

		t2 = timerset(2000);

		while (!timeup(t2)&&!strstr(strupr(stringa),"OK")&&!mdm_err){

			i=c=0;

			while(c != 0x0D && !mdm_err && i < MODEM_STRING_LEN){
				if (CHAR_AVAIL ())
					c = MODEM_IN ();
				else {
					t1 = timerset (200);
					while (!CHAR_AVAIL ()&&!mdm_err) {
						if (timeup (t1)){
							mdm_err=1;
						}

						release_timeslice ();
					}
					if (!mdm_err) c = MODEM_IN();
					else c = 0x0D;
				}

				if (c < 0x20 || c == ' ')
					continue;

				if (c != 0x0D)
					stringa[i++] = c;
				else
					stringa[i++] = 0;

			}

			stringa[i]=0;

			if (!blanked&&strlen(stringa)>1){

				if (emulator) {
					c = stringa[46];
					stringa[46] = '\0';
					scrollbox (15, 11, 18, 69, 1, D_UP);
					prints (18, 12, CYAN|_BLACK, strupr (stringa));
					stringa[46] = c;
				}
				else {
					c = stringa[26];
					stringa[26] = '\0';
					scrollbox (13, 53, 21, 78, 1, D_UP);
					prints (21, 53 + ((26 - strlen (stringa)) / 2), CYAN|_BLACK, strupr (stringa));
					stringa[26] = c;
				}
			}
		}

		if(!strstr(strupr(stringa),"OK")){
			status_line("!Modem not responding - exit");
			status_line("!exit with errorlevel %d",config->modem_OK_errorlevel);
			local_mode=1;
			get_down (config->modem_OK_errorlevel, 0);
		}
	}
}

int wait_for_connect(to)
int to;
{
	int i;
	long t1, brate;

	if (config->modem_timeout)
		t1 = timerset (config->modem_timeout * 100);
	else
		t1 = timerset (6000);
	if (!to) {
		timeout = t1;
		to_row = 8;
	}
	brate = 0L;

	do {
		if (!to && local_kbd == ' ') {
			local_kbd = -1;
			if (!to) {
				timeout = 0L;
				return (ABORTED);
			}
			else
				return (0);
		}

		if (!to && local_kbd != -1) {
			if (local_kbd == 0x1B) {
				timeout = 0L;
				local_kbd = -1;
				return (ABORTED);
			}

			local_kbd = -1;
		}

		if ((i = modem_response()) <= 0) {
			time_release();
			release_timeslice ();
			if (to)
				return (-1);
		}
		else {
			switch(i) {
			case 1:
				brate = 300L;
				break;
			case 5:
				brate = 1200L;
				break;
			case 11:
				brate = 2400L;
				break;
			case 18:
				brate = 4800L;
				break;
			case 16:
				brate = 7200L;
				break;
			case 12:
				brate = 9600L;
				break;
			case 17:
				brate = 12000L;
				break;
			case 15:
				brate = 14400L;
				break;
			case 19:
				brate = 16800L;
				break;
			case 13:
				brate = 19200L;
				break;
			case 14:
				brate = 38400L;
				break;
			case 20:
				brate = 57600L;
				break;
			case 21:
				brate = 21600L;
				break;
			case 22:
				brate = 28800L;
				break;
			case 23:
				brate = 38000L;
				break;
			case 24:
				brate = 56000L;
				break;
			case 25:
				brate = 64000L;
				break;
			case 26:
				brate = 128000L;
				break;
			case 27:
				brate = 256000L;
				break;
			case 28:
				brate = 24000L;
				break;
			case 29:
				brate = 26400L;
				break;
			case 30:
				brate = 31200L;
				break;
			case 31:
				brate = 33600L;
				break;
			case 40:
				brate = config->speed;
				break;
			case 50:
				brate = -1;
				break;
         }

         if (brate) {
            rate = brate;
            if (brate != -1) {
               if (!lock_baud)
                  com_baud(brate);
					MNP_Filter();
            }
            timeout = 0L;
            return (1);
         }
         else if (!to) {
            timeout = 0L;
            return (-i);
         }
         else
            return (0);
      }
   } while (!timeup(t1));

   timeout = 0L;

   if (!to)
		return (TIMEDOUT);
   else
      return (0);
}

int modem_response()
{
   #define MODEM_STRING_LEN 90
   int c, i=0;
   static char stringa[MODEM_STRING_LEN];
   long t1;
   int hours, mins, spcount = 0;

   mdm_flags = NULL;
	if (!CHAR_AVAIL ())
      return(-1);

	if (terminal)
		return (terminal_response ());

	for (;;) {
		if (CHAR_AVAIL ())
			c = MODEM_IN ();
		else {
			t1 = timerset (100);
			while (!CHAR_AVAIL ()) {
				if (timeup (t1))
					return (-1);
				release_timeslice ();
			}
			c = MODEM_IN();
		}

		if (c == 0x0D)
			break;

		if (c < 0x20)
			continue;

		if (c == ' ' && i > 0 && stringa[i - 1] != ' ')
			spcount++;

		if ((c == '/' || c == '\\') && mdm_flags == NULL) {
			c = '\0';
			mdm_flags = (char *)&stringa[i+1];
		}
		else if (c == ' ' && spcount >= 2 && mdm_flags == NULL) {
			c = '\0';
			mdm_flags = (char *)&stringa[i+1];
		}
		else if (spcount == 1 && !isdigit (c))
			spcount = 0;

		stringa[i++] = c;

		if (i >= MODEM_STRING_LEN)
			break;
	}

	stringa[i] = '\0';
	if (i < 2)
		return (-1);

	if (mdm_flags)
		fancy_str (mdm_flags);

	if (!blanked || !strnicmp (stringa, "CONNECT", 7) || !strnicmp(stringa,"RING",4)) {
		resume_blanked_screen ();

		if (emulator) {
			c = stringa[46];
			stringa[46] = '\0';
			scrollbox (15, 11, 18, 69, 1, D_UP);
			prints (18, 12, CYAN|_BLACK, strupr (stringa));
			stringa[46] = c;
		}
		else {
			c = stringa[26];
			stringa[26] = '\0';
			scrollbox (13, 53, 21, 78, 1, D_UP);
			prints (21, 53 + ((26 - strlen (stringa)) / 2), CYAN|_BLACK, strupr (stringa));
			stringa[26] = c;
		}
	}

	strupr (stringa);
	if (config->fax_response[0] && !strnicmp (stringa, config->fax_response, strlen (config->fax_response)))
		return (50);

	if (!strnicmp (stringa, "CONNECT", 7) || !strnicmp (stringa, "CARRIER", 7)) {
		if(!stricmp (&stringa[7]," 300") || stringa[7] == '\0')
			return (1);
		else if(!stricmp (&stringa[7]," 1200"))
			return (5);
		else if(!stricmp (&stringa[7]," 1275") || !stricmp (&stringa[7], " 1200TX"))
			return (10);
		else if(!stricmp (&stringa[7]," 24000"))
			return (28);
		else if(!stricmp (&stringa[7]," 2400"))
			return (11);
		else if(!stricmp (&stringa[7]," 9600"))
			return (12);
		else if(!stricmp (&stringa[7]," FAST"))
			return (12);
		else if(!stricmp (&stringa[7]," 7200"))
			return (16);
		else if(!stricmp (&stringa[7]," 4800"))
			return (18);
		else if(!stricmp (&stringa[7]," 12000"))
			return (17);
		else if(!stricmp (&stringa[7]," 14400"))
			return (15);
		else if(!stricmp (&stringa[7]," 16800"))
			return (19);
		else if(!stricmp (&stringa[7]," 19200"))
			return (13);
		else if(!stricmp (&stringa[7]," 21600"))
			return (21);
		else if(!stricmp (&stringa[7]," 26400"))
			return (29);
		else if(!stricmp (&stringa[7]," 28800"))
			return (22);
		else if(!stricmp (&stringa[7]," 31200"))
			return (30);
		else if(!stricmp(&stringa[7]," 33600"))
			return (31);
		else if(!stricmp (&stringa[7]," 38000"))
			return (23);
		else if(!stricmp (&stringa[7]," 56000"))
			return (24);
		else if(!stricmp (&stringa[7]," 64000"))
			return (25);
		else if(!stricmp(&stringa[7]," 38400"))
			return (14);
		else if(!stricmp(&stringa[7]," 57600"))
			return (20);
		else if(!stricmp (&stringa[7]," 128000"))
			return (26);
		else if(!stricmp (&stringa[7]," 256000"))
			return (27);
		else if(!stricmp(&stringa[7]," FAX"))
			return (50);
		else
			return (40);
	}
	else if(!stricmp(stringa,"OK"))
		return(0);
	else if(!stricmp(stringa,"NO CARRIER")||!stricmp(stringa,"NO CARRIER???")||!stricmp(stringa,"NO ANSWEROK???")) {
		status_line(":%s", fancy_str(stringa));
		answer_flag = 0;
		return(3);
	}
	else if(!stricmp(stringa,"NO DIALTONE")||!stricmp(stringa,"NO DIAL TONE"))
		return(6);

	else if(!stricmp(stringa,"BUSY")||!stricmp(stringa,"BUSY???")||!stricmp(stringa,"NO ANSWER???")) {
		status_line(":%s", fancy_str(stringa));
		return(7);
	}
	else if(!stricmp(stringa,"NO ANSWER")) {
		status_line(":%s", fancy_str(stringa));
		answer_flag=0;
		return(8);
	}
	else if(!stricmp(stringa,"VOICE")) {
		status_line(":%s", fancy_str(stringa));
		answer_flag=0;
		return(9);
	}
	else if(!strnicmp(stringa,"RING",4)) {
//      if (config->manual_answer && !answer_flag && registered != 2) {
		if (config->manual_answer && !answer_flag) {
			status_line(":%s", fancy_str(stringa));
			if (config->limited_hours) {
				dostime (&hours, &mins, &i, &i);
				i = hours * 60 + mins;
				if (config->start_time > config->end_time) {
					if (i > config->end_time && i < config->start_time)
						return (-1);
				}
				else {
					if (i < config->start_time || i > config->end_time)
						return (-1);
				}
			}
			mdm_sendcmd( (config->answer[0] == '\0') ? "ATA|" : config->answer);
			answer_flag=1;
		}
		else
			status_line(":%s", fancy_str(stringa));
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
	case 16800:
		return (19);
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

	SET_DTR_ON ();
	if (value == NULL)
		return;

	for (i = 0; value[i]; i++)
		switch (value[i]) {
			case '|':
				SENDBYTE ('\r');
				timer (5);
				break;
			case '~':                           /* Long delay                */
				empty_delay ();                  /* wait for buffer to clear  */
            timer (10);                      /* then wait 1 second        */
            break;
         case 'v':                           /* Lower DTR                 */
				empty_delay ();                  /* wait for buffer to clear, */
            SET_DTR_OFF ();                  /* Turn off DTR              */
            timer (5);
            break;
         case '^':                           /* Raise DTR                 */
				empty_delay ();                  /* wait for buffer to clear  */
            SET_DTR_ON ();                   /* Turn on DTR               */
            timer (5);
            break;
         case '`':                           /* Short delay               */
            timer (1);                       /* short pause, .1 second    */
            break;
         default:
            if (config->stripdash && value[i] == '-')
               break;
            SENDBYTE (value[i]);
            break;
      }
}

void modem_hangup (void)
{
   if (local_mode || tcpip != 0)
      return;

   if (!config->hangup_string[0]) {
      SET_DTR_OFF ();
      timer (10);
      SET_DTR_ON ();
   }
   else
      mdm_sendcmd (config->hangup_string);

   if (CARRIER)
      status_line("!Unable to drop carrier");

	CLEAR_INBOUND ();
   timer (5);

   answer_flag = 0;
}

int TIMED_READ(t)
int t;
{
   long t1;
   extern long timerset ();

	if (!CHAR_AVAIL ()) {
      t1 = timerset ((unsigned int) (t * 100));
		while (!CHAR_AVAIL ()) {
         if (timeup (t1))
            return (-1);
         if (!CARRIER)
            return (-1);
         time_release ();
      }
   }
   return ((unsigned int)(MODEM_IN ()) & 0x00ff);
}

int com_install (int com_port)
{
   if (ComInit( (byte)com_port ) != 0x1954) {
      MDM_DISABLE ();
      if (ComInit( (byte)com_port ) != 0x1954)
         return (0);
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
   if (local_mode || tcpip != 0)
      return;

	UNBUFFER_BYTES ();
   while (CARRIER && !OUT_EMPTY())
      time_release();
}

static void empty_delay ()
{
   if (local_mode || tcpip != 0)
      return;

   while (CARRIER && !OUT_EMPTY()) {
      time_release();
      release_timeslice ();
   }
}

#ifdef __OS2__
HFILE hfComHandle = NULLHANDLE;

/* transmitter stuff */
#define TSIZE 512
static unsigned char tBuff[TSIZE];
static int tpos = 0;

/* receiver stuff */
#define RSIZE 1024
static unsigned char rbuf[RSIZE];
static USHORT rpos = 0;
static USHORT Rbytes = 0;

#define DevIOCtl DosDevIOCtl32

USHORT DosDevIOCtl32(PVOID pData, USHORT cbData, PVOID pParms, USHORT cbParms,
                     USHORT usFunction, USHORT usCategory, HFILE hDevice)
{
   ULONG ulParmLengthInOut = cbParms, ulDataLengthInOut = cbData;
   return (USHORT)DosDevIOCtl(hDevice, usCategory, usFunction,
                              pParms, cbParms, &ulParmLengthInOut,
                              pData, cbData, &ulDataLengthInOut);
}

unsigned int ComInit (unsigned char port)
{
   int i;
   char str[30];
   ULONG action;
   MODEMSTATUS ms;
   UINT data;
   DCBINFO dcbCom;

   if (tcpip == 0) {
      sprintf (str, "COM%d", port + 1);

      if (hfComHandle == NULLHANDLE) {
         if (DosOpen ((PSZ) str, &hfComHandle, &action, 0L, FILE_NORMAL, FILE_OPEN, OPEN_ACCESS_READWRITE|OPEN_SHARE_DENYNONE, 0L))
            return (0);
      }

      XON_DISABLE ();
      if (DevIOCtl(&dcbCom,sizeof(DCBINFO),NULL,0,ASYNC_GETDCBINFO, IOCTL_ASYNC, hfComHandle))
         return (0);

      /* turn off IDSR, ODSR */
      if (config->terminal)
         dcbCom.fbCtlHndShake &= ~(MODE_DSR_HANDSHAKE | MODE_DSR_SENSITIVITY);

      /* raise DTR, CTS output flow control */
      if (config->terminal)
         dcbCom.fbCtlHndShake &= ~(MODE_DTR_CONTROL | MODE_CTS_HANDSHAKE);
      else
         dcbCom.fbCtlHndShake |= (MODE_DTR_CONTROL | MODE_CTS_HANDSHAKE);

      /* turn off XON/XOFF flow control, error replacement off, null
       * stripping off, break replacement off */
      dcbCom.fbFlowReplace &= ~(MODE_AUTO_TRANSMIT | MODE_AUTO_RECEIVE |
                                MODE_ERROR_CHAR | MODE_NULL_STRIPPING |
                                MODE_BREAK_CHAR);

      /* RTS enable */
      if (config->terminal)
         dcbCom.fbFlowReplace |= MODE_RTS_CONTROL;

/*    dcbCom.fbTimeout |= (MODE_NO_WRITE_TIMEOUT | MODE_NOWAIT_READ_TIMEOUT); */
      dcbCom.fbTimeout |= (MODE_NOWAIT_READ_TIMEOUT);
      dcbCom.fbTimeout &= ~(MODE_NO_WRITE_TIMEOUT);

      dcbCom.usReadTimeout = 100;
      dcbCom.usWriteTimeout = 100;
   
      if (DevIOCtl (NULL, 0, &dcbCom, sizeof (DCBINFO), ASYNC_SETDCBINFO, IOCTL_ASYNC, hfComHandle))
         return (0);

      ms.fbModemOn = RTS_ON;
      ms.fbModemOff = 255;

      if (DevIOCtl (&data, sizeof(data), &ms, sizeof(ms), ASYNC_SETMODEMCTRL, IOCTL_ASYNC, hfComHandle))
         return (0);
   }
#if defined (__OCC__) || defined (__TCPIP__)
   else {
      sock_init ();

      i = 1;
      ioctl (socket, FIONBIO, (char *)&i, sizeof (int));
      for (;;) {
         if (ioctl (socket, FIONREAD, (char *)&i, sizeof (int *)) != 0)
            break;
         if (i == 0)
            break;
         recv (socket, rbuf, sizeof (rbuf), 0);
      }
   }
#endif

   return (0x1954);
}

void com_kick(void)
{
   if (tcpip == 0) {
      if (hfComHandle == NULLHANDLE)
         return;

      DevIOCtl (NULL, 0L, NULL, 0L, ASYNC_STARTTRANSMIT, IOCTL_ASYNC, hfComHandle);
   }
}

void MDM_ENABLE (unsigned rate)
{
   com_kick ();
}

void MDM_DISABLE (void)
{
   MODEMSTATUS ms;
   UINT data;

   if (tcpip == 0) {
      if (hfComHandle != NULLHANDLE) {
         ms.fbModemOn = RTS_ON|DTR_ON;
         ms.fbModemOff = 255;
   
         DevIOCtl (&data, sizeof(data), &ms, sizeof(ms), ASYNC_SETMODEMCTRL, IOCTL_ASYNC, hfComHandle);

         DosClose (hfComHandle);
         hfComHandle = NULLHANDLE;
      }
   }
#if defined (__OCC__) || defined (__TCPIP__)
   else
      soclose (socket);
#endif
}

int com_out_empty (void)
{
   RXQUEUE q;

   if (tcpip == 0) {
      DevIOCtl (&q, sizeof (RXQUEUE), NULL, 0, ASYNC_GETINQUECOUNT, IOCTL_ASYNC, hfComHandle);

      if (q.cch == 0)
         return (1);
      else
         return (0);
   }
	else
		return (1);
}

void UNBUFFER_BYTES (void)
{
	ULONG written, pos;

	if (tcpip == 0) {
		if (hfComHandle == NULLHANDLE)
			return;

		if (tpos) {
			pos = 0;
			do {
				DosWrite (hfComHandle, (PVOID)&tBuff[pos], (long)tpos, &written);
				if (written < tpos) {
					pos += written;
					tpos -= written;
					DosSleep (1L);
				}
			} while (written < tpos);
			tpos = 0;
		}
	}
#if defined (__OCC__) || defined (__TCPIP__)
	else {
//      fwrite (tBuff, tpos, 1, stdout);
		send (socket, tBuff, tpos, 0);
		tpos = 0;
	}
#endif
}

void BUFFER_BYTE (unsigned char ch)
{
	if (tcpip == 0) {
		if (hfComHandle == NULLHANDLE)
			return;
	}

	if (tpos >= TSIZE)
		UNBUFFER_BYTES ();

	tBuff[tpos++] = ch;
}

void do_break (int a)
{
}

int PEEKBYTE (void)
{
	int i;
	ULONG bytesRead;
	RXQUEUE q;

	if (tcpip == 0) {
		if (hfComHandle == NULLHANDLE)
			return (-1);

		UNBUFFER_BYTES ();

		if (rpos >= Rbytes) {
			rpos = Rbytes = 0;

			if (DevIOCtl (&q, sizeof (RXQUEUE), NULL, 0, ASYNC_GETINQUECOUNT, IOCTL_ASYNC, hfComHandle))
				return (-1);

			// if there is a byte or more waiting, then read one.
			if (q.cch > 0) {
				if (q.cch > RSIZE){
//					status_line(">DEBUG: q.cch>RSIZE");
					q.cch = RSIZE;
				}
				DosRead (hfComHandle, (PVOID)rbuf, (long)q.cch, &bytesRead);
				Rbytes += bytesRead;
			}
		}

		if (rpos < Rbytes)
			return ((unsigned int)rbuf[rpos]);
		else
			return (-1);
	}
#if defined (__OCC__) || defined (__TCPIP__)
	else {
		UNBUFFER_BYTES ();

		if (rpos >= Rbytes) {
			rpos = Rbytes = 0;

			i = 0;
			if (ioctl (socket, FIONREAD, (char *)&i, sizeof (int *)) != 0)
				return (-1);

			// if there is a byte or more waiting, then read one.
			if (i > 0)
				Rbytes += recv (socket, rbuf, sizeof (rbuf), 0);
		}

		if (rpos < Rbytes)
			return ((unsigned int)rbuf[rpos]);
		else
			return (-1);
	}
#endif
}

static int check_byte_in (void)
{
	int i;
	ULONG bytesRead;
	RXQUEUE q;

	if (tcpip == 0) {
		if (hfComHandle == NULLHANDLE)
			return (-1);

		if (rpos >= Rbytes) {
			rpos = Rbytes = 0;

			if (DevIOCtl (&q, sizeof (RXQUEUE), NULL, 0, ASYNC_GETINQUECOUNT, IOCTL_ASYNC, hfComHandle))
				return (-1);

			// if there is a byte or more waiting, then read one.
			if (q.cch > 0) {
				if (q.cch > RSIZE)
					q.cch = RSIZE;
            DosRead (hfComHandle, (PVOID)rbuf, (long)q.cch, &bytesRead);
				Rbytes += bytesRead;
         }
      }

		if (rpos < Rbytes)
         return ((unsigned int)rbuf[rpos]);
		else
         return (-1);
   }
#if defined (__OCC__) || defined (__TCPIP__)
   else {
		if (rpos >= Rbytes) {
			rpos = Rbytes = 0;

         i = 0;
         if (ioctl (socket, FIONREAD, (char *)&i, sizeof (int *)) != 0)
            return (-1);

         // if there is a byte or more waiting, then read one.
         if (i > 0)
				Rbytes += recv (socket, rbuf, sizeof (rbuf), 0);
      }

		if (rpos < Rbytes)
         return ((unsigned int)rbuf[rpos]);
      else
         return (-1);
   }
#endif
}

void CLEAR_OUTBOUND (void)
{
   UINT data;
   char parm = 0;

   if (tcpip == 0) {
      if (hfComHandle == NULLHANDLE)
         return;

		DevIOCtl (&data, sizeof (data), &parm, sizeof (parm), DEV_FLUSHOUTPUT, IOCTL_GENERAL, hfComHandle);
	}

	tpos = 0;
}

void CLEAR_INBOUND (void)
{
	UINT data;
	char parm = 0;

	if (tcpip == 0) {
		if (hfComHandle == NULLHANDLE)
			return;

		DevIOCtl (&data, sizeof (data), &parm, sizeof (parm), DEV_FLUSHINPUT, IOCTL_GENERAL, hfComHandle);
	}

	rpos = Rbytes = 0;
}

void SENDBYTE (unsigned char c)
{
	ULONG written;

	if (tcpip == 0) {
		if (hfComHandle == NULLHANDLE)
			return;

		UNBUFFER_BYTES ();
		DosWrite (hfComHandle, (PVOID)&c, 1L, &written);
	}
#if defined (__OCC__) || defined (__TCPIP__)
	else {
		UNBUFFER_BYTES ();
		send (socket, (char *)&c, 1, 0);
	}
#endif
}

int MODEM_STATUS (void)
{
	UINT result = 0;
	UCHAR data = 0, socks[32];

	if (tcpip == 0) {
		if (hfComHandle == NULLHANDLE)
			return (0);

		DevIOCtl (&data, sizeof (UCHAR), NULL, 0, ASYNC_GETMODEMINPUT, IOCTL_ASYNC, hfComHandle);
		result = data;

		if (check_byte_in () >= 0)
			result |= DATA_READY;

		return (result);
	}
#if defined (__OCC__) || defined (__TCPIP__)
   else {
      if (recv (socket, (char *)&socks, sizeof (socks), MSG_PEEK) == 0)
         result = 0;
      else
         result = config->carrier_mask;
      if (check_byte_in () >= 0)
         result |= DATA_READY;
      return (result);
   }
#endif
}

int MODEM_IN (void)
{
   int i;
   ULONG bytesRead;
   UINT data = 0;

   if (tcpip == 0) {
      if (hfComHandle == NULLHANDLE)
         return (-1);

		UNBUFFER_BYTES ();

		if (rpos >= Rbytes)
         for (;;) {
				rpos = Rbytes = 0;

            if (DevIOCtl (&data, sizeof (UINT), NULL, 0, ASYNC_GETINQUECOUNT, IOCTL_ASYNC, hfComHandle))
               return (-1);

            // if there is a byte or more waiting, then read one.
				if (data > 0) {
               if (data > RSIZE)
                  data = RSIZE;
               DosRead (hfComHandle, (PVOID)rbuf, (long)data, &bytesRead);
					Rbytes += bytesRead;
               break;
            }
            if (!CARRIER)
               break;
         }

		if (rpos < Rbytes)
         return ((unsigned int)rbuf[rpos++]);
      else
         return (-1);
   }
#if defined (__OCC__) || defined (__TCPIP__)
   else {
		UNBUFFER_BYTES ();

		if (rpos >= Rbytes) {
         do {
				rpos = Rbytes = 0;

            i = 0;
            if (ioctl (socket, FIONREAD, (char *)&i, sizeof (int *)) != 0)
               return (-1);

            // if there is a byte or more waiting, then read one.
            if (i > 0)
					Rbytes += recv (socket, rbuf, sizeof (rbuf), 0);
			} while (Rbytes > 0 || !CARRIER);
      }

		if (rpos < Rbytes)
         return ((unsigned int)rbuf[rpos++]);
      else
         return (-1);
   }
#endif
}

void SET_DTR_OFF (void)
{
   MODEMSTATUS ms;
   UINT data;

   if (tcpip == 0) {
      if (hfComHandle == NULLHANDLE)
         return;

      ms.fbModemOn = RTS_ON;
      ms.fbModemOff = DTR_OFF;
   
      DevIOCtl (&data, sizeof(data), &ms, sizeof(ms), ASYNC_SETMODEMCTRL, IOCTL_ASYNC, hfComHandle);
   }
}                    

void SET_DTR_ON (void)
{
   MODEMSTATUS ms;
   UINT data;

   if (tcpip == 0) {
      if (hfComHandle == NULLHANDLE)
         return;

      ms.fbModemOn = RTS_ON|DTR_ON;
		ms.fbModemOff = 255;
   
      DevIOCtl (&data, sizeof(data), &ms, sizeof(ms), ASYNC_SETMODEMCTRL, IOCTL_ASYNC, hfComHandle);
   }
}                    

unsigned Com_ (char c, ...)
{
   UINT result = 0;
   UCHAR data = 0;

   if (tcpip == 0) {
      if (hfComHandle == NULLHANDLE)
         return (0);

      switch (c) {
         case 0x03:
            DevIOCtl (&data, sizeof (UCHAR), NULL, 0, ASYNC_GETMODEMINPUT, IOCTL_ASYNC, hfComHandle);
            result = data;

            if (!tpos)
               result |= TX_SHIFT_EMPTY;

            if (check_byte_in () >= 0)
               result |= DATA_READY;

            return (result);
      }
   }
   else if (c == 0x03)
      return (MODEM_STATUS ());

   return (0);
}

void XON_ENABLE (void)
{
   DCBINFO dcbCom;

   if (tcpip == 0) {
      if (hfComHandle == NULLHANDLE)
         return;

      if (DevIOCtl(&dcbCom,sizeof(DCBINFO),NULL,0,ASYNC_GETDCBINFO, IOCTL_ASYNC, hfComHandle)) {
         dcbCom.fbFlowReplace |= (MODE_AUTO_TRANSMIT);
         DevIOCtl (NULL, 0, &dcbCom, sizeof (DCBINFO), ASYNC_SETDCBINFO, IOCTL_ASYNC, hfComHandle);
      }
   }
}

void XON_DISABLE (void)
{
   DCBINFO dcbCom;

   if (tcpip == 0) {
      if (hfComHandle == NULLHANDLE)
         return;

      if (DevIOCtl(&dcbCom,sizeof(DCBINFO),NULL,0,ASYNC_GETDCBINFO, IOCTL_ASYNC, hfComHandle)) {
         dcbCom.fbFlowReplace &= ~(MODE_AUTO_TRANSMIT | MODE_AUTO_RECEIVE);
         DevIOCtl (NULL, 0, &dcbCom, sizeof (DCBINFO), ASYNC_SETDCBINFO, IOCTL_ASYNC, hfComHandle);
      }
      com_kick ();
   }
}

void delay (unsigned int t)
{
	long t1;

   t1 = timerset (t / 10);
   while (!timeup (t1))
      DosSleep (10L);
}

void com_baud (long b)
{
   ULONG dataWords = (unsigned long)b;
   LINECONTROL lc;
   XT_BAUD_RATE rbr;

   if (tcpip == 0) {
      if (hfComHandle == NULLHANDLE)
         return;
      DevIOCtl (&rbr, sizeof(XT_BAUD_RATE), NULL, 0, ASYNC_GETBAUDRATE_XT, IOCTL_ASYNC, hfComHandle);
      if(rbr.max_baud<dataWords){
         status_line("!%ld baud rate not allowed by serial device. Set to %ld",dataWords,rbr.max_baud);
         dataWords = rbr.max_baud;
      }
      if(dataWords>57600L)
         DevIOCtl (NULL, 0, &dataWords, sizeof (ULONG), ASYNC_SETBAUDRATE_XT, IOCTL_ASYNC, hfComHandle);
      else
         DevIOCtl (NULL, 0, &dataWords, sizeof (ULONG), ASYNC_SETBAUDRATE, IOCTL_ASYNC, hfComHandle);

      lc.bDataBits = 8;
      lc.bStopBits = 0;
      lc.bParity = 0;

      DevIOCtl (NULL, 0, &lc, sizeof (LINECONTROL), ASYNC_SETLINECTRL, IOCTL_ASYNC, hfComHandle);
   }
}

void set_prior (int pclass)
{
   char *s;
   static USHORT regular = 0;
   static USHORT janus = 0;
   static USHORT modem = 0;
   USHORT priority;

	switch (pclass) {
      case 2:
         if (regular)
            priority = regular;
         else {
            s = getenv ("REGULARPRIORITY");
            if (s)
               priority = regular = atoi (s);
            else
               priority = regular = 2;
         }
         break;

      case 3:
         if (janus)
            priority = janus;
         else {
            s = getenv ("JANUSPRIORITY");
            if (s)
               priority = janus = atoi (s);
            else
               priority = janus = 3;
         }
         break;

      case 4:
         if (modem)
            priority = modem;
         else {
            s = getenv ("MODEMPRIORITY");
            if (s)
               priority = modem = atoi (s);
            else
               priority = modem = 4;
			}
         break;

      default:
         priority = 2;
         break;
   }

   DosSetPrty ((USHORT) 1, priority, (SHORT) 31, (USHORT) 0);
}

#else
#ifdef __NOFOSSIL__

/*
 *	QUEUE routines to allocate - enqueue - dequeue elements to a queue
 */

typedef struct {
   int size;
   int head;
   int tail;
   int avail;
   char *buf;
} QUEUE;

#define queue_empty(qp) (qp)->head==(qp)->tail
#define queue_avail(qp) (qp)->avail

QUEUE *alloc_queue (int size)
{
   QUEUE  *tmp;

   if ((tmp = (QUEUE *) malloc (sizeof (QUEUE) + size)) != (QUEUE *) 0) {
		tmp->size = size;
      tmp->head = 0;
      tmp->tail = 0;
      tmp->avail = size;
      tmp->buf = ((char *) tmp) + sizeof (QUEUE);
   }

   return (tmp);
}

int en_queue (QUEUE *qp, char ch)
{
   int head = qp->head;

   if (qp->avail == 0)
      return (-1);

   *(qp->buf + head) = ch;
   if (++head == qp->size)
      head = 0;

   qp->head = head;
   --(qp->avail);

   return (qp->avail);
}

int de_queue (QUEUE *qp)
{
   int tail = qp->tail, ch;

   if (qp->avail == qp->size)
      return (-1);

	ch = *(qp->buf + tail);
   if (++tail == qp->size)
      tail = 0;

   qp->tail = tail;
   qp->avail++;

   return (ch & 0xFF);
}

int first_queue (QUEUE *qp)
{
   int tail = qp->tail, ch;

   if (qp->avail == qp->size)
      return (-1);

   ch = *(qp->buf + tail);
   return (ch & 0xFF);
}

#define inbyte(address)          inp(address)
#define outbyte(address, value)  outp(address, value)
#define peekmem(seg,offset, var) var=peek(seg,offset)

#if (!defined (TRUE))
#define TRUE  (1)
#define FALSE (0)
#endif

/*
 *	Intel 8259a interrupt controller addresses and constants
 */

#define INT_cntrl        0x20
#define INT_mask         0x21
#define INT_port_enable  0xEF
#define EOI_word         0x20

/*
 *	Constants used to enable and disable interrupts from the
 *	8250 - they are stored in PORT_command 
 */

#define RX_enable        0x0D /* Enable Rcv & RLS Interrupt       */
#define TX_enable        0x0E /* Enable Xmit & RLS Interrupt      */
#define RX_TX_enable     0x0F /* Enable Rcv, Xmit & RLS Interrupt */
#define ERROR_reset      0xF1
#define LCR_DLAB         0x80

/*
 *	8250 register offsets - should be added to value in PORT_address
 */

#define IER              1  /* interrupt enable offset */
#define IIR              2  /* interupt identification register */
#define FCR              2  /* FIFO control register */
#define LCR              3  /* line control register */
#define MCR              4  /* modem control register */
#define LSR              5  /* line status register */
#define MSR              6  /* modem status register */

/*
 *	The following constants are primarily for documentaiton purposes
 *	to show what the values sent to the 8250 Control Registers do to
 *	the chip.
 *
 *
 *                     INTERRUPT ENABLE REGISTER
 */

#define IER_Received_Data      1
#define IER_Xmt_Hld_Reg_Empty  (1<<1)
#define IER_Recv_Line_Status   (1<<2)
#define IER_Modem_Status       (1<<3)
#define IER_Not_Used           0xF0

/*
 *                 INTERRUPT IDENTIFICATION REGISTER
 */

#define IIR_receive      4
#define IIR_transmit     2
#define IIR_mstatus      0
#define IIR_rls          6
#define IIR_complete     1
#define IIR_Not_Used     0xF8

/*
 *                       LINE CONTROL REGISTER
 */

#define LCR_Word_Length_Mask     3
#define LCR_Stop_Bits            (1<<2)
#define LCR_Parity_Enable        (1<<3)
#define LCR_Even_Parity          (1<<4)
#define LCR_Stick_Parity         (1<<5)
#define LCR_Set_Break            (1<<6)
#define LCR_Divisor_Latch_Access (1<<7) /* Divisor Latch - must be set to 1
                                          * to get to the divisor latches of 
                                          * the baud rate generator - must
                                          * be set to 0 to access the
														* Receiver Buffer Register and
                                          * the Transmit Holding Register
                                          */

/*
 *                    	MODEM CONTROL REGISTER
 */

#define MCR_dtr          1
#define MCR_rts          (1<<1)
#define MCR_Out_1        (1<<2)
#define MCR_Out_2        (1<<3) /* MUST BE ASSERTED TO ENABLE INTERRRUPTS */
#define MCR_Loop_Back    (1<<4)
#define MCR_Not_Used     (7<<1)

/*
 *                        LINE STATUS REGISTER
 */

#define LSR_Data_Ready      1
#define LSR_Overrun_Error   (1<<1)
#define LSR_Parity_Error    (1<<2)
#define LSR_Framing_Error   (1<<3)
#define LSR_Break_Interrupt (1<<4)
#define LSR_THR_Empty       (1<<5)  /* Transmitter Holding Register */
#define LSR_TSR_Empty       (1<<6)  /* Transmitter Shift Register */
#define LSR_Not_Used        (1<<7)

/*
 *                       MODEM STATUS REGISTER
 */

#define MSR_Delta_CTS       1
#define MSR_Delta_DSR       (1<<1)
#define MSR_TERD            (1<<2)  /* Trailing Edge Ring Detect   */
#define MSR_Delta_RLSD      (1<<3)  /* Received Line Signal Detect */
#define MSR_CTS             (1<<4)  /* Clear to Send               */
#define MSR_DSR             (1<<5)  /* Data Set Ready              */
#define MSR_RD              (1<<6)  /* Ring Detect                 */
#define MSR_RLSD            (1<<7)  /* Received Line Signal Detect */

#define FCR_Reset_FIFO      0x00
#define FCR_FIFO_Enable     0xCF

unsigned DTR_PORT_channel;      /* Either 1 or 2 for COM1 or COM2       */
char    dtr8250_SAVE_int_mask,  /* saved interrupt controller mask word */
        dtr8250_IER_save = 0,   /* Saved off Interrupt Enable Register  */
        dtr8250_LCR_save = 0,   /* Saved off Line Control Register      */
        dtr8250_MCR_save = 0,   /* Saved off Modem Control Register     */
        dtr8250_DL_lsb = 0,     /* Saved off Baud Rate LSB              */
        dtr8250_DL_msb = 0;     /* Saved off Baud Rate MSB              */

volatile unsigned DTR_PORT_addr, xmit = 1;
volatile int DTR_PORT_status, RCV_disabled = FALSE, XMIT_disabled = TRUE, dtr8250_MSR_reg = 0, xon_enabled = 0;
volatile QUEUE *dtr8250_inqueue = NULL;
volatile QUEUE *dtr8250_outqueue = NULL;

#define DISABLE_xmit outbyte (DTR_PORT_addr+IER, RX_enable)
#define ENABLE_xmit  outbyte (DTR_PORT_addr+IER, RX_TX_enable)
#define DROPPED_cts  ((dtr8250_MSR_reg&0x10)==0)
#define DROPPED_dsr  ((dtr8250_MSR_reg&0x20)==0)
#define DROPPED_dtr  ((dtr8250_MSR_reg&0x30)!=0x30)
#define ASSERT_dtr   outbyte (DTR_PORT_addr+MCR, inbyte (DTR_PORT_addr+MCR)|9)
#define DROP_dtr     outbyte (DTR_PORT_addr+MCR, inbyte (DTR_PORT_addr+MCR)&0x0A)
#define ASSERT_rts   outbyte (DTR_PORT_addr+MCR, inbyte (DTR_PORT_addr+MCR)|0x0A)
#define DROP_rts     outbyte (DTR_PORT_addr+MCR, inbyte (DTR_PORT_addr+MCR)&9)
#define TSRE_bit     0x40

void (interrupt far * dtr_save_vec) (void);

void interrupt far dtr8250_isr (void)
{
   int ch, count;
   char test_status;

   enable ();
   test_status = inbyte ((DTR_PORT_addr + IIR));

   do {
      switch (test_status) {
         case IIR_rls:
            DTR_PORT_status |= inbyte (DTR_PORT_addr + LSR);
            break;

         case IIR_receive:
            count = 0;
            do {
               ch = inbyte (DTR_PORT_addr);
               if (!RCV_disabled) {
                  if ((en_queue ((QUEUE *)dtr8250_inqueue, ch) < 10)) {
                     RCV_disabled = TRUE;
                     DROP_rts;
                  }
               }
               else if (queue_avail (dtr8250_inqueue) > 20) {
                  RCV_disabled = FALSE;
                  ASSERT_rts;
                  en_queue ((QUEUE *)dtr8250_inqueue, ch);
               }
               if (count++ > 14)
                  break;
            } while (inbyte (DTR_PORT_addr + LSR) & LSR_Data_Ready);
				break;

         case IIR_transmit:
            dtr8250_MSR_reg = inbyte ((DTR_PORT_addr + MSR));
            if (dtr8250_MSR_reg & MSR_CTS) {
               if ((ch = de_queue ((QUEUE *)dtr8250_outqueue)) != -1)
                  outbyte (DTR_PORT_addr, ch);
               else {
                  DISABLE_xmit;
                  xmit = 0;
               }
            }
            break;

         case IIR_mstatus:
            dtr8250_MSR_reg = inbyte (DTR_PORT_addr + MSR);
            if (RCV_disabled && (queue_avail (dtr8250_inqueue) > 20)) {
               RCV_disabled = FALSE;
               ASSERT_rts;
            }
            break;
      }
   } while ((test_status = inbyte (DTR_PORT_addr + IIR)) != IIR_complete);

   disable ();
   outbyte (INT_cntrl, EOI_word);
}

#define DTR8250_STACK_SIZE 1024

int dtr8250_intno;

static int dtr_intmask [] = { 0xef, 0xf7, 0xef, 0xf7 };
static int dtr_intno   [] = { 12, 11, 12, 11 };
static int port_addr   [] = { 0x3F8, 0x2F8, 0x3E8, 0x2E8 };

/*
#define UNKNOWN  0
#define U8250    1
#define U16450   2
#define U16550   3
#define U16550A  4

int uart_type (void)
{
   int temp, p2, p7;
   unsigned char xbyte2, xbyte3;

   p2 = DTR_PORT_addr + 2;
   p7 = DTR_PORT_addr + 7;
   xbyte2 = inportb (p2);
   outportb (p2, 0xC1);
   for (temp = 0; temp < 2; temp++);
   xbyte3 = inportb (p2);
   outportb (p2, xbyte2);

   switch ((xbyte3 & 0xC0) >> 6) {
      case 0:
         xbyte2 = inportb (p7);
         outportb (p7, 0xFA);
         for (temp = 0; temp < 2; temp++);
         if (inportb (p7) == 0xFA) {
            outportb (p7, 0xAF);
            for (temp = 0; temp < 2; temp++);
            if (inportb (p7) == 0xAF) {
               outportb (p7, xbyte2);
               return (U16450);
            }
				else
               return (U8250);
         }
         else
            return (U8250);

      case 1:
         return (UNKNOWN);

      case 2:
         return (U16550);

      case 3:
         return (U16550A);
   }

   return (UNKNOWN);
}
*/

unsigned int ComInit (unsigned char port)
{
   int mask, i;

   DTR_PORT_channel = port + 1;
   dtr8250_inqueue = alloc_queue (4096);
   dtr8250_outqueue = alloc_queue (1024);

   DTR_PORT_addr = port_addr[DTR_PORT_channel - 1];
   mask = dtr_intmask [DTR_PORT_channel - 1];
   dtr8250_SAVE_int_mask = inbyte (INT_mask);
   mask &= dtr8250_SAVE_int_mask;

   dtr8250_intno = dtr_intno [DTR_PORT_channel-1];
   dtr8250_LCR_save = inbyte (DTR_PORT_addr + LCR);

   disable ();

   outbyte (DTR_PORT_addr + LCR, dtr8250_LCR_save | LCR_DLAB);
   dtr8250_MCR_save = inbyte (DTR_PORT_addr + MCR);
   dtr8250_DL_lsb = inbyte (DTR_PORT_addr);
   dtr8250_DL_msb = inbyte (DTR_PORT_addr + 1);
   outbyte (DTR_PORT_addr + LCR, dtr8250_LCR_save & 0x7F);
   dtr8250_IER_save = inbyte (DTR_PORT_addr + IER);

/*
   i = uart_type ();
   if (i == U16550 || i == U16550A) {
      outbyte (DTR_PORT_addr + FCR, FCR_Reset_FIFO);
      for (i = 0; i < 2; i++);
      outbyte (DTR_PORT_addr + FCR, FCR_FIFO_Enable);
   }
*/

   enable ();

   dtr_save_vec = _dos_getvect (dtr8250_intno);
   _dos_setvect (dtr8250_intno, dtr8250_isr);

   outbyte (INT_mask, mask);

   return (0x1954);
}

void dtr8250_write (char ch)
{
   while ((inbyte (DTR_PORT_addr + LSR) & TSRE_bit) == 0)
      ;

   dtr8250_MSR_reg = inbyte ((DTR_PORT_addr + MSR));
   outbyte (DTR_PORT_addr, ch);

   if (RCV_disabled && (queue_avail (dtr8250_inqueue) > 20)) {
      RCV_disabled = FALSE;
      ASSERT_rts;
   }
}

void MDM_ENABLE (unsigned rate)
{
   disable ();

   outbyte (DTR_PORT_addr + IER, RX_TX_enable);
   outbyte (DTR_PORT_addr + MCR, 0x0B);

   inbyte (DTR_PORT_addr + LSR);
   dtr8250_MSR_reg = inbyte ((DTR_PORT_addr + MSR));
   inbyte (DTR_PORT_addr);

   enable ();

   com_baud (rate);
}

void MDM_DISABLE (void)
{
   disable ();
   outbyte (INT_mask, dtr8250_SAVE_int_mask);

   outbyte (DTR_PORT_addr + LCR, LCR_DLAB);
   outbyte (DTR_PORT_addr, dtr8250_DL_lsb);
   outbyte (DTR_PORT_addr + 1, dtr8250_DL_msb);
   outbyte (DTR_PORT_addr + MCR, dtr8250_MCR_save);
   outbyte (DTR_PORT_addr + LCR, 0x7F);
   outbyte (DTR_PORT_addr + IER, dtr8250_IER_save);
   outbyte (DTR_PORT_addr + LCR, dtr8250_LCR_save);

   _dos_setvect (dtr8250_intno, dtr_save_vec);
}

void UNBUFFER_BYTES (void)
{
   disable ();
   ENABLE_xmit;
   xmit = 1;
   enable ();

   while (xmit && CARRIER) {
      if (esc_pressed ())
         break;
   }
}

void BUFFER_BYTE (unsigned char ch)
{
   if (en_queue ((QUEUE *)dtr8250_outqueue, ch) < 10)
		UNBUFFER_BYTES ();
}

void do_break (int a)
{
}

int PEEKBYTE (void)
{
   return (first_queue ((QUEUE *)dtr8250_inqueue));
}                    

void CLEAR_OUTBOUND (void)
{
   disable ();
   while (de_queue ((QUEUE *)dtr8250_outqueue) != -1)
      ;
   enable ();
}

void CLEAR_INBOUND (void)
{
   disable ();
   while (de_queue ((QUEUE *)dtr8250_inqueue) != -1)
      ;
   enable ();
}

void SENDBYTE (unsigned char c)
{
   BUFFER_BYTE (c);

   disable ();
   ENABLE_xmit;
   xmit = 1;
   enable ();
}

int MODEM_STATUS (void)
{
   int result;

   dtr8250_MSR_reg = inbyte (DTR_PORT_addr + MSR);
   result = dtr8250_MSR_reg;
   result &= 0xFF;

   if (queue_empty (dtr8250_outqueue))
      result |= TX_SHIFT_EMPTY;
   if (first_queue ((QUEUE *)dtr8250_inqueue) != -1)
      result |= DATA_READY;

   return (result);
}

int MODEM_IN (void)
{
   int ch;

   while (queue_empty ((QUEUE *)dtr8250_inqueue) && CARRIER)
      time_release ();

   disable ();
   ch = de_queue ((QUEUE *)dtr8250_inqueue);
   enable ();

   if (RCV_disabled && (queue_avail (dtr8250_inqueue) > 20)) {
      RCV_disabled = FALSE;
      ASSERT_rts;
   }

   return (ch);
}

unsigned Com_ (char c, ...)
{
   unsigned f, result;

   switch (c) {
      case 0x03:
         dtr8250_MSR_reg = inbyte (DTR_PORT_addr + MSR);
         result = dtr8250_MSR_reg;
         result &= 0xFF;
         if (queue_empty (dtr8250_outqueue))
            result |= TX_SHIFT_EMPTY;
         if (first_queue ((QUEUE *)dtr8250_inqueue) != -1)
            result |= DATA_READY;
         return (result);

      case 0x06:
         if (f)
            ASSERT_dtr;
         else
            DROP_dtr;
         break;
   }

   return (0);
}

void com_baud (long b)
{
   unsigned baud, data, mode_word, parity, stop;

   baud = (unsigned)b;
   data = 8;
   parity = 0;   // E=3, O=1, N=0
   stop = 1;

   stop = (--stop & 1);
   stop <<= 2;

   if (baud != 11520)
      baud /= 10;
   baud = 11520 / baud;

   parity <<= 3;
   parity &= 0x018;

   data -= 5;
   data &= 3;

   mode_word = data | stop | parity;

   disable ();

   outbyte (DTR_PORT_addr + LCR, inbyte (DTR_PORT_addr + LCR) | LCR_DLAB);
   outbyte (DTR_PORT_addr, baud % 256);
   outbyte (DTR_PORT_addr + 1, baud / 256);
   outbyte (DTR_PORT_addr + LCR, mode_word & 0x7F);
   outbyte (DTR_PORT_addr + IER, RX_enable);
   outbyte (DTR_PORT_addr + MCR, 0x0B);

   inbyte (DTR_PORT_addr + LSR);
   dtr8250_MSR_reg = inbyte ((DTR_PORT_addr + MSR));
   inbyte (DTR_PORT_addr);

   enable ();
}

#else

void com_baud (long b)
{
   int i;
   union REGS inregs, outregs;

   if (b == 0 || local_mode)
      return;

   switch( (unsigned)b ) {
      case 300:
         i = BAUD_300;
         break;
      case 1200:
         i = BAUD_1200;
         break;
      case 2400:
         i = BAUD_2400;
         break;
      case 4800:
         i = BAUD_4800;
         break;
      case 7200:
         i = BAUD_7200;
         break;
      case 9600:
         i = BAUD_9600;
         break;
      case 12000:
         i = BAUD_12000;
         break;
      case 14400:
         i = BAUD_14400;
         break;
      case 16800:
         i = BAUD_16800;
         break;
      case 19200:
         i = BAUD_19200;
         break;
      case 38400U:
         i = BAUD_38400;
         break;
      default:
         return;
	}

	inregs.h.ah = 0x00;
	inregs.h.al = i|NO_PARITY|STOP_1|BITS_8;
	inregs.h.dh = 0x00;
	inregs.h.dl = com_port;
   int86 (0x14, &inregs, &outregs);
}

void set_prior (int pclass)
{
    if (pclass);
}

#endif
#endif

