#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <dir.h>
#include <alloc.h>
#include <sys\stat.h>

#include "lora.h"
#include "quickmsg.h"
#include "msgapi.h"

void Create_Index (int, int, int);
void Purge_Message (int, int);
void Purge_Fido_Areas (int);
void Purge_Quick_Areas (int);
int Scan_Fido_Area (char *, int *, int *);
void Kill_by_Age (int, int, int, int);
void Fido_Renum (char *, int, int, int);
void Renum_Fido_Areas (void);
void Renum_Quick_Areas (void);
void Pack_Quick_Base (int);
void Append_Backslash (char *);
void Get_Config_Info (void);
void Purge_Squish_Areas (int);
void Squish_Kill_by_Age (MSG *, int, int, int, int);
void Squish_Renum (MSG *, int, int, int);
void Renum_Squish_Areas (void);

char *fido_msgpath, *user_file, *sys_path;

extern unsigned int _stklen = 10240;

void main (argc, argv)
int argc;
char *argv[];
{
   int i, xlink, unknow, renum, pack, purge, overwrite;
   int link, stats, index;

   printf("\nLMSG; LoraBBS 2.10 Message maintenance utility\n");
   printf("      Copyright (c) 1991-92 by Marco Maccaferri, All Rights Reserved\n\n");

   xlink = unknow = renum = pack = purge = overwrite = 0;
   link = stats = index = 0;

   for (i = 1; i < argc; i++) {
      if (!strnicmp (argv[i], "-I", 2)) {
         index = 1;
         if (strchr (argv[i], 'C') != NULL || strchr (argv[i], 'c') != NULL)
            xlink = 1;
         if (strchr (argv[i], 'U') != NULL || strchr (argv[i], 'u') != NULL)
            unknow = 1;
         if (strchr (argv[i], 'R') != NULL || strchr (argv[i], 'r') != NULL)
            renum = 1;
      }
      if (!strnicmp (argv[i], "-P", 2)) {
         pack = 1;
         if (strchr (argv[i], 'K') != NULL || strchr (argv[i], 'k') != NULL)
            purge = 1;
         if (strchr (argv[i], 'O') != NULL || strchr (argv[i], 'o') != NULL)
            overwrite = 1;
         if (strchr (argv[i], 'R') != NULL || strchr (argv[i], 'r') != NULL)
            renum = 1;
         if (strchr (argv[i], 'A') != NULL || strchr (argv[i], 'a') != NULL)
            overwrite = 2;
      }
      if (!stricmp (argv[i], "-K"))
         purge = 1;
      if (!stricmp (argv[i], "-L"))
         link = 1;
      if (!stricmp (argv[i], "-R"))
         renum = 1;
      if (!stricmp (argv[i], "-S"))
         stats = 1;
   }

   if ( (!xlink && !unknow && !renum && !pack && !purge && !overwrite &&
         !link && !stats && !index) || argc == 1 )
   {
      printf(" * Command-line parameters:\n\n");

      printf("        -I      Recreate index files & check\n");
      printf("                U=Kill unknown boards  R=Renumber\n");
//      printf("                C=Kill crosslinked messages  U=Kill unknown boards  R=Renumber\n");
      printf("        -P      Pack (compress) message base\n");
      printf("                K=Purge  R=Renumber\n");
//      printf("                K=Purge  R=Renumber  O=Overwrite  A=Overwrite only if necessary\n");
      printf("        -K      Purge messages from info in SYSMSG.DAT\n");
//      printf("        -L      Link reply chains\n");
      printf("        -R      Renumber messages\n\n");

      printf(" * Please refer to the documentation for a more complete command summary\n\n");
   }
   else
   {
      Get_Config_Info ();

      if (purge) {
         Purge_Fido_Areas (renum);
         Purge_Quick_Areas (renum);
         Purge_Squish_Areas (renum);
      }
      if (pack)
         Pack_Quick_Base (renum);
      if (index)
         Create_Index (xlink, unknow, renum);
      if (renum && !purge && !index) {
         Renum_Fido_Areas ();
         Renum_Quick_Areas ();
         Renum_Squish_Areas ();
      }

      printf(" * Done.\n\n");
   }
}

void Create_Index (xlink, unknow, renum)
int xlink, unknow, renum;
{
   int fd, fdidx, fdto, know[200], m, flag;
   char filename[60];
   word msgnum;
   long pos;
   struct _sys tsys;
   struct _msghdr msghdr;
   struct _msgidx msgidx;
   struct _msginfo msginfo;

   msgnum = 1;
   if (xlink);

   if (unknow) {
      for (m = 0; m < 200; m++)
         know[m] = 0;

      sprintf (filename, "%sSYSMSG.DAT", sys_path);
      fd = open(filename, O_RDONLY|O_BINARY);
      if (fd == -1)
         exit (1);

      while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
         if ( !tsys.quick_board )
            continue;

         know[tsys.quick_board-1] = 1;
      }

      close (fd);
   }

   fd = open ("MSGHDR.BBS", O_RDWR|O_BINARY);
   if (fd == -1)
      exit (1);

   printf(" * Regenerating message base indices\n");

   pos = tell (fd);

   memset((char *)&msginfo, 0, sizeof(struct _msginfo));

   sprintf (filename, "%sMSGIDX.BBS", fido_msgpath);
   fdidx = open(filename,O_WRONLY|O_BINARY|O_TRUNC);

   sprintf (filename, "%sMSGTOIDX.BBS", fido_msgpath);
   fdto = open(filename,O_WRONLY|O_BINARY|O_TRUNC);

   while (read(fd, (char *)&msghdr, sizeof (struct _msghdr)) == sizeof (struct _msghdr)) {
      flag = 0;

      if (unknow) {
         if (know[msghdr.board-1] != 1) {
            msghdr.msgattr |= Q_RECKILL;
            if (know[msghdr.board-1] != 2) {
               printf(" * Unknow board #%d\n", msghdr.board);
               know[msghdr.board-1] = 2;
            }
         flag = 1;
         }
      }

      if (renum)
         msghdr.msgnum = msgnum++;

      if (renum || flag) {
         lseek (fd, pos, SEEK_SET);
         write (fd, (char *)&msghdr, sizeof (struct _msghdr));
         pos = tell (fd);
      }

      if (flag) {
         msgidx.msgnum = 0;
         write (fdto, "\x0B* Deleted *                        ", 36);
      }
      else {
         write (fdto, (char *)&msghdr.whoto, sizeof (struct _msgtoidx));
         msgidx.msgnum = msghdr.msgnum;
      }

      msgidx.board = msghdr.board;
      write (fdidx, (char *)&msgidx, sizeof (struct _msgidx));

      msginfo.totalmsgs++;

      if (!msginfo.lowmsg)
         msginfo.lowmsg = msghdr.msgnum;

      if (msginfo.lowmsg > msghdr.msgnum)
         msginfo.lowmsg = msghdr.msgnum;
      if (msginfo.highmsg < msghdr.msgnum)
         msginfo.highmsg = msghdr.msgnum;

      if (!flag)
         msginfo.totalonboard[msgidx.board-1]++;
   }

   close (fdto);
   close (fdidx);
   close (fd);

   sprintf (filename, "%sMSGINFO.BBS", fido_msgpath);
   fd = open(filename,O_WRONLY|O_BINARY);
   write(fd, (char *)&msginfo, sizeof(struct _msginfo));
   close (fd);
}

void Renum_Quick_Areas ()
{
   int fd, fdidx;
   char filename[60];
   word msgnum;
   long pos;
   struct _msghdr msghdr;
   struct _msgidx msgidx;
   struct _msginfo msginfo;

   msgnum = 1;

   sprintf (filename, "%sMSGHDR.BBS", fido_msgpath);
   fd = open (filename, O_RDWR | O_BINARY);
   if (fd == -1)
      exit (1);

   pos = tell (fd);

   memset((char *)&msginfo, 0, sizeof(struct _msginfo));

   sprintf (filename, "%sMSGIDX.BBS", fido_msgpath);
   fdidx = open(filename,O_WRONLY|O_BINARY|O_TRUNC);

   while (read(fd, (char *)&msghdr, sizeof (struct _msghdr)) == sizeof (struct _msghdr)) {
      if (!(msghdr.msgattr & Q_RECKILL)) {
         msghdr.msgnum = msgnum++;
         lseek (fd, pos, SEEK_SET);
         write (fd, (char *)&msghdr, sizeof (struct _msghdr));
      }
      pos = tell (fd);

      if (!(msghdr.msgattr & Q_RECKILL))
         msgidx.msgnum = msghdr.msgnum;
      else
         msgidx.msgnum = 0;
      msgidx.board = msghdr.board;

      write (fdidx, (char *)&msgidx, sizeof (struct _msgidx));

      if (!(msghdr.msgattr & Q_RECKILL))
      {
         if (!msginfo.lowmsg)
            msginfo.lowmsg = msghdr.msgnum;

         if (msginfo.lowmsg > msghdr.msgnum)
            msginfo.lowmsg = msghdr.msgnum;
         if (msginfo.highmsg < msghdr.msgnum)
            msginfo.highmsg = msghdr.msgnum;

         msginfo.totalmsgs++;
         msginfo.totalonboard[msgidx.board-1]++;
      }
   }

   close (fdidx);
   close (fd);

   sprintf (filename, "%sMSGINFO.BBS", fido_msgpath);
   fd = open(filename,O_WRONLY|O_BINARY);
   write(fd, (char *)&msginfo, sizeof(struct _msginfo));
   close (fd);
}

void Purge_Quick_Areas (renum)
{
   int fd, fdidx, max_msgs[200], max_age[200], age_rcvd[200], month[12];
   int cnum[200], pnum[200], area, flag1, x;
   int far *qren[200];
   char filename[60];
   int fdto, dy, mo, yr, days, m, flag;
   word msgnum;
   long pos, tempo, day_now, day_mess;
   struct tm *tim;
   struct _msghdr msghdr;
   struct _msgidx msgidx;
   struct _msginfo msginfo;
   struct _sys tsys;
   struct _usr usr;

   month[0] = 31;
   month[1] = 28;
   month[2] = 31;
   month[3] = 30;
   month[4] = 31;
   month[5] = 30;
   month[6] = 31;
   month[7] = 31;
   month[8] = 30;
   month[9] = 31;
   month[10] = 30;
   month[11] = 31;

   printf(" * Purging QuickBBS messages\n");

   for (m = 0; m < 200; m++) {
      max_msgs[m] = 0;
      max_age[m] = 0;
      age_rcvd[m] = 0;
      cnum[m] = 0;
      pnum[m] = 0;
   }

   msgnum = 0;
   flag = 0;

   sprintf (filename, "%sSYSMSG.DAT", sys_path);
   fd = open(filename, O_RDONLY|O_BINARY);
   if (fd == -1)
      exit (1);

   while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
      if ( !tsys.quick_board )
         continue;

      max_msgs[tsys.quick_board-1] = tsys.max_msgs;
      max_age[tsys.quick_board-1] = tsys.max_age;
      age_rcvd[tsys.quick_board-1] = tsys.age_rcvd;

      if (tsys.max_msgs || tsys.max_age || tsys.age_rcvd)
         flag = 1;
   }

   close (fd);

   if (!flag)
      return;

   sprintf (filename, "%sMSGINFO.BBS", fido_msgpath);
   fd = open(filename,O_RDWR|O_BINARY);
   if (fd == -1)
      exit (1);
   read (fd, (char *)&msginfo, sizeof(struct _msginfo));
   close (fd);

   for (m = 0; m < 200; m++) {
      if (msginfo.totalonboard[m]) {
         qren[m] = (int far *)farmalloc (msginfo.totalonboard[m] * (long)sizeof (int));
         if (qren[m] == NULL)
            exit (2);
      }
      else
         qren[m] = NULL;
   }

   tempo = time(0);
   tim = localtime(&tempo);

   day_now = 365 * tim->tm_year;
   for (m = 0; m < tim->tm_mon; m++)
      day_now += month[m];
   day_now += tim->tm_mday;

   sprintf (filename, "%sMSGHDR.BBS", fido_msgpath);
   fd = open (filename, O_RDWR|O_BINARY);
   if (fd == -1)
      exit (1);

   pos = tell (fd);

   sprintf (filename, "%sMSGIDX.BBS", fido_msgpath);
   fdidx = open(filename,O_RDWR|O_BINARY);

   sprintf (filename, "%sMSGTOIDX.BBS", fido_msgpath);
   fdto = open(filename,O_RDWR|O_BINARY);

   while (read(fd, (char *)&msghdr, sizeof (struct _msghdr)) == sizeof (struct _msghdr)) {
      flag = 0;

      pnum[msghdr.board-1]++;

      if (max_msgs[msghdr.board-1] && msginfo.totalonboard[msghdr.board-1] > max_msgs[msghdr.board-1]) {
         msghdr.msgattr |= Q_RECKILL;
         flag = 1;
      }
      else {
         sscanf(&msghdr.date[1], "%02d-%02d-%02d", &mo, &dy, &yr);

         day_mess = 365 * yr;
         mo--;
         for (m = 0; m < mo; m++)
            day_mess += month[m];
         day_mess += dy;

         days = (int)(day_now - day_mess);

         if (max_age[msghdr.board-1] && days > max_age[msghdr.board-1]) {
            msghdr.msgattr |= Q_RECKILL;
            flag = 1;
         }
         else if (age_rcvd[msghdr.board-1] &&
          ( (msghdr.msgattr & Q_PRIVATE) && (msghdr.msgattr & Q_RECEIVED) ) &&
          days > age_rcvd[msghdr.board-1]) {
            msghdr.msgattr |= Q_RECKILL;
            flag = 1;
         }
      }

      if ( flag ) {
         lseek (fd, pos, SEEK_SET);
         write (fd, (char *)&msghdr, sizeof (struct _msghdr));
         msginfo.totalonboard[msghdr.board-1]--;
         lseek (fdidx, (long)msgnum * sizeof (struct _msgidx), SEEK_SET);
         read (fdidx, (char *)&msgidx, sizeof (struct _msgidx));
         msgidx.msgnum = 0;
         lseek (fdidx, (long)msgnum * sizeof (struct _msgidx), SEEK_SET);
         write (fdidx, (char *)&msgidx, sizeof (struct _msgidx));

         lseek (fdto, (long)msgnum * 36, SEEK_SET);
         write (fdto, "\x0B* Deleted *                        ", 36);
      }
      else
         qren[msghdr.board-1][cnum[msghdr.board-1]++] = pnum[msghdr.board-1];

      if (renum) {
         msgidx.msgnum = msghdr.msgnum;
         msgidx.board = msghdr.board;

         write (fdidx, (char *)&msgidx, sizeof (struct _msgidx));
      }

      pos = tell (fd);
      msgnum++;
   }

   close (fdidx);
   close (fd);
   close (fdto);

   sprintf (filename, "%sMSGINFO.BBS", fido_msgpath);
   fd = open(filename,O_RDWR|O_BINARY);
   if (fd == -1)
      exit (1);
   write (fd, (char *)&msginfo, sizeof(struct _msginfo));
   close (fd);

   for (m = 0; m < 200; m++)
      cnum[m] = 0;

   printf(" * Updating USERS lastread pointer\n");

   sprintf (filename, "%sSYSMSG.DAT", sys_path);
   fd = open(filename, O_RDONLY|O_BINARY);
   if (fd == -1)
      exit (1);

   while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
      if ( !tsys.quick_board )
         continue;
      cnum[tsys.quick_board-1] = tsys.msg_num;
   }

   close (fd);

   sprintf (filename, "%s.BBS", user_file);
   fd = open(filename, O_RDWR | O_BINARY);
   pos = tell (fd);

   while (read(fd, (char *) &usr, sizeof(struct _usr)) == sizeof(struct _usr)) {
      for (area = 0; area < MAXLREAD; area++) {
         for (m = 0; m < 200; m++) {
            if (usr.lastread[area].area == cnum[m])
               break;
         }

         if (m < 200 && usr.lastread[area].msg_num) {
            flag1 = 0;

            for (x = 0; x < msginfo.totalonboard[m]; x++)
               if (qren[m][x] > flag1 && qren[m][x] <= usr.lastread[area].msg_num)
                  flag1 = x + 1;

            usr.lastread[area].msg_num = flag1;
         }
      }

      for (area = 0; area < MAXDLREAD; area++) {
         for (m = 0; m < 200; m++) {
            if (usr.dynlastread[area].area == cnum[m])
               break;
         }

         if (m < 200 && usr.dynlastread[area].msg_num) {
            flag1 = 0;

            for (x = 0; x < msginfo.totalonboard[m]; x++)
               if (qren[m][x] > flag1 && qren[m][x] <= usr.dynlastread[area].msg_num)
                  flag1 = x + 1;

            usr.dynlastread[area].msg_num = flag1;
         }
      }

      lseek (fd, pos, SEEK_SET);
      write (fd, (char *) &usr, sizeof(struct _usr));

      pos = tell (fd);
   }

   close(fd);

   for (m = 0; m < 200; m ++) {
      if (qren[m] != NULL)
         farfree (qren[m]);
   }
}

void Purge_Fido_Areas (renum)
int renum;
{
   int fd, first, last, i, msgnum, kill;
   char filename [80], curdir [80];
   struct _sys tsys;

   getcwd (curdir, 79);

   printf(" * Purging Fido *.MSG messages\n");

   sprintf (filename, "%sSYSMSG.DAT", sys_path);
   fd = open(filename, O_RDONLY|O_BINARY);
   if (fd == -1)
      exit (1);

   msgnum = first = last = 0;

   while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
      if ( tsys.quick_board || tsys.pip_board )
         continue;

      setdisk (toupper (tsys.msg_path[0]) - 'A');
      tsys.msg_path[strlen(tsys.msg_path)-1] = '\0';
      chdir(tsys.msg_path);
      tsys.msg_path[strlen(tsys.msg_path)] = '\\';

      msgnum = Scan_Fido_Area (tsys.msg_path, &first, &last);

      if (first == 1 && tsys.echomail)
         first++;

      if ( tsys.max_msgs && tsys.max_msgs < msgnum ) {
         kill = msgnum - tsys.max_msgs;

         for (i = first; i <= last; i++) {
            sprintf (filename, "%d.MSG", i);
            unlink (filename);

            if (--kill <= 0)
               break;
         }
      }

      if ( tsys.max_age || tsys.age_rcvd )
         Kill_by_Age (tsys.max_age, tsys.age_rcvd, first, last);

      if (renum)
         Fido_Renum (tsys.msg_path, first, last, tsys.msg_num);
   }

   close (fd);

   setdisk (toupper (curdir[0]) - 'A');
   chdir(curdir);
}

void Renum_Fido_Areas ()
{
   int fd, first, last;
   char filename[60];
   struct _sys tsys;

   sprintf (filename, "%sSYSMSG.DAT", sys_path);
   fd = open(filename, O_RDONLY|O_BINARY);
   if (fd == -1)
      exit (1);

   first = last = 0;

   while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
      if ( tsys.quick_board || tsys.pip_board )
         continue;

      Scan_Fido_Area (tsys.msg_path, &first, &last);
      Fido_Renum (tsys.msg_path, first, last, tsys.msg_num);
   }

   close(fd);
}

void Fido_Renum (msg_path, first, last, area_n)
char *msg_path;
int first, last, area_n;
{
   unsigned long p;
   int fdm, fd, m, i, x, msg[1000], flag1, flag2, flag, area;
   long pos;
   char filename [60], newname [40], curdir[80];
   struct _msg mesg;
   struct _usr usr;

   getcwd (curdir, 79);

   setdisk (toupper (msg_path[0]) - 'A');
   msg_path[strlen(msg_path)-1] = '\0';
   chdir(msg_path);
   msg_path[strlen(msg_path)] = '\\';

   m = flag = 0;

   for (i = first; i <= last; i++)
   {
      sprintf(filename, "%d.MSG", i);
      fdm = open(filename, O_RDWR | O_BINARY);
      if (fdm != -1)
      {
         close(fdm);

         msg[m++] = i;

         if (i != m)
         {
            sprintf(newname, "%d.MSG", m);
            flag = 1;
            rename(filename, newname);
         }
      }
   }

   if (!flag)
   {
      setdisk (toupper (curdir[0]) - 'A');
      chdir(curdir);
      return;
   }

   for (i = 1; i <= m; i++)
   {
      sprintf(filename, "%d.MSG", i);
      fdm = open(filename, O_RDWR | O_BINARY);
      read(fdm, (char *) &mesg, sizeof(struct _msg));
      flag1 = flag2 = 0;
      for (x = 0; x < m; x++)
      {
         if (!flag1 && mesg.up == msg[x])
         {
            mesg.up = x + 1;
            flag1 = 1;
         }
         if (!flag2 && mesg.reply == msg[x])
         {
            mesg.reply = x + 1;
            flag2 = 1;
         }
         if (flag1 && flag2)
            break;
      }
      if (!flag1)
         mesg.up = 0;
      if (!flag2)
         mesg.reply = 0;

      lseek(fdm, 0L, 0);
      write(fdm, (char *) &mesg, sizeof(struct _msg));
      close(fdm);
   }

   fd = open ("LASTREAD", O_RDWR | O_BINARY);
   if (fd != -1)
   {
      while (read(fd, (char *) &i, 2) == 2)
      {
         p = tell(fd) - 2L;
         flag1 = 0;
         for (x = 0; x < m; x++)
            if (msg[x] > flag1 && msg[x] <= i)
               flag1 = x + 1;
         i = flag1;
         lseek(fd, p, 0);
         write(fd, (char *) &i, 2);
      }
      close(fd);
   }

   close(fd);

   sprintf (filename, "%s.BBS", user_file);
   fd = open(filename, O_RDWR | O_BINARY);
   pos = tell (fd);

   while (read(fd, (char *) &usr, sizeof(struct _usr)) == sizeof(struct _usr))
   {
      for (area=0; area<MAXLREAD; area++)
         if (usr.lastread[i].area == area_n)
            break;

      if (area < MAXLREAD)
      {
         flag1 = 0;

         for (x = 0; x < m; x++)
            if (msg[x] > flag1 && msg[x] <= usr.lastread[area].msg_num)
               flag1 = x + 1;

         usr.lastread[area].msg_num = flag1;

         lseek(fd, pos, 0);
         write(fd, (char *) &usr, sizeof(struct _usr));
      }
      else {
         for (area=0; area<MAXDLREAD; area++)
            if (usr.dynlastread[i].area == area_n)
               break;

         if (area < MAXDLREAD)
         {
            flag1 = 0;

            for (x = 0; x < m; x++)
               if (msg[x] > flag1 && msg[x] <= usr.dynlastread[area].msg_num)
                  flag1 = x + 1;

            usr.dynlastread[area].msg_num = flag1;

            lseek(fd, pos, 0);
            write(fd, (char *) &usr, sizeof(struct _usr));
         }
      }

      pos = tell (fd);
   }

   close(fd);

   setdisk (toupper (curdir[0]) - 'A');
   chdir(curdir);
}

void Kill_by_Age (max_age, age_rcvd, first, last)
int max_age, age_rcvd, first, last;
{
   int fd, yr, mo, dy, i, month[12], m, days;
   char mese[4], filename [40];
   long day_mess, day_now;
   struct tm *tim;
   struct _msg msg;
   long tempo;

   char *mon[] = {
      "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
      "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
   };

   month[0] = 31;
   month[1] = 28;
   month[2] = 31;
   month[3] = 30;
   month[4] = 31;
   month[5] = 30;
   month[6] = 31;
   month[7] = 31;
   month[8] = 30;
   month[9] = 31;
   month[10] = 30;
   month[11] = 31;

   tempo = time(0);
   tim = localtime(&tempo);

   day_now = 365 * tim->tm_year;
   for (m = 0; m < tim->tm_mon; m++)
      day_now += month[m];
   day_now += tim->tm_mday;

   for (i = first; i <= last; i++)
   {
      sprintf(filename, "%d.MSG", i);
      fd = open(filename, O_RDONLY | O_BINARY);
      if (fd == -1)
         continue;
      read(fd, (char *) &msg, sizeof(struct _msg));
      close(fd);

      sscanf(msg.date, "%2d %3s %2d", &dy, mese, &yr);
      mese[3] = '\0';
      for (mo = 0; mo < 12; mo++)
         if (!stricmp(mese, mon[mo]))
            break;
      if (mo == 12)
         mo = 0;

      day_mess = 365 * yr;
      for (m = 0; m < mo; m++)
         day_mess += month[m];
      day_mess += dy;

      days = (int)(day_now - day_mess);

      if ( days > max_age && max_age != 0 )
         unlink(filename);

      if ( ((msg.attr & MSGREAD) && (msg.attr & MSGPRIVATE)) &&
           days > age_rcvd && age_rcvd != 0)
         unlink(filename);
   }
}

int Scan_Fido_Area (path, first_m, last_m)
char *path;
int *first_m, *last_m;
{
   int i, first, last, msgnum;
   char filename[80], *p;
   struct ffblk blk;

   first = 0;
   last = 0;
   msgnum = 0;

   sprintf(filename, "%s*.MSG", path);

   if (!findfirst(filename, &blk, 0))
      do {
         if ((p = strchr(blk.ff_name,'.')) != NULL)
            *p = '\0';

         i = atoi(blk.ff_name);
         if (last < i || !last)
            last = i;
         if (first > i || !first)
            first = i;
         msgnum++;
      } while (!findnext(&blk));

   *first_m = first;
   *last_m = last;

   return (msgnum);
}

#define SET(b, p)   (b |= (0x01 << p))
#define TEST(b, p)  (b & (0x01 << p))

void Pack_Quick_Base (renum)
int renum;
{
   int fd, fdidx, fdto, dim, cnum[201], pnum[201], area;
   int far *qren[201];
   char filename[60];
   byte *blocks, buff[256];
   word count, msgnum;
   long rpos, wpos;
   struct _usr usr;
   struct _sys tsys;
   struct _msghdr msghdr;
   struct _msgidx msgidx;
   struct _msginfo msginfo;

   sprintf (filename, "%sMSGINFO.BBS", fido_msgpath);
   fd = open(filename,O_RDONLY|O_BINARY);
   read(fd, (char *)&msginfo, sizeof(struct _msginfo));
   close (fd);

   for (dim=1;dim<=200;dim++)
   {
      cnum[dim] = 0;
      pnum[dim] = 0;

      if (msginfo.totalonboard[dim])
      {
         qren[dim] = (int far *)farmalloc (msginfo.totalonboard[dim] * (long)sizeof (int));
         if (qren[dim] == NULL)
            exit (2);
      }
      else
         qren[dim] = NULL;
   }

   sprintf (filename, "%sMSGTXT.BBS", fido_msgpath);
   fd = open(filename,O_RDONLY|O_BINARY);
   if (fd == -1)
      exit (1);
   dim = (int)(filelength (fd) / 8) + 1;
   close (fd);

   blocks = (byte *)malloc (dim);
   if (blocks == NULL)
      exit (1);
   memset ((char *)blocks, 0, dim);

   sprintf (filename, "%sMSGHDR.BBS", fido_msgpath);
   fd = open (filename, O_RDONLY|O_BINARY);
   if (fd == -1)
      exit (1);

   while (read(fd, (char *)&msghdr, sizeof (struct _msghdr)) == sizeof (struct _msghdr))
   {
      if (msghdr.msgattr & Q_RECKILL)
         continue;

      for (; msghdr.numblocks > 0; msghdr.numblocks--, msghdr.startblock++)
         SET (blocks[msghdr.startblock/8], msghdr.startblock % 8);
   }

   close (fd);

   printf(" * Packing MSGTXT.BBS don't interrupt!\n");

   sprintf (filename, "%sMSGTXT.BBS", fido_msgpath);
   fd = open(filename,O_RDWR|O_BINARY);
   if (fd == -1)
      exit (1);

   count = 0;
   rpos = tell (fd);
   wpos = tell (fd);

   while (read(fd, buff, 256) == 256)
   {
      rpos = tell (fd);

      if (TEST (blocks[count/8], count % 8))
      {
         lseek (fd, wpos, SEEK_SET);
         write (fd, buff, 256);
         wpos = tell (fd);
         lseek (fd, rpos, SEEK_SET);
      }

      count++;
   }

   chsize (fd, wpos);
   close (fd);

   printf(" * Packing MSGHDR.BBS and create index files\n");

   sprintf (filename, "%sMSGIDX.BBS", fido_msgpath);
   fdidx = open(filename,O_RDWR|O_BINARY|O_TRUNC);

   sprintf (filename, "%sMSGTOIDX.BBS", fido_msgpath);
   fdto = open(filename,O_RDWR|O_BINARY|O_TRUNC);

   sprintf (filename, "%sMSGHDR.BBS", fido_msgpath);
   fd = open(filename,O_RDWR|O_BINARY);
   if (fd == -1)
      exit (1);

   msgnum = 1;
   count = 0;
   rpos = tell (fd);
   wpos = tell (fd);
   memset ((char *)&msginfo, 0, sizeof (struct _msginfo));

   while (read(fd, (char *)&msghdr, sizeof (struct _msghdr)) == sizeof (struct _msghdr))
   {
      rpos = tell (fd);

      cnum[msghdr.board]++;
      qren[msghdr.board][cnum[msghdr.board]] = pnum[msghdr.board];

      if (!(msghdr.msgattr & Q_RECKILL))
      {
         pnum[msghdr.board]++;
         qren[msghdr.board][cnum[msghdr.board]] = pnum[msghdr.board];

         msghdr.startblock = count;
         if (renum)
            msghdr.msgnum = msgnum++;
         lseek (fd, wpos, SEEK_SET);
         write (fd, (char *)&msghdr, sizeof (struct _msghdr));
         count += msghdr.numblocks;
         wpos = tell (fd);
         lseek (fd, rpos, SEEK_SET);

         msgidx.msgnum = msghdr.msgnum;
         msgidx.board = msghdr.board;

         write (fdidx, (char *)&msgidx, sizeof (struct _msgidx));
         write (fdto, msghdr.whoto, 36);

         msginfo.totalmsgs++;

         if (!msginfo.lowmsg)
            msginfo.lowmsg = msgidx.msgnum;

         if (msginfo.lowmsg > msgidx.msgnum)
            msginfo.lowmsg = msgidx.msgnum;
         if (msginfo.highmsg < msgidx.msgnum)
            msginfo.highmsg = msgidx.msgnum;

         msginfo.totalonboard[msgidx.board-1]++;
      }
   }

   chsize (fd, wpos);
   close (fd);
   close (fdidx);
   close (fdto);

   sprintf (filename, "%sMSGINFO.BBS", fido_msgpath);
   fd = open(filename,O_WRONLY|O_BINARY);
   write(fd, (char *)&msginfo, sizeof(struct _msginfo));
   close (fd);

   for (dim=1;dim<=200;dim++)
      cnum[dim] = 0;

   sprintf (filename, "%sSYSMSG.DAT", sys_path);
   fd = open(filename, O_RDONLY|O_BINARY);
   if (fd == -1)
      exit (1);

   while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA )
   {
      if ( !tsys.quick_board )
         continue;

      cnum[tsys.quick_board] = tsys.msg_num;
   }

   close (fd);

   sprintf (filename, "%s.BBS", user_file);
   fd = open(filename, O_RDWR | O_BINARY);
   wpos = tell (fd);

   while (read(fd, (char *) &usr, sizeof(struct _usr)) == sizeof(struct _usr))
   {
      for (area=0; area<MAXLREAD; area++)
      {
         for (dim=1;dim<=200;dim++)
         {
            if (usr.lastread[area].area == cnum[dim])
               break;
         }

         if (dim <= 200 && usr.lastread[area].msg_num)
            usr.lastread[area].msg_num = qren[dim][usr.lastread[area].msg_num];
      }

      for (area=0; area<MAXDLREAD; area++)
      {
         for (dim=1;dim<=200;dim++)
         {
            if (usr.dynlastread[area].area == cnum[dim])
               break;
         }

         if (dim <= 200 && usr.dynlastread[area].msg_num)
            usr.dynlastread[area].msg_num = qren[dim][usr.dynlastread[area].msg_num];
      }

      lseek(fd, wpos, 0);
      write(fd, (char *) &usr, sizeof(struct _usr));

      wpos = tell (fd);
   }

   close(fd);

   for (dim=1;dim<=200;dim++)
   {
      if (qren[dim] != NULL)
         farfree (qren[dim]);
   }
}

void Get_Config_Info ()
{
   FILE *fp;
   char linea[256], opt[3][50], *p;
   char *config_file = "LORA.CFG";

   if ((p=getenv ("LORA")) != NULL) {
      strcpy (linea, p);
      Append_Backslash (linea);
      strcat (linea, config_file);
      config_file = linea;
   }

   fp = fopen(config_file,"rt");
   if (fp == NULL) {
      printf ("\nCouldn't open CFG file: %s\a\n", config_file);
      exit (1);
   }

   sys_path = "";
   fido_msgpath = "";
   user_file = "USERS";

   while (fgets(linea, 255, fp) != NULL) {
      linea[strlen(linea)-1] = '\0';
      if (!strlen(linea) || linea[0] == ';' || linea[0] == '%')
              continue;

      sscanf(linea,"%s %s %s", opt[0], opt[1], opt[2]);

      if (!stricmp (opt[0], "QUICKBASE_PATH")) {
         Append_Backslash (opt[1]);
         fido_msgpath = (char *)malloc(strlen(opt[1])+1);
         strcpy(fido_msgpath, opt[1]);
      }
      else if (!stricmp (opt[0], "SYSTEM_PATH")) {
         Append_Backslash (opt[1]);
         sys_path = (char *)malloc(strlen(opt[1])+1);
         strcpy(sys_path, opt[1]);
      }
      else if (!stricmp (opt[0], "USER_FILE")) {
         user_file = (char *)malloc(strlen(opt[1])+1);
         strcpy(user_file, opt[1]);
      }
   }

   fclose (fp);
}

void Append_Backslash (s)
char *s;
{
   int i;

   i = strlen(s) - 1;
   if (s[i] != '\\')
      strcat (s, "\\");
}

void Purge_Squish_Areas (renum)
int renum;
{
   int fd, first, last, i, msgnum, kill;
   char filename[80];
   struct _sys tsys;
   struct _minf minf;
   MSG *sq_ptr;

   minf.def_zone = 0;
   if (MsgOpenApi (&minf))
      return;

   printf(" * Purging SquishMail<tm> messages\n");

   sprintf (filename, "%sSYSMSG.DAT", sys_path);
   fd = open(filename, O_RDONLY|O_BINARY);
   if (fd == -1)
      exit (1);

   first = 1;
   msgnum = last = 0;

   while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
      if ( !tsys.squish )
         continue;

      sq_ptr = MsgOpenArea (tsys.msg_path, MSGAREA_NORMAL, MSGTYPE_SQUISH);
      if (sq_ptr == NULL)
         continue;

      msgnum = (int)MsgGetNumMsg (sq_ptr);
      last = msgnum;

      if ( tsys.max_msgs && tsys.max_msgs < msgnum ) {
         kill = msgnum - tsys.max_msgs;

         for (i = first; i <= last; i++) {
            MsgKillMsg (sq_ptr, i);

            if (--kill <= 0)
               break;
         }
      }

      if ( tsys.max_age || tsys.age_rcvd )
         Squish_Kill_by_Age (sq_ptr, tsys.max_age, tsys.age_rcvd, first, last);

      if (renum)
         Squish_Renum (sq_ptr, first, last, tsys.msg_num);

      MsgCloseArea (sq_ptr);
   }

   close (fd);
}

void Squish_Kill_by_Age (sq_ptr, max_age, age_rcvd, first, last)
MSG *sq_ptr;
int max_age, age_rcvd, first, last;
{
   int yr, mo, dy, i, month[12], m, days;
   char mese[4];
   long day_mess, day_now;
   struct tm *tim;
   long tempo;
   MSGH *msgh;
   XMSG xmsg;

   char *mon[] = {
      "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
      "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
   };

   month[0] = 31;
   month[1] = 28;
   month[2] = 31;
   month[3] = 30;
   month[4] = 31;
   month[5] = 30;
   month[6] = 31;
   month[7] = 31;
   month[8] = 30;
   month[9] = 31;
   month[10] = 30;
   month[11] = 31;

   tempo = time(0);
   tim = localtime(&tempo);

   day_now = 365 * tim->tm_year;
   for (m = 0; m < tim->tm_mon; m++)
      day_now += month[m];
   day_now += tim->tm_mday;

   for (i = first; i <= last; i++) {
      msgh = MsgOpenMsg (sq_ptr, MOPEN_RW, (dword)i);
      if (msgh == NULL)
         continue;

      if (MsgReadMsg (msgh, &xmsg, 0L, 0L, NULL, 0, NULL) == -1) {
         MsgCloseMsg (msgh);
         continue;
      }

      MsgCloseMsg (msgh);

      sscanf (xmsg.ftsc_date, "%2d %3s %2d", &dy, mese, &yr);
      mese[3] = '\0';
      for (mo = 0; mo < 12; mo++)
         if (!stricmp(mese, mon[mo]))
            break;
      if (mo == 12)
         mo = 0;

      day_mess = 365 * yr;
      for (m = 0; m < mo; m++)
         day_mess += month[m];
      day_mess += dy;

      days = (int)(day_now - day_mess);

      if ( days > max_age && max_age != 0 )
         MsgKillMsg (sq_ptr, i);

      if ( ((xmsg.attr & MSGREAD) && (xmsg.attr & MSGPRIVATE)) &&
           days > age_rcvd && age_rcvd != 0)
         MsgKillMsg (sq_ptr, i);
   }
}

void Renum_Squish_Areas ()
{
   int fd, first, last;
   char filename[60];
   struct _sys tsys;
   MSG *sq_ptr;

   sprintf (filename, "%sSYSMSG.DAT", sys_path);
   fd = open(filename, O_RDONLY|O_BINARY);
   if (fd == -1)
      exit (1);

   first = 1;

   while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
      if ( !tsys.squish )
         continue;

      sq_ptr = MsgOpenArea (tsys.msg_path, MSGAREA_NORMAL, MSGTYPE_SQUISH);
      if (sq_ptr == NULL)
         continue;

      last = (int)MsgGetNumMsg (sq_ptr);

      Squish_Renum (sq_ptr, first, last, tsys.msg_num);

      MsgCloseArea (sq_ptr);
   }

   close(fd);
}

void Squish_Renum (sq_ptr, first, last, area_n)
MSG *sq_ptr;
int first, last, area_n;
{
   int fd, m, i, x, msg[1000], flag1, flag, area;
   char filename[80];
   long pos;
   struct _usr usr;
   MSGH *msgh;

   m = flag = 0;

   for (i = first; i <= last; i++) {
      if ((msgh = MsgOpenMsg (sq_ptr, MOPEN_RW, (dword)i)) != NULL) {
         MsgCloseMsg (msgh);

         msg[m++] = i;

         if (i != m)
            flag = 1;
      }
   }

   if (!flag)
      return;

   sprintf (filename, "%s.BBS", user_file);
   fd = open(filename, O_RDWR | O_BINARY);
   pos = tell (fd);

   while (read(fd, (char *) &usr, sizeof(struct _usr)) == sizeof(struct _usr)) {
      for (area = 0; area < MAXLREAD; area++)
         if (usr.lastread[i].area == area_n)
            break;

      if (area < MAXLREAD) {
         flag1 = 0;

         for (x = 0; x < m; x++)
            if (msg[x] > flag1 && msg[x] <= usr.lastread[area].msg_num)
               flag1 = x + 1;

         usr.lastread[area].msg_num = flag1;

         lseek(fd, pos, 0);
         write(fd, (char *) &usr, sizeof(struct _usr));
      }
      else {
         for (area = 0; area < MAXDLREAD; area++)
            if (usr.dynlastread[i].area == area_n)
               break;

         if (area < MAXDLREAD) {
            flag1 = 0;

            for (x = 0; x < m; x++)
               if (msg[x] > flag1 && msg[x] <= usr.dynlastread[area].msg_num)
                  flag1 = x + 1;

            usr.dynlastread[area].msg_num = flag1;

            lseek(fd, pos, 0);
            write(fd, (char *) &usr, sizeof(struct _usr));
         }
      }

      pos = tell (fd);
   }

   close(fd);
}

