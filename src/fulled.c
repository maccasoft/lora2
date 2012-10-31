#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <ctype.h>
#include <alloc.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "lsetup.h"
#include "msgapi.h"
#include "sched.h"
#include "externs.h"
#include "prototyp.h"

#ifdef __OS2__
void VioUpdate (void);
#endif

#define MAX_COLS  128
#define MAX_SIZE  16384

char fulleditor = 0;

void m_print2(char *format, ...);
FILE *get_system_file (char *);

static int cx, cy;
static char *txtptr, *ptr, *screenrow[26], *oldrow[26], editrow[128];

void gotoxy_s (int x, int y)
{
   cpos (y + 5, x);
}

void clrscr_s (void)
{
   int i;

   for (i = 0; i < (usr.len - 5); i++) {
      gotoxy_s (1, i + 1);
      del_line ();
   }

   gotoxy_s (1, 1);
}

void display_screen (int toprow, int width, int len)
{
   int i, crow, m;
   char *p, linea[128], adjusted[26], *a;

   p = txtptr;
   crow = 0;

   for (i = 0; i < 26; i++) {
      oldrow[i] = screenrow[i];
      screenrow[i] = NULL;
      adjusted[i] = 0;
   }

   do {
      if (crow && *p == '\n')
         p++;
      if (crow >= toprow)
         screenrow[crow - toprow] = p;

      i = 0;
      while (i < width && *p != '\0' && *p != '\n')
         linea[i++] = *p++;
      linea[i] = '\0';

      if (i >= width && strchr (linea, ' ') != NULL) {
         while (*p != ' ') {
            p--;
            i--;
         }
         linea[i] = '\0';
         while (*p == ' ')
            p++;
      }

      if (crow >= toprow) {
         if ((crow - toprow + 1) >= 26)
            break;
      }

      crow++;
   } while (*p != '\0');

   crow = -1;

   for (i = 0; i < len; i++) {
      if (screenrow[i] == NULL)
         break;
      if (ptr >= screenrow[i] && (ptr < screenrow[i + 1] || screenrow[i + 1] == NULL))
         crow = i + 1;

      if (screenrow[i] != oldrow[i]) {
         if (i && !adjusted[i - 1]) {
            i--;
            m = (int)(screenrow[i + 1] - screenrow[i]);
            strncpy (linea, screenrow[i], m);
            if (linea[m - 1] == '\n')
               m--;
            linea[m] = '\0';
            gotoxy_s (1, i + 1);
            del_line ();

            if ((a = strchr (linea, '>')) == NULL)
               change_attr (CYAN|_BLACK);
            else if ((int)(linea - a) <= 5)
               change_attr (YELLOW|_BLACK);

            m_print ("%s", linea);
            i++;
         }

         if (screenrow[i + 1] == NULL)
            m = strlen (screenrow[i]);
         else
            m = (int)(screenrow[i + 1] - screenrow[i]);
         strncpy (linea, screenrow[i], m);
         if (linea[m - 1] == '\n')
            m--;
         linea[m] = '\0';
         gotoxy_s (1, i + 1);
         del_line ();

         if ((a = strchr (linea, '>')) == NULL)
            change_attr (CYAN|_BLACK);
         else if ((int)(linea - a) <= 5)
            change_attr (YELLOW|_BLACK);

         m_print ("%s", linea);
         adjusted[i] = 1;
      }
   }
   if (crow == -1)
      crow = i + 1;

   if (crow <= len) {
      crow--;

      if (screenrow[crow + 1] == NULL)
         m = strlen (screenrow[crow]);
      else
         m = (int)(screenrow[crow + 1] - screenrow[crow]);
      strncpy (linea, screenrow[crow], m);
      if (linea[m - 1] == '\n')
         m--;
      linea[m] = '\0';

      if (cy != crow)
         strcpy (editrow, linea);

      m = (int)(ptr - screenrow[crow]);

      if (!adjusted[crow] && strcmp (editrow, linea)) {
         if ((a = strchr (linea, '>')) == NULL)
            change_attr (CYAN|_BLACK);
         else if ((int)(linea - a) <= 5)
            change_attr (YELLOW|_BLACK);

         if (m) {
            if (m < cx)
               gotoxy_s (m, crow + 1);
            m_print ("%s", &linea[m - 1]);
         }
         else {
            gotoxy_s (m + 1, crow + 1);
            m_print ("%s", &linea[m]);
         }

         del_line ();
         strcpy (editrow, linea);

         if (linea[m] || m < cx)
            gotoxy_s (m + 1, crow + 1);
      }
      else {
         strcpy (editrow, linea);
         gotoxy_s (m + 1, crow + 1);
      }
   }

#ifdef __OS2__
   UNBUFFER_BYTES ();
   VioUpdate ();
#endif

   cx = m;
   cy = crow;
}

int readkey (void)
{
   int c = -1;

   if (!local_mode) {
      while (!CHAR_AVAIL ()) {
         if (!CARRIER)
            return (-1);

         if (local_kbd != -1)
            break;

         time_release ();
         release_timeslice ();
      }

      if (local_kbd == -1) {
         c = (unsigned char)TIMED_READ(1);
         if (c == 0x1B && TIMED_READ (5) == '[') {

reread:
            switch (TIMED_READ (5)) {
               // Freccia in su'
               case 'A':
                  return (0x4800);

               // Freccia in giu'
               case 'B':
                  return (0x5000);

               // Freccia a destra
               case 'C':
                  return (0x4D00);

               // Freccia a sinistra
               case 'D':
                  return (0x4B00);

               // Home
               case 'H':
                  return (0x4700);

               // End
               case 'K':
                  return (0x4F00);

               case '0':
               case '1':
               case '2':
               case '3':
               case '4':
               case '5':
               case '6':
               case '7':
               case '8':
               case '9':
                  goto reread;

               default:
                  return (-1);
            }
         }
      }
      else {
         c = local_kbd;
         local_kbd = -1;
      }
   }
   else {
      while (local_kbd == -1) {
         if (!CARRIER)
            return (-1);

         time_release ();
         release_timeslice ();
      }

      c = local_kbd;
      local_kbd = -1;
   }

   return (c);
}

int read_editor_help (void)
{
   FILE *fp;
   int i;
   char linea[128];

   strcpy (linea, text_path);
   strcat (linea, "FSHELP");

   if ((fp = get_system_file (linea)) == NULL) {
      strcpy (linea, config->glob_text_path);
      strcat (linea, "FSHELP");

      if ((fp = get_system_file (linea)) == NULL)
         return (0);
   }

   i = 6;

   while (fgets (linea, 120, fp) != NULL) {
      while (strlen (linea) > 0 && (linea[strlen (linea) - 1] == 0x0D || linea[strlen (linea) - 1] == 0x0A))
         linea[strlen (linea) -1] = '\0';

      cpos (i++, (usr.width ? (usr.width - 1) : 79) - strlen (linea));
      m_print ("%s", linea);
   }

   fclose (fp);
   input (linea, 0);

   return (-1);
}

void edit_file (char *name, int len, int width)
{
   int fd, i, startrow, m;
   char insert, *p, savefile = 0, endrun = 0;

#ifdef __OS2__
   if ((txtptr = malloc (MAX_SIZE)) == NULL)
      return;
#else
   if ((txtptr = farmalloc (MAX_SIZE)) == NULL)
      return;
#endif

   memset (txtptr, 0, MAX_SIZE);

   for (i = 0; i < 26; i++) {
      oldrow[i] = NULL;
      screenrow[i] = NULL;
   }

   if ((fd = sh_open (name, SH_DENYRW, O_RDONLY|O_TEXT, S_IREAD|S_IWRITE)) != -1) {
      i = read (fd, txtptr, MAX_SIZE - 1);
      txtptr[i] = '\0';
      close (fd);

      unlink (name);

      ptr = txtptr;
      while (*ptr) {
         if (*ptr == 0x01)
            *ptr = '@';
         ptr++;
      }
   }

   clrscr_s ();
   ptr = txtptr;
   startrow = 0;
   width--;
   insert = 1;

   editrow[0] = '\0';
   display_screen (startrow, width, len);

   while (!endrun && CARRIER) {
      if ((i = readkey ()) == -1)
         continue;

      if (i == 0x1A || i == 0x2C1A) {
         savefile = 1;
         break;
      }

      switch (i) {
         // Home
         case 0x4700:
            if (ptr != screenrow[cy]) {
               ptr = screenrow[cy];
               display_screen (startrow, width, len);
            }
            break;

         // End
         case 0x4F00:
            if (screenrow[cy + 1] == NULL) {
               p = strchr (screenrow[cy], '\0');
               if (ptr != p) {
                  ptr = p;
                  display_screen (startrow, width, len);
               }
            }
            else if (ptr != screenrow[cy + 1] - 1) {
               ptr = screenrow[cy + 1] - 1;
               display_screen (startrow, width, len);
            }
            break;

         // Freccia su'
         case CTRLE:
         case 0x4800:
            if (cy <= 0) {
               if (startrow <= 0)
                  break;
               if (startrow >= 10)
                  startrow -= 10;
               else
                  startrow = 0;
            }
            else
               cy--;

            if (screenrow[cy + 1] == NULL)
               m = strlen (screenrow[cy]);
            else
               m = (int)(screenrow[cy + 1] - screenrow[cy]);
            if (m && screenrow[cy][m - 1] == '\n')
               m--;

            if (cx > m)
               cx = m;
            ptr = screenrow[cy] + cx;
            cy++;
            display_screen (startrow, width, len);
            break;

         // Freccia giu'
         case CTRLX:
         case 0x5000:
            if (screenrow[cy + 1] == NULL)
               break;

            if (cy >= (len - 1)) {
               startrow += 10;
               clrscr_s ();
            }
            cy++;

            if (screenrow[cy + 1] == NULL)
               m = strlen (screenrow[cy]);
            else
               m = (int)(screenrow[cy + 1] - screenrow[cy]);
            if (m && screenrow[cy][m - 1] == '\n')
               m--;

            if (cx > m)
               cx = m;
            ptr = screenrow[cy] + cx;
            cy--;
            display_screen (startrow, width, len);
            break;

         // Freccia sinistra
         case CTRLS:
         case 0x4B00:
            if (cx && ptr > txtptr) {
               ptr--;
               display_screen (startrow, width, len);
            }
            break;

         // Freccia destra
         case CTRLD:
         case 0x4D00:
            if (ptr + 1 < screenrow[cy + 1]) {
               ptr++;
               display_screen (startrow, width, len);
            }
            break;

         // CTRL-Y
         case 0x19:
         case 0x1519:
            if (screenrow[cy + 1] == NULL) {
               if (*ptr != '\0') {
                  *ptr = '\0';
                  display_screen (startrow, width, len);
               }
            }
            else {
               strcpy (screenrow[cy], screenrow[cy + 1]);
               display_screen (startrow, width, len);
            }
            break;

         // Delete
         case 0x5300:
            if (*ptr) {
               strcpy (ptr, &ptr[1]);
               display_screen (startrow, width, len);
               for (i = cy + 1; i < 26; i++) {
                  if (screenrow[i] == NULL)
                     break;
                  screenrow[i]--;
               }
            }
            break;

         // Backspace
         case 0x08:
         case 0x0E08:
            if (ptr > txtptr) {
               ptr--;
               strcpy (ptr, &ptr[1]);
               for (i = cy + 1; i < 26; i++) {
                  if (screenrow[i] == NULL)
                     break;
                  screenrow[i]--;
               }
               if (cy <= 0 && cx <= 0) {
                  if (startrow >= 10)
                     startrow -= 10;
                  else
                     startrow = 0;
                  clrscr_s ();
                  display_screen (startrow, width, len);
               }
               else
                  display_screen (startrow, width, len);
            }
            break;

         // Caratteri speciali (^K)
         case CTRLK:
            cpos (5, 2);
            m_print2 ("œ^K");

            while ((i = readkey ()) == -1) {
               if (!CARRIER)
                  break;
            }

            i &= 0xFF;
            i = toupper (i);

            if (i != 0x1B) {
               if (i < 32)
                  m_print2 ("^%c", i + 0x40);
               else
                  m_print2 ("%c", i);
            }

            switch (i) {
               case CTRLS:
               case 'S':
                  endrun = 1;
                  savefile = 1;
                  break;

               case CTRLQ:
               case 'Q':
                  endrun = 1;
                  savefile = 0;
                  break;

               case '?':
                  if (!read_editor_help ())
                     break;
                  clrscr_s ();
                  for (i = 0; i < 26; i++)
                     screenrow[i] = NULL;
                  break;
            }

            cpos (5, 2);
            m_print2 ("œ    ");

            display_screen (startrow, width, len);
            break;

         // Redraw dello schermo
         case 0x0C:
            cls ();

            change_attr (BLUE|_LGREY);
            del_line ();
            m_print (" * %s\n", sys.msg_name);

            msg_attrib (&msg, last_msg + 1, 0, 0);

            change_attr (RED|_BLUE);
            del_line ();
            cpos (5, (usr.width ? usr.width : 80) - 18);
            m_print ("œ^Z=Save  ^K?=Help");

            change_attr (CYAN|_BLACK);
            m_print (bbstxt[B_ONE_CR]);

            for (i = 0; i < 26; i++)
               screenrow[i] = NULL;
            display_screen (startrow, width, len);
            break;

         // Caratteri speciali (^Q)
         case CTRLQ:
            cpos (5, 2);
            m_print2 ("œ^Q");

            while ((i = readkey ()) == -1) {
               if (!CARRIER)
                  break;
            }

            i &= 0xFF;
            i = toupper (i);

            if (i != 0x1B) {
               if (i < 32)
                  m_print2 ("^%c", i + 0x40);
               else
                  m_print2 ("%c", i);
            }

            switch (i) {
               case CTRLS:
               case 'S':
                  if (ptr != screenrow[cy])
                     ptr = screenrow[cy];
                  break;

               case CTRLD:
               case 'D':
                  if (ptr != screenrow[cy + 1] - 1)
                     ptr = screenrow[cy + 1] - 1;
                  break;
            }

            cpos (5, 2);
            m_print2 ("œ    ");

            display_screen (startrow, width, len);
            break;

         // Enter
         case 0x0D:
         case 0x1C0D:
            i = '\n';
            // Fall through

         // Carattere normale
         default:
            if ((i &= 0xFF) < 32 && i != '\n')
               break;

            if (insert) {
               p = &ptr[strlen (ptr)];
               while (p >= ptr) {
                  *(p + 1) = *p;
                  p--;
               }

               *ptr++ = i;

               for (i = cy + 1; i < 26; i++) {
                  if (screenrow[i] == NULL)
                     break;
                  screenrow[i]++;
               }
            }
            else {
               if (*ptr == '\0') {
                  *ptr++ = i;
                  *ptr = '\0';
               }
               else
                  *ptr++ = i;
            }
            display_screen (startrow, width, len);
            if (cy >= (len)) {
               startrow += 10;
               clrscr_s ();
               display_screen (startrow, width, len);
            }
            break;
      }
   }

   cls ();

   if (savefile) {
      if ((fd = sh_open (name, SH_DENYRW, O_WRONLY|O_TEXT|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE)) != -1) {
         write (fd, txtptr, strlen (txtptr));
         close (fd);
      }
   }

#ifdef __OS2__
   free (txtptr);
#else
   farfree (txtptr);
#endif
}

void fullscreen_editor (void)
{
   cls();

   change_attr (BLUE|_LGREY);
   del_line ();
   m_print (" * %s\n", sys.msg_name);

   msg_attrib (&msg, last_msg + 1, 0, 0);

   change_attr (RED|_BLUE);
   del_line ();
   cpos (5, (usr.width ? usr.width : 80) - 18);
   m_print ("œ^Z=Save  ^K?=Help");

   change_attr (CYAN|_BLACK);
   m_print (bbstxt[B_ONE_CR]);

   fulleditor = 1;
   XON_DISABLE ();
   _BRK_DISABLE ();

   edit_file ("MSGTMP", usr.len - 5, usr.width ? (usr.width - 1) : 79);

   fulleditor = 0;
   XON_ENABLE ();
   _BRK_ENABLE ();
}


