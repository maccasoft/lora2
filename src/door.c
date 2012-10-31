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
#define INCL_DOSDEVICES
#define INCL_DOSDEVIOCTL
#define INCL_DOSSEMAPHORES
#define INCL_NOPM
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

static void general_door (char *, int);

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

static void general_door(s, flag)
char *s;
int flag;
{
   int i, m, oldtime, noreread, retval, oldcume, *varr;
   char ext[200], buffer[50], swapping, tmp[36];
   char noinfo;
   long t, freeze_time;
   struct _usr tmpusr;

   if (!flag)
      status_line(":External %s", s);

   noinfo = 0;
   freeze = 0;
   swapping = 0;
   m = 0;
   noreread = 0;
   set_useron_record(DOOR, 0, 0);
   ext[0] = '\0';

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
         case 'B':
            sprintf(buffer,"%u", local_mode ? 0 : rate);
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case 'C':
            if (getenv("COMSPEC") != NULL)
               strcpy(buffer,getenv("COMSPEC"));
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
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
#ifdef __OS2__
            sprintf (buffer, "%ld", hfComHandle);
            strcat (ext, buffer);
            m += strlen (buffer);
#else
            if (!local_mode)
               MDM_DISABLE();
#endif
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

   translate_filenames (ext, ' ', NULL);

   sprintf(buffer, "%sLORAINFO.T%02X", config->sys_path, line_offset);
   memcpy ((char *)&tmpusr, (char *)&usr, sizeof(struct _usr));
   oldcume = tmpusr.time;
   tmpusr.time += (int)((time(&t) - start_time) / 60);
   tmpusr.baud_rate = local_mode ? 0 : rate;
   lorainfo.baud = local_mode ? 0 : rate;
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
      write (i, (char *)&tmpusr, sizeof(struct _usr));
      write (i, (char *)&lorainfo, sizeof(struct _lorainfo));
      close (i);
   }

   if (!flag)
      read_system_file ("LEAVING");

   if (flag)
      status_line(":External %s", ext);

   varr = ssave ();
   cclrscrn(LGREY|_BLACK);
   showcur();
   fclose (logf);

   getcwd (buffer, 49);

   retval = spawn_program (swapping, ext);

   setdisk (buffer[0] - 'A');
   chdir (buffer);

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
         case '!':
            strcat(ext, "\n");
            m += strlen(buffer);
            break;
         case 'A':
            strcpy(tmp, usr.name);
            get_fancy_string(tmp, buffer);
            strcat(ext, strupr(buffer));
            m += strlen(buffer);
            break;
         case 'b':
            sprintf(buffer,"%u", local_mode ? 0 : rate);
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case 'B':
            strcpy(tmp, usr.name);
            get_fancy_string(tmp, buffer);
            get_fancy_string(tmp, buffer);
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
               sprintf(buffer, "-p%d -b%u", com_port+1, rate);
            else
               strcpy(buffer,"-k");
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case 'M':
            strcat(ext, sys.msg_path);
            m += strlen(sys.msg_path);
            break;
         case 'r':
         case 'n':
            strcat(ext, usr.name);
            m += strlen(usr.name);
            break;
         case 'N':
            strcat(ext, system_name);
            m += strlen(system_name);
            break;
         case 'p':
#ifdef __OS2__
            sprintf (buffer, "%ld", local_mode ? 0L : hfComHandle);
#else
            sprintf (buffer, "%d", com_port);
#endif
            strcat(ext, buffer);
            m += strlen(buffer);
            break;
         case 'P':
            sprintf(buffer,"%d", local_mode ? 0 : com_port+1);
            strcat(ext, buffer);
            m += strlen(buffer);
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
            strcat(ext, cmd_string);
            m += strlen(buffer);
            cmd_string[0] = '\0';
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
            sprintf (buffer,"%u", local_mode ? 0 : rate);
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

typedef struct {
   int Zone;
   int Net;
   int Node;
   int Point;
   char had_to_punt;
} ADDR;

int flag_file (function, zone, net, node, point, do_stat)
int function, zone, net, node, point, do_stat;
{
   FILE *fptr;
   int i;
   char *HoldName;
   char flagname[128];
   char tmpname[128];
   char BSYname[15];

   static ADDR last_set[30];
   static int numlocks = 0;

   if (!line_offset || !config->multitask)
      return (FALSE);

   HoldName = HoldAreaNameMunge (zone);

   switch (function) {
      case INITIALIZE:
         numlocks = 0;
         last_set[numlocks].Zone = -1;
         last_set[numlocks].had_to_punt = 0;

      case CLEAR_SESSION_FLAG:
         /* At the end of a session, delete the task file */
         if (config->flag_dir[0]) {
            (void) sprintf (flagname, "%sTask.%02x", config->flag_dir, line_offset);
            (void) unlink (flagname);
         }
         return (FALSE);

      case SET_SESSION_FLAG:
         /* At the start of a session, set up the task number */
         if (config->flag_dir[0]) {
            (void) sprintf (flagname, "%sTask.%02x", config->flag_dir, line_offset);
            fptr = fopen (flagname, "wb");
            (void) fclose (fptr);
         }
         return (FALSE);

      case TEST_AND_SET:

      /*
       * First see if we already HAVE this lock! If so, return now.
       *
       */

         for (i = 0; i < numlocks; i++) {
            if (last_set[i].Zone == zone && last_set[i].Net == net && last_set[i].Node == node && last_set[i].Point == point)
               return (FALSE);
         }

      /*
       * Next determine the directory in which we will create the flagfile.
       * Also, the name of the file.
       *
       */

         if (point != 0)
            {
            (void) sprintf (flagname, "%s%04x%04x.PNT\\", HoldName, net, node);
            (void) sprintf (BSYname, "%08x.BSY", point);
            }
         else
            {
            (void) strcpy (flagname, HoldName);
            (void) sprintf (BSYname, "%04x%04x.BSY", net, node);
            }
      /*
       * File opens are destructive by nature. Therefore use a file name
       * that's unique to us. Create it in the chosen target. If we can't
       * do that, try to use the flag directory.
       *
       */

         last_set[numlocks].had_to_punt = 0;
         (void) sprintf (tmpname, "%sLORABSY.%02x",flagname,line_offset);
         fptr = fopen (tmpname, "wb");
         if ((fptr == NULL) && (config->flag_dir[0])) {
            last_set[numlocks].had_to_punt = 1;
            (void) strcpy (flagname, config->flag_dir);
            (void) sprintf (tmpname, "%sLORABSY.%02x",flagname,line_offset);
            fptr = fopen (tmpname, "wb");
         }         
      /*
       * Now we've done all we can. The file is either open in the
       * appropriate outbound or it's in the flag directory.
       * If neither option worked out, go away. There's nothing to do.
       *
       */

         if (fptr == NULL) {
            if (do_stat && config->doflagfile)
               status_line (msgtxt[M_FAILED_CREATE_FLAG],tmpname);
            last_set[numlocks].Zone = -1;
            return (TRUE);
         }
         (void) fclose (fptr);

      /*
       * Now the test&set. Attempt to rename the file to a value specific
       * to the remote node's address. If we succeed, we have the lock.
       * If we do not, delete the temp file.
       *
       */
         (void) strcat (flagname, BSYname);   /* Add the .BSY file name */
         if (!rename (tmpname, flagname)) {
            if (do_stat && config->doflagfile)
               status_line (msgtxt[M_CREATED_FLAGFILE],flagname);
            last_set[numlocks].Zone = zone;
            last_set[numlocks].Net = net;
            last_set[numlocks].Node = node;
            last_set[numlocks].Point = point;
            numlocks++;
            return (FALSE);
         }

         if (do_stat && config->doflagfile)
            status_line (msgtxt[M_THIS_ADDRESS_LOCKED], zone, net, node, point);
         (void) unlink (tmpname);
         last_set[numlocks].Zone = -1;
         return (TRUE);

      case CLEAR_FLAG:

      /*
       * Make sure we need to clear something.
       * Zone should be something other than -1 if that's the case.
       *
       */
         if (numlocks == 0)
            return (TRUE);

      /*
       * Next compare what we want to clear with what we think we have.
       *
       */

         for (i = 0; i < numlocks; i++) {
            if (last_set[i].Zone == zone && last_set[i].Net == net && last_set[i].Node == node && last_set[i].Point == point)
               break;
         }
         if (i == numlocks) {
            if (do_stat && config->doflagfile)
               status_line (msgtxt[M_BAD_CLEAR_FLAGFILE], zone, net, node, point);
            return (TRUE);
         }             
     /*
      * We match. Recalculate the directory. Yeah, that's redundant
      * code, but it saves static space.
      *
      */
      if (point != 0)
         {
         (void) sprintf (flagname, "%s%04x%04x.PNT\\", HoldName, net, node);
         (void) sprintf (BSYname, "%08x.BSY", point);
         }
      else
         {
         (void) strcpy (flagname, HoldName);
         (void) sprintf (BSYname, "%04x%04x.BSY", net, node);
         }

         if (last_set[i].had_to_punt)
            (void) strcpy (flagname, config->flag_dir);
         (void) strcat (flagname, BSYname);

         last_set[i].had_to_punt = 0;
         last_set[i].Zone = -1;
         if (!unlink (flagname)) {
            if (do_stat && config->doflagfile)
               status_line (msgtxt[M_CLEARED_FLAGFILE], flagname);
            return (TRUE);
         }

//         if (do_stat && config->doflagfile)
//            status_line (msgtxt[M_FAILED_CLEAR_FLAG],flagname);
         return (FALSE);

      case TEST_FLAG:

      /*
       * First see if we already HAVE this lock! If so, return now.
       *
       */

         for (i = 0; i < numlocks; i++) {
            if (last_set[i].Zone == zone && last_set[i].Net == net && last_set[i].Node == node && last_set[i].Point == point)
               return (FALSE);
         }

      /*
       * Next determine the directory in which we will create the flagfile.
       * Also, the name of the file.
       *
       */

         if (point != 0)
            {
            (void) sprintf (flagname, "%s%04x%04x.PNT\\", HoldName, net, node);
            (void) sprintf (BSYname, "%08x.BSY", point);
            }
         else
            {
            (void) strcpy (flagname, HoldName);
            (void) sprintf (BSYname, "%04x%04x.BSY", net, node);
            }

      /*
       * Check for the *.BSY file.
       *
       */

         (void) strcat (flagname, BSYname);

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

