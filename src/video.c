#include <stdio.h>
#include <process.h>
#include <dos.h>
#include <time.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <conio.h>
#include <stdlib.h>
#include <dir.h>
#include <sys/stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "exec.h"

extern int blanked;

int spawn_program (int swapout, char *outstring);
void idle_system (void);
void clown_clear (void);
void stop_blanking (void);
void open_logfile (void);

#define UpdateCRC(c,crc) (cr3tab[((int) crc ^ c) & 0xff] ^ ((crc >> 8) & 0x00FFFFFFL))
static char *reg_prompt = "[UnRegistered]";

struct _fossil_info
{
   int size;
   char majver;
   char minver;
   char far *id;
   int input_size;
   int input_free;
   int output_size;
   int output_free;
   char width;
   char height;
   char baud;
};

static int fossil_inf(struct _fossil_info far *);
static void virtual_screen (void);
static void system_autoupdate (void);

int ox, oy, wh1;
char serial_id[3];
word serial_no;

/*---------------------------------------------------------------------------
   void setup_screen (void);

   Disegna le due parti piu' importanti dello schermo, dalla riga 1 alla
   riga 23 (dove viene visualizzato il tracciato delle chiamate) e dalla
   riga 24 alla riga 25 (dove appaiono i vari messaggi di stato).
---------------------------------------------------------------------------*/
void setup_screen ()
{
   char stringa[40];

   virtual_screen ();

   cclrscrn(LGREY|_BLACK);
   ox = wherex ();
   oy = wherey ();
   hidecur ();

   wh1 = wopen (0, 0, 24, 79, 5, LGREY|_BLACK, LGREY|_BLACK);
   wactiv (wh1);

   wbox (1, 0, 24, 79, 0, LGREY|_BLACK);
   whline (12, 0, 80, 0, LGREY|_BLACK);
   whline (22, 0, 80, 0, LGREY|_BLACK);
   wvline (1, 52, 24, 0, LGREY|_BLACK);
//   wvline (22, 21, 3, 0, LGREY|_BLACK);
   sprintf (stringa, "%d:%d/%d.%d", config->alias[0].zone, config->alias[0].net, config->alias[0].node, config->alias[0].point);
   prints (0, 1, LGREEN|_BLACK, stringa);
   prints (0, 78 - strlen (VERSION), LGREEN|_BLACK, VERSION);
//   prints (0, 78 - strlen (system_name), LGREEN|_BLACK, system_name);
   prints (1, 2, LCYAN|_BLACK, "LOG");
   prints (1, 54, LCYAN|_BLACK, "SYSTEM");
   prints (12, 2, LCYAN|_BLACK, "OUTBOUND");
   prints (12, 54, LCYAN|_BLACK, "MODEM");
   prints (13, 2, YELLOW|_BLACK, "Node            Try/Con  Type  Size    Status");

   prints (5, 54, LCYAN|_BLACK, "   Status:");
   prints (6, 54, LCYAN|_BLACK, "  Elapsed:");

   idle_system ();

   activation_key ();
   if (registered)
      sprintf (stringa, "%s/%s%05u", VERSION, serial_id[0] ? serial_id : "", serial_no);
   else
      sprintf (stringa, "%s/Demo", VERSION);
   prints (0, 78 - strlen (stringa), LGREEN|_BLACK, stringa);
}

/*---------------------------------------------------------------------------
   void idle_system (void);

---------------------------------------------------------------------------*/
void idle_system ()
{
   char string[20];

   wfill (7, 53, 11, 78, ' ', LCYAN|_BLACK);

   prints (7, 54, LCYAN|_BLACK, "     Next:");
   prints (8, 54, LCYAN|_BLACK, "Remaining:");
   prints (9, 54, LCYAN|_BLACK, "  Current:");
   prints (10, 54, LCYAN|_BLACK, "     Port:");

   mtask_find ();
   sprintf (string, "%-6lu Com%d", rate, com_port + 1);
   prints (10, 65, YELLOW|_BLACK, string);
}

/*---------------------------------------------------------------------------
   void blank_system (void);

---------------------------------------------------------------------------*/
void blank_system ()
{
   wfill (7, 53, 11, 78, ' ', LCYAN|_BLACK);
}

/*---------------------------------------------------------------------------
   void toss_system (void);

---------------------------------------------------------------------------*/
void toss_system ()
{
   wfill (7, 53, 11, 78, ' ', LCYAN|_BLACK);

   prints (7, 54, LCYAN|_BLACK, "   Packet:");
   prints (8, 54, LCYAN|_BLACK, "     Date:");
   prints (9, 54, LCYAN|_BLACK, "     Time:");
   prints (10, 54, LCYAN|_BLACK, "     From:");
   prints (11, 54, LCYAN|_BLACK, " Received:");
}

/*---------------------------------------------------------------------------
   void unpack_system (void);

---------------------------------------------------------------------------*/
void unpack_system ()
{
   wfill (7, 53, 11, 78, ' ', LCYAN|_BLACK);

   prints (7, 54, LCYAN|_BLACK, "   Packet:");
   prints (8, 54, LCYAN|_BLACK, "   Method:");
}

/*---------------------------------------------------------------------------
   void scan_system (void);

---------------------------------------------------------------------------*/
void scan_system ()
{
   wfill (7, 53, 11, 78, ' ', LCYAN|_BLACK);

   prints (7, 54, LCYAN|_BLACK, "  Message:");
   prints (8, 54, LCYAN|_BLACK, "     Area:");
   prints (9, 54, LCYAN|_BLACK, "     Base:");
   prints (10, 54, LCYAN|_BLACK, "Forwarded:");
}

/*---------------------------------------------------------------------------
   void pack_system (void);

---------------------------------------------------------------------------*/
void pack_system ()
{
   wfill (7, 53, 11, 78, ' ', LCYAN|_BLACK);

   prints (7, 54, LCYAN|_BLACK, "     Node:");
   prints (8, 54, LCYAN|_BLACK, " Filename:");
   prints (9, 54, LCYAN|_BLACK, "   Method:");
}

/*---------------------------------------------------------------------------
   void dial_system (void);

   Finestra di stato adatta per le operazioni di chiamata di un altro BBS.
   Viene visualizzato cosa sta facendo il mailer, chi sta chiamado e il
   timeout dell'azione corrente.

   Local status puo' essere "Dialing" e "Connect".
   Action puo' essere "Waiting", "YooHoo/2U2", "EMSI/C1", "EMSI/C2" oppure
   "FTSC-001".
---------------------------------------------------------------------------*/
void dial_system ()
{
   wfill (7, 53, 11, 78, ' ', LCYAN|_BLACK);

   prints (7, 54, LCYAN|_BLACK, "   Action:");
   prints (8, 54, LCYAN|_BLACK, "  Timeout:");
}

void filetransfer_system ()
{
   wfill (7, 53, 11, 78, ' ', LCYAN|_BLACK);

   prints (7, 55, LCYAN|_BLACK, "Protocol:");
   prints (8, 54, LCYAN|_BLACK, "Files I/O:");
}

/*---------------------------------------------------------------------------
   void setup_bbs_screen (void);

   Apre lo schermo da utilizzare per la parte BBS, con due righe di stato
   in basso e il resto della schermo da dedicare alla visualizzazione di
   cio' che l'utente sta facendo.
---------------------------------------------------------------------------*/
void setup_bbs_screen ()
{
   int i;

   if (local_mode != 2)
      clown_clear ();

   i = whandle();
#ifndef __OS2__
   wunlink(i);
#endif

   if (local_mode != 2) {
      status = wopen (23, 0, 24, 79, 5, BLACK|_LGREY, BLACK|_LGREY);
      wactiv (status);
      wprints (0, 65, BLACK|_LGREY, " [Time:      ]");
      wprints (1, 79 - strlen (reg_prompt), BLACK|_LGREY, reg_prompt);
   }

   mainview = wopen (0, 0, (local_mode == 2) ? 24 : 22, 79, 5, LGREY|_BLACK, LGREY|_BLACK);
   wactiv (mainview);

   f4_status ();
}

/*---------------------------------------------------------------------------
  void activation_key (void);

  Verifica la chiave di registrazione del programma, attivando le opzioni
  riservate. Se si tratta di una chiave per point la variabile globale
  registered assume il valore 2, altrimenti il valore 1.
---------------------------------------------------------------------------*/
// #define BETAKEY

#define x32crc(c,crc) (cr3tab[((int) crc ^ c) & 0xff] ^ ((crc >> 8) & 0x00FFFFFFL))

typedef union {
   long value;
   struct {
      char byte1;
      char byte2;
      char byte3;
      char byte4;
   } sp;
} SPLIT4;

typedef union {
   unsigned short value;
   struct {
      char byte1;
      char byte2;
   } sp;
} SPLIT2;

// Valori per flag1 - Opzioni di registrazione
#define REG2    0x01
#define REG5    0x02
#define REG10   0x04
#define REGUNL  0x08
#define POINT   0x10
#define BETA    0x20
#define DOS     0x40
#define OS2     0x80

typedef struct _ak {
   char  crc1;
   char  ending1;
   char  rnd1;
   char  crc2;
   char  flag1;
   char  id[2];
   char  ending2;
   short serial;
   char  crc3;
   char  flag2;
   char  version1;
   char  crc4;
   char  version2;
   char  rnd2;
} AK;

static unsigned long get_crc (void *buffer, int length)
{
   int i;
   unsigned long crc = 0xAABBCCDDL;
   char *b;

   b = (char *)buffer;

   for (i = 0; i < length; i++)
      crc = x32crc ((unsigned char)b[i], crc);

   return (crc);
}

void activation_key()
{
   int i, maxlines;
   char str[32];
   unsigned char *p, px, ppx;
   unsigned long value, mult;
   AK ak;
   SPLIT2 cs2;
   SPLIT4 cs4;
   int decoding[] = { 0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,0,10,11,12,13,14,15,16,17,18,19,20,
                      21,22,23,24,25,26,27,28,29,30,31,32,33,34,35 };

   memset (serial_id, 0, 3);

   memset (&ak, 0, sizeof (AK));
   strcpy (str, config->newkey_code);

   p = (unsigned char *)&ak;
   for (i = 0; i < strlen (str); i++) {
      if ((i % 7) == 0) {
         if (i) {
            memcpy (p, &value, 4);
            p += 4;
         }
         value = 0L;
         mult = 1L;
      }
      value += decoding[str[i] - '0'] * mult;
      mult *= 36L;
   }
   memcpy (p, &value, 4);

   p = (unsigned char *)&ak;
   ppx = 0xAA;
   for (i = 0; i < sizeof (AK); i++) {
      px = *p;
      *p ^= ppx;
      ppx = px;
      p++;
   }

   cs4.sp.byte1 = ak.crc1;
   cs4.sp.byte2 = ak.crc2;
   cs4.sp.byte3 = ak.crc3;
   cs4.sp.byte4 = ak.crc4;
   ak.crc1 = ak.crc2 = ak.crc3 = ak.crc4 = 0;

   cs2.sp.byte1 = ak.ending1;
   cs2.sp.byte2 = ak.ending2;

   if (local_mode)
      maxlines = line_offset;
   else {
      if (ak.flag1 & REG2)
         maxlines = 2;
      else if (ak.flag1 & REG5)
         maxlines = 5;
      else if (ak.flag1 & REG10)
         maxlines = 10;
      else if (ak.flag1 & REGUNL)
         maxlines = 99;
      else if (ak.flag1 & POINT)
         maxlines = 1;
   }

   if (cs4.value == get_crc (&ak, sizeof (AK))) {
#ifdef __OS2__
      if ((ak.flag1 & OS2) && maxlines == 99 || maxlines >= line_offset) {
#else
      if ((ak.flag1 & DOS) && maxlines == 99 || maxlines >= line_offset) {
#endif
         reg_prompt = "  [Registered]";
         if (ak.flag1 & POINT)
            registered = 2;
         else
            registered = 1;
         serial_no = ak.serial;
         serial_id[0] = ak.id[0];
         serial_id[1] = ak.id[1];
      }
   }
   else {
#ifndef COMMERCIAL
//#ifndef __OS2__
//      wopen (9, 12, 18, 66, 0, LRED|_BLACK, LGREY|_BLACK);
//      wtitle (" SECURITY VIOLATION ", TCENTER, YELLOW|_BLACK);
//      wcenters (0, LCYAN|_BLACK, "This is a beta version that need an authorized");
//      wcenters (1, LCYAN|_BLACK, "registration key to work.");
//      wcenters (3, YELLOW|_BLACK|BLINK, "YOU ARE NOT ALLOWED TO USE THIS PROGRAM");
//      wcenters (5, LCYAN|_BLACK, "Please, discontinue to use this version and contact");
//      wcenters (6, LCYAN|_BLACK, "the author to obtain an usable copy.");
//      wcenters (7, LCYAN|_BLACK, "Your system is now freezed");
//      for (;;);
//#endif
#else
      wopen (9, 12, 18, 66, 0, LRED|_BLACK, LGREY|_BLACK);
      wtitle (" SECURITY VIOLATION ", TCENTER, YELLOW|_BLACK);
      wcenters (0, LCYAN|_BLACK, "This is a commercial product that need an");
      wcenters (1, LCYAN|_BLACK, "authorized registration key to work.");
      wcenters (3, YELLOW|_BLACK|BLINK, "YOU ARE NOT ALLOWED TO USE THIS PROGRAM");
      wcenters (5, LCYAN|_BLACK, "Please, discontinue to use this version and contact");
      wcenters (6, LCYAN|_BLACK, "the author to obtain an usable copy.");
      wcenters (7, LCYAN|_BLACK, "Your system is now freezed");
      for (;;);
#endif
   }
}

/*
   int i;
   char kc[30];
   dword crc;

   registered = 0;

#ifdef COMMERCIAL
   crc = 0x6314F177L;
   for (i=0; i<strlen(config->sysop);i++)
      crc = UpdateCRC ((byte)config->sysop[i], crc);

   if (config->keycode == crc) {
      reg_prompt = "  [Registered]";
      registered = 1;
   }

#else

   crc = 0x6FE41197L;
   for (i=0; i<strlen(config->sysop);i++)
      crc = UpdateCRC ((byte)config->sysop[i], crc);

   if (config->keycode == crc && line_offset == 1) {
      reg_prompt = "  [Registered]";
      registered = 2;
   }
   else {
      crc = 0x7FE50189L;
      for (i=0; i<strlen(config->sysop);i++)
         crc = UpdateCRC ((byte)config->sysop[i], crc);

      if (config->keycode == crc && line_offset <= 2) {
         reg_prompt = "  [Registered]";
         registered = 1;
      }
      else {
         crc = 0x74E40291L;
         for (i=0; i<strlen(config->sysop);i++)
            crc = UpdateCRC ((byte)config->sysop[i], crc);

         if (config->keycode == crc && line_offset <= 5) {
            reg_prompt = "  [Registered]";
            registered = 1;
         }
         else {
            crc = 0x7FA45109L;
            for (i=0; i<strlen(config->sysop);i++)
               crc = UpdateCRC ((byte)config->sysop[i], crc);

            if (config->keycode == crc && line_offset <= 10) {
               reg_prompt = "  [Registered]";
               registered = 1;
            }
            else {
               crc = 0x7FE40199L;
               for (i=0; i<strlen(config->sysop);i++)
                  crc = UpdateCRC ((byte)config->sysop[i], crc);


               if (config->keycode == crc) {
                  reg_prompt = "  [Registered]";
                  registered = 1;
               }
            }
         }
      }
   }
#endif

   serial_no = 0;

   if (registered) {
      for (i=0; i<strlen(config->sysop);i++)
         serial_no = xcrc (serial_no, (byte)config->sysop[i]);
      sprintf (kc, "%ld", config->keycode);
      for (i = 0;  i < strlen (kc); i++)
         serial_no = xcrc (serial_no, (byte)kc[i]);
   }
   else {
#ifndef COMMERCIAL
//      wopen (9, 12, 18, 66, 0, LRED|_BLACK, LGREY|_BLACK);
//      wtitle (" SECURITY VIOLATION ", TCENTER, YELLOW|_BLACK);
//      wcenters (0, LCYAN|_BLACK, "This is a beta version that need an authorized");
//      wcenters (1, LCYAN|_BLACK, "registration key to work.");
//      wcenters (3, YELLOW|_BLACK|BLINK, "YOU ARE NOT ALLOWED TO USE THIS PROGRAM");
//      wcenters (5, LCYAN|_BLACK, "Please, discontinue to use this version and contact");
//      wcenters (6, LCYAN|_BLACK, "the author to obtain an usable copy.");
//      wcenters (7, LCYAN|_BLACK, "Your system is now freezed");
//      for (;;);
#else
      wopen (9, 12, 18, 66, 0, LRED|_BLACK, LGREY|_BLACK);
      wtitle (" SECURITY VIOLATION ", TCENTER, YELLOW|_BLACK);
      wcenters (0, LCYAN|_BLACK, "This is a commercial product that need an");
      wcenters (1, LCYAN|_BLACK, "authorized registration key to work.");
      wcenters (3, YELLOW|_BLACK|BLINK, "YOU ARE NOT ALLOWED TO USE THIS PROGRAM");
      wcenters (5, LCYAN|_BLACK, "Please, discontinue to use this version and contact");
      wcenters (6, LCYAN|_BLACK, "the author to obtain an usable copy.");
      wcenters (7, LCYAN|_BLACK, "Your system is now freezed");
      for (;;);
#endif
   }

#ifdef BETAKEY
   crc = 0x3FC41159L;
   for (i=0; i<strlen(config->sysop);i++)
      crc = UpdateCRC ((byte)config->sysop[i], crc);
   for (i=0; i<strlen(config->sysop);i++)
      crc = UpdateCRC ((byte)config->sysop[i], crc);
   crc = ~crc;
   if (!registered || crc != config->betakey) {
      wopen (9, 12, 18, 66, 0, LRED|_BLACK, LGREY|_BLACK);
      wtitle (" SECURITY VIOLATION ", TCENTER, YELLOW|_BLACK);
      wcenters (0, LCYAN|_BLACK, "This is a beta version that need an authorized");
      wcenters (1, LCYAN|_BLACK, "registration key to work.");
      wcenters (3, YELLOW|_BLACK|BLINK, "YOU ARE NOT ALLOWED TO USE THIS PROGRAM");
      wcenters (5, LCYAN|_BLACK, "Please, discontinue to use this version and contact");
      wcenters (6, LCYAN|_BLACK, "the author to obtain an usable copy.");
      wcenters (7, LCYAN|_BLACK, "Your system is now freezed");
      for (;;);
   }
#endif
*/

static int os2_active (void)
{
#ifdef __OS2__
   return (1);
#else
   union REGS regs;

   regs.h.ah = 0x30;
   intdos (&regs, &regs);

   return (regs.h.al >= 20);
#endif
}

/*---------------------------------------------------------------------------
   void mtask_find (void)

   Cerca di identificare il multitasker sotto cui e' installato il programma.
   Per ogni multitasker esiste una routine di time_release appropriata per
   dare agli altri task il tempo CPU non usato da questo task.
---------------------------------------------------------------------------*/
void mtask_find ()
{
#ifndef __OS2__
   have_dv = 0;
   have_ml = 0;
   have_tv = 0;
   have_ddos = 0;
   have_os2 = 0;

   if ((have_dv = dv_get_version()) != 0) {
      ;// wprints (9, 65, YELLOW|_BLACK, "DESQview");
      _vinfo.dvexist = 1;
   }
   else if ((have_ddos = ddos_active ()) != 0)
      ;// wprints (9, 65, YELLOW|_BLACK, "DoubleDOS");
   else if ((have_ml = ml_active ()) != 0)
      ;// wprints (9, 65, YELLOW|_BLACK, "Multilink");
   else if ((have_tv = tv_get_version ()) != 0)
      ;// wprints (9, 65, YELLOW|_BLACK, "TopView");
   else if (windows_active())
      ;// wprints (9, 65, YELLOW|_BLACK, "MS-Windows");
   else if ((have_os2 = os2_active ()) != 0)
      ;// wprints (9, 65, YELLOW|_BLACK, "OS/2");
   else
      ;// wprints (9, 65, YELLOW|_BLACK, "None");

   if (have_dv || have_ddos || have_ml || have_tv || have_os2)
      use_tasker = 1;
   else
      use_tasker = 0;
#else
   // wprints (9, 65, YELLOW|_BLACK, "OS/2");
#endif
}

void write_sysinfo()
{
   int fd;
   char filename[80];
   long pos;
   struct _linestat lt;

   strcode (sysinfo.pwd, "YG&%FYTF%$RTD");

   sprintf (filename, "%sSYSINFO.DAT", config->sys_path);
   fd = shopen(filename, O_BINARY|O_RDWR);
   write(fd, (char *)&sysinfo, sizeof(struct _sysinfo));

   pos = tell (fd);
   while (read(fd, (char *)&lt, sizeof(struct _linestat)) == sizeof (struct _linestat)) {
      if (lt.line == line_offset)
         break;
      pos = tell (fd);
   }

   if (lt.line == line_offset) {
      lseek (fd, pos, SEEK_SET);
      write (fd, (char *)&linestat, sizeof(struct _linestat));
   }

   close (fd);

   strcode (sysinfo.pwd, "YG&%FYTF%$RTD");
}

void read_sysinfo (void)
{
   int fd, iyear, imday, iwday, imon, modif;
   char filename[80];
   long tempo;
   struct tm *tim;

   modif = 0;

   sprintf (filename, "%sSYSINFO.DAT", config->sys_path);
   if ((fd = sh_open (filename, SH_DENYRW, O_BINARY|O_RDWR, S_IREAD|S_IWRITE)) == -1) {
      fd = sh_open (filename, SH_DENYRW, O_BINARY|O_RDWR|O_CREAT, S_IREAD|S_IWRITE);
      memset((char *)&sysinfo, 0, sizeof(struct _sysinfo));
      status_line ("!Creating SYSINFO.DAT file");
      write (fd, (char *)&sysinfo, sizeof(struct _sysinfo));
      lseek (fd, 0, SEEK_SET);
   }

   read (fd, (char *)&sysinfo, sizeof (struct _sysinfo));
   memset ((char *)&linestat, 0, sizeof (struct _linestat));

   tempo = time (NULL);
   tim = localtime (&tempo);

   imday = tim->tm_mday;
   iwday = tim->tm_wday;
   imon = tim->tm_mon;
   iyear = tim->tm_year;

   while (read (fd, (char *)&linestat, sizeof (struct _linestat)) == sizeof (struct _linestat)) {
      if (linestat.line == line_offset)
         break;
   }

   if (linestat.line != line_offset) {
      memset ((char *)&linestat, 0, sizeof (struct _linestat));
      sprintf (linestat.startdate, "%02d-%02d-%02d", tim->tm_mon + 1, tim->tm_mday, tim->tm_year % 100);
      linestat.line = line_offset;
      write (fd, (char *)&linestat, sizeof (struct _linestat));
   }

   close (fd);

   if (sysinfo.pwd[0]) {
      strcode (sysinfo.pwd, "YG&%FYTF%$RTD");
      password = sysinfo.pwd;
      locked = 1;
   }

   if (!local_mode) {
      tim = localtime (&sysinfo.today.timestamp);
      if (tim == NULL || tim->tm_mday != imday) {
         memcpy ((char *)&sysinfo.yesterday, (char *)&sysinfo.today, sizeof (struct _daystat));
         memset ((char *)&sysinfo.today, 0, sizeof (struct _daystat));
         sysinfo.today.timestamp = tempo;
         if (!iwday) {
            memset ((char *)&sysinfo.week, 0, sizeof (struct _daystat));
            sysinfo.week.timestamp = tempo;
         }
         system_autoupdate ();
         modif = 1;
      }

      tim = localtime (&sysinfo.month.timestamp);
      if (tim == NULL || tim->tm_mon != imon) {
         memset ((char *)&sysinfo.month, 0, sizeof (struct _daystat));
         sysinfo.month.timestamp = tempo;
         modif = 1;
      }

      tim = localtime (&sysinfo.year.timestamp);
      if (tim == NULL || tim->tm_year != iyear) {
         memset ((char *)&sysinfo.year, 0, sizeof (struct _daystat));
         sysinfo.year.timestamp = tempo;
         modif = 1;
      }

      if (modif)
         write_sysinfo ();
   }
}

void update_sysinfo_calls (void)
{
   int fd;
   char filename[80];

   sprintf (filename, "%sSYSINFO.DAT", config->sys_path);
   fd = sh_open (filename, SH_DENYRW, O_BINARY|O_RDWR, S_IREAD|S_IWRITE);

   read (fd, (char *)&sysinfo, sizeof (struct _sysinfo));

   sysinfo.total_calls++;
   sysinfo.today.humancalls++;
   sysinfo.week.humancalls++;
   sysinfo.month.humancalls++;
   sysinfo.year.humancalls++;

   lseek (fd, 0L, SEEK_SET);
   write (fd, (char *)&sysinfo, sizeof (struct _sysinfo));

   memset ((char *)&linestat, 0, sizeof (struct _linestat));

   while (read (fd, (char *)&linestat, sizeof (struct _linestat)) == sizeof (struct _linestat)) {
      if (linestat.line == line_offset)
         break;
   }

   close (fd);

   if (sysinfo.pwd[0])
      strcode (sysinfo.pwd, "YG&%FYTF%$RTD");
}

static int fossil_inf(finfo)
struct _fossil_info far *finfo;
{
#ifndef __OS2__
   union REGS regs;
   struct SREGS sregs;
   extern int port;

   regs.h.ah = 0x1B;
   regs.x.cx = sizeof(*finfo);
   sregs.es  = FP_SEG(finfo);
   regs.x.di = FP_OFF(finfo);
   regs.x.dx = com_port;

   int86x(0x14, &regs, &regs, &sregs);

   return(regs.x.ax);
#else
   return (0);
#endif
}

void fossil_version()
{
   struct _fossil_info finfo;

   fossil_inf(&finfo);
   m_print(msgtxt[M_FOSSIL_TYPE], finfo.id);
}

void fossil_version2 (void)
{
   struct _fossil_info finfo;

   if (local_mode)
      m_print(bbstxt[B_FOSSIL_INFO], (char far *)"<< Local mode >>");
   else {
      fossil_inf(&finfo);
      m_print(bbstxt[B_FOSSIL_INFO], finfo.id);
   }
}

void terminating_call (void)
{
   int wh;

   if (caller) {
      if (local_mode != 2) {
         if (config->snooping) {
            hidecur ();
            wh = wopen(11,24,15,54,1,LCYAN|_BLUE,LCYAN|_BLUE);
            wactiv(wh);

            wcenters(1,LCYAN|_BLUE,"Terminating call");
         }
         else
            prints (5, 65, YELLOW|_BLACK, "Terminating ");
      }

      textattr (LGREY|_BLACK);
      update_user ();
   }

   if (sq_ptr != NULL) {
      MsgUnlock (sq_ptr);
      MsgCloseArea (sq_ptr);
      sq_ptr = NULL;
   }

   modem_hangup ();
}

static void virtual_screen()
{
#ifndef __OS2__
   int vbase=(&directvideo)[-1], vvirtual;

   _ES = vbase;
   _DI = 0;
   _AX = 0xFE00;
   __int__(0x10);
   vvirtual = _ES;

   if (vbase != vvirtual) {
      (&directvideo)[-1] = vvirtual;
      _vinfo.videoseg = vvirtual;
   }
#endif
}

static void disappear_effect1 (void);
static void disappear_effect2 (void);
static void disappear_effect3 (void);
static void disappear_effect4 (void);
static void disappear_effect5 (void);
static void disappear_effect6 (void);

void clown_clear ()
{
//#ifndef __OS2__
   struct time dt;

   gettime (&dt);
   srand (dt.ti_sec * 100 + dt.ti_hund);

   switch (random (6) + 1) {
      case 1:
         disappear_effect1 ();
         break;
      case 2:
         disappear_effect2 ();
         break;
      case 3:
         disappear_effect3 ();
         break;
      case 4:
         disappear_effect4 ();
         break;
      case 5:
         disappear_effect5 ();
         break;
      case 6:
         disappear_effect6 ();
         break;
   }
//#endif

   cclrscrn (LGREY|_BLACK);
}

static void disappear_effect1 ()
{
   int i;

   for (i = 0; i < 25; i++) {
      scrollbox (0, 0, 24, 39, 1, D_DOWN);
      scrollbox (0, 40, 24, 79, 1, D_UP);
      delay (15);
   }
}

static void disappear_effect2 ()
{
   int i;

   for (i = 0; i < 25; i++) {
      scrollbox (0, 0, 24, 39, 1, D_UP);
      scrollbox (0, 40, 24, 79, 1, D_DOWN);
      delay (15);
   }
}

static void disappear_effect3 ()
{
   int i;

   for (i = 0; i < 12; i++) {
      scrollbox (0, 0, 12, 79, 1, D_UP);
      scrollbox (13, 0, 24, 79, 1, D_DOWN);
      delay (15);
   }

   scrollbox (0, 0, 12, 79, 1, D_UP);
}

static void disappear_effect4 ()
{
   int i;

   for (i = 0; i < 12; i++) {
      scrollbox (0, 0, 12, 79, 1, D_DOWN);
      scrollbox (13, 0, 24, 79, 1, D_UP);
      delay (15);
   }

   scrollbox (0, 0, 12, 79, 1, D_DOWN);
}

static void disappear_effect5 ()
{
   int i;

   for (i = 0; i < 12; i++) {
      scrollbox (0, 0, 12, 39, 1, D_DOWN);
      scrollbox (0, 40, 12, 79, 1, D_UP);
      scrollbox (13, 0, 24, 39, 1, D_UP);
      scrollbox (13, 40, 24, 79, 1, D_DOWN);
      delay (15);
   }

   scrollbox (0, 0, 12, 39, 1, D_DOWN);
   scrollbox (0, 40, 12, 79, 1, D_UP);
}

static void disappear_effect6 ()
{
   int i;

   for (i = 0; i < 12; i++) {
      scrollbox (0, 0, 12, 39, 1, D_UP);
      scrollbox (0, 40, 12, 79, 1, D_DOWN);
      scrollbox (13, 0, 24, 39, 1, D_DOWN);
      scrollbox (13, 40, 24, 79, 1, D_UP);
      delay (15);
   }

   scrollbox (0, 0, 12, 39, 1, D_UP);
   scrollbox (0, 40, 12, 79, 1, D_DOWN);
}

#define MAXDUPES      1000

struct _dupecheck {
   char  areatag[48];
   short dupe_pos;
   short max_dupes;
   long  area_pos;
   long  dupes[MAXDUPES];
};

struct _dupeindex {
   char  areatag[48];
   long  area_pos;
};

static void system_autoupdate ()
{
   int fdm, fdd, fdn, fdi, *varr;
   char filename[80], found, newname[80];
   struct _sys tsys;
   struct _dupecheck *dupecheck;
   struct _dupeindex dupeindex;

   if (modem_busy != NULL)
      mdm_sendcmd (modem_busy);

   sprintf (filename, "%sDUPES.NEW", config->sys_path);
   fdn = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
   if (fdn != -1) {
      close (fdn);
      return;
   }

   sprintf (filename, "%sDUPES.NEW", config->sys_path);
   fdn = sh_open (filename, SH_DENYWR, O_RDWR|O_BINARY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE);
   if (fdn == -1)
      goto otherprocess;

   if (blanked)
      stop_blanking ();

   status_line ("-Performing system \"AutoMaint\" routine");
   local_status ("AutoMaint");

   sprintf (filename, SYSMSG_PATH, config->sys_path);
   fdm = sh_open (filename, SH_DENYRW, O_RDONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
   if (fdm == -1) {
      close (fdn);
      goto otherprocess;
   }

   sprintf (filename, "%sDUPES.IDX", config->sys_path);
   fdi = sh_open (filename, SH_DENYRW, O_RDWR|O_BINARY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE);
   if (fdi == -1) {
      close (fdm);
      close (fdn);
      goto otherprocess;
   }

   sprintf (filename, "%sDUPES.DAT", config->sys_path);
   fdd = sh_open (filename, SH_DENYRW, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
   if (fdd == -1) {
      close (fdi);
      close (fdm);
      close (fdn);
      goto otherprocess;
   }

   dupecheck = (struct _dupecheck *)malloc (sizeof (struct _dupecheck));
   if (dupecheck == NULL) {
      close (fdi);
      close (fdm);
      close (fdn);
      close (fdd);
      goto otherprocess;
   }

   while (read (fdd, (char *)dupecheck, sizeof (struct _dupecheck)) == sizeof (struct _dupecheck)) {
      lseek (fdm, 0L, SEEK_SET);
      found = 0;

      while (read (fdm, (char *)&tsys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA)
         if (!stricmp (tsys.echotag, dupecheck->areatag)) {
            found = 1;
            break;
         }

      if (found) {
         memset ((char *)&dupeindex, 0, sizeof (struct _dupeindex));
         strcpy (dupeindex.areatag, dupecheck->areatag);
         dupecheck->area_pos = dupeindex.area_pos = tell (fdn);
         write (fdn, (char *)dupecheck, sizeof (struct _dupecheck));
         write (fdi, (char *)&dupeindex, sizeof (struct _dupeindex));
      }

      time_release ();
   }

   close (fdd);
   close (fdn);
   close (fdi);
   close (fdm);

   sprintf (filename, "%sDUPES.NEW", config->sys_path);
   sprintf (newname, "%sDUPES.DAT", config->sys_path);
   unlink (newname);
   rename (filename, newname);

   free (dupecheck);

otherprocess:
   if (config->automaint[0]) {
      fclose (logf);
      getcwd (filename, 49);
      varr = ssave ();
      cclrscrn (LGREY|_BLACK);

      spawn_program (registered, config->automaint);

      if (varr != NULL)
         srestore (varr);
      setdisk (filename[0] - 'A');
      chdir (filename);
      open_logfile ();
   }

   if (modem_busy != NULL)
      modem_hangup ();

   local_status (msgtxt[M_SETTING_BAUD]);
}

