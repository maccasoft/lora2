#include <stdio.h>
#include <io.h>
#include <ctype.h>
#include <string.h>
#include <dir.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"

char *translate_ansi (char *);

static FILE *Infile;
static int fsize2 = 0;

static int prot_read_file (char *);
static int file_more_question (int, int);

void file_list (path, list)
char *path, *list;
{
   FILE *fp;
   int m, i, yr, mo, dy, line;
   char buffer[150], filename[80], name[14], comment[60];
   struct ffblk blk;

   if (list == NULL || !(*list)) {
      i = strlen(path) - 1;
      while (path[i] == ' ')
         path[i--] = '\0';
      while (*path == ' ' && *path != '\0')
         path++;

      sprintf(filename,"%sFILES.BBS",path);
      fp = fopen(filename,"rt");
   }
   else {
      status_line(msgtxt[M_READ_FILE_LIST],list);
      fp = fopen(list,"rt");
   }

   if (fp == NULL)
      return;

   cls();

   m_print(bbstxt[B_CONTROLS]);
   m_print(bbstxt[B_CONTROLS2]);
   change_attr (CYAN|_BLACK);

   line = 3;

   while(fgets(buffer,138,fp) != NULL && CARRIER && !RECVD_BREAK()) {
      buffer[strlen(buffer)-1] = 0;

      if (buffer[0] == '-') {
         change_attr (WHITE|_BLACK);
         m_print ("%s\n", buffer);
         continue;
      }

      m = 0;
      i = 0;
      while(buffer[m] != ' ' && buffer[m])
         name[i++] = buffer[m++];
      name[i]='\0';

      while(buffer[m] == ' ')
         m++;

      if (!sys.no_filedate || path != NULL) {
         sprintf(filename,"%s%s",path,name);
         if(findfirst(filename,&blk,0))
            continue;

         yr = ((blk.ff_fdate >> 9) & 0x7F);
         mo = ((blk.ff_fdate >> 5) & 0x0F);
         dy = blk.ff_fdate & 0x1F;
      }
      else {
         i = 0;
         while(buffer[m] != '\0' && i < 48)
            filename[i++] = buffer[m++];
         filename[i]='\0';

         sscanf (buffer, "%2d-%2d-%2d", &mo, &dy, &yr);
         yr -= 80;

         while (buffer[m] == ' ')
            m++;

         i = 0;
         while(buffer[m] != '\0' && i < 48)
            filename[i++] = buffer[m++];
         filename[i]='\0';

         sscanf (buffer, "%ld", &blk.ff_fsize);

         while (buffer[m] == ' ')
            m++;
      }

      i = 0;
      while(buffer[m] != '\0' && i < 48)
         comment[i++] = buffer[m++];
      comment[i]='\0';

      m_print(bbstxt[B_FILES_FORMAT],strupr(name),blk.ff_fsize,dy,mo,(yr+80)%100,comment);

      if ((line=file_more_question(line, 1)) == 0 || !CARRIER)
         break;
   }

   fclose(fp);

   m_print (bbstxt[B_ONE_CR]);
   if (line && CARRIER)
      press_enter();
}

void file_display()
{
   char filename[80], stringa[20];

   if (!get_command_word (stringa, 14)) {
      m_print(bbstxt[B_WHICH_FILE]);
      input(stringa,14);
      if(!strlen(stringa))
         return;
   }

   status_line("+Display '%s'",strupr(stringa));
   sprintf(filename,"%s%s",sys.filepath,stringa);

   cls();

   if (prot_read_file(filename) > 1 && CARRIER)
      press_enter();
}

void override_path()
{
   char stringa[80];

   if (!get_command_word (stringa, 78)) {
      m_print(bbstxt[B_FULL_OVERRIDE]);
      input(stringa, 78);
      if(!strlen(stringa))
         return;
   }

   append_backslash (stringa);
   status_line("*Path override '%s'",strupr(stringa));

   strcpy (sys.filepath, stringa);
   fancy_str (stringa);
   strcpy (sys.file_name, stringa);
}

static int prot_read_file(name)
char *name;
{
   FILE *fp;
   char linea[130], *p;
   int line;

   XON_ENABLE();
   _BRK_ENABLE();

   if(!name || !(*name))
      return(0);

   fp = fopen(name,"rb");
   if (fp == NULL)
      return(0);

   line = 1;
   nopause = 0;

loop:
   change_attr(LGREY|_BLACK);

   while (fgets(linea,128,fp) != NULL) {
      linea[strlen(linea) - 2] = '\0';

      for (p=linea;(*p);p++) {
         if (!CARRIER || RECVD_BREAK()) {
            CLEAR_OUTBOUND();
            fclose(fp);
            return (0);
         }

         if (*p == 0x1B)
            p = translate_ansi (p);
         else {
            if (!local_mode)
               BUFFER_BYTE(*p);
            if (snooping)
               wputc(*p);
         }
      }

      if (!local_mode) {
         BUFFER_BYTE('\r');
         BUFFER_BYTE('\n');
      }
      if (snooping)
         wputs("\n");

      if(!(line++ < (usr.len-1)) && usr.len != 0) {
         line = 1;
         if(!continua()) {
            fclose(fp);
            break;
         }
      }

      if (*p == CTRLZ) {
         fclose(fp);
         UNBUFFER_BYTES ();
         FLUSH_OUTPUT ();
         break;
      }
   }

   fclose(fp);

   UNBUFFER_BYTES ();
   FLUSH_OUTPUT();

   return(line);
}

void raw_dir()
{
   int yr, mo, dy, line, nfiles;
   char stringa[30], filename[80];
   long fsize;
   struct ffblk blk;

   nopause = 0;

   if (!get_command_word (stringa, 14)) {
      m_print(bbstxt[B_DIRMASK]);
      input(stringa,14);
      if(!CARRIER)
         return;
   }

   if (!strlen (stringa))
      strcpy (stringa, "*.*");

   sprintf(filename,"%s%s",sys.filepath, stringa);

   cls ();
   line = 1;
   nfiles = 0;
   fsize = 0L;

   if (!findfirst(filename,&blk,0)) {
      cls ();
      m_print (bbstxt[B_RAW_HEADER]);

      do {
         yr = (blk.ff_fdate >> 9) & 0x7F;
         mo = (blk.ff_fdate >> 5) & 0x0F;
         dy = blk.ff_fdate & 0x1F;

         m_print(bbstxt[B_RAW_FORMAT], blk.ff_name, blk.ff_fsize, dy, mesi[mo-1], (yr+80)%100);

         nfiles++;
         fsize += blk.ff_fsize;

         if (!(line=file_more_question(line, 1)) || !CARRIER)
            break;
      } while(!findnext(&blk) && CARRIER && !RECVD_BREAK());
   }

   if (CARRIER && line) {
      sprintf (stringa, "%d files", nfiles);
      m_print (bbstxt[B_RAW_FOOTER], stringa, fsize);
      press_enter();
   }
}

void new_file_list (only_one)
int only_one;
{
   FILE *fp;
   int day, mont, year, date, line, m, z, fd;
   word search;
   long size, totfile, totsize;
   char month[4], tmp_date[20], name[13], desc[46];
   char stringa[120], filename[80];
   struct ffblk blk;
   struct _sys tsys;

   strcpy(tmp_date, usr.ldate);
   tmp_date[9] = '\0';
   totfile = totsize = 0L;

   if (only_one != 2)
      m_print(bbstxt[B_NEW_SINCE_LAST]);

   if (only_one != 2 && yesno_question(DEF_YES) == DEF_NO) {
      if (!get_command_word (stringa, 8)) {
         m_print(bbstxt[B_NEW_DATE]);
         input(stringa,8);
         if(stringa[0] == '\0' || !CARRIER)
            return;
      }

      sscanf(stringa, "%2d-%2d-%2d", &day, &mont, &year);
      mont--;
   }
   else {
      sscanf(usr.ldate, "%2d %3s %2d", &day, month, &year);
      month[3] = '\0';
      for (mont = 0; mont < 12; mont++) {
         if ((!stricmp(mtext[mont], month)) || (!stricmp(mesi[mont], month)))
            break;
      }
   }

   if (only_one == 2)
      only_one = 0;

   if ((mont == 12) || (day>31) || (day<1) || (year>99) || (year<80) || !CARRIER)
      return;

   search=(year-80)*512+(mont+1)*32+day;

   cls();
   m_print(bbstxt[B_DATE_SEARCH]);
   m_print(bbstxt[B_DATE_UNDERLINE]);

   line = 4;

   sprintf(filename,"%sSYSFILE.DAT", sys_path);
   fd = shopen(filename, O_RDONLY|O_BINARY);
   if (fd == -1) {
      status_line(msgtxt[M_NO_SYSTEM_FILE]);
      return;
   }

   while (read(fd, (char *)&tsys.file_name,SIZEOF_FILEAREA) == SIZEOF_FILEAREA && line) {
      if (only_one)
         memcpy ((char *)&tsys.file_name, (char *)&sys.file_name, SIZEOF_FILEAREA);
      else {
         if (usr.priv < tsys.file_priv || tsys.no_global_search)
            continue;
         if((usr.flags & tsys.file_flags) != tsys.file_flags)
            continue;
      }

      m_print(bbstxt[B_AREALIST_HEADER],tsys.file_num,tsys.file_name);
      if ((line=file_more_question(line,only_one)) == 0)
              break;

      sprintf(filename,"%sFILES.BBS",tsys.filepath);
      fp = fopen(filename,"rt");
      if(fp == NULL)
         continue;

      while(fgets(stringa,110,fp) != NULL && line) {
         stringa[strlen(stringa)-1] = 0;

         if (!CARRIER || RECVD_BREAK()) {
            line = 0;
            break;
         }

         if(!isalnum(stringa[0]))
            continue;

         m=0;
         while(stringa[m] != ' ' && m<12)
            name[m] = stringa[m++];

         name[m]='\0';

         sprintf(filename,"%s%s",tsys.filepath,name);
         if(findfirst(filename,&blk,0))
            continue;

         if (blk.ff_fdate >= search) {
            size=blk.ff_fsize;
            date=blk.ff_fdate;
            z=0;
            while(stringa[m] == ' ')
               m++;
            while(stringa[m] && z<45)
               desc[z++]=stringa[m++];
            desc[z]='\0';

            totsize += size;
            totfile++;

            m_print(bbstxt[B_FILES_FORMAT],strupr(name),size,(date & 0x1F), ((date >> 5) & 0x0F),((((date >> 9) & 0x7F)+80)%100), desc);

            if (!(line=file_more_question(line,only_one)) || !CARRIER)
               break;
         }
      }

      fclose(fp);

      if (only_one)
         break;
   }

   close(fd);

   m_print (bbstxt[B_LOCATED_MATCH], totfile, totsize);

   if (line && CARRIER)
      press_enter();
}

void download_file(st, global)
char *st;
int global;
{
   int fd, i, m, protocol;
   char filename[80], path[80], dir[80], name[80], ext[5];
   long rp;
   struct ffblk blk;
   struct _sys tsys;

   fnsplit(st,path,dir,name,ext);
   if(!sys.freearea && !usr.xfer_prior && class[usr_class].ratio && !name[0] && usr.n_dnld >= class[usr_class].start_ratio) {
      rp = usr.upld ? (usr.dnld/usr.upld) : usr.dnld;
      if (rp > (long)class[usr_class].ratio) {
         cmd_string[0] = '\0';
         status_line (":Dnld req. would exceed ratio");
         read_system_file ("RATIO");
         return;
      }
   }

   if (user_status != UPLDNLD)
      set_useron_record(UPLDNLD, 0, 0);

   read_system_file ("PREDNLD");

   if (st == NULL)
      cls();
   if ((protocol = selprot()) == 0)
      return;

   fnsplit (st, path, dir, name, ext);
   strcat (path, dir);
   strcat (name, ext);

   if (!dir[0] && st != NULL)
      strcpy (path, sys.filepath);

   if (!name[0]) {
      if (!cmd_string[0]) {
         ask_for_filename (name);
         if (!name[0] || !CARRIER)
            return;
      }
      else {
         strcpy (name, cmd_string);
         cmd_string[0] = '\0';
      }
   }

   if (!display_transfer (protocol, path, name, global))
      return;

   m_print(bbstxt[B_READY_TO_SEND]);
   m_print(bbstxt[B_CTRLX_ABORT]);

   if (local_mode) {
      sysop_error();
      m = 0;
      goto abort_xfer;
   }

   if (protocol == 1 || protocol == 2 || protocol == 3 || protocol == 6) {
      timer (10);
      m = i = 0;

      while (get_string (name, dir) != NULL) {
         sprintf (filename, "%s%s", path, dir);

         if (findfirst(filename,&blk,0)) {
            sprintf (filename, "%sSYSFILE.DAT", sys_path);
            fd = shopen (filename, O_RDONLY|O_BINARY);

            if (global) {
               while (read(fd, (char *)&tsys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA) {
                  if (usr.priv < tsys.file_priv || tsys.no_global_search)
                     continue;
                  if((usr.flags & tsys.file_flags) != tsys.file_flags)
                     continue;
                  if (usr.priv < tsys.download_priv)
                     continue;
                  if((usr.flags & tsys.download_flags) != tsys.download_flags)
                     continue;

                  sprintf (filename, "%s%s", tsys.filepath, dir);

                  if (!findfirst (filename, &blk, 0))
                     break;
               }
            }

            close (fd);
         }

         switch (protocol) {
            case 1:
               m = send (filename, 'X');
               break;
            case 2:
               m = send (filename, 'Y');
               break;
            case 3:
               m = send_Zmodem (filename, dir, i, 0);
               break;
            case 6:
               m = send (filename, 'S');
               break;
         }

         if (!m)
            break;

         i++;

         if (!sys.freearea || st == NULL) {
            usr.dnld += (int)(blk.ff_fsize / 1024L) + 1;
            usr.dnldl += (int)(blk.ff_fsize / 1024L) + 1;
            usr.n_dnld++;

            if (function_active == 3)
               f3_status ();
         }
      }

      if (protocol == 3)
         send_Zmodem (NULL, NULL, ((i) ? END_BATCH : NOTHING_TO_DO), 0);
      else if (protocol == 6)
         send (NULL, 'S');

abort_xfer:
      wactiv (mainview);

      if (m)
         m_print(bbstxt[B_TRANSFER_OK]);
      else {
         m_print(bbstxt[B_TRANSFER_ABORT]);
         sysinfo.lost_download++;
      }
   }
   else {
      if (protocol == 4)
         hslink_protocol (name, path, global);
      else if (protocol == 5)
         puma_protocol (name, path, global, 0);
   }

   press_enter();
}

int display_transfer(protocol, path, name, flag)
int protocol;
char *path, *name;
int flag;
{
   int fd, i, byte_sec, min, sec, nfiles, errfl;
   char filename[80], root[14], back[80];
   long fl;
   struct ffblk blk;
   struct _sys tsys;

   back[0] = '\0';
   fl = 0L;
   nfiles = 0;
   errfl = 0;

   while (get_string (name, root) != NULL) {
      sprintf (filename, "%s%s", path, root);

      if (findfirst (filename, &blk, 0)) {
         sprintf (filename, "%sSYSFILE.DAT", sys_path);
         fd = shopen (filename, O_RDONLY|O_BINARY);

         if (flag) {
            i = 0;

            while (read(fd, (char *)&tsys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA) {
               if (usr.priv < tsys.file_priv || tsys.no_global_search)
                  continue;
               if((usr.flags & tsys.file_flags) != tsys.file_flags)
                  continue;
               if (usr.priv < tsys.download_priv)
                  continue;
               if((usr.flags & tsys.download_flags) != tsys.download_flags)
                  continue;

               sprintf (filename, "%s%s", tsys.filepath, root);

               if (findfirst(filename, &blk, 0))
                  continue;

               do {
                  if ((((fl + blk.ff_fsize) / 1024) + usr.dnldl) > class[usr_class].max_dl && !sys.freearea && !usr.xfer_prior) {
                     status_line (":Dnld req. would exceed limit");
                     errfl = 1;
                     break;
                  }

                  if (strlen (back) + strlen (blk.ff_name) + 1 < 79) {
                     if (nfiles)
                        strcat (back, " ");
                     strcat (back, blk.ff_name);
                     fl += blk.ff_fsize;
                     nfiles++;
                     i = 1;
                  }
               } while (!findnext (&blk));
            }

            if (!i)
               m_print (bbstxt[B_FILE_NOT_FOUND], strupr (root));
         }
         else
            m_print (bbstxt[B_FILE_NOT_FOUND], strupr (root));

         close (fd);
      }
      else
         do {
            if ((((fl + blk.ff_fsize) / 1024) + usr.dnldl) > class[usr_class].max_dl && !sys.freearea && !usr.xfer_prior) {
               status_line (":Dnld req. would exceed limit");
               errfl = 1;
               continue;
            }

            if (strlen (back) + strlen (blk.ff_name) + 1 < 79) {
               if (nfiles)
                  strcat (back, " ");
               strcat (back, blk.ff_name);
               fl += blk.ff_fsize;
               nfiles++;
            }
         } while (!findnext (&blk));
   }

   if (!back[0])
      return (0);

   cls();

   m_print (bbstxt[B_FILE_MASK], strupr (back));
   m_print (bbstxt[B_LEN_MASK], fl);

   byte_sec = rate / 11;
   i = (int)((fl + (fl / 1024)) / byte_sec);
   min = i / 60;
   sec = i - min * 60;

   m_print (bbstxt[B_TOTAL_TIME], min, sec);
   m_print (bbstxt[B_PROTOCOL], protocols[protocol - 1]);

   if(((fl/1024) + usr.dnldl) > class[usr_class].max_dl && !sys.freearea && !usr.xfer_prior) {
      status_line (":Dnld req. would exceed limit");
      read_system_file ("TODAYK");
      return(0);
   }
   else if (errfl && nfiles == 1) {
      read_system_file ("TODAYK");
      return(0);
   }

   if (min > time_remain() && !sys.freearea && !usr.xfer_prior) {
      status_line (":Dnld req. would exceed limit");
      read_system_file ("NOTIME");
      return (0);
   }

   strcpy (name, back);
   return (1);
}

void ask_for_filename(name)
char *name;
{
   char stringa[80];

   for (;;) {
      m_print (bbstxt[B_DOWNLOAD_NAME]);
      input (stringa, 79);
      if (!stringa[0] || !CARRIER)
         return;

      if (invalid_filename(stringa) && usr.priv < SYSOP) {
         m_print (bbstxt[B_INVALID_FILENAME]);
         status_line (msgtxt[M_DL_PATH], stringa);
      }
      else
         break;
   }

   strcpy (name, stringa);
}

int invalid_filename(f)
char *f;
{
   if (strchr(f,'\\') != NULL)
      return(1);
   if (strchr(f,':') != NULL)
      return(1);

   return(0);
}

void upload_file(st,pr)
char *st, pr;
{
        FILE *xferinfo, *fp;
        int i, protocol;
        char filename[80], path[80], stringa[80], name[20], ext[5], *file;
        long fpos, start_upload;
        struct ffblk blk;

        if (pr == 0) {
                cls ();
                read_system_file ("PREUPLD");

                if ((protocol=selprot()) == 0)
                        return;
        }
        else
                protocol = pr;

        if (!st[0] || st == NULL)
                strcpy(path,sys.uppath);
        else
                strcpy(path,st);

        if (protocol == 1 || protocol == 2)
                for (;;) {
                        if (!get_command_word (name, 14))
                        {
                                m_print(bbstxt[B_UPLOAD_NAME]);
                                input(name,14);
                                if(!name[0] || !CARRIER)
                                        return;
                        }

                        sprintf(filename,"%s%s",path,name);
                        if(findfirst(filename,&blk,0))
                                break;

                        m_print(bbstxt[B_ALREADY_HERE]);
                }

        if (pr == 0) {
                m_print(bbstxt[B_READY_TO_RECEIVE],protocols[protocol-1]);
                m_print(bbstxt[B_CTRLX_ABORT]);
        }

        start_upload = time(0);

        if (local_mode) {
                sysop_error();
                i = 0;
                file = NULL;
                goto abort_xfer;
        }

        if (protocol == 1 || protocol == 2 || protocol == 3 || protocol == 6) {
                timer (50);

                sprintf (filename, "XFER%d", line_offset);
                xferinfo = fopen(filename,"w+t");

                i = 0;
                file = NULL;

                switch (protocol) {
                case 1:
                        who_is_he = 0;
                        overwrite = 0;
                        file = receive(path, name, 'X');
                        if (file != NULL)
                                fprintf(xferinfo, "%s\n", file);
                        break;

                case 2:
                        who_is_he = 0;
                        overwrite = 0;
                        file = receive(path, name, 'Y');
                        if (file != NULL)
                                fprintf(xferinfo, "%s\n", file);
                        break;

                case 3:
                        i = get_Zmodem(path,xferinfo);
                        break;

                case 6:
                        i = 0;
                        do {
                                who_is_he = 0;
                                overwrite = 0;
                                file = receive(path, NULL, 'S');
                                if (file != NULL)
                                {
                                        fprintf(xferinfo, "%s\n", file);
                                        i++;
                                }
                        } while (file != NULL);
                        break;

                }

abort_xfer:
                if (!CARRIER) {
                        fclose(xferinfo);
                        sprintf (filename, "XFER%d", line_offset);
                        unlink (filename);
                        return;
                }

                wactiv (mainview);

                allowed += (int)((time(NULL) - start_upload) / 60);

                CLEAR_INBOUND();
                if (i || file != NULL) {
                        m_print(bbstxt[B_TRANSFER_OK]);
                        timer(20);
                        m_print(bbstxt[B_THANKS],usr.name);
                }
                else {
                        m_print(bbstxt[B_TRANSFER_ABORT]);
                        fclose(xferinfo);
                        sprintf (filename, "XFER%d", line_offset);
                        unlink (filename);
                        press_enter();
                        return;
                }

                rewind (xferinfo);
                fpos = filelength(fileno (xferinfo));

                while(ftell(xferinfo) < fpos) {
                        fgets(filename,78,xferinfo);
                        filename[strlen(filename) - 1] = '\0';

                        fnsplit(filename,NULL,NULL,name,ext);
                        strcat(name,ext);

                        do {
                                m_print(bbstxt[B_DESCRIBE],strupr(name));
                                input(stringa,54);
                                if (!CARRIER) {
                                        fclose(xferinfo);
                                        return;
                                }
                        } while (strlen(stringa) < 5);

                        sprintf(filename,"%sFILES.BBS",sys.uppath);
                        fp = fopen(filename,"at");
                        fprintf(fp,"%-12s %s\n",name,stringa);
                        fclose(fp);

                        sprintf(filename,"%s\\%s",path,name);
                        findfirst(filename,&blk,0);

                        usr.upld+=(int)(blk.ff_fsize/1024L)+1;
                        usr.n_upld++;

                        if (function_active == 3)
                                f3_status ();
                }

                fclose(xferinfo);
                sprintf (filename, "XFER%d", line_offset);
                unlink (filename);

                press_enter();
        }
        else {
                if (protocol == 4)
                        hslink_protocol (NULL, NULL, 0);
                else if (protocol == 5)
                        puma_protocol (NULL, NULL, 0, 1);
                wactiv (mainview);
                allowed += (int)((time(NULL) - start_upload) / 60);
        }
}

int selprot()
{
   char c, stringa[4];

   if ((c=get_command_letter ()) == '\0') {
      if (!read_system_file ("XFERPROT")) {
         m_print (bbstxt[B_PROTOCOLS]);
         m_print (bbstxt[B_PROTOCOL_FORMAT], protocols[0][0], protocols[0]);
         m_print (bbstxt[B_PROTOCOL_FORMAT], protocols[1][0], protocols[1]);
         m_print (bbstxt[B_PROTOCOL_FORMAT], protocols[2][0], protocols[2]);
         m_print (bbstxt[B_PROTOCOL_FORMAT], protocols[5][0], protocols[5]);
         if (noask.hslink)
            m_print (bbstxt[B_PROTOCOL_FORMAT], protocols[3][0], protocols[3]);
         if (noask.puma)
            m_print (bbstxt[B_PROTOCOL_FORMAT], protocols[4][0], protocols[4]);
         m_print (bbstxt[B_SELECT_PROT]);
      }

      if (usr.hotkey) {
         cmd_input (stringa, 1);
         m_print (bbstxt[B_ONE_CR]);
      }
      else
         input (stringa, 1);
      c = stringa[0];
      if (c == '\0' || c == '\r' || !CARRIER)
         return(0);
   }

   switch(toupper(c)) {
      case 'X':
         return (1);
      case '1':
         return (2);
      case 'Z':
         return (3);
      case 'H':
         if (noask.hslink)
            return (4);
         else
            return (0);
      case 'P':
         if (noask.puma)
            return (5);
         else
            return (0);
      case 'S':
         return (6);
      default:
         return(0);
   }
}

void SendACK()
{
   char temp[20];

   if ((!recv_ackless) || (block_number == 0)) {
      SENDBYTE(ACK);
      if (sliding)
      {
         SENDBYTE((unsigned char )block_number);
         SENDBYTE((unsigned char )(~(block_number)));
      }
   }

   sprintf (temp, "%5d", block_number);
   wgotoxy (1, 1);
   wputs (temp);

   errs = 0;
}

void SendNAK()
{
   int i;
   long t1;

   errs++;
   real_errs++;
   if (errs > 6)
      return;

   if(++did_nak > 8)
      recv_ackless = 0;

   CLEAR_INBOUND();

   if(!recv_ackless) {
      t1 = timerset(3000);
      if((base_block != block_number) || (errs > 1))
         do {
            i = TIMED_READ(1);
            if (!CARRIER)
               return;
            if(timeup(t1))
               break;
            time_release();
         } while(i >= 0);
   }

   if(block_number > base_block)
      SENDBYTE(NAK);
   else {
      if ((errs < 5) && (!do_chksum))
         SENDBYTE('C');
      else {
         do_chksum = 1;
         SENDBYTE(NAK);
      }
   }

   if(sliding) {
      SENDBYTE((unsigned char )block_number);
      SENDBYTE((unsigned char )(~(block_number)));
   }
}

void getblock()
{
        register int i;
        char *sptr, rbuffer[1024];
        unsigned int crc;
        int in_char, is_resend, blockerr;
        unsigned char chksum;
        struct zero_block *testa;

        blockerr=0;
        is_resend=0;
        chksum='\0';
        testa = (struct zero_block *)&rbuffer[0];

        in_char = TIMED_READ(5);
        if(in_char != (block_number & 0xff)) {
                if(in_char < (int)block_number)
                        is_resend = 1;
                else if((block_number) || (in_char != 1))
                        blockerr++;
                else
                        block_number = 1;
        }

        i = TIMED_READ(5);
        if((i & 0xff) != ((~in_char) & 0xff))
                blockerr++;

        for(sptr = rbuffer,i=0;i<block_size;i++,sptr++) {
                in_char = TIMED_READ(5);
                if(in_char < 0) {
                        if(CARRIER) {
                                SendNAK();
                        }
                        return;
                }
                sptr[0] = (char )in_char;
        }

        if(do_chksum) {
                for(sptr = rbuffer,i=0;i<block_size;i++,sptr++)
                        chksum += sptr[0];

                if(TIMED_READ(5) != chksum) {
                        blockerr++;
                }
        }
        else {
                int lsb, msb;

                for(sptr = rbuffer,i=crc=0;i<block_size;i++,sptr++)
                        crc = xcrc(crc,(byte )sptr[0]);

                msb = TIMED_READ(5);
                lsb = TIMED_READ(5);
                if((lsb < 0) || (msb < 0)) {
                        if(!block_number)
                                sliding = 0;
                        blockerr++;
                }
                else if(((msb << 8) | lsb) != crc) {
                        blockerr++;
                }
        }

        if (blockerr)
                SendNAK();
        else {
                SendACK();

                if(is_resend)
                        return;

                if(block_number) {
                        if(block_number < fsize1)
                                fwrite(rbuffer,block_size,1,Infile);
                        else if (block_number == fsize1)
                                fwrite(rbuffer,fsize2,1,Infile);
                }
                else if (first_block && !may_be_seadog)
                {
                        sptr = &(testa->name[0]);
                        if(sptr[0])
                        {
                                sprintf(final_name,"%s%s",_fpath,sptr);
                                sptr = final_name;
                                fancy_str(sptr);
                                i = strlen(sptr)-1;
                                if((isdigit(sptr[i])) && (sptr[i-1]=='o') && (sptr[i-2]=='m') && (sptr[i-3]=='.'))
                                        got_arcmail=1;
                                wgotoxy (0, 13);
                                wclreol ();
                                wputs (final_name);
                        }
                }
                else if (!first_block) {
                        sptr = &(testa->name[0]);
                        if (netmail)
                                invent_pkt_name(sptr);

                        for(i=0;((sptr[i]) && (i < 17));i++)
                                if (sptr[i]<=' ')
                                        sptr[i]='\0';

                        if(sptr[0])
                        {
                                sprintf(final_name,"%s%s",_fpath,sptr);
                                sptr = final_name;
                                fancy_str(sptr);
                                i = strlen(sptr)-1;
                                if((isdigit(sptr[i])) && (sptr[i-1]=='o') && (sptr[i-2]=='m') && (sptr[i-3]=='.'))
                                        got_arcmail=1;
                                wgotoxy (0, 13);
                                wclreol ();
                                wputs (final_name);
                        }
                        else
                                status_line(msgtxt[M_GRUNGED_HEADER]);

                        if(testa->size)
                        {
                                fsize1 = (int)(testa->size / 128L);
                                if((testa->size % 128) != 0) {
                                        fsize2 = (int)(testa->size % 128);
                                        ++fsize1;
                                }
                        }

                        if((rate >= 9600) && testa->noacks && !no_overdrive)
                                recv_ackless = 1;

                        first_block = 0;
                }

                block_number++;
        }
}

struct zero_block1 {
        long size;
        long time;
        char name[17];
        char moi[15];
        char noacks;
};

char *receive(fpath,fname,protocol)
char *fpath, *fname, protocol;
{
        char tmpname[80];
        int in_char, i, wh;
        long t1;

        did_nak = 0;
        wh = -1;

        if (protocol == 'F') {
                may_be_seadog = 1;
                netmail = 0;
                protocol = 'S';
                first_block = 0;
        }
        else if (protocol == 'B') {
                protocol = 'S';
                may_be_seadog = 1;
                netmail = 1;
                first_block = 1;
        }
        else if (protocol == 'T') {
                protocol = 'S';
                may_be_seadog = 1;
                netmail = 0;
                first_block = 1;
        }
        else {
                may_be_seadog = 0;
                netmail = 0;
                first_block = 0;
        }

        _BRK_DISABLE ();
        XON_DISABLE ();

        fsize1 = 32767;
        if(fname)
                for(in_char = 0;fname[in_char];in_char++)
                        if((fname[in_char]=='*') || (fname[in_char]=='?'))
                                fname[0]='\0';

        _fpath = fpath;
        sliding=1;
        base_block=0;
        do_chksum=0;
        errs=0;
        block_size=128;
        fsize2 = 128;

        switch(protocol) {
        case 'X' :
                base_block=1;
                sliding=0;
                break;
        case 'Y' :
                base_block=1;
                sliding=0;
                fsize2=block_size=1024;
                break;
        case 'S' :
                break;
        case 'T' :
                break;
        case 'M' :
                base_block=1;
                sliding=0;
                break;
        default  :
                return NULL;
        }

        block_number = base_block;

        sprintf(tmpname,"%s_TMP%d_.$$$",fpath,line_offset);
        sprintf(final_name,"%s%s",fpath,(fname && fname[0]) ? fname:"UNKNOWN.$$$");

        status_line(" %s-%c %s",msgtxt[M_RECEIVING],protocol,final_name);

        wh = wopen(16,0,19,79,1,WHITE|_RED,WHITE|_RED);
        wactiv(wh);
        wtitle(" Transfer Status ", TLEFT, WHITE|_RED);

        wgotoxy (0, 1);
        wputs (msgtxt[M_RECEIVING]);
        wputc ('-');
        wputc (protocol);
        wputc (' ');
        wputs (final_name);

        Infile=fopen(tmpname,"wb");
        if(Infile == NULL)
                return(NULL);
        if(isatty(fileno(Infile)))
        {
                fclose(Infile);
                unlink (tmpname);
                return(NULL);
        }

        throughput(0,0L);

        if(!may_be_seadog)
                SendNAK();

        t1 = timerset(300);
        real_errs = 0;

loop_top:
        in_char = TIMED_READ(4);

        switch(in_char) {
        case SOH :
                block_size=128;
                getblock();
                t1 = timerset(300);
                break;
        case STX :
                block_size=1024;
                getblock();
                t1 = timerset(300);
                break;
        case SYN :
                do_chksum = 1;
                getblock();
                do_chksum = 0;
                t1 = timerset(300);
                break;
        case CAN :
                if(TIMED_READ(2) == CAN) {
                        goto fubar;
                }
                t1 = timerset(300);
                break;
        case EOT :
                t1 = timerset (20);
                while(!timeup (t1)) {
                        TIMED_READ(1);
                        time_release();
                }
                if(block_number || protocol == 'S')
                        goto done;
                else
                        goto fubar;
        default  :
                if(!CARRIER)
                        return (NULL);

                if(timeup(t1)) {
                        SendNAK();
                        t1 = timerset(300);
                }
                break;
        }

        if(errs > 14) {
                goto fubar;
        }

        goto loop_top;

fubar:
        if(Infile)
        {
                fclose(Infile);
                unlink(tmpname);
        }

        CLEAR_OUTBOUND();

        send_can();
        status_line("!File not received");

        CLEAR_INBOUND();
        return(NULL);

done:
        recv_ackless = 0;
        SendACK();

        if(Infile) {
                int j, k;
                struct ffblk dta;

                fclose(Infile);

                if(!block_number && protocol == 'S')
                        return (NULL);

                i = strlen(tmpname)-1;
                j = strlen(final_name)-1;

                if(tmpname[i]=='.')
                        tmpname[i]='\0';
                if(final_name[j]=='.') {
                        final_name[j]='\0';
                        j--;
                }

                i = 0;
                k = is_arcmail(final_name, j);
                if ((!overwrite) || k) {
                        while(rename(tmpname,final_name)) {
                                if (isdigit(final_name[j]))
                                        final_name[j]++;
                                else
                                        final_name[j]='0';
                                if (!isdigit(final_name[j]))
                                        return(final_name);
                                i = 1;
                        }
                }
                else {
                        unlink(final_name);
                        rename(tmpname,final_name);
                }
                if(i)
                        status_line("+Dupe file renamed: %s",final_name);

                if(!findfirst(final_name,&dta,0)) {
                        if(real_errs > 4)
                                status_line ("+Corrected %d errors in %d blocks",real_errs, fsize1);
                        throughput(1,dta.ff_fsize);
                        status_line("+UL-%c %s",protocol,strupr(final_name));

                        if (wh != -1) {
                                wactiv(wh);
                                wclose();
                                wunlink(wh);
                                wh = -1;
                        }

                        strcpy(final_name,dta.ff_name);
                        return(final_name);
                }
        }

        if (wh != -1)
        {
                wactiv(wh);
                wclose();
                wunlink(wh);
                wh = -1;
        }

        return(NULL);
}

int send(fname,protocol)
char *fname, protocol;
{
        register int i, j;
        char *b, rbuffer[1024], temps[80];
        int in_char, in_char1, base, head;
        int win_size, full_window, ackerr, wh;
        FILE *fp;
        long block_timer, ackblock, blknum, last_block, temp;
        struct stat st_stat;
        struct ffblk dta;
        struct zero_block1 *testa;

        if(protocol == 'F') {
                protocol = 'S';
                may_be_seadog = 1;
        }
        else if(protocol == 'B') {
                protocol = 'S';
                may_be_seadog = 1;
        }
        else
                may_be_seadog = 0;

        fp = NULL;
        sliding = 0;
        ackblock = -1;
        do_chksum = 0;
        errs = 0;
        ackerr = 0;
        real_errs = 0;
        wh = -1;

        full_window = rate/400;

        if (small_window && full_window > 6)
                full_window = 6;

        _BRK_DISABLE ();
        XON_DISABLE();

        if ((!fname) || (!fname[0])) {

                for(i=0; i<5; i++) {
                        switch(in_char = TIMED_READ(5)) {
                        case 'C':
                        case NAK:
                        case CAN:
                                SENDBYTE(EOT);
                                break;
                        case TSYNC:
                                return(TSYNC);
                        default:
                                if (in_char < ' ')
                                        return(0);
                        }
                }
                return(0);
        }

        strlwr(fname);

        if(!findfirst(fname,&dta,0))
                fp = fopen(fname,"rb");
        else {
                send_can();
                return(0);
        }
        if(isatty(fileno(fp))) {
                fclose(fp);
                return(0);
        }

        testa = (struct zero_block1 *)rbuffer;

        block_size = 128;
        head = SOH;
        switch(protocol) {
        case 'Y' :
                base=1;
                block_size=1024;
                head=STX;
                break;
        case 'X' :
                base=1;
                break;
        case 'S' :
                base=0;
                break;
        case 'T' :
                base=0;
                head=SYN;
                break;
        case 'M' :
                base=1;
                break;
        default  :
                fclose(fp);
                return(0);
        }

        blknum = base;
        last_block = (long )((dta.ff_fsize+((long )block_size-1L))/(long)block_size);
        status_line(msgtxt[M_SEND_MSG],fname,last_block,protocol);
        block_timer = timerset(300);

        wh = wopen(16,0,19,79,1,WHITE|_RED,WHITE|_RED);
        wactiv(wh);
        wtitle(" Transfer Status ", TLEFT, WHITE|_RED);

        wgotoxy (0, 1);
        sprintf (temps, msgtxt[M_SEND_MSG], fname, last_block, protocol);
        wputs (temps);

        if (may_be_seadog)
                goto sendloop;

        do {
                do_chksum = 0;
                i = TIMED_READ(9);
                switch(i) {
                case NAK:
                        do_chksum = 1;
                case 'C' :
                        {
                                int send_tmp;

                                if(((send_tmp = TIMED_READ(4)) >= 0) && (TIMED_READ(2) == ((~send_tmp) & 0xff))) {
                                        if (send_tmp <= 1)
                                                sliding=1;
                                        else {
                                                SENDBYTE(EOT);
                                                continue;
                                        }
                                }
                                if(may_be_seadog)
                                        sliding = 1;
                                errs = 0;
                                CLEAR_INBOUND();
                                throughput(0,0L);
                                goto sendloop;
                        }
                case CAN :
                        goto fubar;
                default  :
                        if((errs++) > 15) {
                                goto fubar;
                        }
                        block_timer = timerset(50);
                        while(!timeup(block_timer))
                                time_release();
                        CLEAR_INBOUND();
                }
        } while(CARRIER);

        goto fubar;

sendloop:
        while(CARRIER) {
                win_size = (blknum < 2) ? 2 : (send_ackless ? 220 : full_window);

                if(blknum <= last_block) {
                        memset(rbuffer,0,block_size);
                        if(blknum) {
                                if(fseek(fp,((long)(blknum-1)*(long)block_size),SEEK_SET) == -1) {
                                        goto fubar;
                                }
                                fread(rbuffer,block_size,1,fp);
                        }
                        else {
                                block_size = 128;
                                testa->size = dta.ff_fsize;
                                stat(fname,&st_stat);
                                testa->time = st_stat.st_atime;

                                strcpy(testa->name,dta.ff_name);

                                if(protocol=='T') {
                                        for(i=0;i<HEADER_NAMESIZE;i++)
                                                if (!(testa->name[i]))
                                                        testa->name[i] = ' ';
                                        testa->time = dta.ff_ftime;
                                }

                                strcpy(testa->moi,VERSION);
                                if ((rate >= 9600) && (sliding)) {
                                        testa->noacks = 1;
                                        send_ackless = 1;
                                }
                                else {
                                        testa->noacks = 0;
                                        send_ackless = 0;
                                }

                                if(no_overdrive) {
                                        testa->noacks = 0;
                                        send_ackless = 0;
                                }

                                ackless_ok = 0;
                        }

                        SENDBYTE((unsigned char )head);
                        SENDBYTE((unsigned char )(blknum & 0xFF));
                        SENDBYTE((unsigned char )(~(blknum & 0xFF)));

                        for(b=rbuffer,i=0;i<block_size;i++,b++)
                                SENDBYTE(*b);

                        if((do_chksum) || (head==SYN)) {
                                unsigned char chksum = '\0';
                                for(b=rbuffer,i=0;i<block_size;i++,b++)
                                        chksum+=(*b);
                                SENDBYTE(chksum);
                        }
                        else {
                                word crc;

                                for(b=rbuffer,crc=i=0;i<block_size;i++,b++)
                                        crc = xcrc(crc,(byte )(*b));

                                SENDBYTE((unsigned char )(crc>>8));
                                SENDBYTE((unsigned char )(crc & 0xff));
                        }
                }

                block_timer = timerset(3000);

slide_reply:
                if (!sliding) {
                        FLUSH_OUTPUT();
                        block_timer = timerset(3000);
                }
                else if((blknum < (ackblock + win_size)) && (blknum < last_block) && (PEEKBYTE() < 0 )) {
                        if((send_ackless) && (blknum > 0)) {
                                ackblock = blknum;

                                if(blknum >= last_block) {
                                        if(ackless_ok) {
                                                sprintf (temps, "%5ld", last_block);
                                                wgotoxy (1, 1);
                                                wputs (temps);
                                                goto done;
                                        }

                                        blknum = last_block + 1;
                                        goto sendloop;
                                }

                                ++blknum;

				if(!(blknum & 0x001F))
                                {
                                        sprintf (temps, "%5ld", blknum);
                                        wgotoxy (1, 1);
                                        wputs (temps);
                                }
                        }
                        else
                                blknum++;
                        goto sendloop;
                }

                if (PEEKBYTE() < 0) {
                        if(send_ackless) {
                                ackblock = blknum;

                                if (blknum >= last_block) {
                                        if (ackless_ok) {
                                                sprintf (temps, "%5ld", last_block);
                                                wgotoxy (1, 1);
                                                wputs (temps);
                                                goto done;
                                        }

                                        blknum = last_block + 1;
                                        goto sendloop;
                                }

                                ++blknum;

                                if((blknum % 20) == 0)
                                {
                                        sprintf (temps, "%5ld", blknum);
                                        wgotoxy (1, 1);
                                        wputs (temps);
                                }

                                goto sendloop;
                        }
                }

reply:
                while(!OUT_EMPTY() && PEEKBYTE() < 0);
                        time_release();

                if((in_char = TIMED_READ(30)) < 0) {
                        goto fubar;
                }

                if(in_char == 'C') {
                        do_chksum = 0;
                        in_char = NAK;
                }

                if(in_char == CAN) {
                        goto fubar;
                }

                if((in_char > 0) && (sliding)) {
                        if(++ackerr >= 10) {
                                if(send_ackless)
                                        send_ackless = 0;
                        }

                        if((in_char == ACK) || (in_char == NAK)) {
                                if((i = TIMED_READ(2)) < 0) {
                                        sliding=0;
                                        if(send_ackless)
                                                send_ackless = 0;
                                }
                                else {
                                        if((j = TIMED_READ(2)) < 0) {
                                                sliding = 0;
                                                if(send_ackless)
                                                        send_ackless = 0;
                                        }
                                        else if(i == (j ^ 0xff)) {
                                                temp = blknum - ((blknum - i) & 0xff);
                                                if ((temp <= blknum) && (temp > (blknum - win_size - 10))) {
                                                        if(in_char == ACK) {
                                                                if ((head == SYN) && (blknum))
                                                                        head = SOH;
                                                                if (send_ackless) {
                                                                        ackless_ok = 1;
                                                                        goto slide_reply;
                                                                }
                                                                else
                                                                        ackblock = temp;

                                                                blknum++;

                                                                if(ackblock >= last_block)
                                                                        goto done;
                                                                errs = 0;
                                                        }
                                                        else {
                                                                blknum = temp;
                                                                CLEAR_OUTBOUND();
                                                                errs++;
                                                                real_errs++;
                                                        }
                                                }
                                        }
                                }
                        }
                        else {
                                if(timeup(block_timer) && OUT_EMPTY()) {
                                        goto fubar;
                                }
                                else if (!OUT_EMPTY())
                                        block_timer=timerset(3000);
                                goto slide_reply;
                        }
                }

                if(!sliding) {
                        if(in_char == ACK) {
                                if(blknum == 10)
                                        timer(3);
                                if (PEEKBYTE() > 0) {
                                        int send_tmp;

                                        if(((send_tmp = TIMED_READ(1)) >= 0) && (TIMED_READ(1) == ((~send_tmp) & 0xff))) {
                                                sliding = 1;
                                                ackblock = send_tmp;
                                        }
                                }

                                if(blknum >= last_block)
                                        goto done;
                                blknum++;

                                if((head == SYN) && (blknum))
                                        head = SOH;

                                errs = 0;
                        }
                        else if(in_char == NAK) {
                                errs++;
                                real_errs++;
                                timer(5);
                                CLEAR_OUTBOUND();
                        }
                        else if(CARRIER) {
                                if (!timeup(block_timer))
                                        goto reply;
                                else {
                                        goto fubar;
                                }
                        }
                        else {
                                goto fubar;
                        }
                }

                if(errs > 10) {
                        goto fubar;
                }

                temp = (blknum <= last_block) ? blknum : last_block;

                sprintf (temps, "%5ld", temp);
                wgotoxy (1, 1);
                wputs (temps);

		if((sliding) && (ackblock > 0))
			if(!send_ackless)
                        {
                                sprintf (temps, ":%-5ld", ackblock);
                                wputs (temps);
                        }
        }

fubar:
        CLEAR_OUTBOUND();
        send_can();
        status_line("!%s not sent",fname);

        if(fp)
                fclose(fp);

        if (wh != -1)
        {
                wactiv(wh);
                wclose();
                wunlink(wh);
                wh = -1;
        }

        return(0);

done:
        CLEAR_INBOUND();
        CLEAR_OUTBOUND();

        SENDBYTE(EOT);

        ackerr = 1;
        blknum = last_block + 1;

        for(i=0; i<5; i++) {
                if(!CARRIER) {
                        ackerr = 1;
                        goto gohome;
                }

                switch(TIMED_READ(5)) {
                case 'C':
                case NAK:
                case CAN:
                        if (sliding && ((in_char = TIMED_READ(1)) >= 0)) {
                                in_char1 = TIMED_READ(1);
                                if (in_char == (in_char1 ^ 0xff)) {
                                        blknum -= ((blknum - in_char) & 0xff);
                                        CLEAR_INBOUND();
                                        if (blknum <= last_block)
                                                goto sendloop;
                                }
                        }
                        CLEAR_INBOUND();
                        SENDBYTE(EOT);
                        break;
                case TSYNC:
                        ackerr = TSYNC;
                        goto gohome;
                case ACK:
                        if (sliding && ((in_char = TIMED_READ(1)) >= 0))
                                in_char1 = TIMED_READ(1);
                        ackerr = 1;
//                        SENDBYTE(EOT);
                        goto gohome;
                }
        }

gohome:
        if(fp)
                fclose(fp);

        throughput(1,dta.ff_fsize);

        if(real_errs > 4)
                status_line("+Corrected %d errors in %ld blocks", real_errs, last_block);
        status_line("+DL-%c %s",protocol,strupr(fname));

        if (wh != -1) {
                wactiv(wh);
                wclose();
                wunlink(wh);
                wh = -1;
        }

        return(ackerr);
}

void locate_files(only_one)
int only_one;
{
   FILE *fp;
   int line, m, z, fd, date;
   long size, totsize, totfile;
   char name[13], desc[46];
   char stringa[120], filename[80], parola[30];
   struct ffblk blk;
   struct _sys tsys;

   if (!get_command_word (parola, 29)) {
      m_print(bbstxt[B_KEY_SEARCH]);
      input(parola, 29);
      if(parola[0] == '\0' || !CARRIER)
         return;
   }

   cls();
   line = 4;
   totsize = totfile = 0L;

   _BRK_ENABLE ();

   m_print(bbstxt[B_CONTROLS]);
   m_print(bbstxt[B_CONTROLS2]);

   sprintf(filename,"%sSYSFILE.DAT", sys_path);
   fd = shopen(filename, O_RDONLY|O_BINARY);
   if (fd == -1) {
      status_line(msgtxt[M_NO_SYSTEM_FILE]);
      return;
   }

   while (read(fd, (char *)&tsys.file_name,SIZEOF_FILEAREA) == SIZEOF_FILEAREA && line) {
      if (only_one)
         memcpy ((char *)&tsys.file_name, (char *)&sys.file_name, SIZEOF_FILEAREA);
      else {
         if (usr.priv < tsys.file_priv || tsys.no_global_search)
            continue;
         if((usr.flags & tsys.file_flags) != tsys.file_flags)
            continue;
      }

      m_print(bbstxt[B_AREALIST_HEADER],tsys.file_num,tsys.file_name);
      if ((line=file_more_question(line,only_one)) == 0)
         break;

      if (!tsys.filelist[0]) {
         sprintf(filename,"%sFILES.BBS",tsys.filepath);
         fp = fopen(filename,"rt");
      }
      else
         fp = fopen (tsys.filelist, "rt");

      if (fp == NULL)
         continue;

      while(fgets(stringa,110,fp) != NULL && line) {
         stringa[strlen(stringa)-1] = 0;

         if (!CARRIER || RECVD_BREAK()) {
            line = 0;
            break;
         }

         if(!isalnum(stringa[0]))
            continue;

         m=0;
         while(stringa[m] != ' ' && m<12)
            name[m] = stringa[m++];

         name[m]='\0';

         sprintf(filename,"%s%s",tsys.filepath,name);
         if(findfirst(filename,&blk,0))
            continue;

         if(stristr (stringa, parola)) {
            size=blk.ff_fsize;
            date=blk.ff_fdate;
            z=0;
            while(stringa[m] == ' ')
               m++;
            while(stringa[m] && z<45)
               desc[z++]=stringa[m++];
            desc[z]='\0';

            totsize += size;
            totfile++;

            m_print(bbstxt[B_FILES_FORMAT],strupr(name),size,(date & 0x1F), ((date >> 5) & 0x0F),((((date >> 9) & 0x7F)+80)%100), desc);

            if (!(line=file_more_question(line,only_one)) || !CARRIER)
               break;
         }
      }

      fclose(fp);

      if (only_one)
         break;
   }

   close(fd);

   m_print (bbstxt[B_LOCATED_MATCH], totfile, totsize);

   if (line && CARRIER)
      press_enter();
}

void hurl ()
{
   #define MAX_HURL  4096
   FILE *fps, *fpd;
   struct _sys tempsys;
   int fds, fdd, m;
   char file[16], hurled[128], *b, filename[128];
   struct ftime ftimep;

   if (!get_command_word (file, 12)) {
      m_print (bbstxt[B_HURL_WHAT]);
      input (file, 12);
      if (!file[0])
         return;
   }

   sprintf (filename, "%s%s", sys.filepath, file);
   fds = shopen (filename, O_RDWR|O_BINARY);
   if(fds == -1)
      return;

   if (!get_command_word (hurled, 79)) {
      m_print (bbstxt[B_HURL_AREA]);
      input (hurled, 80);
      if (!hurled[0]) {
         close (fds);
         return;
      }
   }

   if ((m = atoi(hurled)) != 0) {
      if (!read_system2 (2, m, &tempsys)) {
         close (fds);
         return;
      }
   }
   else
      strcpy (tempsys.filepath, hurled);

   m_print (bbstxt[B_HURLING], sys.filepath, file, tempsys.filepath, file);

   sprintf (filename, "%s%s", tempsys.filepath, file);
   fdd = cshopen (filename, O_CREAT|O_TRUNC|O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
   if (fdd == -1) {
      close (fds);
      return;
   }

   b = (char *)malloc(MAX_HURL);
   if (b == NULL)
      return;

   do {
      m = read (fds, b, MAX_HURL);
      write (fdd, b, m);
   } while (m == MAX_HURL);

   getftime (fds, &ftimep);
   setftime (fdd, &ftimep);

   close (fdd);
   close (fds);

   free(b);

   sprintf (filename, "%s%s", sys.filepath, file);
   unlink (filename);

   sprintf (filename, "%sFILES.BBS", sys.filepath);
   sprintf (hurled, "%sFILES.BAK", sys.filepath);
   unlink (hurled);
   rename (filename, hurled);

   fpd = fopen (filename, "wt");
   fps = fopen (hurled, "rt");

   strupr (file);
   strcat (file," ");

   while (fgets (filename, 128, fps) != NULL) {
      if (!strncmp (filename, file, strlen (file))) {
         strcpy (hurled, filename);
         continue;
      }
      fputs (filename, fpd);
   }

   fclose(fps);
   fclose(fpd);

   sprintf(filename, "%sFILES.BBS", tempsys.filepath);
   fpd = fopen (filename,"at");
   fputs (hurled, fpd);
   fclose (fpd);
}

static int file_more_question (line, only_one)
int line, only_one;
{
   int i;

   if (nopause)
      return (1);

   if (!(line++ < (usr.len-1)) && usr.more) {
      line = 1;
      m_print(bbstxt[B_MORE]);
      if ( (usr.priv < sys.download_priv || sys.download_priv == HIDDEN) ||
           ((usr.flags & sys.download_flags) != sys.download_flags) )
         i = yesno_question(DEF_YES|EQUAL|NO_LF);
      else
         i = yesno_question(DEF_YES|EQUAL|NO_LF|TAG_FILES);

      m_print("\r");
      space (strlen (bbstxt[B_MORE]) + 15);
      m_print("\r");

      if (i == DEF_NO) {
         nopause = 0;
         return (0);
      }

      line = 1;

      if (i == EQUAL)
         nopause = 1;

      if (i == TAG_FILES) {
         nopause = 0;
         if (usr.priv < sys.download_priv || sys.download_priv == HIDDEN)
            return (line);
         if((usr.flags & sys.download_flags) != sys.download_flags)
            return (line);
         download_file (sys.filepath, only_one ? 0 : 1);
         if (user_status != BROWSING)
            set_useron_record(BROWSING, 0, 0);
      }
   }

   return (line);
}

void file_kill ()
{
   FILE *fps, *fpd;
   char filename[80], file[14], buffer[130];
   struct ffblk blk;

   if (!get_command_word (file, 12)) {
      m_print ("\nKill what file? ");
      input (file, 12);
      if (!file[0])
         return;
   }

   sprintf (filename,"%s%s", sys.filepath, file);

   if (!findfirst (filename, &blk, 0))
      do {
         sprintf (filename,"%s%s", sys.filepath, blk.ff_name);

         m_print ("Remove %s ", filename);
         if (yesno_question (DEF_NO) == DEF_YES) {
            sprintf (filename, "%sFILES.BBS", sys.filepath);
            sprintf (buffer, "%sFILES.BAK", sys.filepath);
            unlink (buffer);
            rename (filename, buffer);
            fpd = fopen (filename, "wt");
            fps = fopen (buffer, "rt");
            while (fgets (buffer, 128, fps) != NULL) {
               if (!strncmp (buffer, blk.ff_name, strlen(blk.ff_name)))
                  continue;
               fputs (buffer, fpd);
            }
            fclose (fps);
            fclose (fpd);
         }
      } while (!findnext (&blk));
}

