#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <process.h>
#include <io.h>
#include <dir.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"
#include "exec.h"

void update_status_line (void);
static void general_door (char *, int);

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
        char ext[200], buffer[50], swapping, *args, freeze, tmp[36];
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
                        if (s[i] == '_')
                                ext[m++] = ' ';
                        else if (s[i] == '\\' && s[i+1] == '_')
                                ext[m++] = s[++i];
                        else
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
                case 'B':
                        sprintf(buffer,"%d", local_mode ? 0 : rate);
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
                case 'W':
                        FOSSIL_WATCHDOG (1);
                        break;
                case 'X':
                        noinfo = 1;
                        break;
                default:
                        ext[m++] = '*';
                        ext[m++] = s[i];
                        ext[m] = '\0';
                        break;
                }
        }

        translate_filenames (ext, ' ', NULL);

        sprintf(buffer, "%sLORAINFO.T%02X", sys_path, line_offset);
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
        lorainfo.downloadlimit = class[usr_class].max_dl;
        lorainfo.max_time = class[usr_class].max_time;
        lorainfo.max_call = class[usr_class].max_call;
        lorainfo.ratio = class[usr_class].ratio;
        lorainfo.min_baud = class[usr_class].min_baud;
        lorainfo.min_file_baud = class[usr_class].min_file_baud;
        if (locked && password != NULL && registered)
        {
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

        varr = ssave ();
        cclrscrn(LGREY|_BLACK);
        showcur();
        fclose (logf);

        getcwd (buffer, 49);

        if (!swapping)
                retval = system (ext);
        else
        {
                if ((args = strchr (ext, ' ')) != NULL)
                   *args++ = '\0';
                retval = do_exec (ext, args, USE_ALL|HIDE_FILE, 0xFFFF, NULL);
        }

        setdisk (buffer[0] - 'A');
        chdir (buffer);

        if (!local_mode)
        {
                com_install (com_port);
                com_baud (rate);
                FOSSIL_WATCHDOG (0);
        }

        if (freeze)
                allowed += (int)((time(NULL)-freeze_time)/60);

        if (!noreread && !noinfo)
        {
                sprintf(buffer, "%sLORAINFO.T%02X", sys_path, line_offset);
                i = shopen (buffer, O_RDONLY|O_BINARY);
                read (i, (char *)&tmpusr, sizeof(struct _usr));
                read (i, (char *)&lorainfo, sizeof(struct _lorainfo));
                close (i);

                if (lorainfo.time != oldtime)
                {
                        if (time_to_next (1) < lorainfo.time)
                        {
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

        logf = fopen(log_name, "at");
        setbuf(logf, NULL);
        if (!flag)
                status_line(":Returned from door (%d)", retval);

        unlink(buffer);

        if (local_mode || snooping)
                showcur();

        if (varr != NULL)
           srestore (varr);

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

        for (i=0;i<strlen(s);i++)
        {
                if (s[i] != '%')
                {
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
                        sprintf(buffer,"%d", local_mode ? 0 : rate);
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
                        sprintf(buffer,"%d", class[usr_class].max_dl);
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
                        if (readln != NULL)
                        {
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
                                sprintf(buffer, "-p%d -b%d", com_port+1, rate);
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
                        sprintf(buffer,"%d", com_port);
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
                }
        }

        strcpy (s, ext);
}

int external_mailer(s)
char *s;
{
   int retval;
   char *args;

   status_line(":Shelling to front-end", s);

   cclrscrn(LGREY|_BLACK);
   showcur();
   fclose (logf);

   if ((args = strchr (s, ' ')) != NULL)
      *args++ = '\0';
   retval = do_exec (s, args, USE_ALL, 0xFFFF, NULL);

   com_install (com_port);
   com_baud (rate);
   FOSSIL_WATCHDOG (0);

   logf = fopen(log_name, "at");
   status_line(":Returned from front-end shell (%d)", retval);

   return (retval);
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
        else if (function_active == 10);
}

int flag_file (function, zone, net, node, do_stat)
int function, zone, net, node, do_stat;
{

   FILE *fptr;
   char *HoldName;
   static int last_set_zone, last_set_net, last_set_node;
   char flagname[128];
   char tmpname[128];

   if (!line_offset)
      return (FALSE);

   HoldName = flag_dir;

   switch (function)
      {
      case INITIALIZE:
      last_set_zone = -1;
      break;

      case SET_SESSION_FLAG:
      /* At the start of a session, set up the task number */
      if (flag_dir)
         {
         sprintf (flagname, "%sTask.%d",
                               flag_dir, line_offset);
         fptr = fopen (flagname, "wb");
         fclose (fptr);
         }
      return (FALSE);

      case CLEAR_SESSION_FLAG:
      /* At the end of a session, delete the task file */
      if (flag_dir)
         {
         sprintf (flagname, "%sTask.%d",
                               flag_dir, line_offset);
         unlink (flagname);
         }
      return (FALSE);

      case TEST_AND_SET:

   /*
    * First see if we already HAVE this lock! If so, return now.
    *
    */

      if (last_set_zone == zone &&
          last_set_net == net &&
          last_set_node == node)
         return (FALSE);

   /*
    * Check for the INMAIL.$$$ file.
    *
    */

      if (flag_dir != NULL)
         {
         sprintf (tmpname, "%sINMAIL.$$$",flag_dir);

         if ((fptr = fopen (tmpname, "rb")) != NULL)
            {
            fclose (fptr);
            if (!CARRIER)
               if (do_stat)
                  status_line (":Other Node Processing Mail");
            return (TRUE);
            }
         }

   /*
    * Next create a file using a temporary name.
    *
    */

      sprintf (tmpname, "%sLORA%04x.BSY",HoldName,line_offset);
      fptr = fopen (tmpname, "wb");
      if (fptr == NULL)
         {
         if (do_stat)
            status_line (msgtxt[M_FAILED_CREATE_FLAG],tmpname);
         return (TRUE);
         }
      fclose (fptr);

   /*
    * Now the test&set. Attempt to rename the file.
    * If we succeed, we have the lock. If we do not,
    * delete the temp file.
    *
    */

      sprintf (flagname, "%s%04x%04x.BSY",
                            HoldAreaNameMunge (zone), net, node);
      if (!rename (tmpname, flagname))
         {
//         if (do_stat)
//            status_line (msgtxt[M_CREATED_FLAGFILE],flagname);
         last_set_zone = zone;
         last_set_net = net;
         last_set_node = node;
         return (FALSE);
         }
//      if (do_stat)
//         status_line (msgtxt[M_THIS_ADDRESS_LOCKED], zone, net, node);
      unlink (tmpname);
      return (TRUE);

      case CLEAR_FLAG:

   /*
    * We match. Unlink the flag file. If we're successful, jam a -1
    * into our saved Zone.
    *
    */

      sprintf (flagname, "%s%04x%04x.BSY",
                            HoldAreaNameMunge (zone), net, node);
      if (!unlink (flagname))
         {
//         if (do_stat)
//            status_line (msgtxt[M_CLEARED_FLAGFILE], flagname);
         last_set_zone = -1;
         return (TRUE);
         }

//      if (do_stat)
//         status_line (msgtxt[M_FAILED_CLEAR_FLAG],flagname);
      return (FALSE);

      case TEST_FLAG:

   /*
    * First see if we already HAVE this lock! If so, return now.
    *
    */

      if (last_set_zone == zone &&
          last_set_net == net &&
          last_set_node == node)
         return (FALSE);

   /*
    * Check for the *.BSY file.
    *
    */

      if (flag_dir != NULL)
         {
         sprintf (tmpname, "%s%04x%04x.BSY",
                            HoldAreaNameMunge (zone), net, node);

         if ((fptr = fopen (tmpname, "rb")) != NULL)
            {
            fclose (fptr);
            return (TRUE);
            }
         }

      return (FALSE);

      default:
      break;
      }

   return (TRUE);
}

