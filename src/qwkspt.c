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
#include <cxl\cxlstr.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "quickmsg.h"

union Converter {
   unsigned char  uc[10];
   unsigned int   ui[5];
   unsigned long  ul[2];
   float           f[2];
   double          d[1];
};

static void update_lastread_pointers (void);
static int fido_scan_messages (int *, int *, int, int, int, int, FILE *, char);
static int prescan_tagged_areas (char, FILE *);
static int is_tagged (int);
static int tag (int);
static int un_tag (int);
static char * namefixup (char *);

void tag_area_list (flag, sig) /* flag == 1, Normale due colonne */
int flag, sig;                       /* flag == 2, Normale una colonna */
{                                    /* flag == 3, Anche nomi, una colonna */
   int fd, fdi, i, pari, area, linea, nsys, nm;
   char stringa[13], filename[50], dir[80];
   struct _sys tsys;
   struct _sys_idx sysidx[10];

   sprintf(filename,"%sSYSMSG.IDX", sys_path);
   fdi = shopen(filename, O_RDONLY|O_BINARY);
   if (fdi == -1)
      return;

   m_print(bbstxt[B_AREA_TAG_UNTAG]);

   for (;;) {
      if (!get_command_word (stringa, 12)) {
         change_attr(WHITE|_BLACK);

         m_print(bbstxt[B_TAG_AREA]);
         area = 0;

         input(stringa, 12);
         if (!CARRIER || time_remain() <= 0) {
            close (fdi);
            return;
         }
      }

      if (stringa[0] == '?') {
         sprintf(filename, SYSMSG_PATH, sys_path);
         fd = shopen(filename, O_RDONLY|O_BINARY);
         if (fd == -1) {
            close (fdi);
            return;
         }

         cls();
         m_print(bbstxt[B_AREAS_TITLE], bbstxt[B_MESSAGE]);

         pari = 0;
         linea = 4;
         nm = 0;
         lseek(fdi, 0L, SEEK_SET);

         do {
            nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
            nsys /= sizeof (struct _sys_idx);

            for (i=0; i < nsys; i++, nm++) {
               if (usr.priv < sysidx[i].priv || (usr.flags & sysidx[i].flags) != sysidx[i].flags)
                  continue;

               lseek(fd, (long)nm * SIZEOF_MSGAREA, SEEK_SET);
               read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA);

               if (sig && tsys.msg_sig != sig)
                  continue;

               m_print("%c%3d ... ",is_tagged(sysidx[i].area) ? '@' : ' ',sysidx[i].area);

               if (flag == 1) {
                  strcpy(dir, tsys.msg_name);
                  dir[31] = '\0';
                  if (pari) {
                     m_print("%s\n",dir);
                     pari = 0;
                     if (!(linea = more_question(linea)))
                        break;
                  }
                  else {
                     m_print("%-31s ",dir);
                     pari = 1;
                  }
               }
               else if (flag == 2 || flag == 3) {
                  if (flag == 3)
                     m_print("%-12s ",sysidx[i].key);
                  m_print(" %s\n",tsys.msg_name);

                  if (!(linea = more_question(linea)))
                     break;
               }
            }
         } while (linea && nsys == 10);

         close(fd);

         if (pari)
            m_print(bbstxt[B_ONE_CR]);

         m_print(bbstxt[B_CURRENTLY_TAGGED]);

         area = -1;
      }

      else if (stringa[0] == '-') {
         for (i = 0; i < MAXLREAD; i++)
            un_tag (usr.lastread[i].area);
      }

      else if (strlen(stringa) < 1) {
         close (fdi);
         return;
      }

      else {
         lseek(fdi, 0L, SEEK_SET);
         area = atoi(stringa);
         if (area < 1 || area > MSG_AREAS) {
            area = -1;
            do {
               nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
               nsys /= sizeof (struct _sys_idx);

               for(i=0;i<nsys;i++) {
                  if (usr.priv < sysidx[i].priv || (usr.flags & sysidx[i].flags) != sysidx[i].flags)
                     continue;
                  if (!stricmp(stringa,sysidx[i].key)) {
                     area = sysidx[i].area;
                     break;
                  }
               }
            } while (i == nsys && nsys == 10);
         }

         if (area && read_system2 (area, 1, &tsys)) {
            if (is_tagged (area)) {
               un_tag (area);
               m_print (bbstxt[B_QWK_AREATAG], area, tsys.msg_name, bbstxt[B_QWK_UNTAGGED]);
            }
            else {
               tag (area);
               m_print (bbstxt[B_QWK_AREATAG], area, tsys.msg_name, bbstxt[B_QWK_TAGGED]);
            }
         }
      }
   }
}

static int is_tagged (area)
int area;
{
   int i;

   for (i=0;i<MAXLREAD;i++)
      if (usr.lastread[i].area == area)
         break;

   if (i == MAXLREAD)
      return (0);

   return (1);
}

static int tag (area)
int area;
{
   int i;

   for (i=0;i<MAXLREAD;i++)
      if (!usr.lastread[i].area)
         break;

   if (i == MAXLREAD)
      return (0);

   usr.lastread[i].area = area;
   usr.lastread[i].msg_num = 0;

   return (1);
}

static int un_tag (area)
int area;
{
   int i;

   for (i=0;i<MAXLREAD;i++)
      if (usr.lastread[i].area == area)
         break;

   if (i == MAXLREAD)
      return (0);

   usr.lastread[i].area = 0;
   usr.lastread[i].msg_num = 0;

   return (1);
}

int sort_func (const void *a1, const void *b1)
{
   struct _lastread *a, *b;
   a = (struct _lastread *)a1;
   b = (struct _lastread *)b1;
   return ( (int)(a->area - b->area) );
}

void pack_tagged_areas ()
{
   char file[80], stringa[80], c;
   struct ffblk fbuf;

   qsort (&usr.lastread, MAXLREAD, sizeof (struct _lastread), sort_func);
   getcwd (stringa, 79);

   sprintf (file, "%sTASK%02X", QWKDir, line_offset);
   mkdir (file);

   if (chdir (file) == -1)
      return;

   if (!findfirst ("*.*", &fbuf, 0)) {
      unlink (fbuf.ff_name);
      while (!findnext (&fbuf))
         unlink (fbuf.ff_name);
   }

   chdir (stringa);
   m_print (bbstxt[B_ONE_CR]);

   if (!prescan_tagged_areas (0, NULL))
      return;

   do {
      m_print (bbstxt[B_ASK_COMPRESSOR]);

      if (!cmd_string[0]) {
         input (stringa, 1);
         if (!CARRIER)
            return;
         c = toupper(stringa[0]);
      }
      else {
         c = toupper(cmd_string[0]);
         strcpy (&cmd_string[0], &cmd_string[1]);
         strtrim (cmd_string);
      }
   } while (c != '\0' && c != 'A' && c != 'L' && c != 'Z');

   m_print (bbstxt[B_PLEASE_WAIT]);

   switch (c) {
      case 'A':
         sprintf (file, "%sTASK%02X\\%s.ARJ", QWKDir, line_offset, BBSid);
         unlink (file);
         sprintf (stringa, "ARJ m -e %sTASK%02X\\%s %sTASK%02X\\*.* *M",
                  QWKDir, line_offset, BBSid, QWKDir, line_offset);
         outside_door (stringa);
         break;
      case 'L':
         sprintf (file, "%sTASK%02X\\%s.LZH", QWKDir, line_offset, BBSid);
         unlink (file);
         sprintf (stringa, "LHARC m %sTASK%02X\\%s %sTASK%02X\\*.* *M",
                  QWKDir, line_offset, BBSid, QWKDir, line_offset);
         outside_door (stringa);
         break;
      case '\0':
      case 'Z':
         sprintf (file, "%sTASK%02X\\%s.ZIP", QWKDir, line_offset, BBSid);
         unlink (file);
         sprintf (stringa, "PKZIP -m %sTASK%02X\\%s %sTASK%02X\\*.* *M",
                  QWKDir, line_offset, BBSid, QWKDir, line_offset);
         outside_door (stringa);
         break;
   }

   m_print(bbstxt[B_ONE_CR]);
   cps = 0;
   download_file (file, 0);
   if (cps)
      unlink (file);

   update_lastread_pointers ();
}

void resume_transmission ()
{
   char file[80], *p;
   struct ffblk fbuf;

   sprintf (file, "%sTASK%02X\\%s.*", QWKDir, line_offset, BBSid);

   if (!findfirst (file, &fbuf, 0))
      do {
         if ((p=strchr (fbuf.ff_name, '.')) == NULL)
            continue;
         if (!stricmp (p, ".QWK") || !stricmp (p, ".ZIP") ||
          !stricmp (p, ".ARJ") || !stricmp (p, ".LZH")) {
            sprintf (file, "%sTASK%02X\\%s", QWKDir, line_offset, fbuf.ff_name);
            cps = 0;
            download_file (file, 0);
            if (cps)
               unlink (file);
            break;
         }
      } while (!findnext (&fbuf));
}

void qwk_pack_tagged_areas ()
{
   FILE *fpq;
   int i, fd, totals;
   char file[80], stringa[80], c;
   time_t aclock;
   struct _sys tsys;
   struct ffblk fbuf;
   struct tm *newtime;

   qsort (&usr.lastread, MAXLREAD, sizeof (struct _lastread), sort_func);
   getcwd (stringa, 79);

   sprintf (file, "%sTASK%02X", QWKDir, line_offset);
   mkdir (file);

   if (chdir (file) == -1)
      return;

   if (!findfirst ("*.*", &fbuf, 0)) {
      unlink (fbuf.ff_name);
      while (!findnext (&fbuf))
         unlink (fbuf.ff_name);
   }

   chdir (stringa);
   m_print (bbstxt[B_ONE_CR]);

   memcpy ((char *)&tsys, (char *)&sys, sizeof (struct _sys));

   sprintf (file, "%sTASK%02X\\CONTROL.DAT", QWKDir, line_offset);
   fpq = fopen(file, "wt");
   if (fpq == NULL)
      return;

//   strcpy(stringa,system_name);
//   p = strtok(stringa," \r\n");
   fputs(system_name,fpq);         /* Line #1 */
   fputs("\n",fpq);
   fputs("               \n",fpq);
   fputs("               \n",fpq);
   fputs(sysop,fpq);
   fputs("\n",fpq);
   sprintf(stringa,"00000,%s\n",BBSid);         /* Line #5 */
   fputs(stringa,fpq);
   time(&aclock);
   newtime = localtime(&aclock);
   sprintf(stringa,"%02d-%02d-19%02d,%02d:%02d:%02d\n",
      newtime->tm_mon+1,newtime->tm_mday,newtime->tm_year,
      newtime->tm_hour,newtime->tm_min,newtime->tm_sec);
   fputs(stringa,fpq);              /* Line #6 */
//   strcpy (stringa, usr.name);
   fputs(usr.name,fpq);
   fputs("\n",fpq);
   fputs(" \n",fpq);             /* Line #8 */
   fputs("0\n",fpq);
   fputs("0\n",fpq);             /* Line #10 */
   sprintf (stringa, SYSMSG_PATH, sys_path);
   fd = shopen (stringa, O_RDONLY|O_BINARY);
   totals = 0;
   for (i=0; i < 1000; i++) {
      if ((read (fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA)) != SIZEOF_MSGAREA)
         break;
      if (usr.priv < tsys.msg_priv)
         continue;
      if ((usr.flags & tsys.msg_flags) != tsys.msg_flags)
         continue;
      totals++;
   }
   sprintf (stringa, "%d\n", totals-1);
   fputs (stringa, fpq);
   lseek (fd, 0L, SEEK_SET);
   for (i=0; i < 1000; i++) {
      if ((read (fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA)) != SIZEOF_MSGAREA)
         break;
      if (usr.priv < tsys.msg_priv)
         continue;
      if ((usr.flags & tsys.msg_flags) != tsys.msg_flags)
         continue;
      sprintf (stringa, "%d\n", tsys.msg_num);
      fputs (stringa, fpq);
      memset (stringa, 0, 11);
      strncpy (stringa, tsys.msg_name, 40);
      fputs (stringa, fpq);
      fputs ("\n", fpq);
   }

   fputs ("WELCOME\n", fpq);          /* This is QWK welcome file */
   fputs ("NEWS\n", fpq);          /* This is QWK news file */
   fputs ("GOODBYE\n", fpq);          /* This is QWK bye file */

   close (fd);
   fclose (fpq);

   sprintf (file, "%sTASK%02X\\DOOR.ID", QWKDir, line_offset);
   fpq = fopen(file, "wt");
   if (fpq == NULL)
      return;

   sprintf(stringa,"DOOR = LoraBBS\n");
   fputs(stringa,fpq);           /* Line #1 */

   sprintf(stringa,"VERSION = 2.20\n");
   fputs(stringa,fpq);           /* Line #2 */

   sprintf(stringa,"SYSTEM = LoraBBS\n");
   fputs(stringa,fpq);           /* Line #3 */

   sprintf(stringa,"CONTROLNAME = LORA\n");
   fputs(stringa,fpq);           /* Line #4 */

   sprintf(stringa,"CONTROLTYPE = ADD\n");
   fputs(stringa,fpq);           /* Line #5 */

   sprintf(stringa,"CONTROLTYPE = DROP\n");
   fputs(stringa,fpq);           /* Line #6 */
   sprintf(stringa,"CONTROLTYPE = REQUEST\n");
   fputs(stringa,fpq);           /* Line #6 */

   fclose (fpq);

   sprintf (file, "%sTASK%02X\\MESSAGES.DAT", QWKDir, line_offset);
   fpq = fopen(file, "w+b");
   if (fpq == NULL)
      return;

   fwrite("Produced by Qmail...",20,1,fpq);
   fwrite("Copywright (c) 1987 by Sparkware.  ",35,1,fpq);
   fwrite("All Rights Reserved",19,1,fpq);
   for (i=0; i < 54; i++)
       fwrite(" ",1,1,fpq);

   if (!prescan_tagged_areas (1, fpq))
      return;

   do {
      m_print (bbstxt[B_ASK_COMPRESSOR]);

      if (!cmd_string[0]) {
         input (stringa, 1);
         if (!CARRIER)
            return;
         c = toupper(stringa[0]);
      }
      else {
         c = toupper(cmd_string[0]);
         strcpy (&cmd_string[0], &cmd_string[1]);
         strtrim (cmd_string);
      }
   } while (c != '\0' && c != 'A' && c != 'L' && c != 'Z');

   m_print (bbstxt[B_PLEASE_WAIT]);
   sprintf (file, "%sTASK%02X\\%s.QWK", QWKDir, line_offset, BBSid);
   unlink (file);

   switch (c) {
      case 'A':
         sprintf (stringa, "ARJ m -e %sTASK%02X\\%s.QWK %sTASK%02X\\*.* *M",
                  QWKDir, line_offset, BBSid, QWKDir, line_offset);
         outside_door (stringa);
         break;
      case 'L':
         sprintf (stringa, "LHARC m /m %sTASK%02X\\%s.QWK %sTASK%02X\\*.* *M",
                  QWKDir, line_offset, BBSid, QWKDir, line_offset);
         outside_door (stringa);
         break;
      case '\0':
      case 'Z':
         sprintf (stringa, "PKZIP -m %sTASK%02X\\%s.QWK %sTASK%02X\\*.* *M",
                  QWKDir, line_offset, BBSid, QWKDir, line_offset);
         outside_door (stringa);
         break;
   }

   m_print(bbstxt[B_ONE_CR]);
   cps = 0;
   download_file (file, 0);
   if (cps)
      unlink (file);

   update_lastread_pointers ();
}

float IEEToMSBIN(float f)
{
   union Converter t;
   int   sign,exp;

   t.f[0] = f;

/* Extract sign & change exponent bias from 0x7f to 0x81 */

   sign = t.uc[3] / 0x80;
   exp  = ((t.ui[1] >> 7) - 0x7f + 0x81) & 0xff;

/* reassemble them in MSBIN format */
   t.ui[1] = (t.ui[1] & 0x7f) | (sign << 7) | (exp << 8);
   return t.f[0];
}


/* Fetch REP packet from user and import if ok */
void getrep(void)
{
   #define TRUE  1
   #define FALSE 0
   int x,y,z,msgfile;
   int endspace, upmsgs, repfile, prev_area, msgrecs;
   char  *msgbuff, *p, temp[80], temp1[80];
   struct QWKmsghd *MsgHead;
   struct ffblk fbuf;

   if (!local_mode) {
      getcwd (temp, 79);

      sprintf (temp1, "%sTASK%02X", QWKDir, line_offset);
      mkdir (temp1);

      if (chdir (temp1) == -1)
         return;

      if (!findfirst ("*.*", &fbuf, 0)) {
         unlink (fbuf.ff_name);
         while (!findnext (&fbuf))
            unlink (fbuf.ff_name);
      }

      chdir (temp);

      if ((y=selprot()) == 0)
         return;

      sprintf (temp, "%sTASK%02X\\", QWKDir, line_offset);
      sprintf (temp1, "%s.REP", BBSid);

      m_print(bbstxt[B_READY_TO_RECEIVE],protocols[y-1]);
      m_print(bbstxt[B_CTRLX_ABORT]);
      timer(50);

      switch (y) {
      case 1:
         who_is_he = 0;
         overwrite = 0;
         p = receive (temp, temp1, 'X');
         break;

      case 2:
         who_is_he = 0;
         overwrite = 0;
         p = receive (temp, temp1, 'Y');
         break;

      case 3:
         x = get_Zmodem (temp, NULL);
         break;

      case 6:
         x = 0;
         do {
            who_is_he = 0;
            overwrite = 0;
            p = receive (temp, temp1, 'S');
            if (p != NULL)
               x = 1;
         } while (p != NULL);
         break;
      }

      wactiv (mainview);

      CLEAR_INBOUND();
      if (x || p != NULL) {
         m_print(bbstxt[B_TRANSFER_OK]);
         timer(20);
      }
      else {
         m_print(bbstxt[B_TRANSFER_ABORT]);
         press_enter();
         return;
      }
   }

   sprintf (temp, "%sTASK%02X\\%s.RE?", QWKDir, line_offset, BBSid);
   if (findfirst (temp, &fbuf, 0))
      return;
   sprintf (temp, "%sTASK%02X\\%s", QWKDir, line_offset, fbuf.ff_name);
   repfile = shopen (temp, O_RDONLY|O_BINARY);
   if (repfile == -1)
      return;
   read (repfile, temp1, 10);
   close (repfile);

   if (!strncmp (temp1, "PK", 2))
      sprintf (temp, "PKUNZIP %sTASK%02X\\%s %sTASK%02X*M", QWKDir, line_offset, fbuf.ff_name, QWKDir, line_offset);
   else if (!strncmp (&temp1[2], "-lh", 3))
      sprintf (temp, "LHARC e %sTASK%02X\\%s %sTASK%02X\\*M", QWKDir, line_offset, fbuf.ff_name, QWKDir, line_offset);
   else if (!strncmp (temp1, "`ê", 2))
      sprintf (temp, "ARJ e %sTASK%02X\\%s %sTASK%02X\\*M", QWKDir, line_offset, fbuf.ff_name, QWKDir, line_offset);
   else {
      unlink (temp);
      return;
   }

   outside_door (temp);

   upmsgs = 0;
   prev_area = 0;

   sprintf (temp, "%sTASK%02X\\%s.MSG", QWKDir, line_offset, BBSid);
   repfile = shopen (temp, O_BINARY|O_RDONLY);
   if (repfile >= 0) {
      MsgHead = (struct QWKmsghd *) malloc(sizeof(struct QWKmsghd));
      if (MsgHead == NULL)
         return;
      msgbuff = (char *) malloc(150);
      if (msgbuff == NULL)
         return;
      read(repfile,msgbuff,128);          /* Ignore first record */
      msgbuff[8] = 0;
      strtrim(msgbuff);
      if (strcmpi(BBSid,msgbuff) == 0) {          /* Make sure it's ours! */
         m_print (bbstxt[B_HANG_ON]);
         x = read(repfile,(struct QWKmsghd *) MsgHead,128);
         while (x == 128) {               /* This is the BBS.MSG import loop */
         /* Build the Fidonet message head */
            x = atoi(MsgHead->Msgnum) + 1;            /* Area number */
            if (prev_area != x) {
               read_system (x, 1);
               usr.msg = x;

               if (sys.quick_board)
                  quick_scan_message_base (sys.quick_board, usr.msg, 1);
               else if (sys.pip_board)
                  pip_scan_message_base (usr.msg, 1);
               else if (sys.squish)
                  squish_scan_message_base (usr.msg, sys.msg_path, 1);
               else
                  scan_message_base(usr.msg, 1);
               prev_area = x;
            }

            upmsgs++;
            m_print (bbstxt[B_IMPORTING_MESSAGE], upmsgs);

            memset ((char *)&msg, 0, sizeof (struct _msg));
            msgrecs = atoi(MsgHead->Msgrecs) - 1;
            /* Parse date and time junk */
            strncpy(temp,MsgHead->Msgdate,8);
            temp[8] = 0;
            p = strtok(temp,"-");              /* Fetch month */
            x = atoi(p);
            p = strtok(NULL,"-");              /* Fetch day */
            z = atoi(p);
            p = strtok(NULL," \0-");           /* Fetch year */
            y = (atoi(p) - 80);

            strncpy(temp,MsgHead->Msgtime,5);
            temp[5] = 0;
            sprintf (msg.date,"%d %s %d %s:00",z,mtext[x-1],y+80,temp);

            msg_fzone=alias[sys.use_alias].zone;
            msg.orig=alias[sys.use_alias].node;
            msg.orig_net=alias[sys.use_alias].net;
            msg.dest=alias[sys.use_alias].node;
            msg.dest_net=alias[sys.use_alias].net;

            if (MsgHead->Msgstat == '*') {
               if (!sys.public)
                  msg.attr |= MSGPRIVATE;
            }

            if (sys.private)
               msg.attr |= MSGPRIVATE;

            strcpy(msg.to,namefixup(MsgHead->Msgto));
            fancy_str (msg.to);
            strcpy(msg.from,namefixup(MsgHead->Msgfrom));
            fancy_str (msg.from);
            strcpy(msg.subj,namefixup(MsgHead->Msgsubj));

            sprintf(temp1,"%s%d.TXT", QWKDir, line_offset);
            msgfile = cshopen (temp1, O_TRUNC|O_CREAT|O_BINARY|O_RDWR, S_IREAD|S_IWRITE);

            /* Now prepare to read in the REP msg and convert to fidonet */
            for (x=0; x < msgrecs; x++) {
               read(repfile,msgbuff,128);
               msgbuff[128] = 0;
               p = strchr(msgbuff,0xe3);            /* Look for special linefeeds */
               while (p) {
                  *p = 0x0d;                /* Replace as a cr */
                  p = strchr(p+1,0xe3);
               }
               p = msgbuff + 127;
               endspace = TRUE;

            /* Now go backwards and strip out '---' after a c/r */
               while (p > msgbuff) {
                  if (*p == '-' && sys.echomail) {
                     endspace = FALSE;
                     p--;
                     if (*p == '-') {
                        p--;
                        if (*p == '-') {
                           if (*(p-1) == '\r') {
                              *p = ' ';
                              *(p+1) = ' ';
                              *(p+2) = ' ';
                           }
                           break;
                        }
                     }
                  }

                  if (*p & 0x80) {  /* Hi bit set? */
                     *p &= 0x7f;         /* Make it harmless */
                     endspace = FALSE;
                  }

                  if ((x == (msgrecs - 1)) && *p == ' ' && endspace && *(p-1) == ' ')
                     *p = 0;
                  else
                     endspace = FALSE;
                  p--;
               }

               write(msgfile,msgbuff,strlen(msgbuff));
            }

            close (msgfile);

            if (sys.quick_board)
               quick_save_message(temp1);
            else if (sys.pip_board)
               pip_save_message(temp1);
            else if (sys.squish)
               squish_save_message (temp1);
            else
               save_message(temp1);

            unlink (temp1);

            x = read(repfile,(struct QWKmsghd *) MsgHead,128);
         }
      }
   }

   if (upmsgs) {
      read_system (usr.msg, 1);
      m_print (bbstxt[B_TOTAL_IMPORTED], upmsgs);
   }
}

static char * namefixup(char *name)
{
   name[20] = 0;
   strtrim (name);

   return (name);
}

static int prescan_tagged_areas (qwk, fpq)
char qwk;
FILE *fpq;
{
   int fdi, fdp;
   int i, msgcount, tt, pp, last[MAXLREAD], personal, total, totals;
   char file[80];
   struct _sys tsys;

   qsort (&usr.lastread, MAXLREAD, sizeof (struct _lastread), sort_func);

   total = personal = msgcount = 0;
   totals = 1;
   memcpy ((char *)&tsys, (char *)&sys, sizeof (struct _sys));

   if (qwk) {
      sprintf (file, "%sTASK%02X\\PERSONAL.NDX", QWKDir, line_offset);
      fdp = cshopen(file,O_CREAT | O_TRUNC | O_BINARY | O_WRONLY,S_IREAD|S_IWRITE);
   }
   else
      fdp = -1;

   m_print (bbstxt[B_QWK_HEADER1]);
   m_print (bbstxt[B_QWK_HEADER2]);

   for (i=0; i<MAXLREAD && CARRIER; i++) {
      last[i] = usr.lastread[i].msg_num;

      if (!usr.lastread[i].area)
         continue;

      if (!read_system (usr.lastread[i].area, 1))
         continue;

      if (!qwk) {
         sprintf (file, "%sTASK%02X\\%03d.TXT", QWKDir, line_offset, usr.lastread[i].area);
         fpq = fopen(file, "w+t");
         fprintf (fpq, "[*] AREA: %s\n\n", sys.msg_name);
      }

      m_print (bbstxt[B_SCAN_SEARCHING], sys.msg_num, sys.msg_name);

      tt = pp = 0;

      if (qwk) {
         sprintf (file, "%sTASK%02X\\%03d.NDX", QWKDir, line_offset, usr.lastread[i].area);
         fdi = cshopen(file,O_CREAT | O_TRUNC | O_BINARY | O_WRONLY,S_IREAD|S_IWRITE);
      }
      else
         fdi = -1;

      if (sys.quick_board) {
         quick_scan_message_base (sys.quick_board, usr.lastread[i].area, 1);
         totals = quick_scan_messages (&tt, &pp, sys.quick_board, usr.lastread[i].msg_num + 1, fdi, fdp, totals, fpq, qwk);
      }
      else if (sys.pip_board) {
         pip_scan_message_base (usr.lastread[i].area, 1);
         totals = pip_scan_messages (&tt, &pp, sys.pip_board, usr.lastread[i].msg_num + 1, fdi, fdp, totals, fpq, qwk);
      }
      else if (sys.squish) {
         squish_scan_message_base (usr.lastread[i].area, sys.msg_path, 1);
         totals = squish_scan_messages (&tt, &pp, usr.lastread[i].msg_num + 1, fdi, fdp, totals, fpq, qwk);
      }
      else {
         scan_message_base(usr.lastread[i].area, 1);
         totals = fido_scan_messages (&tt, &pp, usr.lastread[i].msg_num + 1, fdi, fdp, totals, fpq, qwk);
      }

      close (fdi);
      if (!qwk)
         fclose (fpq);

      if (!tt) {
         if (qwk)
            sprintf (file, "%sTASK%02X\\%03d.NDX", QWKDir, line_offset, usr.lastread[i].area);
         else
            sprintf (file, "%sTASK%02X\\%03d.TXT", QWKDir, line_offset, usr.lastread[i].area);
         unlink (file);
      }

      m_print (bbstxt[B_QWK_STATISTICS], last_msg, pp, tt);

      msgcount += tt;
      personal += pp;
      total += last_msg;
      time_release ();
   }

   if (qwk)
      fclose (fpq);

   if (fdp != -1) {
      close (fdp);
      if (!personal) {
         sprintf (file, "%sTASK%02X\\PERSONAL.NDX", QWKDir, line_offset);
         unlink (file);
      }
   }

   m_print (bbstxt[B_QWK_HEADER2]);
   m_print (bbstxt[B_PACKED_MESSAGES], total, personal, msgcount);

   memcpy ((char *)&sys, (char *)&tsys, sizeof (struct _sys));

   if (sys.quick_board)
      quick_scan_message_base (sys.quick_board, usr.msg, 1);
   else if (sys.pip_board)
      pip_scan_message_base (usr.msg, 1);
   else if (sys.squish)
      squish_scan_message_base (usr.msg, sys.msg_path, 1);
   else
      scan_message_base(usr.msg, 1);

   if (msgcount) {
      if (qwk)
         m_print(bbstxt[B_ASK_DOWNLOAD_QWK]);
      else
         m_print(bbstxt[B_ASK_DOWNLOAD_ASCII]);
      if (yesno_question (DEF_YES) == DEF_NO)
         return (0);
   }
   else
      press_enter ();

   return (msgcount);
}

static void update_lastread_pointers ()
{
   int i;
   struct _sys tsys;

   m_print (bbstxt[B_QWK_UPDATE]);

   memcpy ((char *)&tsys, (char *)&sys, sizeof (struct _sys));

   for (i=0; i<MAXLREAD; i++) {
      if (!usr.lastread[i].area)
         continue;

      if (!read_system (usr.lastread[i].area, 1))
         continue;

      if (sys.quick_board)
         quick_scan_message_base (sys.quick_board, usr.lastread[i].area, 1);
      else if (sys.pip_board)
         pip_scan_message_base (usr.lastread[i].area, 1);
      else if (sys.squish)
         squish_scan_message_base (usr.lastread[i].area, sys.msg_path, 1);
      else
         scan_message_base(usr.lastread[i].area, 1);

      usr.lastread[i].msg_num = last_msg;
   }

   memcpy ((char *)&sys, (char *)&tsys, sizeof (struct _sys));

   if (sys.quick_board)
      quick_scan_message_base (sys.quick_board, usr.msg, 1);
   else if (sys.pip_board)
      pip_scan_message_base (usr.msg, 1);
   else if (sys.squish)
      squish_scan_message_base (usr.msg, sys.msg_path, 1);
   else
      scan_message_base(usr.msg, 1);
}

static int fido_scan_messages (total, personal, start, fdi, fdp, totals, fpq, qwk)
int *total, *personal, start, fdi, fdp, totals;
FILE *fpq;
char qwk;
{
   FILE *fp;
   float in, out;
   int i, tt, pp, z, m, pos, blks, msg_num;
   char c, buff[80], wrp[80], qwkbuffer[130], shead;
   long fpos, qpos;
   struct QWKmsghd QWK;
   struct _msg msgt;

   tt = 0;
   pp = 0;

   for(msg_num = start; msg_num <= last_msg; msg_num++) {
      if (!(msg_num % 5))
         display_percentage (msg_num - start, last_msg - start);

      sprintf(buff,"%s%d.MSG",sys.msg_path,msg_num);

      fp = fopen (buff, "rb");
      if(fp == NULL)
         continue;
      fread((char *)&msgt,sizeof(struct _msg),1,fp);

      if((msgt.attr & MSGPRIVATE) && stricmp(msgt.from,usr.name) && stricmp(msgt.to,usr.name) && usr.priv < SYSOP) {
         fclose (fp);
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

      if (!stricmp(msgt.to, usr.name)) {
         pp++;
         if (fdp != -1) {
            write(fdp,&out,sizeof(float));
            write(fdp,&c,sizeof(char));              /* Conference # */
         }
      }

      blks = 1;
      pos = 0;
      memset (qwkbuffer, ' ', 128);
      shead = 0;
      i = 0;
      fpos = filelength(fileno(fp));

      while(ftell(fp) < fpos) {
         c = fgetc(fp);

         if((byte)c == 0x8D || c == 0x0A || c == '\0')
            continue;

         buff[i++]=c;

         if(c == 0x0D) {
            buff[i-1]='\0';

            if(buff[0] == 0x01) {
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
               if (qwk)
                  qwk_header (&msgt,&QWK,msg_num,fpq,&qpos);
               else
                  text_header (&msgt,msg_num,fpq);
               shead = 1;
            }

            if(buff[0] == 0x01 || !strncmp(buff,"SEEN-BY",7)) {
               i=0;
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
            if(i<(usr.width-1))
               continue;

            buff[i]='\0';
            while(i>0 && buff[i] != ' ')
               i--;

            m=0;

            if(i != 0)
               for(z=i+1;z<(usr.width-1);z++) {
                  wrp[m++]=buff[z];
                  buff[i]='\0';
               }

            wrp[m]='\0';

            if (!shead) {
               if (qwk)
                  qwk_header (&msgt,&QWK,msg_num,fpq,&qpos);
               else
                  text_header (&msgt,msg_num,fpq);
               shead = 1;
            }

            if (qwk) {
               write_qwk_string (buff, qwkbuffer, &pos, &blks, fpq);
               write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpq);
            }
            else
               fprintf(fpq,"%s\n",buff);

            buff[0] = '\0';
            strcat(buff,wrp);
            i = strlen(wrp);
         }
      }

      fclose(fp);

      if (qwk) {
         qwkbuffer[128] = 0;
         fwrite(qwkbuffer, 128, 1, fpq);
         blks++;

      /* Now update with record count */
         fseek(fpq,qpos,SEEK_SET);          /* Restore back to header start */
         sprintf(buff,"%d",blks);
         ljstring(QWK.Msgrecs,buff,6);
         fwrite((char *)&QWK,128,1,fpq);           /* Write out the header */
         fseek(fpq,0L,SEEK_END);               /* Bump back to end of file */

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

