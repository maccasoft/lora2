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

int  com_online(void);
void setup_screen(void);
void m_print(char *, ...);
void status_line(char *, ...);
void local_status(char *, ...);
void input(char *, int);
void cmd_input(char *, int);
void inpwd(char *, int);
void fancy_input(char *, int);
void chars_input(char *, int, int);
void mdm_sendcmd(char *);
int modem_response(void);
int read_file(char *);
int read_system_file(char *);
int far parse_config(void);
long timerset(int);
int timeup(long);
void timer(int);
void dostime(int *, int *, int *, int *);
void dosdate(int *, int *, int *, int *);
char * fancy_str(char *);
void press_enter(void);
char * data(char *);
void init_system(void);
int new_user(char *);
int login_user(void);
int get_priv(char *);
int dexists(char *);
void modem_hangup(void);
int get_option(char *);
char print_cmd_text(int, char *, int, int);
void print_hidden_item(int, char *);
void menu_dispatcher(char *);
char * get_string(char *, char *);
char * get_fancy_string(char *, char *);
char * get_number(char *, char *);
char * replace_blank(char *);
int real_flush(int);
int terminal_response(void);
void cls(void);
void change_attr(int);
void del_line(void);
void cpos(int, int);
void cup(int);
void cdo(int);
void cle(int);
void cri(int);
void signature_change(void);
void password_change(void);
void more_change(void);
void color_change(void);
void ansi_change(void);
void avatar_change(void);
void formfeed_change(void);
void fullscreen_change(void);
void tabs_change(void);
void scanmail_change(void);
void screen_change(void);
void nulls_change(void);
void city_change(void);
void voice_phone_change(void);
void data_phone_change(void);
void handle_change(void);
void user_configuration(void);
void user_list(char *);
void software_version(char *);
int time_remain(void);
void big_pause(int);
void Z_UncorkTransmitter(void);
int Z_GetByte(int);
void Z_PutString(unsigned char *);
void Z_SendHexHeader(unsigned int, unsigned char *);
int Z_GetHeader(unsigned char *);
int Z_GetZDL(void);
void Z_PutLongIntoHeader(long);
void show_loc(unsigned long, unsigned int);
void throughput(int, unsigned long);
long zfree(char *);
void send_can(void);
void remove_abort(char *, char *);
void unique_name(char *);
int is_arcmail(char *, int);
int check_failed(char *, char *, char *, char *);
void add_abort(char *, char *, char *, char *, char *);
int get_Zmodem(char *, FILE *);
int send_Zmodem(char *, char *, int, int);
int send_Hello(int);
int get_YOOHOO(int);
int get_bbs_record(int, int, int, int);
int send_YOOHOO(int);
void WaZOO(int);
int mail_session(void);
int continua(void);
int more_question(int);
int poll(int, int, int, int, int);
void display_area_list(int, int, int);
void read_sysinfo(void);
void write_sysinfo(void);
void FLUSH_OUTPUT(void);
void file_display(void);
void raw_dir(void);
void file_list(char *, char *);
void show_quote(void);
void ask_for_filename(char *, int);
void new_file_list(int);
int selprot(void);
int display_transfer(int, char *, char *, int);
void upload_file(char *, char);
int invalid_filename(char *);
void download_file(char *, int);
void read_forward(int, int);
void scan_message_base(int, char);
void squish_scan_message_base(int, char *, char);
void adjust_date(struct _msg *);
void write_msg_to_file(int, char *);
char * show_date(char *, char *, int, int);
int kill_message(int);
int read_message(int, int, int);
int squish_read_message(int, int, int);
void msg_kill(void);
void read_reply(void);
void read_parent(void);
void read_backward(int);
void read_nonstop(void);
int init_read(int);
void list_headers(int);
int msg_attrib(struct _msg *, int, int, int);
void line_editor(int);
void edit_continue(void);
void edit_insert(void);
void edit_line(void);
void edit_delete(void);
void edit_list(void);
void edit_change_subject(void);
void edit_change_to(void);
void free_message_array(void);
int get_message_data(int, char *);
void save_message(char *);
void space(int);
void inp_wrap(char *, char *, int);
int read_system(int, int);
void SendACK(void);
void SendNAK(void);
void getblock(void);
char * receive(char *, char *, char);
void invent_pkt_name(char *);
void get_call_list(void);
void online_users(int flag, int use_alias);
int fsend(char *, char);
int bad_call(int, int, int, int);
int logoff_procedure(void);
void update_user(void);
void show_controls(int);
int yesno_question(int);
int full_read_message(int, int);
int squish_full_read_message(int, int);
void fullread_change(void);
int get_class(int);
void activate_snoop(void);
int scan_mailbox(void);
void mail_read_forward(int);
void mail_read_backward(int);
void open_outside_door(char *);
void outside_door(char *);
void mtask_find(void);
int dv_get_version(void);
int ddos_active(void);
int tv_get_version(void);
int ml_active(void);
int windows_active(void);
void dv_pause(void);
void ddos_pause(void);
void tv_pause(void);
void ml_pause(void);
void msdos_pause(void);
void windows_pause(void);
void os2_pause(void);
void dos_break_off(void);
void com_baud(long);
int TIMED_READ(int);
int com_install(int);
void time_release(void);
void deactivate_snoop(void);
void MNP_Filter(void);
void firing_up(void);
void get_down(int, int);
void terminating_call(void);
long nodeexist(int, int, int);
char * firstchar(char *, char *, int);
void parse_netnode(char *, int *, int *, int *, int *);
void parse_netnode2(char *, int *, int *, int *, int *);
word xcrc(unsigned int, unsigned char);
void parse_command_line(int, char **);
void sysop_error(void);
void Janus(void);
int respond_to_file_requests(int);
int record_reqfile(char *);
void z_message(char *);
void f1_status(void);
void f2_status(void);
void f3_status(void);
void f4_status(void);
void f5_status(void);
void f6_status(void);
void f7_status(void);
void f8_status(void);
void f9_status(void);
void f10_status(void);
void clear_status(void);
void send_online_message(int use_alias);
void set_useron_record(int, int, int);
void cb_chat(void);
int check_multilogon(char *);
int write_message_text(int, int, char *, FILE *);
int squish_write_message_text(int, int, char *, FILE *);
void fossil_version(void);
void fossil_version2(void);
void system_crash(void);
void find_event(void);
void read_sched(void);
void write_sched(void);
int time_to_next(int);
void get_last_caller(void);
void set_last_caller(void);
int wait_for_connect(int);
void show_lastcallers(char *);
int quick_read_message(int msgnum, int flag, int);
void quick_scan_message_base(int board, int gold_board, int area, char upd);
void gather_origin_netnode(char *);
void quick_save_message(char *);
void squish_save_message(char *);
int external_editor(int);
int flag_file(int, int, int, int, int, int);
int load_language(int);
int select_language(void);
char * get_priv_text(int);
int seek_user_record(int, char *);
int MODEM_IN(void);
int MODEM_STATUS(void);
int PEEKBYTE(void);
void SENDBYTE(unsigned char);
void CLEAR_INBOUND(void);
void CLEAR_OUTBOUND(void);
void SENDCHARS(char *, unsigned int, int);
void BUFFER_BYTE(unsigned char);
void UNBUFFER_BYTES(void);
void MDM_ENABLE(unsigned);
void MDM_DISABLE(void);
unsigned Com_(char, ...);
void manual_poll(void);
void display_contents(void);
void quick_list_headers(int, int, int, char);
void squish_list_headers(int, int);
int FTSC_sender(int);
int FTSC_receiver(int);
void sysop_chatting(void);
void yelling_at_sysop(char *);
void pip_scan_message_base(int, char);
int pip_msg_read(unsigned int, unsigned int, char, int, int);
void pip_save_message(char *);
void pip_list_headers(int, int, int);
int quick_write_message_text(int, int, char *, FILE *);
void locate_files(int);
void mail_list(void);
void mail_read_nonstop(void);
int get_command_word(char *, int);
int get_entire_string(char *, int);
char get_command_letter(void);
int external_mailer(char *);
void dial_number(byte, char *);
char * HoldAreaNameMunge(int);
char * HoldAreaNameMungeCreate(int);
void keyboard_password(void);
void set_security(void);
void bulletin_menu(char *);
void translate_filenames(char *, char, char *);
long crc_name(char *);
void override_path(void);
void cb_who_is_where(int);
void append_backslash(char *);
long get_flags(char *);
int pip_write_message_text(int, int, char *, FILE *);
void read_comment(void);
void message_to_next_caller(void);
int quick_mail_header(int, int, char, int);
int pip_mail_list_header(int, int, int, int);
int squish_mail_list_headers(int, int, int);
void broadcast_message(char *);
void hurl(void);
int read_system2(int, int, struct _sys *);
void message_inquire(void);
void tag_area_list(int, int);
void pack_tagged_areas(void);
void resume_transmission(void);
void quick_message_inquire(char *, int, char);
void write_qwk_string(char *, char *, int *, int *, FILE *);
void text_header(struct _msg *, int, FILE *);
unsigned char color_chat(int);
void display_usage(void);
void hotkey_change(void);
void show_account(void);
void deposit_time(void);
void withdraw_time(void);
void deposit_kbytes(void);
void withdraw_kbytes(void);
char * stristr(char *, char *);
void bbs_long_list(int);
void bbs_short_list(int);
void bbs_add_list(int);
int bbs_list(int, struct _bbslist *);
void qwk_header(struct _msg *, struct QWKmsghd *, int, FILE *, long *);
void ljstring(char * dest, char * src, int len);
void qwk_pack_tagged_areas(void);
void bbs_remove(int, int);
void bbs_change(int, int);
void time_statistics(void);
void getrep(void);
int squish_kill_message(int);
int quick_kill_message(int msg_num, char goldbase);
int pip_kill_message(int);
void squish_message_inquire(char *);
int poll_galileo(int, int);
void vote_user(int);
void ballot_votes(void);
void get_emsi_id(char *, int);
void file_kill(int, char *);
void ibmset_change(void);
void import_mail(void);
void import_tic_files(void);
int quick_save_message2(FILE *, FILE *, FILE *, FILE *, FILE *);
int pip_save_message2(FILE *, FILE *, FILE *, FILE *);
int squish_save_message2(FILE *);
void display_percentage(int, int);
float IEEToMSBIN(float f);
int quick_scan_messages(int * total, int * personal, int board, int start, int fdi, int fdp, int totals, FILE * fpt, char qwk, char goldbase);
int squish_scan_messages(int *, int *, int, int, int, int, FILE *, char);
int pip_scan_messages(int *, int *, int, int, int, int, int, FILE *, char);
int is_here(int, int, int, int, struct _fwrd *, int);
int mail_sort_func(const void * a1, const void * b1);
void export_mail(int);
int fido_export_mail(int, struct _fwrd *);
int squish_export_mail(int, struct _fwrd *);
int quick_export_mail(int maxnodes, struct _fwrd * forward, int goldbase);
int pip_export_mail(int, struct _fwrd *);
void quick_read_sysinfo(int);
void process_startup_mail(int);
char * random_origins(void);
void display_new_area_list(int);
void xport_message(void);
void editor_door(char *);
int fido_export_netmail(void);
void build_nodelist_index(int);
void hslink_protocol(char *, char *, int);
void puma_protocol(char *, char *, int, int);
void read_hourly_files(void);
void resume_blanked_screen(void);
void bbs_list_download(int);
void restore_last_color(void);
void select_group(int, int);
void display_outbound_info(int);
void toss_system(void);
void idle_system(void);
void scan_system(void);
void unpack_system(void);
void pack_system(void);
void nlcomp_system(void);
void dial_system(void);
void blank_system(void);
void begin_blanking(void);
void stop_blanking(void);
void blank_progress(void);
void setup_video_interrupt(char *);
int scrollbox(int wsrow, int wscol, int werow, int wecol, int count, int direction);
void show_statistics(long);
void restore_video_interrupt(void);
char * n_frproc(char *, int *, int);
int prep_match(char *, char *);
int match(char *, char *);
int send_WaZOO(int);
void packet_bundle(char *, int);
void mail_report(char *, char *, long, int);
int get_emsi_handshake(int);
int send_emsi_handshake(int);
void setup_bbs_screen(void);
void clown_clear(void);
char * translate_ansi(char *);
void message_setup(void);
void file_setup(void);
void import_areasbbs(void);
void filetransfer_system(void);
void terminal_emulator(void);
void release_timeslice(void);
void file_attach(void);
void file_request(void);
void add_packet_pw(struct _pkthdr2 * pkthdr);
void blank_video_interrupt(void);
void download_filebox(int);
void upload_filebox(void);
int sh_open(char * file, int shmode, int omode, int fmode);
FILE * sh_fopen(char * filename, char * access, int shmode);
void update_filesio(int, int);
void initialize_modem(void);

