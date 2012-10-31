
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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <process.h>
#include <io.h>
#include <dir.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef __OS2__
#define INCL_DOS
#include <os2.h>
#endif

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"
#ifndef __OS2__
#include "exec.h"
#endif

extern int errno, freeze;

#ifdef __OS2__
void VioUpdate (void);
extern HFILE hfComHandle;
#endif

void update_status_line (void);
void open_logfile (void);
void general_door (char *, int);
void create_door_sys(char *, int, int);
void create_dorinfo_def(char *,int ,int );

int spawn_program (int swapout, char *outstring)
{
	int retval, x;
	char *p, *spawnarg[30], privstring[140];

#ifdef __OS2__
	UNBUFFER_BYTES ();
	VioUpdate ();

	if ((p = strchr (outstring, ' ')) != NULL)
		*p = '\0';

	if (stristr (outstring, ".BAT") != NULL || stristr (outstring, ".CMD") != NULL) {
		spawnarg[0] = getenv ("COMSPEC");
      spawnarg[1] = "/C";
      if (spawnarg[0] == NULL)
         spawnarg[0] = "CMD.EXE";
      x = 2;
   }
   else
      x = 0;

   if (p != NULL)
      *p = ' ';

	strcpy (privstring, outstring);
	if ((p = strtok (privstring, " ")) != NULL)
      do {
         spawnarg[x++] = p;
      } while ((p = strtok (NULL, " ")) != NULL);
   spawnarg[x] = NULL;

	if (swapout)
      retval = spawnvp (P_WAIT, spawnarg[0], spawnarg);
   else
      retval = spawnvp (P_WAIT, spawnarg[0], spawnarg);

   if (retval == -1 && stristr (spawnarg[0], ".BAT") == NULL && stristr (spawnarg[0], ".CMD") == NULL) {
      strcpy (privstring, outstring);
      spawnarg[0] = getenv ("COMSPEC");
      spawnarg[1] = "/C";
      if (spawnarg[0] == NULL)
         spawnarg[0] = "CMD.EXE";
      x = 2;

      if ((p = strtok (privstring, " ")) != NULL)
         do {
            spawnarg[x++] = p;
         } while ((p = strtok (NULL, " ")) != NULL);
      spawnarg[x] = NULL;

      retval = spawnvp (P_WAIT, spawnarg[0], spawnarg);
   }
#else
   if ((p = strchr (outstring, ' ')) != NULL)
      *p = '\0';

   if (stristr (outstring, ".BAT") != NULL) {
      if (p != NULL)
			*p = ' ';

      if (getenv ("COMSPEC") == NULL)
         strcpy (privstring, "COMMAND.COM");
      else
         strcpy (privstring, getenv ("COMSPEC"));

		strcat (privstring, " /C ");
      strcat (privstring, outstring);
   }
   else {
      if (p != NULL)
         *p = ' ';
      strcpy (privstring, outstring);
   }

   if (swapout) {
      if ((p = strchr (privstring, ' ')) != NULL)
         *p++ = '\0';
      retval = do_exec (privstring, (p == NULL) ? "" : p, USE_ALL|HIDE_FILE, 0xFFFF, NULL);
      if (p != NULL)
         *(--p) = ' ';
   }
   else {
      x = 0;
      if ((p = strtok (privstring, " ")) != NULL)
         do {
            spawnarg[x++] = p;
         } while ((p = strtok (NULL, " ")) != NULL);
      spawnarg[x] = NULL;

      if ((retval = spawnvp (P_WAIT, spawnarg[0], spawnarg)) == -1 && stristr (spawnarg[0], ".BAT") == NULL) {
         if (getenv ("COMSPEC") == NULL)
            strcpy (privstring, "COMMAND.COM");
			else
            strcpy (privstring, getenv ("COMSPEC"));

         strcat (privstring, " /C ");
         strcat (privstring, outstring);

         x = 0;
			if ((p = strtok (privstring, " ")) != NULL)
            do {
               spawnarg[x++] = p;
            } while ((p = strtok (NULL, " ")) != NULL);
         spawnarg[x] = NULL;

         retval = spawnvp (P_WAIT, spawnarg[0], spawnarg);
      }
   }
#endif

   return (retval);
}

void open_outside_door (s)
char *s;
{
   general_door (s, 0);
}

void editor_door (s)
char *s;
{
	general_door (s, 1);
}

void general_door(s, flag)
char *s;
int flag;
{
	int i, m, oldtime, noreread, retval, oldcume, *varr, count,door_type = 0, handle=0;
	char ext[200], buffer[50], swapping, tmp[36], workdir[80], doordir[40];
	char noinfo;
	long t, freeze_time;
	struct _usr tmpusr;

	if (!flag)
		status_line(":External %s", s);

	getcwd (workdir, 79);

	noinfo = 0;
	freeze = 0;
	swapping = 0;
	m = 0;
	noreread = 0;
	set_useron_record(DOOR, 0, 0);
	ext[0] = '\0';
	doordir[0] = 0;

	freeze_time = time(NULL);

	for (i=0;i<strlen(s);i++) {
		if (s[i] != '*') {
			ext[m++] = s[i];
			ext[m] = '\0';
			continue;
		}

		switch (s[++i]) {
			case '#':
				lorainfo.wants_chat = 0;
				if (function_active == 1)
					f1_status();
				break;
			case '!':
				freeze = 1;
				break;
			case '0':
				strcat(ext, sys.filepath);
				m += strlen(sys.filepath);
				break;
			case '1':
				strcat(ext, sys.msg_path);
				m += strlen(sys.msg_path);
				break;
			case '2':
				if (sys.filelist[0]) {
					strcat (ext, sys.filelist);
					m += strlen (sys.filelist);
				}
				else {
					strcat (ext, sys.filepath);
					strcat (ext, "FILES.BBS");
					m += strlen (sys.filepath);
					m += strlen ("FILES.BBS");
				}
				break;
			case 'A':
				count=0;
				do	{
					doordir[count++] = s[++i];
				} while (s[i]!=' ' && s[i]!=0 && s[i]!='*');

				doordir[count-1] = 0;
				if (doordir[0] && doordir[strlen (doordir) - 1] != '\\')
						strcat (doordir, "\\");
				break;
			case 'B':
				sprintf(buffer,"%lu", local_mode ? 0L : rate);
				strcat(ext, buffer);
				m += strlen(buffer);
				break;
			case 'C':
				if (getenv("COMSPEC") != NULL)
					strcpy(buffer,getenv("COMSPEC"));
				strcat(ext, buffer);
				m += strlen(buffer);
				break;
			case 'D':
				switch(s[++i]) {
					case 'D':
						door_type=1; // DOOR.SYS
						if(s[++i]=='h')
							handle=1;
						else {
							handle=0;
							i--;
						}
						break;
					case 'F':
						door_type=2;  //DOORx.SYS
						if(s[++i]=='h')
							handle=1;
						else {
							handle=0;
							i--;
						}
						break;
					case 'I':
						door_type=3; // DORINFO1.DEF
						if(s[++i]=='h')
							handle=1;
						else {
							handle=0;
							i--;
						}
						break;
					case 'J':
						door_type=4;  // DORINFOx.DEF
						if(s[++i]=='h')
							handle=1;
						else {
							handle=0;
							i--;
						}
						break;
				}
			case 'F':
				strcpy(tmp, usr.name);
				get_fancy_string(tmp, buffer);
				strcat(ext, buffer);
				m += strlen(buffer);
				break;
			case 'G':
				if (usr.ansi)
					sprintf(buffer,"%d", 1);
				else if (usr.avatar)
					sprintf(buffer,"%d", 2);
				else
					sprintf(buffer,"%d", 0);
				strcat(ext, buffer);
				m += strlen(buffer);
				break;
			case 'H':
				if (!local_mode)
					MDM_DISABLE();
				break;
			case 'L':
				strcpy(tmp, usr.name);
				get_fancy_string(tmp, buffer);
				get_fancy_string(tmp, buffer);
				strcat(ext, buffer);
				m += strlen(buffer);
				break;
			case 'M':
				swapping = registered ? 1 : 0;
				break;
			case 'N':
				sprintf(buffer,"%d", line_offset);
				strcat(ext, buffer);
				m += strlen(buffer);
				break;
			case 'P':
				sprintf(buffer,"%d", com_port+1);
				strcat(ext, buffer);
				m += strlen(buffer);
				break;
			case 'R':
				sprintf(buffer,"%d", lorainfo.posuser);
				strcat(ext, buffer);
				m += strlen(buffer);
				break;
			case 'S':
				noreread = 1;
				break;
			case 'T':
				sprintf(buffer,"%d", time_remain());
				strcat(ext, buffer);
				m += strlen(buffer);
				break;
			case 'U':
				sprintf (buffer, "%s%08lx", config->boxpath, usr.id);
				if (buffer[1] == ':')
					setdisk (toupper (buffer[0]) - 'A');
				chdir (buffer);
				break;
			case 'W':
				FOSSIL_WATCHDOG (1);
				break;
			case 'X':
				noinfo = 1;
				break;
			default:
				ext[m++] = s[i];
				ext[m] = '\0';
				break;
		}
	}
	if(door_type == 1)
		create_door_sys(doordir,handle,0);
	else
		if(door_type == 2)
			create_door_sys(doordir,handle,1);
		else
			if(door_type == 3)
				create_dorinfo_def(doordir,handle,0);
			else
				if(door_type == 4)
					create_dorinfo_def(doordir,handle,1);

	translate_filenames (ext, ' ', NULL);

	sprintf(buffer, "%sLORAINFO.T%02X", config->sys_path, line_offset);
	memcpy ((char *)&tmpusr, (char *)&usr, sizeof(struct _usr));
	oldcume = tmpusr.time;
	tmpusr.time += (int)((time(&t) - start_time) / 60);
	tmpusr.baud_rate = local_mode ? 0 : (unsigned int)(rate / 100L);
	lorainfo.baud = local_mode ? 0 : (unsigned int)(rate / 100L);
	lorainfo.port = com_port + 1;
	oldtime = lorainfo.time = time_remain ();
	strcpy (lorainfo.log, log_name);
	strcpy (lorainfo.system, system_name);
	strcpy (lorainfo.sysop, sysop);
	lorainfo.task = line_offset;
	lorainfo.max_baud = speed;
	lorainfo.downloadlimit = config->class[usr_class].max_dl;
	lorainfo.max_time = config->class[usr_class].max_time;
	lorainfo.max_call = config->class[usr_class].max_call;
	lorainfo.ratio = config->class[usr_class].ratio;
	lorainfo.min_baud = config->class[usr_class].min_baud;
	lorainfo.min_file_baud = config->class[usr_class].min_file_baud;
	strcpy (lorainfo.from, msg.from);
	strcpy (lorainfo.to, msg.to);
	strcpy (lorainfo.subj, msg.subj);
	strcpy (lorainfo.msgdate, msg.date);
	if (locked && password != NULL && registered) {
		lorainfo.keylock = 1;
		strcpy (lorainfo.password, password);
		strcode (lorainfo.password, "YG&%FYTF%$RTD");
	}
	lorainfo.total_calls = sysinfo.total_calls;

	if (!noinfo) {
		i = cshopen (buffer, O_WRONLY|O_BINARY|O_TRUNC|O_CREAT, S_IREAD|S_IWRITE);
		write (i, (char *)&tmpusr, sizeof (struct _usr));
		write (i, (char *)&lorainfo, sizeof (struct _lorainfo));
		close (i);
	}

	if (!flag)
		read_system_file ("LEAVING");

	if (flag)
		status_line(":External %s", ext);

	varr = ssave ();
	cclrscrn (LGREY|_BLACK);
	showcur ();
	fclose (logf);

	retval = spawn_program (swapping, ext);

	setdisk (workdir[0] - 'A');
	chdir (workdir);

	if (!local_mode) {
#ifndef __OS2__
		com_install (com_port);
#endif
		if (!config->lock_baud)
			com_baud (rate);
		FOSSIL_WATCHDOG (0);
	}

	if (varr != NULL)
		srestore (varr);

	if (freeze)
		allowed += (int)((time(NULL)-freeze_time)/60);

	if (!noreread && !noinfo) {
		sprintf(buffer, "%sLORAINFO.T%02X", config->sys_path, line_offset);
		i = shopen (buffer, O_RDONLY|O_BINARY);
		read (i, (char *)&tmpusr, sizeof(struct _usr));
		read (i, (char *)&lorainfo, sizeof(struct _lorainfo));
		close (i);

		if (lorainfo.time != oldtime) {
			if (time_to_next (1) < lorainfo.time) {
				lorainfo.time = time_to_next (1);
				read_system_file("TIMEWARN");
			}

			i = (int)((time(&t) - start_time) / 60);
			allowed = i + lorainfo.time;

			if (lorainfo.time < oldtime)
				tmpusr.time += oldtime - lorainfo.time;
		}


		tmpusr.time = oldcume;
		strcpy (tmpusr.name, usr.name);
		strcpy (tmpusr.pwd, usr.pwd);
		if (tmpusr.priv > max_readpriv)
			tmpusr.priv = usr.priv;

		memcpy ((char *)&usr, (char *)&tmpusr, sizeof(struct _usr));
	}

	open_logfile ();
	if (!flag)
		status_line(":Returned from door (%d)", retval);

   unlink(buffer);

	if (local_mode || snooping)
		showcur();

	if (function_active == 1)
		f1_status ();
	else if (function_active == 3)
		f3_status ();

	if (!flag)
		read_system_file ("RETURN");
}

void outside_door(s)
char *s;
{
	general_door (s, 0);
}

void translate_filenames(s, mecca_resp, readln)
char *s, mecca_resp, *readln;
{
   int i, m;
	char ext[128], buffer[50], tmp[36];
   long t;

   m = 0;
   ext[0] = '\0';

   for (i=0;i<strlen(s);i++) {
		if (s[i] != '%') {
         ext[m++] = s[i];
         ext[m] = '\0';
         continue;
		}

      switch (s[++i]) {
         case '%':
            strcat (ext, "%");
            m += strlen(buffer);
				break;
         case '!':
            strcat (ext, "\n");
            m += strlen (buffer);
            break;
         case 'A':
            strcpy(tmp, usr.name);
				get_fancy_string(tmp, buffer);
            strcat(ext, strupr(buffer));
            m += strlen(buffer);
            break;
         case 'b':
            sprintf(buffer,"%lu", local_mode ? 0L : rate);
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case 'B':
				strcpy(tmp, usr.name);
            if (get_fancy_string(tmp, buffer) == NULL)
               break;
            if (get_fancy_string(tmp, buffer) == NULL)
               break;
            strcat(ext, strupr(buffer));
            m += strlen(buffer);
				break;
         case 'c':
            strcat(ext, usr.city);
            m += strlen(usr.city);
				break;
			case 'C':
            ext[m++] = mecca_resp;
            ext[m] = '\0';
            break;
         case 'd':
            sprintf(buffer,"%d", usr.msg);
				strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case 'D':
            sprintf(buffer,"%d", usr.files);
            strcat(ext, buffer);
            m += strlen(buffer);
				break;
         case 'E':
            sprintf(buffer,"%d", usr.len);
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case 'f':
            strcpy(tmp, usr.name);
            get_fancy_string(tmp, buffer);
				strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case 'F':
            strcat(ext, sys.filepath);
            m += strlen(sys.filepath);
            break;
			case 'g':
            if (usr.ansi)
               sprintf(buffer,"%d", 1);
            else if (usr.avatar)
					sprintf(buffer,"%d", 2);
				else
               sprintf(buffer,"%d", 0);
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case 'G':
            sprintf(buffer,"%d", config->class[usr_class].max_dl);
				strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case 'h':
            strcat(ext, usr.voicephone);
            m += strlen(usr.voicephone);
            break;
			case 'H':
            sprintf(buffer,"%u", usr.dnldl);
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case 'i':
            sprintf(buffer,"%ld", usr.dnld);
            strcat(ext, buffer);
				m += strlen(buffer);
            break;
         case 'I':
            sprintf(buffer,"%ld", usr.upld);
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
			case 'j':
            sprintf(buffer,"%ld", (int)((time(&t) - start_time)/60));
            strcat(ext, buffer);
            m += strlen(buffer);
				break;
			case 'J':
            if (readln != NULL) {
               strcat(ext, readln);
               m += strlen(buffer);
            }
            break;
         case 'K':
            sprintf(buffer,"%02X", line_offset);
				strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case 'k':
            sprintf(buffer,"%d", line_offset);
            strcat(ext, buffer);
            m += strlen(buffer);
				break;
         case 'l':
            strcpy(tmp, usr.name);
            get_fancy_string(tmp, buffer);
            get_fancy_string(tmp, buffer);
            strcat(ext, buffer);
            m += strlen(buffer);
				break;
         case 'L':
            if (!local_mode)
               sprintf(buffer, "-p%d -b%lu", com_port+1, rate);
            else
               strcpy(buffer,"-k");
            strcat(ext, buffer);
				m += strlen(buffer);
            break;
         case 'M':
            strcat(ext, sys.msg_path);
				m += strlen(sys.msg_path);
				break;
         case 'n':
            strcat (ext, usr.name);
            m += strlen (usr.name);
            break;
         case 'N':
            strcat (ext, system_name);
            m += strlen (system_name);
            break;
			case 'p':
#ifdef __OS2__
            sprintf (buffer, "%ld", local_mode ? 0L : hfComHandle);
#else
            sprintf (buffer, "%d", com_port);
#endif
            strcat (ext, buffer);
				m += strlen (buffer);
            break;
         case 'P':
            sprintf (buffer, "%d", local_mode ? 0 : com_port + 1);
            strcat (ext, buffer);
            m += strlen (buffer);
				break;
         case 'q':
            strcat(ext, sys.msg_path);
            ext[strlen(ext)-1] = '\0';
            m += strlen(sys.msg_path)-1;
            break;
         case 'Q':
				strcat(ext, sys.filepath);
            ext[strlen(ext)-1] = '\0';
            m += strlen(sys.filepath)-1;
            break;
			case 'R':
				strcat (ext, cmd_string);
            m += strlen (cmd_string);
            cmd_string[0] = '\0';
            break;
         case 'r':
            sprintf (buffer, "%s%08lx", config->boxpath, usr.id);
            strcat (ext, buffer);
            m += strlen (buffer);
            break;
         case 's':
				strcpy(tmp, sysop);
            get_fancy_string(tmp, buffer);
            get_fancy_string(tmp, buffer);
            strcat(ext, strupr(buffer));
            m += strlen(buffer);
            break;
         case 'S':
				strcpy(tmp, sysop);
            get_fancy_string(tmp, buffer);
            strcat(ext, strupr(buffer));
            m += strlen(buffer);
            break;
			case 't':
            sprintf(buffer,"%d", time_remain());
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case 'T':
            sprintf(buffer,"%d", time_remain()*60);
				strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case 'u':
				sprintf(buffer,"%u", lorainfo.posuser);
				strcat(ext, buffer);
            m += 1;
            break;
         case 'U':
            ext[m++] = '_';
            ext[m] = '\0';
            break;
         case 'v':
            strcat(ext, sys.uppath);
            m += strlen(sys.filepath);
            break;
			case 'V':
            strcat(ext, sys.uppath);
            ext[strlen(ext)-1] = '\0';
            m += strlen(sys.filepath)-1;
            break;
         case 'w':
            strcat(ext, sys.filelist);
				m += strlen(sys.filelist);
            break;
         case 'W':
            sprintf(buffer,"%u", speed);
				strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case 'x':
            getcwd (buffer, 40);
            sprintf(buffer,"%c", buffer[0]);
            strcat(ext, buffer);
				m += strlen(buffer);
            break;
         case 'X':
            sprintf(buffer,"%d", lastread);
				strcat(ext, buffer);
				m += strlen(buffer);
            break;
         case 'Y':
            sprintf(buffer,"%d", usr.language);
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case 'Z':
            strcat(ext, strupr(usr.name));
            m += strlen(usr.name);
            break;
         default:
				ext[m++] = s[i];
            ext[m] = '\0';
            break;
      }
   }

   strcpy (s, ext);
}

int external_mailer(s)
char *s;
{
   int retval;

   status_line(":Shelling to front-end", s);

   cclrscrn(LGREY|_BLACK);
	showcur();
   fclose (logf);

   retval = spawn_program (1, s);

	com_install (com_port);
   com_baud (rate);
   FOSSIL_WATCHDOG (0);

   open_logfile ();
   status_line(":Returned from front-end shell (%d)", retval);

   return (retval);
}

void external_bbs (char *s)
{
   int i, m, retval, *varr;
	char ext[200], buffer[50], swapping;

   status_line ("+Shelling to BBS");

   swapping = 0;
   m = 0;
   ext[0] = '\0';

   for (i=0;i<strlen(s);i++) {
		if (s[i] != '*') {
         ext[m++] = s[i];
         ext[m] = '\0';
         continue;
      }

      switch (s[++i]) {
			case 'B':
            sprintf (buffer,"%lu", local_mode ? 0L : rate);
            strcat (ext, buffer);
            m += strlen (buffer);
				break;
			case 'C':
            if (getenv("COMSPEC") != NULL)
               strcpy(buffer,getenv("COMSPEC"));
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case 'H':
#ifdef __OS2__
            sprintf (buffer, "%ld", hfComHandle);
            strcat (ext, buffer);
            m += strlen (buffer);
#else
            if (!local_mode)
               MDM_DISABLE();
#endif
            break;
         case 'M':
            swapping = registered ? 1 : 0;
            break;
         case 'N':
            sprintf(buffer,"%d", line_offset);
				strcat(ext, buffer);
				m += strlen(buffer);
            break;
         case 'P':
            sprintf(buffer,"%d", com_port+1);
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
			case 'T':
            sprintf(buffer,"%d", time_to_next (0));
            strcat(ext, buffer);
            m += strlen(buffer);
				break;
			case 'W':
            FOSSIL_WATCHDOG (1);
            break;
         default:
            ext[m++] = s[i];
            ext[m] = '\0';
            break;
      }
   }

   fclose (logf);

   varr = ssave ();
   clown_clear ();
   showcur();

   getcwd (buffer, 49);

   retval = spawn_program (swapping, ext);

   setdisk (buffer[0] - 'A');
   chdir (buffer);
	hidecur ();

   if (varr != NULL)
      srestore (varr);

   open_logfile ();
   status_line ("+BBS Return Code: %d", retval);

   get_down (retval, 2);
}

void update_status_line ()
{
   if (function_active == 1)
      f1_status ();
   else if (function_active == 2)
      f3_status ();
   else if (function_active == 3)
      f3_status ();
   else if (function_active == 4)
      f4_status ();
   else if (function_active == 5);
   else if (function_active == 6);
   else if (function_active == 7);
   else if (function_active == 8);
   else if (function_active == 9)
      f9_status ();
}

#define MAX_LOCKS   200

typedef struct {
   int Zone;
   int Net;
	int Node;
	int Point;
   char had_to_punt;
} ADDR;

int flag_file (int function, int zone, int net, int node, int point, int do_stat)
{
	static ADDR *last_set = NULL;
   static int numlocks = 0;
   FILE *fptr;
   int i;
	char *HoldName, flagname[128], tmpname[128], BSYname[15];

   if (!line_offset || !config->multitask)
      return (FALSE);

   if (last_set == NULL)
      last_set = (ADDR *)malloc (sizeof (ADDR) * MAX_LOCKS);

   HoldName = HoldAreaNameMunge (zone);

   switch (function) {
      case INITIALIZE:
         numlocks = 0;
         last_set[numlocks].Zone = -1;
         last_set[numlocks].had_to_punt = 0;
         break;

      case CLEAR_SESSION_FLAG:
			// Per prima cosa libera tutti i flag creati, in modo da assicurarsi
         // di non lasciare roba bloccata in giro.
         if (numlocks != 0) {
            for (i = 0; i < numlocks; i++) {
               HoldName = HoldAreaNameMunge (last_set[i].Zone);
					if (last_set[i].Point != 0) {
                  sprintf (flagname, "%s%04x%04x.PNT\\", HoldName, last_set[i].Net, last_set[i].Node);
						sprintf (BSYname, "%08x.BSY", last_set[i].Point);
               }
               else {
                  strcpy (flagname, HoldName);
                  sprintf (BSYname, "%04x%04x.BSY", last_set[i].Net, last_set[i].Node);
					}

               if (last_set[i].had_to_punt)
                  strcpy (flagname, config->flag_dir);
					strcat (flagname, BSYname);

               last_set[i].had_to_punt = 0;
               last_set[i].Zone = -1;

               if (!unlink (flagname)) {
                  if (do_stat && config->doflagfile)
                     status_line (msgtxt[M_CLEARED_FLAGFILE], flagname);
                }
            }

            numlocks = 0;
         }

         // Cancella il flag di sessione relativo a questo task.
         if (config->flag_dir[0]) {
            sprintf (flagname, "%sTask.%02x", config->flag_dir, line_offset);
            unlink (flagname);
         }
			return (FALSE);

      case SET_SESSION_FLAG:
         if (config->flag_dir[0]) {
				sprintf (flagname, "%sTask.%02x", config->flag_dir, line_offset);
            fptr = fopen (flagname, "wb");
            fclose (fptr);
			}
         return (FALSE);

      case TEST_AND_SET:
			for (i = 0; i < numlocks; i++) {
            if (last_set[i].Zone == zone && last_set[i].Net == net && last_set[i].Node == node && last_set[i].Point == point)
               return (FALSE);
         }

			if (point != 0) {
            sprintf (flagname, "%s%04x%04x.PNT\\", HoldName, net, node);
            sprintf (BSYname, "%08x.BSY", point);
         }
         else {
            strcpy (flagname, HoldName);
            sprintf (BSYname, "%04x%04x.BSY", net, node);
         }

         last_set[numlocks].had_to_punt = 0;
         sprintf (tmpname, "%sLORABSY.%02x", flagname, line_offset);
         fptr = fopen (tmpname, "wb");

         if ((fptr == NULL) && (config->flag_dir[0])) {
            last_set[numlocks].had_to_punt = 1;
            strcpy (flagname, config->flag_dir);
            sprintf (tmpname, "%sLORABSY.%02x", flagname, line_offset);
            fptr = fopen (tmpname, "wb");
         }         

         if (fptr == NULL) {
            if (do_stat && config->doflagfile)
					status_line (msgtxt[M_FAILED_CREATE_FLAG], tmpname);

            last_set[numlocks].Zone = -1;
            return (TRUE);
			}

         fclose (fptr);

         strcat (flagname, BSYname);

         if (!rename (tmpname, flagname)) {
				if (do_stat && config->doflagfile)
					status_line (msgtxt[M_CREATED_FLAGFILE], flagname);

            last_set[numlocks].Zone = zone;
            last_set[numlocks].Net = net;
            last_set[numlocks].Node = node;
            last_set[numlocks].Point = point;
            numlocks++;

            return (FALSE);
         }

         if (do_stat && config->doflagfile)
            status_line (msgtxt[M_THIS_ADDRESS_LOCKED], zone, net, node, point);

         unlink (tmpname);
         last_set[numlocks].Zone = -1;

         return (TRUE);

      case CLEAR_FLAG:
			if (numlocks == 0)
				return (TRUE);

			for (i = 0; i < numlocks; i++) {
				if (last_set[i].Zone == zone && last_set[i].Net == net && last_set[i].Node == node && last_set[i].Point == point)
					break;
			}
			if (i == numlocks) {
				if (do_stat && config->doflagfile)
					status_line (msgtxt[M_BAD_CLEAR_FLAGFILE], zone, net, node, point);
				return (TRUE);
			}

			if (point != 0) {
				sprintf (flagname, "%s%04x%04x.PNT\\", HoldName, net, node);
				sprintf (BSYname, "%08x.BSY", point);
			}
			else {
				strcpy (flagname, HoldName);
				sprintf (BSYname, "%04x%04x.BSY", net, node);
			}

			if (last_set[i].had_to_punt)
				strcpy (flagname, config->flag_dir);
			strcat (flagname, BSYname);

			last_set[i].had_to_punt = 0;
			last_set[i].Zone = -1;

			if (!unlink (flagname)) {
				if (do_stat && config->doflagfile)
					status_line (msgtxt[M_CLEARED_FLAGFILE], flagname);
				return (TRUE);
			}

			return (FALSE);

		case TEST_FLAG:
			for (i = 0; i < numlocks; i++) {
				if (last_set[i].Zone == zone && last_set[i].Net == net && last_set[i].Node == node && last_set[i].Point == point)
					return (FALSE);
			}

			if (point != 0) {
				sprintf (flagname, "%s%04x%04x.PNT\\", HoldName, net, node);
				sprintf (BSYname, "%08x.BSY", point);
			}
			else {
				strcpy (flagname, HoldName);
				sprintf (BSYname, "%04x%04x.BSY", net, node);
			}

			strcat (flagname, BSYname);

			fptr = fopen (flagname, "rb");
			if (fptr == NULL)
				return (FALSE);
			fclose (fptr);

			return (TRUE);

		default:
			break;
	}

	return (TRUE);
}

void create_door_sys(char *path,int handle,int tasknum)
{

	char buffer[80],day[5],month[5],year[5],filename[20];
	FILE *fp;

	if(tasknum)
		sprintf (filename,"DOOR%d.SYS",line_offset);
	else
		strcpy (filename,"DOOR.SYS");

	if(*path)
		sprintf (buffer, "%s%s", path,filename);
	else
		sprintf (buffer, "%s%s", config->sys_path,filename);

		  fp = fopen(buffer,"wt");
	if (!fp) {
		status_line("!Unable to create %s",buffer);
		return;
	}
	status_line(":Creating %s",buffer);

	fprintf(fp,"COM%d:\n",local_mode ? 0 : com_port + 1);
	fprintf(fp,"%ld\n",local_mode ? 0 : speed);
	fprintf(fp,"8\n");
	fprintf(fp,"%d\n",line_offset);
	fprintf(fp,"%ld\n",local_mode ? 0 : rate );
	fprintf(fp,"Y\n");
	fprintf(fp,"Y\n");
	fprintf(fp,"Y\n");
	fprintf(fp,"Y\n");
	if(handle)
		fprintf(fp,"%s\n",usr.handle);
	else
		fprintf(fp,"%s\n",usr.name);
	fprintf(fp,"%s\n",usr.city);
	fprintf(fp,"%s\n",usr.voicephone);
	fprintf(fp,"%s\n",usr.dataphone);
//	fprintf(fp,"%s\n",usr.pwd);
	fprintf(fp,"\n"); // La password di Lora e' crittata, skippo.
	fprintf(fp,"%d\n",usr.priv);
	fprintf(fp,"%ld\n",usr.times);
	sscanf(usr.ldate,"%s%s%s",day,month,year);
	if(strstr(strupr(month),"JAN"))
		strcpy(month,"01");
	else if(strstr(month,"FEB"))
				strcpy(month,"02");
	else if(strstr(month,"MAR"))
				strcpy(month,"03");
	else if(strstr(month,"APR"))
				strcpy(month,"04");
	else if(strstr(month,"MAY"))
				strcpy(month,"05");
	else if(strstr(month,"JUN"))
				strcpy(month,"06");
	else if(strstr(month,"JUL"))
				strcpy(month,"07");
	else if(strstr(month,"AUG"))
				strcpy(month,"08");
	else if(strstr(month,"SEP"))
				strcpy(month,"09");
	else if(strstr(month,"OCT"))
				strcpy(month,"10");
	else if(strstr(month,"NOV"))
				strcpy(month,"11");
	else if(strstr(month,"DEC"))
				strcpy(month,"12");
	fprintf(fp,"%s/%s/%s\n",month,day,year);
	fprintf(fp,"%ld\n",(long)time_remain()*60);
	fprintf(fp,"%d\n",time_remain());
	strcpy(buffer,"NG");
	if(usr.ansi||usr.avatar)
		strcpy(buffer,"GR");
	fprintf(fp,"%s\n",buffer);
	fprintf(fp,"%d\n",usr.len);
		  fprintf(fp,"Y\n");
		  fprintf(fp,"\n");
		  fprintf(fp,"\n");
	fprintf(fp,"01/01/99\n");
	fprintf(fp,"1\n");
	fprintf(fp,"N\n");
	fprintf(fp,"%d\n",usr.n_upld);
	fprintf(fp,"%d\n",usr.n_dnld);
	fprintf(fp,"%d\n",usr.dnldl);
	fprintf(fp,"%d\n",config->class[usr_class].max_dl);

	fclose(fp);
	return;
}

void create_dorinfo_def(char *path,int handle,int tasknum)
{

	char buffer[80],filename[20],*p,g=0;
	FILE *fp;

	if(tasknum)
		sprintf (filename,"DORINFO%d.DEF",line_offset);
	else
		strcpy (filename,"DORINFO1.DEF");

	if(*path)
		sprintf (buffer, "%s%s", path,filename);
	else
		sprintf (buffer, "%s%s", config->sys_path,filename);

		  fp = fopen(buffer,"wt");
	if (!fp) {
		status_line("!Unable to create %s",buffer);
		return;
	}
	status_line(":Creating %s",buffer);
	strcpy(buffer,config->system_name);
	fprintf(fp,"%s\n",strupr(buffer));
	strcpy(buffer,config->sysop);
	p = strtok(buffer," ");
	if(p)
		fprintf(fp,"%s\n",strupr(p));
	else
		fprintf(fp,"\n");
	filename[0]=0;
	while((p=strtok(NULL," "))!=0)
		strcat(filename,p);
	fprintf(fp,"%s\n",strupr(filename));
	fprintf(fp,"COM%d\n",local_mode ? 0 : com_port + 1);
	fprintf(fp,"%ld BAUD,N,8,1\n",local_mode ? 0 : speed );
	fprintf(fp,"%d\n",line_offset);
	if(handle)
		strcpy(buffer,usr.handle);
	else
		strcpy(buffer,usr.name);
	if(!stricmp(buffer,config->sysop))
		strcpy(buffer,"SYSOP");
		p = strtok(buffer," ");
	if(p)
		fprintf(fp,"%s\n",strupr(p));
	else
		fprintf(fp,"\n");
	filename[0]=0;
	while((p=strtok(NULL," "))!=0)
		strcat(filename,p);
	fprintf(fp,"%s\n",strupr(filename));
	strcpy(buffer,usr.city);
	fprintf(fp,"%s\n",strupr(buffer));
	if(usr.avatar) g='2';
	else if(usr.ansi) g='1';
	fprintf(fp,"%c\n",g);
	fprintf(fp,"%d\n",usr.priv);
	fprintf(fp,"%d\n",time_remain());
	fprintf(fp,"-1");
	fclose(fp);
	return;
}
