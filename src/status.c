#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <alloc.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <io.h>
#include <dos.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <cxl\cxlwin.h>
#include <cxl\cxlvid.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

#ifdef __OS2__
void VioUpdate (void);
#endif
int scrollbox(int wsrow,int wscol,int werow,int wecol,int count,int direction);

static char *string = NULL;

void open_logfile (void)
{
   if ((logf = sh_fopen (log_name, "at", SH_DENYNONE)) == NULL)
      return;

   if (config->logbuffer > 64)
      config->logbuffer = 64;
   if (config->logbuffer < 0)
      config->logbuffer = 0;

   setvbuf (logf, NULL, _IOFBF, (int)config->logbuffer * 1024);
}

void status_line (char *format, ...)
{
   va_list var_args;
   char tmpdata[80];
   struct tm *tim;
   long tempo;

   if (string == NULL)
      string = (char *)malloc (2048);

   if (string == NULL || strlen (format) > 256)
      return;

   va_start(var_args,format);
   vsprintf(string,format,var_args);
   va_end(var_args);

   tempo = time(0);
   tim   = localtime(&tempo);

   if (logf != NULL) {
      if (frontdoor)
         fprintf(logf,"%c %02d:%02d:%02d  %s\n", string[0], tim->tm_hour, tim->tm_min, tim->tm_sec, &string[1]);
      else
         fprintf(logf,"%c %02d %3s %02d:%02d:%02d LORA %s\n", string[0], tim->tm_mday, mtext[tim->tm_mon], tim->tm_hour, tim->tm_min, tim->tm_sec, &string[1]);
   }

   if (!caller && !emulator) {
      string[44] = '\0';
      sprintf (tmpdata, "%c %02d:%02d %s", string[0], tim->tm_hour, tim->tm_min, &string[1]);
      scrollbox (2, 1, 11, 51, 1, D_UP);
      prints (11, 1, WHITE|_BLACK, tmpdata);
   }

//   fflush(logf);
#ifndef __OS2__
//   real_flush(fileno(logf));
#endif
}

extern long elapsed;

void local_status(char *format, ...)
{
   va_list var_args;

   if (string == NULL)
      string = (char *)malloc (2048);

   if (string==NULL || strlen(format) > 256)
      return;

   va_start(var_args,format);
   vsprintf(string,format,var_args);
   va_end(var_args);

   if (strlen (string)) {
      prints (5, 65, YELLOW|_BLACK, "              ");
      prints (5, 65, YELLOW|_BLACK, string);
      elapsed = time (NULL);
   }

#ifdef __OS2__
   VioUpdate ();
#endif
}

void m_print(char *format, ...)
{
   va_list var_args;
   char *q, visual, strm[2];
   byte c, a, m;

   if (string == NULL)
      string = (char *)malloc (2048);

   if (string==NULL || strlen(format) > 256)
      return;

   va_start(var_args,format);
   vsprintf(string,format,var_args);
   va_end(var_args);

   visual = 1;

   for(q=string;*q;q++) {
      if (*q == 0x1B)
         visual = 0;

      if (*q == LF) {
         if (!local_mode) {
            BUFFER_BYTE ('\r');
            BUFFER_BYTE ('\n');
         }
         if (snooping) {
            wputc ('\r');
            wputc ('\n');
         }
      }
      else if (*q == CTRLV) {
         q++;
         if (*q == CTRLA) {
            q++;

            if (*q == CTRLP) {
               q++;
               change_attr(*q & 0x7F);
            }
            else
               change_attr(*q);
         }
         else if (*q == CTRLG)
            del_line();
         else if (*q == CTRLC)
            cup (1);
         else if (*q == CTRLD)
            cdo (1);
         else if (*q == CTRLE)
            cle (1);
         else if (*q == CTRLF)
            cri (1);
         else if (*q == CTRLH) {
            cpos ( *(q+1), *(q+2) );
            q += 2;
         }
      }
      else if (*q == CTRLL)
         cls ();
      else if (*q == CTRLY) {
         c = *(++q);
         if (!usr.ibmset && (unsigned char)c >= 128)
            c = bbstxt[B_ASCII_CONV][(unsigned char)c - 128];
         a = *(++q);
         if(usr.avatar && !local_mode) {
            BUFFER_BYTE (CTRLY);
            BUFFER_BYTE (c);
            BUFFER_BYTE (a);
         }
         else if (!local_mode)
            for(m=0;m<a;m++)
               BUFFER_BYTE(c);
         if (snooping)
            wdupc (c, a);
      }
      else if (*q == CTRLA) {
         m_print (bbstxt[B_PRESS_ENTER]);
         input (strm, 0);
      }
      else {
         c = *q;
         if (!usr.ibmset && (unsigned char)c >= 128)
            c = bbstxt[B_ASCII_CONV][c - 128];
         if (!local_mode)
            BUFFER_BYTE (c);
         if (snooping && visual)
            wputc(c);
      }
   }

#ifndef __OS2__
   if (!local_mode)
      UNBUFFER_BYTES ();
#endif
}

void m_print2(char *format, ...)
{
   va_list var_args;
   char *q, visual, strm[2];
   byte c, a, m;

   if (string == NULL)
      string = (char *)malloc (2048);

   if (string==NULL || strlen(format) > 256)
      return;

   va_start(var_args,format);
   vsprintf(string,format,var_args);
   va_end(var_args);

   visual = 1;

   for(q=string;*q;q++) {
      if (*q == 0x1B)
         visual = 0;

      if (*q == LF) {
         if (!local_mode) {
            BUFFER_BYTE('\r');
            BUFFER_BYTE('\n');
         }
         if (snooping) {
            wputc ('\r');
            wputc ('\n');
         }
      }
      else if (*q == CTRLV) {
         q++;
         if (*q == CTRLA) {
            q++;

            if (*q == CTRLP) {
               q++;
               change_attr(*q & 0x7F);
            }
            else
               change_attr(*q);
         }
         else if (*q == CTRLG)
            del_line();
         else if (*q == CTRLC)
            cup (1);
         else if (*q == CTRLD)
            cdo (1);
         else if (*q == CTRLE)
            cle (1);
         else if (*q == CTRLF)
            cri (1);
         else if (*q == CTRLH) {
            cpos ( *(q+1), *(q+2) );
            q += 2;
         }
      }
      else if (*q == CTRLL)
         cls ();
      else if (*q == CTRLY) {
         c = *(++q);
         if (!usr.ibmset && (unsigned char)c >= 128)
            c = bbstxt[B_ASCII_CONV][(unsigned char)c - 128];
         a = *(++q);
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
      }
      else if (*q == CTRLA) {
         m_print (bbstxt[B_PRESS_ENTER]);
         input (strm, 0);
      }
      else {
         c = *q;
         if (!usr.ibmset && (unsigned char)c >= 128)
            c = bbstxt[B_ASCII_CONV][c - 128];
         if (!local_mode)
            BUFFER_BYTE(c);
         if (snooping && visual)
            wputc(c);
      }
   }

   if (!local_mode)
      UNBUFFER_BYTES ();

#ifdef __OS2__
   VioUpdate ();
#endif
}

void show_account ()
{
   m_print (bbstxt[B_MINUTES_LEFT], time_remain ());
   m_print (bbstxt[B_IN_BANK], usr.account);
   m_print (bbstxt[B_KBYTES_LEFT], config->class[usr_class].max_dl - usr.dnldl);
   m_print (bbstxt[B_K_IN_BANK], usr.f_account);

   press_enter ();
}

void deposit_time ()
{
   int col;
   char stringa[6];

   m_print (bbstxt[B_MINUTES_LEFT], time_remain ());
   m_print (bbstxt[B_IN_BANK], usr.account);
   m_print (bbstxt[B_CAN_DEPOSIT], time_remain () - 2);

   do {
      m_print(bbstxt[B_HOW_MUCH_DEPOSIT]);
      input (stringa, 4);
      if (!CARRIER || !stringa[0])
         return;
      col = atoi(stringa);
   } while ((col < 0 || col > (time_remain () - 2)) && CARRIER);

   if (col > 0) {
      usr.account += col;
      allowed -= col;
      usr.time += col;

      show_account ();
   }
}

void deposit_kbytes ()
{
   int col;
   char stringa[6];

   m_print (bbstxt[B_KBYTES_LEFT], config->class[usr_class].max_dl - usr.dnldl);
   m_print (bbstxt[B_K_IN_BANK], usr.f_account);
   m_print (bbstxt[B_K_CAN_DEPOSIT], config->class[usr_class].max_dl - usr.dnldl);

   do {
      m_print(bbstxt[B_HOW_MUCH_DEPOSIT]);
      input (stringa, 4);
      if (!CARRIER || !stringa[0])
         return;
      col = atoi(stringa);
   } while ((col < 0 || col > (config->class[usr_class].max_dl - usr.dnldl)) && CARRIER);

   if (col > 0) {
      usr.f_account += col;
      usr.dnldl += col;

      show_account ();
   }
}

void withdraw_time ()
{
   int col;
   char stringa[6];

   m_print (bbstxt[B_MINUTES_LEFT], time_remain ());
   m_print (bbstxt[B_IN_BANK], usr.account);

   do {
      m_print(bbstxt[B_HOW_MUCH_WITHDRAW]);
      input (stringa, 4);
      if (!CARRIER || !stringa[0])
         return;
      col = atoi(stringa);
      if ((time_remain() + col) > time_to_next(1)) {
         read_system_file ("TIMEWARN");
         col = -1;
         continue;
      }
   } while ((col < 0 || col > usr.account) && CARRIER);

   if (col > 0) {
      usr.account -= col;
      allowed += col;
      if (usr.time >= col)
         usr.time -= col;

      show_account ();
   }
}

void withdraw_kbytes ()
{
   int col;
   char stringa[6];

   m_print (bbstxt[B_KBYTES_LEFT], config->class[usr_class].max_dl - usr.dnldl);
   m_print (bbstxt[B_K_IN_BANK], usr.f_account);

   do {
      m_print(bbstxt[B_HOW_MUCH_WITHDRAW]);
      input (stringa, 4);
      if (!CARRIER || !stringa[0])
         return;
      col = atoi(stringa);
   } while ((col < 0 || col > usr.f_account) && CARRIER);

   if (col > 0) {
      usr.f_account -= col;
      usr.dnldl -= col;

      show_account ();
   }
}

int bbs_list (line, bbs)
int line;
struct _bbslist *bbs;
{
   m_print (bbstxt[B_BBSLIST_UNDERLINE]);
   if (!(line=more_question(line)))
      return (0);
   m_print (bbstxt[B_BBS_NAME], bbs->bbs_name);
   if (!(line=more_question(line)))
      return (0);
   m_print (bbstxt[B_BBS_PHONE], bbs->number);
   if (!(line=more_question(line)))
      return (0);
   m_print (bbstxt[B_BBS_BAUD], bbs->baud);
   if (!(line=more_question(line)))
      return (0);
   m_print (bbstxt[B_BBS_OPEN], bbs->open_times);
   if (!(line=more_question(line)))
      return (0);
   m_print (bbstxt[B_BBS_SOFT], bbs->bbs_soft);
   if (!(line=more_question(line)))
      return (0);
   m_print (bbstxt[B_BBS_NET], bbs->net);
   if (!(line=more_question(line)))
      return (0);
   m_print (bbstxt[B_BBS_SYSOP], bbs->sysop_name);
   if (!(line=more_question(line)))
      return (0);
   m_print (bbstxt[B_BBS_OTHER], bbs->other);
   if (!(line=more_question(line)))
      return (0);

   return (line);
}

void bbs_list_download (num)
int num;
{
   FILE *fp;
   int fd;
   char filename[80];
   struct _bbslist bbs;

   if (num)
      sprintf (filename, "%sBBSLIST%d.BBS", config->sys_path, num);
   else
      sprintf (filename, "%sBBSLIST.BBS", config->sys_path);
   fd = cshopen (filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);

   sprintf (filename, "%sBBS%d.TXT", config->sys_path, line_offset);
   fp = fopen (filename, "wt");

   while (read (fd, (char *)&bbs, sizeof (struct _bbslist)) == sizeof (struct _bbslist)) {
      fprintf (fp, bbstxt[B_FBBSLIST_UNDERLINE]);
      fprintf (fp, bbstxt[B_FBBS_NAME], bbs.bbs_name);
      fprintf (fp, bbstxt[B_FBBS_PHONE], bbs.number);
      fprintf (fp, bbstxt[B_FBBS_BAUD], bbs.baud);
      fprintf (fp, bbstxt[B_FBBS_OPEN], bbs.open_times);
      fprintf (fp, bbstxt[B_FBBS_SOFT], bbs.bbs_soft);
      fprintf (fp, bbstxt[B_FBBS_NET], bbs.net);
      fprintf (fp, bbstxt[B_FBBS_SYSOP], bbs.sysop_name);
      fprintf (fp, bbstxt[B_FBBS_OTHER], bbs.other);
   }

   fprintf (fp, bbstxt[B_FBBSLIST_UNDERLINE]);

   fclose (fp);
   close (fd);

   download_file (filename, 0);
}

void bbs_add_list (num)
int num;
{
   int fd;
   char filename [80];
   struct _bbslist bbs;

   memset ((char *)&bbs, 0, sizeof (struct _bbslist));

   m_print (bbstxt[B_BBS_ASKNAME]);
   chars_input (bbs.bbs_name, 39, INPUT_FIELD);
   if (!bbs.bbs_name[0])
      return;
   m_print (bbstxt[B_BBS_ASKSYSOP]);
   sprintf (bbs.sysop_name, usr.name);
   chars_input (bbs.sysop_name, 35, INPUT_FIELD|INPUT_UPDATE);
   if (!bbs.sysop_name[0])
      return;
   m_print (bbstxt[B_BBS_ASKNUMBER]);
   chars_input (bbs.number, 19, INPUT_FIELD);
   if (!bbs.number[0])
      return;
   m_print (bbstxt[B_BBS_ASKBAUD]);
   chars_input (filename, 5, INPUT_FIELD);
   if (!filename[0])
      return;
   bbs.baud = (unsigned int)atoi (filename);
   m_print (bbstxt[B_BBS_ASKOPEN]);
   chars_input (bbs.open_times, 12, INPUT_FIELD);
   if (!bbs.open_times[0])
      return;
   m_print (bbstxt[B_BBS_ASKSOFT]);
   chars_input (bbs.bbs_soft, 15, INPUT_FIELD);
   if (!bbs.bbs_soft[0])
      return;
   m_print (bbstxt[B_BBS_ASKNET]);
   chars_input (bbs.net, 15, INPUT_FIELD);
   m_print (bbstxt[B_BBS_ASKOTHER]);
   chars_input (bbs.other, 59, INPUT_FIELD);

   bbs_list (0, &bbs);

   m_print (bbstxt[B_PHONE_OK]);
   if (yesno_question (DEF_YES) == DEF_NO)
      return;

   if (num)
      sprintf (filename, "%sBBSLIST%d.BBS", config->sys_path, num);
   else
      sprintf (filename, "%sBBSLIST.BBS", config->sys_path);
   fd = cshopen (filename, O_APPEND|O_WRONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
   write (fd, (char *)&bbs, sizeof (struct _bbslist));
   close (fd);
}

void bbs_short_list (num)
int num;
{
   int fd, line;
   char filename [80];
   struct _bbslist bbs;

   if (num)
      sprintf (filename, "%sBBSLIST%d.BBS", config->sys_path, num);
   else
      sprintf (filename, "%sBBSLIST.BBS", config->sys_path);
   fd = cshopen (filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);

   cls ();
   m_print (bbstxt[B_BBS_SHORTH]);

   line = 3;

   while (read (fd, (char *)&bbs, sizeof (struct _bbslist)) == sizeof (struct _bbslist)) {
      m_print (bbstxt[B_BBS_SHORTLIST], bbs.bbs_name, bbs.number, bbs.baud, bbs.open_times);
      if (!(line=more_question(line)))
         break;
   }

   close (fd);

   m_print (bbstxt[B_ONE_CR]);
   press_enter ();
}

void bbs_long_list (num)
int num;
{
   int fd, line;
   char filename [80], name[40];
   struct _bbslist bbs;

   name[0] = 0;
   m_print (bbstxt[B_BBS_NAMETOSEARCH]);
   chars_input (name, 39, INPUT_UPDATE|INPUT_FIELD);
   if (!CARRIER)
      return;

   if (num)
      sprintf (filename, "%sBBSLIST%d.BBS", config->sys_path, num);
   else
      sprintf (filename, "%sBBSLIST.BBS", config->sys_path);
   fd = cshopen (filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);

   cls ();
   line = 1;

   while (read (fd, (char *)&bbs, sizeof (struct _bbslist)) == sizeof (struct _bbslist)) {
      strcpy (filename, bbs.bbs_name);
      if (!name[0] || stristr (filename, name)) {
         if (!(line = bbs_list (line, &bbs)))
            break;
      }
   }

   close (fd);

   m_print (bbstxt[B_ONE_CR]);
   press_enter ();
}

void bbs_change (int num, int restricted)
{
   int fd;
   char filename [80], name[40], found;
   long pos;
   struct _bbslist bbs;

   name[0] = 0;
   m_print (bbstxt[B_BBS_NAMETOCHANGE]);
   chars_input (name, 39, INPUT_FIELD);
   if (!CARRIER || !name[0])
      return;

   if (num)
      sprintf (filename, "%sBBSLIST%d.BBS", config->sys_path, num);
   else
      sprintf (filename, "%sBBSLIST.BBS", config->sys_path);
   fd = sh_open (filename, SH_DENYNONE, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);

   cls ();
   found = 0;
   pos = tell (fd);

   while (read (fd, (char *)&bbs, sizeof (struct _bbslist)) == sizeof (struct _bbslist)) {
      strcpy (filename, bbs.bbs_name);
      if (stristr (filename, name)) {
         bbs_list (0, &bbs);
         found = 1;
         break;
      }

      pos = tell (fd);
   }

   if (!found) {
      close (fd);
      return;
   }

   if (restricted) {
      if (usr.priv < SYSOP && stricmp (usr.name, bbs.sysop_name) && stricmp (usr.handle, bbs.sysop_name)) {
         m_print (bbstxt[B_BBSLIST_NOCHANGE]);
         return;
      }
   }

   cpos (2, 1);
   m_print (bbstxt[B_BBS_ASKNAME]);
   chars_input (bbs.bbs_name, 39, INPUT_UPDATE|INPUT_FIELD);
   m_print (bbstxt[B_BBS_ASKNUMBER]);
   chars_input (bbs.number, 19, INPUT_UPDATE|INPUT_FIELD);
   m_print (bbstxt[B_BBS_ASKBAUD]);
   sprintf (filename, "%u", bbs.baud);
   chars_input (filename, 5, INPUT_UPDATE|INPUT_FIELD);
   bbs.baud = (unsigned int)atoi (filename);
   m_print (bbstxt[B_BBS_ASKOPEN]);
   chars_input (bbs.open_times, 12, INPUT_UPDATE|INPUT_FIELD);
   m_print (bbstxt[B_BBS_ASKSOFT]);
   chars_input (bbs.bbs_soft, 15, INPUT_UPDATE|INPUT_FIELD);
   m_print (bbstxt[B_BBS_ASKNET]);
   chars_input (bbs.net, 15, INPUT_UPDATE|INPUT_FIELD);
   m_print (bbstxt[B_BBS_ASKSYSOP]);
   chars_input (bbs.sysop_name, 35, INPUT_UPDATE|INPUT_FIELD);
   m_print (bbstxt[B_BBS_ASKOTHER]);
   chars_input (bbs.other, 59, INPUT_UPDATE|INPUT_FIELD);

   m_print (bbstxt[B_PHONE_OK]);
   if (yesno_question (DEF_YES) == DEF_NO)
      return;

   lseek (fd, pos, SEEK_SET);
   write (fd, (char *)&bbs, sizeof (struct _bbslist));
   close (fd);
}

void bbs_remove (int num, int restricted)
{
   int fd;
   char filename [80], name[40];
   long pos, wpos;
   struct _bbslist bbs;

   cls ();
   m_print (bbstxt[B_BBS_DELETE]);
   chars_input (name, 39, INPUT_FIELD);
   if (!CARRIER || !name[0])
      return;

   if (num)
      sprintf (filename, "%sBBSLIST%d.BBS", config->sys_path, num);
   else
      sprintf (filename, "%sBBSLIST.BBS", config->sys_path);
   fd = cshopen (filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);

   m_print (bbstxt[B_ONE_CR]);
   wpos = pos = tell (fd);

   while (read (fd, (char *)&bbs, sizeof (struct _bbslist)) == sizeof (struct _bbslist)) {
      pos = tell (fd);

      strcpy (filename, bbs.bbs_name);
      if (stristr (filename, name)) {
         bbs_list (0, &bbs);
         m_print (bbstxt[B_BBSLIST_REMOVE]);
         if (yesno_question (DEF_NO) == DEF_YES) {
            if (restricted) {
               if (usr.priv < SYSOP && stricmp (usr.name, bbs.sysop_name) && stricmp (usr.handle, bbs.sysop_name))
                  m_print (bbstxt[B_BBSLIST_NODELETE]);
               else
                  continue;
            }
            else
               continue;
         }
      }

      lseek (fd, wpos, SEEK_SET);
      write (fd, (char *)&bbs, sizeof (struct _bbslist));
      wpos = tell (fd);
      lseek (fd, pos, SEEK_SET);
   }

   chsize (fd, wpos);
   close (fd);
}

/*
void vote_user (n)
int n;
{
   int fd;
   char stringa[40], linea[80];
   long prev;
   struct _usr tempusr;

   if (vote_limit <= 0) {
      read_system_file ("VOTELIM");
      return;
   }


   if (!get_command_word (stringa, 35)) {
      read_system_file ("PREVOTE");
      m_print(bbstxt[B_VOTE_NAME]);
      chars_input(stringa, 35, INPUT_FANCY|INPUT_FIELD);
      if (!CARRIER || !stringa[0])
         return;
   }

   sprintf(linea, "%s.BBS", config->user_file);
   fd=shopen(linea,O_RDWR|O_BINARY);
   prev = tell(fd);

   while(read(fd,(char *)&tempusr,sizeof(struct _usr)) == sizeof (struct _usr)) {
      if (tempusr.usrhidden || tempusr.deleted || !tempusr.name[0])
         continue;

      if (tempusr.priv != vote_priv || tempusr.novote || !strcmp (tempusr.name, usr.name))
         continue;

      if (stristr(tempusr.name, stringa)) {
         fancy_str (tempusr.name);
         m_print(bbstxt[B_VOTE_OK], n > 0 ? bbstxt[B_VOTE_FOR] : bbstxt[B_VOTE_AGAINST], tempusr.name);
         if (yesno_question (DEF_YES) == DEF_YES) {
            tempusr.votes += n;
            lseek(fd,prev,SEEK_SET);
            write(fd,(char *)&tempusr,sizeof(struct _usr));
            m_print (bbstxt[B_VOTE_COLLECTED]);
            status_line ("+Vote sent for %s", tempusr.name);
         }

         vote_limit--;
         if (vote_limit <= 0) {
            read_system_file ("VOTELIM");
            break;
         }
      }

      prev = tell(fd);
   }
}

void ballot_votes ()
{
   if (usr.priv != vote_priv || usr.novote)
      return;

   if (usr.votes >= target_up && target_up && up_priv) {
      usr.priv = up_priv;
      read_system_file ("VOTEUP");
      usr.votes = 0;
   }
   else if (usr.votes <= target_down && target_down && down_priv) {
      usr.priv = down_priv;
      read_system_file ("VOTEDOWN");
      usr.votes = 0;
   }
}
*/

