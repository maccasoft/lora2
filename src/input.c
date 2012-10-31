#include <stdio.h>
#include <conio.h>
#include <ctype.h>
#include <string.h>
#include <dir.h>

#include <cxl\cxlwin.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

#include <cxl/cxlstr.h>

static long online_msg;
static int read_online_message(void);

int freeze = 0;
char num_hotkey = 0;

#ifdef __OS2__
void VioUpdate (void);
#endif

void input(s,width)
char *s;
int width;
{
   chars_input(s,width,0);
}

void inpwd(s,width)
char *s;
int width;
{
   chars_input(s,width,INPUT_PWD);
}

void cmd_input(s,width)
char *s;
int width;
{
   chars_input(s,width,INPUT_HOT);
}

void fancy_input(s,width)
char *s;
int width;
{
   chars_input(s,width,INPUT_FANCY);
}

void update_input(s,width)
char *s;
int width;
{
   chars_input(s,width,INPUT_UPDATE);
}

void chars_input (char *s, int width, int flag)
{
   char autozm[10];
   unsigned char c;
   int i, upper;
   long inactive, warning, checktime;

   upper = 1;
   online_msg = timerset(200);
   cmd_string[0] = '\0';
   if (config->inactivity_timeout) {
      inactive = timerset (0);
      warning = timerset (0);
      inactive += config->inactivity_timeout * 6000L;
      warning += config->inactivity_timeout * 6000L - 2000L;
   }
   else
      warning = inactive = 0L;

   if (!local_mode)
      UNBUFFER_BYTES ();

   if ((flag & INPUT_FIELD) && usr.color) {
      space (width);
      for (i=0;i<width;i++) {
          if (!local_mode)
             SENDBYTE('\b');
          if (snooping)
             wputs("\b");
      }
   }

   if (flag & INPUT_UPDATE) {
      m_print (s);
      i = strlen (s);
   }
   else
      i = 0;

#ifdef __OS2__
   VioUpdate ();
#endif

   while (CARRIER) {
      if (!local_mode) {
         while (!CHAR_AVAIL ()) {
            if (!CARRIER)
               return;

            if (freeze && config->inactivity_timeout) {
               freeze = 0;

               inactive = timerset (0);
               warning = timerset (0);
               inactive += config->inactivity_timeout * 6000L;
               warning += config->inactivity_timeout * 6000L - 2000L;
            }

            if (inactive && !freeze) {
               if (timeup (inactive)) {
                  m_print (bbstxt[B_INACTIVE_HANG]);
                  terminating_call ();
                  get_down (aftercaller_exit, 2);
               }
               if (warning && timeup (warning)) {
                  m_print (bbstxt[B_INACTIVE_WARN]);
                  warning = 0L;
               }
            }

            if (time_remain () <= 0) {
               m_print(bbstxt[B_TIMEOUT]);
               terminating_call ();
               get_down (aftercaller_exit, 2);
            }

            if (read_online_message ()) {
               s[0] = '\0';
               return;
            }

            if (local_kbd == 0x2E00) {
               sysop_chatting ();
               s[0] = '\0';
               s[1] = 5;
               return;
            }
            else if (local_kbd != -1)
               break;

            checktime = time (NULL);

            time_release ();
            release_timeslice ();

            if (freeze && config->inactivity_timeout) {
               if (time (NULL) - checktime > 10) {
                  freeze = 0;

                  inactive = timerset (0);
                  warning = timerset (0);
                  inactive += config->inactivity_timeout * 6000L;
                  warning += config->inactivity_timeout * 6000L - 2000L;
               }
            }
         }

         if (local_kbd == -1)
            c = (unsigned char)TIMED_READ(1);
         else {
            c = (unsigned char)local_kbd;
            local_kbd = -1;
         }
      }
      else {
         while (local_kbd == -1) {
            if (!CARRIER)
               return;

            if (freeze && config->inactivity_timeout) {
               freeze = 0;

               inactive = timerset (0);
               warning = timerset (0);
               inactive += config->inactivity_timeout * 6000L;
               warning += config->inactivity_timeout * 6000L - 2000L;
            }

            if (inactive && !freeze) {
               if (timeup (inactive)) {
                  m_print (bbstxt[B_INACTIVE_HANG]);
                  terminating_call ();
                  get_down (aftercaller_exit, 2);
               }
               if (warning && timeup (warning)) {
                  m_print (bbstxt[B_INACTIVE_WARN]);
                  warning = 0L;
               }
            }

            if (time_remain() <= 0) {
               change_attr(LRED|_BLACK);
               m_print(bbstxt[B_TIMEOUT]);
               terminating_call ();
               get_down (aftercaller_exit, 2);
            }

            if (read_online_message()) {
               s[0] = '\0';
               return;
            }

            checktime = time (NULL);

            time_release ();
            release_timeslice ();

            if (freeze && config->inactivity_timeout) {
               if (time (NULL) - checktime > 10) {
                  freeze = 0;

                  inactive = timerset (0);
                  warning = timerset (0);
                  inactive += config->inactivity_timeout * 6000L;
                  warning += config->inactivity_timeout * 6000L - 2000L;
               }
            }
         }

         c = (unsigned char)local_kbd;
         local_kbd = -1;
      }

      if (config->inactivity_timeout) {
         inactive = timerset (0);
         warning = timerset (0);
         inactive += config->inactivity_timeout * 6000L;
         warning += config->inactivity_timeout * 6000L - 2000L;
      }
      else
         warning = inactive = 0L;

      if(c == 0x0D)
         break;

      if((c == 0x08 || c == 0x7F) && (i>0)) {
         i--;
         if (i <= 0 || s[i-1] == ' ' || s[i-1] == '_' || s[i-1] == '\'' || s[i-1] == '(' || s[i-1] == '\\')
            upper = 1;
         else
            upper = 0;
         s[i]='\0';
         if (!local_mode) {
            SENDBYTE('\b');
            SENDBYTE(' ');
            SENDBYTE('\b');
         }
         if (snooping)
            wputs("\b \b");
         continue;
      }

      if (i >= width)
         continue;

      if (c < 0x20 || ((flag & INPUT_FANCY) && c == ' ' && upper) )
         continue;

      if (upper && (flag & INPUT_FANCY) && isalpha(c)) {
         c = toupper(c);
         upper = 0;
      }
      else if (flag & INPUT_FANCY)
         c = tolower(c);

      s[i++]=c;

      if(flag & INPUT_PWD) {
         if (!local_mode)
            SENDBYTE('*');
         if (snooping)
#ifdef __OS2__
            wputc('*');
            VioUpdate ();
#else
            wputc('*');
#endif
      }
      else {
         if (!local_mode)
            SENDBYTE(c);
         if (snooping)
#ifdef __OS2__
            wputc(c);
            VioUpdate ();
#else
            wputc(c);
#endif
      }

      if(c == ' ' || c == '_' || c == '\'' || c == '(')
         upper = 1;

      online_msg = timerset(200);

      if ( (flag & INPUT_HOT) ) {
         if ((!isdigit (c) || !num_hotkey) && i == 1)
            break;
      }
   }

   if ((flag & INPUT_FIELD) && usr.color)
      change_attr (LGREY|_BLACK);

   if (!stricmp (s, "rz")) {
      get_emsi_id (autozm, 8);
      if (!strncmp (autozm, "**B0000", 8)) {
         if (!sys.filepath[0])
            send_can ();
         else
            upload_file (NULL, 3);
      }
      s[0] = '\0';
   }

   if(!(flag & INPUT_HOT) && !(flag & INPUT_NOLF)) {
      if (!local_mode) {
         SENDBYTE('\r');
         SENDBYTE('\n');
      }
      if (snooping)
         wputs("\n");
   }

   s[i]='\0';
   if (flag & INPUT_FANCY)
      strtrim (s);
}

static int read_online_message()
{
   int rc, i, m;
   char filename[80], old_status;
   struct ffblk blk;

   if (!timeup(online_msg))
      return (0);

   sprintf (filename, "%sLEXIT*.*", config->sys_path);
   if (!findfirst (filename, &blk, 0))
      do {
         sscanf (blk.ff_name, "LEXIT%d.%d", &i, &m);
         if (i == line_offset || i == 0) {
            unlink (blk.ff_name);
            terminating_call ();
            get_down (m, 3);
         }
      } while (!findnext (&blk));

   sprintf(filename, ONLINE_MSGNAME, ipc_path, line_offset);
   if (dexists(filename) && user_status == BROWSING) {
      old_status = user_status;
      user_status = 0;

      read_file(filename);
      unlink(filename);

      user_status = old_status;
      rc = 1;
   }
   else
      rc = 0;

   online_msg = timerset(200);
   time_release ();

   return (rc);
}

