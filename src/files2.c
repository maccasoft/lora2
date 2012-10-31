#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dir.h>
#include <fcntl.h>
#include <share.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"
#include "arc.h"
#include "exec.h"

typedef unsigned int uint;

#define ts_year(ts)  ((uint)((ts >> 25) & 0x7f) + 1980)
#define ts_month(ts) ((uint)(ts >> 21) & 0x0f)	    /* 1..12 means Jan..Dec */
#define ts_day(ts)   ((uint)(ts >> 16) & 0x1f)	    /* 1..31 means 1st..31st */
#define ts_hour(ts)  ((uint)(ts >> 11) & 0x1f)
#define ts_min(ts)   ((uint)(ts >> 5) & 0x3f)
#define ts_sec(ts)   ((uint)((ts & 0x1f) * 2))

#define SEEK_SIZE  64

#define ARCHIVE_arc       0x00 /* An .ARC                                 */
#define ARCHIVE_lzh       0x01 /* A .LZH                                  */

static void Read_LzhArc(int ,int);
static int Zip_Scan_For_Header(char *);
static void Read_Zip(int);
static int Zip_Read_Directory(int, long, int);
static void Read_Arj(int);

char *memstr(char *, char *, int, int);

void display_contents()
{
   struct _lhhead *lhhead;

   byte buffer[SEEK_SIZE+10];
   char filename[14], an[80];

   int arcfile;

   if (!get_command_word (filename, 12)) {
      m_print (bbstxt[B_WHICH_ARCHIVE]);
      input (filename,12);
      if (!filename[0] || !CARRIER)
         return;
   }

   m_print(bbstxt[B_SEARCH_ARCHIVE],strupr(filename));

   sprintf (an, "%s%s", sys.filepath, filename);
   if ((arcfile=shopen(an,O_RDONLY | O_BINARY))==-1) {
      sprintf (an, "%s%s.ARJ", sys.filepath, filename);

      if ((arcfile=shopen(an,O_RDONLY | O_BINARY))==-1) {
         sprintf (an, "%s%s.ZIP", sys.filepath, filename);

         if ((arcfile=shopen(an,O_RDONLY | O_BINARY))==-1) {
            sprintf (an, "%s%s.LZH", sys.filepath, filename);

            if ((arcfile=shopen(an,O_RDONLY | O_BINARY))==-1) {
               sprintf (an, "%s%s.LHA", sys.filepath, filename);

               if ((arcfile=shopen(an,O_RDONLY | O_BINARY))==-1) {
                  sprintf (an, "%s%s.PAK", sys.filepath, filename);

                  if ((arcfile=shopen(an,O_RDONLY | O_BINARY))==-1) {
                     sprintf (an, "%s%s.ARC", sys.filepath, filename);

                     if ((arcfile=shopen(an,O_RDONLY | O_BINARY))==-1) {
                        m_print (bbstxt[B_ARC_NOTFOUND]);
                        return;
                     }
                  }
               }
            }
         }
      }
   }

   read(arcfile,buffer,SEEK_SIZE-1);
   lseek(arcfile,0L,SEEK_SET);

   lhhead=(struct _lhhead *)buffer;

   if (*buffer=='P' && *(buffer+1)=='K')
      Read_Zip(arcfile);
   else if (lhhead->method[0]=='-' && lhhead->method[1]=='l' &&
           (lhhead->method[2]=='z' || lhhead->method[2]=='h') &&
            isdigit(lhhead->method[3]) && lhhead->method[4]=='-')
      Read_LzhArc(ARCHIVE_lzh,arcfile);
   else if (*buffer=='\x1a')
      Read_LzhArc(ARCHIVE_arc,arcfile);
   else if (*buffer==0x60 && *(buffer+1)==0xEA)
      Read_Arj(arcfile);
   else
      m_print (bbstxt[B_ARC_DAMAGED], filename);

   close(arcfile);
   return;
}



/* Thanks go to Andrew Farmer for the algorithm and _lhhead structure for   *
 * this routine!                                                            */

static void Read_LzhArc(int type,int lzhfile)
{
  struct _lhhead lhhead;
  struct _archead archead;

  char *fname;

  int crc,
      line,
      num_files;

  long total_compressed,
       total_uncompressed;

  num_files=0;
  total_compressed=total_uncompressed=0L;
  line = 4;

  m_print(bbstxt[B_LZHARC_HEADER]);
  m_print(bbstxt[B_LZHARC_UNDERLINE]);

  while (! eof(lzhfile))
  {
    if (!CARRIER)
      return;

    if (type==ARCHIVE_lzh)
    {
      read(lzhfile,(char *)&lhhead,sizeof(struct _lhhead));

      if (!lhhead.bytetotal)
        break;
    }
    else read(lzhfile,(char *)&archead,sizeof(struct _archead));

    if (!eof(lzhfile))
    {
      if (type==ARCHIVE_lzh)
      {
        if ((fname=(char *)malloc(lhhead.namelen+1))==NULL)
          return;

        read(lzhfile,fname,lhhead.namelen);
        fname[lhhead.namelen]='\0';

        read(lzhfile,(char *)&crc,sizeof(crc));

        if (!lhhead.uncompressed_size)
          lhhead.uncompressed_size=1L;

        m_print (bbstxt[B_LZHARC_FORMAT],
             fname,
             lhhead.uncompressed_size,
             lhhead.compressed_size,
             (int)(100-((lhhead.compressed_size*100)/lhhead.uncompressed_size)),
             ts_day (lhhead.stamp),
             ts_month (lhhead.stamp),
             ts_year (lhhead.stamp) % 100,
             ts_hour (lhhead.stamp),
             ts_min (lhhead.stamp));

        free(fname);

        total_compressed += lhhead.compressed_size;
        total_uncompressed += lhhead.uncompressed_size;

        lseek(lzhfile,lhhead.compressed_size,SEEK_CUR);
      }
      else
      {
        if (!archead.uncompressed_size)
          archead.uncompressed_size=1L;

        m_print (bbstxt[B_LZHARC_FORMAT],
             archead.name,
             archead.uncompressed_size,
             archead.compressed_size,
             (int)(100-((archead.compressed_size*100)/archead.uncompressed_size)),
             ts_day (archead.stamp),
             ts_month (archead.stamp),
             ts_year (archead.stamp) % 100,
             ts_hour (archead.stamp),
             ts_min (archead.stamp));

        total_compressed += archead.compressed_size;
        total_uncompressed += archead.uncompressed_size;

        lseek(lzhfile,archead.compressed_size,SEEK_CUR);
      }

      num_files++;

      if ((line=more_question(line)) == 0 || !CARRIER)
        break;
    }
  }

  if (line)
  {
    m_print(bbstxt[B_LZHARC_END]);

     if (total_uncompressed)
        m_print(bbstxt[B_LZHARC_END_FORMAT],
           total_uncompressed,
           total_compressed,
           (int)(100-((total_compressed*100)/total_uncompressed)));

     press_enter ();
  }
}

static int Zip_Scan_For_Header(char *buffer)
{
  char *x;

  if ((x=memstr(buffer,END_STRSIG,SEEK_SIZE,strlen(END_STRSIG))) != NULL)
    return (int)(x-buffer);
  else return 0;
}

static void Read_Zip(int zipfile)
{
  char buffer[SEEK_SIZE+10];
  int found_header;
  long zip_pos;

  if (lseek(zipfile,0L,SEEK_END)==-1L)
  {
    close(zipfile);
    return;
  }

  zip_pos=tell(zipfile);
  found_header=FALSE;

  while ((zip_pos >= 0) && (! found_header))
  {
    if (zip_pos==0)
      zip_pos=-1L;
    else zip_pos -= min((SEEK_SIZE-4L),zip_pos);

    lseek(zipfile,zip_pos,SEEK_SET);

    if (zip_pos >= 0)
    {
      if (read(zipfile,buffer,SEEK_SIZE)==-1)
        return;

      found_header=Zip_Scan_For_Header(buffer);
    }
  }

  if (found_header==FALSE)  /* Couldn't find central dir. header! */
    return;

  if (Zip_Read_Directory(zipfile,zip_pos,found_header)==-1)
    return;

  return;
}

static int Zip_Read_Directory(int zipfile,long zip_pos,int offset)
{
  char temp[5],
       *filename,
       *file_comment;

  long total_uncompressed,
       total_compressed;

  unsigned long stamp;

  int num_files,
      line;

  struct _dir_end dir_end;
  struct _dir_header dir_head;

  line = 4;

  /* Seek to the End_Of_Central_Directory record, which we found the
     location of earlier.                                             */

  lseek(zipfile,(long)((long)zip_pos+(long)offset+4L),SEEK_SET);
  read(zipfile,(char *)&dir_end.info,sizeof(dir_end.info));

  /* We don't support multi-disk ZIPfiles */
  if (dir_end.info.this_disk != 0)
    return -1;

  /* If there is a ZIPfile comment */
  if (dir_end.info.zipfile_comment_length)
  {
    if ((file_comment=(char *)malloc(dir_end.info.zipfile_comment_length+1)) != NULL)
    {
      read(zipfile,file_comment,dir_end.info.zipfile_comment_length);
      file_comment[dir_end.info.zipfile_comment_length]='\0';
      free(file_comment);
    }
  }

  lseek(zipfile,dir_end.info.offset_start_dir,SEEK_SET);

  total_uncompressed=total_compressed=0;
  num_files=0;

  m_print(bbstxt[B_LZHARC_HEADER]);
  m_print(bbstxt[B_LZHARC_UNDERLINE]);

  while (1)
  {
    read(zipfile,temp,4);

    /* If we are at a directory entry */
    if (memcmp(temp,DIR_STRSIG,4)==0)
    {
      if (!CARRIER)
         return -1;

      /* Read in this entry's information */
      read(zipfile,(char *)&dir_head.info,sizeof(dir_head.info));

      /* Allocate memory for filename and comments... */
      if ((filename=(char *)malloc(dir_head.info.filename_length+1))==NULL)
        return -1;

      if (dir_head.info.file_comment_length)
      {
        if ((file_comment=(char *)malloc(dir_head.info.file_comment_length+1))==NULL)
        {
          /* It isn't critical if we can't allocate the memory for the
             file comment, so just zero out the field and continue.    */

          dir_head.info.file_comment_length=0;
        }
      }

      /* Read in the filename */
      read(zipfile,filename,dir_head.info.filename_length);
      filename[dir_head.info.filename_length]='\0';

      /* Skip over the extra field */
      lseek(zipfile,(long)dir_head.info.xtra_field_length,SEEK_CUR);


      /* Read in the file comment */
      if (dir_head.info.file_comment_length)
      {
        read(zipfile,file_comment,dir_head.info.file_comment_length);
        file_comment[dir_head.info.file_comment_length]='\0';
      }

      /* Grab the file's datestamp */
      stamp=dir_head.info.last_mod;

      if (!dir_head.info.uncompressed_size)
        dir_head.info.uncompressed_size=1L;

      m_print (bbstxt[B_LZHARC_FORMAT],
             filename,
             dir_head.info.uncompressed_size,
             dir_head.info.compressed_size,
             (int)(100-((dir_head.info.compressed_size*100)/dir_head.info.uncompressed_size)),
             ts_day (stamp),
             ts_month (stamp),
             ts_year (stamp) % 100,
             ts_hour (stamp),
             ts_min (stamp));

      /* Add this to the totals... */
      total_uncompressed += dir_head.info.uncompressed_size;
      total_compressed += dir_head.info.compressed_size;

      num_files++;

      /* Release the memory allocated for the filename, and possibly the
         file comment.                                                   */

      free(filename);

      if (dir_head.info.file_comment_length)
        free(file_comment);

      if ((line=more_question(line)) == 0 || !CARRIER)
        break;
    }
    else if (memcmp(temp,END_STRSIG,4)==0)
    {
      /* Found end of directory record, so we do post-processing here,
         and exit.                                                    */

      if (total_uncompressed)
      {
         m_print(bbstxt[B_LZHARC_END]);
         m_print(bbstxt[B_LZHARC_END_FORMAT],
               total_uncompressed,total_compressed,
               (int)(100-((total_compressed*100)/total_uncompressed)));
      }

      press_enter ();
      return 0;
    }
    else return -1; /* Else found gibberish! */
  }

  /* We must have gotten here by a 'N' answer to a "More [Y,n,=]?" prompt */
  return 0;
}

char *memstr(char *string,char *search,int lenstring,int strlen_search)
{
  int x,
      last_found;

  for (x=last_found=0;x < lenstring;x++)
  {
    if (string[x]==search[last_found])
      last_found++;
    else
    {
      if (last_found != 0)
      {
        last_found=0;
        x--;
        continue;
      }
    }

    if (last_found==strlen_search)
      return ((string+x)-strlen_search)+1;
  }

  return(NULL);
}

struct _arj_basic_header {
   word id;
   word size;
};

struct _arj_header {
   byte size;
   byte version;
   byte min_to_extract;
   byte host_os;
   byte arj_flags;
   byte method;
   byte file_type;
   byte reserved;
   unsigned long stamp;
   long compressed_size;
   long uncompressed_size;
   long file_crc;
   word entryname;
   word file_access;
   word host_data;
};

static void Read_Arj(int lzhfile)
{
   struct _arj_basic_header bhead;
   struct _arj_header arjhead;
   char *fname;
   int line, num_files, extended;
   long total_compressed, pos, total_uncompressed;

   num_files=0;
   total_compressed=total_uncompressed=0L;
   line = 4;

   m_print(bbstxt[B_LZHARC_HEADER]);
   m_print(bbstxt[B_LZHARC_UNDERLINE]);

   while (! eof(lzhfile)) {
      if (!CARRIER)
         return;

      pos = tell (lzhfile);
      read(lzhfile,(char *)&bhead,sizeof(struct _arj_basic_header));

      if (bhead.id != 0xEA60U || !bhead.size)
         break;

      if (!eof(lzhfile)) {
         read(lzhfile,(char *)&arjhead,sizeof(struct _arj_header));
         if ((fname=(char *)malloc(20))==NULL)
            return;

         if (arjhead.file_type == 0 || arjhead.file_type == 1 || arjhead.file_type == 2) {
            lseek(lzhfile,pos,SEEK_SET);
            lseek (lzhfile, (long)arjhead.size + (long)arjhead.entryname + (long)sizeof (struct _arj_basic_header), SEEK_CUR);
            read(lzhfile,fname,14);

            if (arjhead.file_type != 2) {
               if (!arjhead.uncompressed_size)
                  arjhead.uncompressed_size=1L;

               m_print (bbstxt[B_LZHARC_FORMAT],
                   fname,
                   arjhead.uncompressed_size,
                   arjhead.compressed_size,
                   (int)(100-((arjhead.compressed_size*100)/arjhead.uncompressed_size)),
                   ts_day (arjhead.stamp),
                   ts_month (arjhead.stamp),
                   ts_year (arjhead.stamp) % 100,
                   ts_hour (arjhead.stamp),
                   ts_min (arjhead.stamp));

               total_compressed += arjhead.compressed_size;
               total_uncompressed += arjhead.uncompressed_size;

               num_files++;
            }
         }

         free(fname);

         lseek(lzhfile,pos,SEEK_SET);
         lseek(lzhfile,bhead.size + 8,SEEK_CUR);
         read(lzhfile,(char *)&extended,2);
         if (extended > 0)
            lseek(lzhfile,extended + 4,SEEK_CUR);
         if (arjhead.file_type != 2)
            lseek(lzhfile,arjhead.compressed_size,SEEK_CUR);
      }

      if ((line=more_question(line)) == 0 || !CARRIER)
         break;
   }

   if (line) {
      m_print(bbstxt[B_LZHARC_END]);

      if (total_uncompressed)
          m_print(bbstxt[B_LZHARC_END_FORMAT],
             total_uncompressed,
             total_compressed,
             (int)(100-((total_compressed*100)/total_uncompressed)));

      press_enter ();
   }
}

void hslink_protocol (name, path, global)
char *name, *path;
int global;
{
   FILE *fp, *fp2;
   int fd, *varr;
   char dir[20], filename[150], *p, ext[5], stringa[60];
   long started, bytes;
   struct ffblk blk;
   struct _sys tsys;

   sprintf (filename, "%sHSLINK%d.LST", sys_path, line_offset);
   fp = fopen (filename, "wt");

   if (name != NULL) {
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

         fprintf (fp, "%s\n", filename);
      }
   }

   fclose (fp);
   sprintf (filename, "-P%d -U%s -LF%sHSLINK%d.LOG @%sHSLINK%d.LST", com_port + 1, sys.uppath, sys_path, line_offset, sys_path, line_offset);

   varr = ssave ();
//   MDM_DISABLE ();
   cclrscrn(LGREY|_BLACK);
   showcur();
   fclose (logf);

   do_exec (hslink_exe == NULL ? "HSLINK" : hslink_exe, filename, USE_ALL|HIDE_FILE, 0xFFFF, NULL);

/*
   com_install (com_port);
   com_baud (rate);
*/
   if (varr != NULL)
      srestore (varr);
   logf = fopen(log_name, "at");
   setbuf(logf, NULL);
   wactiv (mainview);

   sprintf (filename, "%sHSLINK%d.LST", sys_path, line_offset);
   unlink (filename);

   sprintf (filename, "%sHSLINK%d.LOG", sys_path, line_offset);
   fp = fopen (filename, "rt");
   if (fp == NULL) {
      m_print(bbstxt[B_TRANSFER_ABORT]);
      return;
   }

   while (fgets (filename, 145, fp) != NULL) {
      while (filename[strlen (filename) -1] == 0x0D || filename[strlen (filename) -1] == 0x0A)
         filename[strlen (filename) -1] = '\0';

      p = strtok (filename, " ");
      if (p[0] == 'H') {
         p = strtok (NULL, " ");
         bytes = atol (p);
         usr.upld += (int)(bytes / 1024L) + 1;
         usr.n_upld++;
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         cps = atoi (p);
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");

         started = (cps * 1000L) / ((long) rate);
         status_line ((char *) msgtxt[M_CPS_MESSAGE], cps, bytes, started);
         status_line ("%s-H %s%s", msgtxt[M_FILE_RECEIVED], (strchr (p, '\\') == NULL) ? sys.uppath : "", strupr (p));

         fnsplit (p, NULL, NULL, dir, ext);
         strcat (dir, ext);

         do {
            m_print(bbstxt[B_DESCRIBE],strupr(dir));
            CLEAR_INBOUND ();
            input(stringa,54);
            if (!CARRIER)
               break;
         } while (strlen(stringa) < 5);

         sprintf(filename,"%sFILES.BBS",sys.uppath);
         fp2 = fopen(filename,"at");
         fprintf(fp2,"%-12s %s\n",dir,stringa);
         fclose(fp2);

         if (function_active == 3)
            f3_status ();
      }
      else if (p[0] == 'h' || p[0] == 'l' || p[0] == 'e') {
         p = strtok (NULL, " ");
         bytes = atol (p);
         usr.dnld += (int)(bytes / 1024L) + 1;
         usr.dnldl += (int)(bytes / 1024L) + 1;
         usr.n_dnld++;
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         cps = atoi (p);
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");

         started = (cps * 1000L) / ((long) rate);
         status_line ((char *) msgtxt[M_CPS_MESSAGE], cps, bytes, started);
         status_line ("+DL-H %s%s", (strchr (p, '\\') == NULL) ? sys.filepath : "", strupr (p));

         if (function_active == 3)
            f3_status ();
      }
   }

   fclose (fp);
   sprintf (filename, "%sHSLINK%d.LOG", sys_path, line_offset);
   unlink (filename);

   m_print(bbstxt[B_TRANSFER_OK]);
}

void puma_protocol (name, path, global, upload)
char *name, *path;
int global, upload;
{
   FILE *fp, *fp2;
   int fd, *varr;
   char dir[20], filename[150], *p, ext[5], stringa[60];
   long started, bytes;
   struct ffblk blk;
   struct _sys tsys;

   if (name != NULL) {
      sprintf (filename, "%sPUMA%d.LST", sys_path, line_offset);
      fp = fopen (filename, "wt");

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

         fprintf (fp, "%s\n", filename);
      }

      fclose (fp);
   }

   if (upload)
      sprintf (filename, "P%d L%sPUMA%d.LOG R %s", com_port + 1, sys_path, line_offset, sys.uppath);
   else
      sprintf (filename, "P%d L%sPUMA%d.LOG S @%sPUMA%d.LST", com_port + 1, sys_path, line_offset, sys_path, line_offset);

   varr = ssave ();
//   MDM_DISABLE ();
   cclrscrn(LGREY|_BLACK);
   showcur();
   fclose (logf);

   do_exec (puma_exe == NULL ? "PUMA" : puma_exe, filename, USE_ALL|HIDE_FILE, 0xFFFF, NULL);

/*
   com_install (com_port);
   com_baud (rate);
*/
   if (varr != NULL)
      srestore (varr);

   logf = fopen(log_name, "at");
   setbuf(logf, NULL);
   wactiv (mainview);

   sprintf (filename, "%sPUMA%d.LST", sys_path, line_offset);
   unlink (filename);

   sprintf (filename, "%sPUMA%d.LOG", sys_path, line_offset);
   fp = fopen (filename, "rt");
   if (fp == NULL) {
      m_print(bbstxt[B_TRANSFER_ABORT]);
      return;
   }

   while (fgets (filename, 145, fp) != NULL) {
      while (filename[strlen (filename) -1] == 0x0D || filename[strlen (filename) -1] == 0x0A)
         filename[strlen (filename) -1] = '\0';

      p = strtok (filename, " ");
      if (p[0] == 'R') {
         p = strtok (NULL, " ");
         bytes = atol (p);
         usr.upld += (int)(bytes / 1024L) + 1;
         usr.n_upld++;
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         cps = atoi (p);
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");

         started = (cps * 1000L) / ((long) rate);
         status_line ((char *) msgtxt[M_CPS_MESSAGE], cps, bytes, started);
         status_line ("%s-P %s%s", msgtxt[M_FILE_RECEIVED], (strchr (p, '\\') == NULL) ? sys.uppath : "", strupr (p));

         fnsplit (p, NULL, NULL, dir, ext);
         strcat (dir, ext);

         do {
            m_print(bbstxt[B_DESCRIBE],strupr(dir));
            CLEAR_INBOUND ();
            input(stringa,54);
            if (!CARRIER)
               break;
         } while (strlen(stringa) < 5);

         sprintf(filename,"%sFILES.BBS",sys.uppath);
         fp2 = fopen(filename,"at");
         fprintf(fp2,"%-12s %s\n",dir,stringa);
         fclose(fp2);

         if (function_active == 3)
            f3_status ();
      }
      else if (p[0] == 'S') {
         p = strtok (NULL, " ");
         bytes = atol (p);
         usr.dnld += (int)(bytes / 1024L) + 1;
         usr.dnldl += (int)(bytes / 1024L) + 1;
         usr.n_dnld++;
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         cps = atoi (p);
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");

         started = (cps * 1000L) / ((long) rate);
         status_line ((char *) msgtxt[M_CPS_MESSAGE], cps, bytes, started);
         status_line ("+DL-P %s%s", (strchr (p, '\\') == NULL) ? sys.filepath : "", strupr (p));

         if (function_active == 3)
            f3_status ();
      }
   }

   fclose (fp);
   sprintf (filename, "%sPUMA%d.LOG", sys_path, line_offset);
   unlink (filename);

   m_print(bbstxt[B_TRANSFER_OK]);
}

