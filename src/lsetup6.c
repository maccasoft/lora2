#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <alloc.h>
#include <sys\stat.h>

#include "lsetup.h"
#include "lprot.h"

extern struct _configuration config;

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>
#include <cxl\cxlkey.h>

#ifdef __OS2__
void VioUpdate (void);
#endif

int sh_open (char *file, int shmode, int omode, int fmode);
char *get_priv_text (int);
void update_message (void);
long get_menu_crc (int n_elem);

static void display_menu (int n_elem);
static int list_menu_items (int beg, int n_elem);

#define MAX_ITEMS   100

struct _menu_header {
   char  name[14];
   short n_elem;
};

static struct _cmd *cmd = NULL;

static char *menu_types[] = {
   " 0 - Display only",
   " 1 - Gosub message area",
   " 2 - Gosub file area",
   " 3 - Logoff",
   " 4 - N/A",
   " 5 - Display file (anywhere)",
   " 6 - Yell at Sysop",
   " 7 - User status",
   " 8 - User list",
   " 9 - Version information",
   " 10 - Run external program",
   " 11 - Bulletin menu",
   " 12 - N/A",
   " 13 - Show quotes",
   " 14 - Clear goto menu",
   " 15 - Clear gosub menu",
   " 16 - Return to MAIN menu",
   " 17 - Change area",
   " 18 - N/A",
   " 19 - Kill message",
   " 20 - N/A",
   " 21 - Goto menu",
   " 22 - File list",
   " 23 - Download file",
   " 24 - File display",
   " 25 - Raw directory",
   " 26 - Kill file",
   " 27 - Set password",
   " 28 - Change language",
   " 29 - Set nulls",
   " 30 - Set screen length",
   " 31 - N/A",
   " 32 - Set 'more?' question",
   " 33 - Set screen clear",
   " 34 - Set editor",
   " 35 - Set location",
   " 36 - Set mail check at logon",
   " 37 - Set Avatar/0+",
   " 38 - Set ANSI",
   " 39 - Set color",
   " 40 - Edit new message",
   " 41 - Reply to message",
   " 42 - Save message",
   " 43 - Abort editing",
   " 44 - List message text",
   " 45 - Edit line",
   " 46 - Insert line",
   " 47 - Delete line",
   " 48 - Continue editing",
   " 49 - Change 'To' field",
   " 50 - Change subject",
   " 51 - Press enter to continue",
   " 52 - N/A",
   " 53 - Short message list",
   " 54 - Read next message",
   " 55 - Read previous message",
   " 56 - Read message non-stop",
   " 57 - Read parent message",
   " 58 - Read child message",
   " 59 - Check mail-box",
   " 60 - Inquire message",
   " 61 - Gosub menu",
   " 62 - Hurl file",
   " 63 - Forward message",
   " 64 - Read individual message",
   " 65 - Tag files for download",
   " 66 - Return to previous",
   " 67 - Clear menu stack",
   " 68 - Locate files",
   " 69 - Upload file",
   " 70 - Set signature",
   " 71 - Write log entry",
   " 72 - Override path",
   " 73 - New files list",
   " 74 - Read next mail",
   " 75 - Read previous mail",
   " 76 - Read mail non-stop",
   " 77 - Set full screen reader",
   " 78 - Send online message",
   " 79 - N/A",
   " 80 - N/A",
   " 81 - Users online",
   " 82 - List mail",
   " 83 - Comment to next caller",
   " 84 - Download from any area",
   " 85 - Read individual mail",
   " 86 - Tag areas",
   " 87 - ASCII download",
   " 88 - Resume download",
   " 89 - Verbose message list",
   " 90 - Time statistics",
   " 91 - Show account",
   " 92 - Deposit time",
   " 93 - Withdraw time",
   " 94 - N/A",
   " 95 - Who is where",
   " 96 - CB-chat system",
   " 97 - Display file (system)",
   " 98 - Display last callers",
   " 99 - Set alias",
   " 100 - Set voice phone",
   " 101 - Set data phone",
   " 102 - Archive contents",
   " 103 - N/A",
   " 104 - Set counter",
   " 105 - Usage graphic",
   " 106 - Set ho-keyed menu",
   " 107 - Add to BBS-list",
   " 108 - Short BBS list",
   " 109 - Long BBS list",
   " 110 - Change BBS-list",
   " 111 - Remove BBS from list",
   " 112 - QWK download",
   " 113 - Upload replies",
   " 114 - Deposit Kbytes",
   " 115 - Withdraw Kbytes",
   " 116 - Vote user",
   " 117 - Set IBM characters",
   " 118 - List areas w/new messages",
   " 119 - Write message to disk",
   " 120 - Download BBS-list",
   " 121 - Set user group",
   " 122 - Upload to filebox",
   " 123 - Download from filebox",
   " 124 - Set birthdate",
   " 125 - Set default protocol",
   " 126 - Set default archiver",
   " 127 - Kill from filebox",
   " 128 - List file in filebox",
   " 129 - List/Remove tagged files",
   " 130 - Set view kludge lines",
   " 131 - BlueWave download",
   NULL
};

static int last_sel;

int winputs (int wy, int wx, char *stri, char *fmt, int mode, char pad, int fieldattr, int textattr)
{
   int linelimit, pos;
   unsigned int c;
   char *result, *tmp, str[2], first;

   linelimit = strlen (fmt);

   if ((result = (char *)malloc (linelimit + 1)) == NULL)
      return (-1);

   if ((tmp = (char *)malloc (linelimit + 1)) == NULL) {
      free (result);
      return (-1);
   }

   memset (tmp, pad, linelimit);
   tmp[linelimit] = '\0';
   wprints (wy, wx, fieldattr, tmp);

   if (mode == 0)
      strcpy (result, "");
   else
      strcpy (result, stri);

   if (mode == 2)
      first = 1;
   else
      first = 0;

   pos = strlen (result);
   wprints (wy, wx, textattr, result);
   wgotoxy (wy, wx + pos);

   showcur ();

   for (;;) {
#ifdef __OS2__
      VioUpdate ();
#endif
      c = getxch ();

      if ((c & 0xFF) != 0) {
         c &= 0xFF;

         if (c >= 32 && c <= 255) {
            if (first) {
               memset (tmp, pad, linelimit);
               tmp[linelimit] = '\0';
               wprints (wy, wx, fieldattr, tmp);

               strcpy (result, "");
               first = 0;

               pos = strlen (result);
               wprints (wy, wx, textattr, result);
               wgotoxy (wy, wx + pos);
            }

            if (strlen (result) < linelimit) {
               str[0] = c;
               str[1] = '\0';

               strcpy (tmp, &result[pos]);
               strcpy (&result[pos], str);
               strcat (&result[pos], tmp);
               result[linelimit] = '\0';

               if (pos < linelimit - 1)
                  pos++;

               wprints (wy, wx, textattr, result);
               wgotoxy (wy, wx + pos);
            }
         }

         else if (c == 0x08) {
            first = 0;

            if (pos) {
               strcpy (&result[pos - 1], &result[pos]);
               pos--;

               wprints (wy, wx, textattr, result);
               wgotoxy (wy, wx + pos);

               memset (tmp, pad, linelimit);
               tmp[linelimit] = '\0';
               wprints (wy, wx + strlen (result), fieldattr, &tmp[strlen (result)]);
            }
         }

         else if (c == 0x0D || c == 0x1B)
            break;

         else {
            if (first) {
               memset (tmp, pad, linelimit);
               tmp[linelimit] = '\0';
               wprints (wy, wx, fieldattr, tmp);

               strcpy (result, "");
               first = 0;

               pos = strlen (result);
               wprints (wy, wx, textattr, result);
               wgotoxy (wy, wx + pos);
            }

            if (strlen (result) < linelimit) {
               str[0] = c;
               str[1] = '\0';

               strcpy (tmp, &result[pos]);
               strcpy (&result[pos], str);
               strcat (&result[pos], tmp);
               result[linelimit] = '\0';

               if (pos < linelimit - 1)
                  pos++;

               wprints (wy, wx, textattr, result);
               wgotoxy (wy, wx + pos);
            }
         }
      }

      // DEL
      else if (c == 0x5300) {
         first = 0;

         if (result[pos]) {
            strcpy (&result[pos], &result[pos + 1]);

            wprints (wy, wx, textattr, result);
            wgotoxy (wy, wx + pos);

            memset (tmp, pad, linelimit);
            tmp[linelimit] = '\0';
            wprints (wy, wx + strlen (result), fieldattr, &tmp[strlen (result)]);
         }
      }

      else if (c == 0x4B00) {
         first = 0;

         if (pos) {
            pos--;
            wgotoxy (wy, wx + pos);
         }
      }

      else if (c == 0x4D00) {
         first = 0;

         if (result[pos]) {
            pos++;
            wgotoxy (wy, wx + pos);
         }
      }

      else if (c == 0x4700) {
         first = 0;

         pos = 0;
         wgotoxy (wy, wx + pos);
      }

      else if (c == 0x4F00) {
         first = 0;

         pos = strlen (result);
         if (pos >= linelimit - 1)
            pos--;
         wgotoxy (wy, wx + pos);
      }

      else if (c == 0x7400) {
         first = 0;

         while (result[pos] != ' ' && result[pos] != '\0')
            pos++;
         while (result[pos] == ' ')
            pos++;
         if (!result[pos])
            pos--;
         wgotoxy (wy, wx + pos);
      }

      else if (c == 0x7300) {
         first = 0;

         if (pos) {
            if (result[pos] == ' ' || result[pos - 1] == ' ') {
               pos--;
               while (pos && result[pos] == ' ')
                  pos--;
            }
            while (result[pos] != ' ' && pos)
               pos--;
            if (result[pos] == ' ')
               pos++;
            wgotoxy (wy, wx + pos);
         }
      }
   }

   if (c == 0x0D)
      strcpy (stri, result);

   free (result);
   free (tmp);

   return (c == 0x0D ? 0 : W_ESCPRESS);
}

static void set_selection (void)
{
   struct _item_t *item;

   item = wmenuicurr ();
   last_sel = item->tagid;
}

static int select_menu_type (int start)
{
   int i, wh;

   wh = wopen (5, 28, 18, 51, 3, LCYAN|_BLACK, LCYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Menu types ", TRIGHT, YELLOW|_BLUE);

   wmenubegc ();
   wmenuitem (0, 0, " Moving between menus ", 'M', 1000, 0, NULL, 0, 0);
      wmenubeg (9, 40, 19, 73, 3, LCYAN|_BLACK, LCYAN|_BLACK, NULL);
      wmenuitem (0, 0, menu_types[14], 0, 14, M_CLALL, set_selection, 0, 0);
      wmenuitem (1, 0, menu_types[15], 0, 15, M_CLALL, set_selection, 0, 0);
      wmenuitem (2, 0, menu_types[16], 0, 16, M_CLALL, set_selection, 0, 0);
      wmenuitem (3, 0, menu_types[21], 0, 21, M_CLALL, set_selection, 0, 0);
      wmenuitem (4, 0, menu_types[61], 0, 61, M_CLALL, set_selection, 0, 0);
      wmenuitem (5, 0, menu_types[66], 0, 66, M_CLALL, set_selection, 0, 0);
      wmenuitem (6, 0, menu_types[67], 0, 67, M_CLALL, set_selection, 0, 0);
      wmenuitem (7, 0, menu_types[1], 0, 1, M_CLALL, set_selection, 0, 0);
      wmenuitem (8, 0, menu_types[2], 0, 2, M_CLALL, set_selection, 0, 0);
      wmenuend (14, M_OMNI|M_PD|M_SAVE, 33, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (1, 0, " Message areas ", 'F', 2000, 0, NULL, 0, 0);
      wmenubeg (4, 38, 21, 73, 3, LCYAN|_BLACK, LCYAN|_BLACK, NULL);
      wmenuitem (0, 0, menu_types[19], 0, 19, M_CLALL, set_selection, 0, 0);
      wmenuitem (1, 0, menu_types[40], 0, 40, M_CLALL, set_selection, 0, 0);
      wmenuitem (2, 0, menu_types[41], 0, 41, M_CLALL, set_selection, 0, 0);
      wmenuitem (3, 0, menu_types[53], 0, 53, M_CLALL, set_selection, 0, 0);
      wmenuitem (4, 0, menu_types[54], 0, 54, M_CLALL, set_selection, 0, 0);
      wmenuitem (5, 0, menu_types[55], 0, 55, M_CLALL, set_selection, 0, 0);
      wmenuitem (6, 0, menu_types[56], 0, 56, M_CLALL, set_selection, 0, 0);
      wmenuitem (7, 0, menu_types[57], 0, 57, M_CLALL, set_selection, 0, 0);
      wmenuitem (8, 0, menu_types[58], 0, 58, M_CLALL, set_selection, 0, 0);
      wmenuitem (9, 0, menu_types[59], 0, 59, M_CLALL, set_selection, 0, 0);
      wmenuitem (10, 0, menu_types[60], 0, 60, M_CLALL, set_selection, 0, 0);
      wmenuitem (11, 0, menu_types[63], 0, 63, M_CLALL, set_selection, 0, 0);
      wmenuitem (12, 0, menu_types[64], 0, 64, M_CLALL, set_selection, 0, 0);
      wmenuitem (13, 0, menu_types[89], 0, 89, M_CLALL, set_selection, 0, 0);
      wmenuitem (14, 0, menu_types[118], 0, 118, M_CLALL, set_selection, 0, 0);
      wmenuitem (15, 0, menu_types[119], 0, 119, M_CLALL, set_selection, 0, 0);
      wmenuend (19, M_OMNI|M_PD|M_SAVE, 35, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (2, 0, " File areas ", 'F', 3000, 0, NULL, 0, 0);
      wmenubeg (4, 40, 21, 73, 3, LCYAN|_BLACK, LCYAN|_BLACK, NULL);
      wmenuitem (0, 0, menu_types[22], 0, 22, M_CLALL, set_selection, 0, 0);
      wmenuitem (1, 0, menu_types[23], 0, 23, M_CLALL, set_selection, 0, 0);
      wmenuitem (2, 0, menu_types[24], 0, 24, M_CLALL, set_selection, 0, 0);
      wmenuitem (3, 0, menu_types[25], 0, 25, M_CLALL, set_selection, 0, 0);
      wmenuitem (4, 0, menu_types[65], 0, 65, M_CLALL, set_selection, 0, 0);
      wmenuitem (5, 0, menu_types[68], 0, 68, M_CLALL, set_selection, 0, 0);
      wmenuitem (6, 0, menu_types[69], 0, 69, M_CLALL, set_selection, 0, 0);
      wmenuitem (7, 0, menu_types[72], 0, 72, M_CLALL, set_selection, 0, 0);
      wmenuitem (8, 0, menu_types[73], 0, 73, M_CLALL, set_selection, 0, 0);
      wmenuitem (9, 0, menu_types[84], 0, 84, M_CLALL, set_selection, 0, 0);
      wmenuitem (10, 0, menu_types[102], 0, 102, M_CLALL, set_selection, 0, 0);
      wmenuitem (11, 0, menu_types[122], 0, 122, M_CLALL, set_selection, 0, 0);
      wmenuitem (12, 0, menu_types[123], 0, 123, M_CLALL, set_selection, 0, 0);
      wmenuitem (13, 0, menu_types[127], 0, 127, M_CLALL, set_selection, 0, 0);
      wmenuitem (14, 0, menu_types[128], 0, 128, M_CLALL, set_selection, 0, 0);
      wmenuitem (15, 0, menu_types[129], 0, 129, M_CLALL, set_selection, 0, 0);
      wmenuend (22, M_OMNI|M_PD|M_SAVE, 33, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (3, 0, " User configuration 1", 'F', 4000, 0, NULL, 0, 0);
      wmenubeg (4, 40, 18, 73, 3, LCYAN|_BLACK, LCYAN|_BLACK, NULL);
      wmenuitem (0, 0, menu_types[27], 0, 27, M_CLALL, set_selection, 0, 0);
      wmenuitem (1, 0, menu_types[28], 0, 28, M_CLALL, set_selection, 0, 0);
      wmenuitem (2, 0, menu_types[29], 0, 29, M_CLALL, set_selection, 0, 0);
      wmenuitem (3, 0, menu_types[30], 0, 30, M_CLALL, set_selection, 0, 0);
      wmenuitem (4, 0, menu_types[32], 0, 32, M_CLALL, set_selection, 0, 0);
      wmenuitem (5, 0, menu_types[33], 0, 33, M_CLALL, set_selection, 0, 0);
      wmenuitem (6, 0, menu_types[34], 0, 34, M_CLALL, set_selection, 0, 0);
      wmenuitem (7, 0, menu_types[35], 0, 35, M_CLALL, set_selection, 0, 0);
      wmenuitem (8, 0, menu_types[36], 0, 36, M_CLALL, set_selection, 0, 0);
      wmenuitem (9, 0, menu_types[37], 0, 37, M_CLALL, set_selection, 0, 0);
      wmenuitem (10, 0, menu_types[38], 0, 38, M_CLALL, set_selection, 0, 0);
      wmenuitem (11, 0, menu_types[39], 0, 39, M_CLALL, set_selection, 0, 0);
      wmenuitem (12, 0, menu_types[70], 0, 70, M_CLALL, set_selection, 0, 0);
      wmenuend (27, M_OMNI|M_PD|M_SAVE, 33, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (4, 0, " User configuration 2", 'F', 4500, 0, NULL, 0, 0);
      wmenubeg (7, 40, 19, 73, 3, LCYAN|_BLACK, LCYAN|_BLACK, NULL);
      wmenuitem (0, 0, menu_types[77], 0, 77, M_CLALL, set_selection, 0, 0);
      wmenuitem (1, 0, menu_types[99], 0, 99, M_CLALL, set_selection, 0, 0);
      wmenuitem (2, 0, menu_types[100], 0, 100, M_CLALL, set_selection, 0, 0);
      wmenuitem (3, 0, menu_types[101], 0, 101, M_CLALL, set_selection, 0, 0);
      wmenuitem (4, 0, menu_types[106], 0, 106, M_CLALL, set_selection, 0, 0);
      wmenuitem (5, 0, menu_types[117], 0, 117, M_CLALL, set_selection, 0, 0);
      wmenuitem (6, 0, menu_types[121], 0, 121, M_CLALL, set_selection, 0, 0);
      wmenuitem (7, 0, menu_types[124], 0, 124, M_CLALL, set_selection, 0, 0);
      wmenuitem (8, 0, menu_types[125], 0, 125, M_CLALL, set_selection, 0, 0);
      wmenuitem (9, 0, menu_types[126], 0, 126, M_CLALL, set_selection, 0, 0);
      wmenuitem (10, 0, menu_types[130], 0, 130, M_CLALL, set_selection, 0, 0);
      wmenuend (77, M_OMNI|M_PD|M_SAVE, 33, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (5, 0, " Line editor ", 'F', 5000, 0, NULL, 0, 0);
      wmenubeg (9, 40, 19, 73, 3, LCYAN|_BLACK, LCYAN|_BLACK, NULL);
      wmenuitem (0, 0, menu_types[42], 0, 42, M_CLALL, set_selection, 0, 0);
      wmenuitem (1, 0, menu_types[43], 0, 43, M_CLALL, set_selection, 0, 0);
      wmenuitem (2, 0, menu_types[44], 0, 44, M_CLALL, set_selection, 0, 0);
      wmenuitem (3, 0, menu_types[45], 0, 45, M_CLALL, set_selection, 0, 0);
      wmenuitem (4, 0, menu_types[46], 0, 46, M_CLALL, set_selection, 0, 0);
      wmenuitem (5, 0, menu_types[47], 0, 47, M_CLALL, set_selection, 0, 0);
      wmenuitem (6, 0, menu_types[48], 0, 48, M_CLALL, set_selection, 0, 0);
      wmenuitem (7, 0, menu_types[49], 0, 49, M_CLALL, set_selection, 0, 0);
      wmenuitem (8, 0, menu_types[50], 0, 50, M_CLALL, set_selection, 0, 0);
      wmenuend (42, M_OMNI|M_PD|M_SAVE, 33, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (6, 0, " Personal mail ", 'F', 6000, 0, NULL, 0, 0);
      wmenubeg (9, 40, 15, 73, 3, LCYAN|_BLACK, LCYAN|_BLACK, NULL);
      wmenuitem (0, 0, menu_types[74], 0, 74, M_CLALL, set_selection, 0, 0);
      wmenuitem (1, 0, menu_types[75], 0, 75, M_CLALL, set_selection, 0, 0);
      wmenuitem (2, 0, menu_types[76], 0, 76, M_CLALL, set_selection, 0, 0);
      wmenuitem (3, 0, menu_types[82], 0, 82, M_CLALL, set_selection, 0, 0);
      wmenuitem (4, 0, menu_types[85], 0, 85, M_CLALL, set_selection, 0, 0);
      wmenuend (74, M_OMNI|M_PD|M_SAVE, 33, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (7, 0, " Multiline chat ", 'F', 7000, 0, NULL, 0, 0);
      wmenubeg (9, 40, 14, 73, 3, LCYAN|_BLACK, LCYAN|_BLACK, NULL);
      wmenuitem (0, 0, menu_types[78], 0, 78, M_CLALL, set_selection, 0, 0);
      wmenuitem (1, 0, menu_types[81], 0, 81, M_CLALL, set_selection, 0, 0);
      wmenuitem (2, 0, menu_types[95], 0, 95, M_CLALL, set_selection, 0, 0);
      wmenuitem (3, 0, menu_types[96], 0, 96, M_CLALL, set_selection, 0, 0);
      wmenuend (78, M_OMNI|M_PD|M_SAVE, 33, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (8, 0, " Offline reader ", 'F', 8000, 0, NULL, 0, 0);
      wmenubeg (9, 40, 16, 73, 3, LCYAN|_BLACK, LCYAN|_BLACK, NULL);
      wmenuitem (0, 0, menu_types[86], 0, 86, M_CLALL, set_selection, 0, 0);
      wmenuitem (1, 0, menu_types[87], 0, 87, M_CLALL, set_selection, 0, 0);
      wmenuitem (2, 0, menu_types[88], 0, 88, M_CLALL, set_selection, 0, 0);
      wmenuitem (3, 0, menu_types[112], 0, 112, M_CLALL, set_selection, 0, 0);
      wmenuitem (4, 0, menu_types[113], 0, 113, M_CLALL, set_selection, 0, 0);
      wmenuitem (5, 0, menu_types[131], 0, 131, M_CLALL, set_selection, 0, 0);
      wmenuend (86, M_OMNI|M_PD|M_SAVE, 33, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (9, 0, " Built-in doors ", 'F', 8500, 0, NULL, 0, 0);
      wmenubeg (6, 40, 18, 73, 3, LCYAN|_BLACK, LCYAN|_BLACK, NULL);
      wmenuitem (0, 0, menu_types[91], 0, 91, M_CLALL, set_selection, 0, 0);
      wmenuitem (1, 0, menu_types[92], 0, 92, M_CLALL, set_selection, 0, 0);
      wmenuitem (2, 0, menu_types[93], 0, 93, M_CLALL, set_selection, 0, 0);
      wmenuitem (3, 0, menu_types[114], 0, 114, M_CLALL, set_selection, 0, 0);
      wmenuitem (4, 0, menu_types[115], 0, 115, M_CLALL, set_selection, 0, 0);
      wmenuitem (5, 0, menu_types[107], 0, 107, M_CLALL, set_selection, 0, 0);
      wmenuitem (6, 0, menu_types[108], 0, 108, M_CLALL, set_selection, 0, 0);
      wmenuitem (7, 0, menu_types[109], 0, 109, M_CLALL, set_selection, 0, 0);
      wmenuitem (8, 0, menu_types[110], 0, 110, M_CLALL, set_selection, 0, 0);
      wmenuitem (9, 0, menu_types[111], 0, 111, M_CLALL, set_selection, 0, 0);
      wmenuitem (10, 0, menu_types[120], 0, 120, M_CLALL, set_selection, 0, 0);
      wmenuend (91, M_OMNI|M_PD|M_SAVE, 33, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (10, 0, " Miscellaneous 1", 'F', 9000, 0, NULL, 0, 0);
      wmenubeg (6, 40, 18, 73, 3, LCYAN|_BLACK, LCYAN|_BLACK, NULL);
      wmenuitem (0, 0, menu_types[0], 0, 999, M_CLALL, set_selection, 0, 0);
      wmenuitem (1, 0, menu_types[3], 0, 3, M_CLALL, set_selection, 0, 0);
      wmenuitem (2, 0, menu_types[5], 0, 5, M_CLALL, set_selection, 0, 0);
      wmenuitem (3, 0, menu_types[6], 0, 6, M_CLALL, set_selection, 0, 0);
      wmenuitem (4, 0, menu_types[7], 0, 7, M_CLALL, set_selection, 0, 0);
      wmenuitem (5, 0, menu_types[8], 0, 8, M_CLALL, set_selection, 0, 0);
      wmenuitem (6, 0, menu_types[9], 0, 9, M_CLALL, set_selection, 0, 0);
      wmenuitem (7, 0, menu_types[10], 0, 10, M_CLALL, set_selection, 0, 0);
      wmenuitem (8, 0, menu_types[11], 0, 11, M_CLALL, set_selection, 0, 0);
      wmenuitem (9, 0, menu_types[13], 0, 13, M_CLALL, set_selection, 0, 0);
      wmenuitem (10, 0, menu_types[17], 0, 17, M_CLALL, set_selection, 0, 0);
      wmenuend (999, M_OMNI|M_PD|M_SAVE, 33, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);
   wmenuitem (11, 0, " Miscellaneous 2", 'F', 9500, 0, NULL, 0, 0);
      wmenubeg (6, 40, 15, 73, 3, LCYAN|_BLACK, LCYAN|_BLACK, NULL);
      wmenuitem (0, 0, menu_types[51], 0, 51, M_CLALL, set_selection, 0, 0);
      wmenuitem (1, 0, menu_types[71], 0, 71, M_CLALL, set_selection, 0, 0);
      wmenuitem (2, 0, menu_types[83], 0, 83, M_CLALL, set_selection, 0, 0);
      wmenuitem (3, 0, menu_types[90], 0, 90, M_CLALL, set_selection, 0, 0);
      wmenuitem (4, 0, menu_types[97], 0, 97, M_CLALL, set_selection, 0, 0);
      wmenuitem (5, 0, menu_types[98], 0, 98, M_CLALL, set_selection, 0, 0);
      wmenuitem (6, 0, menu_types[104], 0, 104, M_CLALL, set_selection, 0, 0);
      wmenuitem (7, 0, menu_types[105], 0, 105, M_CLALL, set_selection, 0, 0);
      wmenuend (51, M_OMNI|M_PD|M_SAVE, 33, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);

   wmenuend (1000, M_OMNI, 22, 0, LGREY|_BLACK, LGREY|_BLACK, DGREY|_BLACK, BLUE|_LGREY);

   last_sel = -1;
   i = wmenuget ();
   if (last_sel == 999)
      last_sel = 0;
   wclose ();

   return (i == -1 ? start : last_sel);
}

int select_color (int color)
{
   int x, y, i, wh;

   wh = wopen (7, 30, 16, 47, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Colors ", TRIGHT, YELLOW|_BLUE);

   for (y = 0; y < 8; y++) {
      for (x = 0; x < 16; x++)
         wprints (y, x, x | (y << 4), "");
   }

   x = color & 0x0F;
   y = (color & 0xF0) >> 4;

   wprints (y, x, LGREY|_BLACK, "");

   for (;;) {
      i = getxch ();
      if ( (i & 0xFF) != 0 )
         i &= 0xFF;

      switch (i) {
         case 0x4800:
            if (y > 0) {
               wprints (y, x, x | (y << 4), "");
               y--;
               wprints (y, x, LGREY|_BLACK, "");
            }
            break;

         case 0x5000:
            if (y < 7) {
               wprints (y, x, x | (y << 4), "");
               y++;
               wprints (y, x, LGREY|_BLACK, "");
            }
            break;

         case 0x4B00:
            if (x > 0) {
               wprints (y, x, x | (y << 4), "");
               x--;
               wprints (y, x, LGREY|_BLACK, "");
            }
            break;

         case 0x4D00:
            if (x < 15) {
               wprints (y, x, x | (y << 4), "");
               x++;
               wprints (y, x, LGREY|_BLACK, "");
            }
            break;
      }

      if (i == 0x1B) {
         color = -1;
         break;
      }
      else if (i == 0x0D) {
         color = x | (y << 4);
         break;
      }
   }

   wclose ();
   return (color);
}

static int edit_single_item (struct _cmd *cmd)
{
   int i = 1, wh1;
   char string[80];
   struct _cmd mi;

   memcpy (&mi, cmd, sizeof (struct _cmd));

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
      wprints (0, 16, LGREY|_BLACK, "0....5...10...15...20...25...30...35...40...45...50...55.");

      wmenuitem ( 1, 1, " Display      ", 0,  1, 0, NULL, 0, 0);
      wmenuitem ( 2, 1, " Type         ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 3, 1, " Data         ", 0,  3, 0, NULL, 0, 0);
      wmenuitem ( 4, 1, " Hot-key      ", 0,  4, 0, NULL, 0, 0);
      wmenuitem ( 5, 1, " Automatic    ", 0,  5, 0, NULL, 0, 0);
      wmenuitem ( 6, 1, " First time   ", 0,  6, 0, NULL, 0, 0);
      wmenuitem ( 7, 1, " Hide display ", 0,  7, 0, NULL, 0, 0);
      wmenuitem ( 8, 1, " Color        ", 0,  8, 0, NULL, 0, 0);
      wmenuitem ( 9, 1, " Hilight      ", 0,  9, 0, NULL, 0, 0);
      wmenuitem (10, 1, " Security     ", 0, 10, 0, NULL, 0, 0);
      wmenuitem (11, 1, " Flags-A      ", 0, 11, 0, NULL, 0, 0);
      wmenuitem (12, 1, " Flags-B      ", 0, 12, 0, NULL, 0, 0);
      wmenuitem (13, 1, " Flags-C      ", 0, 13, 0, NULL, 0, 0);
      wmenuitem (14, 1, " Flags-D      ", 0, 14, 0, NULL, 0, 0);
      wmenuend (i, M_VERT|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      wprints (1, 16, CYAN|_BLACK, mi.name);
      wprints (2, 15, CYAN|_BLACK, menu_types[mi.flag_type]);
      wprints (3, 16, CYAN|_BLACK, mi.argument);
      sprintf (string, "%c", mi.hotkey);
      wprints (4, 16, CYAN|_BLACK, string);
      if (mi.automatic)
         wprints (5, 16, CYAN|_BLACK, "Yes");
      else
         wprints (5, 16, CYAN|_BLACK, "No");
      if (mi.first_time)
         wprints (6, 16, CYAN|_BLACK, "Yes");
      else
         wprints (6, 16, CYAN|_BLACK, "No");
      if (mi.no_display)
         wprints (7, 16, CYAN|_BLACK, "Yes");
      else
         wprints (7, 16, CYAN|_BLACK, "No");
      wprints (8, 16, mi.color, "Sample color text");
      wprints (9, 16, mi.first_color, "Sample color text");
      wprints (10, 15, CYAN|_BLACK, get_priv_text (mi.priv));
      wprints (11, 16, CYAN|_BLACK, get_flagA_text ((mi.flags >> 24) & 0xFF));
      wprints (12, 16, CYAN|_BLACK, get_flagB_text ((mi.flags >> 16) & 0xFF));
      wprints (13, 16, CYAN|_BLACK, get_flagC_text ((mi.flags >> 8) & 0xFF));
      wprints (14, 16, CYAN|_BLACK, get_flagD_text (mi.flags & 0xFF));

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
            strcpy (string, mi.name);
            if (winputs (1, 16, string, "?????????????????????????????????????????????????????????", 1, '±', BLUE|_GREEN, BLUE|_GREEN) != W_ESCPRESS)
               strcpy (mi.name, string);
//            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
//            winpdef (1, 16, string, "?????????????????????????????????????????????????", 0, 1, NULL, 0);
//            if (winpread () != W_ESCPRESS)
//               strcpy (mi.name, strrtrim (string));
            break;

         case 2:
            mi.flag_type = select_menu_type (mi.flag_type);
            break;

         case 3:
            strcpy (string, mi.argument);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (3, 16, string, "?????????????????????????????????????????????????", 0, 1, NULL, 0);
            if (winpread () != W_ESCPRESS)
               strcpy (mi.argument, strbtrim (string));
            break;

         case 4:
            sprintf (string, "%c", mi.hotkey);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (4, 16, string, "?", 0, 1, NULL, 0);
            if (winpread () != W_ESCPRESS)
               mi.hotkey = toupper (string[0]);
            break;

         case 5:
            mi.automatic ^= 1;
            break;

         case 6:
            mi.first_time ^= 1;
            break;

         case 7:
            mi.no_display ^= 1;
            break;

         case 8:
            i = select_color (mi.color);
            if (i != -1)
               mi.color = i;
            i = 8;
            break;

         case 9:
            i = select_color (mi.first_color);
            if (i != -1)
               mi.first_color = i;
            i = 9;
            break;

         case 10:
            mi.priv = select_level (mi.priv, 47, 4);
            break;

         case 11:
            strcpy (string, get_flag_text ((mi.flags >> 24) & 0xFF));
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (11, 16, string, "????????", 0, 1, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               mi.flags &= 0x00FFFFFFL;
               mi.flags |= recover_flags (string) << 24;
            }
            break;

         case 12:
            strcpy (string, get_flag_text ((mi.flags >> 16) & 0xFF));
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (12, 16, string, "????????", 0, 1, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               mi.flags &= 0xFF00FFFFL;
               mi.flags |= recover_flags (string) << 16;
            }
            break;

         case 13:
            strcpy (string, get_flag_text ((mi.flags >> 8) & 0xFF));
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (13, 16, string, "????????", 0, 1, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               mi.flags &= 0xFFFF00FFL;
               mi.flags |= recover_flags (string) << 8;
            }
            break;

         case 14:
            strcpy (string, get_flag_text (mi.flags & 0xFF));
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (14, 16, string, "????????", 0, 1, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               mi.flags &= 0xFFFFFF00L;
               mi.flags |= recover_flags (string);
            }
            break;
      }

      hidecur ();
   } while (i != -1);

   if (memcmp ((char *)&mi, (char *)cmd, sizeof (struct _cmd))) {
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

      if (toupper (string[0]) == 'Y') {
         memcpy ((char *)cmd, (char *)&mi, sizeof (struct _cmd));
         i = 1;
      }
      else
         i = 0;
   }
   else
      i = 0;

   gotoxy_ (24, 1);
   clreol_ ();
   prints (24, 1, LGREY|_BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add  C-Copy  L-List  D-Delete  S-Show");
   prints (24, 1, YELLOW|_BLACK, "PgUp/PgDn");
   prints (24, 26, YELLOW|_BLACK, "E");
   prints (24, 34, YELLOW|_BLACK, "A");
   prints (24, 41, YELLOW|_BLACK, "C");
   prints (24, 49, YELLOW|_BLACK, "L");
   prints (24, 57, YELLOW|_BLACK, "D");
   prints (24, 67, YELLOW|_BLACK, "S");


   return (i);
}

static void edit_menu_items (char *file, char *name)
{
   FILE *fp, *fpd;
   int fd, i, beg = 0, wh, elements, wh1;
   char filename[80], string[80], newname[30];
   long crc;
   struct _menu_header mh;
   struct _cmd mi;

   memset (&mi, 0, sizeof (struct _cmd));

   sprintf (filename, "%s%s.MNU", config.menu_path, file);
   fd = sh_open (filename, SH_DENYWR, O_RDONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
   if (fd == -1)
      return;

   i = 0;
   while (read (fd, &mh, sizeof (struct _menu_header)) == sizeof (struct _menu_header)) {
      if (!stricmp (mh.name, name))
         break;
      lseek (fd, (long)sizeof(struct _cmd) * mh.n_elem, SEEK_CUR);
   }

   cmd = (struct _cmd *)malloc (MAX_ITEMS * sizeof (struct _cmd));
   if (cmd == NULL)
      return;

   i = mh.n_elem;
   if (i > MAX_ITEMS)
      i = MAX_ITEMS;

   read (fd, (char *)cmd, sizeof (struct _cmd) * i);
   close (fd);

   crc = get_menu_crc (i);
   elements = mh.n_elem;

   gotoxy_ (24, 1);
   clreol_ ();
   prints (24, 1, LGREY|_BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add  C-Copy  L-List  D-Delete  S-Show");
   prints (24, 1, YELLOW|_BLACK, "PgUp/PgDn");
   prints (24, 26, YELLOW|_BLACK, "E");
   prints (24, 34, YELLOW|_BLACK, "A");
   prints (24, 41, YELLOW|_BLACK, "C");
   prints (24, 49, YELLOW|_BLACK, "L");
   prints (24, 57, YELLOW|_BLACK, "D");
   prints (24, 67, YELLOW|_BLACK, "S");

   wh = wopen (3, 1, 20, 76, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Menu ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();
      wprints (0, 16, LGREY|_BLACK, "0....5...10...15...20...25...30...35...40...45...50...55.");

      wprints ( 1, 1, LGREY|_BLACK, " Display      ");
      wprints ( 2, 1, LGREY|_BLACK, " Type         ");
      wprints ( 3, 1, LGREY|_BLACK, " Data         ");
      wprints ( 4, 1, LGREY|_BLACK, " Hot-key      ");
      wprints ( 5, 1, LGREY|_BLACK, " Automatic    ");
      wprints ( 6, 1, LGREY|_BLACK, " First time   ");
      wprints ( 7, 1, LGREY|_BLACK, " Hide display ");
      wprints ( 8, 1, LGREY|_BLACK, " Color        ");
      wprints ( 9, 1, LGREY|_BLACK, " Hilight      ");
      wprints (10, 1, LGREY|_BLACK, " Security     ");
      wprints (11, 1, LGREY|_BLACK, " Flags-A      ");
      wprints (12, 1, LGREY|_BLACK, " Flags-B      ");
      wprints (13, 1, LGREY|_BLACK, " Flags-C      ");
      wprints (14, 1, LGREY|_BLACK, " Flags-D      ");

      if (mh.n_elem) {
         wprints (1, 16, CYAN|_BLACK, cmd[beg].name);
         wprints (2, 15, CYAN|_BLACK, menu_types[cmd[beg].flag_type]);
         wprints (3, 16, CYAN|_BLACK, cmd[beg].argument);
         sprintf (string, "%c", cmd[beg].hotkey);
         wprints (4, 16, CYAN|_BLACK, string);
         if (cmd[beg].automatic)
            wprints (5, 16, CYAN|_BLACK, "Yes");
         else
            wprints (5, 16, CYAN|_BLACK, "No");
         if (cmd[beg].first_time)
            wprints (6, 16, CYAN|_BLACK, "Yes");
         else
            wprints (6, 16, CYAN|_BLACK, "No");
         if (cmd[beg].no_display)
            wprints (7, 16, CYAN|_BLACK, "Yes");
         else
            wprints (7, 16, CYAN|_BLACK, "No");
         wprints (8, 16, cmd[beg].color, "Sample color text");
         wprints (9, 16, cmd[beg].first_color, "Sample color text");
         wprints (10, 15, CYAN|_BLACK, get_priv_text (cmd[beg].priv));
         wprints (11, 16, CYAN|_BLACK, get_flagA_text ((cmd[beg].flags >> 24) & 0xFF));
         wprints (12, 16, CYAN|_BLACK, get_flagB_text ((cmd[beg].flags >> 16) & 0xFF));
         wprints (13, 16, CYAN|_BLACK, get_flagC_text ((cmd[beg].flags >> 8) & 0xFF));
         wprints (14, 16, CYAN|_BLACK, get_flagD_text (cmd[beg].flags & 0xFF));
      }

      start_update ();
      i = getxch ();
      if ( (i & 0xFF) != 0 )
         i &= 0xFF;

      switch (i) {
         // PgDn
         case 0x5100:
            if (beg < mh.n_elem - 1)
               beg++;
            break;

         // PgUp
         case 0x4900:
            if (beg > 0)
               beg--;
            break;

         // E Edit
         case 'E':
         case 'e':
            if (mh.n_elem) {
               memcpy (&mi, &cmd[beg], sizeof (struct _cmd));
               edit_single_item (&mi);
               memcpy (&cmd[beg], &mi, sizeof (struct _cmd));
               break;
            }
            // Se si tratta del primo elemento, passa direttamente alla
            // funzione di add.

         // A Add
         case 'A':
         case 'a':
            if (elements + 1 >= MAX_ITEMS)
               break;

            memset (&mi, 0, sizeof (struct _cmd));
            if (mh.n_elem) {
               mi.color = cmd[beg].color;
               mi.first_color = cmd[beg].first_color;
               mi.priv = cmd[beg].priv;
            }
            else {
               mi.color = 15;
               mi.first_color = 14;
               mi.priv = TWIT;
            }

            if (edit_single_item (&mi)) {
               if (mh.n_elem) {
                  for (i = mh.n_elem - 1; i > beg; i--)
                     memcpy (&cmd[i + 1], &cmd[i], sizeof (struct _cmd));

                  beg++;
                  mh.n_elem++;
                  elements++;
                  memcpy (&cmd[beg], &mi, sizeof (struct _cmd));
               }
               else {
                  memcpy (&cmd[beg], &mi, sizeof (struct _cmd));
                  mh.n_elem++;
                  elements++;
               }
            }

            i = 'A';
            break;

         // Show
         case 'S':
         case 's':
            display_menu (elements);
            break;

         // L List
         case 'L':
         case 'l':
            beg = list_menu_items (beg, elements);
            break;

         // C Copy
         case 'C':
         case 'c':
            if (elements + 1 >= MAX_ITEMS)
               break;

            if (mh.n_elem) {
               for (i = mh.n_elem - 1; i > beg; i--)
                  memcpy (&cmd[i + 1], &cmd[i], sizeof (struct _cmd));

               beg++;
               mh.n_elem++;
               elements++;
               memcpy (&cmd[beg], &cmd[beg - 1], sizeof (struct _cmd));
            }
            i = 'C';
            break;

         // D Delete
         case 'D':
         case 'd':
            if (!elements)
               break;

            wh1 = wopen (10, 25, 14, 54, 0, BLACK|_LGREY, BLACK|_LGREY);
            wactiv (wh1);
            wshadow (DGREY|_BLACK);

            wcenters (1, BLACK|_LGREY, "Are you sure (Y/n) ?  ");

            strcpy (string, "Y");
            winpbeg (BLACK|_LGREY, BLACK|_LGREY);
            winpdef (1, 24, string, "?", 0, 2, NULL, 0);

            i = winpread ();
            wclose ();
            hidecur ();

            if (i == W_ESCPRESS)
               break;

            if (toupper (string[0]) == 'Y') {
               memset (&cmd[beg], 0, sizeof (struct _cmd));
               for (i = beg + 1; i < mh.n_elem; i++)
                  memcpy (&cmd[i - 1], &cmd[i], sizeof (struct _cmd));
               mh.n_elem--;
               elements--;
            }

            i = 'D';
            break;

         // ESC Exit
         case 0x1B:
            i = -1;
            break;
      }

   } while (i != -1);

   if (crc != get_menu_crc (elements)) {
      strcpy (newname, name);

      wh1 = wopen (14, 14, 16, 41, 1, LCYAN|_BLACK, CYAN|_BLACK);
      wactiv (wh1);
      wshadow (DGREY|_BLACK);
      wtitle (" Save ", TRIGHT, YELLOW|_BLUE);
      wprints (0, 1, LGREY|_BLACK, "Menu name: ");
      strcpy (filename, "");
      winpbeg (BLUE|_GREEN, BLUE|_GREEN);
      winpdef (0, 12, filename, "?????????????", 0, 1, NULL, 0);
      wclose ();

      if (winpread () != W_ESCPRESS) {
         strcpy (newname, strupr (strbtrim (filename)));
         if (!newname[0])
            strcpy (newname, name);
      }

      update_message ();

      sprintf (filename, "%s%s.MNU", config.menu_path, file);
      fd = sh_open (filename, SH_DENYRW, O_RDONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
      if (fd == -1)
         return;
      fp = fdopen (fd, "rb");

      sprintf (filename, "%s%s.NEW", config.menu_path, file);
      fd = sh_open (filename, SH_DENYRW, O_WRONLY|O_BINARY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE);
      if (fd == -1)
         return;
      fpd = fdopen (fd, "wb");

      while (fread (&mh, 1, sizeof (struct _menu_header), fp) == sizeof (struct _menu_header)) {
         if (!stricmp (mh.name, name)) {
            fseek (fp, (long)sizeof(struct _cmd) * mh.n_elem, SEEK_CUR);
            mh.n_elem = elements;
            strcpy (mh.name, newname);
            if (mh.n_elem) {
               fwrite (&mh, sizeof (struct _menu_header), 1, fpd);
               fwrite ((char *)cmd, sizeof (struct _cmd) * mh.n_elem, 1, fpd);
            }
         }
         else {
            if (mh.n_elem) {
               fwrite (&mh, sizeof (struct _menu_header), 1, fpd);
               for (i = 0; i < mh.n_elem; i++) {
                  fread (&mi, sizeof (struct _cmd), 1, fp);
                  fwrite (&mi, sizeof (struct _cmd), 1, fpd);
               }
            }
         }
      }

      fclose (fp);
      fclose (fpd);

      sprintf (filename, "%s%s.MNU", config.menu_path, file);
      unlink (filename);
      sprintf (string, "%s%s.NEW", config.menu_path, file);
      rename (string, filename);

      wclose ();
   }

   wclose ();
   gotoxy_ (24, 1);
   clreol_ ();

   free (cmd);
}

static void setup_menu (char *file)
{
   int fd, i, m, beg, wh;
   char filename[80], items[MAX_ITEMS][20], *array[MAX_ITEMS];
   struct _menu_header mh;

   sprintf (filename, "%s%s.MNU", config.menu_path, file);
   fd = sh_open (filename, SH_DENYWR, O_RDONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
   if (fd == -1)
      return;

   i = 0;
   while (read (fd, &mh, sizeof (struct _menu_header)) == sizeof (struct _menu_header)) {
      sprintf (items[i], "%-14.14s", strupr (mh.name));
      i++;
      lseek (fd, (long)sizeof(struct _cmd) * mh.n_elem, SEEK_CUR);
   }

   strcpy (items[i++], "< New menu >  ");

   close (fd);

   for (m = 0; m < i; m++)
      array[m] = items[m];
   array[m] = NULL;

   beg = 0;

   do {
      i = wpickstr (5, 10, 17, 27, 1, LCYAN|_BLACK, CYAN|_BLACK, BLUE|_LGREY, array, beg, NULL);
      if (i != -1) {
         if (array[i + 1] == NULL) {
            wh = wopen (14, 14, 16, 41, 1, LCYAN|_BLACK, CYAN|_BLACK);
            wactiv (wh);
            wshadow (DGREY|_BLACK);
            wtitle (" New menu ", TRIGHT, YELLOW|_BLUE);
            wprints (0, 1, LGREY|_BLACK, "Menu name: ");
            strcpy (filename, "");
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (0, 12, filename, "?????????????", 0, 1, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               wclose ();
               hidecur ();

               memset (&mh, 0, sizeof (struct _menu_header));
               strcpy (mh.name, strupr (strbtrim (filename)));
               if (!mh.name[0])
                  continue;

               sprintf (filename, "%s%s.MNU", config.menu_path, file);
               fd = sh_open (filename, SH_DENYWR, O_WRONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
               lseek (fd, 0L, SEEK_END);
               if (fd == -1)
                  return;
               write (fd, &mh, sizeof (struct _menu_header));
               close (fd);

               strcpy (filename, mh.name);

               edit_menu_items (file, filename);

               strcpy (items[i], filename);
               beg = i++;
               strcpy (items[i++], "< New menu >  ");
               for (m = 0; m < i; m++)
                  array[m] = items[m];
               array[m] = NULL;
            }
            else
               wclose ();
         }
         else {
            strcpy (filename, items[i]);
            edit_menu_items (file, strbtrim (filename));
            beg = i;
         }
      }
   } while (i != -1);
}

void manager_menu ()
{
   int wh, i = 1, beg = 1;
   char string[80], mnu[16][20];

   wh = wopen (7, 20, 15, 67, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Select menu ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();

      for (i = 0; i < 15; i++) {
         if (!config.language[i].basename[0])
            break;
         sprintf (string, "%s.MNU", strupr (config.language[i].basename));
         sprintf (mnu[i], " %-12.12s ", string);
         wmenuitem ( (i / 3) + 1, (i % 3) * 15 + 1, mnu[i], 0, i + 1, 0, NULL, 0, 0);
      }

      wmenuend (beg, M_OMNI, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      if (i == 0) {
         wcenters (3, YELLOW|_BLACK, "No menu defined");
         getxch ();
         wclose ();
         return;
      }

      start_update ();
      i = wmenuget ();
      if (i != -1) {
         setup_menu (config.language[i - 1].basename);
         beg = i;
      }

      hidecur ();
   } while (i != -1);

   wclose ();
}

static unsigned long cr3tab[] = {
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

long get_config_crc (void)
{
   int i;
   char *p;
   long crc = 0xFFFFFFFFL;

   p = (char *)&config;

   for (i = 0; i < sizeof (struct _configuration); i++)
      crc = UpdateCRC (p[i], crc);

   return (crc);
}

long get_menu_crc (int n_elem)
{
   int i, max;
   char *p;
   long crc = 0xFFFFFFFFL;

   p = (char *)cmd;
   max = n_elem * sizeof (struct _cmd);

   for (i = 0; i < max; i++)
      crc = UpdateCRC (p[i], crc);

   return (crc);
}

static void display_menu (int n_elem)
{
   int m, first, c, noret;
   char *p;

   wopen (0, 0, 24, 79, 5, LGREY|_BLACK, LGREY|_BLACK);

   for (m = 0; m < n_elem; m++) {
      if (cmd[m].priv == HIDDEN)
         continue;

      first = 1;
      noret = 1;
      wtextattr (cmd[m].color);

      for (p = cmd[m].name; *p; p++) {
         if (*p == ';' && *(p + 1) == '\0')
            noret = 0;

         else if (*p == '^') {
            if (first)
               wtextattr (cmd[m].first_color);
            else
               wtextattr (cmd[m].color);
            first = first ? 0 : 1;
         }

         else if (*p == 0x16) {
            p++;
            if (*p == 0x01) {
               p++;
               if(*p == 0x10) {
                  p++;
                  c = *p & 0x7F;
               }
               else
                  c = *p;

               if (!c)
                  wtextattr (13);
               else
                  wtextattr (c);
            }

            else if (*p == 0x07)
               wclreol ();

            else if (*p == 0x07)
               wclreol ();
         }

         else if (*p == 0x0C)
            wclear ();

         else if (*p < 0x20 && *p > 0x00) {
            wputc ('^');
            wputc (*p + 0x40);
         }

         else
            wputc (*p);
      }

      if (!noret)
         wputs ("\n");
   }

   wtextattr (BLACK|_LGREY);
   wgotoxy (23, 0);
   wclreol ();
   wgotoxy (24, 0);
   wclreol ();
   wprints (23, 1, BLACK|_LGREY, "Press any key to continue");

   getxch ();

   wclose ();
}

int list_menu_items (int beg, int n_elem)
{
   int wh, i;
   char string[80], *array[60];

   wh = wopen (3, 1, 20, 76, 1, LCYAN|_BLACK, CYAN|_BLACK);
   wactiv (wh);
   wshadow (DGREY|_BLACK);
   wtitle (" Select item ", TRIGHT, YELLOW|_BLUE);

   wprints (0, 1, LGREY|_BLACK, "User display             Key  Menu type            Data");
   whline (1, 0, 76, 0, BLUE|_BLACK);

   gotoxy_ (24, 1);
   clreol_ ();
   prints (24, 1, LGREY|_BLACK, "-Move bar  ENTER-Select");
   prints (24, 1, YELLOW|_BLACK, "");
   prints (24, 14, YELLOW|_BLACK, "ENTER");

   for (i = 0; i < n_elem; i++) {
      sprintf (string, " %-22.22s ³'%c'³%-19.19s ³ %-20.20s ", cmd[i].name, cmd[i].hotkey ? cmd[i].hotkey : ' ', menu_types[cmd[i].flag_type], cmd[i].argument);
      array[i] = (char *)malloc (strlen (string) + 1);
      if (array[i] == NULL)
         break;
      strcpy (array[i], string);
   }
   array[i] = NULL;

   i = wpickstr (6, 2, 19, 75, 5, LGREY|_BLACK, CYAN|_BLACK, BLUE|_LGREY, array, beg, NULL);

   wclose ();

   gotoxy_ (24, 1);
   clreol_ ();
   prints (24, 1, LGREY|_BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add  C-Copy  L-List  D-Delete  S-Show");
   prints (24, 1, YELLOW|_BLACK, "PgUp/PgDn");
   prints (24, 26, YELLOW|_BLACK, "E");
   prints (24, 34, YELLOW|_BLACK, "A");
   prints (24, 41, YELLOW|_BLACK, "C");
   prints (24, 49, YELLOW|_BLACK, "L");
   prints (24, 57, YELLOW|_BLACK, "D");
   prints (24, 67, YELLOW|_BLACK, "S");

   return (i == -1 ? beg : i);
}

void bbs_paging_hours (void)
{
   int m, x, i = 1;
   char string[128], *p;

   wopen (6, 22, 16, 49, 3, LCYAN|_BLACK, CYAN|_BLACK);
   wshadow (DGREY|_BLACK);
   wtitle (" Paging hours ", TRIGHT, YELLOW|_BLUE);

   do {
      stop_update ();
      wclear ();

      wmenubegc ();
      wmenuitem ( 1,  1, " Sunday    ", 0,  1, 0, NULL, 0, 0);
      wmenuitem ( 2,  1, " Monday    ", 0,  2, 0, NULL, 0, 0);
      wmenuitem ( 3,  1, " Tuesday   ", 0,  3, 0, NULL, 0, 0);
      wmenuitem ( 4,  1, " Wednesday ", 0,  4, 0, NULL, 0, 0);
      wmenuitem ( 5,  1, " Thursday  ", 0,  5, 0, NULL, 0, 0);
      wmenuitem ( 6,  1, " Friday    ", 0,  6, 0, NULL, 0, 0);
      wmenuitem ( 7,  1, " Saturday  ", 0,  7, 0, NULL, 0, 0);
      wmenuend (i, M_OMNI|M_SAVE, 0, 0, LGREY|_BLACK, LGREY|_BLACK, LGREY|_BLACK, BLUE|_LGREY);

      for (i = 0; i < 7; i++) {
         m = config.page_start[i];
         x = config.page_end[i];
         sprintf (string, "%02d:%02d %02d:%02d", m / 60, m % 60, x / 60, x % 60);
         wprints (i + 1, 13, CYAN|_BLACK, string);
      }

      hidecur ();

      start_update ();
      i = wmenuget ();

      switch (i) {
         case 1:
         case 2:
         case 3:
         case 4:
         case 5:
         case 6:
         case 7:
            m = config.page_start[i - 1];

            sprintf (string, "%02d:%02d", m / 60, m % 60);
            winpbeg (BLUE|_GREEN, BLUE|_GREEN);
            winpdef (i, 13, string, "?????", 0, 2, NULL, 0);
            if (winpread () != W_ESCPRESS) {
               p = strtok (string, ":");
               if (p == NULL)
                  config.page_start[i - 1] = atoi (string) % 1440;
               else {
                  config.page_start[i - 1] = atoi (p) * 60;
                  if ((p = strtok (NULL, "")) != NULL)
                     config.page_start[i - 1] += atoi (p);
               }

               m = config.page_start[i];
               sprintf (string, "%02d:%02d", m / 60, m % 60);
               wprints (i, 13, CYAN|_BLACK, string);

               m = config.page_end[i - 1];

               sprintf (string, "%02d:%02d", m / 60, m % 60);
               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
               winpdef (i, 19, string, "?????", 0, 2, NULL, 0);
               if (winpread () != W_ESCPRESS) {
                  p = strtok (string, ":");
                  if (p == NULL)
                     config.page_end[i - 1] = atoi (string) % 1440;
                  else {
                     config.page_end[i - 1] = atoi (p) * 60;
                     p = strtok (NULL, "");
                     config.page_end[i - 1] += atoi (p);
                  }
               }
            }
            break;
      }

      hidecur ();
   } while (i != -1);

   wclose ();
}


