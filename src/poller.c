#include <stdio.h>
#include <stdlib.h>
#include <dir.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <time.h>
#include <sys/stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"

#define IEMSI_ON        0x0001
#define IEMSI_MAILCHECK 0x0002
#define IEMSI_FILECHECK 0x0004
#define IEMSI_NEWS      0x0008

extern long timeout;
extern int to_row, iemsi;
extern word serial_no;
extern char *VNUM, serial_id[3];

static void galileo_input (char *, int);

void m_print2 (char *, ...);
void set_protocol_flags (int zone, int net, int node, int point);
int esc_pressed (void);
int do_script (char *phone_number);
void emsi_handshake (int);
void get_emsi_id (char *, int);
int time_to_next_nonmail (void);
void iemsi_handshake (void);
int get_emsi_field (char *);

int poll(max_tries, bad, zone, net, node)
int max_tries, bad, zone, net, node;
{
   int fd, tries, i, j, wh, m;
   char buffer[70], jan, waz, ems;
   long t1, t2, tu;
   NODEINFO ni;

   if (CARRIER) {
      dial_system ();

      for (i = 0; i < max_call; i++) {
         if (zone == call_list[i].zone && net == call_list[i].net && node == call_list[i].node && !call_list[i].point)
            break;
      }

      if (i >= max_call) {
        call_list[i].zone = zone;
        call_list[i].net = net;
        call_list[i].node = node;
        call_list[i].type |= MAIL_WILLGO;
        max_call++;
      }

      next_call = i;
      goto online_out;
   }

   if (!get_bbs_record(zone, net, node, 0))
      return (0);

   if (bad && bad_call(net,node,0,0))
      return (0);

   jan = config->janus;
   waz = config->wazoo;
   ems = config->emsi;
   set_protocol_flags (zone, net, node, 0);

   function_active = 0;
   dial_system ();
   local_status ("Dialing");
   sysinfo.today.outcalls++;
   sysinfo.week.outcalls++;
   sysinfo.month.outcalls++;
   sysinfo.year.outcalls++;

//   for (i=0; i < MAX_ALIAS; i++)
//      if (zone && config->alias[i].zone == zone)
//         break;
//   if (i == MAX_ALIAS)
//      assumed = 0;
//   else
//      assumed = i;

//   for (i = 0; i < MAX_ALIAS; i++)
//      if ( config->alias[i].point && config->alias[i].net == net && config->alias[i].node == node && (!remote_zone || config->alias[i].zone == zone) )
//         break;

//   if (i < MAX_ALIAS)
//      assumed = i;

   assumed = 0;

   sprintf (buffer, "%sNODES.DAT", config->net_info);
   if ((fd = sh_open (buffer, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) == -1)
      return (0);

   while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO))
      if (zone == ni.zone && net == ni.net && node == ni.node && ni.point == 0) {
         assumed = ni.mailer_aka;
         break;
      }

   close (fd);

   for (tries = 0; tries < max_tries; tries++) {
      if (tries) {
         modem_hangup ();

         tu = timerset (300);
         while (!timeup (tu)) {
            if (local_kbd == 0x1B) {
               local_kbd = -1;
               i = 0;
               modem_hangup ();
               break;
            }
         }
      }

      status_line(msgtxt[M_PROCESSING_NODE],zone,net,node,nodelist.name);
      status_line(msgtxt[M_DIALING_NUMBER],nodelist.phone);

      wh = wopen (13, 9, 20, 45, 3, RED|_LGREY, BLUE|_LGREY);
      wactiv (wh);
      wtitle (" Dialing ", TCENTER, RED|_LGREY);

      sprintf (buffer, "%d:%d/%d", zone, net, node);
      wcenters (1, BLUE|_LGREY, buffer);
      wcenters (2, BLUE|_LGREY, nodelist.name);
      wcenters (3, BLUE|_LGREY, nodelist.phone);

      CLEAR_INBOUND();
      CLEAR_OUTBOUND();

      i = nodelist.rate * 300;
      if ((long)i > speed)
         i = (word)speed;
      if (!config->lock_baud)
         com_baud ((long)i);
      rate = (long)i;
      answer_flag = 1;

      sprintf (buffer, "%ld Baud", rate);
      wcenters (4, BLUE|_LGREY, buffer);

      if (nodelist.phone[0] == '\"') {
         prints (7, 65, YELLOW|_BLACK, "ScriptExec");
         i = do_script (nodelist.phone);
      }
      else {
         prints (7, 65, YELLOW|_BLACK, "DialPhone");
         dial_number (nodelist.modem, nodelist.phone);
         prints (7, 65, YELLOW|_BLACK, "Waiting  ");
         i = wait_for_connect(0);
      }

      wclose ();

      if (i == ABORTED)
         status_line ("!Aborted by operator");
      else if (i == TIMEDOUT)
         status_line ("!Modem timeout");

      if (i > 0)
         break;
      else {
         prints (7, 65, YELLOW|_BLACK, "Hangup    ");
         modem_hangup ();
         prints (7, 65, YELLOW|_BLACK, "Modem init");
         initialize_modem ();
         if (local_kbd == 0x1B || local_kbd == ' ') {
            local_kbd = -1;
            break;
         }
      }
   }

   if (i <= 0) {
      if (bad)
         bad_call (net, node, 2, i);

      rate = config->speed;
      if (!config->lock_baud)
         com_baud (rate);
      answer_flag = 0;

      config->janus = jan;
      config->wazoo = waz;
      config->emsi = ems;

      idle_system ();
      timeout = 0L;
      function_active = 99;
      return (i);
   }

online_out:
//   set_prior (4);
   local_status ("Online    ");

   if (mdm_flags == NULL) {
      status_line(msgtxt[M_READY_CONNECT], rate, "", "");
      mdm_flags = "";
   }
   else
      status_line(msgtxt[M_READY_CONNECT], rate, "/", mdm_flags);

   if (!lock_baud)
      com_baud(rate);

   remote_zone = called_zone = zone;
   remote_node = called_node = node;
   remote_net = called_net = net;
   remote_point = 0;
   remote_capabilities = 0;
   local_mode = snooping = 0;

   filepath = config->norm_filepath;
   request_list = config->norm_okfile;
   max_requests = config->norm_max_requests;
   max_kbytes = config->norm_max_kbytes;

   prints (7, 65, YELLOW|_BLACK, "Sync.  ");

   t1 = timerset(3000);
   timeout = t1;
   to_row = 8;
   j = 'j';

   while(!timeup(t1) && CARRIER) {
      SENDBYTE (32);
      SENDBYTE (13);
      SENDBYTE (32);
      SENDBYTE (13);

      while (CARRIER && !OUT_EMPTY ())
         time_release ();

      t2 = timerset(300);
      while (CARRIER && !timeup (t2) && !timeup (t1)) {
         if (esc_pressed ())
            goto end_mail;

         i = TIMED_READ(1);

         if(i == YOOHOO)
            continue;

         time_release ();

         switch(i) {
         case '*':
            if (!config->emsi)
               break;
            get_emsi_id (buffer, 30);
            if (!strnicmp (buffer, "**EMSI_REQA77E", 14)) {
               m_print2 ("**EMSI_INQC816\r");
               emsi_handshake (1);
            }
            break;
         case ENQ:
            if (!config->wazoo)
               break;

            if (send_YOOHOO(1)) {
               WaZOO(1);
               get_call_list();
               goto  end_mail;
            }
            break;
         case 0x00:
         case 0x01:
         case 'C':
            if(j == 'C') {
               TIMED_READ(1);
               FTSC_sender(0);
               goto end_mail;
            }
            break;

         case 0xFE:
         case -2:
            if(j == 0x01) {
               FTSC_sender(0);
               goto end_mail;
            }
            break;

         case 0xFF:
         case -1:
            if(j == 0x00) {
               FTSC_sender(0);
               goto end_mail;
            }
            break;

         case NAK:
            if(j == NAK) {
               FTSC_sender(0);
               goto end_mail;
            }
            break;
         }

         if((i != -1) && (i != 0xFF))
            j = i;
      }

      if (config->emsi)
         m_print2 ("**EMSI_INQC816\r**EMSI_INQC816\r");

      if (config->wazoo)
         SENDBYTE (YOOHOO);
      SENDBYTE (TSYNC);
   }

bad_mail:
//   set_prior (2);

   config->janus = jan;
   config->wazoo = waz;
   config->emsi = ems;

   status_line(msgtxt[M_NOBODY_HOME]);

   if (bad)
      bad_call (net, node, 1, TIMEDOUT);

   modem_hangup ();
   sysinfo.today.failed++;
   sysinfo.week.failed++;
   sysinfo.month.failed++;
   sysinfo.year.failed++;

   idle_system ();
   return (TIMEDOUT);

end_mail:
//   set_prior (2);

   config->janus = jan;
   config->wazoo = waz;
   config->emsi = ems;

   for (i = 0; i < max_call; i++) {
      if (zone == call_list[i].zone && net == call_list[i].net && node == call_list[i].node && !call_list[i].point)
         break;
   }
   for (m = i + 1; m < max_call; m++, i++)
      memcpy (&call_list[i], &call_list[m], sizeof (struct _call_list));
   max_call--;

   idle_system ();
   return(1);
}

int bad_call(bnet,bnode,rwd,flags)
int bnet, bnode, rwd, flags;
{
        int res, i, j, mc, mnc;
	struct ffblk bad_dta;
	FILE *bad_wazoo;
        char *p, *HoldName, fname[50], fname1[50];

	HoldName = hold_area;
	sprintf (fname, "%s%04x%04x.$$?", HoldName, bnet, bnode);
	j = strlen (fname) - 1;
	res = -1;

        if (cur_event > -1) {
                mc = e_ptrs[cur_event]->with_connect ? e_ptrs[cur_event]->with_connect : max_connects;
                mnc = e_ptrs[cur_event]->no_connect ? e_ptrs[cur_event]->no_connect : max_noconnects;
        }

        i = 0;
	if (!findfirst(fname,&bad_dta,0))
		do {
			if (isdigit (bad_dta.ff_name[11])) {
				fname[j] = bad_dta.ff_name[11];
				res = fname[j] - '0';
				break;
			}
		} while (!findnext(&bad_dta));

	if (res == -1)
		fname[j] = '0';

	if (rwd > 0) {
		strcpy (fname1, fname);
		fname1[j]++;
		if (fname1[j] > '9')
			fname1[j] = '9';

		if (res == -1) {
			if (rwd == 2)
                                res = cshopen (fname, O_CREAT + O_WRONLY + O_BINARY, S_IWRITE);
			else
                                res = cshopen (fname1, O_CREAT + O_WRONLY + O_BINARY, S_IWRITE);
			i = rwd - 1;
                        write (res, (char *) &i, sizeof (short));
                        write (res, (char *) &flags, sizeof (short));
                        close (res);
		}
		else {
			if (rwd == 2) {
                                i = shopen (fname, O_RDONLY + O_BINARY);
                                res = 0;
                                read (i, (char *) &res, sizeof (short));
				close (i);

				++res;

                                i = cshopen (fname, O_CREAT + O_WRONLY + O_BINARY, S_IWRITE);
                                write (i, (char *) &res, sizeof (short));
                                write (i, (char *) &flags, sizeof (short));
                                close (i);
			}
			else {
				rename (fname, fname1);
			}
		}
	}
	else if (rwd == 0) {
		if (res == -1)
			return (0);
                if (res >= mc)
			return (1);
		res = 0;
                i = shopen (fname, O_RDONLY + O_BINARY);
                res = 0;
                read (i, (char *) &res, sizeof (short));
		close (i);
                return (res >= mnc);
	}
	else {
		if (res != -1) {
			unlink (fname);
		}

		sprintf (fname, "%s%04x%04x.Z", HoldName, bnet, bnode);
		if (dexists (fname)) {
			bad_wazoo = fopen (fname, "ra");
			while (!feof (bad_wazoo)) {
				e_input[0] = '\0';
				if (!fgets (e_input, 64, bad_wazoo))
					break;
				p = strchr (e_input, ' ') + 1;
				p = strchr (p, ' ');
				*p = '\0';
				p = strchr (e_input, ' ') + 1;

				strcpy (fname1, filepath);
				strcat (fname1, p);
				unlink (fname1);
			}
			fclose (bad_wazoo);
			unlink (fname);
		}
	}

	return (0);
}

int mail_session()
{
   int i, flag, oldsnoop;
   char buffer[30], *pwpos;
   long t1, t2;
   char *emsi_req = "**EMSI_REQA77E\r", *iemsi_req = "**EMSI_IRQ8E08\r";

   if (cur_event >= 0 && (e_ptrs[cur_event]->behavior & MAT_NOMAIL24))
      return (0);

   if (!config->mail_only[0])
      strcpy (config->mail_only, msgtxt[M_NO_BBS]);
   if (!config->enterbbs[0])
      strcpy (config->enterbbs, msgtxt[M_PRESS_ESCAPE]);

   filepath = config->norm_filepath;
   request_list = config->norm_okfile;
   max_requests = config->norm_max_requests;
   max_kbytes = config->norm_max_kbytes;

   pwpos = NULL;
   iemsi = 0;
   flag = 0;
   assumed = 0;
   got_arcmail = 0;
   caller = 0;
   oldsnoop = snooping;
   snooping = 0;
   remote_capabilities = 0;
   find_event ();

   t1 = timerset(3000);
   t2 = timerset(400);

   dial_system ();
   function_active = 0;
   timeout = t1;
   to_row = 8;
   local_status ("Connected");
   prints (7, 65, YELLOW|_BLACK, "Sync.");

   if (config->override_pwd[0])
      pwpos = config->override_pwd;
   if (config->emsi)
      m_print2 (emsi_req);

   while(!timeup(t1) && CARRIER) {
      if (timeup(t2) && !flag) {
         if (config->emsi)
            m_print2 (emsi_req);
         m_print2(msgtxt[M_ADDRESS],config->alias[0].zone,config->alias[0].net,config->alias[0].node,config->alias[0].point,VERSION);
         if (config->banner[0] == '@')
            read_file (&config->banner[1]);
         else
            m_print2("%s\n",config->banner);
         if (cur_event >= 0 && !(e_ptrs[cur_event]->behavior & MAT_BBS)) {
            if (config->mail_only[0] == '@')
               read_file (&config->mail_only[1]);
            else {
               m_print2 ("\r");
               m_print2 (config->mail_only, time_to_next_nonmail ());
            }
         }
         else {
            if (config->use_iemsi)
               m_print2 (iemsi_req);
            if (config->enterbbs[0] == '@')
               read_file (&config->enterbbs[1]);
            else
               m_print2("\r%s",config->enterbbs);
         }

         flag=1;
      }

      i = TIMED_READ(1);

      switch (i) {
         case -1:
            break;

         case '*':
            if (!config->emsi)
               break;
            get_emsi_id (buffer, 14);
//            status_line (">DEBUG: get_emsi_id = '%s'", buffer);

            if (!strnicmp (buffer, "**EMSI_INQC816", 14))
               emsi_handshake (0);
            else if (config->use_iemsi) {
               if (!strnicmp (buffer, "**EMSI_ICI", 10)) {
                  iemsi_handshake ();
                  if (iemsi) {
                     m_print2 ("\n\n");
                     snooping = oldsnoop;
                     timer (5);

                     return(0);
                  }
               }
               else if (!strnicmp (buffer, "**EMSI_CLIFA8C", 14))
                  m_print2 (iemsi_req);
            }
            break;

         case ' ':
         case 0x0D:
            if (!flag)
               break;
            if (cur_event >= 0 && !(e_ptrs[cur_event]->behavior & MAT_BBS)) {
               if (config->mail_only[0] == '@')
                  read_file (&config->mail_only[1]);
               else {
                  m_print2 ("\r");
                  m_print2 (config->mail_only, time_to_next_nonmail ());
               }
            }
            else {
               if (config->use_iemsi)
                  m_print2 (iemsi_req);
               if (config->enterbbs[0] == '@')
                  read_file (&config->enterbbs[1]);
               else
                  m_print2("\r%s",config->enterbbs);
            }
            break;

         case YOOHOO:
            if (!timeup(t2) || !config->wazoo)
               break;
            remote_capabilities = 0;
            if (get_YOOHOO(1))
               WaZOO(0);
            return(1);

         case TSYNC:
            if (!timeup(t2))
                break;
            FTSC_receiver (0);
            return(1);

         case 0x1B:
            if (!flag) {
               if (config->emsi)
                  m_print2 (emsi_req);
               m_print2(msgtxt[M_ADDRESS],config->alias[0].zone,config->alias[0].net,config->alias[0].node,config->alias[0].point,VERSION);
               if (config->banner[0] == '@')
                  read_file (&config->banner[1]);
               else
                  m_print2("%s\n",config->banner);
               if (cur_event >= 0 && !(e_ptrs[cur_event]->behavior & MAT_BBS)) {
                  if (config->mail_only[0] == '@')
                     read_file (&config->mail_only[1]);
                  else {
                     m_print2 ("\r");
                     m_print2 (config->mail_only, time_to_next_nonmail ());
                 }
               }
               else {
                  if (config->use_iemsi)
                     m_print2 (iemsi_req);
                  if (config->enterbbs[0] == '@')
                     read_file (&config->enterbbs[1]);
                  else
                     m_print2("\r%s",config->enterbbs);
               }

               flag=1;
            }
            else {
               if (cur_event >= 0 && !(e_ptrs[cur_event]->behavior & MAT_BBS))
                  break;

               m_print2 ("\n\n");
               snooping = oldsnoop;
               timer (5);

               return (0);
            }
            break;

         default:
            if (cur_event >= 0 && !(e_ptrs[cur_event]->behavior & MAT_BBS)) {
               if (pwpos != NULL) {
                  i &= 0xFF;

                  if (i == *pwpos) {
                     pwpos++;
                     if (*pwpos == '\0') {
                        m_print2 ("\n\n");
                        snooping = oldsnoop;
                        timer (5);

                        return (0);
                     }
                  }
                  else
                     pwpos = config->override_pwd;
               }
            }
            break;
      }

      time_release ();
   }

   if (!CARRIER)
      return (1);

   if (cur_event >= 0 && !(e_ptrs[cur_event]->behavior & MAT_BBS))
      return (1);

   m_print2 ("\n\n");
   snooping = oldsnoop;
   timer (5);

   return (0);
}

int iemsi_session (void)
{
   int i, oldsnoop;
   char buffer[30];
   long t1, t2;
   char *iemsi_req = "**EMSI_IRQ8E08\r";

   iemsi = 0;
   assumed = 0;
   got_arcmail = 0;
   caller = 0;
   oldsnoop = snooping;
   snooping = 0;
   remote_capabilities = 0;
   find_event ();

   t2 = timerset (200);
   t1 = timerset (400);

   dial_system ();
   function_active = 0;
   timeout = t1;
   to_row = 8;
   local_status ("Connected");
   prints (7, 65, YELLOW|_BLACK, "Sync.");

   while (!timeup (t1) && CARRIER) {
      if (timeup (t2))
         m_print2 (iemsi_req);

      i = TIMED_READ(1);

      switch (i) {
         case -1:
            break;
         case '*':
            get_emsi_id (buffer, 12);

            if (!strnicmp (buffer, "**EMSI_ICI", 10)) {
               iemsi_handshake ();
               if (iemsi) {
                  m_print2 ("\n\n");
                  snooping = oldsnoop;
                  return (0);
               }
            }
            break;

         case 0x0D:
         case 0x1B:
            m_print2 ("\n\n");
            snooping = oldsnoop;
            return(0);
      }

      time_release ();
   }

   if (!CARRIER)
      return (1);

   m_print2 ("\n\n");
   snooping = oldsnoop;
   return (0);
}

static void galileo_input (s, width)
char *s;
int width;
{
   char c, buffer[80], asc1[40];
   int i = 0;
   long t, lt;
   struct tm *tim;

   t = timerset (300);
   UNBUFFER_BYTES ();

   while (CARRIER && !timeup (t)) {
      while (PEEKBYTE() == -1) {
         if (!CARRIER || timeup (t))
            return;
         lt = time (NULL);
         tim = localtime (&lt);
         strcpy (asc1, asctime (tim));
         asc1[strlen (asc1) - 1] = '\0';
         sprintf (buffer, "%s   (%d)   ", asc1, timezone / 3600L);
         tim = gmtime (&lt);
         strcpy (asc1, asctime (tim));
         asc1[strlen (asc1) - 1] = '\0';
         strcat (buffer, asc1);
         wcenters (5, YELLOW|_BLACK, buffer);
         time_release ();
      }

      c = (char)TIMED_READ(1);
      t = timerset (300);

      if (c == 0x0D)
         break;

      if (i >= width)
         continue;

      if (c < 0x20)
         continue;

      s[i++]=c;
   }

   s[i]='\0';
}

#define isLeap(x) ((x)%1000)?((((x)%100)?(((x)%4)?0:1):(((x)%400)?0:1))):(((x)%4000)?1:0)

int poll_galileo (max_connect, max_tries)
int max_connect, max_tries;
{
   int tries, i, wh, yy, mm, dy, hh, ss, mi;
   char buffer[85], stringa[85];
   long t1, tu;
   struct date dd;
   struct time dt;
   int mdays[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

   function_active = 0;
   dial_system ();
   local_kbd = -1;
   local_status ("ClockSync");

   init = config->galileo_init;
   mdm_sendcmd (config->galileo_init);

   for(tries=0;tries < max_tries;tries++) {
      if (tries) {
         prints (7, 65, YELLOW|_BLACK, "Pausing  ");

         tu = timerset (300);
         while (!timeup (tu)) {
            if (local_kbd == 0x1B) {
               local_kbd = -1;
               i = 0;
               modem_hangup ();
               break;
            }
            time_release ();
         }
      }

      sysinfo.today.outcalls++;
      sysinfo.week.outcalls++;
      sysinfo.month.outcalls++;
      sysinfo.year.outcalls++;

      status_line(msgtxt[M_DIALING_NUMBER],config->galileo);

      wh = wopen (13, 9, 20, 45, 3, RED|_LGREY, BLUE|_LGREY);
      wactiv (wh);
      wtitle (" Dialing ", TCENTER, RED|_LGREY);

      wcenters (1, BLUE|_LGREY, "- Atomic Clock Adjustment -");
      wcenters (2, BLUE|_LGREY, "Ist. Galileo Ferraris");
      wcenters (3, BLUE|_LGREY, config->galileo);

      CLEAR_INBOUND();
      CLEAR_OUTBOUND();

      rate = 1200;
      if (!config->lock_baud)
         com_baud (rate);
      answer_flag = 1;

      sprintf (buffer, "%u Baud", rate);
      wcenters (4, BLUE|_LGREY, buffer);

      prints (7, 65, YELLOW|_BLACK, "DialPhone");
      mdm_sendcmd (config->galileo_dial);
      mdm_sendcmd (config->galileo);
      mdm_sendcmd (config->galileo_suffix[0] == '\0' ? config->galileo_suffix : "\r");
      prints (7, 65, YELLOW|_BLACK, "Waiting  ");
      i = wait_for_connect (0);

      wclose ();

      if (i > 0)
         break;
      else {
         prints (7, 65, YELLOW|_BLACK, "Hangup ");
         modem_hangup ();
         if (i == ABORTED) {
            local_kbd = -1;
            break;
         }
      }
   }

   init = config->init;

   if (i <= 0) {
      answer_flag = 0;

      idle_system ();
      timeout = 0L;
      function_active = 99;
      return (i);
   }

online_out:
   local_status ("Online");

   if (mdm_flags == NULL) {
      status_line(msgtxt[M_READY_CONNECT], rate, "", "");
      mdm_flags = "";
   }
   else
      status_line(msgtxt[M_READY_CONNECT], rate, "/", mdm_flags);

   if (!lock_baud)
      com_baud(rate);

   wh = wopen (12, 0, 24, 79, 0, LGREY|_BLACK, WHITE|_BLACK);
   wactiv (wh);
   wtitle ("REMOTE CLOCK ADJUSTMENT", TLEFT, LCYAN|_BLACK);
   printc (12, 0, LGREY|_BLACK, 'Ã');
   printc (12, 52, LGREY|_BLACK, 'Á');
   printc (12, 79, LGREY|_BLACK, '´');

   wcenters (0, LGREY|_BLACK, "Ist. Galileo Ferraris");
   sprintf (buffer, "Connected at %u baud", rate);
   wcenters (1, LGREY|_BLACK, buffer);

   t1 = timerset(1500);
   timeout = t1;

   while (!timeup(t1) && CARRIER) {
      galileo_input (buffer, 80);
      if (!CARRIER)
         break;
      galileo_input (stringa, 80);
      if (!CARRIER)
         break;

      if (!strncmp (buffer, stringa, 16)) {
         sscanf (&stringa[17], "%2d", &ss);
         sscanf (&stringa[37], "%4d%2d%2d%2d%2d", &yy, &mm, &dy, &hh, &mi);

         hh -= (int)(timezone / 3600L);
         if (!config->solar && !strncmp (&stringa[20], "CEST", 4))
            hh--;

         if (isLeap (yy))
            mdays[2]++;

         if (hh >= 24) {
            hh %= 24;
            if (++dy > mdays[mm]) {
               dy = 1;
               if (++mm > 12) {
                  mm = 1;
                  yy++;
               }
            }
         }
         else if (hh < 0) {
            hh += 24;
            if (--dy < 0) {
               if (--mm < 0) {
                  mm = 12;
                  yy--;
                  dy = 31;
               }
               else
                  dy = mdays[mm];
            }
         }

         dt.ti_hour = hh;
         dt.ti_min = mi;
         dt.ti_sec = ss;
         dt.ti_hund = 0;
         dd.da_year = yy;
         dd.da_mon = mm;
         dd.da_day = dy;

         setdate (&dd);
         settime (&dt);

         status_line ("+Remote clock: %02d-%02d-%02d %02d:%02d:%02d", dd.da_day, dd.da_mon, dd.da_year % 100, dt.ti_hour, dt.ti_min, dt.ti_sec);

         wclose ();
         modem_hangup ();
         status_line ("+Resync succesful");

         answer_flag = 0;
         rate = config->speed;
         if (!lock_baud)
            com_baud(rate);

         idle_system ();
         timeout = 0L;
         function_active = 99;

         return (1);
      }
   }

   wclose ();
   prints (7, 65, YELLOW|_BLACK, "Hangup ");
   modem_hangup ();

   answer_flag = 0;
   rate = config->speed;
   if (!lock_baud)
      com_baud(rate);

   idle_system ();
   timeout = 0L;
   function_active = 99;

   return (0);
}

void iemsi_handshake (void)
{
   int i, tries, miemsi = 0;
   unsigned long crc;
   char string[2048], *p, addr[20];
   long t1, t2;
   struct _usr iusr;

   memset ((char *)&iusr, 0, sizeof (struct _usr));
   prints (7, 65, YELLOW|_BLACK, "IEMSI/C1");

   tries = 0;
   t1 = timerset (1000);
   t2 = timerset (6000);

resend:
   if (tries++ > 6) {
      status_line ("!IEMSI Error: too may tries");
      return;
   }

   if (tries > 1) {
      m_print2 ("**EMSI_NAKEEC3\r");
      t1 = timerset (1000);

      while (CARRIER && !timeup (t1) && !timeup (t2)) {
         if (PEEKBYTE () == '*') {
            get_emsi_id (string, 10);
            if (!strncmp (string, "**EMSI_ICI", 10) || !strncmp (string, "*EMSI_ICI", 9))
               break;
            if (!strncmp (string, "**EMSI_HBT", 10) || !strncmp (string, "**EMSI_ACK", 10))
               t1 = timerset (1000);
         }
         else if (PEEKBYTE () != -1) {
            TIMED_READ(1);
            time_release ();
         }
      }
   }

   if (!get_emsi_field (string))         // Name
      goto resend;
   string[35] = '\0';
   strcpy (iusr.name, fancy_str (strbtrim (string)));
   if (iusr.name[strlen (iusr.name) - 1] == '.')
      iusr.name[strlen (iusr.name) - 1] = '\0';

   if (!get_emsi_field (string))         // Alias
      goto resend;
   string[35] = '\0';
   strcpy (iusr.handle, fancy_str (string));
   if (iusr.handle[strlen (iusr.handle) - 1] == '.')
      iusr.handle[strlen (iusr.handle) - 1] = '\0';

   if (!get_emsi_field (string))         // Location
      goto resend;
   string[25] = '\0';
   strcpy (iusr.city, string);

   if (!get_emsi_field (string))         // Data#
      goto resend;
   string[19] = '\0';
   strcpy (iusr.dataphone, string);

   if (!get_emsi_field (string))         // Voice#
      goto resend;
   string[19] = '\0';
   strcpy (iusr.voicephone, string);

   if (!get_emsi_field (string))         // Password
      goto resend;
   string[15] = '\0';
   strcpy (iusr.pwd, strupr (string));

   if (!get_emsi_field (string))         // Birthdate
      goto resend;
   string[9] = '\0';
   strcpy (iusr.birthdate, string);

   if (!get_emsi_field (string))         // CRTDEF
      goto resend;

   p = strtok (strupr (string), ",");
   if (strcmp (p, "AVT0"))
      iusr.color = iusr.avatar = 1;
   else if (strcmp (p, "ANSI"))
      iusr.color = iusr.ansi = 1;
   else if (strcmp (p, "VT52") || strcmp (p, "VT100"))
      iusr.ansi = 1;

   p = strtok (NULL, ",");
   iusr.len = atoi (p);

   p = strtok (NULL, ",");
   iusr.width = atoi (p);

   p = strtok (NULL, ",");
   iusr.nulls = atoi (p);

   if (!get_emsi_field (string))         // Protocols
      goto resend;

   if (!get_emsi_field (string))         // Capabilities
      goto resend;

   strupr (string);
   if (strstr (string, "ASCII8"))
      iusr.ibmset = 1;

   if (!get_emsi_field (string))         // Requests
      goto resend;

   strupr (string);
   if (strstr (string, "MORE"))
      iusr.more = 1;
   if (strstr (string, "HOT"))
      iusr.hotkey = 1;
   if (strstr (string, "CLR"))
      iusr.formfeed = 1;
   if (strstr (string, "FSED"))
      iusr.use_lore = 0;
   else
      iusr.use_lore = 1;
   if (strstr (string, "MAIL"))
      miemsi |= IEMSI_MAILCHECK;
   if (strstr (string, "FILE"))
      miemsi |= IEMSI_FILECHECK;
   if (strstr (string, "NEWS"))
      miemsi |= IEMSI_NEWS;

   if (!get_emsi_field (string))         // Software
      goto resend;

   if (!get_emsi_field (string))         // Xlattbl
      goto resend;

   prints (7, 65, YELLOW|_BLACK, "IEMSI/C2");

   strcpy (string, "{LoraBBS,");
   strcat (string, VNUM);
   activation_key ();
   if (registered) {
      sprintf (addr, ",%s%05u}{", serial_id[0] ? serial_id : "", serial_no);
      strcat (string, addr);
   }
   else
      strcat (string, ",Demo}{");
   strcat (string, system_name);
   strcat (string, "}{");
   if (location != NULL)
      strcat (string, location);
   strcat (string, "}{");
   strcat (string, sysop);
   strcat (string, "}{");
   sprintf (addr, "%08lX", time (NULL) - timezone);
   strcat (string, addr);
   strcat (string, "}{}{!}{}");

   sprintf (addr, "EMSI_ISI%04X", strlen (string));
   crc = 0xFFFFFFFFL;
   for (i = 0; i < strlen (addr); i++)
      crc = Z_32UpdateCRC (addr[i], crc);

   for (i = 0; i < strlen (string); i++)
      crc = Z_32UpdateCRC (string[i], crc);

   t1 = timerset (6000);
   timeout = t1;
   to_row = 8;
   tries = 1;

   for (;;) {
      m_print2 ("**EMSI_ISI%04X%s", strlen (string), string);
      CLEAR_INBOUND ();
      m_print2 ("%08lX\r", crc);
      FLUSH_OUTPUT ();

      if (tries++ > 10) {
         modem_hangup ();
         status_line ("!Trans. IEMSI failure");
         return;
      }

      t2 = timerset (1000);

      while (!timeup (t1)) {
         if (timeup (t2))
            break;

         if (PEEKBYTE() == '*') {
            get_emsi_id (addr, 14);
            if (!strnicmp (addr, "**EMSI_ICI", 10) || !strnicmp (addr, "*EMSI_ICI", 9)) {
               CLEAR_INBOUND ();
               m_print2 ("**EMSI_ACKA490\r");
               break;
            }
            if (!strnicmp (addr, "**EMSI_ACKA490", 14) || !strnicmp (addr, "*EMSI_ACKA490", 13))
               break;
            if (!strnicmp (addr, "**EMSI_NAKEEC3", 14) || !strnicmp (addr, "*EMSI_NAKEEC3", 13))
               break;
         }
         else if (PEEKBYTE () != -1) {
            TIMED_READ(10);
            time_release ();
         }
      }

      if (!strnicmp (addr, "**EMSI_ACKA490", 14) || !strnicmp (addr, "*EMSI_ACKA490", 13))
         break;
   }

   memcpy ((char *)&usr, (char *)&iusr, sizeof (struct _usr));
   iemsi = miemsi | IEMSI_ON;
}

