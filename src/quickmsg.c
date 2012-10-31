#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <mem.h>
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

void quick_scan_message_base(board, area)
int board, area;
{
   #define MAXREADIDX   100
   int fd, tnum_msg, i, m, x;
   char filename[80];
   struct _msgidx idx[MAXREADIDX];

   sprintf(filename,"%sMSGINFO.BBS",fido_msgpath);
   fd = open(filename,O_RDONLY|O_BINARY);
   read(fd, (char *)&msginfo, sizeof(struct _msginfo));
   close (fd);

   tnum_msg = 0;
   msginfo.totalonboard[board-1] = 0;
   msgidx[0] = 0;
   i = 1;

   sprintf(filename, "%sMSGIDX.BBS",fido_msgpath);
   fd = open(filename,O_RDONLY|O_BINARY);

   do {
      m = read (fd, (char *)&idx, sizeof(struct _msgidx) * MAXREADIDX);
      m /= sizeof (struct _msgidx);

      for (x = 0; x < m; x++)
      {
         if (idx[x].board == board)
         {
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
   if (tnum_msg)
   {
      first_msg = 1;
      last_msg = tnum_msg;
      num_msg = tnum_msg;
   }
   else
   {
      first_msg = 0;
      last_msg = 0;
      num_msg = 0;
   }

   for (i=0;i<MAXLREAD;i++)
      if (usr.lastread[i].area == area)
         break;

   if (i != MAXLREAD) {
      if (usr.lastread[i].msg_num > last_msg) {
         i = usr.lastread[i].msg_num - last_msg;
         usr.lastread[i].msg_num -= i;
         if (usr.lastread[i].msg_num < 0)
            usr.lastread[i].msg_num = 0;
      }

      lastread = usr.lastread[i].msg_num;
   }
   else
      lastread = 0;
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

                                if (!strncmp(buff,"--- ",4) || !strncmp(buff," * Origin: ",11))
                                        m_print("%s\n",buff);
                                else
                                        m_print("%s\n",buff);

                                if (!strncmp(buff," * Origin: ",11))
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

			if (!strncmp(buff,"--- ",4) || !strncmp(buff,"* Origin: ",10))
                                m_print("%s\n",buff);
			else
                                m_print("%s\n",buff);

                        if (!strncmp(buff," * Origin: ",11))
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

                                if (!strncmp(buff,"--- ",4) || !strncmp(buff," * Origin: ",11))
                                        m_print("%s\n",buff);
                                else
                                        m_print("%s\n",buff);

                                if (!strncmp(buff," * Origin: ",11))
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
                        if(i<(usr.width-1))
                                continue;

                        buff[i]='\0';
                        while(i>0 && buff[i] != ' ')
                                i--;

                        m=0;

                        if(i != 0)
                                for(z=i+1;z<(usr.width-1);z++)
                                {
                                        wrp[m++]=buff[z];
                                        buff[i]='\0';
                                }

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

                        if (!strncmp(buff,"--- ",4) || !strncmp(buff,"* Origin: ",10))
                                m_print("%s\n",buff);
                        else
                                m_print("%s\n",buff);

                        if (!strncmp(buff," * Origin: ",11))
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

static int quick_msg_attrib(msg_ptr,s,line,f)
struct _msghdr *msg_ptr;
int s, line, f;
{
        char stringa[60];

        m_print(bbstxt[B_FROM]);

//        change_attr(LGREEN|_BLACK);
        if(sys.netmail) {
                sprintf(stringa,"%-25.25s (%d:%d/%d.%d)",pascal_string(msg_ptr->whofrom),msg_fzone,msg_ptr->orignet,msg_ptr->orignode,msg_fpoint);
                m_print("%36s   ",stringa);
                msg.orig_net = msg_ptr->orignet;
                msg.orig = msg_ptr->orignode;
        }
	else
                m_print("%-36s   ",pascal_string(msg_ptr->whofrom));

        strcpy(msg.from, pascal_string(msg_ptr->whofrom));

        change_attr(WHITE|_BLACK);
        if(msg_ptr->msgattr & Q_PRIVATE)
                m_print("PRIV. ");
	if(msg_ptr->netattr & Q_CRASH)
                m_print("C.M. ");
	if(msg_ptr->msgattr & Q_RECEIVED)
                m_print("LETTO ");
	if(msg_ptr->netattr & Q_SENT)
                m_print("INV. ");
	if(msg_ptr->netattr & Q_FILE)
                m_print("F/ATT. ");
	if(msg_ptr->netattr & Q_KILL)
                m_print("CANC. ");
	if(msg_ptr->netattr & Q_FRQ)
                m_print("F/REQ. ");
	if(msg_ptr->netattr & Q_CPT)
                m_print("RICEV. ");
	if(msg_ptr->netattr & Q_ARQ)
                m_print("AUDIT. ");

        m_print(bbstxt[B_ONE_CR]);
        if ((line=more_question(line)) == 0)
		return(0);

	if(!f) {
                m_print(bbstxt[B_TO]);
//                change_attr(LGREEN|_BLACK);
                if(sys.netmail) {
                        sprintf(stringa,"%-25.25s (%d:%d/%d.%d)",pascal_string(msg_ptr->whoto),msg_tzone,msg_ptr->destnet,msg_ptr->destnode,msg_tpoint);
                        m_print("%36s   ",stringa);
                        msg.dest_net = msg_ptr->destnet;
                        msg.dest = msg_ptr->destnode;
                }
		else
                        m_print("%-36s   ",pascal_string(msg_ptr->whoto));

                strcpy(msg.to, pascal_string(msg_ptr->whoto));

                change_attr(WHITE|_BLACK);
                m_print("Msg: #%d, ",s);
                m_print("%s ",pascal_string(msg_ptr->date));
                m_print("%s\n",pascal_string(msg_ptr->time));

                if ((line=more_question(line)) == 0)
			return(0);
	}

        if(msg_ptr->netattr & Q_FILE || msg_ptr->netattr & Q_FRQ)
                m_print(bbstxt[B_SUBJFILES]);
        else
                m_print(bbstxt[B_SUBJECT]);
//        change_attr(LGREEN|_BLACK);
        m_print("%s\n",pascal_string(msg_ptr->subject));

        strcpy(msg.subj, pascal_string(msg_ptr->subject));

        if ((line=more_question(line)) == 0)
		return(0);

        change_attr(CYAN|_BLACK);

	return(line);
}

static void quick_text_header(msg_ptr,s,fp)
struct _msghdr *msg_ptr;
int s;
FILE *fp;
{
        char stringa[60];

        fprintf (fp,bbstxt[B_FROM]);

        if(sys.netmail) {
                if (!msg_fzone)
                        msg_fzone = alias[0].zone;
                sprintf(stringa,"%-25.25s (%d:%d/%d.%d)",pascal_string(msg_ptr->whofrom),msg_fzone,msg_ptr->orignet,msg_ptr->orignode,msg_fpoint);
                fprintf (fp,"%36s   ",stringa);
                msg.orig_net = msg_ptr->orignet;
                msg.orig = msg_ptr->orignode;
        }
	else
                fprintf (fp,"%-36s   ",pascal_string(msg_ptr->whofrom));

        if(msg_ptr->msgattr & Q_PRIVATE)
                fprintf (fp,"PRIV. ");
	if(msg_ptr->netattr & Q_CRASH)
                fprintf (fp,"C.M. ");
	if(msg_ptr->msgattr & Q_RECEIVED)
                fprintf (fp,"LETTO ");
	if(msg_ptr->netattr & Q_SENT)
                fprintf (fp,"INV. ");
	if(msg_ptr->netattr & Q_FILE)
                fprintf (fp,"F/ATT. ");
	if(msg_ptr->netattr & Q_KILL)
                fprintf (fp,"CANC. ");
	if(msg_ptr->netattr & Q_FRQ)
                fprintf (fp,"F/REQ. ");
	if(msg_ptr->netattr & Q_CPT)
                fprintf (fp,"RICEV. ");
	if(msg_ptr->netattr & Q_ARQ)
                fprintf (fp,"AUDIT. ");

        fprintf (fp,bbstxt[B_ONE_CR]);

        fprintf (fp,bbstxt[B_TO]);
        if(sys.netmail) {
                if (!msg_tzone)
                        msg_tzone = alias[0].zone;
                sprintf(stringa,"%-25.25s (%d:%d/%d.%d)",pascal_string(msg_ptr->whoto),msg_tzone,msg_ptr->destnet,msg_ptr->destnode,msg_tpoint);
                fprintf (fp,"%36s   ",stringa);
                msg.dest_net = msg_ptr->destnet;
                msg.dest = msg_ptr->destnode;
        }
        else
                fprintf (fp,"%-36s   ",pascal_string(msg_ptr->whoto));

        fprintf (fp,"Msg: #%d, ",s);
        fprintf (fp,"%s ",pascal_string(msg_ptr->date));
        fprintf (fp,"%s\n",pascal_string(msg_ptr->time));

        if(msg_ptr->netattr & Q_FILE || msg_ptr->netattr & Q_FRQ)
                fprintf (fp,bbstxt[B_SUBJFILES]);
        else
                fprintf (fp,bbstxt[B_SUBJECT]);
        fprintf (fp,"%s\n\n",pascal_string(msg_ptr->subject));
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
                                if (flags & INCLUDE_HEADER)
                                        quick_text_header (&msgt,msgn,fpt);
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
			if(i < (usr.width-1))
				continue;

			buff[i]='\0';
			while(i > 0 && buff[i] != ' ')
				i--;

			m = 0;

			if(i != 0)
				for(z=i+1;z<(usr.width-1);z++) {
					wrp[m++]=buff[z];
					buff[i]='\0';
				}

			wrp[m]='\0';

                        if (!shead)
                        {
                                if (flags & INCLUDE_HEADER)
                                        quick_text_header (&msgt,msgn,fpt);
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
			i = strlen(wrp);
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
   quick_scan_message_base (sys.quick_board, usr.msg);
   dest = msginfo.highmsg + 1;
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

   if(sys.netmail)
   {
      if (msg_tpoint)
      {
         sprintf(buffer,msgtxt[M_TOPT], msg_tpoint);
         write_pascal_string (buffer, text, &m, &nb, fp);
      }
      if (msg_tzone != msg_fzone)
      {
         sprintf(buffer,msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig);
         write_pascal_string (buffer, text, &m, &nb, fp);
      }
   }

   if(sys.echomail)
   {
      sprintf(buffer,msgtxt[M_PID], VERSION, registered ? "" : NOREG);
      write_pascal_string (buffer, text, &m, &nb, fp);
      sprintf(buffer,msgtxt[M_MSGID], alias[sys.use_alias].zone, alias[sys.use_alias].net, alias[sys.use_alias].node, time(NULL));
      write_pascal_string (buffer, text, &m, &nb, fp);
   }

   if (txt == NULL)
   {
      while(messaggio[i] != NULL)
      {
         write_pascal_string (messaggio[i], text, &m, &nb, fp);

         if(messaggio[i][strlen(messaggio[i])-1] == '\r')
            write_pascal_string ("\n", text, &m, &nb, fp);

         i++;
      }
   }
   else {
      int fptxt;

      fptxt = open(txt, O_RDONLY|O_BINARY);
      if (fptxt == -1)
      {
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

   if (strlen(usr.signature) && registered)
   {
      sprintf(buffer,msgtxt[M_SIGNATURE],usr.signature);
      write_pascal_string (buffer, text, &m, &nb, fp);
   }

   if(sys.echomail)
   {
      sprintf(buffer,msgtxt[M_TEAR_LINE],VERSION, registered ? "" : NOREG);
      write_pascal_string (buffer, text, &m, &nb, fp);

      if(strlen(sys.origin))
         sprintf(buffer,msgtxt[M_ORIGIN_LINE],sys.origin,alias[sys.use_alias].zone,alias[sys.use_alias].net,alias[sys.use_alias].node);
      else
         sprintf(buffer,msgtxt[M_ORIGIN_LINE],system_name,alias[sys.use_alias].zone,alias[sys.use_alias].net,alias[sys.use_alias].node);
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
   else if (sys.echomail)
      hdr.msgattr |= Q_LOCAL;

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

   if(sys.echomail && sys.echotag[0])
   {
      fp = fopen ("ECHOTOSS.LOG", "at");
      fprintf (fp, "%s\n", sys.echotag);
      fclose (fp);
   }

   sprintf(filename,"%sECHOMAIL.BBS",fido_msgpath);
   i = open (filename, O_WRONLY|O_BINARY|O_CREAT|O_APPEND, S_IREAD|S_IWRITE);
   dest--;
   write (i, (char *)&dest, 2);
   close (i);

   m_print("\n");
   status_line(":Write message #%d", last_msg);
}

static void write_pascal_string (st, text, pos, nb, fp)
char *st, *text;
word *pos, *nb;
FILE *fp;
{
   word v, m, blocks;

   m = *pos;
   blocks = *nb;

   for (v=0; st[v]; v++)
   {
      text[m++] = st[v];
      if (m == 256)
      {
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
	fd = open(filename,O_RDONLY|O_BINARY);
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

                if (verbose)
                {
                        if ((line = quick_msg_attrib(&msgt,i,line,0)) == 0)
                                break;

                        m_print(bbstxt[B_ONE_CR]);
                }
                else
                {
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
	fd = open(filename,O_RDONLY|O_BINARY);
        if (fd == -1)
                return;

        sprintf(filename,"%sMSGTXT.BBS", fido_msgpath);
        fdt = open(filename,O_RDONLY|O_BINARY);
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

                p = (char *)malloc((unsigned int)msgt.numblocks * 256);
		if (p != NULL) {
                        lseek(fdt,256L*msgt.startblock,SEEK_SET);
                        read(fdt,p,(unsigned int)msgt.numblocks * 256);
                        p[(unsigned int)msgt.numblocks * 256] = '\0';
		}
		else
			p = NULL;

                memcpy ((char *)&backup, (char *)&msgt, sizeof (struct _msghdr));
                strupr(msgt.whoto);
                strupr(msgt.whofrom);
                strupr(msgt.subject);
                strupr(p);
                msg_tzone = msgt.destzone;
                msg_fzone = msgt.origzone;

                if ((strstr(msgt.whoto,stringa) != NULL) || (strstr(msgt.whofrom,stringa) != NULL) ||
                    (strstr(msgt.subject,stringa) != NULL) || (strstr(p,stringa) != NULL))
                {
                        memcpy ((char *)&msgt, (char *)&backup, sizeof (struct _msg));
                        if ((line = quick_msg_attrib(&msgt,i,line,0)) == 0)
                                break;

                        m_print(bbstxt[B_ONE_CR]);

                        if (!(line=more_question(line)) || !CARRIER && !RECVD_BREAK())
                                break;

                        time_release();
                }

		if (p != NULL)
                        free(p);
        }

	close (fd);

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
   fd = open(filename,O_RDONLY|O_BINARY);
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

