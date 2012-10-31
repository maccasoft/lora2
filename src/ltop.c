#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <sys\stat.h>

#include "lora.h"
#include "version.h"

struct _sorter {
   long thing;
   long pos;
};

struct _sorter *s_ul_times;
struct _sorter *s_dl_times;
struct _sorter *s_ul_kbytes;
struct _sorter *s_dl_kbytes;
struct _sorter *s_calls;
struct _sorter *s_msg_posted;

static void insert_user (struct _sorter *, long, long, long);
static void write_stats (struct _sorter *, int, int, int, char *);

void main (argc, argv)
int argc;
char *argv[];
{
   int i, ul_times, dl_times, dl_kbytes, ul_kbytes, calls, msg_posted;
   int max_users, fd, nosys;
   char name[80];
   long pos;
   struct _usr usr;

#ifdef __OS2__
   printf("\nLTOP; LoraBBS-OS/2 Top User utility, Version %s\n", LTOP_VERSION);
#else
   printf("\nLTOP; LoraBBS-DOS Top User utility, Version %s\n", LTOP_VERSION);
#endif
   printf("      Copyright (C) 1991-94 by Marco Maccaferri, All Rights Reserved\n\n");

   ul_times = dl_times = dl_kbytes = ul_kbytes = calls =  msg_posted = 0;
   max_users = 0;
   name[0] = '\0';
   nosys = 0;

   for (i=1;i<argc;i++)
   {
      if (!strnicmp (argv[i], "-U", 2))
      {
         ul_times = atoi (&argv[i][2]) + 1;
         if (ul_times > max_users)
            max_users = ul_times;
      }
      else if (!strnicmp (argv[i], "-D", 2))
      {
         dl_times = atoi (&argv[i][2]) + 1;
         if (dl_times > max_users)
            max_users = dl_times;
      }
      else if (!strnicmp (argv[i], "-S", 2)) {
         ul_kbytes = atoi (&argv[i][2]) + 1;
         if (ul_kbytes > max_users)
            max_users = ul_kbytes;
      }
      else if (!strnicmp (argv[i], "-R", 2)) {
         dl_kbytes = atoi (&argv[i][2]) + 1;
         if (dl_kbytes > max_users)
            max_users = dl_kbytes;
      }
      else if (!strnicmp (argv[i], "-C", 2)) {
         calls = atoi (&argv[i][2]) + 1;
         if (calls > max_users)
            max_users = calls;
      }
      else if (!strnicmp (argv[i], "-M", 2)) {
         msg_posted = atoi (&argv[i][2]) + 1;
         if (msg_posted > max_users)
            max_users = msg_posted;
      }
      else if (!stricmp (argv[i], "-NS"))
         nosys = 1;
      else
         strcpy (name, argv[i]);
   }

   if ( (!ul_times && !dl_times && !ul_kbytes && !dl_kbytes && !calls && !msg_posted) || argc == 1 || !name[0] ) {
      printf("Usage   : LTOP Output_FileName List_Type\n\n");
                                                                                
      printf("List type is : -Un = Upload Times\n");
      printf("               -Dn = Download Times\n");
      printf("               -Sn = Upload KiloBytes\n");
      printf("               -Rn = Download KiloBytes\n");
      printf("               -Cn = Calls\n");
      printf("               -Mn = Messages Posted\n");
      printf("               -NS = Don't list SYSOP users\n");
                                                                                
      printf("                 n = Number of users to Include in list\n\n");

      printf("Please refer to the documentation for a more complete command summary\n\n");
   }
   else {
      s_ul_times = (struct _sorter *)malloc (ul_times * sizeof (struct _sorter));
      if (s_ul_times == NULL && ul_times)
         exit (1);
      memset ((char *)s_ul_times, 0, ul_times * sizeof (struct _sorter));
      s_dl_times = (struct _sorter *)malloc (dl_times * sizeof (struct _sorter));
      if (s_dl_times == NULL && dl_times)
         exit (1);
      memset ((char *)s_dl_times, 0, dl_times * sizeof (struct _sorter));
      s_ul_kbytes = (struct _sorter *)malloc (ul_kbytes * sizeof (struct _sorter));
      if (s_ul_kbytes == NULL && ul_kbytes)
         exit (1);
      memset ((char *)s_ul_kbytes, 0, ul_kbytes * sizeof (struct _sorter));
      s_dl_kbytes = (struct _sorter *)malloc (dl_kbytes * sizeof (struct _sorter));
      if (s_dl_kbytes == NULL && dl_kbytes)
         exit (1);
      memset ((char *)s_dl_kbytes, 0, dl_kbytes * sizeof (struct _sorter));
      s_calls = (struct _sorter *)malloc (calls * sizeof (struct _sorter));
      if (s_calls == NULL && s_calls)
         exit (1);
      memset ((char *)s_calls, 0, calls * sizeof (struct _sorter));
      s_msg_posted = (struct _sorter *)malloc (msg_posted * sizeof (struct _sorter));
      if (s_msg_posted == NULL && s_msg_posted)
         exit (1);
      memset ((char *)s_msg_posted, 0, msg_posted * sizeof (struct _sorter));

      fd = open ("USERS.BBS", O_RDONLY|O_BINARY);
      if (fd == -1)
         exit (2);

      pos = tell (fd);

      while (read (fd, (char *)&usr, sizeof (struct _usr)) == sizeof (struct _usr)) {
         if (!usr.name[0] || usr.deleted)
            continue;
         if (nosys && usr.priv == SYSOP)
            continue;

         if (ul_times)
            insert_user (s_ul_times, (long)ul_times, (long)usr.n_upld, pos);
         if (dl_times)
            insert_user (s_dl_times, (long)dl_times, (long)usr.n_dnld, pos);
         if (dl_kbytes)
            insert_user (s_dl_kbytes, (long)dl_kbytes, (long)usr.dnld, pos);
         if (ul_kbytes)
            insert_user (s_ul_kbytes, (long)ul_kbytes, (long)usr.upld, pos);
         if (calls)
            insert_user (s_calls, (long)calls, (long)usr.times, pos);
         if (msg_posted)
            insert_user (s_msg_posted, (long)msg_posted, (long)usr.msgposted, pos);

         pos = tell (fd);
      }

      unlink (name);

      if (ul_times)
         write_stats (s_ul_times, ul_times, 1, fd, name);
      if (dl_times)
         write_stats (s_dl_times, dl_times, 2, fd, name);
      if (ul_kbytes)
         write_stats (s_ul_kbytes, ul_kbytes, 3, fd, name);
      if (dl_kbytes)
         write_stats (s_dl_kbytes, dl_kbytes, 4, fd, name);
      if (calls)
         write_stats (s_calls, calls, 5, fd, name);
      if (msg_posted)
         write_stats (s_msg_posted, msg_posted, 6, fd, name);

      close (fd);

      free (s_ul_times);
      free (s_dl_times);
      free (s_ul_kbytes);
      free (s_dl_kbytes);
      free (s_calls);
      free (s_msg_posted);
   }
}

int sort_func (const void *a1, const void *b1)
{
   struct _sorter *a, *b;
   a = (struct _sorter *)a1;
   b = (struct _sorter *)b1;
   return ( (int)(b->thing - a->thing) );
}

static void insert_user (srt, max, thing, pos)
struct _sorter *srt;
long max, thing, pos;
{
   int i, p;
   long val;

   val = LONG_MAX;
   p = -1;

   for (i = 0; i < (int)max; i++)
      if (srt[i].thing < val) {
         val = srt[i].thing;
         p = i;
      }

   if (p == -1)
      return;

   srt[p].thing = thing;
   srt[p].pos = pos;

   qsort (srt, (int)max, sizeof (struct _sorter), sort_func);
}

static void write_stats (srt, max, type, fd, name)
struct _sorter *srt;
int max, type, fd;
char *name;
{
   FILE *fp;
   int i;
   long total;
   float pc;
   struct _usr usr;

   total = 0L;

   for (i = 0; i < max; i++)
      total += srt[i].thing;

   fp = fopen (name, "at");

   fprintf (fp, "\n\n");
#ifdef __OS2__
   fprintf (fp, "N   Generated by LoraTop-OS2 %s Copyright (c) 1991-94 by Marco Maccaferri\n\n", LTOP_VERSION);
#else
   fprintf (fp, "N   Generated by LoraTop-DOS %s Copyright (c) 1991-94 by Marco Maccaferri\n\n", LTOP_VERSION);
#endif
   fprintf (fp, "  ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿\n");
   fprintf (fp, "  ³ ŠNo.  Name                          ");
   if (type == 1)
      fprintf (fp, "     Uploads");
   if (type == 2)
      fprintf (fp, "   Downloads");
   if (type == 3)
      fprintf (fp, "  Uploads KB");
   if (type == 4)
      fprintf (fp, "Downloads KB");
   if (type == 5)
      fprintf (fp, "       Calls");
   if (type == 6)
      fprintf (fp, "    Messages");
   fprintf (fp, "      Percents of  %-6ld ³\n", total);
   fprintf (fp, "  ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´\n");

   max--;

   for (i = 0; i < max; i++)
   {
      lseek (fd, srt[i].pos, SEEK_SET);
      read (fd, (char *)&usr, sizeof (struct _usr));
      if (total)
         pc = ((float)srt[i].thing * 100) / total;
      else
         pc = 0;
      fprintf (fp, "  ³ %2d.  ‰%-36s %6ld         %4.1f%%           ³\n", i + 1, usr.name, srt[i].thing, pc);
   }

   fprintf (fp, "  ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ\n\n");
   fprintf (fp, "                            \n");
   fclose (fp);
}

