#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <dir.h>
#include <dos.h>
#include <string.h>
#include <process.h>
#include <time.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "exec.h"
#include "quickmsg.h"

#define isLORA       0x4E
#include "version.h"

struct _aidx {
   char areatag[32];
   byte board;
};

extern int maxaidx;
extern struct _aidx *aidx;

int maxakainfo;
struct _akainfo *akainfo = NULL;

#define MAXDUPES      1000
#define MAX_FORWARD   128

#define PACK_ECHOMAIL  0x0001
#define PACK_NETMAIL   0x0002

void mail_batch (char *what);
void open_logfile (void);
int spawn_program (int swapout, char *outstring);
void pack_outbound (int);
void build_aidx (void);
void rescan_areas (void);

void export_mail (who)
int who;
{
   FILE *fp;
   int maxnodes, wh, i, fd;
   char filename[80], *p, linea[256], *tag, *location, *forw;
   struct _fwrd *forward;
   struct date datep;
   struct time timep;
   struct _pkthdr2 pkthdr;
   struct _wrec_t *wr;

   local_status ("Export");
   scan_system ();
   
   if (who & ECHOMAIL_RSN) {
      mail_batch (config->pre_export);

      if (!registered)
         config->replace_tear = 3;
      status_line ("#Scanning messages");

      forward = (struct _fwrd *)malloc (MAX_FORWARD * sizeof (struct _fwrd));
      if (forward == NULL)
         return;

      build_aidx ();

      wh = wopen (12, 0, 24, 79, 0, LGREY|_BLACK, LCYAN|_BLACK);
      wactiv (wh);
      wtitle ("PROCESS ECHOMAIL", TLEFT, LCYAN|_BLACK);
      wprints (0, 0, YELLOW|_BLACK, " Num.  Area tag               Base         Forward to");
      printc (12, 0, LGREY|_BLACK, 'Ã');
      printc (12, 52, LGREY|_BLACK, 'Á');
      printc (12, 79, LGREY|_BLACK, '´');
      wr = wfindrec (wh);
      wr->srow++;
      wr->row++;

      maxnodes = 0;
      totalmsg = 0;
      totaltime = timerset (0);

      if (config->use_areasbbs) {
         time_release ();

         fp = fopen (config->areas_bbs, "rt");
         if (fp != NULL) {
            fgets(linea, 255, fp);

            while (fgets(linea, 255, fp) != NULL) {
               if (linea[0] == ';')
                  continue;
               while (linea[strlen (linea) -1] == 0x0D || linea[strlen (linea) -1] == 0x0A || linea[strlen (linea) -1] == ' ')
                  linea[strlen (linea) -1] = '\0';
               if ((location = strtok (linea, " ")) == NULL)
                  continue;
               if ((tag = strtok (NULL, " ")) == NULL)
                  continue;
               if ((forw = strtok (NULL, "")) == NULL)
                  continue;
               while (*forw == ' ')
                  forw++;
               sys.echomail = 1;
               strcpy (sys.echotag, tag);
               strcpy (sys.forward1, forw);
               if (*location == '$') {
                  strcpy (sys.msg_path, ++location);
                  sys.squish = 1;
                  if (sq_ptr != NULL) {
                     MsgUnlock (sq_ptr);
                     MsgCloseArea (sq_ptr);
                  }
                  if ((sq_ptr = MsgOpenArea (sys.msg_path, MSGAREA_CRIFNEC, MSGTYPE_SQUISH)) == NULL) {
                     status_line ("!MsgApi Error: %s", sys.msg_path);
                     continue;
                  }
                  while (MsgLock (sq_ptr) == -1) {
                     time_release ();
                     release_timeslice ();
                  }
                  num_msg = (int)MsgGetNumMsg (sq_ptr);
                  if (num_msg)
                     first_msg = 1;
                  else
                     first_msg = 0;
                  last_msg = num_msg;
                  maxnodes = squish_export_mail (maxnodes, forward);
                  if (maxnodes == -1) {
                     status_line ("!Error exporting mail");
                     return;
                  }
               }
               else if (*location == '!') {
                  sys.pip_board = atoi(++location);
                  maxnodes = pip_export_mail (maxnodes, forward);
                  if (maxnodes == -1) {
                     status_line ("!Error exporting mail");
                     return;
                  }
               }
               else if (!atoi (location) && stricmp (location, "##")) {
                  strcpy (sys.msg_path, location);
                  scan_message_base (0, 0);
                  maxnodes = fido_export_mail (maxnodes, forward);
                  if (maxnodes == -1) {
                     status_line ("!Error exporting mail");
                     return;
                  }
               }
            }

            fclose (fp);
         }
      }

      time_release ();
      maxnodes = quick_export_mail (maxnodes, forward);
      if (maxnodes == -1) {
         status_line ("!Error exporting mail");
         return;
      }

      time_release ();
      sprintf (filename, SYSMSG_PATH, config->sys_path);
      fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
      if (fd != -1) {
         while (read(fd, (char *)&sys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
            if (sys.squish) {
               if (sq_ptr != NULL) {
                  MsgUnlock (sq_ptr);
                  MsgCloseArea (sq_ptr);
               }
               if ((sq_ptr = MsgOpenArea (sys.msg_path, MSGAREA_CRIFNEC, MSGTYPE_SQUISH)) == NULL) {
                  status_line ("!MsgApi Error: Area %d, %s", sys.msg_num, sys.msg_path);
                  continue;
               }
               while (MsgLock (sq_ptr) == -1)
                  ;
               MsgLock (sq_ptr);
               num_msg = (int)MsgGetNumMsg (sq_ptr);
               if (num_msg)
                  first_msg = 1;
               else
                  first_msg = 0;
               last_msg = num_msg;
               maxnodes = squish_export_mail (maxnodes, forward);
               if (maxnodes == -1) {
                  status_line ("!Error exporting mail");
                  close (fd);
                  return;
               }
            }
         }

         if (sq_ptr != NULL) {
            MsgCloseArea (sq_ptr);
            sq_ptr = NULL;
         }
         close (fd);
      }

      time_release ();
      sprintf (filename, SYSMSG_PATH, config->sys_path);
      fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
      if (fd != -1) {
         while (read(fd, (char *)&sys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
            if (sys.pip_board) {
               maxnodes = pip_export_mail (maxnodes, forward);
               if (maxnodes == -1) {
                  status_line ("!Error exporting mail");
                  close (fd);
                  return;
               }
            }
         }

         close (fd);
      }

      time_release ();
      sprintf (filename, SYSMSG_PATH, config->sys_path);
      fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
      if (fd != -1) {
         while (read(fd, (char *)&sys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
            if (!sys.pip_board && !sys.squish && !sys.quick_board && sys.echomail) {
               scan_message_base (0, 0);
               maxnodes = fido_export_mail (maxnodes, forward);
               if (maxnodes == -1) {
                  status_line ("!Error exporting mail");
                  close (fd);
                  return;
               }
            }
         }

         close (fd);
      }

      gettime (&timep);
      getdate (&datep);

      for (i = 0; i < maxnodes; i++) {
         p = HoldAreaNameMunge (forward[i].zone);
         if (forward[i].point)
            sprintf (filename, "%s%04X%04X.PNT\\%08X.XUT", p, forward[i].net, forward[i].node, forward[i].point);
         else
            sprintf (filename, "%s%04X%04X.XUT", p, forward[i].net, forward[i].node);
         fd = open (filename, O_RDWR|O_BINARY);
         if (fd != -1) {
            if (filelength (fd) == 0L) {
               close (fd);
               unlink (filename);
            }
            else {
               read (fd, (char *)&pkthdr, sizeof (struct _pkthdr2));
               if (!pkthdr.year && !pkthdr.month && !pkthdr.day) {
                  pkthdr.hour = timep.ti_hour;
                  pkthdr.minute = timep.ti_min;
                  pkthdr.second = timep.ti_sec;
                  pkthdr.year = datep.da_year;
                  pkthdr.month = datep.da_mon - 1;
                  pkthdr.day = datep.da_day;
                  lseek (fd, 0L, SEEK_SET);
                  write (fd, (char *)&pkthdr, sizeof (struct _pkthdr2));
               }
               close (fd);
            }
         }
      }

      wclose ();
      free (forward);
      if (aidx != NULL)
         free (aidx);
      if (akainfo != NULL)
         free (akainfo);

      local_status ("Export");
      scan_system ();

      mail_batch (config->after_export);

      if (totalmsg) {
         status_line ("+%d ECHOmail message(s) forwarded", totalmsg);
         sysinfo.today.echosent += totalmsg;
         sysinfo.week.echosent += totalmsg;
         sysinfo.month.echosent += totalmsg;
         sysinfo.year.echosent += totalmsg;
      }
      else
         status_line ("+No ECHOmail messages forwarded");
   }

   if (who & NETMAIL_RSN) {
      mail_batch (config->pre_pack);

      wh = wopen (12, 0, 24, 79, 0, LGREY|_BLACK, LCYAN|_BLACK);
      wactiv (wh);
      wtitle ("NETMAIL", TLEFT, LCYAN|_BLACK);
      wprints (0, 0, YELLOW|_BLACK, " Num.  Area tag               Base         Forward to");
      printc (12, 0, LGREY|_BLACK, 'Ã');
      printc (12, 52, LGREY|_BLACK, 'Á');
      printc (12, 79, LGREY|_BLACK, '´');
      wr = wfindrec (wh);
      wr->srow++;
      wr->row++;

      if (fido_export_netmail () == -1) {
         status_line ("!Error exporting mail");
         return;
      }

      wclose ();
      rescan_areas ();

      mail_batch (config->after_pack);
   }

   if (!(who & NO_PACK)) {
      if (!(who & ECHOMAIL_RSN))
         pack_outbound (PACK_NETMAIL);
      else
         pack_outbound (PACK_ECHOMAIL);
   }

   idle_system ();
   memset ((char *)&sys, 0, sizeof (struct _sys));
}

void build_aidx ()
{
   int fd, i;
   char filename[80];
   struct _sys tsys;
   NODEINFO ni;

   sprintf (filename, "%sNODES.DAT", config->net_info);
   if ((fd = sh_open (filename, SH_DENYWR, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) != -1) {
      akainfo = (struct _akainfo *)malloc ((int)((filelength (fd) / sizeof (NODEINFO)) * sizeof (struct _akainfo)));

      i = 0;
      while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO)) {
         akainfo[i].zone = ni.zone;
         akainfo[i].net = ni.net;
         akainfo[i].node = ni.node;
         akainfo[i].point = ni.point;
         akainfo[i].aka = ni.aka;
         i++;
      }

      close (fd);
      maxakainfo = i;
   }
   else
      maxakainfo = 0;

   sprintf (filename, SYSMSG_PATH, config->sys_path);
   fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
   if (fd != -1) {
      maxaidx = 0;

      aidx = (struct _aidx *)malloc ((int)(((filelength (fd) / SIZEOF_MSGAREA) + 10) * sizeof (struct _aidx)));
      if (aidx == NULL) {
         close (fd);
         return;
      }

      i = 0;

      while (read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
         aidx[i].board = tsys.quick_board;
         strcpy (aidx[i++].areatag, tsys.echotag);
      }

      close (fd);

      maxaidx = i;
   }
}

int open_packet (int zone, int net, int node, int point, int ai)
{
   int mi;
   char buff[80], *p;
   struct _pkthdr2 pkthdr;

   p = HoldAreaNameMungeCreate (zone);
   if (point)
      sprintf (buff, "%s%04x%04x.PNT\\%08X.XUT", p, net, node, point);
   else
      sprintf (buff, "%s%04x%04x.XUT", p, net, node);

   mi = open (buff, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
   if (mi == -1 && point) {
      sprintf (buff, "%s%04x%04x.PNT", p, net, node);
      mkdir (buff);
      sprintf (buff, "%s%04x%04x.PNT\\%08X.XUT", p, net, node, point);
      mi = open (buff, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
   }

   if (filelength (mi) > 0L)
      lseek (mi, filelength (mi) - 2, SEEK_SET);
   else {
      memset ((char *)&pkthdr, 0, sizeof (struct _pkthdr2));
      pkthdr.ver = PKTVER;
      pkthdr.product = 0x4E;
      pkthdr.serial = ((MAJVERSION << 4) | MINVERSION);
      pkthdr.capability = 1;
      pkthdr.cwvalidation = 256;
      if (config->alias[ai].point && config->alias[ai].fakenet) {
         pkthdr.orig_node = config->alias[ai].point;
         pkthdr.orig_net = config->alias[ai].fakenet;
         pkthdr.orig_point = 0;
      }
      else {
         pkthdr.orig_node = config->alias[ai].node;
         pkthdr.orig_net = config->alias[ai].net;
         pkthdr.orig_point = config->alias[ai].point;
      }
      pkthdr.orig_zone = config->alias[ai].zone;
      pkthdr.orig_zone2 = config->alias[ai].zone;

      pkthdr.dest_point = point;
      pkthdr.dest_node = node;
      pkthdr.dest_net = net;
      pkthdr.dest_zone = zone;
      pkthdr.dest_zone2 = zone;

      add_packet_pw (&pkthdr);
      write (mi, (char *)&pkthdr, sizeof (struct _pkthdr2));
   }

   return (mi);
}

