//
//      ram2lora.c
//      18-09-91
//      conversione menu' da Remote Access a Lora
//

#include <stdio.h>
#include <dir.h>
#include <dos.h>
#include <mem.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include "lora.h"
#include "crc32.h"


//  #definizioni

#define LEVELS 12     //  i livelli da TWIT a SYSOP incluso sono 12

//  significati degli "attributes" di USERS.BBS di RemoteAccess

#define  deleted()   (usr.attribute&1)    //  deleted
#define  clearscr()  (usr.attribute&2)    //  clear screen
#define  more()      (usr.attribute&4)    //  more prompt
#define  ansi()      (usr.attribute&8)    //  ansi
#define  nokill()    (usr.attribute&16)   //  no-kill
#define  xferpri()   (usr.attribute&32)   //  xfer priority
#define  ansi_ed()   (usr.attribute&64)   //  full msg ed
#define  quiet()     (usr.attribute&128)  //  quiet mode
#define  hotkeys()   (usr.attribute2&1)   //  hot-keys
#define  avatar()    (usr.attribute2&2)   //  avt/0
#define  viewer()    (usr.attribute2&4)   //  full msg view
#define  hidden()    (usr.attribute2&8)   //  hidden

//  definizioni "corte" per la configurazione di RemoteAccess

#define menus    RA.MenuPath
#define txtfiles RA.TextPath
#define nodelist RA.NodelistPath
#define system   RA.SysPath
#define msgbase  RA.MsgBasePath


//  variabili globali

char rasysdir[80];        //  directory di sistema di RemoteAccess
struct _usr user;         //  record dell'USERS.BBS di Lora

char *livello[]=          //  livelli d'accesso di Lora
{
  "TWIT", "DISGRACE", "LIMITED", "NORMAL", "WORTHY", "PRIVEL",
  "FAVORED", "EXTRA", "SPECIAL", "CLERK", "ASSTSYSOP", "SYSOP"
};

char *mname[105]=         //  menu' names di Lora
{
  "",
  "GOSUB MESSAGE AREA",   //  1
  "GOSUB FILE AREA",      //  2
  "GOODBYE",              //  3
  "?",                    //
  "SHOW FILE",            //  5
  "YELL AT SYSOP",        //  6
  "CONFIG",               //  7
  "USERLIST",             //  8
  "VERSION",              //  9
  "OUTSIDE",              //  10
  "?",                    //
  "?",                    //
  "QUOTES",               //  13
  "CLEAR GOTO",           //  14
  "CLEAR GOSUB",          //  15
  "RETURN MAIN",          //  16
  "CHANGE AREA",          //  17
  "?",                    //
  "GOTO MENU'",           //  21
  "FILE TITLES",          //  22
  "FILE DOWNLOAD",        //  23
  "FILE DISPLAY",         //  24
  "RAWDIR",               //  25
  "?",                    //
  "SET PASSWORD",         //  27
  "?",                    //
  "SET NULLS",            //  29
  "SET LENGTH",           //  30
  "?",                    //
  "SET MORE",             //  32
  "SET FORMFEED",         //  33
  "SET EDITOR",           //  34
  "SET CITY",             //  35
  "?",                    //
  "SET AVATAR",           //  37
  "SET ANSI",             //  38
  "SET COLOR",            //  39
  "EDIT NEW MESSAGE",     //  40
  "EDIT REPLY",           //  41
  "SAVE MESSAGE",         //  42
  "ABORT MESSAGE",        //  43
  "LIST MESSAGE",         //  44
  "?",                    //
  "INSERT LINE",          //  46
  "DELETE LINE",          //  47
  "CONTINUE EDITING",     //  48
  "CHANGE TO",            //  49
  "CHANGE SUBJECT",       //  50
  "?",                    //
  "?",                    //
  "LIST MESSAGE",         //  53
  "NEXT MESSAGE",         //  54
  "PRIOR MESSAGE",        //  55
  "READ NONSTOP",         //  56
  "?",                    //
  "?",                    //
  "CHECK MAILBOX",        //  59
  "?",                    //
  "GOSUB MENU'",          //  61
  "?",                    //
  "?",                    //
  "?",                    //
  "?",                    //
  "RETURN MENU'",         //  66
  "CLEAR STACK",          //  67
  "LOCATE FILES",         //  68
  "UPLOAD FILE",          //  69
  "SET SIGNATURE",        //  70
  "?",                    //
  "?",                    //
  "NEW FILES LIST",       //  73
  "NEXT MAIL",            //  74
  "PRIOR MAIL",           //  75
  "READ MAIL NONSTOP",    //  76
  "SET FULL SCR READER"   //  77
  "SEND ONLINE MESSAGE",  //  78
  "?",                    //
  "?",                    //
  "ON LINE USERS",        //  81
  "LIST MAIL",            //  82
  "?",                    //
  "?",                    //
  "?",                    //
  "?",                    //
  "?",                    //
  "?",                    //
  "?",                    //
  "?",                    //
  "?",                    //
  "?",                    //
  "?",                    //
  "?",                    //
  "?",                    //
  "CB CHAT SYSTEM",       //  96
  "READ TEXT FILE",       //  97
  "LAST CALLERS",         //  98
  "SET HANDLE",           //  99
  "SET VOICEPHONE",       //  100
  "SET DATAPHONE",        //  101
  "FILE CONTENTS",        //  102
  "USER EDITOR",          //  103
  "SET COUNTER",          //  104
};




//  strutture

struct messages      //  struttura dei records in MESSAGES.RA
{
  char name[41];
  char type;         //  0=local, 1=netmail, 2=echomail
  char kind;         //  0=both, 1=private, 2=public, 3=read-only
  char attr;         //  bits: 0=origin, 1=combd, 2=fattach, 3=alias, 5=handle
  char dayskill;
  char recvkill;
  int  countkill;
  int  reads;       long readf;
  int  writes;      long writef;
  int  sysops;      long sysopf;
  char origin[61];
  char whichaka;
} msg;


struct files         //  struttura dei records in FILES.RA
{
  char name[31];
  char attrib;
  char filepath[41];
  char freespace[41];    //  campo non usato in RA 1.01
  int  security;
  long flags;    
  int  pvtsecurity;
  long pvtflags;    
} fdir;

     
struct ramenu            //   struttura dei files .MNU di RA
{
  char mtype;
  int  security;
  char flags[4];
  char display[76];
  char hotkey;
  char params[81];
  char foreground;
  char background;
} r,m;


struct users            //  struttura dei record nel file USERS.BBS
{
  char name[36];
  char location[26];
  char password[16];
  char dataphone[13];
  char voicephone[13];
  char lasttime[6];
  char lastdate[9];
  char attribute;
  char flags[4];
  int  credit, pending;
  int  msgsposted, lastread, security, calls;
  int  uploads, downloads, uploadsk, downloadsk, todayk, elapsed, screenlen;
  char lastpwdchange;
  char attribute2;
  char group;
  int  sxi;
  char extraspace[3];
} usr;


struct usersxi          //  struttura dei record nel file USERSXI.BBS
{
  char handle[36];
  char comment[81];
  char firstdate[9];
  char combined[25];
  char birthdate[9];
  char subdate[9];
  char screenwidth;
  char extraspace[83];       //  non usato in RA 1.0x
} sxi;


struct  RA_config
{
  int     VersionID;
  char    CommPort;
  long    Baud;
  char    InitTries;
  char    InitStr[71],          BusyStr[71];
  char    InitResp[41],         BusyResp[41],
          Connect300[41],       Connect1200[41],
          Connect2400[41],      Connect4800[41],
          Connect9600[41],      Connect19k[41],
          Connect38k[41];
  char    AnswerPhone;
  char    Ring[21],             AnswerStr[21];
  char    FlushBuffer;
  int     ModemDelay,           MinimumBaud,
          GraphicsBaud,         TransferBaud;
  char    SlowBaudTimeStart[6], SlowBaudTimeEnd[6],
          DownloadTimeStart[6], DownloadTimeEnd[6],
          PagingTimeStart[6],   PagingTimeEnd[6];
  char    LoadingMsg[71],       ListPrompt[71];
  int     PwdExpiry;
  char    MenuPath[61],         TextPath[61],
          AttachPath[61],       NodelistPath[61],
          MsgBasePath[61],      SysPath[61],
          ExternalEdCmd[61];
  int     Address[10][4];             //  indirizzi z:n/n.p
  char    SystemName[31];
  int     NewSecurity,          NewCredit;
  char    NewFlags[4];
  char    OriginLine[61];
  char    QuoteString[16];
  char    Sysop[36];
  char    LogFileName[61];
  char    FastLogon,            AllowSysRem,
          MonoMode,             StrictPwdChecking,
          DirectWrite,          SnowCheck;
  int     CreditFactor;
  int     UserTimeOut,          LogonTime,
          PasswordTries,        MaxPage,
          PageLength;
  char    CheckForMultiLogon,   ExcludeSysopFromList,
          OneWordNames,         CheckMail;           //  Y/N/=
  char    AskVoicePhone,        AskDataPhone,
          DoFullMailCheck,      AllowFileShells,
          FixUploadDates,       ShowFileDates,
          Ansi,                 ClearScreen,
          MorePrompt,           UploadMsgs,
          KillSent,             CrashAskSec;
  char    CrashAskFlags[4];
  int     CrashSec;
  char    CrashFlags[4];
  int     FAttachSec;
  char    FAttachFlags[4];
  char    NormFore,             NormBack,
          StatFore,             StatBack,
          HiBack,               HiFore,
          WindFore,             WindBack;
  char    ExitLocal,            Exit300,
          Exit1200,             Exit2400,
          Exit4800,             Exit9600,
          Exit19k,              Exit38k;
  char    MultiLine;
  char    MinPwdLen;
  int     MinUpSpace;
  char    HotKeys;
  char    BorderFore,           BorderBack,
          BarFore,              BarBack;
  char    LogStyle,             MultiTasker,
          PwdBoard;
  int     BufferSize;
  char    FKeys[10][61];
  char    WhyPage;
  char    LeaveMsg;
  char    ShowMissingFiles;
  char    MissingString[11];
  char    AllowNetmailReplies;
  char    LogonPrompt[41];
  char    CheckNewFiles;
  char    ReplyHeader[61];
  char    BlankSecs;
  char    ProtocolAttrib[6];
  char    ErrorFreeString[16];
  char    DefaultCombined[25];
  int     RenumThreshold;
  char    LeftBracket,          RightBracket;
  char    AskForHandle;
  char    AskForBirthDate;
  int     Unused;
  char    ConfirmMsgDeletes;
  char    LocationPrompt[61];
  char    PagePrompt[61];
  char    UserfilePrompt[41];
  char    NewUserGroup;
  char    Avatar;
  char    BadPwdArea;
  char    Location[41];
  char    DoAfterAction;
  char    CRprompt[41];
  char    CRfore,               CRback;
  char    ContinuePrompt[41];
  char    SendBreak;
  char    ListPath[61];
  char    FullMsgView;
  char    EMSI_Enable,          EMSI_NewUser;
  char    FutureExpansion[492];
} RA;


struct user_access_level    //  struttura per uso interno
{
  int id;       //  1...65535
  int call;     //  call/day
  int time;     //  time/day
  int logon;    //  minimum baud for logon
  int file;     //  minimum baud for filetransfers
  int k1200;
  int k2400;
  int k9600;
  int flimit;   //  ratio kb
  int fratio;   //  ratio files
  int quotaon;  //  dopo quanti kb entra in funzione la ratio
} level[LEVELS];



//  funzioni

char *flaglist(long flags)      //  converte da 32 flag bits a LORA-flagstring
{
  static char retlist[33];
  char loraflags[]="0123456789ABCDEFGHIJKLMNOPQRSTUV";
  char *p=retlist,i;
  for(i=*p=0;i<32;i++) if((flags>>i)&1) *p++=loraflags[i];
  *p=0;
  return(retlist);
}


char *tp2tc(char *s)     //  converte stringa da TurboPasquale a Turbo C++
{
  int a=s[0];
  if(!a) return(s);
  memmove(s,s+1,a);
  s[a]=0;
  return(s);
}


char *tp2sp(char *s)     //  converte stringa da TurboPasquale a Lora
{
  char *p=s;
  tp2tc(s);
  for(;*p;p++) if(*p==' ') *p='_';
  return(s);
}


int exist(char *fname)          //  esiste il filename?
{
  FILE *fp=fopen(fname,"rb");
  if(fp==NULL) return(0);
  fclose(fp);
  return(1);
}


char *trailing(char *s)                 //  aggiunge trailing backslash
{
  if((s)[strlen(s)-1]!='\\')
    strcat((s),"\\");
  return(s);
}


char *privilege(int priv)      //  ritorna livello corrispondente RA->LORA
{
  for(int i=0;i<LEVELS;i++) if(level[i].id==priv) return(livello[i]);
  return("SYSOP - ???");
}


int nprivilege(int priv)     //  ritorna num livello corrispondente RA->LORA
{
  for(int i=0;i<LEVELS;i++) if(level[i].id==priv) return((i+1)<<4);
  return(16);
}


int extract(char *&src,char *&dest)      //  estrae un "pezzo" di argomento
{
  if(!*src) return(0);
  while(*src!=' ' && *src) *dest++=*src++;
  *dest++=' ';
  *dest=0;
  return(*src!=0);
}


void warning(char *s1,char *s2)         //  warning standard per i .CFG
{
  printf(";!!! WARNING !!!  %s: %s\n",s1,s2);
}


int menutype(int menutype, int hotkey, char *params)
{
  char pw[40];
  char arguments[100],*args=arguments;
  int mtype;
  mtype=-menutype;    //  debug only!
  *args='\0';         //  no args per default;
  switch(menutype)
  {
// -----  i menutypes convertiti senza problemi:  -----

    case 3 :   mtype=66;   break;   //  return
    case 8 :   mtype=9;    break;   //  version
    case 13:   mtype=8;    break;   //  user listing
    case 16:   mtype=35;   break;   //  alter location
    case 17:   mtype=27;   break;   //  alter password
    case 18:   mtype=30;   break;   //  alter screen len
    case 19:   mtype=33;   break;   //  toggle screen clear
    case 20:   mtype=32;   break;   //  toggle page pausing
    case 21:   mtype=38;   break;   //  toggle Ansi
    case 22:   mtype=59;   break;   //  check mailbox
    case 41:   mtype=34;   break;   //  toggle full screen editor
    case 43:   mtype=82;   break;   //  new mail
    case 52:   mtype=81;   break;   //  users online
    case 54:   mtype=78;   break;   //  send online message
    case 57:   mtype=100;  break;   //  set voice phone
    case 58:   mtype=101;  break;   //  set data phone
    case 60:   mtype=99;   break;   //  set handle
    case 61:   mtype=37;   break;   //  set Avatar
    case 62:   mtype=77;   break;   //  set fullscr msg viewer

    case 9 :   mtype=3;
               warning("logoff","displaya GOODBYE.A* anziche' LOGOFF.A*");
               break;

//  -----  i menutypes che possono richiedere qualche modifica:  -----

    case 1 :   mtype=21;                          //  goto menu
               if(extract(params,args))
               {
                 if(*params=='/')
                 {
                   warning("selezione area ignorata",params);
                   break;
                 }
                 strcat(args,"/P=");
                 if(extract(params,args))
                   warning("parametri ignorati",params);
               }
               break;
    case 4 :   mtype=14;                          //  clear stack & goto menu
               if(extract(params,args))
               {
                 if(*params=='/')
                 {
                   warning("selezione area ignorata",params);
                   break;
                 }
                 strcat(args,"/P=");
                 if(extract(params,args))
                   warning("parametri ignorati",params);
               }
               break;
    case 2 :   mtype=61;                          //  gosub menu
               if(extract(params,args))
               {
                 if(*params=='/')
                 {
                   warning("selezione area ignorata",params);
                   break;
                 }
                 strcat(args,"/P=");
                 if(extract(params,args))
                   warning("parametri ignorati",params);
               }
               break;
    case 7 :   mtype=10;
               strcpy(args,params);
               if(strstr(args,"*H")==NULL)
                 strcat(args," *H");
               else
                 strncpy(strstr(args,"*H"),"  ",2);
               if(strstr(args,"*R")!=NULL)
               {
                 strncpy(strstr(args,"*R"),"  ",2);
                 warning("*R","NON SUPPORTATO");
               }
               break;
    case 51:   mtype=98;              //  last 24h callers
               break;

//  -----  i menutypes che sicuramente richiedono modifiche:  -----

    case 5 :   mtype=5;
               strcpy(args,params);               //  show/display ANS/ASC
               break;
    case 11:   mtype=6;
               strcpy(args,params);
               warning("yell at sysop","parametri IGNORATI");
               break;

    case 30:   mtype=25;              //  raw dir
               strcpy(args,params);
               break;
    case 31:   mtype=22;              //  list files
               strcpy(args,params);
               break;
    case 32:   mtype=23;              //  download
               strcpy(args,params);
               break;
    case 33:   mtype=69;              //  upload
               strcpy(args,params);
               break;
    case 34:   mtype=102;             //  view zip/arc/lzh
               strcpy(args,params);
               break;
    case 37:   mtype=73;              //  new files
               strcpy(args,params);
               break;
    case 38:   mtype=24;              //  file display
               strcpy(args,params);
               break;

//  -----  i menutypes non implementati, o non correttamente tali:  -----

    case 6 :   mtype=0;
               warning("bulletin menu","NON SUPPORTATO");
               break;
    case 10:   mtype=0;
               warning("graphic system usage","NON SUPPORTATO");
               break;
    case 12:   mtype=0;
               warning("questionari","NON SUPPORTATI");
               break;
    case 14:   mtype=0;
               warning("timelimits","NON SUPPORTATO");
               break;
    case 15:   mtype=0;
               warning("exit to DOS","NON SUPPORTATO");
               break;
    case 23:   mtype=0;
               warning("read message menu","NON SUPPORTATO");
               break;
    case 24:   mtype=0;
               warning("scan messages","NON SUPPORTATO");
               break;
    case 25:   mtype=0;
               warning("quickscan messages","NON SUPPORTATO");
               break;
    case 26:   mtype=0;
               warning("delete message","NON SUPPORTATO");
               break;
    case 27:   mtype=0;
               warning("insert new message","NON SUPPORTATO");
               break;
    case 28:   mtype=0;
               warning("select combined areas","NON SUPPORTATO");
               break;
    case 29:   mtype=0;
               warning("move a file","NON SUPPORTATO");
               break;
    case 35:   mtype=0;
               warning("file scan by keyword","NON SUPPORTATO");
               break;
    case 36:   mtype=0;
               warning("file scan by filename","NON SUPPORTATO");
               break;
    case 39:   mtype=0;
               warning("display direct file","NON SUPPORTATO");
               break;
    case 40:   mtype=0;
               warning("display ANS/ASC hotkeys","NON SUPPORTATO");
               break;
    case 42:   mtype=0;
               warning("toggle hotkeys","NON SUPPORTATO");
               break;
    case 44:   mtype=0;
               warning("reset combined areas","NON SUPPORTATO");
               break;
    case 45:   mtype=0;
               warning("display textfile and wait","NON SUPPORTATO");
               break;
    case 46:   mtype=0;
               warning("display direct textfile and wait","NON SUPPORTATO");
               break;
    case 47:   mtype=0;
               warning("make a log entry","NON SUPPORTATO");
               break;
    case 48:   mtype=0;
               warning("download specific file","NON SUPPORTATO");
               break;
    case 49:   mtype=0;
               warning("select message area","NON SUPPORTATO");
               break;
    case 50:   mtype=0;
               warning("select file area","NON SUPPORTATO");
               break;
    case 53:   mtype=0;
               warning("do not disturb","NON SUPPORTATO");
               break;
    case 55:   mtype=0;
               warning("download any file","NON SUPPORTATO");
               break;
    case 56:   mtype=0;
               warning("browse nodelist","NON SUPPORTATO");
               break;
    case 59:   mtype=0;
               warning("global download","NON SUPPORTATO");
               break;
  }
  if(mtype)
    if(hotkey>=' ')   printf("        HOT_KEY      %c\n",hotkey);
  if(strlen(args))    printf("        ARGUMENTS    %s\n",args);
  if(!mtype)          printf("        MENU_TYPE 0\n");
    else printf("        MENU_TYPE    %d    ; %s\n",mtype,mname[mtype]);
  return(0);
}


int convert_mnufile(char *fname)
{
  FILE *fp;
  char *s, menuname[10];
  if((fp=fopen(fname,"rb"))==NULL)
  {
    cprintf("! Errore del file %s\n",fname);
    return(1);
  }
  fnsplit(fname,NULL,NULL,menuname,NULL);
  printf("\nBEGIN_MENU %s\n",strupr(menuname));
  fread(&m,sizeof(m),1,fp);       // legge main record
  if(feof(fp))
  {
    cprintf("! Errore EOF leggendo dal file %s\n",fname);
    return(1);
  }
  printf("        DEFAULT_COLORS %d %d\n;\n",m.foreground,m.background);
  tp2sp(m.display);

  for(;;)
  {
    fread(&r,sizeof(r),1,fp);     //  legge un record
    if(feof(fp)) break;
    tp2tc(r.params); 
    tp2sp(r.display);

    if(r.display[strlen(r.display)-1]!=';')    //  converte (==inverte)
      strcat(r.display,";");                   //  lo standard del ';'...
    else
      r.display[strlen(r.display)-1]='\0';

    printf("        DISPLAY      %s\n",r.display);
    menutype(r.mtype,r.hotkey,r.params);
    printf("        ITEM_COLORS  %d %d\n",r.background,r.foreground);
    printf("        OPTION_PRIV  %s\n",privilege(r.security));
    printf("        END_ITEM\n;\n");
  }
  printf("        DISPLAY      %s\n",m.display);      //  prompt...
  printf("        ITEM_COLORS  %d %d\n",m.background,m.foreground);
  printf("        OPTION_PRIV  %s\n",privilege(m.security));
  printf("        MENU_TYPE    0\n        END_ITEM\n;\nEND_MENU\n;\n");
  return(0);
}


int access_levels()
{
  FILE *fp;
  int accesslevel,min,kb300,kb1200,kb2400,kb4800,kb9600;
  int call,ratiok,ratiof,quotaon,i;
  char strlevel[200];
  strcat(strcpy(strlevel,rasysdir),"LIMITS.CTL");
  if((fp=fopen(strlevel,"r"))==NULL)
  {
    cprintf("! Errore: non trovo %s\n",strlevel);
    return(1);
  }
  for(;;)
  {
    fgets(strlevel,200,fp);
    if(feof(fp)) break;
    strcat(strlevel," -1 -1 -1 -1 -1 -1");
    sscanf(strlevel,"%d%d%d%d%d%d%d%d%d",&accesslevel,&min,&kb300,&kb1200,
      &kb2400,&kb4800,&kb9600,&ratiok,&ratiof);
    if(kb1200==-1) kb1200=kb300;
    if(kb2400==-1) kb2400=kb1200;
    if(kb4800==-1) kb4800=kb2400;
    if(kb9600==-1) kb9600=kb4800;
    if(ratiok==-1) ratiok=0;
    if(ratiof==-1) ratiof=0;
    call=min;
    quotaon=0;
    clrscr();
    cprintf("Livelli d'accesso:\r\nLivello    min  call  baud  file  kb1200");
    cprintf(" kb2400 kb9600  Quota  Ratio  QuotaOn\r\n---------------------");
    cprintf("--------------------------------------------------------\r\n");
    for(int n=0;n<LEVELS;n++)
      cprintf("%-3d%-8.8s%3d%6d%6d%6d%8d%7d%7d%7d%7d%9d\r\n",n,livello[n],
        level[n].time,level[n].call,level[n].logon,level[n].file,
        level[n].k1200,level[n].k2400,level[n].k9600,
        level[n].flimit,level[n].fratio,level[n].quotaon);
    cprintf("\r\nLivello \"%d\": min=%d, kb1200=%d, kb2400=%d, kb9600=%d\r\n",
      accesslevel,min,kb1200,kb2400,kb9600);
    cprintf("ratio/kb=%d, ratio/files=%d, quotaon=%d\r\n",
      ratiok,ratiof,quotaon);
    for(;;)
    {
      gotoxy(1,19);
      clreol();
      gotoxy(1,19);
      cputs("A quale livello va assegnato? ");
      if(cscanf("%d",&i)==1) 
      {
        if(!(i<0 || i>LEVELS)) break;
      }
      else cscanf("%c",&i);
    }
    level[i].id=accesslevel;
    level[i].call=level[i].time=min;
    level[i].logon=level[i].file=1200;
    level[i].k1200=kb1200;
    level[i].k2400=kb2400;
    level[i].k9600=kb9600;
    level[i].flimit=ratiok;
    level[i].fratio=ratiof;
  }
  fclose(fp);
  if(!level[0].id)
  {
    for(i=0;(i<LEVELS) && (level[i].id);i++);
    if(i<LEVELS) for(;i;i--) level[i-1]=level[i];
  }
  for(i=1;i<LEVELS;i++)
    if(!level[i].id)
      level[i]=level[i-1];
  clrscr();
  cprintf("Livelli d'accesso:\r\nLivello    min  call  baud  file  kb1200");
  cprintf(" kb2400 kb9600  Quota  Ratio  QuotaOn\r\n---------------------");
  cprintf("--------------------------------------------------------\r\n");
  for(int n=0;n<LEVELS;n++)
    cprintf("%-11.11s%3d%6d%6d%6d%8d%7d%7d%7d%7d%9d\r\n",livello[n],
      level[n].time,level[n].call,level[n].logon,level[n].file,
      level[n].k1200,level[n].k2400,level[n].k9600,
      level[n].flimit,level[n].fratio,level[n].quotaon);
  cprintf("--------------------------------------------------------\r\n");
  return(0);
}


int read_RA_config()
{
  FILE *fp;
  char fname[80];
  if((fp=fopen(strcat(strcpy(fname,rasysdir),"CONFIG.RA"),"rb"))==NULL)
  {
    cprintf("! Errore del file %s\n",fname);
    return(1);
  }
  fread(&RA,sizeof(RA),1,fp);
  fclose(fp);
  if(RA.VersionID!=0x100)
  {
    cprintf("! Errore: conversione valida solo per versioni RA 1.0x\r\n");
    return(1);
  }
  tp2tc(menus);     trailing(menus);
  tp2tc(txtfiles);  trailing(txtfiles);
  tp2tc(msgbase);   trailing(msgbase);
  tp2tc(system);    trailing(system);
  tp2tc(RA.CRprompt);
  tp2tc(RA.ContinuePrompt);
  return(0);
}


int convert_users_bbs()
{
  FILE *fp1,*fp2,*fp3,*fp4;
  int utenti=0;
  long crc;
  char fname[80];
  clrscr();
  cputs("Conversione files USERS*.BBS\r\n\r\n");
  if((fp1=fopen(strcat(strcpy(fname,msgbase),"USERS.BBS"),"rb"))==NULL)
  {
    cprintf("! Errore del file %s\n",fname);
    return(1);
  }
  if((fp2=fopen(strcat(strcpy(fname,msgbase),"USERSXI.BBS"),"rb"))==NULL)
  {
    fclose(fp1);
    cprintf("! Errore del file %s\n",fname);
    return(1);
  }
  if((fp3=fopen("USERS.BBS","wb"))==NULL)
  {
    fclose(fp1);
    fclose(fp2);
    cprintf("! Errore creando il file %s\n",fname);
    return(1);
  }
  if((fp4=fopen("USERS.IDX","wb"))==NULL)
  {
    fclose(fp1);
    fclose(fp2);
    fclose(fp3);
    cprintf("! Errore creando il file %s\n",fname);
    return(1);
  }

//  all'inizio l'user record e' tutti zeri; settiamo i valori validi per tutti:

  user.scanmail=RA.CheckMail;     //  si o no per tutti
  user.tabs=1;                    //  do not expand tabs
  user.ibmset=1;                  //  defaults to IBM-charset
  user.language=1;                //  la prima lingua, per default
  user.files=1;                   //  ultima area files visitata
  user.baud_rate=300;             //  baudrate ultima connessione (buu!!! :-)

  for(;;)                         //  "per ogni utente:"
  {
    fread(&usr,sizeof(usr),1,fp1);       //  legge user da RA USERS.BBS
    if(feof(fp1)) break;
    gotoxy(1,3);
    cputs(tp2tc(usr.name));
    cputs(" - ");
    cputs(tp2tc(usr.location));
    clreol();
    utenti++;
    fseek(fp2,(usr.sxi)*sizeof(sxi),SEEK_SET);
    fread(&sxi,sizeof(sxi),1,fp2);
    strcpy(user.name,usr.name);
    strcpy(user.handle,tp2tc(sxi.handle));
    strcpy(user.city,usr.location);
    strcpy(user.pwd,tp2tc(usr.password));         //  ocio! NON ha convertito!
    strcpy(user.comment,tp2tc(sxi.comment));
    strcpy(user.signature,user.name);             //  ocio! signature=username
    strcpy(user.voicephone,tp2tc(usr.voicephone));
    strcpy(user.dataphone,tp2tc(usr.dataphone));
    strcpy(user.birthdate,tp2tc(sxi.birthdate));
    strcpy(user.subscrdate,tp2tc(sxi.subdate));
    strcpy(user.firstdate,tp2tc(sxi.firstdate));
    strcpy(user.lastpwdchange,usr.lastdate);
    strcpy(strcat(strcpy(user.ldate,tp2tc(usr.lastdate)),"  "),
      tp2tc(usr.lasttime));
    user.times=usr.calls;
    user.msg=usr.lastread;
    user.avatar=avatar();
    user.color=user.ansi=ansi();
    user.use_lore=!ansi_ed();
    user.more=more();
    user.formfeed=clearscr();
    user.hotkey=hotkeys();
    user.full_read=viewer();
    user.usrhidden=hidden();
    user.nokill=nokill();
    user.priv=nprivilege(usr.security);
    for(int z=0;z<4;z++) user.flags[z]=usr.flags[z];
    user.time=usr.elapsed;
    user.upld=usr.uploadsk;
    user.dnld=usr.downloadsk;
    user.dnldl=usr.todayk;
    user.credit=usr.credit;
    user.len=usr.screenlen;
    user.width=sxi.screenwidth;
    crc=0xFFFFFFFFL;
    for(int i=0;i<strlen(user.name);i++)
      crc=Z_32UpdateCRC(((unsigned short)user.name[i]),crc);
    user.id=crc;
    user.msgposted=usr.msgsposted;
    fwrite(&user,sizeof(user),1,fp3);
    fwrite(&user.id,sizeof(user.id),1,fp4);
  }
  gotoxy(1,3);
  cprintf("Utenti totali: %d",utenti);
  clreol();
  fclose(fp1);
  fclose(fp2);
  fclose(fp3);
  fclose(fp4);
  return(0);
}


int convert_system()
{
  FILE *fp1,*fp2,*fp3;
  char fname[80];
  if((fp1=fopen(strcat(strcpy(fname,rasysdir),"FILES.RA"),"rb"))==NULL)
  {
    cprintf("! Errore del file %s\n",fname);
    return(1);
  }
  if((fp2=fopen(strcat(strcpy(fname,rasysdir),"MESSAGES.RA"),"rb"))==NULL)
  {
    fclose(fp1);
    cprintf("! Errore del file %s\n",fname);
    return(1);
  }
  if((fp3=fopen("SYSTEM.CFG","w"))==NULL)
  {
    fclose(fp1);
    fclose(fp2);
    cprintf("Non riesco a creare il file SYSTEM.CFG\n");
    return(1);
  }
  for(int n=1;n<=200;n++)
  {
    fread(&msg,sizeof(msg),1,fp1);
    fread(&fdir,sizeof(fdir),1,fp2);
    if(!*msg.name && !*fdir.name) continue;
    fprintf(fp3,"BEGIN_SYSTEM %d\n",n);
    if(*msg.name)
    {
      tp2sp(msg.name);
      cprintf("(%d)  MESSAGE_NAME  %s\n",n,msg.name);
      fprintf(fp3,"  MESSAGE_NAME  %s\n",msg.name);
      fprintf(fp3,"  MESSAGE_PRIV  %s\n",privilege(msg.reads));
      if(flaglist(msg.readf))
        fprintf(fp3,"  MESSAGE_FLAGS %s\n",flaglist(msg.readf));
      fprintf(fp3,"  WRITE_PRIV    %s\n",privilege(msg.writes));
      if(flaglist(msg.writef))
        fprintf(fp3,"  WRITE_FLAGS   %s\n",flaglist(msg.writef));
      fprintf(fp3,";  SYSOP_PRIV    %s\n",privilege(msg.sysops));
      if(flaglist(msg.sysopf))
        fprintf(fp3,";  SYSOP_FLAGS   %s\n",flaglist(msg.sysopf));
      fprintf(fp3,";  warning: MESSAGE_PATH e' quella di RemoteAccess\n");
      switch(msg.kind)
      {
        case 1 :  fprintf(fp3,"  PRIVATE_ONLY\n");
        case 2 :  fprintf(fp3,"  PUBLIC_ONLY\n");
        case 3 :  fprintf(fp3,";  READ_ONLY\n");         //  ocio che non va!!
      }
      if(msg.type==1)
      {
        fprintf(fp3,"  ECHOMAIL\n");
        fprintf(fp3,";  ocio che manca l'ECHOTAG !!!\n");    //  ocio!
      }
      if(msg.type==2) fprintf(fp3,";  NETMAIL\n");       //  ocio che non va!!
      fprintf(fp3,"  QUICK_BOARD   %d\n",n);
      if(msg.attr & 8)       fprintf(fp3,";  HANDLES_ONLY\n");   //  ocio !!
      else if(msg.attr & 32)  fprintf(fp3,"  OK_ALIAS\n");
      if(msg.attr & 4)       fprintf(fp3,";  ALLOW_FILE_ATTACH\n");  //  ocio!!
      if(msg.attr & 1)
      {
        tp2sp(msg.origin);
        fprintf(fp3,";  ORIGIN_LINE  %s\n",msg.origin);
      }
      if(msg.whichaka)
        fprintf(fp3,";  USE_AKA      %d:%d/%d.%d\n",
          RA.Address[msg.whichaka][0],RA.Address[msg.whichaka][1],
            RA.Address[msg.whichaka][2],RA.Address[msg.whichaka][3]);
      fprintf(fp3,"\n");
    }
    if(*fdir.name)
    {
      tp2sp(fdir.name);
      cprintf("(%d)  FILE_NAME     %s\n",n,fdir.name);
      fprintf(fp3,"  FILE_NAME     %s\n",fdir.name);
      fprintf(fp3,"  FILE_PRIV     %s\n",privilege(fdir.security));
      if(fdir.flags)
        fprintf(fp3,"  FILE_FLAGS    %s\n",flaglist(fdir.flags));
      fprintf(fp3,";  PVTFILE_PRIV  %s\n",privilege(fdir.pvtsecurity));
      if(fdir.flags)                                                 //  ocio!
        fprintf(fp3,";  PVTFILE_FLAGS %s\n",flaglist(fdir.pvtflags));
      tp2tc(fdir.filepath);
      fprintf(fp3,"  DOWNLOAD_PATH %s\n",fdir.filepath);
      fprintf(fp3,"  UPLOAD_PATH   %s\n",fdir.filepath);
    }
    fprintf(fp3,"END_SYSTEM\n\n");
  }
  fclose(fp1);
  fclose(fp2);
  fclose(fp3);
  return(0);
}


int virtualscreen()
{
  register int si,di;                     // sporco kludge :-)
  int vbase=(&directvideo)[-1],vvirtual;
  _ES=vbase, _DI=0, _AX=0xfe00, geninterrupt(0x10), vvirtual=_ES;
  if(vbase!=vvirtual) (&directvideo)[-1]=vvirtual;
  return(vbase!=vvirtual);
}


int write_lora_cfg()
{
  FILE *fp=fopen("LORA.CFG","w");
  if(fp==NULL)
  {
    cprintf("\r\n! Errore: non riesco a creare il file LORA.CFG\r\n");
    return(1);
  }
  fprintf(fp,";  Modem definitions\n");
  fprintf(fp,"MODEM_PORT COM%d %u\nMODEM_INIT %s\nMODEM_DIAL ATDP,\n",
    RA.CommPort,tp2tc(RA.InitStr));
  fprintf(fp,";LOCK_RATE\n\n");
  fprintf(fp,"; Miscellaneous info\n");
  fprintf(fp,"TASK_NUMBER     1\n");
  fprintf(fp,"LOG_NAME        C:\\LORA\\LORA.LOG\n");
  fprintf(fp,"USER_FILE       C:\\LORA\\USERS\n\n");
  fprintf(fp,"; System Path Information\n");
  fprintf(fp,"TEXTFILES_PATH  C:\\LORA\\TXTFILES\\\n");
  fprintf(fp,"SCHED_NAME      C:\\LORA\\SCHED.DAT\n");
  fprintf(fp,"SYSTEM_PATH     C:\\LORA\\\n");
  fprintf(fp,"MENU_PATH       C:\\LORA\\LANGUAGE\\\n");
  fprintf(fp,"NODELIST        C:\\LORA\\NODELIST\\\n");
  fprintf(fp,"IPC_PATH        C:\\LORA\\\n");
  fprintf(fp,"QUICKBASE_PATH  C:\\LORA\\MSG\\\n");
  fprintf(fp,"FLAG_PATH       C:\\LORA\\OUTBOUND\\\n\n");
  fprintf(fp,"OUTBOUND        C:\\LORA\\PACKET\\\n");
  fprintf(fp,"PROT_OUTBOUND   C:\\LORA\\OUTBOUND\\\n\n");
  fprintf(fp,"INBOUND         C:\\LORA\\FILES\\\n");
  fprintf(fp,"KNOW_INBOUND    C:\\LORA\\FILES\\\n");
  fprintf(fp,"PROT_INBOUND    C:\\LORA\\FILES\\\n;\n");
  fprintf(fp,"DATEFORMAT      %D-%C-%Y\n");
  fprintf(fp,"TIMEFORMAT      %E:%M%A\n");
  fprintf(fp,"LOGON_LEVEL     LIMITED\n");
  fprintf(fp,"SPEED_GRAPHICS  1200\n\n");
  fprintf(fp,"; System Information\n");
  fprintf(fp,"SYSOP_NAME      Alfonso_Martone\n");
  fprintf(fp,"SYSTEM_NAME     <FantOZZY>\n");
//  fprintf(fp,"ADDRESS         2:335/213\n");
  fprintf(fp,"ADDRESS         91:1/1\n");
  fprintf(fp,"; Video Mode\n");
  fprintf(fp,"NO_DIRECTVIDEO\n");
  fprintf(fp,"; MONOCHROME_ATTRIBUTE\n");
  fprintf(fp,"; SNOW_CHECKING\n");
  fprintf(fp,"; Mailer Frontend Banner\n");
  fprintf(fp,"MAIL_BANNER  Ti_sei_appena_connesso_a_<FantOZZY>!!!\n");
  fprintf(fp,"ENTER_BBS    Ammacca_ESC_per_continuare\n");
  fprintf(fp,"MAIL_ONLY    Sto_processando_la_posta,_richiama_domani.\n\n");
  fprintf(fp,"; Mailer errorlevels\n");
  fprintf(fp,"AFTERMAIL_EXIT    150\n");
  fprintf(fp,"AFTERCALLER_EXIT  250\n\n");
  fprintf(fp,"; Mailer behavior\n");
  fprintf(fp,"MAX_CONNECTS      2\n");
  fprintf(fp,"MAX_NOCONNECTS    20\n\n");
  fprintf(fp,"; File Requests\n");
  fprintf(fp,"ABOUT              C:\\LORA\\FREQUEST.ERR\n");
  fprintf(fp,"OKFILE             C:\\LORA\\OKFILE.LST\n");
  fprintf(fp,"AVAILIST           C:\\LORA\\FILESREQ\n");
  fprintf(fp,"MAX_REQUESTS       40\n");
  fprintf(fp,"KNOW_OKFILE        C:\\LORA\\OKFILE.LST\n");
  fprintf(fp,"KNOW_AVAILIST      C:\\LORA\\FILESREQ\n");
  fprintf(fp,"KNOW_MAX_REQUESTS  50\n");
  fprintf(fp,"PROT_MAX_REQUESTS  60\n\n");
  fprintf(fp,"; User Levels Definitions\n");
  fprintf(fp,";\n");
  fprintf(fp,";         Livello    Call  Time  Logon   File   File    File\n");
  fprintf(fp,";                                Baud    Baud   Limit   Ratio\n");
  fprintf(fp,";\n");
  fprintf(fp,"DEFINE    TWIT          3     3   1200    1200      1     0\n");
  fprintf(fp,"DEFINE    DISGRACE     10    10   1200    1200     10     0\n");
  fprintf(fp,"DEFINE    LIMITED      15    20   1200    1200     20     0\n");
  fprintf(fp,"DEFINE    NORMAL       20    20   1200    1200    150     0\n");
  fprintf(fp,"DEFINE    WORTHY       30    30   1200    1200    300     0\n");
  fprintf(fp,"DEFINE    PRIVEL       40    40   1200    1200    400     0\n");
  fprintf(fp,"DEFINE    FAVORED      50    50   1200    1200    500     0\n");
  fprintf(fp,"DEFINE    EXTRA        60    60   2400    2400    650     0\n");
  fprintf(fp,"DEFINE    CLERK        70    70   2400    2400    800     0\n");
  fprintf(fp,"DEFINE    ASSTSYSOP    75    75   2400    2400   2500     0\n");
  fprintf(fp,"DEFINE    SYSOP        75    75   9600    9600    999     0\n");
  fprintf(fp,"\n");
  fprintf(fp,"AREACHANGE_KEYS  +-L\n");
  fprintf(fp,"MAX_REREAD_PRIV  Registrato\n");
  fprintf(fp,"\n");
  fprintf(fp,";\n");
  fprintf(fp,";\n");
  fprintf(fp,"; Language file definitions (max. 20 language)\n");
  fprintf(fp,";\n");
  fprintf(fp,";         Filename  Key  Description\n");
  fprintf(fp,";\n");
  fprintf(fp,"LANGUAGE  ITALIOTA   I  Italiota\n");
  fprintf(fp,"LANGUAGE  ENGLISH    E  English_(Inglese)\n");
  fprintf(fp,"LANGUAGE  ITALIAN    A  Italiano_(Italian)\n");
  fprintf(fp,";\n");
  fprintf(fp,"INCLUDE   SCHED\n");
  fprintf(fp,"INCLUDE   SYSTEM\n");
  fprintf(fp,";\n");
  fclose(fp);
  return(0);
}

int main()
{
  int dv=virtualscreen();
  char *s;
  clrscr();
  cprintf("* RA2LORA v0.30, by Alfonso Martone\r\n* %s%x) detected\r\n",dv?
    "DESQview (virtual screen at 0x":"XT/AT-BIOS (video segment at 0x",
      (&directvideo)[-1]);

  if((s=getenv("RA"))==NULL)
  {
    cputs("\r\n\r\nEnvironment variable RA non definita, exiting...\r\n");
    return(1);
  }
  if(exist("SYSTEM.CFG")||exist("LORA.CFG")||exist("ITALIAN.CFG")||
    exist("EVENT.CFG"))
  {
    cputs("\r\n\r\nErrore: esistono gia' dei files *.CFG\r\n");
    return(1);
  }
  trailing(strcpy(rasysdir,s));
  cprintf("* RA directory: %s",rasysdir);
  window(1,5,80,25);
  if(read_RA_config())     return(1);
  if(access_levels())      return(1);
  if(convert_users_bbs())  return(1);
  return(1);
  if(convert_system())     return(1);
  return(0);
}
