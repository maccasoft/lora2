
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
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <alloc.h>
#include <sys\stat.h>

#include "lsetup.h"
#include "lprot.h"
#include "sched.h"

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>
#include <cxl\cxlkey.h>

extern struct _configuration config;

void start_update (void);
void stop_update (void);
char *get_priv_text (int);
void create_path (char *);
int sh_open (char *file, int shmode, int omode, int fmode);
void clear_window (void);

static void edit_single_event (EVENT *);
static void select_event_list (int, EVENT *);

void terminal_misc ()
{
   int wh, i = 1;
   char string[128];

   wh = wopen (7, 9, 17, 69, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Miscellaneous ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " Init            ", 0,  1, 0, NULL, 0, 0);
      wmenuitem ( 2,  1, " Download        ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " Upload          ", 0,  3, 0, NULL, 0, 0);
      wmenuitem ( 4,  1, " Avatar          ", 0,  4, 0, NULL, 0, 0);
      wmenuitem ( 5,  1, " Auto Zmodem     ", 0,  5, 0, NULL, 0, 0);
      wmenuitem ( 6,  1, " Dialing timeout ", 0,  6, 0, NULL, 0, 0);
      wmenuitem ( 7,  1, "    Pause        ", 0,  7, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 19, CYAN|_BLACK, config.term_init);
      wprints (2, 19, CYAN|_BLACK, config.dl_path);
      wprints (3, 19, CYAN|_BLACK, config.ul_path);
      wprints (4, 19, CYAN|_BLACK, config.avatar ? "Yes" : "No");
      wprints (5, 19, CYAN|_BLACK, config.autozmodem ? "Yes" : "No");
      sprintf (string, "%d", config.dial_timeout);
      wprints (6, 19, CYAN|_BLACK, string);
      sprintf (string, "%d", config.dial_pause);
      wprints (7, 19, CYAN|_BLACK, string);

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            strcpy (string, config.term_init);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (1, 19, string, "??????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.term_init, strbtrim (string));
            break;

         case 2:
            strcpy (string, config.dl_path);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (2, 19, string, "??????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (config.dl_path, strbtrim (string));
               if (config.dl_path[0] && config.dl_path[strlen (config.dl_path) - 1] != '\\')
                  strcat (config.dl_path, "\\");
               create_path (config.dl_path);
            }
            break;

         case 3:
            strcpy (string, config.ul_path);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 19, string, "??????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (config.ul_path, strbtrim (string));
               if (config.ul_path[0] && config.ul_path[strlen (config.ul_path) - 1] != '\\')
                  strcat (config.ul_path, "\\");
               create_path (config.ul_path);
            }
            break;

         case 4:
            config.avatar ^= 1;
            break;

         case 5:
            config.autozmodem ^= 1;
            break;

         case 6:
            sprintf (string, "%d", config.dial_timeout);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (6, 19, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.dial_timeout = atoi (string);
            break;

         case 7:
            sprintf (string, "%d", config.dial_pause);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (7, 19, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.dial_pause = atoi (string);
            break;
      }

      hidecur ();

   } while (i != -1);

   wclose ();
}

void iemsi_linehelp_window ()
{
   char *str = "";
   struct _menu_t *mt;

   mt = wmenumcurr ();
   if (mt == NULL)
      return;

   switch (mt->citem->tagid) {
      case 1:
         str = "Yes = IEMSI active";
         break;
      case 2:
         str = "Seconds that remote system infos stay on screen";
         break;
      case 3:
         str = "Password to use";
         break;
      case 4:
         str = "Alternate user's name";
         break;
      case 5:
         str = "Yes = Request hotkeyed menus";
         break;
      case 6:
         str = "Yes = Enable online messages";
         break;
      case 7:
         str = "Yes = Pause at the end of each screen page";
         break;
      case 8:
         str = "Yes = Full screen editor";
         break;
      case 9:
         str = "Yes = Shows news bulletins";
         break;
      case 10:
         str = "Yes = Check for new mail at login";
         break;
      case 11:
         str = "Yes = Check for new files at login";
         break;
      case 12:
         str = "Yes = Send screen clearing codes";
         break;
   }

   clear_window ();
   prints (24, 1, LGREY|_BLACK, str);
}

void terminal_iemsi ()
{
   int wh, i = 1;
   char string[128];

   wh = wopen (4, 13, 19, 69, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" IEMSI Profile ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " IEMSI On       ", 0,  1, 0, NULL, 0, 0);
      wmenuiba (iemsi_linehelp_window, clear_window);
      wmenuitem ( 2,  1, " Ã Info time    ", 0,  2, 0, NULL, 0, 0);
      wmenuiba (iemsi_linehelp_window, clear_window);
      wmenuitem ( 3,  1, " Ã Password     ", 0,  3, 0, NULL, 0, 0);
      wmenuiba (iemsi_linehelp_window, clear_window);
      wmenuitem ( 4,  1, " Ã Handle       ", 0,  4, 0, NULL, 0, 0);
      wmenuiba (iemsi_linehelp_window, clear_window);
      wmenuitem ( 5,  1, " Ã Hot keys     ", 0,  5, 0, NULL, 0, 0);
      wmenuiba (iemsi_linehelp_window, clear_window);
      wmenuitem ( 6,  1, " Ã Quiet        ", 0,  6, 0, NULL, 0, 0);
      wmenuiba (iemsi_linehelp_window, clear_window);
      wmenuitem ( 7,  1, " Ã Pausing      ", 0,  7, 0, NULL, 0, 0);
      wmenuiba (iemsi_linehelp_window, clear_window);
      wmenuitem ( 8,  1, " Ã Editor       ", 0,  8, 0, NULL, 0, 0);
      wmenuiba (iemsi_linehelp_window, clear_window);
      wmenuitem ( 9,  1, " Ã News         ", 0,  9, 0, NULL, 0, 0);
      wmenuiba (iemsi_linehelp_window, clear_window);
      wmenuitem (10,  1, " Ã New mail     ", 0, 10, 0, NULL, 0, 0);
      wmenuiba (iemsi_linehelp_window, clear_window);
      wmenuitem (11,  1, " Ã New files    ", 0, 11, 0, NULL, 0, 0);
      wmenuiba (iemsi_linehelp_window, clear_window);
      wmenuitem (12,  1, " À Screen clear ", 0, 12, 0, NULL, 0, 0);
      wmenuiba (iemsi_linehelp_window, clear_window);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 18, CYAN|_BLACK, config.iemsi_on ? "Yes" : "No");
      sprintf (string, "%d", config.iemsi_infotime);
      wprints (2, 18, CYAN|_BLACK, string);
      wprints (3, 18, CYAN|_BLACK, config.iemsi_pwd);
      wprints (4, 18, CYAN|_BLACK, config.iemsi_handle);
      wprints (5, 18, CYAN|_BLACK, config.iemsi_hotkeys ? "Yes" : "No");
      wprints (6, 18, CYAN|_BLACK, config.iemsi_quiet ? "Yes" : "No");
      wprints (7, 18, CYAN|_BLACK, config.iemsi_pausing ? "Yes" : "No");
      wprints (8, 18, CYAN|_BLACK, config.iemsi_editor ? "Yes" : "No");
      wprints (9, 18, CYAN|_BLACK, config.iemsi_news ? "Yes" : "No");
      wprints (10, 18, CYAN|_BLACK, config.iemsi_newmail ? "Yes" : "No");
      wprints (11, 18, CYAN|_BLACK, config.iemsi_newfiles ? "Yes" : "No");
      wprints (12, 18, CYAN|_BLACK, config.iemsi_screenclr ? "Yes" : "No");

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            config.iemsi_on ^= 1;
            break;

         case 2:
            sprintf (string, "%d", config.iemsi_infotime);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (2, 18, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.iemsi_infotime = atoi (string);
            break;

         case 3:
            strcpy (string, config.iemsi_pwd);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 18, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.iemsi_pwd, strbtrim (string));
            break;

         case 4:
            strcpy (string, config.iemsi_handle);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 18, string, "???????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.iemsi_handle, strbtrim (string));
            break;

         case 5:
            config.iemsi_hotkeys ^= 1;
            break;

         case 6:
            config.iemsi_quiet ^= 1;
            break;

         case 7:
            config.iemsi_pausing ^= 1;
            break;

         case 8:
            config.iemsi_editor ^= 1;
            break;

         case 9:
            config.iemsi_news ^= 1;
            break;

         case 10:
            config.iemsi_newmail ^= 1;
            break;

         case 11:
            config.iemsi_newfiles ^= 1;
            break;

         case 12:
            config.iemsi_screenclr ^= 1;
            break;
      }

      hidecur ();

   } while (i != -1);

   wclose ();
}

void manager_scheduler ()
{
   int fd, fdi, wh2, wh, wh1, i = 1, saved, minute, m, length, days;
   int t1, t2;
   char string[128], readed;
   long pos;
   EVENT nevt, bnevt;

   fd = open (config.sched_name, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
   if (fd == -1)
      return;

   if (!read (fd, (char *)&nevt, sizeof (EVENT))) {
      memset ((char *)&nevt, 0, sizeof (EVENT));
      readed = 0;
   }
   else
      readed = 1;

   gotoxy_ (24, 1);
   clreol_ ();
   prints (24, 1, LGREY|_BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add New Event  L-List  D-Delete  C-Copy");
   prints (24, 1, YELLOW|_BLACK, "PgUp/PgDn");
   prints (24, 26, YELLOW|_BLACK, "E");
   prints (24, 34, YELLOW|_BLACK, "A");
   prints (24, 51, YELLOW|_BLACK, "L");
   prints (24, 59, YELLOW|_BLACK, "D");
   prints (24, 69, YELLOW|_BLACK, "C");

   wh = wopen (3, 11, 21, 65, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Events ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wprints ( 1, 1, LGREY|_BLACK, " Title            ");
      wprints ( 2, 1, LGREY|_BLACK, " Days             ");
      wprints ( 3, 1, LGREY|_BLACK, " Start time       ");
      wprints ( 4, 1, LGREY|_BLACK, " Stop time        ");
      wprints ( 5, 1, LGREY|_BLACK, " Forced           ");
      wprints ( 6, 1, LGREY|_BLACK, " Allow humans     ");
      wprints ( 7, 1, LGREY|_BLACK, " Errorlevel       ");
      wprints ( 8, 1, LGREY|_BLACK, " Clock adjustment ");
      wprints ( 9, 1, LGREY|_BLACK, " Receive only     ");
      wprints (10, 1, LGREY|_BLACK, " Ã Max try        ");
      wprints (11, 1, LGREY|_BLACK, " Ã Max failed     ");
      wprints (12, 1, LGREY|_BLACK, " À Retry delay    ");
      wprints (13, 1, LGREY|_BLACK, " Forced poll      ");
      wprints (14, 1, LGREY|_BLACK, " À Reserved node  ");
      wprints (15, 1, LGREY|_BLACK, " Mail behavior    ");

      if (readed) {
         wprints (1, 20, CYAN|_BLACK, nevt.cmd);
         strcpy (string, "-------");
         if (nevt.days & DAY_SUNDAY)
            string[0] = 'S';
         if (nevt.days & DAY_MONDAY)
            string[1] = 'M';
         if (nevt.days & DAY_TUESDAY)
            string[2] = 'T';
         if (nevt.days & DAY_WEDNESDAY)
            string[3] = 'W';
         if (nevt.days & DAY_THURSDAY)
            string[4] = 'T';
         if (nevt.days & DAY_FRIDAY)
            string[5] = 'F';
         if (nevt.days & DAY_SATURDAY)
            string[6] = 'S';
         wprints (2, 20, CYAN|_BLACK, string);
         sprintf (string, "%02d:%02d", nevt.minute / 60, nevt.minute % 60);
         wprints (3, 20, CYAN|_BLACK, string);
         m = nevt.minute + nevt.length;
         sprintf (string, "%02d:%02d", m / 60, m % 60);
         wprints (4, 20, CYAN|_BLACK, string);
         if (nevt.behavior & MAT_FORCED)
            wprints (5, 20, CYAN|_BLACK, "Yes");
         else
            wprints (5, 20, CYAN|_BLACK, "No");
         if (nevt.behavior & MAT_BBS)
            wprints (6, 20, CYAN|_BLACK, "Yes");
         else
            wprints (6, 20, CYAN|_BLACK, "No");
         sprintf (string, "%d", nevt.errlevel[0]);
         wprints (7, 20, CYAN|_BLACK, string);
         if (nevt.behavior & MAT_RESYNC)
            wprints (8, 20, CYAN|_BLACK, "Yes");
         else
            wprints (8, 20, CYAN|_BLACK, "No");
         if (nevt.behavior & MAT_NOOUT)
            wprints (9, 20, CYAN|_BLACK, "Yes");
         else
            wprints (9, 20, CYAN|_BLACK, "No");
         sprintf (string, "%d", nevt.no_connect);
         wprints (10, 20, CYAN|_BLACK, string);
         sprintf (string, "%d", nevt.with_connect);
         wprints (11, 20, CYAN|_BLACK, string);
         sprintf (string, "%d", nevt.wait_time);
         wprints (12, 20, CYAN|_BLACK, string);
         wprints (13, 20, CYAN|_BLACK, (nevt.echomail & ECHO_FORCECALL) ? "Yes" : "No");
         if (nevt.behavior & MAT_RESERV) {
            sprintf (string, "%d:%d/%d", nevt.res_zone, nevt.res_net, nevt.res_node);
            wprints (14, 20, CYAN|_BLACK, string);
         }
      }

      start_update ();
      i = getxch ();
      if ( (i & 0xFF) != 0 )
         i &= 0xFF;

      switch (i) {
         // PgDn
         case 0x5100:
            if (readed)
               read (fd, (char *)&nevt, sizeof (EVENT));
            break;

         // PgUp
         case 0x4900:
            if (readed) {
               if (tell (fd) > sizeof (EVENT)) {
                  lseek (fd, -2L * sizeof (EVENT), SEEK_CUR);
                  read (fd, (char *)&nevt, sizeof (EVENT));
               }
            }
            break;

         // C Copy
         case 'C':
         case 'c':
            wh2 = wh;

            nevt.cmd[0] = '\0';
            minute = nevt.minute = 0;
            length = nevt.length = 0;
            days = nevt.days = 0;
            i = 1;

            wh = wopen (11, 28, 18, 76, 1, LCYAN|_BLACK, CYAN|_BLACK);
            wactiv (wh);
            wshadow (DGREY|_BLACK);
            wtitle (" Copy event ", TRIGHT, YELLOW|_BLUE);

            do {
               stop_update ();
               wclear ();

               wmenubegc ();
               wmenuitem ( 1,  1, " Title       ", 0,  1, 0, NULL, 0, 0);
               wmenuitem ( 2,  1, " Days        ", 0,  2, 0, NULL, 0, 0);
               wmenuitem ( 3,  1, " Start time  ", 0,  3, 0, NULL, 0, 0);
               wmenuitem ( 4,  1, " Stop time   ", 0,  4, 0, NULL, 0, 0);
               wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

               wprints (1, 15, CYAN|_BLACK, nevt.cmd);
               strcpy (string, "-------");
               if (nevt.days & DAY_SUNDAY)
                  string[0] = 'S';
               if (nevt.days & DAY_MONDAY)
                  string[1] = 'M';
               if (nevt.days & DAY_TUESDAY)
                  string[2] = 'T';
               if (nevt.days & DAY_WEDNESDAY)
                  string[3] = 'W';
               if (nevt.days & DAY_THURSDAY)
                  string[4] = 'T';
               if (nevt.days & DAY_FRIDAY)
                  string[5] = 'F';
               if (nevt.days & DAY_SATURDAY)
                  string[6] = 'S';
               wprints (2, 15, CYAN|_BLACK, string);
               sprintf (string, "%02d:%02d", nevt.minute / 60, nevt.minute % 60);
               wprints (3, 15, CYAN|_BLACK, string);
               m = nevt.minute + nevt.length;
               sprintf (string, "%02d:%02d", m / 60, m % 60);
               wprints (4, 15, CYAN|_BLACK, string);

               start_update ();
               i = wmenuget ();

               switch (i) {
                  case 1:
                     strcpy (string, nevt.cmd);
                     winpbeg (BLUE|_GREEN, BLUE|_GREEN);
                     winpdef (1, 15, string, "??????????????????????????????", 0, 2, NULL, 0);
                     if (winpread () != W_ESCPRESS)
                        strcpy (nevt.cmd, strbtrim (string));
                     break;

                  case 2:
                     wh1 = wopen (4, 30, 14, 50, 1, LCYAN|_BLACK, CYAN|_BLACK);
                     wactiv (wh1);
                     wshadow (DGREY|_BLACK);
                     wtitle (" Days ", TRIGHT, YELLOW|_BLUE);
                     i = 1;

                     do {
                        stop_update ();
                        wclear ();

                        wmenubegc ();
                        wmenuitem ( 1,  1, " Sunday    ", 0, 1, 0, NULL, 0, 0);
                        wmenuitem ( 2,  1, " Monday    ", 0, 2, 0, NULL, 0, 0);
                        wmenuitem ( 3,  1, " Tuesday   ", 0, 3, 0, NULL, 0, 0);
                        wmenuitem ( 4,  1, " Wednesday ", 0, 4, 0, NULL, 0, 0);
                        wmenuitem ( 5,  1, " Thursday  ", 0, 5, 0, NULL, 0, 0);
                        wmenuitem ( 6,  1, " Friday    ", 0, 6, 0, NULL, 0, 0);
                        wmenuitem ( 7,  1, " Saturday  ", 0, 7, 0, NULL, 0, 0);
                        wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

                        if (nevt.days & DAY_SUNDAY)
                           wprints (1, 13, CYAN|_BLACK, "Yes");
                        else
                           wprints (1, 13, CYAN|_BLACK, "No");
                        if (nevt.days & DAY_MONDAY)
                           wprints (2, 13, CYAN|_BLACK, "Yes");
                        else
                           wprints (2, 13, CYAN|_BLACK, "No");
                        if (nevt.days & DAY_TUESDAY)
                           wprints (3, 13, CYAN|_BLACK, "Yes");
                        else
                           wprints (3, 13, CYAN|_BLACK, "No");
                        if (nevt.days & DAY_WEDNESDAY)
                           wprints (4, 13, CYAN|_BLACK, "Yes");
                        else
                           wprints (4, 13, CYAN|_BLACK, "No");
                        if (nevt.days & DAY_THURSDAY)
                           wprints (5, 13, CYAN|_BLACK, "Yes");
                        else
                           wprints (5, 13, CYAN|_BLACK, "No");
                        if (nevt.days & DAY_FRIDAY)
                           wprints (6, 13, CYAN|_BLACK, "Yes");
                        else
                           wprints (6, 13, CYAN|_BLACK, "No");
                        if (nevt.days & DAY_SATURDAY)
                           wprints (7, 13, CYAN|_BLACK, "Yes");
                        else
                           wprints (7, 13, CYAN|_BLACK, "No");

                        start_update ();
                        i = wmenuget ();

                        switch (i) {
                           case 1:
                              nevt.days ^= DAY_SUNDAY;
                              break;

                           case 2:
                              nevt.days ^= DAY_MONDAY;
                              break;

                           case 3:
                              nevt.days ^= DAY_TUESDAY;
                              break;

                           case 4:
                              nevt.days ^= DAY_WEDNESDAY;
                              break;

                           case 5:
                              nevt.days ^= DAY_THURSDAY;
                              break;

                           case 6:
                              nevt.days ^= DAY_FRIDAY;
                              break;

                           case 7:
                              nevt.days ^= DAY_SATURDAY;
                              break;
                        }

                     } while (i != -1);

                     wclose ();

                     i = 2;
                     break;

                  case 3:
                     sprintf (string, "%02d:%02d", nevt.minute / 60, nevt.minute % 60);
                     winpbeg (BLUE|_GREEN, BLUE|_GREEN);
                     winpdef (3, 15, string, "??????", 0, 2, NULL, 0);
                     if (winpread () != W_ESCPRESS) {
                        sscanf (string, "%d:%d", &t1, &t2);
                        nevt.minute = t1 * 60 + t2;
                     }
                     break;

                  case 4:
                     m = nevt.minute + nevt.length;
                     sprintf (string, "%02d:%02d", m / 60, m % 60);
                     winpbeg (BLUE|_GREEN, BLUE|_GREEN);
                     winpdef (4, 15, string, "??????", 0, 2, NULL, 0);
                     if (winpread () != W_ESCPRESS) {
                        sscanf (string, "%d:%d", &t1, &t2);
                        m = t1 * 60 + t2;
                        nevt.length = m - nevt.minute;
                     }
                     break;
               }

               hidecur ();
            } while (i != -1);

            wclose ();
            wh = wh2;
            wactiv (wh);

            if (nevt.minute != minute || nevt.length != length || nevt.days != days) {
               wh1 = wopen (10, 25, 14, 54, 0, BLACK|_LGREY, BLACK|_LGREY);
               wactiv (wh1);
               wshadow (DGREY|_BLACK);

               wcenters (1, BLACK|_LGREY, "Save changes (Y/n) ?  ");

               strcpy (string, "Y");
               winpbeg (BLACK|_LGREY, BLACK|_LGREY);
               winpdef (1, 24, string, "?", 0, 2, NULL, 0);

               i = winpread ();
               wclose ();
               hidecur ();

               if (i == W_ESCPRESS || toupper (string[0]) == 'N') {
                  i = 'C';
                  break;
               }

               nevt.last_ran = 0;

               pos = tell (fd) - (long)sizeof (EVENT);
               close (fd);

               unlink ("SCHED.BAK");
               rename (config.sched_name, "SCHED.BAK");

               fd = open ("SCHED.BAK", O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
               fdi = open (config.sched_name, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE);
               saved = 0;

               while (read (fd, (char *)&bnevt, sizeof (EVENT)) == sizeof (EVENT)) {
                  if (!saved && (nevt.minute + nevt.length) <= (bnevt.minute + bnevt.length)) {
                     pos = tell (fdi);
                     write (fdi, (char *)&nevt, sizeof (EVENT));
                     saved = 1;
                  }
                  if (bnevt.minute != minute || bnevt.length != length || bnevt.days != days)
                     write (fdi, (char *)&bnevt, sizeof (EVENT));
               }

               if (!saved) {
                  pos = tell (fdi);
                  write (fdi, (char *)&nevt, sizeof (EVENT));
               }

               close (fd);
               close (fdi);

               unlink ("SCHED.BAK");

               fd = open (config.sched_name, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
               lseek (fd, pos, SEEK_SET);
               read (fd, (char *)&nevt, sizeof (EVENT));
            }
            else {
               lseek (fd, -1L * sizeof (EVENT), SEEK_CUR);
               read (fd, (char *)&nevt, sizeof (EVENT));
            }

            i = 'C';
            break;

         // E Edit
         case 'E':
         case 'e':
            if (readed) {
               minute = nevt.minute;
               length = nevt.length;
               days = nevt.days;

               edit_single_event (&nevt);

               if (nevt.minute != minute || nevt.length != length) {
                  pos = tell (fd) - (long)sizeof (EVENT);
                  close (fd);

                  unlink ("SCHED.BAK");
                  rename (config.sched_name, "SCHED.BAK");

                  fd = open ("SCHED.BAK", O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
                  fdi = open (config.sched_name, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE);
                  saved = 0;

                  while (read (fd, (char *)&bnevt, sizeof (EVENT)) == sizeof (EVENT)) {
                     if (!saved && (nevt.minute + nevt.length) <= (bnevt.minute + bnevt.length)) {
                        pos = tell (fdi);
                        write (fdi, (char *)&nevt, sizeof (EVENT));
                        saved = 1;
                     }
                     if (bnevt.minute != minute || bnevt.length != length || bnevt.days != days)
                        write (fdi, (char *)&bnevt, sizeof (EVENT));
                  }

                  if (!saved) {
                     pos = tell (fdi);
                     write (fdi, (char *)&nevt, sizeof (EVENT));
                  }

                  close (fd);
                  close (fdi);

                  unlink ("SCHED.BAK");

                  fd = open (config.sched_name, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
                  lseek (fd, pos, SEEK_SET);
                  read (fd, (char *)&nevt, sizeof (EVENT));
               }
               else {
                  lseek (fd, -1L * sizeof (EVENT), SEEK_CUR);
                  write (fd, (char *)&nevt, sizeof (EVENT));
               }
            }
            break;

         // L List
         case 'L':
         case 'l':
            if (readed)
               select_event_list (fd, &nevt);
            break;

         // A Add
         case 'A':
         case 'a':
            memset ((char *)&nevt, 0, sizeof (EVENT));
            minute = nevt.minute;
            length = nevt.length;
            days = nevt.days = 0x7F;

            edit_single_event (&nevt);

            if (nevt.minute != minute || nevt.length != length || nevt.days != days) {
               pos = tell (fd) - (long)sizeof (EVENT);
               close (fd);

               unlink ("SCHED.BAK");
               rename (config.sched_name, "SCHED.BAK");

               fd = open ("SCHED.BAK", O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
               fdi = open (config.sched_name, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE);
               saved = 0;

               while (read (fd, (char *)&bnevt, sizeof (EVENT)) == sizeof (EVENT)) {
                  if (!saved && (nevt.minute + nevt.length) <= (bnevt.minute + bnevt.length)) {
                     pos = tell (fdi);
                     write (fdi, (char *)&nevt, sizeof (EVENT));
                     saved = 1;
                  }
                  if (bnevt.minute != minute || bnevt.length != length || bnevt.days != days)
                     write (fdi, (char *)&bnevt, sizeof (EVENT));
               }

               if (!saved) {
                  pos = tell (fdi);
                  write (fdi, (char *)&nevt, sizeof (EVENT));
               }

               close (fd);
               close (fdi);

               unlink ("SCHED.BAK");

               fd = open (config.sched_name, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
               lseek (fd, pos, SEEK_SET);
               read (fd, (char *)&nevt, sizeof (EVENT));
            }
            else {
               lseek (fd, -1L * sizeof (EVENT), SEEK_CUR);
               read (fd, (char *)&nevt, sizeof (EVENT));
            }
            break;

         // D Delete
         case 'D':
         case 'd':
            wh1 = wopen (10, 25, 14, 54, 0, BLACK|_LGREY, BLACK|_LGREY);
            wactiv (wh1);
            wshadow (DGREY|_BLACK);

            wcenters (1, BLACK|_LGREY, "Are you sure (Y/n) ?  ");

            strcpy (string, "Y");
            winpbeg (BLACK|_LGREY, BLACK|_LGREY);
            winpdef (1, 24, string, "?", 0, 2, NULL, 0);

            i = winpread ();
            wclose ();
            hidecur ();

            if (i == W_ESCPRESS)
               break;

            if (toupper (string[0]) == 'Y') {
               minute = nevt.minute;
               length = nevt.length;
               days = nevt.days;

               pos = tell (fd) - (long)sizeof (EVENT);
               close (fd);

               unlink ("SCHED.BAK");
               rename (config.sched_name, "SCHED.BAK");

               fd = open ("SCHED.BAK", O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
               fdi = open (config.sched_name, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE);
               saved = 0;

               while (read (fd, (char *)&bnevt, sizeof (EVENT)) == sizeof (EVENT)) {
                  if (bnevt.minute != minute || bnevt.length != length || bnevt.days != days)
                     write (fdi, (char *)&bnevt, sizeof (EVENT));
               }

               close (fd);
               close (fdi);

               unlink ("SCHED.BAK");

               fd = open (config.sched_name, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
               if (lseek (fd, pos, SEEK_SET) == -1 || pos == filelength (fd)) {
                  lseek (fd, -1l * sizeof (EVENT), SEEK_END);
                  pos = tell (fd);
               }
               read (fd, (char *)&nevt, sizeof (EVENT));
            }
            break;

         // ESC Exit
         case 0x1B:
            i = -1;
            break;
      }

   } while (i != -1);

   close (fd);

   wclose ();
   gotoxy_ (24, 1);
   clreol_ ();
}

void select_event_list (fd, nevt)
int fd;
EVENT *nevt;
{
   int wh, i, start, stop, beg = 0;
   char string[128], *array[50], days[10];
   EVENT evt;

   wh = wopen (4, 0, 21, 77, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Events ", TRIGHT, YELLOW|_BLUE);

   wprints (0, 0, LGREY|_BLACK, "  #  Title                    Days     Start   Stop    Lev   Try   Fail ");
   whline (1, 0, 76, 0, BLUE|_BLACK);

   gotoxy_ (24, 1);
   clreol_ ();
   prints (24, 1, LGREY|_BLACK, "-Move bar  ENTER-Select");
   prints (24, 1, YELLOW|_BLACK, "");
   prints (24, 14, YELLOW|_BLACK, "ENTER");

   i = 0;
   start = 0;
   lseek (fd, 0L, SEEK_SET);

   while (read (fd, (char *)&evt, sizeof (EVENT)) == sizeof (EVENT)) {
      if (memcmp ((char *)&evt, (char *)nevt, sizeof (EVENT)) == 0)
         beg = i;
      strcpy (days, "-------");
      if (evt.days & DAY_SUNDAY)
         days[0] = 'S';
      if (evt.days & DAY_MONDAY)
         days[1] = 'M';
      if (evt.days & DAY_TUESDAY)
         days[2] = 'T';
      if (evt.days & DAY_WEDNESDAY)
         days[3] = 'W';
      if (evt.days & DAY_THURSDAY)
         days[4] = 'T';
      if (evt.days & DAY_FRIDAY)
         days[5] = 'F';
      if (evt.days & DAY_SATURDAY)
         days[6] = 'S';
      start = evt.minute;
      stop = evt.minute + evt.length;
      sprintf (string, "  %-2d  %-22.22s  %s  %2d:%02d   %2d:%02d   %3d   %3d    %3d    ", i + 1, evt.cmd, days, start / 60, start % 60, stop / 60, stop % 60, evt.errlevel[0], evt.no_connect, evt.with_connect);
      array[i] = (char *)malloc (strlen (string) + 1);
      if (array[i] == NULL)
         break;
      strcpy (array[i++], string);
   }

   array[i] = NULL;

   i = wpickstr (7, 1, 20, 76, 5, LGREY|_BLACK, CYAN|_BLACK, BLUE|_LGREY, array, beg, NULL);

   if (i != -1) {
      lseek (fd, (long)sizeof (EVENT) * (long)i, SEEK_SET);
      read (fd, (char *)nevt, sizeof (EVENT));
   }

   wclose ();

   gotoxy_ (24, 1);
   clreol_ ();
   prints (24, 1, LGREY|_BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add New Event  L-List  D-Delete  C-Copy");
   prints (24, 1, YELLOW|_BLACK, "PgUp/PgDn");
   prints (24, 26, YELLOW|_BLACK, "E");
   prints (24, 34, YELLOW|_BLACK, "A");
   prints (24, 51, YELLOW|_BLACK, "L");
   prints (24, 59, YELLOW|_BLACK, "D");
   prints (24, 69, YELLOW|_BLACK, "C");
}

static void edit_single_event (evt)
EVENT *evt;
{
   int i = 1, wh1, m, wh, t1, t2;
   char string[128], *p;
   EVENT nevt;

   memcpy ((char *)&nevt, (char *)evt, sizeof (EVENT));

   gotoxy_ (24, 1);
   clreol_ ();
   prints (24, 1, LGREY|_BLACK, "ESC-Exit/Save  ENTER-Edit");
   prints (24, 1, YELLOW|_BLACK, "ESC");
   prints (24, 16, YELLOW|_BLACK, "ENTER");

   wh = wopen (3, 11, 21, 65, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Edit event ", TRIGHT, YELLOW|_BLUE);

continue_editing:
   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " Title            ", 0,  1, 0, NULL, 0, 0);
      wmenuitem ( 2,  1, " Days             ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " Start time       ", 0,  3, 0, NULL, 0, 0);
      wmenuitem ( 4,  1, " Stop time        ", 0,  4, 0, NULL, 0, 0);
      wmenuitem ( 5,  1, " Forced           ", 0,  5, 0, NULL, 0, 0);
      wmenuitem ( 6,  1, " Allow humans     ", 0,  6, 0, NULL, 0, 0);
      wmenuitem ( 7,  1, " Errorlevel       ", 0,  7, 0, NULL, 0, 0);
      wmenuitem ( 8,  1, " Clock adjustment ", 0,  8, 0, NULL, 0, 0);
      wmenuitem ( 9,  1, " Receive only     ", 0,  9, 0, NULL, 0, 0);
      wmenuitem (10,  1, " Ã Max try        ", 0, 10, 0, NULL, 0, 0);
      wmenuitem (11,  1, " Ã Max failed     ", 0, 11, 0, NULL, 0, 0);
      wmenuitem (12,  1, " À Retry delay    ", 0, 12, 0, NULL, 0, 0);
      wmenuitem (13,  1, " Forced poll      ", 0, 14, 0, NULL, 0, 0);
      wmenuitem (14,  1, " À Reserved node  ", 0, 15, 0, NULL, 0, 0);
      wmenuitem (15,  1, " Mail behavior    ", 0, 13, 0, NULL, 0, 0);

      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 20, CYAN|_BLACK, nevt.cmd);
      strcpy (string, "-------");
      if (nevt.days & DAY_SUNDAY)
         string[0] = 'S';
      if (nevt.days & DAY_MONDAY)
         string[1] = 'M';
      if (nevt.days & DAY_TUESDAY)
         string[2] = 'T';
      if (nevt.days & DAY_WEDNESDAY)
         string[3] = 'W';
      if (nevt.days & DAY_THURSDAY)
         string[4] = 'T';
      if (nevt.days & DAY_FRIDAY)
         string[5] = 'F';
      if (nevt.days & DAY_SATURDAY)
         string[6] = 'S';
      wprints (2, 20, CYAN|_BLACK, string);
      sprintf (string, "%02d:%02d", nevt.minute / 60, nevt.minute % 60);
      wprints (3, 20, CYAN|_BLACK, string);
      m = nevt.minute + nevt.length;
      sprintf (string, "%02d:%02d", m / 60, m % 60);
      wprints (4, 20, CYAN|_BLACK, string);
      if (nevt.behavior & MAT_FORCED)
         wprints (5, 20, CYAN|_BLACK, "Yes");
      else
         wprints (5, 20, CYAN|_BLACK, "No");
      if (nevt.behavior & MAT_BBS)
         wprints (6, 20, CYAN|_BLACK, "Yes");
      else
         wprints (6, 20, CYAN|_BLACK, "No");
      sprintf (string, "%d", nevt.errlevel[0]);
      wprints (7, 20, CYAN|_BLACK, string);
      if (nevt.behavior & MAT_RESYNC)
         wprints (8, 20, CYAN|_BLACK, "Yes");
      else
         wprints (8, 20, CYAN|_BLACK, "No");
      if (nevt.behavior & MAT_NOOUT)
         wprints (9, 20, CYAN|_BLACK, "Yes");
      else
         wprints (9, 20, CYAN|_BLACK, "No");
      sprintf (string, "%d", nevt.no_connect);
      wprints (10, 20, CYAN|_BLACK, string);
      sprintf (string, "%d", nevt.with_connect);
      wprints (11, 20, CYAN|_BLACK, string);
      sprintf (string, "%d", nevt.wait_time);
      wprints (12, 20, CYAN|_BLACK, string);
      wprints (13, 20, CYAN|_BLACK, (nevt.echomail & ECHO_FORCECALL) ? "Yes" : "No");
      if (nevt.behavior & MAT_RESERV) {
         sprintf (string, "%d:%d/%d", nevt.res_zone, nevt.res_net, nevt.res_node);
         wprints (14, 20, CYAN|_BLACK, string);
      }

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            strcpy (string, nevt.cmd);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (1, 20, string, "??????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nevt.cmd, strbtrim (string));
            break;

         case 2:
            wh1 = wopen (4, 30, 14, 50, 1, LCYAN|_BLACK, CYAN|_BLACK);
            wactiv (wh1);
            wshadow (DGREY|_BLACK);
            wtitle (" Days ", TRIGHT, YELLOW|_BLUE);
            i = 1;

            do {
               stop_update ();
               wclear ();

               wmenubegc ();
               wmenuitem ( 1,  1, " Sunday    ", 0, 1, 0, NULL, 0, 0);
               wmenuitem ( 2,  1, " Monday    ", 0, 2, 0, NULL, 0, 0);
               wmenuitem ( 3,  1, " Tuesday   ", 0, 3, 0, NULL, 0, 0);
               wmenuitem ( 4,  1, " Wednesday ", 0, 4, 0, NULL, 0, 0);
               wmenuitem ( 5,  1, " Thursday  ", 0, 5, 0, NULL, 0, 0);
               wmenuitem ( 6,  1, " Friday    ", 0, 6, 0, NULL, 0, 0);
               wmenuitem ( 7,  1, " Saturday  ", 0, 7, 0, NULL, 0, 0);
               wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

               if (nevt.days & DAY_SUNDAY)
                  wprints (1, 13, CYAN|_BLACK, "Yes");
               else
                  wprints (1, 13, CYAN|_BLACK, "No");
               if (nevt.days & DAY_MONDAY)
                  wprints (2, 13, CYAN|_BLACK, "Yes");
               else
                  wprints (2, 13, CYAN|_BLACK, "No");
               if (nevt.days & DAY_TUESDAY)
                  wprints (3, 13, CYAN|_BLACK, "Yes");
               else
                  wprints (3, 13, CYAN|_BLACK, "No");
               if (nevt.days & DAY_WEDNESDAY)
                  wprints (4, 13, CYAN|_BLACK, "Yes");
               else
                  wprints (4, 13, CYAN|_BLACK, "No");
               if (nevt.days & DAY_THURSDAY)
                  wprints (5, 13, CYAN|_BLACK, "Yes");
               else
                  wprints (5, 13, CYAN|_BLACK, "No");
               if (nevt.days & DAY_FRIDAY)
                  wprints (6, 13, CYAN|_BLACK, "Yes");
               else
                  wprints (6, 13, CYAN|_BLACK, "No");
               if (nevt.days & DAY_SATURDAY)
                  wprints (7, 13, CYAN|_BLACK, "Yes");
               else
                  wprints (7, 13, CYAN|_BLACK, "No");

               start_update ();
               i = wmenuget ();

               switch (i) {
                  case 1:
                     nevt.days ^= DAY_SUNDAY;
                     break;

                  case 2:
                     nevt.days ^= DAY_MONDAY;
                     break;

                  case 3:
                     nevt.days ^= DAY_TUESDAY;
                     break;

                  case 4:
                     nevt.days ^= DAY_WEDNESDAY;
                     break;

                  case 5:
                     nevt.days ^= DAY_THURSDAY;
                     break;

                  case 6:
                     nevt.days ^= DAY_FRIDAY;
                     break;

                  case 7:
                     nevt.days ^= DAY_SATURDAY;
                     break;
               }

            } while (i != -1);

            wclose ();

            i = 2;
            break;

         case 3:
            sprintf (string, "%02d:%02d", nevt.minute / 60, nevt.minute % 60);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 20, string, "??????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               sscanf (string, "%d:%d", &t1, &t2);
               nevt.minute = t1 * 60 + t2;
            }
            break;

         case 4:
            m = nevt.minute + nevt.length;
            sprintf (string, "%02d:%02d", m / 60, m % 60);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 20, string, "??????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               sscanf (string, "%d:%d", &t1, &t2);
               m = t1 * 60 + t2;
               nevt.length = m - nevt.minute;
            }
            break;

         case 5:
            nevt.behavior ^= MAT_FORCED;
            break;

         case 6:
            nevt.behavior ^= MAT_BBS;
            break;

         case 7:
            sprintf (string, "%d", nevt.errlevel[0]);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (7, 20, string, "??????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nevt.errlevel[0] = atoi (string) % 256;
            break;

         case 8:
            nevt.behavior ^= MAT_RESYNC;
            break;

         case 9:
            nevt.behavior ^= MAT_NOOUT;
            break;

         case 10:
            sprintf (string, "%d", nevt.no_connect);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (10, 20, string, "??????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nevt.no_connect = atoi (string);
            break;

         case 11:
            sprintf (string, "%d", nevt.with_connect);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (11, 20, string, "??????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nevt.with_connect = atoi (string);
            break;

         case 12:
            sprintf (string, "%d", nevt.wait_time);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (12, 20, string, "??????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nevt.wait_time = atoi (string);
            break;

         case 13:
            wh1 = wopen (4, 26, 21, 74, 1, LCYAN|_BLACK, CYAN|_BLACK);
            wactiv (wh1);
            wshadow (DGREY|_BLACK);
            wtitle (" Mail behavior ", TRIGHT, YELLOW|_BLUE);
            i = 1;

            do {
               stop_update ();
               wclear ();

               wmenubegc ();
               wmenuitem ( 1,  1, " Allow file requests         ", 0,  1, 0, NULL, 0, 0);
               wmenuitem ( 2,  1, " Make file requests          ", 0,  2, 0, NULL, 0, 0);
               wmenuitem ( 3,  1, " Send to CM systems only     ", 0,  3, 0, NULL, 0, 0);
               wmenuitem ( 4,  1, " Send to non-CM systems only ", 0,  4, 0, NULL, 0, 0);
               wmenuitem ( 5,  1, " Import normal mail          ", 0,  9, 0, NULL, 0, 0);
               wmenuitem ( 6,  1, " Import know mail            ", 0, 10, 0, NULL, 0, 0);
               wmenuitem ( 7,  1, " Import prot mail            ", 0, 11, 0, NULL, 0, 0);
               wmenuitem ( 8,  1, " Export mail                 ", 0,  6, 0, NULL, 0, 0);
               wmenuitem ( 9,  1, " Import at start of event    ", 0,  7, 0, NULL, 0, 0);
               wmenuitem (10,  1, " Export at start of event    ", 0,  8, 0, NULL, 0, 0);
               wmenuitem (11,  1, " Dynamic                     ", 0, 13, 0, NULL, 0, 0);
               wmenuitem (12,  1, " Route TAG                   ", 0, 14, 0, NULL, 0, 0);
               wmenuitem (13,  1, " AfterMail exit errorlevel   ", 0, 15, 0, NULL, 0, 0);
               wmenuitem (14,  1, " Process TIC files           ", 0, 16, 0, NULL, 0, 0);
               wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

               if (nevt.behavior & MAT_NOREQ)
                  wprints (1, 31, CYAN|_BLACK, "No");
               else
                  wprints (1, 31, CYAN|_BLACK, "Yes");
               if (nevt.behavior & MAT_NOOUTREQ)
                  wprints (2, 31, CYAN|_BLACK, "No");
               else
                  wprints (2, 31, CYAN|_BLACK, "Yes");
               if (nevt.behavior & MAT_CM)
                  wprints (3, 31, CYAN|_BLACK, "Yes");
               else
                  wprints (3, 31, CYAN|_BLACK, "No");
               if (nevt.behavior & MAT_NOCM)
                  wprints (4, 31, CYAN|_BLACK, "Yes");
               else
                  wprints (4, 31, CYAN|_BLACK, "No");
               if (nevt.echomail & ECHO_NORMAL)
                  wprints (5, 31, CYAN|_BLACK, "Yes");
               else
                  wprints (5, 31, CYAN|_BLACK, "No");
               if (nevt.echomail & ECHO_KNOW)
                  wprints (6, 31, CYAN|_BLACK, "Yes");
               else
                  wprints (6, 31, CYAN|_BLACK, "No");
               if (nevt.echomail & ECHO_PROT)
                  wprints (7, 31, CYAN|_BLACK, "Yes");
               else
                  wprints (7, 31, CYAN|_BLACK, "No");
               if (nevt.echomail & ECHO_EXPORT)
                  wprints (8, 31, CYAN|_BLACK, "Yes");
               else
                  wprints (8, 31, CYAN|_BLACK, "No");
               if (nevt.echomail & ECHO_STARTIMPORT)
                  wprints (9, 31, CYAN|_BLACK, "Yes");
               else
                  wprints (9, 31, CYAN|_BLACK, "No");
               if (nevt.echomail & ECHO_STARTEXPORT)
                  wprints (10, 31, CYAN|_BLACK, "Yes");
               else
                  wprints (10, 31, CYAN|_BLACK, "No");
               if (nevt.behavior & MAT_DYNAM)
                  wprints (11, 31, CYAN|_BLACK, "Yes");
               else
                  wprints (11, 31, CYAN|_BLACK, "No");
               wprints (12, 31, CYAN|_BLACK, nevt.route_tag);
               sprintf (string, "%d", nevt.errlevel[2]);
               wprints (13, 31, CYAN|_BLACK, string);
               wprints (14, 31, CYAN|_BLACK, (nevt.echomail & ECHO_RESERVED4) ? "Yes" : "No");

               start_update ();
               i = wmenuget ();

               switch (i) {
                  case 1:
                     nevt.behavior ^= MAT_NOREQ;
                     break;

                  case 2:
                     nevt.behavior ^= MAT_NOOUTREQ;
                     break;

                  case 3:
                     nevt.behavior ^= MAT_CM;
                     break;

                  case 4:
                     nevt.behavior ^= MAT_NOCM;
                     break;

                  case 6:
                     nevt.echomail ^= ECHO_EXPORT;
                     break;

                  case 7:
                     nevt.echomail ^= ECHO_STARTIMPORT;
                     break;

                  case 8:
                     nevt.echomail ^= ECHO_STARTEXPORT;
                     break;

                  case 9:
                     nevt.echomail ^= ECHO_NORMAL;
                     break;

                  case 10:
                     nevt.echomail ^= ECHO_KNOW;
                     break;

                  case 11:
                     nevt.echomail ^= ECHO_PROT;
                     break;

                  case 13:
                     nevt.behavior ^= MAT_DYNAM;
                     break;

                  case 14:
                     strcpy (string, nevt.route_tag);
                     winpbeg (BLUE|_GREEN, BLUE|_GREEN);
                     winpdef (12, 31, string, "???????????????", 0, 2, NULL, 0);
                     if (winpread () != W_ESCPRESS)
                        strcpy (nevt.route_tag, strupr (strbtrim (string)));
                     break;

                  case 15:
                     sprintf (string, "%d", nevt.errlevel[2]);
                     winpbeg (BLUE|_GREEN, BLUE|_GREEN);
                     winpdef (13, 31, string, "???", 0, 2, NULL, 0);
                     if (winpread () != W_ESCPRESS)
                        nevt.errlevel[2] = atoi (string) % 256;
                     break;

                  case 16:
                     nevt.echomail ^= ECHO_RESERVED4;
                     break;
               }

               hidecur ();
            } while (i != -1);

            wclose ();

            i = 13;
            break;

         case 14:
            nevt.echomail ^= ECHO_FORCECALL;
            break;

         case 15:
            if (nevt.behavior & MAT_RESERV)
               sprintf (string, "%d:%d/%d", nevt.res_zone, nevt.res_net, nevt.res_node);
            else
               strcpy (string, "");
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (14, 20, string, "?????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               p = strtok (strbtrim (string), " ");
               if (p == NULL || !strlen (p))
						nevt.behavior &= ~MAT_RESERV;
					else {
						int dummy;
						nevt.behavior |= MAT_RESERV;
						parse_netnode(p,(int *)&nevt.res_zone, (int *)&nevt.res_net, (int *)&nevt.res_node,(int *)&dummy);
//                sscanf (p, "%d:%d/%d", &nevt.res_zone, &nevt.res_net, &nevt.res_node);
					}

            }
            break;
      }

      hidecur ();
   } while (i != -1);

   if (memcmp ((char *)&nevt, (char *)evt, sizeof (EVENT))) {
      wh1 = wopen (10, 25, 14, 54, 0, BLACK|_LGREY, BLACK|_LGREY);
      wactiv (wh1);
      wshadow (DGREY|_BLACK);

      wcenters (1, BLACK|_LGREY, "Save changes (Y/n) ?  ");

      strcpy (string, "Y");
      winpbeg (BLACK|_LGREY, BLACK|_LGREY);
      winpdef (1, 24, string, "?", 0, 2, NULL, 0);

      i = winpread ();
      wclose ();
      hidecur ();

      if (i == W_ESCPRESS)
         goto continue_editing;

      if (toupper (string[0]) == 'Y') {
         nevt.last_ran = 0;
         memcpy ((char *)evt, (char *)&nevt, sizeof (EVENT));
      }
   }

   wclose ();

   gotoxy_ (24, 1);
   clreol_ ();
   prints (24, 1, LGREY|_BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add New Event  L-List  D-Delete  C-Copy");
   prints (24, 1, YELLOW|_BLACK, "PgUp/PgDn");
   prints (24, 26, YELLOW|_BLACK, "E");
   prints (24, 34, YELLOW|_BLACK, "A");
   prints (24, 51, YELLOW|_BLACK, "L");
   prints (24, 59, YELLOW|_BLACK, "D");
   prints (24, 69, YELLOW|_BLACK, "C");
}

static void mailproc_linehelp (void)
{
   char *str = "";
   struct _menu_t *mt;

   mt = wmenumcurr ();
   if (mt == NULL)
      return;

   switch (mt->citem->tagid) {
      case 1:
         str = "Method of packing netmail and echomail";
         break;
      case 2:
         str = "Enable / disable the import of the mail for the Sysop to a separate area";
         break;
      case 3:
         str = "Directory to export to the mail addressed to the Sysop";
         break;
      case 4:
         str = "Enable / disable the replace or link of the tear line on local message";
         break;
      case 5:
         str = "Enable / disable the flashing MAIL flag on the bottom right corner";
         break;
      case 6:
         str = "Always import netmail messages without text";
         break;
      case 7:
         str = "Text to put as tearline when replaced";
         break;
      case 8:
         str = "Forces the insertion of the ^aINTL kludge in netmail messages";
         break;
      case 9:
         str = "Exports Internet-type message areas";
         break;
   }

   clear_window ();
   prints (24, 1, LGREY|_BLACK, str);
}

void mail_processing ()
{
   int wh, i = 1;
   char string[128];

   wh = wopen (7, 10, 19, 74, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Mail Processing ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " Method             ", 0,  1, 0, NULL, 0, 0);
      wmenuiba (mailproc_linehelp, clear_window);
      wmenuitem ( 2,  1, " Save Sysop mail    ", 0,  2, 0, NULL, 0, 0);
      wmenuiba (mailproc_linehelp, clear_window);
      wmenuitem ( 3,  1, " ÃÄ Sysop Mail Path ", 0,  3, 0, NULL, 0, 0);
      wmenuiba (mailproc_linehelp, clear_window);
      wmenuitem ( 4,  1, " ÀÄ Flashing flag   ", 0,  5, 0, NULL, 0, 0);
      wmenuiba (mailproc_linehelp, clear_window);
      wmenuitem ( 5,  1, " Replace tear line  ", 0,  4, 0, NULL, 0, 0);
      wmenuiba (mailproc_linehelp, clear_window);
      wmenuitem ( 6,  1, " ÀÄ Tear line       ", 0,  7, 0, NULL, 0, 0);
      wmenuiba (mailproc_linehelp, clear_window);
      wmenuitem ( 7,  1, " Import empty msgs. ", 0,  6, 0, NULL, 0, 0);
      wmenuiba (mailproc_linehelp, clear_window);
      wmenuitem ( 8,  1, " Force INTL line    ", 0,  8, 0, NULL, 0, 0);
      wmenuiba (mailproc_linehelp, clear_window);
      wmenuitem ( 9,  1, " Export Internet    ", 0,  9, 0, NULL, 0, 0);
      wmenuiba (mailproc_linehelp, clear_window);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 22, CYAN|_BLACK, config.mail_method ? "Separate netmail" : "Netmail and echomail together");
      wprints (2, 22, CYAN|_BLACK, config.save_my_mail ? "Yes" : "No");
      wprints (3, 22, CYAN|_BLACK, config.my_mail);
      wprints (4, 22, CYAN|_BLACK, config.mymail_flash ? "Yes" : "No");
      if (config.replace_tear == 0)
         wprints (5, 22, CYAN|_BLACK, "No");
      else if (config.replace_tear == 1)
         wprints (5, 22, CYAN|_BLACK, "Link (unlimited)");
      else if (config.replace_tear == 2)
         wprints (5, 22, CYAN|_BLACK, "Link (limit to 35 char.)");
      else if (config.replace_tear == 3)
         wprints (5, 22, CYAN|_BLACK, "Yes");
      wprints (6, 22, CYAN|_BLACK, config.tearline);
      wprints (7, 22, CYAN|_BLACK, config.keep_empty ? "Yes" : "No");
      wprints (8, 22, CYAN|_BLACK, config.force_intl ? "Yes" : "No");
      wprints (9, 22, CYAN|_BLACK, config.export_internet ? "Yes" : "No");

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            config.mail_method ^= 1;
            break;

         case 2:
            config.save_my_mail ^= 1;
            break;

         case 3:
            strcpy (string, config.my_mail);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 22, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (config.my_mail, strbtrim (string));
               if (config.my_mail[0] && config.my_mail[strlen (config.my_mail) - 1] != '\\')
                  strcat (config.my_mail, "\\");
               create_path (config.my_mail);
            }
            break;

         case 4:
            if (config.replace_tear == 0)
               config.replace_tear = 1;
            else if (config.replace_tear == 1)
               config.replace_tear = 2;
            else if (config.replace_tear == 2)
               config.replace_tear = 3;
            else if (config.replace_tear == 3)
               config.replace_tear = 0;
            break;

         case 5:
            config.mymail_flash ^= 1;
            break;

         case 6:
            config.keep_empty ^= 1;
            break;

         case 7:
            strcpy (string, config.tearline);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (6, 22, string, "???????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.tearline, strbtrim (string));
            break;

         case 8:
            config.force_intl ^= 1;
            break;

         case 9:
            config.export_internet ^= 1;
            break;
      }

      hidecur ();

   } while (i != -1);

   wclose ();
}

static void edit_single_protocol (PROTOCOL *prot)
{
   int i = 1, wh1;
   char string[128];
   PROTOCOL nprot;

   memcpy ((char *)&nprot, (char *)prot, sizeof (PROTOCOL));

   gotoxy_ (24, 1);
   clreol_ ();
   prints (24, 1, LGREY|_BLACK, "ESC-Exit/Save  ENTER-Edit");
   prints (24, 1, YELLOW|_BLACK, "ESC");
   prints (24, 16, YELLOW|_BLACK, "ENTER");

continue_editing:
   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " Active           ", 0,  1, 0, NULL, 0, 0);
      wmenuitem ( 2,  1, " Name             ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " Hotkey           ", 0,  3, 0, NULL, 0, 0);
      wmenuitem ( 4,  1, " Download command ", 0,  4, 0, NULL, 0, 0);
      wmenuitem ( 5,  1, " Upload command   ", 0,  5, 0, NULL, 0, 0);
      wmenuitem ( 6,  1, " Log file name    ", 0,  6, 0, NULL, 0, 0);
      wmenuitem ( 7,  1, " Control file     ", 0,  7, 0, NULL, 0, 0);
      wmenuitem ( 8,  1, " Download string  ", 0,  8, 0, NULL, 0, 0);
      wmenuitem ( 9,  1, " Upload string    ", 0,  9, 0, NULL, 0, 0);
      wmenuitem (10,  1, " Download keyword ", 0, 10, 0, NULL, 0, 0);
      wmenuitem (11,  1, " Upload keyword   ", 0, 11, 0, NULL, 0, 0);
      wmenuitem (12,  1, " Filename word    ", 0, 12, 0, NULL, 0, 0);
      wmenuitem (13,  1, " Size word        ", 0, 13, 0, NULL, 0, 0);
      wmenuitem (14,  1, " CPS word         ", 0, 14, 0, NULL, 0, 0);
      wmenuitem (12, 31, " Batch protocol   ", 0, 15, 0, NULL, 0, 0);
      wmenuitem (13, 31, " Disable FOSSIL   ", 0, 16, 0, NULL, 0, 0);
      wmenuitem (14, 31, " Change to UL dir ", 0, 17, 0, NULL, 0, 0);

      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 20, CYAN|_BLACK, nprot.active ? "Yes" : "No");
      wprints (2, 20, CYAN|_BLACK, nprot.name);
      sprintf (string, "%c", nprot.hotkey);
      wprints (3, 20, CYAN|_BLACK, string);
      wprints (4, 20, CYAN|_BLACK, nprot.dl_command);
      wprints (5, 20, CYAN|_BLACK, nprot.ul_command);
      wprints (6, 20, CYAN|_BLACK, nprot.log_name);
      wprints (7, 20, CYAN|_BLACK, nprot.ctl_name);
      wprints (8, 20, CYAN|_BLACK, nprot.dl_ctl_string);
      wprints (9, 20, CYAN|_BLACK, nprot.ul_ctl_string);
      wprints (10, 20, CYAN|_BLACK, nprot.dl_keyword);
      wprints (11, 20, CYAN|_BLACK, nprot.ul_keyword);
      sprintf (string, "%d", nprot.filename);
      wprints (12, 20, CYAN|_BLACK, string);
      sprintf (string, "%d", nprot.size);
      wprints (13, 20, CYAN|_BLACK, string);
      sprintf (string, "%d", nprot.cps);
      wprints (14, 20, CYAN|_BLACK, string);
      wprints (12, 50, CYAN|_BLACK, nprot.batch ? "Yes" : "No");
      wprints (13, 50, CYAN|_BLACK, nprot.disable_fossil ? "Yes" : "No");
      wprints (14, 50, CYAN|_BLACK, nprot.change_to_dl ? "Yes" : "No");
      start_update ();

      i = wmenuget ();

      switch (i) {
         case 1:
            nprot.active ^= 1;
            break;

         case 2:
            strcpy (string, nprot.name);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (2, 20, string, "?????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nprot.name, strbtrim (string));
            break;

         case 3:
            sprintf (string, "%c", nprot.hotkey);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 20, string, "?", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nprot.hotkey = toupper (string[0]);
            break;

         case 4:
            strcpy (string, nprot.dl_command);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 20, string, "???????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nprot.dl_command, strbtrim (string));
            break;

         case 5:
            strcpy (string, nprot.ul_command);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (5, 20, string, "???????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nprot.ul_command, strbtrim (string));
            break;

         case 6:
            strcpy (string, nprot.log_name);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (6, 20, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nprot.log_name, strbtrim (string));
            break;

         case 7:
            strcpy (string, nprot.ctl_name);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (7, 20, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nprot.ctl_name, strbtrim (string));
            break;

         case 8:
            strcpy (string, nprot.dl_ctl_string);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (8, 20, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nprot.dl_ctl_string, strbtrim (string));
            break;

         case 9:
            strcpy (string, nprot.ul_ctl_string);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (9, 20, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nprot.ul_ctl_string, strbtrim (string));
            break;

         case 10:
            strcpy (string, nprot.dl_keyword);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (10, 20, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nprot.dl_keyword, strbtrim (string));
            break;

         case 11:
            strcpy (string, nprot.ul_keyword);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (11, 20, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nprot.ul_keyword, strbtrim (string));
            break;

         case 12:
            sprintf (string, "%d", nprot.filename);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (12, 20, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nprot.filename = atoi (string);
            break;

         case 13:
            sprintf (string, "%d", nprot.size);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (13, 20, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nprot.size = atoi (string);
            break;

         case 14:
            sprintf (string, "%d", nprot.cps);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (14, 20, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nprot.cps = atoi (string);
            break;

         case 15:
            nprot.batch ^= 1;
            break;

         case 16:
            nprot.disable_fossil ^= 1;
            break;

         case 17:
            nprot.change_to_dl ^= 1;
            break;
      }

      hidecur ();
   } while (i != -1);

   if (memcmp ((char *)&nprot, (char *)prot, sizeof (PROTOCOL))) {
      wh1 = wopen (10, 25, 14, 54, 0, BLACK|_LGREY, BLACK|_LGREY);
      wactiv (wh1);
      wshadow (DGREY|_BLACK);

      wcenters (1, BLACK|_LGREY, "Save changes (Y/n) ?  ");

      strcpy (string, "Y");
      winpbeg (BLACK|_LGREY, BLACK|_LGREY);
      winpdef (1, 24, string, "?", 0, 2, NULL, 0);

      i = winpread ();
      wclose ();
      hidecur ();

      if (i == W_ESCPRESS)
         goto continue_editing;

      if (toupper (string[0]) == 'Y')
         memcpy ((char *)prot, (char *)&nprot, sizeof (PROTOCOL));
   }

   gotoxy_ (24, 1);
   clreol_ ();
   prints (24, 1, LGREY|_BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add New Protocol D-Delete");
   prints (24, 1, YELLOW|_BLACK, "PgUp/PgDn");
   prints (24, 26, YELLOW|_BLACK, "E");
   prints (24, 34, YELLOW|_BLACK, "A");
   prints (24, 53, YELLOW|_BLACK, "D");
}

void bbs_protocol (void)
{
   int fdi, fd, wh, wh1, i = 1;
   char filename[80], string[128], readed;
   long pos;
   PROTOCOL nprot, bprot;

   sprintf (string, "%sPROTOCOL.DAT", config.sys_path);
   fd = open (string, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
   if (fd == -1)
      return;

   if (!read (fd, (char *)&nprot, sizeof (PROTOCOL))) {
      memset ((char *)&nprot, 0, sizeof (PROTOCOL));
      readed = 0;
   }
   else
      readed = 1;

   gotoxy_ (24, 1);
   clreol_ ();
   prints (24, 1, LGREY|_BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add New Protocol D-Delete");
   prints (24, 1, YELLOW|_BLACK, "PgUp/PgDn");
   prints (24, 26, YELLOW|_BLACK, "E");
   prints (24, 34, YELLOW|_BLACK, "A");
   prints (24, 53, YELLOW|_BLACK, "D");

   wh = wopen (3, 5, 20, 74, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Ext. Protocols ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wprints ( 1, 1, LGREY|_BLACK, " Active           ");
      wprints ( 2, 1, LGREY|_BLACK, " Name             ");
      wprints ( 3, 1, LGREY|_BLACK, " Hotkey           ");
      wprints ( 4, 1, LGREY|_BLACK, " Download command ");
      wprints ( 5, 1, LGREY|_BLACK, " Upload command   ");
      wprints ( 6, 1, LGREY|_BLACK, " Log file name    ");
      wprints ( 7, 1, LGREY|_BLACK, " Control file     ");
      wprints ( 8, 1, LGREY|_BLACK, " Download string  ");
      wprints ( 9, 1, LGREY|_BLACK, " Upload string    ");
      wprints (10, 1, LGREY|_BLACK, " Download keyword ");
      wprints (11, 1, LGREY|_BLACK, " Upload keyword   ");
      wprints (12, 1, LGREY|_BLACK, " Filename word    ");
      wprints (13, 1, LGREY|_BLACK, " Size word        ");
      wprints (14, 1, LGREY|_BLACK, " CPS word         ");
      wprints (12, 31, LGREY|_BLACK, " Batch protocol   ");
      wprints (13, 31, LGREY|_BLACK, " Disable FOSSIL   ");
      wprints (14, 31, LGREY|_BLACK, " Change to UL dir ");

      if (readed) {
         wprints (1, 20, CYAN|_BLACK, nprot.active ? "Yes" : "No");
         wprints (2, 20, CYAN|_BLACK, nprot.name);
         sprintf (string, "%c", nprot.hotkey);
         wprints (3, 20, CYAN|_BLACK, string);
         wprints (4, 20, CYAN|_BLACK, nprot.dl_command);
         wprints (5, 20, CYAN|_BLACK, nprot.ul_command);
         wprints (6, 20, CYAN|_BLACK, nprot.log_name);
         wprints (7, 20, CYAN|_BLACK, nprot.ctl_name);
         wprints (8, 20, CYAN|_BLACK, nprot.dl_ctl_string);
         wprints (9, 20, CYAN|_BLACK, nprot.ul_ctl_string);
         wprints (10, 20, CYAN|_BLACK, nprot.dl_keyword);
         wprints (11, 20, CYAN|_BLACK, nprot.ul_keyword);
         sprintf (string, "%d", nprot.filename);
         wprints (12, 20, CYAN|_BLACK, string);
         sprintf (string, "%d", nprot.size);
         wprints (13, 20, CYAN|_BLACK, string);
         sprintf (string, "%d", nprot.cps);
         wprints (14, 20, CYAN|_BLACK, string);
         wprints (12, 50, CYAN|_BLACK, nprot.batch ? "Yes" : "No");
         wprints (13, 50, CYAN|_BLACK, nprot.disable_fossil ? "Yes" : "No");
         wprints (14, 50, CYAN|_BLACK, nprot.change_to_dl ? "Yes" : "No");
      }
      start_update ();

      i = getxch ();
      if ( (i & 0xFF) != 0 )
         i &= 0xFF;

      switch (i) {
         // PgDn
         case 0x5100:
            if (readed)
               read (fd, (char *)&nprot, sizeof (PROTOCOL));
            break;

         // PgUp
         case 0x4900:
            if (readed) {
               if (tell (fd) > sizeof (PROTOCOL)) {
                  lseek (fd, -2L * sizeof (PROTOCOL), SEEK_CUR);
                  read (fd, (char *)&nprot, sizeof (PROTOCOL));
               }
            }
            break;

         // E Edit
         case 'E':
         case 'e':
            if (readed) {
               edit_single_protocol (&nprot);
               lseek (fd, -1L * sizeof (PROTOCOL), SEEK_CUR);
               write (fd, (char *)&nprot, sizeof (PROTOCOL));
            }
            break;

         // A Add
         case 'A':
         case 'a':
            memset ((char *)&nprot, 0, sizeof (PROTOCOL));
            edit_single_protocol (&nprot);

            if (nprot.name[0]) {
               lseek (fd, 0L, SEEK_END);
               write (fd, (char *)&nprot, sizeof (PROTOCOL));
            }

            lseek (fd, -1L * sizeof (PROTOCOL), SEEK_CUR);
            readed = 0;
            if (read (fd, (char *)&nprot, sizeof (PROTOCOL)) == sizeof (PROTOCOL))
               readed = 1;
            break;

         // D Delete
         case 'D':
         case 'd':
            wh1 = wopen (10, 25, 14, 54, 0, BLACK|_LGREY, BLACK|_LGREY);
            wactiv (wh1);
            wshadow (DGREY|_BLACK);

            wcenters (1, BLACK|_LGREY, "Are you sure (Y/n) ?  ");

            strcpy (string, "Y");
            winpbeg (BLACK|_LGREY, BLACK|_LGREY);
            winpdef (1, 24, string, "?", 0, 2, NULL, 0);

            i = winpread ();
            wclose ();
            hidecur ();

            if (i == W_ESCPRESS)
               break;

            if (toupper (string[0]) == 'Y') {
               pos = tell (fd) - (long)sizeof (PROTOCOL);
               close (fd);

               update_message ();

               sprintf (filename, "%sPROTOCOL.BAK", config.sys_path);
               while ((fdi = sh_open (filename, SH_DENYRW, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE)) == -1)
                  ;
               sprintf (string, "%sPROTOCOL.DAT", config.sys_path);
               while ((fd = sh_open (string, SH_DENYRD, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE)) == -1)
                  ;

               while (read (fd, (char *)&bprot, sizeof (PROTOCOL)) == sizeof (PROTOCOL)) {
                  if (memcmp (&bprot, &nprot, sizeof (PROTOCOL)))
                     write (fdi, (char *)&bprot, sizeof (PROTOCOL));
               }

               lseek (fd, 0L, SEEK_SET);
               chsize (fd, 0L);
               lseek (fdi, 0L, SEEK_SET);

               while (read (fdi, (char *)&bprot, sizeof (PROTOCOL)) == sizeof (PROTOCOL))
                  write (fd, (char *)&bprot, sizeof (PROTOCOL));

               close (fd);
               close (fdi);

               unlink (filename);

               sprintf (string, "%sPROTOCOL.DAT", config.sys_path);
               while ((fd = sh_open (string, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE)) == -1)
                  ;
               if (lseek (fd, pos, SEEK_SET) == -1 || pos == filelength (fd)) {
                  lseek (fd, -1L * sizeof (PROTOCOL), SEEK_END);
                  pos = tell (fd);
               }
               read (fd, (char *)&nprot, sizeof (PROTOCOL));

               wclose ();
            }
            break;

         // ESC Exit
         case 0x1B:
            i = -1;
            break;
      }

   } while (i != -1);

   close (fd);

   wclose ();
   gotoxy_ (24, 1);
   clreol_ ();
}

