#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <mem.h>
#include <ctype.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

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

#define UpdateCRC(c,crc) (cr3tab[((int) crc ^ c) & 0xff] ^ ((crc >> 8) & 0x00FFFFFFL))
#define x32crc(c,crc) (cr3tab[((int) crc ^ c) & 0xff] ^ ((crc >> 8) & 0x00FFFFFFL))

typedef union {
   long value;
   struct {
      char byte1;
      char byte2;
      char byte3;
      char byte4;
   } sp;
} SPLIT4;

typedef union {
   unsigned short value;
   struct {
      char byte1;
      char byte2;
   } sp;
} SPLIT2;

// Valori per flag1 - Opzioni di registrazione
#define REG2    0x01
#define REG5    0x02
#define REG10   0x04
#define REGUNL  0x08
#define POINT   0x10
#define BETA    0x20
#define DOS     0x40
#define OS2     0x80

typedef struct _ak {
   char  crc1;
   char  ending1;
   char  rnd1;
   char  crc2;
   char  flag1;
   char  id[2];
   char  ending2;
   short serial;
   char  crc3;
   char  flag2;
   char  version1;
   char  crc4;
   char  version2;
   char  rnd2;
} AK;

unsigned int xcrc (unsigned int crc, unsigned char b)
{
   register newcrc;
   int i;

   newcrc = crc ^ ((int)b << 8);
   for(i = 0; i < 8; ++i)
      if(newcrc & 0x8000)
         newcrc = (newcrc << 1) ^ 0x1021;
   else
      newcrc = newcrc << 1;
   return(newcrc & 0xFFFF);
}

unsigned long get_crc (void *buffer, int length)
{
   register int i;
   unsigned long crc = 0xAABBCCDDL;
   char *b;

   b = (char *)buffer;

   for (i = 0; i < length; i++)
      crc = x32crc ((unsigned char)b[i], crc);

   return (crc);
}

char *mod36 (unsigned long value)
{
   int r, p;
   char *encoding = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
   static char result[10];

   p = 0;

   while (value > 0L) {
      r = (int)(value % 36L);
      value /= 36L;
      result[p++] = encoding[r];
   }
   while (p < 7)
      result[p++] = '0';
   result[p] = '\0';
   return (result);
}

/*

void activation_key (int log_status)
{
   int i, maxlines;
   char str[32];
   unsigned char *p, px, ppx;
   unsigned long value, mult;
   AK ak;
   SPLIT2 cs2;
   SPLIT4 cs4;
   struct date datep;
   int decoding[] = { 0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,0,10,11,12,13,14,15,16,17,18,19,20,
                      21,22,23,24,25,26,27,28,29,30,31,32,33,34,35 };

   memset (&ak, 0, sizeof (AK));
   p = (unsigned char *)&ak;
   for (i = 0; i < strlen (config.newkey_code); i++) {
      if ((i % 7) == 0) {
         if (i) {
            memcpy (p, &value, 4);
            p += 4;
         }
         value = 0L;
         mult = 1L;
      }
      value += decoding[str[i] - '0'] * mult;
      mult *= 36L;
   }
   memcpy (p, &value, 4);

   p = (unsigned char *)&ak;
   ppx = 0xAA;
   for (i = 0; i < sizeof (AK); i++) {
      px = *p;
      *p ^= ppx;
      ppx = px;
      p++;
   }

   cs4.sp.byte1 = ak.crc1;
   cs4.sp.byte2 = ak.crc2;
   cs4.sp.byte3 = ak.crc3;
   cs4.sp.byte4 = ak.crc4;
   ak.crc1 = ak.crc2 = ak.crc3 = ak.crc4 = 0;

   cs2.sp.byte1 = ak.ending1;
   cs2.sp.byte2 = ak.ending2;

   if (ak.flag1 & REG2)
      maxlines = 2;
   else if (ak.flag1 & REG5)
      maxlines = 5;
   else if (ak.flag1 & REG10)
      maxlines = 10;
   else if (ak.flag1 & REGUNL)
      maxlines = 99;
   else if (ak.flag1 & POINT)
      maxlines = 1;

   if (cs4.value == get_crc (&ak, sizeof (AK)))
      if (maxlines == 99 || maxlines >= line_offset) {
         reg_prompt = "  [Registered]";
         if (ak.flag1 & POINT)
            registered = 2;
         else
            registered = 1;
         serial_no = ak.serial;
         serial_id1 = ak.id[0];
         serial_id2 = ak.id[1];

         if (log_status && cs2.value != 0xFFFF) {
            getdate (&datep);
         }
      }
   }

#ifndef COMMERCIAL
   wopen (9, 12, 18, 66, 0, LRED|_BLACK, LGREY|_BLACK);
   wtitle (" SECURITY VIOLATION ", TCENTER, YELLOW|_BLACK);
   wcenters (0, LCYAN|_BLACK, "This is a beta version that need an authorized");
   wcenters (1, LCYAN|_BLACK, "registration key to work.");
   wcenters (3, YELLOW|_BLACK|BLINK, "YOU ARE NOT ALLOWED TO USE THIS PROGRAM");
   wcenters (5, LCYAN|_BLACK, "Please, discontinue to use this version and contact");
   wcenters (6, LCYAN|_BLACK, "the author to obtain an usable copy.");
   wcenters (7, LCYAN|_BLACK, "Your system is now freezed");
   for (;;);
#else
   wopen (9, 12, 18, 66, 0, LRED|_BLACK, LGREY|_BLACK);
   wtitle (" SECURITY VIOLATION ", TCENTER, YELLOW|_BLACK);
   wcenters (0, LCYAN|_BLACK, "This is a commercial product that need an");
   wcenters (1, LCYAN|_BLACK, "authorized registration key to work.");
   wcenters (3, YELLOW|_BLACK|BLINK, "YOU ARE NOT ALLOWED TO USE THIS PROGRAM");
   wcenters (5, LCYAN|_BLACK, "Please, discontinue to use this version and contact");
   wcenters (6, LCYAN|_BLACK, "the author to obtain an usable copy.");
   wcenters (7, LCYAN|_BLACK, "Your system is now freezed");
   for (;;);
#endif
}
*/

void main (void)
{
   FILE *fp, *fpr;
   int i, m, year, mont, day, point, maxlines, serial, os, copy;
   unsigned int serial_no;
   char str[32], sysop[36], addr[50], citta[50], id1, id2, kc[32];
   unsigned char *p, px;
   unsigned long value, crc;
   AK ak, akold;
   SPLIT2 cs2;
   SPLIT4 cs4;

   printf("\nLoraBBS 2.32 - Registration Key Generator\n");
   printf("CopyRight (c) 1991-93 by Marco Maccaferri. All Rights Reserved\n\n");

   memset (&ak, 0, sizeof (AK));

   printf ("Sysop Name.................: ");
   gets (sysop);
   printf ("Street Address.............: ");
   gets (addr);
   printf ("ZIP, City..................: ");
   gets (citta);

   printf ("Key Type (BBS/POINT).......: ");
   gets (str);
   if (!stricmp (str, "point"))
      point = 1;
   else
      point = 0;

   id1 = id2 = '\0';

   do {
      printf ("Serial Number (0000).......: ");
      if ((i = open ("LORAKEY.DAT", O_RDONLY|O_BINARY)) == -1) {
         gets (str);
         if (isalpha (str[0]) && isalpha (str[1])) {
            id1 = ak.id[0] = toupper (str[0]);
            id2 = ak.id[1] = toupper (str[1]);
            serial = ak.serial = atoi (&str[2]);
         }
         else
            serial = ak.serial = atoi (str);

         i = open ("LORAKEY.DAT", O_WRONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
         write (i, &serial, sizeof (int));
         close (i);
      }
      else {
         read (i, &serial, sizeof (int));
         close (i);
         ak.serial = ++serial;
         printf ("%04u\n", serial);
      }

//      if (serial < 3001 || serial > 3999) {
//         printf ("\n\aInvalid serial number.\n\n");
//         serial = 0;
//         unlink ("LORAKEY.DAT");
//      }
   } while (serial == 0);

   if (point) {
      ak.flag1 = POINT;
      maxlines = 0;
   }
   else {
      printf ("Number of Lines............: ");
      gets (str);
      maxlines = m = atoi (str);
      if (m <= 2)
         ak.flag1 = REG2;
      else if (m <= 5)
         ak.flag1 = REG5;
      else if (m <= 10)
         ak.flag1 = REG10;
      else
         ak.flag1 = REGUNL;
   }

   printf ("Operating System (DOS/OS2).: ");
   gets (str);
   if (!stricmp (str, "dos")) {
      os = 1;
      ak.flag1 |= DOS;
   }
   else if (!stricmp (str, "os2")) {
      os = 2;
      ak.flag1 |= OS2;
   }
   else {
      os = 3;
      ak.flag1 |= DOS|OS2;
   }

/*
   printf ("Expiration Date (DD-MM-YY).: ");
   gets (str);
   if (str[0]) {
      sscanf (str, "%2d-%2d-%2d", &day, &mont, &year);
      cs2.value = (year - 80) * 512 + mont * 32 + day;
   }
   else
*/
      cs2.value = 0xFFFFL;

   printf ("Number of copies...........: ");
   gets (str);
   copy = atoi (str);
   if (copy == 0)
      copy = 1;

/*
   printf ("Beta Key (Y/N).............: ");
   gets (str);
   if (toupper (str[0]) == 'Y')
      ak.flag1 |= BETA;
*/

   printf ("\n");

   ak.ending1 = cs2.sp.byte1;
   ak.ending2 = cs2.sp.byte2;
   ak.version1 = 2;
   ak.version2 = 32;
   memcpy (&akold, &ak, sizeof (AK));

   if (maxlines == 0)
      crc = 0x6FE41197L;
   else if (maxlines <= 2)
      crc = 0x7FE50189L;
   else if (maxlines <= 5)
      crc = 0x74E40291L;
   else if (maxlines <= 10)
      crc = 0x7FA45109L;
   else
      crc = 0x7FE40199L;

   serial_no = 0;

   for (i=0; i<strlen(sysop);i++)
      serial_no = xcrc (serial_no, (unsigned char)sysop[i]);
   sprintf (kc, "%ld", crc);
   for (i=0; i<strlen(kc);i++)
      serial_no = xcrc (serial_no, (unsigned char)kc[i]);

   for (i = 0; i < strlen (sysop); i++)
      crc = UpdateCRC (sysop[i], crc);

   randomize ();

   unlink ("LORAKEY.TXT");
   fp = fopen("LORAKEY.TXT","wt");

   fpr = fopen("LORAKEY.REG","at");

   fprintf (fp, "Sysop..............: %s\n", sysop);
   fprintf (fp, "Version registered.: 2.35\n");
   if (os == 1)
      fprintf (fp, "Operating System...: DOS\n");
   else if (os == 2)
      fprintf (fp, "Operating System...: OS/2\n");
   else if (os == 3)
      fprintf (fp, "Operating System...: DOS and OS/2\n");
   if (maxlines == 99)
      fprintf (fp, "Number of lines....: Unlimited\n", maxlines);
   else if (maxlines)
      fprintf (fp, "Number of lines....: %d\n", maxlines);
   else
      fprintf (fp, "Number of lines....: 1 (POINT)\n");

   if (copy > 1) {
      fprintf (fp, "Serial Numbers.....: ");
      if (id1 && id2)
         fprintf (fp, "%c%c%04u to %c%c%04u\n\n", id1, id2, serial, id1, id2, serial + copy - 1);
      else
         fprintf (fp, "%04u to %04u\n\n", serial, serial + copy - 1);
   }
   else {
      fprintf (fp, "Serial Number......: ");
      if (id1 && id2)
         fprintf (fp, "%c%c%04u\n\n", id1, id2, serial);
      else
         fprintf (fp, "%04u\n\n", serial);
   }

   if (copy > 1)
      fprintf (fp, "+- Keys ---------------------------------------------------------+\n");
   else
      fprintf (fp, "+- Key ----------------------------------------------------------+\n");

   fprintf (fpr, "Sysop..............: %s\n", sysop);
   fprintf (fpr, "Address............: %s\n", addr);
   fprintf (fpr, "City, ZIP..........: %s\n", citta);

   fprintf (fpr, "Version registered.: 2.35\n");
   if (os == 1)
      fprintf (fpr, "Operating System...: DOS\n");
   else if (os == 2)
      fprintf (fpr, "Operating System...: OS/2\n");
   else if (os == 3)
      fprintf (fpr, "Operating System...: DOS and OS/2\n");
   if (maxlines == 99)
      fprintf (fpr, "Number of lines....: Unlimited\n", maxlines);
   else if (maxlines)
      fprintf (fpr, "Number of lines....: %d\n", maxlines);
   else
      fprintf (fpr, "Number of lines....: 1 (POINT)\n");

   while (copy--) {
      akold.serial = serial++;
      memcpy (&ak, &akold, sizeof (AK));

      ak.rnd1 = random (255);
      ak.rnd2 = random (255);
      cs4.value = get_crc (&ak, sizeof (AK));
      ak.crc1 = cs4.sp.byte1;
      ak.crc2 = cs4.sp.byte2;
      ak.crc3 = cs4.sp.byte3;
      ak.crc4 = cs4.sp.byte4;

      p = (unsigned char *)&ak;
      px = 0xAA;
      for (i = 0; i < sizeof (AK); i++) {
         *p ^= px;
         px = *p++;
      }

      printf ("Key........................: ");
      p = (unsigned char *)&ak;
      str[0] = '\0';
      for (i = 0; i < sizeof (AK); i+=4) {
         memcpy (&value, &p[i], 4);
         printf ("%s", mod36 (value));
         strcat (str, mod36 (value));
      }
      printf ("\n");

      if (id1 && id2)
         fprintf (fp, "|          %c%c%04u  >>> %s <<<          |\n", id1, id2, serial - 1, str);
      else
         fprintf (fp, "|           %04u  >>> %s <<<           |\n", serial - 1, str);

      if (copy)
         fprintf (fp, "|----------------------------------------------------------------|\n");

      if (id1 && id2)
         fprintf (fpr, "Key code...........: %s (Serial #%c%c%04u)\n", str, id1, id2, serial - 1);
      else
         fprintf (fpr, "Key code...........: %s (Serial #%04u)\n", str, serial - 1);
   }

   fprintf (fp, "+----------------------------------------------------------------+\n");

   i = open ("LORAKEY.DAT", O_WRONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
   serial--;
   write (i, &serial, sizeof (int));
   close (i);

   fprintf (fpr, "-------------------------------------------------------------------------\n");

   fclose(fp);
   fclose (fpr);
}
