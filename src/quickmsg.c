#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <mem.h>
#include <dir.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "quickmsg.h"

#define MAX_MESSAGES    1000

static char *pascal_string (char *);
static int quick_msg_attrib(struct _msghdr *, int, int, int);
static void quick_text_header(struct _msghdr *, int, FILE *);
static int quick_full_read_message (int);
static void write_pascal_string (char *, char *, word *, word *, FILE *);
static void quick_qwk_text_header (struct _msghdr *, int, FILE *, struct QWKmsghd *, long *);

static int far msgidx[1000];
struct _msginfo msginfo;

void quick_scan_message_base(board, area, upd)
int board, area;
char upd;
{
   #define MAXREADIDX   100
   int fd, tnum_msg, i, m, x;
   char filename[80];
   struct _msgidx idx[MAXREADIDX];

   sprintf(filename,"%sMSGINFO.BBS",fido_msgpath);
   fd = shopen(filename,O_RDONLY|O_BINARY);
   read(fd, (char *)&msginfo, sizeof(struct _msginfo));
   close (fd);

   tnum_msg = 0;
   msginfo.totalonboard[board-1] = 0;
   msgidx[0] = 0;
   i = 1;

   sprintf(filename, "%sMSGIDX.BBS",fido_msgpath);
   fd = shopen(filename,O_RDONLY|O_BINARY);

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
        msg_fzone = msg_tzone = alias[sys.use_alias].zone;
        msg_fpoint = msg_tpoint = 0;
        absnum = msgidx[msg_num];

        sprintf(buff, "%sMSGHDR.BBS", fido_msgpath);
        fp = fopen(buff,"r+b");
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

        sprintf(buff,"%sMSGTXT.BBS",fido_msgpath);
	fp = fopen(buff,"rb");

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

        if(line > 1 && !flag && usr.more) {
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
        msg_fzone = msg_tzone = alias[sys.use_alias].zone;
        msg_fpoint = msg_tpoint = 0;
        absnum = msgidx[msg_num];

        sprintf(buff, "%sMSGHDR.BBS", fido_msgpath);
        fp = fopen(buff,"r+b");
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

        sprintf(buff,"%sMSGTXT.BBS",fido_msgpath);
        fp = fopen(buff,"rb");

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

        if(line > 1 && usr.more) {
                press_enter();
                line=1;
        }

        return(1);
}

static int quick_msg_attrib (msgt, s, line, f)
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

static void quick_text_header(msgt, s, fp)
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

static char *pascal_string(s)
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
        long fpos, qpos;
	struct _msghdr msgt;
        struct QWKmsghd QWK;

        absnum = msgidx[msgn];

        sprintf(filename,"%sMSGHDR.BBS", fido_msgpath);
	fp = fopen(filename,"r+b");
	if(fp == NULL)
                return (0);

        fseek(fp,(long)sizeof(struct _msghdr) * absnum,SEEK_SET);
	fread((char *)&msgt,sizeof(struct _msghdr),1,fp);

        fclose(fp);

        blks = 1;
        shead = 0;
        pos = 0;

        if (msgt.msgattr & Q_RECKILL)
                return (0);

        if(msgt.msgattr & Q_PRIVATE)
        {
                if(stricmp(pascal_string(msgt.whofrom),usr.name) &&
                   stricmp(pascal_string(msgt.whoto),usr.name))
                        return (0);
	}

        if (sm == NULL)
        {
           fpt = fopen(stringa, (flags & APPEND_TEXT) ? "at" : "wt");
           if (fpt == NULL)
                return (0);
        }
        else
           fpt = sm;


        sprintf(buff,"%sMSGTXT.BBS",  fido_msgpath);
	fp = fopen(buff,"rb");

	fpos = 256L*(msgt.startblock+msgt.numblocks-1);
	fseek(fp,fpos,SEEK_SET);
	fpos += fgetc(fp);

	fseek(fp,256L*msgt.startblock,SEEK_SET);

	i = 0;

        if (flags & QUOTE_TEXT)
        {
                strcpy(buff, " > ");
                i = strlen(buff);
        }
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
                                if (flags & INCLUDE_HEADER) {
                                        quick_text_header (&msgt,msgn,fpt);
                                }
                                else if (flags & QWK_TEXTFILE)
                                        quick_qwk_text_header (&msgt,msgn,fpt,&QWK,&qpos);
                                shead = 1;
                        }

                        if(buff[0] == 0x01 || !strncmp(buff,"SEEN-BY",7)) {
                                i=0;
                                continue;
                        }

                        if (flags & QWK_TEXTFILE)
                        {
                                write_qwk_string (buff, qwkbuffer, &pos, &blks, fpt);
                                write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpt);
                        }
                        else
                                fprintf(fpt,"%s\n",buff);

                        if (flags & QUOTE_TEXT)
                        {
                                strcpy(buff, " > ");
                                i = strlen(buff);
                        }
                        else
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

                        if (!shead)
                        {
                                if (flags & INCLUDE_HEADER) {
                                        quick_text_header (&msgt,msgn,fpt);
                                }
                                else if (flags & QWK_TEXTFILE)
                                        quick_qwk_text_header (&msgt,msgn,fpt,&QWK,&qpos);
                                shead = 1;
                        }

                        if (flags & QWK_TEXTFILE)
                        {
                                write_qwk_string (buff, qwkbuffer, &pos, &blks, fpt);
                                write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpt);
                        }
                        else
                                fprintf(fpt,"%s\n",buff);

                        if (flags & QUOTE_TEXT)
                                strcpy(buff, " > ");
                        else
                                buff[0] = '\0';
                        strcat(buff,wrp);
                        i = strlen(buff);
		}
	}

        fclose(fp);

        if (flags & QWK_TEXTFILE)
        {
                fwrite(qwkbuffer, 128, 1, fpt);
                blks++;

                fseek(fpt,qpos,SEEK_SET);          /* Restore back to header start */
                sprintf(buff,"%d",blks);
                ljstring(QWK.Msgrecs,buff,6);
                fwrite((char *)&QWK,128,1,fpt);           /* Write out the header */
                fseek(fpt,0L,SEEK_END);               /* Bump back to end of file */

                if (sm == NULL)
                        fclose(fpt);
                return (blks);
        }
        else
                fprintf(fpt,bbstxt[B_TWO_CR]);

        if (sm == NULL)
                fclose(fpt);

        return (1);
}

static void quick_qwk_text_header (msgt,msgn,fpt,QWK,qpos)
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
   word i, dest, m, nb, x;
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

   sprintf(filename,"%sMSGIDX.BBS",fido_msgpath);
   fp = fopen(filename,"ab");

   idx.msgnum = dest;
   idx.board = sys.quick_board;

   fwrite((char *)&idx,sizeof(struct _msgidx),1,fp);
   fclose(fp);

   sprintf(filename,"%sMSGTOIDX.BBS",fido_msgpath);
   fp = fopen(filename,"ab");

   strcpy(&toidx.string[1],msg.to);
   toidx.string[0] = strlen(msg.to);

   fwrite((char *)&toidx,sizeof(struct _msgtoidx),1,fp);
   fclose(fp);

   sprintf(filename,"%sMSGTXT.BBS",fido_msgpath);
   fp = fopen(filename,"ab");

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
      sprintf(buffer,msgtxt[M_PID], VERSION, registered ? "" : NOREG);
      write_pascal_string (buffer, text, &m, &nb, fp);
      sprintf(buffer,msgtxt[M_MSGID], alias[sys.use_alias].zone, alias[sys.use_alias].net, alias[sys.use_alias].node, alias[sys.use_alias].point, time(NULL));
      write_pascal_string (buffer, text, &m, &nb, fp);
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
      sprintf(buffer,msgtxt[M_TEAR_LINE],VERSION, registered ? "" : NOREG);
      write_pascal_string (buffer, text, &m, &nb, fp);

      if(strlen(sys.origin))
         sprintf(buffer,msgtxt[M_ORIGIN_LINE],random_origins(),alias[sys.use_alias].zone,alias[sys.use_alias].net,alias[sys.use_alias].node, alias[sys.use_alias].point);
      else
         sprintf(buffer,msgtxt[M_ORIGIN_LINE],system_name,alias[sys.use_alias].zone,alias[sys.use_alias].net,alias[sys.use_alias].node, alias[sys.use_alias].point);
      write_pascal_string (buffer, text, &m, &nb, fp);
   }

   (unsigned char)text[0] = (unsigned char)m - 1;
   fwrite(text, 256, 1, fp);

   fclose(fp);

   sprintf(filename,"%sMSGHDR.BBS",fido_msgpath);
   fp = fopen(filename,"ab");

   hdr.numblocks = nb;
   hdr.msgnum = dest;
   hdr.prevreply = msg.reply;
   hdr.nextreply = msg.up;
   hdr.timesread = 0;
   hdr.destnet = msg.dest_net;
   hdr.destnode = msg.dest;
   hdr.orignet = msg.orig_net;
   hdr.orignode = msg.orig;
   hdr.destzone = hdr.origzone = alias[sys.use_alias].zone;
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
   i = (word)(filelength(fileno(fp)) / sizeof (struct _msghdr));
   msgidx[last_msg] = i;

   fwrite((char *)&hdr,sizeof(struct _msghdr),1,fp);
   fclose(fp);

   sprintf(filename,"%sMSGINFO.BBS",fido_msgpath);
   fp = fopen(filename,"wb");
   fwrite((char *)&msginfo,sizeof(struct _msginfo),1,fp);
   fclose(fp);

   m_print(bbstxt[B_ONE_CR]);
   status_line(msgtxt[M_INSUFFICIENT_DATA], last_msg);
}

static void write_pascal_string (st, text, pos, nb, fp)
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
         (unsigned char)text[0] = (unsigned char)m - 1;
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
        int fd, i, line=3;
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

		if ((line=more_question(line)) == 0)
			break;
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
        int i, tt, pp, z, m, pos, blks, msgn, fd, pr;
        char c, buff[80], wrp[80], qwkbuffer[130], shead;
        long fpos, qpos;
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

                fpos = 256L*(msgt.startblock+msgt.numblocks-1);
                fseek(fp,fpos,SEEK_SET);
                fpos += fgetc(fp);

                fseek(fp,256L*msgt.startblock,SEEK_SET);

                blks = 1;
                pos = 0;
                memset (qwkbuffer, ' ', 128);
                i = 0;
                pp = 0;
                shead = 0;

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

void quick_save_message2 (txt, f1, f2, f3, f4, f5)
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

   dest = msginfo.highmsg + 1;

   idx.msgnum = dest;
   idx.board = sys.quick_board;
   fwrite((char *)&idx,sizeof(struct _msgidx),1,f1);

   strcpy(&toidx.string[1],msg.to);
   toidx.string[0] = strlen(msg.to);
   fwrite((char *)&toidx,sizeof(struct _msgtoidx),1,f2);

   fflush (f3);
   real_flush (fileno (f3));
   fp = f3;

   i = (word)(filelength(fileno(fp)) / 256L);
   hdr.startblock = i;

   i = 0;
   m = 1;
   nb = 1;

   memset(text, 0, 256);

   do {
      memset (buffer, 0, 256);
      i = fread(buffer, 1, 255, txt);
      for (x = 0; x < i; x++) {
         if (buffer[x] == 0x1A)
            buffer[x] = ' ';
      }
      buffer[i] = '\0';
      write_pascal_string (buffer, text, &m, &nb, fp);
   } while (i == 255);

   (unsigned char)text[0] = (unsigned char)m - 1;
   fwrite(text, 256, 1, fp);

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
   else
      hdr.msgattr |= Q_LOCAL;
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

   msginfo.highmsg++;
   msginfo.totalonboard[sys.quick_board-1]++;
   msginfo.totalmsgs++;

   fwrite((char *)&hdr,sizeof(struct _msghdr),1,f4);

   fseek (f5, 0L, SEEK_SET);
   fwrite((char *)&msginfo,sizeof(struct _msginfo),1,f5);

   last_msg++;
}

int quick_export_mail (maxnodes, forward)
int maxnodes;
struct _fwrd *forward;
{
   FILE *fpd, *f2, *fp;
   int f1, i, pp, z, fd, ne, no, m, n_seen, cnet, cnode, mi;
   char buff[80], wrp[80], c, last_board, found, *p, buffer[2050];
   char *location, *tag, *forw, need_origin;
   long fpos;
   struct _msghdr msghdr;
   struct _msghdr2 mhdr;
   struct _pkthdr2 pkthdr;
   struct _fwrd *seen;
   struct _msginfo msginfo;

   sprintf(buff, "%sMSGINFO.BBS", fido_msgpath);
   f1 = open(buff, O_RDWR|O_BINARY);
   if(f1 == -1)
      return (maxnodes);
   read (f1, (char *)&msginfo, sizeof (struct _msginfo));
   close (f1);

   sprintf(buff, "%sMSGHDR.BBS", fido_msgpath);
   f1 = open(buff, O_RDWR|O_BINARY);
   if(f1 == -1)
      return (maxnodes);

   sprintf(buff,"%sMSGTXT.BBS",fido_msgpath);
   f2 = fopen(buff,"rb");
   if (f2 == NULL) {
      close (f1);
      return (maxnodes);
   }
   setvbuf (f2, NULL, _IOFBF, 2048);

   sprintf (buff, SYSMSG_PATH, sys_path);
   fd = cshopen(buff, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
   if (fd == -1) {
      close (f1);
      fclose (f2);
      return (maxnodes);
   }

   sprintf (buff, "%sAREAS.BBS", sys_path);
   fp = fopen (buff, "rt");

   last_board = 0;
   seen = (struct _fwrd *)malloc (MAX_EXPORT_SEEN * sizeof (struct _fwrd));
   if (seen == NULL) {
      close (f1);
      fclose (f2);
      close (fd);
      if (fp != NULL)
         fclose (fp);
      return (maxnodes);
   }

   fpd = fopen ("MSGTMP.EXP", "rb+");
   if (fpd == NULL) {
      fpd = fopen ("MSGTMP.EXP", "wb");
      fclose (fpd);
      fpd = fopen ("MSGTMP.EXP", "r+b");
   }
   setvbuf (fpd, NULL, _IOFBF, 8192);

   wclreol ();
   while (read(f1, (char *)&msghdr,sizeof(struct _msghdr)) == sizeof (struct _msghdr)) {
      sprintf (wrp, "%d / %d\r", msghdr.msgnum, msginfo.totalmsgs);
      wputs (wrp);

      if (!(msghdr.msgattr & Q_LOCAL))
         continue;

      if (msghdr.board != last_board) {
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
                  strcpy (sys.forward1, forw);
                  sys.quick_board = atoi (location);
                  sys.echomail = 1;
                  found = 1;
                  break;
               }
            }
         }

         if (!found) {
           lseek (fd, 0L, SEEK_SET);
            while (read(fd, (char *)&sys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
               if (sys.quick_board == msghdr.board) {
                  found = 1;
                  break;
               }
            }
         }

         if (!found)
            continue;

         for (i = 0; i < maxnodes; i++)
            forward[i].reset = forward[i].export = 0;

         if (!sys.use_alias) {
            z = alias[sys.use_alias].zone;
            parse_netnode2 (sys.forward1, &z, &ne, &no, &pp);
            if (z != alias[sys.use_alias].zone) {
               for (i = 0; alias[i].net; i++)
                  if (z == alias[i].zone)
                     break;
               if (alias[i].net)
                  sys.use_alias = i;
            }
         }

         z = alias[sys.use_alias].zone;
         ne = alias[sys.use_alias].net;
         no = alias[sys.use_alias].node;
         pp = 0;

         p = strtok (sys.forward1, " ");
         if (p != NULL)
            do {
               parse_netnode2 (p, &z, &ne, &no, &pp);
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
            z = alias[sys.use_alias].zone;
            parse_netnode2 (sys.forward2, &z, &ne, &no, &pp);
            if (z != alias[sys.use_alias].zone) {
               for (i = 0; alias[i].net; i++)
                  if (z == alias[i].zone)
                     break;
               if (alias[i].net)
                  sys.use_alias = i;
            }
         }

         z = alias[sys.use_alias].zone;
         ne = alias[sys.use_alias].net;
         no = alias[sys.use_alias].node;
         pp = 0;

         p = strtok (sys.forward2, " ");
         if (p != NULL)
            do {
               parse_netnode2 (p, &z, &ne, &no, &pp);
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
            z = alias[sys.use_alias].zone;
            parse_netnode2 (sys.forward3, &z, &ne, &no, &pp);
            if (z != alias[sys.use_alias].zone) {
               for (i = 0; alias[i].net; i++)
                  if (z == alias[i].zone)
                     break;
               if (alias[i].net)
                  sys.use_alias = i;
            }
         }

         z = alias[sys.use_alias].zone;
         ne = alias[sys.use_alias].net;
         no = alias[sys.use_alias].node;
         pp = 0;

         p = strtok (sys.forward3, " ");
         if (p != NULL)
            do {
               parse_netnode2 (p, &z, &ne, &no, &pp);
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

      for (i = 0; i < maxnodes; i++)
         if (forward[i].reset)
            forward[i].export = 1;

      need_origin = 1;

      fseek (fpd, 0L, SEEK_SET);
      chsize (fileno (fpd), 0L);
      fprintf (fpd, "AREA:%s\r\n", sys.echotag);

      fpos = 256L*(msghdr.startblock+msghdr.numblocks-1);
      fseek(f2,fpos,SEEK_SET);
      fpos += fgetc(f2);

      fseek(f2,256L*msghdr.startblock,SEEK_SET);

      mi = i = 0;
      pp = 0;
      n_seen = 0;

      while(ftell(f2) < fpos) {
         if (!pp)
            fgetc(f2);

         c = fgetc(f2);
         pp++;

         if (c == '\0')
            break;

         if (pp == 255)
            pp = 0;

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
                  fprintf (fpd, msgtxt[M_TEAR_LINE],VERSION, registered ? "" : NOREG);
                  if (strlen(sys.origin))
                     fprintf(fpd,msgtxt[M_ORIGIN_LINE],random_origins(),alias[sys.use_alias].zone,alias[sys.use_alias].net,alias[sys.use_alias].node, alias[sys.use_alias].point);
                  else
                     fprintf(fpd,msgtxt[M_ORIGIN_LINE],system_name,alias[sys.use_alias].zone,alias[sys.use_alias].net,alias[sys.use_alias].node, alias[sys.use_alias].point);
                  need_origin = 0;
               }

               p = strtok (buff, " ");
               ne = alias[sys.use_alias].net;
               no = alias[sys.use_alias].node;
               pp = alias[sys.use_alias].point;
               z = alias[sys.use_alias].zone;
               while ((p = strtok (NULL, " ")) != NULL) {
                  parse_netnode2 (p, &z, &ne, &no, &pp);
                  seen[n_seen].net = ne;
                  seen[n_seen].node = no;
                  seen[n_seen].point = pp;
                  seen[n_seen].zone = z;
                  for (i = 0; i < maxnodes; i++) {
                     if (forward[i].zone == seen[n_seen].zone && forward[i].net == seen[n_seen].net && forward[i].node == seen[n_seen].node && forward[i].point == seen[n_seen].point) {
                        forward[i].export = 0;
                        break;
                     }
                  }
                  if (seen[n_seen].net != alias[sys.use_alias].fakenet && !seen[n_seen].point)
                     n_seen++;
               }

               for (i = 0; i < maxnodes; i++) {
                  if (!forward[i].export || forward[i].net == alias[sys.use_alias].fakenet || forward[i].point)
                     continue;
                  seen[n_seen].net = forward[i].net;
                  seen[n_seen++].node = forward[i].node;
               }

               qsort (seen, n_seen, sizeof (struct _fwrd), mail_sort_func);

               cnet = cnode = 0;
               strcpy (wrp, "SEEN-BY: ");

               for (i = 0; i < n_seen; i++) {
                  if (cnet != seen[i].net && cnode != seen[i].node) {
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

                  if (strlen (wrp) + strlen (buffer) > 70) {
                     fprintf (fpd, "%s\r\n", wrp);
                     strcpy (wrp, "SEEN-BY: ");
                     sprintf (buffer, "%d/%d ", seen[i].net, seen[i].node);
                  }

                  strcat (wrp, buffer);
               }

               fprintf (fpd, "%s\r\n", wrp);
            }
            else if (!strncmp (buff, "\001PATH: ", 7) && !n_seen) {
               if (need_origin) {
                  fprintf (fpd, msgtxt[M_TEAR_LINE],VERSION, registered ? "" : NOREG);
                  if (strlen(sys.origin))
                     fprintf(fpd,msgtxt[M_ORIGIN_LINE],random_origins(),alias[sys.use_alias].zone,alias[sys.use_alias].net,alias[sys.use_alias].node, alias[sys.use_alias].point);
                  else
                     fprintf(fpd,msgtxt[M_ORIGIN_LINE],system_name,alias[sys.use_alias].zone,alias[sys.use_alias].net,alias[sys.use_alias].node, alias[sys.use_alias].point);
                  need_origin = 0;
               }

               for (i = 0; alias[i].net; i++) {
                  if (alias[i].zone != alias[sys.use_alias].zone)
                     continue;
                  seen[n_seen].net = alias[i].net;
                  seen[n_seen++].node = alias[i].node;
               }

               for (i = 0; i < maxnodes; i++) {
                  if (!forward[i].export || forward[i].net == alias[sys.use_alias].fakenet || forward[i].point)
                     continue;
                  seen[n_seen].net = forward[i].net;
                  seen[n_seen++].node = forward[i].node;
               }

               qsort (seen, n_seen, sizeof (struct _fwrd), mail_sort_func);

               cnet = cnode = 0;
               strcpy (wrp, "SEEN-BY: ");

               for (i = 0; i < n_seen; i++) {
                  if (cnet != seen[i].net && cnode != seen[i].node) {
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

               fprintf (fpd, "%s\r\n", wrp);
               fprintf (fpd, "%s\r\n", buff);
            }
            else
               fprintf (fpd, "%s\r\n", buff);

            mi = 0;
         }
         else {
            if(mi < 78)
               continue;

            if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
               need_origin = 0;

            buff[mi]='\0';
            fprintf(fpd,"%s",buff);
            mi=0;
            buff[mi] = '\0';
         }
      }

      if (!n_seen) {
         if (need_origin) {
            fprintf (fpd, msgtxt[M_TEAR_LINE],VERSION, registered ? "" : NOREG);
            if (strlen(sys.origin))
               fprintf(fpd,msgtxt[M_ORIGIN_LINE],random_origins(),alias[sys.use_alias].zone,alias[sys.use_alias].net,alias[sys.use_alias].node, alias[sys.use_alias].point);
            else
               fprintf(fpd,msgtxt[M_ORIGIN_LINE],system_name,alias[sys.use_alias].zone,alias[sys.use_alias].net,alias[sys.use_alias].node, alias[sys.use_alias].point);
            need_origin = 0;
         }

         if (!alias[sys.use_alias].point) {
            seen[n_seen].net = alias[sys.use_alias].net;
            seen[n_seen++].node = alias[sys.use_alias].node;
         }
         else if (alias[sys.use_alias].fakenet) {
            seen[n_seen].net = alias[sys.use_alias].fakenet;
            seen[n_seen++].node = alias[sys.use_alias].point;
         }

         for (i = 0; i < maxnodes; i++) {
            if (!forward[i].export || forward[i].net == alias[sys.use_alias].fakenet)
               continue;
            seen[n_seen].net = forward[i].net;
            seen[n_seen++].node = forward[i].node;
         }

         qsort (seen, n_seen, sizeof (struct _fwrd), mail_sort_func);

         cnet = cnode = 0;
         strcpy (wrp, "SEEN-BY: ");

         for (i = 0; i < n_seen; i++) {
            if (cnet != seen[i].net && cnode != seen[i].node) {
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

         fprintf (fpd, "%s\r\n", wrp);
         if (alias[sys.use_alias].point && alias[sys.use_alias].fakenet)
            fprintf (fpd, "\001PATH: %d/%d\r\n", alias[sys.use_alias].fakenet, alias[sys.use_alias].point);
         else if (alias[sys.use_alias].point)
            fprintf (fpd, "\001PATH: %d/%d.%d\r\n", alias[sys.use_alias].net, alias[sys.use_alias].node, alias[sys.use_alias].point);
         else
            fprintf (fpd, "\001PATH: %d/%d\r\n", alias[sys.use_alias].net, alias[sys.use_alias].node);
      }

      sprintf (wrp, "%5d %-20.20s ", msghdr.msgnum, sys.echotag);
      wputs (wrp);

      mhdr.ver = PKTVER;
      if (alias[sys.use_alias].point && alias[sys.use_alias].fakenet) {
         mhdr.orig_node = alias[sys.use_alias].point;
         mhdr.orig_net = alias[sys.use_alias].fakenet;
      }
      else {
         mhdr.orig_node = alias[sys.use_alias].node;
         mhdr.orig_net = alias[sys.use_alias].net;
      }
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

         if (forward[i].point)
            sprintf (wrp, "%d/%d.%d ", forward[i].net, forward[i].node, forward[i].point);
         else
            sprintf (wrp, "%d/%d ", forward[i].net, forward[i].node);
         wreadcur (&z, &m);
         if ( (m + strlen (wrp)) > 68)
            wputs ("\n                           ");
         wputs (wrp);

         mhdr.dest_net = forward[i].net;
         mhdr.dest_node = forward[i].node;
         p = HoldAreaNameMunge (forward[i].zone);
         if (forward[i].point)
            sprintf (buff, "%s%04x%04x.PNT\\%08X.OUT", p, forward[i].net, forward[i].node, forward[i].point);
         else
            sprintf (buff, "%s%04x%04x.OUT", p, forward[i].net, forward[i].node);
         mi = open (buff, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
         if (mi == -1 && forward[i].point) {
            sprintf (buff, "%s%04x%04x.PNT", p, forward[i].net, forward[i].node);
            mkdir (buff);
            sprintf (buff, "%s%04x%04x.PNT\\%08X.OUT", p, forward[i].net, forward[i].node, forward[i].point);
            mi = open (buff, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
         }
         if (filelength (mi) > 0L)
            lseek(mi,filelength(mi)-2,SEEK_SET);
         else {
            memset ((char *)&pkthdr, 0, sizeof (struct _pkthdr2));
            pkthdr.ver = PKTVER;
            pkthdr.product = 0x4E;
            pkthdr.serial = 2 * 16 + 10;
            if (alias[sys.use_alias].point && alias[sys.use_alias].fakenet) {
               pkthdr.orig_node = alias[sys.use_alias].point;
               pkthdr.orig_net = alias[sys.use_alias].fakenet;
               pkthdr.orig_point = 0;
            }
            else {
               pkthdr.orig_node = alias[sys.use_alias].node;
               pkthdr.orig_net = alias[sys.use_alias].net;
               pkthdr.orig_point = alias[sys.use_alias].point;
               pkthdr.capability = 1;
               pkthdr.cwvalidation = 256;
            }
            if (forward[i].point) {
               pkthdr.capability = 1;
               pkthdr.cwvalidation = 256;
            }
            pkthdr.orig_zone = alias[sys.use_alias].zone;
            pkthdr.dest_point = forward[i].point;
            pkthdr.dest_node = forward[i].node;
            pkthdr.dest_net = forward[i].net;
            pkthdr.dest_zone = forward[i].zone;
            write (mi, (char *)&pkthdr, sizeof (struct _pkthdr2));
         }

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
         fseek (fpd, 0L, SEEK_SET);
         do {
            z = fread(buffer, 1, 2048, fpd);
            write(mi, buffer, z);
         } while (z == 2048);
         buff[0] = buff[1] = buff[2] = 0;
         write (mi, buff, 3);
         close (mi);
      }

      lseek (f1, -1L * sizeof (struct _msghdr), SEEK_CUR);
      msghdr.msgattr &= ~Q_LOCAL;
      write(f1, (char *)&msghdr, sizeof(struct _msghdr));

      wputs ("\n");
   }

   if (fp != NULL)
      fclose (fp);
   fclose (fpd);
   free (seen);
   close (fd);
   fclose (f2);
   close (f1);
   unlink ("MSGTMP.EXP");

   return (maxnodes);
}

void quick_read_sysinfo (board)
int board;
{
   int fd, tnum_msg;
   char filename[80];

   sprintf(filename,"%sMSGINFO.BBS",fido_msgpath);
   fd = shopen(filename,O_RDONLY|O_BINARY);
   read(fd, (char *)&msginfo, sizeof(struct _msginfo));
   close (fd);

   tnum_msg = 0;
   msginfo.totalonboard[board-1] = 0;
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

