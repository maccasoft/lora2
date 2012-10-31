#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "quickmsg.h"

#define MAX_MESSAGES    1000

char *pascal_string (char *);
int quick_msg_attrib(struct _msghdr *, int, int, int);
void quick_text_header(struct _msghdr *, int, FILE *);
int quick_full_read_message (int);
void write_pascal_string (char *, char *, word *, word *, FILE *);
void quick_qwk_text_header (struct _msghdr *, int, FILE *, struct QWKmsghd *, long *);
void add_quote_string (char *str, char *from);
void add_quote_header (FILE *fp, char *from, char *to, char *dt, char *tm);

extern int msgidx[MAX_MESSAGES], msg_parent, msg_child;
extern struct _msginfo msginfo;
extern char *internet_to;

void quick_scan_message_base(board, area, upd)
int board, area;
char upd;
{
   #define MAXREADIDX   100
   int fd, tnum_msg, i, m, x;
   char filename[80];
   struct _msgidx idx[MAXREADIDX];

   sprintf(filename,"%sMSGINFO.BBS",fido_msgpath);
   fd = sh_open (filename, SH_DENYWR, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
   read(fd, (char *)&msginfo, sizeof(struct _msginfo));
   close (fd);

   msg_parent = msg_child = 0;
   tnum_msg = 0;
   msginfo.totalonboard[board-1] = 0;
   msgidx[0] = 0;
   i = 1;

   sprintf(filename, "%sMSGIDX.BBS",fido_msgpath);
   fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);

   do {
      m = read (fd, (char *)&idx, sizeof(struct _msgidx) * MAXREADIDX);
      m /= sizeof (struct _msgidx);

      for (x = 0; x < m; x++) {
         if (idx[x].board == board && idx[x].msgnum) {
            msgidx[i++] = tnum_msg;
            msginfo.totalonboard[board-1]++;
         }

         tnum_msg++;

         if (i >= MAX_MESSAGES)
            break;
      }
   } while (m == MAXREADIDX);

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

   for (i=0;i<MAXLREAD;i++)
      if (usr.lastread[i].area == area)
         break;
   if (i != MAXLREAD) {
      if (usr.lastread[i].msg_num > last_msg)
         usr.lastread[i].msg_num = last_msg;
      lastread = usr.lastread[i].msg_num;
   }
   else {
      for (i=0;i<MAXDLREAD;i++)
         if (usr.dynlastread[i].area == area)
            break;
      if (i != MAXDLREAD) {
         if (usr.dynlastread[i].msg_num > last_msg)
            usr.dynlastread[i].msg_num = last_msg;
         lastread = usr.dynlastread[i].msg_num;
      }
      else if (upd) {
         lastread = 0;
         for (i=1;i<MAXDLREAD;i++) {
            usr.dynlastread[i-1].area = usr.dynlastread[i].area;
            usr.dynlastread[i-1].msg_num = usr.dynlastread[i].msg_num;
         }

         usr.dynlastread[i-1].area = area;
         usr.dynlastread[i-1].msg_num = 0;
      }
      else
         lastread = 0;
   }
}

int quick_read_message(msg_num, flag)
int msg_num, flag;
{
        FILE *fp;
        int i, z, m, line, colf=0, pp, shead, absnum;
        char c, buff[80], wrp[80], *p;
        long fpos;
        struct _msghdr msghdr;

        if (usr.full_read && !flag)
                return(quick_full_read_message(msg_num));

        line = 1;
        shead = 0;
        msg_fzone = msg_tzone = config->alias[sys.use_alias].zone;
        msg_fpoint = msg_tpoint = 0;
        absnum = msgidx[msg_num];

        sprintf(buff, "%sMSGHDR.BBS", fido_msgpath);
        i = sh_open (buff, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
        if (i == -1)
                return (0);
        fp = fdopen (i, "r+b");
        if(fp == NULL)
                return (0);

        fseek(fp,(long)sizeof(struct _msghdr) * absnum,SEEK_SET);
        fread((char *)&msghdr,sizeof(struct _msghdr),1,fp);

        if(msghdr.msgattr & Q_RECKILL)
                return (0);

        if(msghdr.msgattr & Q_PRIVATE)
                if(stricmp(pascal_string(msghdr.whofrom),usr.name) &&
                   stricmp(pascal_string(msghdr.whoto),usr.name) &&
                   usr.priv != SYSOP)
                {
                        fclose(fp);
                        return(0);
                }

        msghdr.timesread++;

        if (!flag)
                cls();

        if(flag)
                m_print(bbstxt[B_TWO_CR]);

        allow_reply = 1;

        if(!stricmp(pascal_string(msghdr.whoto),usr.name))
                msghdr.msgattr |= Q_RECEIVED;

        fseek(fp,(long)sizeof(struct _msghdr) * absnum,SEEK_SET);
        fwrite((char *)&msghdr,sizeof(struct _msghdr),1,fp);

        fclose(fp);

        for (i = first_msg; i <= last_msg; i++)
           if (msghdr.prevreply == msgidx[i])
              break;
        if (i <= last_msg)
           msg_parent = i;

        for (i = first_msg; i <= last_msg; i++)
           if (msghdr.nextreply == msgidx[i])
              break;
        if (i <= last_msg)
           msg_child = i;

        sprintf (buff,"%sMSGTXT.BBS",fido_msgpath);
        i = sh_open (buff, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
        fp = fdopen(i,"rb");

        fpos = 256L*(msghdr.startblock+msghdr.numblocks-1);
        fseek(fp,fpos,SEEK_SET);
        fpos += fgetc(fp);

        fseek(fp,256L*msghdr.startblock,SEEK_SET);

        change_attr(CYAN|_BLACK);

        i = 0;
        pp = 0;

        while(ftell(fp) < fpos) {
                if (!pp)
                        fgetc(fp);

                c = fgetc(fp);
                pp++;

                if (c == '\0')
                        break;

                if (pp == 255)
                        pp = 0;

                if((byte)c == 0x8D || c == 0x0A || c == '\0')
                        continue;

                buff[i++]=c;

                if(c == 0x0D) {
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
                                line = quick_msg_attrib(&msghdr,msg_num,line,0);
                                m_print(bbstxt[B_ONE_CR]);
                                shead = 1;
                        }

                        if(!strncmp(buff,"SEEN-BY",7)) {
                                i=0;
                                continue;
                        }

                        if(buff[0] == 0x01) {
                                change_attr(YELLOW|_BLUE);
                                m_print("%s\n",&buff[1]);
                                change_attr(CYAN|BLACK);
                        }
                        else {
                                p = &buff[0];

                                if(((strchr(buff,'>') - p) < 6) && (strchr(buff,'>'))) {
                                        if(!colf) {
                                                change_attr(WHITE|BLACK);
                                                colf=1;
                                        }
                                }
                                else {
                                        if(colf) {
                                                change_attr(CYAN|_BLACK);
                                                colf=0;
                                        }
                                }

                                if (!strncmp(buff,msgtxt[M_TEAR_LINE],4) || !strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
                                        m_print("%s\n",buff);
                                else
                                        m_print("%s\n",buff);

                                if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
                                        gather_origin_netnode (buff);
                        }

                        i=0;

                        if(flag == 1)
                                line=1;

                        if(!(++line < (usr.len-1)) && usr.more) {
                                if(!continua()) {
                                        flag=1;
                                        break;
                                }
                                else
                                        line=1;
                        }
                }
                else {
                        if(i<(usr.width-2))
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
                                line = quick_msg_attrib(&msghdr,msg_num,line,0);
                                m_print(bbstxt[B_ONE_CR]);
                                shead = 1;
                        }

                        if (((strchr(buff,'>') - buff) < 6) && (strchr(buff,'>'))) {
                                if(!colf) {
                                        change_attr(WHITE|_BLACK);
                                        colf=1;
                                }
                        }
                        else {
                                if(colf) {
                                        change_attr(CYAN|_BLACK);
                                        colf=0;
                                }
                        }

                        if (!strncmp(buff,msgtxt[M_TEAR_LINE],4) || !strncmp(buff,msgtxt[M_ORIGIN_LINE],10))
                                m_print("%s\n",buff);
                        else
                                m_print("%s\n",buff);

                        if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
                                gather_origin_netnode (buff);

                        if(flag == 1)
                                line=1;

                        if(!(++line < (usr.len-1)) && usr.more) {
                                if(!continua()) {
                                        flag=1;
                                        break;
                                }
                                else
                                        line=1;
                        }

                        strcpy(buff,wrp);
                        i=strlen(wrp);
                }
        }
        fclose(fp);

        if (!shead) {
                line = quick_msg_attrib(&msghdr,msg_num,line,0);
                m_print(bbstxt[B_ONE_CR]);
                shead = 1;
        }

        if (line > 1 && !flag && usr.more) {
                press_enter();
                line=1;
        }

        return(1);
}

int quick_full_read_message(msg_num)
int msg_num;
{
        FILE *fp;
        int i, z, m, line, colf=0, pp, shead, absnum;
        char c, buff[80], wrp[80], *p;
        long fpos;
        struct _msghdr msghdr;

        line = 2;
        shead = 0;
        msg_fzone = msg_tzone = config->alias[sys.use_alias].zone;
        msg_fpoint = msg_tpoint = 0;
        absnum = msgidx[msg_num];

        sprintf(buff, "%sMSGHDR.BBS", fido_msgpath);
        i = sh_open (buff, SH_DENYNONE, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
        if (i == -1)
                return (0);
        fp = fdopen(i,"r+b");
        if(fp == NULL)
                return(0);

        fseek(fp,(long)sizeof(struct _msghdr) * absnum,SEEK_SET);
        fread((char *)&msghdr,sizeof(struct _msghdr),1,fp);

        if(msghdr.msgattr & Q_RECKILL)
                return (0);

        if(msghdr.msgattr & Q_PRIVATE)
                if(stricmp(pascal_string(msghdr.whofrom),usr.name) &&
                   stricmp(pascal_string(msghdr.whoto),usr.name) &&
                   usr.priv != SYSOP)
                {
                        fclose(fp);
                        return(0);
                }

        cls();

        msghdr.timesread++;
        allow_reply = 1;

        if(!stricmp(pascal_string(msghdr.whoto),usr.name))
                msghdr.msgattr |= Q_RECEIVED;

        fseek(fp,(long)sizeof(struct _msghdr) * absnum,SEEK_SET);
        fwrite((char *)&msghdr,sizeof(struct _msghdr),1,fp);

        fclose(fp);

        for (i = first_msg; i <= last_msg; i++)
           if (msghdr.prevreply == msgidx[i])
              break;
        if (i <= last_msg)
           msg_parent = i;
        else
           msg_parent = 0;

        for (i = first_msg; i <= last_msg; i++)
           if (msghdr.nextreply == msgidx[i])
              break;
        if (i <= last_msg)
           msg_child = i;
        else
           msg_child = 0;

        sprintf(buff,"%sMSGTXT.BBS",fido_msgpath);
        i = sh_open (buff, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
        fp = fdopen(i,"rb");

        fpos = 256L*(msghdr.startblock+msghdr.numblocks-1);
        fseek(fp,fpos,SEEK_SET);
        fpos += fgetc(fp);

        fseek(fp,256L*msghdr.startblock,SEEK_SET);

        change_attr(BLUE|_LGREY);
        del_line();
        m_print(" * %s\n", sys.msg_name);

        i = 0;
        pp = 0;

        while(ftell(fp) < fpos) {
                if (!pp)
                        fgetc(fp);

                c = fgetc(fp);
                pp++;

                if (c == '\0')
                        break;

                if (pp == 255)
                        pp = 0;

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
                                line = quick_msg_attrib(&msghdr,msg_num,line,0);
                                change_attr(LGREY|_BLUE);
                                del_line();
                                change_attr(CYAN|_BLACK);
                                m_print(bbstxt[B_ONE_CR]);
                                shead = 1;
                        }

                        if(!strncmp(buff,"SEEN-BY",7))
                        {
                                i=0;
                                continue;
                        }

                        if(buff[0] == 0x01)
                        {
                                change_attr(YELLOW|_BLUE);
                                m_print("%s\n",&buff[1]);
                                change_attr(CYAN|BLACK);
                        }
                        else
                        {
                                p = &buff[0];

                                if(((strchr(buff,'>') - p) < 6) && (strchr(buff,'>')))
                                {
                                        if(!colf)
                                        {
                                                change_attr(WHITE|BLACK);
                                                colf=1;
                                        }
                                }
                                else {
                                        if(colf) {
                                                change_attr(CYAN|_BLACK);
                                                colf=0;
                                        }
                                }

                                if (!strncmp(buff,msgtxt[M_TEAR_LINE],4) || !strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
                                        m_print("%s\n",buff);
                                else
                                        m_print("%s\n",buff);

                                if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
                                        gather_origin_netnode (buff);
                        }

                        i=0;

                        if(!(++line < (usr.len-1)) && usr.more)
                        {
                                if(!continua())
                                {
                                        line = 1;
                                        break;
                                }
                                else
                                {
                                        while (line > 5) {
                                                cpos(line--, 1);
                                                del_line();
                                        }
                                }
                        }
                }
                else
                {
                        if(i<(usr.width-2))
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
                                line = quick_msg_attrib(&msghdr,msg_num,line,0);
                                change_attr(LGREY|_BLUE);
                                del_line();
                                change_attr(CYAN|_BLACK);
                                m_print(bbstxt[B_ONE_CR]);
                                shead = 1;
                        }

                        if (((strchr(buff,'>') - buff) < 6) && (strchr(buff,'>')))
                        {
                                if(!colf)
                                {
                                        change_attr(WHITE|_BLACK);
                                        colf=1;
                                }
                        }
                        else
                        {
                                if(colf)
                                {
                                        change_attr(CYAN|_BLACK);
                                        colf=0;
                                }
                        }

                        if (!strncmp(buff,msgtxt[M_TEAR_LINE],4) || !strncmp(buff,msgtxt[M_ORIGIN_LINE],10))
                                m_print("%s\n",buff);
                        else
                                m_print("%s\n",buff);

                        if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
                                gather_origin_netnode (buff);

                        if(!(++line < (usr.len-1)) && usr.more)
                        {
                                if(!continua())
                                {
                                        line = 1;
                                        break;
                                }
                                else
                                {
                                        while (line > 5) {
                                                cpos(line--, 1);
                                                del_line();
                                        }
                                }
                        }

                        strcpy(buff,wrp);
                        i=strlen(wrp);
                }
        }

        fclose(fp);

        if (!shead) {
                line = quick_msg_attrib(&msghdr,msg_num,line,0);
                m_print(bbstxt[B_ONE_CR]);
                shead = 1;
        }

        if(line > 1 && usr.more) {
                press_enter();
                line=1;
        }

        return(1);
}

int quick_msg_attrib (msgt, s, line, f)
struct _msghdr *msgt;
int s, line, f;
{
   int dy, mo, yr, hh, mm;
   char stringa[20];
   struct _msg tmsg;

   memset ((char *)&tmsg, 0, sizeof (struct _msg));

   if (msgt->msgattr & Q_PRIVATE)
      tmsg.attr |= MSGPRIVATE;
   if(msgt->msgattr & Q_RECEIVED)
      tmsg.attr |= MSGREAD;
   if(msgt->netattr & Q_CRASH)
      tmsg.attr |= MSGCRASH;
   if(msgt->netattr & Q_SENT)
      tmsg.attr |= MSGSENT;
   if(msgt->netattr & Q_FILE)
      tmsg.attr |= MSGFILE;
   if(msgt->netattr & Q_KILL)
      tmsg.attr |= MSGKILL;
   if(msgt->netattr & Q_FRQ)
      tmsg.attr |= MSGFRQ;
   if(msgt->netattr & Q_CPT)
      tmsg.attr |= MSGCPT;
   if(msgt->netattr & Q_ARQ)
      tmsg.attr |= MSGARQ;

   strcpy (tmsg.to, pascal_string (msgt->whoto));
   strcpy (tmsg.from, pascal_string (msgt->whofrom));
   strcpy (tmsg.subj, pascal_string (msgt->subject));

   strcpy(stringa,pascal_string(msgt->date));
   sscanf(stringa,"%2d-%2d-%2d",&mo,&dy,&yr);
   strcpy(stringa,pascal_string(msgt->time));
   sscanf(stringa,"%2d:%2d",&hh,&mm);
   sprintf(tmsg.date, "%02d %s %02d  %2d:%02d:00", dy, mtext[mo-1], yr, hh, mm);

   line = msg_attrib (&tmsg, s, line, f);
   memcpy ((char *)&msg, (char *)&tmsg, sizeof (struct _msg));

   return (line);
}

void quick_text_header(msgt, s, fp)
struct _msghdr *msgt;
int s;
FILE *fp;
{
   int dy, mo, yr, hh, mm;
   char stringa[20];
   struct _msg tmsg;

   memset ((char *)&tmsg, 0, sizeof (struct _msg));

   if (msgt->msgattr & Q_PRIVATE)
      tmsg.attr |= MSGPRIVATE;
   if(msgt->msgattr & Q_RECEIVED)
      tmsg.attr |= MSGREAD;
   if(msgt->netattr & Q_CRASH)
      tmsg.attr |= MSGCRASH;
   if(msgt->netattr & Q_SENT)
      tmsg.attr |= MSGSENT;
   if(msgt->netattr & Q_FILE)
      tmsg.attr |= MSGFILE;
   if(msgt->netattr & Q_KILL)
      tmsg.attr |= MSGKILL;
   if(msgt->netattr & Q_FRQ)
      tmsg.attr |= MSGFRQ;
   if(msgt->netattr & Q_CPT)
      tmsg.attr |= MSGCPT;
   if(msgt->netattr & Q_ARQ)
      tmsg.attr |= MSGARQ;

   strcpy (tmsg.to, pascal_string (msgt->whoto));
   strcpy (tmsg.from, pascal_string (msgt->whofrom));
   strcpy (tmsg.subj, pascal_string (msgt->subject));

   strcpy(stringa,pascal_string(msgt->date));
   sscanf(stringa,"%2d-%2d-%2d",&mo,&dy,&yr);
   strcpy(stringa,pascal_string(msgt->time));
   sscanf(stringa,"%2d:%2d",&hh,&mm);
   sprintf(tmsg.date, "%02d %s %02d  %2d:%02d:00", dy, mtext[mo-1], yr, hh, mm);

   text_header (&tmsg, s, fp);
}

char *pascal_string(s)
char *s;
{
   strncpy(e_input,(char *)&s[1],(int)s[0]);
   e_input[(int)s[0]] = '\0';

   return (e_input);
}

int quick_write_message_text(msgn, flags, stringa, sm)
int msgn, flags;
char *stringa;
FILE *sm;
{
   FILE *fp, *fpt;
   int i, m, z, pp, absnum, blks, pos;
   char filename[80], c, buff[80], wrp[80], shead, qwkbuffer[130];
   char qfrom[36], qto[36], qtime[10], qdate[10], *p;
   long fpos, qpos;
   struct _msghdr msgt;
   struct QWKmsghd QWK;

   absnum = msgidx[msgn];

   sprintf(filename,"%sMSGHDR.BBS", fido_msgpath);
   fp = fopen(filename,"r+b");
   if (fp == NULL)
      return (0);

   fseek(fp,(long)sizeof(struct _msghdr) * absnum,SEEK_SET);
   fread((char *)&msgt,sizeof(struct _msghdr),1,fp);

   fclose(fp);

   blks = 1;
   shead = 0;
   pos = 0;

   if (msgt.msgattr & Q_RECKILL)
      return (0);

   if (msgt.msgattr & Q_PRIVATE) {
      if(stricmp(pascal_string(msgt.whofrom),usr.name) &&
         stricmp(pascal_string(msgt.whoto),usr.name))
            return (0);
   }

   if (sm == NULL) {
      fpt = fopen(stringa, (flags & APPEND_TEXT) ? "at" : "wt");
      if (fpt == NULL)
         return (0);
   }
   else
      fpt = sm;


   sprintf (buff, "%sMSGTXT.BBS",  fido_msgpath);
   fp = fopen (buff, "rb");

   fpos = 256L * (msgt.startblock + msgt.numblocks - 1);
   fseek (fp, fpos, SEEK_SET);
   fpos += fgetc (fp);

   fseek (fp, 256L * msgt.startblock, SEEK_SET);

   i = 0;

   if (flags & QUOTE_TEXT) {
      add_quote_string (buff, pascal_string (msgt.whofrom));
      i = strlen (buff);
   }
   pp = 0;

   while (ftell (fp) < fpos) {
      if (!pp)
         fgetc (fp);

      c = fgetc (fp);
      pp++;

      if (c == '\0')
         break;

      if (pp == 255)
         pp = 0;

      if ((byte)c == 0x8D || c == 0x0A || c == '\0')
         continue;

      buff[i++] = c;

      if (c == 0x0D) {
         buff[i-1] = '\0';

         if ((p = strchr (buff, 0x01)) != NULL) {
            if (!strncmp(&p[1], "INTL",4) && !shead)
               sscanf (&p[6], "%d:%d/%d %d:%d/%d", &msg_tzone, &i, &i, &msg_fzone, &i, &i);
            if (!strncmp(&p[1],"TOPT",4) && !shead)
               sscanf (&p[6], "%d",&msg_tpoint);
            if (!strncmp (&p[1], "FMPT",4) && !shead)
               sscanf (&p[6], "%d",&msg_fpoint);
         }
         else if (!shead) {
            if (flags & INCLUDE_HEADER)
               quick_text_header (&msgt,msgn,fpt);
            else if (flags & QWK_TEXTFILE)
               quick_qwk_text_header (&msgt,msgn,fpt,&QWK,&qpos);
            else if (flags & QUOTE_TEXT) {
               strcpy (qfrom, pascal_string (msgt.whofrom));
               strcpy (qto, pascal_string (msgt.whoto));
               strcpy (qdate, pascal_string (msgt.date));
               strcpy (qtime, pascal_string (msgt.time));
               add_quote_header (fpt, qfrom, qto, qdate, qtime);
            }
            shead = 1;
         }

         if (strchr (buff, 0x01) != NULL || stristr (buff, "SEEN-BY") != NULL) {
            if (flags & QUOTE_TEXT) {
               add_quote_string (buff, pascal_string (msgt.whofrom));
               i = strlen (buff);
            }
            else
               i = 0;
            continue;
         }

         if (flags & QWK_TEXTFILE) {
            write_qwk_string (buff, qwkbuffer, &pos, &blks, fpt);
            write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpt);
         }
         else
            fprintf (fpt, "%s\n", buff);

         if (flags & QUOTE_TEXT) {
            add_quote_string (buff, pascal_string (msgt.whofrom));
            i = strlen (buff);
         }
         else
            i = 0;
      }
      else {
         if (i < (usr.width-2))
            continue;

         buff[i] = '\0';
         while (i > 0 && buff[i] != ' ')
            i--;

         m = 0;

         if (i != 0)
            for (z = i + 1; buff[z]; z++)
               wrp[m++] = buff[z];

         buff[i] = '\0';
         wrp[m] = '\0';

         if (!shead) {
            if (flags & INCLUDE_HEADER) {
               quick_text_header (&msgt,msgn,fpt);
            }
            else if (flags & QWK_TEXTFILE)
               quick_qwk_text_header (&msgt,msgn,fpt,&QWK,&qpos);
            else if (flags & QUOTE_TEXT) {
               strcpy (qfrom, pascal_string (msgt.whofrom));
               strcpy (qto, pascal_string (msgt.whoto));
               strcpy (qdate, pascal_string (msgt.date));
               strcpy (qtime, pascal_string (msgt.time));
               add_quote_header (fpt, qfrom, qto, qdate, qtime);
            }
            shead = 1;
         }

         if (flags & QWK_TEXTFILE) {
            write_qwk_string (buff, qwkbuffer, &pos, &blks, fpt);
            write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpt);
         }
         else
            fprintf (fpt, "%s\n", buff);

         if (flags & QUOTE_TEXT)
            add_quote_string (buff, pascal_string (msgt.whofrom));
         else
            buff[0] = '\0';
         strcat (buff, wrp);
         i = strlen (buff);
      }
   }

   fclose (fp);

   if (flags & QWK_TEXTFILE) {
      fwrite (qwkbuffer, 128, 1, fpt);
      blks++;

      fseek (fpt, qpos, SEEK_SET);         /* Restore back to header start */
      sprintf (buff, "%d", blks);
      ljstring (QWK.Msgrecs, buff, 6);
      fwrite ((char *)&QWK, 128, 1, fpt);  /* Write out the header */
      fseek (fpt, 0L, SEEK_END);           /* Bump back to end of file */

      if (sm == NULL)
         fclose (fpt);
      return (blks);
   }
   else
      fprintf (fpt, bbstxt[B_TWO_CR]);

   if (sm == NULL)
      fclose (fpt);

   return (1);
}

void quick_qwk_text_header (msgt,msgn,fpt,QWK,qpos)
struct _msghdr *msgt;
int msgn;
FILE *fpt;
struct QWKmsghd *QWK;
long *qpos;
{
   int dy, mo, yr, hh, mm;
   char stringa[20];
   struct _msg tmsg;

   memset ((char *)&tmsg, 0, sizeof (struct _msg));

   if (msgt->msgattr & Q_PRIVATE)
      tmsg.attr |= MSGPRIVATE;
   if(msgt->msgattr & Q_RECEIVED)
      tmsg.attr |= MSGREAD;
   strcpy (tmsg.to, pascal_string (msgt->whoto));
   strcpy (tmsg.from, pascal_string (msgt->whofrom));
   strcpy (tmsg.subj, pascal_string (msgt->subject));

   strcpy(stringa,pascal_string(msgt->date));
   sscanf(stringa,"%2d-%2d-%2d",&mo,&dy,&yr);
   strcpy(stringa,pascal_string(msgt->time));
   sscanf(stringa,"%2d:%2d",&hh,&mm);
   sprintf(tmsg.date, "%02d %s %02d  %2d:%02d:00", dy, mtext[mo-1], yr, hh, mm);

   qwk_header (&tmsg, QWK, msgn, fpt, qpos);
}

void quick_save_message(txt)
char *txt;
{
   FILE *fp;
   word dest, m, nb, x;
   int fdinfo, i;
   char filename[80], text[258], buffer[258];
   long tempo;
   struct tm *tim;
   struct _msgidx idx;
   struct _msghdr hdr;
   struct _msgtoidx toidx;

   memset ((char *)&idx, 0, sizeof (struct _msgidx));
   memset ((char *)&hdr, 0, sizeof (struct _msghdr));
   memset ((char *)&toidx, 0, sizeof (struct _msgtoidx));

   m_print(bbstxt[B_SAVE_MESSAGE]);
   quick_scan_message_base (sys.quick_board, usr.msg, 1);
   dest = msginfo.highmsg + 1;
   activation_key ();
   m_print(" #%d ...", last_msg + 1);

   sprintf(filename,"%sMSGINFO.BBS",fido_msgpath);
   fdinfo = sh_open (filename, SH_DENYRW, O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
   if (fdinfo == -1)
      return;
   read (fdinfo, (char *)&msginfo, sizeof(struct _msginfo));
   lseek (fdinfo, 0L, SEEK_SET);

   sprintf(filename,"%sMSGIDX.BBS",fido_msgpath);
   i = sh_open (filename, SH_DENYNONE, O_APPEND|O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
   if (i == -1)
      return;
   fp = fdopen (i, "ab");

   idx.msgnum = dest;
   idx.board = sys.quick_board;

   fwrite((char *)&idx,sizeof(struct _msgidx),1,fp);
   fclose(fp);

   sprintf(filename,"%sMSGTOIDX.BBS",fido_msgpath);
   i = sh_open (filename, SH_DENYNONE, O_APPEND|O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
   if (i == -1)
      return;
   fp = fdopen (i, "ab");

   strcpy(&toidx.string[1],msg.to);
   toidx.string[0] = strlen(msg.to);

   fwrite((char *)&toidx,sizeof(struct _msgtoidx),1,fp);
   fclose(fp);

   sprintf(filename,"%sMSGTXT.BBS",fido_msgpath);
   i = sh_open (filename, SH_DENYNONE, O_APPEND|O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
   if (i == -1)
      return;
   fp = fdopen (i, "ab");

   i = (word)(filelength(fileno(fp)) / 256L);
   hdr.startblock = i;

   i = 0;
   m = 1;
   nb = 1;

   memset(text, 0, 256);

   if(sys.netmail) {
      if (msg_tpoint) {
         sprintf(buffer,msgtxt[M_TOPT], msg_tpoint);
         write_pascal_string (buffer, text, &m, &nb, fp);
      }
      if (msg_tzone != msg_fzone) {
         sprintf(buffer,msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig);
         write_pascal_string (buffer, text, &m, &nb, fp);
      }
   }

   if(sys.echomail) {
      sprintf(buffer,msgtxt[M_PID], VERSION, registered ? "+" : NOREG);
      write_pascal_string (buffer, text, &m, &nb, fp);
      sprintf(buffer,msgtxt[M_MSGID], config->alias[sys.use_alias].zone, config->alias[sys.use_alias].net, config->alias[sys.use_alias].node, config->alias[sys.use_alias].point, time(NULL));
      write_pascal_string (buffer, text, &m, &nb, fp);
   }

   if (sys.internet_mail && internet_to != NULL) {
      fprintf (fp, "To: %s\r\n\r\n", internet_to);
      free (internet_to);
   }

   if (txt == NULL) {
      while(messaggio[i] != NULL) {
         write_pascal_string (messaggio[i], text, &m, &nb, fp);

         if(messaggio[i][strlen(messaggio[i])-1] == '\r')
            write_pascal_string ("\n", text, &m, &nb, fp);

         i++;
      }
   }
   else {
      int fptxt;

      fptxt = shopen(txt, O_RDONLY|O_BINARY);
      if (fptxt == -1) {
         fclose(fp);
         unlink(filename);
         close (fdinfo);
         return;
      }

      do {
         memset (buffer, 0, 256);
         i = read(fptxt, buffer, 255);
         for (x = 0; x < i; x++) {
            if (buffer[x] == 0x1A)
               buffer[x] = ' ';
         }
         buffer[i] = '\0';
         write_pascal_string (buffer, text, &m, &nb, fp);
      } while (i == 255);

      close(fptxt);
   }

   write_pascal_string ("\r\n", text, &m, &nb, fp);

   if (strlen(usr.signature) && registered) {
      sprintf(buffer,msgtxt[M_SIGNATURE],usr.signature);
      write_pascal_string (buffer, text, &m, &nb, fp);
   }

   if(sys.echomail) {
      sprintf(buffer,msgtxt[M_TEAR_LINE],VERSION, registered ? "+" : NOREG);
      write_pascal_string (buffer, text, &m, &nb, fp);

      if(strlen(sys.origin))
         sprintf(buffer,msgtxt[M_ORIGIN_LINE],random_origins(),config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
      else
         sprintf(buffer,msgtxt[M_ORIGIN_LINE],system_name,config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
      write_pascal_string (buffer, text, &m, &nb, fp);
   }

   *text = m - 1;
   fwrite(text, 256, 1, fp);

   fclose(fp);

   sprintf(filename,"%sMSGHDR.BBS",fido_msgpath);
   i = sh_open (filename, SH_DENYNONE, O_APPEND|O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
   if (i == -1)
      return;
   fp = fdopen (i, "ab");

   hdr.numblocks = nb;
   hdr.msgnum = dest;
   hdr.prevreply = msg.reply;
   hdr.nextreply = msg.up;
   hdr.timesread = 0;
   hdr.destnet = msg.dest_net;
   hdr.destnode = msg.dest;
   hdr.orignet = msg.orig_net;
   hdr.orignode = msg.orig;
   hdr.destzone = hdr.origzone = config->alias[sys.use_alias].zone;
   hdr.cost = msg.cost;

   if (sys.netmail)
      hdr.msgattr |= Q_NETMAIL;

   if(msg.attr & MSGPRIVATE)
      hdr.msgattr |= Q_PRIVATE;
   if(msg.attr & MSGCRASH)
      hdr.netattr |= Q_CRASH;
   if(msg.attr & MSGFILE)
      hdr.netattr |= Q_FILE;
   if(msg.attr & MSGKILL)
      hdr.netattr |= Q_KILL;
   if(msg.attr & MSGFRQ)
      hdr.netattr |= Q_FRQ;
   if(msg.attr & MSGCPT)
      hdr.netattr |= Q_CPT;
   if(msg.attr & MSGARQ)
      hdr.netattr |= Q_ARQ;

   hdr.msgattr |= Q_LOCAL;
   hdr.board = sys.quick_board;

   tempo = time(NULL);
   tim = localtime(&tempo);

   sprintf(&hdr.time[1],"%02d:%02d",tim->tm_hour,tim->tm_min);
   sprintf(&hdr.date[1],"%02d-%02d-%02d",tim->tm_mon+1,tim->tm_mday,tim->tm_year);
   hdr.time[0] = 5;
   hdr.date[0] = 8;

   strcpy(&hdr.whoto[1],msg.to);
   hdr.whoto[0] = strlen(msg.to);
   strcpy(&hdr.whofrom[1],msg.from);
   hdr.whofrom[0] = strlen(msg.from);
   strcpy(&hdr.subject[1],msg.subj);
   hdr.subject[0] = strlen(msg.subj);

   msginfo.highmsg++;
   msginfo.totalonboard[sys.quick_board-1]++;
   msginfo.totalmsgs++;
   if (!first_msg)
      first_msg = 1;
   last_msg++;
   num_msg++;
   i = (word)(filelength(fileno(fp)) / sizeof (struct _msghdr));
   msgidx[last_msg] = i;

   fwrite((char *)&hdr,sizeof(struct _msghdr),1,fp);
   fclose(fp);

   write(fdinfo, (char *)&msginfo, sizeof(struct _msginfo));
   close (fdinfo);

   m_print(bbstxt[B_XPRT_DONE]);
   status_line(msgtxt[M_INSUFFICIENT_DATA], last_msg, sys.msg_num);
}

