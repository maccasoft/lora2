#include <stdio.h>
#include <dos.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "version.h"

char *log_name, *availist, *about, *text_path, *password;
char *rookie, *welcome, *newuser2, *sysop;
char *system_name, *hold_area, *filepath, *request_list;
char *dateformat, *timeformat;
char *request_template = "%s%04x%04x.REQ", *ipc_path;
char *ext_flags = "ODCHI", *fido_msgpath, cmd_string[MAX_CMDLEN];
char *ext_mail_cmd, usr_rip = 0;
char *norm_about, *norm_availist, e_input[128], *answer, *mdm_flags;
char *norm_request_list, *prot_request_list, *prot_availist, *prot_about;
char *know_request_list, *know_availist, *know_about;
char area_change_key[3];
char *pip_msgpath;
char *location, *phone, *flags;
char *bad_msgs, *dupes, *netmail_dir, *modem_busy, *puma_exe, *hslink_exe;

char *ONLINE_MSGNAME = "%sLINE%d.BBS";
char *USERON_NAME    = "%sUSERON.BBS";
char *CBSIM_NAME     = "%sCB%d.BBS";
char *SYSMSG_PATH    = "%sSYSMSG.DAT";

char use_tasker, local_mode, user_status, frontdoor;

byte vote_priv, up_priv, down_priv, vote_limit;

int local_kbd, assumed, have_dv, target_up, target_down, max_kbytes;
int write_screen, speed_graphics, com_port, know_max_kbytes, prot_max_kbytes;
int registered, max_requests, next_call, max_call, norm_max_kbytes;
int no_logins, status, mainview, line_offset, aftermail_exit;
int isOriginator, CurrentReqLim, locked = 1, blank_timer;
int made_request, function_active, aftercaller_exit, totalmsg;
int norm_max_requests, prot_max_requests, know_max_requests;
int remote_zone, remote_net, remote_node, remote_point, have_os2;
int called_zone, called_net, called_node, max_readpriv, msg_parent, msg_child;

char have_ml, have_tv, have_ddos;

long cps = 0L, keycode = 0L, totaltime;

struct _configuration *config = NULL;
struct _sysinfo sysinfo;
struct _linestat linestat;
struct _call_list far call_list[MAX_OUT];
struct _lastcall lastcall;
struct _lorainfo lorainfo;

MSG *sq_ptr;

unsigned int comm_bits = BITS_8;
unsigned int parity = NO_PARITY;
unsigned int stop_bits = STOP_1;
unsigned int carrier_mask = 0x80;
unsigned int handshake_mask =  USE_XON | USE_CTS;

char fossil_buffer[128];
char out_buffer[128];
char *fossil_fetch_pointer = fossil_buffer;
char *out_send_pointer = out_buffer;
int fossil_count = 0;
int out_count = 0;
int old_fossil = 1;
char ctrlc_ctr;

byte answer_flag;
byte lock_baud;
byte terminal, emulator;
byte got_arcmail;
byte caller = 0;
byte nopause = 0;
byte recv_ackless = 0;
byte send_ackless = 0;
byte sliding;
byte small_window = 0;
byte ackless_ok;
byte no_overdrive = 0;
byte do_chksum;
byte may_be_seadog;
byte did_nak;
byte netmail;
byte who_is_he;
byte overwrite;
byte allow_reply;
byte snooping;
long rate, speed;
char *init;
char *dial;
struct _usr usr;
struct _sys sys;
FILE *logf;
long start_time;
int  allowed;
byte usr_class;

char Rxhdr[4];
char Txhdr[4];
long Rxpos;
int Txfcs32;
int Crc32t;
int Crc32;
int Znulls;
int Rxtimeout;
int Rxframeind;
byte *Filename;
word z_size;
byte Resume_WaZOO;
byte Resume_name[13];
byte Resume_info[48];
byte Abortlog_name[PATHLEN];
byte lastsent;
byte *Secbuf;
byte *Txbuf;
long file_length;
word remote_capabilities;
int fsent;
char remote_password[30];
struct _node nodelist;
int num_msg;
int first_msg;
int last_msg;
int lastread;
struct _msg msg;
int msg_tzone, msg_tpoint, msg_fzone, msg_fpoint;
char *messaggio[MAX_MSGLINE];
int errs, real_errs;
word block_number, base_block;
int block_size, fsize1, fsize2, first_block;
char final_name[50], *_fpath;

struct _msg_list *msg_list;
int max_priv_mail, last_mail;

char *mesi[12];

char *mtext [] = {
   "Jan", "Feb", "Mar", "Apr", "May", "Jun",
   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

struct parse_list levels[] = {
   TWIT, "Twit",
   DISGRACE, "Disgrace",
   LIMITED, "Limited",
   NORMAL, "Normal",
   WORTHY, "Worthy",
   PRIVIL, "Privel",
   FAVORED, "Favored",
   EXTRA, "Extra",
   CLERK, "Clerk",
   ASSTSYSOP, "Asst. Sysop",
   SYSOP, "Sysop",
   HIDDEN, "Hidden"
};

char *wtext [] = {
   "Sun",
   "Mon",
   "Tue",
   "Wed",
   "Thu",
   "Fri",
   "Sat"
};

char *mday[] = {
   "January",
   "February",
   "March",
   "April",
   "May",
   "June",
   "July",
   "August",
   "September",
   "October",
   "November",
   "December"
};

unsigned long far cr3tab[] = {                /* CRC polynomial 0xedb88320 */
	0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L, 0x706af48fL, 0xe963a535L, 0x9e6495a3L,
	0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L,
	0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL, 0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L,
	0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L,
	0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L, 0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
	0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L,
	0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L, 0xb8bda50fL,
	0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L, 0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL,
	0x76dc4190L, 0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L,
	0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
	0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L,
	0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L,
	0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L, 0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL,
	0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L,
	0x5005713cL, 0x270241aaL, 0xbe0b1010L, 0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
	0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL,
	0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L,
	0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L, 0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L,
	0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L,
	0xfed41b76L, 0x89d32be0L, 0x10da7a5aL, 0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
	0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL,
	0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL, 0x4669be79L,
	0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L, 0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL,
	0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL,
	0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
	0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L,
	0x86d3d2d4L, 0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L,
	0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL, 0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L,
	0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL,
	0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
	0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L, 0xcdd70693L, 0x54de5729L, 0x23d967bfL,
	0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL
};

unsigned short far crctab[256] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
	0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
	0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
	0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
	0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
	0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
	0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
	0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
	0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
	0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
	0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
	0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
	0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
	0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
	0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
	0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
	0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
	0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
	0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
	0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
	0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
	0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};


char hex[] = "0123456789abcdef";

EVENT *e_ptrs[64];
int requests_ok = 1, num_events = 0, cur_event = -1, next_event = 0;
int got_sched = 0, noforce = 0, max_connects = 3;
int max_noconnects = 10000, matrix_mask = 0, no_requests = 0;
int no_resync = 0, no_sealink = 0;

char *prodcode[] = {
   "Fido",
   "Rover",
   "SEAdog",
   "",
   "Slick/150",
   "Opus",
   "Dutchie",
   "",
   "Tabby",
   "Hoster",
   "Wolf/68k",
   "QMM",
   "FrontDoor",
   "",
   "",
   "",
   "",
   "MailMan",
   "OOPS",
   "GS-Point",
   "BGMail",
   "CrossBow",
   "",
   "",
   "",
   "BinkScan",
   "D'Bridge",
   "BinkleyTerm",
   "Yankee",
   "FGet/FSend",
   "Daisy,Apple ][",
   "Polar Bear",
   "The-Box",
   "STARgate/2",
   "TMail",
   "TCOMMail",
   "Bananna",
   "RBBSMail",
   "Apple-Netmail",
   "Chameleon",
   "Majik Board",
   "QMail",
   "Point And Click",
   "Aurora Three Bundler",
   "FourDog",
   "MSG-PACK",
   "AMAX",
   "Domain Communication System",
   "LesRobot",
   "Rose",
   "Paragon",
   "BinkleyTerm",
   "StarNet",
   "ZzyZx",
   "QEcho",
   "BOOM",
   "PBBS",
   "TrapDoor",
   "Welmat",
   "NetGate",
   "Odie",
   "Quick Gimme",
   "dbLink",
   "TosScan",
   "Beagle",
   "Igor",
   "TIMS",
   "Isis",
   "AirMail",
   "XRS",
   "Juliet Mail System",
   "Jabberwocky",
   "XST",
   "MailStorm",
   "BIX-Mail",
   "IMAIL",
   "FTNGate",
   "RealMail",
   "LoraBBS",
   "TDCS",
   "InterMail",
   "RFD",
   "Yuppie!",
   "EMMA",
   "QBoxMail",
   "Number 4",
   "Number 5",
   "GSBBS",
   "Merlin",
   "TPCS",
   "Raid",
   "Outpost",
   "Nizze",
   "Armadillo",
   "rfmail",
   "Msgtoss",
   "InfoTex",
   "GEcho",
   "CDEhost",
   "Pktize",
   "PC-RAIN",
   "Truffle",
   "Foozle",
   "White Pointer",
   "GateWorks",
   "Portal of Power",
   "MacWoof",
   "Mosaic",
   "TPBEcho",
   "HandyMail",
   "EchoSmith",
   "FileHost",
   "SFScan",
   "Benjamin",
   "RiBBS",
   "MP",
   "Ping",
   "Door2Europe",
   "SWIFT",
   "WMAIL",
   "RATS",
   "Harry the Dirty Dog",
   "Maximus-CBCS",
   "SwifEcho",
   "GCChost",
   "RPX-Mail",
   "Tosser",
   "TCL",
   "MsgTrack",
   "FMail",
   "Scantoss",
   "Point Manager",
   "Dropped",
   "Simplex",
   "UMTP",
   "Indaba",
   "Echomail Engine",
   "DragonMail",
   "Prox",
   "Tick",
   "RA-Echo",
   "TrapToss",
   "Babel",
   "UMS",
   "RWMail",
   "WildMail",
   "AlMAIL",
   "XCS",
   "Fone-Link",
   "Dogfight",
   "Ascan",
   "FastMail",
   "DoorMan",
   "PhaedoZap",
   "SCREAM",
   "MoonMail",
   "Backdoor",
   "MailLink",
   "Mail Manager",
   "Black Star",
   "Bermuda",
   "PT",
   "UltiMail",
   "GMD",
   "FreeMail",
   "Meliora",
   "Foodo",
   "MSBBS",
   "Boston BBS",
   "XenoMail",
   "XenoLink",
   "ObjectMatrix",
   "Milquetoast",
   "PipBase",
   "EzyMail",
   "FastEcho",
   "IOS",
   "Communique",
   "PointMail",
   "Harvey's Robot",
   "2daPoint",
   "CommLink",
   "fronttoss",
   "SysopPoint",
   "PTMAIL",
   "AECHO",
   "DLGMail",
   "GatePrep",
   "Spoint",
   "TurboMail",
   "FXMAIL",
   "NextBBS",
   "EchoToss",
   "SilverBox",
   "MBMail",
   "SkyFreq",
   "ProMailer",
   "Mega Mail",
   "YaBom",
   "TachEcho",
   "XAP",
   "EZMAIL",
   "Arc-Binkley",
   "Roser",
   "UU2",
   "NMS",
   "BBCSCAN",
   "XBBS",
   "LoTek Vzrul",
   "Private Point Project",
   "NoSnail",
   "SmlNet",
   "STIR",
   "RiscBBS",
   "Hercules",
   "AMPRGATE",
   "BinkEMSI",
   "EditMsg",
   "Roof",
   "QwkPkt",
   "MARISCAN",
   "NewsFlash",
   "Paradise",
   "DogMatic-ACB",
   "T-Mail",
   "JetMail",
   "MainDoor"
};

char *msgtxt[] = {
/*M_GRUNGED_HEADER         */        "!Grunged hdr",
/*M_ILLEGAL_CARRIER        */        "+Sendind F/Req. report",
/*M_UNKNOWN_LINE           */        "Unknown or illegal config line: %s\n",
/*-_GIVEN_LEVEL            */        "#Given %d mins (%s)",
/*-_PRESS_ESCAPE           */        "\rPress <Escape> to enter BBS.\r",
/*-_NO_BBS                 */        "\r\rProcessing Mail. Please hang up.\r\r",
/*-_NOTHING_TO_SEND        */        "+Nothing to send to %d:%d/%d.%d",
/*-_CONNECT_ABORTED        */        "!Connection attempt aborted",
/*M_MODEM_HANGUP           */        "#Modem hang up sequence",
/*M_NO_OUT_REQUESTS        */        ":No outgoing file requests",
/*M_OUT_REQUESTS           */        ":Outbound file requests",
/*M_END_OUT_REQUESTS       */        ":End of outbound file requests",
/*-_FREQ_DECLINED          */        "*File Requests declined",
/*-_ADDRESS                */        "\r\r* Network Address %d:%d/%d.%d Using %s\n\n",
/*-_NOBODY_HOME            */        "*Sensor doesn't report intelligent life",
/*-_NO_CARRIER             */        "*Lost Carrier",
/*-_PROTECTED_SESSION      */        "*Password-protected session",
/*-_PWD_ERROR              */        "!Password Error from (%d:%d/%d.%d): His='%s' Ours='%s'",
/*-_CALLED                 */        "!Called %d:%d/%d and got %d:%d/%d",
/*-_WAZOO_METHOD           */        ":WaZOO method: %s",
/*-_WAZOO_END              */        "*End of WaZOO Session",
/*M_PACKET_MSG             */        " Sending Mail Packet",
/*M_OPEN_MSG               */        "Open",
/*M_KBD_MSG                */        " Keyboard Escape",
/*M_TRUNC_MSG              */        " File %s truncated",
/*-_RENAME_MSG             */        "+Dupe file renamed: %s",
/*M_DEVICE_MSG             */        " Open Character Device",
/*M_FUBAR_MSG              */        " Too many errors",
/*M_UNLINKING_MSG          */        " File %s deleted",
/*M_CAN_MSG                */        " Transfer cancelled",
/*-_NO_CTL_FILE            */        "Couldn't open CFG file: %s",
/*-_FOSSIL_TYPE            */        "  FOSSIL: %Fs",
/*-_STARTING_EVENT         */        ":Starting Event %d",
/*M_EVENT_EXIT             */        "#Exit at start of event with errorlevel %d",
/*M_BBS_EXIT               */        ":Message Area #%d %s",
/*M_BBS_SPAWN              */        ":File Area #%d %s",
/*M_EXT_MAIL               */        "Shutdown",
/*M_SETTING_BAUD           */        "Idle",
/*-_REMOTE_USES            */        "*Remote Uses %s",
/*M_VERSION                */        "Version",
/*M_PROGRAM                */        "Program",
/*M_SEND_FALLBACK          */        "*Sending mail using FTS-0001 compatible fallback",
/*M_REFUSING_IN_FREQ       */        "*Refusing inbound file requests",
/*M_TOO_LONG               */        "!Tired of waiting for other end.",
/*M_0001_END               */        "*End of FTS-0001 compatible session",
/*M_RECV_FALLBACK          */        "*Receiving mail using FTS-0001 compatible fallback",
/*M_GIVING_MAIL            */        "*Giving mail to %d:%d/%d.%d",
/*M_REFUSE_PICKUP          */        "*Node %s refused to pickup mail",
/*M_MEM_ERROR              */        "!Memory overflow error",
/*M_OUTBOUND               */        "Outbound",
/*M_FILE_ATTACHES          */        "File Attaches",
/*M_MAKING_FREQ            */        "*Making file request",
/*M_END_OF                 */        "End of",
/*M_RECV_MAIL              */        "*Receiving inbound mail",
/*M_NO_PICKUP              */        "*Pickup is turned off - refusing mail.",
/*M_INBOUND                */        "Inbound",
/*M_MAIL_PACKET            */        "Mail Packet",
/*M_PWD_ERR_ASSUMED        */        "!Password Error assumed",
/*M_CANT_RENAME_MAIL       */        "!Mail Packet '%s' cannot be renamed",
/*M_MAIL_PACKET_RENAMED    */        "!Mail Packet renamed to '%s'",
/*-_PROCESSING_NODE        */        "*Processing %d:%d/%d -- %s",
/*M_NO_ADDRESS             */        "!Couldn't find Address: %d:%d/%d",
/*-_UNABLE_TO_OPEN         */        "!Unable to open %s",
/*M_NODELIST_MEM           */        "!Insufficient memory",
/*M_FILE_REQUESTS          */        "File Requests",
/*M_MATCHING_FILES         */        ":%d matching files sent",
/*M_INCOMING_CALL          */        ":Incoming call, dial aborted",
/*-_EXIT_REQUEST           */        ":Exit requested from keyboard",
/*M_FUNCTION_KEY           */        ":Function key exit - errorlevel %d",
/*-_POLL_MODE              */        ":Entering POLL Mode",
/*-_POLL_COMPLETED         */        ":Poll completed",
#ifdef __OS2__
/*-_SHELLING               */        ":Invoking OS/2 shell",
#else
/*-_SHELLING               */        ":Invoking DOS shell",
#endif
/*-_TYPE_EXIT              */        "\nLoraBBS DOS Shell - Type EXIT To Return - Free RAM %ld\n",
/*-_BINKLEY_BACK           */        ":LoraBBS Reactivated",
/*M_NONE_EVENTS            */        "Scheduler empty",
/*-_READY_CONNECT          */        "+Connect %lu%s%s",
/*-_DIALING_NUMBER         */        ":Dialing %s",
/*-_INSUFFICIENT_DATA      */        ":Write message #%d",
/*M_END_OF_ATTEMPT         */        "+End of connection attempt",
/*M_EXIT_COMPRESSED        */        ":Exit after compressed mail with errorlevel %d",
/*-_EXIT_AFTER_MAIL        */        ":Exit after receiving mail with errorlevel %d",
/*M_PASSWORD_OVERRIDE      */        "!Password override for outgoing call",
/*-_SET_SECURITY           */        "New level: ",
/*-_PHONE_OR_NODE          */        "Please enter a net/node number: ",
/*-_NO_DROP_DTR            */        "!Unable to force carrier drop by dropping DTR!",
/*-_USER_CALLING           */        "+%s calling",
/*-_NOT_IN_LIST            */        "+%s isn't in user list",
/*-_BAD_PASSWORD           */        "!Bad password '%s'",
/*-_INVALID_PASSWORD       */        "!INVALID PASSWORD",
/*-_USER_LAST_TIME         */        ":User's last time: %s",
/*-_TIMEDL_ZEROED          */        ":Time/DL Zeroed",
/*M_YES                    */        "Yes",
/*M_NO                     */        "No",
/*M_DRATS                  */        "drats",
/*M_HE_HUNG_UP             */        "!Other end hung up on us <humph>.",
/*-_CORRECTED_ERRORS       */        "+Corrected %d errors in %ld blocks",
/*-_FILE_SENT              */        "+DL",
/*-_SYNCHRONIZING          */        "+Remote Synchronizing to Offset %ld",
/*M_TEMP_NOT_OPEN          */        "!Temporary receive file '%s' could not be opened",
/*-_ALREADY_HAVE           */        "+Already have %s",
/*M_SYNCHRONIZING_EOF      */        "+Synchronizing to End of File",
/*-_SYNCHRONIZING_OFFSET   */        "+Synchronizing to Offset %ld",
/*M_FILE_RECEIVED          */        "+UL",
/*M_ORIGINAL_NAME_BAD      */        "!Original name of %s could not be used",
/*M_UNEXPECTED_EOF         */        "!EOT not expected until block %ld",
/*M_REMOTE_SYSTEM          */        "*Remote System",
/*M_UNKNOWN_MAILER         */        "UNKNOWN - FTS-0001 Mailer",
/*-_SYSTEM_INITIALIZING    */        "System Initializing.  Please wait...\n",
/*-_UNRECOGNIZED_OPTION    */        "\nUnrecognized option: '%s'\a\n",
/*-_THANKS                 */        "\nThanks for using %s\n",
/*M_REMOTE_REFUSED         */        "+Remote refused %s",
/*M_ERROR                  */        "!Error",
/*M_CANT                   */        "Can't",
/*-_CPS_MESSAGE            */        "+CPS: %lu (%lu bytes)  Efficiency: %lu%%",
/*M_COMPRESSED_MAIL        */        "Compressed Mail",
/*M_NET_FILE               */        "Net File",
/*M_TROUBLE                */        "Trouble?",
/*M_RESENDING_FROM         */        "Resending from %s",
/*M_SEND_MSG               */        " Send %s (%ld %c-blks)",
/*M_UPDATE                 */        "Update",
/*M_FILE                   */        "File",
/*M_REQUEST                */        "Request",
/*M_EXECUTING              */        ":Executing",
/*M_CARRIER_REQUEST_ERR    */        "!Carrier lost, request(s) aborted",
/*M_FREQ_LIMIT             */        "!File Request limit exceeded",
/*M_EVENT_OVERRUN          */        "!Event Overrun - requests aborted",
/*-_NO_AVAIL               */        "No AVAIL list",
/*-_NO_ABOUT               */        "No ABOUT file",
/*-_OKFILE_ERR             */        "!OKFILE Error `%s'",
/*-_FREQ_PW_ERR            */        "!Request password error: '%s' '%s' '%s'",
/*M_RECEIVE_MSG            */        "Rcv %ld blks of %s from %s (%ld bytes)",
/*M_TIMEOUT                */        "Timeout",
/*M_CHECKSUM               */        "Checksum",
/*M_CRC_MSG                */        "CRC",
/*M_JUNK_BLOCK             */        "Junk Block",
/*M_ON_BLOCK               */        "on block",
/*M_FIND_MSG               */        "Find",
/*M_READ_MSG               */        "Read",
/*M_SEEK_MSG               */        "Seek",
/*M_SHRT_MSG               */        "Short Block",
/*M_CLOSE_MSG              */        "Close",
/*M_UNLINK_MSG             */        "Unlink",
/*M_WRITE_MSG              */        "Write",
/*M_SKIP_MSG               */        "!SKIP command received",
/*M_READ_FILE_LIST         */        "#Read File List `%s'",
/*M_NO_SYSTEM_FILE         */        "!No System file",
/*M_DL_PATH                */        "!DL-Path: `%s'",
/*M_INPUT_COMMAND          */        "\r  Write the keyboard password: ",
/*M_ELEMENT_CHOSEN         */        "\r  Please, reenter for safety : ",
/*M_DEBRIS                 */        "Debris",
/*M_LONG_PACKET            */        "Long pkt",
/*M_Z_IGNORING             */        "!Ignoring `%s'",
/*M_OUT_OF_DISK_SPACE      */        "!Out of disk space",
/*M_RECEIVING              */        "Receiving",
/*M_Z_INITRECV             */        "!Zmodem Init Problem %s",
/*M_BAD_POS                */        "bad position",
/*M_Z_RZ                   */        "!Zmodem Recv Problem %s",
/*M_J_BAD_PACKET           */        "Bad packet at %ld",
/*M_SEND                   */        "Send",
/*M_RECV                   */        "Recv",
/*M_OTHER_DIED             */        "!Other end died",
/*M_GOING_ONE_WAY          */        "!Dropping to one-way xfer",
/*M_REFUSING               */        "+Refusing %s",
/*M_UNKNOWN_PACKET         */        "!Unknown packet type %d",
/*M_SESSION_ABORT          */        "!Session aborted",
/*M_SENDING                */        "#Sending %s",
/*M_NO_LENGTH              */        "!Can't decode file length",
/*M_FINISHED_PART          */        "*Finished partial file %s",
/*M_SAVING_PART            */        "*Saving partial file %s",
/*M_REMOTE_CANT_FREQ       */        "*Remote can't handle file requests",
/*M_ORIGIN_LINE            */        " * Origin: %s (%d:%d/%d.%d)\r\n",
/*M_TEAR_LINE              */        "--- %s%s\r\n",
/*M_PID                    */        "\x01PID: %s%s\r\n",
/*M_INTL                   */        "\x01INTL %d:%d/%d %d:%d/%d\r\n",
/*M_TOPT                   */        "\x01TOPT %d\r\n",
/*M_MSGID                  */        "\x01MSGID: %d:%d/%d.%d %lx\r\n",
/*M_SIGNATURE              */        "... %s\r\n",
/*-_DRIVER_DEAD_1          */        "\nI'm sorry, there doesn't appear to be a FOSSIL driver loaded.\n",
/*-_DRIVER_DEAD_2          */        "Lora BBS requires a FOSSIL driver. Please take care of this before\n",
/*-_DRIVER_DEAD_3          */        "attempting to run Lora BBS again.\n",
/*M_FAILED_CREATE_FLAG     */        "!Could not create temp flagfile %s",
/*M_CREATED_FLAGFILE       */        " Created flagfile %s",
/*M_THIS_ADDRESS_LOCKED    */        " Other node sending to %d:%d/%d.%d",
/*M_BAD_CLEAR_FLAGFILE     */        "!Erroneous attempt to clear flag for %d:%d/%d.%d",
/*M_CLEARED_FLAGFILE       */        " Deleted flagfile %s",
/*M_FAILED_CLEAR_FLAG      */        "!Unable to delete flag file %s",
/*M_BYTE_LIMIT             */        "!Exceeded file request byte limit",
/*M_REFRESH_NODELIST       */        " Nodelist index refresh necessary",
/*-_NEXT_NONE              */        "None",
/*-_FILTER                 */        " MNP/V.42 filter",
/*-_NEXT_EVENT             */        "Event %d starts in %d:%02d hours  ",
/*-_DUMMY_PACKET           */        "+Sending dummy .OUT packet"
};

char **bbstxt;

char *protocols [] = {
   "XXmodem",
   "11k-Xmodem",
   "ZZmodem",
   "HHS/Link",
   "PPuma",
   "SSealink"
};

