
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
#include <time.h>
#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <dos.h>
#include <dir.h>
#include <stdarg.h>
#include <alloc.h>
#include <stdlib.h>
#include <sys\stat.h>

#ifdef __OS2__
#define INCL_DOSPROCESS
#include <os2.h>
#endif

#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

extern int msg_parent, msg_child;
extern char usr_rip;

#ifdef __OS2__
void VioUpdate (void);
#endif
int check_hotkey (char c);
int process_menu_option(int, char *);
int get_user_age (void);
void rpnInit (void);
char *rpnProcessString (char *p);
FILE *get_system_file (char *);
int check_subscription (void);

static void raise_priv (void);
static void lower_priv (void);
static int set_priv (char);
static void big_char (char);
static void big_string (int, char *, ...);
static void build_flags (char *, long);

int ansi_attr = 7;
char vx = 1, vy = 1, stop_hotkey = 0, isrip = 0;

void read_hourly_files ()
{
   int start, stop;
   char filename[80];
   struct ffblk blk;
   struct time timep;

   gettime (&timep);

   sprintf (filename, "%s????.*", text_path);
   if (findfirst (filename, &blk, 0))
      return;

   do {
      if (isdigit (blk.ff_name[0]) && isdigit (blk.ff_name[1]) && isdigit (blk.ff_name[2]) && isdigit (blk.ff_name[3]) && blk.ff_name[4] == '.') {
         start = (blk.ff_name[0] - '0') * 10 + (blk.ff_name[1] - '0');
         stop = (blk.ff_name[2] - '0') * 10 + (blk.ff_name[3] - '0');
         if (timep.ti_hour < start || timep.ti_hour > stop)
            continue;

         blk.ff_name[4] = '\0';
         sprintf (filename, "%s%s", text_path, blk.ff_name);
         read_system_file (blk.ff_name);
         return;
      }
   } while (!findnext (&blk));

   if (config->glob_text_path[0]) {
      sprintf (filename, "%s????.*", config->glob_text_path);
      if (findfirst (filename, &blk, 0))
         return;

      do {
         if (isdigit (blk.ff_name[0]) && isdigit (blk.ff_name[1]) && isdigit (blk.ff_name[2]) && isdigit (blk.ff_name[3]) && blk.ff_name[4] == '.') {
            start = (blk.ff_name[0] - '0') * 10 + (blk.ff_name[1] - '0');
            stop = (blk.ff_name[2] - '0') * 10 + (blk.ff_name[3] - '0');
            if (timep.ti_hour < start || timep.ti_hour > stop)
               continue;

            blk.ff_name[4] = '\0';
            sprintf (filename, "%s%s", config->glob_text_path, blk.ff_name);
            read_system_file (blk.ff_name);
            return;
         }
      } while (!findnext (&blk));
   }
}

int naplps_read_file (char *name)
{
   FILE *fp;
   int c;
   char *p, prolog = 0, epilog = 0;

   XON_ENABLE();
   _BRK_ENABLE();

   if (!name || !(*name))
      return (-1);

   if ((p = strstr (name, "/NAP")) != NULL) {
      if (p[4] != '=')
         prolog = epilog = 1;
      else if (!strnicmp (&p[5], "ON", 2))
         prolog = 1;
      else if (!strnicmp (&p[5], "OFF", 3))
         epilog = 1;
   }

   if ((p = strchr (name,'/')) != NULL)
      *(p - 1) = '\0';

   fp = get_system_file (name);

   if (p != NULL)
      *(p - 1) = ' ';

   if (fp == NULL)
      return (-1);

   if (prolog) {
      BUFFER_BYTE (0x1B);
      BUFFER_BYTE (0x25);
      BUFFER_BYTE (0x41);
   }

   while ((c = getc (fp)) != EOF)
      BUFFER_BYTE (c);

   if (epilog) {
      BUFFER_BYTE (0x1B);
      BUFFER_BYTE (0x25);
      BUFFER_BYTE (0x40);
   }

   fclose (fp);

   UNBUFFER_BYTES ();
   FLUSH_OUTPUT();

   return (1);
}

char *line_fgets (char *dest, int max, FILE *fp)
{
   int i, c;

   i = 0;

   while (i < max) {
      if ((c = fgetc (fp)) == EOF) {
         if (i == 0)
            return (NULL);
         else
            break;
      }
      dest[i++] = (char )c;
      if ((char )c == 0x0D) {
         if ((c = fgetc (fp)) == 0x0A)
            dest[i++] = (char )c;
         else
            ungetc (c, fp);
         break;
      }
   }

   dest[i] = '\0';
   return (dest);
}

int read_system_file (char *name)
{
   int i;
   char filename[128];

   strcpy (filename, text_path);
   strcat (filename, name);

   if (stristr (name, "/R") == NULL)
      i = read_file (filename);
   else
      i = naplps_read_file (filename);

   if (i == 0 && config->glob_text_path[0]) {
      strcpy (filename, config->glob_text_path);
      strcat (filename, name);

      if (stristr (name, "/R") == NULL)
         i = read_file (filename);
      else
         i = naplps_read_file (filename);
   }

   return (i == -1 ? 0 : i);
}

FILE *get_system_file (char *name)
{
   FILE *fp;
   char linea[80];

   isrip = 0;

   fp = sh_fopen (name, "rb", SH_DENYWR);

   if (strchr (name, '.') == NULL) {
      if (fp == NULL && usr_rip) {
         sprintf (linea, "%s.RIP", name);
         fp = sh_fopen (linea, "rb", SH_DENYWR);
         if (fp != NULL) {
            isrip = 1;
            return (fp);
         }
      }
      if (fp == NULL && !usr.ansi && !usr.avatar) {
         sprintf (linea, "%s.ASC", name);
         fp = sh_fopen (linea, "rb", SH_DENYWR);
      }
      if (fp == NULL && usr.ansi) {
         sprintf (linea, "%s.ANS", name);
         fp = sh_fopen (linea, "rb", SH_DENYWR);
      }
      if (fp == NULL && usr.avatar) {
         sprintf (linea, "%s.AVT", name);
         fp = sh_fopen (linea, "rb", SH_DENYWR);
      }
      if (fp == NULL) {
         sprintf (linea, "%s.BBS", name);
         fp = sh_fopen (linea, "rb", SH_DENYWR);
      }
   }

   return (fp);
}

#define MAX_LINE_LENGTH  2050

int read_file (char *name)
{
   FILE *fp, *answer, *fpc;
   char *linea, stringa[80], parola[80], *p, lastresp[80];
   char onexit[40], resp, c;
   int line, required, more, m, a, day, mont, year, bignum, fd;
   word search;
   long tempo;
   struct ffblk blk;
   struct tm *tim;

   ansi_attr = 7;
   vx = vy = 1;
   XON_ENABLE();
   _BRK_ENABLE();

   if (!name || !(*name))
      return(0);

   if ((p=strchr(name,'/')) != NULL)
      *(p-1) = '\0';

   fp = get_system_file (name);

   if (p != NULL)
      *(p-1) = ' ';

   if (fp == NULL)
      return (0);

   if ((linea = (char *)malloc (MAX_LINE_LENGTH)) == NULL) {
      fclose (fp);
      return (0);
   }

   more = line = 1;
   nopause = bignum = required = 0;
   fpc = answer = NULL;
   resp = ' ';
   onexit[0] = '\0';

   rpnInit ();

loop:
   change_attr (LGREY|_BLACK);

   while (line_fgets (linea, MAX_LINE_LENGTH - 2, fp) != NULL) {
      linea[MAX_LINE_LENGTH - 2] = '\0';

      for (p = linea; (*p) && (*p != 0x1A); p++) {
         if (!local_mode) {
            if (!CARRIER || RECVD_BREAK()) {
               CLEAR_OUTBOUND();

               fclose (fp);
               if (answer)
                  fclose (answer);
#ifdef __OS2__
               VioUpdate ();
#endif
               free (linea);
               return (line);
            }

            if (stop_hotkey && CHAR_AVAIL ()) {
               c = (char)PEEKBYTE ();
               if (check_hotkey (c)) {
                  CLEAR_OUTBOUND();

                  fclose (fp);
                  if (answer)
                     fclose (answer);
#ifdef __OS2__
                  VioUpdate ();
#endif
                  free (linea);
                  return (line);
               }
               else
                  TIMED_READ (1);
            }
         }

         switch (*p) {
            case CTRLA:
               if (*(p + 1) == CTRLA) {
                  big_string (bignum, bbstxt[B_PRESS_ENTER]);
                  p++;
               }
#ifdef __OS2__
               VioUpdate ();
#endif
               input (stringa, 0);
               line = 1;
               break;
            case CTRLD:
               _BRK_ENABLE ();
               more=1;
               break;
            case CTRLE:
               _BRK_DISABLE ();
               more=0;
               break;
            case CTRLF:
               p++;
               switch(toupper(*p)) {
               case '"':
                  for (m = 0; m < 10; m++) {
                     if (config->packid[m].display[0] == usr.archiver)
                        break;
                  }
                  if (m < 10)
                     big_string (bignum, "%s", &config->packid[m].display[1]);
                  else
                     big_string (bignum, "%s", &bbstxt[B_NONE][1]);
                  break;
               case '%':
                  if (usr.protocol == '\0')
                     big_string (bignum, "%s", &bbstxt[B_NONE][1]);
                  else if (usr.protocol == protocols[0][0])
                     big_string (bignum, "%s", &protocols[0][1]);
                  else if (usr.protocol == protocols[1][0])
                     big_string (bignum, "%s", &protocols[1][1]);
                  else if (usr.protocol == protocols[2][0])
                     big_string (bignum, "%s", &protocols[2][1]);
                  else if (usr.protocol == protocols[5][0])
                     big_string (bignum, "%s", &protocols[5][1]);
                  else if (config->hslink && usr.protocol == protocols[3][0])
                     big_string (bignum, "%s", &protocols[3][1]);
                  else if (config->puma && usr.protocol == protocols[4][0])
                     big_string (bignum, "%s", &protocols[4][1]);
                  else {
                     PROTOCOL prot;

                     sprintf (stringa, "%sPROTOCOL.DAT", config->sys_path);
                     fd = sh_open (stringa, O_RDONLY|O_BINARY, SH_DENYWR, S_IREAD|S_IWRITE);
                     if (fd != -1) {
                        while (read (fd, &prot, sizeof (PROTOCOL)) == sizeof (PROTOCOL)) {
                           if (prot.active && prot.hotkey == usr.protocol) {
                              big_string (bignum, "%s", prot.name);
                              break;
                           }
                        }
                        close (fd);
                     }
                     if (fd == -1 || !prot.active || prot.hotkey != usr.protocol)
                        big_string (bignum, "%s", &bbstxt[B_NONE][1]);
                  }
                  break;
               case 'C':
                  big_string (bignum, "%ld", cps);
                  break;
               case 'D':
                  big_string (bignum, "%s",usr.dataphone);
                  break;
               case 'E':
                  big_string (bignum, "%s",usr.voicephone);
                  break;
               case 'F':
                  usr.ldate[9] = '\0';
                  big_string (bignum, "%s", usr.ldate);
                  usr.ldate[9] = ' ';
                  break;
               case 'G':
                  big_string (bignum, "%s",&usr.ldate[11]);
                  break;
               case 'H':
                  big_string (bignum, "%ld", (cps * 100) / (rate / 10));
                  break;
               case 'I':
                  big_string (bignum, "%s",usr.ibmset ? bbstxt[B_YES] : bbstxt[B_NO]);
                  break;
               case 'K':
                  big_string (bignum, "%s",usr.kludge ? bbstxt[B_YES] : bbstxt[B_NO]);
                  break;
               case 'L':
                  big_string (bignum, "%d", usr.credit);
                  break;
               case 'M':
                  big_string (bignum, "%d", last_mail);
                  break;
               case 'N':
                  big_string (bignum, "%d", lastread);
                  break;
               case 'O':
                  big_string (bignum, "%s", get_priv_text(usr.priv));
                  break;
               case 'Q':
                  big_string (bignum, "%u", usr.n_upld);
                  break;
               case 'R':
                  big_string (bignum, "%lu", usr.upld);
                  break;
               case 'S':
                  big_string (bignum, "%u", usr.n_dnld);
                  break;
               case 'T':
                  big_string (bignum, "%lu", usr.dnld);
                  break;
               case 'V':
                  big_string (bignum, "%d", usr.len);
                  break;
               case 'X':
                  big_string (bignum, "%s",usr.ansi ? bbstxt[B_YES] : bbstxt[B_NO]);
                  break;
               case 'Y':
                  big_string (bignum, "%s",usr.more ? bbstxt[B_YES] : bbstxt[B_NO]);
                  break;
               case 'Z':
                  big_string (bignum, "%s",usr.formfeed ? bbstxt[B_YES] : bbstxt[B_NO]);
                  break;
               case '0':
                  big_string (bignum, "%s",usr.use_lore ? bbstxt[B_NO] : bbstxt[B_YES]);
                  break;
               case '2':
                  big_string (bignum, "%d", get_user_age ());
                  break;
               case '3':
                  big_string (bignum, "%s",usr.hotkey ? bbstxt[B_YES] : bbstxt[B_NO]);
                  break;
               case '4':
                  big_string (bignum, "%s",usr.firstdate);
                  break;
               case '5':
                  big_string (bignum, "%s",usr.birthdate);
                  break;
               case '6':
                  big_string (bignum, "%s",usr.scanmail ? bbstxt[B_YES] : bbstxt[B_NO]);
                  break;
               case '7':
                  big_string (bignum, "%s",usr.subscrdate);
                  break;
               case '8':
                  big_string (bignum, "%s",usr.avatar ? bbstxt[B_YES] : bbstxt[B_NO]);
                  break;
               case '1':
                  m = check_subscription ();
                  if (m != -1)
                     big_string (bignum, "%d", m);
                  break;
               case '!':
                  big_string (bignum, "%s",usr.color ? bbstxt[B_YES] : bbstxt[B_NO]);
                  break;
               case CTRLA:
                  show_quote();
                  break;
               case 'A':
               case CTRLB:
                  big_string (bignum, "%s", usr.name);
                  break;
               case 'B':
               case CTRLC:
                  big_string (bignum, "%s", usr.city);
                  break;
               case CTRLD:
                  data(stringa);
                  stringa[9] = '\0';
                  big_string (bignum, "%s",stringa);
                  break;
               case 'P':
               case CTRLE:
                  big_string (bignum, "%ld",usr.times);
                  break;
               case 'W':
               case CTRLF:
                  strcpy(stringa,usr.name);
                  get_fancy_string(stringa, parola);
                  big_string (bignum, parola);
                  break;
               case CTRLG:
                  timer(10);
                  break;
               case CTRLK:
                  m = (int)((time(NULL)-start_time)/60);
                  m += usr.time;
                  big_string (bignum, "%d",m);
                  break;
               case 'U':
               case CTRLL:
                  m = (int)((time(0)-start_time)/60);
                  big_string (bignum, "%d",m);
                  break;
               case CTRLN:
#ifdef __OS2__
                  VioUpdate ();
#endif
                  modem_hangup();
                  free (linea);
                  return (line);
               case CTRLO:
                  big_string (bignum, "%d",time_remain());
                  break;
               case CTRLP:
                  big_string (bignum, "%s",ctime(&start_time));
                  break;
               case CTRLQ:
                  big_string (bignum, "%lu",sysinfo.total_calls);
                  break;
               case CTRLR:
                  big_string (bignum, "%lu",usr.dnld-usr.upld);
                  break;
               case CTRLS:
                  big_string (bignum, "%s",usr.signature);
                  break;
               case CTRLT:
                  data(stringa);
                  big_string (bignum, &stringa[10]);
                  break;
               case CTRLU:
                  required=1;
                  break;
               case CTRLV:
                  required=0;
                  break;
               case CTRLW:
                  big_string (bignum, "%lu", usr.upld);
                  break;
               case CTRLX:
                  big_string (bignum, "%lu",usr.dnld);
                  break;
               case '9':
                  if(usr.n_upld == 0)
                     big_string (bignum, "0:%u", usr.n_dnld);
                  else {
                      m = (unsigned int)(usr.n_dnld / usr.n_upld);
                      big_string (bignum, "1:%u", m);
                  }
                  break;
               case CTRLY:
               case ':':
                  if (usr.upld == 0)
                     big_string (bignum, "0:%lu", usr.dnld);
                  else {
                     m = (unsigned int)(usr.dnld / usr.upld);
                        big_string (bignum, "1:%u", m);
                  }
                  break;
               case ';':
                  big_string (bignum, "%s",usr.full_read ? bbstxt[B_YES] : bbstxt[B_NO]);
                  break;
               case '[':
                  big_string (bignum, "%lu", config->class[usr_class].max_dl - usr.dnldl);
                  break;
               case '\\':
                  big_string (bignum, "%s", &config->language[usr.language].descr[1]);
                  break;
               case ']':
                  big_string (bignum, "%s", usr.comment);
                  break;
               }
               break;
				case CR:
					break;
				case LF:

					if (vy<25) vy++;
					vx=1;

					if (!local_mode) {
                  BUFFER_BYTE('\r');
                  BUFFER_BYTE('\n');
               }
               if (snooping) {
                  wputc (CR);
                  wputc (LF);
               }

               if (!(line++ < (usr.len-1)) && usr.len != 0) {
                  if (!more)
                     continue;
                  if (!(line = more_question (line))) {
                      fclose(fp);
                      fp = NULL;
                  }
               }
               break;
            case CTRLK:
               p++;
               switch(toupper(*p)) {
               case 'A':
                  big_string (bignum, "%lu",sysinfo.total_calls);
                  break;
               case 'B':
                  big_string (bignum, "%s",lastcall.name);
                  break;
               case 'C':
                  big_string (bignum, "%d", msg_child);
                  break;
               case 'D':
                  big_string (bignum, "%d",first_msg);
                  break;
               case 'E':
                  big_string (bignum, "%d",last_msg);
                  break;
               case 'F':
                  p++;
                  sscanf(usr.ldate, "%2d %3s %2d", &day, parola, &year);
                  parola[3] = '\0';
                  for (mont = 0; mont < 12; mont++) {
                     if ((!stricmp(mtext[mont], parola)) || (!stricmp(mesi[mont], parola)))
                        break;
                  }
                  search=(year-80)*512+(mont+1)*32+day;
                  translate_filenames (p, resp, lastresp);
                  if (findfirst(p,&blk,0) || blk.ff_fdate < search)
                     p = strchr(linea,'\0') - 1;
                  break;
               case 'G':
                  tempo = time(0);
                  tim = localtime(&tempo);
                  big_string (bignum, "%s", bbstxt[B_SUNDAY + tim->tm_wday]);
                  break;
               case 'I':
                  data(stringa);
                  big_string (bignum, &stringa[10]);
                  break;
               case 'J':
                  data(stringa);
                  stringa[9] = '\0';
                  big_string (bignum, "%s",stringa);
                  break;
               case 'K':
                  m = (int)((time(NULL)-start_time)/60);
                  big_string (bignum, "%d",m);
                  break;
               case 'M':
                  big_string (bignum, "%d",max_priv_mail);
                  break;
               case 'O':
                  big_string (bignum, "%d",time_remain());
                  break;
               case 'P':
                  big_string (bignum, "%d", msg_parent);
                  break;
               case 'Q':
                  big_string (bignum, "%d",config->class[usr_class].max_call);
                  break;
               case 'R':
                  big_string (bignum, "%ld", local_mode ? 0 : rate);
                  break;
               case 'T':
                  big_string (bignum, "%d",config->class[usr_class].max_dl);
                  break;
               case 'U':
                  big_string (bignum, "%d", time_to_next (1));
                  break;
               case 'W':
                  big_string (bignum, "%d", line_offset);
                  break;
               case 'X':
#ifdef __OS2__
                  VioUpdate ();
#endif
                  terminating_call ();
                  free (linea);
                  return (line);
               case 'Y':
                  big_string (bignum, "%s",sys.msg_name);
                  break;
               case 'Z':
                  big_string (bignum, "%s",sys.file_name);
                  break;
               case '0':
               case '9':
                  big_string (bignum, "%d",num_msg);
                  break;
               case '1':
                  big_string (bignum, "%d",usr.msg);
                  break;
               case '2':
                  big_string (bignum, "%d",usr.files);
                  break;
               case '5':
                  big_string (bignum, "%s", sys.msg_name);
                  break;
               case '7':
                  big_string (bignum, "%u",usr.account);
                  break;
               case '8':
                  big_string (bignum, "%u",usr.f_account);
                  break;
               case '[':
                  big_string (bignum, "%d", config->class[usr_class].max_dl - usr.dnldl);
                  break;
               case '\\':
                  del_line ();
                  break;
               default:
                  p--;
                  _BRK_DISABLE ();
                  more = 0;
                  break;
               }
               break;
            case CTRLL:
               cls();
               vx = vy = 1;
               line=1;
               break;
            case CTRLO:
               p++;
               switch(toupper(*p)) {
               case 'C':
                  p++;
                  while (p[strlen (p) - 1] == '\r' || p[strlen (p) - 1] == '\n')
                     p[strlen (p) - 1] = '\0';
                  translate_filenames (p, resp, lastresp);
                  open_outside_door (p);
                  p = strchr(linea,'\0') - 1;
                  break;
               case 'E':
                  if (!usr.ansi && !usr.avatar)
                     p = strchr(linea,'\0') - 1;
                  break;
               case 'F':
                  p++;
                  strcpy(onexit,p);
                  p = strchr(linea,'\0') - 1;
                  break;
               case 'M':
                  if(answer) {
                     p++;
                     fprintf(answer,"%s %c\r\n",p,resp);
                  }

                  p = strchr(linea,'\0') - 1;
                  break;
               case 'N':
                  while (CARRIER && time_remain() > 0) {
                     input(stringa,usr.width-1);
                     if(strlen(stringa))
                        break;
                     if(!required)
                        break;
                     big_string (bignum, bbstxt[B_TRY_AGAIN]);
                  }

                  strcpy (lastresp, stringa);

                  if(answer) {
                     putc(' ',answer);
                     putc(' ',answer);
                     p++;
                     fprintf(answer,"%s",p);
                     putc(':',answer);
                     putc(' ',answer);
                     fprintf(answer,"%s\r\n",stringa);
                  }

                  p = strchr(linea,'\0') - 1;
                  break;
               case 'O':
                  if(answer)
                     fclose(answer);
                  p++;
                  p[strlen(p)-2] = '\0';
                  translate_filenames (p, resp, lastresp);
                  answer = fopen (p, "ab");
                  if(answer == NULL)
                     status_line(msgtxt[M_UNABLE_TO_OPEN],p);

                  p = strchr(linea,'\0') - 1;
                  break;
               case 'P':
                  if(answer) {
                     fprintf(answer,"\r\n");
                     usr.ptrquestion = ftell(answer);
                     fprintf(answer,"* %s\t%s\t%s\r\n",usr.name,usr.city,data(parola));
                  }
                  break;
               case 'Q':
                  if (fp != NULL)
                     fclose(fp);

                  UNBUFFER_BYTES ();
                  FLUSH_OUTPUT ();

                  if (fpc == NULL) {
                     fp = fopen(onexit,"rb");
                     if (fp == NULL) {
                        if (answer)
                           fclose (answer);
#ifdef __OS2__
                        VioUpdate ();
#endif
                        free (linea);
                        return (line);
                     }
                  }
                  else {
                     fp = fpc;
                     fpc = NULL;
                     p[1] = '\0';
                  }

                  p = strchr(p,'\0') - 1;
                  break;
               case 'R':
                  line=1;
                  p++;
                  while (CARRIER && time_remain() > 0) {
                     input (stringa, 1);
                     c = toupper(stringa[0]);
                     if(c == '\0')
                        c = 0x7C;
                     if(strchr(strupr(p),c) != NULL)
                        break;
                     big_string (bignum, bbstxt[B_TRY_AGAIN]);
                  }

                  p = strchr(linea,'\0') - 1;
                  resp = c;
                  big_string (bignum, "\n");
                  break;
               case 'S':
                  p++;
                  fclose (fp);
                  fp = get_system_file (p);
                  if (fp == NULL) {
                     fp = get_system_file (onexit);
                     onexit[0] = '\0';
                     if (fp == NULL) {
#ifdef __OS2__
                        VioUpdate ();
#endif
                        free (linea);
                        return (line);
                     }
                  }

                  p = strchr (linea, '\0') - 1;
                  break;
               case 'T':
                  rewind (fp);
                  break;
               case 'U':
                  p++;
                  if(toupper(*p) != resp)
                     p = strchr(linea,'\0') - 1;
                  break;
               case 'V':
                  p++;
                  fseek (fp, atol (p), SEEK_SET);
                  if(toupper(*p) != resp)
                     p = strchr(linea,'\0') - 1;
                  break;
               }
               break;
            case CTRLP:
               p++;
               if ( *p == 'B' ) {
                  p++;
                  a = set_priv ( *p );
                  if (usr.priv > a)
                     p = strchr(linea,'\0') - 1;
               }
               else if ( *p == 'L' ) {
                  p++;
                  a = set_priv ( *p );
                  if (usr.priv < a)
                     p = strchr(linea,'\0') - 1;
               }
               else if ( *p == 'Q' ) {
                  p++;
                  a = set_priv ( *p );
                  if (a != usr.priv)
                     p = strchr(linea,'\0') - 1;
               }
               else if ( *p == 'X' ) {
                  p++;
                  a = set_priv ( *p );
                  if (usr.priv == a)
                     p = strchr(linea,'\0') - 1;
               }
               else {
                  a = set_priv ( *p );
                  if (usr.priv < a)
                     fseek (fp, 0L, SEEK_END);
               }
               break;
            case CTRLV:
               p++;
               switch (*p) {
               case CTRLA:
                  p++;
                  if (*p == CTRLP) {
                     p++;
                     *p &= 0x7F;
                  }

                  if (!*p) {
                     change_attr(13);
                     ansi_attr = 13;
                  }
                  else {
                     change_attr(*p);
                     ansi_attr = *p;
                  }
                  break;
               case CTRLC:
                  cup (1);
                  line--;
                  if (vy > 0)
                     vy++;
                  break;
               case CTRLD:
                  cdo (1);
                  line++;
                  if (vy < usr.len - 1)
                     vy++;
                  break;
               case CTRLE:
                  cle (1);
                  if (vx > 0)
                     vx--;
                  break;
               case CTRLF:
                  cri (1);
                  if (vx < 79)
                     vx++;
                  break;
               case CTRLG:
                  del_line();
                  break;
               case CTRLH:
                  cpos ( *(p + 1), *(p + 2) );
                  vy = line = *(p + 1);
                  vx = *(p + 2);
                  p += 2;
                  break;
               case CTRLY:
                  p++;
                  strncpy (stringa, &p[1], *p);
                  stringa[*p] = '\0';
                  p += *p + 1;
                  for (m = 0; m < *p; m++) {
                     m_print (stringa);
                     vx += strlen (stringa);
                     if (vx >= 80) {
                        vx -= 80;
                        if (vy < usr.len - 1)
                           vy++;
                     }
                  }
                  break;
               }
               break;
            case CTRLR:
               p++;
               p = rpnProcessString (p);
               break;
            case CTRLW:
               p++;
               switch(*p) {
               case 'A':
                  p++;
                  p[strlen(p)-2] = '\0';
                  translate_filenames (p, resp, lastresp);
                  status_line (p);
                  p = strchr(linea,'\0') - 1;
                  break;
               case 'a':
                  p++;
                  p[strlen(p)-2] = '\0';
                  translate_filenames (p, resp, lastresp);
                  broadcast_message (p);
                  p = strchr(linea,'\0') - 1;
                  break;
               case CTRLA:
                  usr.ldate[9] = '\0';
                  big_string (bignum, "%s",usr.ldate);
                  usr.ldate[9] = ' ';
                  break;
               case 'B':
                  bignum = bignum ? 0 : 1;
                  break;
               case 'c':
                  p++;
                  if ( *p == 'A' && !local_mode)
                     p = strchr(linea,'\0') - 1;
                  else if ( *p == 'R' && local_mode)
                     p = strchr(linea,'\0') - 1;
                  break;
               case CTRLB:
                  p++;
                  if ( (*p == '1' && rate >= 1200) ||
                       (*p == '2' && rate >= 2400) ||
                       (*p == '9' && rate >= 9600)
                     )
                     break;
                  p = strchr(linea,'\0') - 1;
                  break;
               case CTRLC:
                  big_string (bignum, system_name);
                  break;
               case CTRLD:
                  big_string (bignum, sysop);
                  break;
               case CTRLE:
                  big_string (bignum, lastresp);
                  break;
               case CTRLG:
#ifdef __OS2__
                  DosBeep (1000, 30);
#else
                  sound (1000);
                  timer (3);
                  nosound ();
#endif
                  break;
               case CR:
                  p++;
                  if ( *p == 'A')
                     big_string (bignum, "%d", usr.msg);
                  if ( *p == 'L')
                     big_string (bignum, "%d",lastread);
                  else if ( *p == 'N')
                     big_string (bignum, "%s",sys.msg_name);
                  else if ( *p == 'H')
                     big_string (bignum, "%d",last_msg);
                  else if ( *p == '#')
                     big_string (bignum, "%d",first_msg - last_msg + 1);
                  break;
               case CTRLN:
                  p++;
                  if ( *p == 'B' || *p == 'C' )
                     big_string (bignum, "%d", usr.credit);
                  else if ( *p == 'D' )
                     SENDBYTE ('0');
                  break;
               case '8':
                  if (usr.len < 79)
                     p = strchr(linea,'\0') - 1;
                  break;
               case 'D':
                  p++;
                  p[strlen(p)-2] = '\0';
                  translate_filenames (p, resp, lastresp);
                  unlink (p);
                  p = strchr(linea,'\0') - 1;
                  break;
               case CTRLF:
               case 'G':
                  p++;
                  if ( *p == 'A')
                     big_string (bignum, "%d", usr.files);
                  else if ( *p == 'N')
                     big_string (bignum, "%s",sys.file_name);
                  break;
               case 'I':
                  p++;
                  if ( *p == 'L' && !local_mode)
                     p = strchr(linea,'\0') - 1;
                  else if ( *p == 'R' && local_mode)
                     p = strchr(linea,'\0') - 1;
                  break;
               case 'k':
                  p++;
                  if ( *p == 'D' ) {
                     build_flags (parola, usr.flags);
                     big_string (bignum, parola);
                  }
                  else if ( *p == 'F' ) {
                     p++;
                     usr.flags &= ~get_flags (p);
                     while (*p != ' ' && *p != '\0')
                        p++;
                     if (*p == '\0')
                        p--;
                  }
                  else if ( *p == 'I' ) {
                     p++;
                     if ((usr.flags & get_flags (p)) != get_flags (p))
                        p = strchr(linea,'\0') - 1;
                     else {
                        while (*p != ' ' && *p != '\0')
                           p++;
                        if (*p == '\0')
                           p--;
                     }
                  }
                  else if ( *p == 'N' ) {
                     p++;
                     if ((usr.flags & get_flags (p)) == 0)
                        p = strchr(linea,'\0') - 1;
                     else {
                        while (*p != ' ' && *p != '\0')
                           p++;
                        if (*p == '\0')
                           p--;
                     }
                  }
                  else if ( *p == 'O' ) {
                     p++;
                     usr.flags |= get_flags (p);
                     while (*p != ' ' && *p != '\0')
                        p++;
                     if (*p == '\0')
                        p--;
                  }
                  else if ( *p == 'T' ) {
                     p++;
                     usr.flags ^= get_flags (p);
                     while (*p != ' ' && *p != '\0')
                        p++;
                     if (*p == '\0')
                        p--;
                  }
                  break;
               case 'L':
                  p++;
                  fpc = fp;
                  p[strlen (p) - 2] = '\0';
                  fp = get_system_file (p);
                  if (fp == NULL) {
                     fp = fpc;
                     fpc = NULL;
                  }
                  p = strchr (p, '\0') - 1;
                  break;
               case 'l':
                  m = 0;
                  p++;
                  while (*p && isdigit (*p)) {
                     m *= 10;
                     m += *p++ - '0';
                  }
                  if (*p == ' ')
                     p++;
                  if (line_offset != m)
                     p = strchr (p, '\0') - 1;
                  break;
               case 'M':
                  p++;
                  if (*p == CTRLP) {
                     p++;
                     *p &= 0x7F;
                  }
                  process_menu_option (*p, p + 1);
                  p = strchr (p, '\0') - 1;
                  break;
               case 'p':
                  p++;
                  if ( *p == 'D' )
                     lower_priv ();
                  else if ( *p == 'U' )
                     raise_priv ();
                  break;
               case 'P':
                  big_string (bignum, "%s", usr.voicephone);
                  break;
               case 'R':
                  big_string (bignum, "%s", usr.handle);
                  break;
               case 'S':
                  big_string (bignum, "%s", usr.signature);
                  break;
               case 's':
                  p++;
                  usr.priv = set_priv ( *p );
                  usr_class = get_class(usr.priv);
                  break;
               case 'W':
                  p++;
                  if(answer) {
                     translate_filenames (p, resp, lastresp);
                     fprintf(answer, "%s", p);
                  }
                  p = strchr(p,'\0') - 1;
                  break;
               case 'h':
                  online_users (1, 1);
                  break;
               case 'w':
                  online_users (1, 0);
                  break;
               case 'X':
                  p++;
                  if ( *p == 'D' || *p == 'R' ) {
                     p++;
                     translate_filenames (p, resp, lastresp);
                     open_outside_door (p);
                     p = strchr(p,'\0') - 1;
                  }
                  break;
               default:
                  p--;
                  timer(5);
                  break;
               }
               break;
            case CTRLX:
               p++;
               translate_filenames (p, resp, lastresp);
               open_outside_door(p);
               *p = '\0';
               p--;
               break;
            case CTRLY:
               c = *(++p);
               if (!usr.ibmset && (unsigned char)c >= 128)
                  c = bbstxt[B_ASCII_CONV][(unsigned char)c - 128];
               a = *(++p);
               vx += a;
               if (vx > 80) {
                  vx -= 80;
                  if (vy < 25)
                     vy++;
               }
               if(usr.avatar && !local_mode) {
                  BUFFER_BYTE(CTRLY);
                  BUFFER_BYTE(c);
                  BUFFER_BYTE(a);
               }
               else if (!local_mode)
                  for(m=0;m<a;m++)
                     BUFFER_BYTE(c);
               if (snooping)
                  wdupc (c, a);
               break;
            case CTRLZ:
               break;
            case 0x1B:
               p = translate_ansi (p);
               break;
            default:
               c = *p;
               if (!usr.ibmset && (unsigned char)c >= 128)
                  c = bbstxt[B_ASCII_CONV][(unsigned char)c - 128];
               if (!bignum || (!usr.ansi && !usr.avatar)) {
                  if (!local_mode)
                     BUFFER_BYTE (c);
                  if (snooping && !isrip)
                     wputc (c);
                  vx++;
                  if (vx > 80) {
                     vx -= 80;
                     if (vy < 25)
                        vy++;
                  }
               }
               else
                  big_char (c);
               break;
         }

         if (fp == NULL)
            break;
      }

      if (*p == CTRLZ || fp == NULL) {
         if (fp != NULL)
            fclose(fp);

         if (!local_mode) {
            UNBUFFER_BYTES ();
            FLUSH_OUTPUT ();
         }

         if (fpc == NULL) {
            fp = fopen(onexit,"rb");
            if (fp == NULL)
               break;
         }
         else {
            fp = fpc;
            fpc = NULL;
         }
      }
   }

   if (fp != NULL)
      fclose(fp);

#ifdef __OS2__
   VioUpdate ();
#endif

   if (fpc == NULL) {
      fp = fopen(onexit,"rb");
      if (fp != NULL)
         goto loop;
   }
   else {
      fp = fpc;
      fpc = NULL;
      goto loop;
   }

   if (fp != NULL)
      fclose(fp);
   if (answer != NULL)
      fclose(answer);

   if (!local_mode) {
      UNBUFFER_BYTES ();
      FLUSH_OUTPUT();
   }

   free (linea);

   return (line ? line : -1);
}

void show_quote()
{
   FILE *quote;
   char linea[129];

   strcpy(linea, text_path);
   strcat(linea, "QUOTES.BBS");

   quote = fopen (linea, "rt");
   if (quote == NULL) {
      strcpy (linea, config->glob_text_path);
      strcat (linea, "QUOTES.BBS");
      if (quote == NULL)
         return;
   }

   fseek(quote,sysinfo.quote_position,0);

   for (;;) {
      if ((fgets(linea,128,quote) == NULL) || linea[0] == CTRLZ) {
         rewind(quote);
         continue;
      }

      if(linea[0] == '\n') {
         sysinfo.quote_position = ftell(quote);
         break;
      }

      m_print( "%s", linea);
   }

   fclose (quote);

   write_sysinfo ();
}

char far macro_bitmap[95][5]=
{
  {  0x00,0x00,0x00,0x00,0x00  },   //
  {  0x00,0x00,0x3d,0x00,0x00  },   //  !
  {  0x00,0x30,0x00,0x30,0x00  },   //  "
  {  0x12,0x3f,0x12,0x3f,0x12  },   //  #
  {  0x09,0x15,0x3f,0x15,0x12  },   //  $
  {  0x19,0x1a,0x04,0x0b,0x13  },   //  %
  {  0x16,0x29,0x15,0x02,0x05  },   //  &
  {  0x00,0x08,0x30,0x00,0x00  },   //  '
  {  0x00,0x1e,0x21,0x00,0x00  },   //  (
  {  0x00,0x21,0x1e,0x00,0x00  },   //  )
  {  0x04,0x15,0x0e,0x15,0x04  },   //  *
  {  0x04,0x04,0x1f,0x04,0x04  },   //  +
  {  0x00,0x00,0x01,0x02,0x00  },   //  ,
  {  0x04,0x04,0x04,0x04,0x04  },   //  -
  {  0x00,0x00,0x03,0x03,0x00  },   //  .
  {  0x01,0x02,0x04,0x08,0x10  },   //  /
  {  0x1e,0x21,0x25,0x21,0x1e  },   //  0
  {  0x00,0x11,0x3f,0x01,0x00  },   //  1
  {  0x11,0x23,0x25,0x29,0x11  },   //  2
  {  0x21,0x29,0x29,0x29,0x16  },   //  3
  {  0x06,0x0a,0x12,0x3f,0x02  },   //  4
  {  0x3a,0x29,0x29,0x29,0x26  },   //  5
  {  0x1e,0x29,0x29,0x29,0x26  },   //  6
  {  0x21,0x22,0x24,0x28,0x30  },   //  7
  {  0x16,0x29,0x29,0x29,0x16  },   //  8
  {  0x10,0x29,0x29,0x29,0x1e  },   //  9
  {  0x00,0x00,0x12,0x00,0x00  },   //  :
  {  0x00,0x01,0x12,0x00,0x00  },   //  ;
  {  0x00,0x04,0x0a,0x11,0x00  },   //  <
  {  0x0a,0x0a,0x0a,0x0a,0x0a  },   //  =
  {  0x00,0x11,0x0a,0x04,0x00  },   //  >
  {  0x10,0x20,0x25,0x28,0x10  },   //  ?
  {  0x1e,0x29,0x35,0x29,0x15  },   //  @
  {  0x1f,0x24,0x24,0x24,0x1f  },   //  A
  {  0x3f,0x29,0x29,0x29,0x16  },   //  B
  {  0x1e,0x21,0x21,0x21,0x12  },   //  C
  {  0x3f,0x21,0x21,0x21,0x1e  },   //  D
  {  0x3f,0x29,0x29,0x29,0x21  },   //  E
  {  0x3f,0x28,0x28,0x28,0x20  },   //  F
  {  0x1e,0x21,0x25,0x25,0x16  },   //  G
  {  0x3f,0x08,0x08,0x08,0x3f  },   //  H
  {  0x00,0x21,0x3f,0x21,0x00  },   //  I
  {  0x02,0x01,0x01,0x01,0x3e  },   //  J
  {  0x3f,0x08,0x14,0x22,0x01  },   //  K
  {  0x3f,0x01,0x01,0x01,0x01  },   //  L
  {  0x3f,0x10,0x08,0x10,0x3f  },   //  M
  {  0x3f,0x10,0x08,0x04,0x3f  },   //  N
  {  0x1e,0x21,0x21,0x21,0x1e  },   //  O
  {  0x3f,0x24,0x24,0x24,0x18  },   //  P
  {  0x1e,0x21,0x25,0x23,0x1e  },   //  Q
  {  0x3f,0x24,0x24,0x26,0x19  },   //  R
  {  0x12,0x29,0x29,0x29,0x26  },   //  S
  {  0x20,0x20,0x3f,0x20,0x20  },   //  T
  {  0x3e,0x01,0x01,0x01,0x3e  },   //  U
  {  0x38,0x06,0x01,0x06,0x38  },   //  V
  {  0x3e,0x01,0x06,0x01,0x3e  },   //  W
  {  0x23,0x14,0x08,0x14,0x23  },   //  X
  {  0x38,0x04,0x03,0x04,0x38  },   //  Y
  {  0x23,0x25,0x29,0x31,0x21  },   //  Z
  {  0x00,0x3f,0x21,0x21,0x00  },   //  [
  {  0x10,0x08,0x04,0x02,0x01  },   //  \
  {  0x00,0x21,0x21,0x3f,0x00  },   //  ]
  {  0x00,0x10,0x20,0x10,0x00  },   //  ^
  {  0x01,0x01,0x01,0x01,0x01  },   //  _
  {  0x00,0x00,0x30,0x08,0x00  },   //  `
  {  0x02,0x15,0x15,0x15,0x0f  },   //  a
  {  0x3f,0x09,0x09,0x09,0x06  },   //  b
  {  0x0e,0x11,0x11,0x11,0x0a  },   //  c
  {  0x06,0x09,0x09,0x09,0x3f  },   //  d
  {  0x0e,0x15,0x15,0x15,0x09  },   //  e
  {  0x00,0x09,0x1f,0x29,0x00  },   //  f
  {  0x08,0x15,0x15,0x15,0x0e  },   //  g
  {  0x3f,0x08,0x08,0x08,0x07  },   //  h
  {  0x00,0x01,0x17,0x01,0x00  },   //  i
  {  0x02,0x01,0x01,0x01,0x16  },   //  j
  {  0x3f,0x04,0x0a,0x11,0x00  },   //  k
  {  0x00,0x21,0x3f,0x01,0x00  },   //  l
  {  0x1f,0x10,0x1f,0x10,0x0f  },   //  m
  {  0x1f,0x10,0x10,0x10,0x0f  },   //  n
  {  0x0e,0x11,0x11,0x11,0x0e  },   //  o
  {  0x1f,0x12,0x12,0x12,0x0c  },   //  p
  {  0x0c,0x12,0x12,0x12,0x1f  },   //  q
  {  0x0f,0x10,0x10,0x10,0x10  },   //  r
  {  0x09,0x15,0x15,0x15,0x12  },   //  s
  {  0x10,0x3e,0x11,0x11,0x00  },   //  t
  {  0x1e,0x01,0x01,0x01,0x1e  },   //  u
  {  0x1c,0x02,0x01,0x02,0x1c  },   //  v
  {  0x1e,0x01,0x0e,0x01,0x1e  },   //  w
  {  0x11,0x0a,0x04,0x0a,0x11  },   //  x
  {  0x19,0x05,0x05,0x05,0x1e  },   //  y
  {  0x11,0x13,0x15,0x19,0x11  },   //  z
  {  0x08,0x16,0x21,0x21,0x00  },   //  {
  {  0x00,0x00,0x37,0x00,0x00  },   //  |
  {  0x00,0x21,0x21,0x16,0x08  },   //  }
  {  0x00,0x10,0x20,0x10,0x20  }    //  ~
};

static void big_char (ch)
char ch;
{
  char chars_bitmap[]=" \334\337\333";
  char bitbuf[3][5];
  char far *q;

  q = (char far *)&macro_bitmap[ch-' '][0];

  for (ch = 0; ch < 5; ch++)
  {
    bitbuf[0][ch] = chars_bitmap[q[ch] >> 4];
    bitbuf[1][ch] = chars_bitmap[(q[ch] >> 2) & 3];
    bitbuf[2][ch] = chars_bitmap[q[ch] & 3];
  }

  cup (2);
  m_print ("%5.5s ", bitbuf[0]);
  cle (6);
  cdo (1);
  m_print ("%5.5s ", bitbuf[1]);
  cle (6);
  cdo (1);
  m_print ("%5.5s ", bitbuf[2]);

  vx += 6;
  if (vx > 80) {
     vx -= 80;
     if (vy < 25)
        vy++;
  }
}

static void big_string (int big, char *format, ...)
{
   char *s;
   va_list var_args;
   char *string;

   string=(char *)malloc(256);

   if (string==NULL || strlen(format) > 256) {
      if (string)
         free(string);
      return;
   }

   va_start(var_args,format);
   vsprintf(string,format,var_args);
   va_end(var_args);

   if (!big || (!usr.ansi && !usr.avatar)) {
      m_print (string);
      vx += strlen (string);
      if (vx > 80) {
         vx -= 80;
         if (vy < 25)
            vy++;
      }
   }
   else
      for (s = string; *s; s++)
         big_char (*s);

   free(string);
}

static void lower_priv ()
{
   switch (usr.priv)
   {
      case DISGRACE:
         usr.priv = TWIT;
         break;
      case LIMITED:
         usr.priv = DISGRACE;
         break;
      case NORMAL:
         usr.priv = LIMITED;
         break;
      case WORTHY:
         usr.priv = NORMAL;
         break;
      case PRIVIL:
         usr.priv = WORTHY;
         break;
      case FAVORED:
         usr.priv = PRIVIL;
         break;
      case EXTRA:
         usr.priv = FAVORED;
         break;
      case CLERK:
         usr.priv = EXTRA;
         break;
      case ASSTSYSOP:
         usr.priv = CLERK;
         break;
      case SYSOP:
         usr.priv = ASSTSYSOP;
         break;
   }

   usr_class = get_class(usr.priv);
}

static void raise_priv ()
{
   switch (usr.priv)
   {
      case TWIT:
         usr.priv = DISGRACE;
         break;
      case DISGRACE:
         usr.priv = LIMITED;
         break;
      case LIMITED:
         usr.priv = NORMAL;
         break;
      case NORMAL:
         usr.priv = WORTHY;
         break;
      case WORTHY:
         usr.priv = PRIVIL;
         break;
      case PRIVIL:
         usr.priv = FAVORED;
         break;
      case FAVORED:
         usr.priv = EXTRA;
         break;
      case EXTRA:
         usr.priv = CLERK;
         break;
      case CLERK:
         usr.priv = ASSTSYSOP;
         break;
      case ASSTSYSOP:
         usr.priv = SYSOP;
         break;
   }

   usr_class = get_class(usr.priv);
}

static int set_priv (c)
char c;
{
   register int i;

   i = usr.priv;

   switch (c)
   {
      case 'T':
         i = TWIT;
         break;
      case 'D':
         i = DISGRACE;
         break;
      case 'L':
         i = LIMITED;
         break;
      case 'N':
         i = NORMAL;
         break;
      case 'W':
         i = WORTHY;
         break;
      case 'P':
         i = PRIVIL;
         break;
      case 'F':
         i = FAVORED;
         break;
      case 'E':
         i = EXTRA;
         break;
      case 'C':
         i = CLERK;
         break;
      case 'A':
         i = ASSTSYSOP;
         break;
      case 'S':
         i = SYSOP;
         break;
   }

   return (i);
}

static void build_flags (buffer, flag)
char *buffer;
long flag;
{
   int i, x;

   memset (buffer, ' ', 36);
   buffer[36] = '\0';

   x = 0;

   for (i=0;i<8;i++) {
      if (flag & 0x80000000L)
         buffer[x++] = '0' + i;
      flag = flag << 1;
   }

   for (i=0;i<8;i++) {
      if (flag & 0x80000000L)
         buffer[x++] = ((i<2)?'8':'?') + i;
      flag = flag << 1;
   }

   for (i=0;i<8;i++) {
      if (flag & 0x80000000L)
         buffer[x++] = 'G' + i;
      flag = flag << 1;
   }

   for (i=0;i<8;i++) {
      if (flag & 0x80000000L)
         buffer[x++] = 'O' + i;
      flag = flag << 1;
   }
}

#define  NORM   0x07
#define  BOLD   0x08
#define  FAINT  0xF7
#define  VBLINK 0x80
#define  REVRS  0x77

char *translate_ansi (pf)
char *pf;
{
   static char sx = 1, sy = 1;
   char c, str[10];
   int Pn[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
   int i = 0, p = 0;

   pf++;
   c = *pf;
   if (c != '[') {
      m_print ("\x1b%c", c);
      vx += 2;
      if (vx > 80) {
         vx -= 80;
         if (vy < 25)
            vy++;
      }
      return (pf);
   }

   while (1) {
      pf++;
      c = *pf;

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
                  cpos (vy, vx);
                  return (pf);
                  /* (CUU) cursor up   */
               case 'A':
                  if (!Pn[0])
                     Pn[0]++;
                  vy -= Pn[0];
                  if (vy < 1)
                     vy = 1;
                  cup (Pn[0]);
                  return (pf);
                  /* (CUD) cursor down */
               case 'B':
                  if (!Pn[0])
                     Pn[0]++;
                  vy += Pn[0];
                  if (vy > 25)
                     vy = 25;
                  cdo (Pn[0]);
                  return (pf);
                  /* (CUF) cursor forward */
               case 'C':
                  if (!Pn[0])
                     Pn[0]++;
                  vx += Pn[0];
                  if (vx > 80)
                     vx = 80;
                  cri (Pn[0]);
                  return (pf);
                  /* (CUB) cursor backward   */
               case 'D':
                  if (!Pn[0])
                     Pn[0]++;
                  vx -= Pn[0];
                  if (vx < 1)
                     vx = 1;
                  cle (Pn[0]);
                  return (pf);
                  /* (SCP) save cursor position */
               case 's':
                  sx = vx;
                  sy = vy;
                  return (pf);
                  /* (RCP) restore cursor position */
               case 'u':
                  vx = sx;
                  vy = sy;
                  cpos (vy, vx);
                  return (pf);
                  /* clear screen   */
               case 'J':
                  if (Pn[0] == 2) {
                     vx = vy = 1;
                     cls ();
                  }
                  return (pf);
                  /* (EL) erase line   */
               case 'K':
                  del_line ();
                  return (pf);
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

                  change_attr (ansi_attr);
                  return (pf);

               default:
                  return (pf);
            }
      }
   }
}

int exist_system_file (char *name)
{
   char linea[80], tmpstr[80], *p;

   strcpy (tmpstr, name);
   if ((p = strchr (tmpstr, '/')) != NULL)
      *(p - 1) = '\0';

   strtrim (tmpstr);

   if (dexists (tmpstr))
      return (-1);

   sprintf (linea, "%s.ASC", tmpstr);
   if (dexists (linea))
      return (-1);

   if (usr.ansi) {
      sprintf (linea, "%s.ANS", tmpstr);
      if (dexists (linea))
         return (-1);
   }

   if (usr.avatar) {
      sprintf (linea, "%s.AVT", tmpstr);
      if (dexists (linea))
         return (-1);
   }

   sprintf (linea, "%s.BBS", tmpstr);
   if (dexists (linea))
      return (-1);

   return (0);
}

