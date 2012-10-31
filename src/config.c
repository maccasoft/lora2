
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
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <process.h>
#include <conio.h>
#include <io.h>
#include <dos.h>
#include <fcntl.h>
#include <sys\stat.h>

#ifdef __OS2__
#define INCL_DOS
#include <os2.h>
#endif

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"
#include "exec.h"

#ifdef __OS2__
extern HFILE hfComHandle;
#endif

extern int blank_timer, socket, lastread_need_save;
extern short tcpip;
extern long exectime;

int spawn_program (int swapout, char *outstring);
void update_lastread_pointers (void);
char *config_file = "CONFIG.DAT", **dos_argv, nopoll = 0, nomailproc = 0, nonetmail = 0;

static MDM_TRNS *mm_head;

int far parse_config()
{
   FILE *fp;
   char linea[256], *p, *pc;

   _vinfo.dvcheck = 0;
   videoinit();

   pc = config_file;

   fp = fopen (config_file, "rb");
   if (fp == NULL) {
      if ((p=getenv ("LORA")) != NULL) {
         strcpy (linea, p);
         append_backslash (linea);
         strcat (linea, config_file);
         config_file = linea;
      }
      fp = fopen (config_file, "rb");
   }

try_setup:
   if (fp != NULL) {
      fread ((char *)config, sizeof (struct _configuration), 1, fp);
      fclose (fp);

      if (config->version != CONFIG_VERSION) {
         printf ("\n\aConfiguration file error. Run LSETUP.\n");
         exit (0);
      }

      hold_area = config->outbound;
      netmail_dir = config->netmail_dir;
      if (config->bad_msgs[0])
         bad_msgs = config->bad_msgs;
      if (config->dupes[0])
         dupes = config->dupes;
      pip_msgpath = config->pip_msgpath;
      fido_msgpath = config->quick_msgpath;
      ipc_path = config->ipc_path;
      sysop = config->sysop;
      system_name = config->system_name;
      location = config->location;
      phone = config->phone;
      flags = config->flags;
      lock_baud = config->lock_baud;
		terminal = config->terminal;

		if(config->areachange_key[0])
			strncpy(area_change_key,config->areachange_key,4);

      if (config->modem_busy[0])
         modem_busy = config->modem_busy;
      if (config->mustbezero) {
         strcpy (config->answer, (char *)&config->mustbezero);
         memset (&config->mustbezero, 0, 20);
      }
      if (config->answer[0])
         answer = config->answer;
      init = config->init;
      dial = config->dial;
      log_name = config->log_name;
      frontdoor = config->log_style;
      filepath = config->norm_filepath;
      if (config->norm_okfile[0])
         request_list = norm_request_list = config->norm_okfile;
      if (config->prot_okfile[0])
         prot_request_list = config->prot_okfile;
      if (config->know_okfile[0])
         know_request_list = config->know_okfile;
      norm_max_kbytes = config->norm_max_kbytes;
      prot_max_kbytes = config->prot_max_kbytes;
      know_max_kbytes = config->know_max_kbytes;
      norm_max_requests = config->norm_max_requests;
      prot_max_requests = config->prot_max_requests;
      know_max_requests = config->know_max_requests;
      blank_timer = config->blank_timer;
      if (config->line_offset == 0)
         config->line_offset = 1;
      line_offset = config->line_offset;
      snooping = config->snooping;
      aftercaller_exit = config->aftercaller_exit;
      aftermail_exit = config->aftermail_exit;
		dateformat = config->dateformat;
      timeformat = config->timeformat;

      if (config->no_direct)
         setvparam (VP_BIOS);
      else
         setvparam (VP_DMA);
      if (config->snow_check)
         setvparam (VP_CGA);
      if (config->mono_attr)
         setvparam (VP_MONO);

      if (!config->speed)
         config->speed = config->old_speed;
      if (!speed)
         speed = rate = config->speed;
      if (com_port == -1)
         com_port = config->com_port;

      if (!config->carrier_mask) {
         config->carrier_mask = 128;
         config->dcd_timeout = 1;
      }

      Txbuf = Secbuf = (byte *) malloc (WAZOOMAX + 16);
      if (Txbuf == NULL)
         exit (250);
   }
   else {
      config_file = pc;
      spawn_program (1, config_file);

      fp = fopen (config_file, "rb");
        if (fp != NULL)
           goto try_setup;

      exit (1);
   }

   config_file = pc;
   return(1);
}

void init_system()
{
   log_name = NULL;
   sysop = system_name = request_list = NULL;
   availist = about = hold_area = NULL;
   password = NULL;
   pip_msgpath = timeformat = dateformat = text_path = ipc_path = NULL;
   answer = ext_mail_cmd = NULL;
   norm_about = norm_availist = NULL;
   norm_request_list = prot_request_list = prot_availist = prot_about = NULL;
   know_request_list = know_availist = know_about = NULL;
   bad_msgs = dupes = netmail_dir = location = phone = flags = NULL;
   modem_busy = NULL;
   hslink_exe = puma_exe = NULL;
   mm_head = NULL;
   msg_list = NULL;

   blank_timer = 0;
   tcpip = 0;

   vote_priv = up_priv = down_priv = 0;

   norm_max_kbytes = prot_max_kbytes = know_max_kbytes = max_kbytes = 0;
   CurrentReqLim = max_call = speed_graphics = max_requests = 0;
   registered = line_offset = no_logins = next_call = 0;
   norm_max_requests = prot_max_requests = know_max_requests = 0;
   allowed = assumed = remote_capabilities = 0;
   target_up = target_down = 0;
   remote_zone = remote_net = remote_node = remote_point = 0;
   com_port = -1;

   emulator = answer_flag = lock_baud = terminal = use_tasker = 0;
   got_arcmail = sliding = ackless_ok = do_chksum = may_be_seadog = 0;
   did_nak = netmail = who_is_he = overwrite = allow_reply = 0;
   frontdoor = local_mode = user_status = caller = 0;
   snooping = 1;

   rate = speed = 0L;

   max_readpriv = ASSTSYSOP;
   sq_ptr = NULL;

   strcpy (area_change_key, "[]?");
   config = (struct _configuration *)malloc (sizeof (struct _configuration));
   if (config == NULL) {
      printf ("\a\a\a");
      exit (0);
   }

#ifdef __OS2__
   hfComHandle = NULLHANDLE;
#endif
}

int get_priv(txt)
char *txt;
{
   int i, priv;

   priv = HIDDEN;

   for (i=0;i<12;i++)
      if (!stricmp(levels[i].p_string, txt) ||
          levels[i].p_string[0] == txt[0] )
      {
         priv = levels[i].p_length;
         break;
      }

   return (priv);
}

char *get_priv_text (priv)
int priv;
{
   int i;
   char *txt;

   txt = "???";

   for (i=0;i<12;i++)
      if (levels[i].p_length == priv)
      {
         txt = levels[i].p_string;
         break;
      }

   return (txt);
}

char *replace_blank(s)
char *s;
{
   char *p;

   while ((p=strchr(s,'_')) != NULL)
      *p = ' ';

   return (s);
}

int get_class(p)
int p;
{
   int m;
   for(m=0;m<MAXCLASS;m++) if(config->class[m].priv == p)
      return(m);
   for(m=0;m<MAXCLASS;m++) if(config->class[m].priv == 255)
   {
      config->class[m].priv=p;
      return(m);
   }
   return(-1);
}

void detect_line_number (void)
{
   int fd;
   char filename[80];
   struct _useron useron;

   sprintf (filename, USERON_NAME, config->sys_path);
   if ((fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) != -1) {
      while (read (fd, (char *)&useron, sizeof (struct _useron)) == sizeof (struct _useron)) {
         if (useron.line == line_offset) {
            line_offset++;
            lseek (fd, 0L, SEEK_SET);
         }
      }
      close (fd);
   }
}

void parse_command_line(argc, argv)
int argc;
char **argv;
{
#ifndef __OS2__
   int i, xms = 1, ems = 1, errors;
#else
   int i, errors;
#endif

   errors = 0;
   dos_argv = argv;
   nopoll = nomailproc = 0;

   for (i = 1; i < argc; i++) {
      if (argv[i][0] != '-' && argv[i][0] != '/')
         continue;

#ifndef __OS2__
      if (!stricmp (argv[i], "-NOXMS"))
         xms = 0;
      else if (!stricmp (argv[i], "-NOEMS"))
         ems = 0;
      if (!stricmp (argv[i], "-NP"))
#else
      else if (!stricmp (argv[i], "-NP"))
#endif         
         nopoll = 1;
      else if (!stricmp (argv[i], "-NM"))
         nomailproc = 1;
      else if (!stricmp (argv[i], "-XN"))
         nonetmail = 1;
#ifdef __OS2__
#if defined (__OCC__) || defined (__TCPIP__)
      else if (!stricmp (argv[i], "-TCPIP")) {
         socket = atoi (argv[++i]);
         tcpip = 1;
         caller = 1;
         snooping = 1;
         dos_argv = NULL;
         continue;
      }
#endif
#endif

      switch (toupper (argv[i][1])) {
         case 'B':
            speed = rate = atoi(&argv[i][2]);
            caller = 1;
            if (rate == 0)
               local_mode = 1;
            dos_argv = NULL;
            break;

         case 'C':
            config_file = &argv[i][2];
            break;

         case 'F':
            mdm_flags = &argv[i][2];
            break;

#ifdef __OS2__
         case 'H':
            hfComHandle = atol (&argv[i][2]);
            break;
#endif

         case 'I':
            init = &argv[i][2];
            break;

         case 'L':
            if (toupper (argv[i][2]) == 'F') {
               local_mode = 2;
               caller = 1;
               snooping = 1;
            }
            else
               local_mode = 1;
            dos_argv = NULL;
            break;

         case 'M':
            no_logins = 1;
            if (argv[i][2])
               ext_mail_cmd = (char *)&argv[i][2];
            break;

         case 'N':
            if (isdigit (argv[i][2]))
					line_offset = atoi (&argv[i][2]);
            else if (toupper (argv[i][2]) == 'A') {
               if (isdigit (argv[i][3]))
                  line_offset = atoi (&argv[i][3]);
               else
                  line_offset = 1;
               detect_line_number ();
            }
            break;

         case 'O':
            dos_argv = NULL;
            parse_netnode (&argv[i][2], &remote_zone, &remote_net, &remote_node, &remote_point);
            break;

         case 'T':
            if (isdigit (argv[i][2]))
               allowed = atoi (&argv[i][2]);
            break;

         case 'P':
            com_port = atoi(&argv[i][2]);
            com_port--;
            break;

         case 'R':
            log_name = (char *)&argv[i][2];
            break;

         default:
            errors = 1;
            printf (msgtxt[M_UNRECOGNIZED_OPTION], argv[i]);
            break;
		}
   }

#ifndef __OS2__
   if (xms) {
      if (_OvrInitExt (0L, 131072L) && ems)
         _OvrInitEms (0, 0, 8);
   }
   else if (ems)
      _OvrInitEms (0, 0, 8);
#endif

   if (errors)
      sleep (5);
}


void parse_config_line (argc, argv)
int argc;
char **argv;
{
   int i;

   for (i = 1; i < argc; i++) {
      if (argv[i][0] != '-' && argv[i][0] != '/')
         continue;

      switch (toupper (argv[i][1])) {
         case 'C':
            config_file = &argv[i][2];
            break;
      }
   }
}

void append_backslash (s)
char *s;
{
   int i;

   i = strlen(s) - 1;
   if (s[i] != '\\')
      strcat (s, "\\");
}

void dial_number (type, phone)
byte type;
char *phone;
{
   char *predial, *postdial;

   predial = config->dial;
   if (config->dial_suffix[0])
      postdial = config->dial_suffix;
   else
      postdial = "|";

   if (type) {
      if (type < 20) {
         if (config->altdial[type - 1].prefix[0])
            predial = config->altdial[type - 1].prefix;
         if (config->altdial[type - 1].suffix[0])
            postdial = config->altdial[type - 1].suffix;
      }
      else {
         type -= 20;
			if (config->prefixdial[type].prefix[0])
            predial = config->prefixdial[type].prefix;
      }
   }

   mdm_sendcmd (predial);
   mdm_sendcmd (phone);
   mdm_sendcmd (postdial);
}

long get_flags (char *p)
{
   long flag;

   flag = 0L;

   while (*p && *p != ' ')
      switch (toupper(*p++))
      {
      case 'V':
         flag |= 0x00000001L;
         break;
      case 'U':
         flag |= 0x00000002L;
         break;
      case 'T':
         flag |= 0x00000004L;
         break;
      case 'S':
         flag |= 0x00000008L;
         break;
      case 'R':
         flag |= 0x00000010L;
			break;
      case 'Q':
         flag |= 0x00000020L;
         break;
      case 'P':
         flag |= 0x00000040L;
         break;
      case 'O':
         flag |= 0x00000080L;
         break;
      case 'N':
         flag |= 0x00000100L;
         break;
      case 'M':
         flag |= 0x00000200L;
         break;
      case 'L':
         flag |= 0x00000400L;
         break;
      case 'K':
         flag |= 0x00000800L;
         break;
      case 'J':
         flag |= 0x00001000L;
         break;
      case 'I':
         flag |= 0x00002000L;
         break;
      case 'H':
         flag |= 0x00004000L;
         break;
      case 'G':
         flag |= 0x00008000L;
			break;
      case 'F':
         flag |= 0x00010000L;
         break;
      case 'E':
         flag |= 0x00020000L;
         break;
		case 'D':
			flag |= 0x00040000L;
			break;
		case 'C':
			flag |= 0x00080000L;
			break;
		case 'B':
			flag |= 0x00100000L;
			break;
		case 'A':
			flag |= 0x00200000L;
			break;
		case '9':
			flag |= 0x00400000L;
			break;
		case '8':
			flag |= 0x00800000L;
			break;
		case '7':
			flag |= 0x01000000L;
			break;
		case '6':
			flag |= 0x02000000L;
			break;
		case '5':
			flag |= 0x04000000L;
			break;
		case '4':
			flag |= 0x08000000L;
			break;
		case '3':
			flag |= 0x10000000L;
			break;
		case '2':
			flag |= 0x20000000L;
			break;
		case '1':
			flag |= 0x40000000L;
			break;
		case '0':
			flag |= 0x80000000L;
			break;
		}

	return (flag);
}

extern int ox, oy, wh1;

void get_down (int errlev, int flag)
{
	char filename[80];

	sysinfo.today.exectime += time (NULL) - exectime;
	sysinfo.week.exectime += time (NULL) - exectime;
	sysinfo.month.exectime += time (NULL) - exectime;
    sysinfo.year.exectime += time (NULL) - exectime;

    if(lastread_need_save)          // Aggiorna se necessario i puntatori
        update_lastread_pointers (); // Lastread per il qwk/BW


	if(something_wrong){
	bad_call (sw_net, sw_node, 1, ABORTED);
	}

	MsgCloseApi ();
	write_sysinfo();
	sprintf (filename, "F-TAG%d.TMP", config->line_offset);
	unlink (filename);
	sprintf (filename, "DL-REP%d.BBS", line_offset);
	unlink (filename);
	sprintf (filename, "REP%d.TXT", line_offset);
	unlink (filename);
	update_user ();

	if (!frontdoor) {
		status_line(":End");
		fprintf(logf, "\n");
	}

	fclose (logf);

	if (errlev || dos_argv == NULL) {
		if (!local_mode) {
			if (modem_busy != NULL)
				mdm_sendcmd (modem_busy);
		}

		MDM_DISABLE();

		wcloseall ();
		cclrscrn(LGREY|_BLACK);
		showcur();
		gotoxy (1, 10);
		exit ((errlev == -1) ? config->altx_errorlevel : errlev);
	}
	else {
		MDM_DISABLE();
		exit (255);
	}
}

