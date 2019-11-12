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
#include <string.h>
#include <stdlib.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "pipbase.h"

extern char * pip[128], *internet_to;
extern int msg_parent, msg_child;

#define SET_MARK 1
#define SET_DEL 2
#define SET_NEW 4

int read0(unsigned char *, FILE *);
void pipstring(unsigned char *, FILE *);
void write0(unsigned char *, FILE *);
int full_pip_msg_read(unsigned int, unsigned int, char, int);

void pipstring(s, f)
unsigned char * s;
FILE * f;
{
    register int best;
    char l;
    while (*s)
    {
        if (*s == 10) {
            s++;
        }
        else
        {
            best = 127;
            l = 1;
            switch (*s)
            {
                case ' ':
                    if (*(s + 1) == ' ') {
                        best = 20;
                        l = 2;
                    }
                    break;
                case '-':
                    if ((*(s + 1) == '-') && (*(s + 2) == '-')) {
                        best = 7;
                        l = 3;
                    }
                    break;
                case '\'':
                    if (*(s + 1) == ' ') {
                        best = 123;
                        l = 2;
                    }
                    break;
                case '+':
                    if (*(s + 1) == '+') {
                        best = 24;
                        l = 2;
                    }
                    break;
                case ';':
                    if (*(s + 1) == ' ') {
                        best = 23;
                        l = 2;
                    }
                    break;
                case ',':
                    if (*(s + 1) == ' ') {
                        best = 21;
                        l = 2;
                    }
                    break;
                case '=':
                    if (*(s + 1) == '=') {
                        best = 121;
                        l = 2;
                    }
                    break;
                case ':':
                    switch (*(s + 1)) {
                        case '-':
                            best = 122;
                            l = 2;
                            break;
                        case ' ':
                            best = 4;
                            l = 2;
                            break;
                    }
                    break;
                case '.':
                    switch (*(s + 1)) {
                        case '.':
                            best = 19;
                            l = 2;
                            break;
                        case ' ':
                            best = 22;
                            l = 2;
                            break;
                    }
                    break;
                case 'M':
                    if ((*(s + 1) == 'S') &&
                            (*(s + 2) == 'G') &&
                            (*(s + 3) == 'I') &&
                            (*(s + 4) == 'D') &&
                            (*(s + 5) == ':') &&
                            (*(s + 6) == ' ')) {
                        best = 2;
                        l = 7;
                    }
                    break;
                case 'P':
                    if ((*(s + 1) == 'A') &&
                            (*(s + 2) == 'T') &&
                            (*(s + 3) == 'H') &&
                            (*(s + 4) == ':') &&
                            (*(s + 5) == ' ')) {
                        best = 3;
                        l = 6;
                    }
                    break;
                case 'S':
                    if ((*(s + 1) == 'E') &&
                            (*(s + 2) == 'E') &&
                            (*(s + 3) == 'N') &&
                            (*(s + 4) == '-') &&
                            (*(s + 5) == 'B') &&
                            (*(s + 6) == 'Y') &&
                            (*(s + 7) == ':') &&
                            (*(s + 8) == ' ')) {
                        best = 1;
                        l = 9;
                    }
                    break;
                case 'a':
                    switch (*(s + 1)) {
                        case ' ':
                            best = 30;
                            l = 2;
                            break;
                        case 'l':
                            best = 15;
                            l = 2;
                            break;
                        case '\'':
                            best = 25;
                            l = 2;
                            break;
                    }
                    break;
                case 'b':
                    switch (*(s + 1)) {
                        case 'b':
                            best = 37;
                            l = 2;
                            break;
                        case 'a':
                            best = 38;
                            l = 2;
                            break;
                        case 'e':
                            best = 39;
                            l = 2;
                            break;
                        case 'i':
                            best = 40;
                            l = 2;
                            break;
                        case 'o':
                            best = 41;
                            l = 2;
                            break;
                        case 'u':
                            best = 42;
                            l = 2;
                            break;
                    }
                    break;
                case 'c':
                    switch (*(s + 1)) {
                        case 'h':
                            if (*(s + 2) == 'i') {
                                best = 9;
                                l = 3;
                            }
                            else if (*(s + 2) == 'e') {
                                best = 8;
                                l = 3;
                            };
                            break;
                        case 'c':
                            best = 43;
                            l = 2;
                            break;
                        case 'a':
                            best = 44;
                            l = 2;
                            break;
                        case 'e':
                            best = 45;
                            l = 2;
                            break;
                        case 'i':
                            best = 46;
                            l = 2;
                            break;
                        case 'o':
                            best = 47;
                            l = 2;
                            break;
                        case 'u':
                            best = 48;
                            l = 2;
                            break;
                    }
                    break;
                case 'd':
                    switch (*(s + 1)) {
                        case 'd':
                            best = 49;
                            l = 2;
                            break;
                        case 'a':
                            best = 50;
                            l = 2;
                            break;
                        case 'e':
                            best = 51;
                            l = 2;
                            break;
                        case 'i':
                            best = 52;
                            l = 2;
                            break;
                        case 'o':
                            best = 53;
                            l = 2;
                            break;
                        case 'u':
                            best = 54;
                            l = 2;
                            break;
                    }
                    break;
                case 'e':
                    switch (*(s + 1)) {
                        case ' ':
                            best = 31;
                            l = 2;
                            break;
                        case 'd':
                            best = 16;
                            l = 2;
                            break;
                        case '\'':
                            best = 26;
                            l = 2;
                            break;
                    }
                    break;
                case 'f':
                    switch (*(s + 1)) {
                        case 'f':
                            best = 55;
                            l = 2;
                            break;
                        case 'a':
                            best = 56;
                            l = 2;
                            break;
                        case 'e':
                            best = 57;
                            l = 2;
                            break;
                        case 'i':
                            best = 58;
                            l = 2;
                            break;
                        case 'o':
                            best = 59;
                            l = 2;
                            break;
                        case 'u':
                            best = 60;
                            l = 2;
                            break;
                    }
                    break;
                case 'g':
                    switch (*(s + 1)) {
                        case 'h':
                            if (*(s + 2) == 'i') {
                                best = 11;
                                l = 3;
                            }
                            else if (*(s + 2) == 'e') {
                                best = 10;
                                l = 3;
                            };
                            break;
                        case 'g':
                            best = 61;
                            l = 2;
                            break;
                        case 'a':
                            best = 62;
                            l = 2;
                            break;
                        case 'e':
                            best = 63;
                            l = 2;
                            break;
                        case 'i':
                            best = 64;
                            l = 2;
                            break;
                        case 'o':
                            best = 65;
                            l = 2;
                            break;
                        case 'u':
                            best = 66;
                            l = 2;
                            break;
                    }
                    break;
                case 'h':
                    switch (*(s + 1)) {
                        case 'i':
                            best = 36;
                            l = 2;
                            break;
                        case 'a':
                            best = 124;
                            l = 2;
                            break;
                        case 'o':
                            best = 125;
                            l = 2;
                            break;
                    }
                    break;
                case 'i':
                    switch (*(s + 1)) {
                        case ' ':
                            best = 32;
                            l = 2;
                            break;
                        case 'l':
                            best = 14;
                            l = 2;
                            break;
                        case '\'':
                            best = 27;
                            l = 2;
                            break;
                    }
                    break;
                case 'l':
                    switch (*(s + 1)) {
                        case 'l':
                            best = 67;
                            l = 2;
                            break;
                        case 'a':
                            best = 68;
                            l = 2;
                            break;
                        case 'e':
                            best = 69;
                            l = 2;
                            break;
                        case 'i':
                            best = 70;
                            l = 2;
                            break;
                        case 'o':
                            best = 71;
                            l = 2;
                            break;
                        case 'u':
                            best = 72;
                            l = 2;
                            break;
                    }
                    break;
                case 'm':
                    switch (*(s + 1)) {
                        case 'm':
                            best = 73;
                            l = 2;
                            break;
                        case 'a':
                            best = 74;
                            l = 2;
                            break;
                        case 'e':
                            if ((*(s + 2) == 'n') && (*(s + 3) == 't')) {
                                best = 6;
                                l = 4;
                            }
                            else {
                                best = 75;
                                l = 2;
                            }
                            break;
                        case 'i':
                            best = 76;
                            l = 2;
                            break;
                        case 'o':
                            best = 77;
                            l = 2;
                            break;
                        case 'u':
                            best = 78;
                            l = 2;
                            break;
                    }
                    break;
                case 'n':
                    switch (*(s + 1)) {
                        case 'n':
                            best = 79;
                            l = 2;
                            break;
                        case 't':
                            best = 35;
                            l = 2;
                            break;
                        case 'a':
                            best = 80;
                            l = 2;
                            break;
                        case 'e':
                            best = 81;
                            l = 2;
                            break;
                        case 'i':
                            best = 82;
                            l = 2;
                            break;
                        case 'o':
                            best = 83;
                            l = 2;
                            break;
                        case 'u':
                            best = 84;
                            l = 2;
                            break;
                    }
                    break;
                case 'o':
                    switch (*(s + 1)) {
                        case ' ':
                            best = 33;
                            l = 2;
                            break;
                        case '\'':
                            best = 28;
                            l = 2;
                            break;
                    }
                    break;
                case 'p':
                    switch (*(s + 1)) {
                        case 'p':
                            best = 85;
                            l = 2;
                            break;
                        case 'r':
                            best = 17;
                            l = 2;
                            break;
                        case 'a':
                            best = 86;
                            l = 2;
                            break;
                        case 'e':
                            best = 87;
                            l = 2;
                            break;
                        case 'i':
                            best = 88;
                            l = 2;
                            break;
                        case 'o':
                            best = 89;
                            l = 2;
                            break;
                        case 'u':
                            best = 90;
                            l = 2;
                            break;
                    }
                    break;
                case 'q':
                    if (*(s + 1) == 'u') {
                        best = 126;
                        l = 2;
                    }
                    break;
                case 'r':
                    switch (*(s + 1)) {
                        case 'r':
                            best = 91;
                            l = 2;
                            break;
                        case 'a':
                            best = 92;
                            l = 2;
                            break;
                        case 'e':
                            best = 93;
                            l = 2;
                            break;
                        case 'i':
                            best = 94;
                            l = 2;
                            break;
                        case 'o':
                            best = 95;
                            l = 2;
                            break;
                        case 'u':
                            best = 96;
                            l = 2;
                            break;
                    }
                    break;
                case 's':
                    switch (*(s + 1)) {
                        case 's':
                            best = 97;
                            l = 2;
                            break;
                        case 't':
                            best = 18;
                            l = 2;
                            if (*(s + 2) == 'r') {
                                best = 12;
                                l = 3;
                            }
                            break;
                        case 'a':
                            best = 98;
                            l = 2;
                            break;
                        case 'e':
                            best = 99;
                            l = 2;
                            break;
                        case 'i':
                            best = 100;
                            l = 2;
                            break;
                        case 'o':
                            best = 101;
                            l = 2;
                            break;
                        case 'u':
                            best = 102;
                            l = 2;
                            break;
                    }
                    break;
                case 't':
                    switch (*(s + 1)) {
                        case 't':
                            best = 103;
                            l = 2;
                            break;
                        case 'a':
                            best = 104;
                            l = 2;
                            break;
                        case 'e':
                            best = 105;
                            l = 2;
                            break;
                        case 'i':
                            best = 106;
                            l = 2;
                            break;
                        case 'o':
                            best = 107;
                            l = 2;
                            break;
                        case 'u':
                            best = 108;
                            l = 2;
                            break;
                    }
                    break;
                case 'u':
                    switch (*(s + 1)) {
                        case ' ':
                            best = 34;
                            l = 2;
                            break;
                        case '\'':
                            best = 29;
                            l = 2;
                            break;
                    }
                    break;
                case 'v':
                    switch (*(s + 1)) {
                        case 'v':
                            best = 109;
                            l = 2;
                            break;
                        case 'a':
                            best = 110;
                            l = 2;
                            break;
                        case 'e':
                            best = 111;
                            l = 2;
                            break;
                        case 'i':
                            best = 112;
                            l = 2;
                            break;
                        case 'o':
                            best = 113;
                            l = 2;
                            break;
                        case 'u':
                            best = 114;
                            l = 2;
                            break;
                    }
                    break;
                case 'z':
                    switch (*(s + 1)) {
                        case 'z':
                            best = 115;
                            l = 2;
                            break;
                        case 'a':
                            best = 116;
                            l = 2;
                            break;
                        case 'e':
                            best = 117;
                            l = 2;
                            break;
                        case 'i':
                            best = 118;
                            l = 2;
                            if ((*(s + 2) == 'o') && (*(s + 3) == 'n')) {
                                best = 5;
                                l = 4;
                            }
                            break;
                        case 'o':
                            best = 119;
                            l = 2;
                            break;
                        case 'u':
                            best = 120;
                            l = 2;
                            break;
                    }
                    break;
            }
            if (best == 127)
            {
                if ((*s < 128) || (*s == 141)) {
                    fputc(*s, f);
                }
                else {
                    fputc(128, f);
                    fputc(*s - 127, f);
                }
                s++;
            }
            else
            {
                fputc(128 + best, f);
                s += l;
            }
        }
    }
}


int read0(s, f)
unsigned char * s;
FILE * f;
{
    register int i = 1;

    while ((*s = fgetc(f)) != 0 && !feof(f) && i < BUFSIZE)
    {
        s++;
        i++;
    }
    if (*s == 0) {
        i = 1;
    }
    else {
        i = 0;
    }
    *s = 0;
    return (i);
}


void write0(s, f)
unsigned char * s;
FILE * f;
{
    while (*s)
    {
        if (*s != 10) {
            fputc(*s, f);
        }
        s++;
    }
    fputc(0, f);
}


void pip_scan_message_base(area, upd)
int area;
char upd;
{
    int f1, i;
    char fn[80];

    sprintf(fn, "%sMPTR%04x.PIP", pip_msgpath, sys.pip_board);
    f1 = sh_open(fn, SH_DENYNONE, O_RDONLY | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
    num_msg = (int)(filelength(f1) / sizeof(MSGPTR));
    close(f1);

    sprintf(fn, "%sMPKT%04x.PIP", pip_msgpath, sys.pip_board);
    f1 = sh_open(fn, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);
    if (f1 == -1) {
        f1 = sh_open(fn, SH_DENYNONE, O_WRONLY | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        fn[0] = fn[1] = '\0';
        write(f1, fn, 2);
    }
    close(f1);

    first_msg = 1;
    last_msg = num_msg;
    msg_parent = msg_child = 0;

    for (i = 0; i < MAXLREAD; i++)
        if (usr.lastread[i].area == area) {
            break;
        }
    if (i != MAXLREAD) {
        if (usr.lastread[i].msg_num > last_msg) {
            usr.lastread[i].msg_num = last_msg;
        }
        lastread = usr.lastread[i].msg_num;
    }
    else {
        for (i = 0; i < MAXDLREAD; i++)
            if (usr.dynlastread[i].area == area) {
                break;
            }
        if (i != MAXDLREAD) {
            if (usr.dynlastread[i].msg_num > last_msg) {
                usr.dynlastread[i].msg_num = last_msg;
            }
            lastread = usr.dynlastread[i].msg_num;
        }
        else if (upd) {
            lastread = 0;
            for (i = 1; i < MAXDLREAD; i++) {
                usr.dynlastread[i - 1].area = usr.dynlastread[i].area;
                usr.dynlastread[i - 1].msg_num = usr.dynlastread[i].msg_num;
            }

            usr.dynlastread[i - 1].area = area;
            usr.dynlastread[i - 1].msg_num = 0;
        }
        else {
            lastread = 0;
        }
    }

    if (lastread < 0) {
        lastread = 0;
    }
}

int full_pip_msg_read(area, msgn, mark, fakenum)
unsigned int area, msgn;
char mark;
int fakenum;
{
    FILE * f1, *f2;
    MSGPTR hdr;
    MSGPKTHDR mpkt;
    char buff[150], wrp[150], *p, c, fn[80], okludge;
    byte a;
    int line, i, colf, shead, m, z;
    long startpos;
    struct _msg msgt;

    msgn--;

    sprintf(fn, "%sMPTR%04x.PIP", pip_msgpath, area);
    f1 = fopen(fn, "r+b");

    if (f1 == NULL) {
        return (0);
    }

    if (fseek(f1, sizeof(hdr)*msgn, SEEK_SET)) {
        fclose(f1);
        return (0);
    }
    if (fread(&hdr, sizeof(hdr), 1, f1) == 0) {
        fclose(f1);
        return (0);
    }
    if (hdr.status & SET_MPTR_DEL) {
        fclose(f1);
        return (0);
    }

    sprintf(fn, "%sMPKT%04x.PIP", pip_msgpath, area);
    f2 = fopen(fn, "r+b");
    if (f2 == NULL) {
        fclose(f1);
        return (0);
    }

    if (!(mark & SET_NEW) || !((hdr.status & SET_MPTR_RCVD) || (hdr.status & SET_MPTR_DEL))) {
        fseek(f2, hdr.pos, SEEK_SET);
        fread(&mpkt, sizeof(mpkt), 1, f2);

        okludge = 0;
        line = 2;
        shead = 0;
        colf = 0;
        memset((char *)&msgt, 0, sizeof(struct _msg));
        msg_fzone = msg_tzone = config->alias[sys.use_alias].zone;
        msg_fpoint = msg_tpoint = 0;

        read0(msgt.date, f2);
        read0(msgt.to, f2);
        read0(msgt.from, f2);
        read0(msgt.subj, f2);

        msgt.orig_net = mpkt.fromnet;
        msgt.orig = mpkt.fromnode;
        if (!(mpkt.attribute & SET_PKT_FROMUS)) {
            msg_fpoint = mpkt.point;
        }

        if (mpkt.attribute & SET_PKT_PRIV) {
            msgt.attr |= MSGPRIVATE;
        }
        if (hdr.status & SET_MPTR_RCVD) {
            msgt.attr |= MSGREAD;
        }

        if (msgt.attr & MSGPRIVATE) {
            if (stricmp(msgt.from, usr.name) && stricmp(msgt.to, usr.name) && stricmp(msgt.from, usr.handle) && stricmp(msgt.to, usr.handle) && usr.priv != SYSOP) {
                fclose(f1);
                fclose(f2);
                return (0);
            }
        }
        if (!stricmp(msgt.to, usr.name) || !stricmp(msgt.to, usr.handle)) {
            mark |= SET_MARK;
        }

        msgt.dest_net = mpkt.tonet;
        msgt.dest = mpkt.tonode;
        if ((mpkt.attribute & SET_PKT_FROMUS)) {
            msg_tpoint = mpkt.point;
        }
        memcpy((char *)&msg, (char *)&msgt, sizeof(struct _msg));

        msg_parent = (int)hdr.prev + 1;
        msg_child = (int)hdr.next + 1;

        cls();
        change_attr(BLUE | _LGREY);
        del_line();
        m_print(" * %s\n", sys.msg_name);

        allow_reply = 1;
        i = 0;
        startpos = ftell(f2);

        while ((a = (byte)fgetc(f2)) != 0 && !feof(f2)) {
            c = a;

            switch (mpkt.pktype) {
                case 2:
                    c = a;
                    break;
                case 10:
                    if (a != 10) {
                        if (a == 141) {
                            a = '\r';
                        }

                        if (a < 128) {
                            c = a;
                        }
                        else {
                            if (a == 128) {
                                a = (byte)fgetc(f2);
                                c = (a) + 127;
                            }
                            else {
                                buff[i] = '\0';
                                strcat(buff, pip[a - 128]);
                                i += strlen(pip[a - 128]);
                                c = buff[--i];
                            }
                        }
                    }
                    else {
                        c = '\0';
                    }
                    break;
                default:
                    return (1);
            }

            if (c == '\0') {
                continue;
            }
            buff[i++] = c;

            if (c == 0x0D) {
                buff[i - 1] = '\0';

                if (buff[0] == 0x01 && !okludge) {
                    if (!strncmp(&buff[1], "INTL", 4) && !shead) {
                        sscanf(&buff[6], "%d:%d/%d %d:%d/%d", &msg_tzone, &i, &i, &msg_fzone, &i, &i);
                    }
                    if (!strncmp(&buff[1], "TOPT", 4) && !shead) {
                        sscanf(&buff[6], "%d", &msg_tpoint);
                    }
                    if (!strncmp(&buff[1], "FMPT", 4) && !shead) {
                        sscanf(&buff[6], "%d", &msg_fpoint);
                    }
                    i = 0;
                    continue;
                }
                else if (!shead) {
                    line = msg_attrib(&msgt, fakenum ? fakenum : msgn + 1, line, 0);
                    change_attr(LGREY | _BLUE);
                    del_line();
                    change_attr(CYAN | _BLACK);
                    m_print(bbstxt[B_ONE_CR]);
                    shead = 1;
                    line++;
                    okludge = 1;
                    fseek(f2, startpos, SEEK_SET);
                    i = 0;
                    continue;
                }

                if (!usr.kludge && (!strncmp(buff, "SEEN-BY", 7) || buff[0] == 0x01)) {
                    i = 0;
                    continue;
                }

                if (buff[0] == 0x01 || !strncmp(buff, "SEEN-BY", 7)) {
                    m_print("\026\001\013%s\n", &buff[1]);
                    change_attr(LGREY | BLACK);
                }
                else if (!strncmp(buff, "SEEN-BY", 7)) {
                    m_print("\026\001\013%s\n", buff);
                    change_attr(LGREY | BLACK);
                }
                else {
                    p = &buff[0];

                    if (((strchr(buff, '>') - p) < 6) && (strchr(buff, '>'))) {
                        if (!colf) {
                            change_attr(LGREY | BLACK);
                            colf = 1;
                        }
                    }
                    else {
                        if (colf) {
                            change_attr(CYAN | BLACK);
                            colf = 0;
                        }
                    }

                    if (!strncmp(buff, msgtxt[M_TEAR_LINE], 4) || !strncmp(buff, msgtxt[M_ORIGIN_LINE], 11)) {
                        m_print("\026\001\002%s\026\001\007\n", buff);
                    }
                    else {
                        m_print("%s\n", buff);
                    }

                    if (!strncmp(buff, msgtxt[M_ORIGIN_LINE], 11)) {
                        gather_origin_netnode(buff);
                    }
                }

                i = 0;

                if (!(++line < (usr.len - 1)) && usr.more) {
                    if (!continua()) {
                        line = 1;
                        break;
                    }
                    else {
                        while (line > 5) {
                            cpos(line--, 1);
                            del_line();
                        }
                    }
                }
            }
            else {
                if (i < (usr.width - 1)) {
                    continue;
                }

                buff[i] = '\0';
                while (i > 0 && buff[i] != ' ') {
                    i--;
                }

                m = 0;

                if (i != 0)
                    for (z = i + 1; buff[z]; z++) {
                        wrp[m++] = buff[z];
                    }

                buff[i] = '\0';
                wrp[m] = '\0';

                if (!shead) {
                    line = msg_attrib(&msgt, fakenum ? fakenum : msgn + 1, line, 0);
                    change_attr(LGREY | _BLUE);
                    del_line();
                    change_attr(CYAN | _BLACK);
                    m_print(bbstxt[B_ONE_CR]);
                    shead = 1;
                    line++;
                    okludge = 1;
                    fseek(f2, startpos, SEEK_SET);
                    i = 0;
                    continue;
                }

                if (((strchr(buff, '>') - buff) < 6) && (strchr(buff, '>')))
                {
                    if (!colf)
                    {
                        change_attr(LGREY | _BLACK);
                        colf = 1;
                    }
                }
                else
                {
                    if (colf)
                    {
                        change_attr(CYAN | _BLACK);
                        colf = 0;
                    }
                }

                if (!strncmp(buff, msgtxt[M_TEAR_LINE], 4) || !strncmp(buff, msgtxt[M_ORIGIN_LINE], 11)) {
                    m_print("\026\001\002%s\026\001\007\n", buff);
                }
                else {
                    m_print("%s\n", buff);
                }

                if (!strncmp(buff, msgtxt[M_ORIGIN_LINE], 11)) {
                    gather_origin_netnode(buff);
                }

                if (!(++line < (usr.len - 1)) && usr.more)
                {
                    if (!continua())
                    {
                        line = 1;
                        break;
                    }
                    else
                    {
                        while (line > 5) {
                            cpos(line--, 1);
                            del_line();
                        }
                    }
                }

                strcpy(buff, wrp);
                i = strlen(wrp);
            }
        }

        if (mark & (SET_MARK | SET_DEL)) {
            hdr.status |= (mark & SET_DEL) ? SET_MPTR_DEL : SET_MPTR_RCVD; /* bit received/deleted set to on*/
            fseek(f1, sizeof(hdr)*msgn, SEEK_SET);
            if (fwrite(&hdr, sizeof(hdr), 1, f1) == 0) {
                return (1);
            }
            mpkt.attribute |= (mark & SET_MARK) ? SET_PKT_RCVD : 0;
            fseek(f2, hdr.pos, SEEK_SET);
            fwrite(&mpkt, sizeof(mpkt), 1, f2);
        }
    }

    fclose(f1);
    fclose(f2);

    if (!shead) {
        line = msg_attrib(&msgt, msgn + 1, line, 0);
        change_attr(LGREY | _BLUE);
        del_line();
        change_attr(CYAN | _BLACK);
        m_print(bbstxt[B_ONE_CR]);
        shead = 1;
        line++;
    }

    if (line) {
        press_enter();
    }

    return (1);
}

int pip_msg_read(area, msgn, mark, flag, fakenum)
unsigned int area, msgn;
char mark;
int flag, fakenum;
{
    FILE * f1, *f2;
    MSGPTR hdr;
    MSGPKTHDR mpkt;
    char buff[150], wrp[150], *p, c, fn[80], okludge;
    byte a;
    int line, i, colf, shead, m, z;
    long startpos;
    struct _msg msgt;

    if (usr.full_read && !flag) {
        return (full_pip_msg_read(area, msgn, mark, fakenum));
    }

    msgn--;

    sprintf(fn, "%sMPTR%04x.PIP", pip_msgpath, area);
    f1 = fopen(fn, "rb+");

    if (f1 == NULL) {
        return (0);
    }

    if (fseek(f1, sizeof(hdr)*msgn, SEEK_SET)) {
        fclose(f1);
        return (0);
    }
    if (fread(&hdr, sizeof(hdr), 1, f1) == 0) {
        fclose(f1);
        return (0);
    }
    if (hdr.status & SET_MPTR_DEL) {
        fclose(f1);
        return (0);
    }

    sprintf(fn, "%sMPKT%04x.PIP", pip_msgpath, area);
    f2 = fopen(fn, "rb+");
    if (f2 == NULL) {
        fclose(f1);
        return (0);
    }

    if (!(mark & SET_NEW) || !((hdr.status & SET_MPTR_RCVD) || (hdr.status & SET_MPTR_DEL))) {
        fseek(f2, hdr.pos, SEEK_SET);
        fread(&mpkt, sizeof(mpkt), 1, f2);

        okludge = 0;
        line = 2;
        shead = 0;
        colf = 0;
        memset((char *)&msgt, 0, sizeof(struct _msg));
        msg_fzone = msg_tzone = config->alias[sys.use_alias].zone;
        msg_fpoint = msg_tpoint = 0;

        read0(msgt.date, f2);
        read0(msgt.to, f2);
        read0(msgt.from, f2);
        read0(msgt.subj, f2);

        msgt.orig_net = mpkt.fromnet;
        msgt.orig = mpkt.fromnode;
        if (!(mpkt.attribute & SET_PKT_FROMUS)) {
            msg_fpoint = mpkt.point;
        }

        if (mpkt.attribute & SET_PKT_PRIV) {
            msgt.attr |= MSGPRIVATE;
        }
        if (mpkt.attribute & SET_PKT_RCVD) {
            msgt.attr |= MSGREAD;
        }

        if (msgt.attr & MSGPRIVATE) {
            if (stricmp(msgt.from, usr.name) && stricmp(msgt.to, usr.name) && stricmp(msgt.from, usr.handle) && stricmp(msgt.to, usr.handle) && usr.priv != SYSOP) {
                fclose(f1);
                fclose(f2);
                return (0);
            }
        }
        if (!stricmp(msgt.to, usr.name) || !stricmp(msgt.to, usr.handle)) {
            mark |= SET_MARK;
        }

        msg_parent = (int)hdr.prev + 1;
        msg_child = (int)hdr.next + 1;

        if (!flag) {
            cls();
        }

        msgt.dest_net = mpkt.tonet;
        msgt.dest = mpkt.tonode;
        if ((mpkt.attribute & SET_PKT_FROMUS)) {
            msg_tpoint = mpkt.point;
        }
        memcpy((char *)&msg, (char *)&msgt, sizeof(struct _msg));

        change_attr(WHITE | _BLACK);
        m_print(" * %s\n", sys.msg_name);

        change_attr(CYAN | _BLACK);
        allow_reply = 1;
        i = 0;
        startpos = ftell(f2);

        while ((a = (byte)fgetc(f2)) != 0 && !feof(f2)) {
            c = a;

            switch (mpkt.pktype) {
                case 2:
                    c = a;
                    break;
                case 10:
                    if (a != 10) {
                        if (a == 141) {
                            a = '\r';
                        }

                        if (a < 128) {
                            c = a;
                        }
                        else {
                            if (a == 128) {
                                a = (byte)fgetc(f2);
                                c = (a) + 127;
                            }
                            else
                            {
                                buff[i] = '\0';
                                strcat(buff, pip[a - 128]);
                                i += strlen(pip[a - 128]);
                                c = buff[--i];
                            }
                        }
                    }
                    else {
                        c = '\0';
                    }
                    break;
                default:
                    return (1);
            }

            if (c == '\0') {
                continue;
            }
            buff[i++] = c;

            if (c == 0x0D) {
                buff[i - 1] = '\0';

                if (buff[0] == 0x01 && !okludge) {
                    if (!strncmp(&buff[1], "INTL", 4) && !shead) {
                        sscanf(&buff[6], "%d:%d/%d %d:%d/%d", &msg_tzone, &i, &i, &msg_fzone, &i, &i);
                    }
                    if (!strncmp(&buff[1], "TOPT", 4) && !shead) {
                        sscanf(&buff[6], "%d", &msg_tpoint);
                    }
                    if (!strncmp(&buff[1], "FMPT", 4) && !shead) {
                        sscanf(&buff[6], "%d", &msg_fpoint);
                    }
                    i = 0;
                    continue;
                }
                else if (!shead) {
                    line = msg_attrib(&msgt, fakenum ? fakenum : msgn + 1, line, 0);
                    m_print(bbstxt[B_ONE_CR]);
                    shead = 1;
                    okludge = 1;
                    fseek(f2, startpos, SEEK_SET);
                    i = 0;
                    continue;
                }

                if (!usr.kludge && (!strncmp(buff, "SEEN-BY", 7) || buff[0] == 0x01)) {
                    i = 0;
                    continue;
                }

                if (buff[0] == 0x01) {
                    m_print("\026\001\013%s\n", &buff[1]);
                    change_attr(LGREY | BLACK);
                }
                else if (!strncmp(buff, "SEEN-BY", 7)) {
                    m_print("\026\001\013%s\n", buff);
                    change_attr(LGREY | BLACK);
                }
                else {
                    p = &buff[0];

                    if (((strchr(buff, '>') - p) < 6) && (strchr(buff, '>'))) {
                        if (!colf) {
                            change_attr(LGREY | BLACK);
                            colf = 1;
                        }
                    }
                    else {
                        if (colf) {
                            change_attr(CYAN | BLACK);
                            colf = 0;
                        }
                    }

                    if (!strncmp(buff, msgtxt[M_TEAR_LINE], 4) || !strncmp(buff, msgtxt[M_ORIGIN_LINE], 11)) {
                        m_print("\026\001\002%s\026\001\007\n", buff);
                    }
                    else {
                        m_print("%s\n", buff);
                    }

                    if (!strncmp(buff, msgtxt[M_ORIGIN_LINE], 11)) {
                        gather_origin_netnode(buff);
                    }
                }

                i = 0;

                if (flag == 1) {
                    line = 1;
                }

                if (!(++line < (usr.len - 1)) && usr.more) {
                    line = 1;
                    if (!continua()) {
                        flag = 1;
                        break;
                    }
                }
            }
            else {
                if (i < (usr.width - 1)) {
                    continue;
                }

                buff[i] = '\0';
                while (i > 0 && buff[i] != ' ') {
                    i--;
                }

                m = 0;

                if (i != 0)
                    for (z = i + 1; buff[z]; z++) {
                        wrp[m++] = buff[z];
                    }

                buff[i] = '\0';
                wrp[m] = '\0';

                if (!shead) {
                    line = msg_attrib(&msgt, fakenum ? fakenum : msgn + 1, line, 0);
                    m_print(bbstxt[B_ONE_CR]);
                    shead = 1;
                    okludge = 1;
                    fseek(f2, startpos, SEEK_SET);
                    i = 0;
                    continue;
                }

                if (((strchr(buff, '>') - buff) < 6) && (strchr(buff, '>'))) {
                    if (!colf) {
                        change_attr(LGREY | _BLACK);
                        colf = 1;
                    }
                }
                else {
                    if (colf) {
                        change_attr(CYAN | _BLACK);
                        colf = 0;
                    }
                }

                if (!strncmp(buff, msgtxt[M_TEAR_LINE], 4) || !strncmp(buff, msgtxt[M_ORIGIN_LINE], 11)) {
                    m_print("\026\001\002%s\026\001\007\n", buff);
                }
                else {
                    m_print("%s\n", buff);
                }

                if (!strncmp(buff, msgtxt[M_ORIGIN_LINE], 11)) {
                    gather_origin_netnode(buff);
                }

                if (flag == 1) {
                    line = 1;
                }

                if (!(++line < (usr.len - 1)) && usr.more) {
                    line = 1;
                    if (!continua()) {
                        flag = 1;
                        break;
                    }
                }

                strcpy(buff, wrp);
                i = strlen(wrp);
            }
        }

        if (mark & (SET_MARK | SET_DEL)) {
            hdr.status |= (mark & SET_DEL) ? SET_MPTR_DEL : SET_MPTR_RCVD; /* bit received/deleted set to on*/
            fseek(f1, sizeof(hdr)*msgn, SEEK_SET);
            if (fwrite(&hdr, sizeof(hdr), 1, f1) == 0) {
                return (1);
            }
            mpkt.attribute |= (mark & SET_MARK) ? SET_PKT_RCVD : 0;
            fseek(f2, hdr.pos, SEEK_SET);
            fwrite(&mpkt, sizeof(mpkt), 1, f2);
        }
    }

    fclose(f1);
    fclose(f2);

    if (!shead) {
        line = msg_attrib(&msgt, msgn + 1, line, 0);
        change_attr(LGREY | _BLUE);
        del_line();
        change_attr(CYAN | _BLACK);
        m_print(bbstxt[B_ONE_CR]);
        shead = 1;
        line++;
    }

    if (line && !flag) {
        press_enter();
    }

    return (1);
}

void pip_save_message(txt)
char * txt;
{
    FILE * f1, *f2;
    int i, dest;
    char fn[128];
    MSGPTR hdr;
    MSGPKTHDR mpkt;
    DESTPTR pt;
    unsigned long crc;

    m_print(bbstxt[B_SAVE_MESSAGE]);
    pip_scan_message_base(sys.pip_board, 0);
    dest = last_msg + 1;
    activation_key();
    m_print(" #%d ...", dest);

    sprintf(fn, "%sMPTR%04x.PIP", pip_msgpath, sys.pip_board);
    f1 = fopen(fn, "rb+");
    if (f1 == NULL) {
        f1 = fopen(fn, "wb");
    }
    sprintf(fn, "%sMPKT%04x.PIP", pip_msgpath, sys.pip_board);
    f2 = fopen(fn, "rb+");
    if (f2 == NULL) {
        f2 = fopen(fn, "wb");
    }

    mpkt.fromnode = config->alias[sys.use_alias].node;
    mpkt.fromnet = config->alias[sys.use_alias].net;
    mpkt.pktype = 10;
    mpkt.attribute = SET_PKT_FROMUS;
    mpkt.tonode = msg.dest;
    mpkt.tonet = msg.dest_net;
    mpkt.point = msg_tpoint;

    fseek(f2, filelength(fileno(f2)) - 2, SEEK_SET);
    hdr.pos = ftell(f2);
    hdr.next = hdr.prev = 0;
    hdr.status = SET_MPTR_FROMUS; /* from us */
    fseek(f1, filelength(fileno(f1)), SEEK_SET);
    pt.area = sys.pip_board;
    pt.msg = (int)(ftell(f1) / sizeof(hdr));
    strcpy(pt.to, msg.to);
    pt.next = 0;

    if (msg.attr & MSGPRIVATE) {
        mpkt.attribute |= SET_PKT_PRIV;
    }

    fwrite(&hdr, sizeof hdr, 1, f1);
    fwrite(&mpkt, sizeof mpkt, 1, f2);
    write0(msg.date, f2);
    write0(msg.to, f2);
    write0(msg.from, f2);
    write0(msg.subj, f2);

    if (sys.netmail) {
        if (msg_tpoint) {
            sprintf(fn, msgtxt[M_TOPT], msg_tpoint);
            pipstring(fn, f2);
        }
        if (msg_tzone != msg_fzone) {
            sprintf(fn, msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig);
            pipstring(fn, f2);
        }
    }

    if (sys.echomail) {
        sprintf(fn, msgtxt[M_PID], VERSION, registered ? "+" : NOREG);
        pipstring(fn, f2);
        crc = time(NULL);
        crc = string_crc(msg.from, crc);
        crc = string_crc(msg.to, crc);
        crc = string_crc(msg.subj, crc);
        crc = string_crc(msg.date, crc);
        sprintf(fn, msgtxt[M_MSGID], config->alias[sys.use_alias].zone, config->alias[sys.use_alias].net, config->alias[sys.use_alias].node, config->alias[sys.use_alias].point, crc);
        pipstring(fn, f2);
    }

    if (sys.internet_mail && internet_to != NULL) {
        sprintf(fn, "To: %s\r\n\r\n", internet_to);
        pipstring(fn, f2);
        free(internet_to);
    }

    if (txt == NULL) {
        i = 0;
        while (messaggio[i] != NULL) {
            pipstring(messaggio[i++], f2);
            if (messaggio[i][strlen(messaggio[i]) - 1] == '\r') {
                pipstring("\n", f2);
            }
        }
    }
    else {
        int fptxt, m;
        char buffer[2050];

        fptxt = shopen(txt, O_RDONLY | O_BINARY);
        if (fptxt != -1) {
            do {
                i = read(fptxt, buffer, 2048);
                buffer[i] = '\0';
                for (m = 0; m < i; m++) {
                    if (buffer[m] == 0x1A) {
                        buffer[m] = ' ';
                    }
                }
                pipstring(buffer, f2);
            } while (i == 2048);

            close(fptxt);
        }
    }

    pipstring("\r\n", f2);

    if (strlen(usr.signature) && registered) {
        sprintf(fn, msgtxt[M_SIGNATURE], usr.signature);
        pipstring(fn, f2);
    }

    if (sys.echomail) {
        sprintf(fn, msgtxt[M_TEAR_LINE], VERSION, registered ? "+" : NOREG);
        pipstring(fn, f2);
        if (strlen(sys.origin)) {
            sprintf(fn, msgtxt[M_ORIGIN_LINE], random_origins(), config->alias[sys.use_alias].zone, config->alias[sys.use_alias].net, config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
        }
        else {
            sprintf(fn, msgtxt[M_ORIGIN_LINE], system_name, config->alias[sys.use_alias].zone, config->alias[sys.use_alias].net, config->alias[sys.use_alias].node, config->alias[sys.use_alias].point);
        }
        pipstring(fn, f2);
    }

    fputc(0, f2);
    fputc(0, f2);
    fputc(0, f2);

    fclose(f1);
    fclose(f2);

    sprintf(fn, "%sDestPtr.Pip", pip_msgpath);
    f1 = fopen(fn, "ab+");
    fwrite(&pt, sizeof pt, 1, f1);
    fclose(f1);

    m_print(bbstxt[B_XPRT_DONE]);
    status_line(msgtxt[M_INSUFFICIENT_DATA], dest, sys.msg_num);
    last_msg = dest;
    num_msg++;
}

void pip_list_headers(m, pip_board, verbose)
int m, pip_board, verbose;
{
    FILE * f1, *f2;
    int i, line = verbose ? 2 : 5, l = 0;
    char fn[80];
    MSGPTR hdr;
    MSGPKTHDR mpkt;
    struct _msg msgt;

    sprintf(fn, "%sMPTR%04x.PIP", pip_msgpath, pip_board);
    f1 = fopen(fn, "rb+");
    sprintf(fn, "%sMPKT%04x.PIP", pip_msgpath, pip_board);
    f2 = fopen(fn, "rb+");

    for (i = m; i <= last_msg; i++) {
        if (fseek(f1, sizeof(hdr) * (i - 1), SEEK_SET)) {
            break;
        }
        if (fread(&hdr, sizeof(hdr), 1, f1) == 0) {
            break;
        }

        memset((char *)&msgt, 0, sizeof(struct _msg));
        msg_fzone = msg_tzone = config->alias[sys.use_alias].zone;
        msg_fpoint = msg_tpoint = 0;

        fseek(f2, hdr.pos, SEEK_SET);
        fread(&mpkt, sizeof(mpkt), 1, f2);

        read0(msgt.date, f2);
        read0(msgt.to, f2);
        read0(msgt.from, f2);
        read0(msgt.subj, f2);

        msgt.orig_net = mpkt.fromnet;
        msgt.orig = mpkt.fromnode;
        if (!(mpkt.attribute & SET_PKT_FROMUS)) {
            msg_fpoint = mpkt.point;
        }

        if (mpkt.attribute & SET_PKT_PRIV) {
            msgt.attr |= MSGPRIVATE;
        }

        msgt.dest_net = mpkt.tonet;
        msgt.dest = mpkt.tonode;
        if ((mpkt.attribute & SET_PKT_FROMUS)) {
            msg_tpoint = mpkt.point;
        }

        if (verbose) {
            if ((line = msg_attrib(&msgt, i, line, 0)) == 0) {
                break;
            }

            m_print(bbstxt[B_ONE_CR]);
        }
        else
            m_print("\026\001\003%-4d \026\001\020%c%-20.20s \026\001\020%c%-20.20s \026\001\013%-32.32s\n",
                    i,
                    stricmp(msgt.from, usr.name) ? '\212' : '\216',
                    msgt.from,
                    stricmp(msgt.to, usr.name) ? '\212' : '\214',
                    msgt.to,
                    msgt.subj);

        if (!(l = more_question(line)) || !CARRIER && !RECVD_BREAK()) {
            break;
        }

        if (!verbose && l < line) {
            l = 5;

            cls();
            m_print(bbstxt[B_LIST_AREAHEADER], usr.msg, sys.msg_name);
            m_print(bbstxt[B_LIST_HEADER1]);
            m_print(bbstxt[B_LIST_HEADER2]);
        }

        line = l;
    }

    fclose(f1);
    fclose(f2);

    if (line && CARRIER) {
        press_enter();
    }
}

