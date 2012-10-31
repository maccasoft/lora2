#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

#include "lsetup.h"
#include "lprot.h"

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>
#include <cxl\cxlkey.h>

extern struct _configuration config;

char *get_priv_text (int);
void create_path (char *);
int sh_open (char *file, int shmode, int omode, int fmode);
void update_message (void);
void clear_window (void);
int winputs (int wy, int wx, char *stri, char *fmt, int mode, char pad, int fieldattr, int textattr);
long window_get_flags (int y, int x, int type, long f);

static void file_select_area_list (int, struct _sys *);
static void file_edit_single_area (struct _sys *);

void modem_hardware ()
{
   int wh, i = 1;
   char string[128];

   wh = wopen (5, 22, 18, 65, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Hardware ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " Modem port        ", 0,  1, 0, NULL, 0, 0);
      wmenuitem ( 2,  1, " Maximum baud rate ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " Lock port         ", 0,  3, 0, NULL, 0, 0);
      wmenuitem ( 4,  1, " Terminal          ", 0,  4, 0, NULL, 0, 0);
      wmenuitem ( 5,  1, " FAX Message       ", 0,  5, 0, NULL, 0, 0);
      wmenuitem ( 6,  1, " À FAX Errorlevel  ", 0,  6, 0, NULL, 0, 0);
      wmenuitem ( 7,  1, " Strip dashes      ", 0,  7, 0, NULL, 0, 0);
      wmenuitem ( 8,  1, " Dialing timeout   ", 0,  8, 0, NULL, 0, 0);
      wmenuitem ( 9,  1, " Carrier mask      ", 0,  9, 0, NULL, 0, 0);
      wmenuitem (10,  1, " DCD drop timeout  ", 0, 10, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      sprintf (string, "%d", config.com_port + 1);
      wprints (1, 21, CYAN|_BLACK, string);
      sprintf (string, "%u", (unsigned int)(config.speed & 0xFFFFU));
      wprints (2, 21, CYAN|_BLACK, string);
      wprints (3, 21, CYAN|_BLACK, config.lock_baud ? "Yes" : "No");
      wprints (4, 21, CYAN|_BLACK, config.terminal ? "Yes" : "No");
      wprints (5, 21, CYAN|_BLACK, config.fax_response);
      sprintf (string, "%d", config.fax_errorlevel);
      wprints (6, 21, CYAN|_BLACK, string);
      wprints (7, 21, CYAN|_BLACK, config.stripdash ? "Yes" : "No");
      sprintf (string, "%d", config.modem_timeout);
      wprints (8, 21, CYAN|_BLACK, string);
      sprintf (string, "%d", config.carrier_mask);
      wprints (9, 21, CYAN|_BLACK, string);
      sprintf (string, "%d", config.dcd_timeout);
      wprints (10, 21, CYAN|_BLACK, string);

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            sprintf (string, "%d", config.com_port + 1);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (1, 21, string, "??", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.com_port = atoi (string) - 1;
            hidecur ();
            break;

         case 2:
            switch ((unsigned short)config.speed) {
               case 300:
                  config.speed = 1200;
                  break;
               case 1200:
                  config.speed = 2400;
                  break;
               case 2400:
                  config.speed = 4800;
                  break;
               case 4800:
                  config.speed = 9600;
                  break;
               case 9600:
                  config.speed = 19200;
                  break;
               case 19200:
                  config.speed = (unsigned short)38400U;
                  break;
               case (unsigned short)38400U:
                  config.speed = (unsigned short)57600U;
                  break;
               case (unsigned short)57600U:
                  config.speed = 300;
                  break;
            }
            break;

         case 3:
            config.lock_baud ^= 1;
            break;

         case 4:
            config.terminal ^= 1;
            break;

         case 5:
            strcpy (string, config.fax_response);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (5, 21, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.fax_response, strbtrim (string));
            break;

         case 6:
            sprintf (string, "%d", config.fax_errorlevel);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (6, 21, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.fax_errorlevel = atoi (strbtrim (string));
            break;

         case 7:
            config.stripdash ^= 1;
            break;

         case 8:
            sprintf (string, "%d", config.modem_timeout);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (8, 21, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.modem_timeout = atoi (strbtrim (string));
            break;

         case 9:
            sprintf (string, "%d", config.carrier_mask);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (9, 21, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.carrier_mask = atoi (strbtrim (string));
            break;

         case 10:
            sprintf (string, "%d", config.dcd_timeout);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (10, 21, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.dcd_timeout = atoi (strbtrim (string));
            break;
      }

      hidecur ();

   } while (i != -1);

   wclose ();
}

void modem_answer_control ()
{
   int wh, i = 1;
   char string[128], *p;

   wh = wopen (5, 20, 13, 61, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Answer control ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " Manual answer   ", 0, 1, 0, NULL, 0, 0);
      wmenuitem ( 2,  1, " Answer command  ", 0, 2, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " Limited hours   ", 0, 3, 0, NULL, 0, 0);
      wmenuitem ( 4,  1, " Ã Starting time ", 0, 4, 0, NULL, 0, 0);
      wmenuitem ( 5,  1, " À Ending time   ", 0, 5, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 19, CYAN|_BLACK, config.manual_answer ? "Yes" : "No");
      wprints (2, 19, CYAN|_BLACK, config.answer);
      wprints (3, 19, CYAN|_BLACK, config.limited_hours ? "Yes" : "No");
      sprintf (string, "%02d:%02d", config.start_time / 60, config.start_time % 60);
      wprints (4, 19, CYAN|_BLACK, string);
      sprintf (string, "%02d:%02d", config.end_time / 60, config.end_time % 60);
      wprints (5, 19, CYAN|_BLACK, string);

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            config.manual_answer ^= 1;
            break;

         case 2:
            strcpy (string, config.answer);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (2, 19, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.answer, strbtrim (string));
            break;

         case 3:
            config.limited_hours ^= 1;
            break;

         case 4:
            sprintf (string, "%02d:%02d", config.start_time / 60, config.start_time % 60);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 19, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               p = strtok (string, ":");
               if (p == NULL)
                  config.start_time = atoi (string) % 1440;
               else {
                  config.start_time = atoi (p) * 60;
                  p = strtok (NULL, "");
                  config.start_time += atoi (p);
               }
            }
            break;

         case 5:
            sprintf (string, "%02d:%02d", config.end_time / 60, config.end_time % 60);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (5, 19, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               p = strtok (string, ":");
               if (p == NULL)
                  config.end_time = atoi (p) % 1440;
               else {
                  config.end_time = atoi (p) * 60;
                  p = strtok (NULL, "");
                  config.end_time += atoi (p);
               }
            }
            break;
      }

      hidecur ();

   } while (i != -1);

   wclose ();
}

void modem_command_strings ()
{
   int wh, i = 1;
   char string[128];

   wh = wopen (6, 10, 16, 67, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Command strings ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " Init 1      ", 0,  1, 0, NULL, 0, 0);
      wmenuitem ( 2,  1, " Init 2      ", 0,  5, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " Init 3      ", 0,  6, 0, NULL, 0, 0);
      wmenuitem ( 4,  1, " Dial prefix ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 5,  1, " Dial suffix ", 0,  3, 0, NULL, 0, 0);
      wmenuitem ( 6,  1, " Offhook     ", 0,  4, 0, NULL, 0, 0);
      wmenuitem ( 7,  1, " Hangup      ", 0,  7, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 15, CYAN|_BLACK, config.init);
      wprints (2, 15, CYAN|_BLACK, config.init2);
      wprints (3, 15, CYAN|_BLACK, config.init3);
      wprints (4, 15, CYAN|_BLACK, config.dial);
      wprints (5, 15, CYAN|_BLACK, config.dial_suffix);
      wprints (6, 15, CYAN|_BLACK, config.modem_busy);
      wprints (7, 15, CYAN|_BLACK, config.hangup_string);

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            strcpy (string, config.init);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (1, 15, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.init, strbtrim (string));
            hidecur ();
            break;

         case 5:
            strcpy (string, config.init2);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (2, 15, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.init2, strbtrim (string));
            hidecur ();
            break;

         case 6:
            strcpy (string, config.init3);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 15, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.init3, strbtrim (string));
            hidecur ();
            break;

         case 2:
            strcpy (string, config.dial);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 15, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.dial, strbtrim (string));
            hidecur ();
            break;

         case 3:
            strcpy (string, config.dial_suffix);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (5, 15, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS);
               strcpy (config.dial_suffix, strbtrim (string));
            hidecur ();
            break;

         case 4:
            strcpy (string, config.modem_busy);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (6, 15, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.modem_busy, strbtrim (string));
            hidecur ();
            break;

         case 7:
            strcpy (string, config.hangup_string);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (7, 15, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.hangup_string, strbtrim (string));
            hidecur ();
            break;
      }

      hidecur ();

   } while (i != -1);

   wclose ();
}

void modem_dialing_strings ()
{
   int wh, i = 6;
   char string[128];

   wh = wopen (4, 10, 19, 67, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Dialing strings ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wprints (2, 11, LGREY|BLACK, "ÚÄPrefixÄÄÄÄÄÄÄÄÄÄÄÄ¿");
      wprints (2, 33, LGREY|BLACK, "ÚÄSuffixÄÄÄÄÄÄÄÄÄÄÄÄ¿");
      wmenuitem ( 3,  1, " Dial 1  ", 0,  6, 0, NULL, 0, 0);
      wmenuitem ( 4,  1, " Dial 2  ", 0,  7, 0, NULL, 0, 0);
      wmenuitem ( 5,  1, " Dial 3  ", 0,  8, 0, NULL, 0, 0);
      wmenuitem ( 6,  1, " Dial 4  ", 0,  9, 0, NULL, 0, 0);
      wmenuitem ( 7,  1, " Dial 5  ", 0, 10, 0, NULL, 0, 0);
      wmenuitem ( 8,  1, " Dial 6  ", 0, 11, 0, NULL, 0, 0);
      wmenuitem ( 9,  1, " Dial 7  ", 0, 12, 0, NULL, 0, 0);
      wmenuitem (10,  1, " Dial 8  ", 0, 13, 0, NULL, 0, 0);
      wmenuitem (11,  1, " Dial 9  ", 0, 14, 0, NULL, 0, 0);
      wmenuitem (11,  1, " Dial 10 ", 0, 15, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (3, 12, CYAN|_BLACK, config.altdial[0].prefix);
      wprints (3, 34, CYAN|_BLACK, config.altdial[0].suffix);
      wprints (4, 12, CYAN|_BLACK, config.altdial[1].prefix);
      wprints (4, 34, CYAN|_BLACK, config.altdial[1].suffix);
      wprints (5, 12, CYAN|_BLACK, config.altdial[2].prefix);
      wprints (5, 34, CYAN|_BLACK, config.altdial[2].suffix);
      wprints (6, 12, CYAN|_BLACK, config.altdial[3].prefix);
      wprints (6, 34, CYAN|_BLACK, config.altdial[3].suffix);
      wprints (7, 12, CYAN|_BLACK, config.altdial[4].prefix);
      wprints (7, 34, CYAN|_BLACK, config.altdial[4].suffix);
      wprints (8, 12, CYAN|_BLACK, config.altdial[5].prefix);
      wprints (8, 34, CYAN|_BLACK, config.altdial[5].suffix);
      wprints (9, 12, CYAN|_BLACK, config.altdial[6].prefix);
      wprints (9, 34, CYAN|_BLACK, config.altdial[6].suffix);
      wprints (10, 12, CYAN|_BLACK, config.altdial[7].prefix);
      wprints (10, 34, CYAN|_BLACK, config.altdial[7].suffix);
      wprints (11, 12, CYAN|_BLACK, config.altdial[8].prefix);
      wprints (11, 34, CYAN|_BLACK, config.altdial[8].suffix);
      wprints (12, 12, CYAN|_BLACK, config.altdial[9].prefix);
      wprints (12, 34, CYAN|_BLACK, config.altdial[9].suffix);

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 6:
         case 7:
         case 8:
         case 9:
         case 10:
         case 11:
         case 12:
         case 13:
         case 14:
         case 15:
            strcpy (string, config.altdial[i - 6].prefix);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (i - 3, 12, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               wprints (i - 3, 12, LGREY|_BLACK, string);
               strcpy (config.altdial[i - 6].prefix, strbtrim (string));
               strcpy (string, config.altdial[i - 6].suffix);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (i - 3, 34, string, "???????????????????", 0, 2, NULL, 0);
               if (winpread () != W_ESCPRESS) {
                  wprints (i - 3, 34, LGREY|_BLACK, string);
                  strcpy (config.altdial[i - 6].suffix, strbtrim (string));
               }
            }
            hidecur ();
            break;
      }

      hidecur ();

   } while (i != -1);

   wclose ();
}

void modem_flag_strings ()
{
   int wh, i = 1;
   char string[128];

   wh = wopen (4, 13, 18, 56, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Nodelist flags ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wprints (1, 11, LGREY|BLACK, "ÚÄFlagÄ¿");
      wprints (1, 19, LGREY|BLACK, "ÚÄPrefixÄÄÄÄÄÄÄÄÄÄÄÄ¿");
      wmenuitem ( 2,  1, " Dial 1  ", 0,  1, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " Dial 2  ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 4,  1, " Dial 3  ", 0,  3, 0, NULL, 0, 0);
      wmenuitem ( 5,  1, " Dial 4  ", 0,  4, 0, NULL, 0, 0);
      wmenuitem ( 6,  1, " Dial 5  ", 0,  5, 0, NULL, 0, 0);
      wmenuitem ( 7,  1, " Dial 6  ", 0,  6, 0, NULL, 0, 0);
      wmenuitem ( 8,  1, " Dial 7  ", 0,  7, 0, NULL, 0, 0);
      wmenuitem ( 9,  1, " Dial 8  ", 0,  8, 0, NULL, 0, 0);
      wmenuitem (10,  1, " Dial 9  ", 0,  9, 0, NULL, 0, 0);
      wmenuitem (11,  1, " Dial 10 ", 0, 10, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (2, 12, CYAN|_BLACK, config.prefixdial[0].flag);
      wprints (2, 20, CYAN|_BLACK, config.prefixdial[0].prefix);
      wprints (3, 12, CYAN|_BLACK, config.prefixdial[1].flag);
      wprints (3, 20, CYAN|_BLACK, config.prefixdial[1].prefix);
      wprints (4, 12, CYAN|_BLACK, config.prefixdial[2].flag);
      wprints (4, 20, CYAN|_BLACK, config.prefixdial[2].prefix);
      wprints (5, 12, CYAN|_BLACK, config.prefixdial[3].flag);
      wprints (5, 20, CYAN|_BLACK, config.prefixdial[3].prefix);
      wprints (6, 12, CYAN|_BLACK, config.prefixdial[4].flag);
      wprints (6, 20, CYAN|_BLACK, config.prefixdial[4].prefix);
      wprints (7, 12, CYAN|_BLACK, config.prefixdial[5].flag);
      wprints (7, 20, CYAN|_BLACK, config.prefixdial[5].prefix);
      wprints (8, 12, CYAN|_BLACK, config.prefixdial[6].flag);
      wprints (8, 20, CYAN|_BLACK, config.prefixdial[6].prefix);
      wprints (9, 12, CYAN|_BLACK, config.prefixdial[7].flag);
      wprints (9, 20, CYAN|_BLACK, config.prefixdial[7].prefix);
      wprints (10, 12, CYAN|_BLACK, config.prefixdial[8].flag);
      wprints (10, 20, CYAN|_BLACK, config.prefixdial[8].prefix);
      wprints (11, 12, CYAN|_BLACK, config.prefixdial[9].flag);
      wprints (11, 20, CYAN|_BLACK, config.prefixdial[9].prefix);

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
         case 2:
         case 3:
         case 4:
         case 5:
         case 6:
         case 7:
         case 8:
         case 9:
         case 10:
            strcpy (string, config.prefixdial[i - 1].flag);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (i + 1, 12, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               wprints (i + 1, 12, LGREY|_BLACK, string);
               strcpy (config.prefixdial[i - 1].flag, strbtrim (string));
               strcpy (string, config.prefixdial[i - 1].prefix);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (i + 1, 20, string, "???????????????????", 0, 2, NULL, 0);
               if (winpread () != W_ESCPRESS) {
                  wprints (i + 1, 20, LGREY|_BLACK, string);
                  strcpy (config.prefixdial[i - 1].prefix, strbtrim (string));
               }
            }
            break;
      }

      hidecur ();

   } while (i != -1);

   wclose ();
}

void general_time ()
{
   int wh, i = 1;
   char string[128];

   wh = wopen (6, 7, 15, 74, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Time adjustment ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " Init        ", 0,  1, 0, NULL, 0, 0);
      wmenuitem ( 2,  1, " Dial prefix ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " Dial suffix ", 0,  3, 0, NULL, 0, 0);
      wmenuitem ( 4,  1, " Number      ", 0,  4, 0, NULL, 0, 0);
      wmenuitem ( 5,  1, " ÀÄ Use DST  ", 0,  6, 0, NULL, 0, 0);
      wmenuitem ( 6,  1, " Nodes       ", 0,  5, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 15, CYAN|_BLACK, config.galileo_init);
      wprints (2, 15, CYAN|_BLACK, config.galileo_dial);
      wprints (3, 15, CYAN|_BLACK, config.galileo_suffix);
      wprints (4, 15, CYAN|_BLACK, config.galileo);
      wprints (5, 15, CYAN|_BLACK, config.solar ? "No" : "Yes");
      wprints (6, 15, CYAN|_BLACK, config.resync_nodes);

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            strcpy (string, config.galileo_init);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (1, 15, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.galileo_init, strbtrim (string));
            break;

         case 2:
            strcpy (string, config.galileo_dial);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (2, 15, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.galileo_dial, strbtrim (string));
            break;

         case 3:
            strcpy (string, config.galileo_suffix);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 15, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS);
               strcpy (config.galileo_suffix, strbtrim (string));
            break;

         case 4:
            strcpy (string, config.galileo);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 15, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.galileo, strbtrim (string));
            break;

         case 5:
            strcpy (string, config.resync_nodes);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (6, 15, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.resync_nodes, strbtrim (string));
            break;

         case 6:
            config.solar ^= 1;
            break;
      }

      hidecur ();

   } while (i != -1);

   wclose ();
}

void mailer_log ()
{
   int wh, i = 1;
   char string[128];

   wh = wopen (7, 15, 14, 72, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Log ", TRIGHT, YELLOW|_BLUE);

   do {
      hidecur ();
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " Name        ", 0,  1, 0, NULL, 0, 0);
      wmenuitem ( 2,  1, " Style       ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " Log flags   ", 0,  3, 0, NULL, 0, 0);
      wmenuitem ( 4,  1, " Buffer size ", 0,  4, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 15, CYAN|_BLACK, config.log_name);
      wprints (2, 15, CYAN|_BLACK, config.log_style ? "Terse" : "Verbose");
      wprints (3, 15, CYAN|_BLACK, config.doflagfile ? "Yes" : "No");
      sprintf (string, "%d", (int)config.logbuffer);
      wprints (4, 15, CYAN|_BLACK, string);

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            strcpy (string, config.log_name);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (1, 15, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.log_name, strbtrim (string));
            break;

         case 2:
            config.log_style = config.log_style ? 0 : 1;
            break;

         case 3:
            config.doflagfile = config.doflagfile ? 0 : 1;
            break;

         case 4:
            sprintf (string, "%d", (int)config.logbuffer);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 15, string, "??", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               if (atoi (string) > 64)
                  config.logbuffer = 64;
               else if (atoi (string) < 0)
                  config.logbuffer = 0;
               else
                  config.logbuffer = (char )atoi (string);
            }
            break;
      }

      hidecur ();

   } while (i != -1);

   wclose ();
}

void mailer_local_editor ()
{
   int wh, i = 1;
   char string[128];

   wh = wopen (8, 13, 12, 70, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Message editor ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " DOS command ", 0,  1, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 15, CYAN|_BLACK, config.local_editor);

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            strcpy (string, config.local_editor);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (1, 15, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.local_editor, strbtrim (string));
            hidecur ();
            break;
      }

      hidecur ();

   } while (i != -1);

   wclose ();
}

void mailer_filerequest ()
{
   int wh, i = 1;
   char string[128];

   wh = wopen (6, 8, 20, 76, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" File request ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " Request list          ", 0,  1, 0, NULL, 0, 0);
      wmenuitem ( 2,  1, " Request list (Secure) ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " Request list (Know)   ", 0,  3, 0, NULL, 0, 0);
      wmenuitem ( 4,  1, " About                 ", 0,  4, 0, NULL, 0, 0);
      wmenuitem ( 5,  1, " File list             ", 0,  5, 0, NULL, 0, 0);
      wmenuitem ( 6,  1, " Max size              ", 0,  6, 0, NULL, 0, 0);
      wmenuitem ( 7,  1, " Max size (Secure)     ", 0,  7, 0, NULL, 0, 0);
      wmenuitem ( 8,  1, " Max size (Know)       ", 0,  8, 0, NULL, 0, 0);
      wmenuitem ( 9,  1, " Max match             ", 0,  9, 0, NULL, 0, 0);
      wmenuitem (10,  1, " Max match (Secure)    ", 0, 10, 0, NULL, 0, 0);
      wmenuitem (11,  1, " Max match (Know)      ", 0, 11, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 25, CYAN|_BLACK, config.norm_okfile);
      wprints (2, 25, CYAN|_BLACK, config.prot_okfile);
      wprints (3, 25, CYAN|_BLACK, config.know_okfile);
      wprints (4, 25, CYAN|_BLACK, config.about);
      wprints (5, 25, CYAN|_BLACK, config.files);
      sprintf (string, "%d", config.norm_max_kbytes);
      wprints (6, 25, CYAN|_BLACK, string);
      sprintf (string, "%d", config.prot_max_kbytes);
      wprints (7, 25, CYAN|_BLACK, string);
      sprintf (string, "%d", config.know_max_kbytes);
      wprints (8, 25, CYAN|_BLACK, string);
      sprintf (string, "%d", config.norm_max_requests);
      wprints (9, 25, CYAN|_BLACK, string);
      sprintf (string, "%d", config.prot_max_requests);
      wprints (10, 25, CYAN|_BLACK, string);
      sprintf (string, "%d", config.know_max_requests);
      wprints (11, 25, CYAN|_BLACK, string);

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            strcpy (string, config.norm_okfile);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (1, 25, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.norm_okfile, strbtrim (string));
            hidecur ();
            break;

         case 2:
            strcpy (string, config.prot_okfile);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (2, 25, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.prot_okfile, strbtrim (string));
            hidecur ();
            break;

         case 3:
            strcpy (string, config.know_okfile);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 25, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.know_okfile, strbtrim (string));
            hidecur ();
            break;

         case 4:
            strcpy (string, config.about);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 25, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.about, strbtrim (string));
            hidecur ();
            break;

         case 5:
            strcpy (string, config.files);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (5, 25, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.files, strbtrim (string));
            hidecur ();
            break;

         case 6:
            sprintf (string, "%d", config.norm_max_kbytes);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (6, 25, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.norm_max_kbytes = atoi (string);
            hidecur ();
            break;

         case 7:
            sprintf (string, "%d", config.prot_max_kbytes);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (7, 25, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.prot_max_kbytes = atoi (string);
            hidecur ();
            break;

         case 8:
            sprintf (string, "%d", config.know_max_kbytes);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (8, 25, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.know_max_kbytes = atoi (string);
            hidecur ();
            break;

         case 9:
            sprintf (string, "%d", config.norm_max_requests);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (9, 25, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.norm_max_requests = atoi (string);
            hidecur ();
            break;

         case 10:
            sprintf (string, "%d", config.prot_max_requests);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (10, 25, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.prot_max_requests = atoi (string);
            hidecur ();
            break;

         case 11:
            sprintf (string, "%d", config.know_max_requests);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (11, 25, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.know_max_requests = atoi (string);
            hidecur ();
            break;
      }

      hidecur ();

   } while (i != -1);

   wclose ();
}

void qwk_setup ()
{
   int wh, i = 1;
   char string[128];

   wh = wopen (7, 13, 13, 72, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" QWK setup ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " Work dir      ", 0,  1, 0, NULL, 0, 0);
      wmenuitem ( 2,  1, " Packet name   ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " Max. messages ", 0,  3, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 17, CYAN|_BLACK, config.QWKDir);
      wprints (2, 17, CYAN|_BLACK, config.BBSid);
      sprintf (string, "%u", config.qwk_maxmsgs);
      wprints (3, 17, CYAN|_BLACK, string);

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            strcpy (string, config.QWKDir);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (1, 17, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (config.QWKDir, strbtrim (string));
               if (config.QWKDir[0] && config.QWKDir[strlen (config.QWKDir) - 1] != '\\')
                  strcat (config.QWKDir, "\\");
               create_path (config.QWKDir);
            }
            hidecur ();
            break;

         case 2:
            strcpy (string, config.BBSid);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (2, 17, string, "????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.BBSid, strbtrim (string));
            hidecur ();
            break;

         case 3:
            sprintf (string, "%u", config.qwk_maxmsgs);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 17, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.qwk_maxmsgs = atoi (string);
            hidecur ();
            break;
      }

      hidecur ();

   } while (i != -1);

   wclose ();
}

void areafix_linehelp (void)
{
   char *str = "";
   struct _menu_t *mt;

   mt = wmenumcurr ();
   if (mt == NULL)
      return;

   switch (mt->citem->tagid) {
      case 1:
         str = "Activate or deactivate the internal areafix processor.";
         break;
      case 2:
         str = "File to send in response to a %HELP request.";
         break;
      case 3:
         str = "Net/Nodes that are allowed to create new echomail areas.";
         break;
      case 4:
         str = "Net/Nodes that will be linked automatically to the new areas.";
         break;
      case 5:
         str = "Net/Nodes that receives a copy of the areafix response message.";
         break;
      case 6:
         str = "AREAS.BBS-type file name to use.";
         break;
      case 7:
         str = "Use the AREAS.BBS file if found.";
         break;
      case 8:
         str = "Update the AREAS.BBS file after any change made by Areafix.";
         break;
      case 9:
         str = "Allow the users to rescan any new linked areas.";
         break;
      case 10:
         str = "Level to allow the users to change an echomail tag name.";
         break;
      case 11:
         str = "Level to allow the users to make Areafix's requests as another node.";
         break;
   }

   clear_window ();
   prints (24, 1, LGREY|_BLACK, str);
}

void mailer_areafix ()
{
   int wh, i = 1;
   char string[128];

   wh = wopen (5, 3, 20, 76, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Areafix ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " Areafix active    ", 0,  1, 0, NULL, 0, 0);
      wmenuiba (areafix_linehelp, clear_window);
      wmenuitem ( 2,  1, " AREAS.BBS         ", 0,  6, 0, NULL, 0, 0);
      wmenuiba (areafix_linehelp, clear_window);
      wmenuitem ( 3,  1, " Help file name    ", 0,  2, 0, NULL, 0, 0);
      wmenuiba (areafix_linehelp, clear_window);
      wmenuitem ( 4,  1, " Creating nodes    ", 0,  3, 0, NULL, 0, 0);
      wmenuiba (areafix_linehelp, clear_window);
      wmenuitem ( 5,  1, " Auto link nodes   ", 0,  4, 0, NULL, 0, 0);
      wmenuiba (areafix_linehelp, clear_window);
      wmenuitem ( 6,  1, " Alert nodes       ", 0,  5, 0, NULL, 0, 0);
      wmenuiba (areafix_linehelp, clear_window);
      wmenuitem ( 7,  1, " Use AREAS.BBS     ", 0,  7, 0, NULL, 0, 0);
      wmenuiba (areafix_linehelp, clear_window);
      wmenuitem ( 8,  1, " Update AREAS.BBS  ", 0,  8, 0, NULL, 0, 0);
      wmenuiba (areafix_linehelp, clear_window);
      wmenuitem ( 9,  1, " Allow rescan      ", 0,  9, 0, NULL, 0, 0);
      wmenuiba (areafix_linehelp, clear_window);
      wmenuitem (10,  1, " Change TAG level  ", 0, 10, 0, NULL, 0, 0);
      wmenuiba (areafix_linehelp, clear_window);
      wmenuitem (11,  1, " Remote maint.     ", 0, 11, 0, NULL, 0, 0);
      wmenuiba (areafix_linehelp, clear_window);
      wmenuitem (12,  1, " Check zones       ", 0, 12, 0, NULL, 0, 0);
      wmenuiba (areafix_linehelp, clear_window);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 21, CYAN|_BLACK, config.areafix ? "Yes" : "No");
      wprints (2, 21, CYAN|_BLACK, config.areas_bbs);
      wprints (3, 21, CYAN|_BLACK, config.areafix_help);
      wprints (4, 21, CYAN|_BLACK, config.newareas_create);
      wprints (5, 21, CYAN|_BLACK, config.newareas_link);
      wprints (6, 21, CYAN|_BLACK, config.alert_nodes);
      wprints (7, 21, CYAN|_BLACK, config.use_areasbbs ? "Yes" : "No");
      wprints (8, 21, CYAN|_BLACK, config.write_areasbbs ? "Yes" : "No");
      wprints (9, 21, CYAN|_BLACK, config.allow_rescan ? "Yes" : "No");
      sprintf (string, "%d", config.afx_change_tag);
      wprints (10, 21, CYAN|_BLACK, string);
      sprintf (string, "%d", config.afx_remote_maint);
      wprints (11, 21, CYAN|_BLACK, string);
      wprints (12, 21, CYAN|_BLACK, config.check_echo_zone ? "Yes" : "No");

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            config.areafix ^= 1;
            break;

         case 7:
            config.use_areasbbs ^= 1;
            break;

         case 8:
            config.write_areasbbs ^= 1;
            break;

         case 9:
            config.allow_rescan ^= 1;
            break;

         case 12:
            config.check_echo_zone ^= 1;
            break;

         case 6:
            strcpy (string, config.areas_bbs);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (2, 21, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.areas_bbs, strbtrim (string));
            break;

         case 2:
            strcpy (string, config.areafix_help);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 21, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.areafix_help, strbtrim (string));
            break;

         case 3:
            strcpy (string, config.newareas_create);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 21, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.newareas_create, strbtrim (string));
            break;

         case 4:
            strcpy (string, config.newareas_link);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (5, 21, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.newareas_link, strbtrim (string));
            break;

         case 5:
            strcpy (string, config.alert_nodes);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (6, 21, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.alert_nodes, strbtrim (string));
            break;

         case 10:
            sprintf (string, "%d", config.afx_change_tag);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (10, 21, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.afx_change_tag = atoi (string) % 256;
            break;

         case 11:
            sprintf (string, "%d", config.afx_remote_maint);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (11, 21, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.afx_remote_maint = atoi (string) % 256;
            break;
      }

      hidecur ();

   } while (i != -1);

   wclose ();
}

void mailer_tic ()
{
   int wh, i = 1;
   char string[128];

   wh = wopen (6, 3, 17, 76, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" TIC ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " TIC active        ", 0,  1, 0, NULL, 0, 0);
      wmenuitem ( 2,  1, " Help file name    ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " Creating nodes    ", 0,  3, 0, NULL, 0, 0);
      wmenuitem ( 4,  1, " Auto link nodes   ", 0,  4, 0, NULL, 0, 0);
      wmenuitem ( 5,  1, " Alert nodes       ", 0,  5, 0, NULL, 0, 0);
      wmenuitem ( 6,  1, " Change TAG level  ", 0,  6, 0, NULL, 0, 0);
      wmenuitem ( 7,  1, " Remote maint.     ", 0,  7, 0, NULL, 0, 0);
      wmenuitem ( 8,  1, " Check zones       ", 0,  8, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 21, CYAN|_BLACK, config.tic_active ? "Yes" : "No");
      wprints (2, 21, CYAN|_BLACK, config.tic_help);
      wprints (3, 21, CYAN|_BLACK, config.tic_newareas_create);
      wprints (4, 21, CYAN|_BLACK, config.tic_newareas_link);
      wprints (5, 21, CYAN|_BLACK, config.tic_alert_nodes);
      sprintf (string, "%d", config.tic_change_tag);
      wprints (6, 21, CYAN|_BLACK, string);
      sprintf (string, "%d", config.tic_remote_maint);
      wprints (7, 21, CYAN|_BLACK, string);
      wprints (8, 21, CYAN|_BLACK, config.tic_check_zone ? "Yes" : "No");

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            config.tic_active ^= 1;
            break;

         case 2:
            strcpy (string, config.tic_help);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (2, 21, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.tic_help, strbtrim (string));
            break;

         case 3:
            strcpy (string, config.tic_newareas_create);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 21, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.tic_newareas_create, strbtrim (string));
            break;

         case 4:
            strcpy (string, config.tic_newareas_link);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 21, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.tic_newareas_link, strbtrim (string));
            break;

         case 5:
            strcpy (string, config.tic_alert_nodes);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (5, 21, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.tic_alert_nodes, strbtrim (string));
            break;

         case 6:
            sprintf (string, "%d", config.tic_change_tag);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (6, 21, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.tic_change_tag = atoi (string) % 256;
            break;

         case 7:
            sprintf (string, "%d", config.tic_remote_maint);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (7, 21, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.tic_remote_maint = atoi (string) % 256;
            break;

         case 8:
            config.tic_check_zone ^= 1;
            break;
      }

      hidecur ();

   } while (i != -1);

   wclose ();
}

void mailer_ext_processing ()
{
   int wh, i = 1;
   char string[128];

   wh = wopen (8, 5, 19, 74, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" External processing ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " Before import ", 0,  1, 0, NULL, 0, 0);
      wmenuitem ( 2,  1, " After import  ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " Before export ", 0,  3, 0, NULL, 0, 0);
      wmenuitem ( 4,  1, " After export  ", 0,  4, 0, NULL, 0, 0);
      wmenuitem ( 5,  1, " Before pack   ", 0,  7, 0, NULL, 0, 0);
      wmenuitem ( 6,  1, " After pack    ", 0,  8, 0, NULL, 0, 0);
      wmenuitem ( 7,  1, " Automaint     ", 0,  5, 0, NULL, 0, 0);
      wmenuitem ( 8,  1, " BBS batch     ", 0,  6, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 17, CYAN|_BLACK, config.pre_import);
      wprints (2, 17, CYAN|_BLACK, config.after_import);
      wprints (3, 17, CYAN|_BLACK, config.pre_export);
      wprints (4, 17, CYAN|_BLACK, config.after_export);
      wprints (5, 17, CYAN|_BLACK, config.pre_pack);
      wprints (6, 17, CYAN|_BLACK, config.after_pack);
      wprints (7, 17, CYAN|_BLACK, config.automaint);
      wprints (8, 17, CYAN|_BLACK, config.bbs_batch);

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            strcpy (string, config.pre_import);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (1, 17, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.pre_import, strbtrim (string));
            break;

         case 2:
            strcpy (string, config.after_import);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (2, 17, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.after_import, strbtrim (string));
            break;

         case 3:
            strcpy (string, config.pre_export);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 17, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.pre_export, strbtrim (string));
            break;

         case 4:
            strcpy (string, config.after_export);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 17, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.after_export, strbtrim (string));
            break;

         case 5:
            strcpy (string, config.automaint);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (7, 17, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.automaint, strbtrim (string));
            break;

         case 6:
            strcpy (string, config.bbs_batch);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (8, 17, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.bbs_batch, strbtrim (string));
            break;

         case 7:
            strcpy (string, config.pre_pack);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (5, 17, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.pre_pack, strbtrim (string));
            break;

         case 8:
            strcpy (string, config.after_pack);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (6, 17, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.after_pack, strbtrim (string));
            break;
      }
      
      hidecur ();

   } while (i != -1);

   wclose ();
}

void bbs_language ()
{
   int wh, i;
   char string[80];

   wh = wopen (2, 1, 21, 77, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Language ", TRIGHT, YELLOW|_BLUE);
   i = 80;

   do {
      stop_update ();
      wclear ();

      wprints (1, 12, LGREY|BLACK, "ÚFilename¿ÚÄÄÄÄÄÄDescriptionÄÄÄÄÄÄ¿ÚÄÄÄÄÄTextfiles pathÄÄÄÄÄÄ¿");

      wmenubegc ();
      wmenuitem ( 2, 1," Lang. 1  ", 'L', 80, 0, NULL, 0, 0);
      wmenuitem ( 3, 1," Lang. 2  ", 'L', 81, 0, NULL, 0, 0);
      wmenuitem ( 4, 1," Lang. 3  ", 'L', 82, 0, NULL, 0, 0);
      wmenuitem ( 5, 1," Lang. 4  ", 'L', 83, 0, NULL, 0, 0);
      wmenuitem ( 6, 1," Lang. 5  ", 'L', 84, 0, NULL, 0, 0);
      wmenuitem ( 7, 1," Lang. 6  ", 'L', 85, 0, NULL, 0, 0);
      wmenuitem ( 8, 1," Lang. 7  ", 'L', 86, 0, NULL, 0, 0);
      wmenuitem ( 9, 1," Lang. 8  ", 'L', 87, 0, NULL, 0, 0);
      wmenuitem (10, 1," Lang. 9  ", 'L', 88, 0, NULL, 0, 0);
      wmenuitem (11, 1," Lang. 10 ", 'L', 89, 0, NULL, 0, 0);
      wmenuitem (12, 1," Lang. 11 ", 'L', 90, 0, NULL, 0, 0);
      wmenuitem (13, 1," Lang. 12 ", 'L', 91, 0, NULL, 0, 0);
      wmenuitem (14, 1," Lang. 13 ", 'L', 92, 0, NULL, 0, 0);
      wmenuitem (15, 1," Lang. 14 ", 'L', 93, 0, NULL, 0, 0);
      wmenuitem (16, 1," Lang. 15 ", 'L', 94, 0, NULL, 0, 0);
      wmenuend (i, M_VERT, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      for (i = 0; i < 15; i++) {
         wprints (i + 2, 13, CYAN|_BLACK, config.language[i].basename);
         wprints (i + 2, 23, CYAN|_BLACK, config.language[i].descr);
         wprints (i + 2, 48, CYAN|_BLACK, config.language[i].txt_path);
      }

      start_update ();
      i = wmenuget ();
      if (i != -1) {
         i -= 80;
         strcpy (string, config.language[i].basename);
         winpbeg (BLUE|_GREEN, BLUE|_GREEN);
         winpdef (i + 2, 13, string, "????????", 0, 2, NULL, 0);
         if (winpread () != W_ESCPRESS) {
            strcpy (config.language[i].basename, strbtrim (string));
            wprints (i + 2, 13, CYAN|_BLACK, "        ");
            wprints (i + 2, 13, CYAN|_BLACK, config.language[i].basename);

            strcpy (string, config.language[i].descr);
//            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
//            winpdef (i + 2, 23, string, "???????????????????????", 0, 2, NULL, 0);
//            if (winpread () != W_ESCPRESS) {
            if (winputs (i + 2, 23, string, "???????????????????????", 1, '±', BLUE|_GREEN, BLUE|_GREEN) != W_ESCPRESS) {
               strcpy (config.language[i].descr, string);
               wprints (i + 2, 23, CYAN|_BLACK, "                       ");
               wprints (i + 2, 23, CYAN|_BLACK, config.language[i].descr);

               strcpy (string, config.language[i].txt_path);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (i + 2, 48, string, "?????????????????????????", 0, 2, NULL, 0);
               if (winpread () != W_ESCPRESS) {
                  strcpy (config.language[i].txt_path, strbtrim (string));
                  strcpy (config.language[i].txt_path, strbtrim (string));
                  if (config.language[i].txt_path[0] && config.language[i].txt_path[strlen (config.language[i].txt_path) - 1] != '\\')
                     strcat (config.language[i].txt_path, "\\");
                  create_path (config.language[i].txt_path);
                  wprints (i + 2, 48, CYAN|_BLACK, "            ");
                  wprints (i + 2, 48, CYAN|_BLACK, config.language[i].txt_path);
               }
            }
         }

         i += 80;
         hidecur ();
      }

   } while (i != -1);

   wclose ();
}

void bbs_files ()
{
   int fd, fdi, fdx, wh, wh1, i = 1, saved;
   char string[128], filename[80];
   long pos;
   struct _sys sys, bsys;
   struct _sys_idx sysidx;

   sprintf (string, "%sSYSFILE.DAT", config.sys_path);
   fd = sh_open (string, SH_DENYNONE, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
   if (fd == -1)
      return;
   memset ((char *)&sys.file_name, 0, SIZEOF_FILEAREA);
   read (fd, (char *)&sys.file_name, SIZEOF_FILEAREA);

   gotoxy_ (24, 1);
   clreol_ ();
   prints (24, 1, LGREY|_BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add New Area  L-List  D-Delete");
   prints (24, 1, YELLOW|_BLACK, "PgUp/PgDn");
   prints (24, 26, YELLOW|_BLACK, "E");
   prints (24, 34, YELLOW|_BLACK, "A");
   prints (24, 50, YELLOW|_BLACK, "L");
   prints (24, 58, YELLOW|_BLACK, "D");

   wh = wopen (1, 2, 22, 76, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" File areas ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wprints ( 1,  1, LGREY|_BLACK, " Number       ");
      wprints ( 1, 28, LGREY|_BLACK, " Short name ");
      wprints ( 2,  1, LGREY|_BLACK, " Name         ");
      wprints ( 3,  1, LGREY|_BLACK, " Download     ");
      wprints ( 4,  1, LGREY|_BLACK, " Upload       ");
      wprints ( 5,  1, LGREY|_BLACK, " File List    ");
      wprints ( 6,  1, LGREY|_BLACK, " Access priv. ");
      wprints ( 7,  1, LGREY|_BLACK, " A Flag       ");
      wprints ( 8,  1, LGREY|_BLACK, " B Flag       ");
      wprints ( 9,  1, LGREY|_BLACK, " C Flag       ");
      wprints (10,  1, LGREY|_BLACK, " D Flag       ");
      wprints ( 6, 28, LGREY|_BLACK, " Download ");
      wprints ( 7, 28, LGREY|_BLACK, " A Flag   ");
      wprints ( 8, 28, LGREY|_BLACK, " B Flag   ");
      wprints ( 9, 28, LGREY|_BLACK, " C Flag   ");
      wprints (10, 28, LGREY|_BLACK, " D Flag   ");
      wprints ( 6, 51, LGREY|_BLACK, " Upload ");
      wprints ( 7, 51, LGREY|_BLACK, " A Flag ");
      wprints ( 8, 51, LGREY|_BLACK, " B Flag ");
      wprints ( 9, 51, LGREY|_BLACK, " C Flag ");
      wprints (10, 51, LGREY|_BLACK, " D Flag ");
      wprints (11,  1, LGREY|_BLACK, " TIC Level    ");
      wprints (12,  1, LGREY|_BLACK, " A Flag       ");
      wprints (13,  1, LGREY|_BLACK, " B Flag       ");
      wprints (14,  1, LGREY|_BLACK, " C Flag       ");
      wprints (15,  1, LGREY|_BLACK, " D Flag       ");
      wprints (12, 28, LGREY|_BLACK, " Group    ");
      wprints (13, 28, LGREY|_BLACK, " Flags    ");
      wprints (14, 28, LGREY|_BLACK, " TIC Tag  ");
      wprints (16,  1, LGREY|_BLACK, " Forward 1    ");
      wprints (17,  1, LGREY|_BLACK, " Forward 2    ");
      wprints (18,  1, LGREY|_BLACK, " Forward 3    ");

      sprintf (string, "%d", sys.file_num);
      wprints (1, 16, CYAN|_BLACK, string);

      wprints (1, 41, CYAN|_BLACK, sys.short_name);

      sys.file_name[52] = '\0';
      wprints (2, 16, CYAN|_BLACK, sys.file_name);

      wprints (3, 16, CYAN|_BLACK, sys.filepath);

      wprints (4, 16, CYAN|_BLACK, sys.uppath);

      wprints (5, 16, CYAN|_BLACK, sys.filelist);

      wprints (6, 15, CYAN|_BLACK, get_priv_text (sys.file_priv));
      wprints (7, 16, CYAN|_BLACK, get_flagA_text ((sys.file_flags >> 24) & 0xFF));
      wprints (8, 16, CYAN|_BLACK, get_flagB_text ((sys.file_flags >> 16) & 0xFF));
      wprints (9, 16, CYAN|_BLACK, get_flagC_text ((sys.file_flags >> 8) & 0xFF));
      wprints (10, 16, CYAN|_BLACK, get_flagD_text (sys.file_flags & 0xFF));

      wprints (6, 38, CYAN|_BLACK, get_priv_text (sys.download_priv));
      wprints (7, 39, CYAN|_BLACK, get_flagA_text ((sys.download_flags >> 24) & 0xFF));
      wprints (8, 39, CYAN|_BLACK, get_flagB_text ((sys.download_flags >> 16) & 0xFF));
      wprints (9, 39, CYAN|_BLACK, get_flagC_text ((sys.download_flags >> 8) & 0xFF));
      wprints (10, 39, CYAN|_BLACK, get_flagD_text (sys.download_flags & 0xFF));

      wprints (6, 59, CYAN|_BLACK, get_priv_text (sys.upload_priv));
      wprints (7, 60, CYAN|_BLACK, get_flagA_text ((sys.upload_flags >> 24) & 0xFF));
      wprints (8, 60, CYAN|_BLACK, get_flagB_text ((sys.upload_flags >> 16) & 0xFF));
      wprints (9, 60, CYAN|_BLACK, get_flagC_text ((sys.upload_flags >> 8) & 0xFF));
      wprints (10, 60, CYAN|_BLACK, get_flagD_text (sys.upload_flags & 0xFF));

      wprints (14, 39, CYAN|_BLACK, sys.tic_tag);

      sprintf (string, "%d", sys.tic_level);
      wprints (11, 16, CYAN|_BLACK, string);

      wprints (12, 16, CYAN|_BLACK, get_flagA_text ((sys.tic_flags >> 24) & 0xFF));
      wprints (13, 16, CYAN|_BLACK, get_flagB_text ((sys.tic_flags >> 16) & 0xFF));
      wprints (14, 16, CYAN|_BLACK, get_flagC_text ((sys.tic_flags >> 8) & 0xFF));
      wprints (15, 16, CYAN|_BLACK, get_flagD_text (sys.tic_flags & 0xFF));

      sprintf (string, "%d", sys.file_sig);
      wprints (12, 39, CYAN|_BLACK, string);

      strcpy (string, "........");
      if (sys.freearea)
         string[0] = 'F';
      if (sys.file_restricted)
         string[1] = 'G';
      if (sys.no_global_search)
         string[2] = 'S';
      if (sys.no_filedate)
         string[3] = 'D';
      if (sys.norm_req)
         string[4] = 'N';
      if (sys.know_req)
         string[5] = 'K';
      if (sys.prot_req)
         string[6] = 'P';
      if (sys.cdrom)
         string[7] = 'C';
      wprints (13, 39, CYAN|_BLACK, string);

      wprints (16, 16, CYAN|_BLACK, sys.tic_forward1);
      wprints (17, 16, CYAN|_BLACK, sys.tic_forward2);
      wprints (18, 16, CYAN|_BLACK, sys.tic_forward3);

      start_update ();

      i = getxch ();
      if ( (i & 0xFF) != 0 )
         i &= 0xFF;

      switch (i) {
         // PgDn
         case 0x5100:
            read (fd, (char *)&sys.file_name, SIZEOF_FILEAREA);
            break;

         // PgUp
         case 0x4900:
            if (tell (fd) > SIZEOF_FILEAREA) {
               lseek (fd, -2L * SIZEOF_FILEAREA, SEEK_CUR);
               read (fd, (char *)&sys.file_name, SIZEOF_FILEAREA);
            }
            break;

         // E Edit
         case 'E':
         case 'e':
            saved = sys.file_num;

            file_edit_single_area (&sys);

            lseek (fd, -1L * SIZEOF_FILEAREA, SEEK_CUR);
            write (fd, (char *)&sys.file_name, SIZEOF_FILEAREA);

            if (saved != sys.file_num) {
               pos = tell (fd) - (long)SIZEOF_FILEAREA;
               close (fd);

               update_message ();

               sprintf (string, "%sSYSFILE.IDX", config.sys_path);
               while ((fdx = sh_open (string, SH_DENYRD, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE)) == -1)
                  ;
               sprintf (filename, "%sSYSFILE.BAK", config.sys_path);
               while ((fdi = sh_open (filename, SH_DENYRW, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE)) == -1)
                  ;
               sprintf (string, "%sSYSFILE.DAT", config.sys_path);
               while ((fd = sh_open (string, SH_DENYRD, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE)) == -1)
                  ;
               saved = 0;

               while (read (fd, (char *)&bsys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA) {
                  if (!saved && bsys.file_num > sys.file_num) {
                     pos = tell (fdi);
                     write (fdi, (char *)&sys.file_name, SIZEOF_FILEAREA);
                     memset ((char *)&sysidx, 0, sizeof (struct _sys_idx));
                     sysidx.priv = sys.file_priv;
                     sysidx.flags = sys.file_flags;
                     sysidx.sig = sys.file_sig;
                     sysidx.area = sys.file_num;
                     strcpy (sysidx.key, sys.short_name);
                     write (fdx, (char *)&sysidx, sizeof (struct _sys_idx));
                     saved = 1;
                  }
                  if (bsys.file_num != sys.file_num) {
                     write (fdi, (char *)&bsys.file_name, SIZEOF_FILEAREA);
                     memset ((char *)&sysidx, 0, sizeof (struct _sys_idx));
                     sysidx.priv = bsys.file_priv;
                     sysidx.flags = bsys.file_flags;
                     sysidx.area = bsys.file_num;
                     sysidx.sig = sys.file_sig;
                     strcpy (sysidx.key, bsys.short_name);
                     write (fdx, (char *)&sysidx, sizeof (struct _sys_idx));
                  }
               }

               if (!saved) {
                  pos = tell (fdi);
                  write (fdi, (char *)&sys.file_name, SIZEOF_FILEAREA);
                  memset ((char *)&sysidx, 0, sizeof (struct _sys_idx));
                  sysidx.priv = sys.file_priv;
                  sysidx.flags = sys.file_flags;
                  sysidx.area = sys.file_num;
                  sysidx.sig = sys.file_sig;
                  strcpy (sysidx.key, sys.short_name);
                  write (fdx, (char *)&sysidx, sizeof (struct _sys_idx));
               }

               lseek (fd, 0L, SEEK_SET);
               chsize (fd, 0L);
               lseek (fdi, 0L, SEEK_SET);

               while (read (fdi, (char *)&bsys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA)
                  write (fd, (char *)&bsys.file_name, SIZEOF_FILEAREA);

               close (fdx);
               close (fd);
               close (fdi);

               unlink (filename);

               sprintf (string, "%sSYSFILE.DAT", config.sys_path);
               while ((fd = sh_open (string, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE)) == -1)
                  ;
               lseek (fd, pos, SEEK_SET);
               read (fd, (char *)&sys.file_name, SIZEOF_FILEAREA);

               wclose ();
            }
            else {
               pos = tell (fd) - (long)SIZEOF_FILEAREA;
               i = (int)(pos / (long)SIZEOF_FILEAREA);

               sprintf (string, "%sSYSFILE.IDX", config.sys_path);
               while ((fdx = sh_open (string, SH_DENYNONE, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE)) == -1)
                  ;
               lseek (fdx, (long)i * sizeof (struct _sys_idx), SEEK_SET);
               memset ((char *)&sysidx, 0, sizeof (struct _sys_idx));
               sysidx.priv = sys.file_priv;
               sysidx.flags = sys.file_flags;
               sysidx.area = sys.file_num;
               sysidx.sig = sys.file_sig;
               strcpy (sysidx.key, sys.short_name);
               write (fdx, (char *)&sysidx, sizeof (struct _sys_idx));
               close (fdx);
            }

            break;

         // A Add
         case 'A':
         case 'a':
            memset ((char *)&sys, 0, sizeof (struct _sys));
            sys.file_priv = sys.download_priv = sys.upload_priv = TWIT;
            file_edit_single_area (&sys);

            if (sys.file_num && strcmp (sys.file_name, "")) {
               pos = tell (fd) - (long)SIZEOF_FILEAREA;
               close (fd);

               update_message ();

               sprintf (string, "%sSYSFILE.IDX", config.sys_path);
               while ((fdx = sh_open (string, SH_DENYRD, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE)) == -1)
                  ;
               sprintf (filename, "%sSYSFILE.BAK", config.sys_path);
               while ((fdi = sh_open (filename, SH_DENYRW, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE)) == -1)
                  ;
               sprintf (string, "%sSYSFILE.DAT", config.sys_path);
               while ((fd = sh_open (string, SH_DENYRD, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE)) == -1)
                  ;
               saved = 0;

               while (read (fd, (char *)&bsys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA) {
                  if (!saved && bsys.file_num > sys.file_num) {
                     pos = tell (fdi);
                     write (fdi, (char *)&sys.file_name, SIZEOF_FILEAREA);
                     memset ((char *)&sysidx, 0, sizeof (struct _sys_idx));
                     sysidx.priv = sys.file_priv;
                     sysidx.flags = sys.file_flags;
                     sysidx.area = sys.file_num;
                     sysidx.sig = sys.file_sig;
                     strcpy (sysidx.key, sys.short_name);
                     write (fdx, (char *)&sysidx, sizeof (struct _sys_idx));
                     saved = 1;
                  }
                  if (bsys.file_num != sys.file_num) {
                     write (fdi, (char *)&bsys.file_name, SIZEOF_FILEAREA);
                     memset ((char *)&sysidx, 0, sizeof (struct _sys_idx));
                     sysidx.priv = bsys.file_priv;
                     sysidx.flags = bsys.file_flags;
                     sysidx.area = bsys.file_num;
                     sysidx.sig = bsys.file_sig;
                     strcpy (sysidx.key, bsys.short_name);
                     write (fdx, (char *)&sysidx, sizeof (struct _sys_idx));
                  }
               }

               if (!saved) {
                  pos = tell (fdi);
                  write (fdi, (char *)&sys.file_name, SIZEOF_FILEAREA);
                  memset ((char *)&sysidx, 0, sizeof (struct _sys_idx));
                  sysidx.priv = sys.file_priv;
                  sysidx.flags = sys.file_flags;
                  sysidx.area = sys.file_num;
                  sysidx.sig = sys.file_sig;
                  strcpy (sysidx.key, sys.short_name);
                  write (fdx, (char *)&sysidx, sizeof (struct _sys_idx));
               }

               lseek (fd, 0L, SEEK_SET);
               chsize (fd, 0L);
               lseek (fdi, 0L, SEEK_SET);

               while (read (fdi, (char *)&bsys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA)
                  write (fd, (char *)&bsys.file_name, SIZEOF_FILEAREA);

               close (fdx);
               close (fd);
               close (fdi);

               unlink (filename);

               sprintf (string, "%sSYSFILE.DAT", config.sys_path);
               while ((fd = sh_open (string, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE)) == -1)
                  ;
               lseek (fd, pos, SEEK_SET);
               read (fd, (char *)&sys.file_name, SIZEOF_FILEAREA);

               wclose ();
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
               pos = tell (fd) - (long)SIZEOF_FILEAREA;
               close (fd);

               update_message ();

               sprintf (string, "%sSYSFILE.IDX", config.sys_path);
               while ((fdx = sh_open (string, SH_DENYRD, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE)) == -1)
                  ;
               sprintf (filename, "%sSYSFILE.BAK", config.sys_path);
               while ((fdi = sh_open (filename, SH_DENYRW, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE)) == -1)
                  ;
               sprintf (string, "%sSYSFILE.DAT", config.sys_path);
               while ((fd = sh_open (string, SH_DENYRD, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE)) == -1)
                  ;

               while (read (fd, (char *)&bsys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA) {
                  if (bsys.file_num != sys.file_num) {
                     write (fdi, (char *)&bsys.file_name, SIZEOF_FILEAREA);
                     memset ((char *)&sysidx, 0, sizeof (struct _sys_idx));
                     sysidx.priv = bsys.file_priv;
                     sysidx.flags = bsys.file_flags;
                     sysidx.area = bsys.file_num;
                     sysidx.sig = bsys.file_sig;
                     strcpy (sysidx.key, bsys.short_name);
                     write (fdx, (char *)&sysidx, sizeof (struct _sys_idx));
                  }
               }

               lseek (fd, 0L, SEEK_SET);
               chsize (fd, 0L);
               lseek (fdi, 0L, SEEK_SET);

               while (read (fdi, (char *)&bsys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA)
                  write (fd, (char *)&bsys.file_name, SIZEOF_FILEAREA);

               close (fdx);
               close (fd);
               close (fdi);

               unlink (filename);

               sprintf (string, "%sSYSFILE.DAT", config.sys_path);
               while ((fd = sh_open (string, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE)) == -1)
                  ;
               if (lseek (fd, pos, SEEK_SET) == -1 || pos == filelength (fd)) {
                  lseek (fd, -1L * SIZEOF_FILEAREA, SEEK_END);
                  pos = tell (fd);
               }
               read (fd, (char *)&sys.file_name, SIZEOF_FILEAREA);

               wclose ();
            }
            break;

         // L List
         case 'L':
         case 'l':
            file_select_area_list (fd, &sys);
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

static void file_edit_single_area (sys)
struct _sys *sys;
{
   int i = 1, wh1, m;
   char string[128];
   struct _sys nsys;

   memcpy ((char *)&nsys.file_name, (char *)sys->file_name, SIZEOF_FILEAREA);

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
      wmenuitem ( 1,  1, " Number       ", 0,  1, 0, NULL, 0, 0);
      wmenuitem ( 1, 28, " Short name ", 0, 23, 0, NULL, 0, 0);
      wmenuitem ( 2,  1, " Name         ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " Download     ", 0,  5, 0, NULL, 0, 0);
      wmenuitem ( 4,  1, " Upload       ", 0,  6, 0, NULL, 0, 0);
      wmenuitem ( 5,  1, " File List    ", 0,  7, 0, NULL, 0, 0);
      wmenuitem ( 6,  1, " Access priv. ", 0,  8, 0, NULL, 0, 0);
      wmenuitem ( 7,  1, " A Flag       ", 0,  9, 0, NULL, 0, 0);
      wmenuitem ( 8,  1, " B Flag       ", 0, 10, 0, NULL, 0, 0);
      wmenuitem ( 9,  1, " C Flag       ", 0, 11, 0, NULL, 0, 0);
      wmenuitem (10,  1, " D Flag       ", 0, 12, 0, NULL, 0, 0);
      wmenuitem ( 6, 28, " Download ", 0, 13, 0, NULL, 0, 0);
      wmenuitem ( 7, 28, " A Flag   ", 0, 14, 0, NULL, 0, 0);
      wmenuitem ( 8, 28, " B Flag   ", 0, 15, 0, NULL, 0, 0);
      wmenuitem ( 9, 28, " C Flag   ", 0, 16, 0, NULL, 0, 0);
      wmenuitem (10, 28, " D Flag   ", 0, 17, 0, NULL, 0, 0);
      wmenuitem ( 6, 51, " Upload ", 0, 18, 0, NULL, 0, 0);
      wmenuitem ( 7, 51, " A Flag ", 0, 19, 0, NULL, 0, 0);
      wmenuitem ( 8, 51, " B Flag ", 0, 20, 0, NULL, 0, 0);
      wmenuitem ( 9, 51, " C Flag ", 0, 21, 0, NULL, 0, 0);
      wmenuitem (10, 51, " D Flag ", 0, 22, 0, NULL, 0, 0);
      wmenuitem (11,  1, " TIC Level ", 0, 28, 0, NULL, 0, 0);
      wmenuitem (12,  1, " A Flag       ", 0, 29, 0, NULL, 0, 0);
      wmenuitem (13,  1, " B Flag       ", 0, 30, 0, NULL, 0, 0);
      wmenuitem (14,  1, " C Flag       ", 0, 31, 0, NULL, 0, 0);
      wmenuitem (15,  1, " D Flag       ", 0, 32, 0, NULL, 0, 0);
      wmenuitem (12, 28, " Group    ", 0,  4, 0, NULL, 0, 0);
      wmenuitem (13, 28, " Flags    ", 0,  3, 0, NULL, 0, 0);
      wmenuitem (14, 28, " TIC Tag  ", 0, 24, 0, NULL, 0, 0);
      wmenuitem (16,  1, " Forward 1    ", 0, 25, 0, NULL, 0, 0);
      wmenuitem (17,  1, " Forward 2    ", 0, 26, 0, NULL, 0, 0);
      wmenuitem (18,  1, " Forward 3    ", 0, 27, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      sprintf (string, "%d", nsys.file_num);
      wprints (1, 16, CYAN|_BLACK, string);

      wprints (1, 41, CYAN|_BLACK, nsys.short_name);

      nsys.file_name[52] = '\0';
      wprints (2, 16, CYAN|_BLACK, nsys.file_name);

      wprints (3, 16, CYAN|_BLACK, nsys.filepath);

      wprints (4, 16, CYAN|_BLACK, nsys.uppath);

      wprints (5, 16, CYAN|_BLACK, nsys.filelist);

      wprints (6, 15, CYAN|_BLACK, get_priv_text (nsys.file_priv));
      wprints (7, 16, CYAN|_BLACK, get_flagA_text ((nsys.file_flags >> 24) & 0xFF));
      wprints (8, 16, CYAN|_BLACK, get_flagB_text ((nsys.file_flags >> 16) & 0xFF));
      wprints (9, 16, CYAN|_BLACK, get_flagC_text ((nsys.file_flags >> 8) & 0xFF));
      wprints (10, 16, CYAN|_BLACK, get_flagD_text (nsys.file_flags & 0xFF));

      wprints (6, 38, CYAN|_BLACK, get_priv_text (nsys.download_priv));
      wprints (7, 39, CYAN|_BLACK, get_flagA_text ((nsys.download_flags >> 24) & 0xFF));
      wprints (8, 39, CYAN|_BLACK, get_flagB_text ((nsys.download_flags >> 16) & 0xFF));
      wprints (9, 39, CYAN|_BLACK, get_flagC_text ((nsys.download_flags >> 8) & 0xFF));
      wprints (10, 39, CYAN|_BLACK, get_flagD_text (nsys.download_flags & 0xFF));

      wprints (6, 59, CYAN|_BLACK, get_priv_text (nsys.upload_priv));
      wprints (7, 60, CYAN|_BLACK, get_flagA_text ((nsys.upload_flags >> 24) & 0xFF));
      wprints (8, 60, CYAN|_BLACK, get_flagB_text ((nsys.upload_flags >> 16) & 0xFF));
      wprints (9, 60, CYAN|_BLACK, get_flagC_text ((nsys.upload_flags >> 8) & 0xFF));
      wprints (10, 60, CYAN|_BLACK, get_flagD_text (nsys.upload_flags & 0xFF));

      wprints (14, 39, CYAN|_BLACK, nsys.tic_tag);

      sprintf (string, "%d", nsys.tic_level);
      wprints (11, 16, CYAN|_BLACK, string);

      wprints (12, 16, CYAN|_BLACK, get_flagA_text ((nsys.tic_flags >> 24) & 0xFF));
      wprints (13, 16, CYAN|_BLACK, get_flagB_text ((nsys.tic_flags >> 16) & 0xFF));
      wprints (14, 16, CYAN|_BLACK, get_flagC_text ((nsys.tic_flags >> 8) & 0xFF));
      wprints (15, 16, CYAN|_BLACK, get_flagD_text (nsys.tic_flags & 0xFF));

      sprintf (string, "%d", nsys.file_sig);
      wprints (12, 39, CYAN|_BLACK, string);

      strcpy (string, "........");
      if (nsys.freearea)
         string[0] = 'F';
      if (nsys.file_restricted)
         string[1] = 'G';
      if (nsys.no_global_search)
         string[2] = 'S';
      if (nsys.no_filedate)
         string[3] = 'D';
      if (nsys.norm_req)
         string[4] = 'N';
      if (nsys.know_req)
         string[5] = 'K';
      if (nsys.prot_req)
         string[6] = 'P';
      if (nsys.cdrom)
         string[7] = 'C';
      wprints (13, 39, CYAN|_BLACK, string);

      wprints (16, 16, CYAN|_BLACK, nsys.tic_forward1);
      wprints (17, 16, CYAN|_BLACK, nsys.tic_forward2);
      wprints (18, 16, CYAN|_BLACK, nsys.tic_forward3);

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            sprintf (string, "%d", nsys.file_num);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (1, 16, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nsys.file_num = atoi (strbtrim (string));
	    break;

         case 2:
            strcpy (string, nsys.file_name);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (2, 16, string, "????????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nsys.file_name, strbtrim (string));
            break;

         case 3:
            wh1 = wopen (6, 42, 17, 70, 1, LCYAN|_BLACK, CYAN|_BLACK);
            wactiv (wh1);
            wshadow (DGREY|_BLACK);
            wtitle (" Flags ", TRIGHT, YELLOW|_BLUE);
            m = 1;

            do {
               wmenubegc ();
               wmenuitem (1, 1, " No download limits ", 0, 1, 0, NULL, 0, 0);
               wmenuitem (2, 1, " Group restricted   ", 0, 5, 0, NULL, 0, 0);
               wmenuitem (3, 1, " No global search   ", 0, 6, 0, NULL, 0, 0);
               wmenuitem (4, 1, " No file date       ", 0, 7, 0, NULL, 0, 0);
               wmenuitem (5, 1, " Unknow can request ", 0, 2, 0, NULL, 0, 0);
               wmenuitem (6, 1, " Know can request   ", 0, 3, 0, NULL, 0, 0);
               wmenuitem (7, 1, " Prot can request   ", 0, 4, 0, NULL, 0, 0);
               wmenuitem (8, 1, " CD-ROM             ", 0, 8, 0, NULL, 0, 0);
               wmenuend (m, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

               wprints (1, 22, CYAN|_BLACK, nsys.freearea ? "Yes" : "No ");
               wprints (2, 22, CYAN|_BLACK, nsys.file_restricted ? "Yes" : "No ");
               wprints (3, 22, CYAN|_BLACK, nsys.no_global_search ? "Yes" : "No ");
               wprints (4, 22, CYAN|_BLACK, nsys.no_filedate ? "Yes" : "No ");
               wprints (5, 22, CYAN|_BLACK, nsys.norm_req ? "Yes" : "No ");
               wprints (6, 22, CYAN|_BLACK, nsys.know_req ? "Yes" : "No ");
               wprints (7, 22, CYAN|_BLACK, nsys.prot_req ? "Yes" : "No ");
               wprints (8, 22, CYAN|_BLACK, nsys.cdrom ? "Yes" : "No ");

               start_update ();
               m = wmenuget ();

               switch (m) {
                  case 1:
                     nsys.freearea = nsys.freearea ? 0 : 1;
                     break;
                  case 2:
                     nsys.norm_req = nsys.norm_req ? 0 : 1;
                     break;
                  case 3:
                     nsys.know_req = nsys.know_req ? 0 : 1;
                     break;
                  case 4:
                     nsys.prot_req = nsys.prot_req ? 0 : 1;
                     break;
                  case 5:
                     nsys.file_restricted = nsys.file_restricted ? 0 : 1;
                     break;
                  case 6:
                     nsys.no_global_search = nsys.no_global_search ? 0 : 1;
                     break;
                  case 7:
                     nsys.no_filedate = nsys.no_filedate ? 0 : 1;
                     break;
                  case 8:
                     nsys.cdrom = nsys.cdrom ? 0 : 1;
                     break;
               }

            } while (m != -1);

            wclose ();
            break;

         case 4:
            sprintf (string, "%d", nsys.file_sig);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (12, 39, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nsys.file_sig = atoi (strbtrim (string));
	    break;

         case 5:
            strcpy (string, nsys.filepath);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 16, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (nsys.filepath, strbtrim (string));
               if (nsys.filepath[0] && nsys.filepath[strlen (nsys.filepath) - 1] != '\\')
                  strcat (nsys.filepath, "\\");
               create_path (nsys.filepath);
            }
            break;

         case 6:
            strcpy (string, nsys.uppath);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 16, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (nsys.uppath, strbtrim (string));
               if (nsys.uppath[0] && nsys.uppath[strlen (nsys.uppath) - 1] != '\\')
                  strcat (nsys.uppath, "\\");
               create_path (nsys.uppath);
            }
            break;

         case 7:
            strcpy (string, nsys.filelist);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (5, 16, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nsys.filelist, strbtrim (string));
            break;

         case 8:
            nsys.file_priv = select_level (nsys.file_priv, 25, 6);
            break;

         case 9:
            nsys.file_flags = window_get_flags (4, 26, 1, nsys.file_flags);
            break;

         case 10:
            nsys.file_flags = window_get_flags (4, 26, 2, nsys.file_flags);
            break;

         case 11:
            nsys.file_flags = window_get_flags (4, 26, 3, nsys.file_flags);
            break;

         case 12:
            nsys.file_flags = window_get_flags (4, 26, 4, nsys.file_flags);
            break;

         case 13:
            nsys.download_priv = select_level (nsys.download_priv, 45, 6);
            break;

         case 14:
            nsys.download_flags = window_get_flags (4, 50, 1, nsys.download_flags);
            break;

         case 15:
            nsys.download_flags = window_get_flags (4, 50, 2, nsys.download_flags);
            break;

         case 16:
            nsys.download_flags = window_get_flags (4, 50, 3, nsys.download_flags);
            break;

         case 17:
            nsys.download_flags = window_get_flags (4, 50, 4, nsys.download_flags);
            break;

         case 18:
            nsys.upload_priv = select_level (nsys.upload_priv, 47, 6);
            break;

         case 19:
            nsys.upload_flags = window_get_flags (4, 66, 1, nsys.upload_flags);
            break;

         case 20:
            nsys.upload_flags = window_get_flags (4, 66, 2, nsys.upload_flags);
            break;

         case 21:
            nsys.upload_flags = window_get_flags (4, 66, 3, nsys.upload_flags);
            break;

         case 22:
            nsys.upload_flags = window_get_flags (4, 66, 4, nsys.upload_flags);
            break;

         case 23:
            strcpy (string, nsys.short_name);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (1, 41, string, "????????????", 0, 1, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nsys.short_name, string);
            break;

         case 24:
            strcpy (string, nsys.tic_tag);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (14, 39, string, "????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nsys.tic_tag, strbtrim (string));
            break;

         case 25:
            strcpy (string, nsys.tic_forward1);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (16, 16, string, "????????????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nsys.tic_forward1, strbtrim (string));
            break;

         case 26:
            strcpy (string, nsys.tic_forward2);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (17, 16, string, "????????????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nsys.tic_forward2, strbtrim (string));
            break;

         case 27:
            strcpy (string, nsys.tic_forward3);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (18, 16, string, "????????????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nsys.tic_forward3, strbtrim (string));
            break;

         case 28:
            sprintf (string, "%d", nsys.tic_level);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (11, 16, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nsys.tic_level = atoi (strbtrim (string));
            break;

         case 29:
            nsys.tic_flags = window_get_flags (9, 26, 1, nsys.tic_flags);
            break;

         case 30:
            nsys.tic_flags = window_get_flags (9, 26, 2, nsys.tic_flags);
            break;

         case 31:
            nsys.tic_flags = window_get_flags (9, 26, 3, nsys.tic_flags);
            break;

         case 32:
            nsys.tic_flags = window_get_flags (9, 26, 4, nsys.tic_flags);
            break;
      }

      hidecur ();
   } while (i != -1);

   if (memcmp ((char *)&nsys.file_name, (char *)sys->file_name, SIZEOF_FILEAREA)) {
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
         memcpy ((char *)sys->file_name, (char *)&nsys.file_name, SIZEOF_FILEAREA);
   }

   gotoxy_ (24, 1);
   clreol_ ();
   prints (24, 1, LGREY|_BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add New Area  L-List  D-Delete");
   prints (24, 1, YELLOW|_BLACK, "PgUp/PgDn");
   prints (24, 26, YELLOW|_BLACK, "E");
   prints (24, 34, YELLOW|_BLACK, "A");
   prints (24, 50, YELLOW|_BLACK, "L");
   prints (24, 58, YELLOW|_BLACK, "D");
}

static void file_select_area_list (fd, osys)
int fd;
struct _sys *osys;
{
   int wh, i, x, start;
   char string[80], **array;
   struct _sys sys;
   long pos;

   wh = wopen (4, 0, 20, 77, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Select area ", TRIGHT, YELLOW|_BLUE);

   wprints (0, 1, LGREY|_BLACK, "Area# Name                                                   Level");
   whline (1, 0, 76, 0, BLUE|_BLACK);

   gotoxy_ (24, 1);
   clreol_ ();
   prints (24, 1, LGREY|_BLACK, "-Move bar  ENTER-Select");
   prints (24, 1, YELLOW|_BLACK, "");
   prints (24, 14, YELLOW|_BLACK, "ENTER");

   i = (int)(filelength (fd) / SIZEOF_FILEAREA);
   i += 2;
   if ((array = (char **)malloc (i * sizeof (char *))) == NULL)
      return;

   pos = tell (fd) - SIZEOF_FILEAREA;
   lseek (fd, 0L, SEEK_SET);
   i = 0;
   start = 0;

   while (read (fd, (char *)&sys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA) {
      sprintf (string, " %-4d %-54.54s%-13s ", sys.file_num, sys.file_name, get_priv_text (sys.file_priv));
      array[i] = (char *)malloc (strlen (string) + 1);
      if (array[i] == NULL)
         break;
      if (sys.file_num == osys->file_num)
         start = i;
      strcpy (array[i++], string);
   }
   array[i] = NULL;

   x = wpickstr (7, 2, 19, 75, 5, LGREY|_BLACK, CYAN|_BLACK, BLUE|_LGREY, array, start, NULL);

   if (x == -1)
      lseek (fd, pos, SEEK_SET);
   else
      lseek (fd, (long)x * SIZEOF_FILEAREA, SEEK_SET);
   read (fd, (char *)osys->file_name, SIZEOF_FILEAREA);

   wclose ();

   i = 0;
   while (array[i] != NULL)
      free (array[i++]);
   free (array);

   gotoxy_ (24, 1);
   clreol_ ();
   prints (24, 1, LGREY|_BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add New Area  L-List  D-Delete");
   prints (24, 1, YELLOW|_BLACK, "PgUp/PgDn");
   prints (24, 26, YELLOW|_BLACK, "E");
   prints (24, 34, YELLOW|_BLACK, "A");
   prints (24, 50, YELLOW|_BLACK, "L");
   prints (24, 58, YELLOW|_BLACK, "D");
}

