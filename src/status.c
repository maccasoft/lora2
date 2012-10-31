#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <alloc.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <cxl\cxlwin.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"


void status_line(char *format, ...)
{
   va_list var_args;
   char *string;
   struct tm *tim;
   long tempo;

   string=(char *)malloc(256);

   if (string==NULL || strlen(format) > 256)
   {
      if (string)
         free(string);
      return;
   }

   va_start(var_args,format);
   vsprintf(string,format,var_args);
   va_end(var_args);

   tempo = time(0);
   tim   = localtime(&tempo);

   if (frontdoor)
      fprintf(logf,"%c %02d:%02d:%02d  %s\n", string[0], tim->tm_hour, tim->tm_min, tim->tm_sec, &string[1]);
   else
      fprintf(logf,"%c %02d %3s %02d:%02d:%02d LORA %s\n", string[0], tim->tm_mday, mtext[tim->tm_mon], tim->tm_hour, tim->tm_min, tim->tm_sec, &string[1]);

   fflush(logf);
   real_flush(fileno(logf));

   free(string);
}


void local_status(char *format, ...)
{
   va_list var_args;
   char *string;

   string=(char *)malloc(256);

   if (string==NULL || strlen(format) > 256)
   {
      if (string)
         free(string);
      return;
   }

   va_start(var_args,format);
   vsprintf(string,format,var_args);
   va_end(var_args);

   wgotoxy(2,2);
   wclreol();
   wprints(2,2,LCYAN|_BLUE,string);

   free(string);
}

void m_print(char *format, ...)
{
   va_list var_args;
   char *string, *q, visual;
   byte c, a, m;

   string=(char *)malloc(256);

   if (string==NULL || strlen(format) > 256)
   {
      if (string)
         free(string);
      return;
   }

   va_start(var_args,format);
   vsprintf(string,format,var_args);
   va_end(var_args);

   visual = 1;

   for(q=string;*q;q++) {
      if (*q == 0x1B)
         visual = 0;

      if (*q == LF)
      {
         if (!local_mode)
         {
            SENDBYTE('\r');
            SENDBYTE('\n');
         }
         if (snooping)
            wputs("\n");
      }
      else if (*q == CTRLV)
      {
         q++;
         if (*q == CTRLA)
         {
            q++;

            if (*q == CTRLP)
            {
               q++;
               change_attr(*q & 0x7F);
            }
            else
               change_attr(*q);
         }
         else if (*q == CTRLG)
            del_line();
      }
      else if (*q == CTRLL)
         cls ();
      else if (*q == CTRLY)
      {
         c = *(++q);
         a = *(++q);
         if(usr.avatar && !local_mode)
         {
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
      else
      {
         if (!local_mode)
            BUFFER_BYTE((*q));
         if (snooping && visual)
            wputc(*q);
      }
   }

   if (!local_mode)
      UNBUFFER_BYTES ();

   free(string);
}

void show_account ()
{
   m_print (bbstxt[B_MINUTES_LEFT], time_remain ());
   m_print (bbstxt[B_IN_BANK], usr.account);

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
      if (!CARRIER)
         return;
      col = atoi(stringa);
   } while ((col < 0 || col > (time_remain () - 2)) && CARRIER);

   if (col > 0)
   {
      usr.account += col;
      allowed -= col;
      usr.time += col;

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
      if (!CARRIER)
         return;
      col = atoi(stringa);
     if ((time_remain() + col) > time_to_next(1))
     {
        read_system_file ("TIMEWARN");
        col = -1;
        continue;
     }
   } while ((col < 0 || col > usr.account) && CARRIER);

   if (col > 0)
   {
      usr.account -= col;
      allowed += col;
      if (usr.time >= col)
         usr.time -= col;

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

void bbs_add_list ()
{
   int fd;
   char filename [80];
   struct _bbslist bbs;

   memset ((char *)&bbs, 0, sizeof (struct _bbslist));

   cls ();
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

   sprintf (filename, "%sBBSLIST.BBS", sys_path);
   fd = open (filename, O_APPEND|O_WRONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
   write (fd, (char *)&bbs, sizeof (struct _bbslist));
   close (fd);
}

void bbs_short_list ()
{
   int fd, line;
   char filename [80];
   struct _bbslist bbs;

   sprintf (filename, "%sBBSLIST.BBS", sys_path);
   fd = open (filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);

   cls ();
   m_print (bbstxt[B_BBS_SHORTH]);

   line = 3;

   while (read (fd, (char *)&bbs, sizeof (struct _bbslist)) == sizeof (struct _bbslist))
   {
      m_print (bbstxt[B_BBS_SHORTLIST], bbs.bbs_name, bbs.number, bbs.baud, bbs.open_times);
      if (!(line=more_question(line)))
         break;
   }

   close (fd);

   m_print (bbstxt[B_ONE_CR]);
   press_enter ();
}

void bbs_long_list ()
{
   int fd, line;
   char filename [80], name[40];
   struct _bbslist bbs;

   cls ();
   name[0] = 0;
   m_print (bbstxt[B_BBS_NAMETOSEARCH]);
   chars_input (name, 39, INPUT_UPDATE|INPUT_FIELD);
   if (!CARRIER)
      return;

   sprintf (filename, "%sBBSLIST.BBS", sys_path);
   fd = open (filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);

   cls ();
   line = 1;

   while (read (fd, (char *)&bbs, sizeof (struct _bbslist)) == sizeof (struct _bbslist))
   {
      strcpy (filename, bbs.bbs_name);
      if (stristr (filename, name))
      {
         if (!(line = bbs_list (line, &bbs)))
            break;
      }
   }

   close (fd);

   m_print (bbstxt[B_ONE_CR]);
   press_enter ();
}

void bbs_change ()
{
   int fd;
   char filename [80], name[40], found;
   long pos;
   struct _bbslist bbs;

   cls ();
   name[0] = 0;
   m_print (bbstxt[B_BBS_NAMETOCHANGE]);
   chars_input (name, 39, INPUT_FIELD);
   if (!CARRIER || !name[0])
      return;

   sprintf (filename, "%sBBSLIST.BBS", sys_path);
   fd = open (filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);

   cls ();
   found = 0;
   pos = tell (fd);

   while (read (fd, (char *)&bbs, sizeof (struct _bbslist)) == sizeof (struct _bbslist))
   {
      strcpy (filename, bbs.bbs_name);
      if (stristr (filename, name))
      {
         bbs_list (0, &bbs);
         found = 1;
         break;
      }

      pos = tell (fd);
   }

   if (!found)
   {
      close (fd);
      return;
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

void bbs_remove ()
{
   int fd;
   char filename [80], name[40];
   long pos, wpos;
   struct _bbslist bbs;

   cls ();
   m_print ("\nŠEnter BBS name to delete from list : ");
   chars_input (name, 39, INPUT_FIELD);
   if (!CARRIER || !name[0])
      return;

   sprintf (filename, "%sBBSLIST.BBS", sys_path);
   fd = open (filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);

   m_print (bbstxt[B_ONE_CR]);
   wpos = pos = tell (fd);

   while (read (fd, (char *)&bbs, sizeof (struct _bbslist)) == sizeof (struct _bbslist))
   {
      pos = tell (fd);

      strcpy (filename, bbs.bbs_name);
      if (stristr (filename, name))
      {
         bbs_list (0, &bbs);
         m_print ("\nRemove this BBS");
         if (yesno_question (DEF_NO) == DEF_YES)
            continue;
      }

      lseek (fd, wpos, SEEK_SET);
      write (fd, (char *)&bbs, sizeof (struct _bbslist));
      wpos = tell (fd);
      lseek (fd, pos, SEEK_SET);
   }

   chsize (fd, wpos);
   close (fd);
}

