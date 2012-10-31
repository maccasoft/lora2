#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <dir.h>
#include <io.h>
#include <fcntl.h>
#include <dos.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"

struct _msgzone {
   int dest_zone;
   int orig_zone;
   int dest_point;
   int orig_point;
};

void read_forward(start,pause)
int start,pause;
{
   int i;

   if (start < 0)
      start = 0;

   if (start < last_msg)
      start++;
   else {
      m_print(bbstxt[B_NO_MORE_MESSAGE]);
      return;
   }

   if (sys.quick_board)
      while (!quick_read_message(start,pause)) {
         if (start < last_msg)
            start++;
         else
            break;
      }
   else if (sys.pip_board)
      while (!pip_msg_read(sys.pip_board, start, 0, pause)) {
         if (start < last_msg)
            start++;
         else
            break;
      }
   else if (sys.squish)
      while (!squish_read_message (start, pause)) {
         if (start < last_msg)
            start++;
         else
            break;
      }
   else
      while (!read_message(start,pause)) {
         if (start < last_msg)
            start++;
         else
            break;
      }

   lastread = start;

   for (i=0;i<MAXLREAD;i++)
      if (usr.lastread[i].area == usr.msg)
         break;
   if (i != MAXLREAD)
      usr.lastread[i].msg_num = start;
   else {
      for (i=0;i<MAXDLREAD;i++)
         if (usr.dynlastread[i].area == usr.msg)
            break;
      if (i != MAXDLREAD)
         usr.dynlastread[i].msg_num = start;
   }
}

void read_nonstop()
{
   int start;

   start = lastread;

   while (start < last_msg && !RECVD_BREAK() && CARRIER)
   {
      if (local_mode && local_kbd == 0x03)
         break;
      read_forward(start,1);
      start = lastread;
      time_release ();
   }
}

void read_backward(start)
int start;
{
   int i;

   if (start > first_msg)
      start--;
   else {
      m_print(bbstxt[B_NO_MORE_MESSAGE]);
      return;
   }

   if (sys.quick_board)
      while (!quick_read_message(start,0)) {
         if (start < last_msg)
            start++;
         else
            break;
      }
   else if (sys.pip_board)
      while (!pip_msg_read(sys.pip_board, start, 0, 0)) {
         if (start < last_msg)
            start++;
         else
            break;
      }
   else if (sys.squish)
      while (!squish_read_message (start, 0)) {
         if (start < last_msg)
            start++;
         else
            break;
      }
   else
      while (!read_message(start,0)) {
         if (start > 1)
            start--;
         else
            break;
      }


   lastread = start;

   for (i=0;i<MAXLREAD;i++)
      if (usr.lastread[i].area == usr.msg)
         break;
   if (i != MAXLREAD)
      usr.lastread[i].msg_num = start;
   else {
      for (i=0;i<MAXDLREAD;i++)
         if (usr.dynlastread[i].area == usr.msg)
            break;
      if (i != MAXDLREAD)
         usr.dynlastread[i].msg_num = start;
   }
}

void read_parent()
{
}

void read_reply()
{
}

void scan_message_base(area, upd)
int area;
char upd;
{
   int i, first, last;
   char filename[80], *p;
   struct ffblk blk;

   num_msg = 0;
   first = 0;
   last = 0;

   sprintf(filename, "%s*.MSG", sys.msg_path);

   if (!findfirst(filename, &blk, 0))
      do {
         if ((p = strchr(blk.ff_name,'.')) != NULL)
            *p = '\0';

         i = atoi(blk.ff_name);
         if (last < i || !last)
            last = i;
         if (first > i || !first)
            first = i;
         num_msg++;
      } while (!findnext(&blk));

   num_msg = num_msg;
   first_msg = first;
   last_msg = last;

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

void msg_kill()
{
   int i, m;
   char stringa[10];

   if (!num_msg) {
      if (sys.quick_board)
         quick_scan_message_base (sys.quick_board, usr.msg, 1);
      else if (sys.pip_board)
         pip_scan_message_base (usr.msg, 1);
      else if (sys.squish)
         squish_scan_message_base (usr.msg, sys.msg_path, 1);
      else
         scan_message_base(usr.msg, 1);
   }

   for (;;) {
      if (!get_command_word (stringa, 4)) {
         m_print (bbstxt[B_MSG_DELETE]);
         chars_input(stringa, 4, INPUT_FIELD);
      }

      if (!stringa[0])
         break;

      i = atoi (stringa);

      if (i >= first_msg && i <= last_msg) {
         if (sys.quick_board)
            m = quick_kill_message (i);
         else if (sys.pip_board)
            m = pip_kill_message (i);
         else if (sys.squish)
            m = squish_kill_message (i);
         else
            m = kill_message (i);

         if (!m)
            m_print (bbstxt[B_CANT_KILL]);
         else {
            m_print (bbstxt[B_MSG_REMOVED], i);
            if (i == last_msg)
               last_msg--;
         }
      }
   }

   if (sys.quick_board)
      quick_scan_message_base (sys.quick_board, usr.msg, 1);
   else if (sys.pip_board)
      pip_scan_message_base (usr.msg, 1);
   else if (sys.squish)
      squish_scan_message_base (usr.msg, sys.msg_path, 1);
   else
      scan_message_base(usr.msg, 1);
}

int kill_message(msg_num)
int msg_num;
{
   int fd;
   char buff[80];
   struct _msg msgt;

   sprintf (buff, "%s%d.MSG", sys.msg_path, msg_num);
   fd = open (buff, O_RDONLY|O_BINARY);
   if (fd == -1)
      return (0);

   read (fd, (char *)&msgt, sizeof(struct _msg));
   close (fd);

   if (!stricmp (msgt.from, usr.name) || !stricmp (msgt.to, usr.name) || usr.priv == SYSOP) {
      unlink (buff);
      return (1);
   }

   return (0);
}


int read_message(msg_num, flag)
int msg_num, flag;
{
	FILE *fp;
        int i, z, m, line, colf=0, shead;
	char c, buff[80], wrp[80], *p;
	long fpos;
	struct _msg msgt;

        if (usr.full_read && !flag)
                return(full_read_message(msg_num));

        line = 1;
        shead = 0;
        msg_fzone = msg_tzone = alias[0].zone;
        msg_fpoint = msg_tpoint = 0;

        sprintf(buff,"%s%d.MSG",sys.msg_path,msg_num);
	fp=fopen(buff,"r+b");
	if(fp == NULL)
		return(0);

        fread((char *)&msgt,sizeof(struct _msg),1,fp);

        if (!stricmp(msgt.from,"ARCmail") && !stricmp(msgt.to,"Sysop"))
        {
                fclose(fp);
                return(0);
        }

	if(msgt.attr & MSGPRIVATE) {
                if(msg_num == 1 && sys.echomail) {
			fclose(fp);
			return(0);
		}

                if(stricmp(msgt.from,usr.name) && stricmp(msgt.to,usr.name) && usr.priv != SYSOP) {
			fclose(fp);
			return(0);
		}
	}

        if (!flag)
                cls();

        memcpy((char *)&msg,(char *)&msgt,sizeof(struct _msg));

	if(flag)
                m_print(bbstxt[B_TWO_CR]);

        change_attr(CYAN|_BLACK);

        allow_reply = 1;
        i=0;
	fpos = filelength(fileno(fp));

	while(ftell(fp) < fpos) {
		c = fgetc(fp);

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
                                line = msg_attrib(&msgt,msg_num,line,0);
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
                                change_attr(LGREY|BLACK);
			}
			else {
				p = &buff[0];

				if(((strchr(buff,'>') - p) < 6) && (strchr(buff,'>'))) {
					if(!colf) {
                                                change_attr(LGREY|BLACK);
						colf=1;
					}
				}
				else {
					if(colf) {
                                                change_attr(CYAN|BLACK);
						colf=0;
					}
				}

                                if (!strncmp(buff,msgtxt[M_TEAR_LINE],4) || !strncmp(buff," * Origin: ",11))
                                        m_print("%s\n",buff);
                                else
                                        m_print("%s\n",buff);

                                if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
                                        gather_origin_netnode (buff);
                        }

			i=0;

			if(flag == 1)
				line=1;

                        if(!(++line < (usr.len-1)) && usr.more)
                        {
                                line = 1;
                                if(!continua())
                                {
					flag=1;
					break;
				}
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
                                for(z=i+1; buff[z]; z++) {
					wrp[m++]=buff[z];
				}

                        buff[i]='\0';
                        wrp[m]='\0';

                        if (!shead) {
                                line = msg_attrib(&msgt,msg_num,line,0);
                                m_print(bbstxt[B_ONE_CR]);
                                shead = 1;
                        }

                        if (((strchr(buff,'>') - buff) < 6) && (strchr(buff,'>'))) {
				if(!colf) {
                                        change_attr(LGREY|_BLACK);
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

			if(flag == 1)
				line=1;

                        if(!(++line < (usr.len-1)) && usr.more)
                        {
                                line = 1;
                                if(!continua())
                                {
					flag=1;
					break;
				}
			}

			strcpy(buff,wrp);
			i=strlen(wrp);
		}
	}

        if(stricmp(msgt.to,usr.name) == 0 && !(msgt.attr & MSGREAD))
        {
                msgt.attr |= MSGREAD;
		rewind(fp);
                fwrite((char *)&msgt,sizeof(struct _msg),1,fp);
	}

	fclose(fp);

        if(line > 1 && !flag && usr.more) {
                press_enter();
		line=1;
	}

	return(1);
}

int full_read_message(msg_num)
int msg_num;
{
        FILE *fp;
        int i, z, m, line, colf=0, shead;
        char c, buff[80], wrp[80], *p;
        long fpos;
        struct _msg msgt;

        line = 2;
        shead = 0;
        msg_fzone = msg_tzone = alias[0].zone;
        msg_fpoint = msg_tpoint = 0;

        sprintf(buff,"%s%d.MSG",sys.msg_path,msg_num);
        fp=fopen(buff,"r+b");
        if(fp == NULL)
                return(0);

        fread((char *)&msgt,sizeof(struct _msg),1,fp);

        if (!stricmp(msgt.from,"ARCmail") && !stricmp(msgt.to,"Sysop"))
        {
                fclose(fp);
                return(0);
        }

        if(msgt.attr & MSGPRIVATE) {
                if(msg_num == 1 && sys.echomail) {
                        fclose(fp);
                        return(0);
                }

                if(stricmp(msgt.from,usr.name) && stricmp(msgt.to,usr.name) && usr.priv != SYSOP) {
                        fclose(fp);
                        return(0);
                }
        }

        cls();

        memcpy((char *)&msg,(char *)&msgt,sizeof(struct _msg));

        change_attr(BLUE|_LGREY);
        del_line();
        m_print(" * %s\n", sys.msg_name);

        allow_reply = 1;
        i=0;
        fpos = filelength(fileno(fp));

        while(ftell(fp) < fpos) {
                c = fgetc(fp);

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
                                line = msg_attrib(&msgt,msg_num,line,0);
                                change_attr(LGREY|_BLUE);
                                del_line();
                                change_attr(CYAN|_BLACK);
                                m_print(bbstxt[B_ONE_CR]);
                                shead = 1;
                        }

                        if(!strncmp(buff,"SEEN-BY",7)) {
                                i=0;
                                continue;
                        }

                        if(buff[0] == 0x01) {
                                change_attr(YELLOW|_BLUE);
                                del_line();
                                m_print("%s\n",&buff[1]);
                                change_attr(CYAN|BLACK);
                        }
                        else {
                                p = &buff[0];

                                if(((strchr(buff,'>') - p) < 6) && (strchr(buff,'>'))) {
                                        if(!colf) {
                                                change_attr(LGREY|BLACK);
                                                colf=1;
                                        }
                                }
                                else {
                                        if(colf) {
                                                change_attr(CYAN|BLACK);
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
                                line = msg_attrib(&msgt,msg_num,line,0);
                                change_attr(LGREY|_BLUE);
                                del_line();
                                change_attr(CYAN|_BLACK);
                                m_print(bbstxt[B_ONE_CR]);
                                shead = 1;
                        }

                        if (((strchr(buff,'>') - buff) < 6) && (strchr(buff,'>'))) {
                                if(!colf) {
                                        change_attr(LGREY|_BLACK);
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

        if(stricmp(msgt.to,usr.name) == 0 && !(msgt.attr & MSGREAD)) {
                msgt.attr |= MSGREAD;
                rewind(fp);
                fwrite((char *)&msgt,sizeof(struct _msg),1,fp);
        }

        fclose(fp);

        if(line > 1 && usr.more) {
                press_enter();
                line=1;
        }

        return(1);
}

int msg_attrib(msg_ptr,s,line,f)
struct _msg *msg_ptr;
int s, line, f;
{
        char stringa[60];

	adjust_date(msg_ptr);

        m_print(bbstxt[B_FROM]);

//        change_attr(LGREEN|_BLACK);
        if(sys.netmail) {
                if (!msg_fzone)
                        msg_fzone = alias[0].zone;
                sprintf(stringa,"%-25.25s (%d:%d/%d.%d)",msg_ptr->from,msg_fzone,msg_ptr->orig_net,msg_ptr->orig,msg_fpoint);
                m_print("%36s   ",stringa);
	}
	else
                m_print("%-36s   ",msg_ptr->from);

        change_attr(WHITE|_BLACK);
        if(msg_ptr->attr & MSGPRIVATE)
                m_print(bbstxt[B_MSGPRIVATE]);
        if (sys.netmail) {
                if(msg_ptr->attr & MSGCRASH)
                        m_print(bbstxt[B_MSGCRASH]);
                if(msg_ptr->attr & MSGREAD)
                        m_print(bbstxt[B_MSGREAD]);
                if(msg_ptr->attr & MSGSENT)
                        m_print(bbstxt[B_MSGSENT]);
                if(msg_ptr->attr & MSGFILE)
                        m_print(bbstxt[B_MSGFILE]);
                if(msg_ptr->attr & MSGFWD)
                        m_print(bbstxt[B_MSGFWD]);
                if(msg_ptr->attr & MSGORPHAN)
                        m_print(bbstxt[B_MSGORPHAN]);
                if(msg_ptr->attr & MSGKILL)
                        m_print(bbstxt[B_MSGKILL]);
                if(msg_ptr->attr & MSGHOLD)
                        m_print(bbstxt[B_MSGHOLD]);
                if(msg_ptr->attr & MSGFRQ)
                        m_print(bbstxt[B_MSGFRQ]);
                if(msg_ptr->attr & MSGRRQ)
                        m_print(bbstxt[B_MSGRRQ]);
                if(msg_ptr->attr & MSGCPT)
                        m_print(bbstxt[B_MSGCPT]);
                if(msg_ptr->attr & MSGARQ)
                        m_print(bbstxt[B_MSGARQ]);
                if(msg_ptr->attr & MSGURQ)
                        m_print(bbstxt[B_MSGURQ]);
        }
        m_print(bbstxt[B_ONE_CR]);
        if ((line=more_question(line)) == 0)
		return(0);

	if(!f) {
                m_print(bbstxt[B_TO]);
//                change_attr(LGREEN|_BLACK);
                if(sys.netmail) {
                        if (!msg_tzone)
                                msg_tzone = alias[0].zone;
                        sprintf(stringa,"%-25.25s (%d:%d/%d.%d)",msg_ptr->to,msg_tzone,msg_ptr->dest_net,msg_ptr->dest,msg_tpoint);
                        m_print("%36s   ",stringa);
		}
		else
                        m_print("%-36s   ",msg_ptr->to,msg_ptr->dest_net,msg_ptr->dest);

                change_attr(WHITE|_BLACK);
                if (s)
                   m_print("Msg: #%d, ",s);
                m_print("%s ",show_date(dateformat,stringa,msg_ptr->date_written.date,msg_ptr->date_written.time));
                m_print("%s\n",show_date(timeformat,stringa,msg_ptr->date_written.date,msg_ptr->date_written.time));

                if ((line=more_question(line)) == 0)
			return(0);
	}

	if(msg_ptr->attr & MSGFILE || msg_ptr->attr & MSGFRQ)
                m_print(bbstxt[B_SUBJFILES]);
	else
                m_print(bbstxt[B_SUBJECT]);
//        change_attr(LGREEN|_BLACK);
        m_print("%s\n",msg_ptr->subj);
        if ((line=more_question(line)) == 0)
		return(0);

        change_attr(CYAN|_BLACK);

	return(line);
}

void adjust_date(msg_ptr)
struct _msg *msg_ptr;
{
   int yr, mo, dy, hh, mm, ss;
   char month[4];

   sscanf(msg_ptr->date,"%2d %3s %2d  %2d:%2d:%2d",&dy,month,&yr,&hh,&mm,&ss);
   month[3]='\0';

   for(mo=0;mo<12;mo++)
      if(!stricmp(month,mtext[mo]))
         break;

   if(mo == 12)
      mo=0;

   msg_ptr->date_written.date=(yr-80)*512+(mo+1)*32+dy;
   msg_ptr->date_written.time=hh*2048+mm*32+ss/2;
}

char *show_date(par,buffer,date,time)
char *par, *buffer;
int date, time;
{
	int yr, mo, dy, hh, mm, ss, flag, i;
        char parola[80];

	yr=((date >> 9) & 0x7F)+80;
	mo=((date >> 5) & 0x0F)-1;
	mo=(mo < 0)?0:mo;
	mo=(mo > 11)?11:mo;
	dy=date & 0x1F;
	hh=(time >> 11) & 0x1F;
	mm=(time >> 5) & 0x3F;
	ss=(time & 0x1F)*2;

	buffer[0]='\0';
	flag=0;

	if(par == NULL)
		return(buffer);

	while(*par) {
		if(flag) {
			switch(toupper(*par)) {
			case 'A':
				if(hh < 12)
					strcat(buffer,"am");
				else
				    strcat(buffer,"pm");
				break;
			case 'B':
				if(strlen(buffer))
					sprintf(parola,"%02d",mo);
				else
				    sprintf(parola,"%2d",mo);
				strcat(buffer,parola);
				break;
			case 'C':
				strcat(buffer,mesi[mo]);
				break;
			case 'D':
				if(strlen(buffer))
					sprintf(parola,"%02d",dy);
				else
				    sprintf(parola,"%2d",dy);
				strcat(buffer,parola);
				break;
			case 'E':
				if(strlen(buffer)) {
					if(hh <= 12)
						sprintf(parola,"%02d",hh);
					else
					    sprintf(parola,"%02d",hh-12);
				}
				else {
					if(hh <= 12)
						sprintf(parola,"%2d",hh);
					else
					    sprintf(parola,"%2d",hh-12);
				}
				strcat(buffer,parola);
				break;
			case 'H':
				if(strlen(buffer))
					sprintf(parola,"%02d",hh);
				else
				    sprintf(parola,"%2d",hh);
				strcat(buffer,parola);
				break;
			case 'M':
				if(strlen(buffer))
					sprintf(parola,"%02d",mm);
				else
				    sprintf(parola,"%2d",mm);
				strcat(buffer,parola);
				break;
			case 'S':
				if(strlen(buffer))
					sprintf(parola,"%02d",ss);
				else
				    sprintf(parola,"%2d",ss);
				strcat(buffer,parola);
				break;
			case 'Y':
				if(strlen(buffer))
					sprintf(parola,"%02d",yr);
				else
				    sprintf(parola,"%2d",yr);
				strcat(buffer,parola);
				break;
			default:
				i=strlen(buffer);
				buffer[i]=*par;
				buffer[i+1]='\0';
				break;
			}
			flag=0;
		}
		else {
			if(*par == '%')
				flag=1;
			else {
				i=strlen(buffer);
				buffer[i]=*par;
				buffer[i+1]='\0';
			}
		}
		par++;
	}

        return(buffer);
}

void list_headers (verbose)
{
	int fd, i, m, line=3;
	char stringa[10], filename[80];
	struct _msg msgt;
        struct _msgzone mz;

        if (!get_command_word (stringa, 4))
        {
                m_print(bbstxt[B_START_WITH_MSG]);

                input(stringa,4);
                if(!strlen(stringa))
                        return;
        }

        if (!num_msg)
                scan_message_base(usr.msg, 1);

        if (stringa[0] != '=') {
                m = atoi(stringa);
                if(m < 1 || m > last_msg)
                        return;
        }
        else
                m = last_msg;

        cls();
        m_print(bbstxt[B_LIST_AREAHEADER],usr.msg,sys.msg_name);

        if (!verbose) {
                m_print(bbstxt[B_LIST_HEADER1]);
                m_print(bbstxt[B_LIST_HEADER2]);
                line += 3;
        }

        if (sys.quick_board) {
                quick_list_headers (m, sys.quick_board, verbose);
                return;
        }
        else if (sys.pip_board) {
                pip_list_headers (m, sys.pip_board, verbose);
                return;
        }
        else if (sys.squish) {
                squish_list_headers (m, verbose);
                return;
        }

        for(i = m; i <= last_msg; i++) {
                sprintf(filename,"%s%d.MSG",sys.msg_path,i);

                fd=shopen(filename,O_RDONLY|O_BINARY);
		if(fd == -1)
			continue;
		read(fd,(char *)&msgt,sizeof(struct _msg));
		close(fd);

                if((msgt.attr & MSGPRIVATE) && stricmp(msgt.from,usr.name) && stricmp(msgt.to,usr.name) && usr.priv < SYSOP)
			continue;

                memcpy ((char *)&mz, (char *)&msgt.date_written, sizeof (struct _msgzone));

                msg_fzone = mz.orig_zone;
                msg_tzone = mz.dest_zone;
                msg_fpoint = mz.orig_point;
                msg_tpoint = mz.dest_point;

                if (verbose) {
                        if ((line = msg_attrib(&msgt,i,line,0)) == 0)
                                break;

                        m_print(bbstxt[B_ONE_CR]);
                }
                else
                        m_print ("%-4d %c%-20.20s %c%-20.20s %-32.32s\n",
                                i,
                                stricmp (msgt.from, usr.name) ? 'Š' : 'Ž',
                                msgt.from,
                                stricmp (msgt.to, usr.name) ? 'Š' : 'Œ',
                                msgt.to,
                                msgt.subj);

                if (!(line=more_question(line)) || !CARRIER && !RECVD_BREAK())
			break;

                time_release();
        }

        if (line && CARRIER)
                press_enter();
}

void message_inquire()
{
   int fd, i, line=4;
   char stringa[30], filename[80], *p;
   struct _msg msgt, backup;

   if (!get_command_word (stringa, 4)) {
      m_print(bbstxt[B_MSG_TEXT]);

      input(stringa,29);
      if(!strlen(stringa))
         return;
   }

   if (!num_msg) {
      if (sys.quick_board)
         quick_scan_message_base (sys.quick_board, usr.msg, 1);
      else if (sys.pip_board)
         pip_scan_message_base (usr.msg, 1);
      else if (sys.squish)
         squish_scan_message_base (usr.msg, sys.msg_path, 1);
      else
         scan_message_base(usr.msg, 1);
   }

   m_print (bbstxt[B_MSG_SEARCHING],strupr(stringa));

   if (sys.quick_board) {
      quick_message_inquire (stringa, sys.quick_board);
      return;
   }
   else if (sys.pip_board) {
      return;
   }
   else if (sys.squish) {
      squish_message_inquire (stringa);
      return;
   }

   for(i = first_msg; i <= last_msg; i++) {
      sprintf(filename,"%s%d.MSG",sys.msg_path,i);

      fd=shopen(filename,O_RDONLY|O_BINARY);
      if(fd == -1)
              continue;
      read(fd,(char *)&msgt,sizeof(struct _msg));

      if (msgt.attr & MSGPRIVATE)
         if (stricmp(msgt.from,usr.name) && stricmp(msgt.to,usr.name) && usr.priv < SYSOP) {
            close(fd);
            continue;
         }

      p = (char *)malloc((unsigned int)filelength(fd)-sizeof(struct _msg)+1);
      if (p != NULL) {
         read(fd,p,(unsigned int)filelength(fd)-sizeof(struct _msg));
         p[(unsigned int)filelength(fd)-sizeof(struct _msg)] = '\0';
      }
      else
         p = NULL;

      close(fd);

      memcpy ((char *)&backup, (char *)&msgt, sizeof (struct _msg));
      strupr(msgt.to);
      strupr(msgt.from);
      strupr(msgt.subj);
      strupr(p);

      if ((strstr(msgt.to,stringa) != NULL) || (strstr(msgt.from,stringa) != NULL) ||
          (strstr(msgt.subj,stringa) != NULL) || (strstr(p,stringa) != NULL)) {
         memcpy ((char *)&msgt, (char *)&backup, sizeof (struct _msg));
         if ((line = msg_attrib(&msgt,i,line,0)) == 0)
            break;

         m_print(bbstxt[B_ONE_CR]);

         if (!(line=more_question(line)) || !CARRIER && !RECVD_BREAK())
            break;

         time_release();
      }

      if (p != NULL)
         free(p);
   }

   if (line && CARRIER)
      press_enter();
}

int fido_export_mail (maxnodes, forward)
int maxnodes;
struct _fwrd *forward;
{
   FILE *fpd, *fp;
   int i, pp, z, ne, no, n_seen, cnet, cnode, mi, msgnum, m;
   char buff[80], wrp[80], c, *p, buffer[2050], need_origin;
   long fpos;
   struct _msghdr2 mhdr;
   struct _pkthdr2 pkthdr;
   struct _fwrd *seen;

   seen = (struct _fwrd *)malloc (MAX_EXPORT_SEEN * sizeof (struct _fwrd));
   if (seen == NULL)
      return (maxnodes);

   fpd = fopen ("MSGTMP.EXP", "rb+");
   if (fpd == NULL) {
      fpd = fopen ("MSGTMP.EXP", "wb");
      fclose (fpd);
      fpd = fopen ("MSGTMP.EXP", "r+b");
   }
   setvbuf (fpd, NULL, _IOFBF, 8192);

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

   sprintf (buff, "%s1.MSG", sys.msg_path);
   i = open (buff, O_RDONLY|O_BINARY);
   if (i != -1) {
      read (i, (char *)&msg, sizeof (struct _msg));
      close (i);
      first_msg = msg.reply;
   }
   else
      first_msg = 1;
   if (first_msg > last_msg)
      first_msg = last_msg;

   wclreol ();
   for (msgnum = first_msg + 1; msgnum <= last_msg; msgnum++) {
      sprintf (wrp, "%d / %d\r", msgnum, last_msg);
      wputs (wrp);

      sprintf (buff, "%s%d.MSG", sys.msg_path, msgnum);
      fp = fopen (buff, "r+b");
      if (fp == NULL)
         continue;

      for (i = 0; i < maxnodes; i++)
         if (forward[i].reset)
            forward[i].export = 1;

      fread((char *)&msg,sizeof(struct _msg),1,fp);

      if (sys.echomail && (msg.attr & MSGSENT)) {
         msg.attr &= ~MSGSENT;
         fseek (fp, 0L, SEEK_SET);
         fwrite((char *)&msg,sizeof(struct _msg),1,fp);
         fclose (fp);
         continue;
      }

      need_origin = 1;

      fseek (fpd, 0L, SEEK_SET);
      chsize (fileno (fpd), 0L);
      fprintf (fpd, "AREA:%s\r\n", sys.echotag);

      mi = i = 0;
      pp = 0;
      n_seen = 0;
      fpos = 0L;

      fpos = filelength(fileno(fp));
      setvbuf (fp, NULL, _IOFBF, 2048);

      while (ftell (fp) < fpos) {
         c = fgetc(fp);

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

      fclose (fp);

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

      sprintf (wrp, "%5d %-20.20s ", msgnum, sys.echotag);
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
      mhdr.cost = 0;
      mhdr.attrib = 0;

      if (sys.public)
         msg.attr &= ~MSGPRIVATE;
      else if (sys.private)
         msg.attr |= MSGPRIVATE;
      if (msg.attr & MSGPRIVATE)
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

         write (mi, msg.date, strlen (msg.date) + 1);
         write (mi, msg.to, strlen (msg.to) + 1);
         write (mi, msg.from, strlen (msg.from) + 1);
         write (mi, msg.subj, strlen (msg.subj) + 1);
         fseek (fpd, 0L, SEEK_SET);
         do {
            z = fread(buffer, 1, 2048, fpd);
            write(mi, buffer, z);
         } while (z == 2048);
         buff[0] = buff[1] = buff[2] = 0;
         write (mi, buff, 3);
         close (mi);
      }

      wputs ("\n");
   }

   sprintf (buff, "%s1.MSG", sys.msg_path);
   i = open (buff, O_RDWR|O_BINARY);
   if (i != -1)
      read (i, (char *)&msg, sizeof (struct _msg));
   else {
      i = open (buff, O_CREAT|O_WRONLY|O_BINARY, S_IREAD|S_IWRITE);
      memset ((char *)&msg, 0, sizeof (struct _msg));
      strcpy (msg.from, "LoraBBS");
      strcpy (msg.to, "Nobody");
      strcpy (msg.subj, "High water mark");
      data (msg.date);
      write (i, (char *)&msg, sizeof (struct _msg));
      write (i, "NOECHO\r\n", 9);
   }
   msg.reply = last_msg ? last_msg : 1;
   lseek (i, 0L, SEEK_SET);
   write (i, (char *)&msg, sizeof (struct _msg));
   close (i);

   fclose (fpd);
   free (seen);
   unlink ("MSGTMP.EXP");

   return (maxnodes);
}

void fido_export_netmail ()
{
   FILE *fpd, *fp;
   int i, z, mi, msgnum;
   char buff[80], wrp[80], c, *p, buffer[2050];
   long fpos;
   struct _msghdr2 mhdr;
   struct _pkthdr2 pkthdr;
   struct date datep;
   struct time timep;

   memset ((char *)&sys, 0, sizeof (struct _sys));
   sys.netmail = 1;
   strcpy (sys.msg_path, netmail_dir);
   scan_message_base (0, 0);

   fpd = fopen ("MSGTMP.EXP", "rb+");
   if (fpd == NULL) {
      fpd = fopen ("MSGTMP.EXP", "wb");
      fclose (fpd);
      fpd = fopen ("MSGTMP.EXP", "r+b");
   }
   setvbuf (fpd, NULL, _IOFBF, 8192);

   wclreol ();
   for (msgnum = 1; msgnum <= last_msg; msgnum++) {
      sprintf (wrp, "%d / %d\r", msgnum, last_msg);
      wputs (wrp);

      sprintf (buff, "%s%d.MSG", netmail_dir, msgnum);
      fp = fopen (buff, "r+b");
      if (fp == NULL)
         continue;

      fread((char *)&msg,sizeof(struct _msg),1,fp);

      if (msg.attr & MSGSENT) {
         fclose (fp);
         continue;
      }

      for (i = 0; alias[i].net; i++) {
         if (alias[i].point && alias[i].fakenet) {
            if (alias[i].fakenet == msg.dest_net && alias[i].point == msg.dest)
               break;
         }
         else {
            if (alias[i].net == msg.dest_net && alias[i].node == msg.dest)
               break;
         }
      }
      if (alias[i].net) {
         fclose (fp);
         continue;
      }

      fseek (fpd, 0L, SEEK_SET);
      chsize (fileno (fpd), 0L);

      msg_fzone = msg_tzone = 0;
      msg_fpoint = msg_tpoint = 0;
      mi = i = 0;
      fpos = 0L;

      fpos = filelength(fileno(fp));
      setvbuf (fp, NULL, _IOFBF, 2048);

      while (ftell (fp) < fpos) {
         c = fgetc(fp);

         if (c == '\0')
            break;

         if (c == 0x0A)
            continue;
         if((byte)c == 0x8D)
            c = ' ';

         buff[mi++] = c;

         if(c == 0x0D) {
            buff[mi - 1]='\0';
            if(buff[0] == 0x01) {
               if (!strncmp(&buff[1],"INTL",4))
                  sscanf(&buff[6],"%d:%d/%d %d:%d/%d", &msg_tzone, &msg.dest_net, &msg.dest, &msg_fzone, &msg.orig_net, &msg.orig);
               if (!strncmp(&buff[1],"MSGID",5)) {
                  parse_netnode (&buff[7], &i, &msg.orig_net, &msg.orig, &msg_fpoint);
                  if (msg_fzone == 0)
                     msg_fzone = i;
               }
               if (!strncmp(&buff[1],"REPLY",5)) {
                  parse_netnode (&buff[7], &i, &msg.dest_net, &msg.dest, &msg_tpoint);
                  if (msg_tzone == 0)
                     msg_tzone = i;
               }
               if (!strncmp(&buff[1],"TOPT",4))
                  sscanf(&buff[6],"%d",&msg_tpoint);
               if (!strncmp(&buff[1],"FMPT",4))
                  sscanf(&buff[6],"%d",&msg_fpoint);
            }
            fprintf (fpd, "%s\r\n", buff);
            mi = 0;
         }
         else {
            if(mi < 78)
               continue;

            buff[mi]='\0';
            fprintf(fpd,"%s",buff);
            mi=0;
            buff[mi] = '\0';
         }
      }

      msg.attr |= MSGSENT;
      fseek (fp, 0L, SEEK_SET);
      fwrite((char *)&msg,sizeof(struct _msg),1,fp);

      fclose (fp);

      sprintf (wrp, "%5d %-20.20s ", msgnum, "Netmail");
      wputs (wrp);

      if ((msg.attr & MSGKILL) && !noask.keeptransit) {
         sprintf (buff, "%s%d.MSG", netmail_dir, msgnum);
         unlink (buff);
      }

      if (!msg_fzone)
         msg_fzone = alias[sys.use_alias].zone;
      if (!msg_tzone)
         msg_tzone = msg_fzone;

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

      if (msg_tpoint)
         sprintf (wrp, "%d/%d.%d ", msg.dest_net, msg.dest, msg_tpoint);
      else
         sprintf (wrp, "%d/%d ", msg.dest_net, msg.dest);
      wputs (wrp);

      gettime (&timep);
      getdate (&datep);

      p = HoldAreaNameMunge (msg_tzone);
      if (msg_tpoint && (msg.attr & MSGHOLD))
         sprintf (buff, "%s%04x%04x.PNT\\%08X.%cUT", p, msg.dest_net, msg.dest, msg_tpoint, (msg.attr & MSGCRASH) ? 'C' : ((msg.attr & MSGHOLD) ? 'H' : 'O'));
      else
         sprintf (buff, "%s%04x%04x.%cUT", p, msg.dest_net, msg.dest, (msg.attr & MSGCRASH) ? 'C' : ((msg.attr & MSGHOLD) ? 'H' : 'O'));
      mi = open (buff, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
      if (mi == -1 && msg_tpoint && (msg.attr & MSGHOLD)) {
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
         pkthdr.orig_node = msg.orig;
         pkthdr.orig_net = msg.orig_net;
         pkthdr.orig_zone = msg_fzone;
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
      fseek (fpd, 0L, SEEK_SET);
      do {
         z = fread(buffer, 1, 2048, fpd);
         write(mi, buffer, z);
      } while (z == 2048);
      buff[0] = buff[1] = buff[2] = 0;
      write (mi, buff, 3);
      close (mi);

      if (msg.attr & MSGFILE) {
         p = HoldAreaNameMunge (msg_tzone);
         sprintf (buff, "%s%04x%04x.%cLO", p, msg.dest_net, msg.dest, (msg.attr & MSGCRASH) ? 'C' : ((msg.attr & MSGHOLD) ? 'H' : 'F'));
         fp = fopen (buff, "at");
         fprintf (fp, "%s\n", msg.subj);
         fclose (fp);
      }

      wputs ("\n");
   }

   fclose (fpd);
   unlink ("MSGTMP.EXP");
}

void xport_message ()
{
   char filename[30];
   int msgnum;

   sprintf (filename, "%d", lastread);
   m_print (bbstxt[B_XPRT_NUMBER]);
   chars_input (filename, 5, INPUT_FIELD|INPUT_UPDATE);
   if (!strlen (filename) || !CARRIER)
      return;

   msgnum = atoi (filename);
   if (!msgnum || msgnum < 1 || msgnum > last_msg)
      return;

   strcpy (filename, "LORA.OUT");
   m_print (bbstxt[B_XPRT_DEVICE]);
   chars_input (filename, 25, INPUT_FIELD|INPUT_UPDATE);
   if (!strlen (filename) || !CARRIER)
      return;

   m_print (bbstxt[B_XPRT_WRITING]);

   if (sys.quick_board)
      quick_write_message_text(msgnum, APPEND_TEXT|INCLUDE_HEADER, filename, NULL);
   else if (sys.pip_board)
      pip_write_message_text(msgnum, APPEND_TEXT|INCLUDE_HEADER, filename, NULL);
   else if (sys.squish)
      squish_write_message_text(msgnum, APPEND_TEXT|INCLUDE_HEADER, filename, NULL);
   else
      write_message_text(msgnum, APPEND_TEXT|INCLUDE_HEADER, filename, NULL);

   m_print (bbstxt[B_XPRT_DONE]);
}

