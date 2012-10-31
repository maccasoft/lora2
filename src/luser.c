#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys\stat.h>

#include "lora.h"

void Kill_Users (int, int);
void Purge_Users (void);
void Sort_Users (void);

void main (argc, argv)
int argc;
char *argv[];
{
   int i, days, level, pack, sort;

   printf("\nLUSER; LoraBBS User maintenance utility, Version 2.20\n");
   printf("       Copyright (c) 1991-92 by Marco Maccaferri, All Rights Reserved\n\n");

   pack = sort = days = level = 0;

   for (i=1;i<argc;i++)
   {
      if (!stricmp (argv[i], "-S"))
         sort = 1;
      else if (!stricmp (argv[i], "-P"))
         pack = 1;
      else if (!strnicmp (argv[i], "-D", 2))
         days = atoi (&argv[i][2]);
      else if (!strnicmp (argv[i], "-M", 2))
         level = atoi (&argv[i][2]);
   }

   if ( (!sort && !pack && !days && !level) || argc == 1 )
   {
      printf(" * Command-line parameters:\n\n");

      printf("      -S       Sort users in alphabetical order\n");
      printf("      -P       Pack user file\n");
      printf("      -D[n]    Delete users who haven't called in [n] days\n");
      printf("      -M[s]    Only purge users with security level less than [s]\n\n");

      printf(" * Please refer to the documentation for a more complete command summary\n\n");
   }
   else
   {
      printf(" * Active options:\n\n");

      if (sort)
         printf("   - Sorting users by surname\n");
      else
         printf("   - Not sorting users\n");
      if (pack)
         printf("   - Packing user file\n");
      if (days)
         printf("   - Killing users who have not called for %d days\n\n", days);
      printf ("\n");

      if (days)
         Kill_Users (days, level);
      if (sort)
         Sort_Users ();
      if (pack)
         Purge_Users ();
   }
}

void Kill_Users(day, kill_priv)
int day, kill_priv;
{
   char ch_mon[4];
   int fd, month[12], m, yr, mo, dy, days;
   struct tm *tim;
   long tempo, day_mess, day_now, pos;
   struct _usr usr;

   char *usa_month[] = {
      "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
      "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
   };

   month[0] = 31;
   month[1] = 28;
   month[2] = 31;
   month[3] = 30;
   month[4] = 31;
   month[5] = 30;
   month[6] = 31;
   month[7] = 31;
   month[8] = 30;
   month[9] = 31;
   month[10] = 30;
   month[11] = 31;

   tempo = time(0);
   tim = localtime(&tempo);

   fd = open("USERS.BBS", O_BINARY | O_RDWR);

   day_now = (long) 365 *tim->tm_year;
   for (m = 0; m < tim->tm_mon; m++)
      day_now += (long) month[m];
   day_now += (long) tim->tm_mday;

   pos = tell (fd);

   while (read(fd, (char *) &usr, sizeof(struct _usr)) == sizeof(struct _usr))
   {
      sscanf(usr.ldate, "%2d %3s %2d", &dy, ch_mon, &yr);
      ch_mon[3] = '\0';
      strupr(ch_mon);
      for (mo = 0; mo < 12; mo++)
         if (!strcmp(ch_mon, usa_month[mo]))
            break;
      if (mo == 12)
         mo = 0;

      day_mess = (long) 365 *yr;
      for (m = 0; m < mo; m++)
         day_mess += (long) month[m];
      day_mess += (long) dy;

      days = (int) (day_now - day_mess);

      if ((days > day) && (kill_priv == 0 || usr.priv < kill_priv))
      {
         usr.deleted = 1;
         lseek (fd, pos, SEEK_SET);
         write(fd, (char *) &usr, sizeof(struct _usr));
      }

      pos = tell (fd);
   }

   close(fd);
}

void Purge_Users ()
{
   int fdo, fdn, fdi;
   struct _usr usr;
   struct _usridx usridx;

   printf(" * Packing user file\n");

   unlink ("USERS.BAK");
   rename ("USERS.BBS", "USERS.BAK");

   fdo = open ("USERS.BAK", O_RDONLY|O_BINARY);
   fdn = open ("USERS.BBS", O_WRONLY|O_BINARY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
   fdi = open ("USERS.IDX", O_WRONLY|O_BINARY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);

   while (read (fdo, (char *)&usr, sizeof (struct _usr)) == sizeof (struct _usr))
   {
      if (!usr.name[0] || usr.deleted)
         continue;
      write (fdn, (char *)&usr, sizeof (struct _usr));
      usridx.id = usr.id;
      write (fdi, (char *)&usridx, sizeof (struct _usridx));
   }

   close (fdi);
   close (fdn);
   close (fdo);

   printf("\n");
}

struct _sort {
   char *name;
   long pos;
};

int sort_function (const void *a1, const void *b1)
{
   struct _sort *a, *b;
   a = (struct _sort *)a1;
   b = (struct _sort *)b1;
   return ( strcmp (a->name, b->name) );
}

void Sort_Users ()
{
   int fdo, fdn, fdi, i, m;
   char *p;
   struct _usr usr;
   struct _sort *sort;
   struct _usridx usridx;

   printf(" * Sorting user file\n");

   fdo = open ("USERS.BBS", O_RDONLY|O_BINARY);
   sort = (struct _sort *)malloc ( (int)(filelength(fdo) / sizeof (struct _usr)) * sizeof (struct _sort) );
   if (sort == NULL)
      exit (1);

   i = 0;

   while (read (fdo, (char *)&usr, sizeof (struct _usr)) == sizeof (struct _usr))
   {
      sort[i].name = (char *)malloc (strlen(usr.name)+1);
      if (sort[i].name == NULL)
         exit (1);
      p = strchr (usr.name, '\0');
      while (*p != ' ' && p != usr.name)
         p--;
      if (p != usr.name)
         *p++ = '\0';
      strcpy (sort[i].name, p);
      strcat (sort[i].name, " ");
      strcat (sort[i].name, usr.name);
      sort[i].pos = tell (fdo) - sizeof (struct _usr);
      i++;
   }

   close (fdo);

   qsort ( (void *)sort, i, sizeof (struct _sort), sort_function);

   unlink ("USERS.BAK");
   rename ("USERS.BBS", "USERS.BAK");

   fdo = open ("USERS.BAK", O_RDONLY|O_BINARY);
   fdn = open ("USERS.BBS", O_WRONLY|O_BINARY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
   fdi = open ("USERS.IDX", O_WRONLY|O_BINARY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);

   for (m = 0; m < i; m++)
   {
      lseek (fdo, sort[m].pos, SEEK_SET);
      read (fdo, (char *)&usr, sizeof (struct _usr));
      write (fdn, (char *)&usr, sizeof (struct _usr));
      usridx.id = usr.id;
      write (fdi, (char *)&usridx, sizeof (struct _usridx));
   }

   close (fdi);
   close (fdn);
   close (fdo);

   free (sort);

   printf("\n");
}


