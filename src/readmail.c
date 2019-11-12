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
#include <dir.h>
#include <io.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "quickmsg.h"
#include "pipbase.h"

void m_print2(char * format, ...);

#define MAX_MAIL_BUFFER   20

static int last_read_system;

static int quick_index(int, int *, int *, int);
static int gold_index(int, int *, int *, int);
static int pipbase_index(int, int *, int *, int);
static int fido_index(int, int *, int *, int);
static int squish_index(int, int *, int *, int);

static char * rotate = "|/-\\", rotatepos;

int scan_mailbox()
{
    int i, z, *area_msg, *area_num, maxareas, line;
    char filename[80];
    struct _sys tsys;

    sprintf(filename, SYSMSG_PATH, config->sys_path);
    if ((i = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) == -1) {
        return (0);
    }

    z = (int)(filelength(i) / SIZEOF_MSGAREA);

    if ((area_msg = (int *)malloc(z * sizeof(int))) == NULL) {
        return (0);
    }

    if ((area_num = (int *)malloc(z * sizeof(int))) == NULL) {
        free(area_msg);
        return (0);
    }

    z = 0;
    while (read(i, &tsys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
        area_num[z++] = tsys.msg_num;
    }
    maxareas = z;

    close(i);

    memset(area_msg, 0, sizeof(int) * z);
    z = 1;

    if (msg_list != NULL) {
        free(msg_list);
    }

    if ((msg_list = (struct _msg_list *)malloc(MAX_PRIV_MAIL * sizeof(struct _msg_list))) == NULL) {
        return (0);
    }

    m_print(bbstxt[B_CHECK_MAIL]);

    rotatepos = 0;
    m_print(" \b%c", rotate[rotatepos]);
    rotatepos = (rotatepos + 1) % 4;

    z = fido_index(z, area_msg, area_num, maxareas);
    z = quick_index(z, area_msg, area_num, maxareas);
    z = gold_index(z, area_msg, area_num, maxareas);
    z = pipbase_index(z, area_msg, area_num, maxareas);
    z = squish_index(z, area_msg, area_num, maxareas);

    max_priv_mail = z - 1;
    last_mail = 0;
    last_read_system = 0;

    if (z == 1) {
        free(area_num);
        free(area_msg);

        m_print(bbstxt[B_NO_MAIL_TODAY]);
        press_enter();

        return (0);
    }

    num_msg = last_msg = max_priv_mail;
    first_msg = 0;

    m_print(bbstxt[B_THIS_MAIL]);
    m_print(bbstxt[B_THIS_MAIL_UNDERLINE]);
    line = 3;

    for (i = 0; i < maxareas; i++) {
        if (!area_msg[i] || !read_system2(area_num[i], 1, &tsys)) {
            continue;
        }

        m_print(bbstxt[B_MAIL_LIST_FORMAT], tsys.msg_name, area_msg[i]);
        if ((line = more_question(line)) == 0) {
            break;
        }
    }

    free(area_num);
    free(area_msg);

    m_print(bbstxt[B_READ_MAIL_NOW]);
    i = yesno_question(DEF_YES);

    m_print(bbstxt[B_ONE_CR]);
    return (i);
}

void mail_read_forward(pause)
int pause;
{
    int start, i;

    start = last_mail;

    if (start < 0) {
        start = 1;
    }

    if (start < max_priv_mail) {
        start++;
    }
    else {
        m_print(bbstxt[B_NO_MORE_MESSAGE]);
        return;
    }

    if (last_read_system != msg_list[start].area) {
        read_system(msg_list[start].area, 1);
        usr.msg = 0;
        if (sys.quick_board || sys.gold_board) {
            quick_scan_message_base(sys.quick_board, sys.gold_board, msg_list[start].area, 1);
        }
        else if (sys.pip_board) {
            pip_scan_message_base(msg_list[start].area, 1);
        }
        else if (sys.squish) {
            squish_scan_message_base(msg_list[start].area, sys.msg_path, 1);
        }
        else {
            scan_message_base(msg_list[start].area, 1);
        }
        usr.msg = last_read_system = msg_list[start].area;
        num_msg = last_msg = max_priv_mail;
        first_msg = 0;
    }

    for (;;) {
        if (sys.quick_board || sys.gold_board) {
            i = quick_read_message(msg_list[start].msg_num, pause, start);
        }
        else if (sys.pip_board) {
            i = pip_msg_read(sys.pip_board, msg_list[start].msg_num, 0, pause, start);
        }
        else if (sys.squish) {
            i = squish_read_message(msg_list[start].msg_num, pause, start);
        }
        else {
            i = read_message(msg_list[start].msg_num, pause, start);
        }

        if (!i) {
            if (start < last_msg) {
                start++;
                if (last_read_system != msg_list[start].area) {
                    read_system(msg_list[start].area, 1);
                    if (sys.quick_board || sys.gold_board) {
                        quick_scan_message_base(sys.quick_board, sys.gold_board, msg_list[start].area, 1);
                    }
                    else if (sys.pip_board) {
                        pip_scan_message_base(msg_list[start].area, 1);
                    }
                    else if (sys.squish) {
                        squish_scan_message_base(msg_list[start].area, sys.msg_path, 1);
                    }
                    else {
                        scan_message_base(msg_list[start].area, 1);
                    }
                    last_read_system = msg_list[start].area;
                    num_msg = last_msg = max_priv_mail;
                    first_msg = 0;
                }
            }
            else {
                break;
            }
        }
        else {
            break;
        }
    }

    lastread = msg_list[start].msg_num;
    last_mail = start;
}

void mail_read_backward(pause)
int pause;
{
    int start, i;

    start = last_mail;

    if (start > 1) {
        start--;
    }
    else {
        m_print(bbstxt[B_NO_MORE_MESSAGE]);
        return;
    }

    if (last_read_system != msg_list[start].area) {
        read_system(msg_list[start].area, 1);
        usr.msg = 0;
        if (sys.quick_board || sys.gold_board) {
            quick_scan_message_base(sys.quick_board, sys.gold_board, msg_list[start].area, 1);
        }
        else if (sys.pip_board) {
            pip_scan_message_base(msg_list[start].area, 1);
        }
        else if (sys.squish) {
            squish_scan_message_base(msg_list[start].area, sys.msg_path, 1);
        }
        else {
            scan_message_base(msg_list[start].area, 1);
        }

        usr.msg = last_read_system = msg_list[start].area;
        num_msg = last_msg = max_priv_mail;
        first_msg = 0;
    }

    for (;;) {
        if (sys.quick_board || sys.gold_board) {
            i = quick_read_message(msg_list[start].msg_num, pause, start);
        }
        else if (sys.pip_board) {
            i = pip_msg_read(sys.pip_board, msg_list[start].msg_num, 0, pause, start);
        }
        else if (sys.squish) {
            i = squish_read_message(msg_list[start].msg_num, pause, start);
        }
        else {
            i = read_message(msg_list[start].msg_num, pause, start);
        }

        if (!i) {
            if (start > 1) {
                start--;
                if (last_read_system != msg_list[start].area) {
                    read_system(msg_list[start].area, 1);
                    if (sys.quick_board || sys.gold_board) {
                        quick_scan_message_base(sys.quick_board, sys.gold_board, msg_list[start].area, 1);
                    }
                    else if (sys.pip_board) {
                        pip_scan_message_base(msg_list[start].area, 1);
                    }
                    else if (sys.squish) {
                        squish_scan_message_base(msg_list[start].area, sys.msg_path, 1);
                    }
                    else {
                        scan_message_base(msg_list[start].area, 1);
                    }
                    last_read_system = msg_list[start].area;
                    num_msg = last_msg = max_priv_mail;
                    first_msg = 0;
                }
            }
            else {
                break;
            }
        }
        else {
            break;
        }
    }

    lastread = msg_list[start].msg_num;
    last_mail = start;
}

void mail_read_nonstop()
{
    while (last_mail < max_priv_mail) {
        if (local_mode && local_kbd == 0x03) {
            break;
        }
        else if (!local_mode && !RECVD_BREAK()) {
            break;
        }
        mail_read_forward(1);
        time_release();
    }
}

void mail_list()
{
    int fd, m, z, line = 3, last_read_system = 0;
    char stringa[10], filename[80];
    struct _msg msgt;

    if (!get_command_word(stringa, 4)) {
        m_print(bbstxt[B_START_WITH_MSG]);

        input(stringa, 4);
        if (!strlen(stringa)) {
            return;
        }
    }

    m = atoi(stringa);
    if (m < 1 || m > last_msg) {
        return;
    }

    cls();

    for (z = m; z <= max_priv_mail; z++) {
        if (msg_list[z].area != last_read_system) {
            read_system(msg_list[z].area, 1);
            usr.msg = 0;
            if (sys.quick_board || sys.gold_board) {
                quick_scan_message_base(sys.quick_board, sys.gold_board, usr.msg, 1);
            }
            else if (sys.pip_board) {
                pip_scan_message_base(sys.quick_board, 1);
            }
            else if (sys.squish) {
                squish_scan_message_base(msg_list[z].area, sys.msg_path, 1);
            }
            else {
                scan_message_base(sys.msg_num, 1);
            }
            m_print(bbstxt[B_AREALIST_HEADER], sys.msg_num, sys.msg_name);
            if (bbstxt[B_AREALIST_HEADER][strlen(bbstxt[B_AREALIST_HEADER]) - 1] == '\r') {
                m_print(bbstxt[B_ONE_CR]);
            }
            usr.msg = last_read_system = msg_list[z].area;
            num_msg = last_msg = max_priv_mail;
        }

        if (sys.quick_board) {
            if (!(line = quick_mail_header(msg_list[z].msg_num, line, 0, z))) {
                break;
            }
        }
        else if (sys.gold_board) {
            if (!(line = quick_mail_header(msg_list[z].msg_num, line, 1, z))) {
                break;
            }
        }
        else if (sys.pip_board) {
            if (!(line = pip_mail_list_header(msg_list[z].msg_num, sys.pip_board, line, z))) {
                break;
            }
        }
        else if (sys.squish) {
            if (!(line = squish_mail_list_headers(msg_list[z].msg_num, line, z))) {
                break;
            }
        }
        else {
            sprintf(filename, "%s%d.MSG", sys.msg_path, msg_list[z].msg_num);

            fd = shopen(filename, O_RDONLY | O_BINARY);
            if (fd == -1) {
                continue;
            }
            read(fd, (char *)&msgt, sizeof(struct _msg));
            close(fd);

            if ((msgt.attr & MSGPRIVATE) && stricmp(msgt.from, usr.name) && stricmp(msgt.to, usr.name)
                    && stricmp(msgt.from, usr.handle) && stricmp(msgt.to, usr.handle) && usr.priv < SYSOP) {
                continue;
            }

            if ((line = msg_attrib(&msgt, z, line, 0)) == 0) {
                break;
            }
        }

        m_print(bbstxt[B_ONE_CR]);

        if (!(line = more_question(line)) || !CARRIER || RECVD_BREAK()) {
            break;
        }

        time_release();
    }

    if (line && CARRIER) {
        press_enter();
    }
}

static int quick_index(z, area_msg, area_num, maxareas)
int z, *area_msg, *area_num, maxareas;
{
#define MAX_TOIDX_READ  20
    FILE * fpi, *fpn;
    int i, m, fdhdr, x;
    word pos, currmsg[201];
    char filename[80], username[36], to[MAX_TOIDX_READ][36];
    byte boards[201];
    struct _msghdr hdr;
    struct _sys tsys;
    struct _msgidx idx[MAX_TOIDX_READ];

    for (i = 0; i < 201; i++) {
        boards[i] = 0;
        currmsg[i] = 0;
    }

    sprintf(filename, SYSMSG_PATH, config->sys_path);
    while ((i = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) == -1)
        ;
    fpi = fdopen(i, "rb");
    while (fread((char *)&tsys.msg_name, SIZEOF_MSGAREA, 1, fpi) == 1) {
        if (!tsys.quick_board) {
            continue;
        }
        if (usr.priv < tsys.msg_priv || (usr.flags & tsys.msg_flags) != tsys.msg_flags) {
            continue;
        }
        boards[tsys.quick_board] = tsys.msg_num;
    }
    fclose(fpi);

    sprintf(filename, "%sMSGHDR.BBS", fido_msgpath);
    fdhdr = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);
    if (fdhdr == -1) {
        return (z);
    }

    sprintf(filename, "%sMSGTOIDX.BBS", fido_msgpath);
    while ((i = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) == -1)
        ;
    fpi = fdopen(i, "rb");
    if (fpi == NULL) {
        close(fdhdr);
        return (z);
    }

    sprintf(filename, "%sMSGIDX.BBS", fido_msgpath);
    while ((i = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) == -1)
        ;
    fpn = fdopen(i, "rb");
    if (fpn == NULL) {
        close(fdhdr);
        fclose(fpi);
        return (z);
    }

    if (!local_mode) {
        _BRK_ENABLE();
    }
    pos = 0;

    do {
        i = fread(to, 36, MAX_TOIDX_READ, fpi);
        i = fread((char *)&idx, sizeof(struct _msgidx), MAX_TOIDX_READ, fpn);

        if (!local_mode) {
            if (!CARRIER || RECVD_BREAK()) {
                break;
            }
        }

        for (m = 0; m < i; m++) {
            if (!idx[m].msgnum || !boards[idx[m].board]) {
                pos++;
                continue;
            }

            currmsg[idx[m].board]++;
            strncpy(username, &to[m][1], to[m][0]);
            username[to[m][0]] = '\0';
            if (!stricmp(username, usr.name) || !stricmp(username, usr.handle)) {
                lseek(fdhdr, (long)pos * sizeof(struct _msghdr), SEEK_SET);
                read(fdhdr, (char *)&hdr, sizeof(struct _msghdr));

                strncpy(username, &hdr.whoto[1], hdr.whoto[0]);
                username[hdr.whoto[0]] = '\0';
                if ((!stricmp(username, usr.name) || !stricmp(username, usr.handle)) && hdr.board == idx[m].board) {
                    if (boards[hdr.board] && !(hdr.msgattr & Q_RECEIVED) && !(hdr.msgattr & Q_RECKILL)) {
                        if (z >= MAX_PRIV_MAIL) {
                            break;
                        }

                        msg_list[z].area = boards[hdr.board];
                        msg_list[z++].msg_num = currmsg[hdr.board];

                        for (x = 0; x < maxareas; x++)
                            if (area_num[x] == boards[hdr.board]) {
                                break;
                            }
                        area_msg[x]++;
                    }
                }
            }
            pos++;
            if ((pos % 150) == 0) {
                m_print2("\b%c", rotate[rotatepos]);
                rotatepos = (rotatepos + 1) % 4;
            }
        }
    } while (i == MAX_TOIDX_READ);

    close(fdhdr);
    fclose(fpn);
    fclose(fpi);

    return (z);
}

static int gold_index(z, area_msg, area_num, maxareas)
int z, *area_msg, *area_num, maxareas;
{
#define MAX_TOIDX_READ  20
    FILE * fpi, *fpn;
    int i, m, fdhdr, x;
    word pos, currmsg[501];
    char filename[80], username[36], to[MAX_TOIDX_READ][36];
    int boards[501];
    struct _gold_msghdr hdr;
    struct _sys tsys;
    struct _gold_msgidx idx[MAX_TOIDX_READ];

    for (i = 0; i < 501; i++) {
        boards[i] = 0;
        currmsg[i] = 0;
    }

    sprintf(filename, SYSMSG_PATH, config->sys_path);
    while ((i = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) == -1)
        ;
    fpi = fdopen(i, "rb");
    while (fread((char *)&tsys.msg_name, SIZEOF_MSGAREA, 1, fpi) == 1) {
        if (!tsys.gold_board) {
            continue;
        }
        if (usr.priv < tsys.msg_priv || (usr.flags & tsys.msg_flags) != tsys.msg_flags) {
            continue;
        }
        boards[tsys.gold_board] = tsys.msg_num;
    }
    fclose(fpi);

    sprintf(filename, "%sMSGHDR.DAT", fido_msgpath);
    fdhdr = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);
    if (fdhdr == -1) {
        return (z);
    }

    sprintf(filename, "%sMSGTOIDX.DAT", fido_msgpath);
    while ((i = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) == -1)
        ;
    fpi = fdopen(i, "rb");
    if (fpi == NULL) {
        close(fdhdr);
        return (z);
    }

    sprintf(filename, "%sMSGIDX.DAT", fido_msgpath);
    while ((i = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) == -1)
        ;
    fpn = fdopen(i, "rb");
    if (fpn == NULL) {
        close(fdhdr);
        fclose(fpi);
        return (z);
    }

    if (!local_mode) {
        _BRK_ENABLE();
    }
    pos = 0;

    do {
        i = fread(to, 36, MAX_TOIDX_READ, fpi);
        i = fread((char *)&idx, sizeof(struct _gold_msgidx), MAX_TOIDX_READ, fpn);

        if (!local_mode) {
            if (!CARRIER || RECVD_BREAK()) {
                break;
            }
        }

        for (m = 0; m < i; m++) {
            if (!idx[m].msgnum || !boards[idx[m].board]) {
                pos++;
                continue;
            }

            currmsg[idx[m].board]++;
            strncpy(username, &to[m][1], to[m][0]);
            username[to[m][0]] = '\0';
            if (!stricmp(username, usr.name) || !stricmp(username, usr.handle)) {
                lseek(fdhdr, (long)pos * sizeof(struct _gold_msghdr), SEEK_SET);
                read(fdhdr, (char *)&hdr, sizeof(struct _gold_msghdr));

                strncpy(username, &hdr.whoto[1], hdr.whoto[0]);
                username[hdr.whoto[0]] = '\0';
                if ((!stricmp(username, usr.name) || !stricmp(username, usr.handle)) && hdr.board == idx[m].board) {
                    if (boards[hdr.board] && !(hdr.msgattr & Q_RECEIVED) && !(hdr.msgattr & Q_RECKILL)) {
                        if (z >= MAX_PRIV_MAIL) {
                            break;
                        }

                        msg_list[z].area = boards[hdr.board];
                        msg_list[z++].msg_num = currmsg[hdr.board];

                        for (x = 0; x < maxareas; x++)
                            if (area_num[x] == boards[hdr.board]) {
                                break;
                            }
                        area_msg[x]++;
                    }
                }
            }
            pos++;
            if ((pos % 150) == 0) {
                m_print2("\b%c", rotate[rotatepos]);
                rotatepos = (rotatepos + 1) % 4;
            }
        }
    } while (i == MAX_TOIDX_READ);

    close(fdhdr);
    fclose(fpn);
    fclose(fpi);

    return (z);
}

static int pipbase_index(z, area_msg, area_num, maxareas)
int z, *area_msg, *area_num, maxareas;
{
    FILE * fpi;
    int f1, i, *boards, oldboard, x, maxboards, xx;
    char filename[80];
    DESTPTR hdr;
    MSGPTR mhdr;
    struct _sys tsys;

    sprintf(filename, SYSMSG_PATH, config->sys_path);
    while ((i = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) == -1)
        ;

    x = (int)(filelength(i) / SIZEOF_MSGAREA);
    if ((boards = (int *)malloc(x * sizeof(int))) == NULL) {
        close(i);
        return (z);
    }
    memset(boards, 0, x * sizeof(int));

    fpi = fdopen(i, "rb");
    x = 0;

    while (fread((char *)&tsys.msg_name, SIZEOF_MSGAREA, 1, fpi) == 1) {
        boards[x++] = tsys.pip_board;
    }
    maxboards = x;

    fclose(fpi);

    sprintf(filename, "%sDESTPTR.PIP", pip_msgpath);
    i = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);
    if (i == -1) {
        free(boards);
        return (z);
    }
    fpi = fdopen(i, "rb");
    if (fpi == NULL) {
        free(boards);
        return (z);
    }

    if (!local_mode) {
        _BRK_ENABLE();
    }

    i = 0;
    f1 = oldboard = -1;

    while (fread((char *)&hdr, sizeof(DESTPTR), 1, fpi) == 1) {
        if (!local_mode) {
            if (!CARRIER || RECVD_BREAK()) {
                break;
            }
        }

        i++;
        if ((i % 30) == 0) {
            m_print2("\b%c", rotate[rotatepos]);
            rotatepos = (rotatepos + 1) % 4;
        }

        for (xx = 0; xx < maxboards; xx++)
            if (boards[xx] == hdr.area) {
                break;
            }

        if (xx >= maxboards) {
            continue;
        }

        if (!read_system2(area_num[xx], 1, &tsys)) {
            continue;
        }
        if (usr.priv < tsys.msg_priv || (usr.flags & tsys.msg_flags) != tsys.msg_flags) {
            continue;
        }

        if (!stricmp(usr.name, hdr.to) || !stricmp(usr.handle, hdr.to)) {
            if (oldboard != hdr.area) {
                if (f1 != -1) {
                    close(f1);
                }
                sprintf(filename, "%sMPTR%04x.PIP", pip_msgpath, hdr.area);
                f1 = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);
                oldboard = hdr.area;
            }

            if (f1 != -1) {
                if (lseek(f1, sizeof(mhdr) * hdr.msg, SEEK_SET)) {
                    read(f1, &mhdr, sizeof(mhdr));
                }
                if (mhdr.status & (SET_MPTR_RCVD | SET_MPTR_DEL)) {
                    continue;
                }
            }
            else {
                continue;
            }

            if (z >= MAX_PRIV_MAIL) {
                break;
            }

            msg_list[z].area = boards[xx];
            msg_list[z++].msg_num = hdr.msg + 1;
            area_msg[xx]++;
        }
    }

    fclose(fpi);
    if (f1 != -1) {
        close(f1);
    }

    free(boards);

    return (z);
}

static int fido_index(z, area_msg, area_num, maxareas)
int z, *area_msg, *area_num, maxareas;
{
    FILE * fp;
    int i, m, fd, ct = 0, x;
    char filename[50];
    long crcalias;
    struct _mail idxm[MAX_MAIL_BUFFER];
    struct _sys tsys;
    struct _msg tmsg;

    crcalias = crc_name(usr.handle);

    sprintf(filename, "%sMSGTOIDX.DAT", config->sys_path);
    i = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);
    if (i == -1) {
        return (z);
    }
    fp = fdopen(i, "rb");

    if (fp != NULL) {
        if (!local_mode) {
            _BRK_ENABLE();
        }

        do {
            i = fread((char *)&idxm, sizeof(struct _mail), MAX_MAIL_BUFFER, fp);

            if (!local_mode) {
                if (!CARRIER || RECVD_BREAK()) {
                    break;
                }
            }

            for (m = 0; m < i; m++) {
                if (idxm[m].to != usr.id && idxm[m].to != crcalias) {
                    continue;
                }
                if (!idxm[m].area || !read_system2(idxm[m].area, 1, &tsys)) {
                    continue;
                }
                if (usr.priv < tsys.msg_priv || (usr.flags & tsys.msg_flags) != tsys.msg_flags) {
                    continue;
                }

                for (x = 0; x < maxareas; x++)
                    if (area_num[x] == tsys.msg_num) {
                        break;
                    }

                if (!local_mode) {
                    if (!CARRIER || RECVD_BREAK()) {
                        i = 0;
                        break;
                    }
                }

                if (z >= MAX_PRIV_MAIL) {
                    i = 0;
                    break;
                }

                sprintf(filename, "%s%d.MSG", tsys.msg_path, idxm[m].msg_num);
                fd = shopen(filename, O_RDONLY | O_BINARY);
                if (fd == -1) {
                    continue;
                }
                read(fd, (char *)&tmsg, sizeof(struct _msg));
                close(fd);

                if ((stricmp(tmsg.to, usr.name) && stricmp(tmsg.to, usr.handle)) || (tmsg.attr & MSGREAD)) {
                    continue;
                }

                msg_list[z].area = idxm[m].area;
                msg_list[z++].msg_num = idxm[m].msg_num;
                area_msg[x]++;
            }

            if ((ct++ % 10) == 0) {
                m_print2("\b%c", rotate[rotatepos]);
                rotatepos = (rotatepos + 1) % 4;
            }
        } while (i == MAX_MAIL_BUFFER && z < MAX_PRIV_MAIL);

        fclose(fp);
    }

    return (z);
}

#define MAX_SQIDX  32

typedef struct _sqidx
{
    unsigned long offs;     /* offset del messaggio nel file dati */
    unsigned long msgid;    /* unique msgid */
    unsigned long in;       /* hash del destinatario */
} SQIDX;

static int squish_index(z, area_msg, area_num, maxareas)
int z, *area_msg, *area_num, maxareas;
{
    int fdidx, fd, i, x;
    unsigned int max;
    char filename[80];
    dword hash, msgn, hashalias;
    struct _sys tsys;
    MSGH * sq_msgh;
    XMSG xmsg;
    SQIDX * sqidx;

    hash = SquishHash(usr.name);
    hashalias = SquishHash(usr.handle);

    sprintf(filename, SYSMSG_PATH, config->sys_path);
    while ((fd = sopen(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) == -1)
        ;

    while (read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
        if (!tsys.squish) {
            continue;
        }
        if (usr.priv < tsys.msg_priv || (usr.flags & tsys.msg_flags) != tsys.msg_flags) {
            continue;
        }

        if (sq_ptr != NULL) {
            MsgUnlock(sq_ptr);
            MsgCloseArea(sq_ptr);
            sq_ptr = NULL;
        }

        for (x = 0; x < maxareas; x++)
            if (area_num[x] == tsys.msg_num) {
                break;
            }

        m_print2("\b%c", rotate[rotatepos]);
        rotatepos = (rotatepos + 1) % 4;

        sprintf(filename, "%s.SQI", tsys.msg_path);
        fdidx = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);
        if (fdidx == -1) {
            continue;
        }

        sqidx = (SQIDX *)malloc((unsigned int)filelength(fdidx));
        if (sqidx == NULL) {
            close(fdidx);
            continue;
        }

        max = read(fdidx, sqidx, (unsigned int)filelength(fdidx));
        close(fdidx);

        max /= sizeof(SQIDX);
        for (i = 0; i < max; i++) {
            if (sqidx[i].in == hash || sqidx[i].in == hashalias) {
                if (sq_ptr == NULL) {
                    sq_ptr = MsgOpenArea(tsys.msg_path, MSGAREA_NORMAL, MSGTYPE_SQUISH);
                    if (sq_ptr == NULL) {
                        continue;
                    }
                }

                msgn = MsgUidToMsgn(sq_ptr, sqidx[i].msgid, UID_EXACT);
                if (msgn == 0) {
                    continue;
                }

                sq_msgh = MsgOpenMsg(sq_ptr, MOPEN_RW, msgn);
                if (sq_msgh == NULL) {
                    continue;
                }

                if (MsgReadMsg(sq_msgh, &xmsg, 0L, 0L, NULL, 0, NULL) == -1) {
                    MsgCloseMsg(sq_msgh);
                    continue;
                }

                MsgCloseMsg(sq_msgh);

                if ((!stricmp(xmsg.to, usr.name) || !stricmp(xmsg.to, usr.handle)) && !(xmsg.attr & MSGREAD)) {
                    if (z >= MAX_PRIV_MAIL) {
                        break;
                    }

                    msg_list[z].area = tsys.msg_num;
                    msg_list[z++].msg_num = (int)msgn;
                    area_msg[x]++;
                }
            }
        }

        free(sqidx);

        if (sq_ptr != NULL) {
            MsgCloseArea(sq_ptr);
            sq_ptr = NULL;
        }
    }

    close(fd);

    if (sq_ptr != NULL) {
        MsgUnlock(sq_ptr);
        MsgCloseArea(sq_ptr);
        sq_ptr = NULL;
    }

    return (z);
}


