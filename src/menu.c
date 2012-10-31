
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
#include <ctype.h>
#include <alloc.h>
#include <string.h>
#include <io.h>
#include <time.h>
#include <fcntl.h>
#include <conio.h>
#include <stdlib.h>
#include <dir.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlstr.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

int process_menu_option(int, char *);
int get_user_age (void);
void ask_default_protocol (void);
void ask_default_archiver (void);
void tag_files (int);
void ask_birthdate (void);
void list_tagged_files (int remove);
int naplps_read_file (char *name);
void kludge_change (void);
int bluewave_pack_tagged_areas (void);
void offline_pack_tagged_areas (int bw);
void forward_message_area (int sig, int flag);

#define MENU_LENGTH  14
#define MENU_STACK   10
#define MAX_ITEMS   100

extern int msg_parent, msg_child;
extern char stop_hotkey, num_hotkey, no_precheck;

static int i;
static char mnu_name[MENU_LENGTH], *st, max_menu, active, old_activ;
static char menu_stack[MENU_STACK][MENU_LENGTH], filename[50], first_time;
static char menu_sp, readed, file_stack[MENU_STACK][MENU_LENGTH], *mnu_file;
static struct _cmd *cmd = NULL;

static int get_menu_password (char *);
static void return_menu(void);
static void gosub_menu(char *);
static void process_user_display (char *, int);
static void get_buffer_keystrokes (char *);
static int ok_counter (char *);
static int get_sig (char *);
static int user_age_ok (char *arg);

struct _menu_header {
   char name[14];
   short  n_elem;
};

int check_hotkey (char c)
{
   int m = 0, i;

   for (i = 0; i < max_menu; i++) {
      if (usr.priv < cmd[i].priv || cmd[i].priv == HIDDEN)
         continue;
      if ((usr.flags & cmd[i].flags) != cmd[i].flags)
         continue;
      if (!cmd[i].flag_type || cmd[i].automatic)
         continue;
      if (cmd[i].first_time && !first_time)
         continue;
      if (cmd[i].hotkey == toupper (c) || (cmd[i].hotkey == '|' && !c)) {
         m = 1;
         break;
      }
   }

   return (m);
}

void menu_dispatcher(start)
char *start;
{
   FILE *fd;
   int m, processed;
   char last_command, err;
   struct _menu_header m_header;

   st = start;
   strcpy(mnu_name, (start == NULL) ? "MAIN" : start);
   mnu_file = NULL;

   max_menu = 0;
   err = 0;
   readed = 0;
   menu_sp = 0;
   old_activ = 0;
   active = (start == NULL) ? 0 : 4;
   allow_reply = (active == 4) ? 1 : 0;
   cmd = NULL;

   if (start == NULL)
      cls ();

   i = 0;
//   cmd[i].nocls = 0;
   cmd_string[0] = '\0';

   i = (start == NULL) ? 0 : -1;

   for (;;) {
      XON_ENABLE();
      _BRK_ENABLE();

      if (!readed) {
         num_hotkey = 0;

         if (mnu_file == NULL)
            sprintf (filename, "%s%s.MNU", config->menu_path, config->language[usr.language].basename);
         else
            sprintf (filename, "%s%s.MNU", config->menu_path, mnu_file);
         fd = fopen (filename, "rb");
         if (fd == NULL || filelength (fileno(fd)) < sizeof (struct _menu_header)) {
            if (mnu_file != NULL) {
               mnu_file = NULL;
               sprintf (filename, "%s%s.MNU", config->menu_path, config->language[usr.language].basename);
               if (fd != NULL)
                  fclose (fd);
               fd = fopen (filename, "rb");
            }
            if (fd == NULL || filelength (fileno(fd)) < sizeof (struct _menu_header)) {
               if (fd != NULL)
                  fclose (fd);
               sprintf (filename,"%s%s.MNU", config->menu_path, config->language[0].basename);
               fd = fopen (filename, "rb");
               if (fd == NULL) {
                  status_line ("!Can't open `%s'", filename);
                  return;
               }
            }
         }

         for (;;) {
            if (fread((char *)&m_header, 1, sizeof(struct _menu_header),fd) != sizeof(struct _menu_header)) {
               status_line("!Menu `%s' not found", mnu_name);
               if (err == 1)
                  return;
               strcpy (mnu_name, "MAIN");
               fseek (fd, 0, SEEK_SET);
               err = 1;
               menu_sp = 0;
               mnu_file = NULL;
            }
            else {
               if (!stricmp(m_header.name, mnu_name))
                  break;

               fseek (fd, (long)sizeof(struct _cmd) * m_header.n_elem, SEEK_CUR);
            }
         }

         err = 0;

         if (m_header.n_elem > MAX_ITEMS)
            m_header.n_elem = MAX_ITEMS;
         if (cmd != NULL)
            free (cmd);
         if (sq_ptr != NULL) {
            MsgCloseArea (sq_ptr);
            if (sys.squish) {
               sq_ptr = MsgOpenArea (sys.msg_path, MSGAREA_CRIFNEC, MSGTYPE_SQUISH);
               MsgUnlock (sq_ptr);
            }
            else
               sq_ptr = NULL;
         }
         cmd = (struct _cmd *)malloc (m_header.n_elem * sizeof (struct _cmd));
         if (cmd == NULL) {
            status_line(msgtxt[M_NODELIST_MEM]);
            return;
         }

         fread((char *)cmd,sizeof(struct _cmd) * m_header.n_elem,1,fd);
         fclose (fd);

         readed = 1;
         first_time = 1;
         max_menu = m_header.n_elem;
      }

      i = 0;

      for (m=0; m < max_menu; m++) {
         if(usr.priv < cmd[m].priv || cmd[m].priv == HIDDEN)
            continue;
         if((usr.flags & cmd[m].flags) != cmd[m].flags)
            continue;

         if (cmd[m].first_time && !first_time)
            continue;

         if (!user_age_ok (cmd[m].argument))
            continue;

         if (cmd[m].flag_type == _MSG_INDIVIDUAL || cmd[m].flag_type == _MAIL_INDIVIDUAL)
            num_hotkey = 1;

         if ((cmd[m].flag_type == _F_DNLD || cmd[m].flag_type == _F_GLOBAL_DNLD) && stristr (cmd[m].argument, "/F=") == NULL) {
            if (usr.priv < sys.download_priv || sys.download_priv == HIDDEN)
               continue;
            if((usr.flags & sys.download_flags) != sys.download_flags)
               continue;
         }

         if(cmd[m].flag_type == _F_TAG) {
            if (usr.priv < sys.download_priv || sys.download_priv == HIDDEN)
               continue;
            if((usr.flags & sys.download_flags) != sys.download_flags)
               continue;
         }

         if(cmd[m].flag_type == _F_UPLD) {
            if (usr.priv < sys.upload_priv || sys.upload_priv == HIDDEN)
               continue;
            if((usr.flags & sys.upload_flags) != sys.upload_flags)
               continue;
         }

/*
         if(cmd[m].flag_type == _F_TITLES) {
            if (usr.priv < sys.list_priv || sys.list_priv == HIDDEN)
               continue;
            if((usr.flags & sys.list_flags) != sys.list_flags)
               continue;
         }
*/

         if((cmd[m].flag_type == _MSG_EDIT_NEW) || (cmd[m].flag_type == _MSG_EDIT_REPLY))
            if (strstr (cmd[m].argument, "/A=") == NULL) {
               if (usr.priv < sys.write_priv || sys.write_priv == HIDDEN)
                  continue;
               if((usr.flags & sys.write_flags) != sys.write_flags)
                  continue;
            }

         if ((cmd[m].flag_type == _MSG_CHILD && !msg_child) || (cmd[m].flag_type == _MSG_PARENT && !msg_parent))
            continue;

         if (cmd[m].automatic && cmd[m].flag_type) {
            if (cmd_string[0] && (cmd[m].flag_type == _SHOW ||
                                  cmd[m].flag_type == _SHOW_TEXT ||
                                  cmd[m].flag_type == _CONFIG))
               continue;

            if (!ok_counter (cmd[m].argument))
               continue;

            get_buffer_keystrokes (cmd[m].argument);
            i = m;
            stop_hotkey = 1;
            if (process_menu_option(cmd[m].flag_type, cmd[m].argument)) {
               stop_hotkey = 0;
               if (cmd != NULL)
                  free (cmd);
               return;
            }
            stop_hotkey = 0;
            if (!readed)
               break;
            continue;
         }

         if (!cmd_string[0] && !cmd[m].no_display && cmd[m].name[0])
            process_user_display (cmd[m].name, m);
      }

      if (!readed)
         continue;

      if (user_status != BROWSING)
         set_useron_record(BROWSING, 0, 0);
      first_time = 0;

      if (!cmd_string[0]) {
         if (local_mode) {
            while (kbhit())
               m = getch ();
            local_kbd = -1;
         }

         if (usr.hotkey) {
            do {
               cmd_input (cmd_string, MAX_CMDLEN - 1);
               if (!cmd_string[0] && cmd_string[1] == 5)
                  break;
               m = 0;
               for (i = 0; i < max_menu; i++) {
                  if (usr.priv < cmd[i].priv || cmd[i].priv == HIDDEN)
                     continue;
                  if ((usr.flags & cmd[i].flags) != cmd[i].flags)
                     continue;
                  if (!cmd[i].flag_type || cmd[i].automatic)
                     continue;
                  if (cmd[i].first_time && !first_time)
                     continue;
                  if (cmd[i].hotkey == toupper (cmd_string[0]) || (cmd[i].hotkey == '|' && !cmd_string[0])) {
                     m = 1;
                     break;
                  }
               }
               if (!cmd_string[0])
                  m_print (" ");

               if (m == 0)
                  m_print ("\b \b");
            } while (m == 0 && CARRIER && time_remain() > 0);

            m_print (bbstxt[B_ONE_CR]);
         }
         else
            input (cmd_string, MAX_CMDLEN - 1);
      }
      else
         CLEAR_OUTBOUND();

      if (!CARRIER || time_remain() <= 0)
         return;

      last_command = '\0';
      processed = 0;

      for (i = 0; i < max_menu && i >= 0; i++) {
         if(usr.priv < cmd[i].priv || cmd[i].priv == HIDDEN)
            continue;
         if((usr.flags & cmd[i].flags) != cmd[i].flags)
            continue;
         if (!cmd[i].flag_type || cmd[i].automatic)
            continue;
         if (cmd[i].first_time && !first_time)
            continue;

         if (!user_age_ok (cmd[i].argument))
            continue;

         if ((cmd[i].flag_type == _F_DNLD || cmd[m].flag_type == _F_GLOBAL_DNLD) && stristr (cmd[i].argument, "/F=") == NULL) {
            if (usr.priv < sys.download_priv || sys.download_priv == HIDDEN)
               continue;
            if((usr.flags & sys.download_flags) != sys.download_flags)
               continue;
         }
         if(cmd[i].flag_type == _F_TAG) {
            if (usr.priv < sys.download_priv || sys.download_priv == HIDDEN)
               continue;
            if((usr.flags & sys.download_flags) != sys.download_flags)
               continue;
         }
         if(cmd[i].flag_type == _F_UPLD) {
            if (usr.priv < sys.upload_priv || sys.upload_priv == HIDDEN)
               continue;
            if((usr.flags & sys.upload_flags) != sys.upload_flags)
               continue;
         }
/*
         if(cmd[i].flag_type == _F_TITLES) {
            if (usr.priv < sys.list_priv || sys.list_priv == HIDDEN)
               continue;
            if((usr.flags & sys.list_flags) != sys.list_flags)
               continue;
         }
*/
         if((cmd[i].flag_type == _MSG_EDIT_NEW) || (cmd[i].flag_type == _MSG_EDIT_REPLY))
            if (strstr (cmd[i].argument, "/A=") == NULL) {
               if (usr.priv < sys.write_priv || sys.write_priv == HIDDEN)
                  continue;
               if((usr.flags & sys.write_flags) != sys.write_flags)
                  continue;
            }

         if ((cmd[i].flag_type == _MSG_CHILD && !msg_child) || (cmd[i].flag_type == _MSG_PARENT && !msg_parent))
            continue;

         if ((cmd[i].hotkey == toupper(cmd_string[0]) && !processed) ||
             (cmd[i].hotkey == '|' && !cmd_string[0] && !processed) ||
             (cmd[i].hotkey == toupper(last_command) && processed))
         {
            last_command = cmd[i].hotkey;
            if (!ok_counter (cmd[i].argument))
               break;
            get_buffer_keystrokes (cmd[i].argument);
            if (cmd_string[0] && cmd[i].flag_type != _MSG_INDIVIDUAL && cmd[i].flag_type != _MAIL_INDIVIDUAL) {
               strcpy (&cmd_string[0], &cmd_string[1]);
               strtrim (cmd_string);
            }
            if (process_menu_option(cmd[i].flag_type, cmd[i].argument)) {
               if (cmd != NULL)
                  free (cmd);
               return;
            }
            processed = 1;
            if (i == -1)
               break;
         }
      }

      if (!processed && cmd_string[0]) {
         m_print (bbstxt[B_UNKNOW_CMD], toupper (cmd_string[0]));
         cmd_string[0] = '\0';
      }

      if (!CARRIER || time_remain() <= 0)
         break;
   }
}


int process_menu_option (int flag_type, char *argument)
{
   static int s, v, gg, lo;
   int xp;
   char *p = NULL, buf[128];

   switch (flag_type) {
      case _MESSAGE:
         if (!get_menu_password (argument))
            break;
         active = 1;
         xp = get_sig (argument);
         if ((p=strstr(argument, "/A=")) != NULL)
            usr.msg = atoi(p + 3);
         if (!read_system(usr.msg, active)) {
            if (strstr (argument, "/2") != NULL)
               display_area_list(active, 1, xp);
            else if (strstr (argument, "/3") != NULL)
               display_area_list(active, 3, xp);
            else if (strstr (argument, "/4") != NULL)
               display_area_list(active, 4, xp);
            else
               display_area_list(active, 2, xp);
         }
         else
            status_line(msgtxt[M_BBS_EXIT], usr.msg, sys.msg_name);
         if (sys.quick_board || sys.gold_board)
            quick_scan_message_base (sys.quick_board, sys.gold_board, usr.msg, 1);
         else if (sys.pip_board)
            pip_scan_message_base (usr.msg, 1);
         else if (sys.squish)
            squish_scan_message_base (usr.msg, sys.msg_path, 1);
         else
            scan_message_base(usr.msg, 1);
         allow_reply = 0;
         if (strstr (argument, "/NS") == NULL)
            cls ();
         gosub_menu (argument);
         break;
      case _FILE:
         if (!get_menu_password (cmd[i].argument))
            break;
         active = 2;
         xp = get_sig (argument);
         if ((p=strstr(argument, "/A=")) != NULL)
            usr.files = atoi(p + 3);
         if (!read_system (usr.files, active)) {
            if (strstr (argument, "/2") != NULL)
               display_area_list(active, 1, xp);
            else if (strstr (argument, "/3") != NULL)
               display_area_list(active, 3, xp);
            else if (strstr (argument, "/4") != NULL)
               display_area_list(active, 4, xp);
            else
               display_area_list(active, 2, xp);
         }
         else
            status_line(msgtxt[M_BBS_SPAWN], usr.files, sys.file_name);
         if (strstr (argument, "/NS") == NULL)
            cls ();
         gosub_menu (argument);
         break;
      case _GOODBYE:
         if (logoff_procedure())
            return (1);
         break;
      case _SHOW:
         if (stristr (argument, "/NS") == NULL)
            cls ();
         if (stristr (argument, "/R") == NULL)
            read_file (argument);
         else
            naplps_read_file (argument);
         if (stristr (argument, "/P") != NULL)
            press_enter ();
         break;
      case _YELL:
         if (stristr (argument, "/NL") == NULL) {
            if (!lorainfo.wants_chat)
               status_line (":User request chat"),
            lorainfo.wants_chat = 1;
            lorainfo.yelling++;
            if (function_active == 1)
               f1_status();
         }
         yelling_at_sysop (argument);
         break;
      case _CONFIG:
         user_configuration();
         if (strstr (argument, "/R") != NULL)
            press_enter ();
         break;
      case _USERLIST:
         user_list(argument);
         break;
      case _BULLETIN_MENU:
         bulletin_menu (argument);
         break;
      case _OUTSIDE:
         open_outside_door(argument);
         break;
      case _VERSION:
         software_version (argument);
         break;
      case _QUOTES:
         if (strstr (argument, "/NS") == NULL)
            cls ();
         read_system_file ("APHORISM");
         show_quote();
         if (strstr (argument, "/P") != NULL)
            press_enter ();
         break;
      case _CLEAR_GOTO:
         menu_sp = 0;
         mnu_file = NULL;
         if ((p=strchr (argument, ' ')) != NULL)
            *p = '\0';
         strcpy(mnu_name, argument);
         readed = 0;
         if (strstr (argument, "/NS") == NULL)
            cls ();
         break;
      case _CLEAR_GOSUB:
         menu_sp = 0;
         mnu_file = NULL;
         if (!get_menu_password (cmd[i].argument))
            break;
         if (strstr (argument, "/NS") == NULL)
            cls ();
         gosub_menu (argument);
         break;
      case _MAIN:
         if (!get_menu_password (cmd[i].argument))
            break;
         strcpy(mnu_name, "MAIN");
         readed = 0;
         active = 0;
         if (strstr (argument, "/NS") == NULL)
            cls ();
         show_quote();
         break;
      case _CHG_AREA:
         xp = get_sig (argument);
         do {
            if (strstr (argument, "/R") != NULL) {
               if (strstr (argument, "/2") != NULL)
                  display_area_list(active, 0x81, xp);
               else if (strstr (argument, "/3") != NULL)
                  display_area_list(active, 0x83, xp);
               else if (strstr (argument, "/4") != NULL)
                  display_area_list(active, 0x84, xp);
               else
                  display_area_list(active, 0x82, xp);
            }
            else {
               if (strstr (argument, "/2") != NULL)
                  display_area_list(active, 1, xp);
               else if (strstr (argument, "/3") != NULL)
                  display_area_list(active, 3, xp);
               else if (strstr (argument, "/4") != NULL)
                  display_area_list(active, 4, xp);
               else
                  display_area_list(active, 2, xp);
            }
            if (!CARRIER || time_remain() <= 0)
               break;

            s = 0;
            if (active == 1)
               s = read_system(usr.msg, active);
            else if (active == 2)
               s = read_system(usr.files, active);
         } while (!s);
         if (active == 1) {
            if (sys.quick_board || sys.gold_board)
               quick_scan_message_base (sys.quick_board, sys.gold_board, usr.msg, 1);
            else if (sys.pip_board)
               pip_scan_message_base (usr.msg, 1);
            else if (sys.squish)
               squish_scan_message_base (usr.msg, sys.msg_path, 1);
            else
               scan_message_base(usr.msg, 1);
            allow_reply = 0;
         }
         first_time = 1;
         break;
      case _MSG_KILL:
         msg_kill();
         break;
      case _GOTO_MENU:
         xp = get_sig (argument);
         if ((p=strstr(argument, "/M")) != NULL) {
            active = 1;
            if (p[2] == '=')
               usr.msg = atoi (p + 3);
            if (!read_system (usr.msg, active)) {
               if (strstr (argument, "/2") != NULL)
                  display_area_list(active, 1, xp);
               else if (strstr (argument, "/3") != NULL)
                  display_area_list(active, 3, xp);
               else if (strstr (argument, "/4") != NULL)
                  display_area_list(active, 4, xp);
               else
                  display_area_list(active, 2, xp);
            }
            else
               status_line(msgtxt[M_BBS_EXIT], usr.msg, sys.msg_name);
            if (sys.quick_board || sys.gold_board)
               quick_scan_message_base (sys.quick_board, sys.gold_board, usr.msg, 1);
            else if (sys.pip_board)
               pip_scan_message_base (usr.msg, 1);
            else if (sys.squish)
               squish_scan_message_base (usr.msg, sys.msg_path, 1);
            else
               scan_message_base(usr.msg, 1);
            allow_reply = 0;
         }
         if ((p=strstr(argument, "/F")) != NULL) {
            active = 2;
            if (p[2] == '=')
               usr.files = atoi(p + 3);
            if (!read_system (usr.files, active)) {
               if (strstr (argument, "/2") != NULL)
                  display_area_list(active, 1, xp);
               else if (strstr (argument, "/3") != NULL)
                  display_area_list(active, 3, xp);
               else if (strstr (argument, "/4") != NULL)
                  display_area_list(active, 4, xp);
               else
                  display_area_list(active, 2, xp);
            }
            else
               status_line(msgtxt[M_BBS_SPAWN], usr.files, sys.file_name);
         }
         if ((p=strchr(argument, ' ')) != NULL)
            *p = '\0';
         strcpy(mnu_name, argument);
         readed = 0;
         if (strstr (argument, "/NS") == NULL)
            cls ();
         break;
      case _F_TITLES:
         if (argument[0] != '\0') {
            if ((p = strstr (argument, " ")) != NULL) {
               while (*p == ' ')
                  p++;
               *(p - 1) = '\0';
               file_list (argument, p);
               *(p - 1) = ' ';
            }
            else
               file_list (argument, NULL);
         }
         else if (active == 2)
            file_list(sys.filepath, sys.filelist);
         break;
      case _F_DNLD:
         set_useron_record(UPLDNLD, 0, 0);
         v = 0;
         if (stristr (argument, "/NOREM"))
            v = -2;
         if ((p = stristr (argument, "/F=")) != NULL) {
            strcpy (buf, &p[3]);
            p = strtok (buf, " ");
            translate_filenames (p, 0, NULL);
         }
         else
            p = sys.filepath;
         download_file (p, v);
         break;
      case _F_DISPL:
         file_display();
         break;
      case _F_RAWDIR:
         raw_dir();
         break;
      case _F_KILL:
         file_kill (0, NULL);
         break;
      case _SET_PWD:
         password_change();
         break;
      case _SET_HELP:
         v = select_language ();
         if (v != -1) {
#ifndef __OS2__
            if (cmd != NULL) {
               free (cmd);
               cmd = NULL;
            }
#endif
            usr.language = (byte) v;
            free (bbstxt);
            if (!load_language (usr.language)) {
               usr.language = 0;
               load_language (usr.language);
            }
            if (config->language[usr.language].txt_path[0])
               text_path = config->language[usr.language].txt_path;
            else
               text_path = config->language[0].txt_path;
            readed = 0;
         }
         break;
      case _SET_NULLS:
         nulls_change();
         break;
      case _SET_LEN:
         screen_change();
         break;
      case _SET_TABS:
         tabs_change();
         break;
      case _SET_MORE:
         more_change();
         break;
      case _SET_CLS:
         formfeed_change();
         break;
      case _SET_EDIT:
         fullscreen_change();
         break;
      case _SET_CITY:
         city_change ();
         break;
      case _SET_SCANMAIL:
         scanmail_change ();
         break;
      case _SET_AVATAR:
         avatar_change ();
         break;
      case _SET_ANSI:
         ansi_change ();
         break;
      case _SET_COLOR:
         color_change ();
         break;
      case _MSG_EDIT_NEW:
         s = active;
         active = 1;
         lo = 0;
         if (strstr (argument, "/L"))
            lo = 1;
         set_useron_record (READWRITE, 0, 0);
         if ((gg = get_message_data (0, argument)) == 1) {
            if (usr.use_lore) {
               line_editor (0);
               gosub_menu (argument);
            }
            else {
               if (external_editor (0))
                  if (lo) {
                     logoff_procedure ();
                     return (1);
                  }
               active = s;
            }
         }
         else {
            if (gg == 0)
               m_print(bbstxt[B_LORE_MSG3]);

            active = s;
         }
         break;
      case _MSG_EDIT_REPLY:
         if (!allow_reply)
            break;

         s = active;
         active = 1;
         set_useron_record (READWRITE, 0, 0);

         if ((gg = get_message_data (lastread, argument)) == 1) {
            if (usr.use_lore) {
               line_editor(0);
               gosub_menu (argument);
            }
            else {
               external_editor (1);
               active = s;
            }
         }
         else {
            if (gg == 0)
               m_print (bbstxt[B_LORE_MSG3]);

            active = s;
         }
         break;
      case _ED_SAVE:
			if (sys.quick_board||sys.gold_board)
				quick_save_message(NULL);
         else if (sys.pip_board)
            pip_save_message(NULL);
         else if (sys.squish)
            squish_save_message(NULL);
         else
            save_message(NULL);

         usr.msgposted++;

         if (lo) {
            free_message_array();
            logoff_procedure ();
            return (1);
         }

         if (strstr(argument, "/RET")) {
            active = s;
            free_message_array();
            return_menu();
         }
         break;
      case _ED_ABORT:
         change_attr (LRED|_BLACK);
         m_print(bbstxt[B_LORE_MSG3]);
         free_message_array();
         active = s;
         return_menu();
         break;
      case _ED_LIST:
         edit_list();
         break;
      case _ED_CHG:
         edit_line();
         break;
      case _ED_INSERT:
         edit_insert();
         break;
      case _ED_DEL:
         edit_delete();
         break;
      case _ED_CONT:
         edit_continue();
         break;
      case _ED_TO:
         edit_change_to();
         break;
      case _ED_SUBJ:
         edit_change_subject();
         break;
      case _PRESS_ENTER:
         if (argument[0]) {
            m_print (argument);
            input(filename,2);
         }
         else
            press_enter ();
         break;
      case _CHG_AREA_NAME:
         xp = get_sig (argument);
         do {
            display_area_list(active, 3, xp);
            if (!CARRIER || time_remain() <= 0)
               break;

            s = 0;
            if (active == 1)
               s = read_system( usr.msg, active);
            else if (active == 2)
               s = read_system(usr.files, active);
         } while (!s);
         if (active == 1) {
            if (sys.quick_board || sys.gold_board)
               quick_scan_message_base (sys.quick_board, sys.gold_board, usr.msg, 1);
            else if (sys.pip_board)
               pip_scan_message_base (usr.msg, 1);
            else if (sys.squish)
               squish_scan_message_base (usr.msg, sys.msg_path, 1);
            else
               scan_message_base(usr.msg, 1);
            allow_reply = 0;
         }
         first_time = 1;
         break;
      case _MSG_LIST:
         list_headers (0);
         break;
      case _MSG_VERBOSE:
         list_headers (1);
         break;
      case _MSG_NEXT:
         read_forward(lastread,0);
         break;
      case _MSG_PRIOR:
         read_backward(lastread);
         break;
      case _MSG_NONSTOP:
         read_nonstop();
         break;
		case _MSG_PARENT:
			read_parent ();
			break;
		case _MSG_CHILD:
			read_reply ();
			break;
		case _MSG_SCAN:
			if (scan_mailbox())
			{
				v = active;
				active = 4;
				i = -1;
				mail_read_forward (0);
				allow_reply = 1;
				gosub_menu ("READMAIL");
			}
			break;
		case _MSG_INQ:
			message_inquire ();
         break;
      case _GOSUB_MENU:
         if (!get_menu_password (cmd[i].argument))
            break;
         xp = get_sig (argument);
         if ((p=strstr(argument, "/M")) != NULL) {
            active = 1;
            if (p[2] == '=')
               usr.msg = atoi(p + 3);
            if (!read_system(usr.msg, active))
               display_area_list(active, 1, xp);
            else
               status_line(msgtxt[M_BBS_EXIT], usr.msg, sys.msg_name);
            if (sys.quick_board || sys.gold_board)
               quick_scan_message_base (sys.quick_board, sys.gold_board, usr.msg, 1);
            else if (sys.pip_board)
               pip_scan_message_base (usr.msg, 1);
            else if (sys.squish)
               squish_scan_message_base (usr.msg, sys.msg_path, 1);
            else
               scan_message_base(usr.msg, 1);
            allow_reply = 0;
         }
         if ((p=strstr(argument, "/F")) != NULL) {
            active = 2;
            if (p[2] == '=')
               usr.files = atoi(p + 3);
            if (!read_system (usr.files, active))
               display_area_list(active, 1, xp);
            else
               status_line(msgtxt[M_BBS_SPAWN], usr.files, sys.file_name);
         }
         if (strstr (argument, "/NS") == NULL)
            cls ();
         gosub_menu (argument);
         break;
      case _FILE_HURL:
         hurl ();
         break;
      case _MSG_INDIVIDUAL:
         if ((p = strchr (cmd_string, ' ')) != NULL)
            *p = '\0';
         read_forward(atoi(cmd_string) - 1, 0);
         if (p != NULL)
         {
            *p = ' ';
            strcpy (cmd_string, p);
         }
         else
            cmd_string[0] = '\0';
         break;
      case _F_TAG:
         tag_files (0);
         break;
      case _RETURN_MENU:
         if (st != NULL)
            return (1);

         return_menu();

         if (active == 4) {
            active = v;

            if (active == 1) {
               read_system(usr.msg, active);
               if (sys.quick_board || sys.gold_board)
                  quick_scan_message_base (sys.quick_board, sys.gold_board, usr.msg, 1);
               else if (sys.pip_board)
                  pip_scan_message_base (usr.msg, 1);
               else if (sys.squish)
                  squish_scan_message_base (usr.msg, sys.msg_path, 1);
               else
                  scan_message_base(usr.msg, 1);
               allow_reply = 0;
            }
         }
         if (strstr (argument, "/NS") == NULL)
            cls ();
         break;
      case _MENU_CLEAR:
         menu_sp = 0;
         mnu_file = 0;
         break;
      case _F_LOCATE:
         if (strstr (argument, "/F"))
            locate_files (1);
         else
            locate_files (0);
         break;
      case _F_UPLD:
         set_useron_record (UPLDNLD, 0, 0);

         strcpy (buf, argument);
         strupr (buf);

         // Se e' presente il /C effettua prima il controllo sui file
         // mandati, altrimenti lo esegue dopo.
         if (strstr (buf, "/C") == NULL && strstr (buf, "/c") == NULL)
            no_precheck = 1;
         else
            no_precheck = 0;

         // /F= definisce il nome del file da mandare (o la directory in cui
         // far finire gli upload.
         if ((p = strstr (buf, "/F=")) != NULL) {
            p = strtok (&p[3], " ");
            upload_file (p, 0);
         }
         else
            upload_file (sys.uppath, 0);

         no_precheck = 0;
         break;
      case _SET_SIGN:
         signature_change();
         break;
      case _MAKE_LOG:
         status_line (argument);
         break;
      case _F_OVERRIDE:
         override_path ();
         break;
      case _F_NEW:
         if (strstr (argument, "/F"))
            new_file_list (1);
         else
            new_file_list (0);
         break;
      case _MAIL_NEXT:
         mail_read_forward(0);
         break;
      case _MAIL_PRIOR:
         mail_read_backward(0);
         break;
      case _MAIL_NONSTOP:
         mail_read_nonstop ();
         break;
      case _SET_FULLREAD:
         fullread_change();
         break;
      case _ONLINE_MESSAGE:
         if (stristr (argument, "/H"))
            send_online_message (1);
         else
            send_online_message (0);
         break;
      case _MAIL_END:
         if (st != NULL)
            return (1);

         return_menu();

         if (old_activ) {
            active = old_activ;
            if (active == 1)
               read_system(usr.msg, active);
            old_activ = 0;
         }
         break;
      case _CHG_AREA_LONG:
         xp = get_sig (argument);
         do {
            display_area_list(active, 2, xp);
            if (!CARRIER || time_remain() <= 0)
               break;

            s = 0;
            if (active == 1)
               s = read_system( usr.msg, active);
            else if (active == 2)
               s = read_system(usr.files, active);
         } while (!s);
         if (active == 1) {
            if (sys.quick_board || sys.gold_board)
               quick_scan_message_base (sys.quick_board, sys.gold_board, usr.msg, 1);
            else if (sys.pip_board)
               pip_scan_message_base (usr.msg, 1);
            else if (sys.squish)
               squish_scan_message_base (usr.msg, sys.msg_path, 1);
            else
               scan_message_base(usr.msg, 1);
            allow_reply = 0;
         }
         first_time = 1;
         break;
      case _ONLINE_USERS:
         if (stristr (argument, "/H") != NULL)
            online_users (1, 1);
         else
            online_users (1, 0);
         break;
      case _MAIL_LIST:
         if (active == 4)
            mail_list ();
         break;
      case _WRITE_NEXT_CALLER:
         message_to_next_caller ();
         break;
      case _F_GLOBAL_DNLD:
         set_useron_record(UPLDNLD, 0, 0);
         v = 1;
         if (stristr (argument, "/NOREM"))
            v = -3;
         if ((p = stristr (argument, "/F=")) != NULL) {
            strcpy (buf, &p[3]);
            p = strtok (buf, " ");
            translate_filenames (p, 0, NULL);
         }
         else
            p = sys.filepath;
         download_file (p, v);
         break;
      case _MAIL_INDIVIDUAL:
         if ((p = strchr (cmd_string, ' ')) != NULL)
            *p = '\0';
         last_mail = atoi(cmd_string) - 1;
         mail_read_forward(0);
         if (p != NULL)
         {
            *p = ' ';
            strcpy (cmd_string, p);
         }
         else
            cmd_string[0] = '\0';
         break;
      case _DRAW_WINDOW:
         time_statistics ();
         break;
      case _CHAT_WHOIS:
         cb_who_is_where (1);
         break;
      case _CB_CHAT:
         cb_chat();
         break;
      case _SHOW_TEXT:
         if (stristr (argument, "/NS") == NULL)
            cls ();
         read_system_file (argument);
         if (stristr (argument, "/P") != NULL)
            press_enter ();
         break;
      case _LASTCALLERS:
         show_lastcallers(argument);
         break;
      case _SET_HANDLE:
         handle_change ();
         break;
      case _SET_VOICE:
         voice_phone_change ();
         break;
      case _SET_DATA:
         data_phone_change ();
         break;
      case _F_CONTENTS:
         display_contents ();
         break;
      case _COUNTER:
         if ((p=strstr(argument, "/C=")) != NULL)
         {
            gg = atoi(p + 3);
            if (gg < 0 || gg >= MAXCOUNTER)
               break;
            if ((p=strstr(argument, "/I=")) != NULL)
               usr.counter[gg] += atoi (p + 3);
            if ((p=strstr(argument, "/D=")) != NULL)
               usr.counter[gg] -= atoi (p + 3);
            if ((p=strstr(argument, "/R=")) != NULL)
               usr.counter[gg] = atoi (p + 3);
         }
         break;
      case _MSG_TAG:
         xp = get_sig (argument);

         if (strstr (argument, "/2") != NULL)
            tag_area_list (1, xp);
         else if (strstr (argument, "/3") != NULL)
            tag_area_list (3, xp);
         else if (strstr (argument, "/4") != NULL)
            tag_area_list (4, xp);
         else
            tag_area_list (2, xp);
         break;
      case _ASCII_DOWNLOAD:
         offline_pack_tagged_areas (0);
         break;
      case _RESUME_DOWNLOAD:
         resume_transmission ();
         break;
      case _USAGE:
         display_usage ();
         break;
      case _BANK_ACCOUNT:
         show_account ();
         break;
      case _BANK_DEPOSIT:
			deposit_time ();
         break;
      case _BANK_WITHDRAW:
         withdraw_time ();
         break;
      case _BANK_KDEPOSIT:
         deposit_kbytes ();
         break;
      case _BANK_KWITHDRAW:
         withdraw_kbytes ();
         break;
      case _SET_HOTKEY:
         hotkey_change ();
         break;
      case _BBSLIST_ADD:
         if ((p = strstr (argument, "/L=")) != NULL)
            bbs_add_list (atoi (&p[3]));
         else
            bbs_add_list (0);
         break;
      case _BBSLIST_SHORT:
         if ((p = strstr (argument, "/L=")) != NULL)
            bbs_short_list (atoi (&p[3]));
         else
            bbs_short_list (0);
         break;
      case _BBSLIST_LONG:
         if ((p = strstr (argument, "/L=")) != NULL)
            bbs_long_list (atoi (&p[3]));
         else
            bbs_long_list (0);
         break;
      case _BBSLIST_CHANGE:
         if (strstr (argument, "/R") != NULL)
            xp = 1;
         else
            xp = 0;
         if ((p = strstr (argument, "/L=")) != NULL)
            bbs_change (atoi (&p[3]), xp);
         else
            bbs_change (0, xp);
         break;
      case _BBSLIST_REMOVE:
         if (strstr (argument, "/R") != NULL)
            xp = 1;
         else
            xp = 0;
         if ((p = strstr (argument, "/L=")) != NULL)
            bbs_remove (atoi (&p[3]), xp);
         else
            bbs_remove (0, xp);
         break;
      case _QWK_DOWNLOAD:
         qwk_pack_tagged_areas ();
         break;
      case _QWK_UPLOAD:
         getrep ();
         break;
//      case _VOTE_USER:
//         if ((p=strstr(argument, "/V=")) != NULL) {
//            xp = atoi(p + 3);
//            if (xp != 0)
//               vote_user (xp);
//         }
//         break;
      case 117:
         ibmset_change ();
         break;
      case 118:
         if (!get_menu_password (argument))
            break;
         xp = get_sig (argument);
         display_new_area_list (xp);
         if (sys.quick_board || sys.gold_board)
             quick_scan_message_base (sys.quick_board, sys.gold_board, usr.msg, 1);
         else if (sys.pip_board)
            pip_scan_message_base (usr.msg, 1);
         else if (sys.squish)
            squish_scan_message_base (usr.msg, sys.msg_path, 1);
         else
            scan_message_base(usr.msg, 1);
         allow_reply = 0;
         break;
      case 119:
         xport_message ();
         break;
      case 120:
         if ((p=strchr(argument,'/')) != NULL && isdigit(*(p + 1)))
            bbs_list_download (atoi (p + 1));
         else
            bbs_list_download (0);
         break;
      case 121:
         if ((p=strstr(argument, "/G=")) != NULL)
            xp = atoi (&p[3]);
         else
            xp = -1;

         if (strstr(argument, "/M") != NULL)
            select_group (1, xp);
         else if (strstr(argument, "/F") != NULL)
            select_group (2, xp);
         else
            select_group (0, xp);
         break;
      case 122:
         upload_filebox ();
         break;
      case 123:
         download_filebox (1);
         break;
      case 124:
         ask_birthdate ();
         break;
      case 125:
         ask_default_protocol ();
         break;
      case 126:
         ask_default_archiver ();
         break;
      case _FBOX_KILL:
         file_kill (1, NULL);
         break;
      case _FBOX_LIST:
         download_filebox (0);
         break;
      case 129:
         if (stristr (argument, "/R") != NULL)
            list_tagged_files (1);
         else
            list_tagged_files (0);
         break;
      case 130:
         kludge_change ();
         break;
      case 131:
         offline_pack_tagged_areas (1);
         break;
      case _MSG_FORWARD:
         xp = get_sig (argument);
         if (strstr (argument, "/2") != NULL)
            forward_message_area (xp, 1);
         else if (strstr (argument, "/3") != NULL)
            forward_message_area (xp, 3);
         else if (strstr (argument, "/4") != NULL)
            forward_message_area (xp, 4);
         else
            forward_message_area (xp, 2);
         break;
   }
   return (0);
}

static void return_menu()
{
   if (menu_sp) {
      menu_sp--;
      strcpy (mnu_name, menu_stack[menu_sp]);
      if (file_stack[menu_sp][0] == '\0')
         mnu_file = NULL;
      else
         mnu_file = file_stack[menu_sp];
      i = -1;
      readed = 0;
   }
}

static void gosub_menu (char *m)
{
   char *p, *v;

   if (menu_sp < MENU_STACK) {
      strcpy (menu_stack[menu_sp], mnu_name);
      if (mnu_file == NULL)
         file_stack[menu_sp][0] = '\0';
      else
         strcpy (file_stack[menu_sp], mnu_file);
      menu_sp++;

      if ((p = stristr (m, "/L=")) != NULL) {
         if ((v = strchr (p, ' ')) == NULL)
            v = p;
         else
            *v = '\0';
         strcpy (file_stack[menu_sp], v);
         if (v != p)
            *v = ' ';
         mnu_file = file_stack[menu_sp];
      }
      else {
         if (mnu_file == NULL)
            file_stack[menu_sp][0] = '\0';
         else
            strcpy (file_stack[menu_sp], mnu_file);
      }

      if ((p = strchr (m, ' ')) == NULL)
         p = m;
      else
         *p = '\0';

      strcpy (mnu_name, m);
      readed = 0;

      if (p != m)
         *p = ' ';
   }
}

static void process_user_display (st, item)
char *st;
int item;
{
   int m, first, i, fd;
   char ext[256], buffer[50], *p, parola[36];
   long tempo;
   struct tm *tim;

   m = 0;
   ext[0] = '\0';
   first = 1;

   for (p=st; *p; p++)
      switch (*p) {
      case '^':
         if (first)
            sprintf(buffer,"%c", cmd[item].first_color);
         else
            sprintf(buffer,"%c", cmd[item].color);
         strcat(ext, buffer);
         m += strlen(buffer);
         first = first ? 0 : 1;
         break;
      case '~':
         sprintf(buffer,"%d", time_remain());
         strcat(ext, buffer);
         m += strlen(buffer);
         break;
      case CTRLF:
         p++;
         switch(*p) {
            case '"':
               for (i = 0; i < 10; i++) {
                  if (config->packid[i].display[0] == usr.archiver)
                     break;
               }
               if (i < 10)
                  sprintf (buffer, "%s", &config->packid[i].display[1]);
               else
                  sprintf (buffer, "%s", &bbstxt[B_NONE][1]);
               strcat (ext, buffer);
               m += strlen (buffer);
               break;
            case '%':
               if (usr.protocol == protocols[0][0])
                  sprintf (buffer, "%s", &protocols[0][1]);
               else if (usr.protocol == protocols[1][0])
                  sprintf (buffer, "%s", &protocols[1][1]);
               else if (usr.protocol == protocols[2][0])
                  sprintf (buffer, "%s", &protocols[2][1]);
               else if (usr.protocol == protocols[5][0])
                  sprintf (buffer, "%s", &protocols[5][1]);
               else if (config->hslink && usr.protocol == protocols[3][0])
                  sprintf (buffer, "%s", &protocols[3][1]);
               else if (config->puma && usr.protocol == protocols[4][0])
                  sprintf (buffer, "%s", &protocols[4][1]);
               else {
                  PROTOCOL prot;

                  sprintf (buffer, "%sPROTOCOL.DAT", config->sys_path);
                  fd = sh_open (buffer, O_RDONLY|O_BINARY, SH_DENYWR, S_IREAD|S_IWRITE);
                  buffer[0] = '\0';
                  if (fd != -1) {
                     while (read (fd, &prot, sizeof (PROTOCOL)) == sizeof (PROTOCOL)) {
                        if (prot.active && prot.hotkey == usr.protocol) {
                           sprintf (buffer, "%s", prot.name);
                           break;
                        }
                     }
                     close (fd);
                  }
                  if (fd == -1 || !prot.active || prot.hotkey != usr.protocol)
                     sprintf (buffer, "%s", &protocols[B_NONE][1]);
               }
               strcat (ext, buffer);
               m += strlen (buffer);
               break;
            case 'C':
               if (cps) {
                  sprintf (buffer, "%ld", cps);
                  strcat(ext, buffer);
                  m += strlen(buffer);
               }
               break;
            case 'D':
               sprintf (buffer, "%s",usr.dataphone);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'E':
               sprintf (buffer, "%s",usr.voicephone);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'F':
               usr.ldate[9] = '\0';
               sprintf (buffer, "%s", usr.ldate);
               usr.ldate[9] = ' ';
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'G':
               sprintf (buffer, "%s",&usr.ldate[11]);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'H':
               if (cps) {
                  sprintf (buffer, "%ld", (cps * 100) / (rate / 10));
                  strcat(ext, buffer);
                  m += strlen(buffer);
               }
               break;
            case 'I':
               sprintf (buffer, "%s",usr.ibmset ? bbstxt[B_YES] : bbstxt[B_NO]);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'L':
               sprintf (buffer, "%d", usr.credit);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'M':
               sprintf (buffer, "%d", last_mail);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'N':
               sprintf (buffer, "%d", lastread);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'O':
               sprintf (buffer, "%s", get_priv_text(usr.priv));
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'Q':
               sprintf (buffer, "%u", usr.n_upld);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'R':
               sprintf (buffer, "%lu", usr.upld);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'S':
               sprintf (buffer, "%u", usr.n_dnld);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'T':
               sprintf (buffer, "%lu", usr.dnld);
               strcat(ext, buffer);
               m += strlen(buffer);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'V':
               sprintf (buffer, "%d", usr.len);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'X':
               sprintf (buffer, "%s",usr.ansi ? bbstxt[B_YES] : bbstxt[B_NO]);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'Y':
               sprintf (buffer, "%s",usr.more ? bbstxt[B_YES] : bbstxt[B_NO]);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'Z':
               sprintf (buffer, "%s",usr.formfeed ? bbstxt[B_YES] : bbstxt[B_NO]);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '0':
               sprintf (buffer, "%s",usr.use_lore ? bbstxt[B_NO] : bbstxt[B_YES]);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '2':
               sprintf (buffer, "%d", get_user_age ());
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '3':
               sprintf (buffer, "%s",usr.hotkey ? bbstxt[B_YES] : bbstxt[B_NO]);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '4':
               sprintf (buffer, "%s",usr.firstdate);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '5':
               sprintf (buffer, "%s",usr.birthdate);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '6':
               sprintf (buffer, "%s",usr.scanmail ? bbstxt[B_YES] : bbstxt[B_NO]);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '7':
               sprintf (buffer, "%s",usr.subscrdate);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '8':
               sprintf (buffer, "%s",usr.avatar ? bbstxt[B_YES] : bbstxt[B_NO]);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '!':
               sprintf (buffer, "%s",usr.color ? bbstxt[B_YES] : bbstxt[B_NO]);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case CTRLA:
               show_quote();
               break;
            case 'A':
            case CTRLB:
               sprintf (buffer, "%s", usr.name);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'B':
            case CTRLC:
               sprintf (buffer, "%s", usr.city);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case CTRLD:
               data(parola);
               buffer[9] = '\0';
               sprintf (buffer, "%s", parola);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'P':
            case CTRLE:
               sprintf (buffer, "%ld",usr.times);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'W':
            case CTRLF:
               strcpy(buffer,usr.name);
               get_fancy_string(buffer, parola);
               strcat(ext, parola);
               m += strlen(parola);
               break;
            case CTRLG:
               timer(10);
               break;
            case CTRLK:
               m = (int)((time(NULL)-start_time)/60);
               m += usr.time;
               sprintf (buffer, "%d",m);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'U':
            case CTRLL:
               m = (int)((time(0)-start_time)/60);
               sprintf (buffer, "%d",m);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case CTRLO:
               sprintf (buffer, "%d",time_remain());
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case CTRLP:
               sprintf (buffer, "%s",ctime(&start_time));
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case CTRLQ:
               sprintf (buffer, "%lu",sysinfo.total_calls);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case CTRLR:
               sprintf (buffer, "%lu",usr.dnld-usr.upld);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case CTRLS:
               strcat(ext, usr.signature);
               m += strlen(usr.signature);
               break;
            case CTRLT:
               data(buffer);
               strcpy (buffer, &buffer[10]);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case CTRLW:
               sprintf (buffer, "%lu", usr.upld);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case CTRLX:
               sprintf (buffer, "%lu",usr.dnld);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '9':
               if (usr.n_upld == 0)
                  sprintf (buffer, "0:%u", usr.n_dnld);
               else {
                  m = (unsigned int)(usr.n_dnld / usr.n_upld);
                  sprintf (buffer, "1:%u", m);
               }
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case CTRLY:
            case ':':
               if (usr.upld == 0)
                  sprintf (buffer, "0:%lu", usr.dnld);
               else {
                  m = (unsigned int)(usr.dnld / usr.upld);
                  sprintf (buffer, "1:%u", m);
               }
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case ';':
               sprintf (buffer, "%s",usr.full_read ? bbstxt[B_YES] : bbstxt[B_NO]);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '[':
               sprintf (buffer, "%lu", config->class[usr_class].max_dl - usr.dnldl);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '\\':
               sprintf (buffer, "%s", &config->language[usr.language].descr[1]);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case ']':
               sprintf (buffer, "%s", usr.comment);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
         }
         break;
      case CTRLK:
         p++;
         switch(*p) {
            case 'A':
               sprintf (buffer, "%lu",sysinfo.total_calls);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'B':
               sprintf (buffer, "%s",lastcall.name);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'C':
               sprintf (buffer, "%d", msg_child);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'D':
               sprintf (buffer, "%d",first_msg);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'E':
               sprintf (buffer, "%d",last_msg);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'G':
               tempo = time(0);
               tim = localtime(&tempo);
               sprintf (buffer, "%s", bbstxt[B_SUNDAY + tim->tm_wday]);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'I':
               data(buffer);
               strcpy (buffer, &buffer[10]);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'J':
               data(buffer);
               buffer[9] = '\0';
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'K':
               m = (int)((time(NULL)-start_time)/60);
               sprintf (buffer, "%d",m);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'M':
               sprintf (buffer, "%d",max_priv_mail);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'O':
               sprintf (buffer, "%d",time_remain());
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'P':
               sprintf (buffer, "%d", msg_parent);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'Q':
               sprintf (buffer, "%d",config->class[usr_class].max_call);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'R':
               sprintf (buffer, "%lu", local_mode ? 0L : rate);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'T':
               sprintf (buffer, "%d",config->class[usr_class].max_dl);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'U':
               sprintf (buffer, "%d", time_to_next (1));
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'W':
               sprintf (buffer, "%d", line_offset);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case 'Y':
               strcat(ext, sys.msg_name);
               m += strlen(sys.msg_name);
               break;
            case 'Z':
               strcat(ext, sys.file_name);
               m += strlen(sys.file_name);
               break;
            case '0':
            case '9':
               sprintf (buffer, "%d",num_msg);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '1':
               sprintf (buffer, "%d",usr.msg);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '2':
               sprintf (buffer, "%d",usr.files);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '5':
               sprintf (buffer, "%s", sys.msg_name);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '7':
               sprintf (buffer, "%d",usr.account);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '8':
               sprintf (buffer, "%d",usr.f_account);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '[':
               sprintf (buffer, "%d", config->class[usr_class].max_dl - usr.dnldl);
               strcat(ext, buffer);
               m += strlen(buffer);
               break;
            case '\\':
               del_line ();
               break;
         }
         break;
      case CTRLV:
         p++;

         i = 0;
         buffer[i++] = CTRLV;

         switch(*p) {
         case CTRLA:
            buffer[i++] = CTRLA;

            p++;
            if (*p == CTRLP)
               buffer[i++] = CTRLP;

            buffer[i++] = *p;

            buffer[i] = '\0';
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case CTRLC:
            buffer[i++] = CTRLC;
            buffer[i] = '\0';
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case CTRLD:
            buffer[i++] = CTRLD;
            buffer[i] = '\0';
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case CTRLE:
            buffer[i++] = CTRLE;
            buffer[i] = '\0';
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case CTRLF:
            buffer[i++] = CTRLF;
            buffer[i] = '\0';
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case CTRLG:
            buffer[i++] = CTRLG;
            buffer[i] = '\0';
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case CTRLH:
            buffer[i++] = *(p+1);
            buffer[i++] = *(p+2);
            buffer[i] = '\0';
            strcat(ext, buffer);
            m += strlen(buffer);
            p+=2;
            break;
         }
         break;
      case CTRLW:
         p++;
         switch(*p) {
            case 'P':
               strcat (ext, usr.voicephone);
               m += strlen (usr.voicephone);
               break;
            case 'R':
               strcat (ext, usr.handle);
               m += strlen (usr.handle);
               break;
            case 'S':
               strcat (ext, usr.signature);
               m += strlen (usr.signature);
               break;
         }
         break;
      default:
         ext[m++] = *p;
         ext[m] = '\0';
         break;
      }

   change_attr(cmd[item].color);

   m = strlen(ext) - 1;
   if (ext[m] == ';') {
      ext[m] = '\0';
      m_print("%s\n",ext);
   }
   else
      m_print("%s",ext);
}

static int get_menu_password (args)
char *args;
{
   char *s, *p, password[40], c;

   if ((s=strstr(args,"/P=")) == NULL)
      return (1);

   s+=3;
   if ((p = strchr(s, ' ')) == NULL)
      p = strchr(s, '\0');
   c = *p;
   *p = '\0';

   change_attr(LRED|_BLACK);

   if (!get_command_word (password, 39))
      do {
         m_print(bbstxt[B_MENU_PASSWORD]);
         inpwd(password, 39);
         if (!CARRIER || !strlen(password))
         {
            *p = c;
            return (0);
         }
      } while(!strlen(password));

   if(!stricmp(password, s))
   {
      *p = c;
      return (1);
   }

   change_attr(YELLOW|_BLACK);
   m_print(bbstxt[B_DENIED]);
   status_line(msgtxt[M_BAD_PASSWORD], password);
   press_enter ();

   *p = c;

   return (0);
}

static void get_buffer_keystrokes (args)
char *args;
{
   char *s, *p, c;

   if (!registered)
      return;

   if ((s=strstr(args,"/K=")) == NULL)
      return;

   s+=3;
   if ((p = strchr(s, ' ')) == NULL)
      p = strchr(s, '\0');
   c = *p;
   *p = '\0';

   if (cmd_string[0] == '\0')
      strcat (cmd_string, s);
   else
      strins (s, cmd_string, 1);

   *p = c;
}

static int ok_counter (args)
char *args;
{
   char *p, op;
   int gg, val;

   if ((p = strstr (args, "/C")) == NULL)
      return (1);

   if (!isdigit (p[2]))
      return (1);
   if (sscanf ( (p + 2), "%d%c%d", &gg, &op, &val) < 3)
      return (1);

   if (gg < 0 || gg >= MAXCOUNTER)
      return (0);

   switch (op) {
      case '=':
         if (usr.counter[gg] == val)
            return (1);
         break;
      case '<':
         if (usr.counter[gg] < val)
            return (1);
         break;
      case '>':
         if (usr.counter[gg] > val)
            return (1);
         break;
      case '!':
         if (usr.counter[gg] != val)
            return (1);
         break;
      default:
         return (1);
   }

   return (0);
}

static int get_sig (argument)
char *argument;
{
   char *p;
   int xp = 0;

   if ((p=strstr(argument, "/F")) != NULL)
      active = 2;

   if ((p=strstr(argument, "/M")) != NULL)
      active = 1;

   if ((p=strstr(argument, "/G")) != NULL) {
      if ( *(p +2) == '=') {
         if ( *(p + 3) == '?' )
            xp = -1;
         else
            xp = atoi(p + 3);
      }
      else {
         if (active == 1)
            xp = usr.msg_sig;
         else if (active == 2)
            xp = usr.file_sig;
      }
   }

   return (xp);
}

static int user_age_ok (char *arg)
{
   int i;
   char *p, argument[80];

   strcpy (argument, arg);
   strupr (argument);
   if ((p = strstr (argument, "/AGE=")) != NULL) {
      p = strtok (p, " ");
      i = atoi (&p[5]);
      if (i > get_user_age ())
         return (0);
   }

   return (1);
}

