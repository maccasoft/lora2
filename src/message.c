
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
#include <dos.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

extern int maxakainfo, msg_parent, msg_child;
extern char *internet_to;
extern struct _akainfo *akainfo;

extern struct _node2name *nametable; // Gestione nomi per aree PRIVATE
extern int nodes_num;

int track_outbound_messages (FILE *fpd, struct _msg *msgt, int fzone, int fpoint, int tzone, int tpoint);
int check_hold (int zone, int net, int node, int point);
FILE *mopen (char *filename, char *mode);
int mclose (FILE *fp);
int mputs (char *s, FILE *fp);
void mprintf (FILE *fp, char *format, ...);
long mseek (FILE *fp, long position, int offset);
int mread (char *s, int n, int e, FILE *fp);
void process_areafix_request (FILE *, int, int, int, int, char *);
void process_raid_request (FILE *, int, int, int, int, char *);
void replace_tearline (FILE *fpd, char *buf);
int open_packet (int zone, int net, int node, int point, int ai);

struct _msgzone {
   short dest_zone;
   short orig_zone;
   short dest_point;
   short orig_point;
};

void scan_message_base(area, upd)
int area;
char upd;
{
   int i, first, last;
   char filename[80], *p;
   struct ffblk blk;

   num_msg = 0;
   first = 0;
   last = 0;
   msg_parent = msg_child = 0;

   sprintf(filename, "%s*.MSG", sys.msg_path);

   if (!findfirst(filename, &blk, 0))
      do {
         if ((p = strchr(blk.ff_name,'.')) != NULL)
            *p = '\0';

         i = atoi(blk.ff_name);
			if (last < i || !last)
            last = i;
         if (first > i || !first)
            first = i;
         num_msg++;
      } while (!findnext(&blk));

   num_msg = num_msg;
   first_msg = first;
   last_msg = last;

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
void read_forward(start,pause)
int start,pause;
{
   int i;

   if (msg_list != NULL) {
      free (msg_list);
      msg_list = NULL;
   }

   if (start < 0)
      start = 0;

   if (start < last_msg)
      start++;
   else {
      m_print(bbstxt[B_NO_MORE_MESSAGE]);
      return;
   }

   if (sys.quick_board || sys.gold_board)
      while (!quick_read_message(start,pause,0)) {
         if (start < last_msg)
            start++;
         else
            break;
      }
   else if (sys.pip_board)
      while (!pip_msg_read(sys.pip_board, start, 0, pause,0)) {
         if (start < last_msg)
            start++;
         else
            break;
      }
   else if (sys.squish)
      while (!squish_read_message (start, pause,0)) {
         if (start < last_msg)
            start++;
         else
            break;
      }
	else {
      while (!read_message(start,pause,0)) {
         if (start < last_msg)
            start++;
         else
            break;
      }
   }

   lastread = start;

   for (i=0;i<MAXLREAD;i++)
      if (usr.lastread[i].area == usr.msg)
         break;
   if (i != MAXLREAD)
      usr.lastread[i].msg_num = start;
   else {
      for (i=0;i<MAXDLREAD;i++)
         if (usr.dynlastread[i].area == usr.msg)
            break;
      if (i != MAXDLREAD)
         usr.dynlastread[i].msg_num = start;
   }
}

void read_nonstop()
{
   int start;

   start = lastread;

   while (start < last_msg && !RECVD_BREAK() && CARRIER) {
      if (local_mode && local_kbd == 0x1B)
         break;
      read_forward(start,1);
      start = lastread;
      if (time_remain () <= 0)
         break;
      time_release ();
   }
}

void read_backward(start)
int start;
{
   int i;

   if (msg_list != NULL) {
      free (msg_list);
      msg_list = NULL;
   }

   if (start > first_msg)
      start--;
   else {
      m_print(bbstxt[B_NO_MORE_MESSAGE]);
      return;
   }

   if (sys.quick_board || sys.gold_board)
      while (!quick_read_message(start,0,0)) {
         if (start < last_msg)
            start++;
         else
            break;
      }
   else if (sys.pip_board)
      while (!pip_msg_read(sys.pip_board, start, 0, 0,0)) {
         if (start < last_msg)
            start++;
         else
            break;
      }
   else if (sys.squish)
      while (!squish_read_message (start, 0,0)) {
         if (start < last_msg)
            start++;
         else
            break;
      }
   else {
      while (!read_message(start,0,0)) {
         if (start > 1)
            start--;
         else
            break;
      }
   }

   lastread = start;

   for (i=0;i<MAXLREAD;i++)
      if (usr.lastread[i].area == usr.msg)
         break;
   if (i != MAXLREAD)
      usr.lastread[i].msg_num = start;
   else {
      for (i=0;i<MAXDLREAD;i++)
         if (usr.dynlastread[i].area == usr.msg)
            break;
      if (i != MAXDLREAD)
         usr.dynlastread[i].msg_num = start;
   }
}

void read_parent (void)
{
   int i, start = msg_parent;

   if (msg_list != NULL) {
      free (msg_list);
      msg_list = NULL;
   }

   if (start < 1 || start > last_msg) {
      m_print(bbstxt[B_NO_MORE_MESSAGE]);
      return;
   }

   if (sys.quick_board || sys.gold_board) {
      start--;
      if (!quick_read_message (start, 0, 0))
         start = lastread;
   }
   else if (sys.pip_board) {
      if (!pip_msg_read(sys.pip_board, start, 0, 0, 0))
         start = lastread;
   }
   else if (sys.squish) {
      if (!squish_read_message (start, 0, 0))
         start = lastread;
   }
	else {
      if (!read_message (start, 0, 0))
         start = lastread;
   }

   lastread = start;

   for (i = 0; i < MAXLREAD; i++)
      if (usr.lastread[i].area == usr.msg)
         break;
   if (i != MAXLREAD)
      usr.lastread[i].msg_num = start;
   else {
      for (i = 0; i < MAXDLREAD; i++)
         if (usr.dynlastread[i].area == usr.msg)
            break;
      if (i != MAXDLREAD)
         usr.dynlastread[i].msg_num = start;
   }
}

void read_reply (void)
{
   int i, start = msg_child;

   if (msg_list != NULL) {
      free (msg_list);
      msg_list = NULL;
   }

   if (start < 1 || start > last_msg) {
      m_print(bbstxt[B_NO_MORE_MESSAGE]);
      return;
   }

   if (sys.quick_board || sys.gold_board) {
      start--;
      if (!quick_read_message (start, 0, 0))
         start = lastread;
   }
   else if (sys.pip_board) {
      if (!pip_msg_read(sys.pip_board, start, 0, 0, 0))
         start = lastread;
   }
	else if (sys.squish) {
      if (!squish_read_message (start, 0, 0))
         start = lastread;
   }
   else {
      if (!read_message (start, 0, 0))
         start = lastread;
   }

   lastread = start;

   for (i = 0; i < MAXLREAD; i++)
      if (usr.lastread[i].area == usr.msg)
         break;
   if (i != MAXLREAD)
      usr.lastread[i].msg_num = start;
   else {
      for (i = 0; i < MAXDLREAD; i++)
         if (usr.dynlastread[i].area == usr.msg)
            break;
      if (i != MAXDLREAD)
         usr.dynlastread[i].msg_num = start;
   }
}

void msg_kill()
{
   int i, m;
   char stringa[10];

   if (!num_msg) {
      if (sys.quick_board || sys.gold_board)
         quick_scan_message_base (sys.quick_board, sys.gold_board, usr.msg, 1);
      else if (sys.pip_board)
         pip_scan_message_base (usr.msg, 1);
      else if (sys.squish)
         squish_scan_message_base (usr.msg, sys.msg_path, 1);
      else
         scan_message_base(usr.msg, 1);
   }

   for (;;) {
      if (!get_command_word (stringa, 4)) {
         m_print (bbstxt[B_MSG_DELETE]);
			chars_input(stringa, 4, INPUT_FIELD);
      }

      if (!stringa[0])
         break;

      i = atoi (stringa);

      if (i >= first_msg && i <= last_msg) {
         if (sys.quick_board)
            m = quick_kill_message (i, 0);
         else if (sys.gold_board)
            m = quick_kill_message (i, 1);
         else if (sys.pip_board)
            m = pip_kill_message (i);
         else if (sys.squish)
            m = squish_kill_message (i);
         else
            m = kill_message (i);

         if (!m)
            m_print (bbstxt[B_CANT_KILL], i);
         else {
            m_print (bbstxt[B_MSG_REMOVED], i);
            if (i == last_msg)
               last_msg--;
         }
      }
   }

/*
   if (sys.quick_board || sys.gold_board)
      quick_scan_message_base (sys.quick_board, sys.gold_board, usr.msg, 1);
   else if (sys.pip_board)
      pip_scan_message_base (usr.msg, 1);
   else if (sys.squish)
      squish_scan_message_base (usr.msg, sys.msg_path, 1);
   else
      scan_message_base (usr.msg, 1);
*/
}

int kill_message(msg_num)
int msg_num;
{
   int fd;
   char buff[80];
   struct _msg msgt;

   sprintf (buff, "%s%d.MSG", sys.msg_path, msg_num);
   fd = open (buff, O_RDONLY|O_BINARY);
   if (fd == -1)
      return (0);

   read (fd, (char *)&msgt, sizeof(struct _msg));
   close (fd);

   if (!stricmp (msgt.from, usr.name) || !stricmp (msgt.to, usr.name) || !stricmp (msgt.from, usr.handle) || !stricmp (msgt.to, usr.handle) || usr.priv == SYSOP) {
      unlink (buff);
		return (1);
	}

	return (0);
}


int read_message(msg_num, flag, fakenum)
int msg_num, flag, fakenum;
{
	FILE *fp;
	int i, z, m, line, colf=0, shead;
	char c, buff[150], wrp[150], *p, okludge;
	long fpos;
	struct _msg msgt;

	if (usr.full_read && !flag)
		return(full_read_message(msg_num,fakenum));

	okludge = 0;
	line = 1;
	shead = 0;
	msg_fzone = msg_tzone = config->alias[sys.use_alias].zone;
   msg_fpoint = msg_tpoint = 0;

   sprintf(buff,"%s%d.MSG",sys.msg_path,msg_num);
   i = sh_open (buff, SH_DENYWR, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
   if (i == -1)
      return(0);

   fp = fdopen (i, "r+b");
   if(fp == NULL)
      return(0);

   fread((char *)&msgt,sizeof(struct _msg),1,fp);

   if (!stricmp(msgt.from,"ARCmail") && !stricmp(msgt.to,"Sysop")) {
      fclose(fp);
      return(0);
   }

   if(msgt.attr & MSGPRIVATE) {
      if(msg_num == 1 && sys.echomail) {
         fclose(fp);
         return(0);
      }

      if (sys.netmail && usr.priv != SYSOP) {
         for (i = 0; config->alias[i].net != 0 && i < MAX_ALIAS; i++) {
            if (msgt.dest_net == config->alias[i].net && msgt.dest == config->alias[i].node)
               break;
         }
         if (config->alias[i].net == 0 || i >= MAX_ALIAS) {
            fclose (fp);
            return (0);
         }
      }

      if (stricmp (msgt.from, usr.name) && stricmp (msgt.to, usr.name) && stricmp (msgt.from, usr.handle) && stricmp (msgt.to, usr.handle) && usr.priv != SYSOP) {
         fclose (fp);
         return (0);
      }
   }

   if (!flag)
      cls();

   memcpy((char *)&msg,(char *)&msgt,sizeof(struct _msg));
   msg_parent = msg.reply;
   msg_child = msg.up;

   if(flag)
      m_print(bbstxt[B_TWO_CR]);

   change_attr (WHITE|_BLACK);
   m_print (" * %s\n", sys.msg_name);

   change_attr(CYAN|_BLACK);

   allow_reply = 1;
   i=0;
   fpos = filelength(fileno(fp));

   while (ftell (fp) < fpos) {
		c = fgetc (fp);

      if ((byte)c == 0x8D || c == 0x0A || c == '\0')
         continue;

      buff[i++] = c;

      if (c == 0x0D) {
         buff[i - 1] = '\0';

         if (buff[0] == 0x01 && !okludge) {
				if (!strncmp (&buff[1], "INTL", 4) && !shead)
					sscanf (&buff[6], "%d:%d/%d %d:%d/%d", &msg_tzone, &i, &i, &msg_fzone, &i, &i);
				if (!strncmp(&buff[1], "TOPT", 4) && !shead)
               sscanf (&buff[6], "%d", &msg_tpoint);
				if (!strncmp (&buff[1], "FMPT", 4) && !shead)
					sscanf (&buff[6], "%d", &msg_fpoint);
            i = 0;
            continue;
         }
         else if (!shead) {
            if (sys.internet_mail && !strnicmp (buff, "To:", 3)) {
               strtok (buff, ":");
               if ((p = strtok (NULL, ": ")) != NULL) {
                  if (strlen (p) > 35)
                     p[35] = '\0';
                  strcpy (msgt.to, p);
               }
               line = msg_attrib (&msgt, msg_num, line, 0);
               m_print(bbstxt[B_ONE_CR]);
               shead = 1;
               i = 0;
               continue;
            }
            line = msg_attrib (&msgt, fakenum ? fakenum : msg_num, line, 0);
            m_print(bbstxt[B_ONE_CR]);
            shead = 1;
            okludge = 1;
            fseek (fp, (long)sizeof (struct _msg), SEEK_SET);
            i = 0;
            continue;
         }

         if (!usr.kludge && (!strncmp (buff, "SEEN-BY", 7) || buff[0] == 0x01)) {
				i = 0;
            continue;
         }

         if (buff[0] == 0x01) {
            m_print ("%s\n", &buff[1]);
            change_attr (LGREY|BLACK);
         }
         else if (!strncmp (buff, "SEEN-BY", 7)) {
            m_print ("%s\n", buff);
            change_attr (LGREY|BLACK);
         }
         else {
            p = &buff[0];

            if(((strchr(buff,'>') - p) < 6) && (strchr(buff,'>'))) {
               if(!colf) {
                  change_attr(LGREY|BLACK);
                  colf=1;
               }
            }
            else {
               if(colf) {
                  change_attr(CYAN|BLACK);
                  colf=0;
               }
            }

            if (!strncmp(buff,msgtxt[M_TEAR_LINE],4) || !strncmp(buff," * Origin: ",11))
               m_print("%s\n",buff);
            else
               m_print("%s\n",buff);

            if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
               gather_origin_netnode (buff);
         }

         i=0;

         if(flag == 1)
            line=1;

         if(!(++line < (usr.len-1)) && usr.more) {
            line = 1;
				if(!continua()) {
               flag=1;
               break;
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

         if(i != 0) {
            for(z=i+1; buff[z]; z++)
               wrp[m++]=buff[z];
         }

         buff[i]='\0';
         wrp[m]='\0';

         if (!shead) {
            line = msg_attrib(&msgt,fakenum ? fakenum : msg_num,line,0);
            m_print(bbstxt[B_ONE_CR]);
            shead = 1;
            okludge = 1;
            fseek (fp, (long)sizeof (struct _msg), SEEK_SET);
            i = 0;
            continue;
         }

         if (((strchr(buff,'>') - buff) < 6) && (strchr(buff,'>'))) {
            if(!colf) {
               change_attr(LGREY|_BLACK);
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
            m_print("%s\n",buff);
         else
            m_print("%s\n",buff);

         if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
            gather_origin_netnode (buff);

         if(flag == 1)
            line=1;

         if(!(++line < (usr.len-1)) && usr.more) {
            line = 1;
            if(!continua()) {
               flag=1;
               break;
            }
         }

         strcpy(buff,wrp);
         i=strlen(wrp);
      }
   }

   if ((!stricmp (msgt.to, usr.name) || !stricmp (msgt.to, usr.handle)) && !(msgt.attr & MSGREAD)) {
      msgt.attr |= MSGREAD;
      rewind(fp);
      fwrite((char *)&msgt,sizeof(struct _msg),1,fp);
   }

   fclose(fp);

   if (!shead) {
      line = msg_attrib(&msgt,msg_num,line,0);
      m_print(bbstxt[B_ONE_CR]);
      shead = 1;
   }

   if (line > 1 && !flag && usr.more) {
      press_enter ();
      line = 1;
	}

   return (1);
}

int full_read_message(msg_num,fakenum)
int msg_num, fakenum;
{
        FILE *fp;
        int i, z, m, line, colf=0, shead;
        char c, buff[150], wrp[150], *p, okludge;
        long fpos;
        struct _msg msgt;

        okludge = 0;
        line = 2;
        shead = 0;
		  msg_fzone = msg_tzone = config->alias[sys.use_alias].zone;
		  msg_fpoint = msg_tpoint = 0;

        sprintf(buff,"%s%d.MSG",sys.msg_path,msg_num);
        i = sh_open (buff, SH_DENYWR, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
        if (i == -1)
                return(0);

        fp = fdopen (i, "r+b");
        if(fp == NULL)
                return(0);

        fread((char *)&msgt,sizeof(struct _msg),1,fp);

        if (!stricmp(msgt.from,"ARCmail") && !stricmp(msgt.to,"Sysop")) {
                fclose(fp);
                return(0);
        }

        if(msgt.attr & MSGPRIVATE) {
                if(msg_num == 1 && sys.echomail) {
                        fclose(fp);
                        return(0);
                }

      if (sys.netmail && usr.priv != SYSOP) {
         for (i = 0; config->alias[i].net != 0 && i < MAX_ALIAS; i++) {
            if (msgt.dest_net == config->alias[i].net && msgt.dest == config->alias[i].node)
               break;
         }
         if (config->alias[i].net == 0 || i >= MAX_ALIAS) {
            fclose (fp);
            return (0);
         }
      }

                if(stricmp(msgt.from,usr.name) && stricmp(msgt.to,usr.name) && stricmp(msgt.from,usr.handle) && stricmp(msgt.to,usr.handle) && usr.priv != SYSOP) {
                        fclose(fp);
								return(0);
                }
        }

        cls();

        memcpy((char *)&msg,(char *)&msgt,sizeof(struct _msg));
        msg_parent = msg.reply;
        msg_child = msg.up;

        change_attr(BLUE|_LGREY);
        del_line();
        m_print(" * %s\n", sys.msg_name);

        allow_reply = 1;
        i=0;
        fpos = filelength(fileno(fp));

        while(ftell(fp) < fpos) {
                c = fgetc(fp);

                if((byte)c == 0x8D || c == 0x0A || c == '\0')
                        continue;

                buff[i++]=c;

                if(c == 0x0D) {
                        buff[i-1]='\0';

                        if (buff[0] == 0x01 && !okludge) {
										  if (!strncmp(&buff[1],"INTL",4) && !shead)
													 sscanf(&buff[6],"%d:%d/%d %d:%d/%d",&msg_tzone,&i,&i,&msg_fzone,&i,&i);
                                if (!strncmp(&buff[1],"TOPT",4) && !shead)
                                        sscanf(&buff[6],"%d",&msg_tpoint);
                                if (!strncmp(&buff[1],"FMPT",4) && !shead)
                                        sscanf(&buff[6],"%d",&msg_fpoint);
                                i=0;
                                continue;
                        }
                        else if (!shead) {
                                if (sys.internet_mail && !strnicmp (buff, "To:", 3)) {
                                   strtok (buff, ":");
                                   if ((p = strtok (NULL, ": ")) != NULL) {
                                      if (strlen (p) > 35)
													  p[35] = '\0';
                                      strcpy (msgt.to, p);
                                   }
                                   line = msg_attrib(&msgt,msg_num,line,0);
                                   change_attr(LGREY|_BLUE);
                                   del_line();
                                   change_attr(CYAN|_BLACK);
                                   m_print(bbstxt[B_ONE_CR]);
                                   shead = 1;
                                   i = 0;
                                   continue;
                                }
                                line = msg_attrib(&msgt,fakenum ? fakenum : msg_num,line,0);
                                change_attr(LGREY|_BLUE);
                                del_line();
                                change_attr(CYAN|_BLACK);
                                m_print(bbstxt[B_ONE_CR]);
                                shead = 1;
                                okludge = 1;
                                fseek (fp, (long)sizeof (struct _msg), SEEK_SET);
                                i = 0;
                                continue;
                        }

                        if (!usr.kludge && (!strncmp (buff, "SEEN-BY", 7) || buff[0] == 0x01)) {
                           i = 0;
                           continue;
                        }

                        if (buff[0] == 0x01) {
                                m_print("%s\n",&buff[1]);
                                change_attr(CYAN|BLACK);
                        }
                        else if (!strncmp (buff, "SEEN-BY", 7)) {
                                m_print ("%s\n", buff);
                                change_attr (LGREY|BLACK);
                        }
                        else {
                                p = &buff[0];

                                if(((strchr(buff,'>') - p) < 6) && (strchr(buff,'>'))) {
                                        if(!colf) {
                                                change_attr(LGREY|BLACK);
                                                colf=1;
													 }
                                }
                                else {
                                        if(colf) {
                                                change_attr(CYAN|BLACK);
                                                colf=0;
                                        }
                                }

                                if (!strncmp(buff,msgtxt[M_TEAR_LINE],4) || !strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
                                        m_print("%s\n",buff);
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
                                line = msg_attrib(&msgt,fakenum ? fakenum : msg_num,line,0);
                                change_attr(LGREY|_BLUE);
                                del_line();
                                change_attr(CYAN|_BLACK);
                                m_print(bbstxt[B_ONE_CR]);
                                shead = 1;
                                okludge = 1;
                                fseek (fp, (long)sizeof (struct _msg), SEEK_SET);
                                i = 0;
                                continue;
                        }

                        if (((strchr(buff,'>') - buff) < 6) && (strchr(buff,'>'))) {
                                if(!colf) {
                                        change_attr(LGREY|_BLACK);
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
                                m_print("%s\n",buff);
                        else
                                m_print("%s\n",buff);

                        if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
                                gather_origin_netnode (buff);

                        if(!(++line < (usr.len-1)) && usr.more)
                        {
                                if(!continua())
                                {
													 line = 1;
                                        break;
                                }
                                else
                                {
                                        while (line > 5) {
                                                cpos(line--, 1);
                                                del_line();
                                        }
                                }
                        }

                        strcpy(buff,wrp);
                        i=strlen(wrp);
                }
        }

        if((!stricmp(msgt.to,usr.name) || !stricmp(msgt.to,usr.handle)) && !(msgt.attr & MSGREAD)) {
                msgt.attr |= MSGREAD;
                rewind(fp);
                fwrite((char *)&msgt,sizeof(struct _msg),1,fp);
        }

        fclose(fp);

        if (!shead) {
                line = msg_attrib(&msgt,msg_num,line,0);
                m_print(bbstxt[B_ONE_CR]);
                shead = 1;
        }

		  if(line > 1 && usr.more) {
                press_enter();
                line=1;
        }

        return(1);
}

int msg_attrib(msg_ptr,s,line,f)
struct _msg *msg_ptr;
int s, line, f;
{
   char stringa[60], *pto;

   adjust_date(msg_ptr);

   m_print(bbstxt[B_FROM]);
   if(sys.netmail || sys.internet_mail) {
		if (!msg_fzone)
			msg_fzone = config->alias[sys.use_alias].zone;
		sprintf(stringa,"%-25.25s (%d:%d/%d.%d)",msg_ptr->from,msg_fzone,msg_ptr->orig_net,msg_ptr->orig,msg_fpoint);
      m_print("%36s  ",stringa);
   }
   else
      m_print("%-36s  ",msg_ptr->from);

   change_attr(WHITE|_BLACK);
   if(msg_ptr->attr & MSGPRIVATE)
      m_print(bbstxt[B_MSGPRIVATE]);
   if(msg_ptr->attr & MSGREAD)
      m_print(bbstxt[B_MSGREAD]);
   if (sys.netmail || sys.internet_mail) {
      if(msg_ptr->attr & MSGCRASH)
         m_print(bbstxt[B_MSGCRASH]);
      if(msg_ptr->attr & MSGSENT)
			m_print(bbstxt[B_MSGSENT]);
      if(msg_ptr->attr & MSGFILE)
         m_print(bbstxt[B_MSGFILE]);
      if(msg_ptr->attr & MSGFWD)
         m_print(bbstxt[B_MSGFWD]);
      if(msg_ptr->attr & MSGORPHAN)
         m_print(bbstxt[B_MSGORPHAN]);
      if(msg_ptr->attr & MSGKILL)
         m_print(bbstxt[B_MSGKILL]);
      if(msg_ptr->attr & MSGHOLD)
         m_print(bbstxt[B_MSGHOLD]);
      if(msg_ptr->attr & MSGFRQ)
         m_print(bbstxt[B_MSGFRQ]);
      if(msg_ptr->attr & MSGRRQ)
         m_print(bbstxt[B_MSGRRQ]);
      if(msg_ptr->attr & MSGCPT)
         m_print(bbstxt[B_MSGCPT]);
      if(msg_ptr->attr & MSGARQ)
         m_print(bbstxt[B_MSGARQ]);
      if(msg_ptr->attr & MSGURQ)
         m_print(bbstxt[B_MSGURQ]);
   }
	m_print(bbstxt[B_ONE_CR]);
   if ((line=more_question(line)) == 0)
      return(0);

   if(!f) {
      if (!stricmp (msg_ptr->to, "@ALL"))
         pto = usr.name;
      else
         pto = msg_ptr->to;

      m_print(bbstxt[B_TO]);
      if(sys.netmail || sys.internet_mail) {
         if (!msg_tzone)
				msg_tzone = config->alias[sys.use_alias].zone;
			if (sys.internet_mail && internet_to != NULL)
            sprintf(stringa,"%-25.25s (%d:%d/%d.%d)",internet_to,msg_tzone,msg_ptr->dest_net,msg_ptr->dest,msg_tpoint);
         else
            sprintf(stringa,"%-25.25s (%d:%d/%d.%d)",pto,msg_tzone,msg_ptr->dest_net,msg_ptr->dest,msg_tpoint);
         m_print("%36s  ",stringa);
      }
      else
         m_print("%-36s  ",pto,msg_ptr->dest_net,msg_ptr->dest);

      change_attr(WHITE|_BLACK);
      if (s)
         m_print("Msg: #%d, ",s);
      m_print("%s ",show_date(config->dateformat,stringa,msg_ptr->date_written.date,msg_ptr->date_written.time));
      m_print("%s\n",show_date(config->timeformat,stringa,msg_ptr->date_written.date,msg_ptr->date_written.time));

      if ((line=more_question(line)) == 0)
         return(0);
   }

   if(msg_ptr->attr & MSGFILE || msg_ptr->attr & MSGFRQ)
      m_print(bbstxt[B_SUBJFILES]);
   else
      m_print(bbstxt[B_SUBJECT]);
   m_print("%s\n",msg_ptr->subj);
   if ((line=more_question(line)) == 0)
      return(0);

   change_attr(CYAN|_BLACK);

   return(line);
}

void adjust_date(msg_ptr)
struct _msg *msg_ptr;
{
   int yr, mo, dy, hh, mm, ss;
   char month[4];

   sscanf(msg_ptr->date,"%2d %3s %2d  %2d:%2d:%2d",&dy,month,&yr,&hh,&mm,&ss);
   month[3]='\0';

   for(mo=0;mo<12;mo++)
      if(!stricmp(month,mtext[mo]))
         break;

   if(mo == 12)
      mo=0;

   msg_ptr->date_written.date=(yr-80)*512+(mo+1)*32+dy;
   msg_ptr->date_written.time=hh*2048+mm*32+ss/2;
}

char *show_date(par,buffer,date,time)
char *par, *buffer;
int date, time;
{
	int yr, mo, dy, hh, mm, ss, flag, i;
        char parola[80];

	yr=((date >> 9) & 0x7F)+80;
	mo=((date >> 5) & 0x0F)-1;
	mo=(mo < 0)?0:mo;
	mo=(mo > 11)?11:mo;
	dy=date & 0x1F;
	hh=(time >> 11) & 0x1F;
	mm=(time >> 5) & 0x3F;
	ss=(time & 0x1F)*2;

	buffer[0]='\0';
	flag=0;

	if(par == NULL)
		return(buffer);

	while(*par) {
		if(flag) {
			switch(toupper(*par)) {
			case 'A':
				if(hh < 12)
					strcat(buffer,"am");
				else
				    strcat(buffer,"pm");
				break;
			case 'B':
				if(strlen(buffer))
                                        sprintf(parola,"%02d",mo + 1);
				else
                                    sprintf(parola,"%2d",mo + 1);
				strcat(buffer,parola);
				break;
			case 'C':
				strcat(buffer,mesi[mo]);
				break;
			case 'D':
				if(strlen(buffer))
					sprintf(parola,"%02d",dy);
				else
				    sprintf(parola,"%2d",dy);
				strcat(buffer,parola);
				break;
			case 'E':
				if(strlen(buffer)) {
					if(hh <= 12)
						sprintf(parola,"%02d",hh);
					else
					    sprintf(parola,"%02d",hh-12);
				}
				else {
					if(hh <= 12)
						sprintf(parola,"%2d",hh);
					else
					    sprintf(parola,"%2d",hh-12);
				}
				strcat(buffer,parola);
				break;
			case 'H':
				if(strlen(buffer))
					sprintf(parola,"%02d",hh);
				else
				    sprintf(parola,"%2d",hh);
				strcat(buffer,parola);
				break;
			case 'M':
				if(strlen(buffer))
					sprintf(parola,"%02d",mm);
				else
				    sprintf(parola,"%2d",mm);
				strcat(buffer,parola);
				break;
			case 'S':
				if(strlen(buffer))
					sprintf(parola,"%02d",ss);
				else
				    sprintf(parola,"%2d",ss);
				strcat(buffer,parola);
				break;
			case 'Y':
				if(strlen(buffer))
					sprintf(parola,"%02d",yr);
				else
				    sprintf(parola,"%2d",yr);
				strcat(buffer,parola);
				break;
			default:
				i=strlen(buffer);
				buffer[i]=*par;
				buffer[i+1]='\0';
				break;
			}
			flag=0;
		}
		else {
			if(*par == '%')
				flag=1;
			else {
				i=strlen(buffer);
				buffer[i]=*par;
				buffer[i+1]='\0';
			}
		}
		par++;
	}

        return(buffer);
}

void list_headers (verbose)
{
   int fd, i, m, line = verbose ? 2 : 5, l = 0;
   char stringa[10], filename[80];
   struct _msg msgt;
   struct _msgzone mz;

   if (!get_command_word (stringa, 4)) {
      m_print(bbstxt[B_START_WITH_MSG]);

      input(stringa,4);
      if(!strlen(stringa))
         return;
   }

   if (!num_msg)
      scan_message_base(usr.msg, 1);

   if (stringa[0] != '=') {
      m = atoi(stringa);
      if(m < 1 || m > last_msg)
			return;
	}
	else
		m = last_msg;

	cls ();
	m_print (bbstxt[B_LIST_AREAHEADER], usr.msg, sys.msg_name);

	if (!verbose) {
		m_print(bbstxt[B_LIST_HEADER1]);
		m_print(bbstxt[B_LIST_HEADER2]);
	}

	if (sys.quick_board) {
		quick_list_headers (m, sys.quick_board, verbose, 0);
		return;
	}
	else if (sys.gold_board) {
		quick_list_headers (m, sys.gold_board, verbose, 1);
		return;
	}
	else if (sys.pip_board) {
		pip_list_headers (m, sys.pip_board, verbose);
		return;
	}
	else if (sys.squish) {
		squish_list_headers (m, verbose);
		return;
	}

	for(i = m; i <= last_msg; i++) {
		sprintf(filename,"%s%d.MSG",sys.msg_path,i);

		fd=shopen(filename,O_RDONLY|O_BINARY);
		if(fd == -1)
			continue;
		read(fd,(char *)&msgt,sizeof(struct _msg));
		close(fd);

		if((msgt.attr & MSGPRIVATE) && stricmp(msgt.from,usr.name) && stricmp(msgt.to,usr.name) && stricmp(msgt.from,usr.handle) && stricmp(msgt.to,usr.handle) && usr.priv < SYSOP)
			continue;

		memcpy ((char *)&mz, (char *)&msgt.date_written, sizeof (struct _msgzone));

		msg_fzone = mz.orig_zone;
      msg_tzone = mz.dest_zone;
      msg_fpoint = mz.orig_point;
      msg_tpoint = mz.dest_point;

      if (verbose) {
         if ((line = msg_attrib(&msgt,i,line,0)) == 0)
            break;

         m_print(bbstxt[B_ONE_CR]);
      }
      else
         m_print ("%-4d %c%-20.20s %c%-20.20s %-32.32s\n",
                 i,
                 stricmp (msgt.from, usr.name) ? 'Š' : 'Ž',
                 msgt.from,
                 stricmp (msgt.to, usr.name) ? 'Š' : 'Œ',
                 msgt.to,
                 msgt.subj);

      if (!(l = more_question (line)) || !CARRIER && !RECVD_BREAK())
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

   if (line && CARRIER)
      press_enter ();
}

void message_inquire ()
{
   int fd, i, line=4;
   char stringa[30], filename[80], *p;
   struct _msg msgt, backup;

   if (msg_list != NULL) {
      free (msg_list);
      msg_list = NULL;
   }

   if (!get_command_word (stringa, 4)) {
      m_print (bbstxt[B_MSG_TEXT]);

      input (stringa,29);
      if (!strlen (stringa))
         return;
   }

   if (!num_msg) {
      if (sys.quick_board || sys.gold_board)
         quick_scan_message_base (sys.quick_board, sys.gold_board, usr.msg, 1);
      else if (sys.pip_board)
         pip_scan_message_base (usr.msg, 1);
		else if (sys.squish)
         squish_scan_message_base (usr.msg, sys.msg_path, 1);
      else
         scan_message_base(usr.msg, 1);
   }

   m_print (bbstxt[B_MSG_SEARCHING], strupr (stringa));

   if (sys.quick_board) {
      quick_message_inquire (stringa, sys.quick_board, 0);
      return;
   }
   else if (sys.gold_board) {
      quick_message_inquire (stringa, sys.gold_board, 1);
      return;
   }
   else if (sys.pip_board) {
      return;
   }
   else if (sys.squish) {
      squish_message_inquire (stringa);
      return;
   }

   for (i = first_msg; i <= last_msg; i++) {
      sprintf (filename, "%s%d.MSG", sys.msg_path, i);

      fd = shopen (filename,O_RDONLY|O_BINARY);
      if (fd == -1)
         continue;
      read (fd, (char *)&msgt, sizeof (struct _msg));

      if (msgt.attr & MSGPRIVATE)
         if (stricmp (msgt.from, usr.name) && stricmp (msgt.to, usr.name) && stricmp (msgt.from, usr.handle) && stricmp (msgt.to, usr.handle) && usr.priv < SYSOP) {
            close (fd);
            continue;
         }

      p = (char *)malloc ((unsigned int)filelength (fd) - sizeof (struct _msg) + 1);
      if (p != NULL) {
         read (fd, p, (unsigned int)filelength (fd) - sizeof (struct _msg));
         p[(unsigned int)filelength (fd) - sizeof (struct _msg)] = '\0';
      }
      else
			p = NULL;

      close (fd);

      memcpy ((char *)&backup, (char *)&msgt, sizeof (struct _msg));
      strupr (msgt.to);
      strupr (msgt.from);
      strupr (msgt.subj);

      if (p != NULL)
         strupr (p);

      if ((strstr (msgt.to, stringa) != NULL) || (strstr (msgt.from, stringa) != NULL) ||
          (strstr (msgt.subj, stringa) != NULL) || (strstr (p, stringa) != NULL)) {
         memcpy ((char *)&msgt, (char *)&backup, sizeof (struct _msg));
         if ((line = msg_attrib (&msgt, i, line, 0)) == 0)
            break;

         m_print (bbstxt[B_ONE_CR]);

         if (!(line = more_question (line)) || !CARRIER && !RECVD_BREAK ())
            break;

         time_release ();
      }

      if (p != NULL)
         free (p);
   }

   if (line && CARRIER)
      press_enter ();
}

void xport_message ()
{
   char filename[30];
   int msgnum;

   sprintf (filename, "%d", lastread);
   m_print (bbstxt[B_XPRT_NUMBER]);
   chars_input (filename, 5, INPUT_FIELD|INPUT_UPDATE);
   if (!strlen (filename) || !CARRIER)
      return;

   msgnum = atoi (filename);
   if (!msgnum || msgnum < 1 || msgnum > last_msg)
      return;

   strcpy (filename, "LORA.OUT");
   m_print (bbstxt[B_XPRT_DEVICE]);
   chars_input (filename, 25, INPUT_FIELD|INPUT_UPDATE);
   if (!strlen (filename) || !CARRIER)
      return;

   m_print (bbstxt[B_XPRT_WRITING]);

   if (sys.quick_board)
      quick_write_message_text(msgnum, APPEND_TEXT|INCLUDE_HEADER, filename, NULL);
   else if (sys.pip_board)
      pip_write_message_text(msgnum, APPEND_TEXT|INCLUDE_HEADER, filename, NULL);
   else if (sys.squish)
      squish_write_message_text(msgnum, APPEND_TEXT|INCLUDE_HEADER, filename, NULL);
   else
      write_message_text(msgnum, APPEND_TEXT|INCLUDE_HEADER, filename, NULL);

   m_print (bbstxt[B_XPRT_DONE]);
}

int select_area_list (int flag, int sig)
{
   int fd, fdi, i, pari, area, linea, nsys, nm;
   char stringa[80], filename[50], dir[80], first;
   struct _sys tsys;
   struct _sys_idx sysidx[10];

   m_print ("\nPlease enter the area number to forward the message to.\n");
   memset (&tsys, 0, sizeof (struct _sys));

   first = 1;

   if (sig == -1) {
      read_system_file ("MGROUP");

      do {
         m_print (bbstxt[B_GROUP_LIST]);
         chars_input (stringa, 4, INPUT_FIELD);
         if (!CARRIER || (sig = atoi (stringa)) == 0)
				return (0);
      } while (sig < 0);

      usr.msg_sig = sig;
   }

   sprintf(filename,"%sSYSMSG.DAT", config->sys_path);
   fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD);
   if (fd == -1)
      return (0);

   sprintf(filename,"%sSYSMSG.IDX", config->sys_path);
   fdi = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD);
   if (fdi == -1) {
      close (fd);
      return (0);
   }

   area = usr.msg;

   do {
      if (!get_command_word (stringa, 12)) {
         m_print ("\nForward area: [Area #, '?'=list, <enter>=Quit]: ");
         input (stringa, 12);
         if (!CARRIER || time_remain() <= 0) {
            close (fdi);
            close (fd);
            return (0);
         }
      }

      if (!stringa[0] && first) {
         first = 0;
         stringa[0] = area_change_key[2];
      }

      if (stringa[0] == area_change_key[2] || (!stringa[0] && !area)) {
         cls();
         m_print(bbstxt[B_AREAS_TITLE], bbstxt[B_MESSAGE]);

         first = 0;
         pari = 0;
         linea = 4;
         nm = 0;
			lseek(fdi, 0L, SEEK_SET);

         do {
            if (!CARRIER || time_remain() <= 0) {
               close (fdi);
               close (fd);
               return (0);
            }

            nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
            nsys /= sizeof (struct _sys_idx);

            for (i=0; i < nsys; i++, nm++) {
               if (usr.priv < sysidx[i].priv || (usr.flags & sysidx[i].flags) != sysidx[i].flags)
                  continue;

               if (sig) {
                  if (sysidx[i].sig != sig)
                     continue;
               }

               if (read_system_file ("MSGAREA"))
                  break;

               lseek(fd, (long)nm * SIZEOF_MSGAREA, SEEK_SET);
               read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA);

               if (sig || tsys.msg_restricted) {
                  if (tsys.msg_sig != sig)
                     continue;
               }

               if (flag == 1) {
                  strcpy (dir, tsys.msg_name);
                  dir[31] = '\0';

                  sprintf (stringa, "%3d ... %-31s ", sysidx[i].area, dir);

                  if (pari) {
                     m_print ("%s\n", strtrim (stringa));
                     pari = 0;
                     if (!(linea = more_question(linea)))
                        break;
                  }
						else {
                     m_print (stringa);
                     pari = 1;
                  }
               }
               else if (flag == 2) {
                  m_print("%3d ... %s\n", sysidx[i].area, tsys.msg_name);
                  if (!(linea = more_question(linea)))
                     break;
               }
               else if (flag == 3) {
                  m_print("%3d ... %-12s %s\n", sysidx[i].area, sysidx[i].key, tsys.msg_name);
                  if (!(linea = more_question(linea)))
                     break;
               }
               else if (flag == 4) {
                  if (!sysidx[i].key[0])
                     sprintf (sysidx[i].key, "%d", sysidx[i].area);
                  m_print("%-12s ... %s\n", sysidx[i].key, tsys.msg_name);
                  if (!(linea = more_question(linea)))
                     break;
               }
            }
         } while (linea && nsys == 10);

         if (pari)
            m_print(bbstxt[B_ONE_CR]);
         area = -1;
      }
      else if (strlen (stringa) < 1) {
         close (fdi);
         close (fd);
         return (0);
      }
      else {
         lseek (fdi, 0L, SEEK_SET);
         area = atoi (stringa);
         if (area < 1) {
            area = -1;
            do {
               nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
               nsys /= sizeof (struct _sys_idx);

               for(i=0;i<nsys;i++) {
						if (usr.priv < sysidx[i].priv || (usr.flags & sysidx[i].flags) != sysidx[i].flags)
                     continue;
                  if (sig) {
                     if (sysidx[i].sig != sig)
                        continue;
                  }
                  if (!stricmp(strbtrim (stringa), strbtrim (sysidx[i].key))) {
                     area = sysidx[i].area;
                     break;
                  }
               }
            } while (i == nsys && nsys == 10);
         }
         else {
            do {
               nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
               nsys /= sizeof (struct _sys_idx);

               for(i=0;i<nsys;i++) {
                  if (usr.priv < sysidx[i].priv || (usr.flags & sysidx[i].flags) != sysidx[i].flags)
                     continue;
                  if (sig) {
                     if (sysidx[i].sig != sig)
                        continue;
                  }
                  if (sysidx[i].area == area)
                     break;
               }
            } while (i == nsys && nsys == 10);

            if (i == nsys)
               area = -1;
         }
      }
   } while (area == -1 || !read_system2 (area, 1, &tsys));

   close (fdi);
   close (fd);

   return (area);
}

void forward_message_area (int sig, int flag)
{
	char filename[30];
   int msgnum, area;
   struct _sys tsys, bsys;

   sprintf (filename, "%d", lastread);
   m_print ("\nMessage number to forward: ");
   chars_input (filename, 5, INPUT_FIELD|INPUT_UPDATE);
   if (!strlen (filename) || !CARRIER)
      return;

   msgnum = atoi (filename);
   if (!msgnum || msgnum < 1 || msgnum > last_msg)
      return;

   sprintf (filename, "%sFWD%d.TMP", config->flag_dir, line_offset);
   if (sys.quick_board)
      quick_write_message_text (msgnum, APPEND_TEXT|INCLUDE_HEADER, filename, NULL);
   else if (sys.pip_board)
      pip_write_message_text (msgnum, APPEND_TEXT|INCLUDE_HEADER, filename, NULL);
   else if (sys.squish)
      squish_write_message_text (msgnum, APPEND_TEXT|INCLUDE_HEADER, filename, NULL);
   else
      write_message_text (msgnum, APPEND_TEXT|INCLUDE_HEADER, filename, NULL);

   area = select_area_list (flag, sig);
   if (area == 0 || !read_system2 (area, 1, &tsys) || !CARRIER) {
      unlink (filename);
      return;
   }

   memcpy (&bsys, &sys, SIZEOF_MSGAREA);
   memcpy (&sys, &tsys, SIZEOF_MSGAREA);

   if (sys.msg_num != bsys.msg_num) {
      if (sys.quick_board || sys.gold_board)
         quick_scan_message_base (sys.quick_board, sys.gold_board, sys.msg_num, 1);
      else if (sys.pip_board)
         pip_scan_message_base (sys.msg_num, 1);
      else if (sys.squish)
         squish_scan_message_base (sys.msg_num, sys.msg_path, 1);
      else
        scan_message_base (sys.msg_num, 1);
   }

	memset (&msg, 0, sizeof (struct _msg));

   if (get_message_data (0, NULL) == 1) {
      if (sys.quick_board || sys.gold_board)
         quick_save_message (filename);
      else if (sys.pip_board)
         pip_save_message (filename);
      else if (sys.squish)
         squish_save_message (filename);
      else
         save_message (filename);

      usr.msgposted++;
   }

   if (sys.msg_num != bsys.msg_num) {
      memcpy (&sys, &bsys, SIZEOF_MSGAREA);
      usr.msg = sys.msg_num;
   }

   if (sys.quick_board || sys.gold_board)
      quick_scan_message_base (sys.quick_board, sys.gold_board, usr.msg, 1);
   else if (sys.pip_board)
      pip_scan_message_base (usr.msg, 1);
   else if (sys.squish)
		squish_scan_message_base (usr.msg, sys.msg_path, 1);
	else
	  scan_message_base(usr.msg, 1);

	unlink (filename);
}
#endif

int fido_export_mail (int maxnodes, struct _fwrd *forward)
{
	FILE *fpd, *fp;
	int i, pp, z, ne, no, n_seen, cnet, cnode, mi, msgnum, m, sent, ai;
	char buff[80], wrp[80], c, *p, buffer[2050], need_origin, need_seen, *flag;
	long fpos;
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
			while (*p == '<' || *p == '>' || *p == '!' || *p == 'P' || *p == 'p')
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
         while (*flag == '<' || *flag == '>' || *flag =='!'||*flag=='p'||*flag=='P') {
            if (*flag == '>')
               forward[i].receiveonly = 1;
            if (*flag == '<')
               forward[i].sendonly = 1;
            if (*flag == '!')
               forward[i].passive = 1;
            if (*flag == 'P' || *flag=='p')
               forward[i].private =1;
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
         while (*p == '<' || *p == '>' || *p == '!' || *p == 'P' || *p == 'p')
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
         while (*flag == '<' || *flag == '>' || *flag =='!'||*flag=='p'||*flag=='P') {
            if (*flag == '>')
               forward[i].receiveonly = 1;
            if (*flag == '<')
					forward[i].sendonly = 1;
            if (*flag == '!')
               forward[i].passive = 1;
            if (*flag == 'P' || *flag=='p')
               forward[i].private =1;
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
         while (*p == '<' || *p == '>' || *p == '!' || *p == 'P' || *p == 'p')
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
         while (*flag == '<' || *flag == '>' || *flag =='!'||*flag=='p'||*flag=='P') {
            if (*flag == '>')
               forward[i].receiveonly = 1;
				if (*flag == '<')
               forward[i].sendonly = 1;
            if (*flag == '!')
					forward[i].passive = 1;
            if (*flag == 'P' || *flag=='p')
               forward[i].private =1;
            flag++;
         }
      } while ((p = strtok (NULL, " ")) != NULL);

   sprintf (buff, "%s1.MSG", sys.msg_path);
   i = open (buff, O_RDONLY|O_BINARY);
   if (i != -1) {
      read (i, (char *)&msg, sizeof (struct _msg));
      close (i);
      first_msg = msg.reply;
   }
   else
      first_msg = 1;
   if (first_msg > last_msg)
      first_msg = last_msg;

   strcpy (wrp, sys.echotag);
   wrp[14] = '\0';
   prints (8, 65, YELLOW|_BLACK, "              ");
   prints (8, 65, YELLOW|_BLACK, wrp);

   prints (7, 65, YELLOW|_BLACK, "              ");
   prints (9, 65, YELLOW|_BLACK, "              ");
   prints (9, 65, YELLOW|_BLACK, "Fido");

   sprintf (wrp, "%d / %d", first_msg, last_msg);
   prints (7, 65, YELLOW|_BLACK, wrp);
   sent = 0;

	for (msgnum = first_msg + 1; msgnum <= last_msg; msgnum++) {
      sprintf (wrp, "%d / %d", msgnum, last_msg);
      prints (7, 65, YELLOW|_BLACK, wrp);

		if ( !(msgnum % 20))
         time_release ();

      sprintf (buff, "%s%d.MSG", sys.msg_path, msgnum);
      i = sh_open (buff, SH_DENYWR, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
      if (i == -1)
         continue;

		fp = fdopen (i, "r+b");
		if (fp == NULL)
			continue;

		for (i = 0; i < maxnodes; i++)
			if (forward[i].reset)
				forward[i].export = 1;

		fread((char *)&msg,sizeof(struct _msg),1,fp);

		if (sys.echomail && (msg.attr & MSGSENT)) {
			fclose (fp);
			continue;
		}
			for (i=0;i<maxnodes;i++) {
			if (forward[i].passive||forward[i].sendonly) forward[i].export=0;
			if (forward[i].private) {

				int ii;

				forward[i].export=0;
				if (!nametable) continue;

				for(ii=0;ii<nodes_num;ii++) {
					if (forward[i].zone==nametable[ii].zone&&
						 forward[i].net==nametable[ii].net&&
						 forward[i].node==nametable[ii].node&&
						 forward[i].point==nametable[ii].point&&
						 !stricmp(msg.to,nametable[ii].name)) {
							 forward[i].export=1;
							 break;
					}
				}
			}


		}

      sprintf (wrp, "%5d  %-22.22s Fido         ", msgnum, sys.echotag);
      wputs (wrp);

      need_seen = need_origin = 1;

      mseek (fpd, 0L, SEEK_SET);
      mprintf (fpd, "AREA:%s\r\n", sys.echotag);

      mi = i = 0;
      pp = 0;
		n_seen = 0;
      fpos = 0L;

      fpos = filelength(fileno(fp));
      setvbuf (fp, NULL, _IOFBF, 2048);

      while (ftell (fp) < fpos) {
         c = fgetc(fp);

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

            if (!strcmp (buff, "NOECHO")) {
               msg.attr |= MSGSENT;
               break;
            }

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
            else if ((msg.attr & MSGLOCAL) && !strncmp (buff, "--- ", 4) && (config->replace_tear || !registered))
					replace_tearline (fpd, buff);
            else
               mprintf (fpd, "%s\r\n", buff);

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

      if (msg.attr & MSGSENT) {
         fseek (fp, 0L, SEEK_SET);
         msg.attr |= MSGSENT;
         fwrite ((char *)&msg, sizeof(struct _msg), 1, fp);
         fclose (fp);
         continue;
      }

      fseek (fp, 0L, SEEK_SET);
      msg.attr |= MSGSENT;
      fwrite ((char *)&msg, sizeof(struct _msg), 1, fp);
      fclose (fp);

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
      mhdr.cost = 0;
      mhdr.attrib = 0;

      if (sys.private)
         mhdr.attrib |= MSGPRIVATE;

      for (i = 0; i < maxnodes; i++) {
         if (!forward[i].export || forward[i].passive || forward[i].sendonly)
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

         write (mi, msg.date, strlen (msg.date) + 1);
         write (mi, msg.to, strlen (msg.to) + 1);
         write (mi, msg.from, strlen (msg.from) + 1);
         write (mi, msg.subj, strlen (msg.subj) + 1);
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

      wputs ("\n");
      sent++;
   }

   sprintf (buff, "%s1.MSG", sys.msg_path);
   i = open (buff, O_RDWR|O_BINARY);
   if (i != -1)
      read (i, (char *)&msg, sizeof (struct _msg));
   else {
      i = open (buff, O_CREAT|O_WRONLY|O_BINARY, S_IREAD|S_IWRITE);
      memset ((char *)&msg, 0, sizeof (struct _msg));
      strcpy (msg.from, "LoraBBS");
      strcpy (msg.to, "Nobody");
      strcpy (msg.subj, "High water mark");
      data (msg.date);
      write (i, (char *)&msg, sizeof (struct _msg));
      write (i, "NOECHO\r\n", 9);
   }
   msg.reply = last_msg ? last_msg : 1;
   lseek (i, 0L, SEEK_SET);
   write (i, (char *)&msg, sizeof (struct _msg));
   close (i);

   if (sent)
      status_line (":   %-20.20s (Sent=%04d)", sys.echotag, sent);

   mclose (fpd);
   free (seen);
   unlink ("MSGTMP.EXP");

   return (maxnodes);
}

void fido_rescan_echomail (char *tag, int zone, int net, int node, int point)
{
   FILE *fpd, *fp;
   int i, pp, z, ne, no, n_seen, cnet, cnode, mi, msgnum, m, sent, fd, ai = 0;
   char buff[80], wrp[80], c, *p, buffer[2050], need_origin, need_seen;
   long fpos;
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
   strcpy (sys.echotag, tag);

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

   first_msg = 1;
   if (first_msg > last_msg)
      first_msg = last_msg;

   strcpy (wrp, sys.echotag);
   wrp[14] = '\0';
   prints (8, 65, YELLOW|_BLACK, "              ");
   prints (8, 65, YELLOW|_BLACK, wrp);

   prints (7, 65, YELLOW|_BLACK, "              ");
   prints (9, 65, YELLOW|_BLACK, "              ");
   prints (9, 65, YELLOW|_BLACK, "Fido");

   sprintf (wrp, "%d / %d", first_msg, last_msg);
   prints (7, 65, YELLOW|_BLACK, wrp);
   sent = 0;

   for (msgnum = first_msg + 1; msgnum <= last_msg; msgnum++) {
      sprintf (wrp, "%d / %d", msgnum, last_msg);
      prints (7, 65, YELLOW|_BLACK, wrp);

      if ( !(msgnum % 20))
         time_release ();

      sprintf (buff, "%s%d.MSG", sys.msg_path, msgnum);
      i = sh_open (buff, SH_DENYWR, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
      if (i == -1)
         continue;

      fp = fdopen (i, "r+b");
      if (fp == NULL)
         continue;

      fread((char *)&msg,sizeof(struct _msg),1,fp);

      sprintf (wrp, "%5d  %-22.22s Fido         ", msgnum, sys.echotag);
      wputs (wrp);

      need_seen = need_origin = 1;

      mseek (fpd, 0L, SEEK_SET);
      mprintf (fpd, "AREA:%s\r\n", sys.echotag);

      mi = i = 0;
      pp = 0;
      n_seen = 0;
      fpos = 0L;

      fpos = filelength(fileno(fp));
      setvbuf (fp, NULL, _IOFBF, 2048);

      while (ftell (fp) < fpos) {
         c = fgetc(fp);

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
            else if ((msg.attr & MSGLOCAL) && !strncmp (buff, "--- ", 4) && (config->replace_tear || !registered))
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

      fseek (fp, 0L, SEEK_SET);
      msg.attr |= MSGSENT;
      fwrite ((char *)&msg, sizeof(struct _msg), 1, fp);
      fclose (fp);

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

      if (sys.private)
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

      write (mi, msg.date, strlen (msg.date) + 1);
      write (mi, msg.to, strlen (msg.to) + 1);
      write (mi, msg.from, strlen (msg.from) + 1);
      write (mi, msg.subj, strlen (msg.subj) + 1);
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
      wputs ("\n");
      sent++;
   }

   if (sent)
      status_line (":   %-20.20s (Sent=%04d)", sys.echotag, sent);

   mclose (fpd);
   free (seen);
   unlink ("MSGTMP.EXP");

   return;
}

void move_to_bad_message (char *netdir, int msgnum)
{
   int i, last;
   char filename[80], newname[80], *p;
   struct ffblk blk;

   last = 0;
   sprintf (filename, "%s*.MSG", config->bad_msgs);

   if (!findfirst (filename, &blk, 0))
		do {
         if ((p = strchr (blk.ff_name,'.')) != NULL)
            *p = '\0';

         i = atoi (blk.ff_name);
         if (last < i || !last)
            last = i;
      } while (!findnext (&blk));

   sprintf (newname, "%s%d.MSG", config->bad_msgs, ++last);
   sprintf (filename, "%s%d.MSG", netdir, msgnum);

   status_line (":Moved to bad message #%d", last);

   rename (filename, newname);
}

int check_afix (char *to)
{
   char *p, buffer[80];

   strcpy (buffer, config->areafix_watch);
   if ((p = strtok (buffer, " ")) == NULL)
      return (stricmp (to, "Areafix"));
   else {
      do {
         if (!stricmp (to, p))
            return (0);
      } while ((p = strtok (NULL, " ")) != NULL);
   }

   return (-1);
}

int check_tic (char *to)
{
   char *p, buffer[80];

	strcpy (buffer, config->tic_watch);
   if ((p = strtok (buffer, " ")) == NULL)
      return (stricmp (to, "Raid"));
   else {
      do {
         if (!stricmp (to, p))
				return (0);
      } while ((p = strtok (NULL, " ")) != NULL);
	}

	return (-1);
}

#define XATTR_IMM  0x01
#define XATTR_DIR  0x02
#define XATTR_TFS  0x04
#define XATTR_XMA  0x08
#define XATTR_KFS  0x10
#define XATTR_ZON  0x20

int fido_export_netmail (void)
{
	FILE *fpd, *fp;
    int i, z, mi, msgnum, sent, fd, xattr, dne, dno, removesent;
	char buff[80], wrp[80], c, *p, buffer[2050], countlimit, ourpoint, *b;
    char intl_found = 0,reply,msgid;
	long fpos;
	struct _msg omsg;
	struct _msghdr2 mhdr;
	struct _pkthdr2 pkthdr;
	struct date datep;
	struct time timep;
	NODEINFO ni;

    scan_message_base (0, 0);

	status_line ("#Packing from %s (%d msgs)", sys.msg_path, last_msg);
	sent = 0;

	fpd = fopen ("MSGTMP.EXP", "rb+");
	if (fpd == NULL) {
		fpd = fopen ("MSGTMP.EXP", "wb");
		fclose (fpd);
	}
	else
		fclose (fpd);
	fpd = mopen ("MSGTMP.EXP", "r+b");

	prints (7, 65, YELLOW|_BLACK, "              ");
	prints (8, 65, YELLOW|_BLACK, "              ");
	prints (8, 65, YELLOW|_BLACK, "Netmail");
	prints (9, 65, YELLOW|_BLACK, "              ");
	prints (9, 65, YELLOW|_BLACK, "Fido");

	for (msgnum = 1; msgnum <= last_msg; msgnum++) {
		if ( !(msgnum % 6)) {
			sprintf (wrp, "%d / %d", msgnum, last_msg);
			prints (7, 65, YELLOW|_BLACK, wrp);
			time_release ();
		}

		sprintf (buff, "%s%d.MSG", sys.msg_path, msgnum);
		i = sh_open (buff, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
		if (i == -1)
			continue;

		fp = fdopen (i, "r+b");
		if (fp == NULL)
			continue;

      removesent = 0;
		fread ((char *)&msg, sizeof (struct _msg), 1, fp);

		if (msg.attr & MSGSENT) {
			fclose (fp);
			continue;
		}

		for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++) {
			if (config->alias[i].point && config->alias[i].fakenet) {
				if (config->alias[i].fakenet == msg.dest_net && config->alias[i].point == msg.dest)
					break;
			}
			if (config->alias[i].net == msg.dest_net && config->alias[i].node == msg.dest)
				break;
		}

		if (i < MAX_ALIAS && config->alias[i].net == msg.dest_net && config->alias[i].node == msg.dest && !config->alias[i].point) {
			countlimit = 10;
			ourpoint = 1;
         removesent = 1;
      }
		else if (i < MAX_ALIAS && config->alias[i].fakenet == msg.dest_net && config->alias[i].point == msg.dest && config->alias[i].point) {
			fclose (fp);
			continue;
		}
		else {
			countlimit = 0;
			ourpoint = 0;
		}

		sprintf (wrp, "%d / %d", msgnum, last_msg);
		prints (7, 65, YELLOW|_BLACK, wrp);

		memcpy ((char *)&omsg, (char *)&msg, sizeof (struct _msg));

		mseek (fpd, 0L, SEEK_SET);

		msg_fzone = msg_tzone = 0;
		msg_fpoint = msg_tpoint = 0;
		mi = i = 0;
		fpos = 0L;
		xattr = 0;
      intl_found = msgid = reply = 0;

		fpos = filelength(fileno(fp));

		while (ftell (fp) < fpos) {
			c = fgetc(fp);

			if (c == '\0')
				break;

			if (c == 0x0A)
				continue;
			if((byte)c == 0x8D)
				c = ' ';

			buff[mi++] = c;

			if(c == 0x0D) {
				buff[mi - 1]='\0';
				if(buff[0] == 0x01) {
					if (!strncmp(&buff[1],"INTL",4)) {
//                        mprintf (fpd, "%s\r\n", buff);
						p=strtok(buff," ");
						if(p){
							p=strtok(NULL," ");
							if(p){
								dne=dno=0;
								parse_netnode(p,&msg_tzone,&dne,&dno,&i);
								p=strtok(NULL," ");
								if(p)
									parse_netnode(p,&msg_fzone,&i,&i,&i);
							}
						}
						if (dne != msg.dest_net || dno != msg.dest) {
							msg_tzone = msg_fzone;
//							mprintf (fpd, "%s\r\n", buff);
						}
						intl_found = 1;
					}

					else if (!strncmp(&buff[1],"TOPT",4)) {
						sscanf(&buff[6],"%d",&msg_tpoint);
						countlimit = 0;
                  removesent = 0;
                  for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++) {
							if (config->alias[i].net == msg.dest_net && config->alias[i].node == msg.dest && config->alias[i].point == msg_tpoint)
								break;
						}
						if (i < MAX_ALIAS && config->alias[i].net) {
							msg.attr |= MSGSENT;
							break;
						}
					}

					else if (!strncmp(&buff[1],"FMPT",4))
						sscanf(&buff[6],"%d",&msg_fpoint);

//                    else if (!strncmp (&buff[1], "REALNAME:", 9))
//                        strcpy (msg.from, &buff[11]);

					else
						mprintf (fpd, "%s\r\n", buff);

					if (!strncmp(&buff[1],"MSGID",5)) {
						msgid=1;
						if (!intl_found && strstr(buff,"/")){
                     parse_netnode (&buff[7],&msg_fzone,&i, &i, &i);
							if(!reply)
								msg_tzone=msg_fzone;
						}
					}

/*					if (!strncmp(&buff[1],"REPLY",5)) {
						reply=1;
						if (!intl_found && strstr(buff,"/")){
                     parse_netnode (&buff[7],&msg_tzone,&i, &i, &i);
//                     if(!msgid)
//                        msg_fzone=msg_tzone;
						}
					}
*/
					if (!strncmp (&buff[1], "FLAGS", 5)) {
						if (strstr (buff, " IMM") != NULL)
							xattr |= XATTR_IMM;
						if (strstr (buff, " DIR") != NULL)
							xattr |= XATTR_DIR;
						if (strstr (buff, " TFS") != NULL)
							xattr |= XATTR_TFS;
						if (strstr (buff, " XMA") != NULL)
							xattr |= XATTR_XMA;
						if (strstr (buff, " KFS") != NULL)
							xattr |= XATTR_KFS;
						if (strstr (buff, " ZON") != NULL)
							xattr |= XATTR_ZON;
					}
				}
				else if ((msg.attr & MSGLOCAL) && !strncmp (buff, "--- ", 4) && (config->replace_tear || !registered))
					replace_tearline (fpd, buff);
				else
					mprintf (fpd, "%s\r\n", buff);

				mi = 0;

				if (countlimit) {
					countlimit--;
               if (!countlimit)
						msg.attr |= MSGSENT;
				}
			}
			else {
				if(mi < 78)
					continue;

				buff[mi]='\0';
				mprintf(fpd,"%s",buff);
				mi=0;
				buff[mi] = '\0';

				if (countlimit) {
					countlimit--;
               if (!countlimit)
                  msg.attr |= MSGSENT;
            }
			}
		}
		if (msg_fzone == 0 && msg_tzone == 0)
			msg_fzone = msg_tzone = config->alias[0].zone;
		else{
			if (msg_fzone == 0)
				msg_fzone = msg_tzone;
			if (msg_tzone == 0)
				msg_tzone = msg_fzone;
		}

		if (config->msgtrack)
			if (!track_outbound_messages (fpd, &msg, msg_fzone, msg_fpoint, msg_tzone, msg_tpoint)) {
				omsg.attr |= MSGSENT;
				fseek (fp, 0L, SEEK_SET);
				fwrite ((char *)&omsg, sizeof(struct _msg), 1, fp);

				fclose (fp);

				move_to_bad_message (sys.msg_path, msgnum);
				continue;
			}

		if (msg_tpoint && check_hold (msg_tzone, msg.dest_net, msg.dest, msg_tpoint))
			msg.attr |= MSGHOLD;

		if(msg_tzone && ourpoint){
			ourpoint=0;
			for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++){
				if (config->alias[i].zone == msg_tzone && config->alias[i].net == msg.dest_net && config->alias[i].node == msg.dest) {
					ourpoint=1;
					break;
				}
			}
		}

		if (ourpoint && check_afix (msg.to) && check_tic (msg.to)) {
			sprintf (buff, "%sNODES.DAT", config->net_info);
			fd = open (buff, O_RDONLY|O_BINARY);
			if (fd != -1) {
				fpos = -1L;

				while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO)) {
					if ((!msg_tzone || msg_tzone == ni.zone) && !stricmp (ni.sysop_name, msg.to) && ni.remap4d) {
						if (msg_tzone != ni.zone || msg.dest_net != ni.net || msg.dest != ni.node || msg_tpoint != ni.point) {
							status_line ("#  Remap #%d %d:%d/%d.%d => %d:%d/%d.%d", msgnum, msg_tzone, msg.dest_net, msg.dest, msg_tpoint, ni.zone, ni.net, ni.node, ni.point);
							mprintf (fpd, "\x01PointMap %d:%d/%d.%d => %d:%d/%d.%d\r\n", msg_tzone, msg.dest_net, msg.dest, msg_tpoint, ni.zone, ni.net, ni.node, ni.point);
						}
						msg_tzone = ni.zone;
						msg.dest_net = ni.net;
						msg.dest = ni.node;
						msg_tpoint = ni.point;
						if (!msg_tpoint)
							ourpoint = 0;
						msg.attr |= MSGFWD;
						msg.attr &= ~MSGSENT;
                  removesent = 0;
                  fpos = -1L;
						break;
					}
					else if (fpos == -1L && !stricmp (ni.sysop_name, msg.to) && ni.remap4d)
						fpos = tell (fd) - sizeof (NODEINFO);
				}

				if (fpos != -1L) {
					lseek (fd, fpos, SEEK_SET);
					read (fd, (char *)&ni, sizeof (NODEINFO));
					status_line ("#  Remap #%d %d:%d/%d.%d => %d:%d/%d.%d", msgnum, msg_tzone, msg.dest_net, msg.dest, msg_tpoint, ni.zone, ni.net, ni.node, ni.point);
					mprintf (fpd, "\x01PointMap %d:%d/%d.%d => %d:%d/%d.%d\r\n", msg_tzone, msg.dest_net, msg.dest, msg_tpoint, ni.zone, ni.net, ni.node, ni.point);
					msg_tzone = ni.zone;
					msg.dest_net = ni.net;
					msg.dest = ni.node;
					msg_tpoint = ni.point;
					if (!msg_tpoint)
						ourpoint = 0;
					msg.attr &= ~MSGSENT;
					msg.attr |= MSGFWD;
               removesent = 0;
            }

				close (fd);
			}
		}

		if (!msg_fzone && msg_tzone)
			msg_fzone = msg_tzone;
		else if (!msg_tzone && msg_fzone)
			msg_tzone = msg_fzone;
		else if (!msg_tzone && !msg_fzone)
			msg_tzone = msg_fzone = config->alias[sys.use_alias].zone;

		if (ourpoint && registered == 1 && !check_afix (msg.to) && !(msg.attr & MSGREAD) && config->areafix) {
			msg.attr |= MSGSENT|MSGREAD;
			fseek (fp, 0L, SEEK_SET);
			fwrite((char *)&msg,sizeof(struct _msg),1,fp);

			mclose (fpd);

			sprintf (wrp, "%5d  %-22.22s Fido         ", msgnum, "Areafix");
			wputs (wrp);
			fseek (fp, (long)sizeof (struct _msg), SEEK_SET);
			process_areafix_request (fp, msg_fzone, msg.orig_net, msg.orig, msg_fpoint, msg.subj);
			wputs ("\n");

			fclose (fp);
			fpd = mopen ("MSGTMP.EXP", "r+b");
			continue;
		}
		else if (ourpoint && registered == 1 && !check_tic (msg.to) && !(msg.attr & MSGREAD) && config->tic_active) {
			msg.attr |= MSGSENT|MSGREAD;
			fseek (fp, 0L, SEEK_SET);
			fwrite((char *)&msg,sizeof(struct _msg),1,fp);

			mclose (fpd);

			sprintf (wrp, "%5d  %-22.22s Fido         ", msgnum, "Raid");
			wputs (wrp);
			fseek (fp, (long)sizeof (struct _msg), SEEK_SET);
			process_raid_request (fp, msg_fzone, msg.orig_net, msg.orig, msg_fpoint, msg.subj);
			wputs ("\n");

			fclose (fp);
			fpd = mopen ("MSGTMP.EXP", "r+b");
			continue;
		}

		if ((msg.attr & MSGSENT) || (ourpoint && !msg_tpoint)) {
			omsg.attr = msg.attr;
			omsg.attr |= MSGSENT;
         if (removesent)
            omsg.attr &= ~MSGSENT;
         fseek (fp, 0L, SEEK_SET);
			fwrite ((char *)&omsg, sizeof(struct _msg), 1, fp);
			fclose (fp);
			continue;
		}

		omsg.attr = msg.attr;
		omsg.attr |= MSGSENT;
      if (removesent)
         omsg.attr &= ~MSGSENT;
		fseek (fp, 0L, SEEK_SET);
		fwrite ((char *)&omsg, sizeof(struct _msg), 1, fp);

		fclose (fp);

		if (sys.netmail)
			sprintf (wrp, "%5d  %-22.22s Fido         ", msgnum, "Netmail");
		else
			sprintf (wrp, "%5d  %-22.22s Fido         ", msgnum, "Internet");
		wputs (wrp);

		if ((msg.attr & MSGKILL) && !config->keeptransit) {
			sprintf (buff, "%s%d.MSG", sys.msg_path, msgnum);
			unlink (buff);
		}

		if (!msg_fzone)
			msg_fzone = config->alias[sys.use_alias].zone;
		if (!msg_tzone)
			msg_tzone = msg_fzone;

		mhdr.ver = PKTVER;
		mhdr.orig_node = msg.orig;
		mhdr.orig_net = msg.orig_net;
		mhdr.dest_node = msg.dest;
		mhdr.dest_net = msg.dest_net;

		mhdr.cost = 0;

		if (sys.public)
			msg.attr &= ~MSGPRIVATE;
		else if (sys.private)
			msg.attr |= MSGPRIVATE;
		mhdr.attrib = msg.attr & ~(MSGCRASH|MSGSENT|MSGFWD|MSGKILL|MSGLOCAL);

		sprintf (wrp, "%d:%d/%d.%d ", msg_tzone, msg.dest_net, msg.dest, msg_tpoint);
		wputs (wrp);

		gettime (&timep);
		getdate (&datep);

		if (!ourpoint && msg_tpoint && check_hold (msg_tzone, msg.dest_net, msg.dest, msg_tpoint))
			msg.attr |= MSGHOLD;

		p = HoldAreaNameMungeCreate (msg_tzone);
		if (ourpoint)
			sprintf (buff, "%s%04x%04x.PNT\\%08X.XPR", p, msg.dest_net, msg.dest, msg_tpoint);
		else if (msg_tpoint && (msg.attr & MSGHOLD))
			sprintf (buff, "%s%04x%04x.PNT\\%08X.%cUT", p, msg.dest_net, msg.dest, msg_tpoint, 'H');
		else {
			if (xattr & XATTR_DIR)
				sprintf (buff, "%s%04x%04x.%cUT", p, msg.dest_net, msg.dest, 'D');
			else {
				if (msg.attr & MSGCRASH)
					sprintf (buff, "%s%04x%04x.%cUT", p, msg.dest_net, msg.dest, 'C');
				else if (msg.attr & MSGHOLD)
					sprintf (buff, "%s%04x%04x.%cUT", p, msg.dest_net, msg.dest, 'H');
				else
					sprintf (buff, "%s%04x%04x.XPR", p, msg.dest_net, msg.dest);
			}
		}
		mi = sh_open (buff, SH_DENYWR, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
		if (mi == -1 && msg_tpoint && ((msg.attr & MSGHOLD) || ourpoint)) {
			sprintf (buff, "%s%04x%04x.PNT", p, msg.dest_net, msg.dest);
			mkdir (buff);
			if (msg.attr & MSGHOLD)
				sprintf (buff, "%s%04x%04x.PNT\\%08X.%cUT", p, msg.dest_net, msg.dest, msg_tpoint, 'H');
			else
				sprintf (buff, "%s%04x%04x.PNT\\%08X.XPR", p, msg.dest_net, msg.dest, msg_tpoint);
			mi = sh_open (buff, SH_DENYWR, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
		}
		if (filelength (mi) > (long)sizeof (struct _pkthdr2))
			lseek(mi,filelength(mi)-2,SEEK_SET);
		else {
			memset ((char *)&pkthdr, 0, sizeof (struct _pkthdr2));
			pkthdr.ver = PKTVER;
			pkthdr.product = 0x4E;
			pkthdr.serial = 2 * 16 + 30;
			pkthdr.capability = 1;
			pkthdr.cwvalidation = 256;

			if (sys.msg_num == 0) {
				for (i=0; i < MAX_ALIAS; i++)
					if (msg_fzone && config->alias[i].zone == msg_fzone)
						break;
				if (i == MAX_ALIAS)
					z = 0;
				else
					z = i;

				for (i = 0; i < MAX_ALIAS; i++)
					if ( config->alias[i].point && config->alias[i].net == msg.orig_net && config->alias[i].node == msg.orig && (!msg_fzone || config->alias[i].zone == msg_fzone) )
						break;

				if (i < MAX_ALIAS)
					z = i;
			}
			else
				z = sys.use_alias;

			pkthdr.orig_node = config->alias[z].node;
			pkthdr.orig_net = config->alias[z].net;
			pkthdr.orig_zone = config->alias[z].zone;
			pkthdr.orig_zone2 = config->alias[z].zone;
			pkthdr.orig_point = config->alias[z].point;

			pkthdr.dest_node = msg.dest;
			pkthdr.dest_net = msg.dest_net;
			pkthdr.dest_zone = msg_tzone;
			pkthdr.dest_zone2 = msg_tzone;
			if (ourpoint || (msg_tpoint && (msg.attr & MSGHOLD)))
				pkthdr.dest_point = msg_tpoint;
			pkthdr.hour = timep.ti_hour;
			pkthdr.minute = timep.ti_min;
			pkthdr.second = timep.ti_sec;
			pkthdr.year = datep.da_year;
			pkthdr.month = datep.da_mon - 1;
			pkthdr.day = datep.da_day;
			add_packet_pw (&pkthdr);

			write (mi, (char *)&pkthdr, sizeof (struct _pkthdr2));
		}

		write (mi, (char *)&mhdr, sizeof (struct _msghdr2));

		write (mi, msg.date, strlen (msg.date) + 1);
		write (mi, msg.to, strlen (msg.to) + 1);
		write (mi, msg.from, strlen (msg.from) + 1);
		write (mi, msg.subj, strlen (msg.subj) + 1);

//        if (intl_modified||(!intl_found && (config->force_intl || msg_tzone != msg_fzone))) {
          if (!(!config->force_intl && !intl_found && (msg_tzone==msg_fzone))){
			sprintf (buffer, msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig);
			write (mi, buffer, strlen (buffer));
		}
      if (msg_fpoint) {
         sprintf (buffer, "\001FMPT %d\r\n", msg_fpoint);
         write (mi, buffer, strlen (buffer));
      }
      if (msg_tpoint) {
         sprintf (buffer, msgtxt[M_TOPT], msg_tpoint);
         write (mi, buffer, strlen (buffer));
      }

      mseek (fpd, 0L, SEEK_SET);
      do {
         z = mread(buffer, 1, 2048, fpd);
         write(mi, buffer, z);
      } while (z == 2048);
      buff[0] = buff[1] = buff[2] = 0;
      write (mi, buff, 3);
      close (mi);

      if (msg.attr & MSGFILE) {
         p = HoldAreaNameMungeCreate (msg_tzone);
         if (ourpoint)
            sprintf (buff, "%s%04x%04x.PNT\\%08X.%cLO", p, msg.dest_net, msg.dest, msg_tpoint, (msg.attr & MSGCRASH) ? 'C' : ((msg.attr & MSGHOLD) ? 'H' : 'F'));
         else if (msg_tpoint && (msg.attr & MSGHOLD))
            sprintf (buff, "%s%04x%04x.PNT\\%08X.%cLO", p, msg.dest_net, msg.dest, msg_tpoint, 'H');
         else
            sprintf (buff, "%s%04x%04x.%cLO", p, msg.dest_net, msg.dest, (msg.attr & MSGCRASH) ? 'C' : ((msg.attr & MSGHOLD) ? 'H' : 'F'));
         fp = fopen (buff, "at");

         if (msg.attr & MSGFWD) {
            if ((p = strtok (msg.subj, " ")) != NULL)
               do {
                  fnsplit (p, NULL, NULL, wrp, buff);
                  strcat (wrp, buff);
                  if (config->norm_filepath[0]) {
                     sprintf (buff, "%s%s", config->norm_filepath, wrp);
                     if (dexists (buff))
                        fprintf (fp, "^%s\n", buff);
                  }
                  if (config->know_filepath[0]) {
                     sprintf (buff, "%s%s", config->know_filepath, wrp);
		     if (dexists (buff))
                        fprintf (fp, "^%s\n", buff);
                  }
                  if (config->prot_filepath[0]) {
                     sprintf (buff, "%s%s", config->prot_filepath, wrp);
                     if (dexists (buff))
                        fprintf (fp, "^%s\n", buff);
                  }
               } while ((p = strtok (NULL, " ")) != NULL);
         }
         else {
            if ((p = strtok (msg.subj, " ")) != NULL)
               do {
                  if (xattr & XATTR_KFS)
                     fprintf (fp, "^%s\n", p);
                  else if (xattr & XATTR_TFS)
                     fprintf (fp, "#%s\n", p);
                  else
                     fprintf (fp, "%s\n", p);
               } while ((p = strtok (NULL, " ")) != NULL);
         }

         fclose (fp);
      }
      else if (msg.attr & MSGFRQ) {
	 p = HoldAreaNameMungeCreate (msg_tzone);
			if (ourpoint)
				sprintf (buff, "%s%04x%04x.PNT\\%08X.REQ", p, msg.dest_net, msg.dest, msg_tpoint);
			else if (msg_tpoint && (msg.attr & MSGHOLD))
				sprintf (buff, "%s%04x%04x.PNT\\%08X.REQ", p, msg.dest_net, msg.dest);
			else
				sprintf (buff, "%s%04x%04x.REQ", p, msg.dest_net, msg.dest);
			fp = fopen (buff, "at");

			if ((p = strtok (msg.subj, " ")) != NULL)
				do {
					b = strtok (NULL, " ");
					if (b != NULL && *b == '!') {
						fprintf (fp, "%s %s\n", p, b);
						p = strtok (NULL, " ");
					}
					else {
						fprintf (fp, "%s\n", p);
						p = b;
					}
				} while (p != NULL);

			fclose (fp);
		}

		wputs ("\n");

		sent++;
		sysinfo.today.emailsent++;
		sysinfo.week.emailsent++;
		sysinfo.month.emailsent++;
		sysinfo.year.emailsent++;

		time_release ();
	}

	status_line (":  Packed=%d", sent);

	mclose (fpd);
	unlink ("MSGTMP.EXP");
	return (0);
}

