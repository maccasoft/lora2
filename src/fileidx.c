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

#include <errno.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <time.h>
#include <dir.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <sys\stat.h>

#include "lsetup.h"
#include "version.h"

#ifdef __OS2__
extern int errno = 0;
#endif

typedef struct {
    char name[13];
    word area;
    long datpos;
} FILEIDX;

long totfiles;
char quiet = 0;

int sh_open(char * file, int shmode, int omode, int fmode)
{
    int i;
    long t1, t2;
    long time(long *);

    t1 = time(NULL);
    while (time(NULL) < t1 + 20) {
        if ((i = sopen(file, omode, shmode, fmode)) != -1 || errno != EACCES) {
            break;
        }
        t2 = time(NULL);
        while (time(NULL) < t2 + 1)
            ;
    }

    return (i);
}

void build_fileidx_range(char * dest, int start, int stop)
{
    int fda, fdd, nfiles;
    char filename[80];
    struct ffblk blk;
    struct _sys sys;
    FILEIDX fidx;

    if (start == stop) {
        printf(" * Adding index for file area #%d\n", start);
    }
    else {
        printf(" * Adding index for file areas #%d to #%d\n", start, stop);
    }

    fda = sh_open("SYSFILE.DAT", SH_DENYWR, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);
    if (fda == -1) {
        return;
    }

    fdd = sh_open(dest, SH_DENYRW, O_WRONLY | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
    if (fdd == -1) {
        close(fda);
        return;
    }
    lseek(fdd, 0L, SEEK_END);

    while (read(fda, &sys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA) {
        if (!sys.filepath[0] || sys.file_num < start || sys.file_num > stop) {
            continue;
        }

        if (!quiet) {
            if (sys.file_num != start) {
                gotoxy(wherex(), wherey() - 1);
                printf("   \303\n");
            }

            clreol();
            printf("   \300\304 %3d - %-50.50s ", sys.file_num, sys.file_name);
        }
        nfiles = 0;

        sprintf(filename, "%s*.*", sys.filepath);
        if (!findfirst(filename, &blk, 0))
            do {
                if (!stricmp(blk.ff_name, "FILES.BBS") || !stricmp(blk.ff_name, "FILES.BAK")) {
                    continue;
                }
                memset(&fidx, 0, sizeof(FILEIDX));
                strcpy(fidx.name, blk.ff_name);
                fidx.area = sys.file_num;
                fidx.datpos = blk.ff_fsize;
                write(fdd, &fidx, sizeof(FILEIDX));
                nfiles++;
                if (!quiet) {
                    printf("%5d\b\b\b\b\b", nfiles);
                }
            } while (!findnext(&blk));

        if (!quiet) {
            printf("\n");
        }
        totfiles += nfiles;
    }

    close(fda);
    close(fdd);
}

void main(int argc, char * argv[])
{
    int i;
    char * p;

#ifdef __OS2__
    printf("\nFILEIDX; LoraBBS-OS/2 File-Area Index utility, Version %s\n", FILEIDX_VERSION);
#else
    printf("\nFILEIDX; LoraBBS-DOS File-Area Index utility, Version %s\n", FILEIDX_VERSION);
#endif
    printf("         Copyright (C) 1991-96 by Marco Maccaferri, All Rights Reserved\n\n");

    if (argc < 3) {
        printf("Usage   : FILEIDX [index] [commands] [-Q]\n\n");

        printf("Commands are : AREA <n> [<n>]  - Adds the file area specified by <n> [<n>] ...\n");
        printf("               RANGE <n1-n2>   - Adds the file areas in the range <n1> to <n2>\n");
        printf("               -Q              - Quiet mode\n\n");

        printf("Please refer to the documentation for a more complete command summary\n\n");
        exit(0);
    }

    unlink(argv[1]);
    totfiles = 0;

    for (i = 2; i < argc; i++)
        if (!stricmp(argv[i], "-Q")) {
            quiet = 1;
        }

    for (i = 2; i < argc; i++) {
        if (!stricmp(argv[i], "AREA")) {
            i++;
            while (i < argc && isdigit(argv[i][0])) {
                build_fileidx_range(argv[1], atoi(argv[i]), atoi(argv[i]));
                i++;
            }
            i--;
        }
        else if (!stricmp(argv[i], "RANGE")) {
            i++;
            while (i < argc && isdigit(argv[i][0])) {
                if ((p = strchr(argv[i], '-')) == NULL) {
                    break;
                }
                *p++ = '\0';
                build_fileidx_range(argv[1], atoi(argv[i]), atoi(p));
                i++;
            }
            i--;
        }
    }

    printf(" * Total of %ld files.\n", totfiles);
}

