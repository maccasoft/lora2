#include <stdio.h>
#include <io.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <alloc.h>
#include <sys\stat.h>

#include <cxl\cxlstr.h>
#include <cxl\cxlvid.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"

#define MAX_INDEX    500

void f1_special_status(char *, char *, char *);

static int is_trashpwd (char *);
static int is_trashcan (char *);

int login_user()
{
   int fd, fflag, posit, m, original_time, i, xmq;
   char stringa[36], filename[80], c;
   long crc;
   struct _usr tusr;
   struct _usridx usridx[MAX_INDEX];

   memset((char *)&lorainfo,0,sizeof(struct _lorainfo));

   start_time = time(NULL);

   xmq = allowed;
   if (!allowed)
      allowed = time_to_next (1);
   if (allowed > 10)
      allowed = 10;
   usr_class = get_class(logon_priv);
   user_status = 0;

   CLEAR_INBOUND();

   if (noask.ansilogon) {
      if (noask.ansilogon == 3)
         usr.ansi = usr.color = usr.formfeed = 1;
      else if (local_mode || noask.ansilogon == 2) {
         m_print (bbstxt[B_ANSI_LOGON]);
         if (yesno_question (DEF_YES) == DEF_YES)
            usr.ibmset = usr.ansi = usr.color = usr.formfeed = 1;
      }
      else if (noask.ansilogon == 1) {
         SENDBYTE (0x1B);
         SENDBYTE ('[');
         SENDBYTE ('6');
         SENDBYTE ('n');

         i = 0;
         while ((m = TIMED_READ (1)) != -1) {
            if (i == 0 && m != 0x1B)
               break;
            if (i == 1 && m != '[')
               break;
            if (i == 2 && m == 'R')
               break;
            if (i == 2 && !isdigit(m) && m != ';')
               break;
            if (i < 2)
               i++;
         }

         if (i == 2 && m == 'R')
            usr.ibmset = usr.ansi = usr.color = usr.formfeed = 1;
      }
   }

   read_system_file("LOGO");

new_login:
   stringa[0] = 0;

   m_print("\n%s%s login.\n", VERSION, registered ? "" : NOREG);

   do {
      m_print(bbstxt[B_FULLNAME]);
      chars_input(stringa, 35, INPUT_FANCY|INPUT_FIELD);
      if (!CARRIER)
         return (0);
   } while (strlen(stringa) < 5);

   fancy_str(stringa);

   if (is_trashcan (stringa)) {
      status_line ("!Trashcan user `%s'", stringa);
      read_system_file ("TRASHCAN");
      return (0);
   }

   status_line(msgtxt[M_USER_CALLING], stringa);
   crc = crc_name (stringa);

   sprintf (filename, "%s.IDX", user_file);
   fd = shopen (filename, O_RDONLY|O_BINARY);

   fflag = 0;
   posit = 0;

   do {
      i = read(fd,(char *)&usridx,sizeof(struct _usridx) * MAX_INDEX);
      m = i / sizeof (struct _usridx);

      for (i=0; i < m; i++)
         if (usridx[i].id == crc) {
            m = 0;
            posit += i;
            fflag = 1;
            break;
         }

      if (!fflag)
         posit += m;
   } while (m == MAX_INDEX && !fflag);

   close (fd);

   sprintf (filename, "%s.BBS", user_file);

   fd = shopen (filename, O_RDWR|O_BINARY);
   lseek (fd, (long)posit * sizeof (struct _usr), SEEK_SET);
   read(fd, (char *)&tusr, sizeof(struct _usr));
   close (fd);

   if (!fflag) {
      status_line(msgtxt[M_NOT_IN_LIST],stringa);

      m_print(bbstxt[B_TWO_CR]);

      if (check_multilogon(stringa)) {
         read_system_file("1ATATIME");
         return (0);
      }

      m_print(bbstxt[B_NAME_NOT_FOUND]);
      m_print(bbstxt[B_NAME_ENTERED], stringa);

      do {
         m_print(bbstxt[B_NEW_USER]);
         c = yesno_question (DEF_NO|QUESTION);
         if (c == QUESTION)
            read_system_file("WHY_NEW");
            if (!CARRIER)
               return (1);
      } while(c != DEF_NO && c != DEF_YES);

      if (c == DEF_NO)
         goto new_login;

      if (logon_priv == HIDDEN || (!registered && posit >= 50)) {
         read_system_file ("PREREG");
         return (0);
      }

      if (!new_user(stringa))
         goto new_login;
      if (!CARRIER)
         return (0);

      sysinfo.new_users++;
   }
   else {
      m = 1;
      m_print(bbstxt[B_ONE_CR]);

      free (bbstxt);
      if (!load_language (tusr.language)) {
         tusr.language = 0;
         load_language (tusr.language);
      }
      if (lang_txtpath[tusr.language] != NULL)
         text_path = lang_txtpath[tusr.language];
      else
         text_path = lang_txtpath[0];
      usr.ansi = tusr.ansi;
      usr.avatar = tusr.avatar;
      usr.color = tusr.color;

      for(;;) {
         m_print(bbstxt[B_PASSWORD]);
         chars_input(stringa,15,INPUT_PWD|INPUT_FIELD);
         if (!CARRIER)
            return (0);

         if(!stricmp(tusr.pwd,strcode(strupr(stringa),tusr.name)))
            break;

         m_print(bbstxt[B_BAD_PASSWORD]);
         status_line(msgtxt[M_BAD_PASSWORD],strcode(stringa,tusr.name));

         if(++m > 4) {
            if (!read_system_file("BADPWD"))
               m_print (bbstxt[B_DENIED]);
            status_line(msgtxt[M_INVALID_PASSWORD]);

            if (registered) {
               tusr.badpwd = 1;
               memcpy((char *)&usr,(char *)&tusr,sizeof(struct _usr));
            }

            sysinfo.failed_password++;

            return (0);
         }
      }

      if (check_multilogon(tusr.name)) {
         read_system_file("1ATATIME");
         return (0);
      }

      memcpy((char *)&usr,(char *)&tusr,sizeof(struct _usr));
   }

   set_useron_record(0, 0, 0);
   sysinfo.total_calls++;
   if (local_mode)
      sysinfo.local_logons++;
   else {
      switch (rate) {
         case 300:
            sysinfo.calls_300++;
            break;
         case 1200:
            sysinfo.calls_1200++;
            break;
         case 2400:
         case 4800:
         case 7200:
            sysinfo.calls_2400++;
            break;
         case 9600:
         case 12000:
            sysinfo.calls_9600++;
            break;
         case 14400:
            sysinfo.calls_14400++;
            break;
      }
   }

   data(stringa);
   if(strncmp(stringa,usr.ldate,9))
   {
      usr.time = 0;
      usr.dnldl = 0;
      usr.chat_minutes = 0;
   }

   usr_class = get_class(usr.priv);

   if (usr.ovr_class.max_time)
      class[usr_class].max_time = usr.ovr_class.max_time;
   if (usr.ovr_class.max_call)
      class[usr_class].max_call = usr.ovr_class.max_call;
   if (usr.ovr_class.max_dl)
      class[usr_class].max_dl = usr.ovr_class.max_dl;
   else
   {
      switch (rate)
      {
      case 300:
         if (class[usr_class].dl_300)
            class[usr_class].max_dl = class[usr_class].dl_300;
         break;
      case 1200:
         if (class[usr_class].dl_1200)
            class[usr_class].max_dl = class[usr_class].dl_1200;
         break;
      case 2400:
      case 4800:
      case 7200:
         if (class[usr_class].dl_2400)
            class[usr_class].max_dl = class[usr_class].dl_2400;
         break;
      case 9600:
      case 12000:
      case 14400:
         if (class[usr_class].dl_9600)
            class[usr_class].max_dl = class[usr_class].dl_9600;
         break;
      }
   }
   if (usr.ovr_class.ratio)
      class[usr_class].ratio = usr.ovr_class.ratio;
   if (usr.ovr_class.min_baud)
      class[usr_class].min_baud = usr.ovr_class.min_baud;
   if (usr.ovr_class.min_file_baud)
      class[usr_class].min_file_baud = usr.ovr_class.min_file_baud;
   if (usr.ovr_class.start_ratio)
      class[usr_class].start_ratio = usr.ovr_class.start_ratio;

   original_time = class[usr_class].max_call;
   if ((usr.time + original_time) > class[usr_class].max_time)
      original_time = class[usr_class].max_time - usr.time;

   if (usr.times)
      status_line(msgtxt[M_USER_LAST_TIME], usr.ldate);

   if(strncmp(stringa,usr.ldate,9))
           status_line(msgtxt[M_TIMEDL_ZEROED]);

   f1_status();

   strcpy (lorainfo.logindate, stringa);
   usr.times++;

   if(usr.badpwd && registered)
   {
      read_system_file("WARNPWD");
      usr.badpwd = 0;
   }

   if(original_time <= 0)
   {
      read_system_file("DAYLIMIT");
      FLUSH_OUTPUT();
      modem_hangup();
      return(0);
   }

   allowed = xmq ? xmq : time_to_next (1);

   if (original_time < allowed)
      allowed = original_time;
   else
      read_system_file ("TIMEWARN");

   status_line(msgtxt[M_GIVEN_LEVEL], time_remain (), get_priv_text (usr.priv));

   if (usr.baud_rate && rate)
   {
      if (usr.baud_rate < rate)
         read_system_file ("MOREBAUD");
      if (usr.baud_rate > rate)
         read_system_file ("LESSBAUD");
   }

   usr.baud_rate = rate;

   sprintf (filename, "%s.BBS", user_file);

   fd = shopen (filename, O_RDWR|O_BINARY);
   lseek (fd, (long)posit * sizeof (struct _usr), SEEK_SET);
   write(fd,(char *)&usr,sizeof(struct _usr));
   close(fd);

   lorainfo.posuser = posit;

   if(rate < class[usr_class].min_baud)
   {
      read_system_file("TOOSLOW");
      FLUSH_OUTPUT();
      modem_hangup();
      return (0);
   }

   read_system_file("WELCOME");
   if (!CARRIER)
      return (0);

   read_hourly_files ();
   read_comment ();

   data(stringa);
   if (!strncmp(stringa,usr.birthdate,6))
      read_system_file("BIRTHDAY");

   if(usr.times > 1) {
      if(usr.times < min_calls)
         read_system_file("ROOKIE");

      sprintf(filename,"%s%d",sys_path,posit);
      if(dexists(filename))
         read_file(filename);
   }
   else
      read_system_file("NEWUSER2");

   ballot_votes ();

   if (!CARRIER)
      return (0);

   return (1);
}

int new_user(buff)
char *buff;
{
        int c, i, fd, k;
        char stringa[40], filename[80];
        struct _usr tusr;
        struct _usridx usridx;

        memset((char *)&tusr,0,sizeof(struct _usr));
        memset((char *)&usr,0,sizeof(struct _usr));

        while ((c = select_language ()) == -1 && CARRIER);
        tusr.language = (byte) c;

        if (!CARRIER)
                return (0);
        free (bbstxt);
        if (!load_language (tusr.language))
        {
                tusr.language = 0;
                load_language (tusr.language);
        }
        if (lang_txtpath[tusr.language] != NULL)
                text_path = lang_txtpath[tusr.language];
        else
                text_path = lang_txtpath[0];

        read_system_file("APPLIC");

        strcpy(tusr.name, buff);
        strcpy(tusr.handle, buff);

        m_print(bbstxt[B_ONE_CR]);

        if(rate >= speed_graphics) {
                do {
                        m_print(bbstxt[B_ASK_ANSI]);
                        c = yesno_question (DEF_NO|QUESTION);
                        if(c == QUESTION)
                                read_system_file("WHY_ANSI");
                        if (!CARRIER)
                                return (1);
                } while(c != DEF_NO && c != DEF_YES);

                if(c == DEF_YES)
                        tusr.ansi=1;
		else {
                        do {
                                m_print(bbstxt[B_ASK_AVATAR]);
                                c = yesno_question (DEF_NO|QUESTION);
                                if(c == QUESTION)
                                        read_system_file("WHY_AVT");
                                if (!CARRIER)
                                        return (1);
                        } while(c != DEF_NO && c != DEF_YES);

                        if(c == DEF_YES)
                                tusr.avatar=1;
		}

                if (!CARRIER)
                        return (1);

                if(tusr.ansi || tusr.avatar) {
                        do {
                                m_print(bbstxt[B_ASK_COLOR]);
                                c = yesno_question (DEF_YES|QUESTION);
                                if(c == QUESTION)
                                        read_system_file("WHY_COL");
                                if (!CARRIER)
                                        return (1);
                        } while(c != DEF_NO && c != DEF_YES);

                        if(c == DEF_YES)
                                tusr.color=1;
                }

                memcpy((char *)&usr, (char *)&tusr, sizeof(struct _usr));
                usr.name[0] = '\0';

                tusr.use_lore = 1;

                if(tusr.ansi) {
                        do {
                                m_print(bbstxt[B_ASK_EDITOR]);
                                c = yesno_question (DEF_YES|QUESTION);
                                if(c == QUESTION)
                                        read_system_file("WHY_OPED");
                                if (!CARRIER)
                                        return (1);
                        } while(c != DEF_NO && c != DEF_YES);

                        if(c == DEF_YES)
                                tusr.use_lore = 0;
                }

                if(tusr.ansi || tusr.avatar) {
                        do {
                                m_print(bbstxt[B_ASK_FULLREAD]);
                                c = yesno_question (DEF_YES|QUESTION);
                                if(c == QUESTION)
                                        read_system_file("WHY_FULR");
                                if (!CARRIER)
                                        return (1);
                        } while(c != DEF_NO && c != DEF_YES);

                        if(c == DEF_YES)
                                tusr.full_read=1;
                }
        }

        do {
                m_print(bbstxt[B_ASK_IBMSET]);
                c = yesno_question ((usr.ansi || usr.avatar) ? DEF_YES|QUESTION : DEF_NO|QUESTION);
                if(c == QUESTION)
                        read_system_file("WHY_IBM");
                if (!CARRIER)
                        return (1);
        } while(c != DEF_NO && c != DEF_YES);

        if(c == DEF_YES)
                tusr.ibmset=1;

        usr.len = 24;
        screen_change ();
        tusr.len = usr.len;

        do {
                m_print(bbstxt[B_ASK_MORE]);
                c = yesno_question (DEF_YES);
                if (!CARRIER)
                        return (1);
        } while(c != DEF_NO && c != DEF_YES);

        if(c == DEF_YES)
                tusr.more = 1;

        do {
                m_print(bbstxt[B_ASK_CLEAR]);
                c = yesno_question (DEF_YES);
                if (!CARRIER)
                        return (1);
        } while(c != DEF_NO && c != DEF_YES);

        if(c == DEF_YES)
                tusr.formfeed = 1;

        memcpy((char *)&usr, (char *)&tusr, sizeof(struct _usr));
        usr.name[0] = '\0';

        do {
                m_print(bbstxt[B_CITY_STATE]);
                chars_input(stringa,35,INPUT_FIELD);
		if (!CARRIER)
			return (1);
        } while(!strlen(stringa));

        strcpy(tusr.city,fancy_str(stringa));

        if (!noask.birthdate) {
           do {
                   m_print(bbstxt[B_BIRTHDATE]);
                   chars_input(stringa,8,INPUT_FIELD);
                   if (!CARRIER)
                           return (1);
                   sscanf(stringa, "%2d-%2d-%2d", &c, &i, &k);
                   if (c < 1 || c > 31)
                           continue;
                   if (i < 1 || i > 12)
                           continue;
                   if (k < 1 || k > 99)
                           continue;
           } while(strlen(stringa) < 8);

           sprintf(tusr.birthdate, "%2d %3s %2d", c, mtext[i-1], k);
        }

        if (!noask.voicephone) {
                voice_phone_change();
                strcpy(tusr.voicephone, usr.voicephone);
                if (!CARRIER)
                        return (1);
        }

        if (!noask.dataphone) {
                data_phone_change();
                strcpy(tusr.dataphone, usr.dataphone);
                if (!CARRIER)
                        return (1);
        }

        do {
                m_print(bbstxt[B_LOGON_CHECKMAIL]);
                c = yesno_question (DEF_YES);
                if (!CARRIER)
                        return (1);
        } while(c != DEF_NO && c != DEF_YES);

        if(c == DEF_YES)
                tusr.scanmail = 1;

        memcpy((char *)&usr, (char *)&tusr, sizeof(struct _usr));
        usr.name[0] = '\0';

        strcpy (tusr.ldate," 1 Jan 80  00:00:00");
        data (tusr.firstdate);
        tusr.priv = logon_priv;
        tusr.flags = logon_flags;
        tusr.width = 80;
        tusr.tabs = 1;
        tusr.hotkey = 0;
        tusr.ptrquestion = -1L;

        memcpy((char *)&usr, (char *)&tusr, sizeof(struct _usr));
        usr.name[0] = '\0';

        read_system_file("NEWUSER1");
        change_attr(LRED|_BLACK);

	for(;;) {
		do {
                        m_print(bbstxt[B_SELECT_PASSWORD]);
                        chars_input(stringa,15,INPUT_PWD|INPUT_FIELD);
                        if (!CARRIER)
                                return (1);
                } while(strlen(stringa) < 4);

                if (is_trashpwd (stringa)) {
                   status_line ("!Trashcan password `%s'", stringa);
                   read_system_file ("TRASHPWD");
                   continue;
                }

                strcpy(tusr.pwd,stringa);

                do {
                        m_print(bbstxt[B_VERIFY_PASSWORD]);
                        chars_input(stringa,15,INPUT_PWD|INPUT_FIELD);
                        if (!CARRIER)
				return (1);
		} while(!strlen(stringa));

                if(!strcmp(stringa, tusr.pwd))
			break;

                m_print(bbstxt[B_WRONG_PWD1],strupr(tusr.pwd));
                m_print(bbstxt[B_WRONG_PWD2],strupr(stringa));
	}

        strcpy(tusr.pwd,strcode(strupr(stringa),tusr.name));

        usridx.id = crc_name (tusr.name);
        tusr.id = usridx.id;
        memcpy((char *)&usr, (char *)&tusr, sizeof(struct _usr));

        sprintf (filename, "%s.IDX", user_file);
        fd = open (filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
        lseek (fd, 0L, SEEK_END);
        write (fd, (char *)&usridx, sizeof (struct _usridx));
        close (fd);

        return (1);
}

long crc_name (name)
char *name;
{
   int i;
   long crc;

   crc = 0xFFFFFFFFL;
   for (i=0; i < strlen(name); i++)
           crc = Z_32UpdateCRC (((unsigned short) name[i]), crc);

   return (crc);
}

static int is_trashpwd (s)
char *s;
{
   FILE *fp;
   char buffer[40];

   fp = fopen ("PWDTRASH.DAT", "rt");
   if (fp != NULL) {
      while (fgets (buffer, 38, fp) != NULL) {
         buffer[strlen (buffer) - 1] = '\0';
         if (!stricmp (buffer, s))
            return (1);
      }

      fclose (fp);
   }

   return (0);
}

static int is_trashcan (s)
char *s;
{
   FILE *fp;
   char buffer[40];

   fp = fopen ("TRASHCAN.DAT", "rt");
   if (fp != NULL) {
      while (fgets (buffer, 38, fp) != NULL) {
         buffer[strlen(buffer) - 1] = '\0';
         if (!stricmp (buffer, s))
            return (1);
      }

      fclose (fp);
   }

   return (0);
}

