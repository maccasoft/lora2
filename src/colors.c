#include <stdio.h>
#include <string.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"

void cls()
{
        if (snooping)
           change_attr(LGREY|_BLACK);

        if (usr.formfeed) {
                if (usr.ansi)
                        m_print("\x1B[2J\x1B[1;1f");
                if (usr.avatar && !local_mode)
                        BUFFER_BYTE(CTRLL);
                if (snooping)
                        wclear();
        }
        else if (!local_mode) {
                BUFFER_BYTE('\r');
                BUFFER_BYTE('\n');
                if (snooping)
                        wputs("\n");
	}

        if (!local_mode)
           UNBUFFER_BYTES ();
}

void change_attr(i)
int i;
{
	char s[30];

        if ((!usr.ansi && !usr.avatar) || !usr.color)
		return;

        if (snooping)
                wtextattr(i);

        if (usr.avatar && !local_mode) {
                BUFFER_BYTE(CTRLV);
                BUFFER_BYTE(CTRLA);
                BUFFER_BYTE(i & 0xFF);

                UNBUFFER_BYTES ();
        }
        else if (usr.ansi) {
		strcpy(s,"\033[0;");

		switch (i & 0x0F) {
		case BLACK:
			strcat(s,"30");
			break;
		case BLUE:
			strcat(s,"34");
			break;
		case GREEN:
			strcat(s,"32");
			break;
		case CYAN:
			strcat(s,"36");
			break;
		case RED:
			strcat(s,"31");
			break;
		case MAGENTA:
			strcat(s,"35");
			break;
		case BROWN:
			strcat(s,"33");
			break;
		case LGREY:
			strcat(s,"37");
			break;
		case LBLUE:
			strcat(s,"1;34");
			break;
		case LGREEN:
			strcat(s,"1;32");
			break;
		case LCYAN:
			strcat(s,"1;36");
			break;
		case LRED:
			strcat(s,"1;31");
			break;
		case LMAGENTA:
			strcat(s,"1;35");
			break;
		case YELLOW:
			strcat(s,"1;33");
			break;
		case WHITE:
			strcat(s,"1;37");
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

		strcat(s,"m");

                m_print(s);
	}
}

void del_line()
{
   if (usr.ansi)
      m_print("\x1B[2K");
   if (usr.avatar && !local_mode) {
      BUFFER_BYTE(CTRLV);
      BUFFER_BYTE(CTRLC);

      UNBUFFER_BYTES ();
   }
   if (snooping)
      wclreol();
}

void cpos(y, x)
int y, x;
{
        if (usr.ansi && !local_mode)
                m_print("\x1B[%d;%df",y,x);
        if (usr.avatar && !local_mode) {
                BUFFER_BYTE (CTRLV);
                BUFFER_BYTE (CTRLH);
                BUFFER_BYTE (y);
                BUFFER_BYTE (x);

                UNBUFFER_BYTES ();
        }
        if (snooping && (usr.avatar || usr.ansi))
                wgotoxy (y-1, x-1);
}

void cup(y)
int y;
{
   int a, b;

   if (snooping && (usr.avatar || usr.ansi))
   {
      wreadcur (&a, &b);
      a -= y;
      wgotoxy (a, b);
   }

   if (usr.ansi)
      m_print ("\x1B[%dA", y);
   else if (usr.avatar)
      for(;--y >= 0;)
      {
         SENDBYTE (CTRLV);
         SENDBYTE (CTRLD);
      }
}

void cdo(y)
int y;
{
   int a, b;

   if (snooping && (usr.avatar || usr.ansi))
   {
      wreadcur (&a, &b);
      a += y;
      wgotoxy (a, b);
   }

   if (usr.ansi)
      m_print ("\x1B[%dB", y);
   else if (usr.avatar)
      for(;--y >= 0;)
      {
         SENDBYTE (CTRLV);
         SENDBYTE (CTRLE);
      }
}

void cri(x)
int x;
{
   int a, b;

   if (snooping && (usr.avatar || usr.ansi))
   {
      wreadcur (&a, &b);
      b += x;
      wgotoxy (a, b);
   }

   if (usr.ansi)
      m_print ("\x1B[%dC", x);
   else if (usr.avatar)
      for(;--x >= 0;)
      {
         SENDBYTE (CTRLV);
         SENDBYTE (CTRLG);
      }
}

void cle(x)
int x;
{
   int a, b;

   if (snooping && (usr.avatar || usr.ansi))
   {
      wreadcur (&a, &b);
      b -= x;
      wgotoxy (a, b);
   }

   if (usr.ansi)
      m_print ("\x1B[%dD", x);
   else if (usr.avatar)
      for(;--x >= 0;)
      {
         SENDBYTE (CTRLV);
         SENDBYTE (CTRLF);
      }
}

