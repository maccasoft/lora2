
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
#include <dos.h>
#include <conio.h>
#include <time.h>
#include <string.h>
#include <alloc.h>
#include <dir.h>
#include <stdlib.h>
#include <process.h>
#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

#ifdef __OS2__
#define INCL_DOSPROCESS
#include <os2.h>
#endif

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlkey.h>
#include <cxl\cxlstr.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "exec.h"
#include "zmodem.h"

typedef struct {
   char name[36];
   char location[30];
   char phone[30];
   char password[20];
   char download[40];
   char capture[40];
   word speed;
   bit  iemsi        :1;
   bit  local_echo   :1;
   bit  strip_high   :1;
   bit  open_capture :1;
   char filler[256];
} DIALREC;

#define BITS_7          0x02

extern word serial_no;
extern char *VNUM, serial_id[3];

#ifdef __OS2__
void VioUpdate (void);
#endif

int spawn_program (int swapout, char *outstring);
void idle_system (void);
void clown_clear (void);
int get_emsi_field (char *);
void m_print2(char *format, ...);
void general_external_protocol (char *name, char *path, int id, int global, int dl);

static FILE *open_capture (char *predefined);
static void capture_closed (void);
static void close_capture (FILE *fp_cap);
static void keyboard_loop (void);
static void pull_down_menu (void);
static void set_selection (void);
static void translate_ansibbs (void);
static void translate_avatar (void);
static int  exit_confirm (void);
static void terminal_select_file (char *title, int upload, DIALREC *dialrec);
static void terminal_iemsi_handshake (char *password);
static void dial_entry (DIALREC *dialrec);
static int  dialing_directory (DIALREC *dialrec);
static void translate_phone (char *phone);

static char interpoint = ':', expand_cr = 0, local_echo = 0, doorway = 0;
static int last_sel = -1, vx, vy, ansi_attr;
static long online_elapsed = 0L, clocks = 0L;

void update_statusline (void)
{
   char string[90];

   if (doorway)
      return;

   wactiv (status);
   sprintf (string, "COM%d \263 %6lu %s", com_port + 1, rate, "N81");
   wprints (0, 23, LGREY|_BLUE, string);
   wactiv (mainview);
}

void change_baud (void)
{
   int m;

   hidecur ();

   wopen (6, 52, 17, 62, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wshadow (DGREY|_BLACK);
   wtitle (" Speed ", TRIGHT, YELLOW|_BLUE);

   wmenubegc ();
   wmenuitem (1, 1, "   300 ", 0,   300, 0, NULL, 0, 0);
   wmenuitem (2, 1, "  1200 ", 0,  1200, 0, NULL, 0, 0);
   wmenuitem (3, 1, "  2400 ", 0,  2400, 0, NULL, 0, 0);
   wmenuitem (4, 1, "  4800 ", 0,  4800, 0, NULL, 0, 0);
   wmenuitem (5, 1, "  9600 ", 0,  9600, 0, NULL, 0, 0);
   wmenuitem (6, 1, " 19200 ", 0, 19200, 0, NULL, 0, 0);
   wmenuitem (7, 1, " 38400 ", 0, 38400U, 0, NULL, 0, 0);
   wmenuitem (8, 1, " 57600 ", 0, 57600U, 0, NULL, 0, 0);
   wmenuend ((unsigned)rate, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

   m = wmenuget ();
   wclose ();

   if (m != -1) {
      rate = m;
      com_baud (rate);
      update_statusline ();
   }

   showcur ();
}

int MODEM_AVAILABLE (void)
{
   long t;

   if (CHAR_AVAIL ())
      return (CHAR_AVAIL ());

   t = timerset (10);
   while (!timeup (t)) {
      if (CHAR_AVAIL ())
         return (CHAR_AVAIL ());
   }

   return (CHAR_AVAIL ());
}

static long get_phone_cost (char *phone, long online)
{
   int fd, dcost, dtime, cost_first, time_first, ora, wd, c1, c2, c3, c4, i;
   char filename[80], rep, deftrasl[60];
   long cost, tempo;
   struct tm *tim;
   ACCOUNT ai;

   dcost = dtime = cost_first = time_first = 0;
   c1 = c2 = c3 = c4 = 0;
   rep = 0;
   deftrasl[0] = '\0';

   sprintf (filename, "%sCOST.DAT", config->net_info);
   fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);

   while (read (fd, (char *)&ai, sizeof (ACCOUNT)) == sizeof (ACCOUNT)) {
      if (!strncmp (phone, ai.search, strlen (ai.search))) {
         tempo = time (NULL);
         tim = localtime (&tempo);
         ora = tim->tm_hour * 60 + tim->tm_sec;
         wd = 1 << tim->tm_wday;

         for (i = 0; i < MAXCOST; i++)
            if ((ai.cost[i].days & wd)) {
               if (ai.cost[i].start < ai.cost[i].stop) {
                  if (ora >= ai.cost[i].start && ora <= ai.cost[i].stop) {
                     dcost = ai.cost[i].cost;
                     dtime = ai.cost[i].time;
                     cost_first = ai.cost[i].cost_first;
                     time_first = ai.cost[i].time_first;
                     break;
                  }
               }
               else {
                  if (ora >= ai.cost[i].start && ora <= 1439) {
                     dcost = ai.cost[i].cost;
                     dtime = ai.cost[i].time;
                     cost_first = ai.cost[i].cost_first;
                     time_first = ai.cost[i].time_first;
                     break;
                  }
                  if (ora >= 0 && ora <= ai.cost[i].stop) {
                     dcost = ai.cost[i].cost;
                     dtime = ai.cost[i].time;
                     cost_first = ai.cost[i].cost_first;
                     time_first = ai.cost[i].time_first;
                     break;
                  }
               }
            }

         rep = 1;
         break;
      }
      else if (!strcmp (ai.search, "/")) {
         strcpy (deftrasl, ai.traslate);
         c1 = ai.cost[i].cost;
         c2 = ai.cost[i].time;
         c3 = ai.cost[i].cost_first;
         c4 = ai.cost[i].time_first;
      }
   }

   if (!rep && deftrasl[0]) {
      dcost = c1;
      dtime = c2;
      cost_first = c3;
      time_first = c4;
   }

   close (fd);

   online *= 10L;

   if (time_first) {
      if (online > (long)time_first) {
         cost = (long)cost_first;
         online -= (long)time_first;
      }
      else
         cost = 0L;
   }
   else
      cost = 0L;

   if (online <= 0L)
      return (cost);

   if (dtime) {
      cost += (online / (long)dtime) * (long)dcost;
      if ((online % (long)dtime) != 0)
         cost += (long)dcost;
   }

   return (cost);
}

void terminal_emulator ()
{
   FILE *fp_cap;
   int i, relflag, cdflag = -1, *varr, *varr2, pos, m;
   char string[90], c, *cmd, cmdname[60];
   DIALREC dir;
   long olc, origrate;

   setkbloop (NULL);
   status_line ("+Starting terminal emulator");

   varr = ssave ();
   clown_clear ();

   mainview = wopen (0, 0, 23, 79, 5, LGREY|_BLACK, LGREY|_BLACK);
   wactiv (mainview);

   local_echo = expand_cr = 0;
   com_baud (config->speed);
   origrate = rate = config->speed;

   status = wopen (24, 0, 24, 79, 5, LGREY|_BLUE, LGREY|_BLUE);
   wactiv (status);
   sprintf (string, " ALT-Z for Menu      \263 COM%d \263 %6lu %s \263    \263          \263       \263            ", com_port + 1, rate, "N81");
   wprints (0, 0, LGREY|_BLUE, string);
   wprints (0, 48, LGREY|_BLUE, config->avatar ? "Ansi/Avt" : "Ansi");

   wactiv (mainview);
   showcur ();

   memset (&dir, 0, sizeof (DIALREC));
   fp_cap = NULL;
   remote_capabilities = 0;
   pos = 0;
   caller = 0;
   emulator = 1;
   ansi_attr = 7;
   keyboard_loop ();
   mdm_sendcmd (config->term_init);

   XON_DISABLE ();
   _BRK_DISABLE ();

   for (;;) {
      relflag = 1;

      if ((Com_(0x03) & config->carrier_mask)) {
         if (cdflag != 1) {
            hidecur ();
            prints (24, 67, LGREY|_BLUE, "Online 00:00");
            online_elapsed = time (NULL);

            if (mdm_flags == NULL) {
               status_line (msgtxt[M_READY_CONNECT], rate, "", "");
               mdm_flags = "";
            }
            else
               status_line (msgtxt[M_READY_CONNECT], rate, "/", mdm_flags);

            showcur ();
            cdflag = 1;
         }
      }
      else {
         if (cdflag != 0) {
            hidecur ();
            prints (24, 0, LGREY|_BLUE, " ALT-Z for Menu      ");
            prints (24, 67, LGREY|_BLUE, "  Offline   ");

            if (online_elapsed)
               online_elapsed = time (NULL) - online_elapsed;

            if (online_elapsed) {
               mdm_flags = NULL;

               if (dir.phone[0])
                  olc = get_phone_cost (dir.phone, online_elapsed);
               else
                  olc = 0L;
               status_line ("*Session with '%s' Time: %ld:%02ld, Cost: $%ld.%02ld", dir.phone[0] ? dir.name : "Unknown", online_elapsed / 60L, online_elapsed % 60L, olc / 100L, olc % 100L);

               wopen (10, 22, 14, 57, 3, LCYAN|_BLACK, LCYAN|_BLACK);
               wshadow (LGREY|_BLACK);
               sprintf (string, "Connect time %ld:%02ld min.", online_elapsed / 60L, online_elapsed % 60L);
               wcenters (1, LGREY|_BLACK, string);
               timer (15);
               wclose ();
            }

            online_elapsed = 0L;
            showcur ();
            cdflag = 0;

            rate = origrate;
            com_baud (rate);

            if (fp_cap != NULL) {
               close_capture (fp_cap);
               capture_closed ();
            }

            update_statusline ();
         }
      }

      if (CHAR_AVAIL ()) {
         relflag = 0;
         m = 0;
         while (MODEM_AVAILABLE ()) {
            c = MODEM_IN ();
            if (dir.strip_high)
               c &= 0x7F;

            if (c == 0x1B)
               translate_ansibbs ();
            else if (c == CTRLV && config->avatar)
               translate_avatar ();
            else if (c == CTRLY && config->avatar) {
               c = TIMED_READ (2);
               i = TIMED_READ (2);
               for (; i > 0; i--)
                  wputc (c);
            }
            else if (c == CTRLG) {
#ifdef __OS2__
               DosBeep (1000, 30);
#else
               sound (1000);
               timer (3);
               nosound ();
#endif
            }
            else if (c == CTRLL)
               wclear ();
            else {
               if (fp_cap != NULL)
                  fputc (c, fp_cap);

               wputc (c);
               if (c == 0x0D && expand_cr)
                 wputc ('\n');

               if (c != 0x0A) {
                  string[pos++] = c;
                  string[pos] = '\0';
               }

               if (c == 0x0D) {
                  if (config->autozmodem && strstr (string, "rz\r") != NULL) {
                     get_emsi_id (string, 6);
                     if (strstr (string, "**\030") != NULL) {
                        if (dir.download[0])
                           get_Zmodem (dir.download, NULL);
                        else
                           get_Zmodem (config->dl_path, NULL);
                        XON_DISABLE ();
                        _BRK_DISABLE ();
                     }
                  }
                  else if (!stricmp (string, "**EMSI_IRQ8E08\r")) {
                     if (dir.phone[0]) {
                        if (dir.iemsi)
                           terminal_iemsi_handshake (dir.password);
                     }
                     else if (config->iemsi_on)
                        terminal_iemsi_handshake (NULL);
                  }
                  pos = 0;
               }
               else if (pos > 80)
                  pos = 0;
            }

            if (++m > (rate / 10))
               break;
         }
      }
#ifdef __OS2__
      else
         VioUpdate ();
#endif

      if (kbmhit ()) {
         i = getxch ();
         if (kbstat () & SCRLOCK) {
            if (!(i & 0xFF)) {
               SENDBYTE (0);
               SENDBYTE ((i & 0xFF00) >> 8);
            }
            else
               SENDBYTE (i & 0xFF);
         }
         else if ( !(i & 0xFF)) {
            // ALT-X Uscita
            if (i == 0x2D00) {
               if (exit_confirm ())
                  break;
            }

            switch (i) {
               // Alt-E Local echo
               case 0x1200:
                  local_echo = local_echo ? 0 : 1;
                  break;

               // ALT-D Dialing directory
               case 0x2000:
                  if (dialing_directory (&dir) >= 0) {
                     dial_entry (&dir);
                     if (CARRIER) {
                        if (dir.open_capture) {
                           close_capture (fp_cap);
                           fp_cap = open_capture (dir.capture);
                        }

                        update_statusline ();
                        sprintf (string, " %-19.19s", dir.name);
                        prints (24, 0, LGREY|_BLUE, string);

                        if (!config->lock_baud)
                           com_baud (rate);
                     }
                     else
                        memset (&dir, 0, sizeof (DIALREC));
                  }
                  else
                     memset (&dir, 0, sizeof (DIALREC));
                  break;

               // ALT-B Change baud rate
               case 0x3000:
                  change_baud ();
                  origrate = rate;
                  break;

               // ALT-R Reset timer
               case 0x1300:
                  if (online_elapsed)
                     online_elapsed = time (NULL);
                  break;

               // ALT-C Cls
               case 0x2E00:
                  wclear ();
                  break;

               // ALT-I Reinizializza il modem
               case 0x1700:
                  mdm_sendcmd (config->term_init);
                  break;

               // ALT-L Capture ON/OFF
               case 0x2600:
                  if (fp_cap != NULL) {
                     close_capture (fp_cap);
                     capture_closed ();
                     fp_cap = NULL;
                  }
                  else
                     fp_cap = open_capture (NULL);
                  break;

               // ALT-N Send user's name
               case 0x3100:
                  wopen (10, 22, 14, 57, 3, LCYAN|_BLACK, LCYAN|_BLACK);
                  wshadow (LGREY|_BLACK);

                  wcenters (1, LGREY|_BLACK, "Sending user name");
                  m_print ("%s", config->sysop);

                  timer (20);
                  wclose ();
                  break;

               // ALT-S Send active password
               case 0x1F00:
                  if (dir.password[0]) {
                     wopen (10, 22, 14, 57, 3, LCYAN|_BLACK, LCYAN|_BLACK);
                     wshadow (LGREY|_BLACK);

                     wcenters (1, LGREY|_BLACK, "Sending active password");
                     m_print ("%s", dir.password);

                     timer (20);
                     wclose ();
                  }
                  break;

               // ALT-Z Menu' pull-down
               case 0x2C00:
                  pull_down_menu ();
                  break;

               // ALT-J DOS Shell
               case 0x2400:
                  getcwd (string, 79);
                  cmd = getenv ("COMSPEC");
                  strcpy (cmdname, (cmd == NULL) ? "command.com" : cmd);
                  if ((varr2 = ssave ()) == NULL)
                     break;
                  clown_clear ();

                  gotoxy (1, 8);
#ifdef __OS2__
                  printf (msgtxt[M_TYPE_EXIT], 0L);
#else
                  printf (msgtxt[M_TYPE_EXIT], farcoreleft ());
#endif
                  spawn_program (registered, cmdname);

                  setdisk (string[0] - 'A');
                  chdir (string);
                  srestore (varr2);
                  clocks = 0L;
                  break;

               // ALT-H Hangup
               case 0x2300:
                  wopen (10, 22, 14, 57, 3, LCYAN|_BLACK, LCYAN|_BLACK);
                  wshadow (LGREY|_BLACK);
                  wcenters (1, LGREY|_BLACK, "Disconnecting modem");

                  modem_hangup ();

                  wclose ();
                  break;

               // PgUp Upload file
               case 0x4900:
                  terminal_select_file ("UPLOAD", 1, &dir);
                  XON_DISABLE ();
                  _BRK_DISABLE ();
                  break;

               // PgDn Download file
               case 0x5100:
                  terminal_select_file ("DOWNLOAD", 0, &dir);
                  XON_DISABLE ();
                  _BRK_DISABLE ();
                  break;

               // Freccia in su
               case 0x4800:
                  SENDBYTE (0x1B);
                  SENDBYTE ('[');
                  SENDBYTE ('A');
                  break;

               // Freccia in giu
               case 0x5000:
                  SENDBYTE (0x1B);
                  SENDBYTE ('[');
                  SENDBYTE ('B');
                  break;

               // Freccia a sinistra
               case 0x4D00:
                  SENDBYTE (0x1B);
                  SENDBYTE ('[');
                  SENDBYTE ('D');
                  break;

               // Freccia a destra
               case 0x4B00:
                  SENDBYTE (0x1B);
                  SENDBYTE ('[');
                  SENDBYTE ('C');
                  break;

               // Home
               case 0x4700:
                  SENDBYTE (0x1B);
                  SENDBYTE ('[');
                  SENDBYTE ('H');
                  break;

               // End
               case 0x4F00:
                  SENDBYTE (0x1B);
                  SENDBYTE ('[');
                  SENDBYTE ('K');
                  break;
            }
         }
         else {
            i &= 0xFF;
            SENDBYTE (i);

            if (dir.phone[0] && dir.local_echo) {
               if (fp_cap != NULL)
                  fputc (i, fp_cap);
               wputc (i);
#ifdef __OS2__
               VioUpdate ();
#endif
            }
            else if (local_echo) {
               if (fp_cap != NULL)
                  fputc (i, fp_cap);
               wputc (i);
#ifdef __OS2__
               VioUpdate ();
#endif
            }
         }

         relflag = 0;
      }

      keyboard_loop ();
      if (relflag)
         release_timeslice ();
   }

   hidecur ();
   wactiv (mainview);
   wclose ();
   wactiv (status);
   wclose ();
   srestore (varr);

   emulator = 0;
   status_line (":Returning to mailer mode");

   com_baud (config->speed);
   mdm_sendcmd (config->init);
}

static void keyboard_loop ()
{
   char cmdname[40];
   long t;
   struct time timep;

#ifndef __OS2__
   if ( (kbstat () & SCRLOCK) ) {
      if (!doorway) {
         doorway = 1;
         wactiv (mainview);
         wsize (24, 79);
      }
   }
   else if (doorway) {
      doorway = 0;
      wactiv (mainview);
      wsize (23, 79);
   }
#endif

   if (!doorway && timeup (clocks)) {
      clocks = timerset (98);

      gettime ((struct time *)&timep);
      sprintf(cmdname, "%02d%c%02d", timep.ti_hour % 24, interpoint, timep.ti_min % 60);
      prints (24, 59, LGREY|_BLUE, cmdname);

      if (online_elapsed) {
         t = (time (NULL) - online_elapsed) / 60;
         sprintf (cmdname, "%02ld%c%02ld", t / 60L, interpoint, t % 60L);
         prints (24, 74, LGREY|_BLUE, cmdname);
      }

      interpoint = (interpoint == ':') ? ' ' : ':';
   }
}

static void set_selection (void)
{
   struct _item_t *item;

   item = wmenuicurr ();
   last_sel = item->tagid;
}

static void terminal_select_file (char *title, int upload, DIALREC *dialrec)
{
   FILE *fp;
   int fd, i, protocol, x = 0, taginit = -1;
   char filename[80], *p, filetosend[80], *exts[100];
   struct ffblk blk;
   PROTOCOL prot;

   hidecur ();

   i = 0;
   if (config->prot_zmodem)
      i++;
   if (config->prot_xmodem)
      i++;
   if (config->prot_1kxmodem)
      i++;
   if (config->prot_sealink)
      i++;

   sprintf (filename, "%sPROTOCOL.DAT", config->sys_path);
   fd = sh_open (filename, O_RDONLY|O_BINARY, SH_DENYWR, S_IREAD|S_IWRITE);
   if (fd != -1) {
      while (read (fd, &prot, sizeof (PROTOCOL)) == sizeof (PROTOCOL)) {
         if (prot.active)
            i++;
      }
   }

   wopen (3, 27, 4 + i, 70, 3, RED|_LGREY, BLUE|_LGREY);
   wtitle (title, TLEFT, RED|_LGREY);
   wshadow (DGREY|_BLACK);

   wmenubegc ();
   i = 0;
   if (config->prot_zmodem) {
      if (taginit == -1)
         taginit = 1;
      wmenuitem (i++, 0, " Z - ZModem (internal) ", 'Z', 1, M_CLALL, set_selection, 0, 0);
   }
   if (config->prot_xmodem) {
      if (taginit == -1)
         taginit = 2;
      wmenuitem (i++, 0, " X - XModem ", 'X', 2, M_CLALL, set_selection, 0, 0);
   }
   if (config->prot_1kxmodem) {
      if (taginit == -1)
         taginit = 3;
      wmenuitem (i++, 0, " 1 - 1k-Xmodem ", '1', 3, M_CLALL, set_selection, 0, 0);
   }
   if (config->prot_sealink) {
      if (taginit == -1)
         taginit = 4;
      wmenuitem (i++, 0, " S - SEAlink ", 'S', 4, M_CLALL, set_selection, 0, 0);
   }

   x = 0;
   if (fd != -1) {
      lseek (fd, 0L, SEEK_SET);
      while (read (fd, &prot, sizeof (PROTOCOL)) == sizeof (PROTOCOL))
         if (prot.active) {
            sprintf (filename, " %c - %s ", prot.hotkey, prot.name);
            exts[x] = (char *)malloc (strlen (filename) + 1);
            if (exts[x] == NULL)
               continue;
            strcpy (exts[x], filename);
            if (taginit == -1)
               taginit = x + 10;
            wmenuitem (i++, 0, exts[x], prot.hotkey, x + 10, M_CLALL, set_selection, 0, 0);
            x++;
         }
   }
   close (fd);

   exts[x] = NULL;
   wmenuend (taginit, M_VERT|M_SAVE, 42, 0, BLUE|_LGREY, WHITE|_LGREY, DGREY|_LGREY, YELLOW|_BLACK);

   if (wmenuget () == -1) {
      wclose ();
      for (x = 0; exts[x] != NULL; x++)
         free (exts[x]);
      showcur ();
      return;
   }
   else
      protocol = last_sel;

   for (x = 0; exts[x] != NULL; x++)
      free (exts[x]);

   showcur ();

   if (upload || (protocol == 2 || protocol == 3)) {
      wopen (11, 15, 13, 65, 3, LGREY|_BLACK, LCYAN|_BLACK);
      wtitle (upload ? "UPLOAD FILES" : "DOWNLOAD FILES", TLEFT, LCYAN|_BLACK);

      wprints (0, 1, YELLOW|_BLACK, "File(s):");
      winpbeg (BLACK|_LGREY, BLACK|_LGREY);
      winpdef (0, 10, filename, "??????????????????????????????????????", 0, 0, NULL, 0);

      if ((i = winpread ()) == W_ESCPRESS) {
         wclose ();
         wclose ();
         return;
      }

      wclose ();

      strtrim (filename);
      if (!filename[0]) {
         wclose ();
         return;
      }
   }

   wclose ();

   if (upload) {
      i = 0;
      fp = NULL;

      if (protocol >= 10) {
         sprintf (filename, "%sEXTRN%d.LST", config->sys_path, line_offset);
         fp = fopen (filename, "wt");
      }

      do {
         p = strtok (filename, " ");

         if (isalpha (filename[0]) && filename[1] == ':')
            strcpy (filetosend, "");
         else if (filename[0] == '\\')
            strcpy (filetosend, "");
         else
            strcpy (filetosend, config->ul_path);
         strcat (filetosend, filename);

         if (!findfirst (filetosend, &blk, 0))
            do {
               sprintf (filetosend, "%s%s", config->ul_path, blk.ff_name);
               switch (protocol) {
                  case 1:
                     send_Zmodem (filetosend, NULL, i++, 0);
                     break;

                  case 2:
                     fsend (filetosend, 'X');
                     break;

                  case 3:
                     fsend (filetosend, 'Y');
                     break;

                  case 4:
                     fsend (filetosend, 'S');
                     break;

                  default:
                     if (fp != NULL)
                        fprintf (fp, "%s%s\n", config->ul_path, blk.ff_name);
                     break;
               }
            } while (!findnext (&blk));
      } while (protocol != 2 && protocol != 3 && (p = strtok (NULL, " ")) != NULL);

      if (protocol >= 10 && fp != NULL)
         fclose (fp);

      if (protocol == 1)
         send_Zmodem (NULL, NULL, ((i) ? END_BATCH : NOTHING_TO_DO), 0);
      else if (protocol == 4)
         fsend (NULL, 'S');
      else if (protocol >= 10) {
         general_external_protocol (NULL, dialrec->download[0] ? dialrec->download : config->dl_path, protocol, 0, 1);
         sprintf (filename, "%sEXTRN%d.LST", config->sys_path, line_offset);
         unlink (filename);
      }
   }
   else {
      switch (protocol) {
         case 1:
            get_Zmodem (dialrec->download[0] ? dialrec->download : config->dl_path, NULL);
            break;

         case 2:
            who_is_he = 0;
            overwrite = 0;
            receive (dialrec->download[0] ? dialrec->download : config->dl_path, filename, 'X');
            break;

         case 3:
            who_is_he = 0;
            overwrite = 0;
            receive (dialrec->download[0] ? dialrec->download : config->dl_path, filename, 'Y');
            break;

         case 4:
            do {
               who_is_he = 0;
               overwrite = 0;
               p = receive (dialrec->download[0] ? dialrec->download : config->dl_path, NULL, 'S');
            } while (p != NULL);
            break;
      }

      if (protocol >= 10)
         general_external_protocol (NULL, dialrec->download[0] ? dialrec->download : config->dl_path, protocol, 0, 0);
   }

   setkbloop (NULL);
}

static void addshadow (void)
{
   wshadow (DGREY|_BLACK);
}

static void pull_down_menu ()
{
   setkbloop (keyboard_loop);
   hidecur ();

   wmenubeg (0, 0, 0, 79, 5, BLACK|_LGREY, BLACK|_LGREY, NULL);
   wmenuitem (0, 1, " File ", 'F', 100, M_HASPD, NULL, 0, 0);
      wmenubeg (1, 1, 6, 29, 3, RED|_LGREY, BLUE|_LGREY, addshadow);
      wmenuitem (0, 0, " Download files      PgDn  ", 0, 101, M_CLALL, set_selection, 0, 0);
      wmenuitem (1, 0, " Upload files        PgUp  ", 0, 102, M_CLALL, set_selection, 0, 0);
#ifdef __OS2__
      wmenuitem (2, 0, " OS/2 Shell          Alt-J ", 0, 105, M_CLALL, set_selection, 0, 0);
#else
      wmenuitem (2, 0, " DOS Shell           Alt-J ", 0, 105, M_CLALL, set_selection, 0, 0);
#endif
      wmenuitem (3, 0, " Leave Terminal      Alt-X ", 0, 106, M_CLALL, set_selection, 0, 0);
      wmenuend (101, M_PD|M_SAVE, 0, 0, BLUE|_LGREY, WHITE|_LGREY, DGREY|_LGREY, YELLOW|_BLACK);
   wmenuitem (0, 7, " Line ", 'L', 200, M_HASPD, NULL, 0, 0);
      wmenubeg (1, 7, 6, 38, 3, RED|_LGREY, BLUE|_LGREY, addshadow);
      wmenuitem (0, 0, " Change baud rate       Alt-B ", 0, 201, M_CLALL, set_selection, 0, 0);
      wmenuitem (1, 0, " Expand CR to CR/LF           ", 0, 205, M_CLALL, set_selection, 0, 0);
      wmenuitem (2, 0, " Local echo             Alt-E ", 0, 206, M_CLALL, set_selection, 0, 0);
      wmenuitem (3, 0, " Auto-ZModem download         ", 0, 207, M_CLALL, set_selection, 0, 0);
      wmenuend (201, M_PD|M_SAVE, 0, 0, BLUE|_LGREY, WHITE|_LGREY, DGREY|_LGREY, YELLOW|_BLACK);
   wmenuitem (0, 13, " Session ", 'S', 300, M_HASPD, NULL, 0, 0);
      wmenubeg (1, 13, 8, 41, 3, RED|_LGREY, BLUE|_LGREY, addshadow);
      wmenuitem (0, 0, " Capture             Alt-L ", 0, 301, M_CLALL, set_selection, 0, 0);
      wmenuitem (1, 0, " Hangup              Alt-H ", 0, 302, M_CLALL, set_selection, 0, 0);
      wmenuitem (2, 0, " Initialize modem    Alt-I ", 0, 304, M_CLALL, set_selection, 0, 0);
      wmenuitem (3, 0, " Reset timer         Alt-R ", 0, 305, M_CLALL, set_selection, 0, 0);
      wmenuitem (4, 0, " Clear screen        Alt-C ", 0, 306, M_CLALL, set_selection, 0, 0);
      wmenuitem (5, 0, " Dialing directory   Alt-D ", 0, 307, M_CLALL, set_selection, 0, 0);
      wmenuend (301, M_PD|M_SAVE, 0, 0, BLUE|_LGREY, WHITE|_LGREY, DGREY|_LGREY, YELLOW|_BLACK);

   if (last_sel == -1)
      last_sel = 100;
   else
      last_sel = (last_sel / 100) * 100;

   wmenuend (last_sel, M_HORZ, 0, 0, BLUE|_LGREY, WHITE|_LGREY, DGREY|_LGREY, YELLOW|_BLACK);

   kbput (0x1C0D);
   if (wmenuget () != -1)
      switch (last_sel) {
         // PgDn - Download
         case 101:
            kbput (0x5100);
            break;

         // PgUp - Upload
         case 102:
            kbput (0x4900);
            break;

         // Alt-J Shell
         case 105:
            kbput (0x2400);
            break;

         // Alt-X Exit
         case 106:
            kbput (0x2D00);
            break;

         // Alt-B Baud rate
         case 201:
            kbput (0x3000);
            break;

         // Expand CR to CR/LF
         case 205:
            expand_cr = expand_cr ? 0 : 1;
            prints (24, 44, LGREY|_BLUE, expand_cr ? "T" : " ");
            break;

         // Alt-E Local echo
         case 206:
            local_echo = local_echo ? 0 : 1;
            break;

         // Auto-ZModem
         case 207:
            config->autozmodem = config->autozmodem ? 0 : 1;
            break;

         // Alt-L
         case 301:
            kbput (0x2600);
            break;

         // Alt-H
         case 302:
            kbput (0x2300);
            break;

         // Alt-I
         case 304:
            kbput (0x1700);
            break;

         // Alt-R
         case 305:
            kbput (0x1300);
            break;

         // Alt-C
         case 306:
            kbput (0x2E00);
            break;

         // Alt-D
         case 307:
            kbput (0x2000);
            break;
      }

   setkbloop (NULL);
   showcur ();
}

#define  NORM   0x07
#define  BOLD   0x08
#define  FAINT  0xF7
#define  VBLINK 0x80
#define  REVRS  0x77

static void translate_ansibbs ()
{
   static char sx = 1, sy = 1;
   char c, str[15];
   int Pn[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
   int i = 0, p = 0, v;

   wactiv (mainview);
   wreadcur (&vy, &vx);

   c = TIMED_READ (2);
   if (c != '[') {
      wputc (0x1B);
      wputc (c);
      return;
   }

   while (1) {
      c = TIMED_READ (2);

      if (isdigit (c))
         *(str + i++) = c;
      else {
         *(str + i) = '\0';
         i = 0;
         Pn[p++] = atoi (str);

         if (c == ';')
            continue;
         else
            switch(c) {
               /* (CUP) set cursor position  */
               case 'H':
               case 'F':
               case 'h':
               case 'f':
                  vy = Pn[0] ? Pn[0] : 1;
                  vx = Pn[1] ? Pn[1] : 1;
                  wgotoxy (vy - 1, vx - 1);
                  return;
                  /* (CUU) cursor up   */
               case 'A':
                  vy -= Pn[0] ? Pn[0] : 1;
                  if (vy < 1)
                     vy = 1;
                  wgotoxy (vy, vx);
                  return;
                  /* (CUD) cursor down */
               case 'B':
                  vy += Pn[0] ? Pn[0] : 1;
                  if (vy > 25)
                     vy = 25;
                  wgotoxy (vy, vx);
                  return;
                  /* (CUF) cursor forward */
               case 'C':
                  vx += Pn[0] ? Pn[0] : 1;
                  if (vx > 80)
                     vx = 80;
                  wgotoxy (vy, vx);
                  return;
                  /* (CUB) cursor backward   */
               case 'D':
                  vx -= Pn[0] ? Pn[0] : 1;
                  if (vx < 1)
                     vx = 1;
                  wgotoxy (vy, vx);
                  return;
                  /* (SCP) save cursor position */
               case 's':
                  sx = vx;
                  sy = vy;
                  return;
                  /* (RCP) restore cursor position */
               case 'u':
                  vx = sx;
                  vy = sy;
                  wgotoxy (vy, vx);
                  return;
                  /* clear screen   */
               case 'J':
                  if (Pn[0] == 2)
                     wclear ();
                  return;
                  /* (EL) erase line   */
               case 'K':
                  wclreol ();
                  return;
                  /* An attribute command is more elaborate than the    */
                  /* others because it may have many numeric parameters */
               case 'm':
                  for(i=0; i<p; i++) {
                     if (Pn[i] >= 30 && Pn[i] <= 37) {
                        ansi_attr &= 0xf8;
                        switch (Pn[i]) {
                           case 30:
                              ansi_attr |= BLACK;
                              break;
                           case 31:
                              ansi_attr |= RED;
                              break;
                           case 32:
                              ansi_attr |= GREEN;
                              break;
                           case 33:
                              ansi_attr |= BROWN;
                              break;
                           case 34:
                              ansi_attr |= BLUE;
                              break;
                           case 35:
                              ansi_attr |= MAGENTA;
                              break;
                           case 36:
                              ansi_attr |= CYAN;
                              break;
                           case 37:
                              ansi_attr |= LGREY;
                              break;
                        }
                     }

                     if (Pn[i] >= 40 && Pn[i] <= 47) {
                        ansi_attr &= 0x8f;
                        switch (Pn[i]) {
                           case 40:
                              ansi_attr |= _BLACK;
                              break;
                           case 41:
                              ansi_attr |= _RED;
                              break;
                           case 42:
                              ansi_attr |= _GREEN;
                              break;
                           case 43:
                              ansi_attr |= _BROWN;
                              break;
                           case 44:
                              ansi_attr |= _BLUE;
                              break;
                           case 45:
                              ansi_attr |= _MAGENTA;
                              break;
                           case 46:
                              ansi_attr |= _CYAN;
                              break;
                           case 47:
                              ansi_attr |= _LGREY;
                              break;
                        }
                     }

                     if (Pn[i] >= 0 && Pn[i] <= 7)
                        switch(Pn[i]) {
                           case 0:
                              ansi_attr = NORM;
                              break;
                           case 1:
                              ansi_attr |= BOLD;
                              break;
                           case 2:
                              ansi_attr &= FAINT;
                              break;
                           case 5:
                           case 6:
                              ansi_attr |= VBLINK;
                              break;
                           case 7:
                              ansi_attr ^= REVRS;
                              break;
                           default:
                              ansi_attr = NORM;
                              break;
                        }
                  }

                  wtextattr (ansi_attr);
                  return;

               case 'n':
                  if (Pn[0] == 6) {
                     sprintf (str, "\x1B[%d;%dR", vx + 1, vy + 1);
                     for (v = 0; str[v]; v++)
                        SENDBYTE (str[v]);
                  }
                  break;

               default:
                  return;
            }
      }
   }
}

static void translate_avatar (void)
{
   int c, i, m;
   char stringa[80], *p;

   wactiv (mainview);
   wreadcur (&vy, &vx);

   c = TIMED_READ (2);
   switch (c) {
      case CTRLA:
         c = TIMED_READ (2);
         ansi_attr = c;
         wtextattr (ansi_attr);
         break;
      /* (CUU) cursor up   */
      case CTRLC:
         if (vy)
            vy--;
         wgotoxy (vy, vx);
         return;
      /* (CUD) cursor down */
      case CTRLD:
         if (vy < 23)
            vy++;
         wgotoxy (vy, vx);
         return;
      /* (CUF) cursor forward */
      case CTRLF:
         if (vx < 79)
            vx++;
         wgotoxy (vy, vx);
         return;
      /* (CUB) cursor backward   */
      case CTRLE:
         if (vx)
            vx--;
         wgotoxy (vy, vx);
         return;
      /* (EL) erase line   */
      case CTRLG:
         wclreol ();
         return;
      case CTRLH:
         vy = TIMED_READ (2) & 0xFF;
         vx = TIMED_READ (2) & 0xFF;
         wgotoxy (vy - 1, vx - 1);
         return;
      case CTRLY:
         i = TIMED_READ (2);
         for (m = 0; m < i; m++)
            stringa[m] = TIMED_READ (2);
         stringa[m] = '\0';
         i = TIMED_READ (2);
         for (m = 0; m < i; m++) {
            p = stringa;
            while (*p) {
               if (*p == CTRLV) {
                  p++;
                  switch (*p) {
                     /* (CUU) cursor up   */
                     case CTRLC:
                        if (vy)
                           vy--;
                        wgotoxy (vy, vx);
                        break;
                     /* (CUD) cursor down */
                     case CTRLD:
                        if (vy < 23)
                           vy++;
                        wgotoxy (vy, vx);
                        break;
                     /* (CUF) cursor forward */
                     case CTRLF:
                        if (vx < 79)
                           vx++;
                        wgotoxy (vy, vx);
                        break;
                     /* (CUB) cursor backward   */
                     case CTRLE:
                        if (vx)
                           vx--;
                        wgotoxy (vy, vx);
                        break;
                     default:
                        wputc (*p);
                        break;
                  }
                  wreadcur (&vy, &vx);
               }
               else
                  wputc (*p);
               p++;
            }
         }
         return;
   }
}

static void close_capture (FILE *fp_cap)
{
   if (fp_cap != NULL) {
      fprintf (fp_cap, "\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\n\n");
      fprintf (fp_cap, "\n");
      fclose (fp_cap);
      prints (24, 43, LGREY|_BLUE, " ");
   }
}

static void capture_closed (void)
{
   wopen (10, 22, 14, 57, 3, LCYAN|_BLACK, LCYAN|_BLACK);
   wshadow (LGREY|_BLACK);

   wcenters (1, LGREY|_BLACK, "Capture file closed");

   timer (20);
   wclose ();
}

static FILE *open_capture (char *predefined)
{
   FILE *fp;
   int wh;
   char string[80], cpath[80], *p;
   long t;
   struct tm *tim;

   if (predefined == NULL || *predefined == '\0') {
      getcwd (cpath, 78);

      wh = wopen (11, 7, 13, 73, 3, LGREY|_BLACK, LCYAN|_BLACK);
      wactiv (wh);
      wtitle ("OPEN CAPTURE", TLEFT, LCYAN|_BLACK);

      wprints (0, 1, YELLOW|_BLACK, "Filename");
      sprintf (string, "%s\\TERMINAL.CAP", cpath);
      winpbeg (BLACK|_LGREY, BLACK|_LGREY);
      winpdef (0, 10, string, "?????????????????????????????????????????????????????", 0, 2, NULL, 0);

      if (winpread () == W_ESCPRESS) {
         wclose ();
         return (NULL);
      }

      wclose ();

      p = strbtrim (string);
   }
   else
      p = predefined;

   if ((fp = fopen (p, "ab")) != NULL) {
      t = time (NULL);
      tim = localtime (&t);

      fprintf (fp, "                    Capture file opened %02d-%s-%d %2d:%02d\n", tim->tm_mday, mtext[tim->tm_mon], tim->tm_year, tim->tm_hour, tim->tm_min);
      fprintf (fp, "\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\n");

      if (fp != NULL)
         prints (24, 43, LGREY|_BLUE, "C");
   }

   return (fp);
}

static int exit_confirm ()
{
   int wh, i;
   char string[10];

   wh = wopen (10, 22, 14, 57, 3, LCYAN|_BLACK, LCYAN|_BLACK);
   wactiv (wh);
   wshadow (LGREY|_BLACK);

   wcenters (1, LGREY|_BLACK, "Exit terminal mode (Y/n) ?  ");

   strcpy (string, "Y");
   winpbeg (LGREY|_BLACK, LGREY|_BLACK);
   winpdef (1, 30, string, "?", 0, 2, NULL, 0);

   i = winpread ();
   wclose ();

   if (i == W_ESCPRESS || toupper (string[0]) == 'N')
      return (0);

   return (1);
}

static void terminal_iemsi_handshake (char *password)
{
   int i, tries;
   unsigned long crc;
   char string[2048], addr[20], *p, id[40], name[40], location[40], operator[40], notice[70];
   long t1, t2;

   wopen (1, 40, 8, 76, 3, RED|_LGREY, BLUE|_LGREY);
   wshadow (DGREY|_BLACK);
   wtitle ("NEGOTIATING IEMSI", TLEFT, RED|_LGREY);

   wputs (" Received IEMSI request\n");

   strcpy (string, "{");
   strcat (string, config->sysop);
   strcat (string, "}{");
   strcat (string, config->iemsi_handle);
   strcat (string, "}{");
   strcat (string, config->location);
   strcat (string, "}{");
   strcat (string, config->phone);
   strcat (string, "}{}{");
   if (password != NULL && *password)
      strcat (string, password);
   else
      strcat (string, config->iemsi_pwd);
   strcat (string, "}{}{");
   if (config->avatar)
      strcat (string, "AVT0,");
   else
      strcat (string, "ANSI,");
   strcat (string, "24,80,0}{ZMO,SLK}{TAB,ASCII8}{");
   if (config->iemsi_news)
      strcat (string, "NEWS,");
   if (config->iemsi_newmail)
      strcat (string, "MAIL,");
   if (config->iemsi_newfiles)
      strcat (string, "FILE,");
   if (config->iemsi_hotkeys)
      strcat (string, "HOT,");
   if (config->iemsi_screenclr)
      strcat (string, "CLR,");
   if (config->iemsi_quiet)
      strcat (string, "HUSH,");
   if (config->iemsi_pausing)
      strcat (string, "MORE,");
   if (config->iemsi_editor)
      strcat (string, "FSED,");
   if (string[strlen (string) - 1] == ',')
      string[strlen (string) - 1] = '\0';
#ifdef __OS2__
   strcat (string, "}{LoraBBS-OS/2,");
#else
   strcat (string, "}{LoraBBS-DOS,");
#endif
   strcat (string, VNUM);
   activation_key ();
   if (registered) {
      sprintf (addr, ",%s%05u}{", serial_id[0] ? serial_id : "", serial_no);
      strcat (string, addr);
   }
   else
      strcat (string, ",Demo}{");
   strcat (string, "}");

   sprintf (addr, "EMSI_ICI%04X", strlen (string));
   crc = 0xFFFFFFFFL;
   for (i = 0; i < strlen (addr); i++)
      crc = Z_32UpdateCRC (addr[i], crc);

   for (i = 0; i < strlen (string); i++)
      crc = Z_32UpdateCRC (string[i], crc);

   t1 = timerset (6000);
   tries = 1;

   for (;;) {
      if (tries++ > 4 || timeup (t1)) {
         wputs (" Too many errors\n");
         timer (5);
         wclose ();
         status_line ("!Trans. IEMSI/T failure");
         return;
      }

      wputs (" Sending Client packet\n");

      sprintf (addr, "**EMSI_ICI%04X", strlen (string));
      for (p = addr; *p; p++)
         BUFFER_BYTE (*p);
      for (p = string; *p; p++)
         BUFFER_BYTE (*p);

      UNBUFFER_BYTES ();
      CLEAR_INBOUND ();

      sprintf (addr, "%08lX\r", crc);
      for (p = addr; *p; p++)
         BUFFER_BYTE (*p);
      UNBUFFER_BYTES ();

      t2 = timerset (1000);

      while (!timeup (t1)) {
         if (timeup (t2))
            break;

         if (PEEKBYTE() == '*') {
            get_emsi_id (addr, 14);
            if (!strnicmp (addr, "**EMSI_ISI", 10))
               goto receive_isi;
         }
         else if (PEEKBYTE () != -1) {
            TIMED_READ (10);
            time_release ();
         }
      }
   }

receive_isi:

   wputs (" Receiving Server packet\n");

   tries = 0;
   t1 = timerset (1000);
   t2 = timerset (6000);

resend:
   if (tries++ > 6) {
      wclose ();
      status_line ("!IEMSI/T Error: too may tries");
      return;
   }

   if (!get_emsi_field (string))
      goto resend;
   if (strlen (string) > 39)
      string[39] = '\0';
   strcpy (id, string);

   if (!get_emsi_field (string))
      goto resend;
   if (strlen (string) > 39)
      string[39] = '\0';
   strcpy (name, string);

   if (!get_emsi_field (string))
      goto resend;
   if (strlen (string) > 39)
      string[39] = '\0';
   strcpy (location, string);

   if (!get_emsi_field (string))
      goto resend;
   if (strlen (string) > 39)
      string[39] = '\0';
   strcpy (operator, string);

   if (!get_emsi_field (string))
      goto resend;
   if (strlen (string) > 39)
      string[39] = '\0';

   if (!get_emsi_field (string))
      goto resend;
   if (strlen (string) > 69)
      string[39] = '\0';
   strcpy (notice, string);

   if (!get_emsi_field (string))
      goto resend;
   if (strlen (string) > 39)
      string[39] = '\0';
   if (!get_emsi_field (string))
      goto resend;
   if (strlen (string) > 39)
      string[39] = '\0';

   snooping = 0;
   timer (5);
   m_print2 ("**EMSI_ACKA490\r**EMSI_ACKA490\r");
   snooping = 1;
   wclose ();
   CLEAR_INBOUND ();

   wopen (3, 2, 13, 76, 3, RED|_LGREY, BLUE|_LGREY);
   wshadow (DGREY|_BLACK);
   wtitle ("IEMSI SERVER", TLEFT, RED|_LGREY);

   wprints (1, 1, BLACK|_LGREY, "You have reached :");
   wprints (3, 1, BLACK|_LGREY, "Id");
   wprints (4, 1, BLACK|_LGREY, "Name");
   wprints (5, 1, BLACK|_LGREY, "Location");
   wprints (6, 1, BLACK|_LGREY, "Operator");
   wprints (7, 1, BLACK|_LGREY, "Notice");

   wprints (3, 12, BLUE|_LGREY, id);
   wprints (4, 12, BLUE|_LGREY, name);
   wprints (5, 12, BLUE|_LGREY, location);
   wprints (6, 12, BLUE|_LGREY, operator);
   wprints (7, 12, BLUE|_LGREY, notice);

   timer (config->iemsi_infotime * 10);
   wclose ();
}

int pickstrw(int srow,int scol,int erow,int ecol,int btype,int bordattr,
             int winattr,int barattr,char *strarr[],int initelem,
             void (*open)(void),
             char *(*insert)(int),int (*delete)(char *,int),void (*edit)(int));

#define MAX_PHONES 100

static int numphones;
static char **list;
static DIALREC *dr;

void edit_single_entry (DIALREC *dr, char *title)
{
   int i;
   char string[50];
   DIALREC nd;

   memcpy (&nd, dr, sizeof (DIALREC));

   wopen (7, 13, 20, 68, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wshadow (DGREY|_BLACK);
   wtitle (title, TRIGHT, YELLOW|_BLUE);
   i = 1;

   do {
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " Name           ", 0,  1, 0, NULL, 0, 0);
      wmenuitem ( 2,  1, " Location       ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " Phone          ", 0,  3, 0, NULL, 0, 0);
      wmenuitem ( 4,  1, " Password       ", 0,  4, 0, NULL, 0, 0);
      wmenuitem ( 5,  1, " Download path  ", 0,  5, 0, NULL, 0, 0);
      wmenuitem ( 6,  1, " Auto capture   ", 0,  6, 0, NULL, 0, 0);
      wmenuitem ( 7,  1, " \300\304 Capture     ", 0,  7, 0, NULL, 0, 0);
      wmenuitem ( 8,  1, " IEMSI          ", 0,  8, 0, NULL, 0, 0);
      wmenuitem ( 9,  1, " Local echo     ", 0,  9, 0, NULL, 0, 0);
      wmenuitem (10,  1, " Strip high bit ", 0, 10, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 18, CYAN|_BLACK, nd.name);
      wprints (2, 18, CYAN|_BLACK, nd.location);
      wprints (3, 18, CYAN|_BLACK, nd.phone);
      memset (string, '.', 20);
      string[strlen (nd.password)] = '\0';
      wprints (4, 18, CYAN|_BLACK, string);
      wprints (5, 18, CYAN|_BLACK, nd.download);
      wprints (6, 18, CYAN|_BLACK, nd.open_capture ? "Yes" : "No");
      wprints (7, 18, CYAN|_BLACK, nd.capture);
      wprints (8, 18, CYAN|_BLACK, nd.iemsi ? "Yes" : "No");
      wprints (9, 18, CYAN|_BLACK, nd.local_echo ? "Yes" : "No");
      wprints (10, 18, CYAN|_BLACK, nd.strip_high ? "Yes" : "No");

      i = wmenuget ();

      switch (i) {
         case 1:
            strcpy (string, nd.name);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (1, 18, string, "???????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nd.name, strtrim (string));
            break;

         case 2:
            strcpy (string, nd.location);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (2, 18, string, "?????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nd.location, strtrim (string));
            break;

         case 3:
            strcpy (string, nd.phone);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 18, string, "?????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nd.phone, strtrim (string));
            break;

         case 4:
            strcpy (string, nd.password);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 18, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nd.password, strtrim (string));
            break;

         case 5:
            strcpy (string, nd.download);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (5, 18, string, "???????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nd.download, strtrim (string));
            break;

         case 6:
            nd.open_capture ^= 1;
            break;

         case 7:
            strcpy (string, nd.capture);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (7, 18, string, "???????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nd.capture, strtrim (string));
            break;

         case 8:
            nd.iemsi ^= 1;
            break;

         case 9:
            nd.local_echo ^= 1;
            break;

         case 10:
            nd.strip_high ^= 1;
            break;
      }

      hidecur ();
   } while (i != -1);

   wclose ();
   memcpy (dr, &nd, sizeof (DIALREC));
}

char *insert_entry (int n)
{
   int i;
   char string[50], *nlist;
   DIALREC nd;

   if (numphones < MAX_PHONES) {
      memset (&nd, 0, sizeof (DIALREC));

      edit_single_entry (&nd, " Add entry ");

      nlist = (char *)malloc (72);
      sprintf (nlist, " %2d  %-29.29s %-20.20s %-15.15s", n + 1, nd.name, nd.location, nd.phone);

      for (i = numphones; i >= n; i--)
         memcpy (&dr[i + 1], &dr[i], sizeof (DIALREC));
      memcpy (&dr[n], &nd, sizeof (DIALREC));
      numphones++;

      for (i = n; i < numphones - 1; i++) {
         sprintf (string, " %2d", i + 2);
         memcpy (list[i], string, 3);
      }

      return (nlist);
   }

   return (NULL);
}

void edit_entry (int n)
{
   edit_single_entry (&dr[n], " Edit entry ");
   sprintf (list[n], " %2d  %-29.29s %-20.20s %-15.15s", n + 1, dr[n].name, dr[n].location, dr[n].phone);
}

int delete_entry (char *str, int n)
{
   int i;
   char string[4];

   if (numphones == 0 || n >= numphones)
      return (0);

   wopen (10, 25, 14, 54, 3, BLACK|_LGREY, BLACK|_LGREY);
   wshadow (DGREY|_BLACK);

   wcenters (1, BLACK|_LGREY, "Are you sure (Y/n) ?  ");

   strcpy (string, "Y");
   winpbeg (BLACK|_LGREY, BLACK|_LGREY);
   winpdef (1, 24, string, "?", 0, 2, NULL, 0);

   i = winpread ();
   wclose ();
   hidecur ();

   if (i == W_ESCPRESS)
      return (0);

   if (toupper (string[0]) == 'Y') {
      for (i = n; i < numphones; i++) {
         memcpy (&dr[i], &dr[i + 1], sizeof (DIALREC));
         sprintf (string, " %2d", i);
         memcpy (list[i], string, 3);
      }

      free (str);

      numphones--;
      return (1);
   }

   return (0);
}

int dialing_directory (DIALREC *dialrec)
{
   int fd, i, res;

   setkbloop (keyboard_loop);
   hidecur ();
   numphones = 0;

   dr = (DIALREC *)malloc (MAX_PHONES * sizeof (DIALREC));
   memset (dr, 0, MAX_PHONES * sizeof (DIALREC));

   list = (char **)malloc (MAX_PHONES * sizeof (char *));
   memset (list, 0, MAX_PHONES * sizeof (char *));

   if ((fd = open ("PHONE.DAT", O_RDONLY|O_BINARY)) != -1) {
      numphones = (int)(filelength (fd) / sizeof (DIALREC));
      if (numphones > MAX_PHONES)
         numphones = MAX_PHONES;
      read (fd, dr, numphones * sizeof (DIALREC));
      close (fd);
   }

   for (i = 0; i < numphones; i++) {
      list[i] = (char *)malloc (72);
      sprintf (list[i], " %2d  %-29.29s %-20.20s %-15.15s", i + 1, dr[i].name, dr[i].location, dr[i].phone);
   }

   if (i < MAX_PHONES) {
      list[i] = (char *)malloc (72);
      memset (list[i], ' ', 71);
      list[i][71] = '\0';
   }

   wopen (4, 4, 21, 76, 3, LCYAN|_BLACK, LCYAN|_BLACK);
   wshadow (DGREY|_BLACK);
   wprints (0, 0, LGREY|_BLACK, "  #  System name                   Location             Phone");
   whline (1, 0, 71, 2, BLUE|_BLACK);
   whline (14, 0, 71, 2, BLUE|_BLACK);
   wprints (15, 1, LGREY|_BLACK, "ESC-Exit  ENTER-Dial  INS-Add  E-Edit  DEL-Delete");
   wprints (15, 1, YELLOW|_BLACK, "ESC");
   wprints (15, 11, YELLOW|_BLACK, "ENTER");
   wprints (15, 23, YELLOW|_BLACK, "INS");
   wprints (15, 32, YELLOW|_BLACK, "E");
   wprints (15, 40, YELLOW|_BLACK, "DEL");

   res = pickstrw (7, 5, 18, 75, 5, LCYAN|_BLACK, LCYAN|_BLACK, BLACK|_LGREY, list, 0, NULL, insert_entry, delete_entry, edit_entry);
   wclose ();

   if (res >= 0 && dialrec != NULL)
      memcpy (dialrec, &dr[res], sizeof (DIALREC));

   if ((fd = open ("PHONE.DAT", O_WRONLY|O_BINARY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE)) != -1) {
      write (fd, dr, sizeof (DIALREC) * numphones);
      close (fd);
   }

   for (i = 0; i < numphones; i++)
      free (list[i]);
   free (list[i]);
   free (list);
   free (dr);

   showcur ();
   setkbloop (NULL);

   return (res);
}

void dial_entry (DIALREC *dialrec)
{
   int i, ctry = 1, tout;
   char string[40], phone[60];
   long t;

   setkbloop (keyboard_loop);
   hidecur ();

   wopen (6, 10, 19, 70, 3, LCYAN|_BLACK, LCYAN|_BLACK);
   wtitle (" Dial system ", TRIGHT, YELLOW|_BLUE);
   wshadow (DGREY|_BLACK);
   wprints (0, 1, LGREY|_BLACK, "Attempt #");
   whline (1, 0, 71, 2, BLUE|_BLACK);

   wprints (2, 1, LGREY|_BLACK, "Name       : ");
   wprints (3, 1, LGREY|_BLACK, "Location   : ");
   wprints (4, 1, LGREY|_BLACK, "Phone      : ");

   wprints (2, 14, CYAN|_BLACK, dialrec->name);
   wprints (3, 14, CYAN|_BLACK, dialrec->location);
   strcpy (phone, dialrec->phone);
   translate_phone (phone);
   wprints (4, 14, CYAN|_BLACK, phone);

   whline (5, 0, 71, 2, BLUE|_BLACK);
   wprints (6, 1, LGREY|_BLACK, "ESC-Abort  SPACE-Retry  A-Add 15 secs.");
   wprints (6, 1, YELLOW|_BLACK, "ESC");
   wprints (6, 12, YELLOW|_BLACK, "SPACE");
   wprints (6, 25, YELLOW|_BLACK, "A");
   whline (7, 0, 71, 2, BLUE|_BLACK);

   for (;;) {
      wprints (0, 15, LGREY|_BLACK, "         ");
      tout = config->dial_timeout;

      sprintf (string, "%d", ctry);
      wprints (0, 11, YELLOW|_BLACK, string);

      t = time (NULL);
      sprintf (string, "   Timeout %d seconds", tout);
      wrjusts (0, 57, LGREY|_BLACK, string);
      sprintf (string, "%d", tout);
      wprints (0, 50 - strlen (string), YELLOW|_BLACK, string);

retry1:
      status_line (msgtxt[M_DIALING_NUMBER], phone);
      dial_number (0, phone);

retry3:
      while (tout) {
         if (time (NULL) != t) {
            tout--;
            t = time (NULL);
            sprintf (string, "   Timeout %d seconds", tout);
            wrjusts (0, 57, LGREY|_BLACK, string);
            sprintf (string, "%d", tout);
            wprints (0, 50 - strlen (string), YELLOW|_BLACK, string);
         }

         release_timeslice ();
			if (kbmhit ())
				break;

			if ((i = modem_response ()) != -1) {
				switch (i) {
					case 1:
						rate = 300;
						break;
					case 5:
						rate = 1200;
						break;
					case 11:
						rate = 2400;
						break;
					case 18:
						rate = 4800;
						break;
					case 16:
						rate = 7200;
						break;
					case 12:
						rate = 9600;
						break;
					case 17:
						rate = 12000;
						break;
					case 15:
						rate = 14400;
						break;
					case 19:
						rate = 16800;
						break;
					case 13:
						rate = 19200;
						break;
					case 14:
						rate = 38400U;
						break;
					case 20:
						rate = 57600U;
						break;
					case 21:
						rate = 21600;
						break;
					case 22:
						rate = 28800;
						break;
					case 28:
						rate = 24000;
						break;
					case 29:
						rate = 26400;
						break;
					case 30:
						rate = 31200;
						break;
					case 31:
						rate = 33600;
						break;
				}

				switch (i) {
					case 1:
					case 5:
					case 11:
					case 18:
					case 16:
					case 12:
					case 17:
					case 15:
					case 19:
					case 13:
					case 14:
					case 20:
					case 21:
					case 22:
					case 28:
					case 29:
					case 30:
					case 31:
					case 40:
						wclose ();
						setkbloop (NULL);

						wopen (10, 22, 14, 57, 3, LCYAN|_BLACK, LCYAN|_BLACK);
						wshadow (LGREY|_BLACK);
						sprintf (string, "Connect at %u bps", rate);
						wcenters (1, LGREY|_BLACK, string);
						timer (15);
						wclose ();
						return;
				}

				break;
         }
      }

      if (kbmhit ()) {
         if ((i = getxch ()) == 0x011B)
            break;
         else if (i == 0x1E61 || i == 0x1E41) {
            tout += 15;
            sprintf (string, "   Timeout %d seconds", tout);
            wrjusts (0, 57, LGREY|_BLACK, string);
            sprintf (string, "%d", tout);
            wprints (0, 50 - strlen (string), YELLOW|_BLACK, string);
            goto retry3;
         }
         else if (i != 0x3920)
            goto retry3;
      }

      tout = config->dial_pause;

      modem_hangup ();
      wprints (0, 15, LGREY|_BLACK, "(Pausing)");

      t = time (NULL);
      sprintf (string, "   Timeout %d seconds", tout);
      wrjusts (0, 57, LGREY|_BLACK, string);
      sprintf (string, "%d", tout);
      wprints (0, 50 - strlen (string), YELLOW|_BLACK, string);

retry2:
      while (tout) {
         if (time (NULL) != t) {
            tout--;
            t = time (NULL);
            sprintf (string, "   Timeout %d seconds", tout);
            wrjusts (0, 57, LGREY|_BLACK, string);
            sprintf (string, "%d", tout);
            wprints (0, 50 - strlen (string), YELLOW|_BLACK, string);
         }

         modem_response ();
         release_timeslice ();

         if (kbmhit ())
            break;
      }

      if (kbmhit ()) {
         if ((i = getxch ()) == 0x011B)
            break;
         else if (i == 0x1E61 || i == 0x1E41) {
            tout += 15;
            sprintf (string, "   Timeout %d seconds", tout);
            wrjusts (0, 57, LGREY|_BLACK, string);
            sprintf (string, "%d", tout);
            wprints (0, 50 - strlen (string), YELLOW|_BLACK, string);
            goto retry2;
         }
         else if (i != 0x3920)
            goto retry2;
      }

      ctry++;
   }

   wprints (0, 15, LGREY|_BLACK, "(Aborting)");
   modem_hangup ();

   wclose ();
   showcur ();
   setkbloop (NULL);
}

static void translate_phone (char *phone)
{
   int fd;
   char filename[80], deftrasl[60], rep = 0;
   ACCOUNT ai;

   deftrasl[0] = '\0';

   sprintf (filename, "%sCOST.DAT", config->net_info);
   fd = open (filename, O_RDONLY|O_BINARY);

   while (read (fd, (char *)&ai, sizeof (ACCOUNT)) == sizeof (ACCOUNT)) {
      if (!strncmp (phone, ai.search, strlen (ai.search))) {
         strisrep (phone, ai.search, ai.traslate);
         rep = 1;
         break;
      }
      else if (!strcmp (ai.search, "/"))
         strcpy (deftrasl, ai.traslate);
   }

   if (!rep && deftrasl[0]) {
      strcpy (filename, deftrasl);
      strcat (filename, phone);
      strcpy (phone, filename);
   }

   close (fd);
}

