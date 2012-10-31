#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <dir.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "msgapi.h"


void squish_scan_message_base (area, name, upd)
int area;
char *name, upd;
{
   int i;

   if (sq_ptr != NULL)
      MsgCloseArea (sq_ptr);

   sq_ptr = MsgOpenArea (name, MSGAREA_CRIFNEC, MSGTYPE_SQUISH);
   MsgUnlock (sq_ptr);

   num_msg = (int)MsgGetNumMsg (sq_ptr);
   if (num_msg)
      first_msg = 1;
   else
      first_msg = 0;
   last_msg = num_msg;

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
}


int squish_read_message(msg_num, flag)
int msg_num, flag;
{
   int i, z, m, line, colf=0, xx, rd;
   char c, buff[80], wrp[80], *p, sqbuff[80];
   long fpos;
   MSGH *sq_msgh;
   XMSG xmsg;

   if (usr.full_read && !flag)
      return (squish_full_read_message(msg_num));

   line = 1;
   msg_fzone = msg_tzone = alias[0].zone;
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
      if (stricmp(xmsg.from,usr.name) && stricmp(xmsg.to,usr.name) && usr.priv != SYSOP) {
         MsgCloseMsg (sq_msgh);
         return (0);
      }

   if (!flag)
      cls ();

   memset ((char *)&msg, 0, sizeof (struct _msg));
   strcpy (msg.from, xmsg.from);
   strcpy (msg.to, xmsg.to);
   strcpy (msg.subj, xmsg.subj);
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

   if (flag)
      m_print(bbstxt[B_TWO_CR]);

   change_attr (CYAN|_BLACK);
   line = msg_attrib (&msg, msg_num, line, 0);
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

            if (!strncmp (buff, "SEEN-BY", 7) || buff[0] == 0x01) {
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
               m_print ("%s\n", buff);
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
            if (i < (usr.width - 2))
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
               m_print ("%s\n", buff);
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

   if (!stricmp(xmsg.to, usr.name) && !(xmsg.attr & MSGREAD)) {
      xmsg.attr |= MSGREAD;
      MsgWriteMsg (sq_msgh, 1, &xmsg, NULL, 0, 0, 0, NULL);
   }

   MsgCloseMsg (sq_msgh);

   if (line > 1 && !flag && usr.more)
      press_enter();

   return(1);
}

int squish_full_read_message(msg_num)
int msg_num;
{
   int i, z, m, line, colf=0, xx, rd;
   char c, buff[80], wrp[80], *p, sqbuff[80];
   long fpos;
   MSGH *sq_msgh;
   XMSG xmsg;

   line = 2;
   msg_fzone = msg_tzone = alias[0].zone;
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
      if (stricmp(xmsg.from,usr.name) && stricmp(xmsg.to,usr.name) && usr.priv != SYSOP) {
         MsgCloseMsg (sq_msgh);
         return (0);
      }

   memset ((char *)&msg, 0, sizeof (struct _msg));
   strcpy (msg.from, xmsg.from);
   strcpy (msg.to, xmsg.to);
   strcpy (msg.subj, xmsg.subj);
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

   cls ();
   change_attr (BLUE|_LGREY);
   del_line ();
   m_print (" * %s\n", sys.msg_name);
   change_attr (CYAN|_BLACK);
   line = msg_attrib (&msg, msg_num, line, 0);
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

            if (!strncmp (buff, "SEEN-BY", 7) || buff[0] == 0x01) {
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
               m_print ("%s\n", buff);
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
            if (i < (usr.width - 2))
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
               m_print ("%s\n", buff);
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

   if (!stricmp(xmsg.to, usr.name) && !(xmsg.attr & MSGREAD)) {
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
   char origin[128], tear[50], signature[100], pid[40], msgid[40];
   long totlen;
   MSGH *sq_msgh;
   XMSG xmsg;

   m_print(bbstxt[B_SAVE_MESSAGE]);
   dest = (int)MsgGetNumMsg (sq_ptr) + 1;
   activation_key ();
   m_print(" #%d ...",dest);

   sq_msgh = MsgOpenMsg (sq_ptr, MOPEN_CREATE, (dword)0);
   if (sq_msgh == NULL)
      return;

   memset ((char *)&xmsg, 0, sizeof (XMSG));
   strcpy (xmsg.from, msg.from);
   strcpy (xmsg.to, msg.to);
   strcpy (xmsg.subj, msg.subj);
   strcpy (xmsg.ftsc_date, msg.date);
   xmsg.orig.zone = alias[sys.use_alias].zone;
   xmsg.orig.node = alias[sys.use_alias].node;
   xmsg.orig.net = alias[sys.use_alias].net;
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
      sprintf (tear, msgtxt[M_TEAR_LINE],VERSION, registered ? "" : NOREG);
      if (strlen (sys.origin))
         sprintf (origin, msgtxt[M_ORIGIN_LINE], random_origins(), alias[sys.use_alias].zone, alias[sys.use_alias].net, alias[sys.use_alias].node, alias[sys.use_alias].point);
      else
         sprintf (origin, msgtxt[M_ORIGIN_LINE], system_name, alias[sys.use_alias].zone, alias[sys.use_alias].net, alias[sys.use_alias].node, alias[sys.use_alias].point);

      sprintf(pid,msgtxt[M_PID], VERSION, registered ? "" : NOREG);
      sprintf(msgid,msgtxt[M_MSGID], alias[sys.use_alias].zone, alias[sys.use_alias].net, alias[sys.use_alias].node, alias[sys.use_alias].point, time(NULL));

      totlen += (long)strlen (tear);
      totlen += (long)strlen (origin);
      totlen += (long)strlen (pid);
      totlen += (long)strlen (msgid);
   }

   if (txt == NULL) {
      i = 0;
      while (messaggio[i] != NULL) {
         totlen += (long)strlen (messaggio[i]);
         i++;
      }

      MsgWriteMsg (sq_msgh, 0, &xmsg, NULL, 0L, totlen, 0L, NULL);

      if (sys.echomail) {
         MsgWriteMsg (sq_msgh, 1, NULL, pid, strlen (pid), totlen, 0L, NULL);
         MsgWriteMsg (sq_msgh, 1, NULL, msgid, strlen (msgid), totlen, 0L, NULL);
      }

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
      if (fptxt == -1)
         return;

      MsgWriteMsg (sq_msgh, 0, &xmsg, NULL, 0L, totlen + filelength (fptxt), 0L, NULL);

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

   m_print (" done!\n\n");
   status_line (msgtxt[M_INSUFFICIENT_DATA], dest);
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

   msg_fzone = msg_tzone = alias[0].zone;
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
      if (stricmp(xmsg.from,usr.name) && stricmp(xmsg.to,usr.name) && usr.priv != SYSOP) {
         MsgCloseMsg (sq_msgh);
         return (0);
      }

   memset ((char *)&msgt, 0, sizeof (struct _msg));
   strcpy (msgt.from, xmsg.from);
   strcpy (msgt.to, xmsg.to);
   strcpy (msgt.subj, xmsg.subj);
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
      strcpy(buff, " > ");
      i = strlen(buff);
   }
   else
      i = 0;

   fpos = 0L;

   if (flags & INCLUDE_HEADER) {
      text_header (&msgt, msg_num, fpq);
   }
   else if (flags & QWK_TEXTFILE)
      qwk_header (&msgt, &QWK, msg_num, fpq, &qpos);

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

            if (flags & QWK_TEXTFILE) {
               write_qwk_string (buff, qwkbuffer, &pos, &blks, fpq);
               write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpq);
            }
            else
               fprintf (fpq, "%s\n", buff);

            if (flags & QUOTE_TEXT) {
               strcpy(buff, " > ");
               i = strlen(buff);
            }
            else
               i = 0;
         }
         else {
            if (i < (usr.width - 2))
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

            if (flags & QWK_TEXTFILE) {
               write_qwk_string (buff, qwkbuffer, &pos, &blks, fpq);
               write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpq);
            }
            else
               fprintf (fpq, "%s\n", buff);

            if (flags & QUOTE_TEXT)
               strcpy(buff, " > ");
            else
               buff[0] = '\0';

            strcat (buff, wrp);
            i = strlen (buff);
         }
      }
   } while (rd == 80);

   if (!stricmp(xmsg.to, usr.name) && !(xmsg.attr & MSGREAD)) {
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
   int i, line = 3;
   struct _msg msgt;
   MSGH *sq_msgh;
   XMSG xmsg;

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
         if (stricmp(xmsg.from,usr.name) && stricmp(xmsg.to,usr.name) && usr.priv != SYSOP)
            continue;

      memset ((char *)&msgt, 0, sizeof (struct _msg));
      strcpy (msgt.from, xmsg.from);
      strcpy (msgt.to, xmsg.to);
      strcpy (msgt.subj, xmsg.subj);
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
         m_print ("%-4d %c%-20.20s %c%-20.20s %-32.32s\n",
                   i,
                   stricmp (msgt.from, usr.name) ? 'Š' : 'Ž',
                   msgt.from,
                   stricmp (msgt.to, usr.name) ? 'Š' : 'Œ',
                   msgt.to,
                   msgt.subj);

      if (!(line=more_question(line)) || !CARRIER && !RECVD_BREAK())
         break;

      time_release();
   }

   m_print (bbstxt[B_ONE_CR]);

   if (line && CARRIER)
      press_enter();
}

int squish_mail_list_headers (m, line)
int m, line;
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
      if (stricmp(xmsg.from,usr.name) && stricmp(xmsg.to,usr.name) && usr.priv != SYSOP)
         return (line);

   memset ((char *)&msgt, 0, sizeof (struct _msg));
   strcpy (msgt.from, xmsg.from);
   strcpy (msgt.to, xmsg.to);
   strcpy (msgt.subj, xmsg.subj);
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

   if ((line = msg_attrib (&msgt, m, line, 0)) == 0)
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
         if (stricmp(xmsg.from,usr.name) && stricmp(xmsg.to,usr.name) && usr.priv != SYSOP) {
            MsgCloseMsg (sq_msgh);
            continue;
         }

      memset ((char *)&msgt, 0, sizeof (struct _msg));
      strcpy (msgt.from, xmsg.from);
      strcpy (msgt.to, xmsg.to);
      strcpy (msgt.subj, xmsg.subj);
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

   if (!stricmp (xmsg.from, usr.name) || !stricmp (xmsg.to, usr.name) || usr.priv == SYSOP) {
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
   long fpos, qpos;
   struct _msg msgt;
   struct QWKmsghd QWK;
   MSGH *sq_msgh;
   XMSG xmsg;

   tt = 0;
   pp = 0;

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
         if (stricmp(xmsg.from,usr.name) && stricmp(xmsg.to,usr.name) && usr.priv != SYSOP) {
            MsgCloseMsg (sq_msgh);
            continue;
         }

      totals++;
      if (fdi != -1) {
         sprintf(buff,"%u",totals);   /* Stringized version of current position */
         in = (float) atof(buff);
         out = IEEToMSBIN(in);
         write(fdi,&out,sizeof(float));

         c = 0;
         write(fdi,&c,sizeof(char));              /* Conference # */
      }

      if (!stricmp(xmsg.to,usr.name)) {
         pp++;
         if (fdp != -1) {
            write(fdp,&out,sizeof(float));
            write(fdp,&c,sizeof(char));              /* Conference # */
         }
      }

      memset ((char *)&msgt, 0, sizeof (struct _msg));
      strcpy (msgt.from, xmsg.from);
      strcpy (msgt.to, xmsg.to);
      strcpy (msgt.subj, xmsg.subj);
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

      if (qwk)
         qwk_header (&msgt, &QWK, msg_num, fpq, &qpos);
      else
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

               if (qwk) {
                  write_qwk_string (buff, qwkbuffer, &pos, &blks, fpq);
                  write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpq);
               }
               else
                  fprintf(fpq,"%s\n",buff);

               i = 0;
            }
            else {
               if (i < (usr.width - 2))
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

               if (qwk) {
                  write_qwk_string (buff, qwkbuffer, &pos, &blks, fpq);
                  write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpq);
               }
               else
                  fprintf(fpq,"%s\n",buff);

               strcpy (buff, wrp);
               i = strlen (buff);
            }
         }
      } while (rd == 80);

      if (!stricmp(xmsg.to, usr.name) && !(xmsg.attr & MSGREAD)) {
         xmsg.attr |= MSGREAD;
         MsgWriteMsg (sq_msgh, 1, &xmsg, NULL, 0, 0, 0, NULL);
      }

      MsgCloseMsg (sq_msgh);

      if (qwk) {
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
      else
         fprintf(fpq,bbstxt[B_TWO_CR]);

      tt++;
   }

   *personal = pp;
   *total = tt;

   return (totals);
}

void squish_save_message2(txt)
FILE *txt;
{
   int i, dest, m;
   char buffer[2050];
   long totlen;
   MSGH *sq_msgh;
   XMSG xmsg;

   dest = (int)MsgGetNumMsg (sq_ptr) + 1;

   sq_msgh = MsgOpenMsg (sq_ptr, MOPEN_CREATE, (dword)0);
   if (sq_msgh == NULL)
      return;

   memset ((char *)&xmsg, 0, sizeof (XMSG));
   strcpy (xmsg.from, msg.from);
   strcpy (xmsg.to, msg.to);
   strcpy (xmsg.subj, msg.subj);
   strcpy (xmsg.ftsc_date, msg.date);
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
   if (sys.echomail && (msg.attr & MSGSENT))
      xmsg.attr |= MSGSCANNED;

   totlen = filelength (fileno (txt));

   MsgWriteMsg (sq_msgh, 0, &xmsg, NULL, 0L, totlen, 0L, NULL);

   do {
      i = fread (buffer, 1, 2048, txt);
      for (m = 0; m < i; m++) {
         if (buffer[m] == 0x1A)
            buffer[m] = ' ';
      }

      MsgWriteMsg (sq_msgh, 1, NULL, buffer, (long)i, totlen, 0L, NULL);
   } while (i == 2048);

   MsgCloseMsg (sq_msgh);

   num_msg = last_msg = dest;
}

int squish_export_mail (maxnodes, forward)
int maxnodes;
struct _fwrd *forward;
{
   FILE *fpd;
   int i, pp, z, ne, no, n_seen, cnet, cnode, mi, msgnum, xx, rd, m;
   char buff[80], wrp[80], c, *p, buffer[2050], sqbuff[80], need_origin;
   long fpos;
   MSGH *sq_msgh;
   XMSG xmsg;
   struct _msghdr2 mhdr;
   struct _pkthdr2 pkthdr;
   struct _fwrd *seen;

   seen = (struct _fwrd *)malloc (MAX_EXPORT_SEEN * sizeof (struct _fwrd));
   if (seen == NULL)
      return (maxnodes);

   fpd = fopen ("MSGTMP.EXP", "rb+");
   if (fpd == NULL) {
      fpd = fopen ("MSGTMP.EXP", "wb");
      fclose (fpd);
      fpd = fopen ("MSGTMP.EXP", "r+b");
   }
   setvbuf (fpd, NULL, _IOFBF, 8192);

   for (i = 0; i < maxnodes; i++)
      forward[i].reset = forward[i].export = 0;

   if (!sys.use_alias) {
      z = alias[sys.use_alias].zone;
      parse_netnode2 (sys.forward1, &z, &ne, &no, &pp);
      if (z != alias[sys.use_alias].zone) {
         for (i = 0; alias[i].net; i++)
            if (z == alias[i].zone)
               break;
         if (alias[i].net)
            sys.use_alias = i;
      }
   }

   z = alias[sys.use_alias].zone;
   ne = alias[sys.use_alias].net;
   no = alias[sys.use_alias].node;
   pp = 0;

   p = strtok (sys.forward1, " ");
   if (p != NULL)
      do {
         parse_netnode2 (p, &z, &ne, &no, &pp);
         if ((i = is_here (z, ne, no, pp, forward, maxnodes)) != -1)
            forward[i].reset = forward[i].export = 1;
         else {
            forward[maxnodes].zone = z;
            forward[maxnodes].net = ne;
            forward[maxnodes].node = no;
            forward[maxnodes].point = pp;
            forward[maxnodes].export = 1;
            forward[maxnodes].reset = 1;
            maxnodes++;
         }
      } while ((p = strtok (NULL, " ")) != NULL);

   if (!sys.use_alias) {
      z = alias[sys.use_alias].zone;
      parse_netnode2 (sys.forward2, &z, &ne, &no, &pp);
      if (z != alias[sys.use_alias].zone) {
         for (i = 0; alias[i].net; i++)
            if (z == alias[i].zone)
               break;
         if (alias[i].net)
            sys.use_alias = i;
      }
   }

   z = alias[sys.use_alias].zone;
   ne = alias[sys.use_alias].net;
   no = alias[sys.use_alias].node;
   pp = 0;

   p = strtok (sys.forward2, " ");
   if (p != NULL)
      do {
         parse_netnode2 (p, &z, &ne, &no, &pp);
         if ((i = is_here (z, ne, no, pp, forward, maxnodes)) != -1)
            forward[i].reset = forward[i].export = 1;
         else {
            forward[maxnodes].zone = z;
            forward[maxnodes].net = ne;
            forward[maxnodes].node = no;
            forward[maxnodes].point = pp;
            forward[maxnodes].export = 1;
            forward[maxnodes].reset = 1;
            maxnodes++;
         }
      } while ((p = strtok (NULL, " ")) != NULL);

   if (!sys.use_alias) {
      z = alias[sys.use_alias].zone;
      parse_netnode2 (sys.forward3, &z, &ne, &no, &pp);
      if (z != alias[sys.use_alias].zone) {
         for (i = 0; alias[i].net; i++)
            if (z == alias[i].zone)
               break;
         if (alias[i].net)
            sys.use_alias = i;
      }
   }

   z = alias[sys.use_alias].zone;
   ne = alias[sys.use_alias].net;
   no = alias[sys.use_alias].node;
   pp = 0;

   p = strtok (sys.forward3, " ");
   if (p != NULL)
      do {
         parse_netnode2 (p, &z, &ne, &no, &pp);
         if ((i = is_here (z, ne, no, pp, forward, maxnodes)) != -1)
            forward[i].reset = forward[i].export = 1;
         else {
            forward[maxnodes].zone = z;
            forward[maxnodes].net = ne;
            forward[maxnodes].node = no;
            forward[maxnodes].point = pp;
            forward[maxnodes].export = 1;
            forward[maxnodes].reset = 1;
            maxnodes++;
         }
      } while ((p = strtok (NULL, " ")) != NULL);

   msgnum = (int)MsgGetHighWater(sq_ptr) + 1;

   wclreol ();
   for (; msgnum <= last_msg; msgnum++) {
      sprintf (wrp, "%d / %d\r", msgnum, last_msg);
      wputs (wrp);

      for (i = 0; i < maxnodes; i++)
         if (forward[i].reset)
            forward[i].export = 1;

      sq_msgh = MsgOpenMsg (sq_ptr, MOPEN_RW, (dword)msgnum);
      if (sq_msgh == NULL)
         continue;

      if (MsgReadMsg (sq_msgh, &xmsg, 0L, 0L, NULL, 0, NULL) == -1) {
         MsgCloseMsg (sq_msgh);
         continue;
      }

      if (xmsg.attr & MSGSCANNED) {
         xmsg.attr &= ~MSGSCANNED;
         MsgWriteMsg (sq_msgh, 1, &xmsg, NULL, 0, 0, 0, NULL);
         MsgCloseMsg (sq_msgh);
         continue;
      }

      need_origin = 1;

      fseek (fpd, 0L, SEEK_SET);
      chsize (fileno (fpd), 0L);
      fprintf (fpd, "AREA:%s\r\n", sys.echotag);

      mi = i = 0;
      pp = 0;
      n_seen = 0;
      fpos = 0L;

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
                     fprintf (fpd, msgtxt[M_TEAR_LINE],VERSION, registered ? "" : NOREG);
                     if (strlen(sys.origin))
                        fprintf(fpd,msgtxt[M_ORIGIN_LINE],random_origins(),alias[sys.use_alias].zone,alias[sys.use_alias].net,alias[sys.use_alias].node, alias[sys.use_alias].point);
                     else
                        fprintf(fpd,msgtxt[M_ORIGIN_LINE],system_name,alias[sys.use_alias].zone,alias[sys.use_alias].net,alias[sys.use_alias].node, alias[sys.use_alias].point);
                     need_origin = 0;
                  }

                  p = strtok (buff, " ");
                  ne = alias[sys.use_alias].net;
                  no = alias[sys.use_alias].node;
                  pp = alias[sys.use_alias].point;
                  z = alias[sys.use_alias].zone;
                  while ((p = strtok (NULL, " ")) != NULL) {
                     parse_netnode2 (p, &z, &ne, &no, &pp);
                     seen[n_seen].net = ne;
                     seen[n_seen].node = no;
                     seen[n_seen].point = pp;
                     seen[n_seen].zone = z;
                     for (i = 0; i < maxnodes; i++) {
                        if (forward[i].zone == seen[n_seen].zone && forward[i].net == seen[n_seen].net && forward[i].node == seen[n_seen].node && forward[i].point == seen[n_seen].point) {
                           forward[i].export = 0;
                           break;
                        }
                     }
                     if (seen[n_seen].net != alias[sys.use_alias].fakenet && !seen[n_seen].point)
                        n_seen++;
                  }

                  for (i = 0; i < maxnodes; i++) {
                     if (!forward[i].export || forward[i].net == alias[sys.use_alias].fakenet || forward[i].point)
                        continue;
                     seen[n_seen].net = forward[i].net;
                     seen[n_seen++].node = forward[i].node;
                  }

                  qsort (seen, n_seen, sizeof (struct _fwrd), mail_sort_func);

                  cnet = cnode = 0;
                  strcpy (wrp, "SEEN-BY: ");

                  for (i = 0; i < n_seen; i++) {
                     if (cnet != seen[i].net && cnode != seen[i].node) {
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

                     if (strlen (wrp) + strlen (buffer) > 70) {
                        fprintf (fpd, "%s\r\n", wrp);
                        strcpy (wrp, "SEEN-BY: ");
                        sprintf (buffer, "%d/%d ", seen[i].net, seen[i].node);
                     }

                     strcat (wrp, buffer);
                  }

                  fprintf (fpd, "%s\r\n", wrp);
               }
               else if (!strncmp (buff, "\001PATH: ", 7) && !n_seen) {
                  if (need_origin) {
                     fprintf (fpd, msgtxt[M_TEAR_LINE],VERSION, registered ? "" : NOREG);
                     if (strlen(sys.origin))
                        fprintf(fpd,msgtxt[M_ORIGIN_LINE],random_origins(),alias[sys.use_alias].zone,alias[sys.use_alias].net,alias[sys.use_alias].node, alias[sys.use_alias].point);
                     else
                        fprintf(fpd,msgtxt[M_ORIGIN_LINE],system_name,alias[sys.use_alias].zone,alias[sys.use_alias].net,alias[sys.use_alias].node, alias[sys.use_alias].point);
                     need_origin = 0;
                  }

                  for (i = 0; alias[i].net; i++) {
                     if (alias[i].zone != alias[sys.use_alias].zone)
                        continue;
                     seen[n_seen].net = alias[i].net;
                     seen[n_seen++].node = alias[i].node;
                  }

                  for (i = 0; i < maxnodes; i++) {
                     if (!forward[i].export || forward[i].net == alias[sys.use_alias].fakenet || forward[i].point)
                        continue;
                     seen[n_seen].net = forward[i].net;
                     seen[n_seen++].node = forward[i].node;
                  }

                  qsort (seen, n_seen, sizeof (struct _fwrd), mail_sort_func);

                  cnet = cnode = 0;
                  strcpy (wrp, "SEEN-BY: ");

                  for (i = 0; i < n_seen; i++) {
                     if (cnet != seen[i].net && cnode != seen[i].node) {
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

                  fprintf (fpd, "%s\r\n", wrp);
                  fprintf (fpd, "%s\r\n", buff);
               }
               else
                  fprintf (fpd, "%s\r\n", buff);

               mi = 0;
            }
            else {
               if(mi < 78)
                  continue;

               if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
                  need_origin = 0;

               buff[mi]='\0';
               fprintf(fpd,"%s",buff);
               mi=0;
               buff[mi] = '\0';
            }
         }
      } while (rd == 80);

      if (!n_seen) {
         if (need_origin) {
            fprintf (fpd, msgtxt[M_TEAR_LINE],VERSION, registered ? "" : NOREG);
            if (strlen(sys.origin))
               fprintf(fpd,msgtxt[M_ORIGIN_LINE],random_origins(),alias[sys.use_alias].zone,alias[sys.use_alias].net,alias[sys.use_alias].node, alias[sys.use_alias].point);
            else
               fprintf(fpd,msgtxt[M_ORIGIN_LINE],system_name,alias[sys.use_alias].zone,alias[sys.use_alias].net,alias[sys.use_alias].node, alias[sys.use_alias].point);
            need_origin = 0;
         }

         if (!alias[sys.use_alias].point) {
            seen[n_seen].net = alias[sys.use_alias].net;
            seen[n_seen++].node = alias[sys.use_alias].node;
         }
         else if (alias[sys.use_alias].fakenet) {
            seen[n_seen].net = alias[sys.use_alias].fakenet;
            seen[n_seen++].node = alias[sys.use_alias].point;
         }

         for (i = 0; i < maxnodes; i++) {
            if (!forward[i].export || forward[i].net == alias[sys.use_alias].fakenet)
               continue;
            seen[n_seen].net = forward[i].net;
            seen[n_seen++].node = forward[i].node;
         }

         qsort (seen, n_seen, sizeof (struct _fwrd), mail_sort_func);

         cnet = cnode = 0;
         strcpy (wrp, "SEEN-BY: ");

         for (i = 0; i < n_seen; i++) {
            if (cnet != seen[i].net && cnode != seen[i].node) {
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

         fprintf (fpd, "%s\r\n", wrp);
         if (alias[sys.use_alias].point && alias[sys.use_alias].fakenet)
            fprintf (fpd, "\001PATH: %d/%d\r\n", alias[sys.use_alias].fakenet, alias[sys.use_alias].point);
         else if (alias[sys.use_alias].point)
            fprintf (fpd, "\001PATH: %d/%d.%d\r\n", alias[sys.use_alias].net, alias[sys.use_alias].node, alias[sys.use_alias].point);
         else
            fprintf (fpd, "\001PATH: %d/%d\r\n", alias[sys.use_alias].net, alias[sys.use_alias].node);
      }

      sprintf (wrp, "%5d %-20.20s ", msgnum, sys.echotag);
      wputs (wrp);

      mhdr.ver = PKTVER;
      if (alias[sys.use_alias].point && alias[sys.use_alias].fakenet) {
         mhdr.orig_node = alias[sys.use_alias].point;
         mhdr.orig_net = alias[sys.use_alias].fakenet;
      }
      else {
         mhdr.orig_node = alias[sys.use_alias].node;
         mhdr.orig_net = alias[sys.use_alias].net;
      }
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

         if (forward[i].point)
            sprintf (wrp, "%d/%d.%d ", forward[i].net, forward[i].node, forward[i].point);
         else
            sprintf (wrp, "%d/%d ", forward[i].net, forward[i].node);
         wreadcur (&z, &m);
         if ( (m + strlen (wrp)) > 68)
            wputs ("\n                           ");
         wputs (wrp);

         mhdr.dest_net = forward[i].net;
         mhdr.dest_node = forward[i].node;
         p = HoldAreaNameMunge (forward[i].zone);
         if (forward[i].point)
            sprintf (buff, "%s%04x%04x.PNT\\%08X.OUT", p, forward[i].net, forward[i].node, forward[i].point);
         else
            sprintf (buff, "%s%04x%04x.OUT", p, forward[i].net, forward[i].node);
         mi = open (buff, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
         if (mi == -1 && forward[i].point) {
            sprintf (buff, "%s%04x%04x.PNT", p, forward[i].net, forward[i].node);
            mkdir (buff);
            sprintf (buff, "%s%04x%04x.PNT\\%08X.OUT", p, forward[i].net, forward[i].node, forward[i].point);
            mi = open (buff, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
         }
         if (filelength (mi) > 0L)
            lseek(mi,filelength(mi)-2,SEEK_SET);
         else {
            memset ((char *)&pkthdr, 0, sizeof (struct _pkthdr2));
            pkthdr.ver = PKTVER;
            pkthdr.product = 0x4E;
            pkthdr.serial = 2 * 16 + 10;
            if (alias[sys.use_alias].point && alias[sys.use_alias].fakenet) {
               pkthdr.orig_node = alias[sys.use_alias].point;
               pkthdr.orig_net = alias[sys.use_alias].fakenet;
               pkthdr.orig_point = 0;
            }
            else {
               pkthdr.orig_node = alias[sys.use_alias].node;
               pkthdr.orig_net = alias[sys.use_alias].net;
               pkthdr.orig_point = alias[sys.use_alias].point;
               pkthdr.capability = 1;
               pkthdr.cwvalidation = 256;
            }
            if (forward[i].point) {
               pkthdr.capability = 1;
               pkthdr.cwvalidation = 256;
            }
            pkthdr.orig_zone = alias[sys.use_alias].zone;
            pkthdr.dest_point = forward[i].point;
            pkthdr.dest_node = forward[i].node;
            pkthdr.dest_net = forward[i].net;
            pkthdr.dest_zone = forward[i].zone;
            write (mi, (char *)&pkthdr, sizeof (struct _pkthdr2));
         }

         write (mi, (char *)&mhdr, sizeof (struct _msghdr2));

         write (mi, xmsg.ftsc_date, strlen (xmsg.ftsc_date) + 1);
         write (mi, xmsg.to, strlen (xmsg.to) + 1);
         write (mi, xmsg.from, strlen (xmsg.from) + 1);
         write (mi, xmsg.subj, strlen (xmsg.subj) + 1);
         fseek (fpd, 0L, SEEK_SET);
         do {
            z = fread(buffer, 1, 2048, fpd);
            write(mi, buffer, z);
         } while (z == 2048);
         buff[0] = buff[1] = buff[2] = 0;
         write (mi, buff, 3);
         close (mi);
      }

      MsgCloseMsg (sq_msgh);

      wputs ("\n");
   }

   MsgSetHighWater(sq_ptr, (dword)last_msg);

   fclose (fpd);
   free (seen);
   unlink ("MSGTMP.EXP");

   return (maxnodes);
}

