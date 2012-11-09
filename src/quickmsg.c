
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
#include <io.h>
#include <fcntl.h>
#include <dir.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "quickmsg.h"

#define MAX_MESSAGES   1000

char *pascal_string (char *);
int quick_msg_attrib(struct _msghdr *, int, int, int);
void quick_text_header(struct _msghdr *, int, FILE *);
void write_pascal_string (char *, char *, word *, word *, FILE *);
void quick_qwk_text_header (struct _msghdr *, int, FILE *, struct QWKmsghd *, long *);
void replace_tearline (FILE *fpd, char *buf);
int open_packet (int zone, int net, int node, int point, int ai);
int gold_msg_attrib(struct _gold_msghdr *, int, int, int);
void gold_qwk_text_header (struct _gold_msghdr *, int, FILE *, struct QWKmsghd *, long *);
void gold_text_header (struct _gold_msghdr *, int, FILE *);
void quick_bluewave_text_header (struct _msghdr *msgt, int msgn, int fdi, long bw_start, long bw_len);
void gold_bluewave_text_header (struct _gold_msghdr *msgt, int msgn, int fdi, long bw_start, long bw_len);

FILE *mopen (char *filename, char *mode);
int mclose (FILE *fp);
int mputs (char *s, FILE *fp);
void mprintf (FILE *fp, char *format, ...);
long mseek (FILE *fp, long position, int offset);
int mread (char *s, int n, int e, FILE *fp);

long *msgidx = NULL;
struct _msginfo msginfo;
struct _gold_msginfo gmsginfo;

struct _aidx {
//   char areatag[32];
	unsigned long areatag;
	byte board;
	word gold_board;
};

extern struct _aidx *aidx;
extern int maxaidx;

extern int maxakainfo;
extern struct _akainfo *akainfo;

extern struct _node2name *nametable; // Gestione nomi per aree PRIVATE
extern int nodes_num;

void write_pascal_string (st, text, pos, nb, fp)
char *st, *text;
word *pos, *nb;
FILE *fp;
{
	word v, m, blocks;

	m = *pos;
	blocks = *nb;

	for (v=0; st[v]; v++) {
		text[m++] = st[v];
		if (m == 256) {
			*text = m - 1;
			fwrite(text, 256, 1, fp);
			memset(text, 0, 256);
			m = 1;
			blocks++;
		}
	}

	*pos = m;
	*nb = blocks;
}

void quick_list_headers (start, board, verbose, goldbase)
int start, board, verbose;
char goldbase;
{
   int fd, i, line = verbose ? 2 : 5, l = 0;
   char filename[80];
   struct _msghdr msgt;
   struct _gold_msghdr gmsgt;

   if (goldbase)
      sprintf (filename, "%sMSGHDR.DAT", fido_msgpath);
   else
      sprintf (filename, "%sMSGHDR.BBS", fido_msgpath);

   if ((fd = shopen (filename,O_RDONLY|O_BINARY)) == -1)
      return;

	for (i = start; i <= last_msg; i++) {
      if (goldbase) {
         lseek(fd, (long)sizeof (struct _gold_msghdr) * msgidx[i], SEEK_SET);
         read (fd, (char *)&gmsgt, sizeof (struct _gold_msghdr));

         if (gmsgt.board != board)
            continue;
			if (gmsgt.msgattr & Q_RECKILL)
            continue;
         if ((gmsgt.msgattr & Q_PRIVATE) && stricmp (pascal_string (gmsgt.whofrom), usr.name) && stricmp (pascal_string (gmsgt.whoto), usr.name) 
         && stricmp (pascal_string (gmsgt.whofrom), usr.handle) && stricmp (pascal_string (gmsgt.whoto), usr.handle) && usr.priv < SYSOP)
            continue;

         msg_tzone = gmsgt.destzone;
         msg_fzone = gmsgt.origzone;
      }
      else {
         lseek(fd, (long)sizeof (struct _msghdr) * msgidx[i], SEEK_SET);
         read (fd, (char *)&msgt, sizeof (struct _msghdr));

         if (msgt.board != board)
            continue;
			if (msgt.msgattr & Q_RECKILL)
            continue;
         if ((msgt.msgattr & Q_PRIVATE) && stricmp (pascal_string (msgt.whofrom), usr.name) && stricmp (pascal_string (msgt.whoto), usr.name) 
			&& stricmp (pascal_string (msgt.whofrom), usr.handle) && stricmp (pascal_string (msgt.whoto), usr.handle) && usr.priv < SYSOP)
            continue;

         msg_tzone = msgt.destzone;
         msg_fzone = msgt.origzone;
      }

      if (verbose) {
         if (goldbase) {
            if ((line = gold_msg_attrib (&gmsgt, i, line, 0)) == 0)
               break;
         }
         else {
            if ((line = quick_msg_attrib (&msgt, i, line, 0)) == 0)
               break;
         }

         m_print (bbstxt[B_ONE_CR]);
      }
		else {
         if (goldbase) {
            m_print ("\026\001\003%-4d \026\001\020%c%-20.20s ", i, (stricmp (pascal_string(gmsgt.whofrom), usr.name) && stricmp (pascal_string(gmsgt.whofrom), usr.handle)) ? '\212' : '\216', pascal_string (gmsgt.whofrom));
            m_print ("\026\001\020%c%-20.20s ", (stricmp(pascal_string(gmsgt.whoto), usr.name)&&stricmp(pascal_string (gmsgt.whoto),usr.handle)) ? '\212' : '\214', pascal_string (gmsgt.whoto));
            m_print ("\026\001\013%-32.32s\n", pascal_string (gmsgt.subject));
         }
         else {
            m_print ("\026\001\003%-4d \026\001\020%c%-20.20s ", i, (stricmp (pascal_string(msgt.whofrom), usr.name)&&stricmp (pascal_string(msgt.whofrom), usr.handle)) ? '\212' : '\216', pascal_string (msgt.whofrom));
            m_print ("\026\001\020%c%-20.20s ", (stricmp (pascal_string(msgt.whoto), usr.name)&&stricmp (pascal_string (msgt.whoto),usr.handle)) ? '\212' : '\214', pascal_string (msgt.whoto));
            m_print ("\026\001\013%-32.32s\n", pascal_string (msgt.subject));
         }
      }

      if ((l = more_question (line)) == 0)
         break;

      if (!verbose && l < line) {
         l = 5;

         cls ();
         m_print (bbstxt[B_LIST_AREAHEADER], usr.msg, sys.msg_name);
         m_print (bbstxt[B_LIST_HEADER1]);
         m_print (bbstxt[B_LIST_HEADER2]);
      }

		line = l;
   }

   close (fd);

   if (line)
      press_enter();
}

void quick_message_inquire (stringa, board, goldbase)
char *stringa;
int board;
char goldbase;
{
   int fd, i, line = 4;
   char filename[80];
   struct _msghdr msgt, backup;
   struct _gold_msghdr gmsgt;

	if (goldbase)
      sprintf (filename, "%sMSGHDR.DAT", fido_msgpath);
   else
      sprintf (filename, "%sMSGHDR.BBS", fido_msgpath);
   if ((fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) == -1)
      return;

   for (i = 1; i <= last_msg; i++) {
      if (goldbase) {
         lseek (fd, (long)sizeof (struct _gold_msghdr) * msgidx[i], SEEK_SET);
         read (fd, (char *)&gmsgt, sizeof (struct _gold_msghdr));

         if (gmsgt.board != board)
            continue;
			if (gmsgt.msgattr & Q_RECKILL)
            continue;
         if ((gmsgt.msgattr & Q_PRIVATE) && stricmp (pascal_string (gmsgt.whofrom), usr.name) && stricmp (pascal_string (gmsgt.whoto), usr.name) 
         && stricmp (pascal_string (gmsgt.whofrom), usr.handle) && stricmp(pascal_string (gmsgt.whoto), usr.handle)&& usr.priv < SYSOP)
            continue;

         strcpy (msgt.whoto, pascal_string (gmsgt.whoto));
         strcpy (msgt.whofrom, pascal_string (gmsgt.whofrom));
         strcpy (msgt.subject, pascal_string (gmsgt.subject));

         msg_tzone = gmsgt.destzone;
			msg_fzone = gmsgt.origzone;
      }
      else {
         lseek (fd, (long)sizeof (struct _msghdr) * msgidx[i], SEEK_SET);
         read (fd, (char *)&msgt, sizeof (struct _msghdr));

         if (msgt.board != board)
            continue;
			if (msgt.msgattr & Q_RECKILL)
            continue;
         if ((msgt.msgattr & Q_PRIVATE) && stricmp(pascal_string(msgt.whofrom),usr.name) && stricmp(pascal_string(msgt.whoto),usr.name) 
         && stricmp(pascal_string(msgt.whofrom),usr.handle) && stricmp(pascal_string(msgt.whoto),usr.handle) && usr.priv < SYSOP)
            continue;

         memcpy ((char *)&backup, (char *)&msgt, sizeof (struct _msghdr));
         strcpy (msgt.whoto, pascal_string (msgt.whoto));
         strcpy (msgt.whofrom, pascal_string (msgt.whofrom));
         strcpy (msgt.subject, pascal_string (msgt.subject));

			msg_tzone = msgt.destzone;
         msg_fzone = msgt.origzone;
      }

      strupr (msgt.whoto);
      strupr (msgt.whofrom);
      strupr (msgt.subject);

      if ((strstr (msgt.whoto, stringa) != NULL) || (strstr (msgt.whofrom, stringa) != NULL) || (strstr (msgt.subject, stringa) != NULL)) {
         if (goldbase) {
            if ((line = gold_msg_attrib (&gmsgt, i, line, 0)) == 0)
               break;
         }
         else {
            memcpy ((char *)&msgt, (char *)&backup, sizeof (struct _msghdr));
            if ((line = quick_msg_attrib (&msgt, i, line, 0)) == 0)
               break;
         }

         m_print (bbstxt[B_ONE_CR]);

         if (!(line = more_question (line)) || !CARRIER && !RECVD_BREAK ())
            break;

         time_release ();
		}
   }

   close (fd);

   if (line)
      press_enter ();
}

int quick_mail_header (i, line, goldbase, ovrmsgn)
int i, line;
char goldbase;
int ovrmsgn;
{
   int fd;
   char filename[80];
   struct _msghdr msgt;
   struct _gold_msghdr gmsgt;

	if (goldbase) {
		sprintf (filename, "%sMSGHDR.DAT", fido_msgpath);
		if ((fd = shopen (filename, O_RDONLY|O_BINARY)) == -1)
			return (0);

		lseek (fd, (long)sizeof (struct _gold_msghdr) * msgidx[i], SEEK_SET);
		read (fd, (char *)&gmsgt, sizeof (struct _gold_msghdr));
		close (fd);

		if (gmsgt.msgattr & Q_RECKILL)
			return (line);

		if ((gmsgt.msgattr & Q_PRIVATE) && stricmp (pascal_string (gmsgt.whofrom), usr.name) && stricmp (pascal_string (gmsgt.whoto), usr.name)
		&& stricmp (pascal_string (gmsgt.whofrom), usr.handle) && stricmp (pascal_string (gmsgt.whoto), usr.handle) && usr.priv < SYSOP)
			return (line);

		msg_tzone = gmsgt.destzone;
		msg_fzone = gmsgt.origzone;
		if ((line = gold_msg_attrib (&gmsgt, ovrmsgn ? ovrmsgn : i, line, 0)) == 0)
			return (0);
	}
	else {
		sprintf (filename, "%sMSGHDR.BBS", fido_msgpath);
		if ((fd = shopen (filename,O_RDONLY|O_BINARY)) == -1)
			return (0);

		lseek (fd, (long)sizeof (struct _msghdr) * msgidx[i], SEEK_SET);
		read (fd, (char *)&msgt, sizeof (struct _msghdr));
		close (fd);

		if (msgt.msgattr & Q_RECKILL)
			return (line);

		if ((msgt.msgattr & Q_PRIVATE) && stricmp (pascal_string (msgt.whofrom), usr.name) && stricmp (pascal_string (msgt.whoto), usr.name)
		&& stricmp (pascal_string (msgt.whofrom), usr.handle) && stricmp (pascal_string (msgt.whoto), usr.handle) && usr.priv < SYSOP)
			return (line);

		msg_tzone = msgt.destzone;
		msg_fzone = msgt.origzone;
		if ((line = quick_msg_attrib (&msgt, ovrmsgn ? ovrmsgn : i, line, 0)) == 0)
			return (0);
	}

	return (line);
}

int quick_kill_message (msg_num, goldbase)
int msg_num;
char goldbase;
{
	int fd, fdidx, fdto;
	char filename[80];
	struct _msghdr msgt;
	struct _msgidx idx;
	struct _gold_msghdr gmsgt;
	struct _gold_msgidx gidx;

	if (goldbase) {
		sprintf (filename, "%sMSGHDR.DAT", fido_msgpath);
		if ((fd = sh_open (filename, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE)) == -1)
			return (0);

		lseek (fd, (long)sizeof (struct _gold_msghdr) * msgidx[msg_num], SEEK_SET);
		read (fd, (char *)&gmsgt, sizeof (struct _gold_msghdr));

		if (!strnicmp (&gmsgt.whofrom[1], usr.name, gmsgt.whofrom[0]) || !strnicmp (&gmsgt.whoto[1], usr.name, gmsgt.whoto[0]) ||
		!strnicmp (&gmsgt.whofrom[1], usr.handle, gmsgt.whofrom[0]) || !strnicmp (&gmsgt.whoto[1], usr.handle, gmsgt.whoto[0]) || usr.priv == SYSOP) {
			gmsgt.msgattr |= Q_RECKILL;
//			gmsgt.msgnum = 0;
			lseek (fd, (long)sizeof (struct _gold_msghdr) * msgidx[msg_num], SEEK_SET);
			write (fd, (char *)&gmsgt, sizeof (struct _gold_msghdr));

			sprintf (filename, "%sMSGIDX.DAT", fido_msgpath);
			fdidx = sh_open (filename, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
			lseek (fdidx, (long)msgidx[msg_num] * sizeof (struct _msgidx), SEEK_SET);
			read (fdidx, (char *)&gidx, sizeof (struct _gold_msgidx));
			gidx.msgnum = -1;
			lseek (fdidx, (long)msgidx[msg_num] * sizeof (struct _gold_msgidx), SEEK_SET);
			write (fdidx, (char *)&gidx, sizeof (struct _gold_msgidx));
			close (fdidx);

			sprintf (filename, "%sMSGTOIDX.DAT", fido_msgpath);
         fdto = sh_open (filename, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
         lseek (fdto, (long)msgidx[msg_num] * 36L, SEEK_SET);
         write (fdto, "\x0B* Deleted *                        ", 36);
         close (fdto);

         close (fd);

         return (1);
      }

      close (fd);
   }
   else {
      sprintf (filename, "%sMSGHDR.BBS", fido_msgpath);
      if ((fd = sh_open (filename, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE)) == -1)
         return (0);

      lseek (fd, (long)sizeof (struct _msghdr) * msgidx[msg_num], SEEK_SET);
      read (fd, (char *)&msgt, sizeof (struct _msghdr));

      if (!strnicmp (&msgt.whofrom[1], usr.name, msgt.whofrom[0]) || !strnicmp (&msgt.whoto[1], usr.name, msgt.whoto[0]) ||
      !strnicmp (&msgt.whofrom[1], usr.handle, msgt.whofrom[0]) || !strnicmp (&msgt.whoto[1], usr.handle, msgt.whoto[0]) || usr.priv == SYSOP) {
			msgt.msgattr |= Q_RECKILL;
//			msgt.msgnum = 0;  NON TESTATO!
         lseek (fd, (long)sizeof (struct _msghdr) * msgidx[msg_num], SEEK_SET);
         write (fd, (char *)&msgt, sizeof (struct _msghdr));

         sprintf (filename, "%sMSGIDX.BBS", fido_msgpath);
         fdidx = sh_open (filename, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
         lseek (fdidx, (long)msgidx[msg_num] * sizeof (struct _msgidx), SEEK_SET);
         read (fdidx, (char *)&idx, sizeof (struct _msgidx));
			idx.msgnum = -1;
			lseek (fdidx, (long)msgidx[msg_num] * sizeof (struct _msgidx), SEEK_SET);
			write (fdidx, (char *)&idx, sizeof (struct _msgidx));
			close (fdidx);

			sprintf (filename, "%sMSGTOIDX.BBS", fido_msgpath);
			fdto = sh_open (filename, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
			lseek (fdto, (long)msgidx[msg_num] * 36L, SEEK_SET);
			write (fdto, "\x0B* Deleted *                        ", 36);
			close (fdto);

			close (fd);

			return (1);
		}

		close (fd);
	}

   return (0);
}

int quick_scan_messages (total, personal, board, start, fdi, fdp, totals, fpt, qwk, goldbase)
int *total, *personal, board, start, fdi, fdp, totals;
FILE *fpt;
char qwk, goldbase;
{
   FILE *fp;
   float in, out;
   int i, tt, z, m, pos, blks, msgn, pr, fd;
   char c, buff[130], wrp[130], qwkbuffer[130], shead;
   byte tpbyte, lastr;
   long qpos, bw_start;
   struct QWKmsghd QWK;
   struct _msghdr msgt;
   struct _gold_msghdr gmsgt;

   tt = 0;
   pr = 0;
   shead = 0;

   if (start > last_msg) {
      *personal = pr;
      *total = tt;

      return (totals);
   }

   sprintf (buff, goldbase ? "%sMSGHDR.DAT" : "%sMSGHDR.BBS", fido_msgpath);
   fd = shopen (buff,O_RDONLY|O_BINARY);
   if (fd == -1)
      return (totals);

	sprintf (buff, goldbase ? "%sMSGTXT.DAT" : "%sMSGTXT.BBS",  fido_msgpath);
   fp = fopen (buff,"rb");
   if (fp == NULL) {
      close (fd);
      return (totals);
   }
   setvbuf (fp, NULL, _IOFBF, 1024);

   for (msgn = start; msgn <= last_msg; msgn++) {
      if (!(msgn % 5))
         display_percentage (msgn - start, last_msg - start);

      if (goldbase) {
         lseek (fd, (long)sizeof (struct _gold_msghdr) * msgidx[msgn], SEEK_SET);
         read (fd, (char *)&gmsgt, sizeof (struct _gold_msghdr));

         if (gmsgt.board != board)
            continue;
			if (gmsgt.msgattr & Q_RECKILL)
            continue;
         if ((gmsgt.msgattr & Q_PRIVATE) && stricmp (pascal_string (gmsgt.whofrom), usr.name) && stricmp (pascal_string (gmsgt.whoto), usr.name)
         && stricmp (pascal_string (gmsgt.whofrom), usr.handle) && stricmp (pascal_string (gmsgt.whoto), usr.handle) && usr.priv < SYSOP)
            continue;
      }
      else {
         lseek (fd, (long)sizeof (struct _msghdr) * msgidx[msgn], SEEK_SET);
         read (fd, (char *)&msgt, sizeof (struct _msghdr));

         if (msgt.board != board)
            continue;
			if (msgt.msgattr & Q_RECKILL)
            continue;
         if ((msgt.msgattr & Q_PRIVATE) && stricmp (pascal_string (msgt.whofrom), usr.name) && stricmp (pascal_string (msgt.whoto), usr.name) 
         && stricmp (pascal_string (msgt.whofrom), usr.handle) && stricmp (pascal_string (msgt.whoto), usr.handle) && usr.priv < SYSOP)
            continue;
      }

      totals++;
      if (qwk == 1 && fdi != -1) {
         sprintf (buff, "%u", totals);   /* Stringized version of current position */
         in = (float) atof (buff);
         out = IEEToMSBIN (in);
         write (fdi, &out, sizeof (float));

			c = 0;
         write (fdi, &c, sizeof(char));              /* Conference # */
      }
      else if (qwk == 2) {
         bw_start = ftell (fpt);
         fputc (' ', fpt);
      }

      if ((!goldbase && !stricmp (pascal_string (msgt.whoto), usr.name)) ||
      (goldbase && !stricmp (pascal_string (gmsgt.whoto), usr.name))||
      (!goldbase && !stricmp (pascal_string (msgt.whoto), usr.handle)) ||
      (goldbase && !stricmp (pascal_string (gmsgt.whoto), usr.handle))) {
         pr++;
         if (fdp != -1 && qwk == 1 && fdi != -1) {
             write (fdp, &out, sizeof (float));
             write (fdp, &c, sizeof (char));              /* Conference # */
         }
      }

      if (goldbase)
         fseek (fp, 256L * gmsgt.startblock, SEEK_SET);
      else
         fseek (fp, 256L * msgt.startblock, SEEK_SET);

      tpbyte = 0;
      lastr = 255;
      blks = 1;
      pos = 0;
      memset (qwkbuffer, ' ', 128);
      i = 0;
      shead = 0;

      for (;;) {
         if (!tpbyte) {
            if (lastr != 255)
               break;
            tpbyte = fgetc (fp);
            lastr = tpbyte;
            if (!tpbyte)
               break;
         }

         c = fgetc(fp);
         tpbyte--;

         if (c == '\0')
            break;

         if (c == 0x0A || c == '\0')
            continue;

         buff[i++] = c;

         if (c == 0x0D || (byte)c == 0x8D) {
            buff[i-1] = '\0';

            if (buff[0] == 0x01) {
               if (!strncmp (&buff[1], "INTL",4) && !shead)
                  sscanf (&buff[6], "%d:%d/%d %d:%d/%d", &msg_tzone, &i, &i, &msg_fzone, &i, &i);
               if (!strncmp (&buff[1], "TOPT",4) && !shead)
                  sscanf (&buff[6], "%d", &msg_tpoint);
               if (!strncmp (&buff[1], "FMPT",4) && !shead)
                  sscanf (&buff[6], "%d", &msg_fpoint);
               i = 0;
               continue;
            }
            else if (!shead) {
               if (goldbase) {
                  if (qwk == 1)
                     gold_qwk_text_header (&gmsgt, msgn, fpt, &QWK, &qpos);
                  else if (qwk == 0)
                     gold_text_header (&gmsgt, msgn, fpt);
               }
               else {
                  if (qwk == 1)
                     quick_qwk_text_header (&msgt, msgn, fpt, &QWK, &qpos);
                  else if (qwk == 0)
                     quick_text_header (&msgt, msgn, fpt);
               }
               shead = 1;
            }

            if (buff[0] == 0x01 || !strncmp (buff, "SEEN-BY", 7)) {
               i = 0;
               continue;
            }

            if (qwk == 1) {
					write_qwk_string (buff, qwkbuffer, &pos, &blks, fpt);
               write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpt);
            }
            else
               fprintf (fpt, "%s\n", buff);

            i = 0;
         }
         else {
            if(i < (usr.width - 1))
               continue;

            buff[i] = '\0';
            while (i > 0 && buff[i] != ' ')
               i--;

            m = 0;

            if(i != 0)
               for(z=i+1;buff[z];z++)
                  wrp[m++]=buff[z];

            buff[i]='\0';
            wrp[m]='\0';

            if (!shead) {
               if (goldbase) {
                  if (qwk == 1)
                     gold_qwk_text_header (&gmsgt, msgn, fpt, &QWK, &qpos);
                  else if (qwk == 0)
                     gold_text_header (&gmsgt, msgn, fpt);
               }
               else {
                  if (qwk == 1)
                     quick_qwk_text_header (&msgt, msgn, fpt, &QWK, &qpos);
                  else if (qwk == 0)
                     quick_text_header (&msgt, msgn, fpt);
               }
               shead = 1;
            }

            if (qwk == 1) {
               write_qwk_string (buff, qwkbuffer, &pos, &blks, fpt);
               write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpt);
				}
            else
               fprintf (fpt, "%s\n", buff);

            strcpy (buff, wrp);
            i = strlen (buff);
         }
      }

      if (qwk == 1) {
         qwkbuffer[128] = 0;
         fwrite (qwkbuffer, 128, 1, fpt);
         blks++;

         fseek (fpt, qpos, SEEK_SET);          /* Restore back to header start */
         sprintf (buff, "%d", blks);
         ljstring (QWK.Msgrecs, buff, 6);
         fwrite ((char *)&QWK, 128, 1, fpt);           /* Write out the header */
         fseek (fpt, 0L, SEEK_END);               /* Bump back to end of file */
      }
      else if (qwk == 2) {
         if (goldbase)
            gold_bluewave_text_header (&gmsgt, msgn, fdi, bw_start, ftell (fpt) - bw_start);
         else
            quick_bluewave_text_header (&msgt, msgn, fdi, bw_start, ftell (fpt) - bw_start);
      }
      else
         fprintf (fpt, bbstxt[B_TWO_CR]);

      tt++;
      totals += blks - 1;
   }

   fclose (fp);
   close (fd);

   *personal = pr;
   *total = tt;

   return (totals);
}

int quick_save_message2 (txt, f1, f2, f3, f4)
FILE *txt, *f1, *f2, *f3, *f4;
{
	FILE *fp;
	word i, dest, m, nb, x, hh, mm, dd, mo, aa;
	char text[258], buffer[258];
	long gdest;
	struct _msgidx idx;
	struct _msghdr hdr;
	struct _msgtoidx toidx;
	struct _gold_msgidx gidx;
	struct _gold_msghdr ghdr;

	memset ((char *)&toidx, 0, sizeof (struct _msgtoidx));

	memcpy (&toidx.string[1], msg.to, strlen (msg.to));
	toidx.string[0] = strlen (msg.to);
	if (fwrite ((char *)&toidx, sizeof (struct _msgtoidx), 1, f2) != 1)
		return (0);

	if (sys.gold_board) {
      memset ((char *)&gidx, 0, sizeof (struct _gold_msgidx));
      memset ((char *)&ghdr, 0, sizeof (struct _gold_msghdr));

      gdest = gmsginfo.highmsg + 1;
      if (gmsginfo.highmsg > gdest) {
			status_line ("!Gold-base exceeds msgnum limit");
			return (0);
		}

		gidx.msgnum = gdest;
		gidx.board = sys.gold_board;
		if (fwrite ((char *)&gidx, sizeof (struct _gold_msgidx), 1, f1) != 1)
			return (0);
	}
	else {
		memset ((char *)&idx, 0, sizeof (struct _msgidx));
		memset ((char *)&hdr, 0, sizeof (struct _msghdr));

		dest = msginfo.highmsg + 1;
		if (msginfo.highmsg == 65535U) {
			status_line ("!Hudson-base exceeds 65535 messages");
			return (0);
		}

		idx.msgnum = dest;
		idx.board = sys.quick_board;
		if (fwrite ((char *)&idx, sizeof (struct _msgidx), 1, f1) != 1){
			status_line("!Problems with quick msgidx file");
			return (0);
		}
	}

	fp = f3;

	if (sys.gold_board)
		ghdr.startblock = ftell (fp) / 256L;
	else {
		i = (word)(ftell (fp) / 256L);
		hdr.startblock = i;
	}

	i = 0;
	m = 1;
	nb = 1;

	memset (text, 0, 256);

	do {
		memset (buffer, 0, 256);
		i = mread (buffer, 1, 255, txt);
		for (x = 0; x < i; x++) {
			if (buffer[x] == 0x1A)
				buffer[x] = ' ';
		}
		buffer[i] = '\0';
		write_pascal_string (buffer, text, &m, &nb, fp);
	} while (i == 255);

	*text = m - 1;
	if (fwrite (text, 256, 1, fp) != 1)
		return (0);

	if (sys.public)
		msg.attr &= ~MSGPRIVATE;
	else if (sys.private)
		msg.attr |= MSGPRIVATE;

	sscanf (msg.date, "%2d %3s %2d %2d:%2d", &dd, buffer, &aa, &hh, &mm);
	buffer[3] = '\0';
	for (mo = 0; mo < 12; mo++) {
		if (!stricmp (buffer, mtext[mo]))
			break;
	}
	if (mo == 12)
		mo = 0;

	if (sys.gold_board) {
		ghdr.numblocks = nb;
		ghdr.msgnum = gdest;
		ghdr.prevreply = msg.reply;
		ghdr.nextreply = msg.up;
		ghdr.timesread = 0;
		ghdr.destnet = msg.dest_net;
		ghdr.destnode = msg.dest;
		ghdr.orignet = msg.orig_net;
		ghdr.orignode = msg.orig;
		ghdr.destzone = msg_tzone;
		ghdr.origzone = msg_fzone;
		ghdr.cost = msg.cost;

		if (msg.attr & MSGPRIVATE)
			ghdr.msgattr |= Q_PRIVATE;

		if (sys.echomail && (msg.attr & MSGSENT))
         ghdr.msgattr &= ~Q_LOCAL;
      else {
         ghdr.msgattr &= ~Q_LOCAL;
         ghdr.msgattr |= 32;
      }

      ghdr.board = sys.gold_board;
      sprintf (&ghdr.time[1], "%02d:%02d", hh, mm);
      sprintf (&ghdr.date[1], "%02d-%02d-%02d", mo + 1, dd, aa);
      ghdr.time[0] = 5;
      ghdr.date[0] = 8;

      memcpy (&ghdr.whoto[1], msg.to, strlen (msg.to));
      ghdr.whoto[0] = strlen (msg.to);
      memcpy (&ghdr.whofrom[1], msg.from, strlen (msg.from));
      ghdr.whofrom[0] = strlen (msg.from);
      memcpy (&ghdr.subject[1], msg.subj, strlen (msg.subj));
      ghdr.subject[0] = strlen (msg.subj);

      gmsginfo.totalonboard[sys.gold_board - 1]++;
      gmsginfo.totalmsgs++;
		gmsginfo.highmsg++;

      if (fwrite ((char *)&ghdr, sizeof (struct _gold_msghdr), 1, f4) != 1)
         return (0);
   }
   else {
      hdr.numblocks = nb;
		hdr.msgnum = dest;
      hdr.prevreply = msg.reply;
      hdr.nextreply = msg.up;
      hdr.timesread = 0;
      hdr.destnet = msg.dest_net;
      hdr.destnode = msg.dest;
      hdr.orignet = msg.orig_net;
      hdr.orignode = msg.orig;
      hdr.destzone = msg_tzone;
      hdr.origzone = msg_fzone;
      hdr.cost = msg.cost;

      if (msg.attr & MSGPRIVATE)
         hdr.msgattr |= Q_PRIVATE;

      if (sys.echomail && (msg.attr & MSGSENT))
         hdr.msgattr &= ~Q_LOCAL;
      else {
         hdr.msgattr &= ~Q_LOCAL;
         hdr.msgattr |= 32;
      }

      hdr.board = sys.quick_board;
      sprintf (&hdr.time[1], "%02d:%02d", hh, mm);
      sprintf (&hdr.date[1], "%02d-%02d-%02d", mo + 1, dd, aa);
      hdr.time[0] = 5;
      hdr.date[0] = 8;

      memcpy (&hdr.whoto[1], msg.to, strlen (msg.to));
      hdr.whoto[0] = strlen (msg.to);
      memcpy (&hdr.whofrom[1], msg.from, strlen (msg.from));
      hdr.whofrom[0] = strlen (msg.from);
      memcpy (&hdr.subject[1], msg.subj, strlen (msg.subj));
      hdr.subject[0] = strlen (msg.subj);

      msginfo.totalonboard[sys.quick_board - 1]++;
      msginfo.totalmsgs++;
		msginfo.highmsg++;

      if (fwrite ((char *)&hdr, sizeof (struct _msghdr), 1, f4) != 1)
         return (0);

      if ((int)(65536L - hdr.startblock) < hdr.numblocks) {
         status_line ("!Hudson-base exceeds MSGTXT size limit");
         return (0);
      }
   }

   last_msg++;

   return (1);
}

int quick_export_mail (maxnodes, forward, goldbase)
int maxnodes;
struct _fwrd *forward;
int goldbase;
{
	FILE *fpd, *f2, *fp;
	int finfo, f1, i, pp, z, fd, ne, no, m, n_seen, cnet, cnode, mi, sent;
	int ai, last_board, fdmsg;
	char buff[80], wrp[80], c, found, *p, buffer[2050];
	char *location, *tag, *forw, need_origin, need_seen, *flag;
	byte tpbyte, lastr;
	unsigned short quick_msgexp;
	long gold_msgexp;
	struct _msghdr msghdr;
	struct _msghdr2 mhdr;
	struct _fwrd *seen;
	struct _gold_msghdr gmsghdr;

	if (goldbase) {
		sprintf (buff, "%sMSGINFO.DAT", fido_msgpath);
		finfo = sh_open (buff, SH_DENYWR, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
		if (finfo == -1)
			return (maxnodes);
		read (finfo, (char *)&gmsginfo, sizeof (struct _gold_msginfo));

		sprintf(buff, "%sMSGHDR.DAT", fido_msgpath);
		f1 = sh_open (buff, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
		if (f1 == -1) {
			close (finfo);
			return (maxnodes);
		}

		gmsginfo.totalmsgs = (long)(filelength (f1) / sizeof (struct _gold_msghdr));

		sprintf (buff, "%sMSGTXT.DAT", fido_msgpath);
	}
	else {
		sprintf (buff, "%sMSGINFO.BBS", fido_msgpath);
		finfo = sh_open (buff, SH_DENYWR, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
		if(finfo == -1)
			return (maxnodes);
		read (finfo, (char *)&msginfo, sizeof (struct _msginfo));

		sprintf(buff, "%sMSGHDR.BBS", fido_msgpath);
		f1 = sh_open(buff, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
		if (f1 == -1) {
			close (finfo);
			return (maxnodes);
		}

		msginfo.totalmsgs = (word)(filelength (f1) / sizeof (struct _msghdr));

		sprintf (buff, "%sMSGTXT.BBS", fido_msgpath);
	}

	i = sh_open (buff, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
	if (i == -1) {
		close (f1);
		close (finfo);
		return (maxnodes);
	}
	f2 = fdopen (i,"rb");
   if (f2 == NULL) {
      close (i);
      close (f1);
      close (finfo);
      return (maxnodes);
   }
   setvbuf (f2, NULL, _IOFBF, 1024);

   sprintf (buff, SYSMSG_PATH, config->sys_path);
   fd = sh_open (buff, SH_DENYNONE, O_RDONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
   if (fd == -1) {
      close (f1);
      fclose (f2);
      close (finfo);
      return (maxnodes);
   }

   if (config->use_areasbbs)
      fp = fopen (config->areas_bbs, "rt");
   else
      fp = NULL;

   last_board = 0;
   seen = (struct _fwrd *)malloc ((MAX_EXPORT_SEEN + 1) * sizeof (struct _fwrd));
   if (seen == NULL) {
      close (f1);
      fclose (f2);
      close (fd);
		close (finfo);
      if (fp != NULL)
         fclose (fp);
      return (maxnodes);
   }

   fpd = fopen ("MSGTMP.EXP", "rb+");
   if (fpd == NULL) {
      fpd = fopen ("MSGTMP.EXP", "wb");
      fclose (fpd);
   }
   else
      fclose (fpd);
   fpd = mopen ("MSGTMP.EXP", "r+b");

	prints (7, 65, YELLOW|_BLACK, "              ");
   prints (8, 65, YELLOW|_BLACK, "N/A           ");
   prints (9, 65, YELLOW|_BLACK, "              ");
   prints (9, 65, YELLOW|_BLACK, goldbase ? "GoldBase" : "QuickBBS");

   sent = 0;

   if (goldbase) {
      sprintf (buff, "%sECHOMAIL.DAT", fido_msgpath);
      fdmsg = sh_open (buff, SH_DENYRW, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
   }
   else {
      sprintf (buff, "%sECHOMAIL.BBS", fido_msgpath);
      fdmsg = sh_open (buff, SH_DENYRW, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
   }

   for (;;) {
      if (goldbase) {
         if (fdmsg != -1) {
            if (read (fdmsg, &gold_msgexp, sizeof (long)) != sizeof (long))
               break;
            lseek (f1, gold_msgexp * sizeof (struct _gold_msghdr), SEEK_SET);
         }

         if (read (f1, (char *)&gmsghdr, sizeof (struct _gold_msghdr)) != sizeof (struct _gold_msghdr))
            break;
			sprintf (wrp, "%ld / %ld", gmsghdr.msgnum, gmsginfo.totalmsgs);

			if ( !(gmsghdr.msgnum % 60) )
				time_release ();
      }
      else {
         if (fdmsg != -1) {
            if (read (fdmsg, &quick_msgexp, sizeof (unsigned short)) != sizeof (unsigned short))
               break;
            lseek (f1, (long)quick_msgexp * sizeof (struct _msghdr), SEEK_SET);
         }

         if (read (f1, (char *)&msghdr, sizeof (struct _msghdr)) != sizeof (struct _msghdr))
            break;
			sprintf (wrp, "%d / %d", msghdr.msgnum, msginfo.totalmsgs);

			if ( !(msghdr.msgnum % 60) )
            time_release ();
		}

      prints (7, 65, YELLOW|_BLACK, wrp);

      if (goldbase) {
         if ( !(gmsghdr.msgattr & 32) )
            continue;
			if (gmsghdr.msgattr & Q_RECKILL)
            continue;
      }
      else {
         if ( !(msghdr.msgattr & 32) )
            continue;
			if (msghdr.msgattr & Q_RECKILL)
            continue;
      }

      if ((!goldbase && msghdr.board != last_board) || (goldbase && gmsghdr.board != last_board)) {
         if (sent && last_board) {
            status_line (":   %-20.20s (Sent=%04d)", sys.echotag, sent);
            sent = 0;
         }

         last_board = goldbase ? gmsghdr.board : msghdr.board;
         memset ((char *)&sys, 0, sizeof (struct _sys));
         found = 0;

         if (fp != NULL) {
            rewind (fp);
				fgets (buffer, 255, fp);

            while (fgets(buffer, 255, fp) != NULL) {
               if (buffer[0] == ';' || !isdigit (buffer[0]))
                  continue;
               while (buffer[strlen (buffer) -1] == 0x0D || buffer[strlen (buffer) -1] == 0x0A || buffer[strlen (buffer) -1] == ' ')
                  buffer[strlen (buffer) -1] = '\0';
               location = strtok (buffer, " ");
               tag = strtok (NULL, " ");
               forw = strtok (NULL, "");

               if ((!goldbase && atoi (location) == msghdr.board) || (!goldbase && atoi (location) == gmsghdr.board)) {
                  while (*forw == ' ')
                     forw++;
                  memset ((char *)&sys, 0, sizeof (struct _sys));
						strcpy (sys.echotag, tag);
                  if (forw != NULL)
                     strcpy (sys.forward1, forw);
                  if (goldbase)
                     sys.gold_board = atoi (location);
                  else
                     sys.quick_board = atoi (location);
                  sys.echomail = 1;
                  sys.use_alias = 0;
                  found = 1;
                  break;
               }
            }
         }

         if (!found) {
            for (i = 0; i < maxaidx; i++) {
               if (goldbase) {
                  if (aidx[i].gold_board == gmsghdr.board)
                     break;
               }
               else {
                  if (aidx[i].board == msghdr.board)
                     break;
               }
            }

            if (i < maxaidx) {
               lseek (fd, (long)i * SIZEOF_MSGAREA, SEEK_SET);
					read (fd, (char *)&sys.msg_name, SIZEOF_MSGAREA);
               found = 1;
            }
         }

         if (!found)
            continue;

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
               while (*p == '<' || *p == '>' || *p == '!'||*p=='p'||*p=='P')
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
               while (*flag == '<' || *flag == '>' || *flag =='!'||*flag=='P'||*flag=='p') {
                  if (*flag == '>')
                     forward[i].receiveonly = 1;
                  if (*flag == '<')
                     forward[i].sendonly = 1;
                  if (*flag == '!')
                     forward[i].passive = 1;                  
                  if (*flag =='p'||*flag=='P')
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
               while (*p == '<' || *p == '>' || *p == '!'||*p=='p'||*p=='P')
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
               while (*flag == '<' || *flag == '>' || *flag =='!'||*flag=='P'||*flag=='p') {
                  if (*flag == '>')
                     forward[i].receiveonly = 1;
                  if (*flag == '<')
                     forward[i].sendonly = 1;
						if (*flag == '!')
                     forward[i].passive = 1;                  
                  if (*flag =='p'||*flag=='P')
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
               while (*p == '<' || *p == '>' || *p == '!'||*p=='p'||*p=='P')
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
					while (*flag == '<' || *flag == '>' || *flag =='!'||*flag=='P'||*flag=='p') {
						if (*flag == '>')
							forward[i].receiveonly = 1;
						if (*flag == '<')
							forward[i].sendonly = 1;
						if (*flag == '!')
							forward[i].passive = 1;
						if (*flag =='p'||*flag=='P')
							forward[i].private = 1;
						flag++;
					}
				} while ((p = strtok (NULL, " ")) != NULL);
		}
		else if (!found)
			continue;

		for (i = 0; i < maxnodes; i++)
			if (forward[i].reset)
				forward[i].export = 1;

		for (i=0;i<maxnodes;i++) {
			if (forward[i].passive||forward[i].sendonly) forward[i].export=0;
			if (forward[i].private) {

				char dest[40];
				int ii;

				forward[i].export=0;
				if (!nametable) continue;

				if(goldbase) strncpy(dest,pascal_string(gmsghdr.whoto),36);
				else strncpy(dest,pascal_string(msghdr.whoto),36);
				for(ii=0;ii<nodes_num;ii++) {
					if (forward[i].zone==nametable[ii].zone&&
						 forward[i].net==nametable[ii].net&&
						 forward[i].node==nametable[ii].node&&
						 forward[i].point==nametable[ii].point&&
						 !stricmp(dest,nametable[ii].name)) {
							 forward[i].export=1;
							 break;
					}
				}
			}


		}

		need_seen = need_origin = 1;

		strcpy (wrp, sys.echotag);
		wrp[14] = '\0';
		prints (8, 65, YELLOW|_BLACK, "              ");
		prints (8, 65, YELLOW|_BLACK, wrp);

		mseek (fpd, 0L, SEEK_SET);
		mprintf (fpd, "AREA:%s\r\n", sys.echotag);

		if (goldbase) {
			sprintf (wrp, "%5ld  %-22.22s %s     ", gmsghdr.msgnum, sys.echotag, "GoldBase");
			fseek (f2, 256L * gmsghdr.startblock, SEEK_SET);
		}
		else {
			sprintf (wrp, "%5d  %-22.22s %s     ", msghdr.msgnum, sys.echotag, "QuickBBS");
			fseek (f2, 256L * msghdr.startblock, SEEK_SET);
		}

		wputs (wrp);

		mi = i = 0;
		tpbyte = 0;
		lastr = 255;
		n_seen = 0;

		for (;;) {
			if (!tpbyte) {
				if (lastr != 255)
					break;
				tpbyte = fgetc (f2);
				lastr = tpbyte;
				if (!tpbyte)
					break;
			}

			c = fgetc (f2);
			tpbyte--;

			if (c == '\0')
				break;

			if (c == 0x0A)
				continue;
			if ((byte)c == 0x8D)
				c = ' ';

			buff[mi++] = c;

			if (c == 0x0D) {
				buff[mi - 1]='\0';
				if (!strncmp (buff, msgtxt[M_ORIGIN_LINE], 11))
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
				else {
					if ((!goldbase && (msghdr.msgattr & 64)) || (goldbase && (gmsghdr.msgattr & 64))) {
						if (!strncmp (buff, "--- ", 4) && (config->replace_tear || !registered))
							replace_tearline (fpd, buff);
						else
							mprintf (fpd, "%s\r\n", buff);
					}
					else
						mprintf (fpd, "%s\r\n", buff);
				}

				mi = 0;
			}
			else {
				if (mi < 78)
					continue;

				if (!strncmp (buff, msgtxt[M_ORIGIN_LINE], 11))
					need_origin = 0;

				buff[mi] = '\0';
				mprintf (fpd, "%s", buff);
				mi = 0;
				buff[mi] = '\0';
			}
		}

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

			ai = sys.use_alias;
			for (i = 0; i < maxnodes; i++) {
				if (!forward[i].export || forward[i].net == config->alias[sys.use_alias].fakenet)
					continue;
				seen[n_seen].net = forward[i].net;
				seen[n_seen++].node = forward[i].node;

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
		mhdr.cost = msghdr.cost;
		mhdr.attrib = 0;

		if (sys.public)
			mhdr.attrib &= ~Q_PRIVATE;
		else if (sys.private)
			mhdr.attrib |= Q_PRIVATE;

		if (goldbase) {
			if (gmsghdr.msgattr & Q_PRIVATE)
				mhdr.attrib |= MSGPRIVATE;
		}
		else {
			if (msghdr.msgattr & Q_PRIVATE)
				mhdr.attrib |= MSGPRIVATE;
		}

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

         if (goldbase) {
            strcpy (wrp, pascal_string (gmsghdr.date));
            sscanf (wrp, "%2d-%2d-%2d", &z, &ne, &no);
            strcpy (wrp, pascal_string (gmsghdr.time));
            sscanf (wrp, "%2d:%2d", &pp, &m);
            sprintf (buff, "%02d %s %02d  %2d:%02d:00", ne, mtext[z-1], no, pp, m);
            write (mi, buff, strlen (buff) + 1);
            strcpy (buff, pascal_string (gmsghdr.whoto));
            write (mi, buff, strlen (buff) + 1);
            strcpy (buff, pascal_string (gmsghdr.whofrom));
            write (mi, buff, strlen (buff) + 1);
            strcpy (buff, pascal_string (gmsghdr.subject));
            write (mi, buff, strlen (buff) + 1);
			}
         else {
            strcpy (wrp, pascal_string (msghdr.date));
            sscanf (wrp, "%2d-%2d-%2d", &z, &ne, &no);
            strcpy (wrp, pascal_string (msghdr.time));
            sscanf (wrp, "%2d:%2d", &pp, &m);
            sprintf (buff, "%02d %s %02d  %2d:%02d:00", ne, mtext[z-1], no, pp, m);
            write (mi, buff, strlen (buff) + 1);
            strcpy (buff, pascal_string (msghdr.whoto));
            write (mi, buff, strlen (buff) + 1);
            strcpy (buff, pascal_string (msghdr.whofrom));
            write (mi, buff, strlen (buff) + 1);
				strcpy (buff, pascal_string (msghdr.subject));
            write (mi, buff, strlen (buff) + 1);
         }

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
      }

      if (goldbase) {
         lseek (f1, -1L * sizeof (struct _gold_msghdr), SEEK_CUR);
         gmsghdr.msgattr &= ~32;
         write (f1, (char *)&gmsghdr, sizeof (struct _gold_msghdr));
      }
      else {
         lseek (f1, -1L * sizeof (struct _msghdr), SEEK_CUR);
         msghdr.msgattr &= ~32;
         write (f1, (char *)&msghdr, sizeof (struct _msghdr));
      }

      wputs ("\n");
      sent++;
	}

   if (sent)
      status_line (":   %-20.20s (Sent=%04d)", sys.echotag, sent);

   if (fdmsg != -1) {
      close (fdmsg);

      if (goldbase)
         sprintf (buff, "%sECHOMAIL.DAT", fido_msgpath);
      else
         sprintf (buff, "%sECHOMAIL.BBS", fido_msgpath);
		unlink (buff);
   }

   if (fp != NULL)
      fclose (fp);
   mclose (fpd);
   free (seen);
   close (fd);
   fclose (f2);
   close (f1);
   close (finfo);
   unlink ("MSGTMP.EXP");

   return (maxnodes);
}


int quick_rescan_echomail (int board, int zone, int net, int node, int point, int goldboard)
{
   FILE *fpd, *f2, *fp;
   int f1, i, pp, z, fd, ne, no, m, n_seen, cnet, cnode, mi, sent, ai = 0;
   int last_board;
   char buff[80], wrp[80], c, found, *p, buffer[2050];
   char *location, *tag, *forw, need_origin, need_seen;
   byte tpbyte, lastr;
   struct _msghdr msghdr;
   struct _msghdr2 mhdr;
   struct _fwrd *seen;
   struct _gold_msghdr gmsghdr;
   NODEINFO ni;

   memset ((char *)&sys, 0, sizeof (struct _sys));

   sprintf (buff, "%sNODES.DAT", config->net_info);
   if ((fd = sh_open (buff, SH_DENYWR, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) != -1) {
      while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO))
         if (zone == ni.zone && net == ni.net && node == ni.node && point == ni.point) {
            ai = ni.aka;
            break;
         }

      close (fd);
   }

	if (goldboard) {
      sprintf (buff, "%sMSGHDR.DAT", fido_msgpath);
      f1 = sh_open (buff, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
      if (f1 == -1)
         return (0);

      gmsginfo.totalmsgs = filelength (f1) / sizeof (struct _gold_msghdr);

      sprintf (buff, "%sMSGTXT.DAT", fido_msgpath);
   }
   else {
      sprintf (buff, "%sMSGHDR.BBS", fido_msgpath);
      f1 = sh_open (buff, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
      if (f1 == -1)
         return (0);

      msginfo.totalmsgs = (word)(filelength (f1) / sizeof (struct _msghdr));

      sprintf(buff,"%sMSGTXT.BBS",fido_msgpath);
   }

   f2 = fopen (buff, "rb");
   if (f2 == NULL) {
      close (f1);
      return (0);
   }
   setvbuf (f2, NULL, _IOFBF, 1024);

   sprintf (buff, SYSMSG_PATH, config->sys_path);
   fd = sh_open (buff, SH_DENYNONE, O_RDONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
   if (fd == -1) {
      close (f1);
		fclose (f2);
      return (0);
   }

   if (config->use_areasbbs)
      fp = fopen (config->areas_bbs, "rt");
   else
      fp = NULL;

   last_board = 0;
   seen = (struct _fwrd *)malloc ((MAX_EXPORT_SEEN + 1) * sizeof (struct _fwrd));
   if (seen == NULL) {
		close (f1);
      fclose (f2);
      close (fd);
      if (fp != NULL)
         fclose (fp);
      return (0);
   }

   fpd = fopen ("MSGTMP.EXP", "rb+");
   if (fpd == NULL) {
      fpd = fopen ("MSGTMP.EXP", "wb");
      fclose (fpd);
   }
   else
      fclose (fpd);
   fpd = mopen ("MSGTMP.EXP", "r+b");

   prints (7, 65, YELLOW|_BLACK, "              ");
   prints (9, 65, YELLOW|_BLACK, "              ");
   prints (9, 65, YELLOW|_BLACK, goldboard ? "GoldBase" : "QuickBBS");

   sent = 0;

   for (;;) {
      if (goldboard) {
         if (read (f1, (char *)&gmsghdr, sizeof (struct _gold_msghdr)) != sizeof (struct _gold_msghdr))
            break;
			sprintf (wrp, "%ld / %ld", gmsghdr.msgnum, gmsginfo.totalmsgs);
			if ( !(gmsghdr.msgnum % 60))
            time_release ();
      }
      else {
			if (read (f1, (char *)&msghdr, sizeof (struct _msghdr)) != sizeof (struct _msghdr))
				break;
			sprintf (wrp, "%d / %d", msghdr.msgnum, msginfo.totalmsgs);
			if ( !(msghdr.msgnum % 60))
				time_release ();
		}
		prints (7, 65, YELLOW|_BLACK, wrp);

		if (goldboard) {
			if (gmsghdr.msgattr & Q_RECKILL)
				continue;
			if (gmsghdr.board != board)
				continue;
		}
		else {
			if (msghdr.msgattr & Q_RECKILL)
				continue;
			if (msghdr.board != board)
				continue;
		}

		if ((!goldboard && msghdr.board != last_board) || (goldboard && gmsghdr.board != last_board)) {
			last_board = goldboard ? gmsghdr.board : msghdr.board;
			memset ((char *)&sys, 0, sizeof (struct _sys));
			found = 0;

			if (fp != NULL) {
				rewind (fp);
				fgets (buffer, 255, fp);

				while (fgets(buffer, 255, fp) != NULL) {
					if (buffer[0] == ';')
						continue;
					if ((!goldboard && !isdigit (buffer[0])) || (goldboard && toupper (buffer[0]) != 'G'))
						continue;
					while (buffer[strlen (buffer) -1] == 0x0D || buffer[strlen (buffer) -1] == 0x0A || buffer[strlen (buffer) -1] == ' ')
						buffer[strlen (buffer) -1] = '\0';
					location = strtok (buffer, " ");
					tag = strtok (NULL, " ");
					forw = strtok (NULL, "");
					if ((!goldboard && atoi (location) == msghdr.board) || (goldboard && atoi (&location[1]) == gmsghdr.board)) {
						while (*forw == ' ')
							forw++;
						memset ((char *)&sys, 0, sizeof (struct _sys));
						strcpy (sys.echotag, tag);
						if (forw != NULL)
							strcpy (sys.forward1, forw);
						sys.echomail = 1;
						sys.use_alias = 0;
						if (goldboard)
							sys.gold_board = atoi (location);
						else
							sys.quick_board = atoi (location);
						found = 1;
						break;
					}
				}
         }

         if (!found) {
            for (i = 0; i < maxaidx; i++) {
               if (!goldboard && aidx[i].board == msghdr.board)
                  break;
               else if (goldboard && aidx[i].gold_board == gmsghdr.board)
                  break;
            }

            if (i < maxaidx) {
               lseek (fd, (long)i * SIZEOF_MSGAREA, SEEK_SET);
               read(fd, (char *)&sys.msg_name, SIZEOF_MSGAREA);
               found = 1;
            }
         }

         if (!found)
            continue;
      }
      else if (!found)
         continue;

      need_seen = need_origin = 1;

      strcpy (wrp, sys.echotag);
      wrp[14] = '\0';
      prints (8, 65, YELLOW|_BLACK, "              ");
      prints (8, 65, YELLOW|_BLACK, wrp);

      if (goldboard) {
			sprintf (wrp, "%5ld  %-22.22s GoldBase     ", gmsghdr.msgnum, sys.echotag);
         fseek (f2, 256L * gmsghdr.startblock, SEEK_SET);
      }
      else {
			sprintf (wrp, "%5d  %-22.22s QuickBBS     ", msghdr.msgnum, sys.echotag);
         fseek (f2, 256L * msghdr.startblock, SEEK_SET);
      }
      wputs (wrp);

      mseek (fpd, 0L, SEEK_SET);
      mprintf (fpd, "AREA:%s\r\n", sys.echotag);

		mi = i = 0;
      tpbyte = 0;
      lastr = 255;
      n_seen = 0;

      for (;;) {
         if (!tpbyte) {
            if (lastr != 255)
               break;
            tpbyte = fgetc(f2);
            lastr = tpbyte;
            if (!tpbyte)
               break;
         }

         c = fgetc (f2);
         tpbyte--;

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

                  if (n_seen + 1 >= MAX_EXPORT_SEEN)
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
            else {
               if ((!goldboard && (msghdr.msgattr & 64)) || (goldboard && (gmsghdr.msgattr & 64))) {
                  if (!strncmp (buff, "--- ", 4) && (config->replace_tear || !registered))
                     replace_tearline (fpd, buff);
                  else
                     mprintf (fpd, "%s\r\n", buff);
               }
               else
                  mprintf (fpd, "%s\r\n", buff);
            }

            mi = 0;
			}
         else {
            if(mi < 78)
               continue;

            if (!strncmp (buff, msgtxt[M_ORIGIN_LINE], 11))
               need_origin = 0;

            buff[mi] = '\0';
            mprintf (fpd, "%s", buff);
            mi = 0;
            buff[mi] = '\0';
         }
      }

      if (!n_seen || need_seen) {
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
      mhdr.attrib = 0;

      if (goldboard) {
         mhdr.cost = gmsghdr.cost;
         if (gmsghdr.msgattr & Q_PRIVATE)
            mhdr.attrib |= MSGPRIVATE;
      }
      else {
         mhdr.cost = msghdr.cost;
         if (msghdr.msgattr & Q_PRIVATE)
            mhdr.attrib |= MSGPRIVATE;
		}

      if (sys.public)
         mhdr.attrib &= ~Q_PRIVATE;
      else if (sys.private)
         mhdr.attrib |= Q_PRIVATE;

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

      if (goldboard) {
         strcpy (wrp, pascal_string (gmsghdr.date));
         sscanf (wrp, "%2d-%2d-%2d", &z, &ne, &no);
         strcpy (wrp, pascal_string (gmsghdr.time));
         sscanf (wrp, "%2d:%2d", &pp, &m);
         sprintf (buff, "%02d %s %02d  %2d:%02d:00", ne, mtext[z-1], no, pp, m);
         write (mi, buff, strlen (buff) + 1);
         strcpy (buff, pascal_string (gmsghdr.whoto));
         write (mi, buff, strlen (buff) + 1);
         strcpy (buff, pascal_string (gmsghdr.whofrom));
         write (mi, buff, strlen (buff) + 1);
         strcpy (buff, pascal_string (gmsghdr.subject));
         write (mi, buff, strlen (buff) + 1);
      }
      else {
         strcpy (wrp, pascal_string (msghdr.date));
         sscanf (wrp, "%2d-%2d-%2d", &z, &ne, &no);
         strcpy (wrp, pascal_string (msghdr.time));
			sscanf (wrp, "%2d:%2d", &pp, &m);
         sprintf (buff, "%02d %s %02d  %2d:%02d:00", ne, mtext[z-1], no, pp, m);
         write (mi, buff, strlen (buff) + 1);
         strcpy (buff, pascal_string (msghdr.whoto));
         write (mi, buff, strlen (buff) + 1);
         strcpy (buff, pascal_string (msghdr.whofrom));
         write (mi, buff, strlen (buff) + 1);
         strcpy (buff, pascal_string (msghdr.subject));
         write (mi, buff, strlen (buff) + 1);
      }

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

      wputs ("\n");
      sent++;
   }

   if (sent)
      status_line (":   %-20.20s (Sent=%04d)", sys.echotag, sent);

   if (fp != NULL)
      fclose (fp);
   mclose (fpd);
   free (seen);
   close (fd);
   fclose (f2);
   close (f1);
   unlink ("MSGTMP.EXP");

   return (0);
}

