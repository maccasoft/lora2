
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
#include <conio.h>
#include <stdlib.h>
#include <dir.h>
#include <io.h>
#include <fcntl.h>
#include <alloc.h>
#include <dos.h>
#include <string.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>

#include "lsetup.h"
#include "version.h"

#define OLD_SIZEOF_FILEAREA   256

int fdin, fdout = -1;

#define UNPACK    1
#define MKDIR     2
#define TARGET    3
#define OVERWRITE 4
#define UPGRADE   5
#define ERASE     6
#define STORED    7
#define EXECUTE   8
#define STAMPDATE 9

typedef struct {
   short action;
   short fid;
   char  filename[14];
   unsigned short ftime;
   unsigned short fdate;
   long complen;
} COMPREC;

long fsize;

void proportional (void)
{
   unsigned m;
   char filename[80];

   memset (filename, 'Û', 77);
   m = (int)(tell (fdin) * 77L / filelength (fdin));
   filename[m] = '\0';
   gotoxy (3, 14);
   cprintf (filename);
}

#ifdef __OS2__
unsigned ReadBuff (char *buff, unsigned short int *size)
{
   unsigned rs, m;

   rs = *size;
   if (rs > fsize)
      rs = (unsigned )fsize;
   fsize -= rs;

   m = read (fdin, buff, rs);
   proportional ();

   return (m);
}

unsigned WriteBuff (char *buff, unsigned short int *size)
{
   return (write (fdout, buff, *size));
}
#else
unsigned far pascal ReadBuff (char far *buff, unsigned short int far *size)
{
   unsigned rs, m;

   rs = *size;
   if (rs > fsize)
      rs = (unsigned )fsize;
   fsize -= rs;

   m = read (fdin, buff, rs);
   proportional ();

   return (m);
}

unsigned far pascal WriteBuff (char far *buff, unsigned short int far *size)
{
   return (write (fdout, buff, *size));
}
#endif

void store_file (char *work_buff)
{
   unsigned short int i;
   unsigned short int size = 12574;

   do {
      i = ReadBuff (work_buff, &size);
      WriteBuff (work_buff, &i);
   } while (i == size);
}

struct _configuration config;

void main (int argc, char *argv[])
{
   int overwrite = 1, skipupgrade = 0;
   char filename[80], tdir[14];
   char *WorkBuff;
   long stublen = -1L;
   COMPREC cr;

   if (argc);
   WorkBuff = (char *)malloc(12574);

   fdin = open ("CONFIG.DAT", O_RDONLY|O_BINARY);
   if (fdin != -1) {
      skipupgrade = 1;
      close (fdin);
   }

   if ((fdin = open (argv[0], O_RDONLY|O_BINARY)) == -1)
      return;

   hidecur ();
   clrscr ();

//#ifdef __OS2__
//   cprintf ("          LoraBBS-OS/2 v%s - The Computer-Based Information System\r\n", INSTALL_VERSION);
//#else
//   cprintf ("          LoraBBS-DOS v%s - The Computer-Based Information System\r\n", INSTALL_VERSION);
//#endif
#ifdef __OS2__
   cprintf ("          LoraBBS-OS/2 v%s - The Computer-Based Information System\r\n", "2.99.41");
#else
   cprintf ("          LoraBBS-DOS v%s - The Computer-Based Information System\r\n", "2.99.41");
#endif
   cprintf ("        CopyRight (c) 1997-97 by Marco Maccaferri. All Rights Reserved\r\n");
   cprintf ("ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ");
   gotoxy (1, 24);
   cprintf ("ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ");
   gotoxy (30, 9);
   cprintf ("SOFTWARE INSTALLATION");
   gotoxy (1, 12);
   cprintf ("ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿");
   cprintf ("³ 0%                                  50%                                 100% ³");
   cprintf ("³                                                                              ³");
   cprintf ("³                                                                              ³");
   cprintf ("ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ");

   gotoxy (3, 14);
   cprintf ("±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±");

   lseek (fdin, filelength (fdin) - 4L, SEEK_SET);
   read (fdin, &stublen, 4);

   lseek (fdin, stublen, SEEK_SET);

   for (;;) {
      if (read (fdin, &cr, sizeof (COMPREC)) < sizeof (COMPREC))
        break;

      if (cr.action == MKDIR) {
         if (!overwrite && skipupgrade)
            proportional ();
         else {
            gotoxy (1, 25);
            sprintf (filename, "Creating directory: %s", cr.filename);
            cprintf ("%-78.78s", filename);

            mkdir (cr.filename);
            strcpy (tdir, cr.filename);
         }
      }

      else if (cr.action == EXECUTE) {
         gotoxy (1, 25);
         sprintf (filename, "Executing: %s", cr.filename);
         cprintf ("%-78.78s", filename);

         gotoxy (1, 16);
         sprintf (filename, "%s", cr.filename);
         system (filename);

         gotoxy (1, 17);
         cprintf ("                                                                                ");
         cprintf ("                                                                                ");
         cprintf ("                                                                                ");
         cprintf ("                                                                                ");
         cprintf ("                                                                                ");
         cprintf ("                                                                                ");
         cprintf ("                                                                                ");
      }

      else if (cr.action == ERASE) {
         _chmod (filename, 1, (_chmod (filename, 0) & ~(FA_HIDDEN|FA_RDONLY)));
         unlink (cr.filename);
      }

      else if (cr.action == UPGRADE)
         overwrite = 1;

      else if (cr.action == OVERWRITE)
         overwrite = 0;

      else if (cr.action == STORED) {
         if (!overwrite && skipupgrade) {
            lseek (fdin, cr.complen, SEEK_CUR);
            proportional ();
         }
         else {
            gotoxy (1, 25);
            sprintf (filename, "Unpacking: %s", cr.filename);
            cprintf ("%-78.78s", filename);

            sprintf (filename, "%s\\%s", tdir, cr.filename);
            _chmod (filename, 1, (_chmod (filename, 0) & ~(FA_HIDDEN|FA_RDONLY)));
            if (overwrite)
               fdout = open (filename, O_WRONLY|O_BINARY|O_TRUNC|O_CREAT, S_IREAD|S_IWRITE);
            else
               fdout = open (filename, O_WRONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
            if (!overwrite && filelength (fdout) > 0L) {
               close (fdout);
               lseek (fdin, cr.complen, SEEK_CUR);
               proportional ();
               continue;
            }
            fsize = cr.complen;
            store_file (WorkBuff);

            _dos_setftime (fdout, cr.fdate, cr.ftime);
            close (fdout);
         }
      }

/*
      else if (cr.action == UNPACK) {
#ifndef __OS2__
         if (!overwrite && skipupgrade) {
            lseek (fdin, cr.complen, SEEK_CUR);
            proportional ();
         }
         else {
            sprintf (filename, "%s\\%s", tdir, cr.filename);
            _chmod (filename, 1, _chmod (filename, 0, 0) & ~(FA_HIDDEN|FA_RDONLY));
            if (overwrite)
               fdout = open (filename, O_WRONLY|O_BINARY|O_TRUNC|O_CREAT, S_IREAD|S_IWRITE);
            else
               fdout = open (filename, O_WRONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
            if (!overwrite && filelength (fdout) > 0L) {
               lseek (fdin, cr.complen, SEEK_CUR);
               proportional ();
               continue;
            }
            fsize = cr.complen;
            explode (ReadBuff, WriteBuff, WorkBuff);

            _dos_setftime (fdout, cr.fdate, cr.ftime);
            close (fdout);
         }
#endif
      }
*/

      else if (cr.action == STAMPDATE) {
         if (overwrite || !skipupgrade) {
            sprintf (filename, "%s\\%s", tdir, cr.filename);
            _chmod (filename, 1, (_chmod (filename, 0) & ~(FA_HIDDEN|FA_RDONLY)));
            fdout = open (filename, O_WRONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
            _dos_setftime (fdout, cr.fdate, cr.ftime);
            close (fdout);
         }
      }
   }

   close (fdin);
   _chmod (argv[0], 1, _chmod (argv[0], 0, 0) & ~(FA_HIDDEN|FA_RDONLY));
   unlink (argv[0]);

/*
   i = open ("KEYFILE.DAT", O_RDONLY|O_BINARY);
   if (i != -1) {
      newkey = (struct _newkey *)malloc ((int)filelength (i));
      read (i, newkey, (int)filelength (i));
      sk = (int)filelength (i);
      nk = sk  / sizeof (struct _newkey);
      close (i);
   }
   else
      sk = nk = 0;

   p = (char *)newkey;
   for (i = 0; i < sk; i++) {
      *p ^= 0xAA;
      p++;
   }

   if ((fdin = open ("CONFIG.DAT", O_RDWR|O_BINARY)) != -1) {
      read (fdin, &config, sizeof (struct _configuration));
      lseek (fdin, 0L, SEEK_SET);
      if (config.keycode && !config.newkey_code[0]) {
         for (i = 0; i < nk; i++) {
            if (newkey[i].oldkey == config.keycode && !stricmp (config.sysop, newkey[i].name))
               break;
         }
         if (i < nk) {
            config.keycode = 0;
            strcpy (config.newkey_code, newkey[i].newkey);
            write (fdin, &config, sizeof (struct _configuration));
         }
      }
      close (fdin);
   }

   i = open ("KEYFILE.DAT", O_WRONLY|O_BINARY|O_TRUNC);
   if (i != -1)
      close (i);
   unlink ("KEYFILE.DAT");
*/

   clrscr ();
   gotoxy (1, 12);
   printf ("Installation complete. Type LORA <ÄÙ to start.");

   gotoxy (1, 24);
   showcur ();
}

