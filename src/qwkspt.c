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

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "quickmsg.h"

extern char *VNUM, no_description, no_check, no_external;

union Converter {
   unsigned char  uc[10];
   unsigned short ui[5];
   unsigned long  ul[2];
   float           f[2];
   double          d[1];
};

int spawn_program (int swapout, char *outstring);
void update_lastread_pointers (void);
void m_print2(char *format, ...);

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

   sprintf(filename,"%sSYSMSG.IDX", config->sys_path);
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
         sprintf(filename, SYSMSG_PATH, config->sys_path);
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

int selpacker (void)
{
   int i, m;
   char c, stringa[4], cmd[20];

reread:
   if (!usr.archiver) {
      if ((c=get_command_letter ()) == '\0') {
         if (!read_system_file ("DEFCOMP")) {
            m_print (bbstxt[B_AVAIL_COMPR]);

            m = 0;
            for (i = 0; i < 10; i++) {
               if (config->packid[i].display[0]) {
                  m_print (bbstxt[B_PROTOCOL_FORMAT], config->packid[i].display[0], &config->packid[i].display[1]);
                  cmd[m++] = config->packid[i].display[0];
               }
            }
            m_print (bbstxt[B_PROTOCOL_FORMAT], bbstxt[B_HELP][0], &bbstxt[B_HELP][1]);
            cmd[m++] = bbstxt[B_HELP][0];
            cmd[m] = '\0';

            m_print (bbstxt[B_ASK_COMPRESSOR]);
         }
         else {
            m = 0;
            for (i = 0; i < 10; i++) {
               if (config->packid[i].display[0])
                  cmd[m++] = config->packid[i].display[0];
            }
            cmd[m++] = bbstxt[B_HELP][0];
            cmd[m] = '\0';
         }

         if (usr.hotkey) {
            cmd_input (stringa, 1);
            m_print (bbstxt[B_ONE_CR]);
         }
         else
            input (stringa, 1);
         c = stringa[0];
         if (c == '\0' || c == '\r' || !CARRIER)
            return (0);
      }
   }
   else {
      m = 0;
      for (i = 0; i < 10; i++) {
         if (config->packid[i].display[0])
            cmd[m++] = config->packid[i].display[0];
      }
      cmd[m++] = bbstxt[B_HELP][0];
      cmd[m] = '\0';

      c = usr.archiver;
   }

   c = toupper(c);
   strupr (cmd);

   if (strchr (cmd, c) == NULL && usr.archiver != '\0') {
      usr.archiver = '\0';
      goto reread;
   }
   else if (strchr (cmd, c) == NULL && usr.archiver == '\0')
      return (0);

   if (c == bbstxt[B_HELP][0]) {
      read_system_file ("COMPHELP");
      goto reread;
   }

   return (c);
}

/*
   char c, stringa[4];

   for (;;) {
      if ((c = get_command_letter ()) == '\0') {
         if (!usr.archiver) {
            if (!read_system_file ("DEFCOMP")) {
               m_print (bbstxt[B_AVAIL_COMPR]);
               m_print (bbstxt[B_PROTOCOL_FORMAT], bbstxt[B_ZIP][0], &bbstxt[B_ZIP][1]);
               m_print (bbstxt[B_PROTOCOL_FORMAT], bbstxt[B_ARJ][0], &bbstxt[B_ARJ][1]);
               m_print (bbstxt[B_PROTOCOL_FORMAT], bbstxt[B_LHA][0], &bbstxt[B_LHA][1]);
               m_print (bbstxt[B_PROTOCOL_FORMAT], bbstxt[B_HELP][0], &bbstxt[B_HELP][1]);
               m_print (bbstxt[B_ASK_COMPRESSOR]);
            }

            if (usr.hotkey) {
               cmd_input (stringa, 1);
               m_print (bbstxt[B_ONE_CR]);
            }
            else
               input (stringa, 1);
            c = stringa[0];
            if (c == '\0' || c == '\r' || !CARRIER)
               return (0);
         }
         else
            c = usr.archiver;
      }

      c = toupper(c);
      if (c == bbstxt[B_ZIP][0])
         return (1);
      else if (c == bbstxt[B_ARJ][0])
         return (2);
      else if (c == bbstxt[B_LHA][0])
         return (3);
      else if (c == bbstxt[B_HELP][0])
         read_system_file ("COMPHELP");
      else {
         if (c != toupper (usr.archiver))
            return (0);
         usr.archiver = '\0';
      }
   }
}
*/

void pack_tagged_areas ()
{
   int i, x, swapout = 0, *varr;
   char file[80], stringa[180], c, path[80];
   struct ffblk fbuf;

   qsort (&usr.lastread, MAXLREAD, sizeof (struct _lastread), sort_func);
   getcwd (stringa, 79);

   sprintf (file, "%sTASK%02X", config->QWKDir, line_offset);
   mkdir (file);

   if (file[1] == ':')
      setdisk (toupper (file[0]) - 'A');
   if (chdir (file) == -1)
      return;

   set_useron_record (7, 0, 0);

   if (!findfirst ("*.*", &fbuf, 0)) {
      unlink (fbuf.ff_name);
      while (!findnext (&fbuf))
         unlink (fbuf.ff_name);
   }

   setdisk (toupper (stringa[0]) - 'A');
   chdir (stringa);
   m_print (bbstxt[B_ONE_CR]);

   if (!prescan_tagged_areas (0, NULL))
      return;

   if ((c = selpacker ()) == '\0')
      return;

   m_print2 (bbstxt[B_PLEASE_WAIT]);

   for (i = 0; i < 10; i++) {
      if (toupper (config->packid[i].display[0]) == c)
         break;
   }

   if (i < 10) {
      sprintf (file, "%sTASK%02X\\%s", config->QWKDir, line_offset, config->BBSid);
      sprintf (path, "%sTASK%02X\\*.*", config->QWKDir, line_offset);
      if (config->packers[i].packcmd[0] == '+') {
         strcpy (stringa, &config->packers[i].packcmd[1]);
         swapout = 1;
      }
      else
         strcpy (stringa, config->packers[i].packcmd);

      strsrep (stringa, "%1", file);
      strsrep (stringa, "%2", path);

      status_line ("#%sing ASCII mail packet", config->packers[i].id);

      varr = ssave ();
      gotoxy_ (1, 0);
      wtextattr (LGREY|_BLACK);
      cclrscrn (LGREY|_BLACK);
      showcur();

      x = spawn_program (swapout, stringa);

      if (varr != NULL)
         srestore (varr);
      if (local_mode || snooping)
         showcur();
      else
         hidecur ();

      if (x != 0)
         status_line (":Return code: %d", x);
   }

   m_print(bbstxt[B_ONE_CR]);

   cps = 0;
   c = sys.freearea;
   sys.freearea = 1;
   set_useron_record (7, 0, 0);

   strcat (file, ".*");
   download_file (file, 0);

   sys.freearea = c;

   if (cps)
      unlink (file);
   if (cps || local_mode)
      update_lastread_pointers ();
}

void resume_transmission ()
{
   char file[80], c;
   struct ffblk fbuf;

   sprintf (file, "%sTASK%02X\\%s.*", config->QWKDir, line_offset, config->BBSid);

   if (!findfirst (file, &fbuf, 0))
      do {
         if (strchr (fbuf.ff_name, '.') != NULL) {
            sprintf (file, "%sTASK%02X\\%s", config->QWKDir, line_offset, fbuf.ff_name);
            cps = 0;
            c = sys.freearea;
            sys.freearea = 1;
            set_useron_record (7, 0, 0);
            download_file (file, 0);
            sys.freearea = c;
            if (cps)
               unlink (file);
            break;
         }
      } while (!findnext (&fbuf));
}

void qwk_pack_tagged_areas ()
{
   FILE *fpq;
   int i, fd, totals, x, swapout = 0, *varr;
   char file[80], stringa[180], c, path[80];
   time_t aclock;
   struct _sys tsys;
   struct ffblk fbuf;
   struct tm *newtime;

   qsort (&usr.lastread, MAXLREAD, sizeof (struct _lastread), sort_func);
   getcwd (stringa, 79);

   sprintf (file, "%sTASK%02X", config->QWKDir, line_offset);
   mkdir (file);

   if (file[1] == ':')
      setdisk (toupper (file[0]) - 'A');
   if (chdir (file) == -1)
      return;

   set_useron_record (7, 0, 0);

   if (!findfirst ("*.*", &fbuf, 0)) {
      unlink (fbuf.ff_name);
      while (!findnext (&fbuf))
         unlink (fbuf.ff_name);
   }

   setdisk (toupper (stringa[0]) - 'A');
   chdir (stringa);
   m_print (bbstxt[B_ONE_CR]);

   memcpy ((char *)&tsys, (char *)&sys, sizeof (struct _sys));

   sprintf (file, "%sTASK%02X\\CONTROL.DAT", config->QWKDir, line_offset);
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
   sprintf(stringa,"00000,%s\n",config->BBSid);         /* Line #5 */
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
   sprintf (stringa, SYSMSG_PATH, config->sys_path);
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
      if (tsys.qwk_name[0])
         strcpy (stringa, tsys.qwk_name);
      else
         strncpy (stringa, tsys.msg_name, 13);
      stringa[13] = '\0';
      fputs (stringa, fpq);
      fputs ("\n", fpq);
   }

   fputs ("WELCOME\n", fpq);          /* This is QWK welcome file */
   fputs ("NEWS\n", fpq);          /* This is QWK news file */
   fputs ("GOODBYE\n", fpq);          /* This is QWK bye file */

   close (fd);
   fclose (fpq);

   sprintf (file, "%sTASK%02X\\DOOR.ID", config->QWKDir, line_offset);
   fpq = fopen(file, "wt");
   if (fpq == NULL)
      return;

   sprintf(stringa,"DOOR = LoraBBS\n");
   fputs(stringa,fpq);           /* Line #1 */

   sprintf(stringa,"VERSION = %s\n", VNUM);
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

   sprintf (file, "%sTASK%02X\\MESSAGES.DAT", config->QWKDir, line_offset);
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

   if ((c = selpacker ()) == '\0')
      return;

   m_print (bbstxt[B_PLEASE_WAIT]);

   for (i = 0; i < 10; i++) {
      if (toupper (config->packid[i].display[0]) == c)
         break;
   }

   if (i < 10) {
      sprintf (file, "%sTASK%02X\\%s.QWK", config->QWKDir, line_offset, config->BBSid);
      sprintf (path, "%sTASK%02X\\*.*", config->QWKDir, line_offset);
      if (config->packers[i].packcmd[0] == '+') {
         strcpy (stringa, &config->packers[i].packcmd[1]);
         swapout = 1;
      }
      else
         strcpy (stringa, config->packers[i].packcmd);

      strsrep (stringa, "%1", file);
      strsrep (stringa, "%2", path);

      status_line ("#%sing QWK mail packet", config->packers[i].id);

      varr = ssave ();
      gotoxy_ (1, 0);
      wtextattr (LGREY|_BLACK);
      cclrscrn (LGREY|_BLACK);
      showcur();

      x = spawn_program (swapout, stringa);

      if (varr != NULL)
         srestore (varr);
      if (local_mode || snooping)
         showcur();
      else
         hidecur ();

      if (x != 0)
         status_line (":Return code: %d", x);
   }

   m_print(bbstxt[B_ONE_CR]);
   cps = 0;
   c = sys.freearea;
   sys.freearea = 1;
   set_useron_record (7, 0, 0);
   download_file (file, 0);
   sys.freearea = c;

   if (cps)
      unlink (file);
   if (cps || local_mode)
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


static void unpack_repfile (char *dir, char *rep)
{
   int id, f, m, i, swapout = 0, x, *varr;
   char filename[80], outstring[128], *p, r, cpath[80];
   unsigned char headstr[30];
   struct ffblk blk;

   sprintf (filename, "%s\\%s", dir, rep);
   if (findfirst (filename, &blk, 0))
      return;

   getcwd (cpath, 79);
   if (dir[1] == ':')
      setdisk (toupper (dir[0]) - 'A');
   chdir (dir);

   do {
      f = open (blk.ff_name, O_RDONLY|O_BINARY);
      id = -1;

      if (f != -1) {
         for (i = 0; i < 10; i++) {
            if (config->packid[i].offset >= 0)
               lseek (f, config->packid[i].offset, SEEK_SET);
            else
               lseek (f, config->packid[i].offset, SEEK_END);
            read (f, headstr, 14);

            if (!config->packers[i].id[0] || !config->packid[i].ident[0])
               continue;

            id = i;
            m = 0;
            p = (char *)&config->packid[i].ident[0];

            while (*p) {
               r = 0;
               if (isdigit (*p)) {
                  r *= 16L;
                  r += *p++ - '0';
               }
               else if (isxdigit (*p)) {
                  r *= 16L;
                  r += *p++ - 55;
               }
               if (isdigit (*p)) {
                  r *= 16L;
                  r += *p++ - '0';
               }
               else if (isxdigit (*p)) {
                  r *= 16L;
                  r += *p++ - 55;
               }
               if (r != headstr[m++]) {
                  id = -1;
                  break;
               }
            }

            if (id != -1)
               break;
         }
         close (f);
      }

      if (id == -1) {
         status_line ("!Unrecognized method for %s\\%s", dir, blk.ff_name);
         id = 0;
         unlink (blk.ff_name);
         status_line (":%s\\%s deleted", dir, blk.ff_name);
      }
      else {
         activation_key ();
         status_line ("#Un%sing %s\\%s", config->packers[id].id, dir, blk.ff_name);

         time_release ();
         if (config->packers[id].unpackcmd[0] == '+') {
            strcpy (outstring, &config->packers[id].unpackcmd[1]);
            swapout = 1;
         }
         else {
            strcpy (outstring, config->packers[id].unpackcmd);
            swapout = 0;
         }
         strsrep (outstring, "%1", blk.ff_name);
         strsrep (outstring, "%2", "*.MSG");

         varr = ssave ();
         gotoxy_ (1, 0);
         wtextattr (LGREY|_BLACK);

         x = spawn_program (swapout, outstring);

         if (varr != NULL)
            srestore (varr);

         if (x != 0)
            status_line (":Return code: %d", x);

         time_release ();
         unlink (blk.ff_name);
      }
   } while (!findnext (&blk));

   setdisk (toupper (cpath[0]) - 'A');
   chdir (cpath);
}

/* Fetch REP packet from user and import if ok */
void getrep(void)
{
   #define TRUE  1
   #define FALSE 0
   int x,y,z,msgfile;
   int endspace, upmsgs, repfile, prev_area, msgrecs;
   char  *msgbuff, *p, temp[80], temp1[80];
   long t;
   struct QWKmsghd *MsgHead;
   struct ffblk fbuf;

   if (!local_mode) {
      getcwd (temp, 79);

      sprintf (temp1, "%sTASK%02X", config->QWKDir, line_offset);
      mkdir (temp1);

      if (temp1[1] == ':')
         setdisk (toupper (temp1[0]) - 'A');
      if (chdir (temp1) == -1)
         return;

      if (!findfirst ("*.*", &fbuf, 0)) {
         unlink (fbuf.ff_name);
         while (!findnext (&fbuf))
            unlink (fbuf.ff_name);
      }

      setdisk (toupper (temp[0]) - 'A');
      chdir (temp);

      sprintf (temp, "%sTASK%02X\\%s.REP", config->QWKDir, line_offset, config->BBSid);

      no_description = no_check = 1;
      upload_file (temp, -1);
      no_description = no_check = 0;
   }

   sprintf (temp, "%sTASK%02X\\%s.RE?", config->QWKDir, line_offset, config->BBSid);
   if (findfirst (temp, &fbuf, 0))
      return;

   sprintf (temp, "%sTASK%02X", config->QWKDir, line_offset);
   unpack_repfile (temp, "*.RE?");

   upmsgs = 0;
   prev_area = 0;

   sprintf (temp, "%sTASK%02X\\%s.MSG", config->QWKDir, line_offset, config->BBSid);
   repfile = sh_open (temp, SH_DENYNONE, O_BINARY|O_RDONLY, S_IREAD|S_IWRITE);
   if (repfile != -1) {
      MsgHead = (struct QWKmsghd *) malloc (sizeof (struct QWKmsghd));
      if (MsgHead == NULL) {
         close (repfile);
         return;
      }
      msgbuff = (char *) malloc (150);
      if (msgbuff == NULL) {
         close (repfile);
         return;
      }
      read (repfile, msgbuff, 128);          /* Ignore first record */
      msgbuff[8] = 0;
      strtrim (msgbuff);
      if (stricmp (config->BBSid, msgbuff) == 0) {          /* Make sure it's ours! */
         m_print2 (bbstxt[B_HANG_ON]);
         x = read(repfile,(struct QWKmsghd *) MsgHead,128);
         while (x == 128) {               /* This is the BBS.MSG import loop */
         /* Build the Fidonet message head */
            x = atoi(MsgHead->Msgnum);               /* Area number */
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
            m_print2 (bbstxt[B_IMPORTING_MESSAGE], upmsgs);
            m_print2 ("Š%s\n", sys.msg_name);

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

            msg_fzone=config->alias[sys.use_alias].zone;
            msg.orig=config->alias[sys.use_alias].node;
            msg.orig_net=config->alias[sys.use_alias].net;
            msg.dest=config->alias[sys.use_alias].node;
            msg.dest_net=config->alias[sys.use_alias].net;

            if (MsgHead->Msgstat == '*') {
               if (!sys.public)
                  msg.attr |= MSGPRIVATE;
            }

            if (sys.private)
               msg.attr |= MSGPRIVATE;

            strcpy(msg.to,namefixup(MsgHead->Msgto));
            fancy_str (msg.to);
            m_print2 ("To: %s\n", msg.to);
            strcpy(msg.from,namefixup(MsgHead->Msgfrom));
            fancy_str (msg.from);
            strcpy(msg.subj,namefixup(MsgHead->Msgsubj));
            m_print2 ("Subject: %s\n", msg.subj);

            sprintf(temp1,"%s%d.TXT", config->QWKDir, line_offset);
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

            t = time (NULL);
            while (t == time (NULL));

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

      free (MsgHead);
      free (msgbuff);
      close (repfile);
   }

   sprintf (temp, "%sTASK%02X\\%s.MSG", config->QWKDir, line_offset, config->BBSid);
   unlink (temp);

   if (upmsgs) {
      read_system (usr.msg, 1);
      m_print (bbstxt[B_TOTAL_IMPORTED], upmsgs);

      press_enter ();
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
      sprintf (file, "%sTASK%02X\\PERSONAL.NDX", config->QWKDir, line_offset);
      fdp = cshopen(file,O_CREAT | O_TRUNC | O_BINARY | O_WRONLY,S_IREAD|S_IWRITE);
   }
   else
      fdp = -1;

   m_print (bbstxt[B_QWK_HEADER1]);
   m_print (bbstxt[B_QWK_HEADER2]);

   for (i=0; i < MAXLREAD && CARRIER; i++) {
      if (config->qwk_maxmsgs && msgcount >= config->qwk_maxmsgs)
         break;

      last[i] = usr.lastread[i].msg_num;

      if (!usr.lastread[i].area)
         continue;

      if (!read_system (usr.lastread[i].area, 1))
         continue;

      if (!qwk) {
         sprintf (file, "%sTASK%02X\\%03d.TXT", config->QWKDir, line_offset, usr.lastread[i].area);
         fpq = fopen(file, "w+t");
         fprintf (fpq, "[*] AREA: %s\n\n", sys.msg_name);
      }

      m_print2 (bbstxt[B_SCAN_SEARCHING], sys.msg_num, sys.msg_name);

      tt = pp = 0;

      if (qwk) {
         sprintf (file, "%sTASK%02X\\%03d.NDX", config->QWKDir, line_offset, usr.lastread[i].area);
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
            sprintf (file, "%sTASK%02X\\%03d.NDX", config->QWKDir, line_offset, usr.lastread[i].area);
         else
            sprintf (file, "%sTASK%02X\\%03d.TXT", config->QWKDir, line_offset, usr.lastread[i].area);
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
         sprintf (file, "%sTASK%02X\\PERSONAL.NDX", config->QWKDir, line_offset);
         unlink (file);
      }
   }

   if (config->qwk_maxmsgs && msgcount >= config->qwk_maxmsgs && i < MAXLREAD && CARRIER)
      m_print (bbstxt[B_QWK_LIMIT]);
   else if (!CARRIER)
      return (0);

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

void update_lastread_pointers ()
{
   int i;
   struct _sys tsys;

   m_print (bbstxt[B_QWK_UPDATE]);
#ifdef __OS2__
   UNBUFFER_BYTES ();
   VioUpdate ();
#endif

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

   if (start > last_msg)
      start = last_msg;

   if (start == last_msg) {
      *personal = pp;
      *total = tt;

      return (totals);
   }

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

