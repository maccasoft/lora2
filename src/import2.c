#include <stdio.h>
#include <dir.h>
#include <dos.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <sys\stat.h>

#include <cxl\cxlwin.h>
#include <cxl\cxlvid.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "prototyp.h"
#include "externs.h"

#define Z_32UpdateCRC(c,crc) (cr3tab[((int) crc ^ c) & 0xff] ^ ((crc >> 8) & 0x00FFFFFFL))

struct _aidx {
   char areatag[32];
   byte board;
};

extern struct _aidx *aidx;
extern char *wtext[];
extern int maxakainfo;
extern struct _akainfo *akainfo;

int check_route_flag (int zone, int net, int node, int point);
void fido_save_message2 (FILE *, char *);
FILE *mopen (char *filename, char *mode);
int mclose (FILE *fp);
void mprintf (FILE *fp, char *format, ...);
long mseek (FILE *fp, long position, int offset);
int mputs (char *s, FILE *fp);
int mread (char *s, int n, int e, FILE *fp);

void build_aidx (void);
int quick_rescan_echomail (int board, int zone, int net, int node, int point);
void pip_rescan_echomail (int board, char *tag, int zone, int net, int node, int point);
void fido_rescan_echomail (char *tag, int zone, int net, int node, int point);
void squish_rescan_echomail (char *tag, int zone, int net, int node, int point);

static void rename_bad_tics (char *, char *);

struct _fwd_alias {
   short zone;
   short net;
   short node;
   short point;
   bit   passive :1;
   bit   receive :1;
   bit   send    :1;
};

#define MAX_FORWARD 50

/*
   ! - Nodo passivo
   < - Nodo a sola trasmissione (il nodo non riceve i files)
   > - Nodo a sola ricezione (il nodo non puo' trasmettere files)
*/

static long xtol (char *p)
{
   long r = 0L;

   while (*p) {
      if (isdigit (*p)) {
         r *= 16L;
         r += *p - '0';
      }
      else if (isxdigit (*p)) {
         r *= 16L;
         r += *p - 55;
      }
      p++;
   }

   return (r);
}

#define MAX_BUFFERING 2048

void import_tic_files ()
{
   FILE *fp, *fpcfg, *fpd;
   int i, zo, ne, no, po, fzo, fne, fno, fpo, fds, fdd, d;
   int wh, ozo, one, ono, opo, nrec, nsent, n_forw, nf;
   char *inpath, filename[80], linea[MAX_BUFFERING], area[20], descr[512], name[14], *p;
   char pw[32], dpath[80], dpw[32], found, fi, repl[14], seen;
   char path;
   long crc, pos, crcf;
   float t;
   struct ffblk blk;
   struct _wrec_t *wr;
   struct tm *tim;
   struct _sys tsys;
   struct _fwd_alias *forward;

   forward = (struct _fwd_alias *)malloc (MAX_FORWARD * sizeof (struct _fwd_alias));
   if (forward == NULL)
      return;

   sprintf (filename, "%sSYSFILE.DAT", config->sys_path);
   fpcfg = fopen (filename, "r+b");
   if (fpcfg == NULL) {
      free (forward);
      return;
   }

   fi = 0;
   nrec = nsent = 0;

   local_status ("TIC");
   wfill (7, 53, 11, 78, ' ', LCYAN|_BLACK);

   prints (7, 54, LCYAN|_BLACK, "   Packet:");
   prints (8, 54, LCYAN|_BLACK, "     File:");
   prints (9, 54, LCYAN|_BLACK, "     From:");
   prints (10, 54, LCYAN|_BLACK, "Recv/Sent:");

   wh = wopen (12, 0, 24, 79, 0, LGREY|_BLACK, LCYAN|_BLACK);
   wactiv (wh);
   wtitle ("PROCESS TIC FILES", TLEFT, LCYAN|_BLACK);
   wprints (0, 0, YELLOW|_BLACK, " File          Size     Area name            From");
   printc (12, 0, LGREY|_BLACK, 'Ã');
   printc (12, 52, LGREY|_BLACK, 'Á');
   printc (12, 79, LGREY|_BLACK, '´');
   wr = wfindrec (wh);
   wr->srow++;
   wr->row++;

   for (d = 0; d < 3; d++) {
      if ( (e_ptrs[cur_event]->echomail & (ECHO_PROT|ECHO_KNOW|ECHO_NORMAL)) ) {
         if (d == 0 && config->prot_filepath[0] && (e_ptrs[cur_event]->echomail & ECHO_PROT))
            inpath = config->prot_filepath;
         if (d == 1 && config->know_filepath[0] && (e_ptrs[cur_event]->echomail & ECHO_KNOW))
            inpath = config->know_filepath;
         if (d == 2 && config->norm_filepath[0] && (e_ptrs[cur_event]->echomail & ECHO_NORMAL))
            inpath = config->norm_filepath;
      }
      else {
         if (d == 0 && config->prot_filepath[0])
            inpath = config->prot_filepath;
         if (d == 1 && config->know_filepath[0])
            inpath = config->know_filepath;
         if (d == 2 && config->norm_filepath[0])
            inpath = config->norm_filepath;
      }

      sprintf (filename, "%s*.TIC", inpath);
      if (findfirst (filename, &blk, 0))
         continue;

      if (!fi) {
         status_line ("+Processing inbound TICs");
         fi = 1;
      }

      for (;;) {
         sprintf (filename, "%s*.TIC", inpath);
         if (findfirst (filename, &blk, 0))
            break;

         sprintf (filename, "%s%s", inpath, blk.ff_name);
         fp = fopen (filename, "rt");
         if (fp == NULL)
            continue;

         prints (7, 65, YELLOW|_BLACK, blk.ff_name);
         descr[0] = repl[0] = '\0';
         name[0] = area[0] = pw[0] = '\0';
         crc = 0L;
         fzo = fne = fno = fpo = 0;
         ozo = one = ono = opo = 0;

         while (fgets (linea, 1020, fp) != NULL) {
            while (strlen (linea) > 0 && (linea[strlen (linea) - 1] == 0x0D || linea[strlen (linea) - 1] == 0x0A))
               linea[strlen (linea) - 1] = '\0';

            if (!strnicmp (linea, "Desc ", 5)) {
               p = &linea[5];
               if (strlen (p) > 511)
                  p[511] = '\0';
               strcpy (descr, p);
               continue;
            }

            if ((p = strtok (linea, " ")) == NULL)
               continue;

            if (!stricmp (p, "Area")) {
               p = strtok (NULL, " ");
               if (strlen (p) > 19)
                  p[19] = '\0';
               strcpy (area, p);
            }
            else if (!stricmp (p, "From")) {
               p = strtok (NULL, " ");
               fzo = config->alias[0].zone;
               fne = config->alias[0].net;
               fno = fpo = 0;
               parse_netnode (p, &fzo, &fne, &fno, &fpo);
               prints (9, 65, YELLOW|_BLACK, "            ");
               if (strlen (p) > 14)
                  p[14] = '\0';
               prints (9, 65, YELLOW|_BLACK, p);
            }
            else if (!stricmp (p, "Origin")) {
               p = strtok (NULL, " ");
               ozo = config->alias[0].zone;
               one = config->alias[0].net;
               ono = opo = 0;
               parse_netnode (p, &ozo, &one, &ono, &opo);
            }
            else if (!stricmp (p, "File")) {
               p = strtok (NULL, " ");
               if (strlen (p) > 12)
                  p[12] = '\0';
               strcpy (name, p);
               prints (8, 65, YELLOW|_BLACK, "            ");
               prints (8, 65, YELLOW|_BLACK, name);
            }
            else if (!stricmp (p, "Replaces")) {
               p = strtok (NULL, " ");
               if (strlen (p) > 12)
                  p[12] = '\0';
               strcpy (repl, p);
            }
            else if (!stricmp (p, "CRC")) {
               p = strtok (NULL, " ");
               crc = xtol (p);
            }
            else if (!stricmp (p, "Pw")) {
               if ((p = strtok (NULL, " ")) != NULL) {
                  if (strlen (p) > 31)
                     p[31] = '\0';
                  strcpy (pw, p);
               }
               else
                  pw[0] = '\0';
            }
         }

         sprintf (filename, "%s%s", inpath, name);
         fds = open (filename, O_RDONLY|O_BINARY);
         if (fds != -1) {
            pos = filelength (fds);

            crcf = 0xFFFFFFFFL;

            do {
               nf = read (fds, linea, MAX_BUFFERING);
               for (i = 0; i < nf; i++)
                  crcf = Z_32UpdateCRC ((unsigned short)linea[i], crcf);
            } while (nf == 2048);
            close (fds);

            crcf = ~crcf;

            if (crcf != crc) {
               fclose (fp);
               status_line ("!%s bad CRC! (%08lX / %08lX)", name, crcf, crc);
               rename_bad_tics (inpath, blk.ff_name);
               continue;
            }
         }
         else {
            fclose (fp);
            status_line ("!%s not found in inbound area", name);
            rename_bad_tics (inpath, blk.ff_name);
            continue;
         }

         sprintf (filename, "%d:%d/%d.%d", fzo, fne, fno, fpo);
         sprintf (linea, " %-12.12s  %7ld  %-19.19s  %-24.24s", name, pos, area, filename);
         wputs (linea);

         prints (10, 65, YELLOW|_BLACK, "            ");
         sprintf (linea, "%d / %d", ++nrec, nsent);
         prints (10, 65, YELLOW|_BLACK, linea);

         rewind (fpcfg);
         found = 0;

         while (fread (&tsys.file_name, SIZEOF_FILEAREA, 1, fpcfg) == 1) {
            if (!stricmp (tsys.tic_tag, area)) {
               found = 1;
               break;
            }
         }

         if (!found) {
            if (config->tic_newareas_create[0]) {
               zo = config->alias[0].zone;
               ne = config->alias[0].net;
               no = config->alias[0].node;
               po = config->alias[0].point;

               strcpy (linea, config->tic_newareas_create);
               p = strtok (linea, " ");
               if (p != NULL)
                  do {
                     parse_netnode2 (p, &zo, &ne, &no, &po);
                     if (zo == fzo && ne == fne && no == fno && po == fpo) {
                        found = 1;
                        break;
                     }
                  } while ((p = strtok (NULL, " ")) != NULL);
            }

            if (!found)
               status_line ("!Unknown area \"%s\" in %s", area, blk.ff_name);
            else {
               i = tsys.file_num;

               memset ((char *)&tsys.file_name, 0, SIZEOF_FILEAREA);
               tsys.file_num = i + 1;
               tsys.file_priv = HIDDEN;
               tsys.download_priv = HIDDEN;
               tsys.upload_priv = HIDDEN;
               strcpy (tsys.tic_tag, area);
               strcpy (tsys.file_name, area);
               strcat (tsys.file_name, " (New area)");

               mkdir ("FNEWAREA");

               if (strlen (area) > 8)
                  area[8] = '\0';
               sprintf (filename, "FNEWAREA\\%s", area);

               if (mkdir (filename) == -1) {
                  i = 0;
                  do {
                     i++;
                     sprintf (filename, "FNEWAREA\\%s.%03d", area, i);
                  } while (mkdir (filename) == -1);
               }

               strcpy (area, tsys.tic_tag);

               getcwd (tsys.filepath, 25);
               strcat (tsys.filepath, "\\");
               strcat (tsys.filepath, filename);
               strcat (tsys.filepath, "\\");

               if (fpo)
                  sprintf (tsys.tic_forward1, "%d:%d/%d.%d ", fzo, fne, fno, fpo);
               else
                  sprintf (tsys.tic_forward1, "%d:%d/%d ", fzo, fne, fno);
               if (config->tic_newareas_link[0])
                  strcat (tsys.tic_forward1, config->tic_newareas_link);


               fseek (fpcfg, 0L, SEEK_END);
               fwrite (&tsys.file_name, SIZEOF_FILEAREA, 1, fpcfg);
            }
         }

         if (found) {
            found = 0;
            n_forw = 0;

            zo = config->alias[0].zone;
            ne = config->alias[0].net;
            no = po = 0;

            strcpy (linea, tsys.tic_forward1);
            if ((p = strtok (linea, " ")) != NULL)
               do {
                  forward[n_forw].passive = forward[n_forw].receive = forward[n_forw].send = 0;
                  if (*p == '<') {
                     forward[n_forw].send = 1;
                     p++;
                  }
                  if (*p == '>') {
                     forward[n_forw].receive = 1;
                     p++;
                  }
                  if (*p == '!') {
                     forward[n_forw].passive = 1;
                     p++;
                  }
                  parse_netnode2 (p, &zo, &ne, &no, &po);
                  if (!forward[n_forw].passive && !forward[n_forw].receive && zo == fzo && ne == fne && no == fno && po == fpo)
                     found = 1;
                  forward[n_forw].zone = zo;
                  forward[n_forw].net = ne;
                  forward[n_forw].node = no;
                  forward[n_forw].point = po;
                  n_forw++;
               } while ((p = strtok (NULL, " ")) != NULL);

            strcpy (linea, tsys.tic_forward2);
            if ((p = strtok (linea, " ")) != NULL)
               do {
                  forward[n_forw].passive = forward[n_forw].receive = forward[n_forw].send = 0;
                  if (*p == '<') {
                     forward[n_forw].send = 1;
                     p++;
                  }
                  if (*p == '>') {
                     forward[n_forw].receive = 1;
                     p++;
                  }
                  if (*p == '!') {
                     forward[n_forw].passive = 1;
                     p++;
                  }
                  parse_netnode2 (p, &zo, &ne, &no, &po);
                  if (!forward[n_forw].passive && !forward[n_forw].receive && zo == fzo && ne == fne && no == fno && po == fpo)
                     found = 1;
                  forward[n_forw].zone = zo;
                  forward[n_forw].net = ne;
                  forward[n_forw].node = no;
                  forward[n_forw].point = po;
                  n_forw++;
               } while ((p = strtok (NULL, " ")) != NULL);

            strcpy (linea, tsys.tic_forward3);
            if ((p = strtok (linea, " ")) != NULL)
               do {
                  forward[n_forw].passive = forward[n_forw].receive = forward[n_forw].send = 0;
                  if (*p == '<') {
                     forward[n_forw].send = 1;
                     p++;
                  }
                  if (*p == '>') {
                     forward[n_forw].receive = 1;
                     p++;
                  }
                  if (*p == '!') {
                     forward[n_forw].passive = 1;
                     p++;
                  }
                  parse_netnode2 (p, &zo, &ne, &no, &po);
                  if (!forward[n_forw].passive && !forward[n_forw].receive && zo == fzo && ne == fne && no == fno && po == fpo)
                     found = 1;
                  forward[n_forw].zone = zo;
                  forward[n_forw].net = ne;
                  forward[n_forw].node = no;
                  forward[n_forw].point = po;
                  n_forw++;
               } while ((p = strtok (NULL, " ")) != NULL);

            if (!found)
               status_line ("!Bad origin node %d:%d/%d.%d", fzo, fne, fno, fpo);
            else if (found == 1) {
               if (!get_bbs_record (fzo, fne, fno, fpo)) {
                  wputs ("Bad Node\n");
                  status_line ("!Node %d:%d/%d.%d not found", fzo, fne, fno, fpo);
                  fclose (fp);
                  rename_bad_tics (inpath, blk.ff_name);
                  continue;
               }

               if (nodelist.pw_tic[0] && stricmp (pw, nodelist.pw_tic)) {
                  wputs ("Bad Pwd\n");
                  status_line ("!Bad password from %d:%d/%d.%d", fzo, fne, fno, fpo);
                  fclose (fp);
                  rename_bad_tics (inpath, blk.ff_name);
                  continue;
               }

               strcpy (dpath, tsys.filepath);

               if (repl[0]) {
                  sprintf (filename, "%s%s", dpath, repl);
                  if (unlink (filename) == -1)
                     status_line ("*%s doesn't exist", filename);
                  else
                     status_line ("*Replaces: %s", repl);
               }

               status_line (" Moving %s%s to %s%s", inpath, name, dpath, name);

//               if (inpath[1] == ':' && dpath[1] == ':' && toupper (inpath[0]) == toupper (dpath[0])) {
//                  sprintf (linea, "%s%s", dpath, name);
//                  sprintf (filename, "%s%s", inpath, name);
//                  unlink (linea);
//                  rename (filename, linea);
//               }
//               else {
               sprintf (filename, "%s%s", dpath, name);
               fdd = open (filename, O_WRONLY|O_BINARY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE);

               sprintf (filename, "%s%s", inpath, name);
               fds = open (filename, O_RDONLY|O_BINARY);

               do {
                  i = read (fds, linea, MAX_BUFFERING);
                  if (write (fdd, linea, i) != i) {
                     found = 0;
                     status_line ("!Error during copy");
                     break;
                  }

                  t = ((float)filelength (fdd) * (float)100) / (float)filelength (fds);
                  sprintf (linea, "\b\b\b\b\b%04.1f%%", t);
                  wputs (linea);
                  time_release ();
               } while (i == MAX_BUFFERING);

               close (fdd);
               close (fds);

               if (!found) {
                  sprintf (filename, "%s%s", dpath, name);
                  unlink (filename);
               }
//               }

               if (found) {
                  unlink (filename);

                  status_line (":AREA: %s", area);
                  status_line (":%s", descr);
                  status_line (":Originated by %d:%d/%d.%d", ozo, one, ono, opo);

                  if (!stricmp (tsys.filepath, dpath) && tsys.filelist[0])
                     fpd = fopen (tsys.filelist, "ab");
                  else {
                     sprintf (filename, "%sFILES.BBS", dpath);
                     fpd = fopen (filename, "ab");
                  }
                  sprintf (filename, "%c  0%c ", config->dl_counter_limits[0], config->dl_counter_limits[1]);
                  fprintf (fpd, "%-12.12s %s%s\r\n", strupr (name), config->keep_dl_count ? filename : "", descr);
                  fclose (fpd);

                  crc = time (NULL);
                  tim = gmtime (&crc);

                  for (nf = 0; nf < n_forw; nf++) {
                     if (forward[nf].passive || forward[nf].send)
                        continue;

                     zo = forward[nf].zone;
                     ne = forward[nf].net;
                     no = forward[nf].node;
                     po = forward[nf].point;

                     if (zo == fzo && ne == fne && no == fno && po == fpo)
                        continue;

                     if (!get_bbs_record (zo, ne, no, po))
                        dpw[0] = '\0';
                     else
                        strcpy (dpw, nodelist.pw_tic);

                     rewind (fp);
                     seen = 0;

                     while (fgets (linea, 1020, fp) != NULL) {
                        while (strlen (linea) > 0 && (linea[strlen (linea) - 1] == 0x0D || linea[strlen (linea) - 1] == 0x0A))
                           linea[strlen (linea) - 1] = '\0';

                        p = strtok (linea, " ");
                        if (stricmp (p, "Seenby"))
                           continue;

                        p = strtok (NULL, " ");
                        ozo = config->alias[0].zone;
                        one = config->alias[0].net;
                        ono = opo = 0;
                        parse_netnode (p, &ozo, &one, &ono, &opo);

                        if (zo == ozo && ne == one && no == ono && po == opo) {
                           seen = 1;
                           break;
                        }
                     }

                     p = "Normal";

                     if (!seen && p != NULL) {
                        rewind (fp);

                        status_line ("+Sending %s to %d:%d/%d.%d %s", name, zo, ne, no, po, p);
                        if (po) {
                           sprintf (filename, "%s%04X%04X.PNT", HoldAreaNameMungeCreate (zo), ne, no);
                           mkdir (filename);
                        }

                        do {
                           pos = time (NULL) % 1000000L;
                           if (po)
                              sprintf (filename, "%s%04X%04X.PNT\\TK%06lu.TIC", HoldAreaNameMungeCreate (zo), ne, no, pos);
                           else
                              sprintf (filename, "%sTK%06lu.TIC", HoldAreaNameMungeCreate (zo), pos);
                        } while (dexists (filename));

                        fpd = fopen (filename, "wb");

                        seen = 0;
                        path = 0;

                        while (fgets (linea, 1020, fp) != NULL) {
                           while (strlen (linea) > 0 && (linea[strlen (linea) - 1] == 0x0D || linea[strlen (linea) - 1] == 0x0A))
                              linea[strlen (linea) - 1] = '\0';

                           if (path == 1 && strnicmp (linea, "Path ", 5)) {
                              fprintf (fpd, "Path %d:%d/%d %ld %s %s %02d %02d:%02d:%02d %d GMT\r\n", config->alias[0].zone, config->alias[0].net, config->alias[0].node, crc, wtext[tim->tm_wday], mtext[tim->tm_mon], tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec, tim->tm_year + 1900);
                              path = 2;
                           }

                           if (seen == 1 && strnicmp (linea, "Seenby ", 7)) {
                              for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++) {
                                 if (config->alias[i].point)
                                    fprintf (fpd, "Seenby %d:%d/%d.%d\r\n", config->alias[i].zone, config->alias[i].net, config->alias[i].node, config->alias[i].point);
                                 else
                                    fprintf (fpd, "Seenby %d:%d/%d\r\n", config->alias[i].zone, config->alias[i].net, config->alias[i].node);
                              }

                              if (po)
                                 fprintf (fpd, "Seenby %d:%d/%d.%d\r\n", zo, ne, no, po);
                              else
                                 fprintf (fpd, "Seenby %d:%d/%d\r\n", zo, ne, no);

                              seen = 2;
                           }

                           if (!strnicmp (linea, "From ", 5)) {
                              if ((i = nodelist.akainfo) > 0)
                                 i--;
                              fprintf (fpd, "From %d:%d/%d\r\n", config->alias[i].zone, config->alias[i].net, config->alias[i].node);
                           }
                           else if (!strnicmp (linea, "Created by ", 11))
                              fprintf (fpd, "Created by %s (C) Copyright Marco Maccaferri - 1989-94\r\n", VERSION);
                           else if (!strnicmp (linea, "Pw ", 3))
                              fprintf (fpd, "Pw %s\r\n", strupr (dpw));
                           else
                              fprintf (fpd, "%s\r\n", linea);

                           if (!strnicmp (linea, "Path ", 5))
                              path = 1;
                           else if (!strnicmp (linea, "Seenby ", 7))
                              seen = 1;
                        }

                        if (path == 0) {
                           if ((i = nodelist.akainfo) > 0)
                              i--;
                           if (config->alias[i].point)
                              fprintf (fpd, "Path %d:%d/%d.%d %ld %s %s %02d %02d:%02d:%02d %d GMT\r\n", config->alias[i].zone, config->alias[i].net, config->alias[i].node, config->alias[i].point, crc, wtext[tim->tm_wday], mtext[tim->tm_mon], tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec, tim->tm_year + 1900);
                           else
                              fprintf (fpd, "Path %d:%d/%d %ld %s %s %02d %02d:%02d:%02d %d GMT\r\n", config->alias[i].zone, config->alias[i].net, config->alias[i].node, crc, wtext[tim->tm_wday], mtext[tim->tm_mon], tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec, tim->tm_year + 1900);
                        }

                        if (seen == 0) {
                           for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++) {
                              if (config->alias[i].point)
                                 fprintf (fpd, "Seenby %d:%d/%d.%d\r\n", config->alias[i].zone, config->alias[i].net, config->alias[i].node, config->alias[i].point);
                              else
                                 fprintf (fpd, "Seenby %d:%d/%d\r\n", config->alias[i].zone, config->alias[i].net, config->alias[i].node);
                           }

                           if (po)
                              fprintf (fpd, "Seenby %d:%d/%d.%d\r\n", zo, ne, no, po);
                           else
                              fprintf (fpd, "Seenby %d:%d/%d\r\n", zo, ne, no);
                        }


                        fclose (fpd);

                        if (po)
                           sprintf (filename, "%s%04X%04X.PNT\\%08X.%cLO", HoldAreaNameMungeCreate (zo), ne, no, po, check_route_flag (zo, ne, no, po));
                        else
                           sprintf (filename, "%s%04X%04X.%cLO", HoldAreaNameMungeCreate (zo), ne, no, check_route_flag (zo, ne, no, 0));

                        fpd = fopen (filename, "at");
                        fprintf (fpd, "%s%s\n", dpath, name);
                        if (po)
                           fprintf (fpd, "^%s%04X%04X.PNT\\TK%06lu.TIC\n", HoldAreaNameMungeCreate (zo), ne, no, pos);
                        else
                           fprintf (fpd, "^%sTK%06lu.TIC\n", HoldAreaNameMungeCreate (zo), pos);
                        fclose (fpd);

                        prints (10, 65, YELLOW|_BLACK, "            ");
                        sprintf (linea, "%d / %d", nrec, ++nsent);
                        prints (10, 65, YELLOW|_BLACK, linea);
                     }
                  }
               }
            }
         }

         fclose (fp);

         if (found) {
            sprintf (filename, "%s%s", inpath, blk.ff_name);
            status_line ("+Deleting %s", filename);
            unlink (filename);
         }
         else
            rename_bad_tics (inpath, blk.ff_name);

         wputs ("\n");
      }
   }

   fclose (fpcfg);
   wr->srow--;
   wclose ();

   if (!fi)
      status_line (" No inbound TIC's to process");

   free (forward);
   idle_system ();
}

/*

                  if (stricmp (pw, dpw)) {
                     status_line ("!Bad password from %d:%d/%d.%d", fzo, fne, fno, fpo);
                     found = -1;
                  }
                  else {
                     p = strtok (NULL, " ");
                     if (strlen (p) > 9)
                        p[9] = '\0';
                     strcpy (dflag, p);
                     found = 1;
                  }

                  break;
               }
*/

static void rename_bad_tics (path, name)
char *path, *name;
{
   char filename[80], linea[80];

   name[8] = '\0';
   sprintf (filename, "%s%s.TIC", path, name);
   sprintf (linea, "%s%s.BAD", path, name);
   rename (filename, linea);
   name[8] = 'T';
}

//  2:332/402.0 Q 43 SYSOP_CHAT.ITA

void rescan_areas (void)
{
   FILE *fp;
   int fd, wh, zo, ne, no, po, board, adjtime;
   char linea[128], *p, *tag, type;
   struct _wrec_t *wr;
   struct date datep;
   struct time timep;
   struct _pkthdr2 pkthdr;

   if ((fp = fopen ("RESCAN.LOG", "rt")) == NULL)
      return;

   status_line ("+Rescan message base");
   local_status ("Rescan");
   scan_system ();

   wh = wopen (12, 0, 24, 79, 0, LGREY|_BLACK, LCYAN|_BLACK);
   wactiv (wh);
   wtitle ("RESCAN ECHOMAIL", TLEFT, LCYAN|_BLACK);
   wprints (0, 0, YELLOW|_BLACK, " Num.  Area tag               Base         Forward to");
   printc (12, 0, LGREY|_BLACK, 'Ã');
   printc (12, 52, LGREY|_BLACK, 'Á');
   printc (12, 79, LGREY|_BLACK, '´');
   wr = wfindrec (wh);
   wr->srow++;
   wr->row++;

   build_aidx ();
   totaltime = timerset (0);

   while (fgets (linea, 70, fp) != NULL) {
      while (strlen (linea) > 0 && (linea[strlen (linea) - 1] == 0x0D || linea[strlen (linea) - 1] == 0x0A))
         linea[strlen (linea) -1] = '\0';

      if ((p = strtok (linea, " ")) == NULL)
         continue;
      zo = config->alias[0].zone;
      ne = config->alias[0].net;
      no = po = 0;
      parse_netnode (p, &zo, &ne, &no, &po);

      memset ((char *)&sys, 0, sizeof (struct _sys));

      if ((p = strtok (NULL, " ")) == NULL)
         continue;
      type = *p;

      if ((p = strtok (NULL, " ")) == NULL)
         continue;
      if (type == 'S' || type == 'M')
         strcpy (sys.msg_path, p);
      else if (type == 'Q' || type == 'P')
         board = atoi (p);

      if ((tag = strtok (NULL, " ")) == NULL)
         continue;

      if (strlen (tag) > 31)
         tag[31] = '\0';
      strcpy (sys.echotag, strupr (tag));

      if (strlen (tag) > 14)
         tag[14] = '\0';
      prints (8, 65, YELLOW|_BLACK, tag);

      if (po)
         sprintf (linea, "%s%04X%04X.PNT\\%08X.OUT", HoldAreaNameMungeCreate (zo), ne, no, po);
      else
         sprintf (linea, "%s%04X%04X.OUT", HoldAreaNameMungeCreate (zo), ne, no);
      fd = open (linea, O_RDWR|O_BINARY);
      if (fd != -1) {
         if (filelength (fd) == 0L) {
            close (fd);
            unlink (linea);
            adjtime = 1;
         }
         else {
            close (fd);
            adjtime = 0;
         }
      }
      else
         adjtime = 1;

      if (type == 'Q')
         quick_rescan_echomail (board, zo, ne, no, po);
      else if (type == 'S') {
         sys.squish = 1;
         squish_scan_message_base (0, sys.msg_path, 0);
         if (sq_ptr != NULL) {
            while (MsgLock (sq_ptr) == -1)
               ;
            squish_rescan_echomail (sys.echotag, zo, ne, no, po);
         }
      }
      else if (type == 'M') {
         scan_message_base (0, 0);
         fido_rescan_echomail (sys.echotag, zo, ne, no, po);
      }
      else if (type == 'P')
         pip_rescan_echomail (board, sys.echotag, zo, ne, no, po);

      if (adjtime) {
         gettime (&timep);
         getdate (&datep);

         fd = open (linea, O_RDWR|O_BINARY);
         if (fd != -1) {
            if (filelength (fd) == 0L) {
               close (fd);
               unlink (linea);
            }
            else {
               read (fd, (char *)&pkthdr, sizeof (struct _pkthdr2));
               pkthdr.hour = timep.ti_hour;
               pkthdr.minute = timep.ti_min;
               pkthdr.second = timep.ti_sec;
               pkthdr.year = datep.da_year;
               pkthdr.month = datep.da_mon - 1;
               pkthdr.day = datep.da_day;
               lseek (fd, 0L, SEEK_SET);
               write (fd, (char *)&pkthdr, sizeof (struct _pkthdr2));
               close (fd);
            }
         }
      }
   }

   if (sq_ptr != NULL) {
      MsgUnlock (sq_ptr);
      MsgCloseArea (sq_ptr);
      sq_ptr = NULL;
   }

   if (aidx != NULL)
      free (aidx);
   if (akainfo != NULL)
      free (akainfo);

   fclose (fp);
   wclose ();

   memset ((char *)&sys, 0, sizeof (struct _sys));
   unlink ("RESCAN.LOG");
}

struct _msgzone {
   short dest_zone;
   short orig_zone;
   short dest_point;
   short orig_point;
};

void track_inbound_messages (FILE *fpd, struct _msg *msgt, int fzone)
{
   if (!(msgt->attr & MSGLOCAL)) {
      if (!get_bbs_record (fzone, msgt->orig_net, msgt->orig, 0)) {
         mprintf (fpd, "==========================================================================\r\n");
         mprintf (fpd, "WARNING [%s]: The originating address on this message was\r\n", VERSION);
         mprintf (fpd, "=not= listed in the NodeList at %d:%d/%d.  Please ensure that you do\r\n", config->alias[0].zone, config->alias[0].net, config->alias[0].node);
         mprintf (fpd, "not send a reply via this system, as it will be bounced as undeliverable.\r\n");
         mprintf (fpd, "==========================================================================\r\n");
      }
   }
}

int track_outbound_messages (FILE *fpd, struct _msg *msgt, int fzone, int fpoint, int tzone, int tpoint)
{
   FILE *fp;
   int i, aka, m;
   char filename[80], buffer[2050];
   struct _msg tmsg;
   struct _msgzone mz;

   if (fzone == 0)
      fzone = config->alias[0].zone;
   if (tzone == 0)
      tzone = config->alias[0].zone;

   if (get_bbs_record (tzone, msgt->dest_net, msgt->dest, 0))
      return (1);

   for (i = 0;i < MAX_ALIAS && config->alias[i].net; i++)
      if (config->alias[i].zone == fzone && config->alias[i].net == msgt->orig_net && config->alias[i].node == msgt->orig)
         break;
   if (i < MAX_ALIAS && config->alias[i].net)
      return (1);

   if (!get_bbs_record (fzone, msgt->orig_net, msgt->orig, 0))
      return (0);

   for (i = 0;i < MAX_ALIAS && config->alias[i].net; i++)
      if (config->alias[i].zone == fzone)
         break;
   if (i < MAX_ALIAS && config->alias[i].net)
      aka = i;
   else
      aka = 0;

   memset (&tmsg, 0, sizeof (struct _msg));
   sprintf (filename, "%s%d.MSG", sys.msg_path, ++last_msg);

   fp = fopen (filename, "wb");
   if (fp == NULL)
        return (0);

   tmsg.attr |= MSGPRIVATE;

   if (get_bbs_record (fzone, msgt->orig_net, msgt->orig, 0)) {
      mz.dest_zone = fzone;
      tmsg.dest_net = msgt->orig_net;
      tmsg.dest = msgt->orig;
      mz.dest_point = fpoint;
   }
   else {
      mz.dest_zone = config->alias[0].zone;
      tmsg.dest_net = config->alias[0].net;
      tmsg.dest = config->alias[0].node;
      mz.dest_point = config->alias[0].point;
   }

   mz.orig_zone = config->alias[aka].zone;
   tmsg.orig_net = config->alias[aka].net;
   tmsg.orig = config->alias[aka].node;
   mz.orig_point = config->alias[aka].point;
   memcpy (&tmsg.date_written, &mz, sizeof (struct _msgzone));

   strcpy (tmsg.from, VERSION);
   strcpy (tmsg.to, msgt->from);
   sprintf (tmsg.subj, "Netmail bounced by %d:%d/%d", config->alias[aka].zone, config->alias[aka].net, config->alias[aka].node);
   data (tmsg.date);

   if (fwrite ((char *)&tmsg, sizeof(struct _msg), 1, fp) != 1)
      return (0);

   if (config->alias[aka].point)
      fprintf (fp, "\001FMPT %d\r\n", config->alias[aka].point);
   if (fpoint)
      fprintf (fp, msgtxt[M_TOPT], fpoint);
   if (mz.dest_zone != config->alias[0].zone || mz.dest_zone != mz.orig_zone)
      fprintf (fp, msgtxt[M_INTL], mz.dest_zone, tmsg.dest_net, tmsg.dest, mz.orig_zone, tmsg.orig_net, tmsg.orig);
   fprintf (fp, msgtxt[M_MSGID], mz.orig_zone, tmsg.orig_net, tmsg.orig, mz.orig_point, time (NULL));

   fprintf (fp, "The destination address on the following message was =not= listed in the\r\n");
   fprintf (fp, "NodeList at %d:%d/%d. The message is, therefore, being returned as\r\n", config->alias[aka].zone, config->alias[aka].net, config->alias[aka].node);
   fprintf (fp, "undeliverable.  Sorry for any inconvenience.\r\n\r\n");

   fprintf (fp, "%s - Sysop, %d:%d/%d\r\n\r\n", config->sysop, config->alias[aka].zone, config->alias[aka].net, config->alias[aka].node);

   fprintf (fp, "--------------  b o u n c e d   m e s s a g e   f o l l o w s  --------------\r\n\r\n");

   fprintf (fp, "Date:  %s\r\n", msgt->date);
   fprintf (fp, "From:  %s (%d:%d/%d.%d)\r\n", msgt->from, fzone, msgt->orig_net, msgt->orig, fpoint);
   fprintf (fp, "To:    %s (%d:%d/%d.%d)\r\n", msgt->to, tzone, msgt->dest_net, msgt->dest, tpoint);
   fprintf (fp, "Subj:  %s\r\n\r\n", msgt->subj);

   mseek (fpd, 0L, SEEK_SET);
   do {
      i = mread (buffer, 1, 2048, fpd);
      buffer[2048] = '\0';
      for (m = 0; m < i; m++) {
         if (buffer[m] == 0x1A)
            buffer[m] = ' ';
         if (buffer[m] == 0x01)
            buffer[m] = '@';
      }
      if (strstr (buffer, "--- ") != NULL) {
         strsrep (buffer, "\n--- ", "\n-+- ");
         strsrep (buffer, "\r--- ", "\r-+- ");
      }
      if (fwrite (buffer, 1, i, fp) != i)
         return (0);
   } while (i == 2048);

   fprintf (fp, "\r\n---------------  e n d   o f   b o u n c e d   m e s s a g e  ---------------\r\n\r\n");
   put_tearline (fp);

   fputc ('\0', fp);
   fclose (fp);

   return (0);
}

