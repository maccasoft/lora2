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

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"
#include "arc.h"

typedef unsigned int uint;

#define shopen(path,access)       sopen(path,access,SH_DENYNONE,S_IREAD | S_IWRITE)
#define cshopen(path,access,mode) sopen(path,access,SH_DENYNONE,mode)

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
  char filename[14],
       an[80];

  int arcfile;

  if (!get_command_word (filename, 12))
  {
    m_print (bbstxt[B_WHICH_ARCHIVE]);
    input (filename,12);
    if (!filename[0] || !CARRIER)
       return;
  }

  strcpy(an,sys.filepath);
  strcat(an,filename);

  cls ();
  m_print(bbstxt[B_SEARCH_ARCHIVE],strupr(filename));

  if ((arcfile=shopen(an,O_RDONLY | O_BINARY))==-1)
    return;

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

  int line,
      num_files,
      extended;

  long total_compressed,
       pos,
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

    pos = tell (lzhfile);
    read(lzhfile,(char *)&bhead,sizeof(struct _arj_basic_header));

    if (bhead.id != 0xEA60U || !bhead.size)
      break;

    if (!eof(lzhfile))
    {
      read(lzhfile,(char *)&arjhead,sizeof(struct _arj_header));
      if ((fname=(char *)malloc(20))==NULL)
        return;

      lseek(lzhfile,pos,SEEK_SET);
      lseek (lzhfile, (long)arjhead.size + (long)arjhead.entryname + (long)sizeof (struct _arj_basic_header), SEEK_CUR);
      read(lzhfile,fname,14);

      if (arjhead.file_type == 0 || arjhead.file_type == 1)
      {
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

      free(fname);

      lseek(lzhfile,pos,SEEK_SET);
      lseek(lzhfile,bhead.size + 8,SEEK_CUR);
      read(lzhfile,(char *)&extended,2);
      if (extended > 0)
        lseek(lzhfile,extended + 4,SEEK_CUR);
      lseek(lzhfile,arjhead.compressed_size,SEEK_CUR);
    }

    if ((line=more_question(line)) == 0 || !CARRIER)
      break;
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


