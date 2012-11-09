
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <dir.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <dos.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "msgapi.h"
#include "bluewave.h"

extern int maxakainfo, msg_parent, msg_child;
extern struct _akainfo *akainfo;
extern char *internet_to;

extern struct _node2name *nametable; // Gestione nomi per aree PRIVATE
extern int nodes_num;

FILE *mopen (char *filename, char *mode);
int mclose (FILE *fp);
int mputs (char *s, FILE *fp);
void mprintf (FILE *fp, char *format, ...);
long mseek (FILE *fp, long position, int offset);
int mread (char *s, int n, int e, FILE *fp);
long memlength (void);
void replace_tearline (FILE *fpd, char *buf);
void add_quote_string (char *str, char *from);
void add_quote_header (FILE *fp, char *from, char *to, char *subj, char *dt, char *tm);
int open_packet (int zone, int net, int node, int point, int ai);
void bluewave_header (int fdi, long startpos, long numbytes, int msgnum, struct _msg *msgt);

void squish_scan_message_base (area, name, upd)
int area;
char *name, upd;
{
	int i;

	if (sq_ptr != NULL) {
		MsgUnlock (sq_ptr);
		MsgCloseArea (sq_ptr);
	}

	sq_ptr = MsgOpenArea (name, MSGAREA_CRIFNEC, MSGTYPE_SQUISH);

	if(sq_ptr == NULL){
		status_line("! Critical: Unable to open %s",name);
	}

	MsgUnlock (sq_ptr);

	num_msg = (int)MsgGetNumMsg (sq_ptr);
	if (num_msg)
		first_msg = 1;
	else
		first_msg = 0;
	last_msg = num_msg;
	msg_parent = msg_child = 0;

	for (i=0;i<MAXLREAD;i++)
		if (usr.lastread[i].area == area)
			break;
	if (i != MAXLREAD) {
		if (usr.lastread[i].msg_num > last_msg)
			usr.lastread[i].msg_num = last_msg;
		lastread = usr.lastread[i].msg_num;
	}
	else {
		for (i=0;i<MAXDLREAD;i++)
			if (usr.dynlastread[i].area == area)
				break;
		if (i != MAXDLREAD) {
			if (usr.dynlastread[i].msg_num > last_msg)
				usr.dynlastread[i].msg_num = last_msg;
			lastread = usr.dynlastread[i].msg_num;
		}
		else if (upd) {
			lastread = 0;
			for (i=1;i<MAXDLREAD;i++) {
				usr.dynlastread[i-1].area = usr.dynlastread[i].area;
				usr.dynlastread[i-1].msg_num = usr.dynlastread[i].msg_num;
			}

			usr.dynlastread[i-1].area = area;
			usr.dynlastread[i-1].msg_num = 0;
		}
		else
			lastread = 0;
	}

	if (lastread < 0)
		lastread = 0;
}

#ifndef POINT
static void date_conversion (XMSG *xmsg)
{
	sprintf (xmsg->ftsc_date, "%2d %3s %2d %2d:%2d:%2d", xmsg->date_written.date.da, mtext[xmsg->date_written.date.mo - 1], xmsg->date_written.date.yr + 80, xmsg->date_written.time.hh, xmsg->date_written.time.mm, xmsg->date_written.time.ss);
}

int squish_read_message(msg_num, flag, fakenum)
int msg_num, flag, fakenum;
{
	int i, z, m, line, colf=0, xx, rd;
	char c, buff[80], wrp[80], *p, sqbuff[80];
	long fpos;
	MSGH *sq_msgh;
	XMSG xmsg;

	if (sq_ptr == NULL) {
		squish_scan_message_base (sys.msg_num, sys.msg_path, 1);
		if (sq_ptr == NULL)
			return (0);
	}

	if (usr.full_read && !flag)
		return (squish_full_read_message(msg_num, fakenum));

	line = 1;
	msg_fzone = msg_tzone = config->alias[0].zone;
	msg_fpoint = msg_tpoint = 0;

	sq_msgh = MsgOpenMsg (sq_ptr, MOPEN_RW, (dword)msg_num);
	if (sq_msgh == NULL)
		return(0);

   if (MsgReadMsg (sq_msgh, &xmsg, 0L, 0L, NULL, 0, NULL) == -1) {
      MsgCloseMsg (sq_msgh);
      return (0);
   }

   if (!stricmp (xmsg.from,"ARCmail") && !stricmp (xmsg.to, "Sysop")) {
      MsgCloseMsg (sq_msgh);
      return (0);
   }

   if (xmsg.attr & MSGPRIVATE)
      if (stricmp(xmsg.from,usr.name) && stricmp(xmsg.to,usr.name) && stricmp(xmsg.from,usr.handle) && stricmp(xmsg.to,usr.handle) && usr.priv != SYSOP) {
         MsgCloseMsg (sq_msgh);
         return (0);
      }

   if (!flag)
      cls ();

   memset ((char *)&msg, 0, sizeof (struct _msg));
   strcpy (msg.from, xmsg.from);
   strcpy (msg.to, xmsg.to);
   strcpy (msg.subj, xmsg.subj);
   if (!xmsg.ftsc_date[0])
      date_conversion (&xmsg);
   strcpy (msg.date, xmsg.ftsc_date);
   msg_fzone = xmsg.orig.zone;
   msg.orig = xmsg.orig.node;
   msg.orig_net = xmsg.orig.net;
   msg_fpoint = xmsg.orig.point;
   msg_tzone = xmsg.dest.zone;
   msg.dest = xmsg.dest.node;
   msg.dest_net = xmsg.dest.net;
   msg_tpoint = xmsg.dest.point;
   msg.attr = (word)xmsg.attr;
   msg_parent = (int)MsgUidToMsgn (sq_ptr, xmsg.replyto, UID_EXACT);
   msg_child = (int)MsgUidToMsgn (sq_ptr, xmsg.replies[0], UID_EXACT);

   if (flag)
      m_print(bbstxt[B_TWO_CR]);

   change_attr (WHITE|_BLACK);
   m_print (" * %s\n", sys.msg_name);

   change_attr (CYAN|_BLACK);
   line = msg_attrib (&msg, fakenum ? fakenum : msg_num, line, 0);
   m_print(bbstxt[B_ONE_CR]);

   allow_reply = 1;
   i = 0;
   fpos = 0L;

   do {
      rd = (int)MsgReadMsg (sq_msgh, NULL, fpos, 80L, sqbuff, 0, NULL);
      fpos += (long)rd;

      for (xx = 0; xx < rd; xx++) {
         c = sqbuff[xx];
         if ((byte)c == 0x8D || c == 0x0A || c == '\0')
            continue;

         buff[i++]=c;

         if(c == 0x0D) {
            buff[i-1]='\0';

            if (!usr.kludge && (!strncmp (buff, "SEEN-BY", 7) || buff[0] == 0x01)) {
               i = 0;
               continue;
            }

            p = &buff[0];

            if (((strchr (buff, '>') - p) < 6) && (strchr (buff, '>'))) {
               if (!colf) {
                  change_attr (LGREY|BLACK);
                  colf = 1;
               }
            }
            else {
               if (colf) {
                  change_attr (CYAN|BLACK);
                  colf = 0;
               }
            }

            if (!strncmp (buff, msgtxt[M_TEAR_LINE], 4) || !strncmp (buff, msgtxt[M_ORIGIN_LINE], 11))
               m_print ("\026\001\002%s\026\001\007\n", buff);
            else if (buff[0] == 0x01)
               m_print ("\026\001\013%s\n", &buff[1]);
            else if (!strncmp (buff, "SEEN-BY", 7)) {
               m_print ("\026\001\013%s\n", buff);
               change_attr (LGREY|BLACK);
            }
            else
               m_print ("%s\n", buff);

            if (!strncmp (buff, msgtxt[M_ORIGIN_LINE], 11))
               gather_origin_netnode (buff);

            i = 0;

            if (flag == 1)
               line = 1;

            if (!(++line < (usr.len - 1)) && usr.more) {
               line = 1;
               if (!continua ()) {
                  flag = 1;
                  rd = 0;
                  break;
               }
            }
         }
         else {
            if (i < (usr.width - 1))
               continue;

            buff[i] = '\0';
            while (i > 0 && buff[i] != ' ')
               i--;

            m = 0;

            if (i != 0)
               for (z = i + 1; buff[z]; z++)
                  wrp[m++] = buff[z];

            buff[i] = '\0';
            wrp[m] = '\0';

            if (((strchr (buff, '>') - buff) < 6) && (strchr (buff, '>'))) {
               if (!colf) {
                  change_attr (LGREY|_BLACK);
                  colf = 1;
               }
            }
            else {
               if (colf) {
                  change_attr (CYAN|_BLACK);
                  colf = 0;
               }
            }

            if (!strncmp (buff, msgtxt[M_TEAR_LINE],4) || !strncmp (buff, msgtxt[M_ORIGIN_LINE], 11))
               m_print ("\026\001\002%s\026\001\007\n", buff);
            else
               m_print ("%s\n", buff);

            if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
               gather_origin_netnode (buff);

            if (flag == 1)
               line = 1;

            if (!(++line < (usr.len - 1)) && usr.more) {
               line = 1;
               if (!continua ()) {
                  flag = 1;
                  rd = 0;
                  break;
               }
            }

            strcpy (buff, wrp);
            i = strlen(wrp);
         }
      }
   } while (rd == 80);

   if ((!stricmp(xmsg.to, usr.name) || !stricmp(xmsg.to, usr.handle)) && !(xmsg.attr & MSGREAD)) {
      xmsg.attr |= MSGREAD;
      MsgWriteMsg (sq_msgh, 1, &xmsg, NULL, 0, 0, 0, NULL);
   }

   MsgCloseMsg (sq_msgh);

   if (line > 1 && !flag && usr.more)
      press_enter();

   return(1);
}

int squish_full_read_message(msg_num, fakenum)
int msg_num, fakenum;
{
   int i, z, m, line, colf=0, xx, rd;
   char c, buff[80], wrp[80], *p, sqbuff[80];
   long fpos;
   MSGH *sq_msgh;
   XMSG xmsg;

   line = 2;
   msg_fzone = msg_tzone = config->alias[0].zone;
   msg_fpoint = msg_tpoint = 0;

   sq_msgh = MsgOpenMsg (sq_ptr, MOPEN_RW, (dword)msg_num);
   if (sq_msgh == NULL)
      return(0);

   if (MsgReadMsg (sq_msgh, &xmsg, 0L, 0L, NULL, 0, NULL) == -1) {
      MsgCloseMsg (sq_msgh);
      return (0);
   }

   if (!stricmp (xmsg.from,"ARCmail") && !stricmp (xmsg.to, "Sysop")) {
      MsgCloseMsg (sq_msgh);
      return (0);
   }

   if (xmsg.attr & MSGPRIVATE)
      if (stricmp(xmsg.from,usr.name) && stricmp(xmsg.to,usr.name) && stricmp(xmsg.from,usr.handle) && stricmp(xmsg.to,usr.handle) && usr.priv != SYSOP) {
         MsgCloseMsg (sq_msgh);
         return (0);
      }

   memset ((char *)&msg, 0, sizeof (struct _msg));
   strcpy (msg.from, xmsg.from);
   strcpy (msg.to, xmsg.to);
   strcpy (msg.subj, xmsg.subj);
   if (!xmsg.ftsc_date[0])
      date_conversion (&xmsg);
   strcpy (msg.date, xmsg.ftsc_date);
   msg_fzone = xmsg.orig.zone;
   msg.orig = xmsg.orig.node;
   msg.orig_net = xmsg.orig.net;
   msg_fpoint = xmsg.orig.point;
   msg_tzone = xmsg.dest.zone;
   msg.dest = xmsg.dest.node;
   msg.dest_net = xmsg.dest.net;
   msg_tpoint = xmsg.dest.point;
   msg.attr = (word)xmsg.attr;
   msg_parent = (int)MsgUidToMsgn (sq_ptr, xmsg.replyto, UID_EXACT);
   msg_child = (int)MsgUidToMsgn (sq_ptr, xmsg.replies[0], UID_EXACT);

   cls ();
   change_attr (BLUE|_LGREY);
   del_line ();
   m_print (" * %s\n", sys.msg_name);
   change_attr (CYAN|_BLACK);
   line = msg_attrib (&msg, fakenum ? fakenum : msg_num, line, 0);
   change_attr (LGREY|_BLUE);
   del_line ();
   change_attr (CYAN|_BLACK);
   m_print(bbstxt[B_ONE_CR]);

   allow_reply = 1;
   i = 0;
   fpos = 0L;

   do {
      rd = (int)MsgReadMsg (sq_msgh, NULL, fpos, 80L, sqbuff, 0, NULL);
      fpos += (long)rd;

      for (xx = 0; xx < rd; xx++) {
         c = sqbuff[xx];
         if ((byte)c == 0x8D || c == 0x0A || c == '\0')
            continue;

         buff[i++]=c;

         if(c == 0x0D) {
            buff[i-1]='\0';

            if (!usr.kludge && (!strncmp (buff, "SEEN-BY", 7) || buff[0] == 0x01)) {
               i = 0;
               continue;
            }

            p = &buff[0];

            if (((strchr (buff, '>') - p) < 6) && (strchr (buff, '>'))) {
               if (!colf) {
                  change_attr (LGREY|BLACK);
                  colf = 1;
               }
            }
            else {
               if (colf) {
                  change_attr (CYAN|BLACK);
                  colf = 0;
               }
            }

            if (!strncmp (buff, msgtxt[M_TEAR_LINE], 4) || !strncmp (buff, msgtxt[M_ORIGIN_LINE], 11))
               m_print ("\026\001\002%s\026\001\007\n", buff);
            else if (buff[0] == 0x01)
               m_print ("\026\001\013%s\n", &buff[1]);
            else if (!strncmp (buff, "SEEN-BY", 7)) {
               m_print ("\026\001\013%s\n", buff);
               change_attr (LGREY|BLACK);
            }
            else
               m_print ("%s\n", buff);

            if (!strncmp (buff, msgtxt[M_ORIGIN_LINE], 11))
               gather_origin_netnode (buff);

            i = 0;

            if (!(++line < (usr.len - 1)) && usr.more) {
               if (!continua ()) {
                  line = 1;
                  rd = 0;
                  break;
               }
               else
                  while (line > 5) {
                     cpos (line--, 1);
                     del_line ();
                  }
            }
         }
         else {
            if (i < (usr.width - 1))
               continue;

            buff[i] = '\0';
            while (i > 0 && buff[i] != ' ')
               i--;

            m = 0;

            if (i != 0)
               for (z = i + 1; buff[z]; z++)
                  wrp[m++] = buff[z];

            buff[i] = '\0';
            wrp[m] = '\0';

            if (((strchr (buff, '>') - buff) < 6) && (strchr (buff, '>'))) {
               if (!colf) {
                  change_attr (LGREY|_BLACK);
                  colf = 1;
               }
            }
            else {
               if (colf) {
                  change_attr (CYAN|_BLACK);
                  colf = 0;
               }
            }

            if (!strncmp (buff, msgtxt[M_TEAR_LINE],4) || !strncmp (buff, msgtxt[M_ORIGIN_LINE], 11))
               m_print ("\026\001\002%s\026\001\007\n", buff);
            else
               m_print ("%s\n", buff);

            if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
               gather_origin_netnode (buff);

            if (!(++line < (usr.len - 1)) && usr.more) {
               if (!continua ()) {
                  line = 1;
                  rd = 0;
                  break;
					}
               else
                  while (line > 5) {
                     cpos (line--, 1);
                     del_line ();
                  }
            }

            strcpy (buff, wrp);
            i = strlen(wrp);
         }
      }
   } while (rd == 80);

   if ((!stricmp(xmsg.to, usr.name) || !stricmp(xmsg.to,usr.name)||
   !stricmp(xmsg.to, usr.handle) || !stricmp(xmsg.to, usr.handle)) && !(xmsg.attr & MSGREAD)) {
      xmsg.attr |= MSGREAD;
      MsgWriteMsg (sq_msgh, 1, &xmsg, NULL, 0, 0, 0, NULL);
   }

   MsgCloseMsg (sq_msgh);

   if (line > 1 && usr.more)
		press_enter();

	return(1);
}

void squish_save_message(txt)
char *txt;
{
	int i, dest;
	char origin[128], tear[50], signature[100], pid[40], msgid[40], string[50];
	long totlen;
	unsigned long crc;
	struct date da;
	struct time ti;
	MSGH *sq_msgh;
	XMSG xmsg;

	if (!sq_ptr){
		sq_ptr = MsgOpenArea (sys.msg_path, MSGAREA_CRIFNEC, MSGTYPE_SQUISH);
		if (sq_ptr == NULL)
		return;
	}

	while (MsgLock (sq_ptr) == -1 && msgapierr != MERR_NOMEM)
		;

	m_print(bbstxt[B_SAVE_MESSAGE]);
	dest = (int)MsgGetNumMsg (sq_ptr) + 1;
	activation_key ();
	m_print(" #%d ...",dest);

	sq_msgh = MsgOpenMsg (sq_ptr, MOPEN_CREATE, (dword)0);
	if (sq_msgh == NULL) {
		MsgUnlock (sq_ptr);
		return;
	}

	memset ((char *)&xmsg, 0, sizeof (XMSG));
	strcpy (xmsg.from, msg.from);
	strcpy (xmsg.to, msg.to);
	strcpy (xmsg.subj, msg.subj);
	strcpy (xmsg.ftsc_date, msg.date);
	getdate (&da);
	gettime (&ti);
	xmsg.date_written.date.da = xmsg.date_arrived.date.da = da.da_day;
	xmsg.date_written.date.mo = xmsg.date_arrived.date.mo = da.da_mon;
	xmsg.date_written.date.yr = xmsg.date_arrived.date.yr = (da.da_year % 100) - 80;
	xmsg.date_written.time.hh = xmsg.date_arrived.time.hh = ti.ti_hour;
	xmsg.date_written.time.mm = xmsg.date_arrived.time.mm = ti.ti_min;
	xmsg.date_written.time.ss = xmsg.date_arrived.time.ss = ti.ti_sec;
	xmsg.orig.zone = config->alias[sys.use_alias].zone;
	xmsg.orig.node = config->alias[sys.use_alias].node;
	xmsg.orig.net = config->alias[sys.use_alias].net;
	xmsg.dest.zone = msg_tzone;
	xmsg.dest.node = msg.dest;
	xmsg.dest.net = msg.dest_net;
	xmsg.dest.point = msg_tpoint;
	xmsg.attr = msg.attr;

	totlen = 2L;

	if (strlen(usr.signature) && registered) {
		sprintf (signature, msgtxt[M_SIGNATURE], usr.signature);
		totlen += (long)strlen (signature);
	}

	if (sys.echomail) {
		sprintf (tear, msgtxt[M_TEAR_LINE],VERSION, registered ? "+" : NOREG);
		if (strlen (sys.origin))
			sprintf (origin, msgtxt[M_ORIGIN_LINE], random_origins(), config->alias[sys.use_alias].zone, config->alias[sys.use_alias].net, config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
		else
			sprintf (origin, msgtxt[M_ORIGIN_LINE], system_name, config->alias[sys.use_alias].zone, config->alias[sys.use_alias].net, config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);

		sprintf(pid,msgtxt[M_PID], VERSION, registered ? "" : NOREG);

		crc = time (NULL);
		crc = string_crc(msg.from,crc);
		crc = string_crc(msg.to,crc);
		crc = string_crc(msg.subj,crc);
		crc = string_crc(msg.date,crc);

		sprintf(msgid,msgtxt[M_MSGID], config->alias[sys.use_alias].zone, config->alias[sys.use_alias].net, config->alias[sys.use_alias].node, config->alias[sys.use_alias].point, crc);

		totlen += (long)strlen (tear);
		totlen += (long)strlen (origin);
		totlen += (long)strlen (pid);
		totlen += (long)strlen (msgid);
	}

	if (sys.internet_mail && internet_to != NULL) {
		sprintf (string, "To: %s\r\n\r\n", internet_to);
		free (internet_to);
		totlen += (long)strlen (string);
	}

	if (txt == NULL) {
		i = 0;
		while (messaggio[i] != NULL) {
			totlen += (long)strlen (messaggio[i]);
			i++;
		}

		MsgWriteMsg (sq_msgh, 0, &xmsg, NULL, 0L, totlen, 1L, "\x00");

		if (sys.echomail) {
			MsgWriteMsg (sq_msgh, 1, NULL, pid, strlen (pid), totlen, 0L, NULL);
			MsgWriteMsg (sq_msgh, 1, NULL, msgid, strlen (msgid), totlen, 0L, NULL);
		}

		if (sys.internet_mail)
			MsgWriteMsg (sq_msgh, 1, NULL, string, strlen (string), totlen, 0L, NULL);

		i = 0;
		while (messaggio[i] != NULL) {
			MsgWriteMsg (sq_msgh, 1, NULL, messaggio[i], (long)strlen(messaggio[i]), totlen, 0L, NULL);
			i++;
      }
   }
   else {
      int fptxt, m;
      char buffer[2050];

      fptxt = shopen (txt, O_RDONLY|O_BINARY);
      if (fptxt == -1) {
         MsgUnlock (sq_ptr);
         return;
      }

      MsgWriteMsg (sq_msgh, 0, &xmsg, NULL, 0L, totlen + filelength (fptxt), 1L, "\x00");

      if (sys.echomail) {
         MsgWriteMsg (sq_msgh, 1, NULL, pid, strlen (pid), totlen, 0L, NULL);
         MsgWriteMsg (sq_msgh, 1, NULL, msgid, strlen (msgid), totlen, 0L, NULL);
      }

      do {
         i = read (fptxt, buffer, 2048);
         for (m = 0; m < i; m++) {
            if (buffer[m] == 0x1A)
               buffer[m] = ' ';
         }

         MsgWriteMsg (sq_msgh, 1, NULL, buffer, (long)i, totlen, 0L, NULL);
      } while (i == 2048);

      close(fptxt);
   }

   MsgWriteMsg (sq_msgh, 1, NULL, "\r\n", 2L, totlen, 0L, NULL);

   if (strlen(usr.signature) && registered)
      MsgWriteMsg (sq_msgh, 1, NULL, signature, strlen (signature), totlen, 0L, NULL);

   if (sys.echomail) {
      MsgWriteMsg (sq_msgh, 1, NULL, tear, strlen (tear), totlen, 0L, NULL);
      MsgWriteMsg (sq_msgh, 1, NULL, origin, strlen (origin), totlen, 0L, NULL);
   }

   MsgCloseMsg (sq_msgh);
   MsgUnlock (sq_ptr);

   m_print (bbstxt[B_XPRT_DONE]);
   status_line(msgtxt[M_INSUFFICIENT_DATA], dest, sys.msg_num);
   num_msg = last_msg = dest;
}

int squish_write_message_text (msg_num, flags, quote_name, sm)
int msg_num, flags;
char *quote_name;
FILE *sm;
{
   FILE *fpq;
   int i, z, m, pos, blks, xx, rd;
   char c, buff[80], wrp[80], qwkbuffer[130], sqbuff[80];
   long fpos, qpos;
   struct _msg msgt;
   struct QWKmsghd QWK;
   MSGH *sq_msgh;
   XMSG xmsg;

   msg_fzone = msg_tzone = config->alias[0].zone;
   msg_fpoint = msg_tpoint = 0;

   sq_msgh = MsgOpenMsg (sq_ptr, MOPEN_RW, (dword)msg_num);
   if (sq_msgh == NULL)
      return(0);

   if (MsgReadMsg (sq_msgh, &xmsg, 0L, 0L, NULL, 0, NULL) == -1) {
      MsgCloseMsg (sq_msgh);
      return (0);
   }

   if (!stricmp (xmsg.from,"ARCmail") && !stricmp (xmsg.to, "Sysop")) {
      MsgCloseMsg (sq_msgh);
      return (0);
   }

   if (xmsg.attr & MSGPRIVATE)
      if (stricmp(xmsg.from,usr.name) && stricmp(xmsg.to,usr.name) 
      && stricmp(xmsg.from,usr.handle) && stricmp(xmsg.to,usr.handle) && usr.priv != SYSOP) {
         MsgCloseMsg (sq_msgh);
         return (0);
      }

   memset ((char *)&msgt, 0, sizeof (struct _msg));
   strcpy (msgt.from, xmsg.from);
   strcpy (msgt.to, xmsg.to);
   strcpy (msgt.subj, xmsg.subj);
   if (!xmsg.ftsc_date[0])
      date_conversion (&xmsg);
   strcpy (msgt.date, xmsg.ftsc_date);
   msg_fzone = xmsg.orig.zone;
   msgt.orig = xmsg.orig.node;
   msgt.orig_net = xmsg.orig.net;
   msg_fpoint = xmsg.orig.point;
   msg_tzone = xmsg.dest.zone;
   msgt.dest = xmsg.dest.node;
   msgt.dest_net = xmsg.dest.net;
   msg_tpoint = xmsg.dest.point;
   msg.attr = (word)xmsg.attr;

   blks = 1;
   pos = 0;
   memset (qwkbuffer, ' ', 128);

   if (sm == NULL) {
      fpq = fopen (quote_name, (flags & APPEND_TEXT) ? "at" : "wt");
      if (fpq == NULL) {
         MsgCloseMsg (sq_msgh);
         return (0);
      }
   }
   else
      fpq = sm;

   if (flags & QUOTE_TEXT) {
      add_quote_string (buff, msgt.from);
      i = strlen (buff);
   }
   else
      i = 0;

   fpos = 0L;

   if (flags & INCLUDE_HEADER) {
      text_header (&msgt, msg_num, fpq);
   }
   else if (flags & QWK_TEXTFILE)
      qwk_header (&msgt, &QWK, msg_num, fpq, &qpos);
   else if (flags & QUOTE_TEXT)
      add_quote_header (fpq, msgt.from, msgt.to, msgt.subj, msgt.date, NULL);

   do {
      rd = (int)MsgReadMsg (sq_msgh, NULL, fpos, 80L, sqbuff, 0, NULL);
      fpos += (long)rd;

      for (xx = 0; xx < rd; xx++) {
         c = sqbuff[xx];
         if ((byte)c == 0x8D || c == 0x0A || c == '\0')
            continue;

         buff[i++]=c;

         if (c == 0x0D) {
            buff[i-1] = '\0';
            if (!strnicmp (buff, msgtxt[M_TEAR_LINE], 4))
               buff[1] = '+';
            if (!strnicmp (buff, msgtxt[M_ORIGIN_LINE], 10))
               buff[3] = '0';

            if (strchr (buff, 0x01) != NULL || stristr (buff, "SEEN-BY") != NULL) {
               if (flags & QUOTE_TEXT) {
                  add_quote_string (buff, msgt.from);
                  i = strlen (buff);
               }
               else
                  i = 0;
               continue;
            }

            if (flags & QWK_TEXTFILE) {
               write_qwk_string (buff, qwkbuffer, &pos, &blks, fpq);
               write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpq);
            }
            else
               fprintf (fpq, "%s\n", buff);

            if (flags & QUOTE_TEXT) {
               add_quote_string (buff, msgt.from);
               i = strlen(buff);
            }
            else
               i = 0;
         }
         else {
            if (i < (usr.width - 1))
               continue;

            buff[i] = '\0';
            while (i > 0 && buff[i] != ' ')
               i--;

            m = 0;

            if (i != 0)
               for (z = i + 1; buff[z]; z++)
                  wrp[m++] = buff[z];

            buff[i] = '\0';
            wrp[m] = '\0';
            if (!strnicmp (buff, msgtxt[M_TEAR_LINE], 4))
               buff[1] = '+';
            if (!strnicmp (buff, msgtxt[M_ORIGIN_LINE], 10))
               buff[3] = '0';

            if (flags & QWK_TEXTFILE) {
               write_qwk_string (buff, qwkbuffer, &pos, &blks, fpq);
               write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpq);
            }
            else
               fprintf (fpq, "%s\n", buff);

            if (flags & QUOTE_TEXT)
               add_quote_string (buff, msgt.from);
            else
               buff[0] = '\0';

            strcat (buff, wrp);
            i = strlen (buff);
         }
      }
   } while (rd == 80);

   if ((!stricmp(xmsg.to, usr.name)||!stricmp(xmsg.to, usr.handle)) && !(xmsg.attr & MSGREAD)) {
      xmsg.attr |= MSGREAD;
      MsgWriteMsg (sq_msgh, 1, &xmsg, NULL, 0, 0, 0, NULL);
   }

   MsgCloseMsg (sq_msgh);

   if (flags & QWK_TEXTFILE) {
      qwkbuffer[128] = 0;
      fwrite (qwkbuffer, 128, 1, fpq);
      blks++;

   /* Now update with record count */
      fseek (fpq, qpos, SEEK_SET);          /* Restore back to header start */
      sprintf (buff, "%d", blks);
      ljstring (QWK.Msgrecs, buff, 6);
      fwrite ((char *)&QWK, 128, 1, fpq);           /* Write out the header */
      fseek (fpq, 0L, SEEK_END);               /* Bump back to end of file */

      if (sm == NULL)
         fclose(fpq);

      return (blks);
   }
   else
      fprintf(fpq,bbstxt[B_TWO_CR]);

   if (sm == NULL)
      fclose(fpq);

   return(1);
}

void squish_list_headers (m, verbose)
int m, verbose;
{
	int i, line = verbose ? 2 : 5, l = 0;
	struct _msg msgt;
	MSGH *sq_msgh;
	XMSG xmsg;

	if (!sq_ptr){
		sq_ptr = MsgOpenArea (sys.msg_path, MSGAREA_CRIFNEC, MSGTYPE_SQUISH);
		if (sq_ptr == NULL)
		return;
	}


	for (i = m; i <= last_msg; i++) {
      sq_msgh = MsgOpenMsg (sq_ptr, MOPEN_RW, (dword)i);
      if (sq_msgh == NULL)
         continue;

      if (MsgReadMsg (sq_msgh, &xmsg, 0L, 0L, NULL, 0, NULL) == -1) {
         MsgCloseMsg (sq_msgh);
         continue;
      }

      MsgCloseMsg (sq_msgh);

      if (!stricmp (xmsg.from,"ARCmail") && !stricmp (xmsg.to, "Sysop"))
         continue;

      if (xmsg.attr & MSGPRIVATE)
         if (stricmp(xmsg.from,usr.name) && stricmp(xmsg.to,usr.name) &&
         stricmp(xmsg.from,usr.handle) && stricmp(xmsg.to,usr.handle) && usr.priv != SYSOP)
            continue;

      memset ((char *)&msgt, 0, sizeof (struct _msg));
      strcpy (msgt.from, xmsg.from);
      strcpy (msgt.to, xmsg.to);
      strcpy (msgt.subj, xmsg.subj);
      if (!xmsg.ftsc_date[0])
         date_conversion (&xmsg);
      strcpy (msgt.date, xmsg.ftsc_date);
      msg_fzone = xmsg.orig.zone;
      msgt.orig = xmsg.orig.node;
      msgt.orig_net = xmsg.orig.net;
      msg_fpoint = xmsg.orig.point;
      msg_tzone = xmsg.dest.zone;
      msgt.dest = xmsg.dest.node;
      msgt.dest_net = xmsg.dest.net;
      msg_tpoint = xmsg.dest.point;
      msg.attr = (word)xmsg.attr;

      if (verbose) {
         if ((line = msg_attrib (&msgt, i, line, 0)) == 0)
            break;

         m_print (bbstxt[B_ONE_CR]);
      }
      else
         m_print ("\026\001\003%-4d \026\001\020%c%-20.20s \026\001\020%c%-20.20s \026\001\013%-32.32s\n",
                   i,
                   stricmp (msgt.from, usr.name) ? '\212' : '\216',
                   msgt.from,
                   stricmp (msgt.to, usr.name) ? '\212' : '\214',
                   msgt.to,
                   msgt.subj);

      if (!(l=more_question(line)) || !CARRIER && !RECVD_BREAK())
         break;

      if (!verbose && l < line) {
         l = 5;

         cls ();
         m_print (bbstxt[B_LIST_AREAHEADER], usr.msg, sys.msg_name);
         m_print(bbstxt[B_LIST_HEADER1]);
         m_print(bbstxt[B_LIST_HEADER2]);
      }

      line = l;
   }

   m_print (bbstxt[B_ONE_CR]);

   if (line && CARRIER)
      press_enter();
}

int squish_mail_list_headers (m, line, ovrmsgn)
int m, line, ovrmsgn;
{
   struct _msg msgt;
   MSGH *sq_msgh;
   XMSG xmsg;

   sq_msgh = MsgOpenMsg (sq_ptr, MOPEN_RW, (dword)m);
   if (sq_msgh == NULL)
      return (line);

   if (MsgReadMsg (sq_msgh, &xmsg, 0L, 0L, NULL, 0, NULL) == -1) {
      MsgCloseMsg (sq_msgh);
      return (line);
   }

   MsgCloseMsg (sq_msgh);

   if (!stricmp (xmsg.from,"ARCmail") && !stricmp (xmsg.to, "Sysop"))
      return (line);

   if (xmsg.attr & MSGPRIVATE)
      if (stricmp(xmsg.from,usr.name) && stricmp(xmsg.to,usr.name) &&
      stricmp(xmsg.from,usr.handle) && stricmp(xmsg.to,usr.handle) && usr.priv != SYSOP)
         return (line);

   memset ((char *)&msgt, 0, sizeof (struct _msg));
   strcpy (msgt.from, xmsg.from);
   strcpy (msgt.to, xmsg.to);
   strcpy (msgt.subj, xmsg.subj);
   if (!xmsg.ftsc_date[0])
      date_conversion (&xmsg);
   strcpy (msgt.date, xmsg.ftsc_date);
   msg_fzone = xmsg.orig.zone;
   msgt.orig = xmsg.orig.node;
   msgt.orig_net = xmsg.orig.net;
   msg_fpoint = xmsg.orig.point;
   msg_tzone = xmsg.dest.zone;
   msgt.dest = xmsg.dest.node;
   msgt.dest_net = xmsg.dest.net;
   msg_tpoint = xmsg.dest.point;
   msg.attr = (word)xmsg.attr;

   if ((line = msg_attrib (&msgt, ovrmsgn ? ovrmsgn : m, line, 0)) == 0)
      return (0);

   return (line);
}

void squish_message_inquire (stringa)
char *stringa;
{
   int i, line=4;
   char *p;
   struct _msg msgt, backup;
   MSGH *sq_msgh;
	XMSG xmsg;

	if (!sq_ptr){
		sq_ptr = MsgOpenArea (sys.msg_path, MSGAREA_CRIFNEC, MSGTYPE_SQUISH);
		if (sq_ptr == NULL)
		return;
	}


	for (i = first_msg; i <= last_msg; i++) {
		sq_msgh = MsgOpenMsg (sq_ptr, MOPEN_RW, (dword)i);
		if (sq_msgh == NULL)
			continue;

		if (MsgReadMsg (sq_msgh, &xmsg, 0L, 0L, NULL, 0, NULL) == -1) {
			MsgCloseMsg (sq_msgh);
			continue;
		}

      if (!stricmp (xmsg.from,"ARCmail") && !stricmp (xmsg.to, "Sysop")) {
         MsgCloseMsg (sq_msgh);
         continue;
      }

      if (xmsg.attr & MSGPRIVATE)
         if (stricmp(xmsg.from,usr.name) && stricmp(xmsg.to,usr.name) &&
         stricmp(xmsg.from,usr.handle) && stricmp(xmsg.to,usr.handle) && usr.priv != SYSOP) {
            MsgCloseMsg (sq_msgh);
            continue;
         }

      memset ((char *)&msgt, 0, sizeof (struct _msg));
      strcpy (msgt.from, xmsg.from);
      strcpy (msgt.to, xmsg.to);
      strcpy (msgt.subj, xmsg.subj);
      if (!xmsg.ftsc_date[0])
         date_conversion (&xmsg);
      strcpy (msgt.date, xmsg.ftsc_date);
      msg_fzone = xmsg.orig.zone;
      msgt.orig = xmsg.orig.node;
      msgt.orig_net = xmsg.orig.net;
      msg_fpoint = xmsg.orig.point;
      msg_tzone = xmsg.dest.zone;
      msgt.dest = xmsg.dest.node;
      msgt.dest_net = xmsg.dest.net;
      msg_tpoint = xmsg.dest.point;
      msg.attr = (word)xmsg.attr;

      p = (char *)malloc((unsigned int)MsgGetTextLen (sq_msgh) + 1);
      if (p != NULL) {
         MsgReadMsg (sq_msgh, NULL, 0L, MsgGetTextLen (sq_msgh), p, 0, NULL);
         p[(unsigned int)MsgGetTextLen (sq_msgh)] = '\0';
      }
      else
         p = NULL;

      MsgCloseMsg (sq_msgh);

      memcpy ((char *)&backup, (char *)&msgt, sizeof (struct _msg));
      strupr (msgt.to);
      strupr (msgt.from);
      strupr (msgt.subj);
      strupr (p);

      if ((strstr(msgt.to,stringa) != NULL) || (strstr(msgt.from,stringa) != NULL) ||
          (strstr(msgt.subj,stringa) != NULL) || (strstr(p,stringa) != NULL)) {
         memcpy ((char *)&msgt, (char *)&backup, sizeof (struct _msg));
         if ((line = msg_attrib (&msgt, i, line, 0)) == 0)
            break;

         m_print(bbstxt[B_ONE_CR]);

         if (!(line=more_question(line)) || !CARRIER && !RECVD_BREAK())
            break;

         time_release();
      }

      if (p != NULL)
         free(p);
   }

   if (line && CARRIER)
      press_enter();
}

int squish_kill_message (msg_num)
int msg_num;
{
   MSGH *sq_msgh;
   XMSG xmsg;

   sq_msgh = MsgOpenMsg (sq_ptr, MOPEN_RW, (dword)msg_num);
   if (sq_msgh == NULL)
      return (0);

   if (MsgReadMsg (sq_msgh, &xmsg, 0L, 0L, NULL, 0, NULL) == -1) {
      MsgCloseMsg (sq_msgh);
      return (0);
   }

   MsgCloseMsg (sq_msgh);

   if (!stricmp (xmsg.from, usr.name) || !stricmp (xmsg.to, usr.name) ||
   !stricmp (xmsg.from, usr.handle) || !stricmp (xmsg.to, usr.handle) || usr.priv == SYSOP) {
      MsgKillMsg (sq_ptr, msg_num);
      return (1);
   }

   return (0);
}

int squish_scan_messages (total, personal, start, fdi, fdp, totals, fpq, qwk)
int *total, *personal, start, fdi, fdp, totals;
FILE *fpq;
char qwk;
{
   float in, out;
   int i, z, m, pos, blks, xx, rd, tt, pp, msg_num;
   char c, buff[80], wrp[80], qwkbuffer[130], sqbuff[80];
   long fpos, qpos, bw_start;
   struct _msg msgt;
   struct QWKmsghd QWK;
   MSGH *sq_msgh;
   XMSG xmsg;

   tt = 0;
   pp = 0;

   if (start > last_msg) {
      *personal = pp;
      *total = tt;

      return (totals);
   }

   MsgLock (sq_ptr);

   for (msg_num = start; msg_num <= last_msg; msg_num++) {
      if (!(msg_num % 5))
         display_percentage (msg_num - start, last_msg - start);

      sq_msgh = MsgOpenMsg (sq_ptr, MOPEN_RW, (dword)msg_num);
      if (sq_msgh == NULL)
         continue;

      if (MsgReadMsg (sq_msgh, &xmsg, 0L, 0L, NULL, 0, NULL) == -1) {
         MsgCloseMsg (sq_msgh);
         continue;
      }

      if (!stricmp (xmsg.from,"ARCmail") && !stricmp (xmsg.to, "Sysop")) {
         MsgCloseMsg (sq_msgh);
         continue;
      }

      if (xmsg.attr & MSGPRIVATE)
         if (stricmp(xmsg.from,usr.name) && stricmp(xmsg.to,usr.name) &&
         stricmp(xmsg.from,usr.handle) && stricmp(xmsg.to,usr.handle) && usr.priv != SYSOP) {
            MsgCloseMsg (sq_msgh);
            continue;
         }

      totals++;
      if (qwk == 1 && fdi != -1) {
         sprintf(buff,"%u",totals);   /* Stringized version of current position */
         in = (float) atof(buff);
         out = IEEToMSBIN(in);
         write(fdi,&out,sizeof(float));

         c = 0;
         write(fdi,&c,sizeof(char));              /* Conference # */
      }
      else if (qwk == 2) {
         bw_start = ftell (fpq);
         fputc (' ', fpq);
      }

      if (!stricmp(xmsg.to,usr.name)||!stricmp(xmsg.to,usr.handle)) {
         pp++;
         if (qwk == 1 && fdi != -1 && fdp != -1) {
            write(fdp,&out,sizeof(float));
            write(fdp,&c,sizeof(char));              /* Conference # */
         }
      }

      memset ((char *)&msgt, 0, sizeof (struct _msg));
      strcpy (msgt.from, xmsg.from);
      strcpy (msgt.to, xmsg.to);
      strcpy (msgt.subj, xmsg.subj);
      if (!xmsg.ftsc_date[0])
         date_conversion (&xmsg);
      strcpy (msgt.date, xmsg.ftsc_date);
      msg_fzone = xmsg.orig.zone;
      msgt.orig = xmsg.orig.node;
      msgt.orig_net = xmsg.orig.net;
      msg_fpoint = xmsg.orig.point;
      msg_tzone = xmsg.dest.zone;
      msgt.dest = xmsg.dest.node;
      msgt.dest_net = xmsg.dest.net;
      msg_tpoint = xmsg.dest.point;
      msg.attr = (word)xmsg.attr;

      blks = 1;
      pos = 0;
      memset (qwkbuffer, ' ', 128);

      i = 0;
      fpos = 0L;

      if (qwk == 1)
         qwk_header (&msgt, &QWK, msg_num, fpq, &qpos);
      else if (qwk == 0)
         text_header (&msgt,msg_num,fpq);

      do {
         rd = (int)MsgReadMsg (sq_msgh, NULL, fpos, 80L, sqbuff, 0, NULL);
         fpos += (long)rd;

         for (xx = 0; xx < rd; xx++) {
            c = sqbuff[xx];
            if ((byte)c == 0x8D || c == 0x0A || c == '\0')
               continue;

            buff[i++]=c;

            if(c == 0x0D) {
               buff[i-1]='\0';

               if (!strncmp (buff, "SEEN-BY", 7) || buff[0] == 0x01) {
                  i = 0;
                  continue;
               }

               if (qwk == 1) {
                  write_qwk_string (buff, qwkbuffer, &pos, &blks, fpq);
                  write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpq);
               }
               else
                  fprintf (fpq, "%s\n", buff);

               i = 0;
            }
            else {
               if (i < (usr.width - 1))
                  continue;

               buff[i] = '\0';
               while (i > 0 && buff[i] != ' ')
                  i--;

               m = 0;

               if (i != 0)
                  for (z = i + 1; buff[z]; z++) {
                     wrp[m++] = buff[z];
                     buff[i] = '\0';
                  }

               wrp[m] = '\0';

               if (qwk == 1) {
                  write_qwk_string (buff, qwkbuffer, &pos, &blks, fpq);
                  write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpq);
               }
               else
                  fprintf (fpq, "%s\n", buff);

               strcpy (buff, wrp);
               i = strlen (buff);
            }
         }
      } while (rd == 80);

      if ((!stricmp(xmsg.to, usr.name)||!stricmp(xmsg.to, usr.handle)) && !(xmsg.attr & MSGREAD)) {
         xmsg.attr |= MSGREAD;
         MsgWriteMsg (sq_msgh, 1, &xmsg, NULL, 0, 0, 0, NULL);
      }

      MsgCloseMsg (sq_msgh);

      if (qwk == 1) {
         qwkbuffer[128] = 0;
         fwrite (qwkbuffer, 128, 1, fpq);
         blks++;

      /* Now update with record count */
         fseek (fpq, qpos, SEEK_SET);          /* Restore back to header start */
         sprintf (buff, "%d", blks);
         ljstring (QWK.Msgrecs, buff, 6);
         fwrite ((char *)&QWK, 128, 1, fpq);           /* Write out the header */
         fseek (fpq, 0L, SEEK_END);               /* Bump back to end of file */

         totals += blks - 1;
      }
      else if (qwk == 2)
         bluewave_header (fdi, bw_start, ftell (fpq) - bw_start, msg_num, &msgt);
      else
         fprintf(fpq,bbstxt[B_TWO_CR]);

      tt++;
   }

   MsgUnlock (sq_ptr);

   *personal = pp;
   *total = tt;

   return (totals);
}
#endif

int squish_save_message2 (FILE *txt)
{
	int i, dest, m, dd, hh, mm, ss, aa, mo;
	char buffer[2050];
	long totlen;
	struct date da;
	struct time ti;
	MSGH *sq_msgh;
	XMSG xmsg;

	dest = (int)MsgGetNumMsg (sq_ptr) + 1;

	sq_msgh = MsgOpenMsg (sq_ptr, MOPEN_CREATE, (dword)0);
	if (sq_msgh == NULL)
		return (0);

	memset ((char *)&xmsg, 0, sizeof (XMSG));
	strcpy (xmsg.from, msg.from);
	strcpy (xmsg.to, msg.to);
	strcpy (xmsg.subj, msg.subj);
	strcpy (xmsg.ftsc_date, msg.date);
	sscanf(msg.date, "%2d %3s %2d %2d:%2d:%2d", &dd, buffer, &aa, &hh, &mm, &ss);
	buffer[3] = '\0';
	for (mo = 0; mo < 12; mo++)
		if (!stricmp(buffer, mtext[mo]))
			break;
	if (mo == 12)
		mo = 0;
	xmsg.date_written.date.da = dd;
	xmsg.date_written.date.mo = mo + 1;
	xmsg.date_written.date.yr = (aa % 100) - 80;
	xmsg.date_written.time.hh = hh;
	xmsg.date_written.time.mm = mm;
	xmsg.date_written.time.ss = ss;
	getdate (&da);
	gettime (&ti);
	xmsg.date_arrived.date.da = da.da_day;
	xmsg.date_arrived.date.mo = da.da_mon;
	xmsg.date_arrived.date.yr = (da.da_year % 100) - 80;
	xmsg.date_arrived.time.hh = ti.ti_hour;
	xmsg.date_arrived.time.mm = ti.ti_min;
	xmsg.date_arrived.time.ss = ti.ti_sec;
	xmsg.orig.zone = msg_fzone;
	xmsg.orig.node = msg.orig;
	xmsg.orig.net = msg.orig_net;
	xmsg.dest.zone = msg_tzone;
	xmsg.dest.node = msg.dest;
	xmsg.dest.net = msg.dest_net;
	xmsg.dest.point = msg_tpoint;
	xmsg.attr = msg.attr;
	if (sys.public)
		xmsg.attr &= ~MSGPRIVATE;
	else if (sys.private)
		xmsg.attr |= MSGPRIVATE;
	if (sys.echomail && (msg.attr & MSGSENT)) {
		xmsg.attr |= MSGSCANNED;
		xmsg.attr &= ~MSGSENT;
	}

	totlen = memlength ();

	if (MsgWriteMsg (sq_msgh, 0, &xmsg, NULL, 0L, totlen, 1L, "\x00"))
		return (0);

	do {
		i = mread (buffer, 1, 2048, txt);
		for (m = 0; m < i; m++) {
			if (buffer[m] == 0x1A)
				buffer[m] = ' ';
		}

		if (MsgWriteMsg (sq_msgh, 1, NULL, buffer, (long)i, totlen, 0L, NULL))
			return (0);
	} while (i == 2048);

	MsgCloseMsg (sq_msgh);

	num_msg = last_msg = dest;
	return (1);
}

int squish_export_mail (int maxnodes, struct _fwrd *forward)
{
	FILE *fpd;
	int i, pp, z, ne, no, n_seen, cnet, cnode, mi, msgnum, xx, rd, m, sent, ai;
	char buff[82], wrp[82], c, *p, buffer[2050], sqbuff[82], need_origin;
	char need_seen, *flag;
	long fpos;
	MSGH *sq_msgh;
	XMSG xmsg;
	struct _msghdr2 mhdr;
	struct _fwrd *seen;

	seen = (struct _fwrd *)malloc ((MAX_EXPORT_SEEN + 1) * sizeof (struct _fwrd));
	if (seen == NULL)
		return (maxnodes);

	fpd = fopen ("MSGTMP.EXP", "rb+");
	if (fpd == NULL) {
		fpd = fopen ("MSGTMP.EXP", "wb");
		fclose (fpd);
	}
	else
		fclose (fpd);
	fpd = mopen ("MSGTMP.EXP", "r+b");

	for (i = 0; i < maxnodes; i++)
		forward[i].receiveonly = forward[i].sendonly = forward[i].passive = forward[i].private = forward[i].reset = forward[i].export = 0;

	z = config->alias[sys.use_alias].zone;
	ne = config->alias[sys.use_alias].net;
	no = config->alias[sys.use_alias].node;
	pp = 0;

	p = strtok (sys.forward1, " ");
	if (p != NULL)
		do {
			flag = p;
			while (*p == '<' || *p == '>' || *p == '!' || *p == 'p' || *p == 'P')
				p++;
			parse_netnode2 (p, &z, &ne, &no, &pp);
			for (i = 0; i < MAX_ALIAS; i++) {
				if (config->alias[i].net == 0)
					continue;
				if (config->alias[i].zone == z && config->alias[i].net == ne && config->alias[i].node == no && config->alias[i].point == pp)
					break;
				if (config->alias[i].point && config->alias[i].fakenet) {
					if (config->alias[i].zone == z && config->alias[i].fakenet == ne && config->alias[i].point == no)
						break;
				}
			}
			if (i < MAX_ALIAS && config->alias[i].net)
				continue;
			if ((i = is_here (z, ne, no, pp, forward, maxnodes)) != -1)
				forward[i].reset = forward[i].export = 1;
			else {
				i = maxnodes;
				forward[maxnodes].zone = z;
				forward[maxnodes].net = ne;
				forward[maxnodes].node = no;
				forward[maxnodes].point = pp;
				forward[maxnodes].export = 1;
				forward[maxnodes].reset = 1;
				maxnodes++;
			}
			forward[i].receiveonly = forward[i].sendonly = forward[i].passive = forward[i].private = 0;
			while (*flag=='<'||*flag=='>'||*flag=='!'||*flag=='P'||*flag=='p') {
				if (*flag == '>')
					forward[i].receiveonly = 1;
				if (*flag == '<')
					forward[i].sendonly = 1;
				if (*flag == '!')
					forward[i].passive = 1;
				if (*flag == 'p' || *flag == 'P')
					forward[i].private = 1;
				flag++;
			}
		} while ((p = strtok (NULL, " ")) != NULL);

	z = config->alias[sys.use_alias].zone;
	ne = config->alias[sys.use_alias].net;
	no = config->alias[sys.use_alias].node;
	pp = 0;

	p = strtok (sys.forward2, " ");
	if (p != NULL)
		do {
			flag = p;
			while (*p == '<' || *p == '>' || *p == '!' || *p == 'p' || *p == 'P')
				p++;
			parse_netnode2 (p, &z, &ne, &no, &pp);
			for (i = 0; i < MAX_ALIAS; i++) {
				if (config->alias[i].net == 0)
					continue;
				if (config->alias[i].zone == z && config->alias[i].net == ne && config->alias[i].node == no && config->alias[i].point == pp)
					break;
				if (config->alias[i].point && config->alias[i].fakenet) {
					if (config->alias[i].zone == z && config->alias[i].fakenet == ne && config->alias[i].point == no)
						break;
				}
			}
			if (i < MAX_ALIAS && config->alias[i].net)
				continue;
			if ((i = is_here (z, ne, no, pp, forward, maxnodes)) != -1)
				forward[i].reset = forward[i].export = 1;
			else {
				i = maxnodes;
				forward[maxnodes].zone = z;
				forward[maxnodes].net = ne;
				forward[maxnodes].node = no;
				forward[maxnodes].point = pp;
				forward[maxnodes].export = 1;
				forward[maxnodes].reset = 1;
				maxnodes++;
			}
			forward[i].receiveonly = forward[i].sendonly = forward[i].passive = forward[i].private = 0;
			while (*flag=='<'||*flag=='>'||*flag=='!'||*flag=='P'||*flag=='p') {
				if (*flag == '>')
					forward[i].receiveonly = 1;
				if (*flag == '<')
					forward[i].sendonly = 1;
				if (*flag == '!')
					forward[i].passive = 1;
				if (*flag == 'p' || *flag == 'P')
					forward[i].private = 1;
				flag++;
			}

		} while ((p = strtok (NULL, " ")) != NULL);

	z = config->alias[sys.use_alias].zone;
	ne = config->alias[sys.use_alias].net;
	no = config->alias[sys.use_alias].node;
	pp = 0;

	p = strtok (sys.forward3, " ");
	if (p != NULL)
		do {
			flag = p;
			while (*p == '<' || *p == '>' || *p == '!' || *p == 'p' || *p == 'P')
				p++;
			parse_netnode2 (p, &z, &ne, &no, &pp);
			for (i = 0; i < MAX_ALIAS; i++) {
				if (config->alias[i].net == 0)
					continue;
				if (config->alias[i].zone == z && config->alias[i].net == ne && config->alias[i].node == no && config->alias[i].point == pp)
					break;
				if (config->alias[i].point && config->alias[i].fakenet) {
					if (config->alias[i].zone == z && config->alias[i].fakenet == ne && config->alias[i].point == no)
						break;
				}
			}
			if (i < MAX_ALIAS && config->alias[i].net)
				continue;
			if ((i = is_here (z, ne, no, pp, forward, maxnodes)) != -1)
				forward[i].reset = forward[i].export = 1;
			else {
				i = maxnodes;
				forward[maxnodes].zone = z;
				forward[maxnodes].net = ne;
				forward[maxnodes].node = no;
				forward[maxnodes].point = pp;
				forward[maxnodes].export = 1;
				forward[maxnodes].reset = 1;
				maxnodes++;
			}
			forward[i].receiveonly = forward[i].sendonly = forward[i].passive = forward[i].private = 0;
			while (*flag=='<'||*flag=='>'||*flag=='!'||*flag=='P'||*flag=='p') {
				if (*flag == '>')
					forward[i].receiveonly = 1;
				if (*flag == '<')
					forward[i].sendonly = 1;
				if (*flag == '!')
					forward[i].passive = 1;
				if (*flag == 'p' || *flag == 'P')
					forward[i].private = 1;
				flag++;
			}

		} while ((p = strtok (NULL, " ")) != NULL);

	msgnum = (int)MsgGetHighWater(sq_ptr) + 1;

	strcpy (wrp, sys.echotag);
	wrp[14] = '\0';
	prints (8, 65, YELLOW|_BLACK, "              ");
	prints (8, 65, YELLOW|_BLACK, wrp);

	prints (7, 65, YELLOW|_BLACK, "              ");
	prints (9, 65, YELLOW|_BLACK, "              ");
	prints (9, 65, YELLOW|_BLACK, "Squish<tm>");

	sprintf (wrp, "%d / %d", msgnum, last_msg);
	prints (7, 65, YELLOW|_BLACK, wrp);
	sent = 0;

	for (; msgnum <= last_msg; msgnum++) {
		sprintf (wrp, "%d / %d", msgnum, last_msg);
		prints (7, 65, YELLOW|_BLACK, wrp);

		for (i = 0; i < maxnodes; i++)
			if (forward[i].reset)
				forward[i].export = 1;

		for (i = 0; i < maxnodes; i++) {
			if (forward[i].sendonly||forward[i].passive) forward[i].export=0;
		}


		sq_msgh = MsgOpenMsg (sq_ptr, MOPEN_RW, (dword)msgnum);
		if (sq_msgh == NULL)
			continue;

		if (MsgReadMsg (sq_msgh, &xmsg, 0L, 0L, NULL, 0, NULL) == -1) {
			MsgCloseMsg (sq_msgh);
			continue;
		}

		if (xmsg.attr & MSGSCANNED) {
			MsgCloseMsg (sq_msgh);
			continue;
		}
		else {
			xmsg.attr |= MSGSCANNED;
			MsgWriteMsg (sq_msgh, 1, &xmsg, NULL, 0, 0, 0, NULL);
		}
		for (i=0;i<maxnodes;i++) {
			if (forward[i].private) {

				int ii;

				forward[i].export=0;
				if (!nametable) continue;

				for(ii=0;ii<nodes_num;ii++) {
					if (forward[i].zone==nametable[ii].zone&&
						 forward[i].net==nametable[ii].net&&
						 forward[i].node==nametable[ii].node&&
						 forward[i].point==nametable[ii].point&&
						 !stricmp(xmsg.to,nametable[ii].name)) {
							 forward[i].export=1;
							 break;
					}
				}
			}


		}

		sprintf (wrp, "%5d  %-22.22s Squish<tm>   ", msgnum, sys.echotag);
		wputs (wrp);

		need_seen = need_origin = 1;

		mseek (fpd, 0L, SEEK_SET);
      mprintf (fpd, "AREA:%s\r\n", sys.echotag);

      mi = i = 0;
      pp = 0;
      n_seen = 0;
      fpos = 0L;

      MsgReadMsg (sq_msgh, &xmsg, 0L, 0L, NULL, 1024, buffer);
      if ((p = strtok (buffer, "\x01")) != NULL)
         do {
            mprintf (fpd, "\x01%s\r\n", p);
         } while ((p = strtok (NULL, "\x01")) != NULL);

      do {
         rd = (int)MsgReadMsg (sq_msgh, NULL, fpos, 80L, sqbuff, 0, NULL);
         fpos += (long)rd;

         for (xx = 0; xx < rd; xx++) {
            c = sqbuff[xx];
            if (c == '\0')
               break;

            if (c == 0x0A)
               continue;
            if((byte)c == 0x8D)
               c = ' ';

            buff[mi++] = c;

            if(c == 0x0D) {
               buff[mi - 1]='\0';
               if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
                  need_origin = 0;

               if (!strncmp (buff, "SEEN-BY: ", 9)) {
                  if (need_origin) {
                     mprintf (fpd, msgtxt[M_TEAR_LINE],VERSION, registered ? "+" : NOREG);
                     if (strlen(sys.origin))
                        mprintf(fpd,msgtxt[M_ORIGIN_LINE],random_origins(),config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
                     else
                        mprintf(fpd,msgtxt[M_ORIGIN_LINE],system_name,config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
                     need_origin = 0;
                  }

                  p = strtok (buff, " ");
                  ne = config->alias[sys.use_alias].net;
                  no = config->alias[sys.use_alias].node;
                  pp = config->alias[sys.use_alias].point;
                  z = config->alias[sys.use_alias].zone;

                  while ((p = strtok (NULL, " ")) != NULL) {
                     parse_netnode2 (p, &z, &ne, &no, &pp);

                     seen[n_seen].net = ne;
                     seen[n_seen].node = no;
                     seen[n_seen].point = pp;
                     seen[n_seen].zone = z;

                     for (i = 0; i < maxnodes; i++) {
                        if (forward[i].net == seen[n_seen].net && forward[i].node == seen[n_seen].node && forward[i].point == seen[n_seen].point) {
                           forward[i].export = 0;
                           break;
                        }
                     }
                     
                     if (n_seen + maxnodes >= MAX_EXPORT_SEEN)
                        continue;

                     if (seen[n_seen].net != config->alias[sys.use_alias].fakenet && !seen[n_seen].point)
                        n_seen++;
                  }
               }
               else if (!strncmp (buff, "\001PATH: ", 7) && need_seen) {
                  if (need_origin) {
                     mprintf (fpd, msgtxt[M_TEAR_LINE],VERSION, registered ? "+" : NOREG);
                     if (strlen(sys.origin))
                        mprintf(fpd,msgtxt[M_ORIGIN_LINE],random_origins(),config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
                     else
                        mprintf(fpd,msgtxt[M_ORIGIN_LINE],system_name,config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
                     need_origin = 0;
                  }

                  if (!config->alias[sys.use_alias].point) {
                     seen[n_seen].net = config->alias[sys.use_alias].net;
                     seen[n_seen++].node = config->alias[sys.use_alias].node;
                  }
                  else if (config->alias[sys.use_alias].fakenet) {
                     seen[n_seen].net = config->alias[sys.use_alias].fakenet;
                     seen[n_seen++].node = config->alias[sys.use_alias].point;
                  }

                  for (i = 0; i < maxnodes; i++) {
                     if (!forward[i].export || forward[i].net == config->alias[sys.use_alias].fakenet || forward[i].point)
                        continue;
                     seen[n_seen].net = forward[i].net;
                     seen[n_seen++].node = forward[i].node;

                     ai = sys.use_alias;
                     for (m = 0; m < maxakainfo; m++)
                        if (akainfo[m].zone == forward[i].zone && akainfo[m].net == forward[i].net && akainfo[m].node == forward[i].node && akainfo[m].point == forward[i].point) {
                           if (akainfo[m].aka)
                              ai = akainfo[m].aka - 1;
                           break;
                        }
                     if (ai != sys.use_alias) {
                        if (!config->alias[ai].point) {
                           if (config->alias[ai].net != seen[n_seen - 1].net || config->alias[ai].node != seen[n_seen - 1].node) {
                              seen[n_seen].net = config->alias[ai].net;
                              seen[n_seen++].node = config->alias[ai].node;
                           }
                        }
                        else if (config->alias[ai].fakenet) {
                           if (config->alias[ai].fakenet != seen[n_seen - 1].net || config->alias[ai].point != seen[n_seen - 1].node) {
                              seen[n_seen].net = config->alias[ai].fakenet;
                              seen[n_seen++].node = config->alias[ai].point;
                           }
                        }
                     }
                  }

                  qsort (seen, n_seen, sizeof (struct _fwrd), mail_sort_func);

                  cnet = cnode = 0;
                  strcpy (wrp, "SEEN-BY: ");

                  for (i = 0; i < n_seen; i++) {
                     if (strlen (wrp) > 65) {
                        mprintf (fpd, "%s\r\n", wrp);
                        cnet = cnode = 0;
                        strcpy (wrp, "SEEN-BY: ");
                     }
                     if (i && seen[i].net == seen[i - 1].net && seen[i].node == seen[i - 1].node)
                        continue;

                     if (cnet != seen[i].net) {
                        sprintf (buffer, "%d/%d ", seen[i].net, seen[i].node);
                        cnet = seen[i].net;
                        cnode = seen[i].node;
                     }
                     else if (cnet == seen[i].net && cnode != seen[i].node) {
                        sprintf (buffer, "%d ", seen[i].node);
                        cnode = seen[i].node;
                     }
                     else
                        strcpy (buffer, "");

                     strcat (wrp, buffer);
                  }

                  mprintf (fpd, "%s\r\n", wrp);
                  mprintf (fpd, "%s\r\n", buff);

                  need_seen = 0;
               }
               else if ((xmsg.attr & MSGLOCAL) && !strncmp (buff, "--- ", 4) && (config->replace_tear || !registered))
                  replace_tearline (fpd, buff);
               else
                  mprintf (fpd, "%s\r\n", buff);

               mi = 0;
            }
            else {
               if(mi < 78)
                  continue;

               if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
                  need_origin = 0;

               buff[mi]='\0';
               mprintf(fpd,"%s",buff);
               mi=0;
               buff[mi] = '\0';
            }
         }
      } while (rd == 80);

      if (!n_seen) {
         if (need_origin) {
            mprintf (fpd, msgtxt[M_TEAR_LINE],VERSION, registered ? "+" : NOREG);
            if (strlen(sys.origin))
               mprintf(fpd,msgtxt[M_ORIGIN_LINE],random_origins(),config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
            else
               mprintf(fpd,msgtxt[M_ORIGIN_LINE],system_name,config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
            need_origin = 0;
         }

         if (!config->alias[sys.use_alias].point) {
            seen[n_seen].net = config->alias[sys.use_alias].net;
            seen[n_seen++].node = config->alias[sys.use_alias].node;
         }
         else if (config->alias[sys.use_alias].fakenet) {
            seen[n_seen].net = config->alias[sys.use_alias].fakenet;
            seen[n_seen++].node = config->alias[sys.use_alias].point;
         }

         for (i = 0; i < maxnodes; i++) {
            if (!forward[i].export || forward[i].net == config->alias[sys.use_alias].fakenet)
               continue;
            seen[n_seen].net = forward[i].net;
            seen[n_seen++].node = forward[i].node;

            ai = sys.use_alias;
            for (m = 0; m < maxakainfo; m++)
               if (akainfo[m].zone == forward[i].zone && akainfo[m].net == forward[i].net && akainfo[m].node == forward[i].node && akainfo[m].point == forward[i].point) {
                  if (akainfo[m].aka)
                     ai = akainfo[m].aka - 1;
                  break;
               }
            if (ai != sys.use_alias) {
               if (!config->alias[ai].point) {
                  if (config->alias[ai].net != seen[n_seen - 1].net || config->alias[ai].node != seen[n_seen - 1].node) {
                     seen[n_seen].net = config->alias[ai].net;
                     seen[n_seen++].node = config->alias[ai].node;
                  }
               }
               else if (config->alias[ai].fakenet) {
                  if (config->alias[ai].fakenet != seen[n_seen - 1].net || config->alias[ai].point != seen[n_seen - 1].node) {
                     seen[n_seen].net = config->alias[ai].fakenet;
                     seen[n_seen++].node = config->alias[ai].point;
                  }
               }
            }
         }

         qsort (seen, n_seen, sizeof (struct _fwrd), mail_sort_func);

         cnet = cnode = 0;
         strcpy (wrp, "SEEN-BY: ");

         for (i = 0; i < n_seen; i++) {
            if (cnet != seen[i].net) {
               sprintf (buffer, "%d/%d ", seen[i].net, seen[i].node);
               cnet = seen[i].net;
               cnode = seen[i].node;
            }
            else if (cnet == seen[i].net && cnode != seen[i].node) {
               sprintf (buffer, "%d ", seen[i].node);
               cnode = seen[i].node;
            }
            else
               strcpy (buffer, "");

            strcat (wrp, buffer);
         }

         mprintf (fpd, "%s\r\n", wrp);
         if (config->alias[sys.use_alias].point && config->alias[sys.use_alias].fakenet)
            mprintf (fpd, "\001PATH: %d/%d\r\n", config->alias[sys.use_alias].fakenet, config->alias[sys.use_alias].point);
         else if (!config->alias[sys.use_alias].point)
            mprintf (fpd, "\001PATH: %d/%d\r\n", config->alias[sys.use_alias].net, config->alias[sys.use_alias].node);
      }

      mhdr.ver = PKTVER;
      mhdr.cost = 0;
      mhdr.attrib = 0;

      if (sys.public)
         xmsg.attr &= ~MSGPRIVATE;
      else if (sys.private)
         xmsg.attr |= MSGPRIVATE;

      if (xmsg.attr & MSGPRIVATE)
         mhdr.attrib |= MSGPRIVATE;

      for (i = 0; i < maxnodes; i++) {
         if (!forward[i].export)
            continue;

         ai = sys.use_alias;
         for (m = 0; m < maxakainfo; m++)
            if (akainfo[m].zone == forward[i].zone && akainfo[m].net == forward[i].net && akainfo[m].node == forward[i].node && akainfo[m].point == forward[i].point) {
               if (akainfo[m].aka)
                  ai = akainfo[m].aka - 1;
               break;
            }

         if (config->alias[ai].point && config->alias[ai].fakenet) {
            mhdr.orig_node = config->alias[ai].point;
            mhdr.orig_net = config->alias[ai].fakenet;
         }
         else {
            mhdr.orig_node = config->alias[ai].node;
            mhdr.orig_net = config->alias[ai].net;
         }

         if (forward[i].point)
            sprintf (wrp, "%d/%d.%d ", forward[i].net, forward[i].node, forward[i].point);
         else
            sprintf (wrp, "%d/%d ", forward[i].net, forward[i].node);
         wreadcur (&z, &m);
         if ( (m + strlen (wrp)) > 78) {
            wputs ("\n");
            wputs ("                                           ");
         }
         wputs (wrp);
         if ( (m + strlen (wrp)) == 78)
            wputs ("                                           ");

         mi = open_packet (forward[i].zone, forward[i].net, forward[i].node, forward[i].point, ai);

         mhdr.dest_net = forward[i].net;
         mhdr.dest_node = forward[i].node;
         write (mi, (char *)&mhdr, sizeof (struct _msghdr2));

         write (mi, xmsg.ftsc_date, strlen (xmsg.ftsc_date) + 1);
         write (mi, xmsg.to, strlen (xmsg.to) + 1);
         write (mi, xmsg.from, strlen (xmsg.from) + 1);
         write (mi, xmsg.subj, strlen (xmsg.subj) + 1);
         mseek (fpd, 0L, SEEK_SET);
         do {
            z = mread(buffer, 1, 2048, fpd);
            write(mi, buffer, z);
         } while (z == 2048);
         buff[0] = buff[1] = buff[2] = 0;
         if (write (mi, buff, 3) != 3)
            return (-1);
         close (mi);

         totalmsg++;
         sprintf (wrp, "%d (%.1f/s) ", totalmsg, (float)totalmsg / ((float)(timerset (0) - totaltime) / 100));
         prints (10, 65, YELLOW|_BLACK, wrp);

         time_release ();
      }

      MsgCloseMsg (sq_msgh);

      wputs ("\n");
      sent++;
   }

   MsgSetHighWater(sq_ptr, (dword)last_msg);

   if (sent)
      status_line (":   %-20.20s (Sent=%04d)", sys.echotag, sent);

   mclose (fpd);
   free (seen);
   unlink ("MSGTMP.EXP");

   return (maxnodes);
}

void squish_rescan_echomail (char *tag, int zone, int net, int node, int point)
{
   FILE *fpd;
   int i, pp, z, ne, no, n_seen, cnet, cnode, mi, msgnum, xx, rd, m, sent, fd, ai = 0;
   char buff[80], wrp[80], c, *p, buffer[2050], sqbuff[80], need_origin;
   char need_seen;
   long fpos;
   MSGH *sq_msgh;
   XMSG xmsg;
   struct _msghdr2 mhdr;
   struct _fwrd *seen;
   NODEINFO ni;

   sprintf (buff, "%sNODES.DAT", config->net_info);
   if ((fd = sh_open (buff, SH_DENYWR, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) != -1) {
      while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO))
         if (zone == ni.zone && net == ni.net && node == ni.node && point == ni.point) {
            ai = ni.aka;
            break;
         }

      close (fd);
   }

   strcpy (buff, tag);
   memset ((char *)&sys, 0, sizeof (struct _sys));
   strcpy (sys.echotag, buff);

   seen = (struct _fwrd *)malloc ((MAX_EXPORT_SEEN + 1) * sizeof (struct _fwrd));
   if (seen == NULL)
      return;

   fpd = fopen ("MSGTMP.EXP", "rb+");
   if (fpd == NULL) {
      fpd = fopen ("MSGTMP.EXP", "wb");
      fclose (fpd);
   }
   else
      fclose (fpd);
   fpd = mopen ("MSGTMP.EXP", "r+b");

   msgnum = 1;

   strcpy (wrp, sys.echotag);
   wrp[14] = '\0';
   prints (8, 65, YELLOW|_BLACK, "              ");
   prints (8, 65, YELLOW|_BLACK, wrp);

   prints (7, 65, YELLOW|_BLACK, "              ");
   prints (9, 65, YELLOW|_BLACK, "              ");
   prints (9, 65, YELLOW|_BLACK, "Squish<tm>");

   sprintf (wrp, "%d / %d", msgnum, last_msg);
   prints (7, 65, YELLOW|_BLACK, wrp);
   sent = 0;

   for (; msgnum <= last_msg; msgnum++) {
      sprintf (wrp, "%d / %d", msgnum, last_msg);
      prints (7, 65, YELLOW|_BLACK, wrp);

      if ( !(msgnum % 20))
         time_release ();

      sq_msgh = MsgOpenMsg (sq_ptr, MOPEN_RW, (dword)msgnum);
      if (sq_msgh == NULL)
         continue;

      if (MsgReadMsg (sq_msgh, &xmsg, 0L, 0L, NULL, 0, NULL) == -1) {
         MsgCloseMsg (sq_msgh);
         continue;
      }

      sprintf (wrp, "%5d  %-22.22s Squish<tm>   ", msgnum, sys.echotag);
      wputs (wrp);

      need_seen = need_origin = 1;

      mseek (fpd, 0L, SEEK_SET);
      mprintf (fpd, "AREA:%s\r\n", sys.echotag);

      mi = i = 0;
      pp = 0;
      n_seen = 0;
      fpos = 0L;

      MsgReadMsg (sq_msgh, &xmsg, 0L, 0L, NULL, 1024, buffer);
      if ((p = strtok (buffer, "\x01")) != NULL)
         do {
            mprintf (fpd, "\x01%s\r\n", p);
         } while ((p = strtok (NULL, "\x01")) != NULL);

      do {
         rd = (int)MsgReadMsg (sq_msgh, NULL, fpos, 80L, sqbuff, 0, NULL);
         fpos += (long)rd;

         for (xx = 0; xx < rd; xx++) {
            c = sqbuff[xx];
            if (c == '\0')
               break;

            if (c == 0x0A)
               continue;
            if((byte)c == 0x8D)
               c = ' ';

            buff[mi++] = c;

            if(c == 0x0D) {
               buff[mi - 1]='\0';
               if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
                  need_origin = 0;

               if (!strncmp (buff, "SEEN-BY: ", 9)) {
                  if (need_origin) {
                     mprintf (fpd, msgtxt[M_TEAR_LINE],VERSION, registered ? "+" : NOREG);
                     if (strlen(sys.origin))
                        mprintf(fpd,msgtxt[M_ORIGIN_LINE],random_origins(),config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
                     else
                        mprintf(fpd,msgtxt[M_ORIGIN_LINE],system_name,config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
                     need_origin = 0;
                  }

                  p = strtok (buff, " ");
                  ne = config->alias[sys.use_alias].net;
                  no = config->alias[sys.use_alias].node;
                  pp = config->alias[sys.use_alias].point;
                  z = config->alias[sys.use_alias].zone;

                  while ((p = strtok (NULL, " ")) != NULL) {
                     parse_netnode2 (p, &z, &ne, &no, &pp);

                     seen[n_seen].net = ne;
                     seen[n_seen].node = no;
                     seen[n_seen].point = pp;
                     seen[n_seen].zone = z;

                     if (n_seen >= MAX_EXPORT_SEEN)
                        continue;

                     n_seen++;
                  }
               }
               else if (!strncmp (buff, "\001PATH: ", 7) && need_seen) {
                  if (need_origin) {
                     mprintf (fpd, msgtxt[M_TEAR_LINE],VERSION, registered ? "+" : NOREG);
                     if (strlen(sys.origin))
                        mprintf(fpd,msgtxt[M_ORIGIN_LINE],random_origins(),config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
                     else
                        mprintf(fpd,msgtxt[M_ORIGIN_LINE],system_name,config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
                     need_origin = 0;
                  }

                  if (!config->alias[sys.use_alias].point) {
                     seen[n_seen].net = config->alias[sys.use_alias].net;
                     seen[n_seen++].node = config->alias[sys.use_alias].node;
                  }
                  else if (config->alias[sys.use_alias].fakenet) {
                     seen[n_seen].net = config->alias[sys.use_alias].fakenet;
                     seen[n_seen++].node = config->alias[sys.use_alias].point;
                  }

                  seen[n_seen].net = net;
                  seen[n_seen++].node = node;

                  qsort (seen, n_seen, sizeof (struct _fwrd), mail_sort_func);

                  cnet = cnode = 0;
                  strcpy (wrp, "SEEN-BY: ");

                  for (i = 0; i < n_seen; i++) {
                     if (strlen (wrp) > 65) {
                        mprintf (fpd, "%s\r\n", wrp);
                        cnet = cnode = 0;
                        strcpy (wrp, "SEEN-BY: ");
                     }
                     if (i && seen[i].net == seen[i - 1].net && seen[i].node == seen[i - 1].node)
                        continue;

                     if (cnet != seen[i].net) {
                        sprintf (buffer, "%d/%d ", seen[i].net, seen[i].node);
                        cnet = seen[i].net;
                        cnode = seen[i].node;
                     }
                     else if (cnet == seen[i].net && cnode != seen[i].node) {
                        sprintf (buffer, "%d ", seen[i].node);
                        cnode = seen[i].node;
                     }
                     else
                        strcpy (buffer, "");

                     strcat (wrp, buffer);
                  }

                  mprintf (fpd, "%s\r\n", wrp);
                  mprintf (fpd, "%s\r\n", buff);

                  need_seen = 0;
               }
               else if ((xmsg.attr & MSGLOCAL) && !strncmp (buff, "--- ", 4) && (config->replace_tear || !registered))
                  replace_tearline (fpd, buff);
               else
                  mprintf (fpd, "%s\r\n", buff);

               mi = 0;
            }
            else {
               if(mi < 78)
                  continue;

               if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
                  need_origin = 0;

               buff[mi]='\0';
               mprintf(fpd,"%s",buff);
               mi=0;
               buff[mi] = '\0';
            }
         }
      } while (rd == 80);

      if (!n_seen) {
         if (need_origin) {
            mprintf (fpd, msgtxt[M_TEAR_LINE],VERSION, registered ? "+" : NOREG);
            if (strlen(sys.origin))
               mprintf(fpd,msgtxt[M_ORIGIN_LINE],random_origins(),config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
            else
               mprintf(fpd,msgtxt[M_ORIGIN_LINE],system_name,config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
            need_origin = 0;
         }

         if (!config->alias[sys.use_alias].point) {
            seen[n_seen].net = config->alias[sys.use_alias].net;
            seen[n_seen++].node = config->alias[sys.use_alias].node;
         }
         else if (config->alias[sys.use_alias].fakenet) {
            seen[n_seen].net = config->alias[sys.use_alias].fakenet;
            seen[n_seen++].node = config->alias[sys.use_alias].point;
         }

         seen[n_seen].net = net;
         seen[n_seen++].node = node;

         qsort (seen, n_seen, sizeof (struct _fwrd), mail_sort_func);

         cnet = cnode = 0;
         strcpy (wrp, "SEEN-BY: ");

         for (i = 0; i < n_seen; i++) {
            if (cnet != seen[i].net) {
               sprintf (buffer, "%d/%d ", seen[i].net, seen[i].node);
               cnet = seen[i].net;
               cnode = seen[i].node;
            }
            else if (cnet == seen[i].net && cnode != seen[i].node) {
               sprintf (buffer, "%d ", seen[i].node);
               cnode = seen[i].node;
            }
            else
               strcpy (buffer, "");

            strcat (wrp, buffer);
         }

         mprintf (fpd, "%s\r\n", wrp);
         if (config->alias[sys.use_alias].point && config->alias[sys.use_alias].fakenet)
            mprintf (fpd, "\001PATH: %d/%d\r\n", config->alias[sys.use_alias].fakenet, config->alias[sys.use_alias].point);
         else if (!config->alias[sys.use_alias].point)
            mprintf (fpd, "\001PATH: %d/%d\r\n", config->alias[sys.use_alias].net, config->alias[sys.use_alias].node);
      }

      if (ai)
         sys.use_alias = ai - 1;

      mhdr.ver = PKTVER;
      if (config->alias[sys.use_alias].point && config->alias[sys.use_alias].fakenet) {
         mhdr.orig_node = config->alias[sys.use_alias].point;
         mhdr.orig_net = config->alias[sys.use_alias].fakenet;
      }
      else {
         mhdr.orig_node = config->alias[sys.use_alias].node;
         mhdr.orig_net = config->alias[sys.use_alias].net;
      }
      mhdr.cost = 0;
      mhdr.attrib = 0;

      if (sys.public)
         xmsg.attr &= ~MSGPRIVATE;
      else if (sys.private)
         xmsg.attr |= MSGPRIVATE;

      if (xmsg.attr & MSGPRIVATE)
         mhdr.attrib |= MSGPRIVATE;

      if (point)
         sprintf (wrp, "%d/%d.%d ", net, node, point);
      else
         sprintf (wrp, "%d/%d ", net, node);
      wreadcur (&z, &m);
      if ( (m + strlen (wrp)) > 78) {
         wputs ("\n");
         wputs ("                                           ");
      }
      wputs (wrp);
      if ( (m + strlen (wrp)) == 78)
         wputs ("                                           ");

      mi = open_packet (zone, net, node, point, sys.use_alias);

      mhdr.dest_net = net;
      mhdr.dest_node = node;
      write (mi, (char *)&mhdr, sizeof (struct _msghdr2));

      write (mi, xmsg.ftsc_date, strlen (xmsg.ftsc_date) + 1);
      write (mi, xmsg.to, strlen (xmsg.to) + 1);
      write (mi, xmsg.from, strlen (xmsg.from) + 1);
      write (mi, xmsg.subj, strlen (xmsg.subj) + 1);
      mseek (fpd, 0L, SEEK_SET);
      do {
         z = mread(buffer, 1, 2048, fpd);
         write(mi, buffer, z);
      } while (z == 2048);
      buff[0] = buff[1] = buff[2] = 0;
      if (write (mi, buff, 3) != 3)
         return;
      close (mi);

      totalmsg++;
      sprintf (wrp, "%d (%.1f/s) ", totalmsg, (float)totalmsg / ((float)(timerset (0) - totaltime) / 100));
      prints (10, 65, YELLOW|_BLACK, wrp);

      time_release ();
      MsgCloseMsg (sq_msgh);

      wputs ("\n");
      sent++;
   }

   if (sent)
      status_line (":   %-20.20s (Sent=%04d)", sys.echotag, sent);

   mclose (fpd);
   free (seen);
   unlink ("MSGTMP.EXP");
}

