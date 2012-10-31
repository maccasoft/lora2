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

struct _votes {
   int  minvote;
   byte priv;
};

#define NO_CARRIER      -3
#define NO_DIALTONE     -6
#define BUSY            -7
#define NO_ANSWER       -8
#define VOICE           -9
#define ABORTED         -50
#define TIMEDOUT        -51

struct _call_list {
   int   zone;
   int   node;
   int   net;
   int   point;
   int   type;
   long  size;
   int   call_nc;
   byte  call_wc;
   int   flags;
   int   n_mail;
   int   n_data;
   long  b_mail;
   long  b_data;
   unsigned short ftime;
   unsigned short fdate;
};

struct  _node {
   char name[36];
   char phone[40];
   char city[30];
   char password[20];
   char rate;
   char modem;
   int  cost_first;
   int  time_first;
   int  cost;
   int  time;
   char pw_areafix[20];
   char pw_tic[20];
   char sysop[36];
   char akainfo;
};

struct _lastread {
   word area;
   int msg_num;
};

#define MAXLREAD   50
#define MAXDLREAD  20
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
   bit   havebox   :1;
   bit   security  :1;

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

   int   msg_sig;
   word  account;
   word  f_account;
   int   votes;
   int   file_sig;

   char  extradata[290];
};

#define SIZEOF_MSGAREA    512
#define SIZEOF_FILEAREA   640
#define SIZEOF_DBASEAREA  119

struct _sys_idx {
   byte priv;
   long flags;
   char key[13];
   word area;
   word sig;
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
   bit  kill_unlisted :1;
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
   char forward1[80];
   char forward2[80];
   char forward3[80];
   bit  msg_restricted :1;
   bit  passthrough    :1;
   bit  internet_mail  :1;
   byte areafix;
   char qwk_name[14];
   long afx_flags;
   word goldboard;
   char filler1[27];

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
   bit  file_restricted :1;
   word file_sig;
   byte file_priv;
   long file_flags;
   byte download_priv;
   long download_flags;
   byte upload_priv;
   long upload_flags;
   byte list_priv;
   long list_flags;
   char filler2[10];
   char short_name[13];
   char filler3[8];
   char tic_tag[32];
   char tic_forward1[80];
   char tic_forward2[80];
   char tic_forward3[80];
   byte tic_level;
   long tic_flags;
   bit  cdrom     :1;
   char filler4[106];
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

struct _daystat {
   // Flag di validita' della statistica
   long timestamp;

   // Numero di chiamate e trasmissione/ricezione files
   long incalls;
   long outcalls;
   long humancalls;
   long filesent;
   long bytesent;
   long filerequest;
   long filereceived;
   long bytereceived;
   long completed;
   long failed;
   long cost;

   // Statistico messaggi ricevuti/mandati
   long emailreceived;
   long emailsent;
   long echoreceived;
   long echosent;
   long dupes;
   long bad;

   // Tempi di esecuzione del programma
   long exectime;
   long idletime;
   long interaction;
   long echoscan;
   long inconnects;
   long outconnects;
   long humanconnects;
};

struct _sysinfo {
   long stamp_up;
   long stamp_down;
   long quote_position;
   long total_calls;
   char pwd[40];
   struct _daystat today;
   struct _daystat yesterday;
   struct _daystat week;
   struct _daystat month;
   struct _daystat year;
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
   long timestamp;
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
   char my_name[58];
   word serial_no;         /* Used only by LoraBBS >= 2.31 */
   char sysop[20];
   word my_zone;
   word my_net;
   word my_node;
   word my_point;
   char my_password[8];
   byte reserved2[4];
   int  n_mail;            /* Used only by LoraBBS >= 2.20 */
   int  n_data;            /* Used only by LoraBBS >= 2.20 */
   word capabilities;
   long tranx;             /* Used only by LoraBBS >= 2.10 */
   long b_mail;            /* Used only by LoraBBS >= 2.20 */
   long b_data;            /* Used only by LoraBBS >= 2.20 */
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
   bit  logtype     :1;
   char logindate[20];
   char password[40];
   unsigned long total_calls;
   word start_ratio;
   char from[36];
   char to[36];
   char subj[72];
   char msgdate[20];
   word attrib;
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

struct _pkthdr2
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
   byte product;           /* product type                   */
   char serial;            /* serial number (some systems)   */

   /* ------------------------------ */
   /* THE FOLLOWING SECTION IS NOT   */
   /* THE SAME ACROSS SYSTEM LINES:  */
   /* ------------------------------ */

   byte password[8];       /* session/pickup password        */
   int  orig_zone;         /* originating zone               */
   int  dest_zone;         /* Destination zone               */
   int  auxnet;
   word cwvalidation;
   byte producth;
   byte revision;
   word capability;
   int  orig_zone2;        /* originating zone               */
   int  dest_zone2;        /* Destination zone               */
   int  orig_point;        /* originating zone               */
   int  dest_point;        /* Destination zone               */
   long B_fill3;
};

struct _fwrd {
   int  zone;
   int  net;
   int  node;
   int  point;
   bit  export: 1;
   bit  reset:  1;
   int  fd;
   char aka;
};

struct _msghdr2 {
   int ver;
   int orig_node;
   int dest_node;
   int orig_net;
   int dest_net;
   int attrib;
   int cost;
};

struct _akainfo {
   int  zone;
   int  net;
   int  node;
   int  point;
   char aka;
};

