
#include "sched.h"
#include "msgapi.h"

extern byte logon_priv, vote_priv, up_priv, down_priv;

extern int max_requests, line_offset, aftermail_exit, aftercaller_exit;
extern int speed_graphics, registered, assumed, target_up, target_down;
extern int next_call, max_call, local_kbd, max_readpriv, max_kbytes;
extern int no_logins, status, mainview, locked, norm_max_kbytes;
extern int isOriginator, CurrentReqLim, have_dv, prot_max_kbytes;
extern int made_request, function_active, com_port, know_max_kbytes;
extern int norm_max_requests, prot_max_requests, know_max_requests;
extern int remote_zone, remote_net, remote_node, remote_point;
extern int called_zone, called_net, called_node;

extern char far *prodcode[];

extern char *VERSION, *NOREG, *log_name, *user_file, *sys_path, *mtext[];
extern char *mesi[], *menu_bbs, *sysop, *system_name, hex[], *hold_area;
extern char *filepath, *availist, *about, *request_list;
extern char *net_info, *banner, *enterbbs, *mail_only, *text_path;
extern char *sched_name, *dateformat, *timeformat, *password;
extern char *ext_flags, *ipc_path, *fido_msgpath, cmd_string[];
extern char *request_template, **bbstxt, *flag_dir, *protocols[];
extern char *prot_filepath, *know_filepath, e_input[128];
extern char *norm_filepath, *norm_about, *norm_availist, *answer;
extern char *norm_request_list, *prot_request_list, *prot_availist, *prot_about;
extern char *know_request_list, *know_availist, *know_about, *mdm_flags;
extern char *lang_name[MAX_LANG], *lang_descr[MAX_LANG], area_change_key[3];
extern char *ext_mail_cmd, *ext_editor, *pip_msgpath, lang_keys[MAX_LANG];
extern char *lang_txtpath[MAX_LANG], *QWKDir, *BBSid, frontdoor;
extern char *galileo, *location, *phone, *flags;

extern long logon_flags, cps, keycode;

extern char *msgtxt[];

extern char *ONLINE_MSGNAME;
extern char *USERON_NAME;
extern char *CBSIM_NAME;
extern char *SYSMSG_PATH;

extern char use_tasker, local_mode, user_status;
extern char have_ml, have_tv, have_ddos;

extern char far exit300, exit1200, exit2400, exit9600, exit14400;
extern char far exit16800, exit19200, exit38400;

extern struct _alias alias[MAX_ALIAS];
extern struct class_rec class[MAXCLASS];
extern struct _sysinfo sysinfo;
extern struct _linestat linestat;
extern struct _call_list far call_list[MAX_OUT];
extern struct _lastcall lastcall;
extern struct _lorainfo lorainfo;
extern struct _noask noask;
extern struct _alias resync[MAX_RESYNC];

extern MSG *sq_ptr;

extern struct parse_list menu_code[];
extern struct parse_list levels[];

extern unsigned short far crctab[256];
extern unsigned long far cr3tab[];

extern unsigned int comm_bits;
extern unsigned int parity;
extern unsigned int stop_bits;
extern unsigned int carrier_mask;
extern unsigned int handshake_mask;

extern char fossil_buffer[128];
extern char out_buffer[128];
extern char *fossil_fetch_pointer;
extern char *out_send_pointer;
extern int fossil_count;
extern int out_count;
extern int old_fossil;
extern char ctrlc_ctr;

extern EVENT *e_ptrs[64];
extern int requests_ok;
extern int num_events;
extern int cur_event;
extern int next_event;
extern int got_sched;
extern int noforce;
extern int max_connects;
extern int max_noconnects;
extern int matrix_mask;
extern int no_requests;

extern  word speed;
extern  byte answer_flag;
extern  byte lock_baud;
extern  byte terminal;
extern  byte got_arcmail;
extern  byte caller;
extern  byte nopause;
extern  byte recv_ackless;
extern  byte send_ackless;
extern  byte sliding;
extern  byte small_window;
extern  byte ackless_ok;
extern  byte no_overdrive;
extern  byte do_chksum;
extern  byte may_be_seadog;
extern  byte did_nak;
extern  byte netmail;
extern  byte who_is_he;
extern  byte overwrite;
extern  byte allow_reply;
extern  byte snooping;
extern  word rate;
extern  char *init;
extern  char *dial;
extern  struct _usr usr;
extern  struct _sys sys;
extern  FILE *logf;
extern  long start_time;
extern  int  allowed;
extern  byte usr_class;

extern  char Rxhdr[4];
extern  char Txhdr[4];
extern  long Rxpos;
extern  int Txfcs32;
extern  int Crc32t;
extern  int Crc32;
extern  int Znulls;
extern  int Rxtimeout;
extern  int Rxframeind;
extern  byte *Filename;
extern  word z_size;
extern  byte Resume_WaZOO;
extern  byte Resume_name[13];
extern  byte Resume_info[48];
extern  byte Abortlog_name[PATHLEN];
extern  byte lastsent;
extern  byte *Secbuf;
extern  byte *Txbuf;
extern  long file_length;
extern  word remote_capabilities;
extern  int fsent;
extern  char remote_password[10];
extern  struct _node nodelist;

extern  int num_msg;
extern  int first_msg;
extern  int last_msg;
extern  int lastread;
extern  struct _msg msg;
extern  int msg_tzone;
extern  int msg_tpoint;
extern  int msg_fzone;
extern  int msg_fpoint;
extern  char *messaggio[MAX_MSGLINE];
extern  int errs;
extern  int real_errs;
extern  word block_number;
extern  word base_block;
extern  int block_size;
extern  int fsize1;
extern  int fsize2;
extern  int first_block;
extern  char final_name[50];
extern  char *_fpath;

extern  struct _msg_list far msg_list[MAX_PRIV_MAIL];
extern  int max_priv_mail;
extern  int last_mail;


