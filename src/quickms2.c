
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
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "quickmsg.h"

#define MAX_MESSAGES    1000

char *pascal_string (char *);
int quick_msg_attrib(struct _msghdr *, int, int, int);
void quick_text_header(struct _msghdr *, int, FILE *);
int quick_full_read_message (int, struct _msghdr *, struct _gold_msghdr *, int);
void write_pascal_string (char *, char *, word *, word *, FILE *);
void quick_qwk_text_header (struct _msghdr *, int, FILE *, struct QWKmsghd *, long *);
void add_quote_string (char *str, char *from);
void add_quote_header (FILE *fp, char *from, char *to, char *subj, char *dt, char *tm);
int gold_msg_attrib(struct _gold_msghdr *, int, int, int);
void gold_qwk_text_header (struct _gold_msghdr *, int, FILE *, struct QWKmsghd *, long *);
void gold_text_header (struct _gold_msghdr *, int, FILE *);
void bluewave_header (int fdi, long startpos, long numbytes, int msgnum, struct _msg *msgt);

extern long *msgidx;
extern int msg_parent, msg_child;
extern struct _msginfo msginfo;
extern struct _gold_msginfo gmsginfo;
extern char *internet_to;

void quick_scan_message_base (board, gold_board, area, upd)
int board, gold_board, area;
char upd;
{
   #define MAXREADIDX   100
   int fd, tnum_msg, i, m, x;
   char filename[80];
   long gnum_msg;
   struct _msgidx idx[MAXREADIDX];
   struct _gold_msgidx gidx[MAXREADIDX];

   if (gold_board) {
      sprintf (filename, "%sMSGINFO.DAT", fido_msgpath);
      fd = sh_open (filename, SH_DENYWR, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
      read (fd, (char *)&gmsginfo, sizeof (struct _gold_msginfo));
      close (fd);
   }
   else {
      sprintf (filename, "%sMSGINFO.BBS", fido_msgpath);
      fd = sh_open (filename, SH_DENYWR, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
      read (fd, (char *)&msginfo, sizeof (struct _msginfo));
      close (fd);
   }

   msg_parent = msg_child = 0;
   tnum_msg = 0;
   gnum_msg = 0;
   i = 1;

   if (msgidx == NULL)
      msgidx = (long *)malloc (MAX_MESSAGES * sizeof (long));

   if (gold_board) {
      msgidx[0] = 0;
      gmsginfo.totalonboard[gold_board - 1] = 0;

      sprintf (filename, "%sMSGIDX.DAT", fido_msgpath);
   }
   else {
      msgidx[0] = 0;
      msginfo.totalonboard[board - 1] = 0;

      sprintf (filename, "%sMSGIDX.BBS", fido_msgpath);
   }

   fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);

   if (gold_board) {
      do {
         m = read (fd, (char *)&gidx, sizeof (struct _gold_msgidx) * MAXREADIDX);
         m /= sizeof (struct _gold_msgidx);

         for (x = 0; x < m; x++) {
				if (gidx[x].board == gold_board && gidx[x].msgnum > 0) {
               msgidx[i++] = gnum_msg;
               gmsginfo.totalonboard[gold_board - 1]++;
            }

            gnum_msg++;

            if (i >= MAX_MESSAGES)
               break;
         }
      } while (m == MAXREADIDX);

      tnum_msg = (int)gmsginfo.totalonboard[gold_board - 1];
   }
   else {
      do {
         m = read (fd, (char *)&idx, sizeof (struct _msgidx) * MAXREADIDX);
         m /= sizeof (struct _msgidx);

         for (x = 0; x < m; x++) {
				if (idx[x].board == board && idx[x].msgnum > 0 ) {
               msgidx[i++] = tnum_msg;
               msginfo.totalonboard[board - 1]++;
            }

            tnum_msg++;

            if (i >= MAX_MESSAGES)
               break;
         }
      } while (m == MAXREADIDX);

      tnum_msg = msginfo.totalonboard[board - 1];
   }

   close (fd);

   if (tnum_msg) {
      first_msg = 1;
      last_msg = tnum_msg;
      num_msg = tnum_msg;
   }
   else {
      first_msg = 0;
      last_msg = 0;
      num_msg = 0;
   }

   for (i = 0; i < MAXLREAD; i++)
      if (usr.lastread[i].area == area)
         break;
   if (i != MAXLREAD) {
      if (usr.lastread[i].msg_num > last_msg)
         usr.lastread[i].msg_num = last_msg;
      lastread = usr.lastread[i].msg_num;
   }
   else {
      for (i = 0; i < MAXDLREAD; i++)
         if (usr.dynlastread[i].area == area)
            break;
      if (i != MAXDLREAD) {
         if (usr.dynlastread[i].msg_num > last_msg)
            usr.dynlastread[i].msg_num = last_msg;
         lastread = usr.dynlastread[i].msg_num;
      }
      else if (upd) {
         lastread = 0;
         for (i = 1; i < MAXDLREAD; i++) {
            usr.dynlastread[i - 1].area = usr.dynlastread[i].area;
            usr.dynlastread[i - 1].msg_num = usr.dynlastread[i].msg_num;
         }

         usr.dynlastread[i - 1].area = area;
         usr.dynlastread[i - 1].msg_num = 0;
      }
      else
         lastread = 0;
   }

   if (lastread < 0)
      lastread = 0;
}

int quick_read_message (int msg_num, int flag, int fakenum)
{
   FILE *fp;
   int i, z, m, line, colf=0, pp, shead;
   char c, buff[150], wrp[150], *p, okludge;
   long fpos, absnum, startpos;
   struct _msghdr msghdr;
   struct _gold_msghdr gmsghdr;

   okludge = 0;
   line = 1;
   shead = 0;
   msg_fzone = msg_tzone = config->alias[sys.use_alias].zone;
   msg_fpoint = msg_tpoint = 0;
   absnum = msgidx[msg_num];

   if (sys.gold_board) {
      sprintf (buff, "%sMSGHDR.DAT", fido_msgpath);
      i = sh_open (buff, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
      if (i == -1)
         return (0);
      fp = fdopen (i, "r+b");
      if (fp == NULL)
         return (0);

      fseek (fp, (long)sizeof (struct _gold_msghdr) * absnum, SEEK_SET);
      fread ((char *)&gmsghdr, sizeof (struct _gold_msghdr), 1, fp);

		if (gmsghdr.msgattr & Q_RECKILL)
         return (0);

      if (gmsghdr.msgattr & Q_PRIVATE)
         if (stricmp (pascal_string (gmsghdr.whofrom), usr.name) && stricmp (pascal_string (gmsghdr.whoto), usr.name) && stricmp (pascal_string (gmsghdr.whofrom), usr.handle) && stricmp (pascal_string (gmsghdr.whoto), usr.handle) && usr.priv != SYSOP) {
            fclose (fp);
            return (0);
         }

      gmsghdr.timesread++;

      if (!stricmp (pascal_string (gmsghdr.whoto), usr.name) || !stricmp (pascal_string (gmsghdr.whoto), usr.handle))
         gmsghdr.msgattr |= Q_RECEIVED;

      fseek (fp, (long)sizeof (struct _gold_msghdr) * absnum, SEEK_SET);
      fwrite ((char *)&gmsghdr, sizeof (struct _gold_msghdr), 1, fp);

      fclose (fp);

      for (i = first_msg; i <= last_msg; i++)
         if (gmsghdr.prevreply == msgidx[i])
            break;
      if (i <= last_msg)
         msg_parent = i;

      for (i = first_msg; i <= last_msg; i++)
         if (gmsghdr.nextreply == msgidx[i])
            break;
      if (i <= last_msg)
         msg_child = i;
   }
   else {
      sprintf (buff, "%sMSGHDR.BBS", fido_msgpath);
      i = sh_open (buff, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
      if (i == -1)
         return (0);
      fp = fdopen (i, "r+b");
      if (fp == NULL)
         return (0);

      fseek (fp, (long)sizeof (struct _msghdr) * absnum, SEEK_SET);
      fread ((char *)&msghdr, sizeof (struct _msghdr), 1, fp);

		if (msghdr.msgattr & Q_RECKILL)
         return (0);

      if (msghdr.msgattr & Q_PRIVATE)
         if (stricmp (pascal_string (msghdr.whofrom), usr.name) && stricmp (pascal_string (msghdr.whoto), usr.name) &&
         stricmp (pascal_string (msghdr.whofrom), usr.handle) && stricmp (pascal_string (msghdr.whoto), usr.handle) && usr.priv != SYSOP) {
            fclose (fp);
            return (0);
         }

      msghdr.timesread++;

      if (!stricmp (pascal_string (msghdr.whoto), usr.name)||!stricmp(pascal_string (msghdr.whoto), usr.handle))
         msghdr.msgattr |= Q_RECEIVED;

      fseek (fp, (long)sizeof (struct _msghdr) * absnum, SEEK_SET);
      fwrite ((char *)&msghdr, sizeof(struct _msghdr), 1, fp);

      fclose (fp);

      for (i = first_msg; i <= last_msg; i++)
         if (msghdr.prevreply == msgidx[i])
            break;
      if (i <= last_msg)
         msg_parent = i;

      for (i = first_msg; i <= last_msg; i++)
         if (msghdr.nextreply == msgidx[i])
            break;
      if (i <= last_msg)
         msg_child = i;
   }

   allow_reply = 1;

   if (!flag)
      cls ();

   if (usr.full_read && !flag) {
      if (sys.gold_board)
         return (quick_full_read_message (msg_num, NULL, &gmsghdr, fakenum));
      else
         return (quick_full_read_message (msg_num, &msghdr, NULL, fakenum));
   }

   if (flag)
      m_print (bbstxt[B_TWO_CR]);

   if (sys.gold_board) {
      fpos = 256L * (gmsghdr.startblock + gmsghdr.numblocks - 1);
      sprintf (buff, "%sMSGTXT.DAT", fido_msgpath);
   }
   else {
      fpos = 256L * (msghdr.startblock + msghdr.numblocks - 1);
      sprintf (buff, "%sMSGTXT.BBS", fido_msgpath);
   }
   i = sh_open (buff, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
   fp = fdopen (i,"rb");

   fseek (fp, fpos, SEEK_SET);
   fpos += fgetc (fp);

   if (sys.gold_board)
      startpos = 256L * gmsghdr.startblock;
   else
      startpos = 256L * msghdr.startblock;

   fseek (fp, startpos, SEEK_SET);

   change_attr (WHITE|_BLACK);
   m_print (" * %s\n", sys.msg_name);

   change_attr (CYAN|_BLACK);

   i = 0;
   pp = 0;

   while (ftell(fp) < fpos) {
      if (!pp)
         fgetc (fp);

      c = fgetc (fp);
      pp++;

      if (c == '\0')
         break;

      if (pp == 255)
         pp = 0;

      if ((byte)c == 0x8D || c == 0x0A || c == '\0')
         continue;

      buff[i++] = c;

      if (c == 0x0D) {
         buff[i - 1] = '\0';

         if(buff[0] == 0x01 && !okludge) {
            if (!strncmp(&buff[1],"INTL",4) && !shead)
               sscanf(&buff[6],"%d:%d/%d %d:%d/%d",&msg_tzone,&i,&i,&msg_fzone,&i,&i);
            if (!strncmp(&buff[1],"TOPT",4) && !shead)
               sscanf(&buff[6],"%d",&msg_tpoint);
            if (!strncmp(&buff[1],"FMPT",4) && !shead)
               sscanf(&buff[6],"%d",&msg_fpoint);
            i = 0;
            continue;
         }
         else if (!shead) {
            if (sys.gold_board)
               line = gold_msg_attrib (&gmsghdr, fakenum ? fakenum : msg_num, line, 0);
            else
               line = quick_msg_attrib (&msghdr, fakenum ? fakenum : msg_num, line, 0);
            m_print (bbstxt[B_ONE_CR]);
            shead = 1;
            okludge = 1;
            fseek (fp, startpos, SEEK_SET);
            pp = 0;
            i = 0;
            continue;
         }

         if (!usr.kludge && (!strncmp (buff, "SEEN-BY", 7) || buff[0] == 0x01)) {
            i = 0;
            continue;
         }

         if (buff[0] == 0x01) {
            m_print ("\026\001\013%s\n", &buff[1]);
            change_attr (CYAN|BLACK);
         }
         else if (!strncmp (buff, "SEEN-BY", 7)) {
            m_print ("\026\001\013%s\n", buff);
            change_attr (LGREY|BLACK);
         }
         else {
            p = &buff[0];

            if(((strchr(buff,'>') - p) < 6) && (strchr(buff,'>'))) {
               if(!colf) {
                  change_attr(WHITE|BLACK);
                  colf=1;
               }
            }
            else {
               if(colf) {
                  change_attr(CYAN|_BLACK);
                  colf=0;
               }
            }

            if (!strncmp(buff,msgtxt[M_TEAR_LINE],4) || !strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
               m_print("\026\001\002%s\026\001\007\n",buff);
            else
               m_print("%s\n",buff);

            if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
               gather_origin_netnode (buff);
         }

         i=0;

         if(flag == 1)
            line=1;

         if(!(++line < (usr.len-1)) && usr.more) {
            if(!continua()) {
               flag=1;
               break;
            }
            else
               line=1;
         }
      }
      else {
         if(i<(usr.width-1))
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

         if (!shead) {
            if (sys.gold_board)
               line = gold_msg_attrib (&gmsghdr, fakenum ? fakenum : msg_num, line, 0);
            else
               line = quick_msg_attrib (&msghdr, fakenum ? fakenum : msg_num, line, 0);
            m_print(bbstxt[B_ONE_CR]);
            shead = 1;
            okludge = 1;
            fseek (fp, startpos, SEEK_SET);
            pp = 0;
            i = 0;
            continue;
         }

         if (((strchr(buff,'>') - buff) < 6) && (strchr(buff,'>'))) {
            if(!colf) {
               change_attr(WHITE|_BLACK);
               colf=1;
            }
         }
         else {
            if(colf) {
               change_attr(CYAN|_BLACK);
               colf=0;
            }
         }

         if (!strncmp(buff,msgtxt[M_TEAR_LINE],4) || !strncmp(buff,msgtxt[M_ORIGIN_LINE],10))
            m_print("\026\001\002%s\026\001\007\n",buff);
         else
            m_print("%s\n",buff);

         if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
            gather_origin_netnode (buff);

         if(flag == 1)
            line=1;

         if(!(++line < (usr.len-1)) && usr.more) {
            if(!continua()) {
               flag=1;
               break;
            }
            else
               line=1;
         }

         strcpy(buff,wrp);
         i=strlen(wrp);
      }
   }
   fclose(fp);

   if (!shead) {
      if (sys.gold_board)
         line = gold_msg_attrib (&gmsghdr, msg_num, line, 0);
      else
         line = quick_msg_attrib (&msghdr, msg_num, line, 0);
      m_print (bbstxt[B_ONE_CR]);
      shead = 1;
   }

   if (line > 1 && !flag && usr.more) {
      press_enter ();
      line = 1;
   }

   return (1);
}

int quick_full_read_message (msg_num, msghdr, gmsghdr, fakenum)
int msg_num;
struct _msghdr *msghdr;
struct _gold_msghdr *gmsghdr;
int fakenum;
{
   FILE *fp;
   int i, z, m, line, colf=0, pp, shead;
   char c, buff[150], wrp[150], *p, okludge;
   long fpos, startpos;

   okludge = 0;
   line = 2;
   shead = 0;
   msg_fzone = msg_tzone = config->alias[sys.use_alias].zone;
   msg_fpoint = msg_tpoint = 0;

   if (gmsghdr != NULL) {
      sprintf (buff, "%sMSGTXT.DAT", fido_msgpath);
      if ((i = sh_open (buff, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) == -1)
         return (1);
      if ((fp = fdopen (i, "rb")) == NULL) {
         close (i);
         return (1);
      }

      fpos = 256L * (gmsghdr->startblock + gmsghdr->numblocks - 1);
      fseek (fp, fpos, SEEK_SET);
      fpos += fgetc (fp);

      startpos = 256L * gmsghdr->startblock;
   }
   else {
      sprintf (buff, "%sMSGTXT.BBS", fido_msgpath);
      i = sh_open (buff, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
      fp = fdopen (i,"rb");

      fpos = 256L * (msghdr->startblock + msghdr->numblocks - 1);
      fseek (fp, fpos, SEEK_SET);
      fpos += fgetc (fp);

      startpos = 256L * msghdr->startblock;
   }

   fseek (fp, startpos, SEEK_SET);

   change_attr (BLUE|_LGREY);
   del_line ();
   m_print (" * %s\n", sys.msg_name);

   i = 0;
   pp = 0;

   while (ftell (fp) < fpos) {
      if (!pp)
         fgetc (fp);

      c = fgetc (fp);
      pp++;

      if (c == '\0')
         break;

      if (pp == 255)
         pp = 0;

      if ((byte)c == 0x8D || c == 0x0A || c == '\0')
         continue;

      buff[i++] = c;

      if (c == 0x0D) {
         buff[i-1]='\0';

         if (buff[0] == 0x01 && !okludge) {
            if (!strncmp(&buff[1],"INTL",4) && !shead)
               sscanf(&buff[6],"%d:%d/%d %d:%d/%d",&msg_tzone,&i,&i,&msg_fzone,&i,&i);
            if (!strncmp(&buff[1],"TOPT",4) && !shead)
               sscanf(&buff[6],"%d",&msg_tpoint);
            if (!strncmp(&buff[1],"FMPT",4) && !shead)
               sscanf(&buff[6],"%d",&msg_fpoint);
            i = 0;
            continue;
         }
         else if (!shead) {
            if (gmsghdr != NULL)
               line = gold_msg_attrib (gmsghdr, fakenum ? fakenum : msg_num, line, 0);
            else
               line = quick_msg_attrib (msghdr, fakenum ? fakenum : msg_num, line, 0);
            change_attr(LGREY|_BLUE);
            del_line();
            change_attr(CYAN|_BLACK);
            m_print(bbstxt[B_ONE_CR]);
            shead = 1;
            okludge = 1;
            fseek (fp, startpos, SEEK_SET);
            pp = 0;
            i = 0;
            continue;
         }

         if (!usr.kludge && (!strncmp (buff, "SEEN-BY", 7) || buff[0] == 0x01)) {
            i = 0;
            continue;
         }

         if (buff[0] == 0x01) {
            m_print ("\026\001\013%s\n", &buff[1]);
            change_attr (CYAN|BLACK);
         }
         else if (!strncmp (buff, "SEEN-BY", 7)) {
            m_print ("\026\001\013%s\n", buff);
            change_attr (LGREY|BLACK);
         }
         else {
            p = &buff[0];

            if(((strchr(buff,'>') - p) < 6) && (strchr(buff,'>'))) {
               if(!colf) {
                  change_attr(WHITE|BLACK);
                  colf=1;
               }
            }
            else {
               if(colf) {
                  change_attr(CYAN|_BLACK);
                  colf=0;
               }
            }

            if (!strncmp(buff,msgtxt[M_TEAR_LINE],4) || !strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
               m_print("\026\001\002%s\026\001\007\n",buff);
            else
               m_print("%s\n",buff);

            if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
               gather_origin_netnode (buff);
         }

         i=0;

         if(!(++line < (usr.len-1)) && usr.more) {
            if(!continua()) {
               line = 1;
               break;
            }
            else {
               while (line > 5) {
                  cpos(line--, 1);
                  del_line();
               }
            }
         }
      }
      else {
         if(i<(usr.width-1))
            continue;

         buff[i]='\0';
         while(i>0 && buff[i] != ' ')
            i--;

         m=0;

         if(i != 0)
            for(z=i+1;buff[z];z++)
               wrp[m++]=buff[z];

         buff[i]='\0';
         wrp[m]='\0';

         if (!shead) {
            if (gmsghdr != NULL)
               line = gold_msg_attrib (gmsghdr, fakenum ? fakenum : msg_num, line, 0);
            else
               line = quick_msg_attrib (msghdr, fakenum ? fakenum : msg_num, line, 0);
            change_attr (LGREY|_BLUE);
            del_line ();
            change_attr (CYAN|_BLACK);
            m_print (bbstxt[B_ONE_CR]);
            shead = 1;
            okludge = 1;
            fseek (fp, startpos, SEEK_SET);
            pp = 0;
            i = 0;
            continue;
         }

         if (((strchr(buff,'>') - buff) < 6) && (strchr(buff,'>'))) {
            if(!colf) {
               change_attr(WHITE|_BLACK);
               colf=1;
            }
         }
         else {
            if(colf) {
               change_attr(CYAN|_BLACK);
               colf=0;
            }
         }

         if (!strncmp (buff, msgtxt[M_TEAR_LINE], 4) || !strncmp (buff, msgtxt[M_ORIGIN_LINE], 10))
            m_print ("\026\001\002%s\026\001\007\n", buff);
         else
            m_print ("%s\n", buff);

         if (!strncmp (buff, msgtxt[M_ORIGIN_LINE], 11))
            gather_origin_netnode (buff);

         if (!(++line < (usr.len - 1)) && usr.more) {
            if(!continua ()) {
               line = 1;
               break;
            }
            else {
               while (line > 5) {
                  cpos (line--, 1);
                  del_line ();
               }
            }
         }

         strcpy (buff, wrp);
         i = strlen (wrp);
      }
   }

   fclose (fp);

   if (!shead) {
      if (gmsghdr != NULL)
         line = gold_msg_attrib (gmsghdr, msg_num, line, 0);
      else
         line = quick_msg_attrib (msghdr, msg_num, line, 0);
      m_print (bbstxt[B_ONE_CR]);
      shead = 1;
   }

   if (line > 1 && usr.more) {
      press_enter ();
      line = 1;
   }

   return (1);
}

int quick_msg_attrib (msgt, s, line, f)
struct _msghdr *msgt;
int s, line, f;
{
   int dy, mo, yr, hh, mm;
   char stringa[20];
   struct _msg tmsg;

   memset ((char *)&tmsg, 0, sizeof (struct _msg));

   if (msgt->msgattr & Q_PRIVATE)
      tmsg.attr |= MSGPRIVATE;
   if(msgt->msgattr & Q_RECEIVED)
      tmsg.attr |= MSGREAD;
   if(msgt->netattr & Q_CRASH)
      tmsg.attr |= MSGCRASH;
   if(msgt->netattr & Q_SENT)
      tmsg.attr |= MSGSENT;
   if(msgt->netattr & Q_FILE)
      tmsg.attr |= MSGFILE;
   if(msgt->netattr & Q_KILL)
      tmsg.attr |= MSGKILL;
   if(msgt->netattr & Q_FRQ)
      tmsg.attr |= MSGFRQ;
   if(msgt->netattr & Q_CPT)
      tmsg.attr |= MSGCPT;
   if(msgt->netattr & Q_ARQ)
      tmsg.attr |= MSGARQ;

   strcpy (tmsg.to, pascal_string (msgt->whoto));
   strcpy (tmsg.from, pascal_string (msgt->whofrom));
   strcpy (tmsg.subj, pascal_string (msgt->subject));

   strcpy(stringa,pascal_string(msgt->date));
   sscanf(stringa,"%2d-%2d-%2d",&mo,&dy,&yr);
   strcpy(stringa,pascal_string(msgt->time));
   sscanf(stringa,"%2d:%2d",&hh,&mm);
   sprintf(tmsg.date, "%02d %s %02d  %2d:%02d:00", dy, mtext[mo-1], yr, hh, mm);

   line = msg_attrib (&tmsg, s, line, f);
   memcpy ((char *)&msg, (char *)&tmsg, sizeof (struct _msg));

   return (line);
}

int gold_msg_attrib (msgt, s, line, f)
struct _gold_msghdr *msgt;
int s, line, f;
{
   int dy, mo, yr, hh, mm;
   char stringa[20];
   struct _msg tmsg;

   memset ((char *)&tmsg, 0, sizeof (struct _msg));

   if (msgt->msgattr & Q_PRIVATE)
      tmsg.attr |= MSGPRIVATE;
   if(msgt->msgattr & Q_RECEIVED)
      tmsg.attr |= MSGREAD;
   if(msgt->netattr & Q_CRASH)
      tmsg.attr |= MSGCRASH;
   if(msgt->netattr & Q_SENT)
      tmsg.attr |= MSGSENT;
   if(msgt->netattr & Q_FILE)
      tmsg.attr |= MSGFILE;
   if(msgt->netattr & Q_KILL)
      tmsg.attr |= MSGKILL;
   if(msgt->netattr & Q_FRQ)
      tmsg.attr |= MSGFRQ;
   if(msgt->netattr & Q_CPT)
      tmsg.attr |= MSGCPT;
   if(msgt->netattr & Q_ARQ)
      tmsg.attr |= MSGARQ;

   strcpy (tmsg.to, pascal_string (msgt->whoto));
   strcpy (tmsg.from, pascal_string (msgt->whofrom));
   strcpy (tmsg.subj, pascal_string (msgt->subject));

   strcpy(stringa,pascal_string(msgt->date));
   sscanf(stringa,"%2d-%2d-%2d",&mo,&dy,&yr);
   strcpy(stringa,pascal_string(msgt->time));
   sscanf(stringa,"%2d:%2d",&hh,&mm);
   sprintf(tmsg.date, "%02d %s %02d  %2d:%02d:00", dy, mtext[mo-1], yr, hh, mm);

   line = msg_attrib (&tmsg, s, line, f);
   memcpy ((char *)&msg, (char *)&tmsg, sizeof (struct _msg));

   return (line);
}

void quick_text_header(msgt, s, fp)
struct _msghdr *msgt;
int s;
FILE *fp;
{
   int dy, mo, yr, hh, mm;
   char stringa[20];
   struct _msg tmsg;

   memset ((char *)&tmsg, 0, sizeof (struct _msg));

   if (msgt->msgattr & Q_PRIVATE)
      tmsg.attr |= MSGPRIVATE;
   if(msgt->msgattr & Q_RECEIVED)
      tmsg.attr |= MSGREAD;
   if(msgt->netattr & Q_CRASH)
      tmsg.attr |= MSGCRASH;
   if(msgt->netattr & Q_SENT)
      tmsg.attr |= MSGSENT;
   if(msgt->netattr & Q_FILE)
      tmsg.attr |= MSGFILE;
   if(msgt->netattr & Q_KILL)
      tmsg.attr |= MSGKILL;
   if(msgt->netattr & Q_FRQ)
      tmsg.attr |= MSGFRQ;
   if(msgt->netattr & Q_CPT)
      tmsg.attr |= MSGCPT;
   if(msgt->netattr & Q_ARQ)
      tmsg.attr |= MSGARQ;

   strcpy (tmsg.to, pascal_string (msgt->whoto));
   strcpy (tmsg.from, pascal_string (msgt->whofrom));
   strcpy (tmsg.subj, pascal_string (msgt->subject));

   strcpy(stringa,pascal_string(msgt->date));
   sscanf(stringa,"%2d-%2d-%2d",&mo,&dy,&yr);
   strcpy(stringa,pascal_string(msgt->time));
   sscanf(stringa,"%2d:%2d",&hh,&mm);
   sprintf(tmsg.date, "%02d %s %02d  %2d:%02d:00", dy, mtext[mo-1], yr, hh, mm);

   text_header (&tmsg, s, fp);
}

void gold_text_header(msgt, s, fp)
struct _gold_msghdr *msgt;
int s;
FILE *fp;
{
   int dy, mo, yr, hh, mm;
   char stringa[20];
   struct _msg tmsg;

   memset ((char *)&tmsg, 0, sizeof (struct _msg));

   if (msgt->msgattr & Q_PRIVATE)
      tmsg.attr |= MSGPRIVATE;
   if(msgt->msgattr & Q_RECEIVED)
      tmsg.attr |= MSGREAD;
   if(msgt->netattr & Q_CRASH)
      tmsg.attr |= MSGCRASH;
   if(msgt->netattr & Q_SENT)
      tmsg.attr |= MSGSENT;
   if(msgt->netattr & Q_FILE)
      tmsg.attr |= MSGFILE;
   if(msgt->netattr & Q_KILL)
      tmsg.attr |= MSGKILL;
   if(msgt->netattr & Q_FRQ)
      tmsg.attr |= MSGFRQ;
   if(msgt->netattr & Q_CPT)
      tmsg.attr |= MSGCPT;
   if(msgt->netattr & Q_ARQ)
      tmsg.attr |= MSGARQ;

   strcpy (tmsg.to, pascal_string (msgt->whoto));
   strcpy (tmsg.from, pascal_string (msgt->whofrom));
   strcpy (tmsg.subj, pascal_string (msgt->subject));

   strcpy(stringa,pascal_string(msgt->date));
   sscanf(stringa,"%2d-%2d-%2d",&mo,&dy,&yr);
   strcpy(stringa,pascal_string(msgt->time));
   sscanf(stringa,"%2d:%2d",&hh,&mm);
   sprintf(tmsg.date, "%02d %s %02d  %2d:%02d:00", dy, mtext[mo-1], yr, hh, mm);

   text_header (&tmsg, s, fp);
}

char *pascal_string(s)
char *s;
{
   strncpy(e_input,(char *)&s[1],(int)s[0]);
   e_input[(int)s[0]] = '\0';

   return (e_input);
}

int quick_write_message_text(msgn, flags, stringa, sm)
int msgn, flags;
char *stringa;
FILE *sm;
{
   FILE *fp, *fpt;
   int i, m, z, pp, blks, pos;
   char filename[80], c, buff[80], wrp[80], shead, qwkbuffer[130];
   char qfrom[36], qto[36], qtime[10], qdate[10], qsubj[72], *p;
   long fpos, qpos, absnum;
   struct _msghdr msgt;
   struct _gold_msghdr gmsgt;
   struct QWKmsghd QWK;

   absnum = msgidx[msgn];

   if (sys.gold_board) {
      sprintf (filename, "%sMSGHDR.DAT", fido_msgpath);
      fp = sh_fopen (filename, "r+b", SH_DENYNONE);
      if (fp == NULL)
         return (0);

      fseek (fp,(long)sizeof (struct _gold_msghdr) * absnum, SEEK_SET);
      fread ((char *)&gmsgt, sizeof (struct _gold_msghdr), 1, fp);

      fclose (fp);

		if (gmsgt.msgattr & Q_RECKILL)
         return (0);

      if (gmsgt.msgattr & Q_PRIVATE) {
         if (stricmp (pascal_string (gmsgt.whofrom), usr.name) && stricmp(pascal_string (gmsgt.whoto), usr.name)&&
         stricmp (pascal_string (gmsgt.whofrom), usr.handle) && stricmp(pascal_string (gmsgt.whoto), usr.handle))
         return (0);
      }
   }
   else {
      sprintf (filename, "%sMSGHDR.BBS", fido_msgpath);
      fp = sh_fopen (filename, "r+b", SH_DENYNONE);
      if (fp == NULL)
         return (0);

      fseek (fp, (long)sizeof (struct _msghdr) * absnum, SEEK_SET);
      fread ((char *)&msgt, sizeof (struct _msghdr), 1, fp);

      fclose (fp);

		if (msgt.msgattr & Q_RECKILL)
         return (0);

      if (msgt.msgattr & Q_PRIVATE) {
         if (stricmp (pascal_string (msgt.whofrom), usr.name) && stricmp(pascal_string (msgt.whoto), usr.name)
         &&stricmp (pascal_string (msgt.whofrom), usr.handle) && stricmp(pascal_string (msgt.whoto), usr.handle))
               return (0);
      }
   }

   blks = 1;
   shead = 0;
   pos = 0;

   if (sm == NULL) {
      fpt = fopen(stringa, (flags & APPEND_TEXT) ? "at" : "wt");
      if (fpt == NULL)
         return (0);
   }
   else
      fpt = sm;

   if (sys.gold_board) {
      sprintf (buff, "%sMSGTXT.DAT",  fido_msgpath);
      fp = sh_fopen (buff, "rb", SH_DENYNONE);

      fpos = 256L * (gmsgt.startblock + gmsgt.numblocks - 1);
      fseek (fp, fpos, SEEK_SET);
      fpos += fgetc (fp);

      fseek (fp, 256L * gmsgt.startblock, SEEK_SET);
   }
   else {
      sprintf (buff, "%sMSGTXT.BBS",  fido_msgpath);
      fp = sh_fopen (buff, "rb", SH_DENYNONE);

      fpos = 256L * (msgt.startblock + msgt.numblocks - 1);
      fseek (fp, fpos, SEEK_SET);
      fpos += fgetc (fp);

      fseek (fp, 256L * msgt.startblock, SEEK_SET);
   }

   i = 0;

   if (flags & QUOTE_TEXT) {
      if (sys.gold_board)
         add_quote_string (buff, pascal_string (gmsgt.whofrom));
      else
         add_quote_string (buff, pascal_string (gmsgt.whofrom));
      i = strlen (buff);
   }
   pp = 0;

   while (ftell (fp) < fpos) {
      if (!pp)
         fgetc (fp);

      c = fgetc (fp);
      pp++;

      if (c == '\0')
         break;

      if (pp == 255)
         pp = 0;

      if ((byte)c == 0x8D || c == 0x0A || c == '\0')
         continue;

      buff[i++] = c;

      if (c == 0x0D) {
         buff[i-1] = '\0';
         if (!strnicmp (buff, msgtxt[M_TEAR_LINE], 4))
            buff[1] = '+';
         if (!strnicmp (buff, msgtxt[M_ORIGIN_LINE], 10))
            buff[3] = '0';

         if ((p = strchr (buff, 0x01)) != NULL) {
            if (!strncmp(&p[1], "INTL",4) && !shead)
               sscanf (&p[6], "%d:%d/%d %d:%d/%d", &msg_tzone, &i, &i, &msg_fzone, &i, &i);
            if (!strncmp(&p[1],"TOPT",4) && !shead)
               sscanf (&p[6], "%d",&msg_tpoint);
            if (!strncmp (&p[1], "FMPT",4) && !shead)
               sscanf (&p[6], "%d",&msg_fpoint);
         }
         else if (!shead) {
            if (flags & INCLUDE_HEADER) {
               if (sys.gold_board)
                  gold_text_header (&gmsgt, msgn, fpt);
               else
                  quick_text_header (&msgt, msgn, fpt);
            }
            else if (flags & QWK_TEXTFILE) {
               if (sys.gold_board)
                  gold_qwk_text_header (&gmsgt, msgn, fpt, &QWK, &qpos);
               else
                  quick_qwk_text_header (&msgt, msgn, fpt, &QWK, &qpos);
            }
            else if (flags & QUOTE_TEXT) {
               if (sys.gold_board) {
                  strcpy (qfrom, pascal_string (gmsgt.whofrom));
                  strcpy (qto, pascal_string (gmsgt.whoto));
                  strcpy (qdate, pascal_string (gmsgt.date));
                  strcpy (qtime, pascal_string (gmsgt.time));
                  strcpy (qsubj, pascal_string (gmsgt.subject));
               }
               else {
                  strcpy (qfrom, pascal_string (msgt.whofrom));
                  strcpy (qto, pascal_string (msgt.whoto));
                  strcpy (qdate, pascal_string (msgt.date));
                  strcpy (qtime, pascal_string (msgt.time));
                  strcpy (qsubj, pascal_string (msgt.subject));
               }
               add_quote_header (fpt, qfrom, qto, qsubj, qdate, qtime);
            }
            shead = 1;
         }

         if (strchr (buff, 0x01) != NULL || stristr (buff, "SEEN-BY") != NULL) {
            if (flags & QUOTE_TEXT) {
               if (sys.gold_board)
                  add_quote_string (buff, pascal_string (gmsgt.whofrom));
               else
                  add_quote_string (buff, pascal_string (gmsgt.whofrom));
               i = strlen (buff);
            }
            else
               i = 0;
            continue;
         }

         if (flags & QWK_TEXTFILE) {
            write_qwk_string (buff, qwkbuffer, &pos, &blks, fpt);
            write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpt);
         }
         else
            fprintf (fpt, "%s\n", buff);

         if (flags & QUOTE_TEXT) {
            if (sys.gold_board)
               add_quote_string (buff, pascal_string (gmsgt.whofrom));
            else
               add_quote_string (buff, pascal_string (gmsgt.whofrom));
            i = strlen (buff);
         }
         else
            i = 0;
      }
      else {
         if (i < (usr.width-1))
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

         if (!shead) {
            if (flags & INCLUDE_HEADER) {
               if (sys.gold_board)
                  gold_text_header (&gmsgt, msgn, fpt);
               else
                  quick_text_header (&msgt, msgn, fpt);
            }
            else if (flags & QWK_TEXTFILE) {
               if (sys.gold_board)
                  gold_qwk_text_header (&gmsgt, msgn, fpt, &QWK, &qpos);
               else
                  quick_qwk_text_header (&msgt, msgn, fpt, &QWK, &qpos);
            }
            else if (flags & QUOTE_TEXT) {
               if (sys.gold_board) {
                  strcpy (qfrom, pascal_string (gmsgt.whofrom));
                  strcpy (qto, pascal_string (gmsgt.whoto));
                  strcpy (qdate, pascal_string (gmsgt.date));
                  strcpy (qtime, pascal_string (gmsgt.time));
                  strcpy (qsubj, pascal_string (gmsgt.subject));
               }
               else {
                  strcpy (qfrom, pascal_string (msgt.whofrom));
                  strcpy (qto, pascal_string (msgt.whoto));
                  strcpy (qdate, pascal_string (msgt.date));
                  strcpy (qtime, pascal_string (msgt.time));
                  strcpy (qsubj, pascal_string (msgt.subject));
               }
               add_quote_header (fpt, qfrom, qto, qsubj, qdate, qtime);
            }
            shead = 1;
         }

         if (flags & QWK_TEXTFILE) {
            write_qwk_string (buff, qwkbuffer, &pos, &blks, fpt);
            write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpt);
         }
         else
            fprintf (fpt, "%s\n", buff);

         if (flags & QUOTE_TEXT) {
            if (sys.gold_board)
               add_quote_string (buff, pascal_string (gmsgt.whofrom));
            else
               add_quote_string (buff, pascal_string (gmsgt.whofrom));
         }
         else
            buff[0] = '\0';
         strcat (buff, wrp);
         i = strlen (buff);
      }
   }

   fclose (fp);

   if (flags & QWK_TEXTFILE) {
      fwrite (qwkbuffer, 128, 1, fpt);
      blks++;

      fseek (fpt, qpos, SEEK_SET);         /* Restore back to header start */
      sprintf (buff, "%d", blks);
      ljstring (QWK.Msgrecs, buff, 6);
      fwrite ((char *)&QWK, 128, 1, fpt);  /* Write out the header */
      fseek (fpt, 0L, SEEK_END);           /* Bump back to end of file */

      if (sm == NULL)
         fclose (fpt);
      return (blks);
   }
   else
      fprintf (fpt, bbstxt[B_TWO_CR]);

   if (sm == NULL)
      fclose (fpt);

   return (1);
}

void quick_qwk_text_header (msgt,msgn,fpt,QWK,qpos)
struct _msghdr *msgt;
int msgn;
FILE *fpt;
struct QWKmsghd *QWK;
long *qpos;
{
   int dy, mo, yr, hh, mm;
   char stringa[20];
   struct _msg tmsg;

   memset ((char *)&tmsg, 0, sizeof (struct _msg));

   if (msgt->msgattr & Q_PRIVATE)
      tmsg.attr |= MSGPRIVATE;
   if(msgt->msgattr & Q_RECEIVED)
      tmsg.attr |= MSGREAD;
   strcpy (tmsg.to, pascal_string (msgt->whoto));
   strcpy (tmsg.from, pascal_string (msgt->whofrom));
   strcpy (tmsg.subj, pascal_string (msgt->subject));

   strcpy(stringa,pascal_string(msgt->date));
   sscanf(stringa,"%2d-%2d-%2d",&mo,&dy,&yr);
   strcpy(stringa,pascal_string(msgt->time));
   sscanf(stringa,"%2d:%2d",&hh,&mm);
   sprintf(tmsg.date, "%02d %s %02d  %2d:%02d:00", dy, mtext[mo-1], yr, hh, mm);

   qwk_header (&tmsg, QWK, msgn, fpt, qpos);
}

void quick_bluewave_text_header (struct _msghdr *msgt, int msgn, int fdi, long bw_start, long bw_len)
{
   int dy, mo, yr, hh, mm;
   char stringa[20];
   struct _msg tmsg;

   memset ((char *)&tmsg, 0, sizeof (struct _msg));

   if (msgt->msgattr & Q_PRIVATE)
      tmsg.attr |= MSGPRIVATE;
   if(msgt->msgattr & Q_RECEIVED)
      tmsg.attr |= MSGREAD;
   strcpy (tmsg.to, pascal_string (msgt->whoto));
   strcpy (tmsg.from, pascal_string (msgt->whofrom));
   strcpy (tmsg.subj, pascal_string (msgt->subject));

   strcpy(stringa,pascal_string(msgt->date));
   sscanf(stringa,"%2d-%2d-%2d",&mo,&dy,&yr);
   strcpy(stringa,pascal_string(msgt->time));
   sscanf(stringa,"%2d:%2d",&hh,&mm);
   sprintf(tmsg.date, "%02d %s %02d  %2d:%02d:00", dy, mtext[mo-1], yr, hh, mm);

   bluewave_header (fdi, bw_start, bw_len, msgn, &tmsg);
}

void gold_qwk_text_header (msgt, msgn, fpt, QWK, qpos)
struct _gold_msghdr *msgt;
int msgn;
FILE *fpt;
struct QWKmsghd *QWK;
long *qpos;
{
   int dy, mo, yr, hh, mm;
   char stringa[20];
   struct _msg tmsg;

   memset ((char *)&tmsg, 0, sizeof (struct _msg));

   if (msgt->msgattr & Q_PRIVATE)
      tmsg.attr |= MSGPRIVATE;
   if(msgt->msgattr & Q_RECEIVED)
      tmsg.attr |= MSGREAD;
   strcpy (tmsg.to, pascal_string (msgt->whoto));
   strcpy (tmsg.from, pascal_string (msgt->whofrom));
   strcpy (tmsg.subj, pascal_string (msgt->subject));

   strcpy(stringa,pascal_string(msgt->date));
   sscanf(stringa,"%2d-%2d-%2d",&mo,&dy,&yr);
   strcpy(stringa,pascal_string(msgt->time));
   sscanf(stringa,"%2d:%2d",&hh,&mm);
   sprintf(tmsg.date, "%02d %s %02d  %2d:%02d:00", dy, mtext[mo-1], yr, hh, mm);

   qwk_header (&tmsg, QWK, msgn, fpt, qpos);
}

void gold_bluewave_text_header (struct _gold_msghdr *msgt, int msgn, int fdi, long bw_start, long bw_len)
{
   int dy, mo, yr, hh, mm;
   char stringa[20];
   struct _msg tmsg;

   memset ((char *)&tmsg, 0, sizeof (struct _msg));

   if (msgt->msgattr & Q_PRIVATE)
      tmsg.attr |= MSGPRIVATE;
   if(msgt->msgattr & Q_RECEIVED)
      tmsg.attr |= MSGREAD;
   strcpy (tmsg.to, pascal_string (msgt->whoto));
   strcpy (tmsg.from, pascal_string (msgt->whofrom));
   strcpy (tmsg.subj, pascal_string (msgt->subject));

   strcpy(stringa,pascal_string(msgt->date));
   sscanf(stringa,"%2d-%2d-%2d",&mo,&dy,&yr);
   strcpy(stringa,pascal_string(msgt->time));
   sscanf(stringa,"%2d:%2d",&hh,&mm);
   sprintf(tmsg.date, "%02d %s %02d  %2d:%02d:00", dy, mtext[mo-1], yr, hh, mm);

   bluewave_header (fdi, bw_start, bw_len, msgn, &tmsg);
}

void quick_save_message (char *txt)
{
   FILE *fp;
   word dest, m, nb, x;
   int fdinfo, i;
	char filename[80], text[258], buffer[258];
	long tempo, gdest;
	unsigned long crc;
	struct tm *tim;
	struct _msgidx idx;
	struct _msghdr hdr;
   struct _msgtoidx toidx;
   struct _gold_msgidx gidx;
   struct _gold_msghdr ghdr;

	memset ((char *)&toidx, 0, sizeof (struct _msgtoidx));

	m_print (bbstxt[B_SAVE_MESSAGE]);
	quick_scan_message_base (sys.quick_board, sys.gold_board, usr.msg, 1);

	// prima era qui
	//if (sys.gold_board)
	//	gdest = gmsginfo.highmsg + 1;
	//else
	//	dest = msginfo.highmsg + 1;
	activation_key ();
	m_print (" #%d ...", last_msg + 1);

	if (sys.gold_board) {
		memset ((char *)&gidx, 0, sizeof (struct _gold_msgidx));
		memset ((char *)&ghdr, 0, sizeof (struct _gold_msghdr));

		sprintf (filename, "%sMSGINFO.DAT", fido_msgpath);
		fdinfo = sh_open (filename, SH_DENYRW, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
		if (fdinfo == -1)
			return;
		read (fdinfo, (char *)&gmsginfo, sizeof (struct _gold_msginfo));
		lseek (fdinfo, 0L, SEEK_SET);

		gdest = gmsginfo.highmsg + 1;

		sprintf (filename, "%sMSGIDX.DAT", fido_msgpath);
		i = sh_open (filename, SH_DENYNONE, O_APPEND|O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
		if (i == -1)
			return;
		fp = fdopen (i, "ab");

		gidx.msgnum = gdest;
		gidx.board = sys.gold_board;

		fwrite ((char *)&gidx, sizeof (struct _gold_msgidx), 1, fp);
		fclose (fp);

		sprintf (filename, "%sMSGTOIDX.DAT", fido_msgpath);
	}
	else {
		memset ((char *)&idx, 0, sizeof (struct _msgidx));
		memset ((char *)&hdr, 0, sizeof (struct _msghdr));

		sprintf (filename, "%sMSGINFO.BBS", fido_msgpath);
		fdinfo = sh_open (filename, SH_DENYRW, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
		if (fdinfo == -1)
			return;
		read (fdinfo, (char *)&msginfo, sizeof (struct _msginfo));
		lseek (fdinfo, 0L, SEEK_SET);

		dest = msginfo.highmsg + 1;

		sprintf (filename, "%sMSGIDX.BBS", fido_msgpath);
		i = sh_open (filename, SH_DENYNONE, O_APPEND|O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
		if (i == -1)
			return;
		fp = fdopen (i, "ab");

		idx.msgnum = dest;
		idx.board = sys.quick_board;

		fwrite ((char *)&idx, sizeof (struct _msgidx), 1, fp);
		fclose (fp);

		sprintf (filename, "%sMSGTOIDX.BBS", fido_msgpath);
	}



	if ((i = sh_open (filename, SH_DENYNONE, O_APPEND|O_RDWR|O_BINARY, S_IREAD|S_IWRITE)) == -1)
      return;

   strcpy (&toidx.string[1], msg.to);
   toidx.string[0] = strlen (msg.to);

   write (i, (char *)&toidx, sizeof (struct _msgtoidx));
   close (i);

   if (sys.gold_board)
      sprintf (filename, "%sMSGTXT.DAT", fido_msgpath);
   else
		sprintf (filename, "%sMSGTXT.BBS", fido_msgpath);

   i = sh_open (filename, SH_DENYNONE, O_APPEND|O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
   if (i == -1)
      return;
   fp = fdopen (i, "ab");

   if (sys.gold_board)
      ghdr.startblock = filelength (fileno (fp)) / 256L;
	else
      hdr.startblock = (word)(filelength (fileno (fp)) / 256L);

   i = 0;
   m = 1;
   nb = 1;

   memset (text, 0, 256);

   if (sys.netmail) {
      if (msg_tpoint) {
         sprintf (buffer, msgtxt[M_TOPT], msg_tpoint);
         write_pascal_string (buffer, text, &m, &nb, fp);
      }
      if (msg_tzone != msg_fzone) {
         sprintf (buffer, msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig);
         write_pascal_string (buffer, text, &m, &nb, fp);
      }
   }

   if (sys.echomail) {
      sprintf(buffer,msgtxt[M_PID], VERSION, registered ? "+" : NOREG);
		write_pascal_string (buffer, text, &m, &nb, fp);

		crc = time (NULL);
		crc = string_crc(msg.from,crc);
		crc = string_crc(msg.to,crc);
		crc = string_crc(msg.subj,crc);
		crc = string_crc(msg.date,crc);

		sprintf(buffer,msgtxt[M_MSGID], config->alias[sys.use_alias].zone, config->alias[sys.use_alias].net, config->alias[sys.use_alias].node, config->alias[sys.use_alias].point, crc);
      write_pascal_string (buffer, text, &m, &nb, fp);
   }

   if (sys.internet_mail && internet_to != NULL) {
      fprintf (fp, "To: %s\r\n\r\n", internet_to);
      free (internet_to);
   }

   if (txt == NULL) {
      while(messaggio[i] != NULL) {
         write_pascal_string (messaggio[i], text, &m, &nb, fp);

         if(messaggio[i][strlen(messaggio[i])-1] == '\r')
            write_pascal_string ("\n", text, &m, &nb, fp);

         i++;
      }
   }
   else {
      int fptxt;

      fptxt = shopen(txt, O_RDONLY|O_BINARY);
      if (fptxt == -1) {
         fclose(fp);
         unlink(filename);
         close (fdinfo);
         return;
      }

      do {
         memset (buffer, 0, 256);
         i = read(fptxt, buffer, 255);
         for (x = 0; x < i; x++) {
            if (buffer[x] == 0x1A)
               buffer[x] = ' ';
         }
         buffer[i] = '\0';
         write_pascal_string (buffer, text, &m, &nb, fp);
      } while (i == 255);

      close(fptxt);
   }

   write_pascal_string ("\r\n", text, &m, &nb, fp);

   if (strlen(usr.signature) && registered) {
      sprintf(buffer,msgtxt[M_SIGNATURE],usr.signature);
      write_pascal_string (buffer, text, &m, &nb, fp);
   }

   if(sys.echomail) {
      sprintf(buffer,msgtxt[M_TEAR_LINE],VERSION, registered ? "+" : NOREG);
      write_pascal_string (buffer, text, &m, &nb, fp);

      if(strlen(sys.origin))
         sprintf(buffer,msgtxt[M_ORIGIN_LINE],random_origins(),config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
      else
         sprintf(buffer,msgtxt[M_ORIGIN_LINE],system_name,config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
      write_pascal_string (buffer, text, &m, &nb, fp);
   }

   *text = m - 1;
   fwrite (text, 256, 1, fp);

   fclose (fp);

   if (sys.gold_board)
      sprintf (filename,"%sMSGHDR.DAT",fido_msgpath);
   else
      sprintf (filename,"%sMSGHDR.BBS",fido_msgpath);

   i = sh_open (filename, SH_DENYNONE, O_APPEND|O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
   if (i == -1)
      return;
   fp = fdopen (i, "ab");

   tempo = time (NULL);
   tim = localtime (&tempo);

   if (!first_msg)
      first_msg = 1;
   last_msg++;
   num_msg++;

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
      ghdr.destzone = ghdr.origzone = config->alias[sys.use_alias].zone;
      ghdr.cost = msg.cost;

      if (sys.netmail)
         ghdr.msgattr |= Q_NETMAIL;

      if (msg.attr & MSGPRIVATE)
         ghdr.msgattr |= Q_PRIVATE;
      if (msg.attr & MSGCRASH)
         ghdr.netattr |= Q_CRASH;
      if (msg.attr & MSGFILE)
         ghdr.netattr |= Q_FILE;
      if (msg.attr & MSGKILL)
         ghdr.netattr |= Q_KILL;
      if (msg.attr & MSGFRQ)
         ghdr.netattr |= Q_FRQ;
      if (msg.attr & MSGCPT)
         ghdr.netattr |= Q_CPT;
      if (msg.attr & MSGARQ)
         ghdr.netattr |= Q_ARQ;

      ghdr.msgattr |= Q_LOCAL;
      ghdr.board = sys.gold_board;

      sprintf (&ghdr.time[1], "%02d:%02d", tim->tm_hour, tim->tm_min);
      sprintf (&ghdr.date[1], "%02d-%02d-%02d", tim->tm_mon + 1, tim->tm_mday, tim->tm_year);
      ghdr.time[0] = 5;
      ghdr.date[0] = 8;

      strcpy (&ghdr.whoto[1], msg.to);
      ghdr.whoto[0] = strlen (msg.to);
      strcpy (&ghdr.whofrom[1], msg.from);
      ghdr.whofrom[0] = strlen (msg.from);
      strcpy (&ghdr.subject[1], msg.subj);
      ghdr.subject[0] = strlen (msg.subj);

      gmsginfo.highmsg++;
      gmsginfo.totalonboard[sys.gold_board - 1]++;
      gmsginfo.totalmsgs++;
      msgidx[last_msg] = filelength (fileno (fp)) / sizeof (struct _gold_msghdr);

      fwrite ((char *)&ghdr, sizeof (struct _gold_msghdr), 1, fp);
      fclose (fp);

		write (fdinfo, (char *)&gmsginfo, sizeof (struct _gold_msginfo));
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
      hdr.destzone = hdr.origzone = config->alias[sys.use_alias].zone;
      hdr.cost = msg.cost;

      if (sys.netmail)
         hdr.msgattr |= Q_NETMAIL;

      if (msg.attr & MSGPRIVATE)
         hdr.msgattr |= Q_PRIVATE;
      if (msg.attr & MSGCRASH)
         hdr.netattr |= Q_CRASH;
      if (msg.attr & MSGFILE)
         hdr.netattr |= Q_FILE;
      if (msg.attr & MSGKILL)
         hdr.netattr |= Q_KILL;
      if (msg.attr & MSGFRQ)
         hdr.netattr |= Q_FRQ;
      if (msg.attr & MSGCPT)
         hdr.netattr |= Q_CPT;
      if (msg.attr & MSGARQ)
         hdr.netattr |= Q_ARQ;

      hdr.msgattr |= Q_LOCAL;
      hdr.board = sys.quick_board;

      sprintf(&hdr.time[1],"%02d:%02d",tim->tm_hour,tim->tm_min);
      sprintf(&hdr.date[1],"%02d-%02d-%02d",tim->tm_mon+1,tim->tm_mday,tim->tm_year);
      hdr.time[0] = 5;
      hdr.date[0] = 8;

      strcpy (&hdr.whoto[1], msg.to);
      hdr.whoto[0] = strlen (msg.to);
      strcpy (&hdr.whofrom[1], msg.from);
      hdr.whofrom[0] = strlen (msg.from);
      strcpy (&hdr.subject[1], msg.subj);
      hdr.subject[0] = strlen (msg.subj);

      msginfo.highmsg++;
      msginfo.totalonboard[sys.quick_board - 1]++;
      msginfo.totalmsgs++;
      msgidx[last_msg] = filelength (fileno (fp)) / sizeof (struct _msghdr);

      fwrite ((char *)&hdr, sizeof (struct _msghdr), 1, fp);
      fclose (fp);

      write (fdinfo, (char *)&msginfo, sizeof (struct _msginfo));
   }

   close (fdinfo);

   m_print (bbstxt[B_XPRT_DONE]);
   status_line (msgtxt[M_INSUFFICIENT_DATA], last_msg, sys.msg_num);
}

