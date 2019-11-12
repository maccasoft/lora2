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
#include <share.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <alloc.h>
#include <conio.h>
#include <dir.h>
#include <process.h>
#include <sys\stat.h>

#include "sched.h"
#include "lsetup.h"
#include "lprot.h"

extern char * LSETUP_VERSION;

void clear_window(void);
void linehelp_window(void);
int sh_open(char * file, int shmode, int omode, int fmode);
long window_get_flags(int y, int x, int type, long f);

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>
#include <cxl\cxlkey.h>

extern struct _configuration config;

char * get_priv_text(int);

static void edit_single_node(NODEINFO *);
static void edit_single_location(ACCOUNT *);
static void select_nodes_list(int, NODEINFO *);
static void select_cost_list(int, ACCOUNT *);

char * get_priv_text(priv)
int priv;
{
    int i;
    char * txt;

    txt = " ??? ";

    for (i = 0; i < 12; i++)
        if (levels[i].p_length == priv)
        {
            txt = levels[i].p_string;
            break;
        }

    return (txt);
}

void mailer_misc()
{
    int wh, i = 1;
    char string[128];

    wh = wopen(6, 5, 18, 74, 1, LCYAN | _BLACK, CYAN | _BLACK);
    wactiv(wh);
    wshadow(DGREY | _BLACK);
    wtitle(" Miscellaneous ", TRIGHT, YELLOW | _BLUE);

    do {
        stop_update();
        wclear();

        wmenubegc();
        wmenuitem(1, 1, " Banner        ", 'B', 1, 0, NULL, 0, 0);
        wmenuiba(linehelp_window, clear_window);
        wmenuitem(2, 1, " Mail-only     ", 'M', 2, 0, NULL, 0, 0);
        wmenuiba(linehelp_window, clear_window);
        wmenuitem(3, 1, " Enter-BBS     ", 'E', 3, 0, NULL, 0, 0);
        wmenuiba(linehelp_window, clear_window);
        wmenuitem(4, 1, " Events file  ", 'J', 10, 0, NULL, 0, 0);
        wmenuiba(linehelp_window, clear_window);
        wmenuitem(6, 1, " WaZOO              ", 'W', 4, 0, NULL, 0, 0);
        wmenuiba(linehelp_window, clear_window);
        wmenuitem(7, 1, " EMSI               ", 'E', 5, 0, NULL, 0, 0);
        wmenuiba(linehelp_window, clear_window);
        wmenuitem(8, 1, " Janus              ", 'J', 6, 0, NULL, 0, 0);
        wmenuiba(linehelp_window, clear_window);
        wmenuitem(9, 1, " Random redial time ", 'J', 12, 0, NULL, 0, 0);
        wmenuiba(linehelp_window, clear_window);
        wmenuitem(6, 31, " Secure             ", 'J', 7, 0, NULL, 0, 0);
        wmenuiba(linehelp_window, clear_window);
        wmenuitem(7, 31, " Keep netmail       ", 'J', 8, 0, NULL, 0, 0);
        wmenuiba(linehelp_window, clear_window);
        wmenuitem(8, 31, " Flashing mail      ", 'J', 11, 0, NULL, 0, 0);
        wmenuiba(linehelp_window, clear_window);
        wmenuitem(9, 31, " Track Netmail      ", 'J', 13, 0, NULL, 0, 0);
        wmenuiba(linehelp_window, clear_window);
        wmenuend(i, M_OMNI | M_SAVE, 0, 0, LGREY | _BLACK, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY);

        wprints(1, 17, CYAN | _BLACK, config.banner);
        wprints(2, 17, CYAN | _BLACK, config.mail_only);
        wprints(3, 17, CYAN | _BLACK, config.enterbbs);
        wprints(4, 17, CYAN | _BLACK, config.sched_name);
        wprints(6, 22, CYAN | _BLACK, config.wazoo ? "Yes" : "No");
        wprints(7, 22, CYAN | _BLACK, config.emsi ? "Yes" : "No");
        wprints(8, 22, CYAN | _BLACK, config.janus ? "Yes" : "No");
        wprints(9, 22, CYAN | _BLACK, config.random_redial ? "Yes" : "No");
        wprints(6, 52, CYAN | _BLACK, config.secure ? "Yes" : "No");
        wprints(7, 52, CYAN | _BLACK, config.keeptransit ? "Yes" : "No");
        wprints(8, 52, CYAN | _BLACK, config.newmail_flash ? "Yes" : "No");
        wprints(9, 52, CYAN | _BLACK, config.msgtrack ? "Yes" : "No");

        start_update();
        i = wmenuget();

        switch (i) {
            case 1:
                strcpy(string, config.banner);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(1, 17, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.banner, strbtrim(string));
                }
                hidecur();
                break;

            case 2:
                strcpy(string, config.mail_only);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(2, 17, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.mail_only, strbtrim(string));
                }
                break;

            case 3:
                strcpy(string, config.enterbbs);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(3, 17, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.enterbbs, strbtrim(string));
                }
                break;

            case 4:
                config.wazoo ^= 1;
                break;

            case 5:
                config.emsi ^= 1;
                break;

            case 6:
                config.janus ^= 1;
                break;

            case 7:
                config.secure ^= 1;
                break;

            case 8:
                config.keeptransit ^= 1;
                break;

            case 10:
                strcpy(string, config.sched_name);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(4, 17, string, "???????????????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.sched_name, strbtrim(string));
                }
                hidecur();
                break;

            case 11:
                config.newmail_flash ^= 1;
                break;

            case 12:
                config.random_redial ^= 1;
                break;

            case 13:
                config.msgtrack ^= 1;
                break;
        }

        hidecur();

    } while (i != -1);

    wclose();
}

void write_areasbbs()
{
    FILE * fp;
    int wh, i = 1, fd;
    char string[128];
    struct _sys sys;

    wh = wopen(7, 4, 11, 73, 1, LCYAN | _BLACK, CYAN | _BLACK);
    wactiv(wh);
    wshadow(DGREY | _BLACK);
    wtitle(" Write AREAS.BBS ", TRIGHT, YELLOW | _BLUE);

    wprints(1, 1, CYAN | _BLACK, " Filename:");
    if (!config.areas_bbs[0]) {
        sprintf(string, "%sAREAS.BBS", config.sys_path);
    }
    else {
        strcpy(string, config.areas_bbs);
    }
    winpbeg(BLACK | _LGREY, BLACK | _LGREY);
    winpdef(1, 12, string, "?????????????????????????????????????????????????????", 0, 2, NULL, 0);
    i = winpread();

    hidecur();

    if (i == W_ESCPRESS) {
        wclose();
        return;
    }

    if ((fp = fopen(string, "wt")) == NULL) {
        wclose();
        return;
    }

    fprintf(fp, "%s ! %s\n;\n", config.system_name, config.sysop);

    sprintf(string, "%sSYSMSG.DAT", config.sys_path);
    while ((fd = sopen(string, SH_DENYWR, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) == -1)
        ;

    while (read(fd, (char *)&sys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
        if (!sys.echomail && !sys.passthrough) {
            continue;
        }

        if (sys.passthrough) {
            fprintf(fp, "##%s %s %s", "", sys.echotag, sys.forward1);
            if (sys.forward2[0]) {
                fprintf(fp, " %s", sys.forward2);
            }
            if (sys.forward3[0]) {
                fprintf(fp, " %s", sys.forward3);
            }
        }
        else if (sys.quick_board) {
            fprintf(fp, "%d %s %s", sys.quick_board, sys.echotag, sys.forward1);
            if (sys.forward2[0]) {
                fprintf(fp, " %s", sys.forward2);
            }
            if (sys.forward3[0]) {
                fprintf(fp, " %s", sys.forward3);
            }
        }
        else if (sys.gold_board) {
            fprintf(fp, "G%d %s %s", sys.gold_board, sys.echotag, sys.forward1);
            if (sys.forward2[0]) {
                fprintf(fp, " %s", sys.forward2);
            }
            if (sys.forward3[0]) {
                fprintf(fp, " %s", sys.forward3);
            }
        }
        else if (sys.pip_board) {
            fprintf(fp, "!%d %s %s", sys.pip_board, sys.echotag, sys.forward1);
            if (sys.forward2[0]) {
                fprintf(fp, " %s", sys.forward2);
            }
            if (sys.forward3[0]) {
                fprintf(fp, " %s", sys.forward3);
            }
        }
        else if (sys.squish) {
            fprintf(fp, "$%s %s %s", sys.msg_path, sys.echotag, sys.forward1);
            if (sys.forward2[0]) {
                fprintf(fp, " %s", sys.forward2);
            }
            if (sys.forward3[0]) {
                fprintf(fp, " %s", sys.forward3);
            }
        }
        else {
            fprintf(fp, "%s %s %s", sys.msg_path, sys.echotag, sys.forward1);
            if (sys.forward2[0]) {
                fprintf(fp, " %s", sys.forward2);
            }
            if (sys.forward3[0]) {
                fprintf(fp, " %s", sys.forward3);
            }
        }

        fprintf(fp, "\n");
    }

    fprintf(fp, ";\n; Created by LSETUP v.%s\n;\n", LSETUP_VERSION);

    fclose(fp);
    close(fd);

    wclose();
}

void manager_nodelist()
{
    int wh, i;
    char string[80];

    wh = wopen(0, 16, 23, 57, 1, LCYAN | _BLACK, CYAN | _BLACK);
    wactiv(wh);
    wshadow(DGREY | _BLACK);
    wtitle(" Nodelist ", TRIGHT, YELLOW | _BLUE);
    i = 80;

    do {
        stop_update();
        wclear();

//      wprints (1, 11, LGREY|BLACK, "\332\304\304Nodelist\304\304\277\332\304\304Nodediff\304\304\277\332\304\304\304Packed\304\304\304\277");
        wprints(1, 11, LGREY | BLACK, "\332\304\304Nodelist\304\304\277\332\304\304Nodediff\304\304\277");

        wmenubegc();
        wmenuitem(2, 1, " List 1  ", 'L', 80, 0, NULL, 0, 0);
        wmenuitem(3, 1, " List 2  ", 'L', 81, 0, NULL, 0, 0);
        wmenuitem(4, 1, " List 3  ", 'L', 82, 0, NULL, 0, 0);
        wmenuitem(5, 1, " List 4  ", 'L', 83, 0, NULL, 0, 0);
        wmenuitem(6, 1, " List 5  ", 'L', 84, 0, NULL, 0, 0);
        wmenuitem(7, 1, " List 6  ", 'L', 85, 0, NULL, 0, 0);
        wmenuitem(8, 1, " List 7  ", 'L', 86, 0, NULL, 0, 0);
        wmenuitem(9, 1, " List 8  ", 'L', 87, 0, NULL, 0, 0);
        wmenuitem(10, 1, " List 9  ", 'L', 88, 0, NULL, 0, 0);
        wmenuitem(11, 1, " List 10 ", 'L', 89, 0, NULL, 0, 0);
        wmenuitem(12, 1, " List 11 ", 'L', 90, 0, NULL, 0, 0);
        wmenuitem(13, 1, " List 12 ", 'L', 91, 0, NULL, 0, 0);
        wmenuitem(14, 1, " List 13 ", 'L', 92, 0, NULL, 0, 0);
        wmenuitem(15, 1, " List 14 ", 'L', 93, 0, NULL, 0, 0);
        wmenuitem(16, 1, " List 15 ", 'L', 94, 0, NULL, 0, 0);
        wmenuitem(17, 1, " List 16 ", 'L', 95, 0, NULL, 0, 0);
        wmenuitem(18, 1, " List 17 ", 'L', 96, 0, NULL, 0, 0);
        wmenuitem(19, 1, " List 18 ", 'L', 97, 0, NULL, 0, 0);
        wmenuitem(20, 1, " List 19 ", 'L', 98, 0, NULL, 0, 0);
        wmenuitem(21, 1, " List 20 ", 'L', 99, 0, NULL, 0, 0);
        wmenuend(i, M_VERT, 0, 0, LGREY | _BLACK, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY);

        for (i = 0; i < MAXNL; i++) {
            wprints(i + 2, 12, CYAN | _BLACK, config.nl[i].list_name);
            wprints(i + 2, 26, CYAN | _BLACK, config.nl[i].diff_name);
//         wprints (i + 2, 40, CYAN|_BLACK, config.nl[i].arc_name);
        }

        start_update();
        i = wmenuget();
        if (i != -1) {
            i -= 80;
            strcpy(string, config.nl[i].list_name);
            winpbeg(BLUE | _GREEN, BLUE | _GREEN);
            winpdef(i + 2, 12, string, "????????????", 0, 2, NULL, 0);
            if (winpread() != W_ESCPRESS) {
                strcpy(config.nl[i].list_name, strbtrim(string));
                wprints(i + 2, 12, CYAN | _BLACK, "            ");
                wprints(i + 2, 12, CYAN | _BLACK, config.nl[i].list_name);

                strcpy(string, config.nl[i].diff_name);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(i + 2, 26, string, "????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.nl[i].diff_name, strbtrim(string));
                    wprints(i + 2, 26, CYAN | _BLACK, "            ");
                    wprints(i + 2, 26, CYAN | _BLACK, config.nl[i].diff_name);

//               strcpy (string, config.nl[i].arc_name);
//               winpbeg (BLUE|_GREEN, BLUE|_GREEN);
//               winpdef (i + 2, 40, string, "????????????", 0, 2, NULL, 0);
//               if (winpread () != W_ESCPRESS) {
//                  strcpy (config.nl[i].arc_name, strbtrim (string));
//                  wprints (i + 2, 40, CYAN|_BLACK, "            ");
//                  wprints (i + 2, 40, CYAN|_BLACK, config.nl[i].arc_name);
//               }
                }
            }

            i += 80;
            hidecur();
        }

    } while (i != -1);

    wclose();
}

void manager_edit_packer(int o)
{
    int wh, i = 1;
    char string[80];

    wh = wopen(4, 20, 13, 69, 1, LCYAN | _BLACK, CYAN | _BLACK);
    wactiv(wh);
    wshadow(DGREY | _BLACK);
    wtitle(" Edit Packers ", TRIGHT, YELLOW | _BLUE);

    do {
        stop_update();
        wclear();

        wmenubegc();
        wmenuitem(1, 1, " Log ID         ", 'I', 1, 0, NULL, 0, 0);
        wmenuitem(2, 1, " Users display  ", 'd', 2, 0, NULL, 0, 0);
        wmenuitem(3, 1, " Pack command   ", 'P', 3, 0, NULL, 0, 0);
        wmenuitem(4, 1, " Unpack command ", 'U', 4, 0, NULL, 0, 0);
        wmenuitem(5, 1, " Offset         ", 'O', 5, 0, NULL, 0, 0);
        wmenuitem(6, 1, " \300\304 ID bytes    ", 'b', 6, 0, NULL, 0, 0);
        wmenuend(i, M_OMNI | M_SAVE, 0, 0, LGREY | _BLACK, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY);

        wprints(1, 18, CYAN | _BLACK, config.packers[o].id);
        wprints(2, 18, CYAN | _BLACK, config.packid[o].display);
        wprints(3, 18, CYAN | _BLACK, config.packers[o].packcmd);
        wprints(4, 18, CYAN | _BLACK, config.packers[o].unpackcmd);
        sprintf(string, "%d", config.packid[o].offset);
        wprints(5, 18, CYAN | _BLACK, string);
        wprints(6, 18, CYAN | _BLACK, config.packid[o].ident);

        start_update();
        i = wmenuget();

        switch (i) {
            case 1:
                strcpy(string, config.packers[o].id);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(1, 18, string, "????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.packers[o].id, strbtrim(string));
                }
                break;

            case 2:
                strcpy(string, config.packid[o].display);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(2, 18, string, "???????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.packid[o].display, strbtrim(string));
                }
                break;

            case 3:
                strcpy(string, config.packers[o].packcmd);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(3, 18, string, "?????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.packers[o].packcmd, strbtrim(string));
                }
                break;

            case 4:
                strcpy(string, config.packers[o].unpackcmd);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(4, 18, string, "?????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.packers[o].unpackcmd, strbtrim(string));
                }
                break;

            case 5:
                sprintf(string, "%d", config.packid[o].offset);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(5, 18, string, "?????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    config.packid[o].offset = atoi(strbtrim(string));
                }
                break;

            case 6:
                strcpy(string, config.packid[o].ident);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(6, 18, string, "??????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.packid[o].ident, strupr(strbtrim(string)));
                }
                break;
        }

        hidecur();
    } while (i != -1);

    wclose();
}

void manager_packers()
{
    int i = 1, m;
    char packs[10][20];

    wopen(6, 32, 19, 45, 1, LCYAN | _BLACK, CYAN | _BLACK);
    wshadow(DGREY | _BLACK);
    wtitle(" Packer ", TRIGHT, YELLOW | _BLUE);

    for (;;) {
        stop_update();
        wmenubegc();
        for (m = 0; m < 10; m++) {
            sprintf(packs[m], " %-8.8s ", config.packers[m].id);
            wmenuitem(m + 1, 1, packs[m], 0, m + 1, 0, NULL, 0, 0);
        }
        wmenuend(i, M_OMNI | M_SAVE, 0, 0, LGREY | _BLACK, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY);

        start_update();
        if ((i = wmenuget()) == -1) {
            break;
        }

        manager_edit_packer(i - 1);
    }

    wclose();
}

static int node_sort_func(NODEINFO * a1, NODEINFO * b1)
{
    if (a1->zone != b1->zone) {
        return (a1->zone - b1->zone);
    }
    if (a1->net != b1->net) {
        return (a1->net - b1->net);
    }
    if (a1->node != b1->node) {
        return (a1->node - b1->node);
    }
    return (a1->point - b1->point);
}

void manager_nodes()
{
    int fd, fdi, wh, wh1, i = 1, saved, zz, ne, no, po;
    char string[128], readed, filename[80];
    long pos;
    NODEINFO ni, bni;

    sprintf(string, "%sNODES.DAT", config.net_info);
    fd = open(string, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
    if (fd == -1) {
        return;
    }

    if (!read(fd, (char *)&ni, sizeof(NODEINFO))) {
        memset((char *)&ni, 0, sizeof(NODEINFO));
        readed = 0;
    }
    else {
        readed = 1;
    }

    gotoxy_(24, 1);
    clreol_();
    prints(24, 1, LGREY | _BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add New Node  C-Copy  L-List  D-Delete");
    prints(24, 1, YELLOW | _BLACK, "PgUp/PgDn");
    prints(24, 26, YELLOW | _BLACK, "E");
    prints(24, 34, YELLOW | _BLACK, "A");
    prints(24, 50, YELLOW | _BLACK, "C");
    prints(24, 58, YELLOW | _BLACK, "L");
    prints(24, 66, YELLOW | _BLACK, "D");

    wh = wopen(2, 6, 22, 75, 1, LCYAN | _BLACK, CYAN | _BLACK);
    wactiv(wh);
    wshadow(DGREY | _BLACK);
    wtitle(" Nodes ", TRIGHT, YELLOW | _BLUE);

    do {
        stop_update();
        wclear();

        wprints(1,  1, LGREY | _BLACK, " Address         ");
        wprints(2,  1, LGREY | _BLACK, " System          ");
        wprints(3,  1, LGREY | _BLACK, " Sysop Name      ");
        wprints(4,  1, LGREY | _BLACK, " Packer          ");
        wprints(4, 33, LGREY | _BLACK, " Mailer aka      ");
        wprints(5,  1, LGREY | _BLACK, " Session Pw      ");
        wprints(6,  1, LGREY | _BLACK, " Packet Pw (OUT) ");
        wprints(6, 33, LGREY | _BLACK, " Packet Pw (IN)  ");
        wprints(7,  1, LGREY | _BLACK, " Areafix Pw      ");
        wprints(8,  1, LGREY | _BLACK, " TIC Pw          ");
        wprints(9,  1, LGREY | _BLACK, " Phone           ");
        wprints(10,  1, LGREY | _BLACK, " Dial Prefix     ");
        wprints(11,  1, LGREY | _BLACK, " Capabilities    ");
        wprints(10, 33, LGREY | _BLACK, " Min. baud rate  ");
        wprints(12,  1, LGREY | _BLACK, " Echomail aka    ");
        wprints(13,  1, LGREY | _BLACK, " Areafix Level   ");
        wprints(14,  1, LGREY | _BLACK, " \303\304 A Flag       ");
        wprints(15,  1, LGREY | _BLACK, " \303\304 B Flag       ");
        wprints(16,  1, LGREY | _BLACK, " \303\304 C Flag       ");
        wprints(17,  1, LGREY | _BLACK, " \300\304 D Flag       ");
        wprints(12, 33, LGREY | _BLACK, " TIC aka         ");
        wprints(13, 33, LGREY | _BLACK, " TIC Level       ");
        wprints(14, 33, LGREY | _BLACK, " \303\304 A Flag       ");
        wprints(15, 33, LGREY | _BLACK, " \303\304 B Flag       ");
        wprints(16, 33, LGREY | _BLACK, " \303\304 C Flag       ");
        wprints(17, 33, LGREY | _BLACK, " \300\304 D Flag       ");

        if (readed) {
            sprintf(string, "%u:%u/%u.%u", ni.zone, ni.net, ni.node, ni.point);
            wprints(1, 19, CYAN | _BLACK, string);
            wprints(2, 19, CYAN | _BLACK, ni.system);
            wprints(3, 19, CYAN | _BLACK, ni.sysop_name);
            wprints(4, 19, CYAN | _BLACK, config.packers[ni.packer].id);
            sprintf(string, "%u:%u/%u.%u", config.alias[ni.mailer_aka].zone, config.alias[ni.mailer_aka].net, config.alias[ni.mailer_aka].node, config.alias[ni.mailer_aka].point);
            wprints(4, 51, CYAN | _BLACK, string);
            wprints(5, 19, CYAN | _BLACK, ni.pw_session);
            wprints(6, 19, CYAN | _BLACK, ni.pw_packet);
            wprints(6, 51, CYAN | _BLACK, ni.pw_inbound_packet);
            wprints(7, 19, CYAN | _BLACK, ni.pw_areafix);
            wprints(8, 19, CYAN | _BLACK, ni.pw_tic);
            wprints(9, 19, CYAN | _BLACK, ni.phone);
            sprintf(string, "%d", ni.modem_type);
            wprints(10, 19, CYAN | _BLACK, string);
            strcpy(string, "----");
            if (ni.remap4d) {
                string[0] = 'M';
            }
            if (ni.wazoo) {
                string[1] = 'W';
            }
            if (ni.emsi) {
                string[2] = 'E';
            }
            if (ni.janus) {
                string[3] = 'J';
            }
            wprints(11, 19, CYAN | _BLACK, string);
            sprintf(string, "%ld", ni.min_baud_rate);
            wprints(10, 51, CYAN | _BLACK, string);
            if (ni.aka == 0) {
                ni.aka++;
            }
            sprintf(string, "%u:%u/%u.%u", config.alias[ni.aka - 1].zone, config.alias[ni.aka - 1].net, config.alias[ni.aka - 1].node, config.alias[ni.aka - 1].point);
            wprints(12, 19, CYAN | _BLACK, string);

            sprintf(string, "%d", ni.afx_level);
            wprints(13, 19, CYAN | _BLACK, string);
            wprints(14, 19, CYAN | _BLACK, get_flagA_text((ni.afx_flags >> 24) & 0xFF));
            wprints(15, 19, CYAN | _BLACK, get_flagB_text((ni.afx_flags >> 16) & 0xFF));
            wprints(16, 19, CYAN | _BLACK, get_flagC_text((ni.afx_flags >> 8) & 0xFF));
            wprints(17, 19, CYAN | _BLACK, get_flagD_text(ni.afx_flags & 0xFF));
            if (ni.tic_aka == 0) {
                ni.tic_aka++;
            }
            sprintf(string, "%u:%u/%u.%u", config.alias[ni.tic_aka - 1].zone, config.alias[ni.tic_aka - 1].net, config.alias[ni.tic_aka - 1].node, config.alias[ni.tic_aka - 1].point);
            wprints(12, 51, CYAN | _BLACK, string);

            sprintf(string, "%d", ni.tic_level);
            wprints(13, 51, CYAN | _BLACK, string);
            wprints(14, 51, CYAN | _BLACK, get_flagA_text((ni.tic_flags >> 24) & 0xFF));
            wprints(15, 51, CYAN | _BLACK, get_flagB_text((ni.tic_flags >> 16) & 0xFF));
            wprints(16, 51, CYAN | _BLACK, get_flagC_text((ni.tic_flags >> 8) & 0xFF));
            wprints(17, 51, CYAN | _BLACK, get_flagD_text(ni.tic_flags & 0xFF));
        }

        start_update();
        i = getxch();
        if ((i & 0xFF) != 0) {
            i &= 0xFF;
        }

        switch (i) {
            // PgDn
            case 0x5100:
                if (readed) {
                    read(fd, (char *)&ni, sizeof(NODEINFO));
                }
                break;

            // PgUp
            case 0x4900:
                if (readed) {
                    if (tell(fd) > sizeof(NODEINFO)) {
                        lseek(fd, -2L * sizeof(NODEINFO), SEEK_CUR);
                        read(fd, (char *)&ni, sizeof(NODEINFO));
                    }
                }
                break;

            // E Edit
            case 'E':
            case 'e':
                if (readed) {
                    zz = ni.zone;
                    ne = ni.net;
                    no = ni.node;
                    po = ni.point;

                    edit_single_node(&ni);

                    if (zz != ni.zone || ne != ni.net || no != ni.node || po != ni.point) {
                        pos = tell(fd) - (long)sizeof(NODEINFO);
                        close(fd);

                        sprintf(filename, "%sNODES.BAK", config.net_info);
                        unlink(filename);
                        sprintf(string, "%sNODES.DAT", config.net_info);
                        rename(string, filename);

                        fd = open(filename, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                        fdi = open(string, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
                        saved = 0;

                        while (read(fd, (char *)&bni, sizeof(NODEINFO)) == sizeof(NODEINFO)) {
                            if (!saved && node_sort_func(&ni, &bni) < 0) {
                                pos = tell(fdi);
                                write(fdi, (char *)&ni, sizeof(NODEINFO));
                                saved = 1;
                            }
                            if ((bni.zone != ni.zone || bni.net != ni.net || bni.node != ni.node || bni.point != ni.point) &&
                                    (bni.zone != zz || bni.net != ne || bni.node != no || bni.point != po)) {
                                write(fdi, (char *)&bni, sizeof(NODEINFO));
                            }
                        }

                        if (!saved) {
                            pos = tell(fdi);
                            write(fdi, (char *)&ni, sizeof(NODEINFO));
                        }

                        close(fd);
                        close(fdi);

                        unlink(filename);

                        sprintf(string, "%sNODES.DAT", config.net_info);
                        fd = open(string, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                        lseek(fd, pos, SEEK_SET);
                        read(fd, (char *)&ni, sizeof(NODEINFO));
                    }
                    else {
                        lseek(fd, -1L * sizeof(NODEINFO), SEEK_CUR);
                        write(fd, (char *)&ni, sizeof(NODEINFO));
                    }
                }
                break;

            // L List
            case 'L':
            case 'l':
                if (readed) {
                    select_nodes_list(fd, &ni);
                }
                break;

            // A Add & C Copy
            case 'A':
            case 'a':

                memset((char *)&ni, 0, sizeof(NODEINFO));
                ni.remap4d = ni.wazoo = ni.emsi = ni.janus = 1;


            case 'C':
            case 'c':

                if ((i == 'C' || i == 'c') && !readed) {
                    break;
                }

                edit_single_node(&ni);

                if (ni.zone) {
                    pos = tell(fd) - (long)sizeof(NODEINFO);
                    close(fd);

                    sprintf(filename, "%sNODES.BAK", config.net_info);
                    unlink(filename);
                    sprintf(string, "%sNODES.DAT", config.net_info);
                    rename(string, filename);

                    fd = open(filename, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                    fdi = open(string, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
                    saved = 0;

                    while (read(fd, (char *)&bni, sizeof(NODEINFO)) == sizeof(NODEINFO)) {
                        if (!saved && node_sort_func(&ni, &bni) < 0) {
                            pos = tell(fdi);
                            write(fdi, (char *)&ni, sizeof(NODEINFO));
                            saved = 1;
                        }
                        if (bni.zone != ni.zone || bni.net != ni.net || bni.node != ni.node || bni.point != ni.point) {
                            write(fdi, (char *)&bni, sizeof(NODEINFO));
                        }
                    }

                    if (!saved) {
                        pos = tell(fdi);
                        write(fdi, (char *)&ni, sizeof(NODEINFO));
                    }

                    close(fd);
                    close(fdi);

                    unlink(filename);

                    sprintf(string, "%sNODES.DAT", config.net_info);
                    fd = open(string, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                    lseek(fd, pos, SEEK_SET);
                    read(fd, (char *)&ni, sizeof(NODEINFO));
                }
                break;


            // D Delete
            case 'D':
            case 'd':
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

                pos = tell(fd) - (long)sizeof(NODEINFO);
                close(fd);

                if (toupper(string[0]) == 'Y') {
                    sprintf(filename, "%sNODES.BAK", config.net_info);
                    unlink(filename);
                    sprintf(string, "%sNODES.DAT", config.net_info);
                    rename(string, filename);

                    fd = open(filename, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                    fdi = open(string, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);

                    while (read(fd, (char *)&bni, sizeof(NODEINFO)) == sizeof(NODEINFO)) {
                        if (bni.zone != ni.zone || bni.net != ni.net || bni.node != ni.node || bni.point != ni.point) {
                            write(fdi, (char *)&bni, sizeof(NODEINFO));
                        }
                    }

                    close(fd);
                    close(fdi);

                    unlink(filename);
                }

                sprintf(string, "%sNODES.DAT", config.net_info);
                fd = open(string, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                lseek(fd, pos, SEEK_SET);
                read(fd, (char *)&ni, sizeof(NODEINFO));
                break;

            // ESC Exit
            case 0x1B:
                i = -1;
                break;
        }

    } while (i != -1);

    close(fd);

    wclose();
    gotoxy_(24, 1);
    clreol_();
}

static void edit_single_node(nip)
NODEINFO * nip;
{
    int wh1, wh, i = 1, m, mx;
    char string[128], packs[10][12], *akas[MAX_ALIAS + 2], temp[42];
    NODEINFO ni;

    memcpy((char *)&ni, (char *)nip, sizeof(NODEINFO));

    gotoxy_(24, 1);
    clreol_();
    prints(24, 1, LGREY | _BLACK, "ESC-Exit/Save  ENTER-Edit");
    prints(24, 1, YELLOW | _BLACK, "ESC");
    prints(24, 16, YELLOW | _BLACK, "ENTER");

continue_editing:
    do {
        stop_update();
        wclear();

        wmenubegc();
        wmenuitem(1,  1, " Address         ", 0,  1, 0, NULL, 0, 0);
        wmenuitem(2,  1, " System          ", 0, 11, 0, NULL, 0, 0);
        wmenuitem(3,  1, " Sysop Name      ", 0, 10, 0, NULL, 0, 0);
        wmenuitem(4,  1, " Packer          ", 0,  3, 0, NULL, 0, 0);
        wmenuitem(4, 33, " Mailer aka      ", 0, 26, 0, NULL, 0, 0);
        wmenuitem(5,  1, " Session Pw      ", 0,  4, 0, NULL, 0, 0);
        wmenuitem(6,  1, " Packet Pw (OUT) ", 0,  5, 0, NULL, 0, 0);
        wmenuitem(6, 33, " Packet Pw (IN)  ", 0, 25, 0, NULL, 0, 0);
        wmenuitem(7,  1, " Areafix Pw      ", 0,  6, 0, NULL, 0, 0);
        wmenuitem(8,  1, " TIC Pw          ", 0, 14, 0, NULL, 0, 0);
        wmenuitem(9,  1, " Phone           ", 0,  8, 0, NULL, 0, 0);
        wmenuitem(10,  1, " Dial Prefix     ", 0,  9, 0, NULL, 0, 0);
        wmenuitem(11,  1, " Capabilities    ", 0, 12, 0, NULL, 0, 0);
        wmenuitem(10, 33, " Min. baud rate  ", 0, 27, 0, NULL, 0, 0);
        wmenuitem(12,  1, " Echomail aka    ", 0, 13, 0, NULL, 0, 0);
        wmenuitem(13,  1, " Areafix Level   ", 0,  2, 0, NULL, 0, 0);
        wmenuitem(14,  1, " \303\304 A Flag       ", 0, 15, 0, NULL, 0, 0);
        wmenuitem(15,  1, " \303\304 B Flag       ", 0, 16, 0, NULL, 0, 0);
        wmenuitem(16,  1, " \303\304 C Flag       ", 0, 17, 0, NULL, 0, 0);
        wmenuitem(17,  1, " \300\304 D Flag       ", 0, 18, 0, NULL, 0, 0);
        wmenuitem(12, 33, " TIC aka         ", 0, 19, 0, NULL, 0, 0);
        wmenuitem(13, 33, " TIC Level       ", 0, 20, 0, NULL, 0, 0);
        wmenuitem(14, 33, " \303\304 A Flag       ", 0, 21, 0, NULL, 0, 0);
        wmenuitem(15, 33, " \303\304 B Flag       ", 0, 22, 0, NULL, 0, 0);
        wmenuitem(16, 33, " \303\304 C Flag       ", 0, 23, 0, NULL, 0, 0);
        wmenuitem(17, 33, " \300\304 D Flag       ", 0, 24, 0, NULL, 0, 0);
        wmenuend(i, M_OMNI | M_SAVE, 0, 0, LGREY | _BLACK, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY);

        sprintf(string, "%u:%u/%u.%u", ni.zone, ni.net, ni.node, ni.point);
        wprints(1, 19, CYAN | _BLACK, string);
        wprints(2, 19, CYAN | _BLACK, ni.system);
        wprints(3, 19, CYAN | _BLACK, ni.sysop_name);
        wprints(4, 19, CYAN | _BLACK, config.packers[ni.packer].id);
        sprintf(string, "%u:%u/%u.%u", config.alias[ni.mailer_aka].zone, config.alias[ni.mailer_aka].net, config.alias[ni.mailer_aka].node, config.alias[ni.mailer_aka].point);
        wprints(4, 51, CYAN | _BLACK, string);
        wprints(5, 19, CYAN | _BLACK, ni.pw_session);
        wprints(6, 19, CYAN | _BLACK, ni.pw_packet);
        wprints(6, 51, CYAN | _BLACK, ni.pw_inbound_packet);
        wprints(7, 19, CYAN | _BLACK, ni.pw_areafix);
        wprints(8, 19, CYAN | _BLACK, ni.pw_tic);
        wprints(9, 19, CYAN | _BLACK, ni.phone);
        sprintf(string, "%d", ni.modem_type);
        wprints(10, 19, CYAN | _BLACK, string);
        strcpy(string, "----");
        if (ni.remap4d) {
            string[0] = 'M';
        }
        if (ni.wazoo) {
            string[1] = 'W';
        }
        if (ni.emsi) {
            string[2] = 'E';
        }
        if (ni.janus) {
            string[3] = 'J';
        }
        wprints(11, 19, CYAN | _BLACK, string);
        sprintf(string, "%ld", ni.min_baud_rate);
        wprints(10, 51, CYAN | _BLACK, string);

        if (ni.aka == 0) {
            ni.aka++;
        }
        sprintf(string, "%u:%u/%u.%u", config.alias[ni.aka - 1].zone, config.alias[ni.aka - 1].net, config.alias[ni.aka - 1].node, config.alias[ni.aka - 1].point);
        wprints(12, 19, CYAN | _BLACK, string);

        sprintf(string, "%d", ni.afx_level);
        wprints(13, 19, CYAN | _BLACK, string);
        wprints(14, 19, CYAN | _BLACK, get_flagA_text((ni.afx_flags >> 24) & 0xFF));
        wprints(15, 19, CYAN | _BLACK, get_flagB_text((ni.afx_flags >> 16) & 0xFF));
        wprints(16, 19, CYAN | _BLACK, get_flagC_text((ni.afx_flags >> 8) & 0xFF));
        wprints(17, 19, CYAN | _BLACK, get_flagD_text(ni.afx_flags & 0xFF));

        if (ni.tic_aka == 0) {
            ni.tic_aka++;
        }
        sprintf(string, "%u:%u/%u.%u", config.alias[ni.tic_aka - 1].zone, config.alias[ni.tic_aka - 1].net, config.alias[ni.tic_aka - 1].node, config.alias[ni.tic_aka - 1].point);
        wprints(12, 51, CYAN | _BLACK, string);

        sprintf(string, "%d", ni.tic_level);
        wprints(13, 51, CYAN | _BLACK, string);
        wprints(14, 51, CYAN | _BLACK, get_flagA_text((ni.tic_flags >> 24) & 0xFF));
        wprints(15, 51, CYAN | _BLACK, get_flagB_text((ni.tic_flags >> 16) & 0xFF));
        wprints(16, 51, CYAN | _BLACK, get_flagC_text((ni.tic_flags >> 8) & 0xFF));
        wprints(17, 51, CYAN | _BLACK, get_flagD_text(ni.tic_flags & 0xFF));

        start_update();
        i = wmenuget();

        switch (i) {
            case 1:
                sprintf(string, "%u:%u/%u.%u", ni.zone, ni.net, ni.node, ni.point);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(1, 19, string, "???????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    parse_netnode(strbtrim(string), (int *)&ni.zone, (int *)&ni.net, (int *)&ni.node, (int *)&ni.point);
                }
                break;

            case 2:
                sprintf(string, "%d", ni.afx_level);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(13, 19, string, "???", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    ni.afx_level = atoi(strbtrim(string));
                }
                break;

            case 3:
                wh = wopen(6, 32, 19, 45, 1, LCYAN | _BLACK, CYAN | _BLACK);
                wactiv(wh);
                wshadow(DGREY | _BLACK);
                wtitle(" Packer ", TRIGHT, YELLOW | _BLUE);
                stop_update();
                wmenubegc();
                for (m = 0; m < 10; m++) {
                    sprintf(packs[m], " %-8.8s ", config.packers[m].id);
                    wmenuitem(m + 1, 1, packs[m], 0, m + 20, 0, NULL, 0, 0);
                }
                wmenuend(ni.packer + 20, M_OMNI | M_SAVE, 0, 0, LGREY | _BLACK, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY);
                start_update();
                i = wmenuget();
                wclose();
                if (i != -1) {
                    ni.packer = i - 20;
                }
                i = 3;
                break;

            case 4:
                strcpy(string, ni.pw_session);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(5, 19, string, "???????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(ni.pw_session, strbtrim(string));
                }
                break;

            case 5:
                strcpy(string, ni.pw_packet);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(6, 19, string, "????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(ni.pw_packet, strbtrim(string));
                }
                break;

            case 6:
                strcpy(string, ni.pw_areafix);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(7, 19, string, "???????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(ni.pw_areafix, strbtrim(string));
                }
                break;

            case 8:
                strcpy(string, ni.phone);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(9, 19, string, "?????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(ni.phone, strbtrim(string));
                }
                break;

            case 9:
                sprintf(string, "%d", ni.modem_type);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(10, 19, string, "?????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    ni.modem_type = atoi(strbtrim(string));
                }
                break;

            case 10:
                strcpy(string, ni.sysop_name);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(3, 19, string, "???????????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(ni.sysop_name, strbtrim(string));
                }
                break;

            case 11:
                strcpy(string, ni.system);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(2, 19, string, "???????????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(ni.system, strbtrim(string));
                }
                break;

            case 12:
                i = 51;
                wh = wopen(9, 32, 16, 56, 1, LCYAN | _BLACK, CYAN | _BLACK);
                wactiv(wh);
                wshadow(DGREY | _BLACK);
                wtitle(" Capabilities ", TRIGHT, YELLOW | _BLUE);
                do {
                    stop_update();
                    wclear();

                    wmenubegc();
                    wmenuitem(1, 1, " Remap to point ", 0, 51, 0, NULL, 0, 0);
                    wmenuitem(2, 1, " WaZOO          ", 0, 53, 0, NULL, 0, 0);
                    wmenuitem(3, 1, " EMSI           ", 0, 54, 0, NULL, 0, 0);
                    wmenuitem(4, 1, " Janus          ", 0, 55, 0, NULL, 0, 0);
                    wmenuend(i, M_OMNI | M_SAVE, 0, 0, LGREY | _BLACK, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY);

                    wprints(1, 18, CYAN | _BLACK, ni.remap4d ? "Yes" : "No");
                    wprints(2, 18, CYAN | _BLACK, ni.wazoo ? "Yes" : "No");
                    wprints(3, 18, CYAN | _BLACK, ni.emsi ? "Yes" : "No");
                    wprints(4, 18, CYAN | _BLACK, ni.janus ? "Yes" : "No");

                    start_update();
                    i = wmenuget();
                    switch (i) {
                        case 51:
                            ni.remap4d ^= 1;
                            break;

                        case 53:
                            ni.wazoo ^= 1;
                            break;

                        case 54:
                            ni.emsi ^= 1;
                            break;

                        case 55:
                            ni.janus ^= 1;
                            break;
                    }
                } while (i != -1);

                wclose();
                i = 12;
                break;


            case 13:
                mx = 0;
                for (m = 0; m <= MAX_ALIAS && config.alias[m].net; m++) {
                    /* MM */
                    sprintf(temp, "%d:%d/%d.%d", config.alias[m].zone, config.alias[m].net, config.alias[m].node, config.alias[m].point);
                    sprintf(string, " %-18.18s ", temp);
                    /* MM*/					akas[m] = (char *)malloc(strlen(string) + 1);
                    strcpy(akas[m], string);
                    if (strlen(string) > mx) {
                        mx = strlen(string);
                    }
                }
                akas[m] = NULL;
                /* MM */ // Modificata la gestione del wpickstr, la finestra e'
                // a dimensione fissa e non viene piu' calcolata come prima.
                // Se necessario aumentare le dimensioni di wopen w wpickstr
                wh1 = wopen(10, 25, 20, 48, 1, LCYAN | _BLACK, CYAN | _BLACK);
                wactiv(wh1);
                wshadow(DGREY | _BLACK);
                wtitle(" AKAs ", TRIGHT, YELLOW | _BLUE);
                m = wpickstr(12, 27, 18, 46, 5, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY, akas, ni.aka - 1, NULL);
                /* MM */
                if (m != -1) {
                    ni.aka = m + 1;
                }
                wclose();
                for (m = 0; m <= MAX_ALIAS && config.alias[m].net; m++) {
                    free(akas[m]);
                }
                break;

            case 14:
                strcpy(string, ni.pw_tic);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(8, 19, string, "???????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(ni.pw_tic, strbtrim(string));
                }
                break;

            case 15:
                ni.afx_flags = window_get_flags(8, 34, 1, ni.afx_flags);
                break;

            case 16:
                ni.afx_flags = window_get_flags(8, 34, 2, ni.afx_flags);
                break;

            case 17:
                ni.afx_flags = window_get_flags(8, 34, 3, ni.afx_flags);
                break;

            case 18:
                ni.afx_flags = window_get_flags(8, 34, 4, ni.afx_flags);
                break;


            case 19:
                mx = 0;
                for (m = 0; m <= MAX_ALIAS && config.alias[m].net; m++) {
                    /* MM */
                    sprintf(temp, "%d:%d/%d.%d", config.alias[m].zone, config.alias[m].net, config.alias[m].node, config.alias[m].point);
                    sprintf(string, " %-18.18s ", temp);
                    /* MM */					akas[m] = (char *)malloc(strlen(string) + 1);
                    strcpy(akas[m], string);
                    if (strlen(string) > mx) {
                        mx = strlen(string);
                    }
                }
                akas[m] = NULL;
                /* MM */ // Modificata la gestione del wpickstr, la finestra e'
                // a dimensione fissa e non viene piu' calcolata come prima.
                // Se necessario aumentare le dimensioni di wopen w wpickstr
                wh1 = wopen(10, 25, 20, 48, 1, LCYAN | _BLACK, CYAN | _BLACK);
                wactiv(wh1);
                wshadow(DGREY | _BLACK);
                wtitle(" AKAs ", TRIGHT, YELLOW | _BLUE);
                m = wpickstr(12, 27, 18, 46, 5, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY, akas, ni.tic_aka - 1, NULL);
                /* MM */				if (m != -1) {
                    ni.tic_aka = m + 1;
                }
                wclose();
                for (m = 0; m <= MAX_ALIAS && config.alias[m].net; m++) {
                    free(akas[m]);
                }
                break;

            case 20:
                sprintf(string, "%d", ni.tic_level);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(13, 51, string, "???", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    ni.tic_level = atoi(strbtrim(string));
                }
                break;

            case 21:
                ni.tic_flags = window_get_flags(8, 46, 1, ni.tic_flags);
                break;

            case 22:
                ni.tic_flags = window_get_flags(8, 46, 2, ni.tic_flags);
                break;

            case 23:
                ni.tic_flags = window_get_flags(8, 46, 3, ni.tic_flags);
                break;

            case 24:
                ni.tic_flags = window_get_flags(8, 46, 4, ni.tic_flags);
                break;

            case 25:
                strcpy(string, ni.pw_inbound_packet);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(6, 51, string, "????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(ni.pw_inbound_packet, strbtrim(string));
                }
                break;

            case 26:
                mx = 0;
                for (m = 0; m < MAX_ALIAS && config.alias[m].net; m++) {
                    /* MM */
                    sprintf(temp, "%d:%d/%d.%d", config.alias[m].zone, config.alias[m].net, config.alias[m].node, config.alias[m].point);
                    sprintf(string, " %-18.18s ", temp);
                    /* MM */					akas[m] = (char *)malloc(strlen(string) + 1);
                    strcpy(akas[m], string);
                    if (strlen(string) > mx) {
                        mx = strlen(string);
                    }
                }
                akas[m] = NULL;
                /* MM */ // Modificata la gestione del wpickstr, la finestra e'
                // a dimensione fissa e non viene piu' calcolata come prima.
                // Se necessario aumentare le dimensioni di wopen w wpickstr
                wh1 = wopen(10, 25, 20, 48, 1, LCYAN | _BLACK, CYAN | _BLACK);
                wactiv(wh1);
                wshadow(DGREY | _BLACK);
                wtitle(" AKAs ", TRIGHT, YELLOW | _BLUE);
                m = wpickstr(12, 27, 18, 46, 5, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY, akas, ni.mailer_aka - 1, NULL);
                /* MM */				if (m != -1) {
                    ni.mailer_aka = m;
                }
                wclose();
                for (m = 0; m < MAX_ALIAS && config.alias[m].net; m++) {
                    free(akas[m]);
                }
                break;

            case 27:
                sprintf(string, "%ld", ni.min_baud_rate);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                wprints(10, 51, CYAN | _BLACK, string);
                winpdef(10, 51, string, "??????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    ni.min_baud_rate = atoi(strbtrim(string));
                }
                break;
        }

        hidecur();
    } while (i != -1);

    if (memcmp((char *)&ni, (char *)nip, sizeof(NODEINFO))) {
        wh = wopen(10, 25, 14, 54, 0, BLACK | _LGREY, BLACK | _LGREY);
        wactiv(wh);
        wshadow(DGREY | _BLACK);

        wcenters(1, BLACK | _LGREY, "Save changes (Y/n) ?  ");

        strcpy(string, "Y");
        winpbeg(BLACK | _LGREY, BLACK | _LGREY);
        winpdef(1, 24, string, "?", 0, 2, NULL, 0);

        i = winpread();
        wclose();
        hidecur();

        if (i == W_ESCPRESS) {
            goto continue_editing;
        }

        if (toupper(string[0]) == 'Y') {
            memcpy((char *)nip, (char *)&ni, sizeof(NODEINFO));
        }
    }

    gotoxy_(24, 1);
    clreol_();
    prints(24, 1, LGREY | _BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add New Node  C-Copy  L-List  D-Delete");
    prints(24, 1, YELLOW | _BLACK, "PgUp/PgDn");
    prints(24, 26, YELLOW | _BLACK, "E");
    prints(24, 34, YELLOW | _BLACK, "A");
    prints(24, 50, YELLOW | _BLACK, "C");
    prints(24, 58, YELLOW | _BLACK, "L");
    prints(24, 66, YELLOW | _BLACK, "D");
}

static void select_nodes_list(fd, oni)
int fd;
NODEINFO * oni;
{
    int wh, i, x = 0;
    char string[80], **array, addr[50];
    NODEINFO ni;
    long pos;

    i = (int)(filelength(fd) / sizeof(NODEINFO));
    i += 2;
    if ((array = (char **)malloc(i * sizeof(char *))) == NULL) {
        return;
    }

    pos = tell(fd) - sizeof(NODEINFO);
    lseek(fd, 0L, SEEK_SET);
    i = 0;

    while (read(fd, (char *)&ni, sizeof(NODEINFO)) == sizeof(NODEINFO)) {
        if (memcmp((char *)&ni, (char *)oni, sizeof(NODEINFO)) == 0) {
            x = i;
        }
        sprintf(addr, "%d:%d/%d.%d", ni.zone, ni.net, ni.node, ni.point);
        sprintf(string, " %-20.20s %-6.6s %-20.20s %5d ", addr, config.packers[ni.packer].id, ni.pw_session, ni.afx_level);
        array[i] = (char *)malloc(strlen(string) + 1);
        if (array[i] == NULL) {
            break;
        }
        strcpy(array[i++], string);
    }
    array[i] = NULL;

    wh = wopen(7, 5, 20, 64, 1, LCYAN | _BLACK, CYAN | _BLACK);
    wactiv(wh);
    wshadow(DGREY | _BLACK);
    wtitle(" Select node ", TRIGHT, YELLOW | _BLUE);

    wprints(0, 2, LGREY | _BLACK, "Node #               Packer Password             Level");
    whline(1, 0, 58, 0, BLUE | _BLACK);

    gotoxy_(24, 1);
    clreol_();
    prints(24, 1, LGREY | _BLACK, "\030\031-Move bar  ENTER-Select");
    prints(24, 1, YELLOW | _BLACK, "\030\031");
    prints(24, 14, YELLOW | _BLACK, "ENTER");

    x = wpickstr(10, 7, 19, 62, 5, LGREY | _BLACK, CYAN | _BLACK, BLUE | _LGREY, array, x, NULL);

    if (x == -1) {
        lseek(fd, pos, SEEK_SET);
    }
    else {
        lseek(fd, (long)x * sizeof(NODEINFO), SEEK_SET);
    }
    read(fd, (char *)oni, sizeof(NODEINFO));

    wclose();

    i = 0;
    while (array[i] != NULL) {
        free(array[i++]);
    }
    free(array);

    gotoxy_(24, 1);
    clreol_();
    prints(24, 1, LGREY | _BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add New Node  C-Copy  L-List  D-Delete");
    prints(24, 1, YELLOW | _BLACK, "PgUp/PgDn");
    prints(24, 26, YELLOW | _BLACK, "E");
    prints(24, 34, YELLOW | _BLACK, "A");
    prints(24, 50, YELLOW | _BLACK, "C");
    prints(24, 58, YELLOW | _BLACK, "L");
    prints(24, 66, YELLOW | _BLACK, "D");
}

void manager_translation()
{
    int fd, wh, i = 1, saved, fdi, wh1, wh2, m;
    char string[128], readed, filename[80];
    long pos;
    ACCOUNT ai, bai;

    sprintf(string, "%sCOST.DAT", config.net_info);
    fd = sh_open(string, SH_DENYNONE, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
    if (fd == -1) {
        return;
    }

    if (!read(fd, (char *)&ai, sizeof(ACCOUNT))) {
        memset((char *)&ai, 0, sizeof(ACCOUNT));
        readed = 0;
    }
    else {
        readed = 1;
    }

    gotoxy_(24, 1);
    clreol_();
    prints(24, 1, LGREY | _BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add  L-List  D-Delete  C-Copy");
    prints(24, 1, YELLOW | _BLACK, "PgUp/PgDn");
    prints(24, 26, YELLOW | _BLACK, "E");
    prints(24, 34, YELLOW | _BLACK, "A");
    prints(24, 41, YELLOW | _BLACK, "L");
    prints(24, 49, YELLOW | _BLACK, "D");
    prints(24, 59, YELLOW | _BLACK, "C");

    wh = wopen(2, 6, 21, 74, 1, LCYAN | _BLACK, CYAN | _BLACK);
    wshadow(DGREY | _BLACK);
    wtitle(" Translation/Cost ", TRIGHT, YELLOW | _BLUE);

    do {
        stop_update();
        wclear();

        wprints(1, 10, LGREY | _BLACK, " Location     ");
        wprints(2, 10, LGREY | _BLACK, " Prefix       ");
        wprints(3, 10, LGREY | _BLACK, " Translate to ");

        wprints(6, 2, LGREY | _BLACK, "\263 S M T W T F S \263Start\263 End \263 Cost / Seconds \263 Cost / Seconds \263");

        if (readed) {
            wprints(1, 25, CYAN | _BLACK, ai.location);
            wprints(2, 25, CYAN | _BLACK, ai.search);
            wprints(3, 25, CYAN | _BLACK, ai.traslate);
            for (i = 0; i < 9; i++) {
                if (!ai.cost[i].start && !ai.cost[i].stop) {
                    continue;
                }

                sprintf(string, "\263               \263%2d:%02d\263%2d:%02d\263 %4d / %3d.%d   \263 %4d / %3d.%d   \263",
                        ai.cost[i].start / 60, ai.cost[i].start % 60,
                        ai.cost[i].stop / 60, ai.cost[i].stop % 60,
                        ai.cost[i].cost_first, ai.cost[i].time_first / 10, ai.cost[i].time_first % 10,
                        ai.cost[i].cost, ai.cost[i].time / 10, ai.cost[i].time % 10);
                if (ai.cost[i].days & DAY_SUNDAY) {
                    string[2] = 'S';
                }
                if (ai.cost[i].days & DAY_MONDAY) {
                    string[4] = 'M';
                }
                if (ai.cost[i].days & DAY_TUESDAY) {
                    string[6] = 'T';
                }
                if (ai.cost[i].days & DAY_WEDNESDAY) {
                    string[8] = 'W';
                }
                if (ai.cost[i].days & DAY_THURSDAY) {
                    string[10] = 'T';
                }
                if (ai.cost[i].days & DAY_FRIDAY) {
                    string[12] = 'F';
                }
                if (ai.cost[i].days & DAY_SATURDAY) {
                    string[14] = 'S';
                }
                wprints(i + 8, 2, LGREY | _BLACK, string);
            }
        }

        start_update();

        wbox(5, 2, 17, 64, 0, LGREY | _BLACK);
        whline(7, 2, 63, 0, LGREY | _BLACK);
        wvline(5, 18, 13, 0, LGREY | _BLACK);
        wvline(5, 24, 13, 0, LGREY | _BLACK);
        wvline(5, 30, 13, 0, LGREY | _BLACK);
        wvline(5, 47, 13, 0, LGREY | _BLACK);
#ifdef __OS2__
        VioUpdate();
#endif

        i = getxch();
        if ((i & 0xFF) != 0) {
            i &= 0xFF;
        }

        switch (i) {
            // PgDn
            case 0x5100:
                if (readed) {
                    read(fd, (char *)&ai, sizeof(ACCOUNT));
                }
                break;

            // PgUp
            case 0x4900:
                if (readed) {
                    if (tell(fd) > sizeof(ACCOUNT)) {
                        lseek(fd, -2L * sizeof(ACCOUNT), SEEK_CUR);
                        read(fd, (char *)&ai, sizeof(ACCOUNT));
                    }
                }
                break;

            // C Copy
            case 'C':
            case 'c':
                ai.search[0] = ai.traslate[0] = ai.location[0] = '\0';

                wh2 = wh;
                i = 1;

                wh = wopen(6, 10, 12, 58, 1, LCYAN | _BLACK, CYAN | _BLACK);
                wactiv(wh);
                wshadow(DGREY | _BLACK);
                wtitle(" Copy translation ", TRIGHT, YELLOW | _BLUE);

                do {
                    stop_update();
                    wclear();

                    wmenubegc();
                    wmenuitem(1, 1, " Location     ", 0, 1, 0, NULL, 0, 0);
                    wmenuitem(2, 1, " Prefix       ", 0, 2, 0, NULL, 0, 0);
                    wmenuitem(3, 1, " Translate to ", 0, 3, 0, NULL, 0, 0);

                    wprints(1, 16, CYAN | _BLACK, ai.location);
                    wprints(2, 16, CYAN | _BLACK, ai.search);
                    wprints(3, 16, CYAN | _BLACK, ai.traslate);

                    wmenuend(i, M_OMNI | M_SAVE, 0, 0, LGREY | _BLACK, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY);

                    start_update();
                    i = wmenuget();

                    switch (i) {
                        case 1:
                            strcpy(string, ai.location);
                            winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                            winpdef(1, 16, string, "?????????????????????????????", 0, 2, NULL, 0);
                            if (winpread() != W_ESCPRESS) {
                                strcpy(ai.location, strbtrim(string));
                            }
                            break;

                        case 2:
                            strcpy(string, ai.search);
                            winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                            winpdef(2, 16, string, "???????????????????", 0, 2, NULL, 0);
                            if (winpread() != W_ESCPRESS) {
                                strcpy(ai.search, strbtrim(string));
                            }
                            break;

                        case 3:
                            strcpy(string, ai.traslate);
                            winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                            winpdef(3, 16, string, "????????????????????????????????????????????", 0, 2, NULL, 0);
                            if (winpread() != W_ESCPRESS) {
                                strcpy(ai.traslate, strbtrim(string));
                            }
                            break;
                    }

                    hidecur();
                } while (i != -1);

                wclose();
                wh = wh2;
                wactiv(wh);

                if (ai.search[0]) {
                    pos = tell(fd) - (long)sizeof(ACCOUNT);
                    close(fd);

                    sprintf(filename, "%sCOST.BAK", config.net_info);
                    unlink(filename);
                    sprintf(string, "%sCOST.DAT", config.net_info);
                    rename(string, filename);

                    fd = open(filename, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                    fdi = open(string, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
                    saved = 0;

                    while (read(fd, (char *)&bai, sizeof(ACCOUNT)) == sizeof(ACCOUNT)) {
                        m = min(strlen(bai.location), strlen(ai.location));
                        if (!saved && strncmp(bai.location, ai.location, m) > 0 || (strncmp(bai.location, ai.location, m) == 0 && strlen(bai.location) < strlen(ai.location))) {
                            pos = tell(fdi);
                            write(fdi, (char *)&ai, sizeof(ACCOUNT));
                            saved = 1;
                        }
                        if (strcmp(bai.location, ai.location)) {
                            write(fdi, (char *)&bai, sizeof(ACCOUNT));
                        }
                    }

                    if (!saved) {
                        pos = tell(fdi);
                        write(fdi, (char *)&ai, sizeof(ACCOUNT));
                    }

                    close(fd);
                    close(fdi);

                    unlink(filename);

                    sprintf(string, "%sCOST.DAT", config.net_info);
                    fd = open(string, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                    lseek(fd, pos, SEEK_SET);
                    read(fd, (char *)&ai, sizeof(ACCOUNT));
                }

                i = 'C';
                break;

            // E Edit
            case 'E':
            case 'e':
                strcpy(string, ai.location);
                edit_single_location(&ai);
                lseek(fd, -1L * sizeof(ACCOUNT), SEEK_CUR);
                write(fd, (char *)&ai, sizeof(ACCOUNT));

                if (stricmp(string, ai.location)) {
                    pos = tell(fd) - (long)sizeof(ACCOUNT);
                    close(fd);

                    sprintf(filename, "%sCOST.BAK", config.net_info);
                    unlink(filename);
                    sprintf(string, "%sCOST.DAT", config.net_info);
                    rename(string, filename);

                    fd = open(filename, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                    fdi = open(string, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
                    saved = 0;

                    while (read(fd, (char *)&bai, sizeof(ACCOUNT)) == sizeof(ACCOUNT)) {
                        m = min(strlen(bai.location), strlen(ai.location));
                        if (!saved && strncmp(bai.location, ai.location, m) > 0 || (strncmp(bai.location, ai.location, m) == 0 && strlen(bai.location) < strlen(ai.location))) {
                            pos = tell(fdi);
                            write(fdi, (char *)&ai, sizeof(ACCOUNT));
                            saved = 1;
                        }
                        if (strcmp(bai.location, ai.location)) {
                            write(fdi, (char *)&bai, sizeof(ACCOUNT));
                        }
                    }

                    if (!saved) {
                        pos = tell(fdi);
                        write(fdi, (char *)&ai, sizeof(ACCOUNT));
                    }

                    close(fd);
                    close(fdi);

                    unlink(filename);

                    sprintf(string, "%sCOST.DAT", config.net_info);
                    fd = open(string, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                    lseek(fd, pos, SEEK_SET);
                    read(fd, (char *)&ai, sizeof(ACCOUNT));
                }
                break;

            // D Delete
            case 'D':
            case 'd':
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

                if (toupper(string[0]) == 'Y') {
                    pos = tell(fd) - (long)sizeof(ACCOUNT);
                    close(fd);

                    sprintf(filename, "%sCOST.BAK", config.net_info);
                    unlink(filename);
                    sprintf(string, "%sCOST.DAT", config.net_info);
                    rename(string, filename);

                    fd = open(filename, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                    fdi = open(string, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
                    saved = 0;

                    while (read(fd, (char *)&bai, sizeof(ACCOUNT)) == sizeof(ACCOUNT)) {
                        if (strcmp(bai.location, ai.location)) {
                            write(fdi, (char *)&bai, sizeof(ACCOUNT));
                        }
                    }

                    close(fd);
                    close(fdi);

                    unlink(filename);

                    sprintf(string, "%sCOST.DAT", config.net_info);
                    fd = open(string, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                    if (read(fd, (char *)&ai, sizeof(ACCOUNT)) < sizeof(ACCOUNT)) {
                        lseek(fd, 0L, SEEK_SET);
                        read(fd, (char *)&ai, sizeof(ACCOUNT));
                    }
                }
                break;

            // L List
            case 'L':
            case 'l':
                select_cost_list(fd, &ai);
                break;

            // A Add
            case 'A':
            case 'a':
                memset((char *)&ai, 0, sizeof(ACCOUNT));
                edit_single_location(&ai);
                lseek(fd, 0L, SEEK_END);
                write(fd, (char *)&ai, sizeof(ACCOUNT));

                if (strlen(ai.search)) {
                    pos = tell(fd) - (long)sizeof(ACCOUNT);
                    close(fd);

                    sprintf(filename, "%sCOST.BAK", config.net_info);
                    unlink(filename);
                    sprintf(string, "%sCOST.DAT", config.net_info);
                    rename(string, filename);

                    fd = open(filename, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                    fdi = open(string, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
                    saved = 0;

                    while (read(fd, (char *)&bai, sizeof(ACCOUNT)) == sizeof(ACCOUNT)) {
                        m = min(strlen(bai.location), strlen(ai.location));
                        if (!saved && strncmp(bai.location, ai.location, m) > 0 || (strncmp(bai.location, ai.location, m) == 0 && strlen(bai.location) < strlen(ai.location))) {
                            pos = tell(fdi);
                            write(fdi, (char *)&ai, sizeof(ACCOUNT));
                            saved = 1;
                        }
                        if (strcmp(bai.location, ai.location)) {
                            write(fdi, (char *)&bai, sizeof(ACCOUNT));
                        }
                    }

                    if (!saved) {
                        pos = tell(fdi);
                        write(fdi, (char *)&ai, sizeof(ACCOUNT));
                    }

                    close(fd);
                    close(fdi);

                    unlink(filename);

                    sprintf(string, "%sCOST.DAT", config.net_info);
                    fd = open(string, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
                    lseek(fd, pos, SEEK_SET);
                    read(fd, (char *)&ai, sizeof(ACCOUNT));
                }
                readed = 1;
                break;

            // ESC Exit
            case 0x1B:
                i = -1;
                break;
        }

    } while (i != -1);

    close(fd);

    wclose();
    gotoxy_(24, 1);
    clreol_();
}

static void edit_single_location(aip)
ACCOUNT * aip;
{
    int wh, i = 1, x, t1, t2;
    char string[128], packs[10][70], start[10], stop[10];
    char cost1[10], seconds1[10], cost2[10], seconds2[10];
    char sun[2], mon[2], tue[2], wed[2], thu[2], fri[2], sat[2];
    ACCOUNT ai;

    memcpy((char *)&ai, (char *)aip, sizeof(ACCOUNT));
    sun[1] = mon[1] = tue[1] = wed[1] = thu[1] = fri[1] = sat[1] = '\0';

    gotoxy_(24, 1);
    clreol_();
    prints(24, 1, LGREY | _BLACK, "ESC-Exit/Save  ENTER-Edit");
    prints(24, 1, YELLOW | _BLACK, "ESC");
    prints(24, 16, YELLOW | _BLACK, "ENTER");

continue_editing:
    do {
        stop_update();
        wclear();

        wprints(1, 10, LGREY | _BLACK, " Location     ");
        wprints(2, 10, LGREY | _BLACK, " Prefix       ");
        wprints(3, 10, LGREY | _BLACK, " Translate to ");
        wprints(6, 2, LGREY | _BLACK, "\263 S M T W T F S \263Start\263 End \263 Cost / Seconds \263 Cost / Seconds \263");

        wmenubegc();
        wmenuitem(1, 10, " Location     ", 0, 1, 0, NULL, 0, 0);
        wmenuitem(2, 10, " Prefix       ", 0, 2, 0, NULL, 0, 0);
        wmenuitem(3, 10, " Translate to ", 0, 3, 0, NULL, 0, 0);

        wprints(1, 25, CYAN | _BLACK, ai.location);
        wprints(2, 25, CYAN | _BLACK, ai.search);
        wprints(3, 25, CYAN | _BLACK, ai.traslate);

        for (x = 0; x < 9; x++) {
            if (!ai.cost[x].days) {
                strcpy(packs[x],  "               \263     \263     \263                \263                ");
            }
            else {
                sprintf(packs[x], "               \263%2d:%02d\263%2d:%02d\263 %4d / %3d.%d   \263 %4d / %3d.%d   ",
                        ai.cost[x].start / 60, ai.cost[x].start % 60,
                        ai.cost[x].stop / 60, ai.cost[x].stop % 60,
                        ai.cost[x].cost_first, ai.cost[x].time_first / 10, ai.cost[x].time_first % 10,
                        ai.cost[x].cost, ai.cost[x].time / 10, ai.cost[x].time % 10);
                if (ai.cost[x].days & DAY_SUNDAY) {
                    packs[x][1] = 'S';
                }
                if (ai.cost[x].days & DAY_MONDAY) {
                    packs[x][3] = 'M';
                }
                if (ai.cost[x].days & DAY_TUESDAY) {
                    packs[x][5] = 'T';
                }
                if (ai.cost[x].days & DAY_WEDNESDAY) {
                    packs[x][7] = 'W';
                }
                if (ai.cost[x].days & DAY_THURSDAY) {
                    packs[x][9] = 'T';
                }
                if (ai.cost[x].days & DAY_FRIDAY) {
                    packs[x][11] = 'F';
                }
                if (ai.cost[x].days & DAY_SATURDAY) {
                    packs[x][13] = 'S';
                }
            }

            wmenuitem(x + 8, 3, packs[x], 0, 4 + x, 0, NULL, 0, 0);
        }

        wmenuend(i, M_OMNI | M_SAVE, 0, 0, LGREY | _BLACK, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY);

        start_update();

        wbox(5, 2, 17, 64, 0, LGREY | _BLACK);
        whline(7, 2, 63, 0, LGREY | _BLACK);
        wvline(5, 18, 13, 0, LGREY | _BLACK);
        wvline(5, 24, 13, 0, LGREY | _BLACK);
        wvline(5, 30, 13, 0, LGREY | _BLACK);
        wvline(5, 47, 13, 0, LGREY | _BLACK);
#ifdef __OS2__
        VioUpdate();
#endif

        i = wmenuget();

        switch (i) {
            case 1:
                strcpy(string, ai.location);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(1, 25, string, "?????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(ai.location, strbtrim(string));
                }
                break;

            case 2:
                strcpy(string, ai.search);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(2, 25, string, "???????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(ai.search, strbtrim(string));
                }
                break;

            case 3:
                strcpy(string, ai.traslate);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(3, 25, string, "????????????????????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(ai.traslate, strbtrim(string));
                }
                break;

            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
                if (!ai.cost[i - 4].days) {
                    strcpy(packs[i - 4],  "               \263     \263     \263                \263                ");
                }
                else {
                    sprintf(packs[i - 4], "               \263%2d:%02d\263%2d:%02d\263 %4d / %3d.%d   \263 %4d / %3d.%d   ",
                            ai.cost[i - 4].start / 60, ai.cost[i - 4].start % 60,
                            ai.cost[i - 4].stop / 60, ai.cost[i - 4].stop % 60,
                            ai.cost[i - 4].cost_first, ai.cost[i - 4].time_first / 10, ai.cost[i - 4].time_first % 10,
                            ai.cost[i - 4].cost, ai.cost[i - 4].time / 10, ai.cost[i - 4].time % 10);
                    if (ai.cost[i - 4].days & DAY_SUNDAY) {
                        packs[i - 4][1] = 'S';
                    }
                    if (ai.cost[i - 4].days & DAY_MONDAY) {
                        packs[i - 4][3] = 'M';
                    }
                    if (ai.cost[i - 4].days & DAY_TUESDAY) {
                        packs[i - 4][5] = 'T';
                    }
                    if (ai.cost[i - 4].days & DAY_WEDNESDAY) {
                        packs[i - 4][7] = 'W';
                    }
                    if (ai.cost[i - 4].days & DAY_THURSDAY) {
                        packs[i - 4][9] = 'T';
                    }
                    if (ai.cost[i - 4].days & DAY_FRIDAY) {
                        packs[i - 4][11] = 'F';
                    }
                    if (ai.cost[i - 4].days & DAY_SATURDAY) {
                        packs[i - 4][13] = 'S';
                    }
                }
                wprints(i + 4, 3, LGREY | _BLACK, packs[i - 4]);

                sun[0] = mon[0] = tue[0] = wed[0] = thu[0] = fri[0] = sat[0] = ' ';

                if (ai.cost[i - 4].days & DAY_SUNDAY) {
                    sun[0] = 'S';
                }
                if (ai.cost[i - 4].days & DAY_MONDAY) {
                    mon[0] = 'M';
                }
                if (ai.cost[i - 4].days & DAY_TUESDAY) {
                    tue[0] = 'T';
                }
                if (ai.cost[i - 4].days & DAY_WEDNESDAY) {
                    wed[0] = 'W';
                }
                if (ai.cost[i - 4].days & DAY_THURSDAY) {
                    thu[0] = 'T';
                }
                if (ai.cost[i - 4].days & DAY_FRIDAY) {
                    fri[0] = 'F';
                }
                if (ai.cost[i - 4].days & DAY_SATURDAY) {
                    sat[0] = 'S';
                }
                sprintf(start, "%2d:%02d", ai.cost[i - 4].start / 60, ai.cost[i - 4].start % 60);
                sprintf(stop, "%2d:%02d", ai.cost[i - 4].stop / 60, ai.cost[i - 4].stop % 60);
                sprintf(cost1, "%4d", ai.cost[i - 4].cost_first);
                sprintf(seconds1, "%3d.%d", ai.cost[i - 4].time_first / 10, ai.cost[i - 4].time_first % 10);
                sprintf(cost2, "%4d", ai.cost[i - 4].cost);
                sprintf(seconds2, "%3d.%d", ai.cost[i - 4].time / 10, ai.cost[i - 4].time % 10);

                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(i + 4,  4, sun, "?", 0, 2, NULL, 0);
                winpdef(i + 4,  6, mon, "?", 0, 2, NULL, 0);
                winpdef(i + 4,  8, tue, "?", 0, 2, NULL, 0);
                winpdef(i + 4, 10, wed, "?", 0, 2, NULL, 0);
                winpdef(i + 4, 12, thu, "?", 0, 2, NULL, 0);
                winpdef(i + 4, 14, fri, "?", 0, 2, NULL, 0);
                winpdef(i + 4, 16, sat, "?", 0, 2, NULL, 0);
                winpdef(i + 4, 19, start, "?????", 0, 2, NULL, 0);
                winpdef(i + 4, 25, stop, "?????", 0, 2, NULL, 0);
                winpdef(i + 4, 32, cost1, "????", 0, 2, NULL, 0);
                winpdef(i + 4, 39, seconds1, "??????", 0, 2, NULL, 0);
                winpdef(i + 4, 49, cost2, "????", 0, 2, NULL, 0);
                winpdef(i + 4, 56, seconds2, "??????", 0, 2, NULL, 0);
                winpread();

                sscanf(start, "%d:%d", &t1, &t2);
                ai.cost[i - 4].start = t1 * 60 + t2;
                sscanf(stop, "%d:%d", &t1, &t2);
                ai.cost[i - 4].stop = t1 * 60 + t2;
                ai.cost[i - 4].cost_first = atoi(cost1);
                if (sscanf(seconds1, "%d.%d", &t1, &t2) == 1) {
                    ai.cost[i - 4].time_first = t1 * 10;
                }
                else {
                    ai.cost[i - 4].time_first = t1 * 10 + t2;
                }
                ai.cost[i - 4].cost = atoi(cost2);
                if (sscanf(seconds2, "%d.%d", &t1, &t2) == 1) {
                    ai.cost[i - 4].time = t1 * 10;
                }
                else {
                    ai.cost[i - 4].time = t1 * 10 + t2;
                }
                if (sun[0] != ' ') {
                    ai.cost[i - 4].days |= DAY_SUNDAY;
                }
                else {
                    ai.cost[i - 4].days &= ~DAY_SUNDAY;
                }
                if (mon[0] != ' ') {
                    ai.cost[i - 4].days |= DAY_MONDAY;
                }
                else {
                    ai.cost[i - 4].days &= ~DAY_MONDAY;
                }
                if (tue[0] != ' ') {
                    ai.cost[i - 4].days |= DAY_TUESDAY;
                }
                else {
                    ai.cost[i - 4].days &= ~DAY_TUESDAY;
                }
                if (wed[0] != ' ') {
                    ai.cost[i - 4].days |= DAY_WEDNESDAY;
                }
                else {
                    ai.cost[i - 4].days &= ~DAY_WEDNESDAY;
                }
                if (thu[0] != ' ') {
                    ai.cost[i - 4].days |= DAY_THURSDAY;
                }
                else {
                    ai.cost[i - 4].days &= ~DAY_THURSDAY;
                }
                if (fri[0] != ' ') {
                    ai.cost[i - 4].days |= DAY_FRIDAY;
                }
                else {
                    ai.cost[i - 4].days &= ~DAY_FRIDAY;
                }
                if (sat[0] != ' ') {
                    ai.cost[i - 4].days |= DAY_SATURDAY;
                }
                else {
                    ai.cost[i - 4].days &= ~DAY_SATURDAY;
                }
        }

        hidecur();
    } while (i != -1);

    if (memcmp((char *)&ai, (char *)aip, sizeof(ACCOUNT))) {
        wh = wopen(10, 25, 14, 54, 0, BLACK | _LGREY, BLACK | _LGREY);
        wactiv(wh);
        wshadow(DGREY | _BLACK);

        wcenters(1, BLACK | _LGREY, "Save changes (Y/n) ?  ");

        strcpy(string, "Y");
        winpbeg(BLACK | _LGREY, BLACK | _LGREY);
        winpdef(1, 24, string, "?", 0, 2, NULL, 0);

        i = winpread();
        wclose();
        hidecur();

        if (i == W_ESCPRESS) {
            goto continue_editing;
        }

        if (toupper(string[0]) == 'Y') {
            memcpy((char *)aip, (char *)&ai, sizeof(ACCOUNT));
        }
    }

    gotoxy_(24, 1);
    clreol_();
    prints(24, 1, LGREY | _BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add  L-List  D-Delete  C-Copy");
    prints(24, 1, YELLOW | _BLACK, "PgUp/PgDn");
    prints(24, 26, YELLOW | _BLACK, "E");
    prints(24, 34, YELLOW | _BLACK, "A");
    prints(24, 41, YELLOW | _BLACK, "L");
    prints(24, 49, YELLOW | _BLACK, "D");
    prints(24, 59, YELLOW | _BLACK, "C");
}

#define COSTARRAY 150

static void select_cost_list(fd, ai)
int fd;
ACCOUNT * ai;
{
    int wh, i, beg;
    char string[128], **array;
    ACCOUNT bai;

    wh = wopen(3, 2, 21, 76, 1, LCYAN | _BLACK, CYAN | _BLACK);
    wactiv(wh);
    wshadow(DGREY | _BLACK);
    wtitle(" Account/Traslation ", TRIGHT, YELLOW | _BLUE);

    wprints(0, 0, LGREY | _BLACK, " Location                      Prefix              Traslate to          ");
    whline(1, 0, 76, 0, BLUE | _BLACK);

    gotoxy_(24, 1);
    clreol_();
    prints(24, 1, LGREY | _BLACK, "\030\031-Move bar  ENTER-Select");
    prints(24, 1, YELLOW | _BLACK, "\030\031");
    prints(24, 14, YELLOW | _BLACK, "ENTER");

    i = (int)(filelength(fd) / sizeof(ACCOUNT));
    i += 2;
    if ((array = (char **)malloc(i * sizeof(char *))) == NULL) {
        return;
    }

    beg = i = 0;
    lseek(fd, 0L, SEEK_SET);

    while (read(fd, (char *)&bai, sizeof(ACCOUNT)) == sizeof(ACCOUNT)) {
        if (memcmp((char *)&bai, (char *)ai, sizeof(ACCOUNT)) == 0) {
            beg = i;
        }
        sprintf(string, " %-28.28s %-19.19s %-20.20s ", bai.location, bai.search, bai.traslate);
        array[i] = (char *)malloc(strlen(string) + 1);
        if (array[i] == NULL) {
            break;
        }
        strcpy(array[i++], string);
    }

    array[i] = NULL;

    i = wpickstr(6, 3, 20, 75, 5, LGREY | _BLACK, CYAN | _BLACK, BLUE | _LGREY, array, beg, NULL);

    if (i != -1) {
        lseek(fd, (long)sizeof(ACCOUNT) * (long)i, SEEK_SET);
        read(fd, (char *)ai, sizeof(ACCOUNT));
    }

    i = 0;
    while (array[i] != NULL) {
        free(array[i++]);
    }
    free(array);

    wclose();

    gotoxy_(24, 1);
    clreol_();
    prints(24, 1, LGREY | _BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add  L-List  D-Delete  C-Copy");
    prints(24, 1, YELLOW | _BLACK, "PgUp/PgDn");
    prints(24, 26, YELLOW | _BLACK, "E");
    prints(24, 34, YELLOW | _BLACK, "A");
    prints(24, 41, YELLOW | _BLACK, "L");
    prints(24, 49, YELLOW | _BLACK, "D");
    prints(24, 59, YELLOW | _BLACK, "C");
}

