#include <stdio.h>
#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <mem.h>
#include <string.h>
#include <stdlib.h>
#include <sys\stat.h>

#include "lora.h"

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

struct _lora_cfg {
   char sys_path[40];
   char menu_path[40];
   char txtfiles[40];
   char msg_base[40];
   char outbound[40];
   char nodelist[40];
   char ipc_path[40];
   char semaphores[40];
   char system_log[40];
   char temp_path[40];
   char name[40];
   char sysop[36];
   char location[40];
   char packet[10];
   struct _alias alias[10];
   struct class_rec class[12];
   char pwd[10];
   byte err_local;
   byte err_300;
   byte err_1200;
   byte err_2400;
   byte err_4800;
   byte err_7200;
   byte err_9600;
   byte err_12000;
   byte err_14400;
   byte err_19200;
   byte err_38400;
} cfg;

static void switches (void);
static void info (void);
static void paths (void);
static void site_info (void);
static void addresses (void);
static void input_system (void);
static void input_menus (void);
static void input_txtfiles (void);
static void input_msgbase (void);
static void input_outbound (void);
static void input_nodelist (void);
static void input_ipcpath (void);
static void input_semaphores (void);
static void input_temp (void);
static void input_system_log (void);
static void input_name (void);
static void input_sysop (void);
static void input_location (void);
static void input_packet (void);
static void add_backslash (char *);
static void input_alias (int);
static char *firstchar (char *, char *, int);
static void parse_netnode (char *, int *, int *, int *, int *);
static void input_limits (int);
static void edit_limits (void);
static void edit_errorlevels (void);

void main()
{
   int fd, x, y;

   videoinit ();
   hidecur ();
   readcur (&x, &y);

   fd = open ("CONFIG.DAT", O_RDWR|O_BINARY);
   if (fd == -1)
   {
      memset ((char *)&cfg, 0, sizeof (struct _lora_cfg));
      fd = open ("CONFIG.DAT", O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
   }
   else
      read (fd, (char *)&cfg, sizeof (struct _lora_cfg));

   lseek (fd, 0L, SEEK_SET);

   wopen (0, 0, 24, 79, 5, LGREY|_BLACK, LGREY|_BLACK);
   whline ( 1, 0, 80, 1, BLUE|_BLACK);
   whline (23, 0, 80, 1, BLUE|_BLACK);
   wfill (2, 0, 22, 79, '±', LGREY|_BLACK);
   wcenters (11, BLACK|_LGREY, " LoraBBS Setup 2.00 ");
   wcenters (13, BLACK|_LGREY, " Copyright (C) 1989-91 Marco Maccaferri. All Rights Reserved ");

   wmenubegc ();
   wmenuitem (0,  5," File    ", 'F', 1, M_HASPD, NULL, 0, 0);
      wmenubeg (1, 5, 6, 17, 1, LCYAN|_BLACK, LCYAN|_BLACK, NULL);
      wmenuitem (0, 0," Switches  ", 'S', 10, 0, switches, 0, 0);
      wmenuitem (1, 0," Info      ", 'I', 11, 0, info, 0, 0);
      wmenuitem (2, 0," DOS Shell ", 'D', 12, 0, NULL, 0, 0);
      wmenuitem (3, 0," Exit      ", 'E', 13, 0, NULL, 0, 0);
      wmenuend (10, M_VERT|M_PD|M_NOQS, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (0, 17," System  ", 'S', 2, M_HASPD, NULL, 0, 0);
      wmenubeg (1, 17, 6, 29, 1, LCYAN|_BLACK, LCYAN|_BLACK, NULL);
      wmenuitem (0, 0," Paths     ", 'P', 20, 0, paths, 0, 0);
      wmenuitem (1, 0," Site info ", 'S', 21, 0, site_info, 0, 0);
      wmenuitem (2, 0," Addresses ", 'A', 22, 0, addresses, 0, 0);
      wmenuitem (3, 0," Security  ", 'e', 23, 0, NULL, 0, 0);
      wmenuend (20, M_VERT|M_PD|M_NOQS, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (0, 31," Options ", 'O', 3, M_HASPD, NULL, 0, 0);
      wmenubeg (1, 31, 10, 46, 1, LCYAN|_BLACK, LCYAN|_BLACK, NULL);
      wmenuitem (0, 0," Messages     ", 'M', 30, 0, NULL, 0, 0);
      wmenuitem (1, 0," Files        ", 'F', 31, 0, NULL, 0, 0);
      wmenuitem (2, 0," Errorlevels  ", 'E', 32, 0, edit_errorlevels, 0, 0);
      wmenuitem (3, 0," Display      ", 'D', 33, 0, NULL, 0, 0);
      wmenuitem (4, 0," Paging       ", 'P', 34, 0, NULL, 0, 0);
      wmenuitem (5, 0," New users    ", 'N', 35, 0, NULL, 0, 0);
      wmenuitem (6, 0," System       ", 'S', 36, 0, NULL, 0, 0);
      wmenuitem (7, 0," Printer      ", 'r', 37, 0, NULL, 0, 0);
      wmenuend (30, M_VERT|M_PD|M_NOQS, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (0, 47," Modem   ", 'M', 4, M_HASPD, NULL, 0, 0);
      wmenubeg (1, 47, 5, 59, 1, LCYAN|_BLACK, LCYAN|_BLACK, NULL);
      wmenuitem (0, 0," Options   ", 'O', 40, 0, NULL, 0, 0);
      wmenuitem (1, 0," Commands  ", 'C', 41, 0, NULL, 0, 0);
      wmenuitem (2, 0," Responses ", 'R', 42, 0, NULL, 0, 0);
      wmenuend (40, M_VERT|M_PD|M_NOQS, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (0, 59," Manager ", 'a', 5, M_HASPD, NULL, 0, 0);
      wmenubeg (1, 59, 11, 72, 1, LCYAN|_BLACK, LCYAN|_BLACK, NULL);
      wmenuitem (0, 0," Msg areas  ", 'M', 50, 0, NULL, 0, 0);
      wmenuitem (1, 0," File areas ", 'F', 51, 0, NULL, 0, 0);
      wmenuitem (2, 0," Protocols  ", 'P', 52, 0, NULL, 0, 0);
      wmenuitem (3, 0," Languages  ", 'L', 53, 0, NULL, 0, 0);
      wmenuitem (4, 0," AltFn keys ", 'A', 54, 0, NULL, 0, 0);
      wmenuitem (5, 0," Events     ", 'E', 55, 0, NULL, 0, 0);
      wmenuitem (6, 0," Menus      ", 'M', 56, 0, NULL, 0, 0);
      wmenuitem (7, 0," Users      ", 'U', 57, 0, NULL, 0, 0);
      wmenuitem (8, 0," Limits     ", 'L', 58, 0, edit_limits, 0, 0);
      wmenuend (50, M_VERT|M_PD|M_NOQS, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);
   wmenuend (1, M_HORZ|M_NOQS, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

   wmenuget ();

   write (fd, (char *)&cfg, sizeof (struct _lora_cfg));
   close (fd);

   wclose ();
   gotoxy_ (x, y);
   showcur ();
}

void paths ()
{
   int wh;

   wh = wopen (8, 10, 21, 64, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wtitle (" Paths ", TRIGHT, YELLOW|_BLUE);

   wmenubegc ();
   wmenuitem (1, 1," System     ", 'S', 71, 0, input_system, 0, 0);
   wmenuitem (2, 1," Menus      ", 'M', 72, 0, input_menus, 0, 0);
   wmenuitem (3, 1," Textfiles  ", 'T', 73, 0, input_txtfiles, 0, 0);
   wmenuitem (4, 1," Msg base   ", 'M', 74, 0, input_msgbase, 0, 0);
   wmenuitem (5, 1," Outbound   ", 'O', 75, 0, input_outbound, 0, 0);
   wmenuitem (6, 1," Nodelist   ", 'N', 76, 0, input_nodelist, 0, 0);
   wmenuitem (7, 1," IPC path   ", 'I', 77, 0, input_ipcpath, 0, 0);
   wmenuitem (8, 1," Semaphores ", 'S', 78, 0, input_semaphores, 0, 0);
   wmenuitem (9, 1," Temp       ", 'T', 79, 0, input_temp, 0, 0);
   wmenuitem (10, 1," System log ", 'S', 80, 0, input_system_log, 0, 0);
   wmenuend (71, M_VERT, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

   wprints (1, 14, CYAN|_BLACK, cfg.sys_path);
   wprints (2, 14, CYAN|_BLACK, cfg.menu_path);
   wprints (3, 14, CYAN|_BLACK, cfg.txtfiles);
   wprints (4, 14, CYAN|_BLACK, cfg.msg_base);
   wprints (5, 14, CYAN|_BLACK, cfg.outbound);
   wprints (6, 14, CYAN|_BLACK, cfg.nodelist);
   wprints (7, 14, CYAN|_BLACK, cfg.ipc_path);
   wprints (8, 14, CYAN|_BLACK, cfg.semaphores);
   wprints (9, 14, CYAN|_BLACK, cfg.temp_path);
   wprints (10, 14, CYAN|_BLACK, cfg.system_log);

   wmenuget ();
   wclose ();
   wunlink (wh);
}

static void input_system ()
{
   winpbeg (BLUE|_GREEN, BLUE|_GREEN);
   winpdef (1, 14, cfg.sys_path, "??????????????????????????????????????", 'U', 2, NULL, 0);
   winpread ();
   hidecur ();
   wgotoxy (1, 14);
   wclreol ();
   add_backslash (cfg.sys_path);
   wprints (1, 14, CYAN|_BLACK, cfg.sys_path);
}

static void input_menus ()
{
   winpbeg (BLUE|_GREEN, BLUE|_GREEN);
   winpdef (2, 14, cfg.menu_path, "??????????????????????????????????????", 'U', 2, NULL, 0);
   winpread ();
   hidecur ();
   wgotoxy (2, 14);
   wclreol ();
   add_backslash (cfg.menu_path);
   wprints (2, 14, CYAN|_BLACK, cfg.menu_path);
}

static void input_txtfiles ()
{
   winpbeg (BLUE|_GREEN, BLUE|_GREEN);
   winpdef (3, 14, cfg.txtfiles, "??????????????????????????????????????", 'U', 2, NULL, 0);
   winpread ();
   hidecur ();
   wgotoxy (3, 14);
   wclreol ();
   add_backslash (cfg.txtfiles);
   wprints (3, 14, CYAN|_BLACK, cfg.txtfiles);
}

static void input_msgbase ()
{
   winpbeg (BLUE|_GREEN, BLUE|_GREEN);
   winpdef (4, 14, cfg.msg_base, "??????????????????????????????????????", 'U', 2, NULL, 0);
   winpread ();
   hidecur ();
   wgotoxy (4, 14);
   wclreol ();
   add_backslash (cfg.msg_base);
   wprints (4, 14, CYAN|_BLACK, cfg.msg_base);
}

static void input_outbound ()
{
   winpbeg (BLUE|_GREEN, BLUE|_GREEN);
   winpdef (5, 14, cfg.outbound, "??????????????????????????????????????", 'U', 2, NULL, 0);
   winpread ();
   hidecur ();
   wgotoxy (5, 14);
   wclreol ();
   add_backslash (cfg.outbound);
   wprints (5, 14, CYAN|_BLACK, cfg.outbound);
}

static void input_nodelist ()
{
   winpbeg (BLUE|_GREEN, BLUE|_GREEN);
   winpdef (6, 14, cfg.nodelist, "??????????????????????????????????????", 'U', 2, NULL, 0);
   winpread ();
   hidecur ();
   wgotoxy (6, 14);
   wclreol ();
   add_backslash (cfg.nodelist);
   wprints (6, 14, CYAN|_BLACK, cfg.nodelist);
}

static void input_ipcpath ()
{
   winpbeg (BLUE|_GREEN, BLUE|_GREEN);
   winpdef (7, 14, cfg.ipc_path, "??????????????????????????????????????", 'U', 2, NULL, 0);
   winpread ();
   hidecur ();
   wgotoxy (7, 14);
   wclreol ();
   add_backslash (cfg.ipc_path);
   wprints (7, 14, CYAN|_BLACK, cfg.ipc_path);
}

static void input_semaphores ()
{
   winpbeg (BLUE|_GREEN, BLUE|_GREEN);
   winpdef (8, 14, cfg.semaphores, "??????????????????????????????????????", 'U', 2, NULL, 0);
   winpread ();
   hidecur ();
   wgotoxy (8, 14);
   wclreol ();
   add_backslash (cfg.semaphores);
   wprints (8, 14, CYAN|_BLACK, cfg.semaphores);
}

static void input_temp ()
{
   winpbeg (BLUE|_GREEN, BLUE|_GREEN);
   winpdef (9, 14, cfg.temp_path, "??????????????????????????????????????", 'U', 2, NULL, 0);
   winpread ();
   hidecur ();
   wgotoxy (9, 14);
   wclreol ();
   add_backslash (cfg.temp_path);
   wprints (9, 14, CYAN|_BLACK, cfg.temp_path);
}

static void input_system_log ()
{
   winpbeg (BLUE|_GREEN, BLUE|_GREEN);
   winpdef (10, 14, cfg.system_log, "??????????????????????????????????????", 'U', 2, NULL, 0);
   winpread ();
   hidecur ();
   wgotoxy (10, 14);
   wclreol ();
   wprints (10, 14, CYAN|_BLACK, cfg.system_log);
}

void site_info ()
{
   int wh;

   wh = wopen (8, 6, 15, 60, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wtitle (" Site information ", TRIGHT, YELLOW|_BLUE);

   wmenubegc ();
   wmenuitem (1, 1," Name       ", 'N', 81, 0, input_name, 0, 0);
   wmenuitem (2, 1," Sysop      ", 'S', 82, 0, input_sysop, 0, 0);
   wmenuitem (3, 1," Location   ", 'L', 83, 0, input_location, 0, 0);
   wmenuitem (4, 1," QWK packet ", 'Q', 84, 0, input_packet, 0, 0);
   wmenuend (81, M_VERT, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

   wprints (1, 14, CYAN|_BLACK, cfg.name);
   wprints (2, 14, CYAN|_BLACK, cfg.sysop);
   wprints (3, 14, CYAN|_BLACK, cfg.location);
   wprints (4, 14, CYAN|_BLACK, cfg.packet);

   wmenuget ();
   wclose ();
   wunlink (wh);
}

static void input_name ()
{
   winpbeg (BLUE|_GREEN, BLUE|_GREEN);
   winpdef (1, 14, cfg.name, "??????????????????????????????????????", '?', 2, NULL, 0);
   winpread ();
   hidecur ();
   wgotoxy (1, 14);
   wclreol ();
   wprints (1, 14, CYAN|_BLACK, cfg.name);
}

static void input_sysop ()
{
   winpbeg (BLUE|_GREEN, BLUE|_GREEN);
   winpdef (2, 14, cfg.sysop, "??????????????????????????????????", 'M', 2, NULL, 0);
   winpread ();
   hidecur ();
   wgotoxy (2, 14);
   wclreol ();
   wprints (2, 14, CYAN|_BLACK, cfg.sysop);
}

static void input_location ()
{
   winpbeg (BLUE|_GREEN, BLUE|_GREEN);
   winpdef (3, 14, cfg.location, "??????????????????????????????????????", '?', 2, NULL, 0);
   winpread ();
   hidecur ();
   wgotoxy (3, 14);
   wclreol ();
   wprints (3, 14, CYAN|_BLACK, cfg.location);
}

static void input_packet ()
{
   winpbeg (BLUE|_GREEN, BLUE|_GREEN);
   winpdef (4, 14, cfg.packet, "????????", 'U', 2, NULL, 0);
   winpread ();
   hidecur ();
   wgotoxy (4, 14);
   wclreol ();
   wprints (4, 14, CYAN|_BLACK, cfg.packet);
}

void addresses ()
{
   int wh, i;
   char alias[10][20];

   wh = wopen (6, 35, 19, 66, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wtitle (" Addresses ", TRIGHT, YELLOW|_BLUE);
   i = 81;

   do {
      wmenubegc ();
      wmenuitem (1, 1," Main  ", 'N', 81, 0, NULL, 0, 0);
      wmenuitem (2, 1," Aka 1 ", 'S', 82, 0, NULL, 0, 0);
      wmenuitem (3, 1," Aka 2 ", 'S', 83, 0, NULL, 0, 0);
      wmenuitem (4, 1," Aka 3 ", 'S', 84, 0, NULL, 0, 0);
      wmenuitem (5, 1," Aka 4 ", 'S', 85, 0, NULL, 0, 0);
      wmenuitem (6, 1," Aka 5 ", 'S', 86, 0, NULL, 0, 0);
      wmenuitem (7, 1," Aka 6 ", 'S', 87, 0, NULL, 0, 0);
      wmenuitem (8, 1," Aka 7 ", 'S', 88, 0, NULL, 0, 0);
      wmenuitem (9, 1," Aka 8 ", 'S', 89, 0, NULL, 0, 0);
      wmenuitem (10, 1," Aka 9 ", 'S', 90, 0, NULL, 0, 0);
      wmenuend (i, M_VERT, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      for (i=0;i<10;i++)
      {
         sprintf (alias[i], "%u:%u/%u", cfg.alias[i].zone, cfg.alias[i].net, cfg.alias[i].node);
         wgotoxy (i+1, 9);
         wclreol ();
         wprints (i+1, 9, CYAN|_BLACK, alias[i]);
      }

      i = wmenuget ();
      if (i != W_ESCPRESS)
         input_alias (i - 81);

   } while (i >= 81 && i <= 90);

   wclose ();
   wunlink (wh);
}

static void input_alias (i)
int i;
{
   int point;
   char stringa[20];

   sprintf (stringa, "%u:%u/%u", cfg.alias[i].zone, cfg.alias[i].net, cfg.alias[i].node);
   winpbeg (BLUE|_GREEN, BLUE|_GREEN);
   winpdef (i+1, 9, stringa, "???????????????????", 'U', 2, NULL, 0);
   winpread ();
   hidecur ();
   parse_netnode (stringa, &cfg.alias[i].zone, &cfg.alias[i].net, &cfg.alias[i].node, &point);
   if (point);
}

static void switches ()
{
   int wh;

   wh = wopen (9, 17, 19, 62, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wtitle (" Command line switches ", TRIGHT, YELLOW|_BLUE);

   wprints (1, 0, LGREY|_BLACK, " LSETUP Command-line switches:");
   wprints (3, 0, LGREY|_BLACK, "  -B Force black & white (monochrome) mode");
   wprints (5, 0, LGREY|_BLACK, "  -L Use the language manager directly");
   wprints (6, 0, LGREY|_BLACK, "  -M Use the menu editor directly");
   wprints (7, 0, LGREY|_BLACK, "  -U Use the user editor directly");

   getch ();

   wclose ();
   wunlink (wh);
}

static void info ()
{
   int wh;

   wh = wopen (9, 21, 17, 62, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wtitle (" About LoraBBS ", TRIGHT, YELLOW|_BLUE);

   wcenters (1, LGREY|_BLACK, "LoraBBS 2.00");
   wcenters (3, LGREY|_BLACK, "Copyright (C) 1989-90 Marco Maccaferri");
   wcenters (5, LGREY|_BLACK, "All Rights Reserved");

   getch ();

   wclose ();
   wunlink (wh);
}

static void add_backslash (s)
char *s;
{
   int i;

   strtrim (s);
   i = strlen(s) - 1;
   if (s[i] != '\\')
      strcat (s, "\\");
}

static void parse_netnode(netnode, zone, net, node, point)
char *netnode;
int *zone, *net, *node, *point;
{
   char *p;

   *zone = cfg.alias[0].zone;
   *net = cfg.alias[0].net;
   *node = 0;
   *point = 0;

   p = netnode;

   /* If we have a zone (and the caller wants the zone to be passed back).. */

   if (strchr(netnode,':') && zone) {
      *zone=atoi(p);
      p = firstchar(p,":",2);
   }

   /* If we have a net number... */

   if (p && strchr(netnode,'/') && net) {
      *net=atoi(p);
      p=firstchar(p,"/",2);
   }

   /* We *always* need a node number... */

   if (p && node)
      *node=atoi(p);

   /* And finally check for a point number... */

   if (p && strchr(netnode,'.') && point) {
      p=firstchar(p,".",2);

      if (p)
         *point=atoi(p);
      else
         *point=0;
   }
}


static char *firstchar(strng, delim, findword)
char *strng, *delim;
int findword;
{
   int x, isw, sl_d, sl_s, wordno=0;
   char *string, *oldstring;

   /* We can't do *anything* if the string is blank... */

   if (! *strng)
      return NULL;

   string=oldstring=strng;

   sl_d=strlen(delim);

   for (string=strng;*string;string++)
   {
      for (x=0,isw=0;x <= sl_d;x++)
         if (*string==delim[x])
            isw=1;

      if (isw==0) {
         oldstring=string;
         break;
      }
   }

   sl_s=strlen(string);

   for (wordno=0;(string-oldstring) < sl_s;string++)
   {
      for (x=0,isw=0;x <= sl_d;x++)
         if (*string==delim[x])
         {
            isw=1;
            break;
         }

      if (!isw && string==oldstring)
         wordno++;

      if (isw && (string != oldstring))
      {
         for (x=0,isw=0;x <= sl_d;x++) if (*(string+1)==delim[x])
         {
            isw=1;
            break;
         }

         if (isw==0)
            wordno++;
      }

      if (wordno==findword)
         return((string==oldstring || string==oldstring+sl_s) ? string : string+1);
   }

   return NULL;
}

static void edit_limits ()
{
   int wh, i;

   wh = wopen (6, 28, 20, 42, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wtitle (" Limits ", TRIGHT, YELLOW|_BLUE);
   i = 81;

   do {
      wmenubegc ();
      wmenuitem (1,  1," Twit      ", 'T', 81, 0, NULL, 0, 0);
      wmenuitem (2,  1," Disgrace  ", 'D', 82, 0, NULL, 0, 0);
      wmenuitem (3,  1," Limited   ", 'L', 83, 0, NULL, 0, 0);
      wmenuitem (4,  1," Normal    ", 'N', 84, 0, NULL, 0, 0);
      wmenuitem (5,  1," Worthy    ", 'W', 85, 0, NULL, 0, 0);
      wmenuitem (6,  1," Privel    ", 'P', 86, 0, NULL, 0, 0);
      wmenuitem (7,  1," Favored   ", 'F', 87, 0, NULL, 0, 0);
      wmenuitem (8,  1," Extra     ", 'E', 88, 0, NULL, 0, 0);
      wmenuitem (9,  1," Clerk     ", 'C', 89, 0, NULL, 0, 0);
      wmenuitem (10, 1," Asstsysop ", 'A', 90, 0, NULL, 0, 0);
      wmenuitem (11, 1," Sysop     ", 'S', 91, 0, NULL, 0, 0);
      wmenuend (i, M_VERT, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      i = wmenuget ();
      if (i >= 81 && i <= 91)
         input_limits (i - 81);

   } while (i >= 81 && i <= 91);

   wclose ();
   wunlink (wh);
}

static void input_limits (num)
{
   int wh, i;
   char stringa[20];

   struct parse_list levels[] = {
      TWIT, " Twit ",
      DISGRACE, " Disgrace ",
      LIMITED, " Limited ",
      NORMAL, " Normal ",
      WORTHY, " Worthy ",
      PRIVIL, " Privel ",
      FAVORED, " Favored ",
      EXTRA, " Extra ",
      CLERK, " Clerk ",
      ASSTSYSOP, " Asstsysop ",
      SYSOP, " Sysop ",
      HIDDEN, " Hidden "
   };

   for (i=0;i<11;i++)
      cfg.class[i].priv = levels[i].p_length;

   wh = wopen (8, 35, 18, 67, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wtitle (levels[num].p_string, TRIGHT, YELLOW|_BLUE);
   i = 81;

   do {
      wmenubegc ();
      wmenuitem (1,  1," Time Limit per Call   ", 'C', 81, 0, NULL, 0, 0);
      wmenuitem (2,  1," Time Limit per Day    ", 'D', 82, 0, NULL, 0, 0);
      wmenuitem (3,  1," Minimum Logon Baud    ", 'L', 83, 0, NULL, 0, 0);
      wmenuitem (4,  1," Minimum Download Baud ", 'B', 84, 0, NULL, 0, 0);
      wmenuitem (5,  1," Download Limit (KB)   ", 'D', 85, 0, NULL, 0, 0);
      wmenuitem (6,  1," Download/Upload Ratio ", 'R', 86, 0, NULL, 0, 0);
      wmenuitem (7,  1," Ratio Start           ", 'S', 87, 0, NULL, 0, 0);
      wmenuend (i, M_VERT, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      sprintf (stringa, "%d", cfg.class[num].max_call);
      wgotoxy (1, 25);
      wclreol ();
      wprints (1, 25, CYAN|_BLACK, stringa);
      sprintf (stringa, "%d", cfg.class[num].max_time);
      wgotoxy (2, 25);
      wclreol ();
      wprints (2, 25, CYAN|_BLACK, stringa);
      sprintf (stringa, "%u", cfg.class[num].min_baud);
      wgotoxy (3, 25);
      wclreol ();
      wprints (3, 25, CYAN|_BLACK, stringa);
      sprintf (stringa, "%u", cfg.class[num].min_file_baud);
      wgotoxy (4, 25);
      wclreol ();
      wprints (4, 25, CYAN|_BLACK, stringa);
      sprintf (stringa, "%d", cfg.class[num].max_dl);
      wgotoxy (5, 25);
      wclreol ();
      wprints (5, 25, CYAN|_BLACK, stringa);
      sprintf (stringa, "%u", cfg.class[num].ratio);
      wgotoxy (6, 25);
      wclreol ();
      wprints (6, 25, CYAN|_BLACK, stringa);
      sprintf (stringa, "%u", cfg.class[num].start_ratio);
      wgotoxy (7, 25);
      wclreol ();
      wprints (7, 25, CYAN|_BLACK, stringa);

      i = wmenuget ();
      if (i != W_ESCPRESS)
         switch (i)
         {
            case 81:
               sprintf (stringa, "%d", cfg.class[num].max_call);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (1, 25, stringa, "?????", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               cfg.class[num].max_call = atoi (stringa);
               break;
            case 82:
               sprintf (stringa, "%d", cfg.class[num].max_time);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (2, 25, stringa, "?????", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               cfg.class[num].max_time = atoi (stringa);
               break;
            case 83:
               sprintf (stringa, "%d", cfg.class[num].min_baud);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (3, 25, stringa, "?????", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               cfg.class[num].min_baud = atoi (stringa);
               break;
            case 84:
               sprintf (stringa, "%d", cfg.class[num].min_file_baud);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (4, 25, stringa, "?????", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               cfg.class[num].min_file_baud = atoi (stringa);
               break;
            case 85:
               sprintf (stringa, "%d", cfg.class[num].max_dl);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (5, 25, stringa, "?????", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               cfg.class[num].max_dl = atoi (stringa);
               break;
            case 86:
               sprintf (stringa, "%d", cfg.class[num].ratio);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (6, 25, stringa, "?????", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               cfg.class[num].ratio = atoi (stringa);
               break;
            case 87:
               sprintf (stringa, "%d", cfg.class[num].start_ratio);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (7, 25, stringa, "?????", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               cfg.class[num].start_ratio = atoi (stringa);
               break;
         }

   } while (i >= 81 && i <= 87);

   wclose ();
   wunlink (wh);
}

static void edit_errorlevels ()
{
   int wh, i;
   char stringa[20];

   wh = wopen (4, 2, 18, 17, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wtitle (" Errorlevels ", TRIGHT, YELLOW|_BLUE);
   i = 81;

   do {
      wmenubegc ();
      wmenuitem (1,  1," Local ", 'L', 81, 0, NULL, 0, 0);
      wmenuitem (2,  1," 300   ", '3', 82, 0, NULL, 0, 0);
      wmenuitem (3,  1," 1200  ", '1', 83, 0, NULL, 0, 0);
      wmenuitem (4,  1," 2400  ", '2', 84, 0, NULL, 0, 0);
      wmenuitem (5,  1," 4800  ", '4', 85, 0, NULL, 0, 0);
      wmenuitem (6,  1," 7200  ", '7', 86, 0, NULL, 0, 0);
      wmenuitem (7,  1," 9600  ", '9', 87, 0, NULL, 0, 0);
      wmenuitem (8,  1," 12000 ", '1', 88, 0, NULL, 0, 0);
      wmenuitem (9,  1," 14400 ", '1', 89, 0, NULL, 0, 0);
      wmenuitem (10,  1," 19200 ", '1', 90, 0, NULL, 0, 0);
      wmenuitem (11,  1," 38400 ", '8', 91, 0, NULL, 0, 0);
      wmenuend (i, M_VERT, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      sprintf (stringa, "%d", cfg.err_local);
      wgotoxy (1, 9);
      wclreol ();
      wprints (1, 9, CYAN|_BLACK, stringa);
      sprintf (stringa, "%d", cfg.err_300);
      wgotoxy (2, 9);
      wclreol ();
      wprints (2, 9, CYAN|_BLACK, stringa);
      sprintf (stringa, "%d", cfg.err_1200);
      wgotoxy (3, 9);
      wclreol ();
      wprints (3, 9, CYAN|_BLACK, stringa);
      sprintf (stringa, "%d", cfg.err_2400);
      wgotoxy (4, 9);
      wclreol ();
      wprints (4, 9, CYAN|_BLACK, stringa);
      sprintf (stringa, "%d", cfg.err_4800);
      wgotoxy (5, 9);
      wclreol ();
      wprints (5, 9, CYAN|_BLACK, stringa);
      sprintf (stringa, "%d", cfg.err_7200);
      wgotoxy (6, 9);
      wclreol ();
      wprints (6, 9, CYAN|_BLACK, stringa);
      sprintf (stringa, "%d", cfg.err_9600);
      wgotoxy (7, 9);
      wclreol ();
      wprints (7, 9, CYAN|_BLACK, stringa);
      sprintf (stringa, "%d", cfg.err_12000);
      wgotoxy (8, 9);
      wclreol ();
      wprints (8, 9, CYAN|_BLACK, stringa);
      sprintf (stringa, "%d", cfg.err_14400);
      wgotoxy (9, 9);
      wclreol ();
      wprints (9, 9, CYAN|_BLACK, stringa);
      sprintf (stringa, "%d", cfg.err_19200);
      wgotoxy (10, 9);
      wclreol ();
      wprints (10, 9, CYAN|_BLACK, stringa);
      sprintf (stringa, "%d", cfg.err_38400);
      wgotoxy (11, 9);
      wclreol ();
      wprints (11, 9, CYAN|_BLACK, stringa);

      i = wmenuget ();
      if (i >= 81 && i <= 91)
         switch (i)
         {
            case 81:
               sprintf (stringa, "%d", cfg.err_local);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (1, 9, stringa, "???", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               cfg.err_local = atoi (stringa);
               break;
            case 82:
               sprintf (stringa, "%d", cfg.err_300);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (2, 9, stringa, "???", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               cfg.err_300 = atoi (stringa);
               break;
            case 83:
               sprintf (stringa, "%d", cfg.err_1200);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (3, 9, stringa, "???", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               cfg.err_1200 = atoi (stringa);
               break;
            case 84:
               sprintf (stringa, "%d", cfg.err_2400);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (4, 9, stringa, "???", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               cfg.err_2400 = atoi (stringa);
               break;
            case 85:
               sprintf (stringa, "%d", cfg.err_4800);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (5, 9, stringa, "???", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               cfg.err_4800 = atoi (stringa);
               break;
            case 86:
               sprintf (stringa, "%d", cfg.err_7200);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (6, 9, stringa, "???", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               cfg.err_7200 = atoi (stringa);
               break;
            case 87:
               sprintf (stringa, "%d", cfg.err_9600);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (7, 9, stringa, "???", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               cfg.err_9600 = atoi (stringa);
               break;
            case 88:
               sprintf (stringa, "%d", cfg.err_12000);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (8, 9, stringa, "???", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               cfg.err_12000 = atoi (stringa);
               break;
            case 89:
               sprintf (stringa, "%d", cfg.err_14400);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (9, 9, stringa, "???", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               cfg.err_14400 = atoi (stringa);
               break;
            case 90:
               sprintf (stringa, "%d", cfg.err_19200);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (10, 9, stringa, "???", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               cfg.err_19200 = atoi (stringa);
               break;
            case 91:
               sprintf (stringa, "%d", cfg.err_38400);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (11, 9, stringa, "???", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               cfg.err_38400 = atoi (stringa);
               break;
         }

   } while (i >= 81 && i <= 91);

   wclose ();
   wunlink (wh);
}

