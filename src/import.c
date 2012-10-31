#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <dir.h>
#include <dos.h>
#include <string.h>
#include <ctype.h>
#include <process.h>
#include <alloc.h>
#include <time.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "defines.h"
#include "lora.h"
#include "prototyp.h"
#include "externs.h"
#include "zmodem.h"
#include "msgapi.h"
#include "exec.h"

#define isMAX_PRODUCT    0xE2

struct _fwd_alias {
   int zone;
   int net;
   int node;
   int point;
};

#define MAXDUPES      1000
#define DUPE_HEADER   (48 + 2 + 2 + 4)
#define DUPE_FOOTER   (MAXDUPES * 4)
#define MAX_SEEN      10
#define MAX_PATH      128
#define MAX_FORWARD   128
#define MAX_DUPEINDEX 40

struct _dupecheck {
   char areatag[48];
   int  dupe_pos;
   int  max_dupes;
   long area_pos;
   long dupes[MAXDUPES];
};

struct _dupeindex {
   char areatag[48];
   long area_pos;
};

extern char *suffixes[];

void pack_outbound (void);

static FILE *f1, *f2, *f3, *f4, *f5;
struct _dupecheck *dupecheck;
static int ndupes, nbad, pkt_zone, pkt_net, pkt_node, pkt_point;

static void rename_bad_packet (char *, char *);
static void process_type2_packet (FILE *, struct _pkthdr2 *);
static void process_type2plus_packet (FILE *, struct _pkthdr2 *);
static void process_packet (FILE *);
static char *get_to_null (FILE *fp, char *, int);
static char *packet_fgets (char *, int, FILE *fp);
static int read_dupe_check (char *);
static void write_dupe_check (void);
static int  check_and_add_dupe (long);
static long compute_crc (char *, long);
static int isbundle (char *);
static void fido_save_message2 (FILE *, char *);
static void passthrough_save_message (FILE *);
static void unpack_arcmail (char *);
static void call_packer (char *, char *, char *, int, int, int);
static char get_flag (void);

/*---------------------------------------------------------------------------
   void import_mail (char *dir, int *ext_flag)

   Shell da richiamare per importare l'echomail e la netmail dalle
   directory di inbound. Il parametro dir specifica la directory da cui
   prelevare i pkt e gli arcmail da scompattare. Il parametro ext_flag
   punta ad una variabile che indica se deve ancora essere eseguito il
   BEFORE_IMPORT (1) oppure se deve essere eseguito anche il AFTER_IMPORT
   (2).
---------------------------------------------------------------------------*/

void import_mail (dir, ext_flag)
char *dir;
int *ext_flag;
{
   FILE *fp;
   int wh, *varr, i;
   char filename[80], *p, pktname[14], curdir[52];
   unsigned int pktdate, pkttime;
   struct ffblk blk;
   struct _pkthdr2 pkthdr;

   wh = whandle ();
   if (wh != mainview && wh != status) {
      wclose ();
      wunlink (wh);
   }
   wactiv (mainview);

   // Scompatta tutti i pacchetti arcmail presenti nella directory indicata.
   unpack_arcmail (dir);

   // Se non ci sono PKT non ritorna immediatamente.
   sprintf (filename, "%s*.PKT", dir);
   if (findfirst (filename, &blk, 0))
      return;

   status_line (":IMPORT");

   // Il comando DOS before import viene eseguito solo se e' la prima volta,
   // come indicato dalla variabile ext_flag passata come puntatore.
   if (*ext_flag == 1) {
      *ext_flag = 0;

      if (pre_import != NULL) {
         getcwd (curdir, 49);
         status_line ("+Running BEFORE_IMPORT: %s", pre_import);
         varr = ssave ();
         cclrscrn(LGREY|_BLACK);
         if ((p = strchr (pre_import, ' ')) != NULL)
            *p++ = '\0';
         do_exec (pre_import, p, USE_ALL|HIDE_FILE, 0xFFFF, NULL);
         if (p != NULL)
            *(--p) = ' ';
         if (varr != NULL)
            srestore (varr);
         setdisk (curdir[0] - 'A');
         chdir (curdir);
      }
   }

   free (Txbuf);
   f1 = f2 = f3 = f4 = f5 = NULL;

   dupecheck = (struct _dupecheck *)malloc (sizeof (struct _dupecheck));
   if (dupecheck == NULL) {
      Txbuf = Secbuf = (byte *) malloc (WAZOOMAX + 16);
      if (Txbuf == NULL)
         exit (250);
      return;
   }

   wh = wopen (7, 10, 20, 70, 1, LCYAN|_BLUE, LCYAN|_BLUE);
   wactiv (wh);

   memset ((char *)&sys, 0, sizeof (struct _sys));
   if (dupes != NULL) {
      strcpy (sys.msg_path, dupes);
      scan_message_base (0, 0);
      ndupes = last_msg;
   }
   if (bad_msgs != NULL) {
      strcpy (sys.msg_path, bad_msgs);
      scan_message_base (0, 0);
      nbad = last_msg;
   }

   for (;;) {
      sprintf (filename, "%s*.PKT", dir);
      if (findfirst (filename, &blk, 0))
         break;

      strcpy (pktname, blk.ff_name);
      pktdate = blk.ff_fdate;
      pkttime = blk.ff_ftime;

      // Routinetta per trovare il .pkt piu' vecchio tra quelli che sono
      // stati ricevuti, in modo da estrarre i messaggi sempre nell'ordine
      // giusto.
      while (!findnext (&blk)) {
         if (pktdate > blk.ff_fdate || (pktdate == blk.ff_fdate && pkttime > blk.ff_ftime)) {
            strcpy (pktname, blk.ff_name);
            pktdate = blk.ff_fdate;
            pkttime = blk.ff_ftime;
         }
      }

      strcpy (blk.ff_name, pktname);

      sprintf (filename, " Processing %s ", blk.ff_name);
      wtitle (filename, TLEFT, LCYAN|_BLUE);

      time_release ();

      sprintf (filename, "%s%s", dir, blk.ff_name);
      fp = fopen (filename, "rb");
      if (fp == NULL)
         continue;
      setvbuf (fp, NULL, _IOFBF, 2048);
      fread ((char *)&pkthdr, sizeof (struct _pkthdr2), 1, fp);

      if (pkthdr.ver != PKTVER) {
         fclose (fp);
         rename_bad_packet (filename, dir);
      }
      else {
         swab ((char *)&pkthdr.cwvalidation, (char *)&i, 2);
         pkthdr.cwvalidation = i;
         if (pkthdr.capability != pkthdr.cwvalidation || !(pkthdr.capability & 0x0001)) {
            if ( (byte)pkthdr.product <= isMAX_PRODUCT)
               status_line ("=Pkt 2 - %s - %s", blk.ff_name, prodcode[ (byte)pkthdr.product]);
            else
               status_line ("=Pkt 2 - %s - %s", blk.ff_name, "Unknow");
            process_type2_packet (fp, &pkthdr);
         }
         else {
            i = pkthdr.producth * 256 + pkthdr.product;
            if (i <= isMAX_PRODUCT)
               status_line ("=Pkt 2+ - %s - %s", blk.ff_name, prodcode[i]);
            else
               status_line ("=Pkt 2+ - %s - %s", blk.ff_name, "Unknow");
            process_type2plus_packet (fp, &pkthdr);
         }

         fclose (fp);
      }

      unlink (filename);
   }

   if (sq_ptr != NULL) {
      MsgCloseArea (sq_ptr);
      sq_ptr = NULL;
   }

   wclose ();
   wunlink (wh);

   free (dupecheck);
   unlink ("MSGTMP.IMP");

   memset ((char *)&sys, 0, sizeof (struct _sys));

   Txbuf = Secbuf = (byte *) malloc (WAZOOMAX + 16);
   if (Txbuf == NULL)
      exit (250);

   if (*ext_flag == 2 && after_import != NULL) {
      getcwd (curdir, 49);
      status_line ("+Running AFTER_IMPORT: %s", after_import);
      *ext_flag = 0;
      varr = ssave ();
      cclrscrn(LGREY|_BLACK);
      if ((p = strchr (after_import, ' ')) != NULL)
         *p++ = '\0';
      do_exec (after_import, p, USE_ALL|HIDE_FILE, 0xFFFF, NULL);
      if (p != NULL)
         *(--p) = ' ';
      if (varr != NULL)
         srestore (varr);
      setdisk (curdir[0] - 'A');
      chdir (curdir);
   }
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static void rename_bad_packet (name, dir)
char *name, *dir;
{
   int i = -1;
   char filename[128], *p;
   struct ffblk blk;

   sprintf (filename, "%sBAD_PKT.*", dir);
   if (!findfirst (filename, &blk, 0))
      do {
         if ((p = strchr (blk.ff_name, '.')) != NULL) {
            p++;
            if (atoi (p) > i)
               i = atoi (p);
         }
      } while (!findnext (&blk));

   i++;
   sprintf (filename, "%sBAD_PKT.%03d", dir, i);
   rename (name, filename);
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static void process_type2_packet (fp, pkthdr)
FILE *fp;
struct _pkthdr2 *pkthdr;
{
   memset ((char *)&msg, 0, sizeof (struct _msg));
   msg.dest_net = pkthdr->dest_net;
   msg.dest = pkthdr->dest_node;
   msg_tzone = pkthdr->dest_zone;
   msg_tpoint = 0;
   pkt_net = msg.orig_net = pkthdr->orig_net;
   pkt_node = msg.orig = pkthdr->orig_node;
   pkt_zone = msg_fzone = pkthdr->orig_zone;
   pkt_point = msg_fpoint = 0;

   status_line ("=Orig:%d:%d/%d.%d Dest:%d:%d/%d.%d", msg_fzone, msg.orig_net, msg.orig, msg_fpoint, msg_tzone, msg.dest_net, msg.dest, msg_tpoint);

   process_packet (fp);
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static void process_type2plus_packet (fp, pkthdr)
FILE *fp;
struct _pkthdr2 *pkthdr;
{
   memset ((char *)&msg, 0, sizeof (struct _msg));

   msg.dest_net = pkthdr->dest_net;
   msg.dest = pkthdr->dest_node;
   msg_tzone = pkthdr->dest_zone;
   msg_tpoint = pkthdr->dest_point;

   pkt_net = msg.orig_net = pkthdr->orig_net;
   pkt_node = msg.orig = pkthdr->orig_node;
   pkt_zone = msg_fzone = pkthdr->orig_zone;
   pkt_point = msg_fpoint = pkthdr->orig_point;

   status_line ("=Orig:%d:%d/%d.%d Dest:%d:%d/%d.%d", msg_fzone, msg.orig_net, msg.orig, msg_fpoint, msg_tzone, msg.dest_net, msg.dest, msg_tpoint);

   process_packet (fp);
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static int sort_func (const void *a1, const void *b1)
{
   struct _fwd_alias *a, *b;
   a = (struct _fwd_alias *)a1;
   b = (struct _fwd_alias *)b1;
   if (a->zone != b->zone)   return (a->zone - b->zone);
   if (a->net != b->net)   return (a->net - b->net);
   if (a->node != b->node)   return (a->node - b->node);
   return ( (int)(a->point - b->point) );
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static void process_packet (fp)
FILE *fp;
{
   FILE *fpd;
   int cpoint, cnet, cnode, m, i, n_path, n_forw, nmsg, npkt;
   char linea[90], last_path[90], addr[40], *p, dupe, bad;
   long crc;
   struct _msghdr2 msghdr;
   struct _fwd_alias seen[MAX_SEEN];
   struct _fwd_alias *path, *forward;

   path = (struct _fwd_alias *)malloc (MAX_PATH * sizeof (struct _fwd_alias));
   if (path == NULL)
      return;
   forward = (struct _fwd_alias *)malloc (MAX_FORWARD * sizeof (struct _fwd_alias));
   if (forward == NULL) {
      free (path);
      return;
   }

   memset ((char *)dupecheck, 0, sizeof (struct _dupecheck));
   memset ((char *)&sys, 0, sizeof (struct _sys));

   fpd = fopen ("MSGTMP.IMP", "rb+");
   if (fpd == NULL) {
      fpd = fopen ("MSGTMP.IMP", "wb");
      fclose (fpd);
      fpd = fopen ("MSGTMP.IMP", "rb+");
   }

   setvbuf (fpd, NULL, _IOFBF, 8192);

   npkt = nmsg = 0;
   wclear ();

   while (fread ((char *)&msghdr, sizeof (struct _msghdr2), 1, fp) != 0) {
      if (msghdr.ver == 0)
         break;

      if (msghdr.ver != PKTVER)
         get_to_null (fp, linea, 20);
      else {
         bad = dupe = 0;

         if (nmsg)
            wputs ("\n");
         sprintf (addr, "%5d -> ", ++nmsg);
         wputs (addr);

         msg.orig_net = pkt_net;
         msg.orig = pkt_node;
         msg_fzone = pkt_zone;
         msg_fpoint = pkt_point;

         memset ((char *)&msg.date, 0, 20);
         strcpy (msg.date, get_to_null (fp, linea, 20));
         memset ((char *)&msg.to, 0, 36);
         strcpy (msg.to, get_to_null (fp, linea, 36));
         memset ((char *)&msg.from, 0, 36);
         strcpy (msg.from, get_to_null (fp, linea, 36));
         memset ((char *)&msg.subj, 0, 72);
         strcpy (msg.subj, get_to_null (fp, linea, 72));
         msg.attr = msghdr.attrib;
         msg.cost = msghdr.cost;

         crc = compute_crc (msg.date, 0xFFFFFFFFL);
         crc = compute_crc (msg.to, crc);
         crc = compute_crc (msg.from, crc);
         crc = compute_crc (msg.subj, crc);

         packet_fgets (linea, 79, fp);
         if (!strncmp (linea, "AREA:", 5)) {
            sys.netmail = 0;
            sys.echomail = 1;
            while (linea[strlen (linea) -1] == 0x0D || linea[strlen (linea) -1] == 0x0A)
               linea[strlen (linea) -1] = '\0';
            if (stricmp (&linea[5], dupecheck->areatag)) {
               if (dupecheck->areatag[0]) {
                  if (npkt && strlen (sys.echotag))
                     status_line ("+%s with %d msg(s)", sys.echotag, npkt);
                  npkt = 0;
                  write_dupe_check ();
               }
               if (!read_dupe_check (&linea[5]))
                  bad = 1;

               n_forw = 0;
               forward[n_forw].zone = alias[sys.use_alias].zone;
               forward[n_forw].net = alias[sys.use_alias].net;
               forward[n_forw].node = alias[sys.use_alias].node;
               forward[n_forw].point = alias[sys.use_alias].point;

               p = strtok (sys.forward1, " ");
               if (p != NULL)
                  do {
                     if (n_forw) {
                        forward[n_forw].zone = forward[n_forw - 1].zone;
                        forward[n_forw].net = forward[n_forw - 1].net;
                        forward[n_forw].node = forward[n_forw - 1].node;
                        forward[n_forw].point = forward[n_forw - 1].point;
                     }
                     parse_netnode2 (p, &forward[n_forw].zone, &forward[n_forw].net, &forward[n_forw].node, &forward[n_forw].point);
                     n_forw++;
                  } while ((p = strtok (NULL, " ")) != NULL);
               p = strtok (sys.forward2, " ");
               if (p != NULL)
                  do {
                     if (n_forw) {
                        forward[n_forw].zone = forward[n_forw - 1].zone;
                        forward[n_forw].net = forward[n_forw - 1].net;
                        forward[n_forw].node = forward[n_forw - 1].node;
                        forward[n_forw].point = forward[n_forw - 1].point;
                     }
                     parse_netnode2 (p, &forward[n_forw].zone, &forward[n_forw].net, &forward[n_forw].node, &forward[n_forw].point);
                     n_forw++;
                  } while ((p = strtok (NULL, " ")) != NULL);
               p = strtok (sys.forward3, " ");
               if (p != NULL)
                  do {
                     if (n_forw) {
                        forward[n_forw].zone = forward[n_forw - 1].zone;
                        forward[n_forw].net = forward[n_forw - 1].net;
                        forward[n_forw].node = forward[n_forw - 1].node;
                        forward[n_forw].point = forward[n_forw - 1].point;
                     }
                     parse_netnode2 (p, &forward[n_forw].zone, &forward[n_forw].net, &forward[n_forw].node, &forward[n_forw].point);
                     n_forw++;
                  } while ((p = strtok (NULL, " ")) != NULL);
            }

            for (i = 0; i < n_forw; i++) {
               if (forward[i].zone == msg_fzone && forward[i].net == msghdr.orig_net && forward[i].node == msghdr.orig_node && forward[i].point == msg_fpoint)
                  break;
            }

            if (i == n_forw && noask.secure) {
               sprintf (addr, "%-5d %-20.20s ", nbad + 1, "(Bad origin node)");
               wputs (addr);
               bad = 1;
            }

            if (!bad) {
               crc = compute_crc (sys.echotag, crc);

               if (check_and_add_dupe (crc)) {
                  sprintf (addr, "%-5d %-20.20s ", ndupes + 1, "(Duplicate message)");
                  wputs (addr);
                  dupe = 1;
               }
               else {
                  sprintf (addr, "%-5d %-20.20s ", last_msg + 1, sys.echotag);
                  wputs (addr);
                  dupe = 0;
               }
            }

            linea[0] = '\0';
         }
         else {
            sys.netmail = 1;
            sys.echomail = 0;
            if (stricmp (sys.msg_name, "Netmail")) {
               memset ((char *)&sys, 0, sizeof (struct _sys));
               strcpy (sys.msg_name, "Netmail");
               strcpy (sys.msg_path, netmail_dir);
               sys.netmail = 1;
               scan_message_base (0, 0);
               if (stricmp ("Netmail", dupecheck->areatag)) {
                  if (dupecheck->areatag[0]) {
                     if (npkt && strlen (sys.echotag))
                        status_line ("+%s with %d msg(s)", sys.echotag, npkt);
                     npkt = 0;
                     write_dupe_check ();
                  }
                  read_dupe_check ("Netmail");
               }
            }

            crc = compute_crc (sys.msg_name, crc);

            if (check_and_add_dupe (crc)) {
               sprintf (addr, "%-5d %-20.20s ", ndupes + 1, "(Duplicate message)");
               wputs (addr);
               dupe = 1;
            }
            else {
               sprintf (addr, "%-5d %-20.20s ", last_msg + 1, sys.msg_name);
               wputs (addr);
               dupe = 0;
            }

            msg.orig_net = msghdr.orig_net;
            msg.orig = msghdr.orig_node;
            msg.dest_net = msghdr.dest_net;
            msg.dest = msghdr.dest_node;
            msg_fzone = msg_tzone = alias[sys.use_alias].zone;
            msg_tpoint = msg_fpoint = 0;

            status_line ("*Netmail: %s -> %s", msg.from, msg.to);
            status_line ("*Orig:%d:%d/%d.%d Dest:%d:%d/%d.%d", msg_fzone, msg.orig_net, msg.orig, msg_fpoint, msg_tzone, msg.dest_net, msg.dest, msg_tpoint);
         }

         fseek (fpd, 0L, SEEK_SET);
         chsize (fileno (fpd), 0L);

         last_path[0] = '\0';
         if (linea[0])
            fprintf (fpd, "%s", linea);

         n_path = 0;

         while (packet_fgets (linea, 79, fp) != NULL) {
            if (linea[0] == '\0')
               break;
            linea[79] = '\0';

            if (sys.netmail) {
               if (!strncmp (linea, "\001MSGID:", 7)) {
                  parse_netnode (&linea[8], &msg_fzone, &msg.orig_net, &msg.orig, &msg_fpoint);
               }
               else if (!strncmp (linea, "\001TOPT", 5))
                  msg_tpoint = atoi (&linea[6]);
               else if (!strncmp (linea, "\001FMPT", 5))
                  msg_fpoint = atoi (&linea[6]);
               else if (!strncmp (linea, "\001INTL", 5)) {
                  strcpy (addr, &linea[6]);
                  p = strtok (addr, " ");
                  parse_netnode (p, &msg_tzone, &msg.dest_net, &msg.dest, &i);
                  p = strtok (NULL, " ");
                  parse_netnode (p, &msg_fzone, &msg.orig_net, &msg.orig, &i);
               }
            }

            if (strncmp (linea, "\001PATH: ", 7) && strncmp (linea, "SEEN-BY: ", 9))
               fprintf (fpd, "%s", linea);
            if (!strncmp (linea, "\001PATH: ", 7) && !sys.netmail) {
               if (!last_path[0]) {
                  strcpy (last_path, linea);
                  if (alias[sys.use_alias].point && alias[sys.use_alias].fakenet) {
                     seen[0].zone = alias[sys.use_alias].zone;
                     seen[0].net = alias[sys.use_alias].fakenet;
                     seen[0].node = alias[sys.use_alias].point;
                     seen[0].point = 0;
                  }
                  else {
                     seen[0].zone = alias[sys.use_alias].zone;
                     seen[0].net = alias[sys.use_alias].net;
                     seen[0].node = alias[sys.use_alias].node;
                     seen[0].point = alias[sys.use_alias].point;
                  }

                  seen[1].zone = msg_fzone;
                  seen[1].net = msg.orig_net;
                  seen[1].node = msg.orig;
                  seen[1].point = msg_fpoint;

                  qsort (&seen, 2, sizeof (struct _fwd_alias), sort_func);

                  cpoint = cnet = cnode = 0;
                  strcpy (linea, "SEEN-BY: ");

                  for (m = 0; m < 2; m++) {
                     if (cnet != seen[m].net && cnode != seen[m].node && cpoint != seen[m].point) {
                        if (!seen[m].point)
                           sprintf (addr, "%d/%d ", seen[m].net, seen[m].node);
                        else
                           sprintf (addr, "%d/%d.%d ", seen[m].net, seen[m].node, seen[m].point);
                        cnet = seen[m].net;
                        cnode = seen[m].node;
                        cpoint = seen[m].point;
                     }
                     else if (cpoint == seen[m].point && cnet != seen[m].net && cnode != seen[m].node) {
                        sprintf (addr, "%d/%d ", seen[m].net, seen[m].node);
                        cnet = seen[m].net;
                        cnode = seen[m].node;
                     }
                     else if (cpoint == seen[m].point && cnet == seen[m].net && cnode != seen[m].node) {
                        sprintf (addr, "%d ", seen[m].node);
                        cnode = seen[m].node;
                     }
                     else if (cpoint != seen[m].point && cnet == seen[m].net && cnode == seen[m].node) {
                        if (!seen[m].point)
                           sprintf (addr, "%d ", seen[m].node);
                        else
                           sprintf (addr, ".%d ", seen[m].point);
                        cpoint = seen[m].point;
                     }
                     else
                        strcpy (addr, "");

                     strcat (linea, addr);
                  }

                  fprintf (fpd, "%s\r\n", linea);
                  strcpy (linea, last_path);
               }

               path[n_path].zone = alias[sys.use_alias].zone;
               if (alias[sys.use_alias].point && alias[sys.use_alias].fakenet) {
                  path[n_path].net = alias[sys.use_alias].fakenet;
                  path[n_path].node = alias[sys.use_alias].point;
                  path[n_path].point = 0;
               }
               else {
                  path[n_path].net = alias[sys.use_alias].net;
                  path[n_path].node = alias[sys.use_alias].node;
                  path[n_path].point = alias[sys.use_alias].point;
               }

               while (linea[strlen (linea) -1] == 0x0D || linea[strlen (linea) -1] == 0x0A || linea[strlen (linea) -1] == ' ')
                  linea[strlen (linea) -1] = '\0';
               p = strtok (linea, " ");
               while ((p = strtok (NULL, " ")) != NULL) {
                  if (n_path)
                     path[n_path].net = path[n_path - 1].net;
                  path[n_path].zone = alias[sys.use_alias].zone;
                  parse_netnode2 (p, &path[n_path].zone, &path[n_path].net, &path[n_path].node, &path[n_path].point);
                  n_path++;
               }
            }
         }

         if (!sys.netmail) {
            path[n_path].zone = alias[sys.use_alias].zone;
            if (alias[sys.use_alias].point && alias[sys.use_alias].fakenet) {
               path[n_path].net = alias[sys.use_alias].fakenet;
               path[n_path].node = alias[sys.use_alias].point;
               path[n_path].point = 0;
            }
            else {
               path[n_path].net = alias[sys.use_alias].net;
               path[n_path].node = alias[sys.use_alias].node;
               path[n_path].point = alias[sys.use_alias].point;
            }
            n_path++;

            i = 0;
            do {
               cpoint = cnet = cnode = 0;
               strcpy (linea, "\001PATH: ");
               do {
                  if (cnet != path[i].net && cnode != path[i].node && cpoint != path[i].point) {
                     if (!path[i].point)
                        sprintf (addr, "%d/%d ", path[i].net, path[i].node);
                     else
                        sprintf (addr, "%d/%d.%d ", path[i].net, path[i].node, path[i].point);
                     cnet = path[i].net;
                     cnode = path[i].node;
                     cpoint = path[i].point;
                  }
                  else if (cpoint == path[i].point && cnet != path[i].net && cnode != path[i].node) {
                     sprintf (addr, "%d/%d ", path[i].net, path[i].node);
                     cnet = path[i].net;
                     cnode = path[i].node;
                  }
                  else if (cpoint == path[i].point && cnet == path[i].net && cnode != path[i].node) {
                     sprintf (addr, "%d ", path[i].node);
                     cnode = path[i].node;
                  }
                  else if (cpoint != path[i].point && cnet == path[i].net && cnode == path[i].node) {
                     if (!path[i].point)
                        sprintf (addr, "%d/%d ", path[i].net, path[i].node);
                     else
                        sprintf (addr, ".%d ", path[i].point);
                     cpoint = path[i].point;
                  }
                  else
                     strcpy (addr, "");

                  strcat (linea, addr);
                  i++;
               } while (i < n_path && strlen (linea) < 70);

               fprintf (fpd, "%s\r\n", linea);
            } while (i < n_path);
         }

         if (sys.netmail) {
            crc = time (NULL);
            sprintf (linea, "\x01Via %s on %d:%d/%d.%d, %s", VERSION, alias[sys.use_alias].zone, alias[sys.use_alias].net, alias[sys.use_alias].node, alias[sys.use_alias].point, ctime (&crc));
            while (linea[strlen (linea) -1] == 0x0D || linea[strlen (linea) -1] == 0x0A)
               linea[strlen (linea) -1] = '\0';
            fprintf (fpd, "%s\r\n", linea);
         }

         fseek (fpd, 0L, SEEK_SET);

         if (sys.echomail && n_forw == 1)
            msg.attr |= MSGSENT;

         if (sys.netmail) {
            for (i = 0; alias[i].net; i++) {
               if (alias[i].point && alias[i].fakenet) {
                  if (alias[i].zone == msg_tzone && alias[i].fakenet == msg.dest_net && alias[i].point == msg.dest)
                     break;
               }
               if (alias[i].zone == msg_tzone && alias[i].net == msg.dest_net && alias[i].node == msg.dest && alias[i].point == msg_tpoint)
                  break;
            }
            if (!alias[i].net) {
               msg.attr |= MSGFWD;
               if (!noask.keeptransit)
                  msg.attr |= MSGKILL;
               msg.attr &= ~(MSGCRASH|MSGSENT);
            }
         }

         if (dupe) {
            if (dupes != NULL) {
               wputs ("Fido      ");
               fido_save_message2 (fpd, dupes);
            }
         }

         else if (bad) {
            if (bad_msgs != NULL) {
               wputs ("Fido      ");
               fido_save_message2 (fpd, bad_msgs);
            }
         }

         else if (sys.passthrough) {
            npkt++;
            wputs ("Passthrough");
            passthrough_save_message (fpd);
         }

         else {
            if (!sys.netmail)
               npkt++;

            if (sys.quick_board) {
               wputs ("QuickBBS   ");
               quick_save_message2 (fpd, f1, f2, f3, f4, f5);
            }
            else if (sys.pip_board) {
               wputs ("Pip-Base   ");
               pip_save_message2 (fpd, f1, f2, f3);
            }
            else if (sys.squish) {
               wputs ("Squish<tm> ");
               squish_save_message2 (fpd);
            }
            else {
               wputs ("Fido       ");
               fido_save_message2 (fpd, NULL);
            }
         }

         wputs (" Ok");
      }
   }

   if (dupecheck->areatag[0])
      write_dupe_check ();

   if (npkt && sys.echomail && strlen (sys.echotag))
      status_line ("+%s with %d msg(s)", sys.echotag, npkt);

   if (f1 != NULL)
      fclose (f1);
   if (f2 != NULL)
      fclose (f2);
   if (f3 != NULL)
      fclose (f3);
   if (f4 != NULL)
      fclose (f4);
   if (f5 != NULL)
      fclose (f5);
   f1 = f2 = f3 = f4 = f5 = NULL;

   fclose (fpd);
   free (forward);
   free (path);
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static char *get_to_null (fp, dest, max)
FILE *fp;
char *dest;
int max;
{
   int i = 0;
   char c;

   max--;
   while ((c = fgetc (fp)) != '\0') {
      if (i < max)
         dest[i++] = c;
   }

   dest[i] = c;
   return (dest);
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static char *packet_fgets (dest, max, fp)
char *dest;
int max;
FILE *fp;
{
   int i = 0, c;

   max--;
   while (i < max) {
      if ((c = fgetc (fp)) == EOF)
         return (NULL);
      dest[i++] = (char )c;
      if ((char )c == '\0')
         break;
      if ((char )c == 0x0D) {
         if ((c = fgetc (fp)) == 0x0A)
            dest[i++] = (char )c;
         else
            ungetc (c, fp);
         break;
      }
   }

   dest[i] = '\0';
   return (dest);
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static int read_dupe_check (area)
char *area;
{
   FILE *fp;
   int fd, found = 0, i, zo, ne, no, po;
   char filename[80], ptype, linea[256], *location, *tag, *forward, *p;
   struct _sys tsys;
   struct _dupeindex dupeindex[MAX_DUPEINDEX];

   if (sys.quick_board)
      ptype = 1;
   else if (sys.pip_board)
      ptype = 2;
   else
      ptype = 0;

   if (!sys.netmail) {
      sprintf (filename, "%sAREAS.BBS", sys_path);
      fp = fopen (filename, "rt");
      if (fp != NULL) {
         fgets(linea, 255, fp);

         while (fgets(linea, 255, fp) != NULL) {
            if (linea[0] == ';')
               continue;
            while (linea[strlen (linea) -1] == 0x0D || linea[strlen (linea) -1] == 0x0A || linea[strlen (linea) -1] == ' ')
               linea[strlen (linea) -1] = '\0';
            location = strtok (linea, " ");
            tag = strtok (NULL, " ");
            forward = strtok (NULL, "");
            while (*forward == ' ')
               forward++;
            if (!stricmp (tag, area)) {
               memset ((char *)&sys, 0, sizeof (struct _sys));

               sys.echomail = 1;
               strcpy (sys.echotag, tag);
               strcpy (sys.forward1, forward);

               zo = alias[sys.use_alias].zone;
               parse_netnode2 (sys.forward1, &zo, &ne, &no, &po);
               if (zo != alias[sys.use_alias].zone) {
                  for (i = 0; alias[i].net; i++)
                     if (zo == alias[i].zone)
                        break;
                  if (alias[i].net)
                     sys.use_alias = i;
               }

               if (!strcmp (location, "##"))
                  sys.passthrough = 1;
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

               found = 1;
               break;
            }
         }

         fclose (fp);
      }

      if (!found) {
         sprintf (filename, SYSMSG_PATH, sys_path);
         fd = cshopen(filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);

         while (read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
            if (!stricmp (tsys.echotag, area)) {
               found = 1;
               break;
            }
         }

         close (fd);

         if (!found) {
            i = 1;

            if (newareas_create != NULL) {
               zo = alias[sys.use_alias].zone;
               ne = alias[sys.use_alias].net;
               no = alias[sys.use_alias].node;
               po = alias[sys.use_alias].point;
               i = 0;

               strcpy (linea, newareas_create);
               p = strtok (linea, " ");
               if (p != NULL)
                  do {
                     parse_netnode2 (p, &zo, &ne, &no, &po);
                     if (zo == msg_fzone && ne == msg.orig_net && no == msg.orig && po == msg_fpoint) {
                        i = 1;
                        break;
                     }
                  } while ((p = strtok (NULL, " ")) != NULL);
            }

            if (i) {
               memset ((char *)&tsys, 0, sizeof (struct _sys));
               tsys.msg_priv = SYSOP;
               strcpy (tsys.echotag, area);
               strcpy (tsys.msg_name, area);
               strcat (tsys.msg_name, " (New area)");

               i = 0;
               do {
                  i++;
                  sprintf (filename, "NEWAREA.%03d", i);
               } while (mkdir (filename) == -1);

               getcwd (tsys.msg_path, 25);
               strcat (tsys.msg_path, "\\");
               strcat (tsys.msg_path, filename);
               strcat (tsys.msg_path, "\\");

               tsys.echomail = 1;
               sprintf (tsys.forward1, "%d:%d/%d", msg_fzone, msg.orig_net, msg.orig);
               strcat (tsys.forward1, newareas_link);

               sprintf (filename, "%sAREAS.BBS", sys_path);
               fp = fopen (filename, "at");
               fprintf (fp, "%-30s %-30s %s", tsys.msg_path, tsys.echotag, tsys.forward1);
               fclose (fp);
            }
            else
               return (0);
         }

         memcpy ((char *)&sys, (char *)&tsys, sizeof (struct _sys));
      }
   }

   if (sys.quick_board) {
      if (ptype == 1) {
         fflush (f5);
         real_flush (fileno (f5));
         fflush (f1);
         real_flush (fileno (f1));
      }
      quick_read_sysinfo (sys.quick_board);
      if (ptype != 1) {
         if (ptype == 2) {
            fclose (f1);
            fclose (f2);
            fclose (f3);
         }
         sprintf (filename, "%sMSGIDX.BBS", fido_msgpath);
         f1 = fopen (filename, "ab");
         sprintf (filename, "%sMSGTOIDX.BBS", fido_msgpath);
         f2 = fopen (filename, "ab");
         sprintf (filename, "%sMSGTXT.BBS", fido_msgpath);
         f3 = fopen (filename, "ab");
         sprintf (filename, "%sMSGHDR.BBS", fido_msgpath);
         f4 = fopen (filename, "ab");
         sprintf (filename, "%sMSGINFO.BBS", fido_msgpath);
         f5 = fopen (filename, "r+b");
      }
   }
   else if (sys.pip_board) {
      if (ptype == 1) {
         fclose (f1);
         fclose (f2);
         fclose (f3);
         fclose (f4);
         fclose (f5);
         f1 = f2 = f3 = f4 = f5 = NULL;
      }

      if (f1 != NULL)
         fclose (f1);
      if (f2 != NULL)
         fclose (f2);
      if (f3 != NULL)
         fclose (f3);
      if (f4 != NULL)
         fclose (f4);
      if (f5 != NULL)
         fclose (f5);

      sprintf(filename,"%sMPTR%04x.PIP", pip_msgpath, sys.pip_board);
      f1=fopen(filename,"rb+");
      if (f1==NULL) {
         f1=fopen(filename,"wb");
         fclose (f1);
         f1=fopen(filename,"rb+");
      }
      sprintf(filename,"%sMPKT%04x.PIP", pip_msgpath, sys.pip_board);
      f2=fopen(filename,"rb+");
      if (f2==NULL) {
         f2=fopen(filename,"wb");
         fputc(0,f2);
         fputc(0,f2);
         fclose (f2);
         f2=fopen(filename,"rb+");
      }
      sprintf (filename, "%sDESTPTR.PIP", pip_msgpath);
      f3 = fopen (filename, "ab");
      f4 = f5 = NULL;

      pip_scan_message_base (0, 0);
   }
   else if (sys.squish) {
      if (f1 != NULL)
         fclose (f1);
      if (f2 != NULL)
         fclose (f2);
      if (f3 != NULL)
         fclose (f3);
      if (f4 != NULL)
         fclose (f4);
      if (f5 != NULL)
         fclose (f5);
      f1 = f2 = f3 = f4 = f5 = NULL;
      squish_scan_message_base (0, sys.msg_path, 0);
   }
   else {
      if (f1 != NULL)
         fclose (f1);
      if (f2 != NULL)
         fclose (f2);
      if (f3 != NULL)
         fclose (f3);
      if (f4 != NULL)
         fclose (f4);
      if (f5 != NULL)
         fclose (f5);
      f1 = f2 = f3 = f4 = f5 = NULL;
      scan_message_base (0, 0);
   }

   found = 0;

   sprintf (filename, "%sDUPES.IDX", sys_path);
   fd = open (filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
   do {
      po = read (fd, (char *)&dupeindex, sizeof(struct _dupeindex) * MAX_DUPEINDEX);
      po /= sizeof (struct _dupeindex);
      for (i = 0; i < po; i++) {
         if (!stricmp (area, dupeindex[i].areatag)) {
            dupecheck->area_pos = dupeindex[i].area_pos;
            found = 1;
            break;
         }
      }
   } while (!found && po == MAX_DUPEINDEX);

   close (fd);

   sprintf (filename, "%sDUPES.DAT", sys_path);
   fd = open (filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
   if (found) {
      lseek (fd, dupecheck->area_pos, SEEK_SET);
      read (fd, (char *)dupecheck, sizeof (struct _dupecheck));
   }
   else {
      lseek (fd, 0L, SEEK_END);
      memset ((char *)dupecheck, 0, sizeof (struct _dupecheck));
      dupecheck->area_pos = tell (fd);
      strcpy (dupecheck->areatag, area);
      write (fd, (char *)dupecheck, sizeof (struct _dupecheck));
      close (fd);

      sprintf (filename, "%sDUPES.IDX", sys_path);
      fd = open (filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
      memset ((char *)&dupeindex[0], 0, sizeof (struct _dupeindex));
      dupeindex[0].area_pos = dupecheck->area_pos;
      strcpy (dupeindex[0].areatag, area);
      lseek (fd, 0L, SEEK_END);
      write (fd, (char *)&dupeindex[0], sizeof(struct _dupeindex));
   }

   close (fd);

   return (1);
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static void write_dupe_check ()
{
   int fd;
   char filename[80];

   sprintf (filename, "%sDUPES.DAT", sys_path);
   fd = open (filename, O_WRONLY|O_BINARY);
   lseek (fd, dupecheck->area_pos, SEEK_SET);
   write (fd, (char *)dupecheck, sizeof (struct _dupecheck));
   close (fd);
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static int check_and_add_dupe (id)
long id;
{
   register int i;

   for (i = 0; i < dupecheck->max_dupes; i++)
      if (dupecheck->dupes[i] == id)
         return (1);
   if (i == dupecheck->max_dupes) {
      i = dupecheck->dupe_pos;
      if (i >= MAXDUPES)
         i = 0;
      dupecheck->dupes[i++] = id;
      dupecheck->dupe_pos = i;
      if (dupecheck->max_dupes < i)
         dupecheck->max_dupes = i;
   }

   return (0);
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static long compute_crc (string, crc)
char *string;
long crc;
{
   int i;

   for (i = 0; string[i]; i++)
      crc = Z_32UpdateCRC ((byte )string[i], crc);

   return (crc);
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static void fido_save_message2 (txt, bad)
FILE *txt;
char *bad;
{
        FILE *fp;
        int i, dest, m;
        char filename[80], buffer[2050];

        if (bad == NULL) {
           if (!last_msg)
              last_msg++;
           dest = last_msg + 1;
           sprintf(filename,"%s%d.MSG",sys.msg_path,dest);
        }
        else {
           if (bad == bad_msgs)
              dest = ++nbad;
           else
              dest = ++ndupes;
           sprintf(filename,"%s%d.MSG",bad,dest);
        }

        fp = fopen(filename,"wb");
        if (fp == NULL)
             return;

        if (sys.public)
           msg.attr &= ~MSGPRIVATE;
        else if (sys.private)
           msg.attr |= MSGPRIVATE;

        fwrite((char *)&msg,sizeof(struct _msg),1,fp);

        if (bad == bad_msgs && sys.echomail)
           fprintf (fp, "AREA:%s\r\n", sys.echotag);

        do {
                i = fread(buffer, 1, 2048, txt);
                for (m=0;m<i;m++) {
                        if (buffer[m] == 0x1A)
                                buffer[m] = ' ';
                }
                fwrite(buffer, 1, i, fp);
        } while (i == 2048);

        fputc('\0',fp);
        fclose(fp);

        last_msg = dest;
}

/*---------------------------------------------------------------------------
   static void passthrough_save_message (fpd)

   Esporta immediatamente il messaggio per le aree passthrough, cioe'
   quelle aree che non devono rimanere residenti nella base messaggi locale
   ma devono essere immediatamente esportate e cancellate.
---------------------------------------------------------------------------*/
static void passthrough_save_message (fpd)
FILE *fpd;
{
   int mi, z;
   char *p, buffer[2050], buff[80];
   struct _msghdr2 mhdr;
   struct _pkthdr2 pkthdr;
   struct date datep;
   struct time timep;

   mhdr.ver = PKTVER;
   mhdr.orig_node = msg.orig;
   mhdr.orig_net = msg.orig_net;
   mhdr.dest_node = msg.dest;
   mhdr.dest_net = msg.dest_net;

   mhdr.cost = 0;

   if (sys.public)
      msg.attr &= ~MSGPRIVATE;
   else if (sys.private)
      msg.attr |= MSGPRIVATE;
   mhdr.attrib = msg.attr;

   gettime (&timep);
   getdate (&datep);

   p = HoldAreaNameMunge (msg_tzone);
   if (msg_tpoint && (msg.attr & (MSGCRASH|MSGHOLD)))
      sprintf (buff, "%s%04x%04x.PNT\\%08X.%cUT", p, msg.dest_net, msg.dest, msg_tpoint, (msg.attr & MSGCRASH) ? 'C' : ((msg.attr & MSGHOLD) ? 'H' : 'O'));
   else
      sprintf (buff, "%s%04x%04x.%cUT", p, msg.dest_net, msg.dest, (msg.attr & MSGCRASH) ? 'C' : ((msg.attr & MSGHOLD) ? 'H' : 'O'));
   mi = open (buff, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
   if (mi == -1 && msg_tpoint && (msg.attr & (MSGCRASH|MSGHOLD))) {
      sprintf (buff, "%s%04x%04x.PNT", p, msg.dest_net, msg.dest);
      mkdir (buff);
      sprintf (buff, "%s%04x%04x.PNT\\%08X.%cUT", p, msg.dest_net, msg.dest, msg_tpoint, (msg.attr & MSGCRASH) ? 'C' : ((msg.attr & MSGHOLD) ? 'H' : 'O'));
      mi = open (buff, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
   }
   if (filelength (mi) > 0L)
      lseek(mi,filelength(mi)-2,SEEK_SET);
   else {
      memset ((char *)&pkthdr, 0, sizeof (struct _pkthdr2));
      pkthdr.ver = PKTVER;
      pkthdr.product = 0x4E;
      pkthdr.serial = 2 * 16 + 11;
      pkthdr.orig_node = alias[sys.use_alias].node;
      pkthdr.orig_net = alias[sys.use_alias].net;
      pkthdr.orig_zone = alias[sys.use_alias].zone;
      pkthdr.dest_node = msg.dest;
      pkthdr.dest_net = msg.dest_net;
      pkthdr.dest_zone = msg_tzone;
      pkthdr.hour = timep.ti_hour;
      pkthdr.minute = timep.ti_min;
      pkthdr.second = timep.ti_sec;
      pkthdr.year = datep.da_year;
      pkthdr.month = datep.da_mon;
      pkthdr.day = datep.da_day;
      write (mi, (char *)&pkthdr, sizeof (struct _pkthdr2));
   }

   write (mi, (char *)&mhdr, sizeof (struct _msghdr2));

   write (mi, msg.date, strlen (msg.date) + 1);
   write (mi, msg.to, strlen (msg.to) + 1);
   write (mi, msg.from, strlen (msg.from) + 1);
   write (mi, msg.subj, strlen (msg.subj) + 1);

   if (sys.echomail) {
      sprintf (buffer, "AREA:%s\r\n", sys.echotag);
      write (mi, buffer, strlen (buffer));
   }

   fseek (fpd, 0L, SEEK_SET);
   do {
      z = fread(buffer, 1, 2048, fpd);
      write(mi, buffer, z);
   } while (z == 2048);

   buff[0] = buff[1] = buff[2] = 0;
   write (mi, buff, 3);

   close (mi);
}

static int isbundle (n)
char *n;
{
   strupr(n);
   return((strstr(n,".MO") || strstr(n,".TU") ||
           strstr(n,".WE") || strstr(n,".TH") ||
           strstr(n,".FR") || strstr(n,".SA") ||
           strstr(n,".SU")) &&
          isdigit(n[strlen(n)-1]));
}

static void unpack_arcmail (dir)
char *dir;
{
   FILE *f;
   int retval, x, y;
   char filename[80], parms[30], extr[80], swaparg[128];
   unsigned char headstr[8];
   struct ffblk blk;

   sprintf (filename, "%s*.*", dir);
   if (findfirst (filename, &blk, 0))
      return;

   do {
      // Controlla se si tratta di un arcmail autentico, guardando il nome
      // (8 caratteri esadecimali) e l'estensione (due lettere del giorno
      // della settimana e un numero decimale).
      if (!isbundle (blk.ff_name))
         continue;

      status_line ("#Importing ARCMail packet %s", blk.ff_name);

      sprintf (extr, "%s%s", dir, blk.ff_name);
      f = fopen (extr,"rb");
      fread (headstr, 7, 1, f);
      fclose (f);

      // Assume di default PKXARC
      sprintf(filename,"PKXARC.COM");
      strcpy(parms,"-r");

      // Se si tratta di un lharc controlla la versione e chiama il
      // compattatore opportuno.
      if ((headstr[2]=='-') && (headstr[3]=='l'))
         switch(headstr[4]) {
            case 'h':
               sprintf(filename,"LHA.EXE");
               if (!access(filename,0) || (headstr[5]>'1'))
                  sprintf(filename,"LHA.EXE");
               else
                  sprintf(filename,"LHARC.EXE");
               strcpy(parms,"x /cm");
               break;
            case 'z': 
               sprintf(filename,"LHA.EXE");
               strcpy(parms,"x /cm");
               break;
            default:
               status_line("!Unknown compression method for ARCMail!");
         }
      else
         switch(*headstr) {
            case 26:
               // In questo caso e' piu' appropriato l'uso di PAK
               sprintf(filename,"PAK.EXE");
               strcpy(parms,"E /WA");
               break;
            case 'P':
               if (headstr[1]==75) {
                  sprintf(filename,"PKUNZIP.EXE");
                  strcpy(parms,"-o -ed");
               }
               break;
            case 'Z':
               if ((headstr[1]=='O') && (headstr[2]=='O')) {
                  sprintf(filename,"ZOO.EXE");
                  strcpy(parms,"E");
                  break;
               }
            case 96:
               if (headstr[1]==234) {
                  sprintf(filename,"ARJ.EXE");
                  strcpy(parms,"e -y");
                  break;
               }
            default:
               break;
         }

      status_line ("#External program: %s", strupr(filename));
      readcur (&y, &x);

      activation_key ();
      if (!registered)
         retval = spawnlp (P_WAIT, filename, filename, parms, extr, NULL);
      else {
         sprintf (swaparg, "%s %s", parms, extr);
         retval = do_exec (filename, swaparg, USE_ALL|HIDE_FILE, 0xFFFF, NULL);
      }

      clrwin (y, x, 22, 79);
      gotoxy_ (y, x);

      // Se il compattatore ritorna un errorlevel diverso da 0 significa che
      // si e' rilevato un errore, pertanto non viene cancellato il pacchetto.
      if (retval)
         status_line (":Return code %d", retval);
      else
         unlink (extr);
   } while (!findnext (&blk));
}


void export_mail (who)
int who;
{
   FILE *fp;
   int maxnodes, wh, i, fd, *varr;
   char filename[80], *p, linea[256], *tag, *location, *forw, curdir[52];
   struct _fwrd *forward;
   struct date datep;
   struct time timep;
   struct _pkthdr2 pkthdr;

   if (pre_export != NULL) {
      getcwd (curdir, 49);
      status_line ("+Running BEFORE_EXPORT: %s", pre_export);
      varr = ssave ();
      cclrscrn(LGREY|_BLACK);
      if ((p = strchr (pre_export, ' ')) != NULL)
         *p++ = '\0';
      do_exec (pre_export, p, USE_ALL|HIDE_FILE, 0xFFFF, NULL);
      if (p != NULL)
         *(--p) = ' ';
      if (varr != NULL)
         srestore (varr);
      setdisk (curdir[0] - 'A');
      chdir (curdir);
   }
//   free (Txbuf);

   if (who & ECHOMAIL_RSN) {
      forward = (struct _fwrd *)malloc (MAX_FORWARD * sizeof (struct _fwrd));
      if (forward == NULL) {
//         Txbuf = Secbuf = (byte *) malloc (WAZOOMAX + 16);
//         if (Txbuf == NULL)
//            exit (250);
         return;
      }

      status_line (":EXPORT");
      wh = wopen (7, 5, 20, 75, 1, LCYAN|_BLUE, LCYAN|_BLUE);
      wactiv (wh);

      maxnodes = 0;

      time_release ();
      wtitle (" Export AREAS.BBS ", TLEFT, LCYAN|_BLUE);
      sprintf (filename, "%sAREAS.BBS", sys_path);
      fp = fopen (filename, "rt");
      if (fp != NULL) {
         fgets(linea, 255, fp);

         while (fgets(linea, 255, fp) != NULL) {
            if (linea[0] == ';')
               continue;
            time_release ();
            while (linea[strlen (linea) -1] == 0x0D || linea[strlen (linea) -1] == 0x0A || linea[strlen (linea) -1] == ' ')
               linea[strlen (linea) -1] = '\0';
            location = strtok (linea, " ");
            tag = strtok (NULL, " ");
            forw = strtok (NULL, "");
            while (*forw == ' ')
               forw++;
            sys.echomail = 1;
            strcpy (sys.echotag, tag);
            strcpy (sys.forward1, forw);
            if (*location == '$') {
               strcpy (sys.msg_path, ++location);
               sys.squish = 1;
               squish_scan_message_base (0, sys.msg_path, 0);
               maxnodes = squish_export_mail (maxnodes, forward);
            }
            else if (*location == '!') {
               sys.pip_board = atoi(++location);
               maxnodes = pip_export_mail (maxnodes, forward);
            }
            else if (!atoi (location) && stricmp (location, "##")) {
               strcpy (sys.msg_path, location);
               scan_message_base (0, 0);
               maxnodes = fido_export_mail (maxnodes, forward);
            }
         }

         fclose (fp);
      }

      time_release ();
      wtitle (" Export QuickBBS ", TLEFT, LCYAN|_BLUE);
      maxnodes = quick_export_mail (maxnodes, forward);

      time_release ();
      wtitle (" Export Squish<tm> ", TLEFT, LCYAN|_BLUE);
      sprintf (filename, SYSMSG_PATH, sys_path);
      fd = cshopen(filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
      if (fd != -1) {
         while (read(fd, (char *)&sys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
            if (sys.squish) {
               squish_scan_message_base (0, sys.msg_path, 0);
               maxnodes = squish_export_mail (maxnodes, forward);
            }
         }

         if (sq_ptr != NULL) {
            MsgCloseArea (sq_ptr);
            sq_ptr = NULL;
         }
         close (fd);
      }

      time_release ();
      wtitle (" Export Pip-Base ", TLEFT, LCYAN|_BLUE);
      sprintf (filename, SYSMSG_PATH, sys_path);
      fd = cshopen(filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
      if (fd != -1) {
         while (read(fd, (char *)&sys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
            if (sys.pip_board) {
               maxnodes = pip_export_mail (maxnodes, forward);
            }
         }

         close (fd);
      }

      time_release ();
      wtitle (" Export Fido ", TLEFT, LCYAN|_BLUE);
      sprintf (filename, SYSMSG_PATH, sys_path);
      fd = cshopen(filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
      if (fd != -1) {
         while (read(fd, (char *)&sys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
            if (!sys.pip_board && !sys.squish && !sys.quick_board && sys.echomail) {
               scan_message_base (0, 0);
               maxnodes = fido_export_mail (maxnodes, forward);
            }
         }

         close (fd);
      }

      gettime (&timep);
      getdate (&datep);

      for (i = 0; i < maxnodes; i++) {
         p = HoldAreaNameMunge (forward[i].zone);
         if (forward[i].point)
            sprintf (filename, "%s%04X%04X.PNT\\%08X.OUT", p, forward[i].net, forward[i].node, forward[i].point);
         else
            sprintf (filename, "%s%04X%04X.OUT", p, forward[i].net, forward[i].node);
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
                  pkthdr.month = datep.da_mon;
                  pkthdr.day = datep.da_day;
                  lseek (fd, 0L, SEEK_SET);
                  write (fd, (char *)&pkthdr, sizeof (struct _pkthdr2));
                  status_line ("#Exported packet for node %d:%d/%d", forward[i].zone, forward[i].net, forward[i].node);
               }
               close (fd);
            }
         }
      }

      wclose ();
      wunlink (wh);
      free (forward);

      pack_outbound ();

      if (after_export != NULL) {
         getcwd (curdir, 49);
         status_line ("+Running AFTER_EXPORT: %s", after_export);
         varr = ssave ();
         cclrscrn(LGREY|_BLACK);
         if ((p = strchr (after_export, ' ')) != NULL)
            *p++ = '\0';
         do_exec (after_export, p, USE_ALL|HIDE_FILE, 0xFFFF, NULL);
         if (p != NULL)
            *(--p) = ' ';
         if (varr != NULL)
            srestore (varr);
         setdisk (curdir[0] - 'A');
         chdir (curdir);
      }
   }

   if (who & NETMAIL_RSN) {
      wh = wopen (9, 10, 17, 70, 1, LCYAN|_BLUE, LCYAN|_BLUE);
      wactiv (wh);

      wtitle (" Export Netmail ", TLEFT, LCYAN|_BLUE);
      fido_export_netmail ();

      wclose ();
      wunlink (wh);

      pack_outbound ();
   }

   memset ((char *)&sys, 0, sizeof (struct _sys));

//   Txbuf = Secbuf = (byte *) malloc (WAZOOMAX + 16);
//   if (Txbuf == NULL)
//      exit (250);
}

void pack_outbound ()
{
   FILE *fp;
   char filename[80], dest[80], outbase[80], *p, dstf, linea[80], do_pack, *v;
   int dzo, dne, dno, pzo, pne, pno, dpo, i, skip_tag;
   long t;
   struct ffblk blk, blko;
   struct tm *dt;

   t = time (NULL);
   dt = localtime (&t);
   skip_tag = 0;

   strcpy (filename, hold_area);
   filename[strlen(filename) - 1] = '\0';
   strcpy (outbase, filename);
   strcat (filename, ".*");

   if (!findfirst (filename, &blko, FA_DIREC))
      do {
         p = strchr (blko.ff_name, '.');

         sprintf (filename, "%s%s\\*.PKT", outbase, p == NULL ? "" : p);
         if (!findfirst (filename, &blk, 0))
            do {
               sprintf (filename, "%s%s\\%s", outbase, p == NULL ? "" : p, blk.ff_name);
               sprintf (dest, "%s%s\\*.P$T", outbase, p == NULL ? "" : p);
               rename (filename, dest);
            } while (!findnext (&blk));
      } while (!findnext (&blko));

   sprintf (filename, "%sROUTE.CFG", sys_path);
   fp = fopen (filename, "rt");
   if (fp == NULL)
      return;

   while (fgets (linea, 79, fp) != NULL) {
      if (linea[0] == ';' || linea[0] == '%')
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
         else
            skip_tag = 0;
      }
      else if (skip_tag)
         continue;

      dstf = 'F';

      if (!stricmp (p, "Poll")) {
         dstf = get_flag ();

         pzo = alias[0].zone;
         dne = alias[0].net;
         dno = alias[0].node;
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

         dzo = alias[0].zone;
         dne = alias[0].net;
         dno = alias[0].node;
         dpo = 0;

         v = strtok (NULL, "");

         while (v != NULL && (p = strtok (v, " ")) != NULL) {
            v = strtok (NULL, "");
            parse_netnode2 (p, &dzo, &dne, &dno, &dpo);
            do_pack = 0;

            if (dne && dno) {
               if (!dpo) {
                  sprintf (filename, "%s%04x%04x.OUT", HoldAreaNameMunge (dzo), dne, dno);
                  if (dexists (filename)) {
                     invent_pkt_name (outbase);
                     sprintf (dest, "%s%s", HoldAreaNameMunge (dzo), outbase);
                     if (rename (filename, dest) == -1)
                        do {
                           invent_pkt_name (outbase);
                           sprintf (dest, "%s%s", HoldAreaNameMunge (dzo), outbase);
                        } while (rename (filename, dest) == -1);
                     do_pack = 1;
                  }
               }
               else {
                  sprintf (filename, "%s%04x%04x.PNT\\%08X.OUT", HoldAreaNameMunge (dzo), dne, dno, dpo);
                  if (!dexists (filename) && dpo) {
                     sprintf (filename, "%s%04x%04x.PNT", HoldAreaNameMunge (dzo), dne, dno);
                     mkdir (filename);
                     sprintf (filename, "%s%04x%04x.PNT\\%08X.OUT", HoldAreaNameMunge (dzo), dne, dno, dpo);
                  }
                  if (dexists (filename)) {
                     invent_pkt_name (outbase);
                     sprintf (dest, "%s%s", HoldAreaNameMunge (dzo), outbase);
                     if (rename (filename, dest) == -1)
                        do {
                           invent_pkt_name (outbase);
                           sprintf (dest, "%s%s", HoldAreaNameMunge (dzo), outbase);
                        } while (rename (filename, dest) == -1);
                     do_pack = 1;
                  }
               }
            }
            else {
               if (dne && !dno)
                  sprintf (filename, "%s%04x*.OUT", HoldAreaNameMunge (dzo), dne);
               else
                  sprintf (filename, "%s*.OUT", HoldAreaNameMunge (dzo));

               if (!findfirst (filename, &blko, 0))
                  do {
                     invent_pkt_name (outbase);
                     sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blko.ff_name);
                     sprintf (dest, "%s%s", HoldAreaNameMunge (dzo), outbase);
                     if (rename (filename, dest) == -1)
                        do {
                           invent_pkt_name (outbase);
                           sprintf (filename, "%s%s", HoldAreaNameMunge (dzo), blko.ff_name);
                           sprintf (dest, "%s%s", HoldAreaNameMunge (dzo), outbase);
                        } while (rename (filename, dest) == -1);

                     sscanf (blko.ff_name, "%04x%04x", &dne, &dno);
                     if (alias[0].point && alias[0].fakenet)
                        sprintf (filename, "%s%04x%04x.%s?", HoldAreaNameMunge (dzo), alias[0].fakenet - dne, alias[0].point - dno, suffixes[dt->tm_wday]);
                     else if (alias[0].point)
                        sprintf (filename, "%s%04x%04x.%s?", HoldAreaNameMunge (dzo), alias[0].node - dne, alias[0].point - dno, suffixes[dt->tm_wday]);
                     else
                        sprintf (filename, "%s%04x%04x.%s?", HoldAreaNameMunge (dzo), alias[0].net - dne, alias[0].node - dno, suffixes[dt->tm_wday]);

                     if (findfirst (filename, &blk, 0)) {
                        if (alias[0].point && alias[0].fakenet)
                           sprintf (filename, "%s%04x%04x.%s0", HoldAreaNameMunge (dzo), alias[0].fakenet - dne, alias[0].point - dno, suffixes[dt->tm_wday]);
                        else if (alias[0].point)
                           sprintf (filename, "%s%04x%04x.%s0", HoldAreaNameMunge (dzo), alias[0].node - dne, alias[0].point - dno, suffixes[dt->tm_wday]);
                        else
                           sprintf (filename, "%s%04x%04x.%s0", HoldAreaNameMunge (dzo), alias[0].net - dne, alias[0].node - dno, suffixes[dt->tm_wday]);
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

                     sprintf (outbase, "%s%04x%04x.%cLO", HoldAreaNameMunge (dzo), dne, dno, dstf);
                     sprintf (dest, "%s*.PKT", HoldAreaNameMunge (dzo));
                     call_packer (outbase, filename, dest, dzo, dne, dno);

                     do_pack = 0;
                  } while (!findnext (&blko));

               if (dne && !dno)
                  dno = alias[0].node;
               else {
                  dne = alias[0].net;
                  dno = alias[0].node;
               }
            }

            if (do_pack) {
               if (alias[0].point && alias[0].fakenet)
                  sprintf (filename, "%s%04x%04x.%s?", HoldAreaNameMunge (dzo), alias[0].fakenet - dne, alias[0].point - dno, suffixes[dt->tm_wday]);
               else
                  sprintf (filename, "%s%04x%04x.%s?", HoldAreaNameMunge (dzo), alias[0].net - dne, alias[0].node - dno, suffixes[dt->tm_wday]);

               if (findfirst (filename, &blk, 0)) {
                  if (alias[0].point && alias[0].fakenet)
                     sprintf (filename, "%s%04x%04x.%s0", HoldAreaNameMunge (dzo), alias[0].fakenet - dne, alias[0].point - dno, suffixes[dt->tm_wday]);
                  else
                     sprintf (filename, "%s%04x%04x.%s0", HoldAreaNameMunge (dzo), alias[0].net - dne, alias[0].node - dno, suffixes[dt->tm_wday]);
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

               sprintf (outbase, "%s%04x%04x.%cLO", HoldAreaNameMunge (dzo), dne, dno, dstf);
               sprintf (dest, "%s*.PKT", HoldAreaNameMunge (dzo));
               call_packer (outbase, filename, dest, dzo, dne, dno);
            }
         }
      }
      else if (!stricmp (p, "Leave")) {
         dzo = alias[0].zone;
         dne = alias[0].net;
         dno = alias[0].node;

         while ((p = strtok (NULL, " ")) != NULL) {
            parse_netnode2 (p, &dzo, &dne, &dno, &i);

            if (dne && dno) {
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
               if (dne && !dno)
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

               if (dne && !dno)
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

               if (dne && !dno)
                  dno = alias[0].node;
               else {
                  dne = alias[0].net;
                  dno = alias[0].node;
               }
            }
         }
      }
      else if (!stricmp (p, "UnLeave")) {
         dzo = alias[0].zone;
         dne = alias[0].net;
         dno = alias[0].node;

         while ((p = strtok (NULL, " ")) != NULL) {
            parse_netnode2 (p, &dzo, &dne, &dno, &i);

            if (dne && dno) {
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
               if (dne && !dno)
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

               if (dne && !dno)
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

               if (dne && !dno)
                  dno = alias[0].node;
               else {
                  dne = alias[0].net;
                  dno = alias[0].node;
               }
            }
         }
      }
      else if (!stricmp (p, "Route-To") || !stricmp (p, "Route")) {
         dstf = get_flag ();

         dzo = alias[0].zone;
         dne = alias[0].net;
         dno = alias[0].node;
         do_pack = 0;

         p = strtok (NULL, " ");
         parse_netnode2 (p, &dzo, &dne, &dno, &i);
         pzo = dzo;
         pne = dne;
         pno = dno;

         for (;;) {
            if (pne && pno) {
               sprintf (filename, "%s%04x%04x.OUT", HoldAreaNameMunge (pzo), pne, pno);
               if (dexists (filename)) {
                  invent_pkt_name (outbase);
                  sprintf (dest, "%s%s", HoldAreaNameMunge (dzo), outbase);
                  if (rename (filename, dest) == -1)
                     do {
                        invent_pkt_name (outbase);
                        sprintf (dest, "%s%s", HoldAreaNameMunge (dzo), outbase);
                     } while (rename (filename, dest) == -1);
                  do_pack = 1;
               }
            }
            else {
               if (pne && !pno)
                  sprintf (filename, "%s%04x*.OUT", HoldAreaNameMunge (pzo), pne);
               else
                  sprintf (filename, "%s*.OUT", HoldAreaNameMunge (pzo));
               if (!findfirst (filename, &blk, 0))
                  do {
                     invent_pkt_name (outbase);
                     sprintf (filename, "%s%s", HoldAreaNameMunge (pzo), blk.ff_name);
                     sprintf (dest, "%s%s", HoldAreaNameMunge (dzo), outbase);
                     if (rename (filename, dest) == -1)
                        do {
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

         if (do_pack) {
            if (alias[0].point && alias[0].fakenet)
               sprintf (filename, "%s%04x%04x.%s?", HoldAreaNameMunge (dzo), alias[0].fakenet - dne, alias[0].point - dno, suffixes[dt->tm_wday]);
            else
               sprintf (filename, "%s%04x%04x.%s?", HoldAreaNameMunge (dzo), alias[0].net - dne, alias[0].node - dno, suffixes[dt->tm_wday]);

            if (findfirst (filename, &blk, 0)) {
               if (alias[0].point && alias[0].fakenet)
                  sprintf (filename, "%s%04x%04x.%s0", HoldAreaNameMunge (dzo), alias[0].fakenet - dne, alias[0].point - dno, suffixes[dt->tm_wday]);
               else
                  sprintf (filename, "%s%04x%04x.%s0", HoldAreaNameMunge (dzo), alias[0].net - dne, alias[0].node - dno, suffixes[dt->tm_wday]);
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

            sprintf (outbase, "%s%04x%04x.%cLO", HoldAreaNameMunge (dzo), dne, dno, dstf);
            sprintf (dest, "%s*.PKT", HoldAreaNameMunge (dzo));
            call_packer (outbase, filename, dest, dzo, dne, dno);
         }
      }
   }

   fclose (fp);
}

static void call_packer (attach, arcmail, packet, zone, net, node)
char *attach, *arcmail, *packet;
int zone, net, node;
{
   FILE *fp;
   int x, y, arctype, dzo, dne, dno, i;
   char outbase[128], *p;

   arctype = -1;

   if (arctype == -1 && pack_zip[0] != NULL) {
        dzo = alias[0].zone;
        dne = alias[0].net;
        dno = alias[0].node;

        for (i = 0; i < MAX_PACKERS && pack_zip[i] != NULL; i++) {
           strcpy (outbase, pack_zip[i]);
           p = strtok (outbase, " ");
           if (p != NULL)
              do {
                 parse_netnode2 (p, &dzo, &dne, &dno, &x);
                 if (dzo == zone && dne == net && dno == node) {
                    arctype = 1;
                    break;
                 }
              } while ((p = strtok (NULL, " ")) != NULL);

           if (arctype == 1)
              break;
        }
   }

   if (arctype == -1 && pack_arj[0] != NULL) {
        dzo = alias[0].zone;
        dne = alias[0].net;
        dno = alias[0].node;

        for (i = 0; i < MAX_PACKERS && pack_arj[i] != NULL; i++) {
           strcpy (outbase, pack_arj[i]);
           p = strtok (outbase, " ");
           if (p != NULL)
              do {
                 parse_netnode2 (p, &dzo, &dne, &dno, &x);
                 if (dzo == zone && dne == net && dno == node) {
                    arctype = 2;
                    break;
                 }
              } while ((p = strtok (NULL, " ")) != NULL);

           if (arctype == 2)
              break;
        }
   }

   if (arctype == -1 && pack_lzh[0] != NULL) {
        dzo = alias[0].zone;
        dne = alias[0].net;
        dno = alias[0].node;

        for (i = 0; i < MAX_PACKERS && pack_lzh[i] != NULL; i++) {
           strcpy (outbase, pack_lzh[i]);
           p = strtok (outbase, " ");
           if (p != NULL)
              do {
                 parse_netnode2 (p, &dzo, &dne, &dno, &x);
                 if (dzo == zone && dne == net && dno == node) {
                    arctype = 3;
                    break;
                 }
              } while ((p = strtok (NULL, " ")) != NULL);

           if (arctype == 3)
              break;
        }
   }

   if (arctype == -1 && pack_lha[0] != NULL) {
        dzo = alias[0].zone;
        dne = alias[0].net;
        dno = alias[0].node;

        for (i = 0; i < MAX_PACKERS && pack_lha[i] != NULL; i++) {
           strcpy (outbase, pack_lha[i]);
           p = strtok (outbase, " ");
           if (p != NULL)
              do {
                 parse_netnode2 (p, &dzo, &dne, &dno, &x);
                 if (dzo == zone && dne == net && dno == node) {
                    arctype = 4;
                    break;
                 }
              } while ((p = strtok (NULL, " ")) != NULL);

           if (arctype == 4)
              break;
        }
   }

   if (arctype == -1 && pack_arc[0] != NULL) {
        dzo = alias[0].zone;
        dne = alias[0].net;
        dno = alias[0].node;

        for (i = 0; i < MAX_PACKERS && pack_arc[i] != NULL; i++) {
           strcpy (outbase, pack_arc[i]);
           p = strtok (outbase, " ");
           if (p != NULL)
              do {
                 parse_netnode2 (p, &dzo, &dne, &dno, &x);
                 if (dzo == zone && dne == net && dno == node) {
                    arctype = 0;
                    break;
                 }
              } while ((p = strtok (NULL, " ")) != NULL);

           if (arctype == 0)
              break;
        }
   }

   x = 0;
   fp = fopen (attach, "rt");
   while (fgets (outbase, 79, fp) != NULL) {
      while (outbase[strlen (outbase) -1] == 0x0D || outbase[strlen (outbase) -1] == 0x0A)
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

   if (!x) {
      fp = fopen (attach, "at");
      fprintf (fp, "#%s\n", fancy_str(arcmail));
      fclose (fp);
   }

   wactiv (mainview);
   readcur (&y, &x);

   activation_key ();

   if (arctype == -1)
      arctype = def_pack;

   if (arctype == 1) {
      sprintf (outbase, "%s %s %s", "-M", arcmail, packet);
      do_exec ("PKZIP", outbase, USE_ALL|HIDE_FILE, 0xFFFF, NULL);
   }
   else if (arctype == 2) {
      sprintf (outbase, "%s %s %s", "M -E -Y", arcmail, packet);
      do_exec ("ARJ", outbase, USE_ALL|HIDE_FILE, 0xFFFF, NULL);
   }
   else if (arctype == 3) {
      sprintf (outbase, "%s %s %s", "m -m", arcmail, packet);
      do_exec ("LHARC", outbase, USE_ALL|HIDE_FILE, 0xFFFF, NULL);
   }
   else if (arctype == 4) {
      sprintf (outbase, "%s %s %s", "m -m -o", arcmail, packet);
      do_exec ("LHA", outbase, USE_ALL|HIDE_FILE, 0xFFFF, NULL);
   }
   else {
      sprintf (outbase, "%s %s %s", "m", arcmail, packet);
      do_exec ("PKARC", outbase, USE_ALL|HIDE_FILE, 0xFFFF, NULL);
   }

   clrwin (y, x, 22, 79);
   gotoxy_ (y, x);
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

