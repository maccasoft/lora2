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
#include <dos.h>
#include <io.h>
#include <share.h>
#include <alloc.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"
#include "msgapi.h"

void display_cpu(void);

static void is_4dos(void);
static void compilation_data(void);

#define MAX_INDEX    200

void software_version(char * arguments)
{
    unsigned char i;
#ifndef __OS2__
    unsigned char c;
    union REGS inregs, outregs;
#endif
    unsigned pp;
    float t, u;
    struct diskfree_t df;

    cls();

    change_attr(LMAGENTA | _BLACK);
    m_print("%s - The Computer-Based Information System\n", VERSION);
    m_print("CopyRight (c) 1989-96 by Marco Maccaferri. All Rights Reserved.\n");
#ifdef __OCC__
    m_print("CopyRight (c) 1995 Old Colorado City Communications. All Rights Reserved.\n");
#endif

    change_attr(LCYAN | _BLACK);
    m_print("\nJanus revision 0.31 - (C) Copyright 1987-90, Bit Bucket Software Co.\n");
    m_print("MsgAPI - Copyright 1990, 1991 by Scott J. Dudley.  All rights reserved.\n");
    m_print("BlueWave Offline Mail System. Copyright 1990-94 by Cutting Edge Computing\n");

    compilation_data();

    activation_key();
    if (registered) {
        m_print("\026\001\003Registered to: %s\n               %s\n\n", config->sysop, config->system_name);
    }

#ifndef __OS2__
    c = peekb(0xFFFF, 0x000E);

    change_attr(WHITE | _BLACK);
    m_print(bbstxt[B_COMPUTER]);
    switch ((byte)c) {
        case 0xFD:
            m_print(bbstxt[B_TYPE_PCJR]);
            break;
        case 0xFE:
            m_print(bbstxt[B_TYPE_XT]);
            break;
        case 0xFF:
            m_print(bbstxt[B_TYPE_PC]);
            break;
        case 0xFC:
            m_print(bbstxt[B_TYPE_AT]);
            break;
        case 0xFA:
            m_print(bbstxt[B_TYPE_PS230]);
            break;
        case 0xF9:
            m_print(bbstxt[B_TYPE_PCCONV]);
            break;
        case 0xF8:
            m_print(bbstxt[B_TYPE_PS280]);
            break;
        case 0x9A:
        case 0x2D:
            m_print(bbstxt[B_TYPE_COMPAQ]);
            break;
        default:
            m_print(bbstxt[B_TYPE_GENERIC]);
            break;
    }
#else
    change_attr(WHITE | _BLACK);
    m_print(bbstxt[B_COMPUTER]);
    m_print(bbstxt[B_TYPE_GENERIC]);
#endif

    display_cpu();

#ifdef __OS2__
    m_print(bbstxt[B_OS_OS2], _osmajor / 10, _osminor);
    m_print(bbstxt[B_ONE_CR]);
    m_print(bbstxt[B_ONE_CR]);
#else
    inregs.h.ah = 0x30;
    intdos(&inregs, &outregs);
    if (outregs.h.al >= 20) {
        m_print(bbstxt[B_OS_OS2], outregs.h.al / 10, outregs.h.ah);
    }
    else {
        m_print(bbstxt[B_OS_DOS], outregs.h.al, outregs.h.ah);
    }
    is_4dos();

    fossil_version2();

    change_attr(LGREEN | _BLACK);
    m_print(bbstxt[B_HEAP_RAM], coreleft());
#endif

    if (strstr(arguments, "/NDSK") == NULL) {
        if (!_dos_getdiskfree(3, &df)) {
            t = (float)df.total_clusters * df.bytes_per_sector * df.sectors_per_cluster / 1048576L;
            u = (float)df.avail_clusters * df.bytes_per_sector * df.sectors_per_cluster / 1048576L;
            pp = (unsigned)(u * 100 / t);
            m_print(bbstxt[B_GENERAL_FREE1], u, t, pp);
        }

        for (i = 4; i <= 26; i++) {
            if (!_dos_getdiskfree(i, &df)) {
                t = (float)df.total_clusters * df.bytes_per_sector * df.sectors_per_cluster / 1048576L;
                u = (float)df.avail_clusters * df.bytes_per_sector * df.sectors_per_cluster / 1048576L;
                pp = (unsigned)(u * 100 / t);
                m_print(bbstxt[B_GENERAL_FREE2], i + 64, u, t, pp);
            }
        }
    }

    m_print(bbstxt[B_ONE_CR]);

    press_enter();
}

void is_4dos()
{
#ifndef __OS2__
    union REGS inregs, outregs;

    inregs.x.ax = 0xD44D;
    inregs.x.bx = 0;
    int86(0x2F, &inregs, &outregs);

    if (outregs.x.ax == 0x44DD) {
        m_print(bbstxt[B_DOS_4DOS], outregs.h.bl, outregs.h.bh);
    }
#endif
}

void display_area_list(type, flag, sig)   /* flag == 1, Normale due colonne */
int type, flag, sig;                      /* flag == 2, Normale una colonna */
{   /* flag == 3, Anche nomi, una colonna */
    int fd, fdi, i, pari, area, linea, nsys, nm;
    char stringa[80], filename[50], dir[80], first;
    struct _sys tsys;
    struct _sys_idx sysidx[10];

    memset(&tsys, 0, sizeof(struct _sys));

    if (!type) {
        return;
    }

    first = 1;

    if (sig == -1) {
        if (type == 1) {
            read_system_file("MGROUP");
        }
        else if (type == 2) {
            read_system_file("FGROUP");
        }

        do {
            m_print(bbstxt[B_GROUP_LIST]);
            chars_input(stringa, 4, INPUT_FIELD);
            if ((sig = atoi(stringa)) == 0) {
                return;
            }
        } while (sig < 0);

        if (type == 1) {
            usr.msg_sig = sig;
        }
        else if (type == 2) {
            usr.file_sig = sig;
        }
    }

    if (type == 1) {
        sprintf(filename, "%sSYSMSG.DAT", config->sys_path);
        area = usr.msg;
    }
    else if (type == 2) {
        sprintf(filename, "%sSYSFILE.DAT", config->sys_path);
        area = usr.files;
    }
    else {
        return;
    }

    fd = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD);
    if (fd == -1) {
        return;
    }

    if (type == 1) {
        sprintf(filename, "%sSYSMSG.IDX", config->sys_path);
        area = usr.msg;
    }
    else if (type == 2) {
        sprintf(filename, "%sSYSFILE.IDX", config->sys_path);
        area = usr.files;
    }

    fdi = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD);
    if (fdi == -1) {
        close(fd);
        return;
    }

    do {
        if (!get_command_word(stringa, 12)) {
            if (type == 1) {
                m_print(bbstxt[B_SELECT_AREAS], bbstxt[B_MESSAGE]);
            }
            else if (type == 2) {
                m_print(bbstxt[B_SELECT_AREAS], bbstxt[B_FILE]);
            }
            input(stringa, 12);
            if (!CARRIER || time_remain() <= 0) {
                close(fdi);
                return;
            }
        }

        if (!stringa[0] && first) {
            first = 0;
            stringa[0] = area_change_key[2];
        }

        if (stringa[0] == area_change_key[2] || (!stringa[0] && !area)) {
            cls();
            m_print(bbstxt[B_AREAS_TITLE], (type == 1) ? bbstxt[B_MESSAGE] : bbstxt[B_FILE]);

            first = 0;
            pari = 0;
            linea = 4;
            nm = 0;
            lseek(fdi, 0L, SEEK_SET);

            do {
                nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
                nsys /= sizeof(struct _sys_idx);

                for (i = 0; i < nsys; i++, nm++) {
                    if (usr.priv < sysidx[i].priv || (usr.flags & sysidx[i].flags) != sysidx[i].flags) {
                        continue;
                    }

                    if (sig) {
                        if (sysidx[i].sig != sig) {
                            continue;
                        }
                    }

                    if (type == 1) {
                        if (read_system_file("MSGAREA")) {
                            break;
                        }

                        lseek(fd, (long)nm * SIZEOF_MSGAREA, SEEK_SET);
                        read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA);

                        if (sig || tsys.msg_restricted) {
                            if (tsys.msg_sig != sig) {
                                continue;
                            }
                        }

                        if (flag == 1) {
                            strcpy(dir, tsys.msg_name);
                            dir[31] = '\0';

                            sprintf(stringa, "\026\001\020\215%3d ... \026\001\003%-31s ", sysidx[i].area, dir);

                            if (pari) {
                                m_print("%s\n", strtrim(stringa));
                                pari = 0;
                                if (!(linea = more_question(linea))) {
                                    break;
                                }
                            }
                            else {
                                m_print(stringa);
                                pari = 1;
                            }
                        }
                        else if (flag == 2) {
                            m_print("\026\001\020\215%3d ... \026\001\003%s\n", sysidx[i].area, tsys.msg_name);
                            if (!(linea = more_question(linea))) {
                                break;
                            }
                        }
                        else if (flag == 3) {
                            m_print("\026\001\020\215%3d ... \026\001\013%-12s \026\001\003%s\n", sysidx[i].area, sysidx[i].key, tsys.msg_name);
                            if (!(linea = more_question(linea))) {
                                break;
                            }
                        }
                        else if (flag == 4) {
                            if (!sysidx[i].key[0]) {
                                sprintf(sysidx[i].key, "%d", sysidx[i].area);
                            }
                            m_print("\026\001\020\215%-12s ... \026\001\003%s\n", sysidx[i].key, tsys.msg_name);
                            if (!(linea = more_question(linea))) {
                                break;
                            }
                        }
                    }
                    else if (type == 2) {
                        if (read_system_file("FILEAREA")) {
                            break;
                        }

                        lseek(fd, (long)nm * SIZEOF_FILEAREA, SEEK_SET);
                        read(fd, (char *)&tsys.file_name, SIZEOF_FILEAREA);

                        if (sig || tsys.file_restricted) {
                            if (tsys.file_sig != sig) {
                                continue;
                            }
                        }

                        if (flag == 1) {
                            strcpy(dir, tsys.file_name);
                            dir[31] = '\0';

                            sprintf(stringa, "\026\001\020\215%3d ... \026\001\003%-31s ", sysidx[i].area, dir);

                            if (pari) {
                                m_print("%s\n", strtrim(stringa));
                                pari = 0;
                                if (!(linea = more_question(linea))) {
                                    break;
                                }
                            }
                            else {
                                m_print(stringa);
                                pari = 1;
                            }
                        }
                        else if (flag == 2) {
                            m_print("\026\001\020\215%3d ... \026\001\003%s\n", sysidx[i].area, tsys.file_name);
                            if (!(linea = more_question(linea))) {
                                break;
                            }
                        }
                        else if (flag == 3) {
                            m_print("\026\001\020\215%3d ... \026\001\013%-12s \026\001\003%s\n", sysidx[i].area, sysidx[i].key, tsys.file_name);
                            if (!(linea = more_question(linea))) {
                                break;
                            }
                        }
                        else if (flag == 4) {
                            if (!sysidx[i].key[0]) {
                                sprintf(sysidx[i].key, "%d", sysidx[i].area);
                            }
                            m_print("\026\001\020\215%-12s ... \026\001\003%s\n", sysidx[i].key, tsys.file_name);
                            if (!(linea = more_question(linea))) {
                                break;
                            }
                        }
                    }
                }
            } while (linea && nsys == 10);

            if (pari) {
                m_print(bbstxt[B_ONE_CR]);
            }
            area = -1;
        }
        else if (stringa[0] == area_change_key[1]) {
            lseek(fdi, 0L, SEEK_SET);

            do {
                nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
                nsys /= sizeof(struct _sys_idx);

                for (i = 0; i < nsys; i++)
                    if (sysidx[i].area == area) {
                        break;
                    }
            } while (i == nsys && nsys == 10);

            if (i == nsys) {
                continue;
            }

            for (;;) {
                i++;
                if (i >= nsys) {
                    i = 0;
                    nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
                    if (nsys == 0) {
                        lseek(fdi, 0L, SEEK_SET);
                        nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
                    }
                    nsys /= sizeof(struct _sys_idx);
                    i = 0;
                }
                if (sig) {
                    if (sysidx[i].sig != sig) {
                        continue;
                    }
                }
                if (sysidx[i].area == area) {
                    break;
                }
                if (usr.priv >= sysidx[i].priv && (usr.flags & sysidx[i].flags) == sysidx[i].flags) {
                    break;
                }
            }

            area = sysidx[i].area;
        }
        else if (stringa[0] == area_change_key[0]) {
            lseek(fdi, 0L, SEEK_SET);

            do {
                nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
                nsys /= sizeof(struct _sys_idx);

                for (i = 0; i < nsys; i++)
                    if (sysidx[i].area == area) {
                        break;
                    }
            } while (i == nsys && nsys == 10);

            if (i == nsys) {
                continue;
            }

            for (;;) {
                i--;
                if (i < 0) {
                    if (lseek(fdi, tell(fdi) - (10L + (long)nsys) * sizeof(struct _sys_idx), SEEK_SET) == -1L) {
                        lseek(fdi, (long)nsys * sizeof(struct _sys_idx), SEEK_END);
                    }

                    nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
                    nsys /= sizeof(struct _sys_idx);
                    i = nsys - 1;
                    if (i < 0) {
                        break;
                    }
                }
                if (sig) {
                    if (sysidx[i].sig != sig) {
                        continue;
                    }
                }
                if (sysidx[i].area == area) {
                    break;
                }
                if (usr.priv >= sysidx[i].priv && (usr.flags & sysidx[i].flags) == sysidx[i].flags) {
                    break;
                }
            }

            area = sysidx[i].area;
        }
        else if (strlen(stringa) < 1 && read_system(usr.msg, type)) {
            close(fdi);
            close(fd);
            return;
        }
        else {
            lseek(fdi, 0L, SEEK_SET);
            area = atoi(stringa);
            if (area < 1) {
                area = -1;
                do {
                    nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
                    nsys /= sizeof(struct _sys_idx);

                    for (i = 0; i < nsys; i++) {
                        if (usr.priv < sysidx[i].priv || (usr.flags & sysidx[i].flags) != sysidx[i].flags) {
                            continue;
                        }
                        if (sig) {
                            if (sysidx[i].sig != sig) {
                                continue;
                            }
                        }
                        if (!stricmp(strbtrim(stringa), strbtrim(sysidx[i].key))) {
                            area = sysidx[i].area;
                            break;
                        }
                    }
                } while (i == nsys && nsys == 10);
            }
            else {
                do {
                    nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
                    nsys /= sizeof(struct _sys_idx);

                    for (i = 0; i < nsys; i++) {
                        if (usr.priv < sysidx[i].priv || (usr.flags & sysidx[i].flags) != sysidx[i].flags) {
                            continue;
                        }
                        if (sig) {
                            if (sysidx[i].sig != sig) {
                                continue;
                            }
                        }
                        if (sysidx[i].area == area) {
                            break;
                        }
                    }
                } while (i == nsys && nsys == 10);

                if (i == nsys) {
                    area = -1;
                }
            }
        }
    } while (area == -1 || !read_system(area, type));

    close(fdi);
    close(fd);

    if (type == 1) {
        status_line(msgtxt[M_BBS_EXIT], area, sys.msg_name);
        usr.msg = area;
    }
    else if (type == 2) {
        status_line(msgtxt[M_BBS_SPAWN], area, sys.file_name);
        usr.files = area;
    }
}

void display_new_area_list(sig)
int sig;
{
    int fd, fdi, i, pari, area, linea, nsys, nm, first;
    char stringa[13], filename[50];
    struct _sys_idx sysidx[10];

    first = 1;

    sprintf(filename, "%sSYSMSG.DAT", config->sys_path);
    fd = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD);
    if (fd == -1) {
        return;
    }

    sprintf(filename, "%sSYSMSG.IDX", config->sys_path);
    area = usr.msg;
    fdi = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD);
    if (fdi == -1) {
        close(fd);
        return;
    }

    do {
        if (!get_command_word(stringa, 12)) {
            m_print(bbstxt[B_SELECT_AREAS], bbstxt[B_MESSAGE]);
            input(stringa, 12);
            if (!CARRIER || time_remain() <= 0) {
                close(fdi);
                return;
            }
        }

        if (!stringa[0] && first) {
            first = 0;
            stringa[0] = area_change_key[2];
        }

        if (stringa[0] == area_change_key[2] || (!stringa[0] && !area)) {
            cls();
            m_print(bbstxt[B_AREAS_TITLE], bbstxt[B_MESSAGE]);

            first = 0;
            pari = 0;
            linea = 4;
            nm = 0;
            lseek(fdi, 0L, SEEK_SET);

            do {
                nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
                nsys /= sizeof(struct _sys_idx);

                for (i = 0; i < nsys; i++, nm++) {
                    if (usr.priv < sysidx[i].priv || (usr.flags & sysidx[i].flags) != sysidx[i].flags) {
                        continue;
                    }
                    if (sig) {
                        if (sysidx[i].sig != sig) {
                            continue;
                        }
                    }

                    lseek(fd, (long)nm * SIZEOF_MSGAREA, SEEK_SET);
                    read(fd, (char *)&sys.msg_name, SIZEOF_MSGAREA);

                    if (sig) {
                        if (sys.msg_sig != sig) {
                            continue;
                        }
                    }

                    if (sys.quick_board || sys.gold_board) {
                        quick_scan_message_base(sys.quick_board, sys.gold_board, sys.msg_num, 0);
                    }
                    else if (sys.pip_board) {
                        pip_scan_message_base(sys.msg_num, 0);
                    }
                    else if (sys.squish) {
                        squish_scan_message_base(sys.msg_num, sys.msg_path, 0);
                    }
                    else {
                        scan_message_base(sys.msg_num, 0);
                    }

                    if (last_msg > lastread) {
                        m_print("\026\001\020\215%3d ... \026\001\003 %s\n", sysidx[i].area, sys.msg_name);

                        if (!(linea = more_question(linea))) {
                            break;
                        }
                    }
                }
            } while (linea && nsys == 10);

            if (pari) {
                m_print(bbstxt[B_ONE_CR]);
            }
            area = -1;
        }
        else if (stringa[0] == area_change_key[1]) {
            lseek(fdi, 0L, SEEK_SET);

            do {
                nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
                nsys /= sizeof(struct _sys_idx);

                for (i = 0; i < nsys; i++)
                    if (sysidx[i].area == area) {
                        break;
                    }
            } while (i == nsys && nsys == 10);

            if (i == nsys) {
                continue;
            }

            do {
                i++;
                if (i >= nsys) {
                    i = 0;
                    nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
                    if (nsys == 0) {
                        lseek(fdi, 0L, SEEK_SET);
                        nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
                    }
                    nsys /= sizeof(struct _sys_idx);
                }
                if (sysidx[i].area == area) {
                    break;
                }
            } while (usr.priv < sysidx[i].priv || (usr.flags & sysidx[i].flags) != sysidx[i].flags);

            area = sysidx[i].area;
        }
        else if (stringa[0] == area_change_key[0]) {
            lseek(fdi, 0L, SEEK_SET);

            do {
                nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
                nsys /= sizeof(struct _sys_idx);

                for (i = 0; i < nsys; i++)
                    if (sysidx[i].area == area) {
                        break;
                    }
            } while (i == nsys && nsys == 10);

            if (i == nsys) {
                continue;
            }

            do {
                i--;
                if (i < 0) {
                    if (lseek(fdi, tell(fdi) - (10L + (long)nsys) * sizeof(struct _sys_idx), SEEK_SET) == -1L) {
                        lseek(fdi, (long)nsys * sizeof(struct _sys_idx), SEEK_END);
                    }

                    nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
                    nsys /= sizeof(struct _sys_idx);
                    i = nsys - 1;
                    if (i < 0) {
                        break;
                    }
                }
                if (sysidx[i].area == area) {
                    break;
                }
            } while (usr.priv < sysidx[i].priv || (usr.flags & sysidx[i].flags) != sysidx[i].flags);

            area = sysidx[i].area;
        }
        else if (strlen(stringa) < 1 && read_system(usr.msg, 1)) {
            close(fdi);
            close(fd);
            return;
        }
        else {
            lseek(fdi, 0L, SEEK_SET);
            area = atoi(stringa);
            if (area < 1) {
                area = -1;
                do {
                    nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
                    nsys /= sizeof(struct _sys_idx);

                    for (i = 0; i < nsys; i++) {
                        if (usr.priv < sysidx[i].priv || (usr.flags & sysidx[i].flags) != sysidx[i].flags) {
                            continue;
                        }
                        if (sig) {
                            if (sysidx[i].sig != sig) {
                                continue;
                            }
                        }
                        if (!stricmp(strbtrim(stringa), strbtrim(sysidx[i].key))) {
                            area = sysidx[i].area;
                            break;
                        }
                    }
                } while (i == nsys && nsys == 10);
            }
            else {
                do {
                    nsys = read(fdi, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
                    nsys /= sizeof(struct _sys_idx);

                    for (i = 0; i < nsys; i++) {
                        if (usr.priv < sysidx[i].priv || (usr.flags & sysidx[i].flags) != sysidx[i].flags) {
                            continue;
                        }
                        if (sig) {
                            if (sysidx[i].sig != sig) {
                                continue;
                            }
                        }
                        if (sysidx[i].area == area) {
                            break;
                        }
                    }
                } while (i == nsys && nsys == 10);

                if (i == nsys) {
                    area = -1;
                }
            }
        }
    } while (area == -1 || !read_system(area, 1));

    close(fdi);
    close(fd);

    status_line(msgtxt[M_BBS_EXIT], area, sys.msg_name);
    usr.msg = area;
}

int read_system(int s, int type)
{
    int fd, nsys, i, mn = 0;
    char filename[50];
    struct _sys_idx sysidx[10];
    struct _sys tsys;
    MSG * ptr;
    offline_reader =  0;

    if (type != 1 && type != 2) {
        return (0);
    }

    if (sq_ptr != NULL) {
        MsgCloseArea(sq_ptr);
        sq_ptr = NULL;
    }

    if (type == 1) {
        sprintf(filename, "%sSYSMSG.IDX", config->sys_path);
    }
    else if (type == 2) {
        sprintf(filename, "%sSYSFILE.IDX", config->sys_path);
    }

    fd = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD);
    if (fd == -1) {
        return (0);
    }

    do {
        nsys = read(fd, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
        nsys /= sizeof(struct _sys_idx);
        for (i = 0; i < nsys; i++, mn++) {
            if (sysidx[i].area == s) {
                break;
            }
        }
    } while (i == nsys && nsys == 10);

    close(fd);

    if (i == nsys) {
        return (0);
    }
//   if (usr.name[0] && (sysidx[i].priv > usr.priv || (usr.flags & sysidx[i].flags) != sysidx[i].flags))
//      return (0);

    i = mn;

    if (type == 1) {
        sprintf(filename, SYSMSG_PATH, config->sys_path);
        fd = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD);
        if (fd == -1) {
            return (0);
        }
        lseek(fd, (long)i * SIZEOF_MSGAREA, SEEK_SET);
        read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA);

        if (usr.priv < tsys.msg_priv) {
            close(fd);
            return (0);
        }
        if ((usr.flags & tsys.msg_flags) != tsys.msg_flags) {
            close(fd);
            return (0);
        }

        lseek(fd, (long)i * SIZEOF_MSGAREA, SEEK_SET);
        read(fd, (char *)&sys.msg_name, SIZEOF_MSGAREA);
        close(fd);
//      memcpy (&sys, &tsys, SIZEOF_MSGAREA);

        if (sys.pip_board) {
            sprintf(filename, "%sMPTR%04x.PIP", pip_msgpath, sys.pip_board);
            fd = cshopen(filename, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
            if (fd == -1) {
                return (0);
            }
            close(fd);

            sprintf(filename, "%sMPKT%04x.PIP", pip_msgpath, sys.pip_board);
            fd = shopen(filename, O_RDONLY | O_BINARY);
            if (fd == -1) {
                fd = cshopen(filename, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
                if (fd == -1) {
                    return (0);
                }
                filename[0] = filename[1] = '\0';
                write(fd, filename, 2);
            }
            close(fd);
        }
        else if (sys.quick_board) {
            sprintf(filename, "%sMSG*.BBS", fido_msgpath);
            if (!dexists(filename)) {
                return (0);
            }
        }
        else if (sys.gold_board) {
            sprintf(filename, "%sMSG*.DAT", fido_msgpath);
            if (!dexists(filename)) {
                return (0);
            }
        }
        else if (sys.squish) {
            ptr = MsgOpenArea(sys.msg_path, MSGAREA_CRIFNEC, MSGTYPE_SQUISH);
            if (ptr == NULL) {
                return (0);
            }
            MsgCloseArea(ptr);
        }
        else {
            sys.msg_path[strlen(sys.msg_path) - 1] = '\0';
            if (!dexists(sys.msg_path)) {
                return (0);
            }
            sys.msg_path[strlen(sys.msg_path)] = '\\';
        }
    }
    else if (type == 2) {
        sprintf(filename, "%sSYSFILE.DAT", config->sys_path);
        while ((fd = sopen(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) == -1)
            ;
        lseek(fd, (long)i * SIZEOF_FILEAREA, SEEK_SET);
        read(fd, (char *)&tsys.file_name, SIZEOF_FILEAREA);

        if (caller && usr.name[0]) {
            if (usr.priv < tsys.file_priv) {
                close(fd);
                return (0);
            }
            if ((usr.flags & tsys.file_flags) != tsys.file_flags) {
                close(fd);
                return (0);
            }
        }

        lseek(fd, (long)i * SIZEOF_FILEAREA, SEEK_SET);
        read(fd, (char *)&sys.file_name, SIZEOF_FILEAREA);
        close(fd);
//      memcpy (&sys.file_name, &tsys.file_name, SIZEOF_FILEAREA);

        sys.filepath[strlen(sys.filepath) - 1] = '\0';
        if (!dexists(sys.filepath)) {
            sys.filepath[0] = '\0';
            return (0);
        }
        sys.filepath[strlen(sys.filepath)] = '\\';
    }

    return (1);
}

int read_system2(s, type, tsys)
int s, type;
struct _sys * tsys;
{
    int fd, nsys, i, mn = 0;
    char filename[50];
    struct _sys_idx sysidx[10];
    offline_reader = 0;

    if (type != 1 && type != 2) {
        return (0);
    }

    if (sq_ptr != NULL) {
        MsgCloseArea(sq_ptr);
        sq_ptr = NULL;
    }

    if (type == 1) {
        sprintf(filename, "%sSYSMSG.IDX", config->sys_path);
    }
    else if (type == 2) {
        sprintf(filename, "%sSYSFILE.IDX", config->sys_path);
    }

    fd = cshopen(filename, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
    if (fd == -1) {
        return (0);
    }

    do {
        nsys = read(fd, (char *)&sysidx, sizeof(struct _sys_idx) * 10);
        nsys /= sizeof(struct _sys_idx);
        for (i = 0; i < nsys; i++, mn++) {
            if (sysidx[i].area == s) {
                break;
            }
        }
    } while (i == nsys && nsys == 10);

    close(fd);

    if (i == nsys || sysidx[i].priv > usr.priv || (usr.flags & sysidx[i].flags) != sysidx[i].flags) {
        return (0);
    }

    i = mn;

    if (type == 1) {
        sprintf(filename, SYSMSG_PATH, config->sys_path);
        fd = cshopen(filename, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        if (fd == -1) {
            return (0);
        }
        lseek(fd, (long)i * SIZEOF_MSGAREA, SEEK_SET);
        read(fd, (char *)&tsys->msg_name, SIZEOF_MSGAREA);
        close(fd);
    }
    else if (type == 2) {
        sprintf(filename, "%sSYSFILE.DAT", config->sys_path);
        fd = cshopen(filename, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        if (fd == -1) {
            return (0);
        }
        lseek(fd, (long)i * SIZEOF_FILEAREA, SEEK_SET);
        read(fd, (char *)&tsys->file_name, SIZEOF_FILEAREA);
        close(fd);
    }

    return (1);
}

void user_list(args)
char * args;
{
    char stringa[40], linea[80], *p, handle, swap, vote, filebox;
    int fd, line, act, nn, day, mont, year;
    int mdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    long days, now;
    struct _usr tempusr;

    nopause = 0;
    line = 3;
    act = 0;
    nn = 0;
    filebox = vote = handle = swap = 0;

    cls();

    change_attr(YELLOW | _BLACK);
    if ((p = strstr(args, "/H")) != NULL) {
        handle = 1;
    }
    if ((p = strstr(args, "/F")) != NULL) {
        filebox = 1;
    }
    if ((p = strstr(args, "/V")) != NULL) {
        vote = 1;
    }
    if ((p = strstr(args, "/S")) != NULL) {
        swap = 1;
    }
    if ((p = strstr(args, "/L")) != NULL) {
        act = 1;
        if (p[2] == '=') {
            nn = atoi(&p[3]);
        }
        else {
            if (!get_command_word(stringa, 5)) {
                m_print(bbstxt[B_MIN_DAYS_LIST]);
                input(stringa, 5);
            }
            if (stringa[0] == '\0') {
                nn = 15;
            }
            nn = atoi(stringa);
        }
        data(stringa);
        sscanf(stringa, "%2d %3s %2d", &day, linea, &year);
        linea[3] = '\0';
        for (mont = 0; mont < 12; mont++) {
            if ((!stricmp(mtext[mont], linea)) || (!stricmp(mesi[mont], linea))) {
                break;
            }
        }
        now = (long)year * 365;
        for (line = 0; line < mont; line++) {
            now += mdays[line];
        }
        now += day;
    }
    else if ((p = strstr(args, "/T")) != NULL) {
        act = 2;
        if (p[2] == '=') {
            nn = atoi(&p[3]);
        }
        else {
            if (!get_command_word(stringa, 5)) {
                m_print(bbstxt[B_MIN_CALLS_LIST]);
                input(stringa, 5);
            }
            if (stringa[0] == '\0') {
                nn = 10;
            }
            nn = atoi(stringa);
        }
    }
    else {
        if (!get_command_word(stringa, 35)) {
            m_print(bbstxt[B_ENTER_USERNAME]);
            input(stringa, 35);
        }
        act = 0;
    }

    strupr(stringa);

    cls();
    change_attr(YELLOW | _BLACK);
    m_print(bbstxt[B_USERLIST_TITLE]);
    m_print(bbstxt[B_USERLIST_UNDERLINE]);

    sprintf(linea, "%s.BBS", config->user_file);
    fd = shopen(linea, O_RDONLY | O_BINARY);

    while (read(fd, (char *)&tempusr, sizeof(struct _usr)) == sizeof(struct _usr)) {
        if (tempusr.usrhidden || tempusr.deleted || !tempusr.name[0]) {
            continue;
        }

        if (filebox && !tempusr.havebox) {
            continue;
        }

        if (vote && tempusr.priv != vote_priv) {
            continue;
        }

        if (handle) {
            strcpy(tempusr.name, tempusr.handle);
        }

        if (act == 0) {
            strupr(tempusr.name);

            if (strstr(tempusr.name, stringa) == NULL) {
                continue;
            }

            fancy_str(tempusr.name);
        }
        else if (act == 1) {
            sscanf(tempusr.ldate, "%2d %3s %2d", &day, linea, &year);
            linea[3] = '\0';
            for (mont = 0; mont < 12; mont++) {
                if ((!stricmp(mtext[mont], linea)) || (!stricmp(mesi[mont], linea))) {
                    break;
                }
            }
            days = (long)year * 365;
            for (line = 0; line < mont; line++) {
                days += mdays[line];
            }
            days += day;
            if ((int)(now - days) > nn) {
                continue;
            }
        }
        else if (act == 2) {
            if (tempusr.times < (long)nn) {
                continue;
            }
        }

        if (swap) {
            strcpy(linea, tempusr.name);
            p = strchr(linea, '\0');
            while (*p != ' ' && p != linea) {
                p--;
            }
            if (p != linea) {
                *p++ = '\0';
            }
            strcpy(tempusr.name, p);
            strcat(tempusr.name, " ");
            strcat(tempusr.name, linea);
        }

        m_print(bbstxt[B_USERLIST_FORMAT], tempusr.name, tempusr.city, tempusr.ldate, tempusr.times, tempusr.n_dnld, tempusr.n_upld);
        if (!(line = more_question(line))) {
            break;
        }
    }

    close(fd);
    m_print(bbstxt[B_ONE_CR]);

    if (line) {
        press_enter();
    }
}

int logoff_procedure()
{
    read_system_file("LOGOFF");

    if (!local_mode && !terminal && CARRIER) {
        timer(2);
    }

    return (1);
}

void update_user(void)
{
    int online, fd, i, m, fflag, posit;
    char filename[80];
    long prev, crc;
    struct _useron useron;
    struct _usridx usridx[MAX_INDEX];

    if (!local_mode) {
        FLUSH_OUTPUT();
    }

    sprintf(filename, USERON_NAME, config->sys_path);
    if ((fd = sh_open(filename, SH_DENYNONE, O_RDWR | O_BINARY, S_IREAD | S_IWRITE)) != -1) {
        for (;;) {
            prev = tell(fd);

            if (read(fd, (char *)&useron, sizeof(struct _useron)) != sizeof(struct _useron)) {
                break;
            }

            if (useron.line == line_offset) {
                lseek(fd, prev, SEEK_SET);
                memset((char *)&useron, 0, sizeof(struct _useron));
                write(fd, (char *)&useron, sizeof(struct _useron));
                break;
            }
        }

        close(fd);
    }

    if (usr.name[0] && usr.city[0] && usr.pwd[0]) {
        fflag = 0;
        posit = 0;

        crc = crc_name(usr.name);

        sprintf(filename, "%s.IDX", config->user_file);
        fd = sh_open(filename, SH_DENYWR, O_RDWR | O_BINARY, S_IREAD | S_IWRITE);

        do {
            i = read(fd, (char *)&usridx, sizeof(struct _usridx) * MAX_INDEX);
            m = i / sizeof(struct _usridx);

            for (i = 0; i < m; i++)
                if (usridx[i].id == crc) {
                    m = 0;
                    posit += i;
                    fflag = 1;
                    break;
                }

            if (!fflag) {
                posit += m;
            }
        } while (m == MAX_INDEX && !fflag);

        close(fd);

        if (!fflag) {
            status_line("!Can't update %s", usr.name);
        }
        else {
            online = (int)((time(NULL) - start_time) / 60);
            usr.time += online;
            if (lorainfo.logindate[0]) {
                strcpy(usr.ldate, lorainfo.logindate);
            }

            sprintf(filename, "%s.BBS", config->user_file);
            fd = sh_open(filename, SH_DENYWR, O_RDWR | O_BINARY, S_IREAD | S_IWRITE);
            lseek(fd, (long)posit * sizeof(struct _usr), SEEK_SET);
            write(fd, (char *)&usr, sizeof(struct _usr));
            close(fd);

            set_last_caller();
        }

        online = (int)((time(NULL) - start_time) / 60);
        status_line("+%s off-line. Calls=%ld, Len=%d, Today=%d", usr.name, usr.times, online, usr.time);
        sysinfo.today.humanconnects += time(NULL) - start_time;
        sysinfo.week.humanconnects += time(NULL) - start_time;
        sysinfo.month.humanconnects += time(NULL) - start_time;
        sysinfo.year.humanconnects += time(NULL) - start_time;
    }

    memset(usr.name, 0, 36);
}

void space(int s)
{
    int i;

    for (i = 0; i < s; i++) {
        if (!local_mode) {
            BUFFER_BYTE(' ');
        }
        if (snooping) {
            wputc(' ');
        }
    }

    if (!local_mode) {
        UNBUFFER_BYTES();
    }
}

static void compilation_data(void)
{
#ifdef __OS2__
#define COMPILER      "Borland C"
#define COMPVERMAJ    1
#define COMPVERMIN    50
#else
#define COMPILER      "Borland C"
#define COMPVERMAJ    3
#define COMPVERMIN    10
#endif

    m_print(bbstxt[B_COMPILER], __DATE__, __TIME__, COMPILER, COMPVERMAJ, COMPVERMIN);
}

void show_lastcallers(char * args)
{
    int fd, i, line;
    char linea[82], filename[80];
    struct _lastcall lc;

    sprintf(filename, "%sLASTCALL.BBS", config->sys_path);
    fd = shopen(filename, O_RDONLY | O_BINARY);

    memset((char *)&lc, 0, sizeof(struct _lastcall));

    cls();
    sprintf(linea, bbstxt[B_TODAY_CALLERS], system_name);
    i = (80 - strlen(linea)) / 2;
    space(i);
    change_attr(WHITE | _BLACK);
    m_print("%s\n", linea);
    change_attr(LRED | _BLACK);
    strnset(linea, '-', strlen(linea));
    i = (80 - strlen(linea)) / 2;
    space(i);
    m_print("%s\n\n", linea);
    change_attr(LGREEN | _BLACK);
    m_print(bbstxt[B_CALLERS_HEADER]);
    change_attr(GREEN | _BLACK);
    m_print("-------------------------------------------------------------------------------\n");

    line = 5;
    i = atoi(args);

    while (read(fd, (char *)&lc, sizeof(struct _lastcall)) == sizeof(struct _lastcall)) {
        if (i && lc.line != i) {
            continue;
        }

        change_attr(LCYAN | _BLACK);
        sprintf(linea, "%-20.20s ", lc.name);
        m_print(linea);
        change_attr(WHITE | _BLACK);
        m_print("%2d    ", lc.line);
        m_print("%6lu ", (long)lc.baud * 100L);
        m_print("    %s ", lc.logon);
        m_print("  %s ", lc.logoff);
        m_print(" %5d  ", lc.times);
        change_attr(YELLOW | _BLACK);
        sprintf(linea, " %s", lc.city);
        linea[19] = '\0';
        m_print("%s\n", linea);

        if (!(line = more_question(line)) || !CARRIER) {
            break;
        }
    }

    close(fd);
    m_print(bbstxt[B_ONE_CR]);

    if (line) {
        press_enter();
    }
}

void bulletin_menu(args)
char * args;
{
    int max_input;
    char stringa[10], *p, *a, filename[80];

    p = args;
    while ((a = strchr(p, ':')) != NULL || (a = strchr(p, '\\')) != NULL) {
        p = a + 1;
    }

    max_input = 8 - strlen(p);

    for (;;) {
        if (!get_command_word(stringa, max_input)) {
            if (!read_file(args)) {
                return;
            }
            input(stringa, max_input);
            if (!stringa[0] || !CARRIER || time_remain() <= 0) {
                return;
            }
        }

        strcpy(filename, args);
        strcat(filename, stringa);
        read_file(filename);
        press_enter();
    }
}

void message_to_next_caller()
{
    FILE * fp;
    int i;
    char filename[80];
    long t;
    struct tm * tim;

    line_editor(0);

    m_print(bbstxt[B_DATABASE]);
    if (yesno_question(DEF_YES) == DEF_NO) {
        free_message_array();
        return;
    }

    sprintf(filename, "%sNEXT%d.BBS", config->sys_path, line_offset);
    fp = fopen(filename, "wb");
    if (fp == NULL) {
        free_message_array();
        return;
    }

    memset((char *)&msg, 0, sizeof(struct _msg));
    msg.attr = MSGLOCAL;

    msg.dest_net = config->alias[sys.use_alias].net;
    msg.dest = config->alias[sys.use_alias].node;
    strcpy(msg.from, usr.name);
    strcpy(msg.subj, "Logout comment");
    data(msg.date);
    msg.up = 0;
    msg_fzone = config->alias[sys.use_alias].zone;
    msg.orig = config->alias[sys.use_alias].node;
    msg.orig_net = config->alias[sys.use_alias].net;
    time(&t);
    tim = localtime(&t);
    msg.date_written.date = (tim->tm_year - 80) * 512 + (tim->tm_mon + 1) * 32 + tim->tm_mday;
    msg.date_written.time = tim->tm_hour * 2048 + tim->tm_min * 32 + tim->tm_sec / 2;
    msg.date_arrived.date = msg.date_written.date;
    msg.date_arrived.time = msg.date_written.time;

    fwrite((char *)&msg, sizeof(struct _msg), 1, fp);

    i = 0;
    while (messaggio[i] != NULL) {
        if (messaggio[i][strlen(messaggio[i]) - 1] == '\r') {
            fprintf(fp, "%s\n", messaggio[i++]);
        }
        else {
            fprintf(fp, "%s\r\n", messaggio[i++]);
        }
    }

    fclose(fp);
    free_message_array();

    status_line(":Write message to next caller");
}

void read_comment()
{
    FILE * fp;
    int line = 1;
    char filename[130];
    struct _msg msgt;

    sprintf(filename, "%sNEXT%d.BBS", config->sys_path, line_offset);
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        return;
    }

    fread((char *)&msgt, sizeof(struct _msg), 1, fp);
    strcpy(msgt.to, usr.name);

    cls();
    line = msg_attrib(&msgt, 0, line, 0);
    m_print(bbstxt[B_ONE_CR]);
    change_attr(LGREY | _BLACK);

    while (fgets(filename, 128, fp) != NULL) {
        while (strlen(filename) > 0 && (filename[strlen(filename) - 1] == 0x0D || filename[strlen(filename) - 1] == 0x0A)) {
            filename[strlen(filename) - 1] = '\0';
        }
        m_print("\026\001\002%s\026\001\007\n", filename);
        if (!(line = more_question(line)) || !CARRIER) {
            break;
        }
    }

    fclose(fp);

    UNBUFFER_BYTES();
    FLUSH_OUTPUT();

    if (line) {
        m_print(bbstxt[B_ONE_CR]);
        press_enter();
    }

    sprintf(filename, "%sNEXT%d.BBS", config->sys_path, line_offset);
    unlink(filename);
}

#define IsBis(y) (!(y % 4) && ((y % 100) || !(y % 400)))

void display_usage()
{
    int i, y_axis[15], z, m, dd, mm, aa, month[13];
    long total, tempo, day_now;
    struct tm * tim;

    month[1] = 31;
    month[2] = 28;
    month[3] = 31;
    month[4] = 30;
    month[5] = 31;
    month[6] = 30;
    month[7] = 31;
    month[8] = 31;
    month[9] = 30;
    month[10] = 31;
    month[11] = 30;
    month[12] = 31;

    tempo = time(&tempo);
    tim = localtime(&tempo);

    total = 365 * tim->tm_year;
    if (IsBis(tim->tm_year)) {
        month[2] = 29;
    }
    else {
        month[2] = 28;
    }
    for (i = 1; i < (tim->tm_mon + 1); i++) {
        total += month[i];
    }
    total += tim->tm_mday;
    total *= 60;

    sscanf(linestat.startdate, "%2d-%2d-%2d", &mm, &dd, &aa);

    day_now = 365 * aa;
    if (IsBis(aa)) {
        month[2] = 29;
    }
    else {
        month[2] = 28;
    }
    for (i = 1; i < mm; i++) {
        day_now += month[i];
    }
    day_now += dd;
    day_now *= 60;

    total -= day_now;
    total += 60;

    for (i = 0; i < 15; i++) {
        y_axis[i] = 0;
    }

    if (total > 0)
        for (i = 0; i < 24; i++)
        {
            m = (int)((linestat.busyperhour[i] * 100L) / total);
            if (m > y_axis[14]) {
                y_axis[14] = m;
            }
        }

    m = y_axis[14] / 15;

    for (i = 13; i >= 0; i--) {
        y_axis[i] = y_axis[i + 1] - m;
    }

    cls();
    m_print(bbstxt[B_PERCENTAGE], linestat.startdate, tim->tm_mon + 1, tim->tm_mday, tim->tm_year % 100, line_offset);

    for (i = 14; i >= 0; i--) {
        m_print(bbstxt[B_PERCY], y_axis[i]);
        for (z = 0; z < 24; z++) {
            if (total > 0) {
                m = (int)((linestat.busyperhour[z] * 100L) / total);
            }
            else {
                m = -1;
            }
            if (m >= y_axis[i] && m > 0) {
                m_print("\333\333 ");
            }
            else {
                m_print("   ");
            }
        }

        m_print(bbstxt[B_ONE_CR]);
    }

    m_print(bbstxt[B_PERCX1]);
    m_print(bbstxt[B_PERCX2]);

    press_enter();
}

void time_statistics()
{
    long t, this_call;
    struct tm * tim;

    this_call = (long)((time(&t) - start_time));
    tim = localtime(&t);

    m_print(bbstxt[B_TSTATS_1], tim->tm_hour, tim->tm_min);
    m_print(bbstxt[B_TSTATS_2], tim->tm_mday, mtext[tim->tm_mon], tim->tm_year);

    m_print(bbstxt[B_TSTATS_3], (int)(this_call / 60), (int)(this_call % 60));

    this_call += usr.time * 60;
    m_print(bbstxt[B_TSTATS_4], (int)(this_call / 60), (int)(this_call % 60));
    m_print(bbstxt[B_TSTATS_5], time_remain());
    m_print(bbstxt[B_TSTATS_6], config->class[usr_class].max_call);

    press_enter();
}

