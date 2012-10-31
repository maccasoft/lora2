typedef unsigned int bit;
typedef unsigned int word;
typedef unsigned char byte;
typedef long dword;


#ifndef __DOS_H
struct time {
    unsigned char ti_min;     /* Minutes */
    unsigned char ti_hour;    /* Hours */
    unsigned char ti_hund;    /* Hundredths of seconds */
    unsigned char ti_sec;     /* Seconds */
};

struct date {
    int  da_year;    /* Year - 1980 */
    char da_day;     /* Day of the month */
    char da_mon;     /* Month (1 = Jan) */
};
#endif


struct _stamp {
   unsigned int date;
   unsigned int time;
};

struct _noask {
   bit ansilogon :1;
   bit birthdate :1;
   bit voicephone:1;
   bit dataphone :1;
   bit emsi      :1;
   bit checkfile :1;
};

struct _votes {
   int  minvote;
   byte priv;
};

struct _call_list {
   int  zone;
   int  node;
   int  net;
   int  type;
   long size;
};

struct  _node {
   int  net;
   int  number;
   int  cost;
   char name[34];
   char phone[40];
   char city[30];
   char password[8];
   int  realcost;
   int  hubnode;
   char rate;
   char modem;
   word flags1;
   int  reserved;
};

struct _vers7 {
   int  Zone;
   int  Net;
   int  Node;
   int  HubNode;          /* If node is a point, this is point number. */
   word CallCost;         /* phone company's charge */
   word MsgFee;           /* Amount charged to user for a message */
   word NodeFlags;        /* set of flags (see below) */
   byte ModemType;        /* RESERVED for modem type */
   byte Phone_len;
   byte Password_len;
   byte Bname_len;
   byte Sname_len;
   byte Cname_len;
   byte pack_len;
   byte BaudRate;         /* baud rate divided by 300 */
};

struct _lastread {
   word area;
   int msg_num;
};

#define MAXLREAD   30
#define MAXDLREAD  10
#define MAXFLAGS   4
#define MAXCOUNTER 10

struct _usridx {
   long id;
};

struct _usr {
   char  name[36];
   char  handle[36];
   char  city[26];

   struct _lastread lastread[MAXLREAD];
   struct _lastread dynlastread[MAXDLREAD];

   char  pwd[16];
   dword times;
   word  nulls;
   word  msg;

   bit   avatar    :1;
   bit   color     :1;
   bit   scanmail  :1;
   bit   use_lore  :1;
   bit   more      :1;
   bit   ansi      :1;
   bit   kludge    :1;
   bit   formfeed  :1;

   bit   hotkey    :1;
   bit   tabs      :1;
   bit   full_read :1;
   bit   badpwd    :1;
   bit   usrhidden :1;
   bit   nokill    :1;
   bit   ibmset    :1;
   bit   deleted   :1;

   byte  language;
   byte  priv;
   long  flags;
   char  ldate[20];
   int   time;
   dword upld;
   dword dnld;
   int   dnldl;
   word  n_upld;
   word  n_dnld;
   word  files;
   word  credit;
   word  dbase;
   char  signature[58];
   char  voicephone[20];
   char  dataphone[20];
   char  birthdate[10];
   char  subscrdate[10];
   char  firstdate[20];
   char  lastpwdchange[10];
   long  ptrquestion;
   byte  len;
   byte  width;
   long  id;
   int   msgposted;
   char  comment[80];
   byte  help;
   word  baud_rate;
   byte  counter[MAXCOUNTER];
   word  chat_minutes;

   bit   xfer_prior:1;
   bit   quiet     :1;
   bit   nerd      :1;
   bit   donotdisturb:1;
   bit   robbed    :1;
   bit   novote    :1;

   char  protocol;
   char  archiver;

   struct _ovr_class_rec {
      int  max_time;
      int  max_call;
      int  max_dl;
      word ratio;
      word min_baud;
      word min_file_baud;
      word start_ratio;
   } ovr_class;

   int   sig;
   word  account;
   word  f_account;
   int   votes;

   char  extradata[28];
};

#define SIZEOF_MSGAREA    224
#define SIZEOF_FILEAREA   225
#define SIZEOF_DBASEAREA  119

struct _sys_idx {
   byte priv;
   long flags;
   char key[13];
   word area;
};

struct _sys {
   char msg_name[70];
   int  msg_num;
   char msg_path[40];
   char origin[56];
   bit  echomail  :1;
   bit  netmail   :1;
   bit  public    :1;
   bit  private   :1;
   bit  anon_ok   :1;
   bit  no_matrix :1;
   bit  squish    :1;
   word msg_sig;
   char echotag[32];
   word pip_board;
   byte quick_board;
   byte msg_priv;
   long msg_flags;
   byte write_priv;
   long write_flags;
   byte use_alias;
   int  max_msgs;
   int  max_age;
   int  age_rcvd;

   char file_name[70];
   int  file_num;
   char uppath[40];
   char filepath[40];
   char filelist[50];
   bit  freearea  :1;
   bit  norm_req  :1;
   bit  know_req  :1;
   bit  prot_req  :1;
   bit  nonews    :1;
   bit  no_global_search :1;
   bit  no_filedate :1;
   word file_sig;
   byte file_priv;
   long file_flags;
   byte download_priv;
   long download_flags;
   byte upload_priv;
   long upload_flags;
   byte list_priv;
   long list_flags;
};

#define  TWIT        0x10
#define  DISGRACE    0x20
#define  LIMITED     0x30
#define  NORMAL      0x40
#define  WORTHY      0x50
#define  PRIVIL      0x60
#define  FAVORED     0x70
#define  EXTRA       0x80
#define  CLERK       0x90
#define  ASSTSYSOP   0xA0
#define  SYSOP       0xB0
#define  HIDDEN      0xC0

/*------------------------------------------------------------------------*/
/*  Livelli di aiuto                                                      */
/*------------------------------------------------------------------------*/
#define  EXPERT      0x02
#define  REGULAR     0x04
#define  NOVICE      0x06

struct _msg {
   char from[36];
   char to[36];
   char subj[72];
   char date[20];
   word times;
   int  dest;
   int  orig;
   int  cost;
   int  orig_net;
   int  dest_net;
   struct _stamp date_written;
   struct _stamp date_arrived;
   int  reply;
   word attr;
   int  up;
};

#define MSGPRIVATE 0x0001
#define MSGCRASH   0x0002

#define MSGREAD    0x0004
#define MSGSENT    0x0008
#define MSGFILE    0x0010
#define MSGFWD     0x0020
#define MSGORPHAN  0x0040
#define MSGKILL    0x0080
#define MSGLOCAL   0x0100
#define MSGHOLD    0x0200
#define MSGXX2     0x0400
#define MSGFRQ     0x0800
#define MSGRRQ     0x1000
#define MSGCPT     0x2000
#define MSGARQ     0x4000
#define MSGURQ     0x8000

struct _sysinfo {
   long quote_position;
   unsigned long total_calls;
   char pwd[40];
   unsigned long local_logons;
   unsigned long calls_300;
   unsigned long calls_1200;
   unsigned long calls_2400;
   unsigned long calls_9600;
   unsigned long calls_14400;
   unsigned long failed_password;
   unsigned long normal_logoff;
   unsigned long lost_download;
   unsigned long lost_carrier;
   unsigned long out_of_time;
   unsigned long idle_logoff;
   unsigned long sysop_logoff;
   unsigned long new_users;
   unsigned long file_ratio;
   unsigned long kbytes_ratio;
   unsigned long minutes_online;
   unsigned long mailer_online;
};

struct _linestat {
   int  line;
   char startdate[10];
   word busyperhour[24];
   word busyperday[7];
};

#define BROWSING     1
#define UPLDNLD      2
#define READWRITE    3
#define DOOR         4
#define CHATTING     5
#define QUESTIONAIRE 6

struct _useron {
   char name[36];
   int  line;
   word baud;
   char city[26];
   bit  donotdisturb: 1;
   bit  priv_chat   : 1;
   int  status;
   int  cb_channel;
};

struct _lastcall {
   int  line;
   char name[36];
   char city[26];
   word baud;
   long times;
   char logon[6];
   char logoff[6];
};

struct class_rec {
   int  priv;
   int  max_time;
   int  max_call;
   int  max_dl;
   int  dl_300;
   int  dl_1200;
   int  dl_2400;
   int  dl_9600;
   word ratio;
   word min_baud;
   word min_file_baud;
   word start_ratio;
};

struct parse_list {
   int p_length;
   char *p_string;
};


struct _cmd {
   char  name[80];
   byte  priv;
   long  flags;
   int   flag_type;
   bit   automatic  :1;
   bit   nocls      :1;
   bit   first_time :1;
   bit   no_display :1;
   byte  color;
   byte  first_color;
   byte  hotkey;
   char  argument[50];
};

struct _alias {
   int  zone;
   int  net;
   int  node;
   int  point;
   word fakenet;
   char *domain;
};

#define HEADER_NAMESIZE 15

struct zero_block {
   long size;                          /* file length                    */
   long time;                          /* file date/time stamp           */
   char name[HEADER_NAMESIZE];         /* original file name             */
   char moi[15];                       /* sending program name           */
   char noacks;                        /* for SLO                        */
};

/*--------------------------------------------------------------------------*/
/* Message packet header                                                    */
/*--------------------------------------------------------------------------*/

#define PKTVER       2

 /*--------------------------------------------*/
 /* POSSIBLE VALUES FOR `product' (below)      */
 /* */
 /* NOTE: These product codes are assigned by  */
 /* the FidoNet<tm> Technical Stardards Com-   */
 /* mittee.  If you are writing a program that */
 /* builds packets, you will need a product    */
 /* code.  Please use ZERO until you get your  */
 /* own.  For more information on codes, write */
 /* to FTSC at 115/333.                        */
 /* */
 /*--------------------------------------------*/
#define isOPUS       5

struct _pkthdr
{
   int orig_node;          /* originating node               */
   int dest_node;          /* destination node               */
   int year;               /* 0..99  when packet was created */
   int month;              /* 0..11  when packet was created */
   int day;                /* 1..31  when packet was created */
   int hour;               /* 0..23  when packet was created */
   int minute;             /* 0..59  when packet was created */
   int second;             /* 0..59  when packet was created */
   int rate;               /* destination's baud rate        */
   int ver;                /* packet version                 */
   int orig_net;           /* originating network number     */
   int dest_net;           /* destination network number     */
   char product;           /* product type                   */
   char serial;            /* serial number (some systems)   */

   /* ------------------------------ */
   /* THE FOLLOWING SECTION IS NOT   */
   /* THE SAME ACROSS SYSTEM LINES:  */
   /* ------------------------------ */

   byte password[8];       /* session/pickup password        */
   int  orig_zone;         /* originating zone               */
   int  dest_zone;         /* Destination zone               */
   byte B_fill2[16];
   long B_fill3;
};

#define YOOHOO 0x00F1
#define TSYNC  0x00AE

struct   _Hello {
   word signal;
   word hello_version;
   word product;
   word product_maj;
   word product_min;
   char my_name[60];
   char sysop[20];
   word my_zone;
   word my_net;
   word my_node;
   word my_point;
   char my_password[8];
   byte reserved2[8];
   word capabilities;
   long tranx;             /* Used only by LoraBBS >= 2.10 */
   byte reserved3[8];
};

struct _ndi {
   int   node;
   int   net;
};

struct _msg_list {
   word area;
   word msg_num;
};

struct _mail {
//   char to[36];
   long to;
   word area;
   word msg_num;
};

struct _lorainfo
{
   word baud;
   int  port;
   int  time;
   char log[40];
   char system[80];
   char sysop[36];
   word task;
   word max_baud;
   long posuser;
   word yelling;
   word downloadlimit;
   int  max_time;
   int  max_call;
   word ratio;
   word min_baud;
   word min_file_baud;
   bit  wants_chat  :1;
   bit  mnp_connect :1;
   bit  keylock     :1;
   char logindate[20];
   char password[40];
   unsigned long total_calls;
};

typedef struct mnums
{
   byte mdm;
   char pre[50];
   char suf[50];
   struct mnums *next;
} MDM_TRNS;

struct _bbslist {
   char bbs_name[40];
   char sysop_name[36];
   char number[20];
   word baud;
   char open_times[12];
   char net[16];
   char bbs_soft[16];
   char other[60];
   char extra_space[100];
};

struct QWKmsghd {               /* Messages.dat header */
   char  Msgstat;               /* message status byte */
   char  Msgnum[7];             /* message number in ascii */
   char  Msgdate[8];            /* date in ascii */
   char  Msgtime[5];            /* time in ascii */
   char  Msgto[25];             /* Person addressed to */
   char  Msgfrom[25];           /* Person who wrote message */
   char  Msgsubj[25];           /* Subject of message */
   char  Msgpass[12];           /* password */
   char  Msgrply[8];            /* message # this refers to */
   char  Msgrecs[6];               /* # of 128 byte recs in message,inc header */
   char  Msglive;                  /* always 0xe1 */
   char  Msgarealo;             /* area number, low byte */
   char  Msgareahi;             /* area number, hi byte */
   char  Msgfiller[3];          /* fill out to 128 bytes */
};


