
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
#include <string.h>
#include <stdarg.h>
#include <alloc.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

extern short tcpip;

static byte last_color = 0x07;

static void ansi_print(char *format, ...);

void cls (void)
{
   byte pc;

   if (snooping) {
      pc = last_color;
      change_attr(LGREY|_BLACK);
      last_color = pc;
   }

   if (usr.ansi || usr.avatar) {
      if (!local_mode) {
			if (usr.ansi && (!usr.formfeed || tcpip))
				ansi_print ("\x1B[2J\x1B[1;1f");
			else
				BUFFER_BYTE (CTRLL);
      }
      if (snooping)
         wclear ();
   }
   else {
      if (!local_mode) {
         BUFFER_BYTE('\r');
         BUFFER_BYTE('\n');
      }
      if (snooping) {
         wputc ('\r');
         wputc ('\n');
      }
   }

   restore_last_color ();

   if (!local_mode)
      UNBUFFER_BYTES ();
}

#define  NORM   0x07
#define  BOLD   0x08
#define  FAINT  0xF7
#define  VBLINK 0x80
#define  REVRS  0x77

void change_attr(i)
int i;
{
   char s[30];

   if ((!usr.ansi && !usr.avatar) || !usr.color)
      return;

   last_color = i;
   if ((snooping || local_mode))
      wtextattr (i);

   if (usr.avatar && !local_mode) {
      BUFFER_BYTE (CTRLV);
      BUFFER_BYTE (CTRLA);
      BUFFER_BYTE (i & 0xFF);

      UNBUFFER_BYTES ();
   }
   else if (usr.ansi && !local_mode) {
      strcpy(s,"\033[0");

      if (i & 0x80)
         strcat (s, ";5");

      switch (i & 0x0F) {
         case BLACK:
            strcat(s,";30");
            break;
         case BLUE:
            strcat(s,";34");
            break;
         case GREEN:
            strcat(s,";32");
            break;
         case CYAN:
            strcat(s,";36");
            break;
         case RED:
            strcat(s,";31");
            break;
         case MAGENTA:
            strcat(s,";35");
            break;
         case BROWN:
            strcat(s,";33");
            break;
         case LGREY:
            strcat(s,";37");
            break;
         case DGREY:
            strcat(s,";1;30");
            break;
         case LBLUE:
            strcat(s,";1;34");
            break;
         case LGREEN:
            strcat(s,";1;32");
            break;
         case LCYAN:
            strcat(s,";1;36");
            break;
         case LRED:
            strcat(s,";1;31");
            break;
         case LMAGENTA:
            strcat(s,";1;35");
            break;
         case YELLOW:
            strcat(s,";1;33");
            break;
         case WHITE:
            strcat(s,";1;37");
            break;
      }

      switch (i & 0xF0) {
         case _BLACK:
            strcat(s,";40");
            break;
         case _BLUE:
            strcat(s,";44");
            break;
         case _GREEN:
            strcat(s,";42");
            break;
         case _CYAN:
            strcat(s,";46");
            break;
         case _RED:
            strcat(s,";41");
            break;
         case _MAGENTA:
            strcat(s,";45");
            break;
         case _BROWN:
            strcat(s,";43");
            break;
         case _LGREY:
            strcat(s,";47");
            break;
      }

      ansi_print ("%sm", s);
   }
}

void del_line()
{
   if (usr.ansi && !local_mode)
      ansi_print("\x1B[K");
   if (usr.avatar && !local_mode) {
      BUFFER_BYTE(CTRLV);
      BUFFER_BYTE(CTRLG);

      UNBUFFER_BYTES ();
   }
   if (snooping)
      wclreol();
}

void cpos(y, x)
int y, x;
{
   if (usr.ansi && !local_mode)
      ansi_print("\x1B[%d;%df",y,x);
   if (usr.avatar && !local_mode) {
      BUFFER_BYTE (CTRLV);
      BUFFER_BYTE (CTRLH);
      BUFFER_BYTE (y);
      BUFFER_BYTE (x);

      UNBUFFER_BYTES ();
   }
   if (snooping && (usr.avatar || usr.ansi))
		wgotoxy ( y ? y-1 : y ,  x ? x-1 : x);
	}

void cup(y)
int y;
{
   int a, b;

   if (snooping && (usr.avatar || usr.ansi)) {
      wreadcur (&a, &b);
      a -= y;
      wgotoxy (a, b);
   }

   if (usr.ansi && !local_mode)
      ansi_print ("\x1B[%dA", y);
   else if (usr.avatar && !local_mode)
      for(;--y >= 0;)
      {
         SENDBYTE (CTRLV);
         SENDBYTE (CTRLC);
      }
}

void cdo(y)
int y;
{
   int a, b;

   if (snooping && (usr.avatar || usr.ansi)) {
      wreadcur (&a, &b);
      a += y;
      wgotoxy (a, b);
   }

   if (usr.ansi && !local_mode)
      ansi_print ("\x1B[%dB", y);
   else if (usr.avatar && !local_mode)
      for(;--y >= 0;) {
         SENDBYTE (CTRLV);
         SENDBYTE (CTRLD);
      }
}

void cri(x)
int x;
{
   int a, b;

   if (snooping && (usr.avatar || usr.ansi)) {
      wreadcur (&a, &b);
      b += x;
      wgotoxy (a, b);
   }

   if (usr.ansi && !local_mode)
      ansi_print ("\x1B[%dC", x);
   else if (usr.avatar && !local_mode) {
      if (x > 6) {
         SENDBYTE (CTRLV);
         SENDBYTE (CTRLY);
         SENDBYTE (2);
         SENDBYTE (CTRLV);
         SENDBYTE (CTRLF);
         SENDBYTE (x);
      }
      else
         for(;--x >= 0;) {
            SENDBYTE (CTRLV);
            SENDBYTE (CTRLF);
         }
   }
}

void cle(x)
int x;
{
   int a, b;

   if (snooping && (usr.avatar || usr.ansi)) {
      wreadcur (&a, &b);
      b -= x;
      wgotoxy (a, b);
   }

   if (usr.ansi && !local_mode)
      ansi_print ("\x1B[%dD", x);
   else if (usr.avatar && !local_mode)
      for(;--x >= 0;) {
         SENDBYTE (CTRLV);
         SENDBYTE (CTRLE);
      }
}

void restore_last_color ()
{
   change_attr (last_color);
}

static void ansi_print(char *format, ...)
{
   va_list var_args;
   char *string, *q, visual;

   string = (char *)malloc (256);

   if (string == NULL || strlen (format) > 256) {
      if (string)
         free (string);
      return;
   }

   va_start (var_args, format);
   vsprintf (string, format, var_args);
   va_end (var_args);

   visual = 1;

   for (q=string; *q; q++) {
      if (*q == 0x1B)
         visual = 0;

      if (!local_mode)
         BUFFER_BYTE ((*q));
      if (snooping && visual)
         wputc (*q);
   }

   if (!local_mode)
      UNBUFFER_BYTES ();

   free (string);
}

