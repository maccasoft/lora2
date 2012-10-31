#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <dir.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "msgapi.h"


void squish_scan_message_base (area, name)
int area;
char *name;
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
      else {
         lastread = 0;
         for (i=1;i<MAXDLREAD;i++) {
            usr.dynlastread[i-1].area = usr.dynlastread[i].area;
            usr.dynlastread[i-1].msg_num = usr.dynlastread[i].msg_num;
         }

         usr.dynlastread[i-1].area = area;
         usr.dynlastread[i-1].msg_num = 0;
      }
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
               for (z = i + 1; z < (usr.width - 1); z++) {
                  wrp[m++] = buff[z];
                  buff[i] = '\0';
               }

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
               for (z = i + 1; z < (usr.width - 1); z++) {
                  wrp[m++] = buff[z];
                  buff[i] = '\0';
               }

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
   FILE *fp;
   int i, dest;
   char origin[128], tear[50], signature[100];
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

   totlen = 2L;

   if (strlen(usr.signature) && registered) {
      sprintf (signature, msgtxt[M_SIGNATURE], usr.signature);
      totlen += (long)strlen (signature) + 1;
   }

   if (sys.echomail) {
      sprintf (tear, msgtxt[M_TEAR_LINE],VERSION, registered ? "" : NOREG);
      if (strlen (sys.origin))
         sprintf (origin, msgtxt[M_ORIGIN_LINE], sys.origin, alias[sys.use_alias].zone, alias[sys.use_alias].net, alias[sys.use_alias].node);
      else
         sprintf (origin, msgtxt[M_ORIGIN_LINE], system_name, alias[sys.use_alias].zone, alias[sys.use_alias].net, alias[sys.use_alias].node);

      totlen += (long)strlen (tear) + 1;
      totlen += (long)strlen (origin) + 1;
   }

   if (txt == NULL) {
      i = 0;
      while (messaggio[i] != NULL) {
         totlen += (long)strlen (messaggio[i]) + 2L;
         i++;
      }

      MsgWriteMsg (sq_msgh, 0, &xmsg, NULL, 0L, totlen, 0L, NULL);

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

   if (sys.echomail && sys.echotag[0]) {
      fp = fopen ("ECHOTOSS.LOG", "at");
      fprintf (fp, "%s\n", sys.echotag);
      fclose (fp);
   }

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

   if (flags & INCLUDE_HEADER)
      text_header (&msgt, msg_num, fpq);
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
            if (i < (usr.width - 1))
               continue;

            buff[i] = '\0';
            while (i > 0 && buff[i] != ' ')
               i--;

            m = 0;

            if (i != 0)
               for (z = i + 1; z < (usr.width - 1); z++) {
                  wrp[m++] = buff[z];
                  buff[i] = '\0';
               }

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
            i = strlen (wrp);
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

   if (!stricmp (xmsg.from,"ARCmail") && !stricmp (xmsg.to, "Sysop")) {
      MsgCloseMsg (sq_msgh);
      return (line);
   }

   if (xmsg.attr & MSGPRIVATE)
      if (stricmp(xmsg.from,usr.name) && stricmp(xmsg.to,usr.name) && usr.priv != SYSOP) {
         MsgCloseMsg (sq_msgh);
         return (line);
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

