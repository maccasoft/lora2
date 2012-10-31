#include <stdio.h>
#include <dos.h>
#include <signal.h>
#include <string.h>
#include <process.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>

#include <alloc.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"

extern unsigned int _stklen = 10240;

static int posit = 0, old_event, outinfo = 0;
static char interpoint = ':', is_carrier = 0;
static long events, clocks, nocdto;

static int execute_events (void);
void initialize_modem (void);

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
   if (_OvrInitExt(0L, 81920L))
      _OvrInitEms(0, 0, 5);

   signal(SIGINT, SIG_IGN);
   signal(SIGABRT, SIG_IGN);
   signal(SIGTERM, SIG_IGN);
   ctrlbrk (ctrlchand);
   setcbrk (0);
   directvideo = 1;

   init_system();

   DTR_ON();

   parse_command_line(argc, argv);

   if (!parse_config())
           exit(1);

   parse_command_line(argc, argv);

   old_event = -1;
   logf = fopen(log_name, "at");

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

   remote_task();
}

void remote_task ()
{
   int i;
   char buffer[80];
   long to;
   struct tm *tim;

   if (frontdoor) {
      time (&to);
      tim = localtime (&to);
      fprintf (logf, "\n----------  %s %02d %s %02d, %s\n", wtext[tim->tm_wday], tim->tm_mday, mtext[tim->tm_mon], tim->tm_year % 100, VERSION);
   }
   else
      status_line(":Begin, %s, (task %d)", &VERSION[9], line_offset);

   system_crash();

   if (ext_mail_cmd && registered)
   {
      speed = 0;

      i = external_mailer (ext_mail_cmd);

      if (i == exit300)
         speed = 300;
      else if (i == exit1200)
         speed = 1200;
      else if (i == exit2400)
         speed = 2400;
      else if (i == exit9600)
         speed = 9600;
      else if (i == exit14400)
         speed = 14400;
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

   if (!local_mode && !caller)
   {
      status_window();
      f4_status();

      initialize_modem ();
      local_status(msgtxt[M_SETTING_BAUD]);
      display_outbound_info (outinfo);

      caller = 0;
      to = timerset (30000);

      do {
         if ((i = wait_for_connect(1)) == 1)
            break;

         time_release();

         if (!execute_events())
            local_status(msgtxt[M_SETTING_BAUD]);

         if (timeup(to))
         {
            if (!answer_flag)
            {
               local_status ("");
               initialize_modem();
               local_status(msgtxt[M_SETTING_BAUD]);
            }
            to = timerset (30000);
         }

         if (dexists ("RESCAN.NOW"))
         {
            get_call_list();
            outinfo = 0;
            display_outbound_info (outinfo);
            unlink ("RESCAN.NOW");
         }
      } while (!local_mode);

      if (local_mode)
      {
         rate = speed;
         local_status(msgtxt[M_EXT_MAIL]);
      }

      i = whandle();
      wclose();
      wunlink(i);
      wactiv(mainview);
   }

   if (local_mode)
   {
      status_line("#Connect Local");
      wputs("* Operating in local mode.\n\n");
      i = 0;
   }
   else
   {
      is_carrier = 1;

      if (mdm_flags == NULL)
      {
         status_line(msgtxt[M_READY_CONNECT],rate, "", "");
         sprintf(buffer, "* Incoming call at %u%s%s baud.\n", rate, "", "");
      }
      else
      {
         status_line(msgtxt[M_READY_CONNECT],rate, "/", mdm_flags);
         sprintf(buffer, "* Incoming call at %u%s%s baud.\n", rate, "/", mdm_flags);
      }
      wputs(buffer);
      if (!no_logins || !registered)
         i = mail_session();
      else
         i = 0;
   }

   caller = 1;

   if (!i)
   {
      showcur();

      read_sysinfo();
      load_language (0);
      text_path = lang_txtpath[0];

      if (login_user())
      {
         sprintf (buffer, "SEC%d", usr.priv);
         read_system_file (buffer);

         if (usr.scanmail)
            if (scan_mailbox())
            {
               mail_read_forward (0);
               menu_dispatcher("READMAIL");
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


void time_release (void)
{
   int sc, i, *varr;
   unsigned int ch;
   char *cmd, cmdname[128];
   struct time timep;

   if (kbhit())
   {
      ch = getch ();

      if (ch == 0)
      {
         ch = getch () * 0x100;
         if (locked && registered && password != NULL && ch != 0x2500)
            ch = -1;
      }
      else if (locked && registered && password != NULL && !local_mode)
      {
         if (ch == password[posit])
         {
            locked = (password[++posit] == '\0') ? 0 : 1;
            if (!locked && function_active == 4)
            {
               i = whandle();
               wactiv(status);
               wprints(1,44,BLACK|_LGREY,"      ");
               wactiv(i);
            }
         }
         else
            posit = 0;
      }

      switch (ch) {
      case 0x2500:
         if (!local_mode)
         {
            local_mode = 1;
            local_kbd = -1;
         }
         break;

      case 0x2D00:
         if (!caller && !local_mode)
         {
            local_status("Taking modem off-hook");

            status_line(msgtxt[M_EXIT_REQUEST]);
            get_down(-1, 0);
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
         showcur();
         cclrscrn(LGREY|_BLACK);
         cmd = getenv ("COMSPEC");
         strcpy (cmdname, (cmd == NULL) ? "command.com" : cmd);
         status_line(msgtxt[M_SHELLING]);
         printf(msgtxt[M_TYPE_EXIT]);
         fclose (logf);
         spawnl (P_WAIT, cmdname, cmdname, NULL);
         logf = fopen(log_name, "at");
         status_line(msgtxt[M_BINKLEY_BACK]);
         srestore (varr);

         if (caller)
            read_system_file ("SHELLHI");
         if (!local_mode)
            hidecur();
         if (!caller)
            get_call_list();
         break;

      case 0x2300:
         if (caller)
         {
            hidecur();
            terminating_call();
            get_down(aftercaller_exit, 2);
         }
         else if (CARRIER)
            modem_hangup ();

         break;

      case 0x2600:
         if (caller)
         {
            hidecur();
            usr.priv = 0;
            terminating_call();
            get_down(aftercaller_exit, 2);
         }

         break;

      case 0x3100:
         if (caller && !local_mode)
            usr.nerd ^= 1;

         break;

      case 0x1000:
         if (!answer_flag && !CARRIER)
         {
            local_status ("");
            initialize_modem ();
            local_status(msgtxt[M_SETTING_BAUD]);
         }
         break;

      case 0x1900:
         if (!caller && !local_mode)
         {
            keyboard_password ();
            if (password != NULL)
               posit = 0;
         }
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
         if (caller)
         {
            allowed += 1;
            if (function_active == 1)
               f1_status ();
         }
         else
         {
            if (outinfo > 0)
            {
               outinfo--;
               display_outbound_info (outinfo);
            }
         }
         break;

      case 0x5000:
         if (caller)
         {
            allowed -= 1;
            if (function_active == 1)
               f1_status ();
         }
         else
         {
            if ( (outinfo + 4) < max_call)
            {
               outinfo++;
               display_outbound_info (outinfo);
            }
         }
         break;

      case 0x6800:
      case 0x6900:
      case 0x6A00:
      case 0x6B00:
      case 0x6C00:
      case 0x6D00:
      case 0x6E00:
      case 0x6F00:
      case 0x7000:
      case 0x7100:
         if (!caller && !local_mode && !CARRIER)
         {
            i = (int) (((unsigned) ch) >> 8);
            status_line (msgtxt[M_FUNCTION_KEY], (i - 0x67) * 10);

            get_down ((i - 0x67) * 10, 3);
         }
         break;

      default:
         if (ch < 128 || ch == 0x2E00)
            local_kbd = ch;
         break;
      }

      return;
   }

   if (timeup(clocks))
   {
      clocks = timerset(100);
      gettime((struct time *)&timep);

      hidecur();
      i = whandle();
      wactiv(status);

      sprintf(cmdname, "%02d%c%02d", timep.ti_hour % 24, interpoint, timep.ti_min % 60);
      wprints(0,73,BLACK|_LGREY,cmdname);
      interpoint = (interpoint == ':') ? ' ' : ':';

      if (caller && function_active == 1)
      {
         sc = time_remain ();
         sprintf(cmdname, "%d mins ", sc);
         wprints(1,26,BLACK|_LGREY,cmdname);
      }

      if (is_carrier && !CARRIER)
      {
         if (nocdto != 0L)
         {
            if (timeup (nocdto))
            {
               nocdto = 0L;
               if (caller)
                  update_user ();
               get_down(aftercaller_exit, 2);
            }
         }
         else
            nocdto = timerset (800);
      }

      if ( (!CARRIER || caller) && function_active == 4 )
      {
         sc = time_to_next (0);
         if (old_event != cur_event)
         {
            wgotoxy(1,1);
            wdupc(' ', 40);
            old_event = cur_event;
         }
         if (next_event >= 0)
         {
            sprintf(cmdname, msgtxt[M_NEXT_EVENT], next_event + 1, sc / 60, sc % 60);
            wprints(1,1,BLACK|_LGREY,cmdname);
         }
         else
            wprints(1,1,BLACK|_LGREY,msgtxt[M_NONE_EVENTS]);
      }

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

   if (events == 0L)
   {
      find_event ();

      if (cur_event >= 0 && !(e_ptrs[cur_event]->behavior & MAT_NOOUT))
         events = timerset (e_ptrs[cur_event]->wait_time * 100);
      else if (cur_event < 0)
         events = timerset (500);
   }

   if (timeup(events) && events > 0L)
   {
      events = 0L;

      if (next_call >= max_call)
              next_call = 0;

      for (;next_call < max_call; next_call++)
      {
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
         else
         {
            i = poll(1, 1, call_list[next_call].zone,
                           call_list[next_call].net,
                           call_list[next_call].node);
            break;
         }
      }

      if (next_call >= max_call && i)
      {
         get_call_list();
         next_call = 0;

         for (;next_call < max_call; next_call++)
         {
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
            else
            {
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

   local_status ("Initializing the modem");

   to = timerset(500);
   answer_flag = 0;
   mdm_sendcmd(init);

   while (modem_response() != 0)
   {
      if (timeup(to))
      {
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


