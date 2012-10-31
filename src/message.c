#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <dir.h>
#include <io.h>
#include <fcntl.h>
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

void scan_message_base(area)
int area;
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
      else {
         lastread = 0;
         for (i=1;i<MAXDLREAD;i++) {
            usr.dynlastread[i-1].area = usr.dynlastread[i].area;
            usr.dynlastread[i-1].msg_num = usr.dynlastread[i].msg_num;
         }

         usr.dynlastread[i-1].area = area;
         usr.dynlastread[i-1].msg_num = 0;
      }
   }
}

void msg_kill()
{
   int i, m;
   char stringa[10];

   if (!num_msg) {
      if (sys.quick_board)
         quick_scan_message_base (sys.quick_board, usr.msg);
      else if (sys.pip_board)
         pip_scan_message_base (usr.msg);
      else if (sys.squish)
         squish_scan_message_base (usr.msg, sys.msg_path);
      else
         scan_message_base(usr.msg);
   }

   for (;;) {
      if (!get_command_word (stringa, 4)) {
         m_print ("\nEnter message to delete : ");
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
      quick_scan_message_base (sys.quick_board, usr.msg);
   else if (sys.pip_board)
      pip_scan_message_base (usr.msg);
   else if (sys.squish)
      squish_scan_message_base (usr.msg, sys.msg_path);
   else
      scan_message_base(usr.msg);
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
        struct _msgzone mz;

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
        memcpy ((char *)&mz, (char *)&msgt.date_written, sizeof (struct _msgzone));

        msg_fzone = mz.orig_zone;
        msg_tzone = mz.dest_zone;
        msg_fpoint = mz.orig_point;
        msg_tpoint = mz.dest_point;

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
        struct _msgzone mz;

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
        memcpy ((char *)&mz, (char *)&msgt.date_written, sizeof (struct _msgzone));

        msg_fzone = mz.orig_zone;
        msg_tzone = mz.dest_zone;
        msg_fpoint = mz.orig_point;
        msg_tpoint = mz.dest_point;

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
                m_print("PRIV. ");
	if(msg_ptr->attr & MSGCRASH)
                m_print("C.M. ");
	if(msg_ptr->attr & MSGREAD)
                m_print("LETTO ");
	if(msg_ptr->attr & MSGSENT)
                m_print("INV. ");
	if(msg_ptr->attr & MSGFILE)
                m_print("F/ATT. ");
	if(msg_ptr->attr & MSGFWD)
                m_print("TRANS. ");
	if(msg_ptr->attr & MSGORPHAN)
                m_print("ORFANO ");
	if(msg_ptr->attr & MSGKILL)
                m_print("CANC. ");
	if(msg_ptr->attr & MSGHOLD)
                m_print("TRATT. ");
	if(msg_ptr->attr & MSGFRQ)
                m_print("F/REQ. ");
	if(msg_ptr->attr & MSGRRQ)
                m_print("R/REQ. ");
	if(msg_ptr->attr & MSGCPT)
                m_print("RICEV. ");
	if(msg_ptr->attr & MSGARQ)
                m_print("AUDIT. ");
	if(msg_ptr->attr & MSGURQ)
                m_print("UPDATE ");
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
                scan_message_base(usr.msg);

        if (stringa[0] != '=') {
                m = atoi(stringa);
                if(m < 1 || m > last_msg)
                        return;
        }
        else
                m = last_msg;

        cls();
        m_print("Area %3d ... %s\n",usr.msg,sys.msg_name);

        if (!verbose) {
                m_print("\nMsg# From                 To                   Subject\n");
                m_print("---- -------------------- -------------------- --------------------------------\n");
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
      m_print("\nText to search for: ");

      input(stringa,29);
      if(!strlen(stringa))
         return;
   }

   if (!num_msg) {
      if (sys.quick_board)
         quick_scan_message_base (sys.quick_board, usr.msg);
      else if (sys.pip_board)
         pip_scan_message_base (usr.msg);
      else if (sys.squish)
         squish_scan_message_base (usr.msg, sys.msg_path);
      else
         scan_message_base(usr.msg);
   }

   cls ();
   m_print (bbstxt[B_AREALIST_HEADER], usr.msg, sys.msg_name);
   m_print ("Searching for `%s'.\n\n",strupr(stringa));

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

