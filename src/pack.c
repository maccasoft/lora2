#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <dir.h>
#include <dos.h>
#include <string.h>
#include <process.h>
#include <time.h>
#include <conio.h>
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

extern char *suffixes[];

#define PACK_ECHOMAIL  0x0001
#define PACK_NETMAIL   0x0002

int spawn_program (int swapout, char *outstring);
void pack_outbound (int flags);
void change_type (int, int, int, int, char, char);

static void call_packer (char *, char *, char *, int, int, int, int);
static char get_flag (void);
static void rename_noarc_packet (char *);

static char *flag_result (char c)
{
   if (c == 'C')
      return ("Crash");
   else if (c == 'H')
      return ("Hold");
   else if (c == 'D')
      return ("Direct");
   else if (c == 'F')
      return ("Normal");
   return ("???");
}

int flagged_file (char *outname)
{
   int fd;
   char string[128];

   strcpy (string, strupr (outname));
   strisrep (string, ".XUT", ".BSY");

   if (!dexists (string)) {
      fd = open (string, O_WRONLY|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
      write (fd, &config->line_offset, sizeof (short));
      close (fd);
      return (0);
   }

   return (-1);
}

void clear_flagged_file (char *outname)
{
   int zone;
   char string[128], *p;
   struct ffblk blk, blko, blkp;

   if (outname != NULL) {
      sprintf (string, "%s*.BSY", outname);

      if (!findfirst (string, &blk, 0))
         do {
            if (blk.ff_fsize > 0L) {
               sprintf (string, "%s%s", outname, blk.ff_name);
               unlink (string);
            }
         } while (!findnext (&blk));
   }
   else {
      strcpy (string, HoldAreaNameMunge (config->alias[0].zone));
      string[strlen (string) - 1] = '\0';
      strcat (string, ".*");

      if (!findfirst (string, &blko, FA_DIREC))
         do {
            if ((p = strchr (blko.ff_name, '.')) != NULL)
               sscanf (&p[1], "%3x", &zone);
            else
               zone = config->alias[0].zone;

            sprintf (string, "%s*.BSY", HoldAreaNameMunge (zone));

            if (!findfirst (string, &blk, 0))
               do {
                  if (blk.ff_fsize > 0L) {
                     sprintf (string, "%s%s", HoldAreaNameMunge (zone), blk.ff_name);
                     unlink (string);
                  }
               } while (!findnext (&blk));

            sprintf (string, "%s*.PNT", HoldAreaNameMunge (zone));

            if (!findfirst (string, &blkp, FA_DIREC))
               do {
                  sprintf (string, "%s%s\\*.BSY", HoldAreaNameMunge (zone), blkp.ff_name);

                  if (!findfirst (string, &blk, 0))
                     do {
                        if (blk.ff_fsize > 0L) {
                           sprintf (string, "%s%s\\%s", HoldAreaNameMunge (zone), blkp.ff_name, blk.ff_name);
                           unlink (string);
                        }
                     } while (!findnext (&blk));
               } while (!findnext (&blkp));
         } while (!findnext (&blko));
   }
}

void clear_temp_file (void)
{
   int zone;
   char string[128], newname[128], *p;
   struct ffblk blk, blko, blkp;

   strcpy (string, HoldAreaNameMunge (config->alias[0].zone));
   string[strlen (string) - 1] = '\0';
   strcat (string, ".*");

   if (!findfirst (string, &blko, FA_DIREC))
      do {
         if ((p = strchr (blko.ff_name, '.')) != NULL)
            sscanf (&p[1], "%3x", &zone);
         else
            zone = config->alias[0].zone;

         sprintf (string, "%s*.$UT", HoldAreaNameMunge (zone));

         if (!findfirst (string, &blk, 0))
            do {
               sprintf (string, "%s%s", HoldAreaNameMunge (zone), blk.ff_name);
               strcpy (newname, string);
               strisrep (newname, ".$UT", ".XUT");
               rename (string, newname);
            } while (!findnext (&blk));

         sprintf (string, "%s*.PNT", HoldAreaNameMunge (zone));

         if (!findfirst (string, &blkp, FA_DIREC))
            do {
               sprintf (string, "%s%s\\*.$UT", HoldAreaNameMunge (zone), blkp.ff_name);

               if (!findfirst (string, &blk, 0))
                  do {
                     sprintf (string, "%s%s\\%s", HoldAreaNameMunge (zone), blkp.ff_name, blk.ff_name);
                     strcpy (newname, string);
                     strisrep (newname, ".$UT", ".XUT");
                     rename (string, newname);
                  } while (!findnext (&blk));
            } while (!findnext (&blkp));
      } while (!findnext (&blko));
}

void pack_outbound (int flags)
{
   FILE *fp;
   char filename[80], dest[80], outbase[80], *p, dstf, linea[256];
   char *px, do_pack, *v, srcf, nopack;
   int dzo, dne, dno, pzo, pne, pno, dpo, i, skip_tag;
   long t;
   struct ffblk blk, blko;
   struct tm *dt;

   t = time (NULL);
   dt = localtime (&t);
   skip_tag = 0;
   nopack = 0;

   strcpy (filename, HoldAreaNameMunge (config->alias[0].zone));
   filename[strlen(filename) - 1] = '\0';
   strcpy (outbase, filename);
   strcat (filename, ".*");

   if (!findfirst (filename, &blko, FA_DIREC))
      do {
         p = strchr (blko.ff_name, '.');

         sprintf (filename, "%s%s\\*.PKT", outbase, p == NULL ? "" : p);
         if (!findfirst (filename, &blk, 0))
            do {
               if ((px = strchr (blk.ff_name, '.')) != NULL)
                  *px = '\0';
               sprintf (filename, "%s%s\\%s.PKT", outbase, p == NULL ? "" : p, blk.ff_name);
               sprintf (dest, "%s%s\\%s.P$T", outbase, p == NULL ? "" : p, blk.ff_name);
               rename (filename, dest);
            } while (!findnext (&blk));
      } while (!findnext (&blko));

   sprintf (filename, "%sROUTE.CFG", config->sys_path);
   fp = fopen (filename, "rt");
   if (fp == NULL)
      return;

   local_status ("Pack");
   pack_system ();
   setup_video_interrupt ("PACK OUTBOUND MAIL");

   while (fgets (linea, 250, fp) != NULL) {
      if (linea[0] == ';' || linea[0] == '%' || !linea[0])
         continue;
      if ((p = strchr (linea, ';')) != NULL)
         *p = '\0';
      if ((p = strchr (linea, '%')) != NULL)
         *p = '\0';
      while (linea[strlen (linea) -1] == 0x0D || linea[strlen (linea) -1] == 0x0A)
         linea[strlen (linea) -1] = '\0';

      if ((p = strtok (linea, " ")) == NULL)
         continue;

      if (!stricmp (p, "Tag")) {
         p = strtok (NULL, " ");
         if (cur_event == -1 || stricmp (p, e_ptrs[cur_event]->route_tag)) {
            skip_tag = 1;
            continue;
         }
         else if (!stricmp (p, "ECHOMAIL") && (flags & PACK_ECHOMAIL)) {
            skip_tag = 1;
            continue;
         }
         else if (!stricmp (p, "NETMAIL") && (flags & PACK_NETMAIL)) {
            skip_tag = 1;
            continue;
         }
         else
            skip_tag = 0;
         continue;
      }
      else if (skip_tag)
         continue;

      dstf = 'F';

      if (!stricmp (p, "Poll")) {
         dstf = get_flag ();

         pzo = config->alias[0].zone;
         dne = config->alias[0].net;
         dno = config->alias[0].node;
         dpo = 0;

         while ((p = strtok (NULL, " ")) != NULL) {
            parse_netnode2 (p, &dzo, &dne, &dno, &dpo);

            if (!dpo)
               sprintf (outbase, "%s%04x%04x.%cLO", HoldAreaNameMunge (dzo), dne, dno, dstf);
            else
               sprintf (outbase, "%s%04x%04x.PNT\\%08X.%cLO", HoldAreaNameMunge (dzo), dne, dno, dpo, dstf);
            i = open (outbase, O_CREAT|O_WRONLY|O_BINARY, S_IREAD|S_IWRITE);
            if (i == -1 && dpo) {
               sprintf (outbase, "%s%04x%04x.PNT", HoldAreaNameMunge (dzo), dne, dno);
               mkdir (outbase);
               sprintf (outbase, "%s%04x%04x.PNT\\%08X.%cLO", HoldAreaNameMunge (dzo), dne, dno, dpo, dstf);
               i = open (outbase, O_CREAT|O_WRONLY|O_BINARY, S_IREAD|S_IWRITE);
            }
            close (i);
         }
      }
      else if (!stricmp (p, "Send-To") || !stricmp (p, "Send")) {
         dstf = get_flag ();

         dzo = config->alias[0].zone;
         dne = config->alias[0].net;
         dno = config->alias[0].node;
         dpo = 0;

         v = strtok (NULL, "");

         while (v != NULL && (p = strtok (v, " ")) != NULL) {
            v = strtok (NULL, "");
            parse_netnode2 (p, &dzo, &dne, &dno, &dpo);
            nopack = do_pack = 0;

            if ((short)dne != -1 && (short)dno != -1) {
               if (!dpo) {
                  sprintf (filename, "%s%04x%04x.XUT", HoldAreaNameMunge (dzo), dne, dno);
                  if (!findfirst (filename, &blk, 0)) {
                     if (flagged_file (filename)) {
                        strcpy (outbase, strupr (blk.ff_name));
                        strisrep (outbase, ".XUT", ".$UT");
                        nopack = 1;
                     }
                     else
                        invent_pkt_name (outbase);
                     sprintf (dest, "%s%s", HoldAreaNameMunge (dzo), outbase);
                     if (rename (filename, dest) == -1)
                        do {
                           time_release ();
                           invent_pkt_name (outbase);
                           sprintf (dest, "%s%s", HoldAreaNameMunge (dzo), outbase);
                        } while (rename (filename, dest) == -1);
                     do_pack = 1;
                  }
               }
               else {
                  if ((short)dpo == -1)
                     sprintf (filename, "%s%04x%04x.PNT\\*.XUT", HoldAreaNameMunge (dzo), dne, dno);
                  else
                     sprintf (filename, "%s%04x%04x.PNT\\%08X.XUT", HoldAreaNameMunge (dzo), dne, dno, dpo);
                  if (!findfirst (filename, &blko, 0))
                     do {
                        sprintf (filename, "%s%04x%04x.PNT\\%s", HoldAreaNameMunge (dzo), dne, dno, blko.ff_name);
                        if (flagged_file (filename)) {
                           strcpy (outbase, strupr (blko.ff_name));
                           strisrep (outbase, ".XUT", ".$UT");
                           nopack = 1;
                        }
                        else
                           invent_pkt_name (outbase);

                        sprintf (filename, "%s%04x%04x.PNT\\%s", HoldAreaNameMunge (dzo), dne, dno, blko.ff_name);
                        sprintf (dest, "%s%04x%04x.PNT\\%s", HoldAreaNameMunge (dzo), dne, dno, outbase);
                        if (rename (filename, dest) == -1)
                           do {
                              time_release ();
                              invent_pkt_name (outbase);
                              sprintf (dest, "%s%04x%04x.PNT\\%s", HoldAreaNameMunge (dzo), dne, dno, outbase);
                           } while (rename (filename, dest) == -1);

                        if (!nopack) {
                           sscanf (blko.ff_name, "%08x", &dpo);

                           sprintf (filename, "%d:%d/%d.%d", dzo, dne, dno, dpo);
                           filename[14] = '\0';
                           prints (7, 65, YELLOW|_BLACK, "              ");
                           prints (7, 65, YELLOW|_BLACK, filename);

                           if (config->alias[0].point && config->alias[0].fakenet)
                              sprintf (filename, "%s%04x%04x.%s?", HoldAreaNameMunge (dzo), (unsigned int)(config->alias[0].fakenet - dne) & 0xFFFF, (unsigned int)(config->alias[0].point - dno) & 0xFFFF, suffixes[dt->tm_wday]);
                           else
                              sprintf (filename, "%s%04x%04x.PNT\\8%07x.%s?", HoldAreaNameMunge (dzo), dne, dno, dpo, suffixes[dt->tm_wday]);

                           if (findfirst (filename, &blk, 0)) {
                              if (config->alias[0].point && config->alias[0].fakenet)
                                 sprintf (filename, "%s%04x%04x.%s0", HoldAreaNameMunge (dzo), (unsigned int)(config->alias[0].fakenet - dne) & 0xFFFF, (unsigned int)(config->alias[0].point - dno) & 0xFFFF, suffixes[dt->tm_wday]);
                              else
                                 sprintf (filename, "%s%04x%04x.PNT\\8%07x.%s0", HoldAreaNameMunge (dzo), dne, dno, dpo, suffixes[dt->tm_wday]);
                           }
                           else if (blk.ff_fsize == 0L) {
                              sprintf (filename, "%s%04x%04x.PNT\\%s", HoldAreaNameMunge (dzo), dne, dno, blk.ff_name);
                              unlink (filename);
                              blk.ff_name[11]++;
                              if (blk.ff_name[11] > '9')
                                 blk.ff_name[11] = '0';
                              sprintf (filename, "%s%04x%04x.PNT\\%s", HoldAreaNameMunge (dzo), dne, dno, blk.ff_name);
                           }
                           else
                              sprintf (filename, "%s%04x%04x.PNT\\%s", HoldAreaNameMunge (dzo), dne, dno, blk.ff_name);

                           prints (8, 65, YELLOW|_BLACK, strupr (&filename[strlen (filename) - 12]));

                           sprintf (outbase, "%s%04x%04x.PNT\\%08x.%cLO", HoldAreaNameMunge (dzo), dne, dno, dpo, dstf);
                           sprintf (dest, "%s%04x%04x.PNT\\*.PKT", HoldAreaNameMunge (dzo), dne, dno);

                           call_packer (outbase, filename, dest, dzo, dne, dno, dpo);
                           clear_flagged_file (HoldAreaNameMunge (dzo));
                        }

                        nopack = 0;
                        do_pack = 0;
                     } while (!findnext (&blko));
               }
            }
            else {
               if ((short)dne != -1 && (short)dno == -1)
                  sprintf (filename, "%s%04x*.XUT", HoldAreaNameMunge (dzo), dne);
               else
                  sprintf (filename, "%s*.XUT", HoldAreaNameMunge (dzo));

               if (!findfirst (filename, &blko, 0))
                  do {
                     sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blko.ff_name);
                     if (flagged_file (filename)) {
                        strcpy (outbase, strupr (blk.ff_name));
                        strisrep (outbase, ".XUT", ".$UT");
                        nopack = 1;
                     }
                     else
                        invent_pkt_name (outbase);

                     sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blko.ff_name);
                     sprintf (dest, "%s%s", HoldAreaNameMunge (dzo), outbase);
                     if (rename (filename, dest) == -1)
                        do {
                           time_release ();
                           invent_pkt_name (outbase);
                           sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blko.ff_name);
                           sprintf (dest, "%s%s", HoldAreaNameMunge (dzo), outbase);
                        } while (rename (filename, dest) == -1);

                     if (!nopack) {
                        sscanf (blko.ff_name, "%04x%04x", &dne, &dno);

                        sprintf (filename, "%d:%d/%d.%d", dzo, dne, dno, dpo);
                        filename[14] = '\0';
                        prints (7, 65, YELLOW|_BLACK, "              ");
                        prints (7, 65, YELLOW|_BLACK, filename);

                        if (config->alias[0].point && config->alias[0].fakenet)
                           sprintf (filename, "%s%04x%04x.%s?", HoldAreaNameMunge (dzo), (unsigned int)(config->alias[0].fakenet - dne) & 0xFFFF, (unsigned int)(config->alias[0].point - dno) & 0xFFFF, suffixes[dt->tm_wday]);
                        else if (config->alias[0].point)
                           sprintf (filename, "%s%04x%04x.%s?", HoldAreaNameMunge (dzo), (unsigned int)(config->alias[0].node - dne) & 0xFFFF, (unsigned int)(config->alias[0].point - dno) & 0xFFFF, suffixes[dt->tm_wday]);
                        else
                           sprintf (filename, "%s%04x%04x.%s?", HoldAreaNameMunge (dzo), (unsigned int)(config->alias[0].net - dne) & 0xFFFF, (unsigned int)(config->alias[0].node - dno) & 0xFFFF, suffixes[dt->tm_wday]);

                        if (findfirst (filename, &blk, 0)) {
                           if (config->alias[0].point && config->alias[0].fakenet)
                              sprintf (filename, "%s%04x%04x.%s0", HoldAreaNameMunge (dzo), (unsigned int)(config->alias[0].fakenet - dne) & 0xFFFF, (unsigned int)(config->alias[0].point - dno) & 0xFFFF, suffixes[dt->tm_wday]);
                           else if (config->alias[0].point)
                              sprintf (filename, "%s%04x%04x.%s0", HoldAreaNameMunge (dzo), (unsigned int)(config->alias[0].node - dne) & 0xFFFF, (unsigned int)(config->alias[0].point - dno) & 0xFFFF, suffixes[dt->tm_wday]);
                           else
                              sprintf (filename, "%s%04x%04x.%s0", HoldAreaNameMunge (dzo), (unsigned int)(config->alias[0].net - dne) & 0xFFFF, (unsigned int)(config->alias[0].node - dno) & 0xFFFF, suffixes[dt->tm_wday]);
                        }
                        else if (blk.ff_fsize == 0L) {
                           sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blk.ff_name);
                           unlink (filename);
                           blk.ff_name[11]++;
                           if (blk.ff_name[11] > '9')
                              blk.ff_name[11] = '0';
                           sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blk.ff_name);
                        }
                        else
                           sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blk.ff_name);

                        prints (8, 65, YELLOW|_BLACK, strupr (&filename[strlen (filename) - 12]));

                        sprintf (outbase, "%s%04x%04x.%cLO", HoldAreaNameMunge (dzo), dne, dno, dstf);
                        sprintf (dest, "%s*.PKT", HoldAreaNameMunge (dzo));
                        call_packer (outbase, filename, dest, dzo, dne, dno, 0);
                        clear_flagged_file (HoldAreaNameMunge (dzo));
                     }

                     nopack = 0;
                     do_pack = 0;
                  } while (!findnext (&blko));

               if ((short)dne != -1 && (short)dno == -1)
                  dno = config->alias[0].node;
               else {
                  dne = config->alias[0].net;
                  dno = config->alias[0].node;
               }
            }

            if (do_pack && !nopack) {
               sprintf (filename, "%d:%d/%d.%d", dzo, dne, dno, dpo);
               filename[14] = '\0';
               prints (7, 65, YELLOW|_BLACK, "              ");
               prints (7, 65, YELLOW|_BLACK, filename);

               if (config->alias[0].point && config->alias[0].fakenet)
                  sprintf (filename, "%s%04x%04x.%s?", HoldAreaNameMunge (dzo), (unsigned int)config->alias[0].fakenet - dne, (unsigned int)(config->alias[0].point - dno) & 0xFFFF, suffixes[dt->tm_wday]);
               else {
                  if (dpo && (short)dpo != -1)
                     sprintf (filename, "%s%04x%04x.PNT\\8%07x.%s?", HoldAreaNameMunge (dzo), dne, dno, dpo, suffixes[dt->tm_wday]);
                  else
                     sprintf (filename, "%s%04x%04x.%s?", HoldAreaNameMunge (dzo), (unsigned int)(config->alias[0].net - dne) & 0xFFFF, (unsigned int)(config->alias[0].node - dno) & 0xFFFF, suffixes[dt->tm_wday]);
               }

               if (findfirst (filename, &blk, 0)) {
                  if (config->alias[0].point && config->alias[0].fakenet)
                     sprintf (filename, "%s%04x%04x.%s0", HoldAreaNameMunge (dzo), (unsigned int)(config->alias[0].fakenet - dne) & 0xFFFF, (unsigned int)(config->alias[0].point - dno) & 0xFFFF, suffixes[dt->tm_wday]);
                  else {
                     if (dpo && (short)dpo != -1)
                        sprintf (filename, "%s%04x%04x.PNT\\8%07x.%s0", HoldAreaNameMunge (dzo), dne, dno, dpo, suffixes[dt->tm_wday]);
                     else
                        sprintf (filename, "%s%04x%04x.%s0", HoldAreaNameMunge (dzo), (unsigned int)(config->alias[0].net - dne) & 0xFFFF, (unsigned int)(config->alias[0].node - dno) & 0xFFFF, suffixes[dt->tm_wday]);
                  }
               }
               else if (blk.ff_fsize == 0L) {
                  if (dpo && (short)dpo != -1)
                     sprintf (filename, "%s%04x%04x.PNT\\%s", HoldAreaNameMunge (dzo), dne, dno, blk.ff_name);
                  else
                     sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blk.ff_name);
                  unlink (filename);
                  blk.ff_name[11]++;
                  if (blk.ff_name[11] > '9')
                     blk.ff_name[11] = '0';
                  if (dpo && (short)dpo != -1)
                     sprintf (filename, "%s%04x%04x.PNT\\%s", HoldAreaNameMunge (dzo), dne, dno, blk.ff_name);
                  else
                     sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blk.ff_name);
               }
               else {
                  if (dpo && (short)dpo != -1)
                     sprintf (filename, "%s%04x%04x.PNT\\%s", HoldAreaNameMunge (dzo), dne, dno, blk.ff_name);
                  else
                     sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blk.ff_name);
               }

               prints (8, 65, YELLOW|_BLACK, strupr (&filename[strlen (filename) - 12]));

               if (dpo && (short)dpo != -1) {
                  sprintf (outbase, "%s%04x%04x.PNT\\%08x.%cLO", HoldAreaNameMunge (dzo), dne, dno, dpo, dstf);
                  sprintf (dest, "%s%04x%04x.PNT\\*.PKT", HoldAreaNameMunge (dzo), dne, dno);
               }
               else {
                  sprintf (outbase, "%s%04x%04x.%cLO", HoldAreaNameMunge (dzo), dne, dno, dstf);
                  sprintf (dest, "%s*.PKT", HoldAreaNameMunge (dzo));
               }
               call_packer (outbase, filename, dest, dzo, dne, dno, dpo);
               clear_flagged_file (HoldAreaNameMunge (dzo));
            }
         }
      }
      else if (!stricmp (p, "Change")) {
         srcf = get_flag ();
         dstf = get_flag ();

         dzo = config->alias[0].zone;
         dne = config->alias[0].net;
         dno = config->alias[0].node;
         dpo = 0;

         while ((p = strtok (NULL, " ")) != NULL) {
            parse_netnode2 (p, &dzo, &dne, &dno, &dpo);
            status_line (":Changing %d:%d/%d.%d from %s to %s", dzo, dne, dno, dpo, flag_result (srcf), flag_result (dstf));
            change_type (dzo, dne, dno, dpo, srcf, dstf);
         }
      }
      else if (!stricmp (p, "Leave")) {
         dzo = config->alias[0].zone;
         dne = config->alias[0].net;
         dno = config->alias[0].node;
         dpo = 0;

         while ((p = strtok (NULL, " ")) != NULL) {
            parse_netnode2 (p, &dzo, &dne, &dno, &i);

            if ((short)dne != -1 && (short)dno != -1) {
               sprintf (filename, "%s%04x%04x.?UT", HoldAreaNameMunge (dzo), dne, dno);
               if (!findfirst (filename, &blk, 0))
                  do {
                     sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blk.ff_name);
                     strcpy (dest, filename);
                     dest[strlen (dest) - 3] = 'N';
                     dest[strlen (dest) - 2] = filename[strlen (filename) - 3];
                     rename (filename, dest);
                  } while (!findnext (&blk));

               sprintf (filename, "%s%04x%04x.?LO", HoldAreaNameMunge (dzo), dne, dno);
               if (!findfirst (filename, &blk, 0))
                  do {
                     sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blk.ff_name);
                     strcpy (dest, filename);
                     dest[strlen (dest) - 3] = 'N';
                     dest[strlen (dest) - 2] = filename[strlen (filename) - 3];
                     rename (filename, dest);
                  } while (!findnext (&blk));
            }
            else {
               if ((short)dne != -1 && (short)dno == -1)
                  sprintf (filename, "%s%04x*.?UT", HoldAreaNameMunge (dzo), dne);
               else
                  sprintf (filename, "%s*.?UT", HoldAreaNameMunge (dzo));

               if (!findfirst (filename, &blk, 0))
                  do {
                     sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blk.ff_name);
                     strcpy (dest, filename);
                     dest[strlen (dest) - 3] = 'N';
                     dest[strlen (dest) - 2] = filename[strlen (filename) - 3];
                     rename (filename, dest);
                  } while (!findnext (&blk));

               if ((short)dne != -1 && (short)dno == -1)
                  sprintf (filename, "%s%04x*.?LO", HoldAreaNameMunge (dzo), dne);
               else
                  sprintf (filename, "%s*.?LO", HoldAreaNameMunge (dzo));

               if (!findfirst (filename, &blk, 0))
                  do {
                     sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blk.ff_name);
                     strcpy (dest, filename);
                     dest[strlen (dest) - 3] = 'N';
                     dest[strlen (dest) - 2] = filename[strlen (filename) - 3];
                     rename (filename, dest);
                  } while (!findnext (&blk));

               if ((short)dne != -1 && (short)dno == -1)
                  dno = config->alias[0].node;
               else {
                  dne = config->alias[0].net;
                  dno = config->alias[0].node;
               }
            }
         }
      }
      else if (!stricmp (p, "UnLeave")) {
         dzo = config->alias[0].zone;
         dne = config->alias[0].net;
         dno = config->alias[0].node;
         dpo = 0;

         while ((p = strtok (NULL, " ")) != NULL) {
            parse_netnode2 (p, &dzo, &dne, &dno, &i);

            if ((short)dne != -1 && (short)dno != -1) {
               sprintf (filename, "%s%04x%04x.N?T", HoldAreaNameMunge (dzo), dne, dno);
               if (!findfirst (filename, &blk, 0))
                  do {
                     sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blk.ff_name);
                     strcpy (dest, filename);
                     dest[strlen (dest) - 2] = 'U';
                     dest[strlen (dest) - 3] = filename[strlen (filename) - 2];
                     rename (filename, dest);
                  } while (!findnext (&blk));

               sprintf (filename, "%s%04x%04x.N?O", HoldAreaNameMunge (dzo), dne, dno);
               if (!findfirst (filename, &blk, 0))
                  do {
                     sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blk.ff_name);
                     strcpy (dest, filename);
                     dest[strlen (dest) - 2] = 'L';
                     dest[strlen (dest) - 3] = filename[strlen (filename) - 2];
                     rename (filename, dest);
                  } while (!findnext (&blk));
            }
            else {
               if ((short)dne != -1 && (short)dno == -1)
                  sprintf (filename, "%s%04x*.N?T", HoldAreaNameMunge (dzo), dne);
               else
                  sprintf (filename, "%s*.N?T", HoldAreaNameMunge (dzo));
               if (!findfirst (filename, &blk, 0))
                  do {
                     sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blk.ff_name);
                     strcpy (dest, filename);
                     dest[strlen (dest) - 2] = 'U';
                     dest[strlen (dest) - 3] = filename[strlen (filename) - 2];
                     rename (filename, dest);
                  } while (!findnext (&blk));

               if ((short)dne != -1 && (short)dno == -1)
                  sprintf (filename, "%s%04x*.N?O", HoldAreaNameMunge (dzo), dne);
               else
                  sprintf (filename, "%s*.N?O", HoldAreaNameMunge (dzo));
               if (!findfirst (filename, &blk, 0))
                  do {
                     sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blk.ff_name);
                     strcpy (dest, filename);
                     dest[strlen (dest) - 2] = 'L';
                     dest[strlen (dest) - 3] = filename[strlen (filename) - 2];
                     rename (filename, dest);
                  } while (!findnext (&blk));

               if ((short)dne != -1 && (short)dno == -1)
                  dno = config->alias[0].node;
               else {
                  dne = config->alias[0].net;
                  dno = config->alias[0].node;
               }
            }
         }
      }
      else if (!stricmp (p, "Route-To") || !stricmp (p, "Route")) {
         dstf = get_flag ();

         dzo = config->alias[0].zone;
         dne = config->alias[0].net;
         dno = config->alias[0].node;
         dpo = 0;
         nopack = do_pack = 0;

         p = strtok (NULL, " ");
         parse_netnode2 (p, &dzo, &dne, &dno, &i);
         pzo = dzo;
         pne = dne;
         pno = dno;

         sprintf (filename, "%s%04x%04x.XUT", HoldAreaNameMunge (dzo), dne, dno);
         if (flagged_file (filename)) {
            status_line ("+Node %d:%d/%d found busy", dzo, dne, dno);
            nopack = 1;
         }

         for (;;) {
            if ((short)pne != -1 && (short)pno != -1) {
               sprintf (filename, "%s%04x%04x.XUT", HoldAreaNameMunge (pzo), pne, pno);
               if (!findfirst (filename, &blk, 0)) {
                  if (nopack) {
                     strcpy (outbase, strupr (blk.ff_name));
                     strisrep (outbase, ".XUT", ".$UT");
                  }
                  else
                     invent_pkt_name (outbase);
                  sprintf (dest, "%s%s", HoldAreaNameMunge (nopack ? pzo : dzo), outbase);
                  if (rename (filename, dest) == -1)
                     do {
                        time_release ();
                        invent_pkt_name (outbase);
                        sprintf (dest, "%s%s", HoldAreaNameMunge (dzo), outbase);
                     } while (rename (filename, dest) == -1);
                  do_pack = 1;
               }
            }
            else {
               if ((short)pne != -1 && (short)pno == -1)
                  sprintf (filename, "%s%04x*.XUT", HoldAreaNameMunge (pzo), pne);
               else
                  sprintf (filename, "%s*.XUT", HoldAreaNameMunge (pzo));
               if (!findfirst (filename, &blk, 0))
                  do {
                     if (nopack) {
                        strcpy (outbase, strupr (blk.ff_name));
                        strisrep (outbase, ".XUT", ".$UT");
                     }
                     else
                        invent_pkt_name (outbase);

                     sprintf (filename, "%s%s", HoldAreaNameMunge (pzo), blk.ff_name);
                     sprintf (dest, "%s%s", HoldAreaNameMunge (nopack ? pzo : dzo), outbase);
                     if (rename (filename, dest) == -1)
                        do {
                           time_release ();
                           invent_pkt_name (outbase);
                           sprintf (filename, "%s%s", HoldAreaNameMunge (pzo), blk.ff_name);
                           sprintf (dest, "%s%s", HoldAreaNameMunge (dzo), outbase);
                        } while (rename (filename, dest) == -1);
                     do_pack = 1;
                  } while (!findnext (&blk));
            }

            if ((p = strtok (NULL, " ")) == NULL)
               break;
            parse_netnode2 (p, &pzo, &pne, &pno, &i);
         }

         if (do_pack && !nopack) {
            sprintf (filename, "%d:%d/%d.%d", dzo, dne, dno, dpo);
            filename[14] = '\0';
            prints (7, 65, YELLOW|_BLACK, "              ");
            prints (7, 65, YELLOW|_BLACK, filename);

            if (config->alias[0].point && config->alias[0].fakenet)
               sprintf (filename, "%s%04x%04x.%s?", HoldAreaNameMunge (dzo), (unsigned int)(config->alias[0].fakenet - dne) & 0xFFFF, (unsigned int)(config->alias[0].point - dno) & 0xFFFF, suffixes[dt->tm_wday]);
            else
               sprintf (filename, "%s%04x%04x.%s?", HoldAreaNameMunge (dzo), (unsigned int)(config->alias[0].net - dne) & 0xFFFF, (unsigned int)(config->alias[0].node - dno) & 0xFFFF, suffixes[dt->tm_wday]);

            if (findfirst (filename, &blk, 0)) {
               if (config->alias[0].point && config->alias[0].fakenet)
                  sprintf (filename, "%s%04x%04x.%s0", HoldAreaNameMunge (dzo), (unsigned int)(config->alias[0].fakenet - dne) & 0xFFFF, (unsigned int)(config->alias[0].point - dno) & 0xFFFF, suffixes[dt->tm_wday]);
               else
                  sprintf (filename, "%s%04x%04x.%s0", HoldAreaNameMunge (dzo), (unsigned int)(config->alias[0].net - dne) & 0xFFFF, (unsigned int)(config->alias[0].node - dno) & 0xFFFF, suffixes[dt->tm_wday]);
            }
            else if (blk.ff_fsize == 0L) {
               sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blk.ff_name);
               unlink (filename);
               blk.ff_name[11]++;
               if (blk.ff_name[11] > '9')
                  blk.ff_name[11] = '0';
               sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blk.ff_name);
            }
            else
               sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blk.ff_name);

            prints (8, 65, YELLOW|_BLACK, strupr (&filename[strlen (filename) - 12]));

            sprintf (outbase, "%s%04x%04x.%cLO", HoldAreaNameMunge (dzo), dne & 0xFFFF, dno & 0xFFFF, dstf);
            sprintf (dest, "%s*.PKT", HoldAreaNameMunge (dzo));
            call_packer (outbase, filename, dest, dzo, dne, dno, 0);
         }

         clear_flagged_file (NULL);
      }
      else
         status_line ("!Unknown keyword '%s'", p);
   }

   fclose (fp);

   clear_temp_file ();
   hidecur ();
   wclose ();

   printc (12, 0, LGREY|_BLACK, 'Ã');
   printc (12, 52, LGREY|_BLACK, 'Å');
   printc (12, 79, LGREY|_BLACK, '´');
}

static void call_packer (attach, arcmail, packet, zone, net, node, point)
char *attach, *arcmail, *packet;
int zone, net, node, point;
{
   FILE *fp;
   int fd, x, arctype, *varr;
   char outbase[128], outstring[128], swapout;
   long pktsize;
   NODEINFO ni;
   struct ffblk blk;
   struct _pkthdr2 pkthdr;

   arctype = -1;

   sprintf (outbase, "%sNODES.DAT", config->net_info);
   if ((fd = open (outbase, O_RDONLY|O_BINARY)) != -1) {
      while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO))
         if (ni.zone == zone && ni.node == node && ni.net == net && ni.point == point) {
            arctype = ni.packer;
            break;
         }
      close (fd);
   }

   strcpy (outbase, packet);
   outbase[strlen (outbase) - 5] = '\0';
   pktsize = 0L;

   if (!findfirst (packet, &blk, 0))
      do {
         pktsize += blk.ff_fsize;
         sprintf (outstring, "%s%s", outbase, blk.ff_name);
         fd = open (outstring, O_RDWR|O_BINARY);
         if (fd != -1) {
            read (fd, (char *)&pkthdr, sizeof (struct _pkthdr2));
            memset (pkthdr.password, 0, 8);
            if (arctype != -1)
               strcpy (pkthdr.password, ni.pw_packet);
            pkthdr.dest_zone2 = pkthdr.dest_zone = zone;
            pkthdr.dest_net = net;
            pkthdr.dest_node = node;
            pkthdr.dest_point = point;
            lseek (fd, 0L, SEEK_SET);
            write (fd, (char *)&pkthdr, sizeof (struct _pkthdr2));
            close (fd);
         }
      } while (!findnext (&blk));

   x = 0;
   if ((fp = fopen (attach, "rt")) != NULL) {
      while (fgets (outbase, 79, fp) != NULL) {
         if (!outbase[0])
            continue;
         while (strlen (outbase) && (outbase[strlen (outbase) -1] == 0x0D || outbase[strlen (outbase) -1] == 0x0A))
            outbase[strlen (outbase) -1] = '\0';
         if ((outbase[0] == '#' || outbase[0] == '^') && !stricmp (&outbase[1], arcmail)) {
            x = 1;
            break;
         }
         else if (!stricmp (outbase, arcmail)) {
            x = 1;
            break;
         }
      }
      fclose (fp);
   }

   if (!x) {
      fp = fopen (attach, "at");
      fprintf (fp, "#%s\n", fancy_str (arcmail));
      fclose (fp);
   }

   activation_key ();

   if (arctype == -1)
      arctype = 0;

   prints (9, 65, YELLOW|_BLACK, "        ");
   prints (9, 65, YELLOW|_BLACK, config->packers[arctype].id);
   time_release ();
   if (config->packers[arctype].packcmd[0] == '+') {
      strcpy (outstring, &config->packers[arctype].packcmd[1]);
      swapout = 1;
   }
   else {
      strcpy (outstring, config->packers[arctype].packcmd);
      swapout = 0;
   }
   strsrep (outstring, "%1", arcmail);
   strsrep (outstring, "%2", packet);

   status_line ("#%sing mail for %d:%d/%d.%d (%ld bytes)", config->packers[arctype].id, zone, net, node, point, pktsize);

   varr = ssave ();
   gotoxy (1, 14);
   wtextattr (LGREY|_BLACK);

   x = spawn_program (swapout, outstring);

   if (varr != NULL)
      srestore (varr);

   if (x != 0) {
      status_line (":Return code: %d", x);
      rename_noarc_packet (packet);
   }

   time_release ();
}

static char get_flag ()
{
   char *p;

   p = strtok (NULL, " ");
   if (!stricmp (p, "Crash"))
      return ('C');
   else if (!stricmp (p, "Hold"))
      return ('H');
   else if (!stricmp (p, "Direct"))
      return ('D');
   else if (!stricmp (p, "Normal"))
      return ('F');
   else
      return ('F');
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static void rename_noarc_packet (name)
char *name;
{
   int i = -1;
   char filename[128], *p, dir[80], newname[80];
   struct ffblk blk;

   strcpy (dir, name);
   dir[strlen (dir) - 5] = '\0';

   sprintf (filename, "%sNOARC.*", dir);
   if (!findfirst (filename, &blk, 0))
      do {
         if ((p = strchr (blk.ff_name, '.')) != NULL) {
            p++;
            if (atoi (p) > i)
               i = atoi (p);
         }
      } while (!findnext (&blk));

   i++;

   if (!findfirst (name, &blk, 0))
      do {
         sprintf (filename, "%s%s", dir, blk.ff_name);
         sprintf (newname, "%sNOARC.%03d", dir, i);
         rename (filename, newname);
      } while (!findnext (&blk));
}

void change_type (zo, ne, no, po, from, to)
int zo, ne, no, po;
char from, to;
{
   FILE *fps, *fpd;
   int i;
   char string[256], filename[80];

   if (from == to)
      return;

   if (from == 'F')
      from = 'O';
   if (to == 'F')
      to = 'O';

   if (po)
      sprintf (filename, "%s%04x%04x.PNT\\%08x.%cUT", HoldAreaNameMunge (zo), ne, no, po, from);
   else
      sprintf (filename, "%s%04x%04x.%cUT", HoldAreaNameMunge (zo), ne, no, from);

   if (po)
      sprintf (string, "%s%04x%04x.PNT\\%08x.%cUT", HoldAreaNameMunge (zo), ne, no, po, to);
   else
      sprintf (string, "%s%04x%04x.%cUT", HoldAreaNameMunge (zo), ne, no, to);

   rename (filename, string);

   if (from == 'O')
      from = 'F';
   if (to == 'O')
      to = 'F';

   if (po)
      sprintf (filename, "%s%04x%04x.PNT\\%08x.%cLO", HoldAreaNameMunge (zo), ne, no, po, from);
   else
      sprintf (filename, "%s%04x%04x.%cLO", HoldAreaNameMunge (zo), ne, no, from);
   fps = fopen (filename, "rt");
   if (fps == NULL)
      return;

   if (po)
      sprintf (filename, "%s%04x%04x.PNT\\%08x.%cLO", HoldAreaNameMunge (zo), ne, no, po, to);
   else
      sprintf (filename, "%s%04x%04x.%cLO", HoldAreaNameMunge (zo), ne, no, to);
   fpd = fopen (filename, "at");
   if (fpd == NULL)
      return;

   while (fgets (string, 250, fps) != NULL)
      fputs (string, fpd);

   fclose (fpd);
   fclose (fps);

   if (po)
      sprintf (filename, "%s%04x%04x.PNT\\%08x.%cLO", HoldAreaNameMunge (zo), ne, no, po, from);
   else
      sprintf (filename, "%s%04x%04x.%cLO", HoldAreaNameMunge (zo), ne, no, from);
   unlink (filename);

   if (po)
      sprintf (filename, "%s%04x%04x.PNT\\%08x.%cUT", HoldAreaNameMunge (zo), ne, no, po, from);
   else
      sprintf (filename, "%s%04x%04x.%cUT", HoldAreaNameMunge (zo), ne, no, from);
   fps = fopen (filename, "rt");
   if (fps == NULL)
      return;

   if (po)
      sprintf (filename, "%s%04x%04x.PNT\\%08x.%cUT", HoldAreaNameMunge (zo), ne, no, po, to);
   else
      sprintf (filename, "%s%04x%04x.%cUT", HoldAreaNameMunge (zo), ne, no, to);
   fpd = fopen (filename, "at");
   if (fpd == NULL)
      return;

   if (filelength (fileno (fpd)) > 0L)
      fseek (fps, sizeof (struct _pkthdr2), SEEK_SET);

   while ((i = fread (string, 1, 250, fps)) == 250)
      fwrite (string, 1, i, fpd);

   fclose (fpd);
   fclose (fps);

   if (po)
      sprintf (filename, "%s%04x%04x.PNT\\%08x.%cUT", HoldAreaNameMunge (zo), ne, no, po, from);
   else
      sprintf (filename, "%s%04x%04x.%cUT", HoldAreaNameMunge (zo), ne, no, from);
   unlink (filename);
}

int check_hold (int zone, int net, int node, int point)
{
   FILE *fp;
   char filename[80], linea[80], *p, *v, dstf;
   int dzo, dne, dno, dpo;

   sprintf (filename, "%sROUTE.CFG", config->sys_path);
   fp = fopen (filename, "rt");
   if (fp == NULL)
      return (0);

   while (fgets (linea, 79, fp) != NULL) {
      if (linea[0] == ';' || linea[0] == '%' || !linea[0])
         continue;
      if ((p = strchr (linea, ';')) != NULL)
         *p = '\0';
      if ((p = strchr (linea, '%')) != NULL)
         *p = '\0';
      while (linea[strlen (linea) -1] == 0x0D || linea[strlen (linea) -1] == 0x0A)
         linea[strlen (linea) -1] = '\0';

      if ((p = strtok (linea, " ")) == NULL)
         continue;

      if (!stricmp (p, "Send-To") || !stricmp (p, "Send")) {
         dstf = get_flag ();
         if (dstf != 'H')
            continue;

         dzo = config->alias[0].zone;
         dne = config->alias[0].net;
         dno = config->alias[0].node;
         dpo = 0;

         v = strtok (NULL, "");

         while (v != NULL && (p = strtok (v, " ")) != NULL) {
            v = strtok (NULL, "");
            parse_netnode2 (p, &dzo, &dne, &dno, &dpo);
            if (dzo == zone && dno == node && dne == net && dpo == point) {
               fclose (fp);
               return (1);
            }
         }
      }
   }

   fclose (fp);
   return (0);
}

int check_route_flag (int zone, int net, int node, int point)
{
   FILE *fp;
   char filename[80], linea[80], *p, *v, dstf;
   int dzo, dne, dno, dpo;

   sprintf (filename, "%sROUTE.CFG", config->sys_path);
   fp = fopen (filename, "rt");
   if (fp == NULL)
      return ('H');

   while (fgets (linea, 79, fp) != NULL) {
      if (linea[0] == ';' || linea[0] == '%' || !linea[0])
         continue;
      if ((p = strchr (linea, ';')) != NULL)
         *p = '\0';
      if ((p = strchr (linea, '%')) != NULL)
         *p = '\0';
      while (linea[strlen (linea) -1] == 0x0D || linea[strlen (linea) -1] == 0x0A)
         linea[strlen (linea) -1] = '\0';

      if ((p = strtok (linea, " ")) == NULL)
         continue;

      if (!stricmp (p, "SendFile-To")) {
         dstf = get_flag ();

         dzo = config->alias[0].zone;
         dne = config->alias[0].net;
         dno = config->alias[0].node;
         dpo = 0;

         v = strtok (NULL, "");

         while (v != NULL && (p = strtok (v, " ")) != NULL) {
            v = strtok (NULL, "");
            parse_netnode2 (p, &dzo, &dne, &dno, &dpo);
            if (dzo == zone && dno == node && dne == net && dpo == point) {
               fclose (fp);
               return (dstf);
            }
         }
      }

      if (!stricmp (p, "RouteFile-To")) {
         dstf = get_flag ();

         dzo = config->alias[0].zone;
         dne = config->alias[0].net;
         dno = config->alias[0].node;
         dpo = 0;

         if ((p = strtok (NULL, " ")) != NULL) {
            parse_netnode2 (p, &dzo, &dne, &dno, &dpo);
            if (dzo == zone && dno == node && dne == net && dpo == point) {
               fclose (fp);
               return (dstf);
            }
         }
      }
   }

   rewind (fp);

   while (fgets (linea, 79, fp) != NULL) {
      if (linea[0] == ';' || linea[0] == '%' || !linea[0])
         continue;
      if ((p = strchr (linea, ';')) != NULL)
         *p = '\0';
      if ((p = strchr (linea, '%')) != NULL)
         *p = '\0';
      while (linea[strlen (linea) -1] == 0x0D || linea[strlen (linea) -1] == 0x0A)
         linea[strlen (linea) -1] = '\0';

      if ((p = strtok (linea, " ")) == NULL)
         continue;

      if (!stricmp (p, "Send-To") || !stricmp (p, "Send")) {
         dstf = get_flag ();

         dzo = config->alias[0].zone;
         dne = config->alias[0].net;
         dno = config->alias[0].node;
         dpo = 0;

         v = strtok (NULL, "");

         while (v != NULL && (p = strtok (v, " ")) != NULL) {
            v = strtok (NULL, "");
            parse_netnode2 (p, &dzo, &dne, &dno, &dpo);
            if (dzo == zone && dno == node && dne == net && dpo == point) {
               fclose (fp);
               return (dstf);
            }
         }
      }

      if (!stricmp (p, "Route-To") || !stricmp (p, "Route")) {
         dstf = get_flag ();

         dzo = config->alias[0].zone;
         dne = config->alias[0].net;
         dno = config->alias[0].node;
         dpo = 0;

         if ((p = strtok (NULL, " ")) != NULL) {
            parse_netnode2 (p, &dzo, &dne, &dno, &dpo);
            if (dzo == zone && dno == node && dne == net && dpo == point) {
               fclose (fp);
               return (dstf);
            }
         }
      }
   }

   fclose (fp);
   return ('H');
}

