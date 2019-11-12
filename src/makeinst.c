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
#include <dir.h>
#include <io.h>
#include <fcntl.h>
#include <alloc.h>
#include <dos.h>
#include <string.h>
#include <sys\stat.h>

#ifndef __OS2__
//#include "implode.h"
#endif

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
    int  action;
    int  fid;
    char filename[14];
    unsigned ftime;
    unsigned fdate;
    long complen;
} COMPREC;

#ifdef __OS2__
unsigned ReadBuff(char * buff, unsigned short int * size)
{
    unsigned m;

    m = read(fdin, buff, *size);
    printf("%3d%%\b\b\b\b", (int)(tell(fdin) * 100L / filelength(fdin)));

    return (m);
}

unsigned WriteBuff(char * buff, unsigned short int * size)
{
    return (write(fdout, buff, *size));
}
#else
unsigned far pascal ReadBuff(char far * buff, unsigned short int far * size)
{
    unsigned m;

    m = read(fdin, buff, *size);
    printf("%3d%%\b\b\b\b", (int)(tell(fdin) * 100L / filelength(fdin)));

    return (m);
}

unsigned far pascal WriteBuff(char far * buff, unsigned short int far * size)
{
    return (write(fdout, buff, *size));
}
#endif

void store_file(char * work_buff)
{
    unsigned short int i;
    unsigned short int size = 32767U;

    do {
        i = ReadBuff(work_buff, &size);
        WriteBuff(work_buff, &i);
    } while (i == size);
}

void main(int argc, char * argv[])
{
    FILE * fp;
    int m, dd, mm, yy, hh, ss, rtime = -1, rdate;
    unsigned short int type;
    unsigned short int dsize;
    char string[180], filename[80], *p, *path, *newname, store = 0;
    char * WorkBuff;
    long pos, stublen = -1L;
    struct ffblk blk;
    COMPREC cr;

    WorkBuff = (char *)malloc(35256U);

    if (argc == 1) {
        fp = fopen("INSTALL.CFG", "rt");
    }
    else {
        fp = fopen(argv[1], "rt");
    }
    while (fgets(string, 70, fp) != NULL) {
        while (string[strlen(string) - 1] == 0x0A) {
            string[strlen(string) - 1] = '\0';
        }
        if (string[0] == ';') {
            continue;
        }

        p = strtok(string, " ");
        if (p == NULL || *p == ';') {
            continue;
        }

        if (!stricmp(p, "OUTFILE")) {
            p = strtok(NULL, " ");
            if (fdout != -1) {
                if (stublen != -1L) {
                    write(fdout, &stublen, 4);
                }
                close(fdout);
            }
            fdout = open(p, O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
            printf("\nCreating File: %s\n", p);
        }
        else if (!stricmp(p, "TARGET")) {
            memset(&cr, 0, sizeof(COMPREC));
            cr.action = MKDIR;

            p = strtok(NULL, " ");
            printf("Target Directory: %s\n", p);
            strcpy(cr.filename, strlwr(p));

            write(fdout, &cr, sizeof(COMPREC));
        }
        else if (!stricmp(p, "STORE")) {
            store = 1;
        }
        else if (!stricmp(p, "COMPRESS")) {
            store = 0;
        }
        else if (!stricmp(p, "ERASE")) {
            memset(&cr, 0, sizeof(COMPREC));
            cr.action = ERASE;

            p = strtok(NULL, " ");
            printf("Erase: %s\n", p);
            strcpy(cr.filename, p);

            write(fdout, &cr, sizeof(COMPREC));
        }
        else if (!stricmp(p, "EXECUTE")) {
            memset(&cr, 0, sizeof(COMPREC));
            cr.action = EXECUTE;

            p = strtok(NULL, " ");
            printf("Execute: %s\n", p);
            strcpy(cr.filename, strlwr(p));

            write(fdout, &cr, sizeof(COMPREC));
        }
        else if (!stricmp(p, "RELEASETIME")) {
            p = strtok(NULL, " ");
            if (!stricmp(p, "ORIGINAL")) {
                rtime = -1;
            }
            else {
                sscanf(p, "%2d-%2d-%2d", &dd, &mm, &yy);
                p = strtok(NULL, " ");
                sscanf(p, "%2d:%2d", &hh, &ss);
                rdate = ((yy - 80) << 9) | (mm << 5) | dd;
                rtime = (hh << 11) | (ss << 5);
                printf("Release Time: %02d-%02d-%02d %02d:%02d\n", dd, mm, yy, hh, ss);
            }
        }
        else if (!stricmp(p, "UPGRADE")) {
            memset(&cr, 0, sizeof(COMPREC));
            cr.action = UPGRADE;
            write(fdout, &cr, sizeof(COMPREC));
        }
        else if (!stricmp(p, "NEWINSTALL")) {
            memset(&cr, 0, sizeof(COMPREC));
            cr.action = OVERWRITE;
            write(fdout, &cr, sizeof(COMPREC));
        }
        else if (!stricmp(p, "STUBEXE")) {
            p = strtok(NULL, " ");
            printf("Stub Executable: %s\n", p);

            if ((fdin = open(p, O_RDONLY | O_BINARY)) != -1) {
                do {
                    m = read(fdin, &string, 170);
                    write(fdout, &string, m);
                } while (m == 170);
                stublen = filelength(fdin);
                close(fdin);
            }
        }
        else if (!stricmp(p, "SOURCE")) {
            memset(&cr, 0, sizeof(COMPREC));

            cr.action = store ? STORED : UNPACK;

            path = strtok(NULL, " ");
            p = strtok(NULL, " ");
            newname = strtok(NULL, " ");

            printf("Source Path: %s, FileSpec.: %s\n", path, p);

            sprintf(filename, "%s\\%s", path, p);
            if (!findfirst(filename, &blk, 0))
                do {
                    printf("\r    > %-12s ", blk.ff_name);
                    if (newname != NULL && newname[0]) {
                        strcpy(cr.filename, strlwr(newname));
                    }
                    else {
                        strcpy(cr.filename, strlwr(blk.ff_name));
                    }
                    sprintf(filename, "%s\\%s", path, blk.ff_name);
                    fdin = open(filename, O_RDONLY | O_BINARY);

                    pos = tell(fdout);
                    write(fdout, &cr, sizeof(COMPREC));

//               if (!store) {
//#ifndef __OS2__
//                  type = CMP_BINARY;
//                  dsize = 4096;
//                  implode (ReadBuff, WriteBuff, WorkBuff, &type, &dsize);
//#endif
//               }
//               else
                    store_file(WorkBuff);

                    cr.complen = tell(fdout) - pos - sizeof(COMPREC);
                    lseek(fdout, pos, SEEK_SET);
                    if (rtime == -1) {
                        _dos_getftime(fdin, &cr.fdate, &cr.ftime);
                    }
                    else {
                        cr.fdate = rdate;
                        cr.ftime = rtime;
                    }
                    write(fdout, &cr, sizeof(COMPREC));
                    lseek(fdout, 0L, SEEK_END);

                    close(fdin);
                } while (!findnext(&blk));
            printf("\r                       \r");
        }
        else if (!stricmp(p, "STAMPDATE")) {
            memset(&cr, 0, sizeof(COMPREC));

            cr.action = STAMPDATE;

            newname = strtok(NULL, " ");
            strcpy(cr.filename, strupr(newname));
            cr.fdate = rdate;
            cr.ftime = rtime;
            write(fdout, &cr, sizeof(COMPREC));
        }
    }
    fclose(fp);

    if (fdout != -1) {
        if (stublen != -1L) {
            write(fdout, &stublen, 4);
        }

        close(fdout);
    }
}

