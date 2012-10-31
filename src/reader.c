
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
#include <conio.h>
#include <string.h>
#include <ctype.h>
#include <alloc.h>
#include <io.h>
#include <fcntl.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>
#include <cxl\cxlkey.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

#ifdef __OS2__
void VioUpdate (void);
#endif

#define MAX_COLS  128

typedef unsigned bit;

typedef struct _ptrch {
   char *row;
   void *next;
   void *prev;
   int   len;
   bit   wrapped :1;
} CHAIN;

int ccol, ofsrow, crow, normcolor = LGREY|_BLACK;
char editrow[MAX_COLS], headline[MAX_COLS];
CHAIN *ptr, *top;

int isok (char c)
{
   return (isalnum (c) || c == '_');
}

char *my_strtok (char *s1, char *s2)
{
    static char *Ss;
    char *tok, *sp;

    if (s1 != NULL)
       Ss = s1;

    while (*Ss) {
        for (sp = s2; *sp; sp++)
            if (*sp == *Ss)
                break;
        if (*sp == 0)
            break;
        Ss++;
    }
    if (*Ss == 0)
        return (NULL);
    tok = Ss;
    while (*Ss) {
        for (sp = s2; *sp; sp++)
            if (*sp == *Ss) {
                Ss++;
                return (tok);
            }
        Ss++;
    }
    return (tok);
}

void display_screen_viewer (CHAIN *start, int rows, int startcol)
{
   int i;
   CHAIN *tmp;
   char tmpstr[MAX_COLS], *p;

   i = 0;
   tmp = ptr;
   while (tmp != start) {
      i++;
      tmp = tmp->next;
   }

   wgotoxy (0, 0);
   wclreol ();
   if (strlen (headline) > startcol)
      wprints (0, 0, RED|_LGREY, &headline[startcol]);

   wgotoxy (i + ofsrow, 0);
   wclreos ();

   for (; i < rows; i++) {
      strncpy (tmpstr, (char *)tmp->row, tmp->len);
      tmpstr[tmp->len] = '\0';
      if ((p = strchr (tmpstr, '\r')) != NULL)
         *p = '\0';
      if (strlen (tmpstr) >= startcol)
         wprints (i + ofsrow, 0, normcolor, &tmpstr[startcol]);
      if (tmp->next == NULL)
         break;
      tmp = (CHAIN *)tmp->next;
   }
}

int rowcount (CHAIN *start)
{
   int i;
   CHAIN *tmp;

   tmp = start;

   for (i = 0; i < 26; i++) {
      if (tmp->next == NULL)
         break;
      tmp = (CHAIN *)tmp->next;
   }

   return (i + 1);
}

char *strtrim(char *str)
{
   register int i;

   for (i = strlen (str) - 1; str[i] == ' ' && i >= 0; i--)
      ;
   str[i + 1] = '\0';

   return (str);
}

void build_ptr_list (CHAIN *ptr)
{
   char *p, *txtptr;
   CHAIN *tmp, *cpos;

   txtptr = ptr->row;
   if (*txtptr == '\r') {
      ptr->row = txtptr;
      ptr->len = 2;
   }
   else
      ptr->row = NULL;

   if ((p = my_strtok (txtptr, "\r")) == NULL) {
      ptr->row = txtptr;
      ptr->len = strlen (ptr->row);

      if (ptr->next != NULL) {
         cpos = ptr->next;
         ptr->next = NULL;
         do {
            tmp = cpos->next;
            free (cpos);
            cpos = tmp;
         } while (cpos != NULL);
      }

      return;
   }

   do {
      if (*p == '\0')
         break;
      if (*p == '\n')
         p++;

      if (ptr->row == NULL)
         ptr->row = p;
      else if (ptr->next != NULL) {
         tmp = ptr->next;
         tmp->prev = ptr;
         tmp->row = p;
         ptr->next = tmp;
         ptr->len = (int)(p - ptr->row);
         ptr = tmp;
      }
      else {
         if ((tmp = (CHAIN *)malloc (sizeof (CHAIN))) == NULL)
            break;
         tmp->wrapped = 0;
         tmp->next = NULL;
         tmp->prev = ptr;
         tmp->row = p;
         ptr->next = tmp;
         ptr->len = (int)(p - ptr->row);
         ptr = tmp;
      }
   } while ((p = my_strtok (NULL, "\r")) != NULL);

   ptr->len = strlen (ptr->row);

   if (ptr->next != NULL) {
      cpos = ptr->next;
      ptr->next = NULL;
      do {
         tmp = cpos->next;
         free (cpos);
         cpos = tmp;
      } while (cpos != NULL);
   }
}

void view_file (char *file, int rows, int cols)
{
   int fd, i, startcol, endcol;
   char *txtptr;
   unsigned int maxsize;
   CHAIN *tmp, *cpos;

   hidecur ();

   if ((fd = open (file, O_RDONLY|O_BINARY)) != -1) {
      maxsize = (unsigned int)filelength (fd);
      if ((txtptr = malloc (maxsize)) == NULL)
         return;
      memset (txtptr, 0, maxsize);
      read (fd, txtptr, maxsize);
      close (fd);
   }

   txtptr[maxsize] = '\0';

   top = (CHAIN *)malloc (sizeof (CHAIN));
   top->wrapped = 0;
   ptr = top;
   top->next = top->prev = NULL;
   top->row = txtptr;
   build_ptr_list (top);
   ptr = top;

   strncpy (headline, top->row, top->len);
   if (top->len > 2)
      headline[top->len - 2] = '\0';
   else
      headline[0] = '\0';
   top = top->next;
   free (ptr);
   top->prev = NULL;
   cpos = tmp = ptr = top;

   endcol = startcol = crow = ccol = 0;
   while (tmp != NULL) {
      if ((tmp->len - 2) > endcol)
         endcol = tmp->len - 2;
      tmp = tmp->next;
   }
   tmp = top;
   display_screen_viewer (ptr, rows, startcol);

   for (;;) {
#ifdef __OS2__
      VioUpdate ();
#endif
      while (!kbmhit ())
         release_timeslice ();

      i = getxch ();
      if ((i & 0xFF) == 0x1B)
         break;

      switch (i) {
         // Home
         case 0x4700:
            if (startcol) {
               startcol = 0;
               display_screen_viewer (ptr, rows, startcol);
            }
            break;

         // ^PgUp
         case 0x8400:
            if (ptr != top) {
               ptr = top;
               startcol = 0;
               display_screen_viewer (ptr, rows, startcol);
            }
            break;

         // PgUp
         case 0x4900:
            tmp = ptr;
            for (i = 0; i < rows; i++) {
               if (tmp->prev == NULL)
                  break;
               tmp = tmp->prev;
            }
            if (i >= rows) {
               ptr = tmp;
               display_screen_viewer (ptr, rows, startcol);
            }
            else if (ptr != top) {
               ptr = top;
               display_screen_viewer (ptr, rows, startcol);
            }
            break;

         // Up
         case 0x4800:
            if (ptr->prev != NULL) {
               ptr = ptr->prev;
               display_screen_viewer (ptr, rows, startcol);
            }
            break;

         // Down
         case 0x5000:
            if (rowcount (ptr) > rows && ptr->next != NULL) {
               ptr = ptr->next;
               display_screen_viewer (ptr, rows, startcol);
            }
            break;

         // PgDn
         case 0x5100:
            tmp = ptr;
            for (i = 0; i < rows; i++) {
               if (tmp->next == NULL)
                  break;
               tmp = tmp->next;
            }
            if (i >= rows) {
               ptr = tmp;
               display_screen_viewer (ptr, rows, startcol);
            }
            break;

         // End
         case 0x4F00:
            if (endcol > cols) {
               startcol = endcol - cols;
               display_screen_viewer (ptr, rows, startcol);
            }
            break;

         // ^PgDn
         case 0x7600:
            if (rowcount (ptr) > rows) {
               while (rowcount (ptr) > rows) {
                  ptr = ptr->next;
                  if (cpos->next != NULL)
                     cpos = cpos->next;
               }

               while (cpos->next != NULL) {
                  cpos = cpos->next;
                  crow++;
               }

               cpos = cpos->prev;
               crow--;

               ccol = cpos->len - 2;
               if (ccol > cols - 1)
                  startcol = ccol - cols + 1;
               else if (ccol < startcol)
                  startcol = ccol;

               display_screen_viewer (ptr, rows, startcol);
            }
            break;

         // ->
         case 0x4D00:
            if (startcol + cols < endcol) {
               startcol++;
               display_screen_viewer (ptr, rows, startcol);
            }
            break;

         // <-
         case 0x4B00:
            if (startcol) {
               startcol--;
               display_screen_viewer (ptr, rows, startcol);
            }
            break;

         default:
            break;
      }
   }

   ptr = tmp = top;

   for (;;) {
      tmp = tmp->next;
      free (ptr);
      if ((ptr = tmp) == NULL)
         break;
   }

   free (txtptr);
}

void display_history (int in)
{
   char string[88];

   if (in)
      sprintf (string, "%sINBOUND.HIS", config->sys_path);
   else
      sprintf (string, "%sOUTBOUND.HIS", config->sys_path);

   if (!dexists (string)) {
      wopen (11, 20, 15, 58, 0, BLACK|_LGREY, BLACK|_LGREY);
      wshadow (DGREY|_BLACK);
      wcenters (1, BLACK|_LGREY, "Unable to read history file");
      timer (20);
      wclose ();
      return;
   }

   normcolor = BLUE|_LGREY;
   wopen (7, 2, 22, 76, 0, RED|_LGREY, normcolor);
   wtitle (in ? "INBOUND HISTORY" : "OUTBOUND HISTORY", TLEFT, RED|_LGREY);
   wshadow (DGREY|_BLACK);

   ofsrow = 1;
   view_file (string, 14, 73);

   wclose ();
}

