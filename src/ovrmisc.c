#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <dos.h>
#include <string.h>
#include <dir.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <alloc.h>
#include <sys\stat.h>

#include <cxl\cxlstr.h>
#include <cxl\cxlwin.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"

static int lora_get_bbs_record(int, int, int, int);
static int update_nodelist (char *, char *);


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

int get_bbs_record (zone, net, node, point)
int zone, net, node;
{
        char stringa[80];

        sprintf (stringa, "%sNODE.IDX", net_info);
        if (dexists (stringa))
                return (lora_get_bbs_record (zone, net, node, point));

        return (0);
}

struct _idx_header {
   char name[14];
   long entry;
};

struct _idx_entry {
   int  zone;
   int  net;
   long offset;
};

void build_nodelist_index (force)
int force;
{
   FILE *fps, *fpd, *fpcfg;
   int nzone, nnet, i;
   char filename[80], linea[512], build, *p, *diff, final[14];
   long hdrpos, entrypos;
   struct stat statbuf, statcfg, statidx;
   struct _idx_header header;
   struct _idx_entry entry;

   build = 0;
   nzone = alias[0].zone;
   nnet = alias[0].net;

   sprintf (filename, "%sNODELIST.CFG", net_info);
   if (stat (filename, &statcfg) == -1)
      return;

   sprintf (filename, "%sNODE.IDX", net_info);
   if (stat (filename, &statidx) == -1)
      build = 1;
//   else if (statidx.st_mtime < statcfg.st_mtime)
//      build = 1;

   sprintf (filename, "%sNODELIST.CFG", net_info);
   fpcfg = fopen (filename, "rt");

   while (fgets(linea, 511, fpcfg) != NULL) {
      if (linea[0] == ';' || linea[0] == '%')
         continue;
      while (linea[strlen (linea) -1] == 0x0D || linea[strlen (linea) -1] == 0x0A || linea[strlen (linea) -1] == ' ')
         linea[strlen (linea) -1] = '\0';
      p = strtok (linea, " ");
      if (!stricmp (p, "NODELIST")) {
         p = strtok (NULL, " ");
         sprintf (filename, "%s%s", net_info, p);
         if (!stat (filename, &statbuf)) {
            if (statbuf.st_mtime > statidx.st_mtime)
               build = 1;
         }
      }
   }

   memset ((char *)&nodelist, 0, sizeof (struct _node));

   sprintf (filename, "%sNODE.IDX", net_info);
   fps = fopen (filename, "rb");

   while (fread ((char *)&header, sizeof (struct _idx_header), 1, fps) == 1) {
      sprintf (filename, "%s%s", net_info, header.name);
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

   if (build || force) {
      rewind (fpcfg);

      status_line(":Rebuild nodelist index");
      wputs("* Rebuild nodelist index\n");

      sprintf (filename, "%sNODE.IDX", net_info);
      fpd = fopen (filename, "w+b");

      while (fgets(linea, 511, fpcfg) != NULL) {
         if (linea[0] == ';' || linea[0] == '%')
            continue;

         while (linea[strlen (linea) -1] == 0x0D || linea[strlen (linea) -1] == 0x0A || linea[strlen (linea) -1] == ' ')
            linea[strlen (linea) -1] = '\0';

         p = strtok (linea, " ");
         if (stricmp (p, "NODELIST"))
            continue;

         p = strtok (NULL, " ");
         if (strchr (p, '.') == NULL) {
            diff = strtok (NULL, " ");
            i = update_nodelist (p, diff);
            if (i == -1)
               continue;
            sprintf (final, "%s.%03d", p, i);
         }
         else
            strcpy (final, p);
         sprintf (filename, "%s%s", net_info, final);
         fps = fopen (filename, "rb");
         if (fps == NULL)
            continue;

         status_line ("+Compiling %s", fancy_str (filename));

         hdrpos = ftell  (fpd);
         memset ((char *)&header, 0, sizeof (struct _idx_header));
         strcpy (header.name, strupr(final));
         fwrite ((char *)&header, sizeof (struct _idx_header), 1, fpd);

         entrypos = ftell (fps);
         nzone = alias[0].zone;
         nnet = alias[0].net;

         while (fgets(linea, 511, fps) != NULL) {
            if (linea[0] == ';') {
               entrypos = ftell (fps);
               continue;
            }

            p = strtok (linea, ",");
            if (stricmp (p, "Zone") && stricmp (p, "Region") && stricmp (p, "Host")) {
               entrypos = ftell (fps);
               continue;
            }

            if (!stricmp (p, "Zone")) {
               p = strtok (NULL, ",");
               nzone = nnet = atoi (p);
            }
            else {
               p = strtok (NULL, ",");
               nnet = atoi (p);
            }

            wclreol ();
            sprintf (filename, "%d:%d/0\r", nzone, nnet);
            wputs (filename);

            entry.zone = nzone;
            entry.net = nnet;
            entry.offset = entrypos;
            fwrite ((char *)&entry, sizeof (struct _idx_entry), 1, fpd);
            header.entry++;
            entrypos = ftell (fps);
         }

         fclose (fps);

         fseek (fpd, hdrpos, SEEK_SET);
         fwrite ((char *)&header, sizeof (struct _idx_header), 1, fpd);
         fseek (fpd, 0L, SEEK_END);
      }

      fclose (fpd);
   }

   wclreol ();

   fclose (fpcfg);
}

static int lora_get_bbs_record(zone, net, node, point)
int zone, net, node, point;
{
   FILE *fp;
   int nzone, nnet, nnode, npoint, i;
   char filename[80], linea[512], *p, first;
   long num;
   struct _idx_header header;
   struct _idx_entry entry;

   if (point);

   memset ((char *)&nodelist, 0, sizeof (struct _node));

   sprintf (filename, "%sNODE.IDX", net_info);
   fp = fopen (filename, "rb");

   while (fread ((char *)&header, sizeof (struct _idx_header), 1, fp) == 1) {
      for (num = 0L; num < header.entry; num++) {
         fread ((char *)&entry, sizeof (struct _idx_entry), 1, fp);
         if ((zone && entry.zone == zone) && entry.net == net) {
            fclose (fp);
            if (strchr (header.name, '.') == NULL) {
               i = update_nodelist (header.name, NULL);
               if (i == -1)
                  return (0);
               sprintf (filename, "%s%s.%03d", net_info, header.name, i);
            }
            else
               sprintf (filename, "%s%s", net_info, header.name);
            fp = fopen (filename, "rb");
            if (fp == NULL)
               return (0);
            fseek (fp, entry.offset, SEEK_SET);
            first = 0;
            while (fgets(linea, 511, fp) != NULL) {
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

                  p = strtok (NULL, ",");
               }
               else
                  p = strtok (linea, ",");

               if (atoi (p) != node)
                  continue;

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

               strtok (NULL, ",");

               p = strtok (NULL, ",");
               if (strlen (p) > 39)
                  p[39] = '\0';
               strcpy (nodelist.phone, p);

               p = strtok (NULL, ",");
               nodelist.rate = atoi (p) / 300;

               nodelist.net = net;
               nodelist.number = node;

               p = strtok (NULL, " ");
               if (strstr (p, "HST") != NULL)
                  nodelist.modem |= 0x01;
               if (strstr (p, "PEP") != NULL)
                  nodelist.modem |= 0x02;

               fclose (fp);

               sprintf (filename, "%sNODELIST.CFG", net_info);
               fp = fopen (filename, "rt");

               while (fgets(linea, 511, fp) != NULL) {
                  if (linea[0] == ';' || linea[0] == '%')
                     continue;

                  while (linea[strlen (linea) -1] == 0x0D || linea[strlen (linea) -1] == 0x0A || linea[strlen (linea) -1] == ' ')
                     linea[strlen (linea) -1] = '\0';

                  p = strtok (linea, " ");
                  if (!stricmp (p, "PASSWORD")) {
                     p = strtok (NULL, " ");
                     parse_netnode (p, &nzone, &nnet, &nnode, &npoint);
                     if (nzone == zone && nnet == net && nnode == node && !npoint) {
                        p = strtok (NULL, " ");
                        if (strlen (p) > 29)
                           p[29] = '\0';
                        strcpy (nodelist.password, strupr(p));
                     }
                  }
                  else if (!stricmp (p, "PHONE")) {
                     p = strtok (NULL, " ");
                     parse_netnode (p, &nzone, &nnet, &nnode, &npoint);
                     if (nzone == zone && nnet == net && nnode == node && !npoint) {
                        p = strtok (NULL, " ");
                        if (strlen (p) > 39)
                           p[39] = '\0';
                        strcpy (nodelist.phone, p);
                     }
                  }
                  else if (!stricmp (p, "DIAL")) {
                     p = strtok (NULL, " ");
                     if (!strncmp (nodelist.phone, p, strlen (p))) {
                        strcpy (filename, p);
                        p = strtok (NULL, " ");
                        strisrep (nodelist.phone, filename, p);
                     }
                  }
               }

               fclose (fp);
               return (1);
            }

            fclose (fp);
            return (0);
         }
      }
   }

   fclose (fp);
   return (0);
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
   struct tm *tim;

   tempo = time (NULL);
   tim = localtime(&tempo);

   sprintf (filename, "%s%s.*", net_info, name);
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
   if ((isLeap (tim->tm_year)) && next > 366)
      next -= 366;
   else if (!(isLeap (tim->tm_year)) && next > 365)
      next -= 365;

   if (diff == NULL)
      return (max);

   sprintf (filename, "%s%s.%03d", net_info, diff, next);
   if (!dexists (filename))
      return (max);

   status_line ("+Updating %s.%03d with %s.%03d", name, max, diff, next);

   fps = fopen(filename, "rt");
   setvbuf (fps, NULL, _IOFBF, 2048);

   sprintf (filename, "%s%s.%03d", net_info, name, max);
   fpn = fopen(filename, "rt");
   setvbuf (fpn, NULL, _IOFBF, 2048);

   sprintf (filename, "%s%s.%03d", net_info, name, next);
   fpd = fopen(filename, "wt");
   setvbuf (fpd, NULL, _IOFBF, 2048);

   fgets(linea, 511, fps);
   fgets(filename, 127, fpn);
   if (stricmp (linea, filename)) {
      status_line ("!%s.%03d doesn't match %s.%03d", diff, next, name, max);
      fclose (fpd);
      fclose (fpn);
      fclose (fps);

      sprintf (filename, "%s%s.%03d", net_info, name, next);
      unlink (filename);
      sprintf (filename, "%s%s.%03d", net_info, diff, next);
      unlink (filename);

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
   }

   fclose (fpd);
   fclose (fpn);
   fclose (fps);

   sprintf (filename, "%s%s.%03d", net_info, name, max);
   unlink (filename);
   sprintf (filename, "%s%s.%03d", net_info, diff, next);
   unlink (filename);

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
