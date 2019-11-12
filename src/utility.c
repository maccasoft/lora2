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
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <dir.h>
#include <stdlib.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>
#include <cxl\cxlkey.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

extern int outinfo, old_event;
extern long exectime, elapsed;

#define PACK_ECHOMAIL  0x0001
#define PACK_NETMAIL   0x0002

void generate_tic_status(FILE * fp, int zo, int ne, int no, int po, int type);
void build_node_queue(int zone, int net, int node, int point, int i);
void pack_outbound(int);
void change_type(int, int, int, int, char, char);
void generate_echomail_status(FILE *, int, int, int, int, int);
int utility_add_echomail_link(char * area, int zo, int ne, int no, int po, FILE * fpr, int f_flags);
int utility_add_tic_link(char * area, int zo, int ne, int no, int po, FILE * fp);
void fido_save_message2(FILE *, char *);
int no_dups(int, int, int, int);
void get_bad_call(int, int, int, int, int);
void sort_call_list(void);
void rescan_areas(void);

static void display_time(int, int, int, long);

void write_call_queue(void)
{
    int fd;
    char filename[80];

    sprintf(filename, "%sQUEUE.DAT", config->sys_path);
    fd = sh_open(filename, SH_DENYRW, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    if (fd != -1) {
        write(fd, (char *)&max_call, sizeof(short));
        write(fd, (char *)&call_list, sizeof(struct _call_list) * max_call);
        close(fd);
    }
}

void make_immediate(int zone, int net, int node, int point)
{
    int fd;
    char fname[80];

    if (!point) {
        sprintf(fname, "%s%04x%04x.ILO", HoldAreaNameMungeCreate(zone), net, node);
        if ((fd = sh_open(fname, SH_DENYNONE, O_WRONLY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE)) != -1) {
            close(fd);
        }
    }
}

void delete_immediate(int zone, int net, int node, int point)
{
    char fname[80];

    if (!point) {
        sprintf(fname, "%s%04x%04x.ILO", HoldAreaNameMungeCreate(zone), net, node);
        unlink(fname);
    }
}

void keyboard_password(void)
{
    int i;
    char string[50], verify[50];

    wopen(11, 15, 13, 66, 0, LGREY | _BLACK, LCYAN | _BLACK);
    wtitle("LOCK KEYBOARD", TLEFT, LCYAN | _BLACK);
    wshadow(DGREY | _BLACK);

    wprints(0, 1, YELLOW | _BLACK, "Password:");
    winpbeg(BLACK | _LGREY, BLACK | _LGREY);
    winpdef(0, 11, string, "??????????????????????????????????????", 'P', 0, NULL, 0);
    i = winpread();
    strtrim(string);

    hidecur();
    wclose();

    if (i == W_ESCPRESS) {
        return;
    }

    wopen(11, 15, 13, 66, 0, LGREY | _BLACK, LCYAN | _BLACK);
    wtitle("LOCK KEYBOARD (Verification)", TLEFT, LCYAN | _BLACK);
    wshadow(DGREY | _BLACK);

    wprints(0, 1, YELLOW | _BLACK, "Password:");
    winpbeg(BLACK | _LGREY, BLACK | _LGREY);
    winpdef(0, 11, verify, "??????????????????????????????????????", 'P', 0, NULL, 0);
    i = winpread();
    strtrim(verify);

    hidecur();
    wclose();

    if (i == W_ESCPRESS || strcmp(string, verify)) {
        return;
    }

    memset(sysinfo.pwd, 0, 40);
    strcpy(sysinfo.pwd, strbtrim(string));
    write_sysinfo();

    if (sysinfo.pwd[0]) {
        password = sysinfo.pwd;
    }
    else {
        password = NULL;
    }

    if (password != NULL) {
        locked = 1;
    }

    if (function_active == 4) {
        f4_status();
    }
}

void file_attach()
{
    FILE * fp;
    int i, wh, zo, ne, no, po, nfiles, err;
    word m;
    char string[50], filename[80], *p, pri, selpri[4];
    char drive[MAXDRIVE], dir[MAXDIR], file[MAXFILE], ext[MAXEXT];
    long size;
    struct ffblk blk;

    wh = wopen(11, 15, 13, 65, 0, LGREY | _BLACK, LCYAN | _BLACK);
    wactiv(wh);
    wtitle("SEND FILES", TLEFT, LCYAN | _BLACK);
    wshadow(DGREY | _BLACK);

    wprints(0, 1, YELLOW | _BLACK, "Address:");
    winpbeg(BLACK | _LGREY, BLACK | _LGREY);
    winpdef(0, 10, string, "??????????????????????????????????????", 0, 0, NULL, 0);
    i = winpread();
    strtrim(string);

    hidecur();
    wclose();

    if (i == W_ESCPRESS || !string[0]) {
        return;
    }

    zo = config->alias[0].zone;
    ne = config->alias[0].net;
    no = 0;
    po = 0;

    parse_netnode(strbtrim(string), &zo, &ne, &no, &po);

    if (!get_bbs_record(zo, ne, no, po))
        if (!get_bbs_record(zo, ne, no, 0)) {
            wh = wopen(11, 20, 15, 58, 0, RED | _BLACK, LGREY | _BLACK);
            wactiv(wh);
            sprintf(string, "Node %d:%d/%d.%d not found!", zo, ne, no, po);
            wcenters(1, YELLOW | _BLACK, string);
            timer(20);
            wclose();
            return;
        }

    wh = wopen(9, 10, 18, 68, 0, RED | _LGREY, BLUE | _LGREY);
    wactiv(wh);
    wshadow(DGREY | _BLACK);
    wtitle("SEND FILES", TLEFT, RED | _LGREY);
    whline(2, 0, 58, 0, RED | _LGREY);
    wprints(0, 1, BLUE | _LGREY, nodelist.name);
    sprintf(string, "%u:%u/%u.%u", zo, ne, no, po);
    wprints(0, 56 - strlen(string), BLUE | _LGREY, string);
    wprints(1, 1, BLUE | _LGREY, nodelist.city);
    wprints(1, 56 - strlen(nodelist.sysop), BLUE | _LGREY, nodelist.sysop);

    for (i = 0; i < max_call; i++) {
        if (call_list[i].zone == zo && call_list[i].net == ne && call_list[i].node == no && call_list[i].point == po) {
            break;
        }
    }

    wprints(3, 1, WHITE | _LGREY, "File(s):");
    wprints(7, 1, WHITE | _LGREY, "Priority:   (Normal/Crash/Direct/Hold/Immediate)");

    string[0] = '\0';

reenter:
    winpbeg(BLUE | _CYAN, BLUE | _CYAN);
    winpdef(3, 10, string, "??????????????????????????????????????????????", 0, 1, NULL, 0);
    i = winpread();
    strtrim(string);

    if (i == W_ESCPRESS || !string[0]) {
        hidecur();
        wclose();
        return;
    }

    err = nfiles = 0;
    size = 0L;
    strcpy(filename, string);
    p = strtok(strbtrim(filename), " ");

    if (p != NULL)
        do {
            if (!findfirst(p, &blk, 0))
                do {
                    nfiles++;
                    size += blk.ff_fsize;
                } while (!findnext(&blk));
            else {
                err++;
            }
        } while ((p = strtok(NULL, " ")) != NULL);

    if (err) {
        wopen(11, 20, 15, 58, 0, RED | _BLACK, LGREY | _BLACK);
        wshadow(DGREY | _BLACK);
        wcenters(1, YELLOW | _BLACK, "File(s) not found!");
        timer(20);
        wclose();
        goto reenter;
    }

//   wfill (4, 0, 7, 56, ' ', LCYAN|_BLACK);

    m = nodelist.rate * 300;
    if (m > rate || m == 0) {
        m = rate;
    }

    i = (int)(size * 10 / m + 53);

    sprintf(filename, "Total of %d files, %ld bytes, %d:%02d at %d baud", nfiles, size, i / 60, i % 60, m);
    wcenters(5, BLACK | _LGREY, filename);

    strcpy(selpri, "I");
    do {
        winpbeg(BLUE | _CYAN, BLUE | _CYAN);
        winpdef(7, 11, selpri, "?", 0, 2, NULL, 0);
        i = winpread();
        selpri[0] = toupper(selpri[0]);
    } while (selpri[0] != 'I' && selpri[0] != 'N' && selpri[0] != 'C' && selpri[0] != 'D' && selpri[0] != 'H');

    hidecur();
    if (i == W_ESCPRESS) {
        wclose();
        return;
    }

    pri = toupper(selpri[0]);
    if (pri == 'N' || pri == 'I') {
        pri = 'F';
    }

    if (po && pri == 'H') {
        sprintf(filename, "%s%04X%04X.PNT", HoldAreaNameMungeCreate(zo), ne, no);
        mkdir(filename);
        sprintf(filename, "%s%04X%04X.PNT\\%08X.%cLO", HoldAreaNameMungeCreate(zo), ne, no, po, pri);
    }
    else {
        sprintf(filename, "%s%04X%04X.%cLO", HoldAreaNameMungeCreate(zo), ne, no, pri);
    }
    fp = fopen(filename, "at");

    p = strtok(strbtrim(string), " ");
    fnsplit(string, drive, dir, file, ext);

    if (fp != NULL && p != NULL)
        do {
            if (!findfirst(p, &blk, 0))
                do {
                    fprintf(fp, "%s%s%s\n", drive, dir, blk.ff_name);
                } while (!findnext(&blk));
        } while ((p = strtok(NULL, " ")) != NULL);

    fclose(fp);

    pri = toupper(selpri[0]);
    if (pri == 'N') {
        pri = 'F';
    }

    if (po && pri != 'H') {
        po = 0;
    }
    i = no_dups(zo, ne, no, po);
    if (max_call <= i) {
        memset(&call_list[i], 0, sizeof(struct _call_list));
        max_call = i + 1;
    }

    build_node_queue(zo, ne, no, po, i);
    sort_call_list();

    for (i = 0; i < max_call; i++) {
        if (call_list[i].zone == zo && call_list[i].net == ne && call_list[i].node == no && call_list[i].point == po) {
            break;
        }
    }

    if (toupper(selpri[0]) == 'I') {
        call_list[i].type |= MAIL_WILLGO;
        next_call = i;
    }

    wclose();

    write_call_queue();
    display_outbound_info(outinfo);
}

void manual_poll()
{
    FILE * fp;
    int i, wh, zo, ne, no, po;
    char string[50], selpri[4], pri, filename[80];

    wh = wopen(11, 15, 13, 65, 0, LGREY | _BLACK, LCYAN | _BLACK);
    wactiv(wh);
    wshadow(DGREY | _BLACK);
    wtitle("FORCED POLL", TLEFT, LCYAN | _BLACK);

    wprints(0, 1, YELLOW | _BLACK, "Address:");
    winpbeg(BLACK | _LGREY, BLACK | _LGREY);
    winpdef(0, 10, string, "??????????????????????????????????????", 0, 0, NULL, 0);
    i = winpread();
    strtrim(string);

    hidecur();
    wclose();

    if (i == W_ESCPRESS || !string[0]) {
        return;
    }

    zo = config->alias[0].zone;
    ne = config->alias[0].net;
    no = 0;
    po = 0;

    parse_netnode(strbtrim(string), &zo, &ne, &no, &po);

    if (po) {
        wh = wopen(11, 25, 15, 53, 0, RED | _BLACK, LGREY | _BLACK);
        wactiv(wh);
        wcenters(1, YELLOW | _BLACK, "No point-address allowed");
        timer(20);
        wclose();
        return;
    }

    if (!get_bbs_record(zo, ne, no, 0)) {
        wh = wopen(11, 25, 15, 53, 0, RED | _BLACK, LGREY | _BLACK);
        wactiv(wh);
        sprintf(string, "Node %d:%d/%d not found!", zo, ne, no);
        wcenters(1, YELLOW | _BLACK, string);
        timer(20);
        wclose();
        return;
    }

    wh = wopen(11, 10, 16, 68, 0, RED | _LGREY, BLUE | _LGREY);
    wactiv(wh);
    wshadow(DGREY | _BLACK);
    wtitle("FORCED POLL", TLEFT, RED | _LGREY);
    whline(2, 0, 58, 0, RED | _LGREY);
    wprints(0, 1, BLUE | _LGREY, nodelist.name);
    sprintf(string, "%u:%u/%u", zo, ne, no);
    wprints(0, 56 - strlen(string), BLUE | _LGREY, string);
    wprints(1, 1, BLUE | _LGREY, nodelist.city);
    wprints(1, 56 - strlen(nodelist.sysop), BLUE | _LGREY, nodelist.sysop);

    wprints(3, 1, WHITE | _LGREY, "Priority:   (Normal/Crash/Direct/Immediate)");
    strcpy(selpri, "I");
    do {
        winpbeg(BLUE | _CYAN, BLUE | _CYAN);
        winpdef(3, 11, selpri, "?", 0, 2, NULL, 0);
        i = winpread();
        selpri[0] = toupper(selpri[0]);
    } while (selpri[0] != 'I' && selpri[0] != 'N' && selpri[0] != 'C' && selpri[0] != 'D');

    hidecur();
    if (i == W_ESCPRESS) {
        wclose();
        return;
    }

    pri = toupper(selpri[0]);
    if (pri == 'N') {
        pri = 'F';
    }
    sprintf(filename, "%s%04X%04X.%cLO", HoldAreaNameMungeCreate(zo), ne, no, pri);
    fp = fopen(filename, "at");
    fclose(fp);

    i = no_dups(zo, ne, no, 0);
    if (max_call <= i) {
        memset(&call_list[i], 0, sizeof(struct _call_list));
        max_call = i + 1;
    }
//   call_list[i].zone = zo;
//   call_list[i].net  = ne;
//   call_list[i].node = no;
//   get_bad_call (zo, ne, no, 0, i);
    build_node_queue(zo, ne, no, 0, i);
    sort_call_list();

    for (i = 0; i < max_call; i++) {
        if (call_list[i].zone == zo && call_list[i].net == ne && call_list[i].node == no && call_list[i].point == 0) {
            break;
        }
    }

    if (toupper(selpri[0]) == 'I') {
        call_list[i].type |= MAIL_WILLGO;
        next_call = i;
    }

//   if (pri == 'C')
//      call_list[i].type |= MAIL_CRASH;
//   else if (pri == 'N')
//      call_list[i].type |= MAIL_NORMAL;
//   else if (pri == 'D')
//      call_list[i].type |= MAIL_DIRECT;

    wclose();

    write_call_queue();
    display_outbound_info(outinfo);
}

void file_request()
{
    FILE * fp;
    int i, wh, zo, ne, no, po;
    char string[50], filename[80], *p, *b, selpri[4], pri;

    wh = wopen(11, 15, 13, 65, 0, LGREY | _BLACK, LCYAN | _BLACK);
    wactiv(wh);
    wshadow(DGREY | _BLACK);
    wtitle("REQUEST FILES", TLEFT, LCYAN | _BLACK);

    wprints(0, 1, YELLOW | _BLACK, "Address:");
    winpbeg(BLACK | _LGREY, BLACK | _LGREY);
    winpdef(0, 10, string, "??????????????????????????????????????", 0, 0, NULL, 0);
    i = winpread();
    strtrim(string);

    hidecur();
    wclose();

    if (i == W_ESCPRESS || !string[0]) {
        return;
    }

    zo = config->alias[0].zone;
    ne = config->alias[0].net;
    no = 0;
    po = 0;

    parse_netnode(strbtrim(string), &zo, &ne, &no, &po);

    if (!get_bbs_record(zo, ne, no, 0)) {
        wh = wopen(11, 25, 15, 53, 0, RED | _BLACK, LGREY | _BLACK);
        wactiv(wh);
        sprintf(string, "Node %d:%d/%d not found!", zo, ne, no);
        wcenters(1, YELLOW | _BLACK, string);
        timer(20);
        wclose();
        return;
    }

    wh = wopen(10, 10, 16, 68, 0, RED | _LGREY, BLUE | _LGREY);
    wactiv(wh);
    wshadow(DGREY | _BLACK);
    wtitle("REQUEST FILES", TLEFT, RED | _LGREY);
    whline(2, 0, 58, 0, RED | _LGREY);
    wprints(0, 1, BLUE | _LGREY, nodelist.name);
    sprintf(string, "%u:%u/%u", zo, ne, no);
    wprints(0, 56 - strlen(string), BLUE | _LGREY, string);
    wprints(1, 1, BLUE | _LGREY, nodelist.city);
    wprints(1, 56 - strlen(nodelist.sysop), BLUE | _LGREY, nodelist.sysop);

    for (i = 0; i < max_call; i++) {
        if (call_list[i].zone == zo && call_list[i].net == ne && call_list[i].node == no && call_list[i].point == 0) {
            break;
        }
    }

    wprints(3, 1, WHITE | _LGREY, "File(s):");
    wprints(4, 1, WHITE | _LGREY, "Priority:   (Normal/Crash/Direct/Hold/Immediate)");

    winpbeg(BLUE | _CYAN, BLUE | _CYAN);
    winpdef(3, 10, string, "??????????????????????????????????????????????", 0, 0, NULL, 0);
    i = winpread();
    strtrim(string);

    if (i == W_ESCPRESS || !string[0]) {
        hidecur();
        wclose();
        return;
    }

    strcpy(selpri, "I");
    do {
        winpbeg(BLUE | _CYAN, BLUE | _CYAN);
        winpdef(4, 11, selpri, "?", 0, 2, NULL, 0);
        i = winpread();
        selpri[0] = toupper(selpri[0]);
    } while (selpri[0] != 'I' && selpri[0] != 'N' && selpri[0] != 'C' && selpri[0] != 'D' && selpri[0] != 'H');

    hidecur();
    if (i == W_ESCPRESS) {
        wclose();
        return;
    }

    pri = toupper(selpri[0]);
    if (pri == 'N') {
        pri = 'F';
    }
    sprintf(filename, "%s%04X%04X.%cLO", HoldAreaNameMungeCreate(zo), ne, no, pri);
    fp = fopen(filename, "at");
    fclose(fp);

    sprintf(filename, "%s%04X%04X.REQ", HoldAreaNameMungeCreate(zo), ne, no);
    fp = fopen(filename, "at");

    p = strtok(strbtrim(string), " ");
    if (p != NULL)
        do {
            b = strtok(NULL, " ");
            if (b != NULL && *b == '!') {
                fprintf(fp, "%s %s\n", p, b);
                p = strtok(NULL, " ");
            }
            else {
                fprintf(fp, "%s\n", p);
                p = b;
            }
        } while (p != NULL);

    fclose(fp);

    i = no_dups(zo, ne, no, 0);
    if (max_call <= i) {
        memset(&call_list[i], 0, sizeof(struct _call_list));
        max_call = i + 1;
    }

    build_node_queue(zo, ne, no, 0, i);
    sort_call_list();

    for (i = 0; i < max_call; i++) {
        if (call_list[i].zone == zo && call_list[i].net == ne && call_list[i].node == no && call_list[i].point == 0) {
            break;
        }
    }

    if (toupper(selpri[0]) == 'I') {
        call_list[i].type |= MAIL_WILLGO;
        next_call = i;
    }

    wclose();

    write_call_queue();
    display_outbound_info(outinfo);
}

void activity_chart()
{
    int wh;
    char string[70];
    long t;

    wh = wopen(0, 0, 24, 79, 0, LGREY | _BLACK, LGREY | _BLACK);
    wactiv(wh);
    wtitle("ACTIVITY CHART", TLEFT, LCYAN | _BLACK);

    wprints(0, 23, LCYAN | _BLACK, "Today  Yesterday   This week   This month    This year");

    wprints(2, 1, LCYAN | _BLACK, "Calls received");
    wprints(3, 1, LCYAN | _BLACK, "EMAIL calls");
    wprints(4, 1, LCYAN | _BLACK, "BBS calls");
//   wprints (2, 1, LCYAN|_BLACK, "GATEWAY calls");
//   wprints (5, 1, LCYAN|_BLACK, "Aborted calls");

    wprints(6, 1, LCYAN | _BLACK, "Calls placed");
    wprints(7, 1, LCYAN | _BLACK, "Connections made");
    wprints(8, 1, LCYAN | _BLACK, "Failed connects");
    wprints(9, 1, LCYAN | _BLACK, "Total cost");

    wprints(11, 1, LCYAN | _BLACK, "File sent");
    wprints(12, 1, LCYAN | _BLACK, "Data sent (kb)");
    wprints(13, 1, LCYAN | _BLACK, "File received");
    wprints(14, 1, LCYAN | _BLACK, "Data received (kb)");
    wprints(15, 1, LCYAN | _BLACK, "File requests");

    wprints(17, 1, LCYAN | _BLACK, "EMAIL messages rcvd");
    wprints(18, 1, LCYAN | _BLACK, "ECHOmail messages");
    wprints(19, 1, LCYAN | _BLACK, "Duplicate messages");
    wprints(20, 1, LCYAN | _BLACK, "Bad messages");
    wprints(21, 1, LCYAN | _BLACK, "EMAIL sent");
    wprints(22, 1, LCYAN | _BLACK, "ECHOmail forwarded");

    sprintf(string, "%ld", sysinfo.today.incalls + sysinfo.today.humancalls);
    wrjusts(2, 27, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.yesterday.incalls + sysinfo.yesterday.humancalls);
    wrjusts(2, 38, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.week.incalls + sysinfo.week.humancalls);
    wrjusts(2, 50, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.month.incalls + sysinfo.month.humancalls);
    wrjusts(2, 63, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.year.incalls + sysinfo.year.humancalls);
    wrjusts(2, 76, YELLOW | _BLACK, string);

    sprintf(string, "%ld", sysinfo.today.incalls);
    wrjusts(3, 27, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.yesterday.incalls);
    wrjusts(3, 38, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.week.incalls);
    wrjusts(3, 50, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.month.incalls);
    wrjusts(3, 63, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.year.incalls);
    wrjusts(3, 76, YELLOW | _BLACK, string);

    sprintf(string, "%ld", sysinfo.today.humancalls);
    wrjusts(4, 27, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.yesterday.humancalls);
    wrjusts(4, 38, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.week.humancalls);
    wrjusts(4, 50, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.month.humancalls);
    wrjusts(4, 63, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.year.humancalls);
    wrjusts(4, 76, YELLOW | _BLACK, string);

    sprintf(string, "%ld", sysinfo.today.outcalls);
    wrjusts(6, 27, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.yesterday.outcalls);
    wrjusts(6, 38, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.week.outcalls);
    wrjusts(6, 50, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.month.outcalls);
    wrjusts(6, 63, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.year.outcalls);
    wrjusts(6, 76, YELLOW | _BLACK, string);

    sprintf(string, "%ld", sysinfo.today.completed);
    wrjusts(7, 27, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.yesterday.completed);
    wrjusts(7, 38, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.week.completed);
    wrjusts(7, 50, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.month.completed);
    wrjusts(7, 63, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.year.completed);
    wrjusts(7, 76, YELLOW | _BLACK, string);

    sprintf(string, "%ld", sysinfo.today.failed);
    wrjusts(8, 27, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.yesterday.failed);
    wrjusts(8, 38, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.week.failed);
    wrjusts(8, 50, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.month.failed);
    wrjusts(8, 63, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.year.failed);
    wrjusts(8, 76, YELLOW | _BLACK, string);

    sprintf(string, "$%ld.%02ld", sysinfo.today.cost / 100L, sysinfo.today.cost % 100L);
    wrjusts(9, 27, YELLOW | _BLACK, string);
    sprintf(string, "$%ld.%02ld", sysinfo.yesterday.cost / 100L, sysinfo.yesterday.cost % 100L);
    wrjusts(9, 38, YELLOW | _BLACK, string);
    sprintf(string, "$%ld.%02ld", sysinfo.week.cost / 100L, sysinfo.week.cost % 100L);
    wrjusts(9, 50, YELLOW | _BLACK, string);
    sprintf(string, "$%ld.%02ld", sysinfo.month.cost / 100L, sysinfo.month.cost % 100L);
    wrjusts(9, 63, YELLOW | _BLACK, string);
    sprintf(string, "$%ld.%02ld", sysinfo.year.cost / 100L, sysinfo.year.cost % 100L);
    wrjusts(9, 76, YELLOW | _BLACK, string);

    sprintf(string, "%ld", sysinfo.today.filesent);
    wrjusts(11, 27, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.yesterday.filesent);
    wrjusts(11, 38, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.week.filesent);
    wrjusts(11, 50, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.month.filesent);
    wrjusts(11, 63, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.year.filesent);
    wrjusts(11, 76, YELLOW | _BLACK, string);

    sprintf(string, "%ld", sysinfo.today.bytesent / 1024L);
    wrjusts(12, 27, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.yesterday.bytesent / 1024L);
    wrjusts(12, 38, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.week.bytesent / 1024L);
    wrjusts(12, 50, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.month.bytesent / 1024L);
    wrjusts(12, 63, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.year.bytesent / 1024L);
    wrjusts(12, 76, YELLOW | _BLACK, string);

    sprintf(string, "%ld", sysinfo.today.filereceived);
    wrjusts(13, 27, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.yesterday.filereceived);
    wrjusts(13, 38, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.week.filereceived);
    wrjusts(13, 50, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.month.filereceived);
    wrjusts(13, 63, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.year.filereceived);
    wrjusts(13, 76, YELLOW | _BLACK, string);

    sprintf(string, "%ld", sysinfo.today.bytereceived / 1024L);
    wrjusts(14, 27, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.yesterday.bytereceived / 1024L);
    wrjusts(14, 38, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.week.bytereceived / 1024L);
    wrjusts(14, 50, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.month.bytereceived / 1024L);
    wrjusts(14, 63, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.year.bytereceived / 1024L);
    wrjusts(14, 76, YELLOW | _BLACK, string);

    sprintf(string, "%ld", sysinfo.today.filerequest);
    wrjusts(15, 27, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.yesterday.filerequest);
    wrjusts(15, 38, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.week.filerequest);
    wrjusts(15, 50, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.month.filerequest);
    wrjusts(15, 63, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.year.filerequest);
    wrjusts(15, 76, YELLOW | _BLACK, string);

    sprintf(string, "%ld", sysinfo.today.emailreceived);
    wrjusts(17, 27, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.yesterday.emailreceived);
    wrjusts(17, 38, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.week.emailreceived);
    wrjusts(17, 50, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.month.emailreceived);
    wrjusts(17, 63, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.year.emailreceived);
    wrjusts(17, 76, YELLOW | _BLACK, string);

    sprintf(string, "%ld", sysinfo.today.echoreceived);
    wrjusts(18, 27, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.yesterday.echoreceived);
    wrjusts(18, 38, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.week.echoreceived);
    wrjusts(18, 50, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.month.echoreceived);
    wrjusts(18, 63, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.year.echoreceived);
    wrjusts(18, 76, YELLOW | _BLACK, string);

    sprintf(string, "%ld", sysinfo.today.dupes);
    wrjusts(19, 27, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.yesterday.dupes);
    wrjusts(19, 38, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.week.dupes);
    wrjusts(19, 50, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.month.dupes);
    wrjusts(19, 63, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.year.dupes);
    wrjusts(19, 76, YELLOW | _BLACK, string);

    sprintf(string, "%ld", sysinfo.today.bad);
    wrjusts(20, 27, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.yesterday.bad);
    wrjusts(20, 38, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.week.bad);
    wrjusts(20, 50, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.month.bad);
    wrjusts(20, 63, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.year.bad);
    wrjusts(20, 76, YELLOW | _BLACK, string);

    sprintf(string, "%ld", sysinfo.today.emailsent);
    wrjusts(21, 27, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.yesterday.emailsent);
    wrjusts(21, 38, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.week.emailsent);
    wrjusts(21, 50, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.month.emailsent);
    wrjusts(21, 63, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.year.emailsent);
    wrjusts(21, 76, YELLOW | _BLACK, string);

    sprintf(string, "%ld", sysinfo.today.echosent);
    wrjusts(22, 27, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.yesterday.echosent);
    wrjusts(22, 38, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.week.echosent);
    wrjusts(22, 50, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.month.echosent);
    wrjusts(22, 63, YELLOW | _BLACK, string);
    sprintf(string, "%ld", sysinfo.year.echosent);
    wrjusts(22, 76, YELLOW | _BLACK, string);

    setkbloop(NULL);
    t = timerset(6000);
    while (!kbmhit() && !timeup(t)) {
        release_timeslice();
    }

    if (!timeup(t)) {
        getxch();
    }

    wclose();
}

void time_usage()
{
    int wh;
    long t;

    wh = wopen(11, 0, 24, 79, 0, LGREY | _BLACK, LGREY | _BLACK);
    wactiv(wh);
    wtitle("TIME USAGE", TLEFT, LCYAN | _BLACK);

    wprints(0, 23, LCYAN | _BLACK, "Today  Yesterday   This week   This month    This year");

    wprints(2, 1, LCYAN | _BLACK, "Execution time");

    wprints(4, 1, LCYAN | _BLACK, "Idle time");
    wprints(5, 1, LCYAN | _BLACK, "Interaction");

    wprints(7, 1, LCYAN | _BLACK, "ECHOmail scan");

    wprints(9, 1, LCYAN | _BLACK, "Connections/IN");
    wprints(10, 1, LCYAN | _BLACK, "Connections/OUT");
    wprints(11, 1, LCYAN | _BLACK, "Connections/BBS");

    display_time(2, 27, YELLOW | _BLACK, sysinfo.today.exectime + time(NULL) - exectime);
    display_time(2, 38, YELLOW | _BLACK, sysinfo.yesterday.exectime);
    display_time(2, 50, YELLOW | _BLACK, sysinfo.week.exectime + time(NULL) - exectime);
    display_time(2, 63, YELLOW | _BLACK, sysinfo.month.exectime + time(NULL) - exectime);
    display_time(2, 76, YELLOW | _BLACK, sysinfo.year.exectime + time(NULL) - exectime);

    display_time(4, 27, YELLOW | _BLACK, sysinfo.today.idletime + time(NULL) - elapsed);
    display_time(4, 38, YELLOW | _BLACK, sysinfo.yesterday.idletime);
    display_time(4, 50, YELLOW | _BLACK, sysinfo.week.idletime + time(NULL) - elapsed);
    display_time(4, 63, YELLOW | _BLACK, sysinfo.month.idletime + time(NULL) - elapsed);
    display_time(4, 76, YELLOW | _BLACK, sysinfo.year.idletime + time(NULL) - elapsed);

    display_time(5, 27, YELLOW | _BLACK, sysinfo.today.interaction);
    display_time(5, 38, YELLOW | _BLACK, sysinfo.yesterday.interaction);
    display_time(5, 50, YELLOW | _BLACK, sysinfo.week.interaction);
    display_time(5, 63, YELLOW | _BLACK, sysinfo.month.interaction);
    display_time(5, 76, YELLOW | _BLACK, sysinfo.year.interaction);

    display_time(7, 27, YELLOW | _BLACK, sysinfo.today.echoscan);
    display_time(7, 38, YELLOW | _BLACK, sysinfo.yesterday.echoscan);
    display_time(7, 50, YELLOW | _BLACK, sysinfo.week.echoscan);
    display_time(7, 63, YELLOW | _BLACK, sysinfo.month.echoscan);
    display_time(7, 76, YELLOW | _BLACK, sysinfo.year.echoscan);

    display_time(9, 27, YELLOW | _BLACK, sysinfo.today.inconnects);
    display_time(9, 38, YELLOW | _BLACK, sysinfo.yesterday.inconnects);
    display_time(9, 50, YELLOW | _BLACK, sysinfo.week.inconnects);
    display_time(9, 63, YELLOW | _BLACK, sysinfo.month.inconnects);
    display_time(9, 76, YELLOW | _BLACK, sysinfo.year.inconnects);

    display_time(10, 27, YELLOW | _BLACK, sysinfo.today.outconnects);
    display_time(10, 38, YELLOW | _BLACK, sysinfo.yesterday.outconnects);
    display_time(10, 50, YELLOW | _BLACK, sysinfo.week.outconnects);
    display_time(10, 63, YELLOW | _BLACK, sysinfo.month.outconnects);
    display_time(10, 76, YELLOW | _BLACK, sysinfo.year.outconnects);

    display_time(11, 27, YELLOW | _BLACK, sysinfo.today.humanconnects);
    display_time(11, 38, YELLOW | _BLACK, sysinfo.yesterday.humanconnects);
    display_time(11, 50, YELLOW | _BLACK, sysinfo.week.humanconnects);
    display_time(11, 63, YELLOW | _BLACK, sysinfo.month.humanconnects);
    display_time(11, 76, YELLOW | _BLACK, sysinfo.year.humanconnects);

//   setkbloop (NULL);
    t = time(NULL);
    while (t == time(NULL)) {
        release_timeslice();
    }

    t = timerset(6000);
    while (!kbmhit() && !timeup(t)) {
        display_time(2, 27, YELLOW | _BLACK, sysinfo.today.exectime + time(NULL) - exectime);
        display_time(2, 50, YELLOW | _BLACK, sysinfo.week.exectime + time(NULL) - exectime);
        display_time(2, 63, YELLOW | _BLACK, sysinfo.month.exectime + time(NULL) - exectime);
        display_time(2, 76, YELLOW | _BLACK, sysinfo.year.exectime + time(NULL) - exectime);

        display_time(5, 27, YELLOW | _BLACK, sysinfo.today.interaction + time(NULL) - elapsed);
        display_time(5, 50, YELLOW | _BLACK, sysinfo.week.interaction + time(NULL) - elapsed);
        display_time(5, 63, YELLOW | _BLACK, sysinfo.month.interaction + time(NULL) - elapsed);
        display_time(5, 76, YELLOW | _BLACK, sysinfo.year.interaction + time(NULL) - elapsed);

        release_timeslice();
    }

    if (!timeup(t)) {
        getxch();
    }

    wclose();
}

static void display_time(y, x, c, t)
int y, x, c;
long t;
{
    float f;
    char string[70];

    if (t > 86400L) {
        f = (float)t / (float)86400L;
        sprintf(string, "%.2f days", f);
        wrjusts(y, x, c, string);
    }
    else if (t >= 3600L) {
        sprintf(string, "%02ld:%02ld:%02ld", t / 3600L, (t / 60L) % 60, t % 60L);
        wrjusts(y, x, c, string);
    }
    else if (t) {
        sprintf(string, "%02ld:%02ld", t / 60L, t % 60L);
        wrjusts(y, x, c, string);
    }
}

FILE * mopen(char * filename, char * mode);
int mclose(FILE * fp);
void mprintf(FILE * fp, char * format, ...);
long mseek(FILE * fp, long position, int offset);

void request_echomail_link()
{
    FILE * fp;
    int i, wh, zo, ne, no, po;
    char string[50], filename[80], *p, selpri[4];

    wh = wopen(11, 15, 13, 65, 0, LGREY | _BLACK, LCYAN | _BLACK);
    wactiv(wh);
    wtitle("REQUEST ECHOMAIL LINK", TLEFT, LCYAN | _BLACK);

    wprints(0, 1, YELLOW | _BLACK, "Address:");
    winpbeg(BLACK | _LGREY, BLACK | _LGREY);
    winpdef(0, 10, string, "??????????????????????????????????????", 0, 0, NULL, 0);
    i = winpread();
    strtrim(string);

    hidecur();
    wclose();

    if (i == W_ESCPRESS || !string[0]) {
        return;
    }

    zo = config->alias[0].zone;
    ne = config->alias[0].net;
    no = 0;
    po = 0;

    parse_netnode(strbtrim(string), &zo, &ne, &no, &po);

    if (!get_bbs_record(zo, ne, no, 0)) {
        wh = wopen(11, 25, 15, 53, 0, RED | _BLACK, LGREY | _BLACK);
        wactiv(wh);
        sprintf(string, "Node %d:%d/%d not found!", zo, ne, no);
        wcenters(1, YELLOW | _BLACK, string);
        timer(20);
        wclose();
        return;
    }

    if (!nodelist.pw_areafix[0]) {
        wh = wopen(11, 20, 15, 55, 0, RED | _BLACK, LGREY | _BLACK);
        wactiv(wh);
        sprintf(string, "Password for %d:%d/%d not found!", zo, ne, no);
        wcenters(1, YELLOW | _BLACK, string);
        timer(20);
        wclose();
        return;
    }

    wh = wopen(10, 10, 16, 68, 0, RED | _LGREY, BLUE | _LGREY);
    wactiv(wh);
    wtitle("REQUEST ECHOMAIL LINK", TLEFT, RED | _LGREY);
    whline(2, 0, 58, 0, RED | _LGREY);
    wprints(0, 1, BLUE | _LGREY, nodelist.name);
    sprintf(string, "%u:%u/%u", zo, ne, no);
    wprints(0, 56 - strlen(string), BLUE | _LGREY, string);
    wprints(1, 1, BLUE | _LGREY, nodelist.city);

    for (i = 0; i < max_call; i++) {
        if (call_list[i].zone == zo && call_list[i].net == ne && call_list[i].node == no && call_list[i].point == 0) {
            break;
        }
    }

    wprints(3, 1, WHITE | _LGREY, "Area(s):");

    winpbeg(BLUE | _CYAN, BLUE | _CYAN);
    winpdef(3, 10, string, "??????????????????????????????????????????????", 0, 0, NULL, 0);
    wprints(4, 1, WHITE | _LGREY, "Priority:   (Normal/Crash/Hold)");
    i = winpread();
    strtrim(string);

    if (i == W_ESCPRESS || !string[0]) {
        hidecur();
        wclose();
        return;
    }

    strcpy(selpri, "I");
    winpbeg(BLUE | _CYAN, BLUE | _CYAN);
    winpdef(4, 11, selpri, "?", 0, 2, NULL, 0);
    i = winpread();

    hidecur();
    if (i == W_ESCPRESS) {
        wclose();
        return;
    }

    memset((char *)&msg, 0, sizeof(struct _msg));
    memset((char *)&sys, 0, sizeof(struct _sys));
    sys.netmail = 1;
    sys.echomail = 0;
    strcpy(sys.msg_path, config->netmail_dir);

    scan_message_base(0, 0);

    strcpy(msg.from, config->sysop);
    strcpy(msg.to, "Areafix");
    strcpy(msg.subj, strupr(nodelist.pw_areafix));
    strcat(msg.subj, " -Q");
    msg.attr = MSGLOCAL | MSGPRIVATE;
    if (selpri[0] == 'C') {
        msg.attr |= MSGCRASH;
    }
    else if (selpri[0] == 'H') {
        msg.attr |= MSGHOLD;
    }
    data(msg.date);
    msg.dest_net = ne;
    msg.dest = no;
    msg_tzone = zo;
    msg_tpoint = 0;
    msg_fzone = config->alias[0].zone;
    if (config->alias[0].point && config->alias[0].fakenet) {
        msg.orig = config->alias[0].point;
        msg.orig_net = config->alias[0].fakenet;
        msg_fpoint = 0;
    }
    else {
        msg.orig = config->alias[0].node;
        msg.orig_net = config->alias[0].net;
        msg_fpoint = config->alias[0].point;
    }

    sprintf(filename, "ECHOR%d.TMP", config->line_offset);
    fp = mopen(filename, "wt");

    if (config->alias[sys.use_alias].point) {
        mprintf(fp, "\001FMPT %d\r\n", config->alias[sys.use_alias].point);
    }
    if (msg_tzone != msg_fzone) {
        mprintf(fp, msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig);
    }
    mprintf(fp, msgtxt[M_PID], VERSION, registered ? "" : NOREG);
    mprintf(fp, msgtxt[M_MSGID], config->alias[sys.use_alias].zone, config->alias[sys.use_alias].net, config->alias[sys.use_alias].node, config->alias[sys.use_alias].point, time(NULL));

    p = strtok(strbtrim(string), " ");
    do {
        mprintf(fp, "%s\r\n", p);
    } while ((p = strtok(NULL, " ")) != NULL);
    mprintf(fp, "\r\n");
    mprintf(fp, msgtxt[M_TEAR_LINE], VERSION, registered ? "" : NOREG);

    mseek(fp, 0L, SEEK_SET);
    fido_save_message2(fp, NULL);
    mclose(fp);

    unlink(filename);

    wclose();
}

void new_echomail_link()
{
    FILE * fp;
    int i, wh, zo, ne, no, po, f_flags = 0;
    char string[50], filename[80], *p, *p1, selpri[4], *a;

    wh = wopen(11, 15, 13, 65, 0, LGREY | _BLACK, LCYAN | _BLACK);
    wactiv(wh);
    wtitle("NEW ECHOMAIL LINK", TLEFT, LCYAN | _BLACK);

    wprints(0, 1, YELLOW | _BLACK, "Address:");
    winpbeg(BLACK | _LGREY, BLACK | _LGREY);
    winpdef(0, 10, string, "??????????????????????????????????????", 0, 0, NULL, 0);
    i = winpread();
    strtrim(string);

    hidecur();
    wclose();

    if (i == W_ESCPRESS || !string[0]) {
        return;
    }

    zo = config->alias[0].zone;
    ne = config->alias[0].net;
    no = 0;
    po = 0;

    parse_netnode(strbtrim(string), &zo, &ne, &no, &po);

    if (!get_bbs_record(zo, ne, no, po)) {
        wh = wopen(11, 20, 15, 60, 0, RED | _BLACK, LGREY | _BLACK);
        wactiv(wh);
        sprintf(string, "Node %d:%d/%d.%d not found!", zo, ne, no, po);
        wcenters(1, YELLOW | _BLACK, string);
        timer(20);
        wclose();
        return;
    }

    if (!nodelist.pw_areafix[0]) {
        wh = wopen(11, 20, 15, 60, 0, RED | _BLACK, LGREY | _BLACK);
        wactiv(wh);
        sprintf(string, "Password for %d:%d/%d.%d not found!", zo, ne, no, po);
        wcenters(1, YELLOW | _BLACK, string);
        timer(20);
        wclose();
        return;
    }

    wh = wopen(10, 10, 16, 68, 0, RED | _LGREY, BLUE | _LGREY);
    wactiv(wh);
    wtitle("NEW ECHOMAIL LINK", TLEFT, RED | _LGREY);
    whline(2, 0, 58, 0, RED | _LGREY);
    wprints(0, 1, BLUE | _LGREY, nodelist.name);
    sprintf(string, "%u:%u/%u.%u", zo, ne, no, po);
    wprints(0, 56 - strlen(string), BLUE | _LGREY, string);
    wprints(1, 1, BLUE | _LGREY, nodelist.city);

    for (i = 0; i < max_call; i++) {
        if (call_list[i].zone == zo && call_list[i].net == ne && call_list[i].node == no && call_list[i].point == 0) {
            break;
        }
    }

    wprints(3, 1, WHITE | _LGREY, "Area(s):");

    winpbeg(BLUE | _CYAN, BLUE | _CYAN);
    winpdef(3, 10, string, "??????????????????????????????????????????????", 0, 0, NULL, 0);
    wprints(4, 1, WHITE | _LGREY, "Priority:   (Normal/Crash/Hold)");
    i = winpread();
    strtrim(string);

    if (i == W_ESCPRESS || !string[0]) {
        hidecur();
        wclose();
        return;
    }

    strcpy(selpri, "N");
    winpbeg(BLUE | _CYAN, BLUE | _CYAN);
    winpdef(4, 11, selpri, "?", 0, 2, NULL, 0);
    i = winpread();

    hidecur();
    if (i == W_ESCPRESS) {
        wclose();
        return;
    }

    memset((char *)&msg, 0, sizeof(struct _msg));
    memset((char *)&sys, 0, sizeof(struct _sys));
    sys.netmail = 1;
    sys.echomail = 0;
    strcpy(sys.msg_path, config->netmail_dir);

    scan_message_base(0, 0);
    wclose();

    strcpy(msg.to, nodelist.sysop);
    strcpy(msg.from, VERSION);
    strcpy(msg.subj, "New echomail link");
    msg.attr = MSGLOCAL;
    if (selpri[0] == 'C') {
        msg.attr |= MSGCRASH;
    }
    else if (selpri[0] == 'H') {
        msg.attr |= MSGHOLD;
    }
    data(msg.date);
    msg.dest_net = ne;
    msg.dest = no;
    msg_tzone = zo;
    msg_tpoint = 0;
    msg_fzone = config->alias[0].zone;
    if (config->alias[0].point && config->alias[0].fakenet) {
        msg.orig = config->alias[0].point;
        msg.orig_net = config->alias[0].fakenet;
        msg_fpoint = 0;
    }
    else {
        msg.orig = config->alias[0].node;
        msg.orig_net = config->alias[0].net;
        msg_fpoint = config->alias[0].point;
    }

    sprintf(filename, "ECHOR%d.TMP", config->line_offset);
    fp = mopen(filename, "wt");

    if (config->alias[sys.use_alias].point) {
        mprintf(fp, "\001FMPT %d\r\n", config->alias[sys.use_alias].point);
    }
    if (msg_tzone != msg_fzone) {
        mprintf(fp, msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig);
    }
    mprintf(fp, msgtxt[M_PID], VERSION, registered ? "" : NOREG);
    mprintf(fp, msgtxt[M_MSGID], config->alias[sys.use_alias].zone, config->alias[sys.use_alias].net, config->alias[sys.use_alias].node, config->alias[sys.use_alias].point, time(NULL));

    mprintf(fp, "Following is a summary from %d:%d/%d.%d of changes in Echomail topology:\r\n\r\n", msg_fzone, msg.orig_net, msg.orig, msg_fpoint);

    p = strtok(strbtrim(string), " ");
    p1 = strtok(NULL, " ");
    a = strtok(NULL, "");

    do {
        if (*p1 == 'P' || *p1 == 'p') {
            f_flags = 1;
        }
        else {
            f_flags = 0;
        }
        i = utility_add_echomail_link(p, zo, ne, no, po, fp, f_flags);
        if (i == 1) {
            if (*p != '-') {
                mprintf(fp, "Area %s has been added.", strupr(p));
                if (f_flags) {
                    mprintf(fp, " - Linked as private\r\n");
                }
                else {
                    mprintf(fp, "\r\n");
                }
            }
            else {
                mprintf(fp, "Area %s has been removed.\r\n", strupr(p));
            }
        }
        else if (i == -1) {
            mprintf(fp, "Area %s not found.\r\n", strupr(p));
        }
        else if (i == -2) {
            mprintf(fp, "Area %s never linked.\r\n", strupr(p));
        }
        else if (i == -3) {
            mprintf(fp, "Area %s already linked.\r\n", strupr(p));
        }
        else if (i == -4) {
            mprintf(fp, "Area %s not linked: level too low.\r\n", strupr(p));
        }

        p = strtok(a, " ");
        p1 = strtok(NULL, " ");
        a = strtok(NULL, "");
    } while (p != NULL);

    mprintf(fp, "\r\n", p);
    mprintf(fp, "------------------------------------------------\r\n");

    generate_echomail_status(fp, zo, ne, no, po, 2);
    mprintf(fp, "\r\n", p);
    mprintf(fp, msgtxt[M_TEAR_LINE], VERSION, registered ? "" : NOREG);

    mseek(fp, 0L, SEEK_SET);
    fido_save_message2(fp, NULL);
    mclose(fp);

    unlink(filename);
}

void new_tic_link()
{
    FILE * fp;
    int i, wh, zo, ne, no, po;
    char string[50], filename[80], *p, selpri[4], *a;

    wh = wopen(11, 15, 13, 65, 0, LGREY | _BLACK, LCYAN | _BLACK);
    wactiv(wh);
    wtitle("NEW TIC LINK", TLEFT, LCYAN | _BLACK);

    wprints(0, 1, YELLOW | _BLACK, "Address:");
    winpbeg(BLACK | _LGREY, BLACK | _LGREY);
    winpdef(0, 10, string, "??????????????????????????????????????", 0, 0, NULL, 0);
    i = winpread();
    strtrim(string);

    hidecur();
    wclose();

    if (i == W_ESCPRESS || !string[0]) {
        return;
    }

    zo = config->alias[0].zone;
    ne = config->alias[0].net;
    no = 0;
    po = 0;

    parse_netnode(strbtrim(string), &zo, &ne, &no, &po);

    if (!get_bbs_record(zo, ne, no, po)) {
        wh = wopen(11, 20, 15, 60, 0, RED | _BLACK, LGREY | _BLACK);
        wactiv(wh);
        sprintf(string, "Node %d:%d/%d.%d not found!", zo, ne, no, po);
        wcenters(1, YELLOW | _BLACK, string);
        timer(20);
        wclose();
        return;
    }

    if (!nodelist.pw_tic[0]) {
        wh = wopen(11, 20, 15, 60, 0, RED | _BLACK, LGREY | _BLACK);
        wactiv(wh);
        sprintf(string, "Password for %d:%d/%d.%d not found!", zo, ne, no, po);
        wcenters(1, YELLOW | _BLACK, string);
        timer(20);
        wclose();
        return;
    }

    wh = wopen(10, 10, 16, 68, 0, RED | _LGREY, BLUE | _LGREY);
    wactiv(wh);
    wtitle("NEW TIC LINK", TLEFT, RED | _LGREY);
    whline(2, 0, 58, 0, RED | _LGREY);
    wprints(0, 1, BLUE | _LGREY, nodelist.name);
    sprintf(string, "%u:%u/%u.%u", zo, ne, no, po);
    wprints(0, 56 - strlen(string), BLUE | _LGREY, string);
    wprints(1, 1, BLUE | _LGREY, nodelist.city);

    for (i = 0; i < max_call; i++) {
        if (call_list[i].zone == zo && call_list[i].net == ne && call_list[i].node == no && call_list[i].point == 0) {
            break;
        }
    }

    wprints(3, 1, WHITE | _LGREY, "Area(s):");

    winpbeg(BLUE | _CYAN, BLUE | _CYAN);
    winpdef(3, 10, string, "??????????????????????????????????????????????", 0, 0, NULL, 0);
    wprints(4, 1, WHITE | _LGREY, "Priority:   (Normal/Crash/Hold)");
    i = winpread();
    strtrim(string);

    if (i == W_ESCPRESS || !string[0]) {
        hidecur();
        wclose();
        return;
    }

    strcpy(selpri, "N");
    winpbeg(BLUE | _CYAN, BLUE | _CYAN);
    winpdef(4, 11, selpri, "?", 0, 2, NULL, 0);
    i = winpread();

    hidecur();
    if (i == W_ESCPRESS) {
        wclose();
        return;
    }

    memset((char *)&msg, 0, sizeof(struct _msg));
    memset((char *)&sys, 0, sizeof(struct _sys));
    sys.netmail = 1;
    sys.echomail = 0;
    strcpy(sys.msg_path, config->netmail_dir);

    scan_message_base(0, 0);
    wclose();

    strcpy(msg.to, nodelist.sysop);
    strcpy(msg.from, VERSION);
    strcpy(msg.subj, "New TIC link");
    msg.attr = MSGLOCAL;
    if (selpri[0] == 'C') {
        msg.attr |= MSGCRASH;
    }
    else if (selpri[0] == 'H') {
        msg.attr |= MSGHOLD;
    }
    data(msg.date);
    msg.dest_net = ne;
    msg.dest = no;
    msg_tzone = zo;
    msg_tpoint = 0;
    msg_fzone = config->alias[0].zone;
    if (config->alias[0].point && config->alias[0].fakenet) {
        msg.orig = config->alias[0].point;
        msg.orig_net = config->alias[0].fakenet;
        msg_fpoint = 0;
    }
    else {
        msg.orig = config->alias[0].node;
        msg.orig_net = config->alias[0].net;
        msg_fpoint = config->alias[0].point;
    }

    sprintf(filename, "ECHOR%d.TMP", config->line_offset);
    fp = mopen(filename, "wt");

    if (config->alias[sys.use_alias].point) {
        mprintf(fp, "\001FMPT %d\r\n", config->alias[sys.use_alias].point);
    }
    if (msg_tzone != msg_fzone) {
        mprintf(fp, msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig);
    }
    mprintf(fp, msgtxt[M_PID], VERSION, registered ? "" : NOREG);
    mprintf(fp, msgtxt[M_MSGID], config->alias[sys.use_alias].zone, config->alias[sys.use_alias].net, config->alias[sys.use_alias].node, config->alias[sys.use_alias].point, time(NULL));

    mprintf(fp, "Following is a summary from %d:%d/%d.%d of changes in TIC topology:\r\n\r\n", msg_fzone, msg.orig_net, msg.orig, msg_fpoint);

    p = strtok(strbtrim(string), " ");
    a = strtok(NULL, "");

    do {
        i = utility_add_tic_link(p, zo, ne, no, po, NULL);
        if (i == 1) {
            if (*p != '-') {
                mprintf(fp, "Area %s has been added.\r\n", strupr(p));
            }
            else {
                mprintf(fp, "Area %s has been removed.\r\n", strupr(p));
            }
        }
        else if (i == -1) {
            mprintf(fp, "Area %s not found.\r\n", strupr(p));
        }
        else if (i == -2) {
            mprintf(fp, "Area %s never linked.\r\n", strupr(p));
        }
        else if (i == -3) {
            mprintf(fp, "Area %s already linked.\r\n", strupr(p));
        }
        else if (i == -4) {
            mprintf(fp, "Area %s not linked: level too low.\r\n", strupr(p));
        }

        p = strtok(a, " ");
        a = strtok(NULL, "");
    } while (p != NULL);

    mprintf(fp, "\r\n", p);
    mprintf(fp, "------------------------------------------------\r\n");

    generate_tic_status(fp, zo, ne, no, po, 2);
    mprintf(fp, "\r\n", p);
    mprintf(fp, msgtxt[M_TEAR_LINE], VERSION, registered ? "" : NOREG);

    mseek(fp, 0L, SEEK_SET);
    fido_save_message2(fp, NULL);
    mclose(fp);

    unlink(filename);
}

void rescan_echomail_link()
{
    FILE * fp;
    int fd, i, wh, zo, ne, no, po;
    char string[50], filename[80], *p, selpri[4];
    struct _sys tsys;

    wh = wopen(11, 15, 13, 65, 0, LGREY | _BLACK, LCYAN | _BLACK);
    wactiv(wh);
    wtitle("RESCAN ECHOMAIL", TLEFT, LCYAN | _BLACK);

    wprints(0, 1, YELLOW | _BLACK, "Address:");
    winpbeg(BLACK | _LGREY, BLACK | _LGREY);
    winpdef(0, 10, string, "??????????????????????????????????????", 0, 0, NULL, 0);
    i = winpread();
    strtrim(string);

    hidecur();
    wclose();

    if (i == W_ESCPRESS || !string[0]) {
        return;
    }

    zo = config->alias[0].zone;
    ne = config->alias[0].net;
    no = 0;
    po = 0;

    parse_netnode(strbtrim(string), &zo, &ne, &no, &po);

    if (!get_bbs_record(zo, ne, no, po)) {
        wh = wopen(11, 20, 15, 60, 0, RED | _BLACK, LGREY | _BLACK);
        wactiv(wh);
        sprintf(string, "Node %d:%d/%d.%d not found!", zo, ne, no, po);
        wcenters(1, YELLOW | _BLACK, string);
        timer(20);
        wclose();
        return;
    }

    wh = wopen(10, 10, 16, 68, 0, RED | _LGREY, BLUE | _LGREY);
    wactiv(wh);
    wtitle("RESCAN ECHOMAIL", TLEFT, RED | _LGREY);
    whline(2, 0, 58, 0, RED | _LGREY);
    wprints(0, 1, BLUE | _LGREY, nodelist.name);
    sprintf(string, "%u:%u/%u.%u", zo, ne, no, po);
    wprints(0, 56 - strlen(string), BLUE | _LGREY, string);
    wprints(1, 1, BLUE | _LGREY, nodelist.city);

    for (i = 0; i < max_call; i++) {
        if (call_list[i].zone == zo && call_list[i].net == ne && call_list[i].node == no && call_list[i].point == 0) {
            break;
        }
    }

    wprints(3, 1, WHITE | _LGREY, "Area(s):");
    wprints(4, 1, WHITE | _LGREY, "Processing now:   (Yes/No)");
    strcpy(selpri, "Y");

    winpbeg(BLUE | _CYAN, BLUE | _CYAN);
    winpdef(3, 10, string, "??????????????????????????????????????????????", 0, 0, NULL, 0);
    i = winpread();
    strtrim(string);

    hidecur();
    if (i == W_ESCPRESS || !string[0]) {
        wclose();
        return;
    }

    winpbeg(BLUE | _CYAN, BLUE | _CYAN);
    winpdef(4, 17, selpri, "?", 0, 2, NULL, 0);
    i = winpread();

    hidecur();
    if (i == W_ESCPRESS) {
        wclose();
        return;
    }

    if ((p = strtok(string, " ")) != NULL) {
        sprintf(filename, SYSMSG_PATH, config->sys_path);
        fd = sh_open(filename, SH_DENYWR, O_RDWR | O_BINARY, S_IREAD | S_IWRITE);
        if (fd == -1) {
            return;
        }

        fp = fopen("RESCAN.LOG", "at");

        do {
            lseek(fd, 0L, SEEK_SET);
            while (read(fd, (char *)&tsys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA)
                if (tsys.echomail && !stricmp(tsys.echotag, p)) {
                    if (tsys.quick_board) {
                        fprintf(fp, "%d:%d/%d.%d %c %d %s\n", zo, ne, no, po, 'Q', tsys.quick_board, tsys.echotag);
                    }
                    else if (tsys.gold_board) {
                        fprintf(fp, "%d:%d/%d.%d %c %d %s\n", zo, ne, no, po, 'G', tsys.gold_board, tsys.echotag);
                    }
                    else if (tsys.pip_board) {
                        fprintf(fp, "%d:%d/%d.%d %c %d %s\n", zo, ne, no, po, 'P', tsys.pip_board, tsys.echotag);
                    }
                    else if (tsys.squish) {
                        fprintf(fp, "%d:%d/%d.%d %c %s %s\n", zo, ne, no, po, 'S', tsys.msg_path, tsys.echotag);
                    }
                    else {
                        fprintf(fp, "%d:%d/%d.%d %c %s %s\n", zo, ne, no, po, 'M', tsys.msg_path, tsys.echotag);
                    }
                    break;
                }
        } while ((p = strtok(NULL, " ")) != NULL);

        close(fd);
        fclose(fp);
    }

    wclose();

    if (toupper(selpri[0]) == 'Y') {
        if (modem_busy != NULL) {
            mdm_sendcmd(modem_busy);
        }
        rescan_areas();
        pack_outbound(PACK_ECHOMAIL);
        idle_system();
        old_event = -1;
        if (modem_busy != NULL) {
            modem_hangup();
        }
    }
}

void edit_outbound_info()
{
    FILE * fp;
    int wh, i, v, mc, mnc, last = 0, ch;
    char j[80], *entry[MAX_OUT], string[80];
    struct ffblk blk;

    if (!max_call) {
        return;
    }

    if (cur_event > -1) {
        mc = e_ptrs[cur_event]->with_connect ? e_ptrs[cur_event]->with_connect : max_connects;
        mnc = e_ptrs[cur_event]->no_connect ? e_ptrs[cur_event]->no_connect : max_noconnects;
    }
    else {
        mc = mnc = 32767;
    }

    outinfo = 0;
    display_outbound_info(outinfo);

rescan:
    string[51] = '\0';

    for (i = 0; i < max_call; i++) {
        memset(string, ' ', 51);

        sprintf(j, "%d:%d/%d.%d", call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point);
        memcpy(&string[1], j, strlen(j));
        if (call_list[i].size < 10240L) {
            sprintf(j, "%ldb ", call_list[i].size);
        }
        else {
            sprintf(j, "%ldKb", call_list[i].size / 1024L);
        }
        memcpy(&string[38 - strlen(j)], j, strlen(j));
        v = 0;
        if (call_list[i].type & MAIL_CRASH) {
            j[v++] = 'C';
        }
        if (call_list[i].type & MAIL_NORMAL) {
            j[v++] = 'N';
        }
        if (call_list[i].type & MAIL_DIRECT) {
            j[v++] = 'D';
        }
        if (call_list[i].type & MAIL_HOLD) {
            j[v++] = 'H';
        }
        if (call_list[i].type & MAIL_REQUEST) {
            j[v++] = 'R';
        }
        if (call_list[i].type & MAIL_WILLGO) {
            j[v++] = 'I';
        }
        j[v] = '\0';
        memcpy(&string[29 - strlen(j)], j, strlen(j));
        sprintf(j, "%d", call_list[i].call_nc);
        memcpy(&string[20 - strlen(j)], j, strlen(j));
        sprintf(j, "%d", call_list[i].call_wc);
        memcpy(&string[23 - strlen(j)], j, strlen(j));

        if (next_call == i) {
            string[38] = '\020';
        }

        if (cur_event >= 0) {
            if (!(call_list[i].type & MAIL_WILLGO)) {
                if (call_list[i].type & MAIL_UNKNOWN) {
                    memcpy(&string[39], "*Unlisted", 9);
                }
                else if (!(call_list[i].type & (MAIL_CRASH | MAIL_DIRECT | MAIL_NORMAL))) {
                    memcpy(&string[39], "*Hold", 5);
                }
                else if (e_ptrs[cur_event]->behavior & MAT_NOOUT) {
                    memcpy(&string[39], " Temp.hold", 10);
                }
                else if ((call_list[i].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_NOCM)) {
                    memcpy(&string[39], " Temp.hold", 10);
                }
                else if (!(call_list[i].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_CM)) {
                    memcpy(&string[39], " Temp.hold", 10);
                }
                else if (e_ptrs[cur_event]->res_net && (call_list[i].net != e_ptrs[cur_event]->res_net || call_list[i].node != e_ptrs[cur_event]->res_node)) {
                    memcpy(&string[39], " Temp.hold", 10);
                }
                else if (call_list[i].call_nc >= mnc || call_list[i].call_wc >= mc) {
                    memcpy(&string[39], " Undialable", 11);
                }
                else {
                    switch (call_list[i].flags) {
                        case NO_CARRIER:
                            memcpy(&string[39], " No Carrier", 11);
                            break;
                        case NO_DIALTONE:
                            memcpy(&string[39], " No Dialtone", 12);
                            break;
                        case BUSY:
                            memcpy(&string[39], " Busy", 5);
                            break;
                        case NO_ANSWER:
                            memcpy(&string[39], " No Answer", 10);
                            break;
                        case VOICE:
                            memcpy(&string[39], " Voice", 6);
                            break;
                        case ABORTED:
                            memcpy(&string[39], " Aborted", 8);
                            break;
                        case TIMEDOUT:
                            memcpy(&string[39], " Timeout", 8);
                            break;
                        default:
                            memcpy(&string[39], " ------", 7);
                            break;
                    }
                }
            }
            else {
                if (call_list[i].type & MAIL_UNKNOWN) {
                    memcpy(&string[39], "*Unlisted", 9);
                }
                else if (call_list[i].call_nc >= mnc || call_list[i].call_wc >= mc) {
                    memcpy(&string[39], "*Undialable", 11);
                }
                else {
                    switch (call_list[i].flags) {
                        case NO_CARRIER:
                            memcpy(&string[39], " No Carrier", 11);
                            break;
                        case NO_DIALTONE:
                            memcpy(&string[39], " No Dialtone", 12);
                            break;
                        case BUSY:
                            memcpy(&string[39], " Busy", 5);
                            break;
                        case NO_ANSWER:
                            memcpy(&string[39], " No Answer", 10);
                            break;
                        case VOICE:
                            memcpy(&string[39], " Voice", 6);
                            break;
                        case ABORTED:
                            memcpy(&string[39], " Aborted", 8);
                            break;
                        case TIMEDOUT:
                            memcpy(&string[39], " Timeout", 8);
                            break;
                        default:
                            memcpy(&string[39], " ------", 7);
                            break;
                    }
                }
            }
        }

        entry[i] = (char *)malloc(strlen(string));
        if (entry[i] == NULL) {
            break;
        }
        strcpy(entry[i], string);
    }

    entry[i] = NULL;

    if (i) {
        i = wpickstr(14, 1, 21, 51, 5, LGREY | _BLACK, LCYAN | _BLACK, BLACK | _LGREY, entry, last, NULL);

        if (i != -1) {
            last = i;

            wh = wopen(7, 22, 19, 58, 3, RED | _BLACK, LCYAN | _BLACK);
            wactiv(wh);
            wtitle("OUTBOUND", TLEFT, LCYAN | _BLACK);
            wshadow(DGREY | _BLACK);

            sprintf(string, "Mail for %d:%d/%d.%d", call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point);
            wcenters(1, LCYAN | _BLACK, string);

            wmenubegc();
            wmenuitem(3,  6, " Reset call counters   ", 0, 2, 0, NULL, 0, 0);
            wmenuitem(4,  6, " Hold mail             ", 0, 3, 0, NULL, 0, 0);
            wmenuitem(5,  6, " Crash mail            ", 0, 4, 0, NULL, 0, 0);
            wmenuitem(6,  6, " Direct mail           ", 0, 5, 0, NULL, 0, 0);
            wmenuitem(7,  6, " Normal mail           ", 0, 6, 0, NULL, 0, 0);
            wmenuitem(8,  6, " Toggle immediate flag ", 0, 7, 0, NULL, 0, 0);
            wmenuitem(9,  6, " Erase entry           ", 0, 1, 0, NULL, 0, 0);
            wmenuend(2, M_OMNI | M_SAVE, 0, 0, LGREY | _BLACK, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY);

            ch = wmenuget();

            switch (ch) {
                case 1:
                {
                    char string[10];
                    int i, wh1;

                    wh1 = wopen(10, 25, 14, 54, 0, BLACK | _LGREY, BLACK | _LGREY);
                    wactiv(wh1);
                    wshadow(DGREY | _BLACK);

                    wcenters(1, BLACK | _LGREY, "Are you sure (Y/n) ?  ");

                    strcpy(string, "Y");
                    winpbeg(BLACK | _LGREY, BLACK | _LGREY);
                    winpdef(1, 24, string, "?", 0, 2, NULL, 0);

                    i = winpread();
                    wclose();
                    hidecur();
                    if (i == W_ESCPRESS) {
                        break;
                    }
                    if (toupper(string[0]) != 'Y') {
                        break;
                    }
                }
                hold_area = HoldAreaNameMunge(call_list[i].zone);
                if (call_list[i].point) {
                    sprintf(string, "%s%04x%04x.PNT\\%08x.*", hold_area, call_list[i].net, call_list[i].node, call_list[i].point);
                }
                else {
                    sprintf(string, "%s%04x%04x.*", hold_area, call_list[i].net, call_list[i].node);
                }
                if (!findfirst(string, &blk, 0))
                    do {
                        if (call_list[i].point) {
                            sprintf(string, "%s%04x%04x.PNT\\%s", hold_area, call_list[i].net, call_list[i].node, blk.ff_name);
                        }
                        else {
                            sprintf(string, "%s%s", hold_area, blk.ff_name);
                        }
                        if (blk.ff_name[8] == '.' && blk.ff_name[10] == 'L' && blk.ff_name[11] == 'O') {
                            fp = fopen(string, "rt");
                            while (fgets(j, 70, fp) != NULL) {
                                while (strlen(j) && (j[strlen(j) - 1] == 0x0D || j[strlen(j) - 1] == 0x0A)) {
                                    j[strlen(j) - 1] = '\0';
                                }
                                if (j[0] == '^') {
                                    unlink(&j[1]);
                                }
                                else if (j[0] == '#') {
                                    unlink(&j[1]);
                                    v = creat(&j[1], S_IREAD | S_IWRITE);
                                    close(v);
                                }
                            }
                            fclose(fp);
                        }
                        unlink(string);
                    } while (!findnext(&blk));

                if (i == next_call) {
                    next_call = 0;
                }
                else if (i < next_call) {
                    next_call--;
                }

                for (i = last + 1; i < max_call; i++) {
                    memcpy(&call_list[i - 1], &call_list[i], sizeof(struct _call_list));
                }
                max_call--;
                if (last >= max_call) {
                    last = max_call - 1;
                }
                break;

                case 2:
                    if (!call_list[i].point) {
                        call_list[i].call_nc = 0;
                        call_list[i].call_wc = 0;
                        hold_area = HoldAreaNameMunge(call_list[i].zone);
                        bad_call(call_list[i].net, call_list[i].node, -2, 0);
                    }
                    break;

                case 7:
                    if (!(call_list[i].type & MAIL_WILLGO)) {
                        make_immediate(call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point);
                        next_call = i;
                    }
                    else {
                        delete_immediate(call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point);
                    }
                    call_list[i].type ^= MAIL_WILLGO;
                    break;

                case 3:
                    change_type(call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point, 'F', 'H');
                    change_type(call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point, 'D', 'H');
                    change_type(call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point, 'C', 'H');
                    if (call_list[i].type & MAIL_WILLGO) {
                        delete_immediate(call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point);
                        call_list[i].type ^= MAIL_WILLGO;
                    }
                    call_list[i].type = MAIL_HOLD;
                    break;

                case 5:
                    change_type(call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point, 'F', 'D');
                    change_type(call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point, 'H', 'D');
                    change_type(call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point, 'C', 'D');
                    if (call_list[i].type & MAIL_WILLGO) {
                        delete_immediate(call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point);
                        call_list[i].type ^= MAIL_WILLGO;
                    }
                    call_list[i].type = MAIL_DIRECT;
                    break;

                case 4:
                    change_type(call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point, 'F', 'C');
                    change_type(call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point, 'D', 'C');
                    change_type(call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point, 'H', 'C');
                    if (call_list[i].type & MAIL_WILLGO) {
                        delete_immediate(call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point);
                        call_list[i].type ^= MAIL_WILLGO;
                    }
                    call_list[i].type = MAIL_CRASH;
                    break;

                case 6:
                    change_type(call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point, 'C', 'F');
                    change_type(call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point, 'D', 'F');
                    change_type(call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point, 'H', 'F');
                    if (call_list[i].type & MAIL_WILLGO) {
                        delete_immediate(call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point);
                        call_list[i].type ^= MAIL_WILLGO;
                    }
                    call_list[i].type = MAIL_NORMAL;
                    break;
            }

            for (i = 0; i < max_call; i++) {
                if (entry[i] == NULL) {
                    break;
                }
                free(entry[i]);
            }

            wclose();
            write_call_queue();
            display_outbound_info(outinfo);
            goto rescan;
        }
    }

    for (i = 0; i < max_call; i++) {
        if (entry[i] == NULL) {
            break;
        }
        free(entry[i]);
    }

    outinfo = 0;

    write_call_queue();
    display_outbound_info(outinfo);
}

void import_newareas(char * newareas)
{
    FILE * fp;
    int fd, found, lastarea, i, zo, ne, no, po, total = 0, cc;
    char string[260], *location, *tag, *forward;
    struct _sys sys;

    fp = fopen(newareas, "rt");
    if (fp == NULL) {
        return;
    }

    sprintf(string, "%sSYSMSG.DAT", config->sys_path);
    fd = sh_open(string, SH_DENYWR, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);

    fgets(string, 255, fp);

    while (fgets(string, 255, fp) != NULL) {
        if (string[0] == ';') {
            continue;
        }
        while (string[strlen(string) - 1] == 0x0D || string[strlen(string) - 1] == 0x0A || string[strlen(string) - 1] == ' ') {
            string[strlen(string) - 1] = '\0';
        }
        if ((location = strtok(string, " ")) == NULL) {
            continue;
        }
        location = strbtrim(location);
        if (strlen(location) > 39) {
            location[39] = '\0';
        }
        if ((tag = strtok(NULL, " ")) == NULL) {
            continue;
        }
        tag = strbtrim(tag);
        if (strlen(tag) > 31) {
            tag[31] = '\0';
        }

        if ((forward = strtok(NULL, "")) == NULL) {
            continue;
        }
        forward = strbtrim(forward);

        lseek(fd, 0L, SEEK_SET);
        cc = lastarea = found = 0;

        while (read(fd, (char *)&sys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
            if (sys.msg_num > lastarea) {
                lastarea = sys.msg_num;
            }

            cc++;

            if (!stricmp(sys.echotag, tag)) {
                strcpy(sys.forward1, forward);
                zo = config->alias[0].zone;
                parse_netnode(sys.forward1, &zo, &ne, &no, &po);
                for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++)
                    if (zo == config->alias[i].zone) {
                        break;
                    }
                if (i < MAX_ALIAS && config->alias[i].net) {
                    sys.use_alias = i;
                }

                if (!strcmp(location, "##")) {
                    sys.passthrough = 1;
                }
                else if (toupper(*location) == 'G') {
                    sys.gold_board = atoi(++location);
                }
                else if (atoi(location)) {
                    sys.quick_board = atoi(location);
                }
                else if (*location == '$') {
                    strcpy(sys.msg_path, ++location);
                    sys.squish = 1;
                }
                else if (*location == '!') {
                    sys.pip_board = atoi(++location);
                }
                else {
                    strcpy(sys.msg_path, location);
                }

                sys.netmail = 0;
                sys.echomail = 1;

                lseek(fd, -1L * SIZEOF_MSGAREA, SEEK_CUR);
                write(fd, (char *)&sys, SIZEOF_MSGAREA);

                found = 1;
                break;
            }
        }

        if (!found) {
            memset((char *)&sys, 0, SIZEOF_MSGAREA);
            sys.msg_num = lastarea + 1;
            strcpy(sys.echotag, tag);
            strcpy(sys.msg_name, tag);
            strcpy(sys.forward1, forward);
            sys.msg_priv = sys.write_priv = SYSOP;
            sys.max_msgs = 200;
            sys.max_age = 14;
            sys.areafix = 255;

            zo = config->alias[0].zone;
            parse_netnode(sys.forward1, &zo, &ne, &no, &po);
            for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++)
                if (zo == config->alias[i].zone) {
                    break;
                }
            if (i < MAX_ALIAS && config->alias[i].net) {
                sys.use_alias = i;
            }

            sys.echomail = 1;

            if (!strcmp(location, "##")) {
                sys.passthrough = 1;
            }
            else if (toupper(*location) == 'G') {
                sys.gold_board = atoi(++location);
            }
            else if (atoi(location)) {
                sys.quick_board = atoi(location);
            }
            else if (*location == '$') {
                strcpy(sys.msg_path, ++location);
                sys.squish = 1;
            }
            else if (*location == '!') {
                sys.pip_board = atoi(++location);
            }
            else {
                strcpy(sys.msg_path, location);
            }

            write(fd, (char *)&sys, SIZEOF_MSGAREA);
            status_line(":  %s added as area #%d", sys.echotag, sys.msg_num);
        }

        total++;
    }

    close(fd);
    fclose(fp);
}


