
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
#include <ctype.h>
#include <mem.h>
#include <alloc.h>
#include <string.h>
#include <io.h>
#include <dos.h>
#include <conio.h>
#include <fcntl.h>
#include <time.h>
#include <share.h>
#include <sys\stat.h>

#define shopen(path,access)       open(path,(access)|SH_DENYNONE)
#define cshopen(path,access,mode) open(path,(access)|SH_DENYNONE,mode)

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "lora.h"
#include "timer.h"
#include "version.h"

long timerset(int);
int timeup(long);
void dostime (int *, int *, int *, int *);
void dosdate (int *, int *, int *, int *);
void mtask_find (void);
void mtask_find(void);
int dv_get_version(void);
int ddos_active(void);
int tv_get_version(void);
int ml_active(void);
int windows_active (void);
void dv_pause(void);
void ddos_pause(void);
void tv_pause(void);
void ml_pause(void);
void msdos_pause(void);
void windows_pause (void);

char have_dv = 0, have_ml = 0, have_tv = 0, have_ddos = 0;

void main (void)
{
   FILE *fp;
   int fd, i, line, x, y, mainview, status, task[20], nd;
   char linea[128], *p, c, *names[20], msgs[3][70];
   long t, cl;
   struct _useron useron;
   struct time timep;

   mtask_find ();
   videoinit ();
   hidecur ();
   readcur (&x, &y);

   for (i = 0; i < 17; i++)
      names[i] = (char *)malloc (36);
   names[i] = NULL;

   mainview = wopen (3, 0, 23, 79, 0, LBLUE|_BLACK, LGREY|_BLACK);
   wactiv (mainview);
   whline (1, 0, 79, 0, LBLUE|_BLACK);
   wvline (0, 32, 20, 0, LBLUE|_BLACK);
   wvline (0, 36, 20, 0, LBLUE|_BLACK);
   wvline (0, 42, 20, 0, LBLUE|_BLACK);
   wvline (0, 63, 20, 0, LBLUE|_BLACK);
   wvline (0, 66, 20, 0, LBLUE|_BLACK);
   wvline (0, 75, 20, 0, LBLUE|_BLACK);
   wprints (0, 0, WHITE|_BLACK, "Name");
   wprints (0, 33, WHITE|_BLACK, "Tsk");
   wprints (0, 37, WHITE|_BLACK, "Speed");
   wprints (0, 43, WHITE|_BLACK, "Location");
   wprints (0, 64, WHITE|_BLACK, "DC");
   wprints (0, 67, WHITE|_BLACK, "Status");
   wprints (0, 76, WHITE|_BLACK, "CB#");

   i = wopen (24, 0, 24, 79, 5, LBLUE|_BLACK, BLACK|_LGREY);
   wactiv (i);

#ifdef __OS2__
   sprintf (linea, "LoraBBS-OS/2 Network Manager %s - Copyright (c) 1992-96 by Marco Maccaferri", LNETMGR_VERSION);
#else
   sprintf (linea, "LoraBBS-DOS Network Manager %s - Copyright (c) 1992-96 by Marco Maccaferri", LNETMGR_VERSION);
#endif
   wcenters (0, BLACK|_LGREY, linea);

   status = wopen (0, 0, 2, 79, 1, LGREEN|_BLACK, LGREY|_BLACK);
   wactiv (status);
   wprints (0, 1, WHITE|_BLACK, "Shut down node   Broadcat   Message    Quit             ");
   wprintc (0, 1, YELLOW|_BLACK, 'S');
   wprintc (0, 18, YELLOW|_BLACK, 'B');
   wprintc (0, 29, YELLOW|_BLACK, 'M');
   wprintc (0, 40, YELLOW|_BLACK, 'Q');

   cl = t = timerset (0);
   wactiv (mainview);

   do {
      if (timeup (t)) {
         t = timerset (300);
         fd = shopen ("USERON.BBS", O_RDONLY|O_BINARY);

         for (line = 2; line < 19; line++) {
            if (read(fd, (char *)&useron, sizeof(struct _useron)) != sizeof(struct _useron))
               memset ((char *)&useron, 0, sizeof (struct _useron));

            useron.city[21] = '\0';

            sprintf (names[line - 2], "%-30.30s", useron.name);
            task[line - 2] = useron.line;

            if (useron.name[0]) {
               sprintf (linea, "%-31.31s", useron.name);
               wprints (line, 0, YELLOW|_BLACK, linea);
               sprintf (linea, "%3d", useron.line);
               wprints (line, 33, YELLOW|_BLACK, linea);
               sprintf (linea, "%5lu", useron.baud);
               wprints (line, 37, YELLOW|_BLACK, linea);
               sprintf (linea, "%-20.20s", useron.city);
               wprints (line, 43, YELLOW|_BLACK, linea);
               sprintf (linea, "%c%c", useron.donotdisturb ? 'D' : ' ', useron.priv_chat ? 'C' : ' ');
               wprints (line, 64, YELLOW|_BLACK, linea);
               sprintf (linea, "%-8.8s", useron.status);
               wprints (line, 67, YELLOW|_BLACK, linea);
               sprintf (linea, "%3d", useron.cb_channel);
               wprints (line, 76, YELLOW|_BLACK, linea);
            }
            else if (useron.line_status) {
               if (useron.line_status != WFC)
                  wprints (line, 0, YELLOW|_BLACK, "                               ");
               else {
                  sprintf (linea, "%-31.31s", "Waiting for call");
                  wprints (line, 0, YELLOW|_BLACK, linea);
               }
               sprintf (linea, "%3d", useron.line);
               wprints (line, 33, YELLOW|_BLACK, linea);
               wprints (line, 37, YELLOW|_BLACK, "     ");
               wprints (line, 43, YELLOW|_BLACK, "                    ");
               wprints (line, 64, YELLOW|_BLACK, "  ");
               if (useron.line_status != WFC) {
                  sprintf (linea, "%-8.8s", useron.status);
                  wprints (line, 67, YELLOW|_BLACK, linea);
               }
               else
                  wprints (line, 67, YELLOW|_BLACK, "        ");
               wprints (line, 76, YELLOW|_BLACK, "   ");
            }
            else {
               wprints (line, 0, YELLOW|_BLACK, "                               ");
               wprints (line, 33, YELLOW|_BLACK, "   ");
               wprints (line, 37, YELLOW|_BLACK, "     ");
               wprints (line, 43, YELLOW|_BLACK, "                    ");
               wprints (line, 64, YELLOW|_BLACK, "  ");
               wprints (line, 67, YELLOW|_BLACK, "        ");
               wprints (line, 76, YELLOW|_BLACK, "   ");
            }
         }

         close(fd);
      }

      if (timeup (cl)) {
         cl = timerset (100);
         gettime((struct time *)&timep);

         wactiv(status);

         sprintf(linea, "%02d:%02d:%02d", timep.ti_hour % 24, timep.ti_min % 60, timep.ti_sec % 60);
         wprints (0, 69, WHITE|_BLACK, linea);

         wactiv (mainview);
      }

      c = 0;
      if (kbhit ())
         c = toupper(getch ());
#ifndef __OS2__
      else {
         if (have_dv)
            dv_pause ();
         else if (have_ddos)
            ddos_pause ();
         else if (have_tv)
            tv_pause ();
         else if (have_ml)
            ml_pause ();
         else
            msdos_pause ();
      }
#endif

      if (c == 'S') {
         wactiv (status);
         wprints (0, 1, WHITE|_BLACK, "Select the node to shut down then <enter>, <esc> to exit");
         i = wpickstr (6, 1, 22, 30, 5, LGREY|_BLACK, YELLOW|_BLACK, BLACK|_CYAN, names, 0, NULL);
         if (i != -1 && task[i]) {
            sprintf (linea, "LEXIT%d.1", task[i]);
            i = open (linea, O_WRONLY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE);
            close (i);
         }
         wactiv (status);
         wprints (0, 1, WHITE|_BLACK, "Shut down node   Broadcat   Message    Quit                ");
         wprintc (0, 1, YELLOW|_BLACK, 'S');
         wprintc (0, 18, YELLOW|_BLACK, 'B');
         wprintc (0, 29, YELLOW|_BLACK, 'M');
         wprintc (0, 40, YELLOW|_BLACK, 'Q');
         wactiv (mainview);
      }
      else if (c == 'B') {
         i = wopen (11, 5, 16, 74, 1, YELLOW|_BLUE, BLACK|_BLUE);
         wactiv (i);
         wprints (0, 1, WHITE|_BLUE, "Write your message, <esc> exit");
         winpbeg (WHITE|_RED, WHITE|_RED);

         winpdef (1, 1, msgs[0], "??????????????????????????????????????????????????????????????????", '?', 0, NULL, 0);
         winpdef (2, 1, msgs[1], "??????????????????????????????????????????????????????????????????", '?', 0, NULL, 0);
         winpdef (3, 1, msgs[2], "??????????????????????????????????????????????????????????????????", '?', 0, NULL, 0);
         showcur ();

         if (winpread () != W_ESCPRESS) {
            strtrim (msgs[0]);
            strtrim (msgs[1]);
            strtrim (msgs[2]);

            if ((fd = shopen ("USERON.BBS", O_RDONLY|O_BINARY)) != -1) {
               for (line = 2; line < 19; line++) {
                  if (read(fd, (char *)&useron, sizeof(struct _useron)) != sizeof(struct _useron))
                     break;

                   if (useron.status == 0)
                      continue;

                   sprintf(linea, "LINE%d.BBS", useron.line);
                   fp = fopen(linea, "at");

                   fprintf(fp, "\n\n** MESSAGE **\n");
                   fprintf(fp, "From: System Manager\n");
                   fprintf(fp, "\n\"%s", msgs[0]);
                   if (msgs[1][0])
                      fprintf (fp, "\n%s", msgs[1]);
                   if (msgs[2][0])
                      fprintf (fp, "\n%s", msgs[2]);
                   fprintf (fp, "\".\n\n", msgs[0], msgs[1], msgs[2]);
                   fprintf(fp, "ç\001");

                   fclose(fp);
               }

               close(fd);
            }
         }

         hidecur ();
         wactiv (mainview);
      }
      else if (c == 'M') {
         wactiv (status);
         wprints (0, 1, WHITE|_BLACK, "Select the node to send message then <enter>, <esc> to exit");
         nd = wpickstr (6, 1, 22, 30, 5, LGREY|_BLACK, YELLOW|_BLACK, BLACK|_CYAN, names, 0, NULL);
         if (nd != -1 && task[nd]) {
            i = wopen (11, 5, 16, 74, 1, YELLOW|_BLUE, BLACK|_BLUE);
            wactiv (i);
            wprints (0, 1, WHITE|_BLUE, "Write your message, <esc> exit");
            winpbeg (WHITE|_RED, WHITE|_RED);

            winpdef (1, 1, msgs[0], "??????????????????????????????????????????????????????????????????", '?', 0, NULL, 0);
            winpdef (2, 1, msgs[1], "??????????????????????????????????????????????????????????????????", '?', 0, NULL, 0);
            winpdef (3, 1, msgs[2], "??????????????????????????????????????????????????????????????????", '?', 0, NULL, 0);
            showcur ();

            if (winpread () != W_ESCPRESS) {
               strtrim (msgs[0]);
               strtrim (msgs[1]);
               strtrim (msgs[2]);

                sprintf(linea, "LINE%d.BBS", task[nd]);
                fp = fopen(linea, "at");

                fprintf(fp, "\n\n** MESSAGE **\n");
                fprintf(fp, "From: System Manager\n");
                fprintf(fp, "\n\"%s", msgs[0]);
                if (msgs[1][0])
                   fprintf (fp, "\n%s", msgs[1]);
                if (msgs[2][0])
                   fprintf (fp, "\n%s", msgs[2]);
                fprintf (fp, "\".\n\n", msgs[0], msgs[1], msgs[2]);
                fprintf(fp, "ç\001");

                fclose(fp);
            }

            hidecur ();
         }

         wactiv (status);
         wprints (0, 1, WHITE|_BLACK, "Shut down node   Broadcat   Message    Quit                ");
         wprintc (0, 1, YELLOW|_BLACK, 'S');
         wprintc (0, 18, YELLOW|_BLACK, 'B');
         wprintc (0, 29, YELLOW|_BLACK, 'M');
         wprintc (0, 40, YELLOW|_BLACK, 'Q');
         wactiv (mainview);
      }

   } while (c != 'Q');

   wcloseall ();
   gotoxy_ (x, y);
   showcur ();
}

long timerset (t)
int t;
{
   long l;
#ifdef __OS2__
   struct time timep;
#endif

#ifdef HIGH_OVERHEAD
   int l2;

#endif
   int hours, mins, secs, ths;

#ifdef HIGH_OVERHEAD
   extern int week_day ();

#endif

   /* Get the DOS time and day of week */
#ifdef __OS2__
   gettime (&timep);
   hours = timep.ti_hour;
   mins = timep.ti_min;
   secs = timep.ti_sec;
   ths = timep.ti_hund;
#else
   dostime (&hours, &mins, &secs, &ths);
#endif

#ifdef HIGH_OVERHEAD
   l2 = week_day ();
#endif

   /* Figure out the hundredths of a second so far this week */
   l =

#ifdef HIGH_OVERHEAD
      l2 * PER_DAY +
      (hours % 24) * PER_HOUR +
#endif
      (mins % 60) * PER_MINUTE +
      (secs % 60) * PER_SECOND +
      ths;

   /* Add in the timer value */
   l += t;

   /* Return the alarm off time */
   return (l);
}

int timeup (t)
long t;
{
   long l;

   /* Get current time in hundredths */
   l = timerset (0);

   /* If current is less then set by more than max int, then adjust */
   if (l < (t - 65536L))
      {
#ifdef HIGH_OVERHEAD
      l += PER_WEEK;
#else
      l += PER_HOUR;
#endif
      }

   /* Return whether the current is greater than the timer variable */
   return ((l - t) >= 0L);
}


void mtask_find ()
{
#ifndef __OS2__
   have_dv = 0;
   have_ml = 0;
   have_tv = 0;
   have_ddos = 0;

   if ((have_dv = dv_get_version()) != 0);
   else if ((have_ddos = ddos_active ()) != 0);
   else if ((have_ml = ml_active ()) != 0);
   else if ((have_tv = tv_get_version ()) != 0);
   else if (windows_active());
#endif
}

