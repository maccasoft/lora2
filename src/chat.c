#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <io.h>
#include <share.h>
#include <fcntl.h>
#include <dos.h>
#include <time.h>
#include <sys/stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"

static int cb_online_users(int, int, int);
static void setfreq (int);
static void speaker (int);
static void chat_wrap (char *, int);
static void cb_send_message (char *, int);

void online_users(flag)
int flag;
{
   int fd, i, line;
   char linea[128], *p;
   struct _useron useron;

   cls();
   sprintf(linea,bbstxt[B_ONLINE_USERS], system_name);
   i = (80 - strlen(linea)) / 2;
   space(i);
   change_attr(WHITE|_BLACK);
   m_print("%s\n",linea);
   change_attr(LRED|_BLACK);
   strnset(linea, '-', strlen(linea));
   i = (80 - strlen(linea)) / 2;
   space(i);
   m_print("%s\n\n",linea);
   m_print(bbstxt[B_ONLINE_HEADER]);
   m_print(bbstxt[B_ONLINE_UNDERLINE]);

   line = 5;
   sprintf(linea,USERON_NAME, sys_path);
   fd = shopen(linea, O_RDONLY|O_BINARY);

   while (read(fd, (char *)&useron, sizeof(struct _useron)) == sizeof(struct _useron)) {
      if (useron.donotdisturb || !useron.name[0])
         continue;

      switch (useron.status) {
      case 0:
         p = "Login   ";
         break;
      case BROWSING:
         p = "Browsing";
         break;
      case UPLDNLD:
         p = "Up/Downl";
         break;
      case READWRITE:
         p = "R/Write ";
         break;
      case DOOR:
         p = "Ext.Door";
         break;
      case CHATTING:
         p = "CB Chat ";
         break;
      case QUESTIONAIRE:
         p = "New user";
         break;
      }

      useron.city[21] = '\0';

      sprintf (linea,"%-29.29s  %4d  %8d  %s  %s\n", useron.name, useron.line, useron.baud, p, useron.city);
      m_print(linea);

      if (!(line = more_question(line)) || !CARRIER)
         break;
   }

   close(fd);
   m_print(bbstxt[B_ONE_CR]);

   if (flag && CARRIER)
      press_enter ();
}
/*
void online_users (flag)
{
   int fd, line;
   char *p, linea[80];
   struct _useron useron;

   m_print("\nUsername                             Node         Status\n");
   m_print("-----------------------------------  ----  -----------------------------\n");

   line = 3;
   sprintf (linea, USERON_NAME, sys_path);
   fd = shopen (linea, O_RDONLY|O_BINARY);

   while (read(fd, (char *)&useron, sizeof(struct _useron)) == sizeof(struct _useron)) {
      if (useron.donotdisturb || !useron.name[0])
         continue;

      switch (useron.status) {
      case 0:
         p = "Login";
         break;
      case BROWSING:
         p = "Available for chat";
         break;
      case UPLDNLD:
         p = "Upload/Download";
         break;
      case READWRITE:
         p = "Read/Write msgs.";
         break;
      case DOOR:
         p = "Ext.Door";
         break;
      case CHATTING:
         p = "Chatting on CB system";
         break;
      case QUESTIONAIRE:
         p = "New user questionnaire";
         break;
      }

      m_print ("%-35s  %4d  %s\n", useron.name, useron.line, p);
      if (!(line = more_question(line)) || !CARRIER)
         break;
   }

   close(fd);
   m_print(bbstxt[B_ONE_CR]);
}
*/

void send_online_message()
{
   FILE *fp;
   int fd, ul;
   char linea[80], filename[80];
   struct _useron useron;

   if (!get_command_word (linea, 4))
   {
      online_users(0);

      change_attr(CYAN|_BLACK);
      m_print(bbstxt[B_SELECT_USER]);
      input(linea, 4);
   }

   if (!strlen(linea) || atoi(linea) <= 0 || !CARRIER)
      return;

   ul = atoi(linea) - 1;
   change_attr(LRED|_BLACK);

   sprintf(filename, USERON_NAME, sys_path);
   fd = shopen(filename, O_RDONLY|O_BINARY);
   lseek(fd, (long)ul * sizeof(struct _useron), SEEK_SET);

   if (read(fd, (char *)&useron, sizeof(struct _useron)) != sizeof(struct _useron) ||
       useron.donotdisturb || !useron.name[0])
   {
      m_print(bbstxt[B_INVALID_LINE]);
      return;
   }

   strltrim (cmd_string);
   if (!cmd_string[0])
   {
      m_print(bbstxt[B_MESSAGE_FOR], useron.name);
      change_attr(WHITE|_BLACK);
      m_print(":");
      input(linea, 77);
      if (!strlen(linea) || !CARRIER)
         return;
   }
   else
   {
      cmd_string[77] = '0';
      strcpy (linea, cmd_string);
      cmd_string[0] = '\0';
   }

   change_attr(CYAN|_BLACK);
   m_print(bbstxt[B_PROCESSING]);

   sprintf(filename, ONLINE_MSGNAME, ipc_path, ul+1);
   fp = fopen(filename, "at");
   fprintf(fp, bbstxt[B_IPC_HEADER]);
   fprintf(fp, bbstxt[B_IPC_FROM], usr.name, line_offset);
   fprintf(fp, bbstxt[B_IPC_MESSAGE], linea);
   fprintf(fp, "\001");
   fclose(fp);

   m_print(bbstxt[B_MESSAGE_SENT]);
   press_enter();
}

void set_useron_record(sta, toggle, cb)
int sta, toggle, cb;
{
   int fd, i;
   char filename[80];
   static char first = 1;
   long prev;
   struct _useron useron;

   sprintf(filename,USERON_NAME, sys_path);
   fd = cshopen(filename, O_CREAT|O_RDWR|O_BINARY,S_IREAD|S_IWRITE);

   memset((char *)&useron, 0, sizeof(struct _useron));

   if (lseek(fd, (line_offset-1) * (long)sizeof(struct _useron), SEEK_SET) == -1)
   {
      for (i=0;i<line_offset;i++)
      {
         prev = tell(fd);
         if (read(fd, (char *)&useron, sizeof(struct _useron)) != sizeof(struct _useron))
            write(fd, (char *)&useron, sizeof(struct _useron));
      }
   }
   else
   {
      prev = tell(fd);
      read(fd, (char *)&useron, sizeof(struct _useron));
   }

   if (first)
   {
      first = 0;
      useron.donotdisturb = usr.quiet;
   }

   strcpy(useron.name, usr.name);
   strcpy(useron.city, usr.city);
   useron.line = line_offset;
   useron.baud = local_mode ? 0 : rate;

   if (sta > 0)
   {
      useron.status = sta;
      user_status = sta;
   }
   if (toggle)
      useron.donotdisturb ^= 1;

   useron.cb_channel = cb;

   lseek(fd, prev, SEEK_SET);
   write(fd, (char *)&useron, sizeof(struct _useron));
   close(fd);
}

int check_multilogon(user_name)
char *user_name;
{
   int fd, rc;
   char filename[80];
   struct _useron useron;

   rc = 0;

   sprintf(filename,USERON_NAME, sys_path);
   fd = shopen(filename, O_RDONLY|O_BINARY);

   while (read(fd, (char *)&useron, sizeof(struct _useron)) == sizeof(struct _useron))
      if (!strcmp(useron.name, user_name))
      {
         rc = 1;
         break;
      }

   close(fd);

   return (rc);
}

void cb_who_is_where (flag)
int flag;
{
   int i, line;

   line = 0;

   for (i = 1; i <= 40; i++)
   {
      if (!(line=cb_online_users (2, i, line)))
         break;
   }

   if (flag && CARRIER && line)
      press_enter();
}

static int cb_online_users(flag, cb_num, line)
int flag, cb_num;
{
        int fd, i, first;
        char linea[82];
        struct _useron useron;

        if (flag != 2)
                cls();
        first = 1;

        line += 5;
        sprintf(linea,USERON_NAME, sys_path);
        fd = shopen(linea, O_RDONLY|O_BINARY);

        while (read(fd, (char *)&useron, sizeof(struct _useron)) == sizeof(struct _useron))
        {
                if (!useron.name[0] || useron.cb_channel != cb_num)
                        continue;

                if (first)
                {
                        sprintf(linea,bbstxt[B_CALLERS_ON_CHANNEL], cb_num);
                        i = (80 - strlen(linea)) / 2;
                        space(i);
                        change_attr(WHITE|_BLACK);
                        m_print("%s\n",linea);
                        if (!(line = more_question(line)) || !CARRIER)
                                break;
                        change_attr(LRED|_BLACK);
                        strnset(linea, '-', strlen(linea));
                        i = (80 - strlen(linea)) / 2;
                        space(i);
                        m_print("%s\n\n",linea);
                        if (!(line = more_question(line)) || !CARRIER)
                                break;
                        m_print(bbstxt[B_CHANNEL_HEADER]);
                        if (!(line = more_question(line)) || !CARRIER)
                                break;
                        m_print(bbstxt[B_CHANNEL_UNDERLINE]);
                        if (!(line = more_question(line)) || !CARRIER)
                                break;

                        first = 0;
                }

                change_attr(LCYAN|_BLACK);
                sprintf(linea,"%-30.30s ", useron.name);
                m_print(linea);
                change_attr(WHITE|_BLACK);
                m_print("%2d    ", useron.line);
                m_print("  %5d   ", useron.baud);
                change_attr(YELLOW|_BLACK);
                sprintf(linea,"%s\n", useron.city);
                linea[24] = '\0';
                m_print(linea);

                if (!(line = more_question(line)) || !CARRIER)
                        break;
        }

        close(fd);

        if (!first)
                m_print("\n");

        if (flag == 1 && CARRIER && line)
                press_enter();

        return (line);
}


void cb_chat()
{
   int fd2, ul, cb_num, endrun, i;
   char linea[80], filename[100], wrp[80];
   long cb_time;

   endrun = 0;
   cb_num = 1;
   set_useron_record(CHATTING, 0, cb_num);
   wrp[0] = '\0';

   cls();
   status_line("+Entering the CB-Chat system");

   cb_online_users(0, cb_num, 0);
   m_print(bbstxt[B_CB_CHAT_HELP1]);
   m_print(bbstxt[B_CB_CHAT_HELP2]);

   sprintf(filename, "\n\n%s joins the conversation.\n\n", usr.name);
   cb_send_message (filename, cb_num);

   cb_time = timerset(50);

   change_attr(LGREEN|_BLACK);
   sprintf(linea, "\n[%-15.15s]: ", usr.name);
   m_print( linea);

   do
   {
      i = local_mode ? local_kbd : PEEKBYTE();

      if (time_remain() <= 0)
      {
         change_attr(LRED|_BLACK);
         m_print(bbstxt[B_TIMEOUT]);
         terminating_call();
         return;
      }

      if (wrp[0])
         i = 0;

      if (i != -1)
      {
         change_attr( (int)color_chat (line_offset) );
         inp_wrap(linea, wrp, 60);

         if (linea[0] && linea[0] != '/')
         {
            sprintf(filename, "�[%-15.15s]: %c%s\n", usr.name, color_chat (line_offset), linea);
            cb_send_message (filename, cb_num);
            cb_time = timerset(50);
         }
         else if (linea[0] == '/')
         {
            switch (toupper(linea[1]))
            {
            case 'C':
               ul = atoi(&linea[2]);
               if (ul >= 1 && ul <= 40)
               {
                  if (cb_num != ul)
                  {
                     sprintf(filename, "\n\n%s leaves the conversation.\n\n", usr.name);
                     cb_send_message (filename, cb_num);
                     sprintf(filename, "\n\n%s joins the conversation.\n\n", usr.name);
                     cb_send_message (filename, ul);
                  }
                  cb_num = ul;
                  set_useron_record(CHATTING, 0, cb_num);
                  status_line("#CB-Chat Channel %d", cb_num);
                  cb_online_users(1, cb_num, 0);
               }
               break;
            case 'H':
               read_system_file("CB_HELP");
               break;
            case 'Q':
               endrun = 1;
               sprintf(filename, "\n\n%s leaves the conversation.\n\n", usr.name);
               cb_send_message (filename, cb_num);
               break;
            case 'W':
               cb_online_users(1, cb_num, 0);
               m_print( "\n");
               break;
            case 'A':
               cb_who_is_where (0);
               m_print( "\n");
               break;
            }
         }

         if (!endrun)
         {
            change_attr(LGREEN|_BLACK);
            sprintf(linea, "[%-15.15s]: ", usr.name);
            m_print( linea);
         }
      }

      if (!timeup(cb_time))
      {
         time_release();
         continue;
      }

      sprintf(filename, CBSIM_NAME, ipc_path, line_offset);
/*
      if (dexists(filename))
      {
         m_print( "\r");
         if (read_file(filename))
            unlink(filename);

         change_attr(LGREEN|_BLACK);
         sprintf(linea, "[%-15.15s]: ", usr.name);
         m_print( linea);
      }
*/
      fd2 = cshopen(filename, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
      if (fd2 != -1)
      {
         if (filelength (fd2) > 0L)
         {
            close (fd2);
            m_print("\r");
            if (read_file(filename))
               unlink (filename);

            change_attr(LGREEN|_BLACK);
            sprintf(linea, "[%-15.15s]: ", usr.name);
            m_print(linea);
         }
      }

      cb_time = timerset(50);
   } while (!endrun && CARRIER);

   if (CARRIER)
   {
      set_useron_record(BROWSING, 0, 0);
      status_line("+Leaving the CB-Chat system");
   }
}

static void cb_send_message (message, cb_num)
char *message;
int cb_num;
{
   int fd, fd2;
   char filename [80];
   long cb_time;
   struct _useron useron;

   sprintf(filename,USERON_NAME, sys_path);
   fd = shopen(filename, O_RDONLY|O_BINARY);

   while (read(fd, (char *)&useron, sizeof(struct _useron)) == sizeof(struct _useron))
   {
      if (!useron.name[0] || useron.cb_channel != cb_num)
         continue;
      if (useron.line == line_offset)
         continue;

      sprintf(filename, CBSIM_NAME, ipc_path, useron.line);
      cb_time = timerset (100);
      do {
         fd2 = sopen(filename, O_CREAT|O_WRONLY|O_APPEND|O_BINARY, SH_DENYRW, S_IREAD|S_IWRITE);
         if (fd2 != -1)
            write (fd2, message, strlen (message));
      } while (!timeup (cb_time) && fd2 == -1);

      if (fd2 == -1)
         status_line ("!CB-CHAT: Locking problems");
      else
         close (fd2);
   }

   close(fd);
}

unsigned char color_chat (task)
int task;
{
   switch (task % 7)
   {
      case 1:
         return (15);
      case 2:
         return (11);
      case 3:
         return (12);
      case 4:
         return (13);
      case 5:
         return (15);
      case 6:
         return (9);
   }

   return (15);
}

void yelling_at_sysop(secs)
{
   FILE *fp;
   int wh;
   char linea[128], s1[20], s2[20], s3[20];
   long t1, maxt;
     
   wh = wopen(10,23,15,55,1,LCYAN|_BLUE,LCYAN|_BLUE);
   wactiv(wh);
   wtitle ("� Sysop Page �", TCENTER, LCYAN|_BLUE);

   wcenters(1,LCYAN|_BLUE,"[C] To break in for a chat");
   wcenters(2,LCYAN|_BLUE,"[A] To terminate the page ");

   maxt = timerset (secs * 100);

   if ((fp = fopen ("PAGE.DAT", "rt")) != NULL)
   {
      for (;;)
      {
         if (timeup (maxt))
            break;

         if (fgets(linea, 127, fp) == NULL)
         {
            rewind (fp);
            if (fgets(linea, 127, fp) == NULL)
               break;
         }

         linea[strlen(linea)-1] = '\0';
         if (!strlen(linea) || linea[0] == ';' || linea[0] == '%')
            continue;

         sscanf(linea,"%s %s %s",s1, s2, s3);

         if (!stricmp (s1, "TONE"))
         {
            setfreq (atoi (s2) );
            t1 = timerset (atoi (s3) );
            speaker (usr.nerd ? 0 : 1);
            while (!timeup (t1))
            {
               if (!CARRIER)
                  return;
               if ( toupper(local_kbd) == 'C')
               {
                  speaker(0);
                  wclose();
                  wunlink (wh);
                  wactiv (mainview);
                  fclose (fp);
                  sysop_chatting ();
                  return;
               }
               if ( toupper(local_kbd) == 'A')
                  break;
               time_release ();
            }
         }
         else if (!stricmp (s1, "WAIT"))
         {
            speaker (0);
            t1 = timerset (atoi (s2) );
            while (!timeup (t1))
            {
               if (!CARRIER)
                  return;
               if ( toupper(local_kbd) == 'C')
               {
                  wclose();
                  wunlink (wh);
                  wactiv (mainview);
                  fclose (fp);
                  sysop_chatting ();
                  return;
               }
               if ( toupper(local_kbd) == 'A')
                  break;
               time_release ();
            }
         }

         if ( toupper(local_kbd) == 'A')
         {
            local_kbd = -1;
            break;
         }
      }

      fclose (fp);
   }

   speaker (0);
   wclose();
   wunlink (wh);
   wactiv (mainview);

   read_system_file ("PAGED");
}


void sysop_chatting ()
{
   char wrp[80];
   long start_write;

   lorainfo.wants_chat = 0;
   if (function_active == 1)
      f1_status();

   if (local_mode)
   {
      sysop_error ();
      return;
   }

   local_kbd = -1;
   start_write = time(NULL);

   m_print(bbstxt[B_BREAKING_CHAT]);
   wrp[0]='\0';

   while (local_kbd != 0x1B)
   {
      chat_wrap(wrp, 79);
      if (!CARRIER)
         return;
   }

   m_print(bbstxt[B_CHAT_END]);
   local_kbd = -1;
   local_mode = 0;

   allowed += (int)((time(NULL)-start_write)/60);
   usr.chat_minutes = (int)((time(NULL)-start_write)/60);
}

static void setfreq(hertz)
int hertz;
{
   unsigned int divisor;

   divisor = (unsigned int)(1193180L/hertz);
   outp(0x43,0xB6);
   outp(0x42,divisor & 0377);
   outp(0x42,divisor >> 8);
}

static void speaker(on)
int on;
{
   int portval;

   portval = inp(0x61);
   if(on)
      portval |= 03;
   else
      portval &=~ 03;
   outp(0x61, portval);
}

static void chat_wrap(wrp, width)
char *wrp;
int width;
{
        static char color = YELLOW|_BLACK;
        char c, s[80];
        int z, i, m;

        strcpy(s,wrp);
        z = strlen(wrp);

        m_print("%c%s", color, wrp);

        while(z < width) {
                if (PEEKBYTE() == -1 && local_kbd == -1)
                {
                        if (!CARRIER)
                                return;
                        time_release();
                        continue;
                }

                if (PEEKBYTE() != -1)
                {
                        c = TIMED_READ(1);
                        if (color != (CYAN+_BLACK))
                        {
                                color = CYAN|_BLACK;
                                change_attr (color);
                        }
                }
                else if (local_kbd != -1) {
                        c = (char)local_kbd;
                        if (c == 0x1B)
                                return;
                        local_kbd = -1;
                        if (color != (YELLOW+_BLACK))
                        {
                                color = YELLOW|_BLACK;
                                change_attr (color);
                        }
                }

                if(c == 0x0D && z == 0) {
                        m_print("\n");
                        s[0]='\0';
                        wrp[0]='\0';
                        return;
                }

                if((c == 0x08 || c == 0x7F) && (z>0)) {
                        s[--z]='\0';
                        SENDBYTE('\b');
                        SENDBYTE(' ');
                        SENDBYTE('\b');
                        if (snooping)
                                wputs("\b \b");
                }

                if(c < 20 && c != 0x0D && c != 0x7F)
                        continue;

                s[z++]=c;

                if(c == 0x0D) {
                        m_print("\n");
                        s[z]='\0';
                        wrp[0]='\0';
                        return;
                }

                SENDBYTE(c);
                if (snooping)
                        wputc(c);
        }

        s[z]='\0';

        while(z > 0 && s[z] != ' ')
                z--;

        m=0;

        if(z != 0) {
                for(i=z+1;i<=width;i++) {
                        SENDBYTE(0x08);
                        wrp[m++]=s[i];
                        if (snooping)
                                wputs("\b");
                        s[i]='\0';
                }

                space(width-z);
        }

        wrp[m]='\0';
        m_print("\n");
}

void broadcast_message (message)
char *message;
{
   int fd, fd2;
   char filename[80];
   struct _useron useron;
   long cb_time;

   sprintf(filename, USERON_NAME, sys_path);
   fd = shopen(filename, O_RDONLY|O_BINARY);

   while (read(fd, (char *)&useron, sizeof(struct _useron)) == sizeof(struct _useron))
   {
      if (!useron.name[0] || useron.donotdisturb)
         continue;
      if (useron.line == line_offset)
         continue;

      sprintf(filename, ONLINE_MSGNAME, ipc_path, useron.line);
      cb_time = timerset (100);
      do {
         fd2 = sopen(filename, O_CREAT|O_WRONLY|O_APPEND|O_BINARY, SH_DENYRW, S_IREAD|S_IWRITE);
         if (fd2 != -1)
             write (fd2, message, strlen (message));
      } while (!timeup (cb_time) && fd2 == -1);
   }

   close(fd);
}