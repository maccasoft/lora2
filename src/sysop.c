#include <stdio.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlstr.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"

static void display_user_mask (void);
static void display_user_parameters (int, struct _usr *, int, int);
static long save_read_user (int, long, long, struct _usr *);

static void display_user_mask ()
{
   cls ();

   m_print("\n\n\n\n  user number\n\n");

   m_print("    user Name\n");
   m_print("         City\n");
   m_print("     Password\n");
   m_print(";phone number                        'handle\n");
   m_print("   priV level                   Time on-line                 # of calls\n");
   m_print("      Uploads      K                Dl (all)      K          dL (today)      K\n");
   m_print("        Flags\n");
}

#define USR_NAME       1
#define USR_PASSWORD   2
#define USR_CITY       3
#define USR_UPLOADS    4
#define USR_DOWNLOADS  5
#define USR_VOICEPHONE 6
#define USR_PRIV       7
#define USR_ONLINE     8
#define USR_TIMES      9
#define USR_DOWNTODAY  10
#define USR_HANDLE     11
#define USR_FLAGS      12
#define USR_ALL        10240

static void display_user_parameters (which, sysusr, nu, cu)
int which;
struct _usr *sysusr;
int nu, cu;
{
   int x, i;
   char passwd[40];
   long flag;
     
   if (which == USR_ALL)
   {
      cpos (4, 14);
      m_print("%5d/%-10d", cu, nu);
      if (!strcmp(sysusr->name, usr.name))
         m_print("Š(Current user)");
      else
         del_line ();
      cpos (6, 56);
      m_print("%s", sysusr->ldate);
   }

   change_attr (CYAN|_BLACK);

   if (which == USR_ALL || which == USR_NAME)
   {
      cpos (6, 14);
      m_print("%-36s", sysusr->name);
   }
   if (which == USR_ALL || which == USR_CITY)
   {
      cpos (7, 14);
      m_print("%-26s", sysusr->city);
   }
   if (which == USR_ALL || which == USR_PASSWORD)
   {
      cpos (8, 14);
      strcpy (passwd, sysusr->pwd);
      strcode (passwd, sysusr->name);
      m_print("%-16s", passwd);
   }
   if (which == USR_ALL || which == USR_VOICEPHONE)
   {
      cpos (9, 14);
      m_print("%-20s", sysusr->voicephone);
   }
   if (which == USR_ALL || which == USR_HANDLE)
   {
      cpos (9, 45);
      m_print("%-36s", sysusr->handle);
   }
   if (which == USR_ALL || which == USR_PRIV)
   {
      cpos (10, 14);
      m_print("%-11s", get_priv_text(sysusr->priv));
   }
   if (which == USR_ALL || which == USR_ONLINE)
   {
      cpos (10, 45);
      m_print("%-5d", sysusr->time);
   }
   if (which == USR_ALL || which == USR_TIMES)
   {
      cpos (10, 72);
      m_print("%-5ld", sysusr->times);
   }
   if (which == USR_ALL || which == USR_UPLOADS)
   {
      cpos (11, 14);
      m_print("%5ld", sysusr->upld);
   }
   if (which == USR_ALL || which == USR_DOWNLOADS)
   {
      cpos (11, 45);
      m_print("%5ld", sysusr->dnld);
   }
   if (which == USR_ALL || which == USR_DOWNTODAY)
   {
      cpos (11, 72);
      m_print("%-5u", sysusr->dnldl);
   }
   if (which == USR_ALL || which == USR_FLAGS)
   {
      cpos (12, 14);
      del_line();
      flag = sysusr->flags;

      x = 0;

      for (i=0;i<8;i++)
      {
         if (flag & 0x80000000L)
            passwd[x++] = '0' + i;
         else
            passwd[x++] = '-';
         flag = flag << 1;
      }

      for (i=0;i<8;i++)
      {
         if (flag & 0x80000000L)
            passwd[x++] = ((i<2)?'8':'?') + i;
         else
            passwd[x++] = '-';
         flag = flag << 1;
      }

      for (i=0;i<8;i++)
      {
         if (flag & 0x80000000L)
            passwd[x++] = 'G' + i;
         else
            passwd[x++] = '-';
         flag = flag << 1;
      }

      for (i=0;i<8;i++)
      {
         if (flag & 0x80000000L)
            passwd[x++] = 'O' + i;
         else
            passwd[x++] = '-';
         flag = flag << 1;
      }

      passwd[x] = '\0';

      m_print("%s", passwd);
   }
}

void sysop_user ()
{
   int fd, cu, nu;
   char stringa[128];
   long pos;
   struct _usr tusr;

   nu = cu = 0;

   sprintf(stringa, "%s.BBS", user_file);
   fd = open(stringa,O_RDWR|O_BINARY);
   pos = save_read_user (fd, -1L, 0L, &tusr);

   nu = (int)(filelength(fd) / sizeof (struct _usr)) - 1;

   display_user_mask ();
   display_user_parameters (USR_ALL, &tusr, nu, cu);

   do {
      cpos (14, 0);
      del_line ();
      change_attr (WHITE|_BLACK);
      m_print("Select: ");

      input (stringa, 20);
      stringa[0] = toupper(stringa[0]);
      if (!CARRIER || time_remain() <= 0)
      {
         close (fd);
         return;
      }

      change_attr (CYAN|_BLACK);
      cpos (14, 0);
      del_line ();

      if (isdigit(stringa[0]) && atoi(stringa) >= 0 && atoi(stringa) <= nu)
      {
         if (!strcmp(tusr.name, usr.name))
            memcpy ((char *)&usr, (char *)&tusr, sizeof (struct _usr));
         pos = save_read_user (fd, pos, (long)sizeof (struct _usr) * atoi(stringa), &tusr);
         cu = atoi(stringa);
         display_user_parameters (USR_ALL, &tusr, nu, cu);
         continue;
      }

      if (strlen(stringa) > 3)
      {
         save_read_user (fd, pos, -1L, &tusr);
         lseek (fd, 0L, SEEK_SET);

         while (read (fd, (char *)&tusr, sizeof (struct _usr)) == sizeof (struct _usr))
         {
            if ( !strncmpi(tusr.name, stringa, strlen (stringa)) )
               break;
         }

         if (tell(fd) == filelength (fd))
         {
            pos = save_read_user (fd, -1L, 0L, &tusr);
            cu = 0;
         }
         else
         {
            pos = tell (fd) - sizeof (struct _usr);
            cu = (int)(pos / sizeof (struct _usr));
            display_user_parameters (USR_ALL, &tusr, nu, cu);
         }

         stringa[0] = '\0';
         continue;
      }

      switch (stringa[0])
      {
         case '+':
            if (cu < nu)
            {
               cu++;
               if (!strcmp(tusr.name, usr.name))
                  memcpy ((char *)&usr, (char *)&tusr, sizeof (struct _usr));
               pos = save_read_user (fd, pos, pos + sizeof (struct _usr), &tusr);
               display_user_parameters (USR_ALL, &tusr, nu, cu);
            }
            break;
         case '-':
            if (cu > 0)
            {
               cu--;
               if (!strcmp(tusr.name, usr.name))
                  memcpy ((char *)&usr, (char *)&tusr, sizeof (struct _usr));
               pos = save_read_user (fd, pos, pos - sizeof (struct _usr), &tusr);
               display_user_parameters (USR_ALL, &tusr, nu, cu);
            }
            break;
         case 'N':
            m_print("Name: ");
            fancy_input (stringa, 36);
            if (stringa[0])
            {
               strcode(tusr.pwd, tusr.name);
               strcpy(tusr.name, stringa);
               strcode(tusr.pwd, tusr.name);
               display_user_parameters (USR_NAME, &tusr, nu, cu);
            }
            break;
         case '\'':
            m_print("Handle: ");
            fancy_input (stringa, 36);
            if (stringa[0])
            {
               strcpy(tusr.handle, stringa);
               display_user_parameters (USR_HANDLE, &tusr, nu, cu);
            }
            break;
         case ';':
            m_print("Voicephone: ");
            fancy_input (stringa, 19);
            if (stringa[0])
            {
               strcpy(tusr.voicephone, stringa);
               display_user_parameters (USR_VOICEPHONE, &tusr, nu, cu);
            }
            break;
         case 'V':
            m_print("Privilege: ");
            fancy_input (stringa, 11);
            if (stringa[0])
            {
               tusr.priv = get_priv (stringa);
               display_user_parameters (USR_PRIV, &tusr, nu, cu);
            }
            break;
         case 'F':
            m_print("Flags: ");
            fancy_input (stringa, 32);
            if (stringa[0])
            {
               strupr (stringa);
               tusr.flags = get_flags (stringa);
               display_user_parameters (USR_FLAGS, &tusr, nu, cu);
            }
            break;
         case 'U':
            m_print("Uploads: ");
            input (stringa, 6);
            if (stringa[0])
            {
               tusr.upld = atol (stringa);
               display_user_parameters (USR_UPLOADS, &tusr, nu, cu);
            }
            break;
         case 'D':
            m_print("Downloads: ");
            input (stringa, 6);
            if (stringa[0])
            {
               tusr.dnld = atol (stringa);
               display_user_parameters (USR_DOWNLOADS, &tusr, nu, cu);
            }
            break;
         case 'L':
            m_print("Downloads today: ");
            input (stringa, 6);
            if (stringa[0])
            {
               tusr.dnldl = atoi (stringa);
               display_user_parameters (USR_DOWNTODAY, &tusr, nu, cu);
            }
            break;
         case 'K':
            memset ((char *)&tusr, 0, sizeof (struct _usr));
            display_user_parameters (USR_ALL, &tusr, nu, cu);
            break;
         case 'P':
            m_print("Password: ");
            inpwd (stringa, 16);
            if (stringa[0])
            {
               strcpy(tusr.pwd, stringa);
               strcode(tusr.pwd, tusr.name);
               display_user_parameters (USR_PASSWORD, &tusr, nu, cu);
            }
            break;
      }
   } while (stringa[0] != 'Q');

   save_read_user (fd, pos, -1L, &tusr);

   close (fd);
}

static long save_read_user (fd, psave, pread, usrp)
int fd;
long psave, pread;
struct _usr *usrp;
{
   if (psave != -1)
   {
      lseek (fd, psave, SEEK_SET);
      write (fd, (char *)usrp, sizeof (struct _usr));
   }

   if (pread != -1)
   {
      lseek (fd, pread, SEEK_SET);
      read (fd, (char *)usrp, sizeof (struct _usr));
   }

   return (pread);
}

