#include <stdio.h>
#include <stdlib.h>
#include <alloc.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#ifdef __OS2__
void VioUpdate (void);
#endif

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

void put_tearline (FILE *fpd);
void m_print2(char *format, ...);

extern char no_external;
char *internet_to = NULL;

static int get_address (char *s, int *zone, int *net, int *node, int *point)
{
   FILE *fp;
   char buffer[90], *p;

   sprintf (buffer, "%sFIDOUSER.LST", config->net_info);
   fp = fopen (buffer, "rt");
   if (fp != NULL) {
      while (fgets (buffer, 85, fp) != NULL) {
         buffer[strlen(buffer) - 1] = '\0';
         if ((p = strchr (buffer, ',')) == NULL)
            continue;
         *p++ = '\0';
         if (!stricmp (buffer, s)) {
            if ((p = strtok (p, " ")) == NULL)
               return (0);
            parse_netnode (p, zone, net, node, point);
            return (1);
         }
      }

      fclose (fp);
   }

   fp = fopen ("NAMES.CFG", "rt");
   if (fp != NULL) {
      while (fgets (buffer, 85, fp) != NULL) {
         buffer[strlen(buffer) - 1] = '\0';
         if ((p = strchr (buffer, ',')) == NULL)
            continue;
         *p++ = '\0';
         if (!stricmp (buffer, s)) {
            if ((p = strtok (p, " ")) == NULL)
               return (0);
            parse_netnode (p, zone, net, node, point);
            return (1);
         }
      }

      fclose (fp);
   }

   return (0);
}

void line_editor(sl)
int sl;
{
   int i;
   char wrp[80];
   long start_write;

   start_write = time(NULL);

   m_print(bbstxt[B_ONE_CR]);
   wrp[0]='\0';

   if (!sl) {
      cls();
      m_print(bbstxt[B_LORE_MSG1]);
      m_print(bbstxt[B_LORE_MSG2]);
   }

   change_attr(YELLOW|_BLACK);
   m_print("\n     [-----------------------------------------------------------------------]\n");

   for(i=sl; i < MAX_MSGLINE;i++) {
      m_print("%3d: ",i+1);

      messaggio[i] = (char *)malloc(80);
      inp_wrap(messaggio[i], wrp, 73);

      if(messaggio[i][0]=='\0') {
         free(messaggio[i]);
         messaggio[i] = NULL;
         break;
      }
   }

   allowed += (int)((time(NULL)-start_write)/60);
}

void inp_wrap(s, wrp, width)
char *s, *wrp;
int width;
{
   unsigned char c;
   int z, i, m;

   strcpy(s,wrp);
   z = strlen(wrp);

   m_print("%s",wrp);

   while(z < width) {
      if (!local_mode) {
         while (PEEKBYTE() == -1) {
            if (!CARRIER)
               return;

            time_release();
            continue;
         }

         c = TIMED_READ(1);
      }
      else {
         while (local_kbd == -1) {
            if (!CARRIER)
               return;

            time_release();
            continue;
         }

         c = (char)local_kbd;
         local_kbd = -1;
      }

      if(c == 0x0D && z == 0) {
         m_print(bbstxt[B_ONE_CR]);
         s[0]='\0';
         wrp[0]='\0';
         return;
      }

      if((c == 0x08 || c == 0x7F) && (z>0)) {
         s[--z]='\0';
         if (!local_mode) {
            SENDBYTE('\b');
            SENDBYTE(' ');
            SENDBYTE('\b');
         }
         if (snooping)
            wputs("\b \b");
      }

      if(c < 0x20 && c != 0x0D)
         continue;

      s[z++]=c;

      if(c == 0x0D) {
         m_print(bbstxt[B_ONE_CR]);
         s[z]='\0';
         wrp[0]='\0';
         return;
      }

      if (!local_mode)
         SENDBYTE(c);
      if (snooping) {
         wputc(c);
#ifdef __OS2__
         VioUpdate ();
#endif
      }
   }

   s[z]='\0';

   while(z > 0 && s[z] != ' ')
      z--;

   m=0;

   if(z != 0) {
      for(i=z+1;i<=width;i++) {
         if (!local_mode)
            SENDBYTE(0x08);
         wrp[m++]=s[i];
         if (snooping)
            wputs("\b");
         s[i]='\0';
      }

      space(width-z);
   }

   wrp[m]='\0';
   m_print(bbstxt[B_ONE_CR]);
}

void save_message(txt)
char *txt;
{
    FILE *fp;
	int i, dest;
	char filename[80];

    m_print(bbstxt[B_SAVE_MESSAGE]);
    scan_message_base(sys.msg_num, 1);
    dest = last_msg + 1;
    activation_key ();
    m_print2 (" #%d ...",dest);

    sprintf(filename,"%s%d.MSG",sys.msg_path,dest);
	fp = fopen(filename,"wb");
	if (fp == NULL)
		return;

    fwrite((char *)&msg,sizeof(struct _msg),1,fp);

    if (sys.netmail || sys.internet_mail) {
       if (config->alias[sys.use_alias].point)
          fprintf(fp,"\001FMPT %d\r\n", config->alias[sys.use_alias].point);
       if (msg_tpoint)
          fprintf(fp,msgtxt[M_TOPT], msg_tpoint);
       if (msg_tzone != config->alias[0].zone || msg_fzone != config->alias[0].zone)
          fprintf(fp,msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig);
    }

    if(sys.echomail) {
       fprintf(fp,msgtxt[M_PID], VERSION, registered ? "+" : NOREG);
       fprintf(fp,msgtxt[M_MSGID], config->alias[sys.use_alias].zone, config->alias[sys.use_alias].net, config->alias[sys.use_alias].node, config->alias[sys.use_alias].point, time(NULL));
    }

    if (sys.internet_mail && internet_to != NULL) {
       fprintf (fp, bbstxt[B_INTERNET_TO], internet_to);
       free (internet_to);
    }

    if (txt == NULL) {
       i = 0;
       while(messaggio[i] != NULL) {
          if(messaggio[i][strlen(messaggio[i])-1] == '\r')
             fprintf(fp,"%s\n",messaggio[i++]);
          else
             fprintf(fp,"%s",messaggio[i++]);
       }
    }
    else {
       int fptxt, m;
       char buffer[2050];

       fptxt = shopen(txt, O_RDONLY|O_BINARY);
       if (fptxt == -1) {
          fclose(fp);
          unlink(filename);
          return;
       }

       do {
          i = read(fptxt, buffer, 2048);
          for (m=0;m<i;m++) {
             if (buffer[m] == 0x1A)
                buffer[m] = ' ';
          }
          fwrite(buffer, 1, i, fp);
       } while (i == 2048);

       close(fptxt);
    }

    fprintf(fp,"\r\n");

    if (strlen(usr.signature) && registered)
       fprintf(fp,msgtxt[M_SIGNATURE],usr.signature);

    if(sys.echomail) {
       put_tearline (fp);
       if(strlen(sys.origin))
          fprintf(fp,msgtxt[M_ORIGIN_LINE],random_origins(),config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
       else
          fprintf(fp,msgtxt[M_ORIGIN_LINE],system_name,config->alias[sys.use_alias].zone,config->alias[sys.use_alias].net,config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
    }

    fputc('\0',fp);
	fclose(fp);

    m_print2 (bbstxt[B_XPRT_DONE]);
    status_line(msgtxt[M_INSUFFICIENT_DATA], dest, sys.msg_num);
    last_msg = dest;
    num_msg++;
}

int get_message_data(reply, s)
int reply;
char *s;
{
   FILE *xferinfo;
   int i, protocol;
   char c, stringa[80], to[80], *p, *v, subj[72];
   long fpos;
   struct _msg msgt;

   cls ();

   to[0] = '\0';
   subj[0] = '\0';

   if (reply > 0) {
      memcpy((char *)&msgt, (char *)&msg, sizeof(struct _msg));
      if (stricmp (msgt.from, usr.name))
         strcpy (to, msgt.from);
   }

   while ((p = strchr(s,'/')) != NULL) {
      if (!strnicmp(p,"/T=\"",4)) {
         if ((v = strchr (&p[4], '"')) != NULL)
            *v = '\0';
         strncpy(to, (char *)&p[4], 35);
         fancy_str(to);
         if (v != NULL)
            *v = '"';
      }

      else if (!strnicmp(p,"/S=\"",4)) {
         if ((v = strchr (&p[4], '"')) != NULL)
            *v = '\0';
         strncpy(subj, (char *)&p[4], 71);
         if (v != NULL)
            *v = '"';
      }

      else if (!strnicmp (p, "/A=", 3)) {
         i = atoi (&p[3]);
         if (!read_system (i, 1)) {
            status_line ("!Access denied to area #%d", i);
            return (0);
         }
      }

      s = p + 1;
   }

   change_attr (WHITE|_BLACK);
   m_print (bbstxt[B_WRITE_MSG]);
   if (sys.private)
      m_print (bbstxt[B_PRIVATE_MSG]);
   else if (sys.public)
      m_print (bbstxt[B_PUBLIC_MSG]);
   m_print (bbstxt[B_INTO_AREA]);
   if (sys.netmail)
      m_print (bbstxt[B_NETMAIL_AREA]);
   else if (sys.echomail)
      m_print (bbstxt[B_ECHOMAIL_AREA]);
   else
      m_print (bbstxt[B_LOCAL_AREA]);
   m_print ("`%s'\n\n", sys.msg_name);

   memset ((char *)&msg, 0, sizeof(struct _msg));
   msg.attr = MSGLOCAL;

   msg.dest_net = config->alias[sys.use_alias].net;
   msg.dest = config->alias[sys.use_alias].node;

   m_print (bbstxt[B_FROM]);
   if (sys.anon_ok) {
      m_print ("%s\n", usr.handle);
      strcpy (msg.from, usr.handle);
   }
   else {
      m_print ("%s\n", usr.name);
      strcpy (msg.from, usr.name);
   }

   m_print (bbstxt[B_TO]);

   if (strlen (to)) {
      strcpy (msg.to, to);
      m_print ("%s\n", msg.to);
   }
   else if (sys.internet_mail) {
      strcpy (msg.to, config->uucp_gatename);
      m_print ("%s\n", msg.to);
   }
   else {
      fancy_input(stringa,35);
      if(!strlen(stringa)) {
         read_system(usr.msg, 1);
         return (0);
      }

      fancy_str(stringa);
      if(!strcmp(stringa,"Sysop") && !sys.netmail) {
         strcpy(msg.to,sysop);
         m_print(bbstxt[B_REDIRECT], msg.to);
      }
      else
         strcpy(msg.to,stringa);
   }

   if(!stricmp (msg.to, usr.name)) {
      read_system(usr.msg, 1);
      return (0);
   }

   if (sys.internet_mail) {
      msg.dest_net = config->uucp_net;
      msg.dest = config->uucp_node;
      msg_tzone = config->uucp_zone;
      msg_tpoint = config->uucp_point;

      if (!get_bbs_record (msg_tzone, msg.dest_net, msg.dest, 0))
         m_print ("%d:%d/%d.%d, %s (%s)\n", msg_tzone, msg.dest_net, msg.dest, msg_tpoint, "Unknown", "Somewhere");
      else
         m_print ("%d:%d/%d.%d, %s (%s)\n", msg_tzone, msg.dest_net, msg.dest, msg_tpoint, nodelist.name, nodelist.city);

      m_print (bbstxt[B_INTERNET]);
      input (stringa, 75);
      if (!strlen (stringa)) {
         read_system (usr.msg, 1);
         return (0);
      }

      internet_to = (char *)malloc (strlen (stringa) + 1);
      strcpy (internet_to, stringa);
   }
   else if (sys.netmail) {
      if (reply > 0) {
         msg.dest_net = msgt.orig_net;
         msg.dest = msgt.orig;
         msg_tzone = msg_fzone;
         msg_tpoint = msg_fpoint;
      }
      else if (get_address (msg.to, &msg_tzone, (int *)&msg.dest_net, (int *)&msg.dest, &msg_tpoint)) {
         m_print(bbstxt[B_ADDRESS_MSG2]);

         if (!get_bbs_record (msg_tzone, msg.dest_net, msg.dest, 0))
            m_print ("%d:%d/%d.%d, %s (%s)\n", msg_tzone, msg.dest_net, msg.dest, msg_tpoint, "Unknown", "Somewhere");
         else
            m_print ("%d:%d/%d.%d, %s (%s)\n", msg_tzone, msg.dest_net, msg.dest, msg_tpoint, nodelist.name, nodelist.city);
      }
      else {
         m_print(bbstxt[B_ADDRESS_MSG1]);
         for (;;) {
            m_print(bbstxt[B_ADDRESS_MSG2]);
            input(stringa, 25);
            if (strlen(stringa) < 1)
               break;

            parse_netnode(stringa, &msg_tzone, (int *)&msg.dest_net, (int *)&msg.dest, &msg_tpoint);

            if (!get_bbs_record (msg_tzone, msg.dest_net, msg.dest, 0))
               m_print (bbstxt[B_NOTFOUND_NODELIST], msg_tzone, msg.dest_net, msg.dest, msg_tpoint);
            else {
               m_print (bbstxt[B_FOUND_NODELIST], msg_tzone, msg.dest_net, msg.dest, msg_tpoint, nodelist.name, nodelist.city);
               m_print(bbstxt[B_PHONE_OK]);
               if (yesno_question (DEF_YES) == DEF_YES)
                  break;
            }
         }

         if (strlen(stringa) < 1) {
            read_system(usr.msg, 1);
            if (internet_to != NULL)
               free (internet_to);
            return (0);
         }
      }
   }

   if((msg.attr & MSGFILE) || (msg.attr & MSGFRQ))
      m_print(bbstxt[B_SUBJFILES]);
   else
      m_print(bbstxt[B_SUBJECT]);

   if (subj[0] == '\0') {
      if (reply > 0) {
         if(strncmp(msgt.subj,"Re: ",4))
            m_print("Re: ");

         m_print("%s\n", msgt.subj);
         m_print(bbstxt[B_KEEP_SAME]);
         m_print(bbstxt[B_SUBJECT]);
      }

      chars_input(stringa,71,INPUT_NOLF);
      if(!strlen(stringa) && !reply) {
         m_print (bbstxt[B_ONE_CR]);
         read_system(usr.msg, 1);
         if (internet_to != NULL)
            free (internet_to);
         return (0);
      }
      else if (!strlen(stringa) && reply > 0) {
         if(strncmp(msgt.subj,"Re: ",4))
            sprintf(stringa,"Re: %s",msgt.subj);
         else
            strcpy(stringa,msgt.subj);

         stringa[71] = '\0';
         m_print("%s\n", stringa);
      }
      else
         m_print (bbstxt[B_ONE_CR]);

      strcpy(msg.subj,stringa);
   }
   else {
      m_print("%s\n", subj);
      strcpy(msg.subj,subj);
   }

   if(!sys.public && !sys.private) {
      do {
         m_print(bbstxt[B_ASK_PRIVATE]);
         c = yesno_question (DEF_NO|QUESTION);
         if(c == QUESTION)
            read_system_file("WHY_PVT");
      } while(c != DEF_NO && c != DEF_YES);

      if(c == DEF_YES)
         msg.attr|=MSGPRIVATE;
   }
   else {
      if(sys.private)
         msg.attr|=MSGPRIVATE;
   }

   data(msg.date);
   msg.up=0;
   msg_fzone=config->alias[sys.use_alias].zone;
   msg.orig=config->alias[sys.use_alias].node;
   msg.orig_net=config->alias[sys.use_alias].net;

   if (sys.netmail || sys.internet_mail) {
      for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++)
         if (config->alias[i].zone == msg_tzone)
            break;
      if (i < MAX_ALIAS && config->alias[i].net) {
         msg_fzone = config->alias[i].zone;
         msg.orig = config->alias[i].node;
         msg.orig_net = config->alias[i].net;
         sys.use_alias = i;
      }
   }

   if (!local_mode) {
      m_print(bbstxt[B_UPLOAD_PREPARED]);
      if (yesno_question (DEF_NO) == DEF_NO)
         return (1);

      no_external = 1;
      protocol = selprot ();
      no_external = 0;

      if (protocol == 0 || !CARRIER)
         return (0);

      sprintf (to, "MSGTMP%d.TMP", line_offset);

      m_print (bbstxt[B_READY_TO_RECEIVE], protocols[protocol - 1]);
      m_print (bbstxt[B_CTRLX_ABORT]);
      timer (50);

      xferinfo = fopen ("XFERINFO", "w+t");

      i = 0;
      v = NULL;

      switch (protocol) {
         case 1:
            who_is_he = 0;
            overwrite = 0;
            v = receive (config->flag_dir, to, 'X');
            if (v != NULL)
               fprintf (xferinfo, "%s%s\n", config->flag_dir, v);
            break;

         case 2:
            who_is_he = 0;
            overwrite = 0;
            v = receive (config->flag_dir, to, 'Y');
            if (v != NULL)
               fprintf(xferinfo, "%s\n", config->flag_dir, v);
            break;

         case 3:
            i = get_Zmodem (config->flag_dir, xferinfo);
            break;

         case 6:
            do {
               who_is_he = 0;
               overwrite = 0;
               v = receive(config->flag_dir, to, 'S');
               if (v != NULL)
                  fprintf (xferinfo, "%s\n", config->flag_dir, v);
            } while (v != NULL);
            break;
      }

      wactiv (mainview);

      CLEAR_INBOUND ();
      if (i || v != NULL) {
         m_print(bbstxt[B_TRANSFER_OK]);
         timer (20);
      }
      else {
         m_print (bbstxt[B_TRANSFER_ABORT]);
         fclose (xferinfo);
         unlink ("XFERINFO");
         press_enter ();
         return (0);
      }

      rewind (xferinfo);
      fpos = filelength(fileno(xferinfo));

      while (ftell (xferinfo) < fpos) {
         fgets (stringa, 78, xferinfo);
         stringa[strlen (stringa) - 1] = '\0';

         if (CARRIER) {
            cls ();

            if (sys.quick_board)
               quick_save_message(stringa);
            else if (sys.pip_board)
               pip_save_message(stringa);
            else if (sys.squish)
               squish_save_message(stringa);
            else
               save_message(stringa);

            usr.msgposted++;
         }

         unlink (stringa);
      }

      fclose (xferinfo);
      unlink ("XFERINFO");
      sprintf (to, "%sMSGTMP%d.TMP", config->flag_dir, line_offset);
      unlink (to);

      return (2);
   }

   return (1);
}

void free_message_array()
{
   int i;

   i = 0;
   while (messaggio[i] != NULL)
      free(messaggio[i++]);

   messaggio[0] = NULL;
}

void edit_change_to()
{
   char stringa[40];

   if (!get_entire_string (stringa, 35)) {
      m_print("\n%s%s\n",bbstxt[B_TO],msg.to);
      fancy_input(stringa,35);
   }

   if(strlen(stringa))
      strcpy(msg.to,fancy_str(stringa));
}

void edit_change_subject()
{
   char stringa[80];

   m_print("\n%s%s\n", bbstxt[B_SUBJECT]);
   input(stringa,71);

   if(strlen(stringa))
      strcpy(msg.subj,stringa);
}

void edit_list()
{
	int i, line;

        cls();

        m_print("%s%s\n",bbstxt[B_FROM],msg.from);
        m_print("%s%s\n",bbstxt[B_TO],msg.to);
        m_print("%s%s\n\n",bbstxt[B_SUBJECT],msg.subj);


	i = 0;
	line = 5;

        while(messaggio[i] != NULL) {
                m_print("%3d: %s\n",i+1,messaggio[i]);
                if(!(++line < (usr.len-1)) && usr.more) {
                        if(!continua())
				break;
			line=1;
		}

		i++;
	}
}

void edit_delete()
{
	int max_line, start, stop, i;
	char stringa[10];

	max_line = 0;

        while(messaggio[max_line] != NULL)
		max_line++;

        if (!get_command_word (stringa, 8))
        {
                m_print(bbstxt[B_STARTING_LINE]);
                input(stringa,8);
                if(!stringa[0])
                        return;
        }

	start = atoi(stringa) - 1;
	if(start > max_line || start < 1)
		return;

        if (!get_command_word (stringa, 8))
        {
                m_print(bbstxt[B_ENDING_LINE]);
                input(stringa,8);
                if(!stringa[0])
                        return;
        }

	stop = atoi(stringa) - 1;
	if(stop < start) {
		i=start;
		start=stop;
		stop=i;
	}
	if(stop >= max_line)
		return;

        i = strlen(messaggio[start]);

        if(messaggio[start][i-1] == 0x0D)
                strcat(messaggio[stop],"\x0D");

	for (i=start;i <= stop; i++) {
                free(messaggio[i]);
                messaggio[i] = NULL;
	}

	while(stop <= max_line) {
                messaggio[start] = messaggio[stop+1];

		start++;
		stop++;
	}

        m_print(bbstxt[B_DELETED_LINES], start - stop + 1);
}

void edit_line()
{
	int start, max_line, i;
	char stringa[80];

	max_line = 0;

        while(messaggio[max_line] != NULL)
		max_line++;

        m_print(bbstxt[B_EDIT_LINE]);
        input(stringa,5);
	if(!stringa[0])
		return;

        start = atoi(stringa);
	if(start < 1 || start > max_line)
		return;

        start--;
        m_print("\n%3d: %s\n",start+1,messaggio[start]);
        m_print("%3d: ",start+1);
        input(stringa,73);

	if (!stringa[0])
		return;

        i = strlen(messaggio[start]);
        if(messaggio[start][i-1] == 0x0D)
		strcat(stringa,"\x0D");
        strcpy(messaggio[start],stringa);
}

void edit_insert()
{
	int i, m, max_line;
	char stringa[10];

	max_line = 0;

        while(messaggio[max_line] != NULL)
		max_line++;

        m_print(bbstxt[B_INSERT_LINE]);
        input(stringa,5);

	if(!stringa[0])
		return;;

        i = atoi(stringa);
        if(i < 1 || i > max_line)
		return;

        i--;
        for(m = max_line; m >= i; m--)
                messaggio[m+1] = messaggio[m];

        i++;
        messaggio[i] = (char *)malloc (80);
        m_print("%3d: ",i+1);
        input(messaggio[i], 73);
        strcat(messaggio[i],"\r");

        m = strlen(messaggio[i-1]);
        if(messaggio[i-1][m-1] != 0x0D)
                strcat(messaggio[i-1],"\x0D");
}

void edit_continue()
{
	int max_line;

	max_line = 0;

        while(messaggio[max_line] != NULL)
		max_line++;

        line_editor(max_line);
}
