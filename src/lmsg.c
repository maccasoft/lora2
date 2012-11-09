
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
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>
#include <dir.h>
#include <alloc.h>
#include <sys\stat.h>

#include "lsetup.h"
#include "quickmsg.h"
#include "msgapi.h"
#include "pipbase.h"
#include "version.h"

struct _qlastread {
	word msgnum[200];
};

struct _glastread {
	word msgnum[500];
};

typedef struct {
	long crc;
	unsigned int num;
	unsigned char board;
} MSGLINK;


char *cfgname = "CONFIG.DAT";
struct _configuration cfg;

extern unsigned int _stklen = 13000;

unsigned long far cr3tab[] = {                /* CRC polynomial 0xedb88320 */
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
   for (i=0; i < strlen(name); i++)
      crc = Z_32UpdateCRC (((unsigned short) name[i]), crc);

	return (crc);
}

void Append_Backslash (char *s)
{
	int i;

	i = strlen(s) - 1;
	if (s[i] != '\\')
		strcat (s, "\\");
}

void Get_Config_Info (void)
{
	int fd;
	char linea[256], *p;

	fd = open (cfgname, O_RDONLY|O_BINARY);
	if (fd == -1) {
		if ((p=getenv ("LORA")) != NULL) {
			strcpy (linea, p);
			Append_Backslash (linea);
			strcat (linea, cfgname);
		}
		fd = open (linea, O_RDONLY|O_BINARY);
		if (fd == -1) {
			printf ("\nCouldn't open DAT file: %s\a\n", strupr (cfgname));
			exit (1);
		}
	}

	read (fd, &cfg, sizeof (struct _configuration));
	close (fd);
}

/* -----------------------------------------------------------------------

	Sezione di gestione della base messaggi Squish.

	----------------------------------------------------------------------- */
void Link_Squish_Areas (void)
{
	int fd;
	unsigned int i, msgnum, m, prev, next, firsttime = 1;
	long crc;
	char subj[80], lwrite;
	struct _sys tsys;
	struct _minf minf;
	MSG *sq_ptr;
	MSGH *msgh;
	XMSG xmsg;
	MSGLINK *ml;

	minf.def_zone = 0;
	if (MsgOpenApi (&minf))
		return;

	clreol ();
	printf(" * Linking Squish messages\n");

	sprintf (subj, "%sSYSMSG.DAT", cfg.sys_path);
	fd = open (subj, O_RDONLY|O_BINARY);
	if (fd == -1){
		printf("! Unable to open SYSMSG.DAT\n");
		exit (1);
	}

	msgnum = 0;

	while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
		if ( !tsys.squish )
			continue;
		sq_ptr = MsgOpenArea (tsys.msg_path, MSGAREA_CRIFNEC, MSGTYPE_SQUISH);
		if (sq_ptr == NULL){
			printf("Unable to open %s area\n",tsys.msg_name);
			continue;
		}

		MsgLock (sq_ptr);

		msgnum = (int)MsgGetNumMsg (sq_ptr);
		ml = (MSGLINK *)malloc (sizeof (MSGLINK) * msgnum);

		if (ml == NULL){
			if (!firsttime) {
				gotoxy (wherex (), wherey () - 1);
				printf ("   \303\n");
			}
			else
				firsttime = 0;
			if(!msgnum){
				clreol ();
				printf("   \300\304 %3d - %-25.25s - Empty\n", tsys.msg_num, tsys.msg_name);
			}
			else
				printf(" Unable to allocate msgmem: %u area: %s\n",msgnum,tsys.msg_name);
			MsgUnlock (sq_ptr);
			MsgCloseArea (sq_ptr);
			continue;
		}

		if (!firsttime) {
			gotoxy (wherex (), wherey () - 1);
			printf ("   \303\n");
		}
		else
			firsttime = 0;

		clreol ();
		printf("   \300\304 %3d - %-25.25s %-25.25s ", tsys.msg_num, tsys.msg_name, tsys.echomail ? tsys.echotag : "");

		for (i = 1; i <= msgnum; i++) {
			msgh = MsgOpenMsg (sq_ptr, MOPEN_RW, (dword)i);
			if (msgh == NULL)
				continue;

			if (MsgReadMsg (msgh, &xmsg, 0L, 0L, NULL, 0, NULL) == -1) {
				MsgCloseMsg (msgh);
				continue;
			}

			strcpy (subj, strupr (xmsg.subj));
			if (!strncmp (subj, "RE: ", 4))
				strcpy (subj, &subj[4]);
			if (!strncmp (subj, "RE:", 3))
				strcpy (subj, &subj[3]);
			ml[i - 1].crc = crc_name (subj);
			ml[i - 1].num = i;

			MsgCloseMsg (msgh);
		}

		for (i = 1; i <= msgnum; i++) {
			msgh = MsgOpenMsg (sq_ptr, MOPEN_RW, (dword)i);
			if (msgh == NULL)
				continue;

			if (MsgReadMsg (msgh, &xmsg, 0L, 0L, NULL, 0, NULL) == -1) {
				MsgCloseMsg (msgh);
				continue;
			}

			prev = next = 0;
			crc = ml[i - 1].crc;

			for (m = 0; m < msgnum; m++) {
				if (ml[m].num == i)
					continue;
				if (ml[m].crc == crc) {
					if (ml[m].num < i)
						prev = ml[m].num;
					else if (ml[m].num > i) {
						next = ml[m].num;
						break;
					}
				}
			}

			lwrite = 0;

			if (prev && xmsg.replyto != MsgMsgnToUid (sq_ptr, (long)prev)) {
				xmsg.replyto = MsgMsgnToUid (sq_ptr, (long)prev);
				lwrite = 1;
			}
			else if (!prev && xmsg.replyto != 0) {
				xmsg.replyto = 0;
				lwrite = 1;
			}

			if (next && xmsg.replies[0] != MsgMsgnToUid (sq_ptr, (long)next)) {
				xmsg.replies[0] = MsgMsgnToUid (sq_ptr, (long)next);
				lwrite = 1;
			}
			else if (!next && xmsg.replies[0] != 0) {
				xmsg.replies[0] = 0;
				lwrite = 1;
			}

			if (lwrite)
				MsgWriteMsg (msgh, 1, &xmsg, NULL, 0, 0, 0, NULL);

			MsgCloseMsg (msgh);
		}

		MsgUnlock (sq_ptr);
		MsgCloseArea (sq_ptr);
		free (ml);

		printf ("\n");
	}

	close (fd);
}

void Purge_Squish_Areas (void)
{
	int fdu, fd, last, i, msgnum, kill=0, nummsg, maxmsgs;
	int yr, mo, dy, month[12], m, days, *lastr, lrpos;
	char filename[80], firsttime = 1, mese[4];
	long day_mess, day_now, tempo, pos;
	struct tm *tim;
	struct _sys tsys;
	struct _minf minf;
	struct _usr usr;
	MSG *sq_ptr;
	MSGH *msgh;
	XMSG xmsg;
	UMSGID waterid;

	char *mon[] = {
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

	day_now = 365 * tim->tm_year;
	for (m = 0; m < tim->tm_mon; m++)
		day_now += month[m];
	day_now += tim->tm_mday;

	minf.def_zone = 0;
	if (MsgOpenApi (&minf))
		return;

	clreol ();
	printf(" * Purging Squish messages\n");

	sprintf (filename, "%sSYSMSG.DAT", cfg.sys_path);
	fd = open (filename, O_RDONLY|O_BINARY);
	if (fd == -1){
		printf("! Unable to open SYSMSG.DAT\n");
		exit (1);
	}

	msgnum = last = 0;

	while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
		if ( !tsys.squish )
			continue;

		sq_ptr = MsgOpenArea (tsys.msg_path, MSGAREA_CRIFNEC, MSGTYPE_SQUISH);
		if (sq_ptr == NULL)
			continue;
		MsgLock (sq_ptr);

		msgnum = (int)MsgGetNumMsg (sq_ptr);
		last = msgnum;

		if (msgnum == 0) {
			MsgUnlock (sq_ptr);
			MsgCloseArea (sq_ptr);
			continue;
		}

		msgnum++;
		if ((lastr = (int *)malloc (msgnum * sizeof (int))) == NULL) {
			MsgUnlock (sq_ptr);
			MsgCloseArea (sq_ptr);
			continue;
		}
		memset (lastr, 0, msgnum * sizeof (int));
		msgnum--;

		if (!firsttime) {
			gotoxy (wherex (), wherey () - 1);
			printf ("   \303\n");
		}
		else
			firsttime = 0;

		clreol ();
		printf("   \300\304 %3d - %-23.23s %-23.23s ", tsys.msg_num, tsys.msg_name, tsys.echomail ? tsys.echotag : "");

		waterid = MsgMsgnToUid (sq_ptr, MsgGetHighWater (sq_ptr));
		msgnum = (int)MsgGetNumMsg (sq_ptr);
		nummsg = 1;
		maxmsgs = last;
		lrpos = 1;

		if ( tsys.max_msgs && tsys.max_msgs < msgnum )
			kill = msgnum - tsys.max_msgs;

		for (i = 1; i <= last; i++) {
			if (kill-- <= 0) {
				kill = 0;

				msgh = MsgOpenMsg (sq_ptr, MOPEN_RW, (dword)i);
				if (msgh == NULL)
					continue;

				if (MsgReadMsg (msgh, &xmsg, 0L, 0L, NULL, 0, NULL) == -1) {
					MsgCloseMsg (msgh);
					continue;
				}

				MsgCloseMsg (msgh);

				sscanf (xmsg.ftsc_date, "%2d %3s %2d", &dy, mese, &yr);
				mese[3] = '\0';
				for (mo = 0; mo < 12; mo++)
					if (!stricmp(mese, mon[mo]))
						break;
				if (mo == 12)
					mo = 0;

				day_mess = 365 * yr;
				for (m = 0; m < mo; m++)
					day_mess += month[m];
				day_mess += dy;

				days = (int)(day_now - day_mess);

				if ( days > tsys.max_age && tsys.max_age != 0 ) {
					MsgKillMsg (sq_ptr, (dword)i);
					i--;
					last--;
				}
				else if ( ((xmsg.attr & MSGREAD) && (xmsg.attr & MSGPRIVATE)) && days > tsys.age_rcvd && tsys.age_rcvd != 0) {
					MsgKillMsg (sq_ptr, (dword)i);
					i--;
					last--;
				}
				else
					lastr[lrpos] = i;
			}
			else {
				MsgKillMsg (sq_ptr, (dword)i);
				i--;
				last--;
			}

			if ((lrpos++ % 5) == 0)
				printf ("%4d / %-4d\b\b\b\b\b\b\b\b\b\b\b", nummsg, maxmsgs);
			nummsg++;
		}

		printf ("%4d / %-4d %4d\b\b\b\b\b\b\n", nummsg - 1, maxmsgs, (nummsg-1)-last);

		if (waterid) {
			waterid = MsgUidToMsgn (sq_ptr, waterid, UID_PREV);
			if (waterid)
				MsgSetHighWater (sq_ptr, waterid);
		}

		MsgUnlock (sq_ptr);
		MsgCloseArea (sq_ptr);

		if (last < maxmsgs) {
			sprintf (filename, "%s.BBS", cfg.user_file);
			fdu = open (filename, O_RDWR | O_BINARY);
			pos = tell (fdu);

			while (read(fdu, (char *) &usr, sizeof(struct _usr)) == sizeof(struct _usr)) {
				for (i = 0; i < MAXLREAD; i++)
					if (usr.lastread[i].area == tsys.msg_num)
						break;

				if (i < MAXLREAD) {
					m = usr.lastread[i].msg_num;
					if (m > msgnum)
						m = msgnum;
					if (m < 0)
						m = 0;
					while (m > 0 && lastr[m] == 0)
						m--;
					usr.lastread[i].msg_num = lastr[m];

					lseek (fdu, pos, 0);
					write (fdu, (char *) &usr, sizeof(struct _usr));
				}
				else {
					for (i = 0; i < MAXDLREAD; i++)
						if (usr.dynlastread[i].area == tsys.msg_num)
							break;

					if (i < MAXDLREAD) {
						m = usr.dynlastread[i].msg_num;
						if (m > msgnum)
							m = msgnum;
						if (m < 0)
							m = 0;
						while (m > 0 && lastr[m] == 0)
							m--;
						usr.dynlastread[i].msg_num = lastr[m];

						lseek (fdu, pos, 0);
						write (fdu, (char *) &usr, sizeof (struct _usr));
					}
				}

				pos = tell (fdu);
			}

			close (fdu);

			if (lastr != NULL)
				free (lastr);

//			printf ("\n");
		}
	}

	close (fd);
}

void Pack_Squish_Areas (void)
{
	int fd, fd2, i, msgnum, ctrlen, m, msgl, waterid;
	char filename[80], destname[80], *kludge, *text, farea = 1;
	struct _sys tsys;
	struct _minf minf;
	MSG *sq_ptr, *sq_dest;
	XMSG xmsg;
	MSGH *sq_msgh, *msgh_dest;
	long textlen, fpos, *sql;

	minf.def_zone = 0;
	if (MsgOpenApi (&minf))
		return;

	clreol ();
	printf(" * Packing Squish messages\n");

	sprintf (filename, "%sSYSMSG.DAT", cfg.sys_path);
	fd = open (filename, O_RDONLY|O_BINARY);
	if (fd == -1)
		exit (1);

	while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
		if ( !tsys.squish )
			continue;

		sq_ptr = MsgOpenArea (tsys.msg_path, MSGAREA_CRIFNEC, MSGTYPE_SQUISH);
		if (sq_ptr == NULL)
			continue;
		MsgLock (sq_ptr);

		if ((msgnum = (int)MsgGetNumMsg (sq_ptr)) > 0) {
			strcpy (filename, tsys.msg_path);
			strcat (filename, ".SQL");

			sql = NULL;

			fd2 = open (filename, O_RDONLY|O_BINARY);
			if (fd2 != -1) {
				sql = (long *)malloc ((unsigned int)filelength (fd2));
				if (sql != NULL) {
					read (fd2, sql, (unsigned int)filelength (fd2));
					msgl = (int)(filelength (fd2) / sizeof (long));
					for (i = 0; i < msgl; i++)
						sql[i] = MsgUidToMsgn (sq_ptr, sql[i], UID_PREV);
				}
				close (fd2);
			}

//			else {
//				printf("! Unable to open %s (orig)\n",filename);
//				MsgUnlock (sq_ptr);
//				MsgCloseArea (sq_ptr);
//				continue;
//			}

			strcpy (filename, tsys.msg_path);
			filename[strlen (filename) - 1] = '!';

			if (!farea) {
				gotoxy (wherex (), wherey () - 1);
				printf("   \303\n");
			}
			else
				farea = 0;

			clreol ();
			printf("   \300\304 %3d - %-25.25s %-25.25s ", tsys.msg_num, tsys.msg_name, tsys.echomail ? tsys.echotag : "");

			sq_dest = MsgOpenArea (filename, MSGAREA_CRIFNEC, MSGTYPE_SQUISH);
			MsgLock (sq_dest);
			waterid = (int)MsgGetHighWater (sq_ptr);
			text = (char *)malloc (16384);

			for (m = 0; m < msgnum; m++) {
				sq_msgh = MsgOpenMsg (sq_ptr, MOPEN_READ, (long )m + 1);
				if (sq_msgh == NULL)
					continue;

				if ((m % 5) == 0)
					printf ("%4d / %-4d\b\b\b\b\b\b\b\b\b\b\b", m, msgnum);

				if (MsgReadMsg (sq_msgh, &xmsg, 0L, 0L, NULL, 0, NULL) == -1) {
					MsgCloseMsg (sq_msgh);
					continue;
				}

				ctrlen = (int )MsgGetCtrlLen (sq_msgh);
				kludge = (char *)malloc (ctrlen + 1);
				textlen = MsgGetTextLen (sq_msgh);
				msgh_dest = MsgOpenMsg (sq_dest, MOPEN_CREATE, 0L);

				MsgReadMsg (sq_msgh, &xmsg, 0L, 0L, NULL, ctrlen, kludge);
				MsgWriteMsg (msgh_dest, 0, &xmsg, NULL, 0L, textlen, (long)ctrlen, kludge);
				fpos = 0L;

				do {
					i = (int)MsgReadMsg (sq_msgh, NULL, fpos, 16384L, text, 0, NULL);
					fpos += (long)i;

					MsgWriteMsg (msgh_dest, 1, NULL, text, (long)i, textlen, 0L, NULL);
				} while (i == 16384);

				MsgCloseMsg (msgh_dest);
				MsgCloseMsg (sq_msgh);
			}

			printf ("%4d / %-4d\b\b\b\b\b\b\b\b\b\b\b\n", m, msgnum);

			if (waterid)
				MsgSetHighWater (sq_dest, waterid);

			free (text);

			if (sql != NULL) {
				strcpy (filename, tsys.msg_path);
				strcat (filename, ".SQL");

				for (i = 0; i < msgl; i++) {
					if (sql[i])
						sql[i] = MsgMsgnToUid (sq_dest, sql[i]);
				}

				fd2 = open (filename, O_WRONLY|O_BINARY);

				if(fd2==-1){
					printf(" ! Unable to open %s (dest)\n",filename);
					MsgUnlock (sq_ptr);
					MsgCloseArea (sq_ptr);
					MsgUnlock (sq_dest);
					MsgCloseArea (sq_dest);
					continue;
				}

				write (fd2, sql, msgl * sizeof (long));
				close (fd2);

				free (sql);
				sql = NULL;
			}

			MsgUnlock (sq_dest);
			MsgCloseArea (sq_dest);
		}

		MsgUnlock (sq_ptr);
		MsgCloseArea (sq_ptr);

		strcpy (filename, tsys.msg_path);
		i = strlen (filename);
		filename[i - 1] = '!';
		strcpy (destname, tsys.msg_path);

		strcpy (&destname[i], ".SQD");
		unlink (destname);
		strcpy (&filename[i], ".SQD");
		rename (filename, destname);

		strcpy (&destname[i], ".SQI");
		unlink (destname);
		strcpy (&filename[i], ".SQI");
		rename (filename, destname);
	}

	close (fd);
}

/* -----------------------------------------------------------------------

	Sezione di gestione della base messaggi Fido *.MSG.

	----------------------------------------------------------------------- */
int Scan_Fido_Area (char *path, int *first_m, int *last_m)
{
	int i, first, last, msgnum;
	char filename[80], *p;
	struct ffblk blk;

	first = 0;
	last = 0;
	msgnum = 0;

	sprintf(filename, "%s*.MSG", path);

	if (!findfirst(filename, &blk, 0))
		do {
			if ((p = strchr(blk.ff_name,'.')) != NULL)
				*p = '\0';

			i = atoi(blk.ff_name);
			if (last < i || !last)
				last = i;
			if (first > i || !first)
				first = i;
			msgnum++;
		} while (!findnext(&blk));

	*first_m = first;
	*last_m = last;

   return (msgnum);
}

#define MSGREAD    0x0004

void Create_Fido_Index (void)
{
   FILE *fp;
   int fd, fdidx, fdmsg, i;
   char filename[80], *p;
   struct ffblk blk;
   struct _sys tsys;
   struct _msg msg;
   struct _mail mail;

   clreol ();
   printf(" * Creating Fido *.MSG index\n");

   sprintf (filename, "%sSYSMSG.DAT", cfg.sys_path);
   fd = open (filename, O_RDONLY|O_BINARY);
   if (fd == -1)
      exit (1);

   sprintf (filename, "%sMSGTOIDX.DAT", cfg.sys_path);
   fdidx = open (filename, O_WRONLY|O_BINARY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE);
   if (fdidx == -1)
      exit (1);
   fp = fdopen (fdidx, "wb");

   memset (&mail, 0, sizeof (struct _mail));

   while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
      if ( tsys.quick_board || tsys.pip_board || tsys.squish || tsys.gold_board)
			continue;

      mail.area = tsys.msg_num;

      sprintf (filename, "%s*.MSG", tsys.msg_path);
      if (!findfirst (filename, &blk, 0))
         do {
            if ((p = strchr (blk.ff_name, '.')) != NULL)
               *p = '\0';

            i = atoi (blk.ff_name);
            mail.msg_num = i;

            sprintf (filename, "%s%d.MSG", tsys.msg_path, i);
            fdmsg = open (filename, O_RDONLY|O_BINARY);
            if (fdmsg == -1)
               continue;
            read (fdmsg, &msg, sizeof (struct _msg));
            close (fdmsg);

            if (!(msg.attr & MSGREAD)) {
               mail.to = crc_name (msg.to);
               fwrite (&mail, sizeof (struct _mail), 1, fp);
            }
         } while (!findnext(&blk));
   }

   fclose (fp);
   close (fd);
}

void Fido_Renum (char *msg_path, int first, int last, int area_n)
{
   unsigned long p;
   int fdm, fd, m, i, x, msg[1000], flag1, flag2, flag, area;
	long pos;
   char filename [60], newname [40], curdir[80];
   struct _msg mesg;
   struct _usr usr;

   getcwd (curdir, 79);

   if (msg_path[1] == ':')
      setdisk (toupper (msg_path[0]) - 'A');
   msg_path[strlen(msg_path)-1] = '\0';
   chdir(msg_path);
   msg_path[strlen(msg_path)] = '\\';

   m = flag = 0;

   for (i = first; i <= last; i++) {
      sprintf(filename, "%d.MSG", i);
      fdm = open(filename, O_RDWR | O_BINARY);
      if (fdm != -1) {
         close(fdm);

         msg[m++] = i;

         if (i != m) {
            sprintf(newname, "%d.MSG", m);
            flag = 1;
            rename(filename, newname);
         }
      }
   }

   if (!flag) {
      setdisk (toupper (curdir[0]) - 'A');
      chdir(curdir);
      return;
	}

   for (i = 1; i <= m; i++) {
      sprintf(filename, "%d.MSG", i);
      fdm = open(filename, O_RDWR | O_BINARY);
      read(fdm, (char *) &mesg, sizeof(struct _msg));
      flag1 = flag2 = 0;
      for (x = 0; x < m; x++) {
         if (!flag1 && mesg.up == msg[x]) {
            mesg.up = x + 1;
            flag1 = 1;
         }
         if (!flag2 && mesg.reply == msg[x]) {
            mesg.reply = x + 1;
            flag2 = 1;
         }
         if (flag1 && flag2)
            break;
      }
      if (!flag1)
         mesg.up = 0;
      if (!flag2)
         mesg.reply = 0;

      lseek(fdm, 0L, 0);
      write(fdm, (char *) &mesg, sizeof(struct _msg));
      close(fdm);
   }

   fd = open ("LASTREAD", O_RDWR | O_BINARY);
   if (fd != -1) {
      while (read(fd, (char *) &i, 2) == 2) {
         p = tell(fd) - 2L;
         flag1 = 0;
         for (x = 0; x < m; x++)
				if (msg[x] > flag1 && msg[x] <= i)
               flag1 = x + 1;
         i = flag1;
         lseek(fd, p, 0);
         write(fd, (char *) &i, 2);
      }
      close(fd);
   }

   close(fd);

   setdisk (toupper (curdir[0]) - 'A');
   chdir (curdir);

   sprintf (filename, "%s.BBS", cfg.user_file);
   fd = open(filename, O_RDWR | O_BINARY);
   pos = tell (fd);

   while (read(fd, (char *) &usr, sizeof(struct _usr)) == sizeof(struct _usr)) {
      for (area=0; area<MAXLREAD; area++)
         if (usr.lastread[area].area == area_n)
            break;

      if (area < MAXLREAD) {
         flag1 = 0;

         for (x = 0; x < m; x++)
            if (msg[x] > flag1 && msg[x] <= usr.lastread[area].msg_num)
               flag1 = x + 1;

         usr.lastread[area].msg_num = flag1;

         lseek(fd, pos, 0);
         write(fd, (char *) &usr, sizeof(struct _usr));
      }
		else {
         for (area=0; area<MAXDLREAD; area++)
            if (usr.dynlastread[area].area == area_n)
               break;

         if (area < MAXDLREAD) {
            flag1 = 0;

            for (x = 0; x < m; x++)
               if (msg[x] > flag1 && msg[x] <= usr.dynlastread[area].msg_num)
                  flag1 = x + 1;

            usr.dynlastread[area].msg_num = flag1;

            lseek(fd, pos, 0);
            write(fd, (char *) &usr, sizeof(struct _usr));
         }
      }

      pos = tell (fd);
   }

   close(fd);
}

void Renum_Fido_Areas (void)
{
   int fd, first, last, msgnum;
   char filename[60], farea = 1;
   struct _sys tsys;

   sprintf (filename, "%sSYSMSG.DAT", cfg.sys_path);
   fd = open(filename, O_RDONLY|O_BINARY);
   if (fd == -1)
      exit (1);

   clreol ();
   printf(" * Renumber Fido *.MSG messages\n");

   first = last = 0;

   while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
      if ( tsys.quick_board || tsys.pip_board || tsys.squish || tsys.gold_board)
         continue;

      msgnum = Scan_Fido_Area (tsys.msg_path, &first, &last);

      if (!farea) {
         gotoxy (wherex (), wherey () - 1);
         printf("   \303\n");
      }
      else
         farea = 0;

      printf("   \300\304 %3d - %-25.25s %-25.25s %d Msg.\n", tsys.msg_num, tsys.msg_name, tsys.echomail ? tsys.echotag : "", msgnum);

      Fido_Renum (tsys.msg_path, first, last, tsys.msg_num);
   }

   close(fd);
}

void Kill_by_Age (int max_age, int age_rcvd, int first, int last)
{
   int fd, yr, mo, dy, i, month[12], m, days;
   char mese[4], filename [40];
   long day_mess, day_now;
   struct tm *tim;
   struct _msg msg;
   long tempo;

   char *mon[] = {
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

   day_now = 365 * tim->tm_year;
   for (m = 0; m < tim->tm_mon; m++)
      day_now += month[m];
   day_now += tim->tm_mday;

   for (i = first; i <= last; i++) {
      sprintf(filename, "%d.MSG", i);
      fd = open(filename, O_RDONLY | O_BINARY);
      if (fd == -1)
         continue;
      read(fd, (char *) &msg, sizeof(struct _msg));
      close(fd);

		sscanf(msg.date, "%2d %3s %2d", &dy, mese, &yr);
      mese[3] = '\0';
      for (mo = 0; mo < 12; mo++)
         if (!stricmp(mese, mon[mo]))
            break;
      if (mo == 12)
         mo = 0;

      day_mess = 365 * yr;
      for (m = 0; m < mo; m++)
         day_mess += month[m];
      day_mess += dy;

      days = (int)(day_now - day_mess);

      if ( days > max_age && max_age != 0 )
			unlink(filename);

		if ( ((msg.attr & MSGREAD) && (msg.attr & MSGPRIVATE)) &&
			  days > age_rcvd && age_rcvd != 0)
			unlink(filename);
	}
}

void Link_Fido_Areas (void)
{
	int fds, fd, first, last, i, m, msgnum, prev, next, lwrite,firsttime=1;
	char filename [80], curdir [80], subj[72];
	long crc;
	struct _sys tsys;
	struct _msg msg;
	MSGLINK *ml;

	getcwd (curdir, 79);

	clreol ();
	printf(" * Linking Fido *.MSG messages\n");

	sprintf (filename, "%sSYSMSG.DAT", cfg.sys_path);
	fds = open(filename, O_RDONLY|O_BINARY);
	if (fds == -1)
		exit (1);

	msgnum = first = last = 0;

	while ( read (fds, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
		if ( tsys.quick_board || tsys.pip_board || tsys.squish || tsys.gold_board)
			continue;

		msgnum = Scan_Fido_Area (tsys.msg_path, &first, &last);
		ml = (MSGLINK *)malloc (sizeof (MSGLINK) * last);
		if (ml == NULL)
			continue;

		if (tsys.msg_path[1] == ':')
			setdisk (toupper (tsys.msg_path[0]) - 'A');
		tsys.msg_path[strlen(tsys.msg_path)-1] = '\0';
		chdir(tsys.msg_path);
		tsys.msg_path[strlen(tsys.msg_path)] = '\\';

		if (first == 1 && tsys.echomail)
			first++;

		if (!firsttime) {
			gotoxy (wherex (), wherey () - 1);
			printf ("   \303\n");
		}
		else
			firsttime = 0;

		clreol ();
		printf("   \300\304 %3d - %-25.25s %-25.25s %d Msg.", tsys.msg_num, tsys.msg_name, tsys.echomail ? tsys.echotag : "", msgnum);


		for (i = first; i <= last; i++) {
			sprintf (filename, "%d.MSG", i);
			fd = open (filename, O_RDONLY|O_BINARY);
			if (fd == -1)
				continue;
			read (fd, &msg, sizeof (struct _msg));
			close (fd);

			strcpy (subj, strupr (msg.subj));
			if (!strncmp (subj, "RE: ", 4))
				strcpy (subj, &subj[4]);
			if (!strncmp (subj, "RE:", 3))
				strcpy (subj, &subj[3]);
			ml[i - 1].crc = crc_name (subj);
			ml[i - 1].num = i;
		}

		for (i = first; i <= last; i++) {
			sprintf (filename, "%d.MSG", i);
			fd = open (filename, O_RDWR|O_BINARY);
			if (fd == -1)
				continue;
			read (fd, &msg, sizeof (struct _msg));

			prev = next = 0;
			crc = ml[i - 1].crc;

			for (m = first - 1; m < last; m++) {
				if (ml[m].num == i)
					continue;
				if (ml[m].crc == crc) {
					if (ml[m].num < i)
						prev = ml[m].num;
					else if (ml[m].num > i) {
						next = ml[m].num;
						break;
					}
				}
			}

			lwrite = 0;

			if (next && msg.up != next) {
				msg.up = next;
				lwrite = 1;
			}
			else if (!next && msg.up != 0) {
				msg.up = 0;
				lwrite = 1;
			}

			if (prev && msg.reply != prev) {
				msg.reply = prev;
				lwrite = 1;
			}
			else if (!prev && msg.reply != 0) {
				msg.reply = 0;
				lwrite = 1;
			}

			if (lwrite) {
				lseek (fd, 0L, SEEK_SET);
				write (fd, &msg, sizeof (struct _msg));
			}

			close (fd);
		}
		printf ("\n");
	}

	close (fds);

	setdisk (toupper (curdir[0]) - 'A');
	chdir (curdir);
}

void Purge_Fido_Areas (int renum)
{
	int fd, first, last, i, msgnum, kill,firsttime=1;
	char filename [80], curdir [80];
	struct _sys tsys;

	getcwd (curdir, 79);

   clreol ();
   printf(" * Purging Fido *.MSG messages\n");

   sprintf (filename, "%sSYSMSG.DAT", cfg.sys_path);
   fd = open(filename, O_RDONLY|O_BINARY);
   if (fd == -1)
      exit (1);

   msgnum = first = last = 0;

   while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
      if ( tsys.quick_board || tsys.pip_board || tsys.squish || tsys.gold_board)
         continue;

      msgnum = Scan_Fido_Area (tsys.msg_path, &first, &last);

      if (first == 1 && tsys.echomail)
         first++;

      if (tsys.msg_path[1] == ':')
         setdisk (toupper (tsys.msg_path[0]) - 'A');
      tsys.msg_path[strlen(tsys.msg_path)-1] = '\0';
      chdir(tsys.msg_path);
		tsys.msg_path[strlen(tsys.msg_path)] = '\\';

		if (!firsttime) {
			gotoxy (wherex (), wherey () - 1);
			printf ("   \303\n");
		}
		else
			firsttime = 0;

		clreol ();
		printf("   \300\304 %3d - %-25.25s %-25.25s %d Msg.", tsys.msg_num, tsys.msg_name, tsys.echomail ? tsys.echotag : "", msgnum);

		if ( tsys.max_age || tsys.age_rcvd )
			Kill_by_Age (tsys.max_age, tsys.age_rcvd, first, last);

		setdisk (toupper (curdir[0]) - 'A');
		chdir (curdir);

		msgnum = Scan_Fido_Area (tsys.msg_path, &first, &last);

		if (tsys.msg_path[1] == ':')
			setdisk (toupper (tsys.msg_path[0]) - 'A');
		tsys.msg_path[strlen(tsys.msg_path)-1] = '\0';
		chdir(tsys.msg_path);
		tsys.msg_path[strlen(tsys.msg_path)] = '\\';

		if ( tsys.max_msgs && tsys.max_msgs < msgnum ) {
			kill = msgnum - tsys.max_msgs;

			for (i = first; i <= last; i++) {
				sprintf (filename, "%d.MSG", i);
				unlink (filename);

				if (--kill <= 0)
					break;
			}
		}

		if (renum) {
			setdisk (toupper (curdir[0]) - 'A');
			chdir (curdir);
			Fido_Renum (tsys.msg_path, first, last, tsys.msg_num);
		}
		printf("\n");
	}

	close (fd);

	setdisk (toupper (curdir[0]) - 'A');
	chdir (curdir);
}

/* -----------------------------------------------------------------------

	Sezione di gestione della base messaggi Hudson (QuickBBS).

	----------------------------------------------------------------------- */
void Create_Quick_Index (int xlink, int unknow, int renum)
{
	int fd, fdidx, fdto, m, flag;
	short know[200];
	char filename[60];
	word msgnum;
	long pos;
	struct _sys tsys;
	struct _msghdr msghdr;
	struct _msgidx msgidx;
	struct _msginfo msginfo;

	msgnum = 1;
//	if (xlink);

	if (unknow) {
		for (m = 0; m < 200; m++)
			know[m] = 0;

		sprintf (filename, "%sSYSMSG.DAT", cfg.sys_path);
		fd = open(filename, O_RDONLY|O_BINARY);
		if (fd == -1)
			exit (1);

		while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
			if ( !tsys.quick_board )
				continue;

			know[tsys.quick_board-1] = 1;
		}

		close (fd);
	}

	sprintf (filename, "%sMSGHDR.BBS", cfg.quick_msgpath);
	fd = open (filename, O_RDWR|O_BINARY);
	if (fd == -1)
		exit (1);

	printf(" * Regenerating message base indices\n");

	pos = tell (fd);

	memset((char *)&msginfo, 0, sizeof(struct _msginfo));

	sprintf (filename, "%sMSGIDX.BBS", cfg.quick_msgpath);
	fdidx = open(filename,O_WRONLY|O_BINARY|O_TRUNC);

	sprintf (filename, "%sMSGTOIDX.BBS", cfg.quick_msgpath);
	fdto = open(filename,O_WRONLY|O_BINARY|O_TRUNC);

	while (read(fd, (char *)&msghdr, sizeof (struct _msghdr)) == sizeof (struct _msghdr)) {
		flag = 0;

		if (unknow) {
			if (know[msghdr.board-1] != 1) {
				msghdr.msgattr |= Q_RECKILL;
				if (know[msghdr.board-1] != 2) {
					printf(" * Unknow board #%d\n", msghdr.board);
					know[msghdr.board-1] = 2;
				}
				flag = 1;
			}
		}

		if (renum)
			msghdr.msgnum = msgnum++;
		else
			msgnum++;

		if (renum || flag) {
			lseek (fd, pos, SEEK_SET);
			write (fd, (char *)&msghdr, sizeof (struct _msghdr));
			pos = tell (fd);
		}

		if (flag) {
			msgidx.msgnum = 0;
			write (fdto, "\x0B* Deleted *                        ", 36);
		}
		else {
			write (fdto, (char *)&msghdr.whoto, sizeof (struct _msgtoidx));
			msgidx.msgnum = msghdr.msgnum;
		}

		msgidx.board = msghdr.board;
		write (fdidx, (char *)&msgidx, sizeof (struct _msgidx));

		msginfo.totalmsgs++;

		if (!msginfo.lowmsg)
			msginfo.lowmsg = msghdr.msgnum;

		if (msginfo.lowmsg > msghdr.msgnum)
			msginfo.lowmsg = msghdr.msgnum;
		if (msginfo.highmsg < msghdr.msgnum)
			msginfo.highmsg = msghdr.msgnum;

		if (!flag)
			msginfo.totalonboard[msgidx.board-1]++;
	}

	close (fdto);
	close (fdidx);
	close (fd);

	sprintf (filename, "%sMSGINFO.BBS", cfg.quick_msgpath);
	fd = open(filename,O_WRONLY|O_BINARY);
	write(fd, (char *)&msginfo, sizeof(struct _msginfo));
	close (fd);
}

void Renum_Quick_Areas (void)
{
	int fd, fdidx;
	char filename[60];
	word msgnum, *lastr, *newnum, lrpos, i, m;
	long pos;
	struct _msghdr msghdr;
	struct _msgidx msgidx;
	struct _msginfo msginfo;
	struct _qlastread lr;

	printf(" * Renumber QuickBBS messages\n");

	sprintf (filename, "%sMSGINFO.BBS", cfg.quick_msgpath);
	fd = open (filename,O_RDONLY|O_BINARY);
	read (fd, (char *)&msginfo, sizeof(struct _msginfo));
	close (fd);

	if(!msginfo.totalmsgs){
		printf(" * No Messages in Hudson base\n");
		return;
	}


	lastr = (word *)malloc (msginfo.totalmsgs * sizeof (word));
	if (lastr == NULL) {
		printf ("MEM Err: LASTREAD allocation (lastr).\n");
		exit (1);
	}
	memset (lastr, 0, msginfo.totalmsgs * sizeof (word));

	newnum = (word *)malloc (msginfo.totalmsgs * sizeof (word));
	if (newnum == NULL) {
		printf ("MEM Err: LASTREAD allocation (newnum).\n");
		exit (1);
	}
	memset (newnum, 0, msginfo.totalmsgs * sizeof (word));

	lrpos = 0;
	msgnum = 1;

	sprintf (filename, "%sMSGHDR.BBS", cfg.quick_msgpath);
	fd = open (filename, O_RDWR | O_BINARY);
	if (fd == -1)
		exit (1);

	pos = tell (fd);

	memset((char *)&msginfo, 0, sizeof(struct _msginfo));

	sprintf (filename, "%sMSGIDX.BBS", cfg.quick_msgpath);
	fdidx = open(filename,O_WRONLY|O_BINARY|O_TRUNC);

	while (read(fd, (char *)&msghdr, sizeof (struct _msghdr)) == sizeof (struct _msghdr)) {
		if (!(msghdr.msgattr & Q_RECKILL) && msghdr.msgnum && msghdr.board) {
			lastr[lrpos] = msghdr.msgnum;
			msghdr.msgnum = msgnum++;
			newnum[lrpos++] = msghdr.msgnum;
			lseek (fd, pos, SEEK_SET);
			write (fd, (char *)&msghdr, sizeof (struct _msghdr));
      }
      pos = tell (fd);

      if (!(msghdr.msgattr & Q_RECKILL) && msghdr.msgnum && msghdr.board)
         msgidx.msgnum = msghdr.msgnum;
      else
         msgidx.msgnum = 0;
      msgidx.board = msghdr.board;

      write (fdidx, (char *)&msgidx, sizeof (struct _msgidx));

      if (!(msghdr.msgattr & Q_RECKILL) && msghdr.msgnum && msghdr.board) {
         if (!msginfo.lowmsg)
            msginfo.lowmsg = msghdr.msgnum;

         if (msginfo.lowmsg > msghdr.msgnum)
            msginfo.lowmsg = msghdr.msgnum;
         if (msginfo.highmsg < msghdr.msgnum)
            msginfo.highmsg = msghdr.msgnum;

         msginfo.totalmsgs++;
         msginfo.totalonboard[msgidx.board-1]++;
      }
   }

   close (fdidx);
   close (fd);

   sprintf (filename, "%sMSGINFO.BBS", cfg.quick_msgpath);
   fd = open(filename,O_WRONLY|O_BINARY);
   write(fd, (char *)&msginfo, sizeof(struct _msginfo));
   close (fd);

   printf(" * Updating LASTREAD.BBS pointer\n");

   sprintf (filename, "%sLASTREAD.BBS", cfg.quick_msgpath);
   fd = open (filename,O_RDWR|O_BINARY);

   pos = 0L;
   while (read (fd, &lr, sizeof (struct _qlastread)) == sizeof (struct _qlastread)) {
      for (i = 0; i < 200; i++) {
         m = 0;
         while (lastr[m] < lr.msgnum[i] && m < lrpos)
            m++;
         while (m > 0 && lastr[m] == 0)
            m--;
         lr.msgnum[i] = newnum[m];
      }

      lseek (fd, pos, SEEK_SET);
      write (fd, &lr, sizeof (struct _qlastread));
      pos = tell (fd);
   }

   close (fd);
}

char *pascal_string (char *s)
{
   static char e_input[90];

   strncpy(e_input,(char *)&s[1],(int)s[0]);
   e_input[(int)s[0]] = '\0';

   return (e_input);
}

void Link_Quick_Areas (void)
{
	int fd;
	unsigned int i, m, nummsg, cur, prev, next;
	char subj[80], lwrite;
	long crc;
	struct _msghdr msghdr;
	MSGLINK *ml;

	sprintf (subj, "%sMSGHDR.BBS", cfg.quick_msgpath);
	fd = open (subj, O_RDWR|O_BINARY);
	if (fd == -1)
		return;

	clreol ();
	printf(" * Linking QuickBBS messages\n");

	nummsg = (unsigned int)(filelength (fd) / sizeof (struct _msghdr));
	ml = (MSGLINK *)malloc (nummsg * sizeof (MSGLINK));
	memset (ml, 0, nummsg * sizeof (MSGLINK));
	cur = 1;

	while (read(fd, (char *)&msghdr, sizeof (struct _msghdr)) == sizeof (struct _msghdr)) {
		strcpy (subj, strupr (pascal_string (msghdr.subject)));
		if (!strncmp (subj, "RE: ", 4))
			strcpy (subj, &subj[4]);
		if (!strncmp (subj, "RE:", 3))
			strcpy (subj, &subj[3]);
		ml[cur - 1].crc = crc_name (subj);
		ml[cur - 1].num = cur;
		ml[cur - 1].board = msghdr.board;
		cur++;
	}

	lseek (fd, 0L, SEEK_SET);

	i = 0;

	while (read(fd, (char *)&msghdr, sizeof (struct _msghdr)) == sizeof (struct _msghdr)) {
		crc = ml[i].crc;
		i++;

		prev = next = 0;

		for (m = 0; m < nummsg; m++) {
			if (ml[m].num == i)
				continue;
			if (ml[m].crc == crc && ml[m].board == msghdr.board) {
				if (ml[m].num < i)
					prev = ml[m].num;
				else if (ml[m].num > i) {
					next = ml[m].num;
					break;
				}
			}
		}

		lwrite = 0;

		if (msghdr.prevreply != prev) {
			msghdr.prevreply = prev;
			lwrite = 1;
		}
		if (msghdr.nextreply != next) {
			msghdr.nextreply = next;
			lwrite = 1;
		}

		if (lwrite) {
			lseek (fd, -1L * sizeof (struct _msghdr), SEEK_CUR);
			write (fd, (char *)&msghdr, sizeof (struct _msghdr));
		}
	}

	close (fd);
	free (ml);
}

void Purge_Quick_Areas (int renum)
{
	#define MAXREADIDX   100
	int fd, fdidx, max_msgs[200], max_age[200], age_rcvd[200], month[12];
	int cnum[200], pnum[200], area, flag1, x, m, i;
	int far *qren[200];
	char filename[60];
   int fdto, dy, mo, yr, days, flag;
   word msgnum, deleted = 0, *lastr, lrpos;
   long pos, tempo, day_now, day_mess;
   struct tm *tim;
   struct _msghdr msghdr;
   struct _msgidx msgidx;
   struct _msgidx idx[MAXREADIDX];
   struct _msginfo msginfo;
	struct _qlastread lr;
   struct _sys tsys;
   struct _usr usr;

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

   for (m = 0; m < 200; m++) {
		max_msgs[m] = 0;
      max_age[m] = 0;
		age_rcvd[m] = 0;
		cnum[m] = 0;
		pnum[m] = 0;
   }

   msgnum = 0;
   flag = 0;

   sprintf (filename, "%sSYSMSG.DAT", cfg.sys_path);
   fd = open(filename, O_RDONLY|O_BINARY);
   if (fd == -1)
      exit (1);

   while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
      if ( tsys.quick_board < 1 || tsys.quick_board > 200)
			continue;

      max_msgs[tsys.quick_board-1] = tsys.max_msgs;
      max_age[tsys.quick_board-1] = tsys.max_age;
		age_rcvd[tsys.quick_board-1] = tsys.age_rcvd;

      if (tsys.max_msgs || tsys.max_age || tsys.age_rcvd)
			flag = 1;
   }

   close (fd);

	if (!flag)
      return;

   printf(" * Reading message base data\n");

   memset (&msginfo, 0, sizeof (struct _msginfo));

   sprintf(filename, "%sMSGIDX.BBS", cfg.quick_msgpath);
	fd = open (filename, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);

   do {
      m = read (fd, (char *)&idx, sizeof(struct _msgidx) * MAXREADIDX);
		m /= sizeof (struct _msgidx);

      for (x = 0; x < m; x++) {
			if (idx[x].board && idx[x].msgnum > 0) {
				if (!msginfo.lowmsg)
					msginfo.lowmsg = idx[x].msgnum;

				if (msginfo.lowmsg > idx[x].msgnum)
					msginfo.lowmsg = idx[x].msgnum;
				if (msginfo.highmsg < idx[x].msgnum)
					msginfo.highmsg = idx[x].msgnum;

				msginfo.totalmsgs++;
				msginfo.totalonboard[idx[x].board - 1]++;
			}
		}
	} while (m == MAXREADIDX);

	close(fd);

	if(!msginfo.totalmsgs){
		printf(" * No Messages in Hudson base\n");
		return;
	}


	printf(" * Purging QuickBBS messages\n");

	lastr = (word *)malloc (msginfo.totalmsgs * sizeof (word));
	if (lastr == NULL) {
		printf ("MEM Err: LASTREAD allocation.\n");
		exit (1);
	}
	memset (lastr, 0, msginfo.totalmsgs * sizeof (word));
	lrpos = 0;

	for (m = 0; m < 200; m++) {
		if (msginfo.totalonboard[m]) {
#ifdef __OS2__
			qren[m] = (int *)malloc (msginfo.totalonboard[m] * (long)sizeof (int));
#else
			qren[m] = (int far *)farmalloc (msginfo.totalonboard[m] * (long)sizeof (int));
#endif
			if (qren[m] == NULL)
				exit (2);
		}
		else
			qren[m] = NULL;
	}

	tempo = time(0);
	tim = localtime(&tempo);

	day_now = 365 * tim->tm_year;
	for (m = 0; m < tim->tm_mon; m++)
		day_now += month[m];
	day_now += tim->tm_mday;

	sprintf (filename, "%sMSGHDR.BBS", cfg.quick_msgpath);
	fd = open (filename, O_RDWR|O_BINARY);
	if (fd == -1)
		exit (1);

	pos = tell (fd);

	sprintf (filename, "%sMSGIDX.BBS", cfg.quick_msgpath);
	fdidx = open(filename,O_RDWR|O_BINARY);

	sprintf (filename, "%sMSGTOIDX.BBS", cfg.quick_msgpath);
	fdto = open(filename,O_RDWR|O_BINARY);

	while (read(fd, (char *)&msghdr, sizeof (struct _msghdr)) == sizeof (struct _msghdr)) {
		flag = 0;
		if (msghdr.board >= 1 && msghdr.board <= 200 && !(msghdr.msgattr & Q_RECKILL)) {
			pnum[msghdr.board-1]++;

			if (max_msgs[msghdr.board-1] && msginfo.totalonboard[msghdr.board-1] > max_msgs[msghdr.board-1]) {
				msghdr.msgattr |= Q_RECKILL;
				flag = 1;
			}
			else {
				sscanf(&msghdr.date[1], "%02d-%02d-%02d", &mo, &dy, &yr);

				day_mess = 365 * yr;
				mo--;
				for (m = 0; m < mo; m++)
					day_mess += month[m];
				day_mess += dy;

				days = (int)(day_now - day_mess);

				if (max_age[msghdr.board-1] && days > max_age[msghdr.board-1]) {
					msghdr.msgattr |= Q_RECKILL;
					flag = 1;
				}
				else if (age_rcvd[msghdr.board-1] &&
				 ( (msghdr.msgattr & Q_PRIVATE) && (msghdr.msgattr & Q_RECEIVED) ) &&
				 days > age_rcvd[msghdr.board-1]) {
					msghdr.msgattr |= Q_RECKILL;
					flag = 1;
				}
			}

			if ( flag ) {
				deleted++;
				lseek (fd, pos, SEEK_SET);
				write (fd, (char *)&msghdr, sizeof (struct _msghdr));
				msginfo.totalmsgs--;
				msginfo.totalonboard[msghdr.board-1]--;
				lseek (fdidx, (long)msgnum * sizeof (struct _msgidx), SEEK_SET);
				read (fdidx, (char *)&msgidx, sizeof (struct _msgidx));
				msgidx.msgnum = -1;
				lseek (fdidx, (long)msgnum * sizeof (struct _msgidx), SEEK_SET);
				write (fdidx, (char *)&msgidx, sizeof (struct _msgidx));

				lseek (fdto, (long)msgnum * 36, SEEK_SET);
				write (fdto, "\x0B* Deleted *                        ", 36);

				lrpos++;
			}
			else {
				qren[msghdr.board-1][cnum[msghdr.board-1]++] = pnum[msghdr.board-1];
				lastr[lrpos++] = msghdr.msgnum;
			}

			if (renum) {
				msgidx.msgnum = msghdr.msgnum;
				msgidx.board = msghdr.board;

				write (fdidx, (char *)&msgidx, sizeof (struct _msgidx));
			}

//      sprintf (filename, "%d", msghdr.msgnum);
//      printf ("%s%*s", filename, strlen (filename), "\b\b\b\b\b\b\b");


			if ((msgnum % 40) == 0 || flag)
				printf("   \300\304 Msg.: %u, Deleted: %u\r", msgnum, deleted);

			msgnum++;
		}
		pos = tell (fd);
		if (msghdr.board < 1 && msghdr.board > 200){
				printf("Unknown board number: %d",msghdr.board);
				printf("Run index utility");
		}
	}

	close (fdidx);
	close (fd);
	close (fdto);

	sprintf (filename, "%sMSGINFO.BBS", cfg.quick_msgpath);
	fd = open(filename,O_RDWR|O_BINARY);
	if (fd == -1)
		exit (1);
	write (fd, (char *)&msginfo, sizeof(struct _msginfo));
	close (fd);

	for (m = 0; m < 200; m++)
		cnum[m] = 0;

	if (deleted) {
		printf("\n * Updating USERS lastread pointer\n");

		sprintf (filename, "%sLASTREAD.BBS", cfg.quick_msgpath);
		fd = open (filename,O_RDWR|O_BINARY);

		pos = 0L;
		while (read (fd, &lr, sizeof (struct _qlastread)) == sizeof (struct _qlastread)) {
			for (i = 0; i < 200; i++) {
				m = 0;
				while (lastr[m] < lr.msgnum[i] && m < msginfo.totalmsgs)
					m++;
				while (m > 0 && lastr[m] == 0)
					m--;
				lr.msgnum[i] = lastr[m];
			}

			lseek (fd, pos, SEEK_SET);
			write (fd, &lr, sizeof (struct _qlastread));
			pos = tell (fd);
		}

		close (fd);

		sprintf (filename, "%sSYSMSG.DAT", cfg.sys_path);
		fd = open(filename, O_RDONLY|O_BINARY);
		if (fd == -1)
			exit (1);

		while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
			if ( tsys.quick_board < 1 || tsys.quick_board > 200)
				continue;
			cnum[tsys.quick_board-1] = tsys.msg_num;
		}

		close (fd);

		sprintf (filename, "%s.BBS", cfg.user_file);
		fd = open(filename, O_RDWR | O_BINARY);
		pos = tell (fd);

		while (read(fd, (char *) &usr, sizeof(struct _usr)) == sizeof(struct _usr)) {
			for (area = 0; area < MAXLREAD; area++) {
				for (m = 0; m < 200; m++) {
					if (usr.lastread[area].area == cnum[m])
						break;
				}

				if (m < 200 && usr.lastread[area].msg_num) {
					flag1 = 0;

					for (x = 0; x < msginfo.totalonboard[m]; x++)
						if (qren[m][x] > flag1 && qren[m][x] <= usr.lastread[area].msg_num)
							flag1 = x + 1;

					usr.lastread[area].msg_num = flag1;
				}
			}

			for (area = 0; area < MAXDLREAD; area++) {
				for (m = 0; m < 200; m++) {
					if (usr.dynlastread[area].area == cnum[m])
						break;
				}

				if (m < 200 && usr.dynlastread[area].msg_num) {
					flag1 = 0;

					for (x = 0; x < msginfo.totalonboard[m]; x++)
						if (qren[m][x] > flag1 && qren[m][x] <= usr.dynlastread[area].msg_num)
							flag1 = x + 1;

					usr.dynlastread[area].msg_num = flag1;
				}
			}

			lseek (fd, pos, SEEK_SET);
			write (fd, (char *) &usr, sizeof(struct _usr));

			pos = tell (fd);
		}

		close(fd);
	}

	for (m = 0; m < 200; m ++) {
		if (qren[m] != NULL)
#ifdef __OS2__
			free (qren[m]);
#else
			farfree (qren[m]);
#endif
	}
}

void Pack_Quick_Base (int renum)
{
	FILE *fph, *fpt, *fpi, *fpto, *outfph, *outfpt;
	int i;
	char filename[80], newname[80];
	unsigned int msgnum;
	unsigned char text[256];
	long block, messages, deleted;
	struct _msghdr msghdr;
	struct _msgidx msgidx;
	struct _msginfo msginfo;

	printf (" * Packing QuickBBS Message Base\r\n");

	memset (&msginfo, 0, sizeof (struct _msginfo));

	sprintf (filename, "%sMSGHDR.BBS", cfg.quick_msgpath);
	if ((fph = fopen (filename, "rb")) == NULL){
	printf (" ! Unable to open MSGHDR.BBS\n");
		return;
	}

	block = 0;
	msgnum = 1;
	deleted = 0;
	messages = filelength (fileno (fph)) / sizeof (struct _msghdr);

	while (fread (&msghdr, sizeof (struct _msghdr), 1, fph) == 1) {
		if (msghdr.msgattr & Q_RECKILL) {
			deleted++;
			continue;
		}
		if (msghdr.msgnum < 1 || msghdr.board < 1 || msghdr.board > 200) {
			deleted++;
			continue;
		}
	}

	if (deleted == 0) {
		fclose (fph);
		printf (" * No Messages Deleted\r\n");
		return;
	}

	messages -= deleted;

	sprintf (filename, "%sMSGTXT.BBS", cfg.quick_msgpath);
	if ((fpt = fopen (filename, "rb")) == NULL){
		printf("! Unable to open MSGTXT.BBS\n");
		exit (2);
		}
	sprintf (filename, "%sMSGIDX.BBS", cfg.quick_msgpath);
	if ((fpi = fopen (filename, "wb")) == NULL){
	printf("! Unable to open MSGIDX.BBS\n");
		exit (2);
	}
	sprintf (filename, "%sMSGTOIDX.BBS", cfg.quick_msgpath);
	if ((fpto = fopen (filename, "wb")) == NULL){
		printf("! Unable to open MSGTOIDX.BBS\n");
		exit (2);
		}
	sprintf (filename, "%sMSGHDR.NEW", cfg.quick_msgpath);
	if ((outfph = fopen (filename, "wb")) == NULL){
		printf("! Unable to open tempfile MSGHDR.NEW\n");
		exit (2);
	}
	sprintf (filename, "%sMSGTXT.NEW", cfg.quick_msgpath);
	if ((outfpt = fopen (filename, "wb")) == NULL){
		printf("! Unable to open tempfile MSGTXT.NEW\n");
		exit (2);
	}

	fseek (fph, 0L, SEEK_SET);

	while (fread (&msghdr, sizeof (struct _msghdr), 1, fph) == 1) {
		if (msghdr.msgattr & Q_RECKILL)
			continue;
		if (msghdr.msgnum < 1 || msghdr.board < 1 || msghdr.board > 200)
			continue;

		printf ("   Packed Msg.# %d / %ld\r", msgnum, messages);

		fseek (fpt, (long)msghdr.startblock * 256L, SEEK_SET);

		msghdr.startblock = (unsigned int)block;
		if (renum)
			msghdr.msgnum = msgnum;
		fwrite (&msghdr, sizeof (struct _msghdr), 1, outfph);

		for (i = 0; i < msghdr.numblocks; i++) {
			fread (text, 256, 1, fpt);
			fwrite (text, 256, 1, outfpt);
		}

		msgidx.msgnum = msghdr.msgnum;
		msgidx.board = msghdr.board;
		fwrite (&msgidx, sizeof (struct _msgidx), 1, fpi);

		fwrite (&msghdr.whoto, 36, 1, fpto);

		if (!msginfo.lowmsg || msghdr.msgnum < msginfo.lowmsg)
			msginfo.lowmsg = msghdr.msgnum;
		if (!msginfo.highmsg || msghdr.msgnum > msginfo.highmsg)
			msginfo.highmsg = msghdr.msgnum;
		msginfo.totalonboard[msghdr.board - 1]++;
		msginfo.totalmsgs++;

		block += msghdr.numblocks;
		msgnum++;
	}

	fclose (outfpt);
	fclose (outfph);
	fclose (fpto);
	fclose (fpi);
	fclose (fpt);
	fclose (fph);

	sprintf (filename, "%sMSGINFO.BBS", cfg.quick_msgpath);
	if ((fpto = fopen (filename, "wb")) == NULL){
		printf("! Unable to open MSGINFO.BBS\n");
		exit (2);
	}
	fwrite (&msginfo, sizeof (struct _msginfo), 1, fpto);
	fclose (fpto);

	sprintf (filename, "%sMSGHDR.NEW", cfg.quick_msgpath);
	sprintf (newname, "%sMSGHDR.BBS", cfg.quick_msgpath);
	unlink (newname);
	rename (filename, newname);

	sprintf (filename, "%sMSGTXT.NEW", cfg.quick_msgpath);
	sprintf (newname, "%sMSGTXT.BBS", cfg.quick_msgpath);
	unlink (newname);
	rename (filename, newname);

	clreol ();
}


/* -----------------------------------------------------------------------

	Sezione di gestione della base messaggi GOLD.

	----------------------------------------------------------------------- */
void Create_Gold_Index (int xlink, int unknow, int renum)
{
	int fd, fdidx, fdto, know[500], m, flag;
	char filename[60];
	long pos, msgnum;
	struct _sys tsys;
	struct _gold_msghdr msghdr;
	struct _gold_msgidx msgidx;
	struct _gold_msginfo msginfo;

	msgnum = 1;
//	if (xlink);

	if (unknow) {
		for (m = 0; m < 500; m++)
			know[m] = 0;

		sprintf (filename, "%sSYSMSG.DAT", cfg.sys_path);
		fd = open(filename, O_RDONLY|O_BINARY);
		if (fd == -1)
			exit (1);

		while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
			if ( !tsys.gold_board )
				continue;

			know[tsys.gold_board-1] = 1;
		}

		close (fd);
	}

	sprintf (filename, "%sMSGHDR.DAT", cfg.quick_msgpath);
	fd = open (filename, O_RDWR|O_BINARY);
	if (fd == -1)
		exit (1);

	printf(" * Regenerating Gold message base indices\n");

	pos = tell (fd);

	memset((char *)&msginfo, 0, sizeof(struct _gold_msginfo));

	sprintf (filename, "%sMSGIDX.DAT", cfg.quick_msgpath);
	fdidx = open(filename,O_WRONLY|O_BINARY|O_TRUNC);

	sprintf (filename, "%sMSGTOIDX.DAT", cfg.quick_msgpath);
	fdto = open(filename,O_WRONLY|O_BINARY|O_TRUNC);

	while (read(fd, (char *)&msghdr, sizeof (struct _gold_msghdr)) == sizeof (struct _gold_msghdr)) {
		flag = 0;

		if (unknow) {
			if (know[msghdr.board-1] != 1) {
				msghdr.msgattr |= Q_RECKILL;
				if (know[msghdr.board-1] != 2) {
					printf(" * Unknow board #%d\n", msghdr.board);
					know[msghdr.board-1] = 2;
				}
				flag = 1;
			}
		}

		if (renum)
			msghdr.msgnum = msgnum++;
		else
			msgnum++;

		if (renum || flag) {
			lseek (fd, pos, SEEK_SET);
			write (fd, (char *)&msghdr, sizeof (struct _gold_msghdr));
			pos = tell (fd);
		}

		if (flag) {
			msgidx.msgnum = 0;
			write (fdto, "\x0B* Deleted *                        ", 36);
		}
		else {
			write (fdto, (char *)&msghdr.whoto, sizeof (struct _msgtoidx));
			msgidx.msgnum = msghdr.msgnum;
		}

		msgidx.board = msghdr.board;
		write (fdidx, (char *)&msgidx, sizeof (struct _gold_msgidx));

		msginfo.totalmsgs++;

		if (!msginfo.lowmsg)
			msginfo.lowmsg = msghdr.msgnum;

		if (msginfo.lowmsg > msghdr.msgnum)
			msginfo.lowmsg = msghdr.msgnum;
		if (msginfo.highmsg < msghdr.msgnum)
			msginfo.highmsg = msghdr.msgnum;

		if (!flag)
			msginfo.totalonboard[msgidx.board-1]++;
	}

	close (fdto);
	close (fdidx);
	close (fd);

	sprintf (filename, "%sMSGINFO.DAT", cfg.quick_msgpath);
	fd = open(filename,O_WRONLY|O_BINARY);
	write(fd, (char *)&msginfo, sizeof(struct _gold_msginfo));
	close (fd);
}

void Purge_Gold_Areas (int renum)
{
	#define MAXREADIDX   100
	int fd, fdidx, max_msgs[500], max_age[500], age_rcvd[500], month[12];
	int cnum[500], pnum[500], area, flag1, x, m, i;
	int far *qren[500];
	char filename[60];
	int fdto, dy, mo, yr, days, flag;
	word deleted = 0, *lastr, lrpos;
	long msgnum, pos, tempo, day_now, day_mess;
	struct tm *tim;
	struct _gold_msghdr msghdr;
	struct _gold_msgidx msgidx;
	struct _gold_msgidx idx[MAXREADIDX];
	struct _gold_msginfo msginfo;
	struct _glastread lr;
	struct _sys tsys;
	struct _usr usr;

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

	for (m = 0; m < 500; m++) {
		max_msgs[m] = 0;
		max_age[m] = 0;
		age_rcvd[m] = 0;
		cnum[m] = 0;
		pnum[m] = 0;
	}

	msgnum = 0;
	flag = 0;

	sprintf (filename, "%sSYSMSG.DAT", cfg.sys_path);
	fd = open(filename, O_RDONLY|O_BINARY);
	if (fd == -1)
		exit (1);

	while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
		if ( tsys.gold_board < 1 || tsys.gold_board > 500)
			continue;

		max_msgs[tsys.gold_board-1] = tsys.max_msgs;
		max_age[tsys.gold_board-1] = tsys.max_age;
		age_rcvd[tsys.gold_board-1] = tsys.age_rcvd;

		if (tsys.max_msgs || tsys.max_age || tsys.age_rcvd)
			flag = 1;
	}

	close (fd);

	if (!flag)
		return;

	printf(" * Reading message base data\n");

	memset (&msginfo, 0, sizeof (struct _gold_msginfo));

	sprintf(filename, "%sMSGIDX.DAT", cfg.quick_msgpath);
	fd = open (filename, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
	if (fd == -1){
		printf(" * Unable to open %s\n",filename);
		return;
	}

	do {
		m = read (fd, (char *)&idx, sizeof (struct _gold_msgidx) * MAXREADIDX);
		m /= sizeof (struct _gold_msgidx);

		for (x = 0; x < m; x++) {
			if (idx[x].board && idx[x].msgnum > 0) {
				if (!msginfo.lowmsg)
					msginfo.lowmsg = idx[x].msgnum;

				if (msginfo.lowmsg > idx[x].msgnum)
					msginfo.lowmsg = idx[x].msgnum;
				if (msginfo.highmsg < idx[x].msgnum)
					msginfo.highmsg = idx[x].msgnum;

				msginfo.totalmsgs++;
				msginfo.totalonboard[idx[x].board - 1]++;
			}
		}
	} while (m == MAXREADIDX);

	close(fd);

	if(!msginfo.totalmsgs){
		printf(" * No Messages in GoldBase\n");
		return;
	}

	printf(" * Purging GoldBase messages\n");

#ifdef __OS2__
	lastr = (word *)malloc (msginfo.totalmsgs * sizeof (word));
#else
	lastr = (word *)farmalloc (msginfo.totalmsgs * sizeof (word));
#endif
	if (lastr == NULL) {
		printf ("MEM Err: LASTREAD allocation.\n");
		exit (1);
	}
	memset (lastr, 0, (unsigned int)msginfo.totalmsgs * sizeof (word));
	lrpos = 0;

	for (m = 0; m < 500; m++) {
		if (msginfo.totalonboard[m]) {
#ifdef __OS2__
			qren[m] = (int *)malloc (msginfo.totalonboard[m] * (long)sizeof (int));
#else
			qren[m] = (int far *)farmalloc (msginfo.totalonboard[m] * (long)sizeof (int));
#endif
			if (qren[m] == NULL)
				exit (2);
		}
		else
			qren[m] = NULL;
	}

	tempo = time(0);
	tim = localtime(&tempo);

	day_now = 365 * tim->tm_year;
	for (m = 0; m < tim->tm_mon; m++)
		day_now += month[m];
	day_now += tim->tm_mday;

	sprintf (filename, "%sMSGHDR.DAT", cfg.quick_msgpath);
	fd = open (filename, O_RDWR|O_BINARY);
	if (fd == -1)
		exit (1);

	pos = tell (fd);

	sprintf (filename, "%sMSGIDX.DAT", cfg.quick_msgpath);
	fdidx = open(filename,O_RDWR|O_BINARY);

	sprintf (filename, "%sMSGTOIDX.DAT", cfg.quick_msgpath);
	fdto = open(filename,O_RDWR|O_BINARY);

	while (read(fd, (char *)&msghdr, sizeof (struct _gold_msghdr)) == sizeof (struct _gold_msghdr)) {
		flag = 0;
		if (msghdr.board >= 1 && msghdr.board <= 500 && msghdr.msgnum > 0) {
			pnum[msghdr.board-1]++;

			if (max_msgs[msghdr.board-1] && msginfo.totalonboard[msghdr.board-1] > max_msgs[msghdr.board-1]) {
				msghdr.msgattr |= Q_RECKILL;
				flag = 1;
			}
			else {
				sscanf(&msghdr.date[1], "%02d-%02d-%02d", &mo, &dy, &yr);

				day_mess = 365 * yr;
				mo--;
				for (m = 0; m < mo; m++)
					day_mess += month[m];
				day_mess += dy;

				days = (int)(day_now - day_mess);

				if (max_age[msghdr.board-1] && days > max_age[msghdr.board-1]) {
					msghdr.msgattr |= Q_RECKILL;
					flag = 1;
				}
				else if (age_rcvd[msghdr.board-1] &&
				 ( (msghdr.msgattr & Q_PRIVATE) && (msghdr.msgattr & Q_RECEIVED) ) &&
				 days > age_rcvd[msghdr.board-1]) {
					msghdr.msgattr |= Q_RECKILL;
					flag = 1;
				}
			}

			if ( flag ) {
				deleted++;
				lseek (fd, pos, SEEK_SET);
				write (fd, (char *)&msghdr, sizeof (struct _gold_msghdr));
				msginfo.totalmsgs--;
				msginfo.totalonboard[msghdr.board-1]--;
				lseek (fdidx, (long)msgnum * sizeof (struct _gold_msgidx), SEEK_SET);
				read (fdidx, (char *)&msgidx, sizeof (struct _gold_msgidx));
				msgidx.msgnum = -1;
				lseek (fdidx, (long)msgnum * sizeof (struct _gold_msgidx), SEEK_SET);
				write (fdidx, (char *)&msgidx, sizeof (struct _gold_msgidx));

				lseek (fdto, (long)msgnum * 36, SEEK_SET);
				write (fdto, "\x0B* Deleted *                        ", 36);

				lrpos++;
			}
			else {
				qren[msghdr.board-1][cnum[msghdr.board-1]++] = pnum[msghdr.board-1];
				lastr[lrpos++] = (word)msghdr.msgnum;
			}

			if (renum) {
				msgidx.msgnum = msghdr.msgnum;
				msgidx.board = msghdr.board;

				write (fdidx, (char *)&msgidx, sizeof (struct _gold_msgidx));
			}

//      sprintf (filename, "%d", msghdr.msgnum);
//      printf ("%s%*s", filename, strlen (filename), "\b\b\b\b\b\b\b");

			pos = tell (fd);
			if ((msgnum % 40) == 0 || flag)
				printf("   \300\304 Msg.: %lu, Deleted: %u\r", msgnum, deleted);

			msgnum++;
		}
		pos = tell (fd);
      if (msghdr.board < 1 && msghdr.board > 500){
				printf("Unknown board number: %d",msghdr.board);
				printf("Run index utility");
      }	}

	close (fdidx);
	close (fd);
	close (fdto);

	sprintf (filename, "%sMSGINFO.DAT", cfg.quick_msgpath);
	fd = open(filename,O_RDWR|O_BINARY);
	if (fd == -1)
		exit (1);
	write (fd, (char *)&msginfo, sizeof (struct _gold_msginfo));
	close (fd);

	for (m = 0; m < 500; m++)
		cnum[m] = 0;

	printf("\n * Updating USERS lastread pointer\n");

	sprintf (filename, "%sLASTREAD.DAT", cfg.quick_msgpath);
	fd = open (filename,O_RDWR|O_BINARY);

	pos = 0L;
	while (read (fd, &lr, sizeof (struct _glastread)) == sizeof (struct _glastread)) {
		for (i = 0; i < 500; i++) {
			m = 0;
			while (lastr[m] < lr.msgnum[i] && m < msginfo.totalmsgs)
				m++;
			while (m > 0 && lastr[m] == 0)
				m--;
			lr.msgnum[i] = lastr[m];
		}

		lseek (fd, pos, SEEK_SET);
		write (fd, &lr, sizeof (struct _glastread));
		pos = tell (fd);
	}

	close (fd);

	sprintf (filename, "%sSYSMSG.DAT", cfg.sys_path);
	fd = open(filename, O_RDONLY|O_BINARY);
	if (fd == -1)
		exit (1);

	while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
		if ( tsys.gold_board < 1 || tsys.gold_board > 500)
			continue;
		cnum[tsys.gold_board-1] = tsys.msg_num;
	}

	close (fd);

	sprintf (filename, "%s.BBS", cfg.user_file);
	fd = open(filename, O_RDWR | O_BINARY);
	pos = tell (fd);

	while (read(fd, (char *) &usr, sizeof(struct _usr)) == sizeof(struct _usr)) {
		for (area = 0; area < MAXLREAD; area++) {
			for (m = 0; m < 500; m++) {
				if (usr.lastread[area].area == cnum[m])
					break;
			}

			if (m < 500 && usr.lastread[area].msg_num) {
				flag1 = 0;

				for (x = 0; x < msginfo.totalonboard[m]; x++)
					if (qren[m][x] > flag1 && qren[m][x] <= usr.lastread[area].msg_num)
						flag1 = x + 1;

				usr.lastread[area].msg_num = flag1;
			}
		}

		for (area = 0; area < MAXDLREAD; area++) {
			for (m = 0; m < 500; m++) {
				if (usr.dynlastread[area].area == cnum[m])
					break;
			}

			if (m < 500 && usr.dynlastread[area].msg_num) {
				flag1 = 0;

				for (x = 0; x < msginfo.totalonboard[m]; x++)
					if (qren[m][x] > flag1 && qren[m][x] <= usr.dynlastread[area].msg_num)
						flag1 = x + 1;

				usr.dynlastread[area].msg_num = flag1;
			}
		}

		lseek (fd, pos, SEEK_SET);
		write (fd, (char *) &usr, sizeof(struct _usr));

		pos = tell (fd);
	}

	close(fd);

	for (m = 0; m < 500; m ++) {
		if (qren[m] != NULL){

#ifdef __OS2__
//         printf("qren[%d] %d", m,qren[m]);
         free (qren[m]);
//         printf("done");
#else
         farfree (qren[m]);
#endif
      }

	}
}

void Pack_Gold_Base (int renum)
{
	FILE *fph, *fpt, *fpi, *fpto, *outfph, *outfpt;
	int i;
	char filename[80], newname[80];
	long msgnum;
	unsigned char text[256];
	long block, messages, deleted;
	struct _gold_msghdr msghdr;
	struct _gold_msgidx msgidx;
	struct _gold_msginfo msginfo;

	printf (" * Packing GoldBBS Message Base\r\n");

	memset (&msginfo, 0, sizeof (struct _gold_msginfo));

	sprintf (filename, "%sMSGHDR.DAT", cfg.quick_msgpath);
	if ((fph = fopen (filename, "rb")) == NULL){
	printf (" ! Unable to open MSGHDR.DAT\n");
		return;
	}

	block = 0;
	msgnum = 1;
	deleted = 0;
	messages = filelength (fileno (fph)) / sizeof (struct _gold_msghdr);

	while (fread (&msghdr, sizeof (struct _gold_msghdr), 1, fph) == 1) {
		if (msghdr.msgattr & Q_RECKILL) {
			deleted++;
			continue;
		}
		if (msghdr.msgnum < 1 || msghdr.board < 1 || msghdr.board > 500) {
			deleted++;
			continue;
		}
	}

	if (deleted == 0) {
		fclose (fph);
		printf (" * No Messages Deleted\r\n");
		return;
	}

	messages -= deleted;

	sprintf (filename, "%sMSGTXT.DAT", cfg.quick_msgpath);
	if ((fpt = fopen (filename, "rb")) == NULL){
		printf("! Unable to open MSGTXT.DAT\n");
		exit (2);
		}
	sprintf (filename, "%sMSGIDX.DAT", cfg.quick_msgpath);
	if ((fpi = fopen (filename, "wb")) == NULL){
	printf("! Unable to open MSGIDX.DAT\n");
		exit (2);
	}
	sprintf (filename, "%sMSGTOIDX.DAT", cfg.quick_msgpath);
	if ((fpto = fopen (filename, "wb")) == NULL){
		printf("! Unable to open MSGTOIDX.DAT\n");
		exit (2);
		}
	sprintf (filename, "%sMSGHDR.NEW", cfg.quick_msgpath);
	if ((outfph = fopen (filename, "wb")) == NULL){
		printf("! Unable to open tempfile MSGHDR.NEW\n");
		exit (2);
	}
	sprintf (filename, "%sMSGTXT.NEW", cfg.quick_msgpath);
	if ((outfpt = fopen (filename, "wb")) == NULL){
		printf("! Unable to open tempfile MSGTXT.NEW\n");
		exit (2);
	}

	fseek (fph, 0L, SEEK_SET);

	while (fread (&msghdr, sizeof (struct _gold_msghdr), 1, fph) == 1) {
		if (msghdr.msgattr & Q_RECKILL)
			continue;
		if (msghdr.msgnum < 1 || msghdr.board < 1 || msghdr.board > 500)
			continue;

		printf ("   Packed Msg.# %ld / %ld\r", msgnum, messages);

		fseek (fpt, (long)msghdr.startblock * 256L, SEEK_SET);

		msghdr.startblock = (long)block;
		if (renum)
			msghdr.msgnum = msgnum;
		fwrite (&msghdr, sizeof (struct _gold_msghdr), 1, outfph);

		for (i = 0; i < msghdr.numblocks; i++) {
			fread (text, 256, 1, fpt);
			fwrite (text, 256, 1, outfpt);
		}

		msgidx.msgnum = msghdr.msgnum;
		msgidx.board = msghdr.board;
		fwrite (&msgidx, sizeof (struct _gold_msgidx), 1, fpi);

		fwrite (&msghdr.whoto, 36, 1, fpto);

		if (!msginfo.lowmsg || msghdr.msgnum < msginfo.lowmsg)
			msginfo.lowmsg = msghdr.msgnum;
		if (!msginfo.highmsg || msghdr.msgnum > msginfo.highmsg)
			msginfo.highmsg = msghdr.msgnum;
		msginfo.totalonboard[msghdr.board - 1]++;
		msginfo.totalmsgs++;

		block += msghdr.numblocks;
		msgnum++;
	}

	fclose (outfpt);
	fclose (outfph);
	fclose (fpto);
	fclose (fpi);
	fclose (fpt);
	fclose (fph);

	sprintf (filename, "%sMSGINFO.DAT", cfg.quick_msgpath);
	if ((fpto = fopen (filename, "wb")) == NULL){
		printf("! Unable to open MSGINFO.DAT\n");
		exit (2);
	}
	fwrite (&msginfo, sizeof (struct _gold_msginfo), 1, fpto);
	fclose (fpto);

	sprintf (filename, "%sMSGHDR.NEW", cfg.quick_msgpath);
	sprintf (newname, "%sMSGHDR.DAT", cfg.quick_msgpath);
	unlink (newname);
	rename (filename, newname);

	sprintf (filename, "%sMSGTXT.NEW", cfg.quick_msgpath);
	sprintf (newname, "%sMSGTXT.DAT", cfg.quick_msgpath);
	unlink (newname);
	rename (filename, newname);

	clreol ();
}



/* -----------------------------------------------------------------------

	Sezione di gestione della base messaggi PIP.

	----------------------------------------------------------------------- */
int read0 (unsigned char *s, FILE *f)
{
	register int i = 1;

   while ((*s=fgetc(f)) != 0 && !feof(f) && i < BUFSIZE) {
      s++;
      i++;
   }
   if (*s == 0)
      i = 1;
   else
      i = 0;
	*s = 0;
   return (i);
}

int readto0 (FILE *f)
{
   register int i = 1;

   while (fgetc(f) != 0 && !feof(f))
      i++;

   return (i);
}

void Create_Pip_Index (void)
{
   FILE *fpdp = NULL, *f1, *f2;
   int fd, mn, msgnum;
   char filename[80], farea = 1, to[36];
   struct _sys tsys;
   DESTPTR dp;
   MSGPTR mp;
	MSGPKTHDR mpkt;

   clreol ();
   printf(" * Create Pip-Base index files\n");

   sprintf (filename, "%sSYSMSG.DAT", cfg.sys_path);
   fd = open(filename, O_RDONLY|O_BINARY);
   if (fd == -1)
      exit (1);

   while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
      if ( !tsys.pip_board )
         continue;

      if (fpdp == NULL) {
         sprintf (filename, "%sDESTPTR.PIP", cfg.pip_msgpath);
         fpdp = fopen (filename, "wb");
      }

      sprintf (filename, "%sMPTR%04x.PIP", cfg.pip_msgpath, tsys.pip_board);
      f1 = fopen (filename, "rb");
      if (f1 == NULL)
         f1 = fopen (filename, "w+b");

      msgnum = (int)(filelength (fileno (f1)) / (long)sizeof (MSGPTR));

      sprintf (filename, "%sMPKT%04x.PIP", cfg.pip_msgpath, tsys.pip_board);
      f2 = fopen (filename, "rb");
      if (f2 == NULL)
         f2 = fopen (filename, "w+b");

      if (!farea) {
         gotoxy (wherex (), wherey () - 1);
         printf("   \303\n");
      }
		else
         farea = 0;

      printf("   \300\304 %3d - %-25.25s %-25.25s %d Msg.\n", tsys.msg_num, tsys.msg_name, tsys.echomail ? tsys.echotag : "", msgnum);

      rewind (f2);
      rewind (f1);
      mn = 0;

      while (fread (&mp, 1, sizeof (MSGPTR), f1) == sizeof (MSGPTR)) {
         fseek (f2, mp.pos, SEEK_SET);
         fread (&mpkt, 1, sizeof (MSGPKTHDR), f2);

			readto0 (f2);     // Date
         read0 (to, f2);   // To

         if (!(mp.status & SET_MPTR_DEL)) {
            memset (&dp, 0, sizeof (MSGPTR));
            strcpy (dp.to, to);
            dp.area = tsys.pip_board;
            dp.msg = mn++;
            fwrite (&dp, sizeof (DESTPTR), 1, fpdp);
         }
         else
            mn++;
		}

      fclose (f2);
      fclose (f1);
   }

   if (fpdp != NULL)
      fclose (fpdp);

   close (fd);
}

void Link_Pip_Areas (void)
{
}

void Purge_Pip_Areas (void)
{
   FILE *f1, *f2;
   int m, yr, mo, dy, month[12], days;
   int fd, msgnum, kill, farea = 1;
   char filename [80], mese[4];
   long day_mess, day_now, tempo;
	struct tm *tim;
   struct _sys tsys;
   MSGPTR mp;

   char *mon[] = {
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

	clreol ();
   printf(" * Purging Pip-Base messages\n");

   tempo = time(0);
   tim = localtime(&tempo);

   day_now = 365 * tim->tm_year;
   for (m = 0; m < tim->tm_mon; m++)
      day_now += month[m];
   day_now += tim->tm_mday;

   sprintf (filename, "%sSYSMSG.DAT", cfg.sys_path);
   fd = open(filename, O_RDONLY|O_BINARY);
	if (fd == -1)
      exit (1);

   msgnum = 0;

   while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
      if (!tsys.pip_board)
         continue;

      sprintf (filename, "%sMPTR%04x.PIP", cfg.pip_msgpath, tsys.pip_board);
      f1 = fopen (filename, "r+b");
      if (f1 == NULL)
			f1 = fopen (filename, "w+b");

      sprintf (filename, "%sMPKT%04x.PIP", cfg.pip_msgpath, tsys.pip_board);
      f2 = fopen (filename, "r+b");
      if (f2 == NULL)
         f2 = fopen (filename, "w+b");

      msgnum = (int)(filelength (fileno (f1)) / (long)sizeof (MSGPTR));
      rewind (f1);

		if (!farea) {
         gotoxy (wherex (), wherey () - 1);
         printf("   \303\n");
      }
      else
         farea = 0;

      printf("   \300\304 %3d - %-25.25s %-25.25s %d Msg.\n", tsys.msg_num, tsys.msg_name, tsys.echomail ? tsys.echotag : "", msgnum);

      if ( (tsys.max_msgs && tsys.max_msgs < msgnum) || tsys.max_age || tsys.age_rcvd ) {
         kill = msgnum - tsys.max_msgs;

         while (fread (&mp, sizeof (MSGPTR), 1, f1) == 1) {
				if (kill-- <= 0) {
               kill = 0;
               fseek (f2, mp.pos + sizeof (MSGPKTHDR), SEEK_SET);
               read0 (filename, f2);

               sscanf (filename, "%2d %3s %2d", &dy, mese, &yr);
               mese[3] = '\0';
               for (mo = 0; mo < 12; mo++)
                  if (!stricmp(mese, mon[mo]))
                     break;
               if (mo == 12)
                  mo = 0;

               day_mess = 365 * yr;
               for (m = 0; m < mo; m++)
                  day_mess += month[m];
               day_mess += dy;

               days = (int)(day_now - day_mess);

               if ( days > tsys.max_age && tsys.max_age != 0 ) {
                  mp.status |= SET_MPTR_DEL;
                  fseek (f1, -1L * (long)sizeof (MSGPTR), SEEK_CUR);
                  fwrite (&mp, sizeof (MSGPTR), 1, f1);
                  fseek (f1, 0L, SEEK_CUR);
               }
               else if (mp.status & SET_MPTR_RCVD) {
                  if ( days > tsys.age_rcvd && tsys.age_rcvd != 0 ) {
                     mp.status |= SET_MPTR_DEL;
                     fseek (f1, -1L * (long)sizeof (MSGPTR), SEEK_CUR);
                     fwrite (&mp, sizeof (MSGPTR), 1, f1);
                     fseek (f1, 0L, SEEK_CUR);
                  }
               }
            }
				else {
               mp.status |= SET_MPTR_DEL;
               fseek (f1, -1L * (long)sizeof (MSGPTR), SEEK_CUR);
               fwrite (&mp, sizeof (MSGPTR), 1, f1);
               fseek (f1, 0L, SEEK_CUR);
            }
         }
      }

      fclose (f2);
      fclose (f1);
   }

   close (fd);
}

int read0p (unsigned char *s, FILE *f, int max)
{
   register int i = 0;

   while ((*s=fgetc(f)) != 0 && !feof(f) && i < max) {
      s++;
      i++;
   }

   return (i);
}

void Pack_Pip_Areas (void)
{
   FILE *f1, *f2, *f1n, *f2n, *fp;
   int i, fd, fdu, msgnum, farea = 1, nmsg, *lastr, lrpos, m;
   char filename [80], buff[1024];
   long pos;
   struct _sys tsys;
	struct _usr usr;
   MSGPTR mp;
   MSGPKTHDR mpkt;
   DESTPTR dptr;

   clreol ();
   printf(" * Packing Pip-Base messages\n");

   sprintf (filename, "%sSYSMSG.DAT", cfg.sys_path);
   fd = open(filename, O_RDONLY|O_BINARY);
   if (fd == -1)
      exit (1);

   msgnum = 0;

   sprintf (filename, "%sDESTPTR.PIP", cfg.pip_msgpath);
   fp = fopen (filename, "wb");

   while ( read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA ) {
      if (!tsys.pip_board)
         continue;

      sprintf (filename, "%sMPTR%04x.PIP", cfg.pip_msgpath, tsys.pip_board);
      f1 = fopen (filename, "rb");
      if (f1 == NULL)
         continue;

      sprintf (filename, "%sMPKT%04x.PIP", cfg.pip_msgpath, tsys.pip_board);
      f2 = fopen (filename, "rb");
      if (f2 == NULL) {
         fclose (f1);
         continue;
      }

      sprintf (filename, "%sMPTR%04x.NEW", cfg.pip_msgpath, tsys.pip_board);
		f1n = fopen (filename, "wb");

      sprintf (filename, "%sMPKT%04x.NEW", cfg.pip_msgpath, tsys.pip_board);
      f2n = fopen (filename, "wb");

      msgnum = (int)(filelength (fileno (f1)) / (long)sizeof (MSGPTR));
      rewind (f1);

      msgnum++;
      lastr = (int *)malloc (msgnum * sizeof (int));
      memset (lastr, 0, msgnum * sizeof (int));
      msgnum--;

      if (!farea) {
         gotoxy (wherex (), wherey () - 1);
         printf("   \303\n");
      }
      else
         farea = 0;

      printf("   \300\304 %3d - %-25.25s %-25.25s %d Msg.\n", tsys.msg_num, tsys.msg_name, tsys.echomail ? tsys.echotag : "", msgnum);
      nmsg = 0;
      lrpos = 1;

      while (fread (&mp, sizeof (MSGPTR), 1, f1) == 1) {
         if (!(mp.status & SET_MPTR_DEL)) {
            lastr[lrpos++] = nmsg + 1;
            fseek (f2, mp.pos, SEEK_SET);

            mp.pos = ftell (f2n);
            fwrite (&mp, sizeof (MSGPTR), 1, f1n);

            fread (&mpkt, sizeof (MSGPKTHDR), 1, f2);
            fwrite (&mpkt, sizeof (MSGPKTHDR), 1, f2n);

				i = read0p (buff, f2, 20);
            fwrite (buff, i + 1, 1, f2n);
            i = read0p (buff, f2, 36);
            fwrite (buff, i + 1, 1, f2n);

            memset (&dptr, 0, sizeof (DESTPTR));
            dptr.area = tsys.pip_board;
            dptr.msg = nmsg++;
            strcpy (dptr.to, buff);
            fwrite (&dptr, sizeof (DESTPTR), 1, fp);

            i = read0p (buff, f2, 36);
				fwrite (buff, i + 1, 1, f2n);
            i = read0p (buff, f2, 72);
            fwrite (buff, i + 1, 1, f2n);

            do {
               i = read0p (buff, f2, 1024);
               fwrite (buff, i, 1, f2n);
            } while (i == 1024);

            fputc (0, f2n);
         }
         else
            lrpos++;
      }

      fputc (0, f2n);
      fputc (0, f2n);

      fclose (f2n);
      fclose (f1n);
      fclose (f2);
      fclose (f1);

		sprintf (filename, "%sMPTR%04x.PIP", cfg.pip_msgpath, tsys.pip_board);
      sprintf (buff, "%sMPTR%04x.NEW", cfg.pip_msgpath, tsys.pip_board);
      unlink (filename);
      rename (buff, filename);

      sprintf (filename, "%sMPKT%04x.PIP", cfg.pip_msgpath, tsys.pip_board);
      sprintf (buff, "%sMPKT%04x.NEW", cfg.pip_msgpath, tsys.pip_board);
      unlink (filename);
      rename (buff, filename);

      sprintf (filename, "%s.BBS", cfg.user_file);
      fdu = open (filename, O_RDWR | O_BINARY);
		pos = tell (fdu);

      while (read(fdu, (char *) &usr, sizeof(struct _usr)) == sizeof(struct _usr)) {
         for (i = 0; i < MAXLREAD; i++)
            if (usr.lastread[i].area == tsys.msg_num)
               break;

         if (i < MAXLREAD) {
            m = usr.lastread[i].msg_num;
            if (m > msgnum)
               m = msgnum;
            if (m < 0)
               m = 0;
            while (m > 0 && lastr[m] == 0)
               m--;
            usr.lastread[i].msg_num = lastr[m];

            lseek (fdu, pos, 0);
            write (fdu, (char *) &usr, sizeof(struct _usr));
         }
         else {
            for (i = 0; i < MAXDLREAD; i++)
               if (usr.dynlastread[i].area == tsys.msg_num)
						break;

            if (i < MAXDLREAD) {
               m = usr.dynlastread[i].msg_num;
               if (m > msgnum)
                  m = msgnum;
               if (m < 0)
                  m = 0;
               while (m > 0 && lastr[m] == 0)
                  m--;
               usr.dynlastread[i].msg_num = lastr[m];

					lseek (fdu, pos, 0);
               write (fdu, (char *) &usr, sizeof (struct _usr));
            }
         }

         pos = tell (fdu);
      }

      close (fdu);

      if (lastr != NULL)
         free (lastr);
   }

   fclose (fp);
   close (fd);
}

void main (argc, argv)
int argc;
char *argv[];
{
   int i, xlink, unknow, renum, pack, purge, overwrite;
	int link, stats, index, xsquish, xfido, xpip, xquick, xgold;

#ifdef __OS2__
   printf("\nLMSG; LoraBBS-OS/2 Message maintenance utility, Version %s\n", LMSG_VERSION);
#else
   printf("\nLMSG; LoraBBS-DOS Message maintenance utility, Version %s\n", LMSG_VERSION);
#endif
   printf("      Copyright (c) 1991-96 by Marco Maccaferri, All Rights Reserved\n\n");

   xlink = unknow = renum = pack = purge = overwrite = 0;
   link = stats = index = 0;
   xquick = xfido = xsquish = xpip = xgold = 0;

   for (i = 1; i < argc; i++) {
      if (argv[i][0] != '-') {
         cfgname = argv[i];
         continue;
      }
      if (!strnicmp (argv[i], "-I", 2)) {
         index = 1;
         if (strchr (argv[i], 'C') != NULL || strchr (argv[i], 'c') != NULL)
            xlink = 1;
         if (strchr (argv[i], 'U') != NULL || strchr (argv[i], 'u') != NULL)
            unknow = 1;
         if (strchr (argv[i], 'R') != NULL || strchr (argv[i], 'r') != NULL)
            renum = 1;
      }
      if (!strnicmp (argv[i], "-P", 2)) {
         pack = 1;
         if (strchr (argv[i], 'K') != NULL || strchr (argv[i], 'k') != NULL)
            purge = 1;
         if (strchr (argv[i], 'O') != NULL || strchr (argv[i], 'o') != NULL)
            overwrite = 1;
			if (strchr (argv[i], 'R') != NULL || strchr (argv[i], 'r') != NULL)
            renum = 1;
			if (strchr (argv[i], 'A') != NULL || strchr (argv[i], 'a') != NULL)
            overwrite = 2;
      }
      if (!strnicmp (argv[i], "-X", 2)) {
         if (strchr (argv[i], 'F') != NULL || strchr (argv[i], 'f') != NULL)
            xfido = 1;
         if (strchr (argv[i], 'Q') != NULL || strchr (argv[i], 'q') != NULL)
            xquick = 1;
         if (strchr (argv[i], 'S') != NULL || strchr (argv[i], 's') != NULL)
            xsquish = 1;
         if (strchr (argv[i], 'P') != NULL || strchr (argv[i], 'p') != NULL)
            xpip = 1;
			if (strchr (argv[i], 'G') != NULL || strchr (argv[i], 'g') != NULL)
            xgold = 1;
      }
      if (!stricmp (argv[i], "-K"))
         purge = 1;
      if (!stricmp (argv[i], "-L"))
         link = 1;
      if (!stricmp (argv[i], "-R"))
         renum = 1;
      if (!stricmp (argv[i], "-S"))
         stats = 1;
   }

   if ( (!xlink && !unknow && !renum && !pack && !purge && !overwrite && !link && !stats && !index) || argc == 1 ) {
      printf(" * Command-line parameters:\n\n");

      printf("        <File>    Configuration file name\n");
      printf("        -I[UR]    Recreate index files & check\n");
      printf("                  U=Kill unknown boards  R=Renumber\n");
      printf("        -P[KR]    Pack (compress) message base\n");
      printf("                  K=Purge  R=Renumber\n");
		printf("        -K        Purge messages from info in SYSMSG.DAT\n");
      printf("        -R        Renumber messages\n");
		printf("        -L        Link messages by subject\n");
      printf("        -X[FQSPG] Exclude message base\n");
      printf("                  F=Fido  Q=Quick  S=Squish  P=Pipbase  G=GoldBase\n\n");

      printf(" * Please refer to the documentation for a more complete command summary\n\n");
   }
   else {
      Get_Config_Info ();

      if (index) {
         if (!xfido) {
            if (renum)
					Renum_Fido_Areas ();
            Create_Fido_Index ();
         }
         if (!xquick)
            Create_Quick_Index (xlink, unknow, renum);
         if (!xgold)
            Create_Gold_Index (xlink, unknow, renum);
         if (!xpip)
            Create_Pip_Index ();
      }

      if (purge) {
         if (!xfido)
            Purge_Fido_Areas (renum);
         if (!xquick)
				Purge_Quick_Areas (renum);
			if (!xgold)
				Purge_Gold_Areas (renum);
			if (!xsquish)
				Purge_Squish_Areas ();
			if (!xpip)
				Purge_Pip_Areas ();
		}

		if (pack) {
			if (!xquick)
				Pack_Quick_Base (renum);
			if (!xgold)
				Pack_Gold_Base (renum);
			if (!xsquish)
				Pack_Squish_Areas ();
			if (!xpip)
				Pack_Pip_Areas ();
		}

		if (renum && !purge && !index) {
			if (!xfido)
				Renum_Fido_Areas ();
			if (!xquick)
				Renum_Quick_Areas ();
		}

		if (link) {
			if (!xfido)
				Link_Fido_Areas ();
			if (!xquick)
				Link_Quick_Areas ();
			if (!xsquish)
				Link_Squish_Areas ();
			if (!xpip)
				Link_Pip_Areas ();
		}

		clreol ();
		printf(" * Done.\n\n");
	}
}

