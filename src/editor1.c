#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <dir.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <cxl\cxlvid.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"


int write_message_text(msg_num, flags, quote_name, sm)
int msg_num, flags;
char *quote_name;
FILE *sm;
{
        FILE *fp, *fpq;
        int i, z, m, pos, blks;
        char c, buff[80], wrp[80], qwkbuffer[130], shead;
        long fpos, qpos;
        struct _msg msgt;
        struct QWKmsghd QWK;

        sprintf(buff,"%s%d.MSG",sys.msg_path,msg_num);
        fp = fopen(buff,"r+b");
        if(fp == NULL)
                return(0);

        fread((char *)&msgt,sizeof(struct _msg),1,fp);

        if(msgt.attr & MSGPRIVATE)
        {
                if(msg_num == 1 && sys.echomail)
                {
			fclose(fp);
			return(0);
		}

                if(stricmp(msgt.from,usr.name) && stricmp(msgt.to,usr.name))
                {
			fclose(fp);
			return(0);
		}
        }

        blks = 1;
        pos = 0;
        memset (qwkbuffer, ' ', 128);
        shead = 0;

        if (sm == NULL)
        {
           fpq = fopen(quote_name, (flags & APPEND_TEXT) ? "at" : "wt");
           if (fpq == NULL)
                return(0);
        }
        else
           fpq = sm;

        i = 0;
        if (flags & QUOTE_TEXT)
        {
                strcpy(buff, " > ");
                i = strlen(buff);
        }

        fpos = filelength(fileno(fp));

        while(ftell(fp) < fpos)
        {
                c = fgetc(fp);

                if((byte)c == 0x8D || c == 0x0A || c == '\0')
                        continue;

                buff[i++]=c;

                if(c == 0x0D) {
                        buff[i-1]='\0';

                        if(buff[0] == 0x01)
                        {
                                if (!strncmp(&buff[1],"INTL",4) && !shead)
                                        sscanf(&buff[6],"%d:%d/%d %d:%d/%d",&msg_tzone,&i,&i,&msg_fzone,&i,&i);
                                if (!strncmp(&buff[1],"TOPT",4) && !shead)
                                        sscanf(&buff[6],"%d",&msg_tpoint);
                                if (!strncmp(&buff[1],"FMPT",4) && !shead)
                                        sscanf(&buff[6],"%d",&msg_fpoint);
                                i=0;
				continue;
			}
                        else if (!shead)
                        {
                                if (flags & INCLUDE_HEADER)
                                        text_header (&msgt,msg_num,fpq);
                                else if (flags & QWK_TEXTFILE)
                                        qwk_header (&msgt,&QWK,msg_num,fpq,&qpos);
                                shead = 1;
                        }

                        if(buff[0] == 0x01 || !strncmp(buff,"SEEN-BY",7)) {
                                i=0;
                                continue;
                        }

                        if (flags & QWK_TEXTFILE)
                        {
                                write_qwk_string (buff, qwkbuffer, &pos, &blks, fpq);
                                write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpq);
                        }
                        else
                                fprintf(fpq,"%s\n",buff);

                        if (flags & QUOTE_TEXT)
                        {
                                strcpy(buff, " > ");
                                i = strlen(buff);
                        }
                        else
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
                                for(z=i+1;z<(usr.width-1);z++) {
                                        wrp[m++]=buff[z];
                                        buff[i]='\0';
                                }

                        wrp[m]='\0';

                        if (!shead)
                        {
                                if (flags & INCLUDE_HEADER)
                                        text_header (&msgt,msg_num,fpq);
                                else if (flags & QWK_TEXTFILE)
                                        qwk_header (&msgt,&QWK,msg_num,fpq,&qpos);
                                shead = 1;
                        }

                        if (flags & QWK_TEXTFILE)
                        {
                                write_qwk_string (buff, qwkbuffer, &pos, &blks, fpq);
                                write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpq);
                        }
                        else
                                fprintf(fpq,"%s\n",buff);

                        if (flags & QUOTE_TEXT)
                                strcpy(buff, " > ");
                        else
                                buff[0] = '\0';
                        strcat(buff,wrp);
                        i = strlen(wrp);
                }
        }

        fclose(fp);

        if (flags & QWK_TEXTFILE)
        {
                qwkbuffer[128] = 0;
                fwrite(qwkbuffer, 128, 1, fpq);
                blks++;

             /* Now update with record count */
                fseek(fpq,qpos,SEEK_SET);          /* Restore back to header start */
                sprintf(buff,"%d",blks);
                ljstring(QWK.Msgrecs,buff,6);
                fwrite((char *)&QWK,128,1,fpq);           /* Write out the header */
                fseek(fpq,0L,SEEK_END);               /* Bump back to end of file */

                if (sm == NULL)
                        fclose(fpq);

                return (blks);
        }
        else
                fprintf(fpq,bbstxt[B_TWO_CR]);

        if (sm == NULL)
                fclose(fpq);

        return(1);
}

void write_qwk_string (st, text, pos, nb, fp)
char *st, *text;
int *pos, *nb;
FILE *fp;
{
        word v, m, blocks;

        m = *pos;
        blocks = *nb;

        for (v=0; st[v]; v++)
        {
                if (st[v] == '\r' && st[v+1] == '\n' ||
                    st[v] == '\n')
                {
                   v++;
                   text[m++] = 0xE3;
                }
                else
                   text[m++] = st[v];

                if (m == 128)
                {
                        fwrite(text, 128, 1, fp);
                        memset(text, ' ', 128);
                        m = 0;
                        blocks++;
                }
        }

        *pos = m;
        *nb = blocks;
}

int external_editor (quote)
int quote;
{
   int never;
   char filename[20];
   struct stat st1, st2;

   strcpy (filename, "MSGTMP");

   if (quote) {
      if (sys.quick_board)
         quick_write_message_text(lastread, 2, filename, NULL);
      else if (sys.pip_board)
         pip_write_message_text(lastread, 2, filename, NULL);
      else if (sys.squish)
         squish_write_message_text(lastread, 2, filename, NULL);
      else
         write_message_text(lastread, 2, filename, NULL);
   }

   if (stat (filename, &st1))
      never = 1;
   else
      never = 0;

   outside_door(ext_editor);

   if (stat (filename, &st2) || !st2.st_size) {
      unlink (filename);
      change_attr (LRED|_BLACK);
      m_print(bbstxt[B_LORE_MSG3]);
      return (0);
   }

   if (!never && st1.st_mtime == st2.st_mtime) {
      unlink (filename);
      change_attr (LRED|_BLACK);
      m_print (bbstxt[B_LORE_MSG3]);
      return (0);
   }

   cls ();

   if (sys.quick_board)
      quick_save_message (filename);
   else if (sys.pip_board)
      pip_save_message (filename);
   else if (sys.squish)
      squish_save_message (filename);
   else
      save_message (filename);

   usr.msgposted++;

   unlink (filename);
   return (1);
}

void text_header (msg_ptr, s, fp)
struct _msg *msg_ptr;
int s;
FILE *fp;
{
   char stringa[50];

   adjust_date(msg_ptr);

   fprintf (fp,"From    : ");

   if(sys.netmail)
   {
      if (!msg_fzone)
         msg_fzone = alias[0].zone;
      sprintf(stringa,"%-25.25s (%d:%d/%d.%d)",msg_ptr->from,msg_fzone,msg_ptr->orig_net,msg_ptr->orig,msg_fpoint);
      fprintf (fp,"%36s   ",stringa);
   }
   else
      fprintf (fp,"%-36s   ",msg_ptr->from);

   if(msg_ptr->attr & MSGPRIVATE)
      fprintf (fp,"PRIV. ");
   if(msg_ptr->attr & MSGCRASH)
      fprintf (fp,"C.M. ");
   if(msg_ptr->attr & MSGREAD)
      fprintf (fp,"LETTO ");
   if(msg_ptr->attr & MSGSENT)
      fprintf (fp,"INV. ");
   if(msg_ptr->attr & MSGFILE)
      fprintf (fp,"F/ATT. ");
   if(msg_ptr->attr & MSGFWD)
      fprintf (fp,"TRANS. ");
   if(msg_ptr->attr & MSGORPHAN)
      fprintf (fp,"ORFANO ");
   if(msg_ptr->attr & MSGKILL)
      fprintf (fp,"CANC. ");
   if(msg_ptr->attr & MSGHOLD)
      fprintf (fp,"TRATT. ");
   if(msg_ptr->attr & MSGFRQ)
      fprintf (fp,"F/REQ. ");
   if(msg_ptr->attr & MSGRRQ)
      fprintf (fp,"R/REQ. ");
   if(msg_ptr->attr & MSGCPT)
      fprintf (fp,"RICEV. ");
   if(msg_ptr->attr & MSGARQ)
      fprintf (fp,"AUDIT. ");
   if(msg_ptr->attr & MSGURQ)
      fprintf (fp,"UPDATE ");
   fprintf (fp,bbstxt[B_ONE_CR]);

   fprintf (fp,"To      : ");
   if(sys.netmail)
   {
      if (!msg_tzone)
         msg_tzone = alias[0].zone;
      sprintf(stringa,"%-25.25s (%d:%d/%d.%d)",msg_ptr->to,msg_tzone,msg_ptr->dest_net,msg_ptr->dest,msg_tpoint);
      fprintf (fp,"%36s   ",stringa);
   }
   else
      fprintf (fp,"%-36s   ",msg_ptr->to,msg_ptr->dest_net,msg_ptr->dest);

   if (s)
      fprintf (fp,"Msg: #%d, ",s);
   fprintf (fp,"%s ",show_date(dateformat,stringa,msg_ptr->date_written.date,msg_ptr->date_written.time));
   fprintf (fp,"%s\n",show_date(timeformat,stringa,msg_ptr->date_written.date,msg_ptr->date_written.time));

   if(msg_ptr->attr & MSGFILE || msg_ptr->attr & MSGFRQ)
      fprintf (fp,"File(s) : ");
   else
      fprintf (fp,"Subject : ");
   fprintf (fp,"%s\n\n",msg_ptr->subj);
}

void qwk_header (msg_ptr, QMsgHead, s, fp, qpos)
struct _msg *msg_ptr;
struct QWKmsghd *QMsgHead;
int s;
FILE *fp;
long *qpos;
{
   int yr, mo, dy, hh, mm;
   char msg_line[40], month[4];

   sscanf(msg_ptr->date,"%2d %3s %2d  %2d:%2d",&dy,month,&yr,&hh,&mm);
   month[3]='\0';

   for(mo=0;mo<12;mo++)
      if(!stricmp(month,mtext[mo]))
         break;

   if(mo == 12)
      mo = 0;

   mo++;

   memset (QMsgHead, ' ', sizeof (struct QWKmsghd));

/* Fill out some defaults in the Messages.dat file header */
   memset(QMsgHead->Msgpass,' ',12);
   memset(QMsgHead->Msgfiller,' ',3);
   QMsgHead->Msglive = 0xe1;

/* Now build the messages.dat msg header */
   if (msg_ptr->attr & MSGPRIVATE)
      QMsgHead->Msgstat = '*';
   else {
      if (!sys.echomail) {         /* Local area */
         if ((msg_ptr->attr & MSGREAD) && !stricmp (msg_ptr->to, usr.name))
            QMsgHead->Msgstat = '-';
         else
            QMsgHead->Msgstat = ' ';
      }
      else
         QMsgHead->Msgstat = ' ';
   }

   sprintf(msg_line,"%d",s);
   ljstring(QMsgHead->Msgnum,msg_line,7);

   sprintf(msg_line,"%02d-%02d-%02d",mo,dy,yr);
   memcpy(QMsgHead->Msgdate,msg_line,8);
   sprintf(msg_line,"%02d:%02d",hh,mm);
   memcpy(QMsgHead->Msgtime,msg_line,5);

   ljstring(QMsgHead->Msgto,msg_ptr->to,25);
   ljstring(QMsgHead->Msgfrom,msg_ptr->from,25);
   ljstring(QMsgHead->Msgsubj,msg_ptr->subj,25);
   sprintf(msg_line,"%d",msg_ptr->reply);
   ljstring(QMsgHead->Msgrply,msg_line,8);

   sys.msg_num--;
   QMsgHead->Msgarealo = (char) (sys.msg_num % 255); /* Lo byte area # */
   QMsgHead->Msgareahi = (char) (sys.msg_num / 255); /* Hi byte area # */
   sys.msg_num++;

   *qpos = ftell (fp);
   fwrite(QMsgHead,128,1,fp);           /* Write out the header */
}

