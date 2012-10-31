#include <stdio.h>
#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <mem.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys\stat.h>

#include <share.h>

#define shopen(path,access)       open(path,(access)|SH_DENYNONE)
#define cshopen(path,access,mode) open(path,(access)|SH_DENYNONE,mode)

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "lora.h"

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
#define USR_TABS       13
#define USR_NERD       14
#define USR_IBMSET     15
#define USR_FSED       16
#define USR_MORE       17
#define USR_CLEAR      18
#define USR_DATAPHONE  19
#define USR_WIDTH      20
#define USR_LENGTH     21
#define USR_VIDEO      22
#define USR_CREDIT     23
#define USR_FULLREAD   24
#define USR_COLOR      25
#define USR_MSG        26
#define USR_FILES      27
#define USR_INLIST     28
#define USR_SIG        29
#define USR_CHAT       30
#define USR_ACTION     31
#define USR_EXPIRE     32
#define USR_EXPBY      33
#define USR_TIMEBANK   34
#define USR_KBANK      35
#define USR_VOTES      36
#define USR_ALL        10240

void DrawUserScreen (void);
void DisplayUserParameters (int, struct _usr *, int, int);
char *GetPrivText (int);
long SaveReadUser (int, long, long, struct _usr *);
int NextPrivilege (int);
long GetFlags (char *);
void SetFlags (char *, long);
void PurgeUsers (void);

void main (void)
{
   int fd, c, true, cu, nu, mainview, x, y, wh;
   char stringa[80], visible, searchname[36];
   long pos, oldpos;
   struct _usr tusr, backup;

   fd = shopen ("USERS.BBS", O_RDWR|O_BINARY);
   if (fd == -1) {
      printf ("\nUSERS.BBS: File NOT found!\n");
      exit (1);
   }

   visible = 0;

   videoinit ();
   hidecur ();
   readcur (&x, &y);

   mainview = wopen (0, 0, 24, 79, 5, LGREY|_BLACK, LGREY|_BLACK);
   wactiv (mainview);

   DrawUserScreen ();

   cu = 0;
   pos = SaveReadUser (fd, -1L, 0L, &tusr);
   nu = (int)(filelength(fd) / sizeof (struct _usr)) - 1;

   memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
   DisplayUserParameters (USR_ALL, &tusr, nu, cu);

   true = 1;
   strcpy (searchname, "");

   while (true) {
      hidecur ();

      c = getch ();
      if (c == 0)
         c = getch ();

      switch (toupper(c)) {
         case '~':
            searchname[0] = '\0';
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (5, 14, searchname, "???????????????????????????????????", 'M', 2, NULL, 0);
            winpread ();
            strtrim (searchname);
            if (!searchname[0])
               break;

            oldpos = pos;
            SaveReadUser (fd, pos, -1L, &tusr);
            lseek (fd, 0L, SEEK_SET);

            wh = wopen (10, 25, 14, 55, 0, LRED|_BLACK, LGREY|_BLACK);
            wactiv (wh);
            wcenters (1, YELLOW|BLACK|BLINK, "Search in progress ...");

            while (read (fd, (char *)&tusr, sizeof (struct _usr)) == sizeof (struct _usr)) {
               if ( !strncmpi(tusr.name, searchname, strlen (searchname)) )
                  break;
            }

            if (tell(fd) == filelength (fd)) {
               pos = SaveReadUser (fd, -1L, oldpos, &tusr);
               wcenters (1, YELLOW|BLACK, "    Search failed     ");
               printf ("\a");
               getch ();
               wclose ();
               wunlink (wh);
               DisplayUserParameters (USR_ALL, &tusr, nu, cu);
            }
            else {
               pos = tell (fd) - sizeof (struct _usr);
               cu = (int)(pos / sizeof (struct _usr));
               wclose ();
               wunlink (wh);
               DisplayUserParameters (USR_ALL, &tusr, nu, cu);
            }
            break;
         case '`':
            if (searchname[0] == '\0')
            {
               printf ("\a");
               break;
            }

            oldpos = pos;
            SaveReadUser (fd, pos, -1L, &tusr);

            wh = wopen (10, 25, 14, 55, 0, LRED|_BLACK, LGREY|_BLACK);
            wactiv (wh);
            wcenters (1, YELLOW|BLACK|BLINK, "Search in progress ...");

            while (read (fd, (char *)&tusr, sizeof (struct _usr)) == sizeof (struct _usr)) {
               if ( !strncmpi(tusr.name, searchname, strlen (searchname)) )
                  break;
            }

            if (tell(fd) == filelength (fd)) {
               pos = SaveReadUser (fd, -1L, oldpos, &tusr);
               wcenters (1, YELLOW|BLACK, "    Search failed     ");
               printf ("\a");
               getch ();
               wclose ();
               wunlink (wh);
               DisplayUserParameters (USR_ALL, &tusr, nu, cu);
            }
            else {
               pos = tell (fd) - sizeof (struct _usr);
               cu = (int)(pos / sizeof (struct _usr));
               wclose ();
               wunlink (wh);
               DisplayUserParameters (USR_ALL, &tusr, nu, cu);
            }
            break;
         case '^':
            memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
            tusr.deleted ^= 1;
            DisplayUserParameters (USR_ALL, &tusr, nu, cu);
            break;
         case '/':
            c = 0;

            wh = wopen (9, 10, 17, 32, 0, LRED|_BLACK, LGREY|_BLACK);
            wactiv (wh);

            while (c != 0x1B) {
               hidecur ();

               wprints (0,  1, LCYAN|BLACK,"Time");
               wprintc (0,  1, YELLOW|BLACK,'T');
               sprintf(stringa, "%-5d", tusr.ovr_class.max_call);
               wprints (0, 15, LGREEN|_BLACK, stringa);

               wprints (1,  1, LCYAN|BLACK,"Cume");
               wprintc (1,  1, YELLOW|BLACK,'C');
               sprintf(stringa, "%-5d", tusr.ovr_class.max_time);
               wprints (1, 15, LGREEN|_BLACK, stringa);

               wprints (2,  1, LCYAN|BLACK,"file Limit");
               wprintc (2,  6, YELLOW|BLACK,'L');
               sprintf(stringa, "%-5d", tusr.ovr_class.max_dl);
               wprints (2, 15, LGREEN|_BLACK, stringa);

               wprints (3,  1, LCYAN|BLACK,"file Ratio");
               wprintc (3,  6, YELLOW|BLACK,'R');
               sprintf(stringa, "%-5d", tusr.ovr_class.ratio);
               wprints (3, 15, LGREEN|_BLACK, stringa);

               wprints (4,  1, LCYAN|BLACK,"Start ratio");
               wprintc (4,  1, YELLOW|BLACK,'S');
               sprintf(stringa, "%-5d", tusr.ovr_class.start_ratio);
               wprints (4, 15, LGREEN|_BLACK, stringa);

               wprints (5,  1, LCYAN|BLACK,"logon Baud");
               wprintc (5,  7, YELLOW|BLACK,'B');
               sprintf(stringa, "%-5d", tusr.ovr_class.min_baud);
               wprints (5, 15, LGREEN|_BLACK, stringa);

               wprints (6,  1, LCYAN|BLACK,"Download baud");
               wprintc (6,  1, YELLOW|BLACK,'D');
               sprintf(stringa, "%-5d", tusr.ovr_class.min_file_baud);
               wprints (6, 15, LGREEN|_BLACK, stringa);

               c = getch ();
               if (c == 0)
                  c = getch ();

               switch (toupper(c)) {
                  case 0x1B:
                     wclose ();
                     wunlink (wh);
                     break;
                  case '\"':
                     memcpy ((char *)&tusr, (char *)&backup, sizeof (struct _usr));
                     break;
                  case 'T':
                     sprintf (stringa, "%u", tusr.ovr_class.max_call);
                     winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
                     winpdef (6, 15, stringa, "99999", '9', 2, NULL, 0);
                     winpread ();
                     strtrim (stringa);
                     if (stringa[0]) {
                        memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
                        tusr.ovr_class.max_call = atoi (stringa);
                     }
                     break;
                  case 'C':
                     sprintf (stringa, "%u", tusr.ovr_class.max_time);
                     winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
                     winpdef (6, 15, stringa, "99999", '9', 2, NULL, 0);
                     winpread ();
                     strtrim (stringa);
                     if (stringa[0]) {
                        memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
                        tusr.ovr_class.max_time = atoi (stringa);
                     }
                     break;
                  case 'L':
                     sprintf (stringa, "%u", tusr.ovr_class.max_dl);
                     winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
                     winpdef (6, 15, stringa, "99999", '9', 2, NULL, 0);
                     winpread ();
                     strtrim (stringa);
                     if (stringa[0]) {
                        memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
                        tusr.ovr_class.max_dl = atoi (stringa);
                     }
                     break;
                  case 'R':
                     sprintf (stringa, "%u", tusr.ovr_class.ratio);
                     winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
                     winpdef (6, 15, stringa, "99999", '9', 2, NULL, 0);
                     winpread ();
                     strtrim (stringa);
                     if (stringa[0]) {
                        memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
                        tusr.ovr_class.ratio = atoi (stringa);
                     }
                     break;
                  case 'S':
                     sprintf (stringa, "%u", tusr.ovr_class.start_ratio);
                     winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
                     winpdef (6, 15, stringa, "99999", '9', 2, NULL, 0);
                     winpread ();
                     strtrim (stringa);
                     if (stringa[0]) {
                        memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
                        tusr.ovr_class.start_ratio = atoi (stringa);
                     }
                     break;
                  case 'B':
                     sprintf (stringa, "%u", tusr.ovr_class.min_baud);
                     winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
                     winpdef (6, 15, stringa, "99999", '9', 2, NULL, 0);
                     winpread ();
                     strtrim (stringa);
                     if (stringa[0]) {
                        memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
                        tusr.ovr_class.min_baud = atoi (stringa);
                     }
                     break;
                  case 'D':
                     sprintf (stringa, "%u", tusr.ovr_class.min_file_baud);
                     winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
                     winpdef (6, 15, stringa, "99999", '9', 2, NULL, 0);
                     winpread ();
                     strtrim (stringa);
                     if (stringa[0]) {
                        memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
                        tusr.ovr_class.min_file_baud = atoi (stringa);
                     }
                     break;
               }
            }

            c = 0;
            break;
         case '\"':
            memcpy ((char *)&tusr, (char *)&backup, sizeof (struct _usr));
            DisplayUserParameters (USR_ALL, &tusr, nu, cu);
            break;
         case '|':
            SaveReadUser (fd, pos, 0L, &tusr);
            close (fd);

            wh = wopen (10, 25, 14, 55, 0, LRED|_BLACK, LGREY|_BLACK);
            wactiv (wh);
            wcenters (1, YELLOW|BLACK|BLINK, "Purging users ...");

            PurgeUsers ();

            fd = shopen ("USERS.BBS", O_RDWR|O_BINARY);

            nu = (int)(filelength(fd) / sizeof (struct _usr)) - 1;
            cu = 0;

            wclose ();
            wunlink (wh);

            pos = SaveReadUser (fd, -1, 0L, &tusr);
            DisplayUserParameters (USR_ALL, &tusr, nu, cu);
            break;
         case '+':
            if (cu < nu)
            {
               cu++;
               pos = SaveReadUser (fd, pos, pos + sizeof (struct _usr), &tusr);
               DisplayUserParameters (USR_ALL, &tusr, nu, cu);
               visible = 0;
            }
            break;
         case '-':
            if (cu > 0) {
               cu--;
               pos = SaveReadUser (fd, pos, pos - sizeof (struct _usr), &tusr);
               DisplayUserParameters (USR_ALL, &tusr, nu, cu);
               visible = 0;
            }
            break;
         case '\r':
            stringa[0] = '\0';
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (3, 14, stringa, "99999", '9', 1, NULL, 0);
            winpread ();
            pos = SaveReadUser (fd, pos, (long)sizeof (struct _usr) * atoi(stringa), &tusr);
            cu = atoi (stringa);
            DisplayUserParameters (USR_ALL, &tusr, nu, cu);
            break;
         case 'N':
            strcpy (stringa, tusr.name);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (5, 14, stringa, "???????????????????????????????????", 'M', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0])
            {
               strcode(tusr.pwd, tusr.name);
               strcpy(tusr.name, stringa);
               strcode(tusr.pwd, tusr.name);
            }
            DisplayUserParameters (USR_NAME, &tusr, nu, cu);
            break;
         case 'K':
            SetFlags (stringa, tusr.flags);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (11, 45, stringa, "???????????????????????????????????", 'U', 1, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0])
               tusr.flags = GetFlags (stringa);
            DisplayUserParameters (USR_FLAGS, &tusr, nu, cu);
            break;
         case 'C':
            strcpy (stringa, tusr.city);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (6, 14, stringa, "?????????????????????????", '?', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0])
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               strcpy(tusr.city, stringa);
            }
            DisplayUserParameters (USR_CITY, &tusr, nu, cu);
            break;
         case '\'':
            strcpy (stringa, tusr.handle);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (7, 45, stringa, "???????????????????????????????????", 'M', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0])
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               strcpy(tusr.handle, stringa);
            }
            DisplayUserParameters (USR_HANDLE, &tusr, nu, cu);
            break;
         case 'D':
            sprintf (stringa, "%ld", tusr.dnld);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (10, 45, stringa, "99999", '9', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0])
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               tusr.dnld = atol (stringa);
            }
            DisplayUserParameters (USR_DOWNLOADS, &tusr, nu, cu);
            break;
         case 'U':
            sprintf (stringa, "%ld", tusr.upld);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (10, 14, stringa, "99999", '9', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0])
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               tusr.upld = atol (stringa);
            }
            DisplayUserParameters (USR_UPLOADS, &tusr, nu, cu);
            break;
/*
         case 'G':
            sprintf (stringa, "%d", tusr.sig);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (14, 45, stringa, "999", '9', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0])
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               tusr.sig = atoi (stringa);
            }
            DisplayUserParameters (USR_SIG, &tusr, nu, cu);
            break;
*/
         case 'L':
            sprintf (stringa, "%u", tusr.dnldl);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (10, 72, stringa, "99999", '9', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0])
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               tusr.dnldl = atoi (stringa);
            }
            DisplayUserParameters (USR_DOWNTODAY, &tusr, nu, cu);
            break;
         case 'T':
            sprintf (stringa, "%u", tusr.time);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (9, 45, stringa, "99999", '9', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0])
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               tusr.time = atoi (stringa);
            }
            DisplayUserParameters (USR_ONLINE, &tusr, nu, cu);
            break;
         case 'M':
            sprintf (stringa, "%u", tusr.msg);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (12, 45, stringa, "99999", '9', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0])
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               tusr.msg = atoi (stringa);
            }
            DisplayUserParameters (USR_MSG, &tusr, nu, cu);
            break;
         case 'E':
            sprintf (stringa, "%u", tusr.votes);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (11, 14, stringa, "99999", '9', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0]) {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               tusr.votes = atoi (stringa);
            }
            DisplayUserParameters (USR_VOTES, &tusr, nu, cu);
            break;
         case 'I':
            sprintf (stringa, "%u", tusr.account);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (12, 14, stringa, "99999", '9', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0]) {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               tusr.account = atoi (stringa);
            }
            DisplayUserParameters (USR_TIMEBANK, &tusr, nu, cu);
            break;
         case '?':
            sprintf (stringa, "%u", tusr.f_account);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (13, 14, stringa, "99999", '9', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0]) {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               tusr.f_account = atoi (stringa);
            }
            DisplayUserParameters (USR_KBANK, &tusr, nu, cu);
            break;
         case 'F':
            sprintf (stringa, "%u", tusr.files);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (12, 72, stringa, "99999", '9', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0])
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               tusr.files = atoi (stringa);
            }
            DisplayUserParameters (USR_FILES, &tusr, nu, cu);
            break;
         case '#':
            sprintf (stringa, "%u", tusr.times);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (9, 72, stringa, "99999", '9', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0])
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               tusr.times = atoi (stringa);
            }
            DisplayUserParameters (USR_TIMES, &tusr, nu, cu);
            break;
         case ';':
            strcpy (stringa, tusr.voicephone);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (8, 14, stringa, "???????????????????", '?', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0])
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               strcpy(tusr.voicephone, stringa);
            }
            DisplayUserParameters (USR_VOICEPHONE, &tusr, nu, cu);
            break;
         case ')':
            strcpy (stringa, tusr.dataphone);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (8, 45, stringa, "???????????????????", '?', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0])
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               strcpy(tusr.dataphone, stringa);
            }
            DisplayUserParameters (USR_DATAPHONE, &tusr, nu, cu);
            break;
         case '>':
            memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
            tusr.nerd ^= 1;
            DisplayUserParameters (USR_NERD, &tusr, nu, cu);
            break;
         case 'S':
            memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
            tusr.full_read ^= 1;
            DisplayUserParameters (USR_FULLREAD, &tusr, nu, cu);
            break;
         case '!':
            memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
            tusr.more ^= 1;
            DisplayUserParameters (USR_MORE, &tusr, nu, cu);
            break;
         case 'B':
            memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
            tusr.tabs ^= 1;
            DisplayUserParameters (USR_TABS, &tusr, nu, cu);
            break;
         case 'H':
            memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
            tusr.color ^= 1;
            DisplayUserParameters (USR_COLOR, &tusr, nu, cu);
            break;
         case 'X':
            memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
            tusr.use_lore ^= 1;
            DisplayUserParameters (USR_FSED, &tusr, nu, cu);
            break;
         case '_':
            memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
            tusr.usrhidden ^= 1;
            DisplayUserParameters (USR_INLIST, &tusr, nu, cu);
            break;
         case '*':
            memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
            tusr.formfeed ^= 1;
            DisplayUserParameters (USR_CLEAR, &tusr, nu, cu);
            break;
         case 'V':
            memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
            tusr.priv = NextPrivilege (tusr.priv);
            DisplayUserParameters (USR_PRIV, &tusr, nu, cu);
            break;
         case 'O':
            if (tusr.ansi)
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               tusr.ansi = 0;
               tusr.avatar = 1;
            }
            else if (tusr.avatar)
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               tusr.ansi = 0;
               tusr.avatar = 0;
            }
            else
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               tusr.ansi = 1;
               tusr.avatar = 0;
            }
            DisplayUserParameters (USR_VIDEO, &tusr, nu, cu);
            break;
         case '$':
            sprintf (stringa, "%u", tusr.width);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (18, 14, stringa, "999", '9', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0])
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               tusr.width = atoi (stringa);
            }
            DisplayUserParameters (USR_WIDTH, &tusr, nu, cu);
            break;
         case '%':
            sprintf (stringa, "%u", tusr.len);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (18, 45, stringa, "999", '9', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0])
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               tusr.len = atoi (stringa);
            }
            DisplayUserParameters (USR_LENGTH, &tusr, nu, cu);
            break;
         case 'R':
            sprintf (stringa, "%u", tusr.credit);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (14, 14, stringa, "99999", '9', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0])
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               tusr.credit = atoi (stringa);
            }
            DisplayUserParameters (USR_CREDIT, &tusr, nu, cu);
            break;
         case '=':
            if (visible)
            {
               memset (stringa, ' ', 16);
               memset (stringa, '*', strlen (tusr.pwd));
               stringa[16] = '\0';
               prints (7, 14, LGREEN|_BLACK, stringa);
               visible = 0;
            }
            else
            {
               memset (stringa, ' ', 16);
               stringa[16] = '\0';
               prints (7, 14, LGREEN|_BLACK, stringa);
               strcpy (stringa, tusr.pwd);
               strcode (strupr (stringa), tusr.name);
               prints (7, 14, LGREEN|_BLACK, stringa);
               visible = 1;
            }
            break;
         case 'P':
            strcpy (stringa, tusr.pwd);
            strcode (strupr (stringa), tusr.name);
            winpbeg (LGREEN|_BLUE, LGREEN|_BLUE);
            winpdef (7, 14, stringa, "???????????????", 'U', 2, NULL, 0);
            winpread ();
            strtrim (stringa);
            if (stringa[0])
            {
               memcpy ((char *)&backup, (char *)&tusr, sizeof (struct _usr));
               strcode (strupr (stringa), tusr.name);
               strcpy(tusr.pwd, stringa);
            }
            DisplayUserParameters (USR_PASSWORD, &tusr, nu, cu);
            break;
         case 'Q':
            true = 0;
            break;
      }
   }

   SaveReadUser (fd, pos, -1L, &tusr);

   close (fd);
   wcloseall ();
   gotoxy_ (x, y);
   showcur ();
}

void DrawUserScreen (void)
{
   wcenters (0, WHITE|BLACK, "LoraBBS v2.20 User Editor");
   prints (1,  0, LRED|BLACK, "컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴");

   prints (3,  0, LCYAN|BLACK,"  user number");

   prints (5,  0, LCYAN|BLACK,"    user Name");
   prints (6,  0, LCYAN|BLACK,"         City");
   prints (7,  0, LCYAN|BLACK,"     Password                         'alias");
   prints (8,  0, LCYAN|BLACK,";phone number                    )data phone");
   prints (9,  0, LCYAN|BLACK,"   priV level                   Time on-line                 # of calls");
   prints (10, 0, LCYAN|BLACK,"      Uploads      K                Dl (all)      K          dL (today)      K");
   prints (11, 0, LCYAN|BLACK,"        votEs                           Keys");
   prints (12, 0, LCYAN|BLACK," tIme in bank                  last Msg.area             last File area");
   prints (13, 0, LCYAN|BLACK,"  ?dl in bank                          >nerd              _in user list");
   prints (14, 0, LCYAN|BLACK,"       cRedit       cents                                      :hotkeys");
   prints (15, 0, LCYAN|BLACK,"      Hcolors                           taBs                     &nulls");
   prints (16, 0, LCYAN|BLACK,"                                  videO mode                 eXt.editor");
   prints (17, 0, LCYAN|BLACK,"!more prompt                  fullScreen msg                *scrn.clear");
   prints (18, 0, LCYAN|BLACK," $scrn.width                     %scrn.lngth                           ");
   prints (19, 0, LRED|BLACK, "컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴");
   prints (20, 0, LCYAN|BLACK," /)Override limits   =)Show Password     ~)Find user         ^)Delete user");
   prints (21, 0, LCYAN|BLACK," +)Next user         A)dd user           `)Find next usr     |)Purge users");
   prints (22, 0, LCYAN|BLACK," -)Prior user        Q)uit           Enter)Goto user #x      \")Undo last chg");

   printc (5,   9, YELLOW|BLACK,'N');
   printc (6,   9, YELLOW|BLACK,'C');
   printc (7,   5, YELLOW|BLACK,'P');
   printc (7,  38, YELLOW|BLACK,'\'');
   printc (8,   0, YELLOW|BLACK,';');
   printc (8,  33, YELLOW|BLACK,')');
   printc (9,   6, YELLOW|BLACK,'V');
   printc (9,  32, YELLOW|BLACK,'T');
   printc (9,  61, YELLOW|BLACK,'#');
   printc (10,  6, YELLOW|BLACK,'U');
   printc (10, 36, YELLOW|BLACK,'D');
   printc (10, 62, YELLOW|BLACK,'L');
   printc (11, 11, YELLOW|BLACK,'E');
   printc (11, 40, YELLOW|BLACK,'K');
   printc (12,  2, YELLOW|BLACK,'I');
   printc (12, 36, YELLOW|BLACK,'M');
   printc (12, 62, YELLOW|BLACK,'F');
   printc (13,  2, YELLOW|BLACK,'?');
   printc (13, 39, YELLOW|BLACK,'>');
   printc (13, 58, YELLOW|BLACK,'_');
   printc (14,  8, YELLOW|BLACK,'R');
   printc (14, 63, YELLOW|BLACK,':');
   printc (15,  6, YELLOW|BLACK,'H');
   printc (15, 42, YELLOW|BLACK,'B');
   printc (15, 65, YELLOW|BLACK,'&');
   printc (16, 38, YELLOW|BLACK,'O');
   printc (16, 62, YELLOW|BLACK,'X');
   printc (17,  0, YELLOW|BLACK,'!');
   printc (17, 34, YELLOW|BLACK,'S');
   printc (17, 60, YELLOW|BLACK,'*');
   printc (18,  1, YELLOW|BLACK,'$');
   printc (18, 33, YELLOW|BLACK,'%');
   printc (20,  1, YELLOW|BLACK,'/');
   printc (20, 21, YELLOW|BLACK,'=');
   printc (20, 41, YELLOW|BLACK,'~');
   printc (20, 61, YELLOW|BLACK,'^');
   printc (21,  1, YELLOW|BLACK,'+');
   printc (21, 21, YELLOW|BLACK,'A');
   printc (21, 41, YELLOW|BLACK,'`');
   printc (21, 61, YELLOW|BLACK,'|');
   printc (22,  1, YELLOW|BLACK,'-');
   printc (22, 21, YELLOW|BLACK,'Q');
   prints (22, 37, YELLOW|BLACK,"Enter");
   printc (22, 61, YELLOW|BLACK,'"');
}

void DisplayUserParameters (which, sysusr, nu, cu)
int which;
struct _usr *sysusr;
int nu, cu;
{
   char buffer[80];

   if (which == USR_ALL)
   {
      sprintf (buffer, "%5d/%-10d", cu, nu);
      prints (3, 14, LGREEN|_BLACK, buffer);
      if (sysusr->deleted)
         prints (3, 56, WHITE|_BLACK, "* DELETED *");
      else
         prints (3, 56, WHITE|_BLACK, "           ");
      prints (4, 56, LGREEN|_BLACK, sysusr->firstdate);
      prints (5, 56, LGREEN|_BLACK, sysusr->ldate);
   }

   if (which == USR_ALL || which == USR_NAME)
   {
      sprintf (buffer, "%-36s", sysusr->name);
      prints (5, 14, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_CITY)
   {
      sprintf (buffer, "%-26s", sysusr->city);
      prints (6, 14, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_PASSWORD)
   {
      memset (buffer, ' ', 16);
      memset (buffer, '*', strlen (sysusr->pwd));
      buffer[16] = '\0';
      prints (7, 14, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_VOICEPHONE)
   {
      sprintf(buffer, "%-19s", sysusr->voicephone);
      prints (8, 14, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_DATAPHONE)
   {
      sprintf(buffer, "%-20s", sysusr->dataphone);
      prints (8, 45, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_HANDLE) {
      sprintf(buffer, "%-35s", sysusr->handle);
      prints (7, 45, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_PRIV) {
      sprintf (buffer, "%-11s", GetPrivText(sysusr->priv));
      prints (9, 14, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_ONLINE) {
      sprintf(buffer, "%-5d", sysusr->time);
      prints (9, 45, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_TIMES) {
      sprintf(buffer, "%-5ld", sysusr->times);
      prints (9, 72, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_UPLOADS) {
      sprintf(buffer, "%5ld", sysusr->upld);
      prints (10, 14, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_DOWNLOADS) {
      sprintf(buffer, "%5ld", sysusr->dnld);
      prints (10, 45, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_DOWNTODAY) {
      sprintf(buffer, "%5u", sysusr->dnldl);
      prints (10, 72, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_VOTES) {
      sprintf(buffer, "%5d", sysusr->votes);
      prints (11, 14, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_FLAGS) {
      SetFlags (buffer, sysusr->flags);
      prints (11, 45, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_TIMEBANK) {
      sprintf(buffer, "%5d", sysusr->account);
      prints (12, 14, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_MSG) {
      sprintf(buffer, "%-5d", sysusr->msg);
      prints (12, 45, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_FILES) {
      sprintf(buffer, "%-5d", sysusr->files);
      prints (12, 72, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_KBANK) {
      sprintf(buffer, "%5d", sysusr->f_account);
      prints (13, 14, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_NERD)
      prints (13, 45, LGREEN|_BLACK, sysusr->nerd ? "YES" : "NO ");
   if (which == USR_ALL || which == USR_INLIST)
      prints (13, 72, LGREEN|_BLACK, sysusr->usrhidden ? "NO " : "YES");
   if (which == USR_ALL || which == USR_CREDIT)
   {
      sprintf(buffer, "%-5d", sysusr->credit);
      prints (14, 14, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_COLOR)
      prints (15, 14, LGREEN|_BLACK, sysusr->color ? "YES" : "NO ");
   if (which == USR_ALL || which == USR_TABS)
      prints (15, 45, LGREEN|_BLACK, sysusr->tabs ? "YES" : "NO ");
//   if (which == USR_ALL || which == USR_IBMSET)
//      prints (16, 14, LGREEN|_BLACK, sysusr->ibmset ? "YES" : "NO ");
   if (which == USR_ALL || which == USR_VIDEO) {
      if (sysusr->ansi)
         prints (16, 45, LGREEN|_BLACK, "Ansi  ");
      else if (sysusr->avatar)
         prints (16, 45, LGREEN|_BLACK, "Avatar");
      else
         prints (16, 45, LGREEN|_BLACK, "TTY   ");
   }
   if (which == USR_ALL || which == USR_FSED)
      prints (16, 72, LGREEN|_BLACK, sysusr->use_lore ? "NO " : "YES");
   if (which == USR_ALL || which == USR_MORE)
      prints (17, 14, LGREEN|_BLACK, sysusr->more ? "YES" : "NO ");
   if (which == USR_ALL || which == USR_FULLREAD)
      prints (17, 45, LGREEN|_BLACK, sysusr->full_read ? "YES" : "NO ");
   if (which == USR_ALL || which == USR_CLEAR)
      prints (17, 72, LGREEN|_BLACK, sysusr->formfeed ? "YES" : "NO ");
   if (which == USR_ALL || which == USR_WIDTH) {
      sprintf(buffer, "%-3d", sysusr->width);
      prints (18, 14, LGREEN|_BLACK, buffer);
   }
   if (which == USR_ALL || which == USR_LENGTH) {
      sprintf(buffer, "%-3d", sysusr->len);
      prints (18, 45, LGREEN|_BLACK, buffer);
   }
}

struct parse_list levels[] = {
   TWIT, "Twit",
   DISGRACE, "Disgrace",
   LIMITED, "Limited",
   NORMAL, "Normal",
   WORTHY, "Worthy",
   PRIVIL, "Privel",
   FAVORED, "Favored",
   EXTRA, "Extra",
   CLERK, "Clerk",
   ASSTSYSOP, "Asstsysop",
   SYSOP, "Sysop",
   HIDDEN, "Hidden"
};

char *GetPrivText (priv)
int priv;
{
   int i;
   char *txt;

   txt = "???";

   for (i=0;i<12;i++)
      if (levels[i].p_length == priv)
      {
         txt = levels[i].p_string;
         break;
      }

   return (txt);
}

long SaveReadUser (fd, psave, pread, usrp)
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
      usrp->name[35] = 0;
      usrp->handle[35] = 0;
      usrp->city[25] = 0;
      usrp->pwd[15] = 0;
      usrp->ldate[19] = 0;
      usrp->signature[57] = 0;
      usrp->voicephone[19] = 0;
      usrp->dataphone[19] = 0;
      usrp->birthdate[9] = 0;
      usrp->subscrdate[9] = 0;
      usrp->firstdate[19] = 0;
      usrp->lastpwdchange[9] = 0;
      usrp->comment[79] = 0;
   }

   return (pread);
}

int NextPrivilege (p)
int p;
{
   switch (p)
   {
   case TWIT:
      return (DISGRACE);
   case DISGRACE:
      return (LIMITED);
   case LIMITED:
      return (NORMAL);
   case NORMAL:
      return (WORTHY);
   case WORTHY:
      return (PRIVIL);
   case PRIVIL:
      return (FAVORED);
   case FAVORED:
      return (EXTRA);
   case EXTRA:
      return (CLERK);
   case CLERK:
      return (ASSTSYSOP);
   case ASSTSYSOP:
      return (SYSOP);
   case SYSOP:
      return (TWIT);
   }

   return (TWIT);
}

long GetFlags (p)
char *p;
{
   long flag;

   flag = 0L;

   while (*p)
      switch (toupper(*p++))
      {
      case 'V':
         flag |= 0x00000001L;
         break;
      case 'U':
         flag |= 0x00000002L;
         break;
      case 'T':
         flag |= 0x00000004L;
         break;
      case 'S':
         flag |= 0x00000008L;
         break;
      case 'R':
         flag |= 0x00000010L;
         break;
      case 'Q':
         flag |= 0x00000020L;
         break;
      case 'P':
         flag |= 0x00000040L;
         break;
      case 'O':
         flag |= 0x00000080L;
         break;
      case 'N':
         flag |= 0x00000100L;
         break;
      case 'M':
         flag |= 0x00000200L;
         break;
      case 'L':
         flag |= 0x00000400L;
         break;
      case 'K':
         flag |= 0x00000800L;
         break;
      case 'J':
         flag |= 0x00001000L;
         break;
      case 'I':
         flag |= 0x00002000L;
         break;
      case 'H':
         flag |= 0x00004000L;
         break;
      case 'G':
         flag |= 0x00008000L;
         break;
      case 'F':
         flag |= 0x00010000L;
         break;
      case 'E':
         flag |= 0x00020000L;
         break;
      case 'D':
         flag |= 0x00040000L;
         break;
      case 'C':
         flag |= 0x00080000L;
         break;
      case 'B':
         flag |= 0x00100000L;
         break;
      case 'A':
         flag |= 0x00200000L;
         break;
      case '9':
         flag |= 0x00400000L;
         break;
      case '8':
         flag |= 0x00800000L;
         break;
      case '7':
         flag |= 0x01000000L;
         break;
      case '6':
         flag |= 0x02000000L;
         break;
      case '5':
         flag |= 0x04000000L;
         break;
      case '4':
         flag |= 0x08000000L;
         break;
      case '3':
         flag |= 0x10000000L;
         break;
      case '2':
         flag |= 0x20000000L;
         break;
      case '1':
         flag |= 0x40000000L;
         break;
      case '0':
         flag |= 0x80000000L;
         break;
      }

   return (flag);
}

void SetFlags (buffer, flag)
char *buffer;
long flag;
{
   int i, x;

   memset (buffer, ' ', 36);
   buffer[36] = '\0';

   x = 0;

   for (i=0;i<8;i++)
   {
      if (flag & 0x80000000L)
         buffer[x++] = '0' + i;
      flag = flag << 1;
   }

   for (i=0;i<8;i++)
   {
      if (flag & 0x80000000L)
         buffer[x++] = ((i<2)?'8':'?') + i;
      flag = flag << 1;
   }

   for (i=0;i<8;i++)
   {
      if (flag & 0x80000000L)
         buffer[x++] = 'G' + i;
      flag = flag << 1;
   }

   for (i=0;i<8;i++)
   {
      if (flag & 0x80000000L)
         buffer[x++] = 'O' + i;
      flag = flag << 1;
   }
}

void PurgeUsers ()
{
   int fdo, fdn, fdi;
   struct _usr usr;
   struct _usridx usridx;

   unlink ("USERS.BAK");
   rename ("USERS.BBS", "USERS.BAK");

   fdo = shopen ("USERS.BAK", O_RDONLY|O_BINARY);
   fdn = cshopen ("USERS.BBS", O_WRONLY|O_BINARY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
   fdi = cshopen ("USERS.IDX", O_WRONLY|O_BINARY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);

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
}

