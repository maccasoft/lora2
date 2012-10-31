#include <stdio.h>
#include <process.h>
#include <dos.h>
#include <time.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <conio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "defines.h"
#include "lora.h"
#include "lorakey.h"
#include "externs.h"
#include "prototyp.h"

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

void setup_screen()
{
   videoinit();
   virtual_screen ();
   hidecur();

   mainview = wopen(0,0,22,79,5,LGREY|_BLACK,LGREY|_BLACK);
   wactiv(mainview);
   free (_winfo.active->wbuf);
   _winfo.active->wbuf = NULL;

   status = wopen(23,0,24,79,5,BLACK|_LGREY,BLACK|_LGREY);
   wactiv(status);
   wprints(0,66,BLACK|_LGREY,"[Time:      ]");
   wprints(1,79 - strlen(reg_prompt),BLACK|_LGREY,reg_prompt);

   wactiv(mainview);
}

void activation_key()
{
   int fd, i, n;
   char *p, xorkey[20], *release_key = "7FE40199";
   struct _reg_key key;
   dword crc;

   crc = 0xFFFFFFFFL;

   registered = 0;

   fd = open("LORA.KEY", O_RDONLY|O_BINARY);
   if (fd == -1)
      return;
   read (fd, (char *)&key, sizeof(struct _reg_key));
   close (fd);

   n = 0;
   p = (char *)&key;
   sprintf(xorkey, "%d%d%d0",alias[0].zone, alias[0].net, alias[0].node);

   for (i=0; i<sizeof(struct _reg_key);i++)
   {
      p[i] = p[i] ^ xorkey[n++];
      if (xorkey[n] == '\0')
         n = 0;
   }

   p = (char *)&key;
   for (i=0; i<sizeof(struct _reg_key) - sizeof(dword);i++)
      crc = UpdateCRC ((byte)*p, crc);

   if (crc != key.crc || strcmp(key.release_key, release_key))
      return;

   if (alias[0].zone != key.zone && alias[0].net != key.net && alias[0].node != key.node)
      return;

   if (key.maxlines < line_offset)
      return;

   if (!strcmp(key.sysop, sysop) && key.version >= 200)
   {
      reg_prompt = "[Registered]";
      registered = 1;
   }
}

void mtask_find ()
{
   char buffer[80];

   have_dv = 0;
   have_ml = 0;
   have_tv = 0;
   have_ddos = 0;

   if ((have_dv = dv_get_version()) != 0)
   {
      sprintf(buffer, "* DESQview %d.%02d detected\n",have_dv >> 8, have_dv & 0xff);
      wputs(buffer);
      _vinfo.dvexist = 1;
   }
   else if ((have_ddos = ddos_active ()) != 0)
      wputs("* DoubleDOS detected\n");
   else if ((have_ml = ml_active ()) != 0)
      wputs("* Multilink detected\n");
   else if ((have_tv = tv_get_version ()) != 0)
      wputs("* TopView/TaskView detected\n");
   else if (windows_active())
      wputs ("* MS-Windows detected\n");
   else if ((byte)peekb(0xFFFF,0x000E) == 0xFC)
      wputs ("* AT-BIOS detected\n");

   if (have_dv || have_ddos || have_ml || have_tv)
      use_tasker = 1;
   else
      use_tasker = 0;
}

void write_sysinfo()
{
   int fd;
   char filename[80];
   long pos;
   struct _linestat lt;

   strcode (sysinfo.pwd, "YG&%FYTF%$RTD");

   sprintf (filename, "%sSYSINFO.DAT", sys_path);
   fd = open(filename, O_BINARY|O_RDWR);
   write(fd, (char *)&sysinfo, sizeof(struct _sysinfo));

   pos = tell (fd);
   while (read(fd, (char *)&lt, sizeof(struct _linestat)) == sizeof (struct _linestat))
   {
      if (lt.line == line_offset)
         break;
      pos = tell (fd);
   }

   if (lt.line == line_offset)
   {
      lseek (fd, pos, SEEK_SET);
      write (fd, (char *)&linestat, sizeof(struct _linestat));
   }

   close (fd);

   strcode (sysinfo.pwd, "YG&%FYTF%$RTD");
}

void read_sysinfo()
{
   int fd;
   char filename[80];
   long tempo;
   struct tm *tim;

   sprintf (filename, "%sSYSINFO.DAT", sys_path);
   fd = open(filename, O_BINARY|O_RDWR);
   if (fd == -1)
   {
      fd = open(filename, O_BINARY|O_RDWR|O_CREAT,S_IREAD|S_IWRITE);
      memset((char *)&sysinfo, 0, sizeof(struct _sysinfo));
      status_line ("!Creating SYSINFO.DAT file");
      write (fd, (char *)&sysinfo, sizeof(struct _sysinfo));
      lseek (fd, 0, SEEK_SET);
   }

   read(fd, (char *)&sysinfo, sizeof(struct _sysinfo));
   memset((char *)&linestat, 0, sizeof(struct _linestat));

   while (read(fd, (char *)&linestat, sizeof(struct _linestat)) == sizeof (struct _linestat))
      if (linestat.line == line_offset)
         break;

   if (linestat.line != line_offset)
   {
      tempo=time(0);
      tim=localtime(&tempo);
      sprintf(linestat.startdate,"%02d-%02d-%02d",tim->tm_mon+1,tim->tm_mday,tim->tm_year % 100);
      linestat.line = line_offset;
      write (fd, (char *)&linestat, sizeof(struct _linestat));
   }

   close (fd);

   if (sysinfo.pwd[0])
   {
      strcode (sysinfo.pwd, "YG&%FYTF%$RTD");
      password = sysinfo.pwd;
      locked = 1;
   }
}

void firing_up()
{
   char buffer[128];
   struct tm *tim;
   long tempo;

   tempo=time(0);
   tim=localtime(&tempo);

   sprintf(buffer,"* %s firing up at %d:%02d on %d-%s-%02d\n",
                   VERSION, tim->tm_hour, tim->tm_min,
                   tim->tm_mday, mtext[tim->tm_mon], tim->tm_year % 100);
   wputs(buffer);
   wputs("* Copyright (c) 1989-91 by Marco Maccaferri. All rights reserved.\n");
}

void get_down(errlev, flag)
int errlev, flag;
{
   struct tm *tim;
   long tempo;

   tempo=time(0);
   tim=localtime(&tempo);
   write_sysinfo();

   if (!frontdoor) {
      status_line(":End");
      fprintf(logf, "\n");
   }

   fclose(logf);

   cclrscrn(LGREY|_BLACK);

   if (flag == 1)
      printf("* Exit at start of event with errorlevel %d\n", errlev);
   else if (flag == 2)
      printf("* Exit after caller with errorlevel %d\n", errlev);
   else if (flag == 3)
      printf("* Exit with errorlevel %d\n", errlev);

   printf("* Lora-CBIS down at %d:%02d %d-%s-%02d\n",
               tim->tm_hour, tim->tm_min,
               tim->tm_mday, mtext[tim->tm_mon], tim->tm_year % 100);

   if (!registered)
      printf("* Don't forget to register.\n");

   gotoxy_(20,0);
   printf(msgtxt[M_THANKS], VERSION);

   if (!local_mode)
   {
      DTR_OFF();
      MDM_DISABLE();
   }

   showcur();
   exit ((errlev == -1) ? 0 : errlev);
}

static int fossil_inf(finfo)
struct _fossil_info far *finfo;
{
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
}

void status_window()
{
   int wh;
   char buffer[80];
   struct _fossil_info finfo;

   wh = wopen(9,14,17,64,1,LCYAN|_BLUE,LCYAN|_BLUE);
   wactiv(wh);
   whline(1,0,50,2,LCYAN|_BLUE);
   whline(3,0,50,2,LCYAN|_BLUE);
   wvline(4,15,3,3,LCYAN|_BLUE);
   wgotoxy (3, 15);
   wputc ('Â');

   fossil_inf(&finfo);

   sprintf(buffer, msgtxt[M_FOSSIL_TYPE], finfo.id);
   buffer[47] = '\0';
   wprints(0,0,LCYAN|_BLUE,buffer);
}

void fossil_version()
{
   char buffer[80];
   struct _fossil_info finfo;

   fossil_inf(&finfo);

   change_attr(LCYAN|_BLACK);
   sprintf(buffer, msgtxt[M_FOSSIL_TYPE], finfo.id);
   m_print("\n");
   m_print(buffer);
}

void terminating_call()
{
   int wh;

   wh = wopen(11,24,15,54,1,LCYAN|_BLUE,LCYAN|_BLUE);
   wactiv(wh);

   wcenters(1,LCYAN|_BLUE,"Terminating call");
   modem_hangup();
   update_user();
}

static void virtual_screen()
{
   int vbase=(&directvideo)[-1], vvirtual;
   _ES=vbase, _DI=0, _AX=0xfe00, __int__(0x10), vvirtual=_ES;
   if(vbase!=vvirtual)
   {
      (&directvideo)[-1]=vvirtual;
      _vinfo.videoseg=vvirtual;
   }
}

