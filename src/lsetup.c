#include <errno.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <alloc.h>
#include <conio.h>
#include <dir.h>
#include <dos.h>
#include <process.h>
#include <sys\stat.h>

extern unsigned int _stklen = 10240U;
unsigned _ovrbuffer = 0xB00;

#ifdef __OS2__
extern int errno = 0;
#endif

#include "lsetup.h"
#include "lprot.h"
#include "sched.h"
#include "version.h"

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>
#include <cxl\cxlkey.h>

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
   ASSTSYSOP, " Asst. Sysop ",
   SYSOP, " Sysop ",
   HIDDEN, " Hidden "
};

struct _configuration config;

int sh_open (char *file, int shmode, int omode, int fmode);
void update_message (void);
void parse_netnode2 (char *netnode, int *zone, int *net, int *node, int *point);

void mailonly_password (void);
void create_route_file (void);
void clear_window (void);
void linehelp_window (void);
void create_path (char *);
void modem_hardware (void);
void modem_command_strings (void);
void modem_dialing_strings (void);
void modem_flag_strings (void);
void modem_answer_control (void);
void mailer_log (void);
void mailer_filerequest (void);
void mailer_misc (void);
void mailer_tic (void);
void qwk_setup (void);
void write_areasbbs (void);
void bbs_newusers (void);
void bbs_general (void);
void bbs_message (void);
void bbs_language (void);
void bbs_files (void);
void bbs_login_limits (void);
void bbs_protocol (void);
void bbs_paging_hours (void);
void manager_nodelist (void);
void manager_packers (void);
void manager_nodes (void);
void manager_translation (void);
void manager_users (void);
void mailer_ext_processing (void);
void mailer_local_editor (void);
void mailer_areafix (void);
void terminal_misc (void);
void terminal_iemsi (void);
void manager_scheduler (void);
void general_time (void);
void manager_menu (void);
void mail_processing (void);
long get_config_crc (void);
void export_config (char *file);
void import_config (char *file, char *cfg);
void write_ticcfg (void);

static void internet_info (void);
static void read_config (char *);
static void save_config (char *);
static void import_areasbbs (void);
static void import_tic (void);
static void shell_to_dos (void);
static void directory_setup (void);
static void site_info (void);
static void addresses (void);
static void input_alias (int);
static char *firstchar (char *, char *, int);
static void add_shadow (void);
static void global_general (void);
static void edit_limits (void);
static void input_limits (int);
static void registration_info (void);
static void export_costfile (void);
static void import_costfile (void);

static long crc;

static void pre_help(void)
{
   wshadow (LGREY|_BLACK);
   wtitle (" Help ", TRIGHT, YELLOW|_BLUE);
}

void main (argc, argv)
int argc;
char *argv[];
{
   int x, y, wh, i;       // , end = 0;
   char string[80];

   if (argc > 1 && stricmp (argv[1], "EXPORT") && stricmp (argv[1], "IMPORT"))
      read_config (argv[1]);
   else
      read_config ("CONFIG.DAT");

/*
   for (i = 1; i < argc; i++) {
      if (!stricmp (argv[i], "EXPORT")) {
         export_config (argv[i + 1]);
         end = 1;
      }
      if (!stricmp (argv[i], "IMPORT")) {
         if (argc > 1 && stricmp (argv[1], "EXPORT") && stricmp (argv[1], "IMPORT"))
	    import_config (argv[i + 1], argv[1]);
         else
            import_config (argv[i + 1], "CONFIG.DAT");
         end = 1;
      }
   }

   if (end)
      exit (0);
*/

#ifdef __OS2__
   printf ("LSETUP-OS/2 v. %s", LSETUP_VERSION);
#else
   printf ("LSETUP-DOS v. %s", LSETUP_VERSION);
#endif

   videoinit ();
   if (config.mono_attr)
      setvparam (VP_MONO);
   else
      setvparam (VP_COLOR);
   hidecur ();
   x = wherex ();
   y = wherey ();


   /* initialize help system, help key = [F1] */
//   whelpdef ("LSETUP.HLP", 0x3B00, LCYAN|_BLACK, LGREY|_BLACK, WHITE|_BLACK, BLUE|_LGREY, pre_help);

   wopen (0, 0, 24, 79, 5, LGREY|_BLACK, LGREY|_BLACK);
   whline ( 1, 0, 80, 1, BLUE|_BLACK);
   whline (23, 0, 80, 1, BLUE|_BLACK);
   wfill (2, 0, 22, 79, '±', LGREY|_BLACK);
#ifdef __OS2__
   sprintf (string, " Lora E-Mail System Setup OS/2 %s ", LSETUP_VERSION);
#else
   sprintf (string, " Lora E-Mail System Setup DOS %s ", LSETUP_VERSION);
#endif
   wcenters (11, BLACK|_LGREY, string);
   wcenters (13, BLACK|_LGREY, " Copyright (C) 1989-95 Marco Maccaferri. All Rights Reserved ");
#ifdef __OCC__
   wcenters (14, BLACK|_LGREY, " Copyright (C) 1995 Old Colorado City Communications. All Rights Reserved ");
#endif

continue_editing:

   wmenubegc ();
   wmenuitem (0, 3, " File ", 'F', 100, M_HASPD, NULL, 0, 0);
      wmenubeg (1, 3, 12, 25, 3, LCYAN|_BLACK, LCYAN|_BLACK, add_shadow);
      wmenuitem (0, 0, " Registration        ", 0, 101, 0, registration_info, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (1, 0, " Write AREAS.BBS     ", 0, 103, 0, write_areasbbs, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (2, 0, " Write TIC.CFG       ", 0, 110, 0, write_ticcfg, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (3, 0, " Write ROUTE.CFG     ", 0, 107, 0, create_route_file, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (4, 0, " Write COST.CFG      ", 0, 109, 0, export_costfile, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (5, 0, " Import AREAS.BBS    ", 0, 105, 0, import_areasbbs, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (6, 0, " Import TIC.CFG      ", 0, 108, 0, import_tic, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (7, 0, " Import COST.CFG     ", 0, 110, 0, import_costfile, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (8, 0, " DOS shell           ", 0, 102, 0, shell_to_dos, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (9, 0, " Quit                ", 0, 106, M_CLALL, NULL, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuend (101, M_PD|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (0, 11, " Global ", 'G', 200, M_HASPD, NULL, 0, 0);
      wmenubeg (1, 11, 8, 30, 3, LCYAN|_BLACK, LCYAN|_BLACK, add_shadow);
      wmenuitem (0, 0, " Site info        ", 0, 204, 0, site_info, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (1, 0, " Directory/Paths  ", 0, 202, 0, directory_setup, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (2, 0, " Addresses        ", 0, 203, 0, addresses, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (3, 0, " General          ", 0, 201, 0, global_general, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (4, 0, " Time adjustment  ", 0, 205, 0, general_time, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (5, 0, " Internet address ", 0, 206, 0, internet_info, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuend (204, M_PD|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (0, 21, " Mailer ", 'M', 300, M_HASPD, NULL, 0, 0);
      wmenubeg (1, 21, 11, 42, 3, LCYAN|_BLACK, LCYAN|_BLACK, add_shadow);
      wmenuitem (0, 0, " Miscellaneous      ", 0, 301, 0, mailer_misc, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (1, 0, " Log                ", 0, 302, 0, mailer_log, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (2, 0, " File requests      ", 0, 303, 0, mailer_filerequest, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (3, 0, " Areafix            ", 0, 306, 0, mailer_areafix, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (4, 0, " TIC Processor      ", 0, 311, 0, mailer_tic, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (5, 0, " Ext. processing    ", 0, 307, 0, mailer_ext_processing, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (6, 0, " Message editor     ", 0, 308, 0, mailer_local_editor, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (7, 0, " Mail processing    ", 0, 309, 0, mail_processing, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (8, 0, " Mail-only password ", 0, 310, 0, mailonly_password, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuend (301, M_PD|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (0, 31, " BBS ", 'B', 400, M_HASPD, NULL, 0, 0);
      wmenubeg (1, 31, 12, 53, 3, LCYAN|_BLACK, LCYAN|_BLACK, add_shadow);
      wmenuitem (0, 0, " Messages            ", 0, 401, 0, bbs_message, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (1, 0, " Files               ", 0, 402, 0, bbs_files, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (2, 0, " QWK setup           ", 0, 403, 0, qwk_setup, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (3, 0, " New users           ", 0, 404, 0, bbs_newusers, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (4, 0, " General options     ", 0, 405, 0, bbs_general, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (5, 0, " Limits              ", 0, 406, 0, edit_limits, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (6, 0, " Login limits        ", 0, 407, 0, bbs_login_limits, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (7, 0, " Paging hours        ", 0, 410, 0, bbs_paging_hours, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (8, 0, " Language            ", 0, 408, 0, bbs_language, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (9, 0, " Ext. protocols      ", 0, 409, 0, bbs_protocol, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuend (401, M_PD|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (0, 38, " Terminal ", 'F', 500, M_HASPD, NULL, 0, 0);
      wmenubeg (1, 38, 4, 54, 3, LCYAN|_BLACK, LCYAN|_BLACK, add_shadow);
      wmenuitem (0, 0, " Miscellaneous ", 0, 501, 0, terminal_misc, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (1, 0, " IEMSI Profile ", 0, 502, 0, terminal_iemsi, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuend (501, M_PD|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (0, 50, " Modem ", 'F', 600, M_HASPD, NULL, 0, 0);
      wmenubeg (1, 50, 7, 68, 3, LCYAN|_BLACK, LCYAN|_BLACK, add_shadow);
      wmenuitem (0, 0, " Hardware        ", 0, 601, 0, modem_hardware, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (1, 0, " Command strings ", 0, 602, 0, modem_command_strings, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (2, 0, " Dialing strings ", 0, 605, 0, modem_dialing_strings, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (3, 0, " Answer control  ", 0, 603, 0, modem_answer_control, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (4, 0, " Nodelist flags  ", 0, 604, 0, modem_flag_strings, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuend (601, M_PD|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (0, 59, " Manager ", 'F', 700, M_HASPD, NULL, 0, 0);
      wmenubeg (1, 59, 9, 76, 3, LCYAN|_BLACK, LCYAN|_BLACK, add_shadow);
      wmenuitem (0, 0, " Events         ", 0, 701, 0, manager_scheduler, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (1, 0, " Nodelist       ", 0, 702, 0, manager_nodelist, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (2, 0, " Translations   ", 0, 703, 0, manager_translation, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (3, 0, " Packers        ", 0, 704, 0, manager_packers, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (4, 0, " Nodes          ", 0, 705, 0, manager_nodes, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (5, 0, " Menu           ", 0, 706, 0, manager_menu, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuitem (6, 0, " Users          ", 0, 707, 0, manager_users, 0, 0);
      wmenuiba (linehelp_window, clear_window);
      wmenuend (701, M_PD|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);

   wmenuend (100, M_HORZ, 0, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);

   kbput (0x1C0D);
   wmenuget ();

   if (crc != get_config_crc ()) {
      wh = wopen (10, 25, 14, 54, 0, BLACK|_LGREY, BLACK|_LGREY);
      wactiv (wh);
      wshadow (DGREY|_BLACK);

      wcenters (1, BLACK|_LGREY, "Save setup (Y/n) ?  ");

      strcpy (string, "Y");
      winpbeg (BLACK|_LGREY, BLACK|_LGREY);
      winpdef (1, 23, string, "?", 0, 2, NULL, 0);

      i = winpread ();
      wclose ();

      if (i == W_ESCPRESS) {
         hidecur ();
         goto continue_editing;
      }

      if (toupper (string[0]) == 'Y')
         save_config (argc > 1 ? argv[1] : "CONFIG.DAT");
   }

   wclose ();
   gotoxy (x, y);
   showcur ();
}

static void add_shadow ()
{
   wshadow (DGREY|_BLACK);
}

static void global_general ()
{
   int wh, i = 1;
   char string[128];

   wh = wopen (5, 16, 16, 50, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" General ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " CGA \"snow\" checking  ", 0,  1, 0, NULL, 0, 0);
      wmenuitem ( 2,  1, " Monochrome           ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " Direct screen writes ", 0,  3, 0, NULL, 0, 0);
      wmenuitem ( 4,  1, " Screenblanker time   ", 0,  4, 0, NULL, 0, 0);
      wmenuitem ( 5,  1, " À Blanker type       ", 0,  8, 0, NULL, 0, 0);
      wmenuitem ( 6,  1, " Line number          ", 0,  5, 0, NULL, 0, 0);
      wmenuitem ( 7,  1, " Multiline system     ", 0,  6, 0, NULL, 0, 0);
      wmenuitem ( 8,  1, " ALT-X errorlevel     ", 0,  7, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 24, CYAN|_BLACK, config.snow_check ? "Yes" : "No");
      wprints (2, 24, CYAN|_BLACK, config.mono_attr ? "Yes" : "No");
      wprints (3, 24, CYAN|_BLACK, config.no_direct ? "No" : "Yes");
      sprintf (string, "%d", config.blank_timer);
      wprints (4, 24, CYAN|_BLACK, string);
      if (config.blanker_type == 0)
         wprints (5, 24, CYAN|_BLACK, "Blank");
      else if (config.blanker_type == 1)
         wprints (5, 24, CYAN|_BLACK, "Stars");
      else if (config.blanker_type == 2)
         wprints (5, 24, CYAN|_BLACK, "Snakes");
      sprintf (string, "%d", config.line_offset);
      wprints (6, 24, CYAN|_BLACK, string);
      wprints (7, 24, CYAN|_BLACK, config.multitask ? "Yes" : "No");
      sprintf (string, "%d", config.altx_errorlevel);
      wprints (8, 24, CYAN|_BLACK, string);

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            config.snow_check ^= 1;
            if (config.snow_check)
               setvparam (config.no_direct ? VP_CGA : VP_BIOS);
            break;

         case 2:
            config.mono_attr ^= 1;
            if (config.mono_attr)
               setvparam (VP_MONO);
            else
               setvparam (VP_COLOR);
            break;

         case 3:
            config.no_direct ^= 1;
            if (config.no_direct)
               setvparam (VP_BIOS);
            else
               setvparam (config.snow_check ? VP_CGA : VP_DMA);
            break;

         case 4:
            sprintf (string, "%d", config.blank_timer);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 24, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.blank_timer = atoi (string);
	    break;

         case 5:
            sprintf (string, "%d", config.line_offset);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (6, 24, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.line_offset = atoi (string);
            break;

         case 6:
            config.multitask ^= 1;
            break;

         case 7:
            sprintf (string, "%d", config.altx_errorlevel);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (8, 24, string, "????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               config.altx_errorlevel = atoi (string) % 256;
            break;

         case 8:
            if (++config.blanker_type >= 3)
               config.blanker_type = 0;
            break;
      }

      hidecur ();

   } while (i != -1);

   wclose ();
}

static void shell_to_dos ()
{
   int *varr;
   char *cmd, cmdname[128], cpath[80];

   getcwd (cpath, 79);
   cmd = getenv ("COMSPEC");
   strcpy (cmdname, (cmd == NULL) ? "command.com" : cmd);

   if ((varr = ssave ()) == NULL)
      return;

   cclrscrn (LGREY|_BLACK);
   showcur();
   gotoxy (1, 8);
#ifdef __OS2__
   printf ("\nLSETUP-OS/2 OS Shell - Type EXIT To Return\n");
#else
   printf ("\nLSETUP-DOS DOS Shell - Type EXIT To Return - Free RAM %ld\n", farcoreleft ());
#endif

   spawnl (P_WAIT, cmdname, cmdname, NULL);

   setdisk (cpath[0] - 'A');
   chdir (cpath);
   srestore (varr);

   hidecur();
}

typedef struct {
   int zone;
   int net;
   int node;
   int point;
   bit passive :1;
   bit receive :1;
   bit send    :1;               
   bit private :1;
} LINK;

#define MAX_LINKS 128

static int sort_func (const void *a1, const void *b1)
{
   LINK *a, *b;

   a = (LINK *)a1;
   b = (LINK *)b1;

   if (a->zone != b->zone)   return (a->zone - b->zone);
   if (a->net != b->net)   return (a->net - b->net);
   if (a->node != b->node)   return (a->node - b->node);
   return ( (int)(a->point - b->point) );
}

static void import_areasbbs ()
{
   FILE *fp;
   int fd, wh, found, lastarea, i, zo, ne, no, po, total = 0, cc, nlink, m, cf;
   char string[260], *location, *tag, *forward, *p, addr[30], linea[80];
   struct _sys sys;
   LINK *link;

   wh = wopen (11, 7, 13, 73, 0, LGREY|_BLACK, LCYAN|_BLACK);
   wactiv (wh);
   wtitle ("IMPORT AREAS.BBS", TLEFT, LCYAN|_BLACK);

   wprints (0, 1, YELLOW|_BLACK, "Filename");
   if (config.areas_bbs[0])
      strcpy (string, config.areas_bbs);
   else
      sprintf (string, "%sAREAS.BBS", config.sys_path);
   winpbeg (BLACK|_LGREY, BLACK|_LGREY);
   winpdef (0, 10, string, "?????????????????????????????????????????????????????", 0, 2, NULL, 0);
   i = winpread ();

   hidecur ();
   wclose ();

   if (i == W_ESCPRESS)
      return;

   strbtrim (string);

   fp = fopen (string, "rt");
   if (fp == NULL)
      return;

   link = (LINK *)malloc (sizeof (LINK) * MAX_LINKS);
   if (link == NULL) {
      fclose (fp);
      return;
   }

   sprintf (string, "%sSYSMSG.DAT", config.sys_path);
   fd = open (string, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);

   fgets (string, 255, fp);

   while (fgets(string, 255, fp) != NULL) {
      if ((p = strchr (string, ';')) != NULL)
         *p = '\0';
      if (string[0] == '\0')
         continue;

      while (string[strlen (string) -1] == 0x0D || string[strlen (string) -1] == 0x0A || string[strlen (string) -1] == ' ')
         string[strlen (string) -1] = '\0';

      if ((location = strtok (string, " ")) == NULL)
         continue;
      location = strbtrim (location);
      if (strlen (location) > 39)
         location[39] = '\0';
      if ((tag = strtok (NULL, " ")) == NULL)
         continue;
      tag = strbtrim (tag);
      if (strlen (tag) > 31)
	 tag[31] = '\0';

      if ((forward = strtok (NULL, "")) == NULL)
	 continue;
      forward = strbtrim (forward);

      nlink = 0;
      zo = config.alias[0].zone;
      ne = config.alias[0].net;
      no = config.alias[0].node;
      po = config.alias[0].point;

      p = strtok (forward, " ");
      if (p != NULL)
	 do {

	    if (strstr(p,">")) link[nlink].receive=1;
	    else link[nlink].receive=0;
	    if (strstr(p,"<")) link[nlink].send=1;
	    else link[nlink].send=0;
	    if (strstr(p,"P")||strstr(p,"p")) link[nlink].private=1;
	    else link[nlink].private=0;
	    if (strstr(p,"!")) link[nlink].passive=1;
	    else link[nlink].passive=0;
            
            parse_netnode2 (p, &zo, &ne, &no, &po);
            link[nlink].zone = zo;
            link[nlink].net = ne;
            link[nlink].node = no;
            link[nlink].point = po;
            nlink++;
            if (nlink >= MAX_LINKS)
               break;
	 } while ((p = strtok (NULL, " ")) != NULL);

      qsort (link, nlink, sizeof (LINK), sort_func);

      lseek (fd, 0L, SEEK_SET);
      cc = lastarea = found = 0;

      while (read (fd, (char *)&sys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
         if (sys.msg_num > lastarea)
            lastarea = sys.msg_num;

         cc++;

         if (!stricmp (sys.echotag, tag)) {
            cf = 1;
            linea[0] = '\0';
            zo = po = ne = no = 0;

            for (m = 0; m < nlink; m++) {
               if (zo != link[m].zone) {
                  if (link[m].point)
                     sprintf (addr, "%d:%d/%d.%d ", link[m].zone, link[m].net, link[m].node, link[m].point);
                  else
                     sprintf (addr, "%d:%d/%d ", link[m].zone, link[m].net, link[m].node);
                  zo = link[m].zone;
		  ne = link[m].net;
                  no = link[m].node;
                  po = link[m].point;
               }
               else if (ne != link[m].net) {
                  if (link[m].point)
                     sprintf (addr, "%d/%d.%d ", link[m].net, link[m].node, link[m].point);
                  else
                     sprintf (addr, "%d/%d ", link[m].net, link[m].node);
                  ne = link[m].net;
		  no = link[m].node;
                  po = link[m].point;
               }
               else if (no != link[m].node) {
                  if (link[m].point)
                     sprintf (addr, "%d.%d ", link[m].node, link[m].point);
                  else
                     sprintf (addr, "%d ", link[m].node);
                  no = link[m].node;
                  po = link[m].point;
               }
               else if (link[m].point && po != link[m].point) {
                  sprintf (addr, ".%d ", link[m].point);
                  po = link[m].point;
               }
               else
                  strcpy (addr, "");

               if (strlen (linea) + strlen (addr) >= 58) {
                  if (cf == 1) {
                     strcpy (sys.forward1, linea);
                     cf++;
                  }
                  else if (cf == 2) {
                     strcpy (sys.forward2, linea);
		     cf++;
                  }
                  else if (cf == 3) {
                     strcpy (sys.forward3, linea);
                     cf++;
                  }

                  linea[0] = '\0';

                  if (link[m].point)
		     sprintf (addr, "%d:%d/%d.%d ", link[m].zone, link[m].net, link[m].node, link[m].point);
		  else
		     sprintf (addr, "%d:%d/%d ", link[m].zone, link[m].net, link[m].node);
		  zo = link[m].zone;
		  ne = link[m].net;
		  no = link[m].node;
		  po = link[m].point;
	       }

	       if(link[m].receive) strcat(linea,">");
	       if(link[m].send) strcat(linea,"<");
	       if(link[m].private) strcat(linea,"P");
	       if(link[m].passive) strcat(linea,"!");
	       strcat (linea, addr);
	    }

	    if (strlen (linea) > 2) {
	       if (cf == 1) {
		  strcpy (sys.forward1, linea);
		  cf++;
	       }
	       else if (cf == 2) {
		  strcpy (sys.forward2, linea);
		  cf++;
	       }
	       else if (cf == 3) {
		  strcpy (sys.forward3, linea);
		  cf++;
	       }
	    }

	    for (i = 0; i < MAX_ALIAS && config.alias[i].net; i++)
	       if (link[0].zone == config.alias[i].zone)
		  break;
	    if (i < MAX_ALIAS && config.alias[i].net)
	       sys.use_alias = i;

	    sys.passthrough = 0;
	    sys.quick_board = 0;
	    sys.gold_board = 0;
	    sys.msg_path[0] = '\0';

	    if (!strcmp (location, "##"))
	       sys.passthrough = 1;
	    else if (atoi (location))
	       sys.quick_board = atoi (location);
	    else if (toupper (location[0]) == 'G' && atoi (&location[1]))
	       sys.gold_board = atoi (++location);
	    else if (*location == '$') {
	       strcpy (sys.msg_path, ++location);
	       sys.squish = 1;
	    }
	    else if (*location == '!')
	       sys.pip_board = atoi(++location);
	    else
	       strcpy (sys.msg_path, location);

	    sys.netmail = 0;
	    sys.echomail = 1;

	    lseek (fd, -1L * SIZEOF_MSGAREA, SEEK_CUR);
	    write (fd, (char *)&sys, SIZEOF_MSGAREA);

	    found = 1;
	    break;
	 }
      }

      if (!found) {
	 memset ((char *)&sys, 0, SIZEOF_MSGAREA);
	 sys.msg_num = lastarea + 1;
	 strcpy (sys.echotag, tag);
	 strcpy (sys.msg_name, tag);

	 cf = 1;
	 linea[0] = '\0';
	 zo = po = ne = no = 0;

	 for (m = 0; m < nlink; m++) {
	    if (zo != link[m].zone) {
	       if (link[m].point)
		  sprintf (addr, "%d:%d/%d.%d ", link[m].zone, link[m].net, link[m].node, link[m].point);
	       else
		  sprintf (addr, "%d:%d/%d ", link[m].zone, link[m].net, link[m].node);
	       zo = link[m].zone;
	       ne = link[m].net;
	       no = link[m].node;
	       po = link[m].point;
	    }
	    else if (ne != link[m].net) {
	       if (link[m].point)
		  sprintf (addr, "%d/%d.%d ", link[m].net, link[m].node, link[m].point);
	       else
		  sprintf (addr, "%d/%d ", link[m].net, link[m].node);
	       ne = link[m].net;
	       no = link[m].node;
	       po = link[m].point;
	    }
	    else if (no != link[m].node) {
	       if (link[m].point)
		  sprintf (addr, "%d.%d ", link[m].node, link[m].point);
	       else
		  sprintf (addr, "%d ", link[m].node);
	       no = link[m].node;
	       po = link[m].point;
	    }
	    else if (link[m].point && po != link[m].point) {
	       sprintf (addr, ".%d ", link[m].point);
	       po = link[m].point;
	    }
	    else
	       strcpy (addr, "");

	    if (strlen (linea) + strlen (addr) >= 58) {
	       if (cf == 1) {
		  strcpy (sys.forward1, linea);
		  cf++;
	       }
	       else if (cf == 2) {
		  strcpy (sys.forward2, linea);
		  cf++;
	       }
	       else if (cf == 3) {
		  strcpy (sys.forward3, linea);
		  cf++;
	       }

	       linea[0] = '\0';

	       if (link[m].point)
		  sprintf (addr, "%d:%d/%d.%d ", link[m].zone, link[m].net, link[m].node, link[m].point);
	       else
		  sprintf (addr, "%d:%d/%d ", link[m].zone, link[m].net, link[m].node);
	       zo = link[m].zone;
	       ne = link[m].net;
	       no = link[m].node;
	       po = link[m].point;
	    }

	    strcat (linea, addr);
	 }

	 if (strlen (linea) > 2) {
	    if (cf == 1) {
	       strcpy (sys.forward1, linea);
	       cf++;
	    }
	    else if (cf == 2) {
	       strcpy (sys.forward2, linea);
	       cf++;
	    }
	    else if (cf == 3) {
	       strcpy (sys.forward3, linea);
	       cf++;
	    }
	 }

	 sys.msg_priv = sys.write_priv = SYSOP;
	 sys.max_msgs = 200;
	 sys.max_age = 14;

	 for (i = 0; i < MAX_ALIAS && config.alias[i].net; i++)
	    if (link[0].zone == config.alias[i].zone)
	       break;
	 if (i < MAX_ALIAS && config.alias[i].net)
	    sys.use_alias = i;

	 sys.echomail = 1;

	 if (location[0] == '#') {
	    sys.passthrough = 1;
	    sys.echomail = 0;
         }
         else if (atoi (location))
            sys.quick_board = atoi (location);
         else if (*location == '$') {
            strcpy (sys.msg_path, ++location);
            sys.squish = 1;
         }
         else if (*location == '!')
            sys.pip_board = atoi(++location);
         else
            strcpy (sys.msg_path, location);

         write (fd, (char *)&sys, SIZEOF_MSGAREA);
      }

      total++;
   }

   close (fd);
   fclose (fp);
}

static void directory_setup ()
{
   int wh, i = 1;
   char string[128];

   wh = wopen (4, 9, 21, 73, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Directory/Paths ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " Main Directory     ", 0,  1, 0, NULL, 0, 0);
      wmenuitem ( 2,  1, " Normal inbound     ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " Know inbound       ", 0,  3, 0, NULL, 0, 0);
      wmenuitem ( 4,  1, " Prot inbound       ", 0,  4, 0, NULL, 0, 0);
      wmenuitem ( 5,  1, " Outbound           ", 0,  5, 0, NULL, 0, 0);
      wmenuitem ( 6,  1, " Netmail Messages   ", 0,  6, 0, NULL, 0, 0);
      wmenuitem ( 7,  1, " Bad Messages       ", 0,  7, 0, NULL, 0, 0);
      wmenuitem ( 8,  1, " Duplicate Messages ", 0,  8, 0, NULL, 0, 0);
      wmenuitem ( 9,  1, " Quick directory    ", 0,  9, 0, NULL, 0, 0);
      wmenuitem (10,  1, " Pip-base path      ", 0, 10, 0, NULL, 0, 0);
      wmenuitem (11,  1, " IPC path           ", 0, 11, 0, NULL, 0, 0);
      wmenuitem (12,  1, " Nodelist           ", 0, 12, 0, NULL, 0, 0);
      wmenuitem (13,  1, " Temporary path     ", 0, 13, 0, NULL, 0, 0);
      wmenuitem (14,  1, " Filebox path       ", 0, 14, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 22, CYAN|_BLACK, config.sys_path);
      wprints (2, 22, CYAN|_BLACK, config.norm_filepath);
      wprints (3, 22, CYAN|_BLACK, config.know_filepath);
      wprints (4, 22, CYAN|_BLACK, config.prot_filepath);
      wprints (5, 22, CYAN|_BLACK, config.outbound);
      wprints (6, 22, CYAN|_BLACK, config.netmail_dir);
      wprints (7, 22, CYAN|_BLACK, config.bad_msgs);
      wprints (8, 22, CYAN|_BLACK, config.dupes);
      wprints (9, 22, CYAN|_BLACK, config.quick_msgpath);
      wprints (10, 22, CYAN|_BLACK, config.pip_msgpath);
      wprints (11, 22, CYAN|_BLACK, config.ipc_path);
      wprints (12, 22, CYAN|_BLACK, config.net_info);
      wprints (13, 22, CYAN|_BLACK, config.flag_dir);
      wprints (14, 22, CYAN|_BLACK, config.boxpath);

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            strcpy (string, config.sys_path);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (1, 22, string, "??????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (config.sys_path, strbtrim (string));
               if (config.sys_path[0] && config.sys_path[strlen (config.sys_path) - 1] != '\\')
                  strcat (config.sys_path, "\\");
               create_path (config.sys_path);
            }
            break;

         case 2:
            strcpy (string, config.norm_filepath);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (2, 22, string, "??????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (config.norm_filepath, strbtrim (string));
               if (config.norm_filepath[0] && config.norm_filepath[strlen (config.norm_filepath) - 1] != '\\')
                  strcat (config.norm_filepath, "\\");
               create_path (config.norm_filepath);
            }
            break;

         case 3:
            strcpy (string, config.know_filepath);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 22, string, "??????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (config.know_filepath, strbtrim (string));
               if (config.know_filepath[0] && config.know_filepath[strlen (config.know_filepath) - 1] != '\\')
                  strcat (config.know_filepath, "\\");
               create_path (config.know_filepath);
            }
            break;

         case 4:
            strcpy (string, config.prot_filepath);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 22, string, "??????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (config.prot_filepath, strbtrim (string));
               if (config.prot_filepath[0] && config.prot_filepath[strlen (config.prot_filepath) - 1] != '\\')
                  strcat (config.prot_filepath, "\\");
               create_path (config.prot_filepath);
            }
            break;

         case 5:
            strcpy (string, config.outbound);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (5, 22, string, "??????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (config.outbound, strbtrim (string));
               if (config.outbound[0] && config.outbound[strlen (config.outbound) - 1] != '\\')
                  strcat (config.outbound, "\\");
               create_path (config.outbound);
            }
            break;

         case 6:
            strcpy (string, config.netmail_dir);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (6, 22, string, "??????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (config.netmail_dir, strbtrim (string));
               if (config.netmail_dir[0] && config.netmail_dir[strlen (config.netmail_dir) - 1] != '\\')
                  strcat (config.netmail_dir, "\\");
               create_path (config.netmail_dir);
            }
            break;

         case 7:
            strcpy (string, config.bad_msgs);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (7, 22, string, "??????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (config.bad_msgs, strbtrim (string));
               if (config.bad_msgs[0] && config.bad_msgs[strlen (config.bad_msgs) - 1] != '\\')
                  strcat (config.bad_msgs, "\\");
               create_path (config.bad_msgs);
            }
            break;

         case 8:
            strcpy (string, config.dupes);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (8, 22, string, "??????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (config.dupes, strbtrim (string));
               if (config.dupes[0] && config.dupes[strlen (config.dupes) - 1] != '\\')
                  strcat (config.dupes, "\\");
               create_path (config.dupes);
            }
            break;

         case 9:
            strcpy (string, config.quick_msgpath);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (9, 22, string, "??????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (config.quick_msgpath, strbtrim (string));
               if (config.quick_msgpath[0] && config.quick_msgpath[strlen (config.quick_msgpath) - 1] != '\\')
                  strcat (config.quick_msgpath, "\\");
               create_path (config.quick_msgpath);
            }
            break;

         case 10:
            strcpy (string, config.pip_msgpath);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (10, 22, string, "??????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (config.pip_msgpath, strbtrim (string));
               if (config.pip_msgpath[0] && config.pip_msgpath[strlen (config.pip_msgpath) - 1] != '\\')
                  strcat (config.pip_msgpath, "\\");
               create_path (config.pip_msgpath);
            }
            break;

         case 11:
            strcpy (string, config.ipc_path);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (11, 22, string, "??????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (config.ipc_path, strbtrim (string));
               if (config.ipc_path[0] && config.ipc_path[strlen (config.ipc_path) - 1] != '\\')
                  strcat (config.ipc_path, "\\");
               create_path (config.ipc_path);
            }
            break;

         case 12:
            strcpy (string, config.net_info);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (12, 22, string, "??????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (config.net_info, strbtrim (string));
               if (config.net_info[0] && config.net_info[strlen (config.net_info) - 1] != '\\')
                  strcat (config.net_info, "\\");
               create_path (config.net_info);
            }
            break;

         case 13:
            strcpy (string, config.flag_dir);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (13, 22, string, "??????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (config.flag_dir, strbtrim (string));
               if (config.flag_dir[0] && config.flag_dir[strlen (config.flag_dir) - 1] != '\\')
                  strcat (config.flag_dir, "\\");
               create_path (config.flag_dir);
            }
            break;

         case 14:
            strcpy (string, config.boxpath);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (14, 22, string, "??????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (config.boxpath, strbtrim (string));
               if (config.boxpath[0] && config.boxpath[strlen (config.boxpath) - 1] != '\\')
                  strcat (config.boxpath, "\\");
               create_path (config.boxpath);
            }
            break;
      }

      hidecur ();

   } while (i != -1);

   wclose ();
}

static void site_info ()
{
   int wh, i = 1;
   char string[128];

   wh = wopen (8, 5, 16, 73, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Site info ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem (1, 1," System Name  ", 'N', 1, 0, NULL, 0, 0);
      wmenuitem (2, 1," Sysop        ", 'S', 2, 0, NULL, 0, 0);
      wmenuitem (3, 1," Location     ", 'L', 3, 0, NULL, 0, 0);
      wmenuitem (4, 1," Phone Number ", 'P', 4, 0, NULL, 0, 0);
      wmenuitem (5, 1," Flags        ", 'F', 5, 0, NULL, 0, 0);
      wmenuend (i, M_VERT|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 16, CYAN|_BLACK, config.system_name);
      wprints (2, 16, CYAN|_BLACK, config.sysop);
      wprints (3, 16, CYAN|_BLACK, config.location);
      wprints (4, 16, CYAN|_BLACK, config.phone);
      wprints (5, 16, CYAN|_BLACK, config.flags);

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            strcpy (string, config.system_name);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (1, 16, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.system_name, strbtrim (string));
            hidecur ();
            break;

         case 2:
            strcpy (string, config.sysop);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (2, 16, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.sysop, strbtrim (string));
            hidecur ();
            break;

         case 3:
            strcpy (string, config.location);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 16, string, "???????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.location, strbtrim (string));
            hidecur ();
            break;

         case 4:
            strcpy (string, config.phone);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 16, string, "???????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.phone, strbtrim (string));
            hidecur ();
            break;

         case 5:
            strcpy (string, config.flags);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (5, 16, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.flags, strbtrim (string));
            hidecur ();
            break;

      }

   } while (i != -1);

   wclose ();
}


static void registration_info ()
{
   int wh, i = 1011;
   char string[128];

   wh = wopen (10, 10, 14, 69, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Registration ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem (1, 1," Registration code ", 0, 1011, 0, NULL, 0, 0);
//      wmenuitem (2, 1," Beta code         ", 0, 1012, 0, NULL, 0, 0);
      wmenuend (i, M_VERT|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 21, CYAN|_BLACK, config.newkey_code);
//      sprintf (string, "%lu", config.keycode);
//      wprints (1, 21, CYAN|_BLACK, string);
//      sprintf (string, "%lu", config.betakey);
//      wprints (2, 21, CYAN|_BLACK, string);

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1011:
            strcpy (string, config.newkey_code);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (1, 21, string, "????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               memset (config.newkey_code, 0, 30);
               strcpy (config.newkey_code, strupr (strbtrim (string)));
            }
            hidecur ();

//            sprintf (string, "%lu", config.keycode);
//            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
//            winpdef (1, 21, string, "????????????????", 0, 2, NULL, 0);
//            if (winpread () != W_ESCPRESS)
//               config.keycode = atol (string);
//            hidecur ();
            break;

//         case 1012:
//            sprintf (string, "%lu", config.betakey);
//            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
//            winpdef (2, 21, string, "????????????????", 0, 2, NULL, 0);
//            if (winpread () != W_ESCPRESS)
//               config.betakey = atol (string);
//            hidecur ();
//            break;
      }

   } while (i != -1);

   wclose ();
}

static void addresses ()
{
   int wh, i, x, y;
   char alias[MAX_ALIAS][20], fake[MAX_ALIAS][10];

   wh = wopen (6, 4, 19, 75, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Addresses ", TRIGHT, YELLOW|_BLUE);
   i = 81;

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1," Main   ", 'N', 81, 0, NULL, 0, 0);
      wmenuitem ( 2,  1," Aka 1  ", 'S', 82, 0, NULL, 0, 0);
      wmenuitem ( 3,  1," Aka 2  ", 'S', 83, 0, NULL, 0, 0);
      wmenuitem ( 4,  1," Aka 3  ", 'S', 84, 0, NULL, 0, 0);
      wmenuitem ( 5,  1," Aka 4  ", 'S', 85, 0, NULL, 0, 0);
      wmenuitem ( 6,  1," Aka 5  ", 'S', 86, 0, NULL, 0, 0);
      wmenuitem ( 7,  1," Aka 6  ", 'S', 87, 0, NULL, 0, 0);
      wmenuitem ( 8,  1," Aka 7  ", 'S', 88, 0, NULL, 0, 0);
      wmenuitem ( 9,  1," Aka 8  ", 'S', 89, 0, NULL, 0, 0);
      wmenuitem (10,  1," Aka 9  ", 'S', 90, 0, NULL, 0, 0);
      wmenuitem ( 1, 35," Aka 10 ", 'N', 91, 0, NULL, 0, 0);
      wmenuitem ( 2, 35," Aka 11 ", 'S', 92, 0, NULL, 0, 0);
      wmenuitem ( 3, 35," Aka 12 ", 'S', 93, 0, NULL, 0, 0);
      wmenuitem ( 4, 35," Aka 13 ", 'S', 94, 0, NULL, 0, 0);
      wmenuitem ( 5, 35," Aka 14 ", 'S', 95, 0, NULL, 0, 0);
      wmenuitem ( 6, 35," Aka 15 ", 'S', 96, 0, NULL, 0, 0);
      wmenuitem ( 7, 35," Aka 16 ", 'S', 97, 0, NULL, 0, 0);
      wmenuitem ( 8, 35," Aka 17 ", 'S', 98, 0, NULL, 0, 0);
      wmenuitem ( 9, 35," Aka 18 ", 'S', 99, 0, NULL, 0, 0);
      wmenuitem (10, 35," Aka 19 ", 'S', 100, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      x = 9;
      y = 1;
      for (i = 0; i < MAX_ALIAS; i++) {
         if (i == 10) {
            x = 44;
            y = 1;
         }
         sprintf (alias[i], "%u:%u/%u.%u", config.alias[i].zone, config.alias[i].net, config.alias[i].node, config.alias[i].point);
         sprintf (fake[i], "%u", config.alias[i].fakenet);
         wgotoxy (y, x);
         wclreol ();
         wprints (y, x + 1, CYAN|_BLACK, alias[i]);
         wprints (y, x + 20, CYAN|_BLACK, fake[i]);
         y++;
      }

      start_update ();
      i = wmenuget ();
      if (i != W_ESCPRESS)
         input_alias (i - 81);

   } while (i >= 81 && i <= 100);

   wclose ();
}

static void input_alias (int i)
{
   int x, d, y;
   char stringa[20], fake[20], *p;

   y = (i % 10) + 1;
   if (i >= 10)
      x = 44;
   else
      x = 9;

   sprintf (stringa, "%u:%u/%u.%u", config.alias[i].zone, config.alias[i].net, config.alias[i].node, config.alias[i].point);
   winpbeg (BLUE|_GREEN, BLUE|_GREEN);
   winpdef (y, x + 1, stringa, "??????????????????", 'U', 2, NULL, 0);
   if (winpread () != W_ESCPRESS) {
      wprints (y, x + 1, CYAN|_BLACK, "                  ");
      wprints (y, x + 1, CYAN|_BLACK, stringa);
      sprintf (fake, "%u", config.alias[i].fakenet);
      winpbeg (BLUE|_GREEN, BLUE|_GREEN);
      winpdef (y, x + 20, fake, "?????", 'U', 2, NULL, 0);
      if (winpread () != W_ESCPRESS) {
         hidecur ();
         p = strbtrim (stringa);
         if (p[0]) {
            parse_netnode (p, (int *)&config.alias[i].zone, (int *)&config.alias[i].net, (int *)&config.alias[i].node, (int *)&config.alias[i].point);
            config.alias[i].fakenet = atoi (strbtrim (fake));
         }
         else
            config.alias[i].fakenet = config.alias[i].zone = config.alias[i].net = config.alias[i].node = config.alias[i].point = 0;

         d = 0;
         for (x = 0; x < MAX_ALIAS; x++)
            if (config.alias[x].net) {
               if (d != x) {
                  config.alias[d].zone = config.alias[x].zone;
                  config.alias[d].net = config.alias[x].net;
                  config.alias[d].node = config.alias[x].node;
                  config.alias[d].point = config.alias[x].point;
                  config.alias[d].fakenet = config.alias[x].fakenet;
                  config.alias[x].fakenet = config.alias[x].zone = config.alias[x].net = config.alias[x].node = config.alias[x].point = 0;
               }
               d++;
            }
      }
   }
}

static void edit_limits ()
{
   int wh, i;

   wh = wopen (6, 28, 20, 42, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
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
}

static void input_limits (num)
int num;
{
   int wh, i;
   char stringa[20];

   for (i=0;i<11;i++)
      config.class[i].priv = levels[i].p_length;

   wh = wopen (5, 35, 19, 67, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (levels[num].p_string, TRIGHT, YELLOW|_BLUE);
   i = 81;

   do {
      wmenubegc ();
      wmenuitem (1,  1," Time Limit per Call   ", 'C', 81, 0, NULL, 0, 0);
      wmenuitem (2,  1," Time Limit per Day    ", 'D', 82, 0, NULL, 0, 0);
      wmenuitem (3,  1," Minimum Logon Baud    ", 'L', 83, 0, NULL, 0, 0);
      wmenuitem (4,  1," Minimum Download Baud ", 'B', 84, 0, NULL, 0, 0);
      wmenuitem (5,  1," Download Limit (KB)   ", 'D', 85, 0, NULL, 0, 0);
      wmenuitem (6,  1," Ã Limit at 300 baud   ", 'D', 86, 0, NULL, 0, 0);
      wmenuitem (7,  1," Ã Limit at 1200 baud  ", 'D', 87, 0, NULL, 0, 0);
      wmenuitem (8,  1," Ã Limit at 2400 baud  ", 'D', 88, 0, NULL, 0, 0);
      wmenuitem (9,  1," À Limit at 9600 baud  ", 'D', 89, 0, NULL, 0, 0);
      wmenuitem (10,  1," Download/Upload Ratio ", 'R', 90, 0, NULL, 0, 0);
      wmenuitem (11,  1," À Ratio Start         ", 'S', 91, 0, NULL, 0, 0);
      wmenuend (i, M_VERT, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      sprintf (stringa, "%d", config.class[num].max_call);
      wgotoxy (1, 25);
      wclreol ();
      wprints (1, 25, CYAN|_BLACK, stringa);
      sprintf (stringa, "%d", config.class[num].max_time);
      wgotoxy (2, 25);
      wclreol ();
      wprints (2, 25, CYAN|_BLACK, stringa);
      sprintf (stringa, "%u", config.class[num].min_baud);
      wgotoxy (3, 25);
      wclreol ();
      wprints (3, 25, CYAN|_BLACK, stringa);
      sprintf (stringa, "%u", config.class[num].min_file_baud);
      wgotoxy (4, 25);
      wclreol ();
      wprints (4, 25, CYAN|_BLACK, stringa);
      sprintf (stringa, "%d", config.class[num].max_dl);
      wgotoxy (5, 25);
      wclreol ();
      wprints (5, 25, CYAN|_BLACK, stringa);
      sprintf (stringa, "%d", config.class[num].dl_300);
      wgotoxy (6, 25);
      wclreol ();
      wprints (6, 25, CYAN|_BLACK, stringa);
      sprintf (stringa, "%d", config.class[num].dl_1200);
      wgotoxy (7, 25);
      wclreol ();
      wprints (7, 25, CYAN|_BLACK, stringa);
      sprintf (stringa, "%d", config.class[num].dl_2400);
      wgotoxy (8, 25);
      wclreol ();
      wprints (8, 25, CYAN|_BLACK, stringa);
      sprintf (stringa, "%d", config.class[num].dl_9600);
      wgotoxy (9, 25);
      wclreol ();
      wprints (9, 25, CYAN|_BLACK, stringa);
      sprintf (stringa, "%u", config.class[num].ratio);
      wgotoxy (10, 25);
      wclreol ();
      wprints (10, 25, CYAN|_BLACK, stringa);
      sprintf (stringa, "%u", config.class[num].start_ratio);
      wgotoxy (11, 25);
      wclreol ();
      wprints (11, 25, CYAN|_BLACK, stringa);

      i = wmenuget ();
      if (i != W_ESCPRESS)
         switch (i) {
            case 81:
               sprintf (stringa, "%d", config.class[num].max_call);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (1, 25, stringa, "?????", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               config.class[num].max_call = atoi (stringa);
               break;
            case 82:
               sprintf (stringa, "%d", config.class[num].max_time);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (2, 25, stringa, "?????", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               config.class[num].max_time = atoi (stringa);
               break;
            case 83:
               sprintf (stringa, "%d", config.class[num].min_baud);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (3, 25, stringa, "?????", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               config.class[num].min_baud = atoi (stringa);
               break;
            case 84:
               sprintf (stringa, "%d", config.class[num].min_file_baud);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (4, 25, stringa, "?????", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               config.class[num].min_file_baud = atoi (stringa);
               break;
            case 85:
               sprintf (stringa, "%d", config.class[num].max_dl);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (5, 25, stringa, "?????", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               config.class[num].max_dl = atoi (stringa);
               break;
            case 86:
               sprintf (stringa, "%d", config.class[num].dl_300);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (6, 25, stringa, "?????", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               config.class[num].dl_300 = atoi (stringa);
               break;
            case 87:
               sprintf (stringa, "%d", config.class[num].dl_1200);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (7, 25, stringa, "?????", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               config.class[num].dl_1200 = atoi (stringa);
               break;
            case 88:
               sprintf (stringa, "%d", config.class[num].dl_2400);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (8, 25, stringa, "?????", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               config.class[num].dl_2400 = atoi (stringa);
               break;
            case 89:
               sprintf (stringa, "%d", config.class[num].dl_9600);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (9, 25, stringa, "?????", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               config.class[num].dl_9600 = atoi (stringa);
               break;
            case 90:
               sprintf (stringa, "%d", config.class[num].ratio);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (10, 25, stringa, "?????", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               config.class[num].ratio = atoi (stringa);
               break;
            case 91:
               sprintf (stringa, "%d", config.class[num].start_ratio);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (11, 25, stringa, "?????", 'N', 2, NULL, 0);
               winpread ();
               hidecur ();
               config.class[num].start_ratio = atoi (stringa);
               break;
         }

   } while (i >= 81 && i <= 91);

   wclose ();
}


static void read_config (name)
char *name;
{
   int fd, fdi, i;
   char filename[80], string[80];
   NODEINFO ni;
   EVENT sched;

   memset ((char *)&config, 0, sizeof (struct _configuration));

   fd = open (name, O_RDONLY|O_BINARY);
   if (fd == -1) {
      crc = get_config_crc ();

      strcpy (config.packers[0].id, "ZIP");
      strcpy (config.packers[0].packcmd, "PKZIP -M %1 %2");
      strcpy (config.packers[0].unpackcmd, "PKUNZIP -o -ed %1 %2");
      strcpy (config.packid[0].display, "ZPKZIP");
      config.packid[0].offset = 0L;
      strcpy (config.packid[0].ident, "504B0304");

      strcpy (config.packers[1].id, "ARJ");
      strcpy (config.packers[1].packcmd, "ARJ M -E -Y %1 %2");
      strcpy (config.packers[1].unpackcmd, "ARJ e -y %1 %2");
      strcpy (config.packid[1].display, "AARJ");
      config.packid[1].offset = 0L;
      strcpy (config.packid[1].ident, "60EA");

      strcpy (config.packers[2].id, "LHA");
      strcpy (config.packers[2].packcmd, "LHA m -m %1 %2");
      strcpy (config.packers[2].unpackcmd, "LHA x /cm %1 %2");
      strcpy (config.packid[2].display, "LLHA (-lh5-)");
      config.packid[2].offset = 2;
      strcpy (config.packid[2].ident, "2D6C68");

      strcpy (config.packers[3].id, "ZOO");
      strcpy (config.packers[3].packcmd, "ZOO -move %1 %2");
      strcpy (config.packers[3].unpackcmd, "ZOO -extract %1 %2");
      strcpy (config.packid[3].display, "OZOO");
      config.packid[3].offset = 21;
      strcpy (config.packid[3].ident, "DCA7C4FD");

      strcpy (config.packers[4].id, "ARC");
      strcpy (config.packers[4].packcmd, "PAK M %1 %2");
      strcpy (config.packers[4].unpackcmd, "PAK E /WA %1 %2");
      strcpy (config.packid[4].display, "PPAK");
      config.packid[4].offset = 0;
      strcpy (config.packid[4].ident, "1A");

      strcpy (config.packers[5].id, "SQZ");
      strcpy (config.packers[5].packcmd, "SQZ a /p3 %1 %2");
      strcpy (config.packers[5].unpackcmd, "SQZ e /o1 %1 %2");
      strcpy (config.packid[5].display, "SSQZ");
      config.packid[5].offset = 0;
      strcpy (config.packid[5].ident, "484C53515A");

      getcwd (config.sys_path, 35);
      if (config.sys_path[strlen (config.sys_path) - 1] != '\\')
         strcat (config.sys_path, "\\");
      strcpy (config.sched_name, "SCHED.DAT");
      strcpy (config.log_name, "LORA.LOG");
      strcpy (config.user_file, "USERS");
      strcpy (config.enterbbs, "Please press your Escape key to enter the BBS.");
      strcpy (config.banner, "Welcome to my FABULOUS Lora E-Mail System!");
      strcpy (config.mail_only, "Sorry, processing mail only, please call later.");
      strcpy (config.BBSid, "QWKPKT");
      config.com_port = 1;
      config.speed = 19200;
      strcpy (config.init, "ATZ|");
      strcpy (config.term_init, "ATZ|");
      strcpy (config.dial, "ATD");
      strcpy (config.answer, "ATA|");
      strcpy (config.galileo, "011-3487892");
      strcpy (config.galileo_init, "AT|");
      strcpy (config.galileo_dial, "ATD");
      strcpy (config.galileo_suffix, "|");
      config.inactivity_timeout = 5;
      config.modem_timeout = 60;
      config.login_timeout = 10;
      config.wazoo = config.janus = config.emsi = 1;
      config.secure = config.autozmodem = config.avatar = config.manual_answer = 1;
      config.prot_xmodem = config.prot_1kxmodem = config.prot_zmodem = config.prot_sealink = 1;
      config.blank_timer = 5;
      config.rookie_calls = 8;
      strcpy (config.location, "-Somewhere-");
      strcpy (config.system_name, "Lora E-Mail Test System");
      strcpy (config.sysop, "Lora Tester");
      strcpy (config.phone, "-Unpublished-");
#ifdef __OS2__
      strcpy (config.tearline, "LoraBBS-OS/2 v%1");
#else
      strcpy (config.tearline, "LoraBBS-DOS v%1");
#endif
      config.newfilescheck = 2;
      config.carrier_mask = 128;
      config.dcd_timeout = 1;
      config.line_offset = 1;
      config.speed_graphics = 300;
      config.logon_level = LIMITED;
      config.min_login_level = TWIT;
      strcpy (config.areachange_key, "[]?");
      strcpy (config.dateformat, "%D-%C-%Y");
      strcpy (config.timeformat, "%E:%M%A");
      config.class[0].priv = TWIT;
      config.class[0].max_time = 10;
      config.class[0].max_call = 10;
      config.class[0].max_dl = 0;
      config.class[0].min_baud = 300;
      config.class[0].min_file_baud = 300;
      config.class[1].priv = DISGRACE;
      config.class[1].max_time = 45;
      config.class[1].max_call = 60;
      config.class[1].max_dl = 200;
      config.class[1].min_baud = 300;
      config.class[1].min_file_baud = 300;
      config.class[2].priv = LIMITED;
      config.class[2].max_time = 50;
      config.class[2].max_call = 70;
      config.class[2].max_dl = 400;
      config.class[2].min_baud = 300;
      config.class[2].min_file_baud = 300;
      config.class[3].priv = NORMAL;
      config.class[3].max_time = 60;
      config.class[3].max_call = 90;
      config.class[3].max_dl = 800;
      config.class[3].min_baud = 300;
      config.class[3].min_file_baud = 300;
      config.class[4].priv = WORTHY;
      config.class[4].max_time = 60;
      config.class[4].max_call = 90;
      config.class[4].max_dl = 800;
      config.class[4].min_baud = 300;
      config.class[4].min_file_baud = 300;
      config.class[5].priv = PRIVIL;
      config.class[5].max_time = 60;
      config.class[5].max_call = 90;
      config.class[5].max_dl = 1000;
      config.class[5].min_baud = 300;
      config.class[5].min_file_baud = 300;
      config.class[6].priv = FAVORED;
      config.class[6].max_time = 60;
      config.class[6].max_call = 90;
      config.class[6].max_dl = 1000;
      config.class[6].min_baud = 300;
      config.class[6].min_file_baud = 300;
      config.class[7].priv = EXTRA;
      config.class[7].max_time = 60;
      config.class[7].max_call = 90;
      config.class[7].max_dl = 1000;
      config.class[7].min_baud = 300;
      config.class[7].min_file_baud = 300;
      config.class[8].priv = CLERK;
      config.class[8].max_time = 90;
      config.class[8].max_call = 120;
      config.class[8].max_dl = 1500;
      config.class[8].min_baud = 300;
      config.class[8].min_file_baud = 300;
      config.class[9].priv = ASSTSYSOP;
      config.class[9].max_time = 120;
      config.class[9].max_call = 180;
      config.class[9].max_dl = 2000;
      config.class[9].min_baud = 300;
      config.class[9].min_file_baud = 300;
      config.class[10].priv = SYSOP;
      config.class[10].max_time = 120;
      config.class[10].max_call = 240;
      config.class[10].max_dl = 5000;
      config.class[10].min_baud = 300;
      config.class[10].min_file_baud = 300;
   }
   else {
      read (fd, (char *)&config, sizeof (struct _configuration));
      crc = get_config_crc ();
      close (fd);

      if (config.version != CONFIG_VERSION) {
         strcpy (config.answer, (char *)&config.mustbezero);
         memset (&config.mustbezero, 0, 20);
         config.speed = (unsigned short)config.old_speed;
         config.old_speed = 0;
      }

      config.version = CONFIG_VERSION;
   }

   if (!config.carrier_mask)
      config.carrier_mask = 128;

/*
   if (!config.packers[0].id[0]) {
      strcpy (config.packers[0].id, "ZIP");
      strcpy (config.packers[0].packcmd, "PKZIP -M %1 %2");
      strcpy (config.packers[0].unpackcmd, "PKUNZIP -o -ed %1");
   }
   if (!config.packers[1].id[0]) {
      strcpy (config.packers[1].id, "ARJ");
      strcpy (config.packers[1].packcmd, "ARJ M -E -Y %1 %2");
      strcpy (config.packers[1].unpackcmd, "ARJ e -y %1");
   }
   if (!config.packers[2].id[0]) {
      strcpy (config.packers[2].id, "LZH");
      strcpy (config.packers[2].packcmd, "LHA m -m -o %1 %2");
      strcpy (config.packers[2].unpackcmd, "LHA x /cm %1");
   }
   if (!config.packers[3].id[0]) {
      strcpy (config.packers[3].id, "LHA");
      strcpy (config.packers[3].packcmd, "LHA m -m %1 %2");
      strcpy (config.packers[3].unpackcmd, "LHA x /cm %1");
   }
   if (!config.packers[4].id[0]) {
      strcpy (config.packers[4].id, "ZOO");
      strcpy (config.packers[4].packcmd, "ZOO -move %1 %2");
      strcpy (config.packers[4].unpackcmd, "ZOO -extract %1");
   }
   if (!config.packers[5].id[0]) {
      strcpy (config.packers[5].id, "ARC");
      strcpy (config.packers[5].packcmd, "PAK M %1 %2");
      strcpy (config.packers[5].unpackcmd, "PAK E /WA %1");
   }
   if (!config.packers[6].id[0]) {
      strcpy (config.packers[6].id, "SQZ");
      strcpy (config.packers[6].packcmd, "SQZ a /p3 %1 %2");
      strcpy (config.packers[6].unpackcmd, "SQZ e /o1 %1");
   }
*/

   fd = open (config.sched_name, O_RDONLY|O_BINARY);
   if (fd != -1) {
      read (fd, filename, 16);
      close (fd);

      if (!strcmp (filename, "LoraScheduler01")) {
         unlink ("SCHED.BAK");
         rename (config.sched_name, "SCHED.BAK");

         fd = open ("SCHED.BAK", O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
         lseek (fd, 16L, SEEK_SET);
         fdi = open (config.sched_name, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE);

         do {
            i = read (fd, filename, 70);
            write (fdi, filename, i);
         } while (i == 70);

         close (fd);
         close (fdi);
         unlink ("SCHED.BAK");
      }
   }
   else {
      fd = open (config.sched_name, O_WRONLY|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
      memset (&sched, 0, sizeof (EVENT));
      sched.days = 0x7F;
      sched.minute = 0;
      sched.length = 1440;
      sched.wait_time = 30;
      sched.with_connect = 5;
      sched.no_connect = 100;
      sched.behavior = MAT_BBS;
      sched.echomail = ECHO_EXPORT|ECHO_NORMAL|ECHO_KNOW|ECHO_PROT;
      strcpy (sched.cmd, "Default event");
      write (fd, &sched, sizeof (EVENT));
      close (fd);
   }

   sprintf (filename, "%sNODES.DAT", config.net_info);
   fd = open (filename, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
   if (fd != -1) {
      if ((filelength (fd) % (long)sizeof (NODEINFO)) != 0) {
         close (fd);

         sprintf (filename, "%sNODES.BAK", config.net_info);
         unlink (filename);
         sprintf (string, "%sNODES.DAT", config.net_info);
         rename (string, filename);

         fd = open (filename, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
         fdi = open (string, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE);

         memset ((char *)&ni, 0, sizeof (NODEINFO));
         ni.remap4d = ni.wazoo = ni.emsi = ni.janus = 1;

         for (;;) {
            if (read (fd, (char *)&ni, 78) != 78)
               break;
            read (fd, (char *)&ni.modem_type, 109);
            write (fdi, (char *)&ni, sizeof (NODEINFO));
         }

         close (fd);
         close (fdi);
      }
      else
         close (fd);
   }

   if (config.snow_check)
      setvparam (VP_CGA);
   if (config.no_direct)
      setvparam (VP_BIOS);
   else
      setvparam (VP_DMA);
   if (config.mono_attr)
      setvparam (VP_MONO);
}

static void save_config (name)
char *name;
{
   int fd;

   fd = open (name, O_WRONLY|O_BINARY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE);
   config.version = CONFIG_VERSION;
   write (fd, (char *)&config, sizeof (struct _configuration));
   close (fd);
}

char *get_flag_text (f)
long f;
{
   static char result[10];

   strcpy (result, "--------");
   if (f & 0x80)
      result[0] = 'X';
   if (f & 0x40)
      result[1] = 'X';
   if (f & 0x20)
      result[2] = 'X';
   if (f & 0x10)
      result[3] = 'X';
   if (f & 0x08)
      result[4] = 'X';
   if (f & 0x04)
      result[5] = 'X';
   if (f & 0x02)
      result[6] = 'X';
   if (f & 0x01)
      result[7] = 'X';

   return (result);
}

long window_get_flags (int y, int x, int type, long f)
{
   int wh, i = 1;
   long fb;

   wh = wopen (y, x, y + 11, x + 8, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wshadow (DGREY|_BLACK);

   if (type == 1)
      fb = (f >> 24) & 0xFF;
   else if (type == 2)
      fb = (f >> 16) & 0xFF;
   else if (type == 3)
      fb = (f >> 8) & 0xFF;
   else if (type == 4)
      fb = f & 0xFF;

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      if (type == 1) {
         wmenuitem (1, 1," 0 ", '0', 1, 0, NULL, 0, 0);
         wmenuitem (2, 1," 1 ", '1', 2, 0, NULL, 0, 0);
         wmenuitem (3, 1," 2 ", '2', 3, 0, NULL, 0, 0);
         wmenuitem (4, 1," 3 ", '3', 4, 0, NULL, 0, 0);
         wmenuitem (5, 1," 4 ", '4', 5, 0, NULL, 0, 0);
         wmenuitem (6, 1," 5 ", '5', 6, 0, NULL, 0, 0);
         wmenuitem (7, 1," 6 ", '6', 7, 0, NULL, 0, 0);
         wmenuitem (8, 1," 7 ", '7', 8, 0, NULL, 0, 0);
      }
      else if (type == 2) {
         wmenuitem (1, 1," 8 ", '8', 1, 0, NULL, 0, 0);
         wmenuitem (2, 1," 9 ", '9', 2, 0, NULL, 0, 0);
         wmenuitem (3, 1," A ", 'A', 3, 0, NULL, 0, 0);
         wmenuitem (4, 1," B ", 'B', 4, 0, NULL, 0, 0);
         wmenuitem (5, 1," C ", 'C', 5, 0, NULL, 0, 0);
         wmenuitem (6, 1," D ", 'D', 6, 0, NULL, 0, 0);
         wmenuitem (7, 1," E ", 'E', 7, 0, NULL, 0, 0);
         wmenuitem (8, 1," F ", 'F', 8, 0, NULL, 0, 0);
      }
      else if (type == 3) {
         wmenuitem (1, 1," G ", 'G', 1, 0, NULL, 0, 0);
         wmenuitem (2, 1," H ", 'H', 2, 0, NULL, 0, 0);
         wmenuitem (3, 1," I ", 'I', 3, 0, NULL, 0, 0);
         wmenuitem (4, 1," J ", 'J', 4, 0, NULL, 0, 0);
         wmenuitem (5, 1," K ", 'K', 5, 0, NULL, 0, 0);
         wmenuitem (6, 1," L ", 'L', 6, 0, NULL, 0, 0);
         wmenuitem (7, 1," M ", 'M', 7, 0, NULL, 0, 0);
         wmenuitem (8, 1," N ", 'N', 8, 0, NULL, 0, 0);
      }
      else if (type == 4) {
         wmenuitem (1, 1," O ", 'O', 1, 0, NULL, 0, 0);
         wmenuitem (2, 1," P ", 'P', 2, 0, NULL, 0, 0);
         wmenuitem (3, 1," Q ", 'Q', 3, 0, NULL, 0, 0);
         wmenuitem (4, 1," R ", 'R', 4, 0, NULL, 0, 0);
         wmenuitem (5, 1," S ", 'S', 5, 0, NULL, 0, 0);
         wmenuitem (6, 1," T ", 'T', 6, 0, NULL, 0, 0);
         wmenuitem (7, 1," U ", 'U', 7, 0, NULL, 0, 0);
         wmenuitem (8, 1," V ", 'V', 8, 0, NULL, 0, 0);
      }
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 5, CYAN|_BLACK, (fb & 0x80) ? "X" : "-");
      wprints (2, 5, CYAN|_BLACK, (fb & 0x40) ? "X" : "-");
      wprints (3, 5, CYAN|_BLACK, (fb & 0x20) ? "X" : "-");
      wprints (4, 5, CYAN|_BLACK, (fb & 0x10) ? "X" : "-");
      wprints (5, 5, CYAN|_BLACK, (fb & 0x08) ? "X" : "-");
      wprints (6, 5, CYAN|_BLACK, (fb & 0x04) ? "X" : "-");
      wprints (7, 5, CYAN|_BLACK, (fb & 0x02) ? "X" : "-");
      wprints (8, 5, CYAN|_BLACK, (fb & 0x01) ? "X" : "-");

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            fb ^= 0x80;
            break;

         case 2:
            fb ^= 0x40;
            break;

         case 3:
            fb ^= 0x20;
            break;

         case 4:
            fb ^= 0x10;
            break;

         case 5:
            fb ^= 0x08;
            break;

         case 6:
            fb ^= 0x04;
            break;

         case 7:
            fb ^= 0x02;
            break;

         case 8:
            fb ^= 0x01;
            break;
      }

      hidecur ();

   } while (i != -1);

   wclose ();

   if (type == 1) {
      f &= 0x00FFFFFFL;
      f |= fb << 24;
   }
   else if (type == 2) {
      f &= 0xFF00FFFFL;
      f |= fb << 16;
   }
   else if (type == 3) {
      f &= 0xFFFF00FFL;
      f |= fb << 8;
   }
   else if (type == 4) {
      f &= 0xFFFFFF00L;
      f |= fb;
   }

   return (f);
}

char *get_flagA_text (long f)
{
   static char result[10];

   strcpy (result, "--------");
   if (f & 0x80)
      result[0] = '0';
   if (f & 0x40)
      result[1] = '1';
   if (f & 0x20)
      result[2] = '2';
   if (f & 0x10)
      result[3] = '3';
   if (f & 0x08)
      result[4] = '4';
   if (f & 0x04)
      result[5] = '5';
   if (f & 0x02)
      result[6] = '6';
   if (f & 0x01)
      result[7] = '7';

   return (result);
}

char *get_flagB_text (long f)
{
   static char result[10];

   strcpy (result, "--------");
   if (f & 0x80)
      result[0] = '8';
   if (f & 0x40)
      result[1] = '9';
   if (f & 0x20)
      result[2] = 'A';
   if (f & 0x10)
      result[3] = 'B';
   if (f & 0x08)
      result[4] = 'C';
   if (f & 0x04)
      result[5] = 'D';
   if (f & 0x02)
      result[6] = 'E';
   if (f & 0x01)
      result[7] = 'F';

   return (result);
}

char *get_flagC_text (long f)
{
   static char result[10];

   strcpy (result, "--------");
   if (f & 0x80)
      result[0] = 'G';
   if (f & 0x40)
      result[1] = 'H';
   if (f & 0x20)
      result[2] = 'I';
   if (f & 0x10)
      result[3] = 'J';
   if (f & 0x08)
      result[4] = 'K';
   if (f & 0x04)
      result[5] = 'L';
   if (f & 0x02)
      result[6] = 'M';
   if (f & 0x01)
      result[7] = 'N';

   return (result);
}

char *get_flagD_text (long f)
{
   static char result[10];

   strcpy (result, "--------");
   if (f & 0x80)
      result[0] = 'O';
   if (f & 0x40)
      result[1] = 'P';
   if (f & 0x20)
      result[2] = 'Q';
   if (f & 0x10)
      result[3] = 'R';
   if (f & 0x08)
      result[4] = 'S';
   if (f & 0x04)
      result[5] = 'T';
   if (f & 0x02)
      result[6] = 'U';
   if (f & 0x01)
      result[7] = 'V';

   return (result);
}

int select_level (start, x, y)
int start, x, y;
{
   int wh1, m;

   wh1 = wopen (y, x, y + 15, x + 16, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh1);
   wshadow (DGREY|_BLACK);
   wtitle (" Levels ", TRIGHT, YELLOW|_BLUE);

   if (!start)
      start = TWIT;

   wmenubegc ();
   wmenuitem (1, 1, levels[0].p_string, 0, levels[0].p_length, 0, NULL, 0, 0);
   wmenuitem (2, 1, levels[1].p_string, 0, levels[1].p_length, 0, NULL, 0, 0);
   wmenuitem (3, 1, levels[2].p_string, 0, levels[2].p_length, 0, NULL, 0, 0);
   wmenuitem (4, 1, levels[3].p_string, 0, levels[3].p_length, 0, NULL, 0, 0);
   wmenuitem (5, 1, levels[4].p_string, 0, levels[4].p_length, 0, NULL, 0, 0);
   wmenuitem (6, 1, levels[5].p_string, 0, levels[5].p_length, 0, NULL, 0, 0);
   wmenuitem (7, 1, levels[6].p_string, 0, levels[6].p_length, 0, NULL, 0, 0);
   wmenuitem (8, 1, levels[7].p_string, 0, levels[7].p_length, 0, NULL, 0, 0);
   wmenuitem (9, 1, levels[8].p_string, 0, levels[8].p_length, 0, NULL, 0, 0);
   wmenuitem (10, 1, levels[9].p_string, 0, levels[9].p_length, 0, NULL, 0, 0);
   wmenuitem (11, 1, levels[10].p_string, 0, levels[10].p_length, 0, NULL, 0, 0);
   wmenuitem (12, 1, levels[11].p_string, 0, levels[11].p_length, 0, NULL, 0, 0);
   wmenuend (start, M_OMNI|M_SAVE, 13, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

   m = wmenuget ();

   wclose ();

   return (m == -1 ? start : m);
}

long recover_flags (s)
char *s;
{
   int i;
   long result;

   result = 0L;

   for (i = 0; i < 8; i++) {
      if (s[i] != '-')
         result |= 1 << (7 - i);
   }

   return (result);
}

void parse_netnode2(netnode, zone, net, node, point)
char *netnode;
int *zone, *net, *node, *point;
{
   char *p;

   p = netnode;
   while (!isdigit (*p) && *p != '.' && *p)
      p++;
   if (*p == '\0')
      return;

   /* If we have a zone (and the caller wants the zone to be passed back).. */

   if (strchr (netnode, ':')) {
      *zone = atoi(p);
      p = firstchar(p, ":", 2);
   }

   /* If we have a net number... */

   if (p && strchr (p, '/')) {
      *net = atoi (p);
      p = firstchar (p, "/", 2);
   }
   else if (!stricmp (p, "ALL")) {
      *net = 0;
      *node = 0;
      *point = 0;
      return;
   }

   /* We *always* need a node number... */

   if (p && *p != '.')
      *node = atoi(p);
   else if (p == NULL || !stricmp (p, "ALL"))
      *node = 0;

   /* And finally check for a point number... */

   if (p && strchr (p, '.')) {
      if (*p == '.')
         p=firstchar (p, ".", 1);
      else
         p=firstchar (p, ".", 2);

      if (p) {
         if (!stricmp (p, "ALL"))
            *point = -1;
         else
            *point = atoi(p);
      }
      else
         *point = 0;
   }
   else
      *point = 0;
}

void parse_netnode(netnode, zone, net, node, point)
char *netnode;
int *zone, *net, *node, *point;
{
   char *p;

   *zone = config.alias[0].zone;
   *net = config.alias[0].net;
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

void create_path (char *path)
{
   int wh, i;
   char *p, respath[256], origpath[256], string[20];
   struct ffblk blk;

   if (!strlen (path) || ! stricmp (path, "\\"))
      return;

   strcpy (origpath, path);
   if (path[0] == '\\')
      strcpy (respath, "\\");
   else
      strcpy (respath, "");

   origpath[strlen (origpath) - 1] = '\0';
   if (!findfirst (origpath, &blk, FA_DIREC))
      return;
   origpath[strlen (origpath)] = '\\';

   wh = wopen (10, 25, 14, 54, 0, BLACK|_LGREY, BLACK|_LGREY);
   wactiv (wh);
   wshadow (DGREY|_BLACK);

   wcenters (1, BLACK|_LGREY, "Create path (Y/n) ?  ");

   strcpy (string, "Y");
   winpbeg (BLACK|_LGREY, BLACK|_LGREY);
   winpdef (1, 24, string, "?", 0, 2, NULL, 0);

   i = winpread ();
   wclose ();

   if (i == W_ESCPRESS) {
      hidecur ();
      return;
   }

   if (toupper (string[0]) == 'Y') {
      p = strtok (origpath, "\\");

      if (p == NULL)
         mkdir (origpath);
      else {
         do {
            strcat (respath, p);
            if (strlen (respath) > 2 || respath[1] != ':')
               mkdir (respath);
            strcat (respath, "\\");
         } while ((p = strtok (NULL, "\\")) != NULL);

         if ((p = strtok (NULL, "")) != NULL) {
            strcat (respath, p);
            mkdir (respath);
         }
      }
   }
}

void clear_window ()
{
   gotoxy_ (24, 1);
   clreol_ ();
}

void linehelp_window ()
{
   char *str = "";
   struct _menu_t *mt;

   mt = wmenumcurr ();
   if (mt == NULL)
      return;

   switch (mt->citem->tagid) {
      case 101:
         str = "Your registration key code number";
         break;
      case 102:
         str = "Invoke temporary DOS shell";
         break;
      case 103:
         str = "Write out a standard AREAS.BBS text file";
         break;
      case 105:
         str = "Create message areas using a standard AREAS.BBS text file";
         break;
      case 106:
#ifdef __OS2__
         str = "Exit LSETUP-OS/2";
#else
         str = "Exit LSETUP-DOS";
#endif
         break;
      case 107:
         str = "Creates a basic ROUTE.CFG file. This option works only for point systems";
         break;
      case 201:
         str = "Video mode and screen blanker options";
         break;
      case 202:
         str = "Path specifications";
         break;
      case 203:
         str = "Fido-technolgy network addresses";
         break;
      case 204:
         str = "Informations about the system and the operator";
         break;
      case 205:
         str = "Informations about the clocksynch feature";
         break;
      case 301:
         str = "Miscellaneous options";
         break;
      case 302:
         str = "Log file name";
         break;
      case 303:
         str = "File request permissions and limits";
         break;
      case 306:
         str = "New echomail areas handling, create permissions, auto link nodes";
         break;
      case 307:
         str = "DOS commands for external mail processing";
         break;
      case 308:
         str = "Local offline message reader/editor";
         break;
      case 309:
         str = "Mail processing options: personal mail saving";
         break;
      case 310:
         str = "Sets a password to override mail-only events";
         break;
      case 401:
         str = "Message areas definitions";
         break;
      case 402:
         str = "File areas definitions";
         break;
      case 403:
         str = "QWK mail packer options";
         break;
      case 404:
         str = "Options for new users handling";
         break;
      case 405:
         str = "General BBS options: protocols, editor, etc.";
         break;
      case 406:
         str = "User limits: time, download, ratio";
         break;
      case 407:
         str = "Login security level, flags and age";
         break;
      case 408:
         str = "User's language definitions";
         break;
      case 409:
         str = "External protocols setup";
         break;
      case 501:
         str = "Terminal mode modem commands, directory, etc.";
         break;
      case 502:
         str = "IEMSI User's profile";
         break;
      case 601:
         str = "Miscellaneous hardware options";
         break;
      case 602:
         str = "Commands sent to the modem";
         break;
      case 603:
         str = "Modem answering period and commands";
         break;
      case 604:
         str = "Alternate dialing prefixes related to nodelist flags";
         break;
      case 701:
         str = "General processing scheduler definitions";
         break;
      case 702:
         str = "Nodelist and nodediff file names";
         break;
      case 703:
         str = "Phone numbers translations and call costs";
         break;
      case 704:
         str = "Packers/Unpackers command names";
         break;
      case 705:
         str = "Know nodes security definitions";
         break;
      case 706:
         str = "BBS menu system";
         break;
   }

   clear_window ();
   prints (24, 1, LGREY|_BLACK, str);
}

int sh_open (char *file, int shmode, int omode, int fmode)
{
   int i;
   long t1, t2;
   long time (long *);

   t1 = time (NULL);
   while (time (NULL) < t1 + 20) {
      if ((i = sopen (file, omode, shmode, fmode)) != -1 || errno != EACCES)
         break;
      t2 = time (NULL);
      while (time (NULL) < t2 + 1)
         ;
   }

   return (i);
}

void update_message (void)
{
   wopen (10, 25, 14, 55, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wshadow (DGREY|_BLACK);
   wcenters (1, LGREY|BLACK, "Updating database");
}

void create_route_file (void)
{
   FILE *fp;
   int i, wh1;

   for (i = 0; i < MAX_ALIAS; i++) {
      if (!config.alias[i].net)
         break;
      if (config.alias[i].point)
         break;
   }

   if (i >= MAX_ALIAS || !config.alias[i].net) {
       wh1 = wopen (10, 20, 14, 59, 0, BLACK|_LGREY, BLACK|_LGREY);
       wactiv (wh1);
       wshadow (DGREY|_BLACK);

       wcenters (1, BLACK|_LGREY, "Valid for point systems only !");
       getxch ();

       wclose ();
       return;
   }

   fp = fopen ("ROUTE.CFG", "wt");

   fprintf (fp, ";\n");
#ifdef __OS2__
   fprintf (fp, "; Route file for Lora/2 v. %s\n", LSETUP_VERSION);
#else
   fprintf (fp, "; Route file for Lora v. %s\n", LSETUP_VERSION);
#endif
   fprintf (fp, ";\n");
   fprintf (fp, "; Node: %d:%d/%d.%d\n", config.alias[0].zone, config.alias[0].net, config.alias[0].node, config.alias[0].point);
   fprintf (fp, "; Sysop: %s\n", config.sysop);
   fprintf (fp, ";\n");
   fprintf (fp, ";   Command Reference:\n");
   fprintf (fp, ";\n");
   fprintf (fp, ";       TAG <tag>\n");
   fprintf (fp, ";       SEND <verb> <nodes...>\n");
   fprintf (fp, ";       SEND-TO <verb> <nodes...>\n");
   fprintf (fp, ";       ROUTE <verb> <hub_node> <nodes...>\n");
   fprintf (fp, ";       ROUTE-TO <verb> <hub_node> <nodes...>\n");
   fprintf (fp, ";       CHANGE <from_verb> <to_verb> <nodes...>\n");
   fprintf (fp, ";       POLL <verb> <nodes...>\n");
   fprintf (fp, ";       LEAVE <nodes...>\n");
   fprintf (fp, ";       UNLEAVE <nodes...>\n");
   fprintf (fp, ";\n");
   fprintf (fp, ";   Key:\n");
   fprintf (fp, ";\n");
   fprintf (fp, ";       <tag> ....... Any text, up to 32 characters in length (no spaces).\n");
   fprintf (fp, ";\n");
   fprintf (fp, ";       <verb> ...... Any one of the following verbs:\n");
   fprintf (fp, ";\n");
   fprintf (fp, ";                     NORMAL ....... (.FLO or .OUT)\n");
   fprintf (fp, ";                     CRASH ........ (.CLO or .CUT)\n");
   fprintf (fp, ";                     HOLD ......... (.HLO or .HUT)\n");
   fprintf (fp, ";                     DIRECT ....... (.DLO or .DUT)\n");
   fprintf (fp, ";\n");
   fprintf (fp, ";       <nodes> ..... Full 'zone:net/node.point' number.  If 'zone' or 'net'\n");
   fprintf (fp, ";                     are ommited, they will default to the previous entry\n");
   fprintf (fp, ";                     on the line.\n");
   fprintf (fp, ";\n");
   fprintf (fp, ";       <hub_node> .. Hub node routed mail is sent to.\n");
   fprintf (fp, ";\n");
   fprintf (fp, ";\n");

   for (i = MAX_ALIAS - 1; i > 0; i--)
      if (config.alias[i].net && config.alias[i].point)
         fprintf (fp, "Send-To Direct %d:%d/%d\n", config.alias[i].zone, config.alias[i].net, config.alias[i].node);

   fprintf (fp, "Route-To Direct %d:%d/%d 1:ALL 2:ALL 3:ALL 4:ALL 5:ALL 6:ALL\n", config.alias[i].zone, config.alias[i].net, config.alias[i].node);

   fprintf (fp, ";\n");
   fprintf (fp, "; Note: this is only an example of how your route file could be.\n");
   fprintf (fp, ";\n");
#ifdef __OS2__
   fprintf (fp, "; Created by LSETUP-OS/2 v.%s.\n", LSETUP_VERSION);
#else
   fprintf (fp, "; Created by LSETUP-DOS v.%s.\n", LSETUP_VERSION);
#endif

   fclose (fp);
}

void mailonly_password (void)
{
   int i;
   char string[30], verify[30];

   wopen (9, 24, 13, 58, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wtitle (" Mail-only password ", TRIGHT, YELLOW|_BLUE);
   wshadow (DGREY|_BLACK);

   wprints (1, 2, LGREY|_BLACK, "Password:");
   winpbeg (BLUE|_GREEN, BLUE|_GREEN);
   winpdef (1, 12, string, "???????????????????", 'P', 0, NULL, 0);
   i = winpread ();
   hidecur ();

   if (i == W_ESCPRESS) {
      wclose ();
      return;
   }

   wtitle (" Mail-only password (Verify) ", TRIGHT, YELLOW|_BLUE);

   wprints (1, 2, LGREY|_BLACK, "Password:");
   winpbeg (BLUE|_GREEN, BLUE|_GREEN);
   winpdef (1, 12, verify, "???????????????????", 'P', 0, NULL, 0);
   i = winpread ();

   hidecur ();
   wclose ();

   if (i == W_ESCPRESS)
      return;

   if (strcmp (string, verify)) {
       wopen (10, 20, 14, 59, 0, BLACK|_LGREY, BLACK|_LGREY);
       wshadow (DGREY|_BLACK);

       wcenters (1, BLACK|_LGREY, "Passwords doesn't match !");
       getxch ();

       wclose ();
       return;
   }

   memset (config.override_pwd, 0, 20);
   strcpy (config.override_pwd, strbtrim (string));
}

static void internet_info (void)
{
   int wh, m, i = 1;
   char string[128];

   wh = wopen (7, 15, 13, 63, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Internet gateway ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem (1, 1," Gateway Type ", 0, 1, 0, NULL, 0, 0);
      wmenuitem (2, 1," Gateway Name ", 0, 2, 0, NULL, 0, 0);
      wmenuitem (3, 1," Address      ", 0, 3, 0, NULL, 0, 0);
      wmenuend (i, M_VERT|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      if (config.internet_gate == 0)
         wprints (1, 16, CYAN|_BLACK, "Uucp");
      else if (config.internet_gate == 1)
         wprints (1, 16, CYAN|_BLACK, "GIGO");
      wprints (2, 16, CYAN|_BLACK, config.uucp_gatename);
      sprintf (string, "%u:%u/%u.%u", config.uucp_zone, config.uucp_net, config.uucp_node, config.uucp_point);
      wprints (3, 16, CYAN|_BLACK, string);

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            if (config.internet_gate == 0)
               config.internet_gate = 1;
            else if (config.internet_gate == 1)
               config.internet_gate = 0;
            break;

         case 2:
            strcpy (string, config.uucp_gatename);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (2, 16, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (config.uucp_gatename, strbtrim (string));
            break;

         case 3:
            sprintf (string, "%u:%u/%u.%u", config.uucp_zone, config.uucp_net, config.uucp_node, config.uucp_point);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 16, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               parse_netnode (string, (int *)&config.uucp_zone, (int *)&config.uucp_net, (int *)&config.uucp_node, (int *)&config.uucp_point);
            break;
      }

      hidecur ();
   } while (i != -1);

   wclose ();
}

static void import_tic ()
{
   FILE *fp;
   int fd, wh, found, lastarea, i, zo, ne, no, po, total = 0, cc, nlink, m, cf;
   char string[260], *location, *tag, *p, addr[30], linea[80], flags[5];
   struct _sys sys;
   LINK *link;

   wh = wopen (11, 7, 13, 73, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wtitle ("IMPORT TIC.CFG", TLEFT, LCYAN|_BLACK);

   wprints (0, 1, YELLOW|_BLACK, "Filename");
   sprintf (string, "%sTIC.CFG", config.sys_path);
   winpbeg (BLACK|_LGREY, BLACK|_LGREY);
   winpdef (0, 10, string, "?????????????????????????????????????????????????????", 0, 2, NULL, 0);
   i = winpread ();

   hidecur ();
   wclose ();

   if (i == W_ESCPRESS)
      return;

   strbtrim (string);

   fp = fopen (string, "rt");
   if (fp == NULL)
      return;

   link = (LINK *)malloc (sizeof (LINK) * MAX_LINKS);
   if (link == NULL) {
      fclose (fp);
      return;
   }

   sprintf (string, "%sSYSFILE.DAT", config.sys_path);
   fd = open (string, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);

   fgets (string, 255, fp);

   while (fgets(string, 255, fp) != NULL) {
      if (string[0] == ';')
         continue;
      while (string[strlen (string) -1] == 0x0D || string[strlen (string) -1] == 0x0A || string[strlen (string) -1] == ' ')
         string[strlen (string) -1] = '\0';

      if ((p = strtok (string, " ")) == NULL)
         continue;
      if (stricmp (p, "AREA"))
         continue;

      if ((location = strtok (NULL, " ")) == NULL)
         continue;
      location = strbtrim (location);
      if (location[strlen (location) - 1] != '\\')
         strcat (location, "\\");
      if (strlen (location) > 39)
         location[39] = '\0';

      if ((tag = strtok (NULL, " ")) == NULL)
         continue;
      tag = strbtrim (tag);
      if (strlen (tag) > 31)
         tag[31] = '\0';

      nlink = 0;
      zo = config.alias[0].zone;
      ne = config.alias[0].net;
      no = config.alias[0].node;
      po = config.alias[0].point;

      for (;;) {
         if (fgets (linea, 70, fp) == NULL)
            break;
         while (linea[strlen (linea) -1] == 0x0D || linea[strlen (linea) -1] == 0x0A || linea[strlen (linea) -1] == ' ')
            linea[strlen (linea) -1] = '\0';
         if (linea[0] == ';' || !linea[0])
            break;

         if ((p = strtok (linea, " ")) != NULL) {
            parse_netnode2 (p, &zo, &ne, &no, &po);
            link[nlink].zone = zo;
            link[nlink].net = ne;
            link[nlink].node = no;
            link[nlink].point = po;

            link[nlink].send = link[nlink].receive = link[nlink].passive = 0;

            if (strtok (NULL, " ") != NULL) {
               if ((p = strtok (NULL, " ")) != NULL) {
                  if (strchr (p, '*') == NULL) {
                     if (strchr (p, '&') != NULL)
                        link[nlink].passive = 1;
                     else
                        link[nlink].receive = 1;
                  }
                  else {
                     if (strchr (p, '&') != NULL)
                        link[nlink].send = 1;
                  }
               }
            }

            nlink++;
            if (nlink >= MAX_LINKS)
               break;
         }
      }

      qsort (link, nlink, sizeof (LINK), sort_func);

      lseek (fd, 0L, SEEK_SET);
      cc = lastarea = found = 0;

      while (read (fd, (char *)&sys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA) {
         if (sys.file_num > lastarea)
            lastarea = sys.file_num;

         cc++;

         if (!stricmp (sys.tic_tag, tag)) {
            cf = 1;
            linea[0] = '\0';
            zo = po = ne = no = 0;
            flags[1] = '\0';

            for (m = 0; m < nlink; m++) {
               if (link[m].passive)
                  flags[0] = '!';
               else if (link[m].send)
                  flags[0] = '<';
               else if (link[m].receive)
                  flags[0] = '>';
               else
                  flags[0] = '\0';

               if (zo != link[m].zone) {
                  if (link[m].point)
                     sprintf (addr, "%s%d:%d/%d.%d ", flags, link[m].zone, link[m].net, link[m].node, link[m].point);
                  else
                     sprintf (addr, "%s%d:%d/%d ", flags, link[m].zone, link[m].net, link[m].node);
                  zo = link[m].zone;
                  ne = link[m].net;
                  no = link[m].node;
                  po = link[m].point;
               }
               else if (ne != link[m].net) {
                  if (link[m].point)
                     sprintf (addr, "%s%d/%d.%d ", flags, link[m].net, link[m].node, link[m].point);
                  else
                     sprintf (addr, "%s%d/%d ", flags, link[m].net, link[m].node);
                  ne = link[m].net;
                  no = link[m].node;
                  po = link[m].point;
               }
               else if (no != link[m].node) {
                  if (link[m].point)
                     sprintf (addr, "%s%d.%d ", flags, link[m].node, link[m].point);
                  else
                     sprintf (addr, "%s%d ", flags, link[m].node);
                  no = link[m].node;
                  po = link[m].point;
               }
               else if (link[m].point && po != link[m].point) {
                  sprintf (addr, "%s.%d ", flags, link[m].point);
                  po = link[m].point;
               }
               else
                  strcpy (addr, "");

               if (strlen (linea) + strlen (addr) >= 58) {
                  if (cf == 1) {
                     strcpy (sys.tic_forward1, linea);
                     cf++;
                  }
                  else if (cf == 2) {
                     strcpy (sys.tic_forward2, linea);
                     cf++;
                  }
                  else if (cf == 3) {
                     strcpy (sys.tic_forward3, linea);
                     cf++;
                  }

                  linea[0] = '\0';

                  if (link[m].point)
                     sprintf (addr, "%s%d:%d/%d.%d ", flags, link[m].zone, link[m].net, link[m].node, link[m].point);
                  else
                     sprintf (addr, "%s%d:%d/%d ", flags, link[m].zone, link[m].net, link[m].node);
                  zo = link[m].zone;
                  ne = link[m].net;
                  no = link[m].node;
                  po = link[m].point;
               }

               strcat (linea, addr);
            }

            if (strlen (linea) > 2) {
               if (cf == 1) {
                  strcpy (sys.tic_forward1, linea);
                  cf++;
               }
               else if (cf == 2) {
                  strcpy (sys.tic_forward2, linea);
                  cf++;
               }
               else if (cf == 3) {
                  strcpy (sys.tic_forward3, linea);
                  cf++;
               }
            }

            strcpy (sys.filepath, location);

            lseek (fd, -1L * SIZEOF_FILEAREA, SEEK_CUR);
            write (fd, (char *)&sys.file_name, SIZEOF_FILEAREA);

            found = 1;
            break;
         }
      }

      if (!found) {
         memset ((char *)&sys.file_name, 0, SIZEOF_FILEAREA);
         sys.file_num = lastarea + 1;
         strcpy (sys.tic_tag, tag);
         strcpy (sys.file_name, tag);

         cf = 1;
         linea[0] = '\0';
         zo = po = ne = no = 0;
         flags[1] = '\0';

         for (m = 0; m < nlink; m++) {
            if (link[m].passive)
               flags[0] = '!';
            else if (link[m].send)
               flags[0] = '<';
            else if (link[m].receive)
               flags[0] = '>';
            else
               flags[0] = '\0';

            if (zo != link[m].zone) {
               if (link[m].point)
                  sprintf (addr, "%s%d:%d/%d.%d ", flags, link[m].zone, link[m].net, link[m].node, link[m].point);
               else
                  sprintf (addr, "%s%d:%d/%d ", flags, link[m].zone, link[m].net, link[m].node);
               zo = link[m].zone;
               ne = link[m].net;
               no = link[m].node;
               po = link[m].point;
            }
            else if (ne != link[m].net) {
               if (link[m].point)
                  sprintf (addr, "%s%d/%d.%d ", flags, link[m].net, link[m].node, link[m].point);
               else
                  sprintf (addr, "%s%d/%d ", flags, link[m].net, link[m].node);
               ne = link[m].net;
               no = link[m].node;
               po = link[m].point;
            }
            else if (no != link[m].node) {
               if (link[m].point)
                  sprintf (addr, "%s%d.%d ", flags, link[m].node, link[m].point);
               else
                  sprintf (addr, "%s%d ", flags, link[m].node);
               no = link[m].node;
               po = link[m].point;
            }
            else if (link[m].point && po != link[m].point) {
               sprintf (addr, "%s.%d ", flags, link[m].point);
               po = link[m].point;
            }
            else
               strcpy (addr, "");

            if (strlen (linea) + strlen (addr) >= 58) {
               if (cf == 1) {
                  strcpy (sys.tic_forward1, linea);
                  cf++;
               }
               else if (cf == 2) {
                  strcpy (sys.tic_forward2, linea);
                  cf++;
               }
               else if (cf == 3) {
                  strcpy (sys.tic_forward3, linea);
                  cf++;
               }

               linea[0] = '\0';

               if (link[m].point)
                  sprintf (addr, "%s%d:%d/%d.%d ", flags, link[m].zone, link[m].net, link[m].node, link[m].point);
               else
                  sprintf (addr, "%s%d:%d/%d ", flags, link[m].zone, link[m].net, link[m].node);
               zo = link[m].zone;
               ne = link[m].net;
               no = link[m].node;
               po = link[m].point;
            }

            strcat (linea, addr);
         }

         if (strlen (linea) > 2) {
            if (cf == 1) {
               strcpy (sys.tic_forward1, linea);
               cf++;
            }
            else if (cf == 2) {
               strcpy (sys.tic_forward2, linea);
               cf++;
            }
            else if (cf == 3) {
               strcpy (sys.tic_forward3, linea);
               cf++;
            }
         }

         sys.file_priv = sys.download_priv = sys.upload_priv = SYSOP;
         strcpy (sys.filepath, location);

         write (fd, (char *)&sys.file_name, SIZEOF_FILEAREA);
      }

      total++;
   }

   close (fd);
   fclose (fp);
}

static void export_costfile (void)
{
   FILE *fp;
   int fd, i;
   char string[80];
   ACCOUNT ai;

   sprintf (string, "%sCOST.DAT", config.net_info);
   if ((fd = sh_open (string, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) == -1)
      return;

   sprintf (string, "%sCOST.CFG", config.net_info);
   if ((fp = fopen (string, "wt")) == NULL) {
      close (fd);
      return;
   }

   while (read (fd, (char *)&ai, sizeof (ACCOUNT)) == sizeof (ACCOUNT)) {
      if (ai.search[0] == '\0')
         strcpy (ai.search, "/");
      if (ai.traslate[0] == '\0')
         strcpy (ai.traslate, "/");
      fprintf (fp, "\nPrefix %s %s \"%s\"\n", ai.search, ai.traslate, ai.location);

      for (i = 0; i < MAXCOST; i++) {
         if (ai.cost[i].days == 0)
            continue;
         strcpy (string, "-------");
         if (ai.cost[i].days & DAY_SUNDAY)
            string[0] = 'S';
         if (ai.cost[i].days & DAY_MONDAY)
            string[1] = 'M';
         if (ai.cost[i].days & DAY_TUESDAY)
            string[2] = 'T';
         if (ai.cost[i].days & DAY_WEDNESDAY)
            string[3] = 'W';
         if (ai.cost[i].days & DAY_THURSDAY)
            string[4] = 'T';
         if (ai.cost[i].days & DAY_FRIDAY)
            string[5] = 'F';
         if (ai.cost[i].days & DAY_SATURDAY)
            string[6] = 'S';
         fprintf (fp, "    %s %2d:%02d-%2d:%02d %4d %3d.%d %4d %3d.%d\n",
                  string,
                  ai.cost[i].start / 60, ai.cost[i].start % 60,
                  ai.cost[i].stop / 60, ai.cost[i].stop % 60,
                  ai.cost[i].cost_first, ai.cost[i].time_first / 10, ai.cost[i].time_first % 10,
                  ai.cost[i].cost, ai.cost[i].time / 10, ai.cost[i].time % 10);
      }
   }

   fclose (fp);
   close (fd);
}

static void import_costfile (void)
{
   FILE *fp;
   int fd, i, t1, t2;
   char string[80], *p;
   ACCOUNT ai;

   sprintf (string, "%sCOST.DAT", config.net_info);
   if ((fd = sh_open (string, SH_DENYNONE, O_WRONLY|O_BINARY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE)) == -1)
      return;

   sprintf (string, "%sCOST.CFG", config.net_info);
   if ((fp = fopen (string, "rt")) == NULL) {
      close (fd);
      return;
   }

   while (fgets (string, 70, fp) != NULL) {
      while (strlen (string) > 0 && (string[strlen (string) - 1] == 0x0D || string[strlen (string) - 1] == 0x0A))
         string[strlen (string) - 1] = '\0';
      if ((p = strtok (string, " ")) == NULL)
         continue;
      if (stricmp (p, "Prefix"))
         continue;

      memset (&ai, 0, sizeof (ACCOUNT));

      if ((p = strtok (NULL, " ")) == NULL)
         continue;
      strcpy (ai.search, p);
      if ((p = strtok (NULL, " ")) == NULL)
         continue;
      if (strcmp (p, "/"))
         strcpy (ai.traslate, p);
      if ((p = strtok (NULL, "\"")) == NULL)
         continue;
      strcpy (ai.location, p);

      for (i = 0; i < MAXCOST; i++) {
         if (fgets (string, 70, fp) == NULL)
            break;
         while (strlen (string) > 0 && (string[strlen (string) - 1] == 0x0D || string[strlen (string) - 1] == 0x0A))
            string[strlen (string) - 1] = '\0';
         if ((p = strtok (string, " ")) == NULL)
            break;
         if (toupper (p[0]) == 'S')
            ai.cost[i].days |= DAY_SUNDAY;
         if (toupper (p[1]) == 'M')
            ai.cost[i].days |= DAY_MONDAY;
         if (toupper (p[2]) == 'T')
            ai.cost[i].days |= DAY_TUESDAY;
         if (toupper (p[3]) == 'W')
            ai.cost[i].days |= DAY_WEDNESDAY;
         if (toupper (p[4]) == 'T')
            ai.cost[i].days |= DAY_THURSDAY;
         if (toupper (p[5]) == 'F')
            ai.cost[i].days |= DAY_FRIDAY;
         if (toupper (p[6]) == 'S')
            ai.cost[i].days |= DAY_SATURDAY;

         if ((p = strtok (NULL, " -")) == NULL)
            break;
         sscanf (p, "%d:%d", &t1, &t2);
         ai.cost[i].start = t1 * 60 + t2;

         if ((p = strtok (NULL, " ")) == NULL)
            break;
         sscanf (p, "%d:%d", &t1, &t2);
         ai.cost[i].stop = t1 * 60 + t2;

         if ((p = strtok (NULL, " ")) == NULL)
            break;
         ai.cost[i].cost_first = atoi (p);
         if ((p = strtok (NULL, " ")) == NULL)
            break;
         if (sscanf (p, "%d.%d", &t1, &t2) == 1)
            ai.cost[i].time_first = t1 * 10;
         else
            ai.cost[i].time_first = t1 * 10 + t2;

         if ((p = strtok (NULL, " ")) == NULL)
            break;
         ai.cost[i].cost = atoi (p);
         if ((p = strtok (NULL, " ")) == NULL)
            break;
         if (sscanf (p, "%d.%d", &t1, &t2) == 1)
            ai.cost[i].time = t1 * 10;
         else
            ai.cost[i].time = t1 * 10 + t2;
      }

      if (i > 0)
         write (fd, &ai, sizeof (ACCOUNT));
   }

   fclose (fp);
   close (fd);
}

void write_ticcfg (void)
{
   FILE *fp;
   int wh, i = 1, fd, zone, net, node, point, fdn;
   char string[128], *p;
   struct _sys sys;
   NODEINFO ni;

   wh = wopen (7, 4, 11, 73, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Write TIC.CFG ", TRIGHT, YELLOW|_BLUE);

   wprints (1, 1, CYAN|_BLACK, " Filename:");
   sprintf (string, "%sTIC.CFG", config.sys_path);
   winpbeg (BLACK|_LGREY, BLACK|_LGREY);
   winpdef (1, 12, string, "?????????????????????????????????????????????????????", 0, 2, NULL, 0);
   i = winpread ();

   hidecur ();

   if (i == W_ESCPRESS) {
      wclose ();
      return;
   }

   if ((fp = fopen (string, "wt")) == NULL) {
      wclose ();
      return;
   }

   fprintf (fp, "Hold %s\n;\n", config.outbound);

   sprintf (string, "%sSYSFILE.DAT", config.sys_path);
   while ((fd = sopen (string, SH_DENYWR, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) == -1)
      ;

   sprintf (string, "%sNODES.DAT", config.net_info);
   if ((fdn = sh_open (string, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) == -1) {
      close (fd);
      fclose (fp);
      return;
   }

   while (read (fd, (char *)&sys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA) {
      if (!sys.tic_tag[0])
         continue;

      fprintf (fp, "AREA %s %s\n", sys.filepath, sys.tic_tag);

      zone = config.alias[0].zone;
      net = config.alias[0].net;
      node = config.alias[0].node;
      point = config.alias[0].point;

      if ((p = strtok (sys.tic_forward1, " ")) != NULL)
         do {
            parse_netnode2 (p, &zone, &net, &node, &point);

            lseek (fdn, 0L, SEEK_SET);
            while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO)) {
               if (zone == ni.zone && net == ni.net && node == ni.node && point == ni.point)
                  break;
            }
            if (zone == ni.zone && net == ni.net && node == ni.node && point == ni.point)
               fprintf (fp, "   %d:%d/%d.%d %s\n", zone, net, node, point, ni.pw_tic);
         } while ((p = strtok (NULL, " ")) != NULL);

      if ((p = strtok (sys.tic_forward2, " ")) != NULL)
         do {
            parse_netnode2 (p, &zone, &net, &node, &point);

            lseek (fdn, 0L, SEEK_SET);
            while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO)) {
               if (zone == ni.zone && net == ni.net && node == ni.node && point == ni.point)
                  break;
            }
            if (zone == ni.zone && net == ni.net && node == ni.node && point == ni.point)
               fprintf (fp, "   %d:%d/%d.%d %s\n", zone, net, node, point, ni.pw_tic);
         } while ((p = strtok (NULL, " ")) != NULL);

      if ((p = strtok (sys.tic_forward3, " ")) != NULL)
         do {
            parse_netnode2 (p, &zone, &net, &node, &point);

            lseek (fdn, 0L, SEEK_SET);
            while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO)) {
               if (zone == ni.zone && net == ni.net && node == ni.node && point == ni.point)
                  break;
            }
            if (zone == ni.zone && net == ni.net && node == ni.node && point == ni.point)
               fprintf (fp, "   %d:%d/%d.%d %s\n", zone, net, node, point, ni.pw_tic);
         } while ((p = strtok (NULL, " ")) != NULL);

      fprintf (fp, ";\n");
   }

   fprintf (fp, "; Created by LSETUP v.%s\n;\n", LSETUP_VERSION);

   fclose (fp);
   close (fd);
   close (fdn);

   wclose ();
}

#ifndef __OS2__
void stop_update (void)
{
}

void start_update (void)
{
}
#endif

