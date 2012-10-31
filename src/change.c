
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
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlstr.h>

#include "sched.h"
#include "lsetup.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

int exist_system_file (char *name);
int daysleft (struct date *d1, struct date *d2);

void user_configuration()
{
        cls();
        del_line();
        m_print(bbstxt[B_CONFIG_HEADER]);

        m_print(bbstxt[B_CONFIG_NAME],usr.name);

        m_print(bbstxt[B_CONFIG_ALIAS], usr.handle, usr.voicephone);

        m_print(bbstxt[B_CONFIG_CITY], usr.city, usr.dataphone);

        m_print(bbstxt[B_CONFIG_LENGTH],
                usr.len,
                usr.formfeed ? bbstxt[B_YES] : bbstxt[B_NO],
                usr.nulls);

        m_print(bbstxt[B_CONFIG_MORE],
                usr.more ? bbstxt[B_YES] : bbstxt[B_NO],
                usr.scanmail ? bbstxt[B_YES] : bbstxt[B_NO],
                usr.tabs ? bbstxt[B_YES] : bbstxt[B_NO]);

        m_print(bbstxt[B_CONFIG_ANSI],
                usr.ansi ? bbstxt[B_YES] : bbstxt[B_NO],
                usr.use_lore ? bbstxt[B_NO] : bbstxt[B_YES],
                usr.color ? bbstxt[B_YES] : bbstxt[B_NO]);

        m_print(bbstxt[B_CONFIG_AVATAR],
                usr.avatar ? bbstxt[B_YES] : bbstxt[B_NO],
                usr.full_read ? bbstxt[B_YES] : bbstxt[B_NO],
                usr.hotkey ? bbstxt[B_YES] : bbstxt[B_NO]);

        m_print(bbstxt[B_CONFIG_IBMSET],
                usr.ibmset ? bbstxt[B_YES] : bbstxt[B_NO]);

        m_print(bbstxt[B_CONFIG_LANGUAGE], &config->language[usr.language].descr[1]);
        m_print(bbstxt[B_CONFIG_SIGN],usr.signature);
}

void more_change()
{
   usr.more ^= 1;
}

void hotkey_change()
{
   usr.hotkey ^= 1;
}

void ibmset_change()
{
   usr.ibmset ^= 1;
}

void color_change()
{
        if(rate < speed_graphics || (!usr.ansi && !usr.avatar))
        return;
        if(usr.color)
                change_attr(LGREY|_BLACK);
        usr.color^=1;
        if (function_active == 1)
                f1_status();

        if (usr.color)
                m_print(bbstxt[B_COLOR_USED]);
        else
                m_print(bbstxt[B_COLOR_NOT_USED]);
        press_enter ();
}

void ansi_change()
{
        if(rate < speed_graphics)
		return;
        if(usr.ansi)
                change_attr(LGREY|_BLACK);

        usr.ansi^=1;

        if(!usr.ansi) {
                usr.use_lore = 1;
                usr.color = 0;
	}
        else {
                usr.avatar = 0;
                usr.color = 1;
        }

        if (function_active == 1)
                f1_status();

        if (usr.ansi)
                m_print(bbstxt[B_ANSI_USED]);
        else {
                m_print(bbstxt[B_ANSI_NOT_USED]);
                if (usr.use_lore)
                        m_print(bbstxt[B_FULL_NOT_USED2]);
        }
        m_print(bbstxt[B_TWO_CR]);
        press_enter ();
}

void avatar_change()
{
        if(usr.avatar)                  
                change_attr(LGREY|_BLACK);
        usr.avatar^=1;

        if(!usr.avatar) {
                usr.use_lore = 1;
                usr.color = 0;
	}                           
        else {
                usr.ansi = 0;
                usr.color = 1;
        }

        if (function_active == 1)
                f1_status();

        if (usr.avatar)
                m_print(bbstxt[B_AVATAR_USED]);
        else {
                m_print(bbstxt[B_AVATAR_NOT_USED]);
                if (usr.use_lore)
                        m_print(bbstxt[B_FULL_NOT_USED2]);
        }
        m_print(bbstxt[B_TWO_CR]);
        press_enter ();
}

void formfeed_change()
{
   usr.formfeed ^= 1;
}

void kludge_change (void)
{
   usr.kludge ^= 1;
}

void scanmail_change (void)
{
   usr.scanmail ^= 1;
}

void fullscreen_change (void)
{
   if (!usr.ansi && !usr.avatar)
      return;

   usr.use_lore ^= 1;

   if (usr.use_lore)
      m_print (bbstxt[B_FULL_NOT_USED]);
   else
      m_print (bbstxt[B_FULL_USED]);

   press_enter ();
}

void tabs_change()
{
   usr.tabs^=1;
}

void fullread_change()
{
        usr.full_read^=1;

        if (usr.full_read)
                m_print(bbstxt[B_FULLREAD_USED]);
        else
                m_print(bbstxt[B_FULLREAD_NOT_USED]);
        press_enter ();
}

void screen_change()
{
   int col;
   char stringa[6];
                  
   do {
      m_print(bbstxt[B_LINE_CHANGE]);
      sprintf (stringa, "%d", usr.len);
      chars_input (stringa, 3, INPUT_UPDATE|INPUT_FIELD);
      if (!CARRIER)
         col = usr.len;
      else
         col = atoi(stringa);
   } while ((col < 10 || col > 66) && CARRIER);

   usr.len = col;
}

void nulls_change()
{
   int col;
   char stringa[6];

   m_print (bbstxt[B_NULLS_CHANGE]);
   sprintf (stringa, "%d", usr.nulls);
   chars_input (stringa, 3, INPUT_UPDATE|INPUT_FIELD);

   col = atoi(stringa);

   if (col < 0 || col > 255)
      return;

   usr.nulls = col;
}

void handle_change()
{
   int fd=0;
   char stringa[40], filename[80];
   long crc, ici;
   struct _usridx usridx;

   read_system_file ("ALIASASK");
   

   strcpy (stringa, usr.handle);
   m_print(bbstxt[B_ALIAS_CHANGE]);
   chars_input (stringa, 35, INPUT_FANCY|INPUT_UPDATE|INPUT_FIELD);

   if (stringa[0] && stricmp (stringa, usr.name)) {
      crc = crc_name (stringa);
      
      sprintf (filename, "%s.IDX", config->user_file);
      fd = open (filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
      while (read (fd, &usridx, sizeof (struct _usridx)) == sizeof (struct _usridx)) {
         if (usridx.id == crc || usridx.alias_id == crc) {
            status_line (":Invalid alias '%s'", stringa);
            stringa[0] = '\0';
            break;
         }
      }
      close (fd);

   }

   if (stringa[0]){
      strcpy (usr.handle, fancy_str (stringa));
      usr.alias_id = crc;
      sprintf (filename, "%s.IDX", config->user_file);
      fd = open (filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
      while (read (fd, &usridx, sizeof (struct _usridx)) == sizeof (struct _usridx)) {
         ici=tell(fd);
         if (usridx.id == usr.id) {
	    lseek(fd,(ici-(sizeof(struct _usridx))),SEEK_SET);
            usridx.alias_id = crc;
            write (fd, (char *)&usridx, sizeof (struct _usridx));                     
            break;
         }        
      }
      close (fd);
   }      
}

void voice_phone_change()
{
   char stringa[20], i;

   for (;;) {
      m_print(bbstxt[B_VOICE_PHONE]);

      m_print(bbstxt[B_ASK_NUMBER]);
      chars_input(stringa,19,INPUT_FIELD);

      if (!CARRIER)
         return;

      for (i = 0; i < strlen (stringa); i++) {
         if ( (stringa[i] >= '0' && stringa[i] <= '9') ||
              stringa[i] == '+' || stringa[i] == '-' || stringa[i] == ' ' ||
              stringa[i] == '(' || stringa[i] == ')' || stringa[i] == '/'
            )
            continue;
         else {
            m_print (bbstxt[B_INVALID_CHAR], stringa[i], stringa[i]);
            break;
         }
      }
      if (i < strlen (stringa) || strlen (stringa) < 4)
         continue;

      m_print(bbstxt[B_PHONE_IS], stringa);
      m_print(bbstxt[B_PHONE_OK]);
      if (yesno_question (DEF_YES) == DEF_YES)
         break;
   }

   strcpy(usr.voicephone,stringa);
}

void data_phone_change()
{
   char stringa[20], i;

   for (;;) {
      m_print(bbstxt[B_DATA_PHONE]);

      m_print(bbstxt[B_ASK_NUMBER]);
      chars_input(stringa,19,INPUT_FIELD);

      if (!CARRIER)
         return;

      for (i = 0; i < strlen (stringa); i++) {
         if ( (stringa[i] >= '0' && stringa[i] <= '9') ||
              stringa[i] == '+' || stringa[i] == '-' || stringa[i] == ' ' ||
              stringa[i] == '(' || stringa[i] == ')' || stringa[i] == '/'
            )
            continue;
         else {
            m_print (bbstxt[B_INVALID_CHAR], stringa[i], stringa[i]);
            break;
         }
      }
      if (i < strlen (stringa) || strlen (stringa) < 4)
         continue;

      m_print(bbstxt[B_PHONE_IS], stringa);
      m_print(bbstxt[B_PHONE_OK]);
      if (yesno_question (DEF_YES) == DEF_YES)
         break;
   }

   strcpy(usr.dataphone,stringa);
}

void city_change()
{
   char stringa[40];

   m_print(bbstxt[B_CITY_STATE]);
   strcpy (stringa, usr.city);
   chars_input(stringa,35,INPUT_FIELD|INPUT_UPDATE);

   if (!strlen (stringa))
      return;

    strcpy(usr.city,stringa);

    if (function_active == 1)
       f1_status();
}

void password_change()
{
   char stringa[20], parola[20];

   read_system_file("CHGPWD");

   m_print(bbstxt[B_ONE_CR]);

   for (;;) {
      m_print(bbstxt[B_SELECT_PASSWORD]);
      chars_input(stringa,15,INPUT_PWD|INPUT_FIELD);

      if (!strlen(stringa))
         return;

      strcpy(parola,stringa);

      m_print(bbstxt[B_VERIFY_PASSWORD]);

      m_print(bbstxt[B_PASSWORD]);
      chars_input(stringa,15,INPUT_PWD|INPUT_FIELD);

      if (!strlen(stringa) || !CARRIER)
         return;

      if(strcmp(stringa,parola) == 0)
         break;

      m_print(bbstxt[B_WRONG_PWD1],strupr(parola));
      m_print(bbstxt[B_WRONG_PWD2],strupr(stringa));
   }

   strcpy(usr.pwd,strcode(strupr(stringa),usr.name));
   data (stringa);
   stringa[9] = '\0';
   strcpy (usr.lastpwdchange, stringa);
}

void signature_change()
{
   char stringa[60];

   m_print(bbstxt[B_SET_SIGN]);
   m_print(bbstxt[B_ASK_SIGN]);
   chars_input(stringa,58,INPUT_FIELD);

   strcpy(usr.signature,stringa);
}

int select_language ()
{
   int i, r;
   char c, stringa[4], strcom[25];

   if (!config->language[1].descr[0])
         return (0);
         
   i = 0;
   strcom[0] = '\0';

   m_print(bbstxt[B_LANGUAGE_AVAIL]);
   
   for (i = 0; i < MAX_LANG; i++) {
      if (!config->language[i].descr[0])
         continue;
      m_print(bbstxt[B_PROTOCOL_FORMAT], config->language[i].descr[0], &config->language[i].descr[1]);
      strcom[i] = toupper(config->language[i].descr[0]);
   }

   strcom[i] = '\0';

   m_print (bbstxt[B_SELECT_LANGUAGE]);
   input (stringa,1);
   c = toupper (stringa[0]);
   if (c == '\0' || c == '\r' || !CARRIER || !strchr (strcom, c))
      return (-1);

   r = -1;
   for (i = 0; i < MAX_LANG; i++) {
      if (!config->language[i].descr[0])
         continue;
      if (config->language[i].descr[0] == c) {
         r = i;
         break;
      }
   }

   return (r);
}

void select_group (type, tosig)
int type, tosig;
{
   int sig;
   char stringa[10];

   if (tosig == -1) {
      if (type == 1)
         read_system_file ("MGROUP");
      else if (type == 2)
         read_system_file ("FGROUP");

      do {
         m_print (bbstxt[B_GROUP_LIST]);
         chars_input (stringa, 4, INPUT_FIELD);
         if (!stringa[0])
            return;
         sig = atoi (stringa);
      } while (sig < 0 || sig > 255);
   }
   else
      sig = tosig;

   if (type == 1)
      usr.msg_sig = sig;
   else if (type == 2)
      usr.file_sig = sig;
}

int ask_random_birth ()
{
   int i, c, k, curr, m;
   char stringa[30], *p;
   struct time dt;

   data (stringa);
   curr = atoi (&stringa[7]);

   gettime (&dt);
   srand (dt.ti_sec * 100 + dt.ti_hund);

   if (!usr.security && random (10000) < 6000)
      return (1);

   if (config->birthdate) {
      read_system_file ("BVERIFY");

      do {
         m_print (bbstxt[B_BIRTHDATE], bbstxt[B_DATEFORMAT + config->inp_dateformat]);
         chars_input(stringa,8,INPUT_FIELD);
         if (!CARRIER)
            return (1);
         c = i = k = 0;
         p = strtok (stringa, "-./ ");
         if (p != NULL)
            c = atoi (p);
         p = strtok (NULL, "-./ ");
         if (p != NULL)
            i = atoi (p);
         p = strtok (NULL, "-./ ");
         if (p != NULL)
            k = atoi (p);

         if (config->inp_dateformat == 1) {
            m = c;
            c = i;
            i = m;
         }
         else if (config->inp_dateformat == 2) {
            m = c;
            c = k;
            k = m;
         }
      } while ((c < 1 || c > 31) || (i < 1 || i > 12) || (k < 1 || k > curr));

      sprintf (stringa, "%2d %3s %2d", c, mtext[i-1], k);
      if (stricmp (stringa, usr.birthdate)) {
         status_line ("!Birthdate check failed");
         read_system_file ("BFAILED");
         return (0);
      }
   }

   return (1);
}

void ask_birthdate ()
{
   int i, c, k, curr, dd1, mm1, yy1, m;
   char stringa[30], *p;

   data (stringa);
   curr = atoi (&stringa[7]);

   sscanf (usr.birthdate, "%2d %3s %2d", &dd1, stringa, &yy1);
   stringa[3] = '\0';
   for (i = 0; i < 12; i++)
      if (!stricmp (stringa, mtext[i]))
         break;
   mm1 = i + 1;

   if ((dd1 < 1 || dd1 > 31) || (mm1 < 1 || mm1 > 12) || (yy1 < 1 || yy1 > 99))
      stringa[0] = '\0';
   else {
      if (config->inp_dateformat == 0)
         sprintf (stringa, "%02d-%02d-%02d", dd1, mm1, yy1);
      else if (config->inp_dateformat == 1)
         sprintf (stringa, "%02d-%02d-%02d", mm1, dd1, yy1);
      else if (config->inp_dateformat == 2)
         sprintf (stringa, "%02d-%02d-%02d", yy1, mm1, dd1);
   }

   do {
      m_print (bbstxt[B_BIRTHDATE], bbstxt[B_DATEFORMAT + config->inp_dateformat]);
      chars_input (stringa, 8, INPUT_FIELD|INPUT_UPDATE);
      if (!CARRIER)
      return;
      c = i = k = 0;
      p = strtok (stringa, "-./ ");
      if (p != NULL)
         c = atoi (p);
      p = strtok (NULL, "-./ ");
      if (p != NULL)
         i = atoi (p);
      p = strtok (NULL, "-./ ");
      if (p != NULL)
         k = atoi (p);

      if (config->inp_dateformat == 1) {
         m = c;
         c = i;
         i = m;
      }
      else if (config->inp_dateformat == 2) {
         m = c;
         c = k;
         k = m;
      }

      if ((dd1 < 1 || dd1 > 31) || (mm1 < 1 || mm1 > 12) || (yy1 < 1 || yy1 > 99))
         stringa[0] = '\0';
      else
         sprintf (stringa, "%02d-%02d-%02d", dd1, mm1, yy1);
   } while ((c < 1 || c > 31) || (i < 1 || i > 12) || (k < 1 || k > curr));

   sprintf (usr.birthdate, "%2d %3s %2d", c, mtext[i-1], k);
}

int get_user_age ()
{
   int yy1, yy2, dd1, dd2, mm1, mm2, age, i;
   char stringa[30], buff[10];

   sscanf (usr.birthdate, "%2d %3s %2d", &dd1, buff, &yy1);
   buff[3] = '\0';
   for (i = 0; i < 12; i++)
      if (!stricmp (buff, mtext[i]))
         break;
   mm1 = i + 1;

   if ((dd1 < 1 || dd1 > 31) || (mm1 < 1 || mm1 > 12) || (yy1 < 1 || yy1 > 99))
      return (0);

   data (stringa);
   sscanf (stringa, "%2d %3s %2d", &dd2, buff, &yy2);
   buff[3] = '\0';
   for (i = 0; i < 12; i++)
      if (!stricmp (buff, mtext[i]))
         break;
   mm2 = i + 1;

   age = yy2 - yy1;
   if (mm2 < mm1)
      age--;
   else if (mm2 == mm1 && dd2 < dd1)
      age--;

   return (age);
}

void ask_default_protocol ()
{
   int i, fd, cmdpos;
   char c, stringa[4], extcmd[50], filename[80];
   PROTOCOL prot;

   cmdpos = 0;
   sprintf (filename, "%sPROTOCOL.DAT", config->sys_path);
   fd = sh_open (filename, O_RDONLY|O_BINARY, SH_DENYNONE, S_IREAD|S_IWRITE);
   if (fd != -1) {
      while (read (fd, &prot, sizeof (PROTOCOL)) == sizeof (PROTOCOL)) {
         if (prot.active)
            extcmd[cmdpos++] = toupper (prot.hotkey);
      }
      close (fd);
   }
   extcmd[cmdpos] = '\0';

reread:
   if ((c=get_command_letter ()) == '\0') {
      if (!read_system_file ("DEFPROT")) {
         m_print (bbstxt[B_PROTOCOLS]);
         if (config->prot_xmodem)
            m_print (bbstxt[B_PROTOCOL_FORMAT], protocols[0][0], &protocols[0][1]);
         if (config->prot_1kxmodem)
            m_print (bbstxt[B_PROTOCOL_FORMAT], protocols[1][0], &protocols[1][1]);
         if (config->prot_zmodem)
            m_print (bbstxt[B_PROTOCOL_FORMAT], protocols[2][0], &protocols[2][1]);
         if (config->prot_sealink)
            m_print (bbstxt[B_PROTOCOL_FORMAT], protocols[5][0], &protocols[5][1]);
         if (config->hslink)
            m_print (bbstxt[B_PROTOCOL_FORMAT], protocols[3][0], &protocols[3][1]);
         if (config->puma)
            m_print (bbstxt[B_PROTOCOL_FORMAT], protocols[4][0], &protocols[4][1]);

         sprintf (filename, "%sPROTOCOL.DAT", config->sys_path);
         fd = sh_open (filename, O_RDONLY|O_BINARY, SH_DENYNONE, S_IREAD|S_IWRITE);
         if (fd != -1) {
            while (read (fd, &prot, sizeof (PROTOCOL)) == sizeof (PROTOCOL)) {
               if (prot.active)
                  m_print (bbstxt[B_PROTOCOL_FORMAT], prot.hotkey, prot.name);
            }
            close (fd);
         }

         m_print (bbstxt[B_PROTOCOL_FORMAT], bbstxt[B_NONE][0], &bbstxt[B_NONE][1]);

         if (exist_system_file ("XFERHELP"))
            m_print (bbstxt[B_PROTOCOL_FORMAT], bbstxt[B_HELP][0], &bbstxt[B_HELP][1]);

         m_print (bbstxt[B_SELECT_PROT]);
      }

      if (usr.hotkey) {
         cmd_input (stringa, 1);
         m_print (bbstxt[B_ONE_CR]);
      }
      else
         input (stringa, 1);
      c = stringa[0];
      if (c == '\0' || c == '\r' || !CARRIER)
         return;
   }

   for (i = 0; i < cmdpos; i++)
      if (toupper (c) == toupper (extcmd[i]))
         break;
   if (i < cmdpos) {
      usr.protocol = toupper (c);
      return;
   }

   switch (toupper (c)) {
      case 'X':
         if (config->prot_xmodem)
            usr.protocol = toupper (c);
         break;
      case '1':
         if (config->prot_1kxmodem)
            usr.protocol = toupper (c);
         break;
      case 'Z':
         if (config->prot_zmodem)
            usr.protocol = toupper (c);
         break;
      case 'H':
         if (config->hslink)
            usr.protocol = toupper (c);
         break;
      case 'P':
         if (config->puma)
            usr.protocol = toupper (c);
         break;
      case 'S':
         if (config->prot_sealink)
           usr.protocol = toupper (c);
         break;
      case '?':
         read_system_file ("XFERHELP");
         goto reread;
      case 'N':
         usr.protocol = '\0';
         break;
      default:
         return;
   }
}

void ask_default_archiver ()
{
   int i, m;
   char c, stringa[4], cmd[20];

reread:
   if ((c=get_command_letter ()) == '\0') {
      if (!read_system_file ("DEFCOMP")) {
         m_print (bbstxt[B_AVAIL_COMPR]);

         m = 0;
         for (i = 0; i < 10; i++) {
            if (config->packid[i].display[0]) {
               m_print (bbstxt[B_PROTOCOL_FORMAT], config->packid[i].display[0], &config->packid[i].display[1]);
               cmd[m++] = config->packid[i].display[0];
            }
         }
         m_print (bbstxt[B_PROTOCOL_FORMAT], bbstxt[B_NONE][0], &bbstxt[B_NONE][1]);
         cmd[m++] = bbstxt[B_NONE][0];
         if (exist_system_file ("COMPHELP")) {
            m_print (bbstxt[B_PROTOCOL_FORMAT], bbstxt[B_HELP][0], &bbstxt[B_HELP][1]);
            cmd[m++] = bbstxt[B_HELP][0];
         }
         cmd[m] = '\0';

         m_print (bbstxt[B_ASK_COMPRESSOR]);
      }
      else {
         m = 0;
         for (i = 0; i < 10; i++) {
            if (config->packid[i].display[0])
               cmd[m++] = config->packid[i].display[0];
         }
         cmd[m] = '\0';
      }

      if (usr.hotkey) {
         cmd_input (stringa, 1);
         m_print (bbstxt[B_ONE_CR]);
      }
      else
         input (stringa, 1);
      c = stringa[0];
      if (c == '\0' || c == '\r' || !CARRIER)
         return;
   }
   else {
      m = 0;
      for (i = 0; i < 10; i++) {
         if (config->packid[i].display[0])
            cmd[m++] = config->packid[i].display[0];
      }
   }

   c = toupper(c);
   strupr (cmd);

   if (strchr (cmd, c) == NULL && usr.archiver != '\0')
      return;
   else if (strchr (cmd, c) == NULL && usr.archiver == '\0')
      goto reread;

   if (c == bbstxt[B_HELP][0]) {
      read_system_file ("COMPHELP");
      goto reread;
   }
   else if (c == bbstxt[B_NONE][0])
      usr.archiver = '\0';
   else
      usr.archiver = c;
}

int check_subscription (void)
{
   int day, year, mont;
   char parola[10];
   struct date now, scad;

   if (!usr.subscrdate[0])
      return (-1);

   sscanf (usr.subscrdate, "%2d %3s %2d", &day, parola, &year);
   parola[3] = '\0';
   for (mont = 0; mont < 12; mont++) {
      if ((!stricmp (mtext[mont], parola)) || (!stricmp (mesi[mont], parola)))
         break;
   }
   if (mont >= 12)
      return (-1);

   scad.da_year = year;
   scad.da_mon = mont + 1;
   scad.da_day = day;

   getdate (&now);
   now.da_year %= 100;

   return (daysleft (&now, &scad));
}

