#include <stdio.h>
#include <dos.h>
#include <signal.h>
#include <string.h>
#include <process.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>
#include <dir.h>
#include <time.h>

#include <alloc.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "exec.h"

extern unsigned int _stklen = 14336U;
extern int blank_timer;

int blanked, outinfo = 0;
static int posit = 0, old_event;
static char interpoint = ':', is_carrier = 0;
static long events, clocks, nocdto, blankto, to, verfile;

#define MAX_STAR  50

void begin_blanking (void);
void stop_blanking (void);
void blank_progress (void);

static int execute_events (void);
void initialize_modem (void);
void process_startup_mail (int);

static char *wtext [] = {
   "Sun",
   "Mon",
   "Tue",
   "Wed",
   "Thu",
   "Fri",
   "Sat"
};

static int ctrlchand (void)
{
   return (1);
}
     
void main (argc, argv)
int argc;
char **argv;
{
   signal (SIGINT, SIG_IGN);
   signal (SIGABRT, SIG_IGN);
   signal (SIGTERM, SIG_IGN);
   ctrlbrk (ctrlchand);
   setcbrk (0);
   directvideo = 1;
   tzset ();

//   _OvrInitEms (0, 0, 16);
   init_system();

   DTR_ON();

   parse_command_line(argc, argv);

   cclrscrn(LGREY|_BLACK);
   if (!parse_config())
           exit(1);

   parse_command_line(argc, argv);

   old_event = -1;
   logf = fopen(log_name, "at");
   setbuf(logf, NULL);

   activation_key();
   if (!registered)
      line_offset = 1;

   read_sysinfo();
   read_sched();
   get_call_list();
   get_last_caller();

   setup_screen();
   firing_up();
   mtask_find();

   events = 0L;
   clocks = 0L;
   nocdto = 0L;
   verfile = 0L;

   if (caller && remote_net && remote_node)
      poll(1, 1, remote_zone, remote_net, remote_node);
   else
      remote_task();

   exit (1);
}

void remote_task ()
{
   int i, m;
   char buffer[80];
   long ansto = 0L;
   struct tm *tim;
   struct ffblk blk;

   if (frontdoor) {
      time (&to);
      tim = localtime (&to);
      fprintf (logf, "\n----------  %s %02d %s %02d, %s\n", wtext[tim->tm_wday], tim->tm_mday, mtext[tim->tm_mon], tim->tm_year % 100, VERSION);
   }
   else
      status_line(":Begin, %s, (task %d)", &VERSION[9], line_offset);

   system_crash();
   f4_status ();

   if (ext_mail_cmd && registered == 1) {
      speed = 0;

      i = external_mailer (ext_mail_cmd);

      if (i == exit300)
         speed = 300;
      else if (i == exit1200)
         speed = 1200;
      else if (i == exit2400)
         speed = 2400;
      else if (i == exit4800)
         speed = 4800;
      else if (i == exit7200)
         speed = 7200;
      else if (i == exit9600)
         speed = 9600;
      else if (i == exit12000)
         speed = 12000;
      else if (i == exit14400)
         speed = 14400;
      else if (i == exit16800)
         speed = 16800;
      else if (i == exit19200)
         speed = 19200;
      else if (i == exit38400)
         speed = 38400U;

      if (speed)
         caller = 1;
      else
         get_down (i, 3);
   }

   rate = speed;
   com_baud (speed);

   if (!local_mode && !caller) {
      status_window();
      f4_status();

      initialize_modem ();
      local_status(msgtxt[M_SETTING_BAUD]);
      display_outbound_info (outinfo);

      caller = 0;
      to = timerset (30000);
      blankto = timerset (0);
      blankto += blank_timer * 6000L;

      for (i = 0; i < num_events; i++)
         e_ptrs[i]->behavior &= ~MAT_SKIP;

      do {
         if (answer_flag) {
            if ((i = wait_for_connect(1)) == 1)
               break;
            if (!answer_flag)
               local_status(msgtxt[M_SETTING_BAUD]);
            else if (timeup (ansto)) {
               answer_flag = 0;
               status_line ("!Answer timeout");
               initialize_modem();
               local_status(msgtxt[M_SETTING_BAUD]);
               to = timerset (30000);
            }
         }
         else if (registered == 1) {
            if ((i = wait_for_connect(1)) == 1)
               break;
            if (answer_flag)
               ansto = timerset (6000);
         }

         if (timeup (verfile)) {
            if (registered && dexists ("RESCAN.NOW")) {
               if (blanked)
                  stop_blanking ();
               get_call_list();
               outinfo = 0;
               display_outbound_info (outinfo);
               unlink ("RESCAN.NOW");
               blankto = timerset (0);
               blankto += blank_timer * 6000L;
            }

            if (registered && (dexists ("ECHOMAIL.RSN") || dexists ("NETMAIL.RSN"))) {
               if (blanked)
                  stop_blanking ();
               if (modem_busy != NULL)
                  mdm_sendcmd (modem_busy);

               i = whandle();
               wclose();
               wunlink(i);
               wactiv(mainview);

               if (dexists ("NETMAIL.RSN")) {
                  export_mail (NETMAIL_RSN|ECHOMAIL_RSN);
                  unlink ("NETMAIL.RSN");
               }
               else
                  export_mail (ECHOMAIL_RSN);
               unlink ("ECHOMAIL.RSN");

               if (cur_event >= 0 && !(e_ptrs[cur_event]->behavior & MAT_NOOUT))
                  events = timerset (e_ptrs[cur_event]->wait_time * 100);
               else if (cur_event < 0)
                  events = timerset (500);

               status_window();
               f4_status();
               local_status(msgtxt[M_SETTING_BAUD]);
               get_call_list();
               outinfo = 0;
               unlink ("RESCAN.NOW");
               display_outbound_info (outinfo);
               events = 0L;
               clocks = 0L;
               nocdto = 0L;
               to = 0L;

               blankto = timerset (0);
               blankto += blank_timer * 6000L;
            }

            if (registered) {
               sprintf (buffer, "%sLEXIT*.*", sys_path);
               if (!findfirst (buffer, &blk, 0))
                  do {
                     sscanf (blk.ff_name, "LEXIT%d.%d", &i, &m);
                     if (i == line_offset || i == 0) {
                        unlink (blk.ff_name);
                        get_down (m, 3);
                     }
                  } while (!findnext (&blk));
            }

            if (!execute_events())
               local_status(msgtxt[M_SETTING_BAUD]);

            verfile = timerset (1000);
         }

         else if (timeup(to)) {
            if (!answer_flag) {
               if (!blanked)
                  local_status ("");
               initialize_modem();
               if (!blanked)
                  local_status(msgtxt[M_SETTING_BAUD]);
            }
            to = timerset (30000);
         }

         else if (timeup (blankto) && blank_timer) {
            if (answer_flag) {
               blankto = timerset (0);
               blankto += blank_timer * 6000L;
            }
            else {
               if (!blanked)
                  begin_blanking ();
               else
                  blank_progress ();
               blankto = timerset (20);
               time_release ();
            }
         }

         else
            time_release ();

      } while (!local_mode);

      if (local_mode) {
         rate = speed;
         local_status(msgtxt[M_EXT_MAIL]);
      }

      i = whandle();
      wclose();
      wunlink(i);
      wactiv(mainview);
   }

   if (local_mode) {
      status_line("#Connect Local");
      wputs("* Operating in local mode.\n\n");
      i = 0;
   }
   else {
      is_carrier = 1;

      if (mdm_flags == NULL) {
         status_line(msgtxt[M_READY_CONNECT],rate, "", "");
         sprintf(buffer, "* Incoming call at %u%s%s baud.\n", rate, "", "");
      }
      else {
         status_line(msgtxt[M_READY_CONNECT],rate, "/", mdm_flags);
         sprintf(buffer, "* Incoming call at %u%s%s baud.\n", rate, "/", mdm_flags);
      }
      wputs(buffer);

      if (!no_logins || !registered || (cur_event >= 0 && (e_ptrs[cur_event]->behavior & MAT_OUTONLY)))
         i = mail_session();
      else
         i = 0;
   }

   caller = 1;

   if (!i) {
      showcur();

      read_sysinfo();
      load_language (0);
      text_path = lang_txtpath[0];

      if (login_user()) {
         sprintf (buffer, "SEC%d", usr.priv);
         if (!read_system_file (buffer))
            read_system_file ("SECALL");

         if (usr.scanmail)
            if (scan_mailbox()) {
               mail_read_forward (0);
               menu_dispatcher("READMAIL");
            }

         if (!noask.checkfile) {
            m_print (bbstxt[B_CHECK_NEW_FILES]);
            if (yesno_question (DEF_NO))
               new_file_list (2);
         }

         read_system (usr.msg, 1);
         read_system (usr.files, 2);

         read_system_file("NEWS");

         menu_dispatcher(NULL);
      }
   }
   else
      caller = 0;

   hidecur();

   if (!(MODEM_STATUS() & 0x80) && !local_mode)
      status_line(msgtxt[M_NO_CARRIER]);

   terminating_call();

   get_down(aftercaller_exit, 2);
}

void resume_blanked_screen ()
{
   if (blanked)
      stop_blanking ();
   else {
      blankto = timerset (0);
      blankto += blank_timer * 6000L;
   }
}

void time_release (void)
{
   int sc, i, *varr;
   unsigned int ch;
   char *cmd, cmdname[128], cpath[80];
   struct time timep;

   if (kbhit()) {
      resume_blanked_screen ();
      ch = getch ();

      if (ch == 0) {
         ch = getch () * 0x100;
         if (locked && registered && password != NULL && ch != 0x2500)
            ch = -1;
      }
      else if (locked && registered && password != NULL && !local_mode) {
         if (ch == password[posit]) {
            locked = (password[++posit] == '\0') ? 0 : 1;
            if (!locked && function_active == 4) {
               i = whandle();
               wactiv(status);
               wprints(1,37,BLACK|_LGREY,"      ");
               wactiv(i);
            }
         }
         else
            posit = 0;
      }

      switch (ch) {
      case 0x1300:
         if (!caller && !local_mode && galileo != NULL) {
            poll_galileo (1, 1);
            status_window ();
            initialize_modem ();
            local_status(msgtxt[M_SETTING_BAUD]);
            display_outbound_info (outinfo);
         }
         break;

      case 0x2500:
         if (!local_mode) {
            local_mode = 1;
            local_kbd = -1;
         }
         break;

      case 0x2D00:
         if (!caller && !local_mode) {
            local_status("Taking modem off-hook");

            status_line(msgtxt[M_EXIT_REQUEST]);
            get_down(-1, 3);
         }
         break;

      case 0x2000:
         if (local_mode)
            sysop_error();
         else if (caller)
            snooping = snooping ? 0 : 1;
         break;

      case 0x2400:
         if (!caller && !local_mode && CARRIER)
            break;
         if ((varr = ssave ()) == NULL)
            break;
         if (caller)
            read_system_file ("SHELLBYE");
         else if (!local_mode && modem_busy != NULL)
            mdm_sendcmd (modem_busy);
         showcur();
         cclrscrn(LGREY|_BLACK);
         getcwd (cpath, 79);
         cmd = getenv ("COMSPEC");
         strcpy (cmdname, (cmd == NULL) ? "command.com" : cmd);
         status_line(msgtxt[M_SHELLING]);
         fclose (logf);

         printf (msgtxt[M_TYPE_EXIT], farcoreleft ());

         if (!registered)
            spawnl (P_WAIT, cmdname, cmdname, NULL);
         else
            do_exec (cmdname, "", USE_ALL|HIDE_FILE, 0xFFFF, NULL);
         setdisk (cpath[0] - 'A');
         chdir (cpath);
         logf = fopen(log_name, "at");
         setbuf(logf, NULL);
         status_line(msgtxt[M_BINKLEY_BACK]);
         srestore (varr);

         if (!local_mode)
            hidecur();
         if (!caller) {
            get_call_list();
            outinfo = 0;
            unlink ("RESCAN.NOW");
            display_outbound_info (outinfo);
            events = 0L;
            clocks = 0L;
            nocdto = 0L;
            to = 0L;
            blankto = timerset (0);
            blankto += blank_timer * 6000L;
         }
         else
            read_system_file ("SHELLHI");
         break;

      case 0x2300:
         if (caller) {
            hidecur();
            terminating_call();
            get_down(aftercaller_exit, 2);
         }
         else if (CARRIER)
            modem_hangup ();

         break;

      case 0x2600:
         if (caller) {
            hidecur();
            usr.priv = 0;
            terminating_call();
            get_down(aftercaller_exit, 2);
         }

         break;

      case 0x3100:                        /* ALT-N - Toggle NERD flag */
         if (caller && !local_mode)
            usr.nerd ^= 1;

         break;

      case 0x1000:                        /* ALT-Q - Init Modem / Rescan Outbound */
         if (!answer_flag && !CARRIER) {
            local_status ("");
            initialize_modem ();
            local_status(msgtxt[M_SETTING_BAUD]);
            get_call_list();
            unlink ("RESCAN.NOW");
            outinfo = 0;
            display_outbound_info (outinfo);
         }
         break;

      case 0x1900:                        /* ALT-P - Keyboard Password */
         if (!caller && !local_mode) {
            keyboard_password ();
            if (password != NULL)
               posit = 0;
         }
         break;

      case 0x1200:                        /* ALT-E - External Editor */
         if (!caller && !local_mode) {
            if (local_editor != NULL) {
               if (modem_busy != NULL)
                  mdm_sendcmd (modem_busy);
               varr = ssave ();
               cclrscrn(LGREY|_BLACK);
               if ((cmd = strchr (local_editor, ' ')) != NULL)
                  *cmd++ = '\0';
               do_exec (local_editor, cmd, USE_ALL|HIDE_FILE, 0xFFFF, NULL);
               if (cmd != NULL)
                  *(--cmd) = ' ';
               if (varr != NULL)
                  srestore (varr);
               if (cur_event >= 0 && !(e_ptrs[cur_event]->behavior & MAT_NOOUT))
                  events = timerset (e_ptrs[cur_event]->wait_time * 100);
               else if (cur_event < 0)
                  events = timerset (500);
               clocks = 0L;
               nocdto = 0L;
               to = 0L;
               blankto = timerset (0);
               blankto += blank_timer * 6000L;
            }
         }
         break;

      case 0x1700:                        /* ALT-I - Import Mail */
         if (!caller && !local_mode)
            process_startup_mail (1);
         break;

      case 0x1F00:
         if (caller)
            set_security ();
         break;

      case 0x3200:
         if (!caller && !local_mode)
            manual_poll ();
         break;

      case 0x3B00:
         if (caller)
            f1_status();
         break;

      case 0x3C00:
         if (caller)
            f2_status();
         break;

      case 0x3D00:
         if (caller)
            f3_status();
         break;

      case 0x3E00:
         f4_status();
         break;

      case 0x4300:
         f9_status();
         break;

      case 0x4800:
         if (caller) {
            allowed += 1;
            if (function_active == 1)
               f1_status ();
         }
         else {
            if (outinfo > 0) {
               outinfo--;
               display_outbound_info (outinfo);
            }
         }
         break;

      case 0x5000:
         if (caller) {
            allowed -= 1;
            if (function_active == 1)
               f1_status ();
         }
         else {
            if ( (outinfo + 3) < max_call) {
               outinfo++;
               display_outbound_info (outinfo);
            }
         }
         break;

      case 0x6800:                        /* ALT-F1  */
      case 0x6900:                        /* ALT-F2  */
      case 0x6A00:                        /* ALT-F3  */
      case 0x6B00:                        /* ALT-F4  */
      case 0x6C00:                        /* ALT-F5  */
      case 0x6D00:                        /* ALT-F6  */
      case 0x6E00:                        /* ALT-F7  */
      case 0x6F00:                        /* ALT-F8  */
      case 0x7000:                        /* ALT-F9  */
      case 0x7100:                        /* ALT-F10 */
         if (!caller && !local_mode && !CARRIER && !answer_flag) {
            i = (int) (((unsigned) ch) >> 8);
            status_line (msgtxt[M_FUNCTION_KEY], (i - 0x67) * 10);

            get_down ((i - 0x67) * 10, 3);
         }
         else {
            i = (int) (((unsigned) ch) >> 8);
            sprintf (cmdname, "ALTF%d", (i - 0x67) * 10);
            read_system_file (cmdname);
         }
         break;

      default:
         if (ch < 128 || ch == 0x2E00)
            local_kbd = ch;
         break;
      }

      return;
   }

   if (timeup(clocks)) {
      clocks = timerset(100);
      gettime((struct time *)&timep);

      if (!blanked) {
         hidecur();
         i = whandle();
         wactiv(status);

         sprintf(cmdname, "%02d%c%02d", timep.ti_hour % 24, interpoint, timep.ti_min % 60);
         wprints(0,73,BLACK|_LGREY,cmdname);
         interpoint = (interpoint == ':') ? ' ' : ':';

         if (caller && function_active == 1) {
            sc = time_remain ();
            sprintf(cmdname, "%d mins ", sc);
            wprints(1,26,BLACK|_LGREY,cmdname);
         }
      }

      if (is_carrier && !CARRIER) {
         if (nocdto != 0L) {
            if (timeup (nocdto)) {
               nocdto = 0L;
               if (caller)
                  update_user ();
               get_down(aftercaller_exit, 2);
            }
         }
         else
            nocdto = timerset (800);
      }

      if ( (!CARRIER || caller) && function_active == 4 ) {
         sc = time_to_next (0);
         if (old_event != cur_event && !blanked) {
            wgotoxy(1,1);
            wdupc(' ', 34);
            old_event = cur_event;
         }
         if (next_event >= 0 && !blanked) {
            sprintf(cmdname, msgtxt[M_NEXT_EVENT], next_event + 1, sc / 60, sc % 60);
            wprints(1,1,BLACK|_LGREY,cmdname);
         }
         else if (!blanked)
            wprints(1,1,BLACK|_LGREY,msgtxt[M_NONE_EVENTS]);
      }

      if (!blanked)
         wactiv(i);
      if (caller)
         showcur();
   }

   if (have_dv)
      dv_pause ();
   else if (have_ddos)
      ddos_pause ();
   else if (have_tv)
      tv_pause ();
   else if (have_ml)
      ml_pause ();
   else
      msdos_pause ();
}

static int execute_events()
{
   int i;

   i = 1;

   if (events == 0L) {
      find_event ();

      if (cur_event >= 0 && !(e_ptrs[cur_event]->behavior & MAT_NOOUT))
         events = timerset (e_ptrs[cur_event]->wait_time * 100);
      else if (cur_event < 0)
         events = timerset (500);
   }

   if (timeup(events) && events > 0L) {
      events = 0L;

      if (next_call >= max_call)
              next_call = 0;

      for (;next_call < max_call; next_call++) {
         if (answer_flag)
            continue;
         if ((call_list[next_call].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_NOCM))
            continue;
         if (!(call_list[next_call].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_CM))
            continue;
         if (!(call_list[next_call].type & (MAIL_CRASH|MAIL_DIRECT|MAIL_NORMAL)))
            continue;
         if (e_ptrs[cur_event]->res_net && (call_list[next_call].net != e_ptrs[cur_event]->res_net ||
                                            call_list[next_call].node != e_ptrs[cur_event]->res_node))
            continue;

         if (flag_file (TEST_FLAG,
                        alias[0].zone,
                        call_list[next_call].net,
                        call_list[next_call].node,
                        0))
            continue;

         if (bad_call(call_list[next_call].net,call_list[next_call].node,0))
            continue;
         else {
            if (blanked)
               stop_blanking ();
            i = poll(1, 1, call_list[next_call].zone,
                           call_list[next_call].net,
                           call_list[next_call].node);
            break;
         }
      }

      if (next_call >= max_call && i) {
         get_call_list();
         next_call = 0;

         for (;next_call < max_call; next_call++) {
            if (answer_flag)
               continue;
            if ((call_list[next_call].type & MAIL_CRASH) && !(e_ptrs[cur_event]->behavior & MAT_NOCM))
               continue;
            if (!(call_list[next_call].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_CM))
               continue;
            if (!(call_list[next_call].type & (MAIL_CRASH|MAIL_DIRECT|MAIL_NORMAL)))
               continue;
            if (e_ptrs[cur_event]->res_net && (call_list[next_call].net != e_ptrs[cur_event]->res_net ||
                                               call_list[next_call].node != e_ptrs[cur_event]->res_node))
               continue;

            if (flag_file (TEST_FLAG,
                           alias[0].zone,
                           call_list[next_call].net,
                           call_list[next_call].node,
                           0))
               continue;

            if (bad_call(call_list[next_call].net,call_list[next_call].node,0))
               continue;
            else {
               if (blanked)
                  stop_blanking ();
               i = poll(1, 1, call_list[next_call].zone,
                              call_list[next_call].net,
                              call_list[next_call].node);
               break;
            }
         }
      }

      next_call++;
   }

   return (i);
}

void initialize_modem ()
{
   int fail = 0;
   long to;

   if (terminal)
      return;

   if (!blanked)
      local_status ("Initializing the modem");

   to = timerset(500);
   answer_flag = 0;
   mdm_sendcmd(init);

   while (modem_response() != 0) {
      if (timeup(to)) {
         if (!blanked)
            local_status ("Initialize failure #%d", ++fail);
         if (fail > 3)
            get_down (-1, 255);
         CLEAR_OUTBOUND();
         CLEAR_INBOUND();
         mdm_sendcmd(init);
         to = timerset(500);
      }
   }
}

void process_startup_mail (import)
int import;
{
   int i;

   if (blanked)
      stop_blanking ();

   if (modem_busy != NULL)
      mdm_sendcmd (modem_busy);

   i = whandle();
   wclose();
   wunlink(i);
   wactiv(mainview);

   if (import) {
      if ( cur_event > -1 && (e_ptrs[cur_event]->echomail & (ECHO_PROT|ECHO_KNOW|ECHO_NORMAL)) ) {
         i = 1;
         if (prot_filepath != NULL && (e_ptrs[cur_event]->echomail & ECHO_PROT))
            import_mail (prot_filepath, &i);
         if (know_filepath != NULL && (e_ptrs[cur_event]->echomail & ECHO_KNOW))
            import_mail (know_filepath, &i);
         if (norm_filepath != NULL && (e_ptrs[cur_event]->echomail & ECHO_NORMAL))
            import_mail (norm_filepath, &i);
         i = 2;
         import_mail (".\\", &i);
      }
      else {
         i = 1;
         if (prot_filepath != NULL)
            import_mail (prot_filepath, &i);
         if (know_filepath != NULL)
            import_mail (know_filepath, &i);
         if (norm_filepath != NULL)
            import_mail (norm_filepath, &i);
         i = 2;
         import_mail (".\\", &i);
      }
   }

   export_mail (ECHOMAIL_RSN|NETMAIL_RSN);

   status_window();
   f4_status();
   initialize_modem ();
   local_status(msgtxt[M_SETTING_BAUD]);
   get_call_list();
   outinfo = 0;
   unlink ("RESCAN.NOW");
   unlink ("ECHOMAIL.RSN");
   unlink ("NETMAIL.RSN");
   display_outbound_info (outinfo);
   events = 0L;
   clocks = 0L;
   nocdto = 0L;
   blankto = timerset (0);
   blankto += blank_timer * 6000L;
}

struct _star {
   char x;
   char y;
   char magnitude;
};

static struct _star *star;
static int *scrsv;

static void begin_blanking ()
{
   int i, m;

   randomize ();
   scrsv = ssave ();
   wtextattr (LGREY|_BLACK);
   cclrscrn (LGREY|_BLACK);

   star = (struct _star *)malloc (sizeof (struct _star) * MAX_STAR);

   for (i = 0; i < MAX_STAR; i++) {
      do {
         star[i].x = random (80);
         star[i].y = random (25);
         for (m = 0; m < i; m++)
            if (star[i].x == star[m].x && star[i].y == star[m].y)
               break;
      } while (m < i);
      star[i].magnitude = 0;
      pokeb (0xB800, star[i].y * 160 + star[i].x * 2, 249);
      pokeb (0xB800, star[i].y * 160 + star[i].x * 2 + 1, 3);
   }

   blanked = 1;
}

static void stop_blanking ()
{
   if (blanked) {
      clrscr ();
      if (scrsv != NULL)
         srestore (scrsv);
      free (star);
      blanked = 0;
      blankto = timerset (0);
      blankto += blank_timer * 6000L;
   }
}

static void blank_progress ()
{
   int m, i, count;
   char c;

   if (!blanked)
      return;

   count = 0;

   for (i = 0; i < MAX_STAR; i++) {
      if (star[i].magnitude > 0) {
         switch (star[i].magnitude) {
            case 0:
               c = 249;
               break;
            case 1:
               c = 7;
               break;
            case 2:
               c = 4;
               break;
            case 3:
               c = 42;
               break;
            case 4:
               c = 15;
               break;
         }
         pokeb (0xB800, star[i].y * 160 + star[i].x * 2, c);
         if ((unsigned)c == 249)
            pokeb (0xB800, star[i].y * 160 + star[i].x * 2 + 1, 3);
         else
            pokeb (0xB800, star[i].y * 160 + star[i].x * 2 + 1, 15);
         star[i].magnitude++;
         if (star[i].magnitude >= 5) {
            pokeb (0xB800, star[i].y * 160 + star[i].x * 2, ' ');
            do {
               star[i].x = random (80);
               star[i].y = random (25);
               for (m = 0; m < i; m++)
                  if (star[i].x == star[m].x && star[i].y == star[m].y)
                     break;
            } while (m < i);
            star[i].magnitude = 0;
            pokeb (0xB800, star[i].y * 160 + star[i].x * 2, 249);
            pokeb (0xB800, star[i].y * 160 + star[i].x * 2 + 1, 3);
         }

         count++;
      }
   }

   for (i = count; i < 4; i++) {
      i = random (MAX_STAR);
      star[i].magnitude++;
   }
}
