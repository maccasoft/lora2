#include <alloc.h>
#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dir.h>
#include <fcntl.h>
#include <share.h>
#include <process.h>
#include <time.h>
#include <dos.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"
#include "arc.h"
#include "exec.h"

extern char no_description, no_check, no_external;

void open_logfile (void);
int spawn_program (int swapout, char *outstring);
void check_uploads (FILE *xferinfo, char *path);
void filebox_list (char *);
int file_more_question (int, int);
void download_report (char *rqname, int id, char *filelist);
void stripcrlf (char *linea);
void update_filestat (char *rqname);
void check_virus (char *rqname);
int m_getch (void);
int check_file_description (char *name, char *uppath);

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
static void Read_Sqz(int);

char *memstr(char *, char *, int, int);

static char *my_strtok (char *s1, char *s2)
{
    static char *Ss;
    char *tok, *sp;

    if (s1 != NULL)
       Ss = s1;

    while (*Ss) {
        for (sp = s2; *sp; sp++)
            if (*sp == *Ss)
                break;
        if (*sp == 0)
            break;
        Ss++;
    }
    if (*Ss == 0)
        return (NULL);
    tok = Ss;
    while (*Ss) {
        for (sp = s2; *sp; sp++)
            if (*sp == *Ss) {
                *Ss++ = '\0';
                return (tok);
            }
        Ss++;
    }
    return (tok);
}

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

   sprintf (an, "%s%s", sys.filepath, filename);
   if ((arcfile=shopen(an,O_RDONLY | O_BINARY))==-1) {
      strcat (filename, ".ARJ");
      sprintf (an, "%s%s", sys.filepath, filename);

      if ((arcfile=shopen(an,O_RDONLY | O_BINARY))==-1) {
         strcpy (&filename[strlen (filename) - 4], ".ZIP");
         sprintf (an, "%s%s", sys.filepath, filename);

         if ((arcfile=shopen(an,O_RDONLY | O_BINARY))==-1) {
            strcpy (&filename[strlen (filename) - 4], ".LZH");
            sprintf (an, "%s%s", sys.filepath, filename);

            if ((arcfile=shopen(an,O_RDONLY | O_BINARY))==-1) {
               strcpy (&filename[strlen (filename) - 4], ".LHA");
               sprintf (an, "%s%s", sys.filepath, filename);

               if ((arcfile=shopen(an,O_RDONLY | O_BINARY))==-1) {
                  strcpy (&filename[strlen (filename) - 4], ".PAK");
                  sprintf (an, "%s%s", sys.filepath, filename);

                  if ((arcfile=shopen(an,O_RDONLY | O_BINARY))==-1) {
                     strcpy (&filename[strlen (filename) - 4], ".ARC");
                     sprintf (an, "%s%s", sys.filepath, filename);

                     if ((arcfile=shopen(an,O_RDONLY | O_BINARY))==-1) {
                        strcpy (&filename[strlen (filename) - 4], ".SQZ");
                        sprintf (an, "%s%s", sys.filepath, filename);

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
   }

   m_print(bbstxt[B_SEARCH_ARCHIVE],strupr(filename));

   read(arcfile,buffer,SEEK_SIZE-1);
   lseek(arcfile,0L,SEEK_SET);

   lhhead=(struct _lhhead *)buffer;

   if (*buffer=='P' && *(buffer+1)=='K')
      Read_Zip(arcfile);
   else if (*buffer=='H' && *(buffer+1)=='L' && *(buffer+2)=='S' && *(buffer+3)=='Q' && *(buffer+4)=='Z')
      Read_Sqz(arcfile);
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

struct _sqz_arc_header {
   byte id[5];
   byte version;
   byte os;
   byte flag;
};

struct _sqz_file_header {
   byte size;
   byte checksum;
   byte method;
   long compr_size;
   long orig_size;
   long datetime;
   byte attrib;
   long crc;
};

static void Read_Sqz (int lzhfile)
{
   struct _sqz_file_header fhead;
   char fname[80];
   int line, num_files, size;
   long total_compressed, total_uncompressed;

   num_files=0;
   total_compressed=total_uncompressed=0L;
   line = 4;

   m_print(bbstxt[B_LZHARC_HEADER]);
   m_print(bbstxt[B_LZHARC_UNDERLINE]);

   lseek (lzhfile, (long)sizeof (struct _sqz_arc_header), SEEK_SET);

   while (! eof(lzhfile)) {
      if (!CARRIER)
         return;

      read (lzhfile, (char *)&fhead, 1);

      if (fhead.size > 18) {
         read (lzhfile, (char *)&fhead.checksum, 17);
         read (lzhfile, fname, fhead.size - 18);

         m_print (bbstxt[B_LZHARC_FORMAT],
             fname,
             fhead.orig_size,
             fhead.compr_size,
             (int)(100-((fhead.compr_size*100)/fhead.orig_size)),
             ts_day (fhead.datetime),
             ts_month (fhead.datetime),
             ts_year (fhead.datetime) % 100,
             ts_hour (fhead.datetime),
             ts_min (fhead.datetime));

         total_compressed += fhead.compr_size;
         total_uncompressed += fhead.orig_size;

         num_files++;

         if ((line=more_question(line)) == 0 || !CARRIER)
            break;
      }

      else if (fhead.size == 0)
         break;

      else {
         read (lzhfile, (char *)&size, 2);
         lseek (lzhfile, (long)size, SEEK_CUR);
      }
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
   char dir[20], filename[150], *p, ext[5], stringa[60], isupld;
   long started, bytes;
   struct ffblk blk;
   struct _sys tsys;

   sprintf (filename, "%sHSLINK%d.LST", config->sys_path, line_offset);
   fp = fopen (filename, "wt");

   if (name != NULL) {
      while (get_string (name, dir) != NULL) {
         sprintf (filename, "%s%s", path, dir);

         if (findfirst(filename,&blk,0)) {
            sprintf (filename, "%sSYSFILE.DAT", config->sys_path);
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
   sprintf (filename, "HSLINK -P%d -U%s -LF%sHSLINK%d.LOG @%sHSLINK%d.LST", com_port + 1, sys.uppath, config->sys_path, line_offset, config->sys_path, line_offset);

   varr = ssave ();
//   MDM_DISABLE ();
   cclrscrn(LGREY|_BLACK);
   showcur();
   fclose (logf);

   spawn_program (registered, filename);

/*
   com_install (com_port);
   com_baud (rate);
*/
   if (varr != NULL)
      srestore (varr);
   open_logfile ();
   wactiv (mainview);

   sprintf (filename, "%sHSLINK%d.LST", config->sys_path, line_offset);
   unlink (filename);

   sprintf (filename, "%sHSLINK%d.LOG", config->sys_path, line_offset);
   fp = fopen (filename, "rt");
   if (fp == NULL) {
      m_print(bbstxt[B_TRANSFER_ABORT]);
      return;
   }

   sprintf (filename, "XFER%d", line_offset);
   fp2 = fopen (filename, "wt");
   isupld = 0;

   while (fgets (filename, 145, fp) != NULL) {
      while (filename[strlen (filename) -1] == 0x0D || filename[strlen (filename) -1] == 0x0A)
         filename[strlen (filename) -1] = '\0';

      p = strtok (filename, " ");
      if (p[0] == 'R') {
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         fprintf (fp2, "%s\n", strupr (p));
         isupld = 1;
      }
   }

   if (isupld)
      check_uploads (fp2, path);

   fclose (fp2);

   sprintf (filename, "XFER%d", line_offset);
   unlink (filename);

   rewind (fp);

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
         sprintf (filename, "%c  0%c ", config->dl_counter_limits[0], config->dl_counter_limits[1]);
         fprintf(fp,"%-12s %s%s\n", dir, config->keep_dl_count ? filename : "", stringa);
         if (config->put_uploader)
            fprintf (fp, bbstxt[B_UPLOADER_NAME], usr.name);
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

         sprintf (filename, "%s%s", (strchr (p, '\\') == NULL) ? sys.filepath : "", strupr (p));
         if (config->keep_dl_count)
            update_filestat (filename);
         download_report (filename, 2, NULL);

         started = (cps * 1000L) / ((long) rate);
         status_line ((char *) msgtxt[M_CPS_MESSAGE], cps, bytes, started);
         status_line ("+DL-H %s%s", (strchr (p, '\\') == NULL) ? sys.filepath : "", strupr (p));

         if (function_active == 3)
            f3_status ();
      }
   }

   fclose (fp);
   sprintf (filename, "%sHSLINK%d.LOG", config->sys_path, line_offset);
   unlink (filename);

   m_print(bbstxt[B_TRANSFER_OK]);
}

void puma_protocol (name, path, global, upload)
char *name, *path;
int global, upload;
{
   FILE *fp, *fp2;
   int fd, *varr;
   char dir[20], filename[150], *p, ext[5], stringa[60], isupld;
   long started, bytes;
   struct ffblk blk;
   struct _sys tsys;

   if (name != NULL) {
      sprintf (filename, "%sPUMA%d.LST", config->sys_path, line_offset);
      fp = fopen (filename, "wt");

      while (get_string (name, dir) != NULL) {
         sprintf (filename, "%s%s", path, dir);

         if (findfirst(filename,&blk,0)) {
            sprintf (filename, "%sSYSFILE.DAT", config->sys_path);
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
      sprintf (filename, "PUMA P%d L%sPUMA%d.LOG R %s", com_port + 1, config->sys_path, line_offset, sys.uppath);
   else
      sprintf (filename, "PUMA P%d L%sPUMA%d.LOG S @%sPUMA%d.LST", com_port + 1, config->sys_path, line_offset, config->sys_path, line_offset);

   varr = ssave ();
//   MDM_DISABLE ();
   cclrscrn(LGREY|_BLACK);
   showcur();
   fclose (logf);

   spawn_program (registered, filename);

/*
   com_install (com_port);
   com_baud (rate);
*/
   if (varr != NULL)
      srestore (varr);

   open_logfile ();
   wactiv (mainview);

   sprintf (filename, "%sPUMA%d.LST", config->sys_path, line_offset);
   unlink (filename);

   sprintf (filename, "%sPUMA%d.LOG", config->sys_path, line_offset);
   fp = fopen (filename, "rt");
   if (fp == NULL) {
      m_print(bbstxt[B_TRANSFER_ABORT]);
      return;
   }

   sprintf (filename, "XFER%d", line_offset);
   fp2 = fopen (filename, "wt");
   isupld = 0;

   while (fgets (filename, 145, fp) != NULL) {
      while (filename[strlen (filename) -1] == 0x0D || filename[strlen (filename) -1] == 0x0A)
         filename[strlen (filename) -1] = '\0';

      p = strtok (filename, " ");
      if (p[0] == 'R') {
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         p = strtok (NULL, " ");
         fprintf (fp2, "%s\n", strupr (p));
         isupld = 1;
      }
   }

   if (isupld)
      check_uploads (fp2, path);

   fclose (fp2);

   sprintf (filename, "XFER%d", line_offset);
   unlink (filename);

   rewind (fp);

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
         sprintf (filename, "%c  0%c ", config->dl_counter_limits[0], config->dl_counter_limits[1]);
         fprintf(fp,"%-12s %s%s\n", dir, config->keep_dl_count ? filename : "", stringa);
         if (config->put_uploader)
            fprintf (fp, bbstxt[B_UPLOADER_NAME], usr.name);
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

         sprintf (filename, "%s%s", (strchr (p, '\\') == NULL) ? sys.filepath : "", strupr (p));
         if (config->keep_dl_count)
            update_filestat (filename);
         download_report (filename, 2, NULL);

         started = (cps * 1000L) / ((long) rate);
         status_line ((char *) msgtxt[M_CPS_MESSAGE], cps, bytes, started);
         status_line ("+DL-P %s%s", (strchr (p, '\\') == NULL) ? sys.filepath : "", strupr (p));

         if (function_active == 3)
            f3_status ();
      }
   }

   fclose (fp);
   sprintf (filename, "%sPUMA%d.LOG", config->sys_path, line_offset);
   unlink (filename);

   m_print(bbstxt[B_TRANSFER_OK]);
}

void display_external_protocol (int id)
{
   int fd;
   char filename[80];
   PROTOCOL prot;

   sprintf (filename, "%sPROTOCOL.DAT", config->sys_path);
   fd = sh_open (filename, O_RDONLY|O_BINARY, SH_DENYWR, S_IREAD|S_IWRITE);
   lseek (fd, (long)(id - 10) * sizeof (PROTOCOL), SEEK_SET);
   read (fd, &prot, sizeof (PROTOCOL));
   close (fd);

   m_print (bbstxt[B_PROTOCOL], prot.name);
}

void general_external_protocol (char *name, char *path, int id, int global, int dl)
{
   FILE *fp, *fp2;
   int fd, *varr, i;
   char dir[80], filename[150], *p, ext[5], stringa[60], isupld, *fnam;
   char cpath[80], killafter = 0;
   long started, bytes;
   struct ffblk blk;
   struct _sys tsys;
   PROTOCOL prot;

   if (global == -1) {
      global = 0;
      killafter = 1;
   }

   sprintf (filename, "%sPROTOCOL.DAT", config->sys_path);
   if ((fd = sh_open (filename, O_RDONLY|O_BINARY, SH_DENYWR, S_IREAD|S_IWRITE)) == -1)
      return;

   lseek (fd, (long)(id - 10) * sizeof (PROTOCOL), SEEK_SET);
   read (fd, &prot, sizeof (PROTOCOL));
   close (fd);

   if (prot.log_name[0]) {
      strcpy (filename, prot.log_name);
      translate_filenames (filename, 0, NULL);
      unlink (filename);
   }

   if (prot.ctl_name[0]) {
      strcpy (filename, prot.ctl_name);
      translate_filenames (filename, 0, NULL);
      if ((fp = fopen (filename, "wt")) == NULL)
         return;

      if (name != NULL) {
         while (get_string (name, dir) != NULL) {
            sprintf (filename, "%s%s", path, dir);

            if (findfirst(filename,&blk,0)) {
               sprintf (filename, "%sSYSFILE.DAT", config->sys_path);
               fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);

               if (global) {
                  while (read(fd, (char *)&tsys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA) {
                     if (usr.priv < tsys.file_priv)
                        continue;
                     if ((usr.flags & tsys.file_flags) != tsys.file_flags)
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

            strcpy (stringa, prot.dl_ctl_string);
            strsrep (stringa, "%1", filename);
            fprintf (fp, "%s\n", stringa);
         }
      }
      else {
         sprintf (filename, "%sEXTRN%d.LST", config->sys_path, line_offset);
         if ((fp2 = fopen (filename, "rt")) != NULL) {
            while (fgets (filename, 145, fp2) != NULL) {
               filename[145] = '\0';
               if (filename[0]) {
                  while (filename[strlen (filename) - 1] == 0x0D || filename[strlen (filename) - 1] == 0x0A)
                     filename[strlen (filename) - 1] = '\0';
               }

               strcpy (stringa, prot.dl_ctl_string);
               strsrep (stringa, "%1", filename);
               fprintf (fp, "%s\n", stringa);
            }

            fclose (fp2);

            sprintf (filename, "%sEXTRN%d.LST", config->sys_path, line_offset);
            unlink (filename);
         }
      }

      fclose (fp);

      if (dl)
         strcpy (filename, prot.dl_command);
      else
         strcpy (filename, prot.ul_command);
   }
   else {
      sprintf (stringa, "%s%s", path, name);
      if (dl) {
         strcpy (filename, prot.dl_command);
         strsrep (filename, "%1", stringa);
      }
      else
         strcpy (filename, prot.ul_command);
   }

   if (!dl) {
      path[strlen (path) - 1] = '\0';
      strsrep (filename, "%1", path);
      path[strlen (path)] = '\\';
      strsrep (filename, "%2", name);
   }

   varr = ssave ();
   cclrscrn(LGREY|_BLACK);
   showcur();
   fclose (logf);

   if (prot.disable_fossil)
      MDM_DISABLE ();

   if (!dl && prot.change_to_dl) {
      getcwd (cpath, 79);
      path[strlen (path) - 1] = '\0';
      if (path[1] == ':')
         setdisk (toupper (path[0]) - 'A');
      chdir (path);
      path[strlen (path)] = '\\';
   }

   translate_filenames (filename, 0, NULL);
   spawn_program (registered, filename);

   com_install (com_port);
   if (!config->lock_baud)
      com_baud (rate);

   if (!dl && prot.change_to_dl) {
      setdisk (toupper (cpath[0]) - 'A');
      chdir (cpath);
   }

   if (varr != NULL)
      srestore (varr);

   open_logfile ();
   wactiv (mainview);

   if (prot.ctl_name[0]) {
      strcpy (filename, prot.ctl_name);
      translate_filenames (filename, 0, NULL);
      unlink (filename);
   }

   if (!emulator) {
      m_print (bbstxt[B_TWO_CR]);

      if (prot.log_name[0]) {
         strcpy (filename, prot.log_name);
         translate_filenames (filename, 0, NULL);
         fp = fopen (filename, "rt");
         if (fp == NULL) {
            m_print(bbstxt[B_TRANSFER_ABORT]);
            return;
         }

         sprintf (filename, "XFER%d", line_offset);
         fp2 = fopen (filename, "w+t");
         isupld = 0;

         while (fgets (filename, 145, fp) != NULL) {
            filename[145] = '\0';
            if (filename[0]) {
               while (filename[strlen (filename) -1] == 0x0D || filename[strlen (filename) -1] == 0x0A)
                  filename[strlen (filename) -1] = '\0';
            }

            if ((p = my_strtok (filename, " ")) != NULL)
               if (!strcmp (p, prot.ul_keyword)) {
                  i = prot.filename - 1;
                  for (;;) {
                     if ((p = my_strtok (NULL, " ")) == NULL)
                        break;
                     if (i-- == 0)
                        break;
                  }
                  if (i == 0 && p != NULL) {
                     fprintf (fp2, "%s\n", strupr (p));
                     isupld = 1;
                  }
               }
         }

         if (isupld && !no_check) {
            fseek (fp2, 0L, SEEK_SET);
            check_uploads (fp2, path);
         }

         timer (10);
         fclose (fp2);

         sprintf (filename, "XFER%d", line_offset);
         unlink (filename);

         rewind (fp);

         while (fgets (filename, 145, fp) != NULL) {
            filename[145] = '\0';
            if (filename[0]) {
               while (filename[strlen (filename) -1] == 0x0D || filename[strlen (filename) -1] == 0x0A)
                  filename[strlen (filename) -1] = '\0';
            }

            if ((p = my_strtok (filename, " ")) == NULL)
               continue;

            if (!strcmp (p, prot.ul_keyword)) {
               i = 1;
               fnam = NULL;

               if (!prot.size)
                  prot.size = 2;
               if (!prot.cps)
                  prot.cps = 5;
               if (!prot.filename)
                  prot.filename = 11;

               while ((p = my_strtok (NULL, " ")) != NULL) {
                  i++;

                  if (i == prot.size) {
                     bytes = atol (p);
                     usr.upld += (int)(bytes / 1024L) + 1;
                     usr.n_upld++;
                  }
                  else if (i == prot.cps)
                     cps = atoi (p);
                  else if (i == prot.filename)
                     fnam = p;
               }

               if (fnam != NULL) {
                  p = fnam;

                  started = (cps * 1000L) / ((long) rate);
                  status_line ((char *) msgtxt[M_CPS_MESSAGE], cps, bytes, started);
                  status_line ("%s-%c %s%s", msgtxt[M_FILE_RECEIVED], prot.hotkey, path, strupr (p));

                  if (!no_description) {
                     fnsplit (p, NULL, NULL, dir, ext);
                     strcat (dir, ext);

                     if (!check_file_description (dir, path)) {
                        do {
                           m_print(bbstxt[B_DESCRIBE],strupr(dir));
                           CLEAR_INBOUND ();
                           input(stringa,54);
                           if (!CARRIER)
                              break;
                        } while (strlen(stringa) < 5);

                        sprintf (filename, "%sFILES.BBS", path);
                        fp2 = fopen (filename, "at");
                        sprintf (filename, "%c  0%c ", config->dl_counter_limits[0], config->dl_counter_limits[1]);
                        fprintf (fp2, "%-12s %s%s\n", dir, config->keep_dl_count ? filename : "", stringa);
                        if (config->put_uploader)
                           fprintf (fp2, bbstxt[B_UPLOADER_NAME], usr.name);
                        fclose (fp2);
                     }
                  }

                  if (function_active == 3)
                     f3_status ();
               }
            }
            else if (!strcmp (p, prot.dl_keyword)) {
               i = 1;
               fnam = NULL;

               if (!prot.size)
                  prot.size = 2;
               if (!prot.cps)
                  prot.cps = 5;
               if (!prot.filename)
                  prot.filename = 11;

               while ((p = my_strtok (NULL, " ")) != NULL) {
                  i++;

                  if (i == prot.size) {
                     bytes = atol (p);
                     usr.dnld += (int)(bytes / 1024L) + 1;
                     usr.dnldl += (int)(bytes / 1024L) + 1;
                     usr.n_dnld++;
                  }
                  else if (i == prot.cps)
                     cps = atoi (p);
                  else if (i == prot.filename)
                     fnam = p;
               }

               if (fnam != NULL) {
                  p = fnam;
                  sprintf (filename, "%s%s", (strchr (p, '\\') == NULL) ? sys.filepath : "", strupr (p));
                  if (config->keep_dl_count)
                     update_filestat (filename);
                  download_report (filename, 2, NULL);

                  started = (cps * 1000L) / ((long) rate);
                  status_line ((char *) msgtxt[M_CPS_MESSAGE], cps, bytes, started);
                  status_line ("+DL-%c %s%s", prot.hotkey, (strchr (p, '\\') == NULL) ? sys.filepath : "", strupr (p));

                  if (killafter)
                     file_kill (1, p);

                  if (function_active == 3)
                     f3_status ();
               }
            }
         }

         fclose (fp);

         strcpy (filename, prot.log_name);
         translate_filenames (filename, 0, NULL);
         unlink (filename);
      }

      m_print(bbstxt[B_TRANSFER_OK]);
   }
   else {
      if (prot.log_name[0]) {
         strcpy (filename, prot.log_name);
         translate_filenames (filename, 0, NULL);
         fp = fopen (filename, "rt");
         if (fp == NULL)
            return;

         while (fgets (filename, 145, fp) != NULL) {
            filename[145] = '\0';
            if (filename[0]) {
               while (filename[strlen (filename) -1] == 0x0D || filename[strlen (filename) -1] == 0x0A)
                  filename[strlen (filename) -1] = '\0';
            }

            if ((p = my_strtok (filename, " ")) == NULL)
               continue;

            if (!strcmp (p, prot.ul_keyword)) {
               i = 1;
               fnam = NULL;

               if (!prot.size)
                  prot.size = 2;
               if (!prot.cps)
                  prot.cps = 5;
               if (!prot.filename)
                  prot.filename = 11;

               while ((p = my_strtok (NULL, " ")) != NULL) {
                  i++;

                  if (i == prot.size) {
                     bytes = atol (p);
                     usr.upld += (int)(bytes / 1024L) + 1;
                     usr.n_upld++;
                  }
                  else if (i == prot.cps)
                     cps = atoi (p);
                  else if (i == prot.filename)
                     fnam = p;
               }

               if (fnam != NULL) {
                  p = fnam;

                  started = (cps * 1000L) / ((long) rate);
                  status_line ((char *) msgtxt[M_CPS_MESSAGE], cps, bytes, started);
                  status_line ("%s-%c %s%s", msgtxt[M_FILE_RECEIVED], prot.hotkey, path, strupr (p));
               }
            }
            else if (!strcmp (p, prot.dl_keyword)) {
               i = 1;
               fnam = NULL;

               if (!prot.size)
                  prot.size = 2;
               if (!prot.cps)
                  prot.cps = 5;
               if (!prot.filename)
                  prot.filename = 11;

               while ((p = my_strtok (NULL, " ")) != NULL) {
                  i++;

                  if (i == prot.size) {
                     bytes = atol (p);
                     usr.dnld += (int)(bytes / 1024L) + 1;
                     usr.dnldl += (int)(bytes / 1024L) + 1;
                     usr.n_dnld++;
                  }
                  else if (i == prot.cps)
                     cps = atoi (p);
                  else if (i == prot.filename)
                     fnam = p;
               }

               if (fnam != NULL) {
                  p = fnam;
                  started = (cps * 1000L) / ((long) rate);
                  status_line ((char *) msgtxt[M_CPS_MESSAGE], cps, bytes, started);
                  status_line ("+DL-%c %s%s", prot.hotkey, (strchr (p, '\\') == NULL) ? sys.filepath : "", strupr (p));
               }
            }
         }

         fclose (fp);

         strcpy (filename, prot.log_name);
         translate_filenames (filename, 0, NULL);
         unlink (filename);
      }
   }
}

void check_uploads (xferinfo, path)
FILE *xferinfo;
char *path;
{
   FILE *fp;
   int fd;
   char filename[80], stringa[80], name[20], ext[5], *p, origname[20];
   struct ffblk blk;
   struct _sys tsys;

   m_print (bbstxt[B_CHECK_UPLOADS]);

   sprintf (filename, "%sSYSFILE.DAT", config->sys_path);
   fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
   if (fd == -1)
      return;

   sprintf (filename, "%sNOCHECK.CFG", config->sys_path);
   fp = fopen (filename, "rt");

   rewind (xferinfo);

   while (fgets (stringa, 78, xferinfo) != NULL) {
      stripcrlf (stringa);

      fnsplit (stringa, NULL, NULL, name, ext);
      strcat (name, ext);
      strcpy (origname, name);

      if ((p = strchr (name, '.')) != NULL)
         strcpy (p, ".*");

      if (fp != NULL) {
         rewind (fp);
         while (fgets (filename, 78, fp) != NULL) {
            stripcrlf (filename);
            if (!stricmp (filename, origname))
               break;
         }
      }

      if (fp == NULL || stricmp (filename, origname)) {
         lseek (fd, 0L, SEEK_SET);

         while (read (fd, (char *)&tsys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA) {
            if (!stricmp (tsys.filepath, path) || tsys.no_global_search)
               continue;
            sprintf (filename, "%s%s", tsys.filepath, name);
            if (!findfirst (filename, &blk, 0)) {
               m_print (bbstxt[B_ALREADY_HAVE], name, tsys.file_num, tsys.file_name);
               sprintf (filename, "%s%s", path, origname);
               unlink (filename);
               status_line ("+Duplicate upload: %s (area #%d)", origname, tsys.file_num);
               allowed -= (int) (blk.ff_fsize * 10 / rate + 53) / 54;
               break;
            }
         }
      }

      sprintf (filename, "%s%s", path, origname);
      if (dexists (filename))
         check_virus (filename);
   }

   m_print (bbstxt[B_TWO_CR]);

   if (fp != NULL)
      fclose (fp);

   close (fd);
   rewind (xferinfo);
}

#define MAX_INDEX    500

void upload_filebox ()
{
   int fd, fflag, posit, i, m;
   char filename[80], stringa[80], olduploader;
   long crc;
   struct _usr tusr;
   struct _usridx usridx[MAX_INDEX];

retry:
   fflag = 0;
   posit = 0;

   do {
      m_print (bbstxt[B_FILEBOX_NAME]);
      chars_input (stringa, 35, INPUT_FANCY|INPUT_FIELD);
      if (!CARRIER || !stringa[0])
         return;

      crc = crc_name (stringa);

      sprintf (filename, "%s.IDX", config->user_file);
      fd = shopen (filename, O_RDONLY|O_BINARY);

      do {
         i = read (fd, (char *)&usridx, sizeof(struct _usridx) * MAX_INDEX);
         m = i / sizeof (struct _usridx);

         for (i = 0; i < m; i++)
            if (usridx[i].id == crc) {
               m = 0;
               posit += i;
               fflag = 1;
               break;
            }

         if (!fflag)
            posit += m;
      } while (m == MAX_INDEX && !fflag);

      close (fd);

      if (!fflag)
         m_print (bbstxt[B_FILEBOX_NOTFOUND]);

   } while (!fflag);

   sprintf (filename, "%s.BBS", config->user_file);

   fd = shopen (filename, O_RDWR|O_BINARY);
   lseek (fd, (long)posit * sizeof (struct _usr), SEEK_SET);
   read(fd, (char *)&tusr, sizeof(struct _usr));
   close (fd);

   if (!tusr.havebox) {
      m_print (bbstxt[B_NO_FILEBOX], stringa);
      goto retry;
   }

   sprintf (filename, "%s%08lx", config->boxpath, crc);
   mkdir (filename);
   strcat (filename, "\\");

   olduploader = config->put_uploader;
   config->put_uploader = 1;
   no_check = 1;

   upload_file (filename, -1);

   no_external = no_check = 0;
   config->put_uploader = olduploader;

   m_print (bbstxt[B_SEND_ANOTHER]);
   if (yesno_question (DEF_NO) == DEF_YES)
      goto retry;
}

void download_filebox (int godown)
{
   char filename[80], have = 0, i;
   struct ffblk blk;

   if (!usr.havebox)
      return;

   sprintf (filename, "%s%08lx\\*.*", config->boxpath, usr.id);
   if (!findfirst (filename, &blk, 0))
      do {
         if (!strcmp (blk.ff_name, "."))
            continue;
         else if (!strcmp (blk.ff_name, ".."))
            continue;
         else if (!stricmp (blk.ff_name, "FILES.BBS"))
            continue;
         else if (!stricmp (blk.ff_name, "FILES.BAK"))
            continue;
         else {
            have = 1;
            break;
         }
      } while (!findnext (&blk));

   if (!have) {
      m_print (bbstxt[B_NO_NEW_FILES]);
      press_enter ();
      return;
   }

   sprintf (filename, "%s%08lx\\", config->boxpath, usr.id);

   m_print (bbstxt[B_FILELIST1]);
   m_print (bbstxt[B_FILELIST2]);

   filebox_list (filename);

   if (godown) {
      m_print (bbstxt[B_ASK_TO_DOWNLOAD]);
      if (yesno_question (DEF_YES) == DEF_NO)
         return;

      i = sys.freearea;
      sys.freearea = 1;

      m_print (bbstxt[B_FB_DELETE_AFTER]);
      if (yesno_question (DEF_YES) == DEF_YES)
         download_file (filename, -1);
      else
         download_file (filename, -2);

      sys.freearea = i;
   }
   else
      press_enter ();
}


typedef struct _dirent {
   char name[13];
   unsigned date;
   long size;
} DIRENT;

DIRENT *scandir (char *dir, int *n);


void filebox_list (char *path)
{
   int fd, m, i, line, z, nnd, yr;
   char filename[80], name[14], desc[70], *fbuf, *p;
   DIRENT *de;

   i = strlen(path) - 1;
   while (path[i] == ' ')
      path[i--] = '\0';
   while (*path == ' ' && *path != '\0')
      path++;

   sprintf(filename,"%sFILES.BBS",path);
   fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);

   // Se il file e' di 0 bytes non importa leggere altro
   if (fd == -1 || filelength (fd) == 0L) {
      if (fd != -1)
         close (fd);
      return;
   }

   line = 3;

   // Alloca il buffer che andra' a contenere l'intero files.bbs per
   // l'area corrente. Se non riesce ad allocarselo scrive una linea
   // nel log con le informazioni utili (spero) per il debugging.
   if ((fbuf = (char *)malloc ((unsigned int)filelength (fd) + 1)) == NULL) {
#ifdef __OS2__
      status_line ("!MEM: new_file_list (%ld)", filelength (fd));
#else
      status_line ("!MEM: new_file_list (%ld / %ld)", filelength (fd), coreleft ());
#endif
      close (fd);
      return;
   }

   // Lettura di tutto il files.bbs nel buffer allocato.
   read (fd, fbuf, (unsigned int)filelength (fd));
   fbuf[(unsigned int)filelength (fd)] = '\0';
   close (fd);

   // Legge tutta la directory in memoria. Se non c'e' spazio sufficiente
   // prosegue (scandir si occupa di segnalare nel log gli errori.
   if ((de = scandir (path, &nnd)) == NULL) {
      free (fbuf);
      return;
   }

   // La scansione del files.bbs avviene in memoria. Tenendo conto che
   // ogni linea e' separata almeno da un \r (CR, 0x0D) si puo' utilizzare
   // la strtok, trattando l'intero buffer come un insieme di token.
   if ((p = strtok (fbuf, "\r")) != NULL)
      do {
         // Nel caso di \r\n (EOL) e' necessario saltare il \n per evitare
         // problemi nel riconoscimento dei files.
         if (*p == '\n')
            p++;

         if (*p == '\0') {
            m_print (bbstxt[B_ONE_CR]);
            if ((line = more_question (line)) == 0 || !CARRIER)
               break;
            continue;
         }

         if (!CARRIER || (!local_mode && RECVD_BREAK())) {
            line = 0;
            break;
         }

         // Se il primo carattere non e' alfanumerico si tratta di un
         // commento, pertanto non e' necessario visualizzarlo.
         if (*p == '-') {
            *p = ' ';
            m_print (bbstxt[B_FILES_DASHLINE], p);

            if ((line = more_question (line)) == 0 || !CARRIER)
               break;
         }
         else if (*p == ' ') {
            if (p[1] == '>' || p[1] == '|') {
               if (sys.no_filedate)
                  m_print (bbstxt[B_FILES_XNODATE], &p[2]);
               else
                  m_print (bbstxt[B_FILES_XDATE], &p[2]);
            }
            else
               m_print (bbstxt[B_FILES_SPACELINE], &p[1]);

            if ((line = more_question (line)) == 0 || !CARRIER)
               break;
         }
         else {
            // Estrae il nome del file dalla stringa puntata da p
            m = 0;
            while (p[m] && p[m] != ' ' && m < 12)
               name[m] = p[m++];
            name[m] = '\0';

            if (m == 0) {
               m_print (bbstxt[B_ONE_CR]);
               if ((line = more_question (line)) == 0 || !CARRIER)
                  break;
               continue;
            }

            // Cerca il file nella lista in memoria puntata da de.
            for (i = 0; i < nnd; i++) {
               if (!stricmp (de[i].name, name))
                  break;
            }

            // Se trova il file nella directory prosegue le verifiche
            // altrimenti salta alla linea sucessiva.
            yr = (i >= nnd) ? -1 : 0;

            z = 0;
            while (p[m] == ' ')
               m++;
            while (p[m] && z < (sys.no_filedate ? 56 : 48))
               desc[z++] = p[m++];
            if (p[m] != '\0')
               while (p[m] != ' ' && z > 0) {
                  m--;
                  z--;
               }
            desc[z] = '\0';

            if (sys.no_filedate) {
               if (yr == -1)
                  m_print (bbstxt[B_FILES_NODATE_MISSING], strupr (name), bbstxt[B_FILES_MISSING], desc);
               else
                  m_print (bbstxt[B_FILES_NODATE], strupr (name), de[i].size, desc);
            }
            else {
               if (yr == -1)
                  m_print (bbstxt[B_FILES_FORMAT_MISSING], strupr (name), bbstxt[B_FILES_MISSING], desc);
               else
                  m_print (bbstxt[B_FILES_FORMAT], strupr (name), de[i].size, show_date (config->dateformat, e_input, de[i].date, 0), desc);
            }
            if (!(line = more_question (line)) || !CARRIER)
               break;

            while (p[m] != '\0') {
               z = 0;
               while (p[m] != '\0' && z < (sys.no_filedate ? 56 : 48))
                  desc[z++] = p[m++];
               if (p[m] != '\0')
                  while (p[m] != ' ' && z > 0) {
                     m--;
                     z--;
                  }
               desc[z]='\0';

               if (sys.no_filedate)
                  m_print (bbstxt[B_FILES_XNODATE], desc);
               else
                  m_print (bbstxt[B_FILES_XDATE], desc);

               if ((line=more_question(line)) == 0 || !CARRIER)
                  break;
            }

            if (line == 0)
               break;
         }
      } while ((p = strtok (NULL, "\r")) != NULL);

   free (de);
   free (fbuf);
}


/*
void filebox_list (path)
char *path;
{
   FILE *fp;
   int m, i, yr, mo, dy, line;
   char buffer[260], filename[80], name[14], comment[60];
   struct ffblk blk;

   i = strlen(path) - 1;
   while (path[i] == ' ')
      path[i--] = '\0';
   while (*path == ' ' && *path != '\0')
      path++;

   sprintf(filename,"%sFILES.BBS",path);
   if ((fp = fopen(filename,"rt")) == NULL)
      return;

   line = 3;

   while (fgets (buffer, 250, fp) != NULL && CARRIER) {
      if (!local_mode && RECVD_BREAK ())
         break;

      buffer[strlen (buffer) - 1] = 0;

      if (buffer[0] == '-') {
         buffer[0] = ' ';
         m_print (bbstxt[B_FILES_DASHLINE], buffer);

         if ((line = more_question (line)) == 0 || !CARRIER)
            break;
      }
      else if (buffer[0] == ' ') {
         if (buffer[1] == '>') {
            if (sys.no_filedate)
               m_print (bbstxt[B_FILES_XNODATE], &buffer[2]);
            else
               m_print (bbstxt[B_FILES_XDATE], &buffer[2]);
         }
         else
            m_print (bbstxt[B_FILES_SPACELINE], buffer);

         if ((line=more_question(line)) == 0 || !CARRIER)
            break;
      }
      else {
         m = 0;
         i = 0;
         while(buffer[m] != ' ' && buffer[m])
            name[i++] = buffer[m++];
         name[i]='\0';

         while(buffer[m] == ' ')
            m++;

         sprintf(filename,"%s%s",path,name);
         if (findfirst (filename, &blk, 0)) {
            if (!config->show_missing)
               continue;
            yr = -1;
         }
         else {
            if (!sys.no_filedate || path != NULL) {
               yr = ((blk.ff_fdate >> 9) & 0x7F);
               mo = ((blk.ff_fdate >> 5) & 0x0F);
               dy = blk.ff_fdate & 0x1F;
            }
         }

         i = 0;
         while(buffer[m] != '\0' && i < (sys.no_filedate ? 56 : 48))
            comment[i++] = buffer[m++];
         if (buffer[m] != '\0')
            while (buffer[m] != ' ') {
               m--;
               i--;
            }
         comment[i]='\0';

         if (sys.no_filedate) {
            if (yr == -1)
               m_print (bbstxt[B_FILES_NODATE_MISSING], strupr(name), bbstxt[B_FILES_MISSING], comment);
            else
               m_print (bbstxt[B_FILES_NODATE], strupr(name), blk.ff_fsize, comment);
         }
         else {
            if (yr == -1)
               m_print (bbstxt[B_FILES_FORMAT_MISSING], strupr(name), bbstxt[B_FILES_MISSING], comment);
            else
               m_print (bbstxt[B_FILES_FORMAT], strupr(name), blk.ff_fsize, dy, mo, (yr + 80) % 100, comment);
         }

         if ((line=more_question(line)) == 0 || !CARRIER)
            break;

         while (buffer[m] != '\0') {
            i = 0;
            while(buffer[m] != '\0' && i < (sys.no_filedate ? 56 : 48))
               comment[i++] = buffer[m++];
            if (buffer[m] != '\0')
               while (buffer[m] != ' ') {
                  m--;
                  i--;
               }
            comment[i]='\0';

            if (sys.no_filedate)
               m_print (bbstxt[B_FILES_XNODATE], comment);
            else
               m_print (bbstxt[B_FILES_XDATE], comment);

            if ((line=more_question(line)) == 0 || !CARRIER)
               break;
         }

         if (buffer[m] != '\0')
            break;
      }
   }

   fclose(fp);
}
*/

void update_filestat (char *rqname)
{
   FILE *fp1, *fp2;
   char name[14], buff[255], file1[81], file2[81];
   char drive[80], path[80], fname[16], ext[6];
   int m, z;
   struct _sys tsys;

   if (rqname == NULL || *rqname == '\0')
      return;

   strupr (rqname);
   fnsplit (rqname, path, drive, fname, ext);
   strcat (path, drive);
   strcat (fname, ext);
   strupr (fname);

   sprintf (file1, "%sSYSFILE.DAT", config->sys_path);
   m = sh_open (file1, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
   if (m == -1)
      return;

   while (read (m, &tsys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA) {
      if (!stricmp (tsys.filepath, path))
         break;
   }

   close (m);

   if (!stricmp (tsys.filepath, path)) {
      if (tsys.filelist[0])
         strcpy (file1, tsys.filelist);
      else
         sprintf (file1, "%sFILES.BBS", path);
   }
   else
      sprintf (file1, "%sFILES.BBS", path);

   m = sh_open (file1, SH_DENYWR, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
   if (m == -1)
      return;
   fp1 = fdopen (m, "rt");

   sprintf (file1, "%sFILE%d.$$$", path, config->line_offset);
   unlink (file1);
   if ((fp2 = fopen (file1, "wt")) == NULL) {
      fclose (fp1);
      return;
   }

   while (fgets (buff, 250, fp1) != NULL) {
      while (buff[strlen (buff) -1] == 0x0D || buff[strlen (buff) -1] == 0x0A)
         buff[strlen (buff) -1] = '\0';
      m = z = 0;
      while (buff[m] != ' ' && m < 12)
         name[m] = buff[m++];
      name[m] = '\0';
      strupr (name);

      while (buff[m] == ' ')
         m++;

      if (!stricmp (fname, name)) {
         if (buff[m] == config->dl_counter_limits[0] && buff[m + 4] == config->dl_counter_limits[1]) {
            sscanf (&buff[m + 1], "%3d", &z);
            z = z + 1;
            m += 6;
         }
         else
            z = 1;

         fprintf (fp2, "%-12.12s %c%3d%c %s\n", name, config->dl_counter_limits[0], z, config->dl_counter_limits[1], &buff[m]);
      }
      else
         fprintf (fp2, "%s\n", buff);
   }

   fclose (fp1);
   fclose (fp2);

   sprintf (file1, "%sFILES.BBS", path);
   sprintf (file2, "%sFILE%d.$$$", path, config->line_offset);
   unlink (file1);
   rename (file2, file1);
}

void tag_files (int only_one)
{
   FILE *fp;
   int fd, i, byte_sec, min, sec;
   char filename[80], *root, name[80], firstflag;
   struct ffblk blk;
   struct _sys tsys;

   fp = NULL;
   firstflag = 1;

   if (!cmd_string[0]) {
      ask_for_filename (name, 1);
      if (!name[0] || !CARRIER)
         return;
   }
   else {
      strcpy (name, cmd_string);
      cmd_string[0] = '\0';
   }

   for (;;) {
      if ((root = my_strtok (name, " ")) == NULL)
         return;

      do {
         sprintf (filename, "%s%s", sys.filepath, root);

         if (findfirst (filename, &blk, 0)) {
            i = 0;

            if (!only_one) {
               sprintf (filename, "%sSYSFILE.DAT", config->sys_path);
               fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);

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
                     if (fp == NULL) {
                        sprintf (filename, "F-TAG%d.TMP", config->line_offset);
                        if ((fp = fopen (filename, "at")) == NULL)
                           break;
                     }

                     fprintf (fp, "%s%s %ld\n", tsys.filepath, blk.ff_name, blk.ff_fsize);

                     byte_sec = rate / 11;
                     i = (int)((blk.ff_fsize + (blk.ff_fsize / 1024)) / byte_sec);
                     min = i / 60;
                     sec = i - min * 60;

                     if (firstflag) {
                        m_print (bbstxt[B_ONE_CR]);
                        firstflag = 0;
                     }
                     m_print (bbstxt[B_TAG_CONFIRM], blk.ff_name, min, sec, blk.ff_fsize);

                     i = 1;
                  } while (!findnext (&blk));
               }

               close (fd);
            }

            if (only_one || !i) {
               if (firstflag) {
                  m_print (bbstxt[B_ONE_CR]);
                  firstflag = 0;
               }
               m_print (bbstxt[B_TAG_FILENOTFOUND], strupr (root));
            }
         }
         else
            do {
               if (fp == NULL) {
                  sprintf (filename, "F-TAG%d.TMP", config->line_offset);
                  if ((fp = fopen (filename, "at")) == NULL)
                     break;
               }

               fprintf (fp, "%s%s %ld\n", sys.filepath, blk.ff_name, blk.ff_fsize);

               byte_sec = rate / 11;
               i = (int)((blk.ff_fsize + (blk.ff_fsize / 1024)) / byte_sec);
               min = i / 60;
               sec = i - min * 60;

               if (firstflag) {
                  m_print (bbstxt[B_ONE_CR]);
                  firstflag = 0;
               }
               m_print (bbstxt[B_TAG_CONFIRM], blk.ff_name, min, sec, blk.ff_fsize);
            } while (!findnext (&blk));
      } while ((root = my_strtok (NULL, " ")) != NULL);

      ask_for_filename (name, 1);
      if (!name[0] || !CARRIER)
         break;

      firstflag = 1;
   }

   if (fp != NULL)
      fclose (fp);
}

void list_tagged_files (int remove)
{
   FILE *fp;
   int nfiles, line, m;
   char name[14], buff[128], *p, *q, fok;
   long pos;

   sprintf (buff, "F-TAG%d.TMP", config->line_offset);
   if ((fp = fopen (buff, "r+t")) == NULL) {
      m_print (bbstxt[B_NOTAGGED_FILES]);
      timer (20);
      return;
   }

   do {
      fok = 0;
      nfiles = 0;
      line = 4;
      rewind (fp);

      while (fgets (buff, 120, fp) != NULL) {
         while (buff[strlen (buff) -1] == 0x0D || buff[strlen (buff) -1] == 0x0A)
            buff[strlen (buff) -1] = '\0';

         if (buff[0] == ';')
            continue;

         p = strtok (buff, " ");
         for (q = name; *p; ) {
            if ((*p == '/') || (*p == '\\') || (*p == ':'))
               q = name;
            else
               *q++ = toupper (*p);
            p++;
         }

         *q = '\0';
         p = strtok (NULL, " ");

         // Scrive l'header della lista solo se ci sono files all'interno.
         if (!fok) {
            m_print (bbstxt[B_TAGGEDLIST_HEADER]);
            fok = 1;
         }

         m_print (bbstxt[B_TAGGEDLIST_FORMAT], ++nfiles, name, atol (p) / 1024L);
         if ((line = more_question (line)) == 0 || !CARRIER)
            break;
      }

      if (fok) {
         if (!remove) {
            m_print (bbstxt[B_ONE_CR]);
            press_enter ();
         }
         else {
            if (!get_command_word (buff, 3)) {
               m_print (bbstxt[B_TAGGEDLIST_ASKREMOVE]);
               chars_input (buff, 3, INPUT_FIELD);
            }

            m = atoi (buff);
            if (!m || m > nfiles || !CARRIER || time_remain () <= 0)
               return;

            nfiles = 0;
            rewind (fp);
            pos = ftell (fp);

            while (fgets (buff, 120, fp) != NULL) {
               while (buff[strlen (buff) -1] == 0x0D || buff[strlen (buff) -1] == 0x0A)
                  buff[strlen (buff) -1] = '\0';

               if (buff[0] == ';') {
                  pos = ftell (fp);
                  continue;
               }

               if (++nfiles == m) {
                  fseek (fp, pos, SEEK_SET);
                  fputc (';', fp);
                  break;
               }

               pos = ftell (fp);
            }
         }
      }
   } while (remove && fok);

   fclose (fp);

   // Se non ci sono piu' files taggati, cancella il file con la lista.
   if (!fok) {
      sprintf (buff, "F-TAG%d.TMP", config->line_offset);
      unlink (buff);
   }
}

int download_tagged_files (void)
{
   FILE *fp, *fpx;
   int i, m, nfiles, byte_sec, min, sec, protocol;
   char filename[80], name[14], buff[128], *p, *q, logoffafter;
   long fl, rp, t;

   sprintf (filename, "F-TAG%d.TMP", config->line_offset);
   if ((fp = fopen (filename, "rt")) == NULL)
      return (0);

   logoffafter = 0;
   nfiles = 0;
   fl = 0L;
   filename[0] = '\0';

   while (fgets (buff, 120, fp) != NULL) {
      while (buff[strlen (buff) -1] == 0x0D || buff[strlen (buff) -1] == 0x0A)
         buff[strlen (buff) -1] = '\0';

      if (buff[0] == ';')
         continue;

      p = strtok (buff, " ");
      for (q = name; *p; ) {
         if ((*p == '/') || (*p == '\\') || (*p == ':'))
            q = name;
         else
            *q++ = toupper (*p);
         p++;
      }

      *q = '\0';
      p = strtok (NULL, " ");

      if (strlen (filename) + strlen (name) + 1 < 70) {
         if (nfiles)
            strcat (filename, " ");
         strcat (filename, name);
         fl += atol (p);
         nfiles++;
      }
      else {
         m_print (bbstxt[B_FILE_MASK], strupr (filename));
         filename[0] = '\0';
         strcpy (filename, name);
         fl += atol (p);
         nfiles++;
      }
   }

   if (!strlen (filename)) {
      fclose (fp);
      sprintf (filename, "F-TAG%d.TMP", config->line_offset);
      unlink (filename);
      return (0);
   }

   m_print (bbstxt[B_TAG_PENDING]);
   if (yesno_question (DEF_YES) == DEF_NO)
      return (0);

   if (user_status != UPLDNLD)
      set_useron_record (UPLDNLD, 0, 0);

   read_system_file ("PREDNLD");

   if ((protocol = selprot ()) == 0) {
      fclose (fp);
      return (1);
   }

   cls ();

   m_print (bbstxt[B_FILE_MASK], strupr (filename));
   m_print (bbstxt[B_LEN_MASK], fl);

   byte_sec = rate / 11;
   i = (int)((fl + (fl / 1024)) / byte_sec);
   min = i / 60;
   sec = i - min * 60;

   m_print (bbstxt[B_TOTAL_TIME], min, sec);
   if (protocol >= 10)
      display_external_protocol (protocol);
   else
      m_print (bbstxt[B_PROTOCOL], &protocols[protocol - 1][1]);

   if(((fl/1024) + usr.dnldl) > config->class[usr_class].max_dl && !usr.xfer_prior) {
      fclose (fp);
      sprintf (filename, "F-TAG%d.TMP", config->line_offset);
      unlink (filename);
      status_line (":Dnld req. would exceed limit");
      read_system_file ("TODAYK");
      return (1);
   }

   if (min > time_remain() && !sys.freearea && !usr.xfer_prior) {
      fclose (fp);
      sprintf (filename, "F-TAG%d.TMP", config->line_offset);
      unlink (filename);
      status_line (":Dnld req. would exceed limit");
      read_system_file ("NOTIME");
      return (1);
   }

   m_print (bbstxt[B_DL_CONFIRM]);
   if (usr.hotkey)
      cmd_input (filename, 1);
   else
      chars_input (filename, 1, 0);

   if (toupper (filename[0]) == 'A') {
      m_print(bbstxt[B_TRANSFER_ABORT]);
      goto abort_xfer;
   }

   if (toupper (filename[0]) == '!')
      logoffafter = 1;
   else
      logoffafter = 0;

   m_print (bbstxt[B_READY_TO_SEND]);
   m_print (bbstxt[B_CTRLX_ABORT]);

   if (local_mode) {
      sysop_error();
      m = 0;
      goto abort_xfer;
   }

   rewind (fp);
   i = 0;
   download_report (NULL, 1, NULL);

   if (protocol == 4) {
      sprintf (filename, "%sHSLINK%d.LST", config->sys_path, line_offset);
      fpx = fopen (filename, "wt");
   }
   else if (protocol == 5) {
      sprintf (filename, "%sPUMA%d.LST", config->sys_path, line_offset);
      fpx = fopen (filename, "wt");
   }
   else if (protocol >= 10) {
      sprintf (filename, "%sEXTRN%d.LST", config->sys_path, line_offset);
      fpx = fopen (filename, "wt");
   }

   while (fgets (buff, 120, fp) != NULL) {
      while (buff[strlen (buff) -1] == 0x0D || buff[strlen (buff) -1] == 0x0A)
         buff[strlen (buff) -1] = '\0';

      p = strtok (buff, " ");
      if (p == NULL || *p == ';')
         continue;

      if (protocol == 1 || protocol == 2 || protocol == 3 || protocol == 6) {
         timer (10);
         m = i = 0;

         switch (protocol) {
            case 1:
               m = send (p, 'X');
               break;
            case 2:
               m = send (p, 'Y');
               break;
            case 3:
               m = send_Zmodem (p, NULL, i, 0);
               break;
            case 6:
               m = send (p, 'S');
               break;
         }

         if (!m) {
            download_report (p, 3, NULL);
            break;
         }

         if (config->keep_dl_count)
            update_filestat (p);
         download_report (p, 2, NULL);
         i++;

         p = strtok (NULL, " ");

         if (atol (p)) {
            usr.dnld += (int)(atol (p) / 1024L) + 1;
            usr.dnldl += (int)(atol (p) / 1024L) + 1;
            usr.n_dnld++;

            if (function_active == 3)
               f3_status ();
         }
      }
      else if (protocol == 4 || protocol == 5 || protocol >= 10)
         fprintf (fpx, "%s\n", p);
   }

   if (protocol == 4) {
      fclose (fpx);
      hslink_protocol (NULL, NULL, 0);
   }
   else if (protocol == 5) {
      fclose (fpx);
      puma_protocol (NULL, NULL, 0, 0);
   }
   else if (protocol >= 10) {
      fclose (fpx);
      general_external_protocol (NULL, NULL, protocol, 0, 1);
   }
   else {
      if (m) {
         if (protocol == 3)
            send_Zmodem (NULL, NULL, ((i) ? END_BATCH : NOTHING_TO_DO), 0);
         else if (protocol == 6)
            send (NULL, 'S');
      }
   }

abort_xfer:
   wactiv (mainview);

   fclose (fp);

   download_report (NULL, 4, NULL);

   sprintf (filename, "F-TAG%d.TMP", config->line_offset);
   unlink (filename);

   if (logoffafter) {
      rp = time (NULL) + 10L;
      do {
         m = -1;
         m_print (bbstxt[B_DL_LOGOFF], (int)(rp - time (NULL)));
         t = time (NULL);
         while (time (NULL) == t && CARRIER) {
            release_timeslice ();
            if ((m = toupper (m_getch ())) == 'A')
               break;
         }
         if (m == 'A')
            break;
      } while (time (NULL) < rp && CARRIER);

      if (m != 'A') {
         read_system_file ("FGOODBYE");

         hidecur();
         terminating_call();
         get_down(aftercaller_exit, 2);
      }
   }

   return (1);
}

void check_virus (char *rqname)
{
   int *varr;
   char drive[80], path[80], fname[16], ext[6], complete[140];

   if (!dexists ("VIRSCAN.BAT"))
      return;

   fclose (logf);

   if ((varr = ssave ()) == NULL)
      return;
   clown_clear ();
   showcur();

   strupr (rqname);
   fnsplit (rqname, path, drive, fname, ext);
   strcat (path, drive);
   strupr (fname);
   strupr (ext);

   sprintf (complete, "VIRSCAN %s %s %s %s", rqname, path, fname, ext);
   spawn_program (registered, complete);

   srestore (varr);

   open_logfile ();

   hidecur();
}

