#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <dos.h>
#include <string.h>
#include <dir.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <alloc.h>
#include <sys\stat.h>

#include <cxl\cxlstr.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlvid.h>

#include "sched.h"
#include "lsetup.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

#define DOWN_FILES  0x0020
#define NO_MESSAGE  0x0040

extern int blanked;
extern word serial_no;
extern char *config_file, *VNUM;
extern long elapsed;

void mprintf (FILE *fp, char *format, ...);

static int lora_get_bbs_record (int, int, int, int);
static int update_nodelist (char *, char *);
static int add_local_info (int, int, int, int);

void check_duplicate_key (word keyno)
{
//   int fd;

   activation_key ();
   if (registered && serial_no == keyno) {
      status_line ("!Duplicate key detected");
      modem_hangup ();

/*
      memset (config, 0, sizeof (struct _configuration));

      fd = open (config_file, O_WRONLY|O_BINARY);
      write (fd, config, sizeof (struct _configuration));
      close (fd);

      fclose (logf);

      logf = fopen ("SECURITY.CHK", "at");
      status_line ("!Security breach");
      fclose (logf);

      _dos_setfileattr ("SECURITY.CHK", _A_RDONLY|_A_HIDDEN|_A_SYSTEM);
*/
   }
}

void replace_tearline (FILE *fpd, char *buff)
{
   char wrp[90], newtear[90];

   if (registered && config->tearline[0]) {
      strcpy (newtear, config->tearline);
      sprintf (wrp, "%s%s", VNUM, registered ? "+" : NOREG);
      strsrep (newtear, "%1", wrp);
   }
   else
      sprintf (newtear, "%s%s", VERSION, registered ? "+" : NOREG);

   if (!registered) {
      mprintf (fpd, msgtxt[M_TEAR_LINE], newtear, "");
      return;
   }

   if (strstr (buff, "Lora") != NULL) {
      mprintf (fpd, "%s\r\n", buff);
      return;
   }

   if (config->replace_tear == 1 || config->replace_tear == 2) {
      strcpy (wrp, newtear);

      if (config->replace_tear == 2) {
         if (strlen (buff) + strlen (wrp) <= 35)
            strcat (buff, wrp);
      }
      else
         strcat (buff, wrp);

      mprintf (fpd, "%s\r\n", buff);
   }
   else if (config->replace_tear == 3)
      mprintf (fpd, msgtxt[M_TEAR_LINE], newtear, "");
}

void put_tearline (FILE *fpd)
{
   char wrp[30], newtear[60];

   if (registered) {
      strcpy (newtear, config->tearline);
      sprintf (wrp, "%s%s", VNUM, registered ? "+" : NOREG);
      strsrep (newtear, "%1", wrp);
   }
   else
      sprintf (newtear, "%s%s", VERSION, registered ? "+" : NOREG);

   fprintf (fpd, msgtxt[M_TEAR_LINE], newtear, "");
}

void throughput (opt, bytes)
int opt;
unsigned long bytes;

{
   static long started = 0L;
   static long elapsed;

   if (!opt)
      started = time (NULL);
   else if (started)
      {
      elapsed = time (NULL);
      /* The next line tests for day wrap without the date rolling over */
      if (elapsed < started)
         elapsed += 86400L;
      elapsed -= started;
      if (elapsed == 0L)
         elapsed = 1L;
      cps = (long) (bytes / (unsigned long) elapsed);
      started = (cps * 1000L) / ((long) rate);
      status_line ((char *) msgtxt[M_CPS_MESSAGE], cps, bytes, started);
      }
}                                                /* throughput */

long zfree (drive)
char *drive;
{
#ifdef __OS2__
   struct diskfree_t df;
   unsigned char driveno;

   if (drive[0] != '\0' && drive[1] == ':')
      {
      driveno = (unsigned char) (islower (*drive) ? toupper (*drive) : *drive);
      driveno = (unsigned char) (driveno - 'A' + 1);
      }
   else driveno = 0;                             /* Default drive    */

   if (!_dos_getdiskfree (driveno, &df))
      return (df.avail_clusters * df.bytes_per_sector * df.sectors_per_cluster);
   else
      return (0);
#else
   union REGS r;

   unsigned char driveno;
   long stat;

   if (drive[0] != '\0' && drive[1] == ':')
      {
      driveno = (unsigned char) (islower (*drive) ? toupper (*drive) : *drive);
      driveno = (unsigned char) (driveno - 'A' + 1);
      }
   else driveno = 0;                             /* Default drive    */

   r.x.ax = 0x3600;                              /* get free space   */
   r.h.dl = driveno;                             /* on this drive    */
   (void) int86 (0x21, &r, &r);                         /* go do it      */

   if (r.x.ax == 0xffff)                         /* error return??   */
      return (0);

   stat = (long) r.x.bx                          /* bx = clusters avail  */
      * (long) r.x.ax                            /* ax = sectors/clust   */
      * (long) r.x.cx;                           /* cx = bytes/sector    */

   return (stat);
#endif
}

void send_can ()
{
   int i;

   CLEAR_INBOUND();
   CLEAR_OUTBOUND();

   for (i = 0; i < 10; i++)
      SENDBYTE (0x18);
   for (i = 0; i < 10; i++)
      SENDBYTE (0x08);
}

void remove_abort (fname, rname)
char *fname, *rname;
{
        FILE *abortlog, *newlog;
        char namebuf[100];
        char linebuf[100];
        char *p;
        int c;

        if (!dexists (fname))
                return;

   if ((abortlog = fopen (fname, "ra")) != NULL)
      {
      strcpy (namebuf, fname);
      strcpy (namebuf + strlen (namebuf) - 1, "TMP");
      c = 0;
      if ((newlog = fopen (namebuf, "wa")) == NULL)
         {
         fclose (abortlog);
         }
      else
         {
         while (!feof (abortlog))
            {
            linebuf[0] = '\0';
            if (!fgets (linebuf, 64, abortlog))
               break;
            p = linebuf;
            while (*p > ' ')
               ++p;
            *p = '\0';
            if (stricmp (linebuf, rname))
               {
               *p = ' ';
               fputs (linebuf, newlog);
               ++c;
               }
            }
         fclose (abortlog);
         fclose (newlog);
         unlink (fname);
         if (c)
            rename (namebuf, fname);
         else unlink (namebuf);
         }
      }
}

void unique_name (fname)
char *fname;
{
   static char suffix[] = ".001";
   register char *p;
   register int n;

   if (dexists (fname))
      {                                          /* If file already exists...      */
      p = fname;
      while (*p && *p != '.')
         p++;                                    /* ...find the extension, if
                                                  * any  */
      for (n = 0; n < 4; n++)                    /* ...fill it out if
                                                  * neccessary   */
         if (!*p)
            {
            *p = suffix[n];
            *(++p) = '\0';
            }
         else p++;

      while (dexists (fname))                    /* ...If 'file.ext' exists
                                                  * suffix++ */
         {
         p = fname + strlen (fname) - 1;
         for (n = 3; n--;)
            {
            if (!isdigit (*p))
               *p = '0';
            if (++(*p) <= '9')
               break;
            else *p-- = '0';
            }                                    /* for */
         }                                       /* while */
      }                                          /* if exist */
}                                                /* unique_name */

int check_failed (fname, theirname, info, ourname)
char *fname, *theirname, *info, *ourname;
{
   FILE *abortlog;
   char linebuf[64];
        char *p, *badname;
        int ret;

        ret = 0;
   if ((abortlog = fopen (fname, "ra")) != NULL)
      {
      while (!feof (abortlog))
         {
         linebuf[0] = '\0';
         if (!fgets ((p = linebuf), 64, abortlog))
            break;
         while (*p >= ' ')
            ++p;
         *p = '\0';
         p = strchr (linebuf, ' ');
         *p = '\0';
         if (!stricmp (linebuf, theirname))
            {
            p = strchr ((badname = ++p), ' ');
            *p = '\0';
            if (!stricmp (++p, info))
               {
               strcpy (ourname, badname);
                                        ret = 1;
               break;
               }
            }
         }
      fclose (abortlog);
      }

        return (ret);
}

void add_abort (fname, rname, cname, cpath, info)
char *fname, *rname, *cname, *cpath, *info;
{
        FILE *abortlog;
        char namebuf[100];

   strcpy (namebuf, cpath);
   strcat (namebuf, "BadWaZOO.001");
   unique_name (namebuf);
   rename (cname, namebuf);
   if ((abortlog = fopen (fname, "at")) == NULL)
      {
      unlink (namebuf);
      }
   else
      {
      fprintf (abortlog, "%s %s %s\n", rname, namebuf + strlen (cpath), info);
      fclose (abortlog);
      }
}

int get_bbs_record (int zone, int net, int node, int point)
{
   int i;

   memset (&nodelist, 0, sizeof (struct _node));

   i = lora_get_bbs_record (zone, net, node, point);
   i += add_local_info (zone, net, node, point);

   return (i);
}

struct _idx_header {
   char name[14];
   long entry;
};

struct _idx_entry {
   short zone;
   short net;
   short node;
   long  offset;
};

void nlcomp_system ()
{
   wfill (7, 53, 11, 78, ' ', LCYAN|_BLACK);

   prints (7, 54, LCYAN|_BLACK, " Nodelist:");
   prints (8, 54, LCYAN|_BLACK, "NET addr.:");
   prints (9, 54, LCYAN|_BLACK, "    Total:");
}

void diffupdate_system (void)
{
   wfill (7, 53, 11, 78, ' ', LCYAN|_BLACK);

   prints (7, 54, LCYAN|_BLACK, " Nodelist:");
   prints (8, 54, LCYAN|_BLACK, " Nodediff:");
   prints (9, 54, LCYAN|_BLACK, "     Done:");
}

int get_last_nodelist (char *name)
{
   int i, max = -1;
   char filename[128], *p;
   struct ffblk blk;

   sprintf (filename, "%s%s.*", config->net_info, name);
   if (findfirst (filename, &blk, 0))
      return (-1);

   do {
      if ((p = strchr (blk.ff_name, '.')) == NULL)
         continue;
      p++;
      i = atoi (p);
      if (i > max)
         max = i;
   } while (!findnext (&blk));

   return (max);
}

void build_nodelist_index (force)
int force;
{
   FILE *fps, *fpd;
   short nzone, nnet, nnode, i, records, cf, m;
   char filename[80], linea[512], build, *p, final[14];
   long hdrpos, entrypos, totalnodes, totaltime;
   struct stat statbuf, statidx;
   struct _idx_header header;
   struct _idx_entry entry;

   cf = 0;
   totalnodes = 0L;
   build = 0;
   records = 0;
   nzone = config->alias[0].zone;
   nnet = config->alias[0].net;

   sprintf (filename, "%sNODE.IDX", config->net_info);
   if (stat (filename, &statidx) == -1)
      build = 1;

   for (cf = 0; cf < 10; cf++) {
      sprintf (filename, "%s%s", config->net_info, config->nl[cf].list_name);
      if (!stat (filename, &statbuf)) {
         if (statbuf.st_mtime > statidx.st_mtime)
            build = 1;
      }
   }

   memset ((char *)&nodelist, 0, sizeof (struct _node));

   sprintf (filename, "%sNODE.IDX", config->net_info);
   if ((fps = fopen (filename, "rb")) != NULL) {
      while (fread ((char *)&header, sizeof (struct _idx_header), 1, fps) == 1) {
         sprintf (filename, "%s%s", config->net_info, header.name);
         if (!stat (filename, &statbuf)) {
            if (statbuf.st_mtime > statidx.st_mtime) {
               build = 1;
               break;
            }
         }
         else {
            build = 1;
            break;
         }

         fseek (fps, header.entry * sizeof (struct _idx_entry), SEEK_CUR);
      }

      fclose (fps);
   }
   else
      build = 1;

   if (build || force) {
      status_line(":Rebuild nodelist index");
      nlcomp_system ();
      local_status ("NL Comp.");

      totaltime = timerset (0);

      sprintf (filename, "%sNODE.IDX", config->net_info);
      fpd = fopen (filename, "w+b");

      for (cf = 0; cf < 10; cf++) {
         if (config->nl[cf].list_name[0] == '\0')
            continue;

         time_release ();

         if (strchr (config->nl[cf].list_name, '.') == NULL && config->nl[cf].diff_name[0]) {
            i = update_nodelist (config->nl[cf].list_name, config->nl[cf].diff_name);
            for (;;) {
               m = update_nodelist (config->nl[cf].list_name, config->nl[cf].diff_name);
               if (m == i)
                  break;
               i = m;
            }
            if (i == -1)
               continue;
            sprintf (final, "%s.%03d", config->nl[cf].list_name, i);
         }
         else if (strchr (config->nl[cf].list_name, '.') == NULL) {
            i = get_last_nodelist (config->nl[cf].list_name);
            if (i == -1)
               continue;
            sprintf (final, "%s.%03d", config->nl[cf].list_name, i);
         }
         else
            strcpy (final, config->nl[cf].list_name);
         sprintf (filename, "%s%s", config->net_info, final);
         fps = fopen (filename, "rb");
         if (fps == NULL)
            continue;

         status_line ("+Compiling %s", fancy_str (filename));
         prints (7, 65, YELLOW|_BLACK, "            ");
         prints (7, 65, YELLOW|_BLACK, strupr (final));

         hdrpos = ftell  (fpd);
         memset ((char *)&header, 0, sizeof (struct _idx_header));
         strcpy (header.name, strupr(final));
         fwrite ((char *)&header, sizeof (struct _idx_header), 1, fpd);

         entrypos = ftell (fps);
         nzone = config->alias[0].zone;
         nnet = config->alias[0].net;

         while (fgets(linea, 511, fps) != NULL) {
            if (linea[0] == ';') {
               entrypos = ftell (fps);
               continue;
            }

            if (strnicmp (linea, "Down,", 5))
               totalnodes++;

            if ( totalnodes && !(totalnodes % 32) ) {
               time_release ();

               if (timerset (0) > totaltime) {
                  sprintf (filename, "%ld (%.0f/s) ", totalnodes, (float)totalnodes / ((float)(timerset (0) - totaltime) / 100));
                  filename[14] = '\0';
                  prints (9, 65, YELLOW|_BLACK, filename);
               }

               prints (8, 65, YELLOW|_BLACK, "              ");
               sprintf (filename, "%d:%d", nzone, nnet);
               prints (8, 65, YELLOW|_BLACK, filename);
            }

            p = strtok (linea, ",");
            if (stricmp (p, "Boss") && stricmp (p, "Zone") && stricmp (p, "Region") && stricmp (p, "Host")) {
               entrypos = ftell (fps);
               continue;
            }

            nnode = 0;
            if (!stricmp (p, "Boss")) {
               p = strtok (NULL, ",");
               parse_netnode (p, (int *)&nzone, (int *)&nnet, (int *)&nnode, (int *)&i);
            }
            else if (!stricmp (p, "Zone")) {
               p = strtok (NULL, ",");
               nzone = nnet = atoi (p);
            }
            else {
               p = strtok (NULL, ",");
               nnet = atoi (p);
            }

            entry.zone = nzone;
            entry.net = nnet;
            entry.node = nnode;
            entry.offset = entrypos;
            fwrite ((char *)&entry, sizeof (struct _idx_entry), 1, fpd);
            header.entry++;
            records++;
            entrypos = ftell (fps);
         }

         fclose (fps);

         fseek (fpd, hdrpos, SEEK_SET);
         fwrite ((char *)&header, sizeof (struct _idx_header), 1, fpd);
         fseek (fpd, 0L, SEEK_END);
      }

      fclose (fpd);

      status_line ("+%ld total nodes, %d records written", totalnodes, records);

      idle_system ();
      mtask_find();
   }
}

static int lora_get_bbs_record(zone, net, node, point)
int zone, net, node, point;
{
   FILE *fp, *fpn;
   short i, checkp, zo, ne, no;
   char filename[80], linea[512], *p, first;
   long num;
   struct _idx_header header;
   struct _idx_entry entry;

   if (point)
      point = 0;

   memset ((char *)&nodelist, 0, sizeof (struct _node));

   sprintf (filename, "%sNODE.IDX", config->net_info);
   fp = sh_fopen (filename, "rb", SH_DENYWR);

   while (fread ((char *)&header, sizeof (struct _idx_header), 1, fp) == 1) {
      for (num = 0L; num < header.entry; num++) {
         if (fread ((char *)&entry, sizeof (struct _idx_entry), 1, fp) != 1)
            break;
         if ((zone && entry.zone == zone) && entry.net == net) {
            if (entry.node && entry.node != node)
               continue;
            if (strchr (header.name, '.') == NULL) {
               i = update_nodelist (header.name, NULL);
               if (i == -1)
                  return (0);
               sprintf (filename, "%s%s.%03d", config->net_info, header.name, i);
            }
            else
               sprintf (filename, "%s%s", config->net_info, header.name);

            fpn = sh_fopen (filename, "rb", SH_DENYWR);
            if (fpn == NULL)
               return (0);

            fseek (fpn, entry.offset, SEEK_SET);

            first = 0;
            checkp = 0;

            while (fgets(linea, 511, fpn) != NULL) {
               if (linea[0] == ';')
                  continue;

               if (linea[0] != ',') {
                  p = strtok (linea, ",");
                  if (!stricmp (p, "Zone") || !stricmp (p, "Region") || !stricmp (p, "Host")) {
                     if (first)
                        break;
                     else
                        first = 1;
                     p = strtok (NULL, ",");
                     *p = '\0';
                  }
                  else if (!stricmp (p, "Boss")) {
                     if (first)
                        break;
                     p = strtok (NULL, ",");
                     parse_netnode (p, (int *)&zo, (int *)&ne, (int *)&no, (int *)&i);
                     if (zo == zone && ne == net && no == node && point) {
                        first = 1;
                        checkp = 1;
                     }
                  }
                  else
                     p = strtok (NULL, ",");
               }
               else
                  p = strtok (linea, ",");

               if (p == NULL)
                  continue;
               if (checkp) {
                  if (atoi (p) != point)
                     continue;
               }
               else {
                  if (atoi (p) != node || point)
                     continue;
               }

               p = strtok (NULL, ",");
               if (strlen (p) > 33)
                  p[33] = '\0';
               strcpy (nodelist.name, p);
               strischg (nodelist.name, "_", " ");

               p = strtok (NULL, ",");
               if (strlen (p) > 29)
                  p[29] = '\0';
               strcpy (nodelist.city, p);
               strischg (nodelist.city, "_", " ");

               p = strtok (NULL, ",");
               if (strlen (p) > 35)
                  p[35] = '\0';
               strcpy (nodelist.sysop, p);
               strischg (nodelist.sysop, "_", " ");

               p = strtok (NULL, ",");
               if (strlen (p) > 39)
                  p[39] = '\0';
               strcpy (nodelist.phone, p);

               p = strtok (NULL, ",");
               nodelist.rate = atoi (p) / 300;

               if ((p = strtok (NULL, "")) != NULL)
                  for (i = 0; i < 10; i++) {
                     if (!config->prefixdial[i].flag[0])
                        continue;
                     if (stristr (p, config->prefixdial[i].flag) != NULL) {
                        nodelist.modem = i + 20;
                        break;
                     }
                  }

               fclose (fp);
               fclose (fpn);

               return (1);
            }

            fclose (fpn);
         }
      }
   }

   fclose (fp);

   return (0);
}

static int add_local_info (zone, net, node, point)
int zone, net, node, point;
{
   int fd, r, rep, ora, i, c1, c2, c3, c4, wd;
   char filename[80], deftrasl[60];
   long tempo;
   struct tm *tim;

   NODEINFO ni;
   ACCOUNT ai;

   sprintf (filename, "%sNODES.DAT", config->net_info);
   if ((fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) == -1)
      return (0);

   c1 = c2 = c3 = c4 = 0;
   r = 0;
   rep = 0;
   deftrasl[0] = '\0';

   while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO))
      if (zone == ni.zone && net == ni.net && node == ni.node && point == ni.point) {
         strcpy (nodelist.password, strupr(ni.pw_session));
         nodelist.modem = ni.modem_type;
         strcpy (nodelist.pw_areafix, ni.pw_areafix);
         strcpy (nodelist.pw_tic, ni.pw_tic);
         strcpy (nodelist.sysop, ni.sysop_name);
         if (ni.system[0])
            strcpy (nodelist.name, ni.system);
         if (ni.phone[0])
            strcpy (nodelist.phone, ni.phone);
         nodelist.akainfo = ni.aka;
         r = 1;
         break;
      }

   close (fd);

   sprintf (filename, "%sCOST.DAT", config->net_info);
   fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);

   while (read (fd, (char *)&ai, sizeof (ACCOUNT)) == sizeof (ACCOUNT)) {
      if (!strncmp (nodelist.phone, ai.search, strlen (ai.search))) {
         strisrep (nodelist.phone, ai.search, ai.traslate);

         tempo = time (NULL);
         tim = localtime (&tempo);
         ora = tim->tm_hour * 60 + tim->tm_sec;
         wd = 1 << tim->tm_wday;
         for (i = 0; i < MAXCOST; i++)
            if ((ai.cost[i].days & wd)) {
               if (ai.cost[i].start < ai.cost[i].stop) {
                  if (ora >= ai.cost[i].start && ora <= ai.cost[i].stop) {
                     nodelist.cost = ai.cost[i].cost;
                     nodelist.time = ai.cost[i].time;
                     nodelist.cost_first = ai.cost[i].cost_first;
                     nodelist.time_first = ai.cost[i].time_first;
                     break;
                  }
               }
               else {
                  if (ora >= ai.cost[i].start && ora <= 1439) {
                     nodelist.cost = ai.cost[i].cost;
                     nodelist.time = ai.cost[i].time;
                     nodelist.cost_first = ai.cost[i].cost_first;
                     nodelist.time_first = ai.cost[i].time_first;
                     break;
                  }
                  if (ora >= 0 && ora <= ai.cost[i].stop) {
                     nodelist.cost = ai.cost[i].cost;
                     nodelist.time = ai.cost[i].time;
                     nodelist.cost_first = ai.cost[i].cost_first;
                     nodelist.time_first = ai.cost[i].time_first;
                     break;
                  }
               }
            }

         rep = 1;
         break;
      }
      else if (!strcmp (ai.search, "/")) {
         strcpy (deftrasl, ai.traslate);
         c1 = ai.cost[i].cost;
         c2 = ai.cost[i].time;
         c3 = ai.cost[i].cost_first;
         c4 = ai.cost[i].time_first;
      }
   }

   if (!rep && deftrasl[0]) {
      strcpy (filename, deftrasl);
      strcat (filename, nodelist.phone);
      strcpy (nodelist.phone, filename);
      nodelist.cost = c1;
      nodelist.time = c2;
      nodelist.cost_first = c3;
      nodelist.time_first = c4;
   }

   close (fd);

   return (r);
}

void set_protocol_flags (int zone, int net, int node, int point)
{
   int fd;
   char filename[80];
   NODEINFO ni;

   sprintf (filename, "%sNODES.DAT", config->net_info);
   fd = open (filename, O_RDONLY|O_BINARY);

   while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO))
      if (zone == ni.zone && net == ni.net && node == ni.node && point == ni.point) {
         if (config->janus)
            config->janus = ni.janus;
         if (config->emsi)
            config->emsi = ni.emsi;
         if (config->wazoo)
            config->wazoo = ni.wazoo;
         break;
      }

   close (fd);
}

long get_phone_cost (zo, ne, no, online)
int zo, ne, no;
long online;
{
   long cost;

   if (!get_bbs_record (zo, ne, no, 0))
      return (0);

   online *= 10L;

   if (nodelist.time_first) {
      if (online > (long)nodelist.time_first) {
         cost = (long)nodelist.cost_first;
         online -= (long)nodelist.time_first;
      }
      else
         cost = 0L;
   }
   else
      cost = 0L;

   if (online <= 0L)
      return (cost);

   if (nodelist.time) {
      cost += (online / (long)nodelist.time) * (long)nodelist.cost;
      if ((online % (long)nodelist.time) != 0)
         cost += (long)nodelist.cost;
   }

   return (cost);
}

void add_packet_pw (pkthdr)
struct _pkthdr2 *pkthdr;
{
   int fd;
   char filename[80];
   NODEINFO ni;

   sprintf (filename, "%sNODES.DAT", config->net_info);
   fd = open (filename, O_RDONLY|O_BINARY);

   while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO))
      if (pkthdr->dest_zone == ni.zone && pkthdr->dest_net == ni.net && pkthdr->dest_node == ni.node && pkthdr->dest_point == ni.point) {
         strcpy (pkthdr->password, strupr(ni.pw_packet));
         break;
      }

   close (fd);
}


#define isLeap(x) ((x)%1000)?((((x)%100)?(((x)%4)?0:1):(((x)%400)?0:1))):(((x)%4000)?1:0)

int update_nodelist (name, diff)
char *name, *diff;
{
   FILE *fps, *fpd, *fpn;
   int i, max = -1, next, m;
   char filename[128], linea[512], *p;
   struct ffblk blk;
   long tempo;
   float t;
   struct tm *tim;

   tempo = time (NULL);
   tim = localtime(&tempo);

   sprintf (filename, "%s%s.*", config->net_info, name);
   if (findfirst (filename, &blk, 0))
      return (-1);

   do {
      if ((p = strchr (blk.ff_name, '.')) == NULL)
         continue;
      p++;
      i = atoi (p);
      if (i > max)
	 max = i;
   } while (!findnext (&blk));

   next = max + 7;
   if ((isLeap (tim->tm_year - 1)) && next > 366)
      next -= 366;
   else if (!(isLeap (tim->tm_year - 1)) && next > 365)
      next -= 365;

   if (diff == NULL || *diff == '\0')
      return (max);

   sprintf (filename, "%s%s.%03d", config->net_info, diff, next);
   if (!dexists (filename))
      return (max);

   diffupdate_system ();

   sprintf (filename, "%s.%03d", name, max);
   prints (7, 65, YELLOW|_BLACK, filename);
   sprintf (filename, "%s.%03d", diff, next);
   prints (8, 65, YELLOW|_BLACK, filename);
   prints (9, 65, YELLOW|_BLACK, "0%%");

   status_line ("+Updating %s.%03d with %s.%03d", name, max, diff, next);

   sprintf (filename, "%s%s.%03d", config->net_info, diff, next);
   fps = sh_fopen (filename, "rt", SH_DENYRW);
   setvbuf (fps, NULL, _IOFBF, 2048);

   sprintf (filename, "%s%s.%03d", config->net_info, name, max);
   fpn = sh_fopen (filename, "rt", SH_DENYNONE);
   setvbuf (fpn, NULL, _IOFBF, 2048);

   sprintf (filename, "%s%s.%03d", config->net_info, name, next);
   fpd = sh_fopen (filename, "wt", SH_DENYRW);
   setvbuf (fpd, NULL, _IOFBF, 2048);

   fgets(linea, 511, fps);
   fgets(filename, 127, fpn);
   if (stricmp (linea, filename)) {
      status_line ("!%s.%03d doesn't match %s.%03d", diff, next, name, max);
      fclose (fpd);
      fclose (fpn);
      fclose (fps);

      sprintf (filename, "%s%s.%03d", config->net_info, name, next);
      unlink (filename);
      sprintf (filename, "%s%s.%03d", config->net_info, diff, next);
      unlink (filename);

      nlcomp_system ();
      return (max);
   }

   rewind (fpn);

   while (fgets(linea, 511, fps) != NULL) {
      while (linea[strlen (linea) -1] == 0x0D || linea[strlen (linea) -1] == 0x0A || linea[strlen (linea) -1] == ' ')
	 linea[strlen (linea) -1] = '\0';

      if (linea[0] == 'A' && isdigit(linea[1])) {
         m = atoi (&linea[1]);
         for (i = 0; i < m; i++) {
            fgets(linea, 511, fps);
            fputs(linea, fpd);
         }
      }
      else if (linea[0] == 'C' && isdigit(linea[1])) {
         m = atoi (&linea[1]);
         for (i = 0; i < m; i++) {
            fgets(linea, 511, fpn);
            fputs(linea, fpd);
         }
      }
      else if (linea[0] == 'D' && isdigit(linea[1])) {
         m = atoi (&linea[1]);
         for (i = 0; i < m; i++)
            fgets(linea, 511, fpn);
      }

      t = (float)ftell (fps) * (float)100 / (float)filelength (fileno (fps));
      sprintf (linea, "%04.1f%%", t);
      prints (9, 65, YELLOW|_BLACK, linea);
   }

   fclose (fpd);
   fclose (fpn);
   fclose (fps);

   sprintf (filename, "%s%s.%03d", config->net_info, name, max);
   unlink (filename);
   sprintf (filename, "%s%s.%03d", config->net_info, diff, next);
   unlink (filename);

   nlcomp_system ();
   elapsed += time (NULL) - tempo;

   return (next);
}

char *random_origins (void)
{
   FILE *fp;
   struct time dt;

   if (sys.origin[0] == '@') {
      gettime (&dt);
      srand (dt.ti_sec * 100 + dt.ti_hund);

      fp = fopen (&sys.origin[1], "rt");
      fseek (fp, (long)random ((int)filelength(fileno (fp))), SEEK_SET);
      fgets (e_input, 120, fp);
      if (fgets (e_input, 120, fp) == NULL) {
         rewind (fp);
         fgets (e_input, 120, fp);
      }

      fclose (fp);
   }
   else
      strcpy (e_input, sys.origin);

   return (e_input);
}

int yesno_question(def)
int def;
{
   char stringa[MAX_CMDLEN], c, bigyes[4], bigno[4], rescode[50];

   sprintf(bigyes, "%c/%c", toupper(bbstxt[B_YES][0]), tolower(bbstxt[B_NO][0]));
   sprintf(bigno, "%c/%c", tolower(bbstxt[B_YES][0]), toupper(bbstxt[B_NO][0]));

   if (!(def & NO_MESSAGE)) {
      sprintf (rescode, " [%s", (def & DEF_YES) ? bigyes : bigno);
      strcat (rescode, (def & EQUAL) ? "/=" : "");
      strcat (rescode, (def & DOWN_FILES) ? "/d" : "");
      strcat (rescode, (def & TAG_FILES) ? "/t" : "");
      strcat (rescode, (def & QUESTION) ? "/?" : "");
      strcat (rescode, "]?  ");

      m_print (rescode);
   }
   else
      m_print (" ");

   cmd_string[0] = '\0';

   do {
      m_print ("\b \b");
      if (!cmd_string[0]) {
         if (usr.hotkey)
            cmd_input (stringa, 1);
         else
            chars_input (stringa, 1, INPUT_NOLF);

         c = toupper(stringa[0]);
      }
      else {
         c = toupper(cmd_string[0]);
         strcpy (&cmd_string[0], &cmd_string[1]);
         strtrim (cmd_string);
      }
      if (c == '?' && (def & QUESTION))
         break;
      if (c == '=' && (def & EQUAL))
         break;
      if (c == 'D' && (def & DOWN_FILES))
         break;
      if (c == 'T' && (def & TAG_FILES))
         break;
      if (time_remain() <= 0 || !CARRIER)
         return (DEF_NO);
   } while (c != bigyes[0] && c != bigno[2] && c != '\0');

   if (!(def & NO_LF))
      m_print(bbstxt[B_ONE_CR]);

//   strcpy (cmd_string, &stringa[1]);

   if (c == '?' && (def & QUESTION))
      return (QUESTION);

   if (c == '=' && (def & EQUAL))
      return (EQUAL);

   if (c == 'D' && (def & DOWN_FILES))
      return (DOWN_FILES);

   if (c == 'T' && (def & TAG_FILES))
      return (TAG_FILES);

   if (def & DEF_YES) {
      if (c == bigno[2])
         return (DEF_NO);
      else
         return (DEF_YES);
   }
   else {
      if (c == bigyes[0])
         return (DEF_YES);
      else
         return (DEF_NO);
   }
}

void big_pause (secs)
int secs;
{
   long timeout, timerset ();

   timeout = timerset ((unsigned int) (secs * 100));
   while (!timeup (timeout)) {
      if (PEEKBYTE() != -1)
         break;
   }
}

void press_enter()
{
   char stringa[2];

   m_print("%s", bbstxt[B_PRESS_ENTER]);
   chars_input(stringa,0,INPUT_NOLF);

   m_print("\r");
   space (strlen (bbstxt[B_PRESS_ENTER]) + 1);
   m_print("\r");
   restore_last_color ();
}

int continua()
{
   int i;

   if (nopause)
      return(1);

   m_print(bbstxt[B_MORE]);
   i = yesno_question(DEF_YES|EQUAL|NO_LF);

   m_print("\r");
   space (strlen (bbstxt[B_MORE]) + 14);
   m_print("\r");
   restore_last_color ();

   if(i == DEF_NO) {
           nopause = 0;
           return(0);
   }

   if (i == EQUAL)
      nopause = 1;

   return(1);
}

int more_question(line)
int line;
{
   if(!(line++ < (usr.len-1)) && usr.more) {
      line = 1;
      if(!continua())
         return(0);
   }

   return(line);
}


char *data(d)
char *d;
{
   long tempo;
   struct tm *tim;

   tempo=time(0);
   tim=localtime(&tempo);

   sprintf(d,"%02d %3s %02d  %2d:%02d:%02d",tim->tm_mday,mtext[tim->tm_mon],\
              tim->tm_year,tim->tm_hour,tim->tm_min,tim->tm_sec);
   return(d);
}

void timer (decs)
int decs;
{
   long timeout;

   timeout = timerset ((unsigned int) (decs * 10));

   while (!timeup (timeout)) {
      time_release();
      release_timeslice ();
   }
}

void update_filesio (sent, received)
int sent, received;
{
   char string[20];

   if (caller || emulator)
      return;

   prints (8, 65, YELLOW|_BLACK, "              ");
   sprintf (string, "%d/%d", received, sent);
   prints (8, 65, YELLOW|_BLACK, string);
   time_release ();
}

int check_new_messages (char *dir)
{
   int fd, i, last;
   char filename[80], *p;
   struct ffblk blk;

   last = 0;

   sprintf (filename, "%s*.MSG", dir);

   if (!findfirst (filename, &blk, 0))
      do {
         if ((p = strchr (blk.ff_name,'.')) != NULL)
            *p = '\0';

         i = atoi (blk.ff_name);
         if (last < i || !last)
            last = i;
      } while (!findnext (&blk));

   i = 0;

   sprintf (filename, "%sLASTREAD", dir);
   fd = sh_open (filename, SH_DENYWR, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
   if (fd != -1) {
      read (fd, (char *)&i, sizeof (short));
      close (fd);
   }
   else
      i = last;

   return (i < last);
}

void check_new_netmail (void)
{
   char filename[80];

   if (config->newmail_flash) {
      if (check_new_messages (config->netmail_dir))
         prints (23, 54, YELLOW|_BLACK|BLINK, "MAIL");
      else
         prints (23, 54, YELLOW|_BLACK, "    ");
   }
   else
      prints (23, 54, YELLOW|_BLACK, "    ");

   if (config->mymail_flash) {
      if (check_new_messages (config->my_mail))
         prints (23, 59, YELLOW|_BLACK|BLINK, "PERSONAL");
      else
         prints (23, 59, YELLOW|_BLACK, "        ");
   }
   else
      prints (23, 59, YELLOW|_BLACK, "        ");

   sprintf (filename, "%sRCVFAX.FLG", config->sys_path);
   if (dexists (filename))
      prints (23, 68, YELLOW|_BLACK|BLINK, "FAX");
   else
      prints (23, 68, YELLOW|_BLACK, "   ");
}

typedef struct {
   char name[13];
   word area;
   long datpos;
} FILEIDX;

char *index_filerequestproc (char *req_list, char *file, char *their_wildcard, int *recno, int updreq, int *jj)
{
   int j;
   char our_wildcard[15];
   FILE *approved;
   FILEIDX fidx;

   approved = fopen (req_list, "rb");
   if (approved == NULL)
      goto err;

   j = *jj;
   status_line (">DEBUG: FR: [%s]", their_wildcard);

   while (fread (&fidx, sizeof (FILEIDX), 1, approved) == 1) {
      prep_match (fidx.name, our_wildcard);
      if (!match (our_wildcard, their_wildcard)) {
         if (read_system (fidx.area, 2)) {
            if (!sys.prot_req && filepath == config->prot_filepath) {
               status_line (">DEBUG: FR:%s %d %08lX (%08lX %08lX %08lX)", fidx.name, fidx.area, filepath, config->prot_filepath, config->norm_filepath, config->know_filepath);
               continue;
            }
            if (!sys.know_req && filepath == config->know_filepath) {
               status_line (">DEBUG: FR:%s %d %08lX (%08lX %08lX %08lX)", fidx.name, fidx.area, filepath, config->prot_filepath, config->norm_filepath, config->know_filepath);
               continue;
            }
            if (!sys.norm_req && filepath == config->norm_filepath) {
               status_line (">DEBUG: FR:%s %d %08lX (%08lX %08lX %08lX)", fidx.name, fidx.area, filepath, config->prot_filepath, config->norm_filepath, config->know_filepath);
               continue;
            }
            sprintf (file, "%s%s", sys.filepath, fidx.name);
            if (dexists (file)) {
               if (++j > *recno)
                  goto gotfile;
            }
            else
               status_line (">DEBUG: FR:%s", file);
         }
      }

      file[0] = '\0';
   }

avail:
   *jj = j;
   if (approved) {
      fclose(approved);
      approved = NULL;
   }

   if (*recno > -1) {
      *recno = -1;
      return (NULL);
   }

   if (!file[0]) {
      *recno = -1;
      return (NULL);
   }

gotfile:
//   ++(*recno);
   *jj = j;
   if (approved)
      fclose (approved);

   return (file);

err:
   *jj = j;
   if (approved)
      fclose (approved);

   *recno = -1;
   status_line ("!%s Request Err: %s", updreq ? "Update" : "File", req_list);

   return (NULL);
}

int montday[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

int daysleft (struct date *d1, struct date *d2)
{
   int i, diff = 0, m;

   if (d1->da_year < 90)
      d1->da_year += 2000;
   else
      d1->da_year += 1900;
   if (d2->da_year < 90)
      d2->da_year += 2000;
   else
      d2->da_year += 1900;

   if (d1->da_year > d2->da_year)
      d2->da_year += 100;

   if (d1->da_year < d2->da_year) {
      for (i = d1->da_year + 1; i < d2->da_year; i++) {
         m = i;
         diff += (isLeap (m)) ? 366 : 365;
      }

      diff += montday[d1->da_mon] - d1->da_day;
      for (i = d1->da_mon + 1; i <= 12; i++)
         diff += montday[i];
      if (d1->da_mon <= 2 && (isLeap (d1->da_year)))
         diff++;
      for (i = 1; i < d2->da_mon; i++)
         diff += montday[i];
      if (d2->da_mon > 2 && (isLeap (d2->da_year)))
         diff++;
      diff += d2->da_day;

      return (diff);
   }
   else if (d1->da_year == d2->da_year) {
      if (d1->da_mon > d2->da_mon)
         return (-1);

      else if (d1->da_mon < d2->da_mon) {
         diff += montday[d1->da_mon] - d1->da_day;
         for (i = d1->da_mon + 1; i < d2->da_mon; i++)
            diff += montday[i];
         if (d1->da_mon <= 2 && (isLeap (d1->da_year)))
            diff++;
         diff += d2->da_day;

         return (diff);
      }

      else
         return (d2->da_day - d1->da_day);
   }

   return (-1);
}

