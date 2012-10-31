
// LoraBBS Version 2.41 Free Edition
// Copyright (C) 1987-98 Marco Maccaferri
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <stdio.h>
#include <alloc.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
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
#include "pipbase.h"

extern int maxakainfo;
extern struct _akainfo *akainfo;

extern struct _node2name *nametable; // Gestione nomi per aree PRIVATE
extern int nodes_num;

FILE *mopen (char *filename, char *mode);
int mclose (FILE *fp);
int mputs (char *s, FILE *fp);
void mprintf (FILE *fp, char *format, ...);
long mseek (FILE *fp, long position, int offset);
int mread (char *s, int n, int e, FILE *fp);
void replace_tearline (FILE *fpd, char *buf);
void add_quote_string (char *str, char *from);
void add_quote_header (FILE *fp, char *from, char *to, char *subj, char *dt, char *tm);
int open_packet (int zone, int net, int node, int point, int ai);

char *pip[128]={
"PREFIX","SEEN-BY: ","MSGID: ","PATH: ",": ",
"zion","ment","---","che","chi","ghe","ghi","str","","il","al","ed",
"pr","st","..","  ",", ",". ","; ","++",
"a'","e'","i'","o'","u'","a ","e ",
"i ","o ","u ","nt","hi",
"bb","ba","be","bi","bo","bu",
"cc","ca","ce","ci","co","cu",
"dd","da","de","di","do","du",
"ff","fa","fe","fi","fo","fu",
"gg","ga","ge","gi","go","gu",
"ll","la","le","li","lo","lu",
"mm","ma","me","mi","mo","mu",
"nn","na","ne","ni","no","nu",
"pp","pa","pe","pi","po","pu",
"rr","ra","re","ri","ro","ru",
"ss","sa","se","si","so","su",
"tt","ta","te","ti","to","tu",
"vv","va","ve","vi","vo","vu",
"zz","za","ze","zi","zo","zu",
"==",":-","' ","ha","ho","qu","#"};

int read0(unsigned char *, FILE *);
void pipstring(unsigned char *, FILE *);
void write0(unsigned char *, FILE *);

int pip_mail_list_header (i, pip_board, line, ovrmsgn)
int i, pip_board, line, ovrmsgn;
{
   FILE *f1, *f2;
   char fn[80];
   MSGPTR hdr;
   MSGPKTHDR mpkt;
   struct _msg msgt;

   sprintf(fn,"%sMPTR%04x.PIP",pip_msgpath,pip_board);
   f1=fopen(fn,"rb+");
   sprintf(fn,"%sMPKT%04x.PIP",pip_msgpath,pip_board);
   f2=fopen(fn,"rb+");

   if (fseek(f1,sizeof(hdr)*(i-1),SEEK_SET))
      return (line);
   if (fread(&hdr,sizeof(hdr),1,f1)==0)
      return (line);

   memset ((char *)&msgt, 0, sizeof (struct _msg));
   msg_fzone = msg_tzone = config->alias[sys.use_alias].zone;
   msg_fpoint = msg_tpoint = 0;

   fseek(f2,hdr.pos,SEEK_SET);
   fread(&mpkt,sizeof(mpkt),1,f2);

   read0(msgt.date,f2);
   read0(msgt.to,f2);
   read0(msgt.from,f2);
   read0(msgt.subj,f2);

   msgt.orig_net = mpkt.fromnet;
   msgt.orig = mpkt.fromnode;
   if (!(mpkt.attribute & SET_PKT_FROMUS))
      msg_fpoint = mpkt.point;

   if (mpkt.attribute & SET_PKT_PRIV)
      msgt.attr |= MSGPRIVATE;

   msgt.dest_net = mpkt.tonet;
   msgt.dest = mpkt.tonode;
   if ( (mpkt.attribute & SET_PKT_FROMUS) )
      msg_tpoint = mpkt.point;

   if ((line = msg_attrib (&msgt, ovrmsgn ? ovrmsgn : i, line, 0)) == 0)
      return (0);

   fclose (f1);
   fclose (f2);

   return (line);
}

int pip_write_message_text (int msg_num, int flags, char *quote_name, FILE *sm)
{
   FILE *f1,*f2, *fpq;
   char buff[86], wrp[80], c, fn[80], shead, qwkbuffer[130], *p;
   byte a;
   int i, m, z, pos, blks;
   long qpos;
   MSGPTR hdr;
   MSGPKTHDR mpkt;
   struct _msg msgt;
   struct QWKmsghd QWK;

   msg_num--;
   shead = 0;
   blks = 1;
   pos = 0;

   sprintf (fn, "%sMPTR%04x.PIP", pip_msgpath, sys.pip_board);
   f1 = fopen (fn, "rb+");

   if (f1 == NULL)
      return (0);

   if (fseek (f1, (long)sizeof (hdr) * msg_num, SEEK_SET)) {
      fclose (f1);
      return (0);
   }
   if (fread (&hdr, sizeof (hdr), 1, f1) == 0) {
      fclose (f1);
      return (0);
   }
   if (hdr.status & SET_MPTR_DEL) {
      fclose (f1);
      return (0);
   }

   sprintf (fn, "%sMPKT%04x.PIP", pip_msgpath, sys.pip_board);
   f2 = fopen (fn, "rb+");
   if (f2 == NULL) {
      fclose(f1);
      return (0);
   }

   if (sm == NULL) {
      fpq = fopen (quote_name, (flags & APPEND_TEXT) ? "at" : "wt");
      if (fpq == NULL) {
         fclose (f1);
         fclose (f2);
         return (0);
      }
   }
   else
      fpq = sm;


   if (!((hdr.status & SET_MPTR_RCVD) || (hdr.status & SET_MPTR_DEL))) {
      fseek (f2, hdr.pos,SEEK_SET);
      fread (&mpkt, sizeof (mpkt), 1, f2);

      memset ((char *)&msgt, 0, sizeof (struct _msg));
      msg_fzone = msg_tzone = config->alias[sys.use_alias].zone;
      msg_fpoint = msg_tpoint = 0;

      read0 (msgt.date, f2);
      read0 (msgt.to, f2);
      read0 (msgt.from, f2);
      read0 (msgt.subj, f2);

      msgt.orig_net = mpkt.fromnet;
      msgt.orig = mpkt.fromnode;
      if (!(mpkt.attribute & SET_PKT_FROMUS))
         msg_fpoint = mpkt.point;

      if (mpkt.attribute & SET_PKT_PRIV)
         msgt.attr |= MSGPRIVATE;

      if (msgt.attr & MSGPRIVATE) {
         if (stricmp (msgt.from, usr.name) && stricmp (msgt.to, usr.name) && stricmp (msgt.from, usr.handle) && stricmp (msgt.to, usr.handle)) {
            fclose (f1);
            fclose (f2);
            if (sm != NULL)
               fclose (fpq);
            return (0);
         }
      }

      msgt.dest_net = mpkt.tonet;
      msgt.dest = mpkt.tonode;
      if ( (mpkt.attribute & SET_PKT_FROMUS) )
         msg_tpoint = mpkt.point;

      i = 0;
      if (flags & QUOTE_TEXT) {
         add_quote_string (buff, msgt.from);
         i = strlen (buff);
      }

      while ((a = (byte)fgetc (f2)) != 0 && !feof (f2)) {
         c = a;

         switch (mpkt.pktype) {
            case 2:
               c = a;
               break;
            case 10:
               if (a != 10) {
                  if (a == 141)
                     a='\r';

                  if (a < 128)
                     c = a;
                  else {
                     if (a == 128) {
                        a = (byte)fgetc (f2);
                        c = (a) + 127;
                     }
                     else {
                        buff[i] = '\0';
                        strcat (buff, pip[a - 128]);
                        i += strlen (pip[a - 128]);
                        c = buff[--i];
                     }
                  }
               }
               else
                  c = '\0';
               break;
            default:
               return (1);
         }

         if (c == '\0')
            continue;
         buff[i++] = c;

         if (c == 0x0D) {
            buff[i - 1] = '\0';
            if (!strnicmp (buff, msgtxt[M_TEAR_LINE], 4))
               buff[1] = '+';
            if (!strnicmp (buff, msgtxt[M_ORIGIN_LINE], 10))
               buff[3] = '0';

            if ((p = strchr (buff, 0x01)) != NULL) {
               if (!strncmp(&p[1],"INTL",4) && !shead)
                  sscanf(&p[6],"%d:%d/%d %d:%d/%d",&msg_tzone,&i,&i,&msg_fzone,&i,&i);
               if (!strncmp(&p[1],"TOPT",4) && !shead)
                  sscanf(&p[6],"%d",&msg_tpoint);
               if (!strncmp(&p[1],"FMPT",4) && !shead)
                  sscanf(&p[6],"%d",&msg_fpoint);
            }
            else if (!shead) {
               if (flags & INCLUDE_HEADER)
                  text_header (&msgt,msg_num,fpq);
               else if (flags & QWK_TEXTFILE)
                  qwk_header (&msgt,&QWK,msg_num,fpq,&qpos);
               else if (flags & QUOTE_TEXT)
                  add_quote_header (fpq, msgt.from, msgt.to, msgt.subj, msgt.date, NULL);
               shead = 1;
            }

            if (strchr (buff, 0x01) != NULL || stristr (buff, "SEEN-BY") != NULL) {
               if (flags & QUOTE_TEXT) {
                  add_quote_string (buff, msgt.from);
                  i = strlen (buff);
               }
               else
                  i = 0;
               continue;
            }

            if (flags & QWK_TEXTFILE) {
               write_qwk_string (buff, qwkbuffer, &pos, &blks, fpq);
               write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpq);
            }
            else
               fprintf (fpq, "%s\n", buff);

            if (flags & QUOTE_TEXT) {
               add_quote_string (buff, msgt.from);
               i = strlen (buff);
            }
            else
               i = 0;
         }
         else {
            if (i < (usr.width - 1))
               continue;

            buff[i] = '\0';
            while (i > 0 && buff[i] != ' ')
               i--;

            m=0;

            if(i != 0)
               for(z=i+1;buff[z];z++)
                  wrp[m++]=buff[z];

            buff[i]='\0';
            wrp[m]='\0';
            if (!strnicmp (buff, msgtxt[M_TEAR_LINE], 4))
               buff[1] = '+';
            if (!strnicmp (buff, msgtxt[M_ORIGIN_LINE], 10))
               buff[3] = '0';

            if (!shead) {
               if (flags & INCLUDE_HEADER) {
                  text_header (&msgt,msg_num,fpq);
               }
               else if (flags & QWK_TEXTFILE)
                  qwk_header (&msgt,&QWK,msg_num,fpq,&qpos);
               else if (flags & QUOTE_TEXT)
                  add_quote_header (fpq, msgt.from, msgt.to, msgt.subj, msgt.date, NULL);
               shead = 1;
            }

            if (flags & QWK_TEXTFILE) {
               write_qwk_string (buff, qwkbuffer, &pos, &blks, fpq);
               write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpq);
            }
            else
               fprintf(fpq,"%s\n",buff);

            if (flags & QUOTE_TEXT)
               add_quote_string (buff, msgt.from);
            else
               buff[0] = '\0';

            strcat (buff, wrp);
            i = strlen (buff);
         }
      }
   }

   fprintf (fpq, bbstxt[B_ONE_CR], buff);

   fclose (f1);
   fclose (f2);

   if (flags & QWK_TEXTFILE) {
      fwrite(qwkbuffer, 128, 1, fpq);
      blks++;

      fseek (fpq, qpos, SEEK_SET);          /* Restore back to header start */
      sprintf (buff, "%d", blks);
      ljstring (QWK.Msgrecs, buff, 6);
      fwrite ((char *)&QWK, 128, 1, fpq);           /* Write out the header */
      fseek (fpq, 0L, SEEK_END);               /* Bump back to end of file */

      if (sm == NULL)
         fclose (fpq);
      return (blks);
   }

   if (sm == NULL)
      fclose(fpq);

   return (1);
}

int pip_kill_message (msg)
int msg;
{
   FILE *f1,*f2;
   MSGPTR hdr;
   MSGPKTHDR mpkt;
   char fn[80];
   struct _msg msgt;

   msg--;

   sprintf (fn, "%sMPTR%04x.PIP", pip_msgpath, sys.pip_board);
   f1 = fopen (fn, "rb+");

   if (f1 == NULL)
      return (0);

   if (fseek(f1,sizeof(hdr)*msg,SEEK_SET)) {
      fclose(f1);
      return (0);
   }
   if (fread(&hdr,sizeof(hdr),1,f1)==0) {
      fclose(f1);
      return (0);
   }
   if (hdr.status & SET_MPTR_DEL) {
      fclose(f1);
      return (0);
   }

   sprintf(fn,"%sMPKT%04x.PIP", pip_msgpath,sys.pip_board);
   f2 = fopen(fn,"rb+");
   if (f2 == NULL) {
      fclose(f1);
      return (0);
   }

   if (!((hdr.status&SET_MPTR_RCVD) || (hdr.status&SET_MPTR_DEL))) {
      fseek(f2,hdr.pos,SEEK_SET);
      fread(&mpkt,sizeof(mpkt),1,f2);

      memset ((char *)&msgt, 0, sizeof (struct _msg));

      read0(msgt.date,f2);
      read0(msgt.to,f2);
      read0(msgt.from,f2);
      read0(msgt.subj,f2);

      if (!stricmp (msgt.from, usr.name) || !stricmp (msgt.to, usr.name) || !stricmp (msgt.from, usr.handle) || !stricmp (msgt.to, usr.handle) || usr.priv == SYSOP) {
         hdr.status |= SET_MPTR_DEL;
         fseek (f1, sizeof(hdr) * msg, SEEK_SET);
         fwrite (&hdr, sizeof(hdr), 1, f1);
         fclose (f1);
         fclose (f2);
         return (1);
      }
   }

   fclose (f1);
   fclose (f2);

   return (0);
}

int pip_scan_messages (total, personal, pip_board, start, fdi, fdp, totals, fpq, qwk)
int *total, *personal, pip_board, start, fdi, fdp, totals;
FILE *fpq;
char qwk;
{
   FILE *f1,*f2;
   float in, out;
   MSGPTR hdr;
   MSGPKTHDR mpkt;
   char buff[86], wrp[80], c, fn[80], shead, qwkbuffer[130];
   byte a;
   int i, m, z, pos, blks, tt, pp, msg_num;
   long qpos, bw_start;
   struct _msg msgt;
   struct QWKmsghd QWK;

   tt = pp = 0;

   if (start > last_msg) {
      *personal = pp;
      *total = tt;

      return (totals);
   }

   sprintf(fn,"%sMPTR%04x.PIP",pip_msgpath,pip_board);
   f1=fopen(fn,"rb+");
   sprintf(fn,"%sMPKT%04x.PIP",pip_msgpath,pip_board);
   f2=fopen(fn,"rb+");

   for(msg_num = start; msg_num <= last_msg; msg_num++) {
      if (!(msg_num % 5))
         display_percentage (msg_num - start, last_msg - start);

      if (fseek(f1,(long)sizeof(hdr)*(msg_num-1),SEEK_SET))
         break;
      if (fread(&hdr,sizeof(hdr),1,f1)==0)
         break;

      if (hdr.status & SET_MPTR_DEL)
         continue;

      memset ((char *)&msgt, 0, sizeof (struct _msg));
      msg_fzone = msg_tzone = config->alias[sys.use_alias].zone;
      msg_fpoint = msg_tpoint = 0;

      fseek (f2, hdr.pos, SEEK_SET);
      fread (&mpkt, sizeof (mpkt), 1, f2);

      read0 (msgt.date, f2);
      read0 (msgt.to, f2);
      read0 (msgt.from, f2);
      read0 (msgt.subj, f2);

      msgt.orig_net = mpkt.fromnet;
      msgt.orig = mpkt.fromnode;
      if (!(mpkt.attribute & SET_PKT_FROMUS))
         msg_fpoint = mpkt.point;

      if (mpkt.attribute & SET_PKT_PRIV)
         msgt.attr |= MSGPRIVATE;

      if((msgt.attr & MSGPRIVATE) && stricmp(msgt.from,usr.name) && stricmp(msgt.to,usr.name) && stricmp(msgt.from,usr.handle) && stricmp(msgt.to,usr.handle) && usr.priv < SYSOP)
         continue;

      totals++;
      if (qwk == 1 && fdi != -1) {
         sprintf(buff,"%u",totals);   /* Stringized version of current position */
         in = (float) atof(buff);
         out = IEEToMSBIN(in);
         write(fdi,&out,sizeof(float));

         c = 0;
         write(fdi,&c,sizeof(char));              /* Conference # */
      }

      if (!stricmp(msgt.to, usr.name)||!stricmp(msgt.to, usr.name)) {
         pp++;
         if (fdp != -1 && qwk == 1 && fdi != -1) {
            write(fdp,&out,sizeof(float));
            write(fdp,&c,sizeof(char));              /* Conference # */
         }
      }

      if (qwk == 2) {
         bw_start = ftell (fpq);
         fputc (' ', fpq);
      }

      memset (qwkbuffer, ' ', 128);
      shead = 0;
      blks = 1;
      pos = 0;
      i = 0;

      while ((a=(byte)fgetc(f2)) != 0 && !feof(f2)) {
         c = a;

         switch(mpkt.pktype) {
            case 2:
               c = a;
               break;
            case 10:
               if (a!=10) {
                  if (a == 141)
                     a='\r';

                  if (a<128)
                     c=a;
                  else {
                     if (a == 128) {
                        a = (byte)fgetc (f2);
                        c = (a) + 127;
                     }
                     else {
                        buff[i] = '\0';
                        strcat (buff, pip[a - 128]);
                        i += strlen (pip[a - 128]);
                        c = buff[--i];
                     }
                  }
               }
               else
                  c='\0';
               break;
            default:
               return(1);
         }

         if (c == '\0')
            continue;
         buff[i++] = c;

         if (c == 0x0D) {
            buff[i-1]='\0';

            if (buff[0] == 0x01) {
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
               if (qwk == 1)
                  qwk_header (&msgt,&QWK,msg_num,fpq,&qpos);
               else if (qwk == 0)
                  text_header (&msgt,msg_num,fpq);
               shead = 1;
            }

            if(buff[0] == 0x01 || !strncmp(buff,"SEEN-BY",7)) {
               i=0;
               continue;
            }

            if (qwk == 1) {
               write_qwk_string (buff, qwkbuffer, &pos, &blks, fpq);
               write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpq);
            }
            else
               fprintf(fpq,"%s\n",buff);

            i = 0;
         }
         else {
            if(i<(usr.width-1))
               continue;

            buff[i]='\0';
            while(i>0 && buff[i] != ' ')
               i--;

            m=0;

            if(i != 0)
               for(z=i+1;buff[z];z++)
                  wrp[m++]=buff[z];

            buff[i]='\0';
            wrp[m]='\0';

            if (!shead) {
               if (qwk == 1)
                  qwk_header (&msgt,&QWK,msg_num,fpq,&qpos);
               else if (qwk == 0)
                  text_header (&msgt,msg_num,fpq);
               shead = 1;
            }

            if (qwk == 1) {
               write_qwk_string (buff, qwkbuffer, &pos, &blks, fpq);
               write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpq);
            }
            else
               fprintf(fpq,"%s\n",buff);

            strcpy(buff,wrp);
            i = strlen(wrp);
         }
      }

      if (qwk == 1) {
         qwkbuffer[128] = 0;
         fwrite(qwkbuffer, 128, 1, fpq);
         blks++;

      /* Now update with record count */
         fseek(fpq,qpos,SEEK_SET);          /* Restore back to header start */
         sprintf(buff,"%d",blks);
         ljstring(QWK.Msgrecs,buff,6);
         fwrite((char *)&QWK,128,1,fpq);           /* Write out the header */
         fseek(fpq,0L,SEEK_END);               /* Bump back to end of file */

         totals += blks - 1;
      }
      else if (qwk == 2)
         bluewave_header (fdi, bw_start, ftell (fpq) - bw_start, msg_num, &msgt);
      else
         fprintf(fpq,bbstxt[B_TWO_CR]);

      tt++;
   }

   fclose (f1);
   fclose (f2);

   *personal = pp;
   *total = tt;

   return (totals);
}

int pip_save_message2(txt, f1, f2, f3)
FILE *txt, *f1, *f2, *f3;
{
   int i, dest, m;
   char buffer[2050];
   MSGPTR hdr;
   MSGPKTHDR mpkt;
   DESTPTR pt;

   dest = last_msg + 1;

   mpkt.fromnode = msg.orig;
   mpkt.fromnet = msg.orig_net;
   mpkt.pktype = 10;
   mpkt.attribute = 0;
   mpkt.tonode = msg.dest;
   mpkt.tonet = msg.dest_net;
   mpkt.point = msg_tpoint;

   fseek(f2,filelength(fileno(f2))-2,SEEK_SET);
   hdr.pos=ftell(f2);
   hdr.next=hdr.prev=0;
   hdr.status=0; /* from us */
   fseek(f1,filelength(fileno(f1)),SEEK_SET);
   pt.area=sys.pip_board;
   pt.msg=(int)(ftell(f1)/sizeof(hdr));
   strcpy(pt.to,msg.to);
   pt.next=0;

   if (sys.public)
      msg.attr &= ~MSGPRIVATE;
   else if (sys.private)
      msg.attr |= MSGPRIVATE;

   if (msg.attr & MSGPRIVATE)
      mpkt.attribute |= SET_PKT_PRIV;

   if (sys.echomail && (msg.attr & MSGSENT))
      hdr.status |= SET_MPTR_SENT;

   if (fwrite(&hdr,sizeof hdr,1,f1) != 1)
      return (0);
   if (fwrite(&mpkt,sizeof mpkt,1,f2) != 1)
      return (0);
   write0(msg.date,f2);
   write0(msg.to,f2);
   write0(msg.from,f2);
   write0(msg.subj,f2);

   do {
      i = mread(buffer, 1, 2048, txt);
      buffer[i] = '\0';
      for (m=0;m<i;m++) {
         if (buffer[m] == 0x1A)
            buffer[m] = ' ';
      }
      pipstring(buffer,f2);
   } while (i == 2048);

   fputc(0,f2);
   fputc(0,f2);
   fputc(0,f2);

   if (fwrite(&pt,sizeof pt,1,f3) != 1)
      return (0);

   fflush (f1);
#ifndef __OS2__
   real_flush (fileno (f1));
#endif
   fflush (f2);
#ifndef __OS2__
   real_flush (fileno (f2));
#endif
   fflush (f3);
#ifndef __OS2__
   real_flush (fileno (f3));
#endif

   last_msg = dest;
   return (1);
}

int pip_export_mail (int maxnodes, struct _fwrd *forward)
{
   FILE *fpd, *f2;
   int f1, i, pp, z, ne, no, n_seen, cnet, cnode, mi, msgnum, m, num_msg, sent, ai;
   char buff[80], wrp[80], c, *p, buffer[2050], need_origin, need_seen, *flag;
   byte a;
   long hdrpos;
   MSGPTR hdr;
   MSGPKTHDR mpkt;
   struct _msghdr2 mhdr;
   struct _fwrd *seen;

   sprintf(buff,"%sMPTR%04x.PIP",pip_msgpath,sys.pip_board);
   f1 = open(buff, O_RDWR|O_BINARY);
   if(f1 == -1)
      return (maxnodes);

   num_msg = (int)(filelength(f1) / sizeof(MSGPTR));

   sprintf(buff,"%sMPKT%04x.PIP",pip_msgpath,sys.pip_board);
   f2 = fopen(buff,"rb+");
   if (f2 == NULL) {
      close (f1);
      return (maxnodes);
   }

   seen = (struct _fwrd *)malloc ((MAX_EXPORT_SEEN + 1) * sizeof (struct _fwrd));
   if (seen == NULL) {
      close (f1);
      fclose (f2);
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

   for (i = 0; i < maxnodes; i++)
		forward[i].receiveonly = forward[i].sendonly = forward[i].passive = forward[i].private = forward[i].reset = forward[i].export = 0;

   z = config->alias[sys.use_alias].zone;
   ne = config->alias[sys.use_alias].net;
   no = config->alias[sys.use_alias].node;
   pp = 0;

   p = strtok (sys.forward1, " ");
   if (p != NULL)
      do {
         flag = p;
         while (*p == '<' || *p == '>' || *p == '!' || *p== 'p' || *p== 'P')
            p++;
         parse_netnode2 (p, &z, &ne, &no, &pp);
         for (i = 0; i < MAX_ALIAS; i++) {
            if (config->alias[i].net == 0)
               continue;
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
            i = maxnodes;
            forward[maxnodes].zone = z;
            forward[maxnodes].net = ne;
            forward[maxnodes].node = no;
            forward[maxnodes].point = pp;
            forward[maxnodes].export = 1;
            forward[maxnodes].reset = 1;
            maxnodes++;
         }
         forward[i].receiveonly = forward[i].sendonly = forward[i].passive = forward[i].private = 0;
         while (*flag == '<' || *flag == '>' || *flag =='!'||*flag=='p'||*flag=='P') {
            if (*flag == '>')
               forward[i].receiveonly = 1;
            if (*flag == '<')
               forward[i].sendonly = 1;
            if (*flag == '!')
               forward[i].passive = 1;
            if (*flag == 'p' || *flag == 'P')
               forward[i].private =1 ;
            flag++;
         }
      } while ((p = strtok (NULL, " ")) != NULL);

   z = config->alias[sys.use_alias].zone;
   ne = config->alias[sys.use_alias].net;
   no = config->alias[sys.use_alias].node;
   pp = 0;

   p = strtok (sys.forward2, " ");
   if (p != NULL)
      do {
         flag = p;
         while (*p == '<' || *p == '>' || *p == '!' || *p== 'p' || *p== 'P')
            p++;
         parse_netnode2 (p, &z, &ne, &no, &pp);
         for (i = 0; i < MAX_ALIAS; i++) {
            if (config->alias[i].net == 0)
               continue;
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
            i = maxnodes;
            forward[maxnodes].zone = z;
            forward[maxnodes].net = ne;
            forward[maxnodes].node = no;
            forward[maxnodes].point = pp;
            forward[maxnodes].export = 1;
            forward[maxnodes].reset = 1;
            maxnodes++;
         }
         forward[i].receiveonly = forward[i].sendonly = forward[i].passive = forward[i].private = 0;
         while (*flag == '<' || *flag == '>' || *flag =='!'||*flag=='p'||*flag=='P') {
            if (*flag == '>')
               forward[i].receiveonly = 1;
            if (*flag == '<')
               forward[i].sendonly = 1;
            if (*flag == '!')
               forward[i].passive = 1;
            if (*flag == 'p' || *flag == 'P')
               forward[i].private =1 ;
            flag++;
         }
      } while ((p = strtok (NULL, " ")) != NULL);

   z = config->alias[sys.use_alias].zone;
   ne = config->alias[sys.use_alias].net;
   no = config->alias[sys.use_alias].node;
   pp = 0;

   p = strtok (sys.forward3, " ");
   if (p != NULL)
      do {
         flag = p;
         while (*p == '<' || *p == '>' || *p == '!' || *p== 'p' || *p== 'P')
            p++;
         parse_netnode2 (p, &z, &ne, &no, &pp);
         for (i = 0; i < MAX_ALIAS; i++) {
            if (config->alias[i].net == 0)
               continue;
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
            i = maxnodes;
            forward[maxnodes].zone = z;
            forward[maxnodes].net = ne;
            forward[maxnodes].node = no;
            forward[maxnodes].point = pp;
            forward[maxnodes].export = 1;
            forward[maxnodes].reset = 1;
            maxnodes++;
         }
         forward[i].receiveonly = forward[i].sendonly = forward[i].passive = forward[i].private = 0;
         while (*flag == '<' || *flag == '>' || *flag =='!'||*flag=='p'||*flag=='P') {
            if (*flag == '>')
               forward[i].receiveonly = 1;
            if (*flag == '<')
               forward[i].sendonly = 1;
            if (*flag == '!')
               forward[i].passive = 1;
            if (*flag == 'p' || *flag == 'P')
               forward[i].private =1 ;
            flag++;
         }
      } while ((p = strtok (NULL, " ")) != NULL);

   msgnum = 0;
   hdrpos = tell (f1);

   strcpy (wrp, sys.echotag);
   wrp[14] = '\0';
   prints (8, 65, YELLOW|_BLACK, "              ");
   prints (8, 65, YELLOW|_BLACK, wrp);

   prints (7, 65, YELLOW|_BLACK, "              ");
   prints (9, 65, YELLOW|_BLACK, "              ");
   prints (9, 65, YELLOW|_BLACK, "Pip-Base");

   sent = 0;

   while (read(f1, (char *)&hdr,sizeof(MSGPTR)) == sizeof (MSGPTR)) {
      sprintf (wrp, "%d / %d", ++msgnum, num_msg);
      prints (7, 65, YELLOW|_BLACK, wrp);

      if ( !(msgnum % 20))
         time_release ();

      if((hdr.status & SET_MPTR_SENT) || (hdr.status & SET_MPTR_DEL)) {
         hdrpos = tell (f1);
			continue;
		}

		for (i = 0; i < maxnodes; i++)
			if (forward[i].reset)
				forward[i].export = 1;

		for (i=0;i<maxnodes;i++){
			if(forward[i].sendonly||forward[i].passive) forward[i].export=0;
		}

		sprintf (wrp, "%5d  %-22.22s Pip-Base     ", msgnum, sys.echotag);
		wputs (wrp);

		need_seen = need_origin = 1;

		mseek (fpd, 0L, SEEK_SET);
//      chsize (fileno (fpd), 0L);
		mprintf (fpd, "AREA:%s\r\n", sys.echotag);

		fseek(f2,hdr.pos,SEEK_SET);
		fread(&mpkt,sizeof(MSGPKTHDR),1,f2);

		memset ((char *)&msg, 0, sizeof (struct _msg));
		read0(msg.date,f2);
		read0(msg.to,f2);
		read0(msg.from,f2);
		read0(msg.subj,f2);

		if (mpkt.attribute & SET_PKT_PRIV)
			msg.attr |= MSGPRIVATE;

		for (i=0;i<maxnodes;i++) {
			if (forward[i].private) {

				int ii;

				forward[i].export=0;
				if (!nametable) continue;

				for(ii=0;ii<nodes_num;ii++) {
					if (forward[i].zone==nametable[ii].zone&&
						 forward[i].net==nametable[ii].net&&
						 forward[i].node==nametable[ii].node&&
						 forward[i].point==nametable[ii].point&&
						 !stricmp(msg.to,nametable[ii].name)) {
							 forward[i].export=1;
							 break;
					}
				}
			}


		}

		mi = i = 0;
		pp = 0;
		n_seen = 0;

		while ((a=(byte)fgetc(f2)) != 0 && !feof(f2)) {
			c = a;

			switch(mpkt.pktype) {
				case 2:
					c = a;
					break;
            case 10:
               if (a!=10) {
                  if (a == 141)
                     a='\r';

                  if (a<128)
                     c=a;
                  else {
                     if (a==128) {
                        a=(byte)fgetc(f2);
                        c=(a)+127;
                     }
                     else {
                        buff[mi] = '\0';
                        strcat(buff,pip[a-128]);
                        mi += strlen(pip[a-128]);
                        c = buff[--mi];
                     }
                  }
               }
               else
                  c='\0';
               break;
            default:
               return(1);
         }

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
            else if ((hdr.status & SET_PKT_FROMUS) && !strncmp (buff, "--- ", 4) && (config->replace_tear || !registered))
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

         for (i = 0; i < maxnodes; i++) {
            if (!forward[i].export || forward[i].net == config->alias[sys.use_alias].fakenet)
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
      mhdr.cost = 0;
      mhdr.attrib = 0;

      if (sys.public)
         mpkt.attribute &= ~SET_PKT_PRIV;
      else if (sys.private)
         mpkt.attribute |= SET_PKT_PRIV;
      if (mpkt.attribute & SET_PKT_PRIV)
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

         write (mi, msg.date, strlen (msg.date) + 1);
         write (mi, msg.to, strlen (msg.to) + 1);
         write (mi, msg.from, strlen (msg.from) + 1);
         write (mi, msg.subj, strlen (msg.subj) + 1);
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

         time_release ();
      }

      lseek (f1, hdrpos, SEEK_SET);
      hdr.status |= SET_MPTR_SENT;
      write(f1, (char *)&hdr, sizeof(MSGPTR));
      hdrpos = tell (f1);

      wputs ("\n");
      sent++;
   }

   if (sent)
      status_line (":   %-20.20s (Sent=%04d)", sys.echotag, sent);

   mclose (fpd);
   free (seen);
   fclose (f2);
   close (f1);
   unlink ("MSGTMP.EXP");

   return (maxnodes);
}

void pip_rescan_echomail (int board, char *tag, int zone, int net, int node, int point)
{
   FILE *fpd, *f2;
   int f1, i, pp, z, ne, no, n_seen, cnet, cnode, mi, msgnum, m, num_msg, sent, fd, ai = 0;
   char buff[80], wrp[80], c, *p, buffer[2050], need_origin, need_seen;
   byte a;
   MSGPTR hdr;
   MSGPKTHDR mpkt;
   struct _msghdr2 mhdr;
   struct _fwrd *seen;
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

   strcpy (buff, tag);
   memset ((char *)&sys, 0, sizeof (struct _sys));
   strcpy (sys.echotag, buff);
   sys.pip_board = board;

   sprintf(buff,"%sMPTR%04x.PIP",pip_msgpath,sys.pip_board);
   f1 = open(buff, O_RDWR|O_BINARY);
   if(f1 == -1)
      return;

   num_msg = (int)(filelength(f1) / sizeof(MSGPTR));

   sprintf(buff,"%sMPKT%04x.PIP",pip_msgpath,sys.pip_board);
   f2 = fopen(buff,"rb+");
   if (f2 == NULL) {
      close (f1);
      return;
   }

   seen = (struct _fwrd *)malloc ((MAX_EXPORT_SEEN + 1) * sizeof (struct _fwrd));
   if (seen == NULL) {
      close (f1);
      fclose (f2);
      return;
   }

   fpd = fopen ("MSGTMP.EXP", "rb+");
   if (fpd == NULL) {
      fpd = fopen ("MSGTMP.EXP", "wb");
      fclose (fpd);
   }
   else
      fclose (fpd);
   fpd = mopen ("MSGTMP.EXP", "r+b");

   msgnum = 0;

   strcpy (wrp, sys.echotag);
   wrp[14] = '\0';
   prints (8, 65, YELLOW|_BLACK, "              ");
   prints (8, 65, YELLOW|_BLACK, wrp);

   prints (7, 65, YELLOW|_BLACK, "              ");
   prints (9, 65, YELLOW|_BLACK, "              ");
   prints (9, 65, YELLOW|_BLACK, "Pip-Base");

   sent = 0;

   while (read(f1, (char *)&hdr,sizeof(MSGPTR)) == sizeof (MSGPTR)) {
      sprintf (wrp, "%d / %d", ++msgnum, num_msg);
      prints (7, 65, YELLOW|_BLACK, wrp);

      if ( !(msgnum % 20))
         time_release ();

      if ((hdr.status & SET_MPTR_DEL))
         continue;

      sprintf (wrp, "%5d  %-22.22s Pip-Base     ", msgnum, sys.echotag);
      wputs (wrp);

      need_seen = need_origin = 1;

      mseek (fpd, 0L, SEEK_SET);
      mprintf (fpd, "AREA:%s\r\n", sys.echotag);

      fseek(f2,hdr.pos,SEEK_SET);
      fread(&mpkt,sizeof(MSGPKTHDR),1,f2);

      memset ((char *)&msg, 0, sizeof (struct _msg));
      read0(msg.date,f2);
      read0(msg.to,f2);
      read0(msg.from,f2);
      read0(msg.subj,f2);

      if (mpkt.attribute & SET_PKT_PRIV)
         msg.attr |= MSGPRIVATE;

      mi = i = 0;
      pp = 0;
      n_seen = 0;

      while ((a=(byte)fgetc(f2)) != 0 && !feof(f2)) {
         c = a;

         switch(mpkt.pktype) {
            case 2:
               c = a;
               break;
            case 10:
               if (a!=10) {
                  if (a == 141)
                     a='\r';

                  if (a<128)
                     c=a;
                  else {
                     if (a==128) {
                        a=(byte)fgetc(f2);
                        c=(a)+127;
                     }
                     else {
                        buff[mi] = '\0';
                        strcat(buff,pip[a-128]);
                        mi += strlen(pip[a-128]);
                        c = buff[--mi];
                     }
                  }
               }
               else
                  c='\0';
               break;
            default:
               return;
         }

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

                  if (n_seen >= MAX_EXPORT_SEEN)
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
            else if ((hdr.status & SET_PKT_FROMUS) && !strncmp (buff, "--- ", 4) && (config->replace_tear || !registered))
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
      mhdr.cost = 0;
      mhdr.attrib = 0;

      if (sys.public)
         mpkt.attribute &= ~SET_PKT_PRIV;
      else if (sys.private)
         mpkt.attribute |= SET_PKT_PRIV;
      if (mpkt.attribute & SET_PKT_PRIV)
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

      write (mi, msg.date, strlen (msg.date) + 1);
      write (mi, msg.to, strlen (msg.to) + 1);
      write (mi, msg.from, strlen (msg.from) + 1);
      write (mi, msg.subj, strlen (msg.subj) + 1);
      mseek (fpd, 0L, SEEK_SET);
      do {
         z = mread(buffer, 1, 2048, fpd);
         write(mi, buffer, z);
      } while (z == 2048);
      buff[0] = buff[1] = buff[2] = 0;
      if (write (mi, buff, 3) != 3)
         return;
      close (mi);

      totalmsg++;
      sprintf (wrp, "%d (%.1f/s) ", totalmsg, (float)totalmsg / ((float)(timerset (0) - totaltime) / 100));
      prints (10, 65, YELLOW|_BLACK, wrp);

      time_release ();
      wputs ("\n");
      sent++;
   }

   if (sent)
      status_line (":   %-20.20s (Sent=%04d)", sys.echotag, sent);

   mclose (fpd);
   free (seen);
   fclose (f2);
   close (f1);
   unlink ("MSGTMP.EXP");
}

