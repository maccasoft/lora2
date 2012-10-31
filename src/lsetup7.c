
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

#include "lsetup.h"
#include "lprot.h"

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>
#include <cxl\cxlkey.h>

extern struct _configuration config;

void start_update (void);
void stop_update (void);
long window_get_flags (int y, int x, int type, long f);
char *get_priv_text (int);
int sh_open (char *file, int shmode, int omode, int fmode);

static char otherdata = 0;

unsigned long cr3tab[] = {                /* CRC polynomial 0xedb88320 */
   0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L, 0x706af48fL, 0xe963a535L, 0x9e6495a3L,
   0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L,
   0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL, 0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L,
   0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L,
   0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L, 0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
   0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L,
   0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L, 0xb8bda50fL,
   0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L, 0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL,
	0x76dc4190L, 0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L,
   0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
   0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L,
   0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L,
   0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L, 0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL,
   0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L,
   0x5005713cL, 0x270241aaL, 0xbe0b1010L, 0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
   0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL,
   0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L,
   0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L, 0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L,
   0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L,
   0xfed41b76L, 0x89d32be0L, 0x10da7a5aL, 0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
   0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL,
   0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL, 0x4669be79L,
   0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L, 0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL,
   0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL,
   0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
   0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L,
   0x86d3d2d4L, 0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L,
   0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL, 0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L,
   0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL,
   0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
   0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L, 0xcdd70693L, 0x54de5729L, 0x23d967bfL,
   0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL
};

#define Z_32UpdateCRC(c,crc) (cr3tab[((int) crc ^ c) & 0xff] ^ ((crc >> 8) & 0x00FFFFFFL))

long crc_name (char *name)
{
   int i;
   long crc;

   crc = 0xFFFFFFFFL;
   for (i=0; i < strlen (name); i++)
           crc = Z_32UpdateCRC (((unsigned short) name[i]), crc);

   return (crc);
}

typedef struct {
   char *name;
   long pos;
   long posidx;
} USRA;

static int user_sort_func (const void *a1, const void *b1)
{
   USRA *a, *b;

   a = (USRA *)a1;
   b = (USRA *)b1;

   return (strcmp (&a->name[6], &b->name[6]));
}

static void user_select_list (int fd, int fdidx, struct _usr *ousr)
{
   int wh, i, x, start;
   char string[80], **names;
   struct _usr usr;
   long pos, posidx;
   USRA *array;

   wh = wopen (2, 0, 22, 77, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Select user ", TRIGHT, YELLOW|_BLUE);

   wprints (0, 1, LGREY|_BLACK, " #    Name                               City                Level");
   whline (1, 0, 76, 0, BLUE|_BLACK);

   gotoxy_ (24, 1);
   clreol_ ();
   prints (24, 1, LGREY|_BLACK, "-Move bar  ENTER-Select");
   prints (24, 1, YELLOW|_BLACK, "");
   prints (24, 14, YELLOW|_BLACK, "ENTER");

   i = (int)(filelength (fd) / sizeof (struct _usr));
   i += 2;
   if ((array = (USRA *)malloc (i * sizeof (USRA))) == NULL)
      return;
   if ((names = (char **)malloc (i * sizeof (char *))) == NULL)
      return;

   pos = tell (fd) - sizeof (struct _usr);
   lseek (fd, 0L, SEEK_SET);
   posidx = tell (fdidx);
   lseek (fdidx, 0L, SEEK_SET);

   i = 0;
   start = -1;

   while (read (fd, (char *)&usr, sizeof (struct _usr)) == sizeof (struct _usr)) {
      lseek (fdidx, (long)sizeof (struct _usridx), SEEK_CUR);
      sprintf (string, " %-4d %-34.34s %-18.18s %-13s ", i + 1, usr.name, usr.city, get_priv_text (usr.priv));
      array[i].name = (char *)malloc (strlen (string) + 1);
      if (array[i].name == NULL)
         break;
      array[i].pos = tell (fd) - sizeof (struct _usr);
      array[i].posidx = tell (fdidx);
      strcpy (array[i++].name, string);
   }
   array[i].name = NULL;
   qsort (array, i, sizeof (USRA), user_sort_func);

   i = 0;
   while (array[i].name != NULL) {
      if (start == -1) {
         if (!strncmp (&array[i].name[6], ousr->name, strlen (ousr->name)))
            start = i;
      }
      names[i] = array[i++].name;
   }
   names[i] = NULL;
   if (start == -1)
      start = 0;

   x = wpickstr (5, 2, 21, 75, 5, LGREY|_BLACK, CYAN|_BLACK, BLUE|_LGREY, names, start, NULL);

   if (x == -1) {
      lseek (fd, pos, SEEK_SET);
      lseek (fdidx, posidx, SEEK_SET);
   }
   else {
      lseek (fd, array[x].pos, SEEK_SET);
      lseek (fdidx, array[x].posidx, SEEK_SET);
   }
   read (fd, (char *)ousr, sizeof (struct _usr));

   wclose ();

   i = 0;
   while (array[i].name != NULL)
      free (array[i++].name);
   free (array);
   free (names);

   gotoxy_ (24, 1);
   clreol_ ();
   prints (24, 1, LGREY|_BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add New User  L-List  D-Delete");
   prints (24, 1, YELLOW|_BLACK, "PgUp/PgDn");
   prints (24, 26, YELLOW|_BLACK, "E");
   prints (24, 34, YELLOW|_BLACK, "A");
   prints (24, 50, YELLOW|_BLACK, "L");
   prints (24, 58, YELLOW|_BLACK, "D");
//   prints (24, 68, YELLOW|_BLACK, "O");
}

void edit_archiver (struct _usr *usr)
{
   int i, dim, start = 255;
   char packs[MAXPACKER][30];

   for (i = 0, dim = 0; i < MAXPACKER; i++)
      if (config.packid[i].display[0])
         dim++;

   wopen (4, 22, dim + 8, 46, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wshadow (DGREY|_BLACK);
   wtitle (" Select archiver ", TRIGHT, YELLOW|_BLUE);

   wmenubegc ();
   wmenuitem ( 1,  1, " None               ", 0, 255, 0, NULL, 0, 0);

   for (i = 0, dim = 1; i < MAXPACKER; i++) {
      if (config.packid[i].display[0]) {
         sprintf (packs[i], " %-18.18s ", &config.packid[i].display[1]);
         wmenuitem ( dim + 1,  1, packs[i], 0, i + 1, 0, NULL, 0, 0);
         dim++;

         if (toupper (config.packid[i].display[0]) == toupper (usr->archiver))
            start = i + 1;
      }
   }
   wmenuend (start, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

   i = wmenuget ();

   if (i != -1) {
      if (i == 255)
         usr->archiver = '\0';
      else
         usr->archiver = toupper (config.packid[i - 1].display[0]);
   }

   wclose ();
}

void edit_language (struct _usr *usr)
{
   int i, dim;
   char packs[MAX_LANG][30];

   for (i = 0, dim = 0; i < MAX_LANG; i++)
      if (config.language[i].descr[0])
         dim++;

   wopen (4, 22, dim + 7, 46, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wshadow (DGREY|_BLACK);
   wtitle (" Select language ", TRIGHT, YELLOW|_BLUE);

   wmenubegc ();

   for (i = 0, dim = 0; i < MAX_LANG; i++) {
      if (config.language[i].descr[0]) {
         sprintf (packs[i], " %-18.18s ", &config.language[i].descr[1]);
         wmenuitem ( dim + 1,  1, packs[i], 0, i + 1, 0, NULL, 0, 0);
         dim++;
      }
   }
   wmenuend (usr->language + 1, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

	i = wmenuget ();

	if (i != -1)
		usr->language = i - 1;

	wclose ();
}

void edit_single_user (struct _usr *usr)
{
	int i = 1, wh1, m;
	char string[128];
	struct _usr nusr;

	memcpy ((char *)&nusr, (char *)usr, sizeof (struct _usr));

	gotoxy_ (24, 1);
	clreol_ ();
	prints (24, 1, LGREY|_BLACK, "ESC-Exit/Save  ENTER-Edit");
	prints (24, 1, YELLOW|_BLACK, "ESC");
	prints (24, 16, YELLOW|_BLACK, "ENTER");

continue_editing:
	do {
		stop_update ();
		wclear ();

		wmenubegc ();
		wmenuitem ( 1,  1, " Name       ", 0,  1, 0, NULL, 0, 0);
		wmenuitem ( 2,  1, " Alias      ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " City       ", 0,  3, 0, NULL, 0, 0);
      wmenuitem ( 3, 43, " First call ", 0,  5, 0, NULL, 0, 0);

      if (!otherdata) {
			wmenuitem ( 4,  1, " Password   ", 0,  4, 0, NULL, 0, 0);
         wmenuitem ( 4, 43, " Last call  ", 0,  6, 0, NULL, 0, 0);
         wmenuitem ( 5,  1, " Level      ", 0,  7, 0, NULL, 0, 0);
         wmenuitem ( 5, 26, " Mail scan  ", 0, 19, 0, NULL, 0, 0);
         wmenuitem ( 5, 43, " Archiver   ", 0, 40, 0, NULL, 0, 0);
         wmenuitem ( 6,  1, " A Flags    ", 0,  8, 0, NULL, 0, 0);
         wmenuitem ( 6, 26, " Editor FS  ", 0, 20, 0, NULL, 0, 0);
         wmenuitem ( 6, 43, " Nulls      ", 0, 30, 0, NULL, 0, 0);
         wmenuitem ( 7,  1, " B Flags    ", 0,  9, 0, NULL, 0, 0);
         wmenuitem ( 7, 26, " More?      ", 0, 21, 0, NULL, 0, 0);
         wmenuitem ( 7, 43, " Language   ", 0, 31, 0, NULL, 0, 0);
         wmenuitem ( 8,  1, " C Flags    ", 0, 10, 0, NULL, 0, 0);
         wmenuitem ( 8, 26, " CLS        ", 0, 22, 0, NULL, 0, 0);
         wmenuitem ( 8, 43, " Minutes    ", 0, 32, 0, NULL, 0, 0);
         wmenuitem ( 9,  1, " D Flags    ", 0, 11, 0, NULL, 0, 0);
         wmenuitem ( 9, 26, " Hot keys   ", 0, 23, 0, NULL, 0, 0);
         wmenuitem ( 9, 43, " Credit     ", 0, 33, 0, NULL, 0, 0);
         wmenuitem (10,  1, " Video      ", 0, 12, 0, NULL, 0, 0);
         wmenuitem (10, 26, " Reader FS  ", 0, 24, 0, NULL, 0, 0);
         wmenuitem (10, 43, " Voice pho. ", 0, 34, 0, NULL, 0, 0);
         wmenuitem (11,  1, " юд Colors  ", 0, 13, 0, NULL, 0, 0);
         wmenuitem (11, 26, " IBM chars. ", 0, 25, 0, NULL, 0, 0);
         wmenuitem (11, 43, " Data phone ", 0, 35, 0, NULL, 0, 0);
         wmenuitem (12,  1, " Calls #    ", 0, 14, 0, NULL, 0, 0);
         wmenuitem (12, 26, " Hidden     ", 0, 26, 0, NULL, 0, 0);
         wmenuitem (12, 43, " Birthdate  ", 0, 36, 0, NULL, 0, 0);
         wmenuitem (13,  1, " Last Msg.  ", 0, 17, 0, NULL, 0, 0);
         wmenuitem (13, 26, " No kill    ", 0, 27, 0, NULL, 0, 0);
         wmenuitem (13, 43, " Expiration ", 0, 37, 0, NULL, 0, 0);
         wmenuitem (14,  1, " Last File  ", 0, 18, 0, NULL, 0, 0);
         wmenuitem (14, 26, " Priority   ", 0, 28, 0, NULL, 0, 0);
         wmenuitem (14, 43, " Length     ", 0, 38, 0, NULL, 0, 0);
         wmenuitem (15, 43, " Width      ", 0, 39, 0, NULL, 0, 0);
         wmenuitem (15, 26, " File box   ", 0, 29, 0, NULL, 0, 0);
         wmenuitem (15,  1, " Protocol   ", 0, 41, 0, NULL, 0, 0);
			wmenuitem (16,  1, " Signature  ", 0, 15, 0, NULL, 0, 0);
			wmenuitem (17,  1, " Comment    ", 0, 16, 0, NULL, 0, 0);
      }
      else {
         wmenuitem ( 4,  1, " Pwd change ", 0, 50, 0, NULL, 0, 0);
         wmenuitem ( 5,  1, " UL Kbytes  ", 0, 51, 0, NULL, 0, 0);
         wmenuitem ( 5, 26, " UL Files   ", 0, 52, 0, NULL, 0, 0);
         wmenuitem ( 6,  1, " DL Kbytes  ", 0, 53, 0, NULL, 0, 0);
         wmenuitem ( 6, 26, " DL Files   ", 0, 54, 0, NULL, 0, 0);
         wmenuitem ( 7,  1, " Quiet      ", 0, 55, 0, NULL, 0, 0);
         wmenuitem ( 7, 26, " Time call  ", 0, 56, 0, NULL, 0, 0);
         wmenuitem ( 8,  1, " Nerd       ", 0, 57, 0, NULL, 0, 0);
         wmenuitem ( 8, 26, " Time day   ", 0, 58, 0, NULL, 0, 0);
         wmenuitem ( 9,  1, " No disturb ", 0, 59, 0, NULL, 0, 0);
         wmenuitem ( 9, 26, " DL Limit   ", 0, 60, 0, NULL, 0, 0);
         wmenuitem (10,  1, " Security   ", 0, 61, 0, NULL, 0, 0);
         wmenuitem (10, 26, " DL Ratio   ", 0, 62, 0, NULL, 0, 0);
         wmenuitem (11,  1, " Bad Pwd    ", 0, 63, 0, NULL, 0, 0);
         wmenuitem (11, 26, " Min. speed ", 0, 64, 0, NULL, 0, 0);
         wmenuitem (12,  1, " Kludge     ", 0, 65, 0, NULL, 0, 0);
         wmenuitem (12, 26, " DL Speed   ", 0, 66, 0, NULL, 0, 0);
         wmenuitem (13,  1, " Msg. group ", 0, 67, 0, NULL, 0, 0);
         wmenuitem (13, 26, " Beg. ratio ", 0, 68, 0, NULL, 0, 0);
         wmenuitem (14,  1, " File group ", 0, 69, 0, NULL, 0, 0);
         wmenuitem (15,  1, " Chat mins. ", 0, 71, 0, NULL, 0, 0);
         wmenuitem (15, 26, " Msg Posted ", 0, 72, 0, NULL, 0, 0);
         wmenuitem (16,  1, " TimeBank   ", 0, 73, 0, NULL, 0, 0);
         wmenuitem (16, 26, " FileBank   ", 0, 74, 0, NULL, 0, 0);
         wmenuitem (17,  1, " Last baud  ", 0, 75, 0, NULL, 0, 0);
         wmenuitem (17, 26, " DL Today   ", 0, 76, 0, NULL, 0, 0);
      }
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints ( 2, 43, LGREY|_BLACK, " FBox ID    ");

      wprints (1, 14, CYAN|_BLACK, nusr.name);
      wprints (2, 14, CYAN|_BLACK, nusr.handle);
      sprintf (string, "%08lX", nusr.id);
      wprints (2, 56, CYAN|_BLACK, string);
      wprints (3, 14, CYAN|_BLACK, nusr.city);
      wprints (3, 56, CYAN|_BLACK, nusr.firstdate);

      if (!otherdata) {
         strcpy (string, "***************");
         string[strlen (nusr.pwd)] = '\0';
         wprints (4, 14, CYAN|_BLACK, string);
         wprints (4, 56, CYAN|_BLACK, nusr.ldate);
         wprints (5, 13, CYAN|_BLACK, get_priv_text (nusr.priv));
         wprints (5, 39, CYAN|_BLACK, nusr.scanmail ? "Yes" : "No");
         for (m = 0; m < 10; m++) {
            if (!config.packid[m].display[0])
               continue;
            if (toupper (config.packid[m].display[0]) == toupper (nusr.archiver))
               break;
         }
         wprints (5, 56, CYAN|_BLACK, (m < 10) ? &config.packid[m].display[1] : "None");
         wprints (6, 14, CYAN|_BLACK, get_flagA_text ((nusr.flags >> 24) & 0xFF));
         wprints (6, 39, CYAN|_BLACK, !nusr.use_lore ? "Yes" : "No");
         sprintf (string, "%d", nusr.nulls);
         wprints (6, 56, CYAN|_BLACK, string);
         wprints (7, 14, CYAN|_BLACK, get_flagB_text ((nusr.flags >> 16) & 0xFF));
         wprints (7, 39, CYAN|_BLACK, nusr.more ? "Yes" : "No");
         if (!config.language[nusr.language].descr[0])
            nusr.language = 0;
         wprints (7, 56, CYAN|_BLACK, &config.language[nusr.language].descr[1]);
         wprints (8, 14, CYAN|_BLACK, get_flagC_text ((nusr.flags >> 8) & 0xFF));
         wprints (8, 39, CYAN|_BLACK, nusr.formfeed ? "Yes" : "No");
         sprintf (string, "%d", nusr.time);
         wprints (8, 56, CYAN|_BLACK, string);
         wprints (9, 14, CYAN|_BLACK, get_flagD_text (nusr.flags & 0xFF));
         wprints (9, 39, CYAN|_BLACK, nusr.hotkey ? "Yes" : "No");
         sprintf (string, "%d", nusr.credit);
         wprints (9, 56, CYAN|_BLACK, string);
         if (nusr.ansi)
            wprints (10, 14, CYAN|_BLACK, "Ansi");
         else if (nusr.avatar)
            wprints (10, 14, CYAN|_BLACK, "Avatar");
         else
            wprints (10, 14, CYAN|_BLACK, "TTY");
         wprints (10, 39, CYAN|_BLACK, nusr.full_read ? "Yes" : "No");
         wprints (10, 56, CYAN|_BLACK, nusr.voicephone);
         wprints (11, 14, CYAN|_BLACK, nusr.color ? "Yes" : "No");
         wprints (11, 39, CYAN|_BLACK, nusr.ibmset ? "Yes" : "No");
         wprints (11, 56, CYAN|_BLACK, nusr.dataphone);
         sprintf (string, "%ld", nusr.times);
         wprints (12, 14, CYAN|_BLACK, string);
         wprints (12, 39, CYAN|_BLACK, nusr.usrhidden ? "Yes" : "No");
         wprints (12, 56, CYAN|_BLACK, nusr.birthdate);
         sprintf (string, "%u", nusr.msg);
         wprints (13, 14, CYAN|_BLACK, string);
         wprints (13, 39, CYAN|_BLACK, nusr.nokill ? "Yes" : "No");
         wprints (13, 56, CYAN|_BLACK, nusr.subscrdate);
         sprintf (string, "%u", nusr.files);
         wprints (14, 14, CYAN|_BLACK, string);
         wprints (14, 39, CYAN|_BLACK, nusr.xfer_prior ? "Yes" : "No");
         sprintf (string, "%d", nusr.len);
         wprints (14, 56, CYAN|_BLACK, string);
         sprintf (string, "%d", nusr.width);
         wprints (15, 56, CYAN|_BLACK, string);
         wprints (15, 39, CYAN|_BLACK, nusr.havebox ? "Yes" : "No");
         sprintf (string, "%c", nusr.protocol);
         wprints (15, 14, CYAN|_BLACK, string);
			wprints (16, 14, CYAN|_BLACK, nusr.signature);
			wprints (17, 14, CYAN|_BLACK, nusr.comment);
      }
      else {
         wprints ( 4, 14, CYAN|_BLACK, nusr.lastpwdchange);
         sprintf (string, "%lu", nusr.upld);
         wprints ( 5, 14, CYAN|_BLACK, string);
         sprintf (string, "%u", nusr.n_upld);
         wprints ( 5, 39, CYAN|_BLACK, string);
         sprintf (string, "%lu", nusr.dnld);
         wprints ( 6, 14, CYAN|_BLACK, string);
         sprintf (string, "%u", nusr.n_dnld);
         wprints ( 6, 39, CYAN|_BLACK, string);
         wprints ( 7, 14, CYAN|_BLACK, nusr.quiet ? "Yes" : "No");
         sprintf (string, "%u", nusr.ovr_class.max_call);
         wprints ( 7, 39, CYAN|_BLACK, string);
         wprints ( 8, 14, CYAN|_BLACK, nusr.nerd ? "Yes" : "No");
         sprintf (string, "%u", nusr.ovr_class.max_time);
         wprints ( 8, 39, CYAN|_BLACK, string);
         wprints ( 9, 14, CYAN|_BLACK, nusr.donotdisturb ? "Yes" : "No");
         sprintf (string, "%u", nusr.ovr_class.max_dl);
         wprints ( 9, 39, CYAN|_BLACK, string);
         wprints (10, 14, CYAN|_BLACK, nusr.security ? "Yes" : "No");
         sprintf (string, "%u", nusr.ovr_class.ratio);
         wprints (10, 39, CYAN|_BLACK, string);
         wprints (11, 14, CYAN|_BLACK, nusr.badpwd ? "Yes" : "No");
         sprintf (string, "%u", nusr.ovr_class.min_baud);
         wprints (11, 39, CYAN|_BLACK, string);
         wprints (12, 14, CYAN|_BLACK, nusr.kludge ? "Yes" : "No");
         sprintf (string, "%u", nusr.ovr_class.min_file_baud);
         wprints (12, 39, CYAN|_BLACK, string);
         sprintf (string, "%u", nusr.msg_sig);
         wprints (13, 14, CYAN|_BLACK, string);
         sprintf (string, "%u", nusr.ovr_class.start_ratio);
         wprints (13, 39, CYAN|_BLACK, string);
         sprintf (string, "%u", nusr.file_sig);
         wprints (14, 14, CYAN|_BLACK, string);
         sprintf (string, "%u", nusr.chat_minutes);
         wprints (15, 14, CYAN|_BLACK, string);
         sprintf (string, "%u", nusr.msgposted);
         wprints (15, 39, CYAN|_BLACK, string);
         sprintf (string, "%u", nusr.account);
         wprints (16, 14, CYAN|_BLACK, string);
         sprintf (string, "%u", nusr.f_account);
         wprints (16, 39, CYAN|_BLACK, string);
         sprintf (string, "%lu", nusr.baud_rate);
         wprints (17, 14, CYAN|_BLACK, string);
         sprintf (string, "%u", nusr.dnldl);
         wprints (17, 39, CYAN|_BLACK, string);
      }

      if (nusr.deleted)
         wprints (1, 62, WHITE|_BLACK, "* DELETED *");

      start_update ();

      i = wmenuget ();

      switch (i) {
         case 1:
            strcpy (string, nusr.name);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (1, 14, string, "???????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcode (nusr.pwd, nusr.name);
               strcpy (nusr.name, strbtrim (string));
               strcode (nusr.pwd, nusr.name);
               nusr.id = crc_name (nusr.name);
               nusr.alias_id = crc_name (nusr.handle);
            }
            break;

         case 2:
            strcpy (string, nusr.handle);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (2, 14, string, "???????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nusr.handle, strbtrim (string));
            break;

         case 3:
            strcpy (string, nusr.city);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 14, string, "?????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nusr.city, strbtrim (string));
            break;

         case 4:
            strcode (nusr.pwd, nusr.name);
            strcpy (string, strupr (nusr.pwd));
            strcode (nusr.pwd, nusr.name);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 14, string, "???????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               strcpy (nusr.pwd, strupr (strbtrim (string)));
               strcode (nusr.pwd, nusr.name);
            }
            break;

         case 5:
            strcpy (string, nusr.firstdate);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 56, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nusr.firstdate, strbtrim (string));
            break;

         case 6:
            strcpy (string, nusr.ldate);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 56, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nusr.ldate, strbtrim (string));
            break;

         case 7:
            nusr.priv = select_level (nusr.priv, 47, 4);
            break;

         case 8:
            nusr.flags = window_get_flags (6, 46, 1, nusr.flags);
            break;

         case 9:
            nusr.flags = window_get_flags (6, 46, 2, nusr.flags);
            break;

         case 10:
            nusr.flags = window_get_flags (6, 46, 3, nusr.flags);
            break;

         case 11:
            nusr.flags = window_get_flags (6, 46, 4, nusr.flags);
            break;

         case 12:
            if (nusr.ansi) {
               nusr.ansi = 0;
               nusr.avatar = 1;
            }
            else if (nusr.avatar)
               nusr.avatar = 0;
            else
               nusr.ansi = 1;
            break;

         case 13:
            nusr.color ^= 1;
            break;

         case 14:
            sprintf (string, "%ld", nusr.times);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (12, 14, string, "???????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.times = atol (string);
            break;

         case 15:
				strcpy (string, nusr.signature);
				winpbeg (BLUE|_GREEN, BLUE|_GREEN);
				winpdef (16, 14, string, "?????????????????????????????????????????????????????????", 0, 2, NULL, 0);
				if (winpread () != W_ESCPRESS)
					strcpy (nusr.signature, strbtrim (string));
				break;

			case 16:
				strcpy (string, nusr.comment);
				winpbeg (BLUE|_GREEN, BLUE|_GREEN);
				winpdef (17, 14, string, "???????????????????????????????????????????????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nusr.comment, strbtrim (string));
            break;

         case 17:
            sprintf (string, "%d", nusr.msg);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (13, 14, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.msg = atoi (string);
            break;

         case 18:
            sprintf (string, "%d", nusr.files);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (14, 14, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.files = atoi (string);
            break;

         case 19:
            nusr.scanmail ^= 1;
            break;

         case 20:
            nusr.use_lore ^= 1;
            break;

         case 21:
            nusr.more ^= 1;
            break;

         case 22:
            nusr.formfeed ^= 1;
            break;

         case 23:
            nusr.hotkey ^= 1;
            break;

         case 24:
            nusr.full_read ^= 1;
            break;

         case 25:
            nusr.ibmset ^= 1;
            break;

         case 26:
            nusr.usrhidden ^= 1;
            break;

         case 27:
            nusr.nokill ^= 1;
            break;

         case 28:
            nusr.xfer_prior ^= 1;
            break;

         case 29:
            nusr.havebox ^= 1;
            break;

         case 30:
            sprintf (string, "%d", nusr.nulls);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (6, 56, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.nulls = atoi (string);
            break;

         case 31:
            edit_language (&nusr);
            break;

         case 32:
            sprintf (string, "%d", nusr.time);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (8, 56, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.time = atoi (string);
            break;

         case 33:
            sprintf (string, "%d", nusr.credit);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (9, 56, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.credit = atoi (string);
            break;

         case 34:
            sprintf (string, "%s", nusr.voicephone);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (10, 56, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nusr.voicephone, strbtrim (string));
            break;

         case 35:
            sprintf (string, "%s", nusr.dataphone);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (11, 56, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nusr.dataphone, strbtrim (string));
            break;

         case 36:
            sprintf (string, "%s", nusr.birthdate);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (12, 56, string, "?????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nusr.birthdate, strbtrim (string));
            break;

         case 37:
            sprintf (string, "%s", nusr.subscrdate);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (13, 56, string, "?????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nusr.subscrdate, strbtrim (string));
            break;

         case 38:
            sprintf (string, "%d", nusr.len);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (14, 56, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.len = atoi (string);
            break;

         case 39:
            sprintf (string, "%d", nusr.width);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (15, 56, string, "???", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.width = atoi (string);
            break;

         case 40:
            edit_archiver (&nusr);
            break;

         case 41:
            sprintf (string, "%c", nusr.protocol);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (15, 14, string, "?", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.protocol = toupper (string[0]);
            break;

         case 50:
            strcpy (string, nusr.lastpwdchange);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 14, string, "???????????????????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (nusr.lastpwdchange, strbtrim (string));
            break;

         case 51:
            sprintf (string, "%lu", nusr.upld);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (5, 14, string, "???????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.upld = atol (string);
            break;

         case 52:
            sprintf (string, "%u", nusr.n_upld);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (5, 39, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.n_upld = atoi (string);
            break;

         case 53:
            sprintf (string, "%lu", nusr.dnld);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (6, 14, string, "???????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.dnld = atol (string);
            break;

         case 54:
            sprintf (string, "%u", nusr.n_dnld);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (6, 39, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.n_dnld = atoi (string);
            break;

         case 55:
            nusr.quiet ^= 1;
            break;

         case 56:
            sprintf (string, "%u", nusr.ovr_class.max_call);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (7, 39, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.ovr_class.max_call = atoi (string);
            break;

         case 57:
            nusr.nerd ^= 1;
            break;

         case 58:
            sprintf (string, "%u", nusr.ovr_class.max_time);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (8, 39, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.ovr_class.max_time = atoi (string);
            break;

         case 59:
            nusr.donotdisturb ^= 1;
            break;

         case 60:
            sprintf (string, "%u", nusr.ovr_class.max_dl);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (9, 39, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.ovr_class.max_dl = atoi (string);
            break;

         case 61:
            nusr.security ^= 1;
            break;

         case 62:
            sprintf (string, "%u", nusr.ovr_class.ratio);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (10, 39, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.ovr_class.ratio = atoi (string);
            break;

         case 63:
            nusr.badpwd ^= 1;
            break;

         case 64:
            sprintf (string, "%u", nusr.ovr_class.min_baud);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (11, 39, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.ovr_class.min_baud = atoi (string);
            break;

         case 65:
            nusr.kludge ^= 1;
            break;

         case 66:
            sprintf (string, "%u", nusr.ovr_class.min_file_baud);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (12, 39, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.ovr_class.min_file_baud = atoi (string);
            break;

         case 67:
            sprintf (string, "%u", nusr.msg_sig);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (13, 14, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.msg_sig = atoi (string);
            break;

         case 68:
            sprintf (string, "%u", nusr.ovr_class.start_ratio);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (13, 39, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.ovr_class.start_ratio = atoi (string);
            break;

         case 69:
            sprintf (string, "%u", nusr.file_sig);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (14, 14, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.file_sig = atoi (string);
            break;

         case 71:
            sprintf (string, "%u", nusr.chat_minutes);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (15, 14, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.chat_minutes = atoi (string);
            break;

         case 72:
            sprintf (string, "%u", nusr.msgposted);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (15, 39, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.msgposted = atoi (string);
            break;

         case 73:
            sprintf (string, "%u", nusr.account);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (16, 14, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.account = atoi (string);
            break;

         case 74:
            sprintf (string, "%u", nusr.f_account);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (16, 39, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.f_account = atoi (string);
            break;

         case 75:
            sprintf (string, "%lu", nusr.baud_rate);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (17, 14, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.baud_rate = atol (string);
            break;

         case 76:
            sprintf (string, "%u", nusr.dnldl);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (17, 39, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS)
               nusr.dnldl = atoi (string);
            break;
      }

      hidecur ();
   } while (i != -1);

   if (memcmp ((char *)&nusr, (char *)usr, sizeof (struct _usr))) {
      wh1 = wopen (10, 25, 14, 54, 0, BLACK|_LGREY, BLACK|_LGREY);
      wactiv (wh1);
      wshadow (DGREY|_BLACK);

      wcenters (1, BLACK|_LGREY, "Save changes (Y/n) ?  ");

      strcpy (string, "Y");
      winpbeg (BLACK|_LGREY, BLACK|_LGREY);
      winpdef (1, 24, string, "?", 0, 2, NULL, 0);

      i = winpread ();
      wclose ();
      hidecur ();

      if (i == W_ESCPRESS)
         goto continue_editing;

      if (toupper (string[0]) == 'Y')
         memcpy ((char *)usr, (char *)&nusr, sizeof (struct _usr));
	}

	gotoxy_ (24, 1);
	clreol_ ();
	prints (24, 1, LGREY|_BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add New User  L-List  D-Delete  O-Other");
	prints (24, 1, YELLOW|_BLACK, "PgUp/PgDn");
	prints (24, 26, YELLOW|_BLACK, "E");
	prints (24, 34, YELLOW|_BLACK, "A");
	prints (24, 50, YELLOW|_BLACK, "L");
	prints (24, 58, YELLOW|_BLACK, "D");
	prints (24, 68, YELLOW|_BLACK, "O");
}

void manager_users (void)
{
	int fd, fdidx, wh, i = 1, readed = 1, m;
	char string[128];
	struct _usr usr;
	struct _usridx usridx;

	sprintf (string, "%s.BBS", config.user_file);
	fd = sh_open (string, SH_DENYNONE, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
	if (fd == -1)
		return;

	sprintf (string, "%s.IDX", config.user_file);
	fdidx = sh_open (string, SH_DENYNONE, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
	if (fdidx == -1)
		return;

	memset ((char *)&usr, 0, sizeof (struct _usr));
	if (read (fd, &usr, sizeof (struct _usr)) < sizeof (struct _usr))
		readed = 0;
	else
		read (fdidx, &usridx, sizeof (struct _usridx));

	gotoxy_ (24, 1);
	clreol_ ();
	prints (24, 1, LGREY|_BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add New User  L-List  D-Delete  O-Other");
	prints (24, 1, YELLOW|_BLACK, "PgUp/PgDn");
	prints (24, 26, YELLOW|_BLACK, "E");
	prints (24, 34, YELLOW|_BLACK, "A");
	prints (24, 50, YELLOW|_BLACK, "L");
	prints (24, 58, YELLOW|_BLACK, "D");
	prints (24, 68, YELLOW|_BLACK, "O");

	wh = wopen (2, 0, 22, 77, 1, LCYAN|_BLACK, CYAN|_BLACK);
	wactiv (wh);
	wshadow (DGREY|_BLACK);
	wtitle (" Users ", TRIGHT, YELLOW|_BLUE);

   otherdata = 0;

   do {
      stop_update ();
      wclear ();

      wprints ( 1,  1, LGREY|_BLACK, " Name       ");
      wprints ( 2,  1, LGREY|_BLACK, " Alias      ");
      wprints ( 2, 43, LGREY|_BLACK, " FBox ID    ");
      wprints ( 3,  1, LGREY|_BLACK, " City       ");
      wprints ( 3, 43, LGREY|_BLACK, " First call ");

      if (readed) {
         wprints (1, 14, CYAN|_BLACK, usr.name);
         wprints (2, 14, CYAN|_BLACK, usr.handle);
         sprintf (string, "%08lX", usr.id);
         wprints (2, 56, CYAN|_BLACK, string);
         wprints (3, 14, CYAN|_BLACK, usr.city);
         wprints (3, 56, CYAN|_BLACK, usr.firstdate);
      }

      if (!otherdata) {
			wprints ( 4,  1, LGREY|_BLACK, " Password   ");
         wprints ( 4, 43, LGREY|_BLACK, " Last call  ");
         wprints ( 5,  1, LGREY|_BLACK, " Level      ");
         wprints ( 5, 26, LGREY|_BLACK, " Mail scan  ");
         wprints ( 5, 43, LGREY|_BLACK, " Archiver   ");
         wprints ( 6,  1, LGREY|_BLACK, " A Flags    ");
         wprints ( 6, 26, LGREY|_BLACK, " Editor FS  ");
         wprints ( 6, 43, LGREY|_BLACK, " Nulls      ");
         wprints ( 7,  1, LGREY|_BLACK, " B Flags    ");
         wprints ( 7, 26, LGREY|_BLACK, " More?      ");
         wprints ( 7, 43, LGREY|_BLACK, " Language   ");
         wprints ( 8,  1, LGREY|_BLACK, " C Flags    ");
         wprints ( 8, 26, LGREY|_BLACK, " CLS        ");
         wprints ( 8, 43, LGREY|_BLACK, " Minutes    ");
         wprints ( 9,  1, LGREY|_BLACK, " D Flags    ");
         wprints ( 9, 26, LGREY|_BLACK, " Hot keys   ");
         wprints ( 9, 43, LGREY|_BLACK, " Credit     ");
         wprints (10,  1, LGREY|_BLACK, " Video      ");
         wprints (10, 26, LGREY|_BLACK, " Reader FS  ");
         wprints (10, 43, LGREY|_BLACK, " Voice pho. ");
         wprints (11,  1, LGREY|_BLACK, " юд Colors  ");
         wprints (11, 26, LGREY|_BLACK, " IBM chars. ");
         wprints (11, 43, LGREY|_BLACK, " Data phone ");
         wprints (12,  1, LGREY|_BLACK, " Calls #    ");
         wprints (12, 26, LGREY|_BLACK, " Hidden     ");
         wprints (12, 43, LGREY|_BLACK, " Birthdate  ");
         wprints (13,  1, LGREY|_BLACK, " Last Msg.  ");
         wprints (13, 26, LGREY|_BLACK, " No kill    ");
         wprints (13, 43, LGREY|_BLACK, " Expiration ");
         wprints (14,  1, LGREY|_BLACK, " Last File  ");
         wprints (14, 26, LGREY|_BLACK, " Priority   ");
         wprints (14, 43, LGREY|_BLACK, " Length     ");
         wprints (15, 43, LGREY|_BLACK, " Width      ");
         wprints (15, 26, LGREY|_BLACK, " File box   ");
         wprints (15,  1, LGREY|_BLACK, " Protocol   ");
			wprints (16,  1, LGREY|_BLACK, " Signature  ");
			wprints (17,  1, LGREY|_BLACK, " Comment    ");

			if (readed) {
				strcpy (string, "***************");
				string[strlen (usr.pwd)] = '\0';
				wprints (4, 14, CYAN|_BLACK, string);
				wprints (4, 56, CYAN|_BLACK, usr.ldate);
				wprints (5, 13, CYAN|_BLACK, get_priv_text (usr.priv));
				wprints (5, 39, CYAN|_BLACK, usr.scanmail ? "Yes" : "No");
				for (m = 0; m < 10; m++) {
					if (!config.packid[m].display[0])
						continue;
					if (toupper (config.packid[m].display[0]) == toupper (usr.archiver))
						break;
				}
				wprints (5, 56, CYAN|_BLACK, (m < 10) ? &config.packid[m].display[1] : "None");
				wprints (6, 14, CYAN|_BLACK, get_flagA_text ((usr.flags >> 24) & 0xFF));
				wprints (6, 39, CYAN|_BLACK, !usr.use_lore ? "Yes" : "No");
				sprintf (string, "%d", usr.nulls);
				wprints (6, 56, CYAN|_BLACK, string);
				wprints (7, 14, CYAN|_BLACK, get_flagB_text ((usr.flags >> 16) & 0xFF));
				wprints (7, 39, CYAN|_BLACK, usr.more ? "Yes" : "No");
				if (!config.language[usr.language].descr[0])
					usr.language = 0;
				wprints (7, 56, CYAN|_BLACK, &config.language[usr.language].descr[1]);
				wprints (8, 14, CYAN|_BLACK, get_flagC_text ((usr.flags >> 8) & 0xFF));
				wprints (8, 39, CYAN|_BLACK, usr.formfeed ? "Yes" : "No");
				sprintf (string, "%d", usr.time);
				wprints (8, 56, CYAN|_BLACK, string);
				wprints (9, 14, CYAN|_BLACK, get_flagD_text (usr.flags & 0xFF));
				wprints (9, 39, CYAN|_BLACK, usr.hotkey ? "Yes" : "No");
				sprintf (string, "%d", usr.credit);
				wprints (9, 56, CYAN|_BLACK, string);
				if (usr.ansi)
					wprints (10, 14, CYAN|_BLACK, "Ansi");
				else if (usr.avatar)
					wprints (10, 14, CYAN|_BLACK, "Avatar");
				else
					wprints (10, 14, CYAN|_BLACK, "TTY");
				wprints (10, 39, CYAN|_BLACK, usr.full_read ? "Yes" : "No");
				wprints (10, 56, CYAN|_BLACK, usr.voicephone);
				wprints (11, 14, CYAN|_BLACK, usr.color ? "Yes" : "No");
				wprints (11, 39, CYAN|_BLACK, usr.ibmset ? "Yes" : "No");
				wprints (11, 56, CYAN|_BLACK, usr.dataphone);
				sprintf (string, "%ld", usr.times);
				wprints (12, 14, CYAN|_BLACK, string);
				wprints (12, 39, CYAN|_BLACK, usr.usrhidden ? "Yes" : "No");
				wprints (12, 56, CYAN|_BLACK, usr.birthdate);
				sprintf (string, "%u", usr.msg);
				wprints (13, 14, CYAN|_BLACK, string);
				wprints (13, 39, CYAN|_BLACK, usr.nokill ? "Yes" : "No");
				wprints (13, 56, CYAN|_BLACK, usr.subscrdate);
				sprintf (string, "%u", usr.files);
				wprints (14, 14, CYAN|_BLACK, string);
				wprints (14, 39, CYAN|_BLACK, usr.xfer_prior ? "Yes" : "No");
				sprintf (string, "%d", usr.len);
				wprints (14, 56, CYAN|_BLACK, string);
				sprintf (string, "%d", usr.width);
				wprints (15, 56, CYAN|_BLACK, string);
				wprints (15, 39, CYAN|_BLACK, usr.havebox ? "Yes" : "No");
				sprintf (string, "%c", usr.protocol);
				wprints (15, 14, CYAN|_BLACK, string);
				wprints (16, 14, CYAN|_BLACK, usr.signature);
				wprints (17, 14, CYAN|_BLACK, usr.comment);

				if (usr.deleted)
					wprints (1, 62, WHITE|_BLACK, "* DELETED *");
			}
		}
		else {
         wprints ( 4,  1, LGREY|_BLACK, " Pwd change ");
         wprints ( 5,  1, LGREY|_BLACK, " UL Kbytes  ");
         wprints ( 5, 26, LGREY|_BLACK, " UL Files   ");
         wprints ( 6,  1, LGREY|_BLACK, " DL Kbytes  ");
         wprints ( 6, 26, LGREY|_BLACK, " DL Files   ");
         wprints ( 7,  1, LGREY|_BLACK, " Quiet      ");
         wprints ( 7, 26, LGREY|_BLACK, " Time call  ");
         wprints ( 8,  1, LGREY|_BLACK, " Nerd       ");
         wprints ( 8, 26, LGREY|_BLACK, " Time day   ");
         wprints ( 9,  1, LGREY|_BLACK, " No disturb ");
         wprints ( 9, 26, LGREY|_BLACK, " DL Limit   ");
         wprints (10,  1, LGREY|_BLACK, " Security   ");
         wprints (10, 26, LGREY|_BLACK, " DL Ratio   ");
         wprints (11,  1, LGREY|_BLACK, " Bad Pwd    ");
         wprints (11, 26, LGREY|_BLACK, " Min. speed ");
         wprints (12,  1, LGREY|_BLACK, " Kludge     ");
         wprints (12, 26, LGREY|_BLACK, " DL Speed   ");
         wprints (13,  1, LGREY|_BLACK, " Msg. group ");
         wprints (13, 26, LGREY|_BLACK, " Beg. ratio ");
         wprints (14,  1, LGREY|_BLACK, " File group ");
         wprints (14, 26, LGREY|_BLACK, "            ");
         wprints (15,  1, LGREY|_BLACK, " Chat mins. ");
         wprints (15, 26, LGREY|_BLACK, " Msg Posted ");
         wprints (16,  1, LGREY|_BLACK, " TimeBank   ");
         wprints (16, 26, LGREY|_BLACK, " FileBank   ");
         wprints (17,  1, LGREY|_BLACK, " Last baud  ");
         wprints (17, 26, LGREY|_BLACK, " DL Today   ");

         if (readed) {
            wprints ( 4, 14, CYAN|_BLACK, usr.lastpwdchange);
            sprintf (string, "%lu", usr.upld);
            wprints ( 5, 14, CYAN|_BLACK, string);
            sprintf (string, "%u", usr.n_upld);
            wprints ( 5, 39, CYAN|_BLACK, string);
            sprintf (string, "%lu", usr.dnld);
            wprints ( 6, 14, CYAN|_BLACK, string);
            sprintf (string, "%u", usr.n_dnld);
            wprints ( 6, 39, CYAN|_BLACK, string);
            wprints ( 7, 14, CYAN|_BLACK, usr.quiet ? "Yes" : "No");
            sprintf (string, "%u", usr.ovr_class.max_call);
            wprints ( 7, 39, CYAN|_BLACK, string);
            wprints ( 8, 14, CYAN|_BLACK, usr.nerd ? "Yes" : "No");
            sprintf (string, "%u", usr.ovr_class.max_time);
            wprints ( 8, 39, CYAN|_BLACK, string);
            wprints ( 9, 14, CYAN|_BLACK, usr.donotdisturb ? "Yes" : "No");
            sprintf (string, "%u", usr.ovr_class.max_dl);
            wprints ( 9, 39, CYAN|_BLACK, string);
            wprints (10, 14, CYAN|_BLACK, usr.security ? "Yes" : "No");
            sprintf (string, "%u", usr.ovr_class.ratio);
            wprints (10, 39, CYAN|_BLACK, string);
            wprints (11, 14, CYAN|_BLACK, usr.badpwd ? "Yes" : "No");
            sprintf (string, "%u", usr.ovr_class.min_baud);
            wprints (11, 39, CYAN|_BLACK, string);
            wprints (12, 14, CYAN|_BLACK, usr.kludge ? "Yes" : "No");
            sprintf (string, "%u", usr.ovr_class.min_file_baud);
            wprints (12, 39, CYAN|_BLACK, string);
            sprintf (string, "%u", usr.msg_sig);
            wprints (13, 14, CYAN|_BLACK, string);
            sprintf (string, "%u", usr.ovr_class.start_ratio);
            wprints (13, 39, CYAN|_BLACK, string);
            sprintf (string, "%u", usr.file_sig);
            wprints (14, 14, CYAN|_BLACK, string);
            sprintf (string, "%u", usr.chat_minutes);
            wprints (15, 14, CYAN|_BLACK, string);
            sprintf (string, "%u", usr.msgposted);
            wprints (15, 39, CYAN|_BLACK, string);
            sprintf (string, "%u", usr.account);
            wprints (16, 14, CYAN|_BLACK, string);
            sprintf (string, "%u", usr.f_account);
            wprints (16, 39, CYAN|_BLACK, string);
            sprintf (string, "%lu", usr.baud_rate);
            wprints (17, 14, CYAN|_BLACK, string);
            sprintf (string, "%u", usr.dnldl);
            wprints (17, 39, CYAN|_BLACK, string);
         }
      }

      start_update ();

      i = getxch ();
      if ( (i & 0xFF) != 0 )
         i &= 0xFF;

      switch (i) {
         // PgDn
         case 0x5100:
            read (fd, (char *)&usr, sizeof (struct _usr));
            read (fdidx, &usridx, sizeof (struct _usridx));
            break;

         // PgUp
         case 0x4900:
            if (tell (fd) > sizeof (struct _usr)) {
               lseek (fd, -2L * sizeof (struct _usr), SEEK_CUR);
               read (fd, (char *)&usr, sizeof (struct _usr));
               lseek (fdidx, -2L * sizeof (struct _usridx), SEEK_CUR);
               read (fdidx, &usridx, sizeof (struct _usridx));
            }
            break;

         // E Edit
         case 'E':
         case 'e':
            edit_single_user (&usr);
            lseek (fd, -1L * sizeof (struct _usr), SEEK_CUR);
            write (fd, (char *)&usr, sizeof (struct _usr));
            usridx.id = usr.id;
            usridx.alias_id = usr.alias_id;
            lseek (fdidx, -1L * sizeof (struct _usridx), SEEK_CUR);
            write (fdidx, &usridx, sizeof (struct _usridx));
            break;

         // A Add
         case 'A':
         case 'a':
            memset (&usr, 0, sizeof (struct _usr));
            usr.priv = config.logon_level;
            usr.flags = config.logon_flags;
            usr.len = 23;
            usr.width = 80;
            usr.ansi = usr.color = usr.ibmset = 1;
            usr.formfeed = 1;

            edit_single_user (&usr);

            if (!usr.name[0]) {
               lseek (fd, -1L * sizeof (struct _usr), SEEK_CUR);
               read (fd, (char *)&usr, sizeof (struct _usr));
            }
            else {
               lseek (fd, 0L, SEEK_END);
               write (fd, (char *)&usr, sizeof (struct _usr));
               usridx.id = usr.id;
               usridx.alias_id = usr.alias_id;
               lseek (fdidx, 0L, SEEK_END);
               write (fdidx, &usridx, sizeof (struct _usridx));
            }
            break;

         case 'O':
         case 'o':
            otherdata = otherdata ? 0 : 1;
            break;

         // D Delete
         case 'D':
         case 'd':
            usr.deleted ^= 1;
            lseek (fd, -1L * sizeof (struct _usr), SEEK_CUR);
            write (fd, (char *)&usr, sizeof (struct _usr));
            break;

         // L List
         case 'L':
         case 'l':
            user_select_list (fd, fdidx, &usr);
            break;

         // ESC Exit
         case 0x1B:
            i = -1;
            break;
      }

   } while (i != -1);

   close (fdidx);
   close (fd);

   wclose ();
   gotoxy_ (24, 1);
   clreol_ ();
}

