#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <dir.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "quickmsg.h"

#define MAX_MESSAGES   1000

char *pascal_string (char *);
int quick_msg_attrib(struct _msghdr *, int, int, int);
void quick_text_header(struct _msghdr *, int, FILE *);
void write_pascal_string (char *, char *, word *, word *, FILE *);
void quick_qwk_text_header (struct _msghdr *, int, FILE *, struct QWKmsghd *, long *);
void replace_tearline (FILE *fpd, char *buf);
int open_packet (int zone, int net, int node, int point, int ai);

FILE *mopen (char *filename, char *mode);
int mclose (FILE *fp);
int mputs (char *s, FILE *fp);
void mprintf (FILE *fp, char *format, ...);
long mseek (FILE *fp, long position, int offset);
int mread (char *s, int n, int e, FILE *fp);

int msgidx[MAX_MESSAGES];
struct _msginfo msginfo;

struct _aidx {
   char areatag[32];
   byte board;
};

extern struct _aidx *aidx;
extern int maxaidx;

extern int maxakainfo;
extern struct _akainfo *akainfo;

void write_pascal_string (st, text, pos, nb, fp)
char *st, *text;
word *pos, *nb;
FILE *fp;
{
   word v, m, blocks;

   m = *pos;
   blocks = *nb;

   for (v=0; st[v]; v++) {
      text[m++] = st[v];
      if (m == 256) {
         *text = m - 1;
         fwrite(text, 256, 1, fp);
         memset(text, 0, 256);
         m = 1;
         blocks++;
      }
   }

   *pos = m;
   *nb = blocks;
}

void quick_list_headers(start, board, verbose)
int start, board, verbose;
{
   int fd, i, line = verbose ? 2 : 5, l = 0;
   char filename[80];
   struct _msghdr msgt;

   sprintf(filename,"%sMSGHDR.BBS", fido_msgpath);
   fd = shopen(filename,O_RDONLY|O_BINARY);
   if (fd == -1)
      return;

   for(i = start; i <= last_msg; i++) {
      lseek(fd,(long)sizeof(struct _msghdr) * msgidx[i],SEEK_SET);
      read(fd,(char *)&msgt,sizeof(struct _msghdr));

      if (msgt.board != board)
         continue;

      if (msgt.msgattr & Q_RECKILL)
         continue;

      if((msgt.msgattr & Q_PRIVATE) && stricmp(pascal_string(msgt.whofrom),usr.name) && stricmp(pascal_string(msgt.whoto),usr.name) && usr.priv < SYSOP)
         continue;

      msg_tzone = msgt.destzone;
      msg_fzone = msgt.origzone;

      if (verbose) {
         if ((line = quick_msg_attrib(&msgt,i,line,0)) == 0)
                 break;

         m_print(bbstxt[B_ONE_CR]);
      }
      else {
         m_print ("%-4d %c%-20.20s ",
                 i,
                 stricmp (pascal_string(msgt.whofrom), usr.name) ? 'Š' : 'Ž',
                 pascal_string(msgt.whofrom));
         m_print ("%c%-20.20s ",
                 stricmp (pascal_string(msgt.whoto), usr.name) ? 'Š' : 'Œ',
                 pascal_string(msgt.whoto));
         m_print ("%-32.32s\n", pascal_string(msgt.subject));
      }

      if ((l=more_question(line)) == 0)
              break;

      if (!verbose && l < line) {
         l = 5;

         cls ();
         m_print (bbstxt[B_LIST_AREAHEADER], usr.msg, sys.msg_name);
         m_print(bbstxt[B_LIST_HEADER1]);
         m_print(bbstxt[B_LIST_HEADER2]);
      }

      line = l;
   }

   close (fd);

   if (line)
      press_enter();
}

void quick_message_inquire (stringa, board)
char *stringa;
int board;
{
        int fd, fdt, i, line=4;
        char filename[80], *p;
        struct _msghdr msgt, backup;

        sprintf(filename,"%sMSGHDR.BBS", fido_msgpath);
        fd = shopen(filename,O_RDONLY|O_BINARY);
        if (fd == -1)
                return;

        sprintf(filename,"%sMSGTXT.BBS", fido_msgpath);
        fdt = shopen(filename,O_RDONLY|O_BINARY);
        if (fdt == -1)
                return;

        for(i = 1; i <= last_msg; i++) {
                lseek(fd,(long)sizeof(struct _msghdr) * msgidx[i],SEEK_SET);
		read(fd,(char *)&msgt,sizeof(struct _msghdr));

                if (msgt.board != board)
                        continue;

                if (msgt.msgattr & Q_RECKILL)
			continue;

		if((msgt.msgattr & Q_PRIVATE) && stricmp(pascal_string(msgt.whofrom),usr.name) && stricmp(pascal_string(msgt.whoto),usr.name) && usr.priv < SYSOP)
			continue;

/*
                p = (char *)malloc((unsigned int)msgt.numblocks * 256);
		if (p != NULL) {
                        lseek(fdt,256L*msgt.startblock,SEEK_SET);
                        read(fdt,p,(unsigned int)msgt.numblocks * 256);
                        p[(unsigned int)msgt.numblocks * 256] = '\0';
		}
		else
*/
			p = NULL;

                memcpy ((char *)&backup, (char *)&msgt, sizeof (struct _msghdr));
                strcpy (msgt.whoto, pascal_string (msgt.whoto));
                strcpy (msgt.whofrom, pascal_string (msgt.whofrom));
                strcpy (msgt.subject, pascal_string (msgt.subject));
                strupr(msgt.whoto);
                strupr(msgt.whofrom);
                strupr(msgt.subject);
                if (p != NULL)
                        strupr(p);
                msg_tzone = msgt.destzone;
                msg_fzone = msgt.origzone;

                if ((strstr(msgt.whoto,stringa) != NULL) || (strstr(msgt.whofrom,stringa) != NULL) ||
                    (strstr(msgt.subject,stringa) != NULL))
                {
                        memcpy ((char *)&msgt, (char *)&backup, sizeof (struct _msghdr));
                        if ((line = quick_msg_attrib(&msgt,i,line,0)) == 0)
                                break;

                        m_print(bbstxt[B_ONE_CR]);

                        if (!(line=more_question(line)) || !CARRIER && !RECVD_BREAK())
                                break;

                        time_release();
                }

                if (p != NULL) {
                        free(p);
                        p = NULL;
                }
        }

	close (fd);

        if (p != NULL) {
                free(p);
                p = NULL;
        }

	if (line)
		press_enter();
}

int quick_mail_header (i, line)
int i, line;
{
   int fd;
   char filename[80];
   struct _msghdr msgt;

   sprintf(filename,"%sMSGHDR.BBS", fido_msgpath);
   fd = shopen(filename,O_RDONLY|O_BINARY);
   if (fd == -1)
      return (0);

   lseek(fd,(long)sizeof(struct _msghdr) * msgidx[i],SEEK_SET);
   read(fd,(char *)&msgt,sizeof(struct _msghdr));
   close (fd);

   if (msgt.msgattr & Q_RECKILL)
      return (line);

   if((msgt.msgattr & Q_PRIVATE) && stricmp(pascal_string(msgt.whofrom),usr.name) && stricmp(pascal_string(msgt.whoto),usr.name) && usr.priv < SYSOP)
      return (line);

   msg_tzone = msgt.destzone;
   msg_fzone = msgt.origzone;
   if ((line = quick_msg_attrib(&msgt,i,line,0)) == 0)
      return (0);

   return (line);
}

int quick_kill_message (msg_num)
int msg_num;
{
   int fd, fdidx, fdto;
   char filename[80];
   struct _msghdr msgt;
   struct _msgidx idx;

   sprintf (filename, "%sMSGHDR.BBS", fido_msgpath);
   fd = open (filename, O_RDWR|O_BINARY);
   if (fd == -1)
      exit (1);

   sprintf (filename, "%sMSGIDX.BBS", fido_msgpath);
   fdidx = open(filename,O_RDWR|O_BINARY);

   sprintf (filename, "%sMSGTOIDX.BBS", fido_msgpath);
   fdto = open(filename,O_RDWR|O_BINARY);

   lseek(fd,(long)sizeof(struct _msghdr) * msgidx[msg_num],SEEK_SET);
   read(fd,(char *)&msgt,sizeof(struct _msghdr));

   if (!strnicmp (&msgt.whofrom[1], usr.name, msgt.whofrom[0]) ||
       !strnicmp (&msgt.whoto[1], usr.name, msgt.whoto[0]) || usr.priv == SYSOP) {
      msgt.msgattr |= Q_RECKILL;
      msgt.msgnum = 0;
      lseek (fd, (long)sizeof(struct _msghdr) * msgidx[msg_num], SEEK_SET);
      write (fd, (char *)&msgt, sizeof(struct _msghdr));

      lseek (fdidx, (long)msgidx[msg_num] * sizeof (struct _msgidx), SEEK_SET);
      read (fdidx, (char *)&idx, sizeof (struct _msgidx));
      idx.msgnum = 0;
      lseek (fdidx, (long)msgidx[msg_num] * sizeof (struct _msgidx), SEEK_SET);
      write (fdidx, (char *)&idx, sizeof (struct _msgidx));

      lseek (fdto, (long)msgidx[msg_num] * 36, SEEK_SET);
      write (fdto, "\x0B* Deleted *                        ", 36);

      close (fd);
      close (fdto);
      close (fdidx);

      return (1);
   }

   close (fd);
   close (fdto);
   close (fdidx);

   return (0);
}

int quick_scan_messages (total, personal, board, start, fdi, fdp, totals, fpt, qwk)
int *total, *personal, board, start, fdi, fdp, totals;
FILE *fpt;
char qwk;
{
   FILE *fp;
   float in, out;
   int i, tt, z, m, pos, blks, msgn, pr, fd;
   char c, buff[80], wrp[80], qwkbuffer[130], shead;
   byte tpbyte, lastr;
   long qpos;
   struct QWKmsghd QWK;
   struct _msghdr msgt;

   tt = 0;
   pr = 0;
   shead = 0;

   sprintf(buff,"%sMSGHDR.BBS", fido_msgpath);
   fd = shopen(buff,O_RDONLY|O_BINARY);
   if (fd == -1)
      return (totals);

   sprintf(buff,"%sMSGTXT.BBS",  fido_msgpath);
   fp = fopen(buff,"rb");
   if (fp == NULL) {
      close (fd);
      return (totals);
   }
   setvbuf (fp, NULL, _IOFBF, 1024);

   if (start > last_msg)
      start = last_msg;

   if (start == last_msg) {
      *personal = pr;
      *total = tt;

      close (fd);
      fclose (fp);

      return (totals);
   }

   for(msgn = start; msgn <= last_msg; msgn++) {
      if (!(msgn % 5))
         display_percentage (msgn - start, last_msg - start);

      lseek(fd,(long)sizeof(struct _msghdr) * msgidx[msgn],SEEK_SET);
      read(fd,(char *)&msgt,sizeof(struct _msghdr));

      if (msgt.board != board)
         continue;

      if (msgt.msgattr & Q_RECKILL)
         continue;

      if((msgt.msgattr & Q_PRIVATE) && stricmp(pascal_string(msgt.whofrom),usr.name) && stricmp(pascal_string(msgt.whoto),usr.name) && usr.priv < SYSOP)
         continue;

      totals++;
      if (fdi != -1) {
         sprintf(buff,"%u",totals);   /* Stringized version of current position */
         in = (float) atof(buff);
         out = IEEToMSBIN(in);
         write(fdi,&out,sizeof(float));

         c = 0;
         write(fdi,&c,sizeof(char));              /* Conference # */
      }

      if(!stricmp(pascal_string(msgt.whoto),usr.name)) {
         pr++;
         if (fdp != -1) {
             write(fdp,&out,sizeof(float));
             write(fdp,&c,sizeof(char));              /* Conference # */
         }
      }

      fseek(fp,256L*msgt.startblock,SEEK_SET);

      tpbyte = 0;
      lastr = 255;
      blks = 1;
      pos = 0;
      memset (qwkbuffer, ' ', 128);
      i = 0;
      shead = 0;

      for (;;) {
         if (!tpbyte) {
            if (lastr != 255)
               break;
            tpbyte = fgetc (fp);
            lastr = tpbyte;
            if (!tpbyte)
               break;
         }

         c = fgetc(fp);
         tpbyte--;

         if (c == '\0')
            break;

         if (c == 0x0A || c == '\0')
            continue;

         buff[i++]=c;

         if(c == 0x0D || (byte)c == 0x8D) {
            buff[i-1]='\0';

            if(buff[0] == 0x01) {
               if (!strncmp(&buff[1],"INTL",4) && !shead)
                  sscanf(&buff[6],"%d:%d/%d %d:%d/%d",&msg_tzone,&i,&i,&msg_fzone,&i,&i);
               if (!strncmp(&buff[1],"TOPT",4) && !shead)
                  sscanf(&buff[6],"%d",&msg_tpoint);
               if (!strncmp(&buff[1],"FMPT",4) && !shead)
                  sscanf(&buff[6],"%d",&msg_fpoint);
               i=0;
               continue;
            }
            else if (!shead) {
               if (qwk)
                  quick_qwk_text_header (&msgt,msgn,fpt,&QWK,&qpos);
               else
                  quick_text_header (&msgt,msgn,fpt);
               shead = 1;
            }

            if(buff[0] == 0x01 || !strncmp(buff,"SEEN-BY",7)) {
               i=0;
               continue;
            }

            if (qwk) {
               write_qwk_string (buff, qwkbuffer, &pos, &blks, fpt);
               write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpt);
            }
            else
               fprintf(fpt,"%s\n",buff);

            i = 0;
         }
         else {
            if(i < (usr.width-2))
               continue;

            buff[i]='\0';
            while(i > 0 && buff[i] != ' ')
               i--;

            m = 0;

            if(i != 0)
               for(z=i+1;buff[z];z++)
                  wrp[m++]=buff[z];

            buff[i]='\0';
            wrp[m]='\0';

            if (!shead) {
               if (qwk)
                  quick_qwk_text_header (&msgt,msgn,fpt,&QWK,&qpos);
               else
                  quick_text_header (&msgt,msgn,fpt);
               shead = 1;
            }

            if (qwk) {
               write_qwk_string (buff, qwkbuffer, &pos, &blks, fpt);
               write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpt);
            }
            else
               fprintf(fpt,"%s\n",buff);

            strcpy(buff,wrp);
            i = strlen(buff);
         }
      }

      if (qwk) {
         qwkbuffer[128] = 0;
         fwrite(qwkbuffer, 128, 1, fpt);
         blks++;

         fseek(fpt,qpos,SEEK_SET);          /* Restore back to header start */
         sprintf(buff,"%d",blks);
         ljstring(QWK.Msgrecs,buff,6);
         fwrite((char *)&QWK,128,1,fpt);           /* Write out the header */
         fseek(fpt,0L,SEEK_END);               /* Bump back to end of file */
      }
      else
         fprintf(fpt,bbstxt[B_TWO_CR]);

      tt++;
      totals += blks - 1;
   }

   fclose(fp);
   close (fd);

   *personal = pr;
   *total = tt;

   return (totals);
}

int quick_save_message2 (txt, f1, f2, f3, f4, f5)
FILE *txt, *f1, *f2, *f3, *f4, *f5;
{
   FILE *fp;
   word i, dest, m, nb, x, hh, mm, dd, mo, aa;
   char text[258], buffer[258];
   struct _msgidx idx;
   struct _msghdr hdr;
   struct _msgtoidx toidx;

   memset ((char *)&idx, 0, sizeof (struct _msgidx));
   memset ((char *)&hdr, 0, sizeof (struct _msghdr));
   memset ((char *)&toidx, 0, sizeof (struct _msgtoidx));

   if (f5);
//   dest = (int)(ftell (f1) / sizeof (struct _msgidx));
   dest = msginfo.highmsg + 1;
   if (msginfo.highmsg > dest) {
      status_line ("!QuickBBS exceeds 65535 messages");
      return (0);
   }

   idx.msgnum = dest;
   idx.board = sys.quick_board;
   if (fwrite((char *)&idx,sizeof(struct _msgidx),1,f1) != 1)
      return (0);

   strcpy(&toidx.string[1],msg.to);
   toidx.string[0] = strlen(msg.to);
   if (fwrite((char *)&toidx,sizeof(struct _msgtoidx),1,f2) != 1)
      return (0);

   fp = f3;

   i = (word)(ftell(fp) / 256L);
   hdr.startblock = i;

   i = 0;
   m = 1;
   nb = 1;

   memset(text, 0, 256);

   do {
      memset (buffer, 0, 256);
      i = mread(buffer, 1, 255, txt);
      for (x = 0; x < i; x++) {
         if (buffer[x] == 0x1A)
            buffer[x] = ' ';
      }
      buffer[i] = '\0';
      write_pascal_string (buffer, text, &m, &nb, fp);
   } while (i == 255);

   *text = m - 1;
   if (fwrite(text, 256, 1, fp) != 1)
      return (0);

   hdr.numblocks = nb;
   hdr.msgnum = dest;
   hdr.prevreply = msg.reply;
   hdr.nextreply = msg.up;
   hdr.timesread = 0;
   hdr.destnet = msg.dest_net;
   hdr.destnode = msg.dest;
   hdr.orignet = msg.orig_net;
   hdr.orignode = msg.orig;
   hdr.destzone = msg_tzone;
   hdr.origzone = msg_fzone;
   hdr.cost = msg.cost;

   if (sys.public)
      msg.attr &= ~MSGPRIVATE;
   else if (sys.private)
      msg.attr |= MSGPRIVATE;

   if(msg.attr & MSGPRIVATE)
      hdr.msgattr |= Q_PRIVATE;

   if (sys.echomail && (msg.attr & MSGSENT))
      hdr.msgattr &= ~Q_LOCAL;
   else {
      hdr.msgattr &= ~Q_LOCAL;
      hdr.msgattr |= 32;
   }
   hdr.board = sys.quick_board;

   sscanf(msg.date, "%2d %3s %2d %2d:%2d", &dd, buffer, &aa, &hh, &mm);
   buffer[3] = '\0';
   for (mo = 0; mo < 12; mo++)
      if (!stricmp(buffer, mtext[mo]))
         break;
   if (mo == 12)
      mo = 0;
   sprintf(&hdr.time[1],"%02d:%02d",hh,mm);
   sprintf(&hdr.date[1],"%02d-%02d-%02d",mo+1,dd,aa);
   hdr.time[0] = 5;
   hdr.date[0] = 8;

   strcpy(&hdr.whoto[1],msg.to);
   hdr.whoto[0] = strlen(msg.to);
   strcpy(&hdr.whofrom[1],msg.from);
   hdr.whofrom[0] = strlen(msg.from);
   strcpy(&hdr.subject[1],msg.subj);
   hdr.subject[0] = strlen(msg.subj);

   msginfo.totalonboard[sys.quick_board-1]++;
   msginfo.totalmsgs++;
   msginfo.highmsg++;

   if (fwrite((char *)&hdr,sizeof(struct _msghdr),1,f4) != 1)
      return (0);

   last_msg++;
   return (1);
}

int quick_export_mail (maxnodes, forward)
int maxnodes;
struct _fwrd *forward;
{
   FILE *fpd, *f2, *fp;
   int finfo, f1, i, pp, z, fd, ne, no, m, n_seen, cnet, cnode, mi, sent, ai;
   char buff[80], wrp[80], c, last_board, found, *p, buffer[2050];
   char *location, *tag, *forw, need_origin, need_seen;
   byte tpbyte, lastr;
   struct _msghdr msghdr;
   struct _msghdr2 mhdr;
   struct _fwrd *seen;
   struct _msginfo msginfo;

   sprintf(buff, "%sMSGINFO.BBS", fido_msgpath);
   finfo = sh_open (buff, SH_DENYWR, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
   if(finfo == -1)
      return (maxnodes);
   read (finfo, (char *)&msginfo, sizeof (struct _msginfo));

   sprintf(buff, "%sMSGHDR.BBS", fido_msgpath);
   f1 = sh_open(buff, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
   if(f1 == -1) {
      close (finfo);
      return (maxnodes);
   }

   msginfo.totalmsgs = (word)(filelength (f1) / sizeof (struct _msghdr));

   sprintf(buff,"%sMSGTXT.BBS",fido_msgpath);
   i = sh_open (buff, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
   if (i == -1) {
      close (f1);
      close (finfo);
      return (maxnodes);
   }
   f2 = fdopen (i,"rb");
   if (f2 == NULL) {
      close (i);
      close (f1);
      close (finfo);
      return (maxnodes);
   }
   setvbuf (f2, NULL, _IOFBF, 1024);

   sprintf (buff, SYSMSG_PATH, config->sys_path);
   fd = sh_open (buff, SH_DENYNONE, O_RDONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
   if (fd == -1) {
      close (f1);
      fclose (f2);
      close (finfo);
      return (maxnodes);
   }

   if (config->use_areasbbs)
      fp = fopen (config->areas_bbs, "rt");
   else
      fp = NULL;

   last_board = 0;
   seen = (struct _fwrd *)malloc ((MAX_EXPORT_SEEN + 1) * sizeof (struct _fwrd));
   if (seen == NULL) {
      close (f1);
      fclose (f2);
      close (fd);
      close (finfo);
      if (fp != NULL)
         fclose (fp);
      return (maxnodes);
   }

   fpd = fopen ("MSGTMP.EXP", "rb+");
   if (fpd == NULL) {
      fpd = fopen ("MSGTMP.EXP", "wb");
      fclose (fpd);
   }
   else
      fclose (fpd);
   fpd = mopen ("MSGTMP.EXP", "r+b");
//   setvbuf (fpd, NULL, _IOFBF, 8192);

   prints (7, 65, YELLOW|_BLACK, "              ");
   prints (8, 65, YELLOW|_BLACK, "N/A           ");
   prints (9, 65, YELLOW|_BLACK, "              ");
   prints (9, 65, YELLOW|_BLACK, "QuickBBS");

   sent = 0;

   while (read(f1, (char *)&msghdr,sizeof(struct _msghdr)) == sizeof (struct _msghdr)) {
      sprintf (wrp, "%d / %d", msghdr.msgnum, msginfo.totalmsgs);
      prints (7, 65, YELLOW|_BLACK, wrp);

      if ( !(msghdr.msgnum % 30))
         time_release ();

      if (!(msghdr.msgattr & 32))
         continue;
      if (msghdr.msgattr & Q_RECKILL)
         continue;

      if (msghdr.board != last_board) {
         if (sent && last_board) {
            status_line (":   %-20.20s (Sent=%04d)", sys.echotag, sent);
            sent = 0;
         }

         last_board = msghdr.board;
         memset ((char *)&sys, 0, sizeof (struct _sys));
         found = 0;

         if (fp != NULL) {
            rewind (fp);
            fgets(buffer, 255, fp);

            while (fgets(buffer, 255, fp) != NULL) {
               if (buffer[0] == ';' || !isdigit (buffer[0]))
                  continue;
               while (buffer[strlen (buffer) -1] == 0x0D || buffer[strlen (buffer) -1] == 0x0A || buffer[strlen (buffer) -1] == ' ')
                  buffer[strlen (buffer) -1] = '\0';
               location = strtok (buffer, " ");
               tag = strtok (NULL, " ");
               forw = strtok (NULL, "");
               if (atoi (location) == msghdr.board) {
                  while (*forw == ' ')
                     forw++;
                  memset ((char *)&sys, 0, sizeof (struct _sys));
                  strcpy (sys.echotag, tag);
                  if (forw != NULL)
                     strcpy (sys.forward1, forw);
                  sys.quick_board = atoi (location);
                  sys.echomail = 1;
                  sys.use_alias = 0;
                  found = 1;
                  break;
               }
            }
         }

         if (!found) {
            for (i = 0; i < maxaidx; i++) {
               if (aidx[i].board == msghdr.board)
                  break;
            }

            if (i < maxaidx) {
               lseek (fd, (long)i * SIZEOF_MSGAREA, SEEK_SET);
               read(fd, (char *)&sys.msg_name, SIZEOF_MSGAREA);
               found = 1;
            }
         }

         if (!found)
            continue;

         for (i = 0; i < maxnodes; i++)
            forward[i].reset = forward[i].export = 0;

         if (!sys.use_alias) {
            z = config->alias[sys.use_alias].zone;
            parse_netnode2 (sys.forward1, &z, &ne, &no, &pp);
            if (z != config->alias[sys.use_alias].zone) {
               for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++)
                  if (z == config->alias[i].zone)
                     break;
               if (i < MAX_ALIAS && config->alias[i].net)
                  sys.use_alias = i;
            }
         }

         z = config->alias[sys.use_alias].zone;
         ne = config->alias[sys.use_alias].net;
         no = config->alias[sys.use_alias].node;
         pp = 0;

         p = strtok (sys.forward1, " ");
         if (p != NULL)
            do {
               parse_netnode2 (p, &z, &ne, &no, &pp);
               for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++) {
                  if (config->alias[i].zone == z && config->alias[i].net == ne && config->alias[i].node == no && config->alias[i].point == pp)
                     break;
                  if (config->alias[i].point && config->alias[i].fakenet) {
                     if (config->alias[i].zone == z && config->alias[i].fakenet == ne && config->alias[i].point == no)
                        break;
                  }
               }
               if (i < MAX_ALIAS && config->alias[i].net)
                  continue;
               if ((i = is_here (z, ne, no, pp, forward, maxnodes)) != -1)
                  forward[i].reset = forward[i].export = 1;
               else {
                  forward[maxnodes].zone = z;
                  forward[maxnodes].net = ne;
                  forward[maxnodes].node = no;
                  forward[maxnodes].point = pp;
                  forward[maxnodes].export = 1;
                  forward[maxnodes].reset = 1;
                  maxnodes++;
               }
            } while ((p = strtok (NULL, " ")) != NULL);

         if (!sys.use_alias) {
            z = config->alias[sys.use_alias].zone;
            parse_netnode2 (sys.forward2, &z, &ne, &no, &pp);
            if (z != config->alias[sys.use_alias].zone) {
               for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++)
                  if (z == config->alias[i].zone)
                     break;
               if (i < MAX_ALIAS && config->alias[i].net)
                  sys.use_alias = i;
            }
         }

         z = config->alias[sys.use_alias].zone;
         ne = config->alias[sys.use_alias].net;
         no = config->alias[sys.use_alias].node;
         pp = 0;

         p = strtok (sys.forward2, " ");
         if (p != NULL)
            do {
               parse_netnode2 (p, &z, &ne, &no, &pp);
               for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++) {
                  if (config->alias[i].zone == z && config->alias[i].net == ne && config->alias[i].node == no && config->alias[i].point == pp)
                     break;
                  if (config->alias[i].point && config->alias[i].fakenet) {
                     if (config->alias[i].zone == z && config->alias[i].fakenet == ne && config->alias[i].point == no)
                        break;
                  }
               }
               if (i < MAX_ALIAS && config->alias[i].net)
                  continue;
               if ((i = is_here (z, ne, no, pp, forward, maxnodes)) != -1)
                  forward[i].reset = forward[i].export = 1;
               else {
                  forward[maxnodes].zone = z;
                  forward[maxnodes].net = ne;
                  forward[maxnodes].node = no;
                  forward[maxnodes].point = pp;
                  forward[maxnodes].export = 1;
                  forward[maxnodes].reset = 1;
                  maxnodes++;
               }
            } while ((p = strtok (NULL, " ")) != NULL);

         if (!sys.use_alias) {
            z = config->alias[sys.use_alias].zone;
            parse_netnode2 (sys.forward3, &z, &ne, &no, &pp);
            if (z != config->alias[sys.use_alias].zone) {
               for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++)
                  if (z == config->alias[i].zone)
                     break;
               if (i < MAX_ALIAS && config->alias[i].net)
                  sys.use_alias = i;
            }
         }

         z = config->alias[sys.use_alias].zone;
         ne = config->alias[sys.use_alias].net;
         no = config->alias[sys.use_alias].node;
         pp = 0;

         p = strtok (sys.forward3, " ");
         if (p != NULL)
            do {
               parse_netnode2 (p, &z, &ne, &no, &pp);
               for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++) {
                  if (config->alias[i].zone == z && config->alias[i].net == ne && config->alias[i].node == no && config->alias[i].point == pp)
                     break;
                  if (config->alias[i].point && config->alias[i].fakenet) {
                     if (config->alias[i].zone == z && config->alias[i].fakenet == ne && config->alias[i].point == no)
                        break;
                  }
               }
               if (i < MAX_ALIAS && config->alias[i].net)
                  continue;
               if ((i = is_here (z, ne, no, pp, forward, maxnodes)) != -1)
                  forward[i].reset = forward[i].export = 1;
               else {
                  forward[maxnodes].zone = z;
                  forward[maxnodes].net = ne;
                  forward[maxnodes].node = no;
                  forward[maxnodes].point = pp;
                  forward[maxnodes].export = 1;
                  forward[maxnodes].reset = 1;
                  maxnodes++;
               }
            } while ((p = strtok (NULL, " ")) != NULL);
      }
      else if (!found)
         continue;

      for (i = 0; i < maxnodes; i++)
         if (forward[i].reset)
            forward[i].export = 1;

      need_seen = need_origin = 1;

      strcpy (wrp, sys.echotag);
      wrp[14] = '\0';
      prints (8, 65, YELLOW|_BLACK, "              ");
      prints (8, 65, YELLOW|_BLACK, wrp);

      sprintf (wrp, "%5d  %-22.22s QuickBBS     ", msghdr.msgnum, sys.echotag);
      wputs (wrp);

      mseek (fpd, 0L, SEEK_SET);
//      chsize (fileno (fpd), 0L);
      mprintf (fpd, "AREA:%s\r\n", sys.echotag);

//      setvbuf (f2, NULL, _IOFBF, 5);
//      fpos = 256L*(msghdr.startblock+msghdr.numblocks-1);
//      fseek(f2,fpos,SEEK_SET);
//      fpos += fgetc(f2);

      fseek(f2,256L*msghdr.startblock,SEEK_SET);
//      if (setvbuf (f2, NULL, _IOFBF, 4096) != 0)
//         status_line (">Mem err: setvbuf");

      mi = i = 0;
      tpbyte = 0;
      lastr = 255;
      n_seen = 0;

//      while(ftell(f2) < fpos) {
      for (;;) {
         if (!tpbyte) {
            if (lastr != 255)
               break;
            tpbyte = fgetc(f2);
            lastr = tpbyte;
            if (!tpbyte)
               break;
         }

         c = fgetc(f2);
         tpbyte--;

         if (c == '\0')
            break;

         if (c == 0x0A)
            continue;
         if((byte)c == 0x8D)
            c = ' ';

         buff[mi++] = c;

         if(c == 0x0D) {
            buff[mi - 1]='\0';
            if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
               need_origin = 0;

            if (!strncmp (buff, "SEEN-BY: ", 9)) {
               if (need_origin) {
                  mprintf (fpd, msgtxt[M_TEAR_LINE],VERSION, registered ? "+" : NOREG);
                  if (strlen(sys.origin))
                     mprintf(fpd,msgtxt[M_ORIGIN_LINE],random_origins(),config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
                  else
                     mprintf(fpd,msgtxt[M_ORIGIN_LINE],system_name,config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
                  need_origin = 0;
               }

               p = strtok (buff, " ");
               ne = config->alias[sys.use_alias].net;
               no = config->alias[sys.use_alias].node;
               pp = config->alias[sys.use_alias].point;
               z = config->alias[sys.use_alias].zone;

               while ((p = strtok (NULL, " ")) != NULL) {
                  parse_netnode2 (p, &z, &ne, &no, &pp);

                  seen[n_seen].net = ne;
                  seen[n_seen].node = no;
                  seen[n_seen].point = pp;
                  seen[n_seen].zone = z;

                  for (i = 0; i < maxnodes; i++) {
                     if (forward[i].net == seen[n_seen].net && forward[i].node == seen[n_seen].node && forward[i].point == seen[n_seen].point) {
                        forward[i].export = 0;
                        break;
                     }
                  }

                  if (n_seen + maxnodes >= MAX_EXPORT_SEEN)
                     continue;

                  if (seen[n_seen].net != config->alias[sys.use_alias].fakenet && !seen[n_seen].point)
                     n_seen++;
               }
            }
            else if (!strncmp (buff, "\001PATH: ", 7) && need_seen) {
               if (need_origin) {
                  mprintf (fpd, msgtxt[M_TEAR_LINE],VERSION, registered ? "+" : NOREG);
                  if (strlen(sys.origin))
                     mprintf(fpd,msgtxt[M_ORIGIN_LINE],random_origins(),config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
                  else
                     mprintf(fpd,msgtxt[M_ORIGIN_LINE],system_name,config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
                  need_origin = 0;
               }

               if (!config->alias[sys.use_alias].point) {
                  seen[n_seen].net = config->alias[sys.use_alias].net;
                  seen[n_seen++].node = config->alias[sys.use_alias].node;
               }
               else if (config->alias[sys.use_alias].fakenet) {
                  seen[n_seen].net = config->alias[sys.use_alias].fakenet;
                  seen[n_seen++].node = config->alias[sys.use_alias].point;
               }

               for (i = 0; i < maxnodes; i++) {
                  if (!forward[i].export || forward[i].net == config->alias[sys.use_alias].fakenet || forward[i].point)
                     continue;
                  seen[n_seen].net = forward[i].net;
                  seen[n_seen++].node = forward[i].node;

                  ai = sys.use_alias;
                  for (m = 0; m < maxakainfo; m++)
                     if (akainfo[m].zone == forward[i].zone && akainfo[m].net == forward[i].net && akainfo[m].node == forward[i].node && akainfo[m].point == forward[i].point) {
                        if (akainfo[m].aka)
                           ai = akainfo[m].aka - 1;
                        break;
                     }
                  if (ai != sys.use_alias) {
                     if (!config->alias[ai].point) {
                        if (config->alias[ai].net != seen[n_seen - 1].net || config->alias[ai].node != seen[n_seen - 1].node) {
                           seen[n_seen].net = config->alias[ai].net;
                           seen[n_seen++].node = config->alias[ai].node;
                        }
                     }
                     else if (config->alias[ai].fakenet) {
                        if (config->alias[ai].fakenet != seen[n_seen - 1].net || config->alias[ai].point != seen[n_seen - 1].node) {
                           seen[n_seen].net = config->alias[ai].fakenet;
                           seen[n_seen++].node = config->alias[ai].point;
                        }
                     }
                  }
               }

               qsort (seen, n_seen, sizeof (struct _fwrd), mail_sort_func);

               cnet = cnode = 0;
               strcpy (wrp, "SEEN-BY: ");

               for (i = 0; i < n_seen; i++) {
                  if (strlen (wrp) > 65) {
                     mprintf (fpd, "%s\r\n", wrp);
                     cnet = cnode = 0;
                     strcpy (wrp, "SEEN-BY: ");
                  }
                  if (i && seen[i].net == seen[i - 1].net && seen[i].node == seen[i - 1].node)
                     continue;

                  if (cnet != seen[i].net) {
                     sprintf (buffer, "%d/%d ", seen[i].net, seen[i].node);
                     cnet = seen[i].net;
                     cnode = seen[i].node;
                  }
                  else if (cnet == seen[i].net && cnode != seen[i].node) {
                     sprintf (buffer, "%d ", seen[i].node);
                     cnode = seen[i].node;
                  }
                  else
                     strcpy (buffer, "");

                  strcat (wrp, buffer);
               }

               mprintf (fpd, "%s\r\n", wrp);
               mprintf (fpd, "%s\r\n", buff);

               need_seen = 0;
            }
            else if ((msghdr.msgattr & 64) && !strncmp (buff, "--- ", 4) && (config->replace_tear || !registered))
               replace_tearline (fpd, buff);
            else
               mprintf (fpd, "%s\r\n", buff);

            mi = 0;
         }
         else {
            if(mi < 78)
               continue;

            if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
               need_origin = 0;

            buff[mi]='\0';
            mprintf(fpd,"%s",buff);
            mi=0;
            buff[mi] = '\0';
         }
      }

      if (!n_seen) {
         if (need_origin) {
            mprintf (fpd, msgtxt[M_TEAR_LINE],VERSION, registered ? "+" : NOREG);
            if (strlen(sys.origin))
               mprintf(fpd,msgtxt[M_ORIGIN_LINE],random_origins(),config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
            else
               mprintf(fpd,msgtxt[M_ORIGIN_LINE],system_name,config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
            need_origin = 0;
         }

         if (!config->alias[sys.use_alias].point) {
            seen[n_seen].net = config->alias[sys.use_alias].net;
            seen[n_seen++].node = config->alias[sys.use_alias].node;
         }
         else if (config->alias[sys.use_alias].fakenet) {
            seen[n_seen].net = config->alias[sys.use_alias].fakenet;
            seen[n_seen++].node = config->alias[sys.use_alias].point;
         }

         ai = sys.use_alias;
         for (i = 0; i < maxnodes; i++) {
            if (!forward[i].export || forward[i].net == config->alias[sys.use_alias].fakenet)
               continue;
            seen[n_seen].net = forward[i].net;
            seen[n_seen++].node = forward[i].node;

            for (m = 0; m < maxakainfo; m++)
               if (akainfo[m].zone == forward[i].zone && akainfo[m].net == forward[i].net && akainfo[m].node == forward[i].node && akainfo[m].point == forward[i].point) {
                  if (akainfo[m].aka)
                     ai = akainfo[m].aka - 1;
                  break;
               }
            if (ai != sys.use_alias) {
               if (!config->alias[ai].point) {
                  if (config->alias[ai].net != seen[n_seen - 1].net || config->alias[ai].node != seen[n_seen - 1].node) {
                     seen[n_seen].net = config->alias[ai].net;
                     seen[n_seen++].node = config->alias[ai].node;
                  }
               }
               else if (config->alias[ai].fakenet) {
                  if (config->alias[ai].fakenet != seen[n_seen - 1].net || config->alias[ai].point != seen[n_seen - 1].node) {
                     seen[n_seen].net = config->alias[ai].fakenet;
                     seen[n_seen++].node = config->alias[ai].point;
                  }
               }
            }
         }

         qsort (seen, n_seen, sizeof (struct _fwrd), mail_sort_func);

         cnet = cnode = 0;
         strcpy (wrp, "SEEN-BY: ");

         for (i = 0; i < n_seen; i++) {
            if (cnet != seen[i].net) {
               sprintf (buffer, "%d/%d ", seen[i].net, seen[i].node);
               cnet = seen[i].net;
               cnode = seen[i].node;
            }
            else if (cnet == seen[i].net && cnode != seen[i].node) {
               sprintf (buffer, "%d ", seen[i].node);
               cnode = seen[i].node;
            }
            else
               strcpy (buffer, "");

            strcat (wrp, buffer);
         }

         mprintf (fpd, "%s\r\n", wrp);
         if (config->alias[sys.use_alias].point && config->alias[sys.use_alias].fakenet)
            mprintf (fpd, "\001PATH: %d/%d\r\n", config->alias[sys.use_alias].fakenet, config->alias[sys.use_alias].point);
         else if (!config->alias[sys.use_alias].point)
//            mprintf (fpd, "\001PATH: %d/%d.%d\r\n", config->alias[sys.use_alias].net, config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
//         else
            mprintf (fpd, "\001PATH: %d/%d\r\n", config->alias[sys.use_alias].net, config->alias[sys.use_alias].node);
      }

      mhdr.ver = PKTVER;
      mhdr.cost = msghdr.cost;
      mhdr.attrib = 0;

      if (sys.public)
         mhdr.attrib &= ~Q_PRIVATE;
      else if (sys.private)
         mhdr.attrib |= Q_PRIVATE;

      if (msghdr.msgattr & Q_PRIVATE)
         mhdr.attrib |= MSGPRIVATE;

      for (i = 0; i < maxnodes; i++) {
         if (!forward[i].export)
            continue;

         ai = sys.use_alias;
         for (m = 0; m < maxakainfo; m++)
            if (akainfo[m].zone == forward[i].zone && akainfo[m].net == forward[i].net && akainfo[m].node == forward[i].node && akainfo[m].point == forward[i].point) {
               if (akainfo[m].aka)
                  ai = akainfo[m].aka - 1;
               break;
            }

         if (config->alias[ai].point && config->alias[ai].fakenet) {
            mhdr.orig_node = config->alias[ai].point;
            mhdr.orig_net = config->alias[ai].fakenet;
         }
         else {
            mhdr.orig_node = config->alias[ai].node;
            mhdr.orig_net = config->alias[ai].net;
         }

         if (forward[i].point)
            sprintf (wrp, "%d/%d.%d ", forward[i].net, forward[i].node, forward[i].point);
         else
            sprintf (wrp, "%d/%d ", forward[i].net, forward[i].node);
         wreadcur (&z, &m);
         if ( (m + strlen (wrp)) > 78) {
            wputs ("\n");
            wputs ("                                           ");
         }
         wputs (wrp);
         if ( (m + strlen (wrp)) == 78)
            wputs ("                                           ");

         mi = open_packet (forward[i].zone, forward[i].net, forward[i].node, forward[i].point, ai);

         mhdr.dest_net = forward[i].net;
         mhdr.dest_node = forward[i].node;
         write (mi, (char *)&mhdr, sizeof (struct _msghdr2));

         strcpy(wrp,pascal_string(msghdr.date));
         sscanf(wrp,"%2d-%2d-%2d",&z,&ne,&no);
         strcpy(wrp,pascal_string(msghdr.time));
         sscanf(wrp,"%2d:%2d",&pp,&m);
         sprintf (buff, "%02d %s %02d  %2d:%02d:00", ne, mtext[z-1], no, pp, m);
         write (mi, buff, strlen (buff) + 1);
         strcpy (buff, pascal_string (msghdr.whoto));
         write (mi, buff, strlen (buff) + 1);
         strcpy (buff, pascal_string (msghdr.whofrom));
         write (mi, buff, strlen (buff) + 1);
         strcpy (buff, pascal_string (msghdr.subject));
         write (mi, buff, strlen (buff) + 1);
         mseek (fpd, 0L, SEEK_SET);
         do {
            z = mread(buffer, 1, 2048, fpd);
            write(mi, buffer, z);
         } while (z == 2048);
         buff[0] = buff[1] = buff[2] = 0;
         if (write (mi, buff, 3) != 3)
            return (-1);
         close (mi);

         totalmsg++;
         sprintf (wrp, "%d (%.1f/s) ", totalmsg, (float)totalmsg / ((float)(timerset (0) - totaltime) / 100));
         prints (10, 65, YELLOW|_BLACK, wrp);
      }

      lseek (f1, -1L * sizeof (struct _msghdr), SEEK_CUR);
      msghdr.msgattr &= ~32;
      write(f1, (char *)&msghdr, sizeof(struct _msghdr));

      wputs ("\n");
      sent++;
   }

   if (sent)
      status_line (":   %-20.20s (Sent=%04d)", sys.echotag, sent);

   if (fp != NULL)
      fclose (fp);
   mclose (fpd);
   free (seen);
   close (fd);
   fclose (f2);
   close (f1);
   close (finfo);
   unlink ("MSGTMP.EXP");

   return (maxnodes);
}


int quick_rescan_echomail (int board, int zone, int net, int node, int point)
{
   FILE *fpd, *f2, *fp;
   int finfo, f1, i, pp, z, fd, ne, no, m, n_seen, cnet, cnode, mi, sent, ai = 0;
   char buff[80], wrp[80], c, last_board, found, *p, buffer[2050];
   char *location, *tag, *forw, need_origin, need_seen;
   byte tpbyte, lastr;
   struct _msghdr msghdr;
   struct _msghdr2 mhdr;
   struct _fwrd *seen;
   struct _msginfo msginfo;
   NODEINFO ni;

   sprintf (buff, "%sNODES.DAT", config->net_info);
   if ((fd = sh_open (buff, SH_DENYWR, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) != -1) {
      while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO))
         if (zone == ni.zone && net == ni.net && node == ni.node && point == ni.point) {
            ai = ni.aka;
            break;
         }

      close (fd);
   }

   sprintf(buff, "%sMSGINFO.BBS", fido_msgpath);
   finfo = sh_open (buff, SH_DENYWR, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
   if(finfo == -1)
      return (0);
   read (finfo, (char *)&msginfo, sizeof (struct _msginfo));

   sprintf(buff, "%sMSGHDR.BBS", fido_msgpath);
   f1 = open(buff, O_RDWR|O_BINARY);
   if(f1 == -1) {
      close (finfo);
      return (0);
   }

   msginfo.totalmsgs = (word)(filelength (f1) / sizeof (struct _msghdr));

   sprintf(buff,"%sMSGTXT.BBS",fido_msgpath);
   f2 = fopen(buff,"rb");
   if (f2 == NULL) {
      close (f1);
      close (finfo);
      return (0);
   }
   setvbuf (f2, NULL, _IOFBF, 1024);

   sprintf (buff, SYSMSG_PATH, config->sys_path);
   fd = sh_open (buff, SH_DENYNONE, O_RDONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
   if (fd == -1) {
      close (f1);
      fclose (f2);
      close (finfo);
      return (0);
   }

   if (config->use_areasbbs)
      fp = fopen (config->areas_bbs, "rt");
   else
      fp = NULL;

   last_board = 0;
   seen = (struct _fwrd *)malloc ((MAX_EXPORT_SEEN + 1) * sizeof (struct _fwrd));
   if (seen == NULL) {
      close (f1);
      fclose (f2);
      close (fd);
      close (finfo);
      if (fp != NULL)
         fclose (fp);
      return (0);
   }

   fpd = fopen ("MSGTMP.EXP", "rb+");
   if (fpd == NULL) {
      fpd = fopen ("MSGTMP.EXP", "wb");
      fclose (fpd);
   }
   else
      fclose (fpd);
   fpd = mopen ("MSGTMP.EXP", "r+b");

   prints (9, 65, YELLOW|_BLACK, "              ");
   prints (9, 65, YELLOW|_BLACK, "QuickBBS");

   sent = 0;

   while (read(f1, (char *)&msghdr,sizeof(struct _msghdr)) == sizeof (struct _msghdr)) {
      sprintf (wrp, "%d / %d", msghdr.msgnum, msginfo.totalmsgs);
      prints (7, 65, YELLOW|_BLACK, wrp);

      if ( !(msghdr.msgnum % 30))
         time_release ();

      if (msghdr.msgattr & Q_RECKILL)
         continue;
      if (msghdr.board != board)
         continue;

      if (msghdr.board != last_board) {
         last_board = msghdr.board;
         memset ((char *)&sys, 0, sizeof (struct _sys));
         found = 0;

         if (fp != NULL) {
            rewind (fp);
            fgets(buffer, 255, fp);

            while (fgets(buffer, 255, fp) != NULL) {
               if (buffer[0] == ';' || !isdigit (buffer[0]))
                  continue;
               while (buffer[strlen (buffer) -1] == 0x0D || buffer[strlen (buffer) -1] == 0x0A || buffer[strlen (buffer) -1] == ' ')
                  buffer[strlen (buffer) -1] = '\0';
               location = strtok (buffer, " ");
               tag = strtok (NULL, " ");
               forw = strtok (NULL, "");
               if (atoi (location) == msghdr.board) {
                  while (*forw == ' ')
                     forw++;
                  memset ((char *)&sys, 0, sizeof (struct _sys));
                  strcpy (sys.echotag, tag);
                  if (forw != NULL)
                     strcpy (sys.forward1, forw);
                  sys.quick_board = atoi (location);
                  sys.echomail = 1;
                  sys.use_alias = 0;
                  found = 1;
                  break;
               }
            }
         }

         if (!found) {
            for (i = 0; i < maxaidx; i++) {
               if (aidx[i].board == msghdr.board)
                  break;
            }

            if (i < maxaidx) {
               lseek (fd, (long)i * SIZEOF_MSGAREA, SEEK_SET);
               read(fd, (char *)&sys.msg_name, SIZEOF_MSGAREA);
               found = 1;
            }
         }

         if (!found)
            continue;

         if (!sys.use_alias) {
            z = config->alias[sys.use_alias].zone;
            if (zone != config->alias[sys.use_alias].zone) {
               for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++)
                  if (z == config->alias[i].zone)
                     break;
               if (i < MAX_ALIAS && config->alias[i].net)
                  sys.use_alias = i;
            }
         }
      }
      else if (!found)
         continue;

      need_seen = need_origin = 1;

      strcpy (wrp, sys.echotag);
      wrp[14] = '\0';
      prints (8, 65, YELLOW|_BLACK, "              ");
      prints (8, 65, YELLOW|_BLACK, wrp);

      sprintf (wrp, "%5d  %-22.22s QuickBBS     ", msghdr.msgnum, sys.echotag);
      wputs (wrp);

      mseek (fpd, 0L, SEEK_SET);
      mprintf (fpd, "AREA:%s\r\n", sys.echotag);

      fseek(f2,256L*msghdr.startblock,SEEK_SET);

      mi = i = 0;
      tpbyte = 0;
      lastr = 255;
      n_seen = 0;

      for (;;) {
         if (!tpbyte) {
            if (lastr != 255)
               break;
            tpbyte = fgetc(f2);
            lastr = tpbyte;
            if (!tpbyte)
               break;
         }

         c = fgetc(f2);
         tpbyte--;

         if (c == '\0')
            break;

         if (c == 0x0A)
            continue;
         if((byte)c == 0x8D)
            c = ' ';

         buff[mi++] = c;

         if(c == 0x0D) {
            buff[mi - 1]='\0';
            if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
               need_origin = 0;

            if (!strncmp (buff, "SEEN-BY: ", 9)) {
               if (need_origin) {
                  mprintf (fpd, msgtxt[M_TEAR_LINE],VERSION, registered ? "+" : NOREG);
                  if (strlen(sys.origin))
                     mprintf(fpd,msgtxt[M_ORIGIN_LINE],random_origins(),config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
                  else
                     mprintf(fpd,msgtxt[M_ORIGIN_LINE],system_name,config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
                  need_origin = 0;
               }

               p = strtok (buff, " ");
               ne = config->alias[sys.use_alias].net;
               no = config->alias[sys.use_alias].node;
               pp = config->alias[sys.use_alias].point;
               z = config->alias[sys.use_alias].zone;

               while ((p = strtok (NULL, " ")) != NULL) {
                  parse_netnode2 (p, &z, &ne, &no, &pp);

                  seen[n_seen].net = ne;
                  seen[n_seen].node = no;
                  seen[n_seen].point = pp;
                  seen[n_seen].zone = z;

                  if (n_seen + 1 >= MAX_EXPORT_SEEN)
                     continue;

                  if (seen[n_seen].net != config->alias[sys.use_alias].fakenet && !seen[n_seen].point)
                     n_seen++;
               }
            }
            else if (!strncmp (buff, "\001PATH: ", 7) && need_seen) {
               if (need_origin) {
                  mprintf (fpd, msgtxt[M_TEAR_LINE],VERSION, registered ? "+" : NOREG);
                  if (strlen(sys.origin))
                     mprintf(fpd,msgtxt[M_ORIGIN_LINE],random_origins(),config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
                  else
                     mprintf(fpd,msgtxt[M_ORIGIN_LINE],system_name,config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
                  need_origin = 0;
               }

               if (!config->alias[sys.use_alias].point) {
                  seen[n_seen].net = config->alias[sys.use_alias].net;
                  seen[n_seen++].node = config->alias[sys.use_alias].node;
               }
               else if (config->alias[sys.use_alias].fakenet) {
                  seen[n_seen].net = config->alias[sys.use_alias].fakenet;
                  seen[n_seen++].node = config->alias[sys.use_alias].point;
               }

               seen[n_seen].net = net;
               seen[n_seen++].node = node;

               qsort (seen, n_seen, sizeof (struct _fwrd), mail_sort_func);

               cnet = cnode = 0;
               strcpy (wrp, "SEEN-BY: ");

               for (i = 0; i < n_seen; i++) {
                  if (strlen (wrp) > 65) {
                     mprintf (fpd, "%s\r\n", wrp);
                     cnet = cnode = 0;
                     strcpy (wrp, "SEEN-BY: ");
                  }
                  if (i && seen[i].net == seen[i - 1].net && seen[i].node == seen[i - 1].node)
                     continue;

                  if (cnet != seen[i].net) {
                     sprintf (buffer, "%d/%d ", seen[i].net, seen[i].node);
                     cnet = seen[i].net;
                     cnode = seen[i].node;
                  }
                  else if (cnet == seen[i].net && cnode != seen[i].node) {
                     sprintf (buffer, "%d ", seen[i].node);
                     cnode = seen[i].node;
                  }
                  else
                     strcpy (buffer, "");

                  strcat (wrp, buffer);
               }

               mprintf (fpd, "%s\r\n", wrp);
               mprintf (fpd, "%s\r\n", buff);

               need_seen = 0;
            }
            else if ((msghdr.msgattr & 64) && !strncmp (buff, "--- ", 4) && (config->replace_tear || !registered))
               replace_tearline (fpd, buff);
            else
               mprintf (fpd, "%s\r\n", buff);

            mi = 0;
         }
         else {
            if(mi < 78)
               continue;

            if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
               need_origin = 0;

            buff[mi]='\0';
            mprintf(fpd,"%s",buff);
            mi=0;
            buff[mi] = '\0';
         }
      }

      if (!n_seen || need_seen) {
         if (need_origin) {
            mprintf (fpd, msgtxt[M_TEAR_LINE],VERSION, registered ? "+" : NOREG);
            if (strlen(sys.origin))
               mprintf(fpd,msgtxt[M_ORIGIN_LINE],random_origins(),config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
            else
               mprintf(fpd,msgtxt[M_ORIGIN_LINE],system_name,config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
            need_origin = 0;
         }

         if (!config->alias[sys.use_alias].point) {
            seen[n_seen].net = config->alias[sys.use_alias].net;
            seen[n_seen++].node = config->alias[sys.use_alias].node;
         }
         else if (config->alias[sys.use_alias].fakenet) {
            seen[n_seen].net = config->alias[sys.use_alias].fakenet;
            seen[n_seen++].node = config->alias[sys.use_alias].point;
         }

         seen[n_seen].net = net;
         seen[n_seen++].node = node;

         qsort (seen, n_seen, sizeof (struct _fwrd), mail_sort_func);

         cnet = cnode = 0;
         strcpy (wrp, "SEEN-BY: ");

         for (i = 0; i < n_seen; i++) {
            if (cnet != seen[i].net) {
               sprintf (buffer, "%d/%d ", seen[i].net, seen[i].node);
               cnet = seen[i].net;
               cnode = seen[i].node;
            }
            else if (cnet == seen[i].net && cnode != seen[i].node) {
               sprintf (buffer, "%d ", seen[i].node);
               cnode = seen[i].node;
            }
            else
               strcpy (buffer, "");

            strcat (wrp, buffer);
         }

         mprintf (fpd, "%s\r\n", wrp);
         if (config->alias[sys.use_alias].point && config->alias[sys.use_alias].fakenet)
            mprintf (fpd, "\001PATH: %d/%d\r\n", config->alias[sys.use_alias].fakenet, config->alias[sys.use_alias].point);
         else if (!config->alias[sys.use_alias].point)
            mprintf (fpd, "\001PATH: %d/%d\r\n", config->alias[sys.use_alias].net, config->alias[sys.use_alias].node);
      }

      if (ai)
         sys.use_alias = ai - 1;

      mhdr.ver = PKTVER;
      if (config->alias[sys.use_alias].point && config->alias[sys.use_alias].fakenet) {
         mhdr.orig_node = config->alias[sys.use_alias].point;
         mhdr.orig_net = config->alias[sys.use_alias].fakenet;
      }
      else {
         mhdr.orig_node = config->alias[sys.use_alias].node;
         mhdr.orig_net = config->alias[sys.use_alias].net;
      }
      mhdr.cost = msghdr.cost;
      mhdr.attrib = 0;

      if (sys.public)
         mhdr.attrib &= ~Q_PRIVATE;
      else if (sys.private)
         mhdr.attrib |= Q_PRIVATE;

      if (msghdr.msgattr & Q_PRIVATE)
         mhdr.attrib |= MSGPRIVATE;

      if (point)
         sprintf (wrp, "%d/%d.%d ", net, node, point);
      else
         sprintf (wrp, "%d/%d ", net, node);
      wreadcur (&z, &m);
      if ( (m + strlen (wrp)) > 78) {
         wputs ("\n");
         wputs ("                                           ");
      }
      wputs (wrp);
      if ( (m + strlen (wrp)) == 78)
         wputs ("                                           ");

      mi = open_packet (zone, net, node, point, sys.use_alias);

      mhdr.dest_net = net;
      mhdr.dest_node = node;
      write (mi, (char *)&mhdr, sizeof (struct _msghdr2));

      strcpy(wrp,pascal_string(msghdr.date));
      sscanf(wrp,"%2d-%2d-%2d",&z,&ne,&no);
      strcpy(wrp,pascal_string(msghdr.time));
      sscanf(wrp,"%2d:%2d",&pp,&m);
      sprintf (buff, "%02d %s %02d  %2d:%02d:00", ne, mtext[z-1], no, pp, m);
      write (mi, buff, strlen (buff) + 1);
      strcpy (buff, pascal_string (msghdr.whoto));
      write (mi, buff, strlen (buff) + 1);
      strcpy (buff, pascal_string (msghdr.whofrom));
      write (mi, buff, strlen (buff) + 1);
      strcpy (buff, pascal_string (msghdr.subject));
      write (mi, buff, strlen (buff) + 1);
      mseek (fpd, 0L, SEEK_SET);
      do {
         z = mread(buffer, 1, 2048, fpd);
         write(mi, buffer, z);
      } while (z == 2048);
      buff[0] = buff[1] = buff[2] = 0;
      if (write (mi, buff, 3) != 3)
         return (-1);
      close (mi);

      totalmsg++;
      sprintf (wrp, "%d (%.1f/s) ", totalmsg, (float)totalmsg / ((float)(timerset (0) - totaltime) / 100));
      prints (10, 65, YELLOW|_BLACK, wrp);

      wputs ("\n");
      sent++;
   }

   if (sent)
      status_line (":   %-20.20s (Sent=%04d)", sys.echotag, sent);

   if (fp != NULL)
      fclose (fp);
   mclose (fpd);
   free (seen);
   close (fd);
   fclose (f2);
   close (f1);
   close (finfo);
   unlink ("MSGTMP.EXP");

   return (0);
}

void quick_read_sysinfo (board)
int board;
{
   int fd, tnum_msg;
   char filename[80];

   tnum_msg = 0;
   msgidx[0] = 0;

   sprintf(filename, "%sMSGINFO.BBS",fido_msgpath);
   fd = shopen(filename,O_RDONLY|O_BINARY);
   read (fd, (char *)&msginfo, sizeof (struct _msginfo));
   close(fd);

   tnum_msg = msginfo.totalonboard[board-1];
   if (tnum_msg) {
      first_msg = 1;
      last_msg = tnum_msg;
      num_msg = tnum_msg;
   }
   else {
      first_msg = 0;
      last_msg = 0;
      num_msg = 0;
   }
}

