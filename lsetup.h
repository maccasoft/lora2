
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

#include "defines.h"
#include "lora.h"

#define CONFIG_VERSION  5

#define MAXNL 20
#define MAXPACKER 10

struct _configuration {
	short version;

	char sys_path[40];

	char log_name[40];
	char log_style;

	char sched_name[40];
	char user_file[40];

	char norm_filepath[40];
	char know_filepath[40];
	char prot_filepath[40];
	char outbound[40];
	char netmail_dir[40];
	char bad_msgs[40];
	char dupes[40];
	char quick_msgpath[40];
	char pip_msgpath[40];
	char ipc_path[40];
	char net_info[40];
	char glob_text_path[40];
	char menu_path[40];
	char flag_dir[40];

	long keycode;

	char about[40];
	char files[40];
	short  norm_max_kbytes;
	short  prot_max_kbytes;
	short  know_max_kbytes;
	short  norm_max_requests;
	short  prot_max_requests;
	short  know_max_requests;
	char def_pack;

	char enterbbs[70];
	char banner[70];
	char mail_only[70];

	short  com_port;
	short  old_speed;
	char modem_busy[20];
	char dial[20];
	char init[40];
	char term_init[40];

	byte mustbezero;

	byte echomail_exit;
	byte netmail_exit;
	byte both_exit;

	long speed;
	short modem_OK_errorlevel;

	byte filler[10];

	struct _altern_dial {
		char prefix[20];
		char suffix[20];
	} altdial[10];

	bit  lock_baud   :1;
	bit  ansilogon   :2;
	bit  birthdate   :1;
	bit  voicephone  :1;
	bit  dataphone   :1;
	bit  emsi        :1;
	bit  ibmset      :1;

	bit  wazoo       :1;
	bit  msgtrack    :1;
	bit  keeptransit :1;
	bit  hslink      :1;
	bit  puma        :1;
	bit  secure      :1;
	bit  janus       :1;
	bit  terminal    :1;

	bit  fillerbug   :1;
	bit  no_direct   :1;
	bit  snooping    :1;
	bit  snow_check  :1;
	bit  unpack_norm :1;
	bit  unpack_know :1;
	bit  unpack_prot :1;

	short  blank_timer;

	struct _language {
		char txt_path[30];
		char descr[24];
		char basename[10];
	} language[MAX_LANG];

	char sysop[36];
	char system_name[50];
	char location[50];
	char phone[32];
	char flags[50];

	struct _alias old_alias[20];

	char newareas_create[50];
	char newareas_link[50];

	short  line_offset;
	short  min_calls;
	short  vote_limit;
	short  target_up;
	short  target_down;
	byte vote_priv;
	byte max_readpriv;

	word speed_graphics;

	byte aftercaller_exit;
	byte aftermail_exit;
	short  max_connects;
	short  max_noconnects;

	byte logon_level;
	long logon_flags;

	char areachange_key[4];
	char dateformat[20];
	char timeformat[20];
	char external_editor[50];

	struct class_rec class[12];

	char local_editor[50];

	char QWKDir[40];
	char BBSid[10];
	word qwk_maxmsgs;

	char galileo[30];

	char norm_okfile[40];
	char know_okfile[40];
	char prot_okfile[40];

	char reg_name[36];
	long betakey;

	struct _packers {
		char id[10];
		char packcmd[30];
		char unpackcmd[30];
	} packers[10];

	struct _nl {
		char list_name[14];
		char diff_name[14];
		char arc_name[14];
	} old_nl[10];

	bit  ansigraphics   :2;
	bit  avatargraphics :2;
   bit  hotkeys        :2;
   bit  screenclears   :2;

   bit  autozmodem     :1;
   bit  avatar         :1;
   bit  moreprompt     :2;
   bit  mailcheck      :2;
   bit  fullscrnedit   :2;

   bit  fillerbits     :2;
   bit  ask_protocol   :1;
   bit  ask_packer     :1;
   bit  put_uploader   :1;
   bit  keep_dl_count  :1;
   bit  use_areasbbs   :1;
   bit  write_areasbbs :1;

   short  rookie_calls;

   char pre_import[40];
   char after_import[40];
   char pre_export[40];
   char after_export[40];

   byte emulation;
   char dl_path[40];
   char ul_path[40];

   bit  manual_answer  :1;
   bit  limited_hours  :1;
   bit  solar          :1;
   bit  areafix        :1;
   bit  doflagfile     :1;
   bit  multitask      :1;
   bit  ask_alias      :1;
   bit  random_birth   :1;

   short  start_time;
   short  end_time;

   char boxpath[40];
   char dial_suffix[20];

   char galileo_dial[40];
   char galileo_suffix[40];
   char galileo_init[40];

   char areafix_help[40];
   char alert_nodes[50];

	char automaint[40];

   byte min_login_level;
   long min_login_flags;
   byte min_login_age;

   char resync_nodes[50];
   char bbs_batch[40];
   byte altx_errorlevel;

   char fax_response[20];
   byte fax_errorlevel;

   char dl_counter_limits[4];

   char areas_bbs[40];
	byte afx_remote_maint;
   byte afx_change_tag;

   bit  allow_rescan    :1;
   bit  check_city      :1;
   bit  check_echo_zone :1;
   bit  save_my_mail    :1;
   bit  mail_method     :2;
   bit  replace_tear    :2;

   char my_mail[40];

   bit  stripdash       :1;
   bit  use_iemsi       :1;
   bit  newmail_flash   :1;
   bit  mymail_flash    :1;
   bit  keep_empty      :1;
   bit  show_missing    :1;
   bit  random_redial   :1;

   char override_pwd[20];
   char pre_pack[40];
   char after_pack[40];

   byte modem_timeout;
   byte login_timeout;
   byte inactivity_timeout;

   struct _altern_prefix {
      char flag[6];
      char prefix[20];
   } prefixdial[10];

   char iemsi_handle[36];
   char iemsi_pwd[20];
   short  iemsi_infotime;

   bit  iemsi_on        :1;
   bit  iemsi_hotkeys   :1;
   bit  iemsi_quiet     :1;
   bit  iemsi_pausing   :1;
   bit  iemsi_editor    :1;
   bit  iemsi_news      :1;
   bit  iemsi_newmail   :1;
   bit  iemsi_newfiles  :1;

   bit  iemsi_screenclr :1;
   bit  prot_xmodem     :1;
   bit  prot_1kxmodem   :1;
   bit  prot_zmodem     :1;
   bit  prot_sealink    :1;
   bit  tic_active      :1;
   bit  tic_check_zone  :1;
   bit  filebox         :1;

   char newkey_code[30];
   char tearline[36];

   char  uucp_gatename[20];
   short uucp_zone;
   short uucp_net;
   short uucp_node;

   byte carrier_mask;
   byte dcd_timeout;

   struct {
      char  display[20];
      short offset;
      char  ident[20];
   } packid[10];

   char quote_string[5];
   char quote_header[50];

   char tic_help[40];
   char tic_alert_nodes[50];
   char tic_newareas_create[50];
   char tic_newareas_link[50];
	byte tic_remote_maint;
   byte tic_change_tag;

	short uucp_point;

   byte  dial_timeout;
   byte  dial_pause;

   bit   newfilescheck  :2;
   bit   mono_attr      :1;
	bit   force_intl     :1;
   bit   inp_dateformat :2;
   bit   single_pass    :1;
   bit   cdrom_swap     :1;

   short ul_free_space;
   char  hangup_string[40];
   char  init2[40];
   char  init3[40];

   short page_start[7];
   short page_end[7];

   char  logbuffer;

   char newareas_path[40];
   char newareas_base;

   char answer[40];

   bit   blanker_type   :3;
   bit   tcpip          :2;
	bit   export_internet:1;

	char  internet_gate;

   char  areafix_watch[50];
   char  tic_watch[50];

   byte  netmail;
   byte  echomail;
	byte  internet;
   byte  net_echo;
	byte  echo_internet;
	byte  net_internet;
	byte  echo_net_internet;

	char  upload_check[50];
	unsigned long setup_pwd;

   struct _alias alias[MAX_ALIAS];

/*   struct old_nl {
		char list_name[14];
		char diff_name[14];
		char arc_name[14];
	} nl[MAXNL];
*/
   struct _nl nl[MAXNL];
	char filler_2 [565]; // Per arrivare a 8192 Bytes
};

typedef struct _nodeinfo {
   short  zone;
   short  net;
   short  node;
	short  point;
   short  afx_level;
   char pw_session[20];
   char pw_areafix[20];
   char pw_tic[20];
   char pw_packet[9];
   short  modem_type;
   char phone[30];
   short  packer;
   char sysop_name[36];
   char system[36];
   bit  remap4d      :1;
   bit  can_do_22pkt :1;
   bit  wazoo        :1;
   bit  emsi         :1;
   bit  janus        :1;
   bit  pkt_create   :2;
   char aka;
   short tic_level;
   long afx_flags;
   long tic_flags;
   char tic_aka;
   long baudrate;
   char pw_inbound_packet[9];
	char mailer_aka;
	long min_baud_rate;
	char filler[40];
} NODEINFO;

typedef struct _node2name {
	char name[36];
	short zone;
	short net;
	short node;
	short point;
} NODE2NAME;


#define MAXCOST 20

typedef struct _translation {
	char location[30];
	char search[20];
	char traslate[60];
	struct _cost {
		short days;
		short start;
		short stop;
		short cost_first;
		short time_first;
		short cost;
		short time;
		bit   crash    :1;
		bit   direct   :1;
		bit   normal   :1;
	} cost[MAXCOST];
} ACCOUNT;

typedef struct {
	char  name[30];
	char  hotkey;

	bit   active          :1;
	bit   batch           :1;
	bit   disable_fossil  :1;
	bit   change_to_dl    :1;

   char  dl_command[80];
   char  ul_command[80];
   char  log_name[40];
   char  ctl_name[40];
   char  dl_ctl_string[40];
   char  ul_ctl_string[40];
   char  dl_keyword[20];
   char  ul_keyword[20];
   short filename;
   short size;
   short cps;
} PROTOCOL;

