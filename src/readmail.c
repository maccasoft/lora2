#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dir.h>
#include <io.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "quickmsg.h"
#include "pipbase.h"

#define MAX_MAIL_BUFFER   20

static int last_read_system;

static int quick_index (int, int *);
static int pipbase_index (int, int *);
static int fido_index (int, int *);
static int squish_index (int, int *);

int scan_mailbox()
{
   int i, z, area_msg[MSG_AREAS];
   struct _sys tsys;

   z = 1;
   memset((char *)&area_msg, 0, sizeof(int) * MSG_AREAS);

   m_print(bbstxt[B_CHECK_MAIL]);

   z = fido_index (z, &area_msg[0]);
   z = quick_index (z, &area_msg[0]);
   z = pipbase_index (z, &area_msg[0]);
   z = squish_index (z, &area_msg[0]);

   max_priv_mail = z - 1;
   last_mail = 0;
   last_read_system = 0;

   if (z == 1) {
      m_print(bbstxt[B_NO_MAIL_TODAY]);
      press_enter();
      return (0);
   }

   num_msg = last_msg = max_priv_mail;
   first_msg = 0;

   m_print(bbstxt[B_THIS_MAIL]);
   m_print(bbstxt[B_THIS_MAIL_UNDERLINE]);

   for (i=0;i<MSG_AREAS;i++) {
      if (!area_msg[i] || !read_system2(i+1, 1, &tsys))
         continue;

      m_print(bbstxt[B_MAIL_LIST_FORMAT], tsys.msg_name, area_msg[i]);
   }

   m_print(bbstxt[B_READ_MAIL_NOW]);
   i = yesno_question(DEF_YES);

   m_print(bbstxt[B_ONE_CR]);
   return (i);
}

void mail_read_forward(pause)
int pause;
{
   int start, i;

   start = last_mail;

   if (start < 0)
      start = 1;

   if (start < max_priv_mail)
      start++;
   else {
      m_print(bbstxt[B_NO_MORE_MESSAGE]);
      return;
   }

   if (last_read_system != msg_list[start].area) {
      read_system(msg_list[start].area, 1);
      usr.msg = 0;
      if (sys.quick_board)
         quick_scan_message_base (sys.quick_board, msg_list[start].area);
      else if (sys.pip_board)
         pip_scan_message_base (msg_list[start].area);
      else
         scan_message_base(msg_list[start].area);
      usr.msg = last_read_system = msg_list[start].area;
      num_msg = last_msg = max_priv_mail;
      first_msg = 0;
   }

   for (;;) {
      if (sys.quick_board)
         i = quick_read_message(msg_list[start].msg_num, pause);
      else if (sys.pip_board)
         i = pip_msg_read(usr.msg, msg_list[start].msg_num, 0, pause);
      else
         i = read_message(msg_list[start].msg_num, pause);

      if (!i) {
         if (start < last_msg) {
            start++;
            if (last_read_system != msg_list[start].area) {
               read_system(msg_list[start].area, 1);
               if (sys.quick_board)
                  quick_scan_message_base (sys.quick_board, msg_list[start].area);
               else if (sys.pip_board)
                  pip_scan_message_base (msg_list[start].area);
               else
                  scan_message_base(msg_list[start].area);
               last_read_system = msg_list[start].area;
               num_msg = last_msg = max_priv_mail;
               first_msg = 0;
            }
         }
         else
            break;
      }
      else
         break;
   }

   lastread = msg_list[start].msg_num;
   last_mail = start;
}

void mail_read_backward(pause)
int pause;
{
   int start, i;

   start = last_mail;

   if (start > 1)
      start--;
   else {
      m_print(bbstxt[B_NO_MORE_MESSAGE]);
      return;
   }

   if (last_read_system != msg_list[start].area) {
      read_system(msg_list[start].area, 1);
      usr.msg = 0;
      if (sys.quick_board)
         quick_scan_message_base (sys.quick_board, msg_list[start].area);
      else if (sys.pip_board)
         pip_scan_message_base (msg_list[start].area);
      else if (sys.squish)
         squish_scan_message_base (msg_list[start].area, sys.msg_path);
      else
         scan_message_base(msg_list[start].area);

      usr.msg = last_read_system = msg_list[start].area;
      num_msg = last_msg = max_priv_mail;
      first_msg = 0;
   }

   for (;;) {
      if (sys.quick_board)
         i = quick_read_message(msg_list[start].msg_num, pause);
      else if (sys.pip_board)
         i = pip_msg_read(usr.msg, msg_list[start].msg_num, 0, pause);
      else if (sys.squish)
         i = squish_read_message (msg_list[start].msg_num, pause);
      else
         i = read_message(msg_list[start].msg_num, pause);

      if (!i) {
         if (start > 1) {
            start--;
            if (last_read_system != msg_list[start].area) {
               read_system(msg_list[start].area, 1);
               if (sys.quick_board)
                  quick_scan_message_base (sys.quick_board, msg_list[start].area);
               else if (sys.pip_board)
                  pip_scan_message_base (msg_list[start].area);
               else if (sys.squish)
                  squish_scan_message_base (msg_list[start].area, sys.msg_path);
               else
                  scan_message_base(msg_list[start].area);
               last_read_system = msg_list[start].area;
               num_msg = last_msg = max_priv_mail;
               first_msg = 0;
            }
         }
         else
            break;
      }
      else
         break;
   }

   lastread = last_mail = start;
}

void mail_read_nonstop ()
{
   while (last_mail < max_priv_mail && !RECVD_BREAK())
   {
      if (local_mode && local_kbd == 0x03)
         break;
      mail_read_forward(1);
      time_release ();
   }
}

void mail_list()
{
   int fd, m, z, line = 3, last_read_system = 0;
   char stringa[10], filename[80];
   struct _msg msgt;

   if (!get_command_word (stringa, 4)) {
      m_print(bbstxt[B_START_WITH_MSG]);

      input(stringa,4);
      if(!strlen(stringa))
         return;
   }

   m = atoi(stringa);
   if(m < 1 || m > last_msg)
      return;

   cls();

   for (z = m; z <= max_priv_mail; z++) {
      if (msg_list[z].area != last_read_system) {
         read_system (msg_list[z].area, 1);
         usr.msg = 0;
         if (sys.quick_board)
            quick_scan_message_base (sys.quick_board, usr.msg);
         else if (sys.pip_board)
            pip_scan_message_base (sys.quick_board);
         else if (sys.squish)
            squish_scan_message_base (msg_list[z].area, sys.msg_path);
         else
            scan_message_base(sys.msg_num);
         m_print(bbstxt[B_AREALIST_HEADER],sys.msg_num,sys.msg_name);
         usr.msg = last_read_system = msg_list[z].area;
      }

      if (sys.quick_board) {
         if (!(line=quick_mail_header (msg_list[z].msg_num, line)))
            break;
      }
      else if (sys.pip_board) {
         if (!(line=pip_mail_list_header (msg_list[z].msg_num, sys.pip_board, line)))
            break;
      }
      else if (sys.squish) {
         if (!(line=squish_mail_list_headers (msg_list[z].msg_num, line)))
            break;
      }
      else {
         sprintf(filename,"%s%d.MSG",sys.msg_path,msg_list[z].msg_num);

         fd = shopen(filename,O_RDONLY|O_BINARY);
         if (fd == -1)
            continue;
         read(fd,(char *)&msgt,sizeof(struct _msg));
         close(fd);

         if((msgt.attr & MSGPRIVATE) && stricmp(msgt.from,usr.name) && stricmp(msgt.to,usr.name) && usr.priv < SYSOP)
            continue;

         if ((line = msg_attrib(&msgt,z,line,0)) == 0)
            break;
      }

      m_print(bbstxt[B_ONE_CR]);

      if (!(line=more_question(line)) || !CARRIER || RECVD_BREAK())
         break;

      time_release();
   }

   if (line && CARRIER)
      press_enter();
}

static int quick_index (z, area_msg)
int z, *area_msg;
{
   #define MAX_TOIDX_READ  10
   FILE *fpi, *fpn;
   int i, m, fdhdr;
   word pos;
   char filename[80], username[36], to[MAX_TOIDX_READ][36];
   byte boards[201], currmsg[201];
   struct _msghdr hdr;
   struct _sys tsys;
   struct _msgidx idx[MAX_TOIDX_READ];

   for (i = 1; i < 201; i++) {
      boards[i] = 0;
      currmsg[i] = 0;
   }

   sprintf (filename, SYSMSG_PATH, sys_path);
   fpi = fopen (filename, "rb");
   while (fread((char *)&tsys.msg_name,SIZEOF_MSGAREA,1,fpi) == 1) {
      if (!tsys.quick_board)
         continue;
      boards[tsys.quick_board] = tsys.msg_num;
   }
   fclose (fpi);

   sprintf (filename, "%sMSGHDR.BBS", fido_msgpath);
   fdhdr = shopen (filename, O_RDONLY|O_BINARY);
   if (fdhdr == -1)
      return (z);

   sprintf (filename, "%sMSGTOIDX.BBS", fido_msgpath);
   fpi = fopen (filename, "rb");
   if (fpi == NULL) {
      close (fdhdr);
      return (z);
   }

   sprintf (filename, "%sMSGIDX.BBS", fido_msgpath);
   fpn = fopen (filename, "rb");
   if (fpn == NULL) {
      close (fdhdr);
      fclose (fpi);
      return (z);
   }

   _BRK_ENABLE();
   pos = 0;

   do {
      i = fread (to, 36, MAX_TOIDX_READ, fpi);
      i = fread ((char *)&idx, sizeof (struct _msgidx), MAX_TOIDX_READ, fpn);

      if (!CARRIER || RECVD_BREAK())
         break;

      for (m = 0; m < i; m++) {
         currmsg[idx[m].board]++;
         strncpy (username, &to[m][1], to[m][0]);
         username[to[m][0]] = '\0';
         if (!stricmp (username, usr.name)) {
            lseek (fdhdr, (long)pos * sizeof (struct _msghdr), SEEK_SET);
            read (fdhdr, (char *)&hdr, sizeof (struct _msghdr));

            if (boards[hdr.board] && !(hdr.msgattr & Q_RECEIVED)) {
               if (z >= MAX_PRIV_MAIL)
                  break;

               msg_list[z].area = boards[hdr.board];
               msg_list[z++].msg_num = currmsg[hdr.board];
               area_msg[boards[hdr.board]-1]++;
            }
         }
         pos++;
      }
   } while (i == MAX_TOIDX_READ);

   close (fdhdr);
   fclose (fpn);
   fclose (fpi);

   return (z);
}

static int pipbase_index (z, area_msg)
int z, *area_msg;
{
   FILE *fpi;
   int i;
   char filename[80];
   byte boards[201];
   DESTPTR hdr;
   struct _sys tsys;

   for (i = 1; i < 201; i++)
      boards[i] = 0;

   sprintf (filename, SYSMSG_PATH, sys_path);
   fpi = fopen (filename, "rb");

   while (fread((char *)&tsys.msg_name,SIZEOF_MSGAREA,1,fpi) == 1) {
      if (!sys.pip_board)
         continue;
      boards[sys.pip_board] = sys.msg_num;
   }

   fclose (fpi);

   sprintf (filename, "%sDESTPTR", pip_msgpath);
   fpi = fopen (filename, "rb");
   if (fpi == NULL)
      return (z);

   _BRK_ENABLE();

   while (fread((char *)&hdr,sizeof(DESTPTR),1,fpi) == 1) {
      if (!CARRIER || RECVD_BREAK())
         break;
      if (!boards[hdr.area])
         continue;
      if (!stricmp (usr.name, hdr.to)) {
         if (z >= MAX_PRIV_MAIL)
            break;

         msg_list[z].area = boards[hdr.area];
         msg_list[z++].msg_num = hdr.msg;
         area_msg[boards[hdr.area]-1]++;
      }
   }

   fclose (fpi);
   return (z);
}

static int fido_index (z, area_msg)
int z, *area_msg;
{
   FILE *fp;
   int i, m, fd;
   char filename[50];
   struct _mail idxm[MAX_MAIL_BUFFER];
   struct _sys tsys;
   struct _msg tmsg;

   sprintf(filename,"%sMSGTOIDX.DAT",sys_path);
   fp = fopen(filename, "rb");

   if (fp != NULL) {
      _BRK_ENABLE();

      do {
         i = fread((char *)&idxm, sizeof(struct _mail), MAX_MAIL_BUFFER, fp);

         if (!CARRIER || RECVD_BREAK())
            break;

         for (m = 0; m < i; m++) {
            if(idxm[m].to != usr.id || !idxm[m].area || !read_system2 (idxm[m].area, 1, &tsys))
               continue;

            if (z >= MAX_PRIV_MAIL)
               break;

            sprintf (filename, "%s%d.MSG", tsys.msg_path, idxm[m].msg_num);
            fd = shopen (filename, O_RDONLY|O_BINARY);
            if (fd == -1)
               continue;
            read (fd, (char *)&tmsg, sizeof (struct _msg));
            close (fd);

            if (stricmp(tmsg.to,usr.name) || (tmsg.attr & MSGREAD))
               continue;

            msg_list[z].area = idxm[m].area;
            msg_list[z++].msg_num = idxm[m].msg_num;
            area_msg[idxm[m].area-1]++;
         }
      } while (i == MAX_MAIL_BUFFER && z < MAX_PRIV_MAIL);

      fclose (fp);
   }

   return (z);
}

static int squish_index (z, area_msg)
int z, *area_msg;
{
   FILE *fpi;
   int i;
   char filename[80];
   struct _sys tsys;
   MSG *sqm;
   MSGH *sq_msgh;
   XMSG xmsg;

   sprintf (filename, SYSMSG_PATH, sys_path);
   fpi = fopen (filename, "rb");

   while (fread((char *)&tsys.msg_name,SIZEOF_MSGAREA,1,fpi) == 1) {
      if (!sys.squish)
         continue;

      sqm = MsgOpenArea (tsys.msg_path, MSGAREA_NORMAL, MSGTYPE_SQUISH);

      _BRK_ENABLE();

      for (i = 1; i < (int)MsgGetNumMsg (sqm); i++) {
         if (!CARRIER || RECVD_BREAK()) {
            fseek (fpi, 0L, SEEK_END);
            break;
         }

         sq_msgh = MsgOpenMsg (sq_ptr, MOPEN_RW, (dword)i);
         if (sq_msgh == NULL)
            continue;

         if (MsgReadMsg (sq_msgh, &xmsg, 0L, 0L, NULL, 0, NULL) == -1) {
            MsgCloseMsg (sq_msgh);
            return (0);
         }

         if (!stricmp (usr.name, xmsg.to)) {
            if (z >= MAX_PRIV_MAIL)
               break;

            msg_list[z].area = sys.msg_num;
            msg_list[z++].msg_num = i;
            area_msg[sys.msg_num - 1]++;
         }

         MsgCloseMsg (sq_msgh);
      }

      MsgCloseArea (sqm);
   }

   fclose (fpi);

   return (z);
}

