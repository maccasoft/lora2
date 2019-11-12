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
#include <string.h>
#include <io.h>
#include <share.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys\stat.h>

#include "lsetup.h"
#include "lprot.h"

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>
#include <cxl\cxlkey.h>

extern struct _configuration config;

void start_update(void);
void stop_update(void);
char * get_priv_text(int);
void create_path(char *);
int sh_open(char * file, int shmode, int omode, int fmode);
void update_message(void);
long window_get_flags(int y, int x, int type, long f);
int winputs(int wy, int wx, char * stri, char * fmt, int mode, char pad, int fieldattr, int textattr);

static void edit_single_area(struct _sys *);
static void select_area_list(int, struct _sys *);

void bbs_newusers()
{
    int wh, i = 1;
    char string[128];

    wh = wopen(4, 5, 21, 69, 1, LCYAN | _BLACK, CYAN | _BLACK);
    wactiv(wh);
    wshadow(DGREY | _BLACK);
    wtitle(" New users ", TRIGHT, YELLOW | _BLUE);

    do {
        stop_update();
        wclear();

        wmenubegc();
        wmenuitem(1,  1, " New user sec. level ", 0, 1, 0, NULL, 0, 0);
        wmenuitem(2,  1, " New user A flags    ", 0, 2, 0, NULL, 0, 0);
        wmenuitem(3,  1, " New user B flags    ", 0, 3, 0, NULL, 0, 0);
        wmenuitem(4,  1, " New user C flags    ", 0, 4, 0, NULL, 0, 0);
        wmenuitem(5,  1, " New user D flags    ", 0, 5, 0, NULL, 0, 0);
        wmenuitem(6,  1, " Ask birthdate          ", 0, 6, 0, NULL, 0, 0);
        wmenuitem(6, 32, " Ask voice phone number ", 0, 7, 0, NULL, 0, 0);
        wmenuitem(7,  1, " Ask data phone number  ", 0, 8, 0, NULL, 0, 0);
        wmenuitem(7, 32, " Ask IBM character set  ", 0, 9, 0, NULL, 0, 0);
        wmenuitem(8,  1, " Ask alias name         ", 0, 20, 0, NULL, 0, 0);
        wmenuitem(8, 32, " Ask default protocol   ", 0, 21, 0, NULL, 0, 0);
        wmenuitem(9,  1, " Ask default packer     ", 0, 22, 0, NULL, 0, 0);
        wmenuitem(9, 32, " FileBOX default        ", 0, 23, 0, NULL, 0, 0);
        wmenuitem(10,  1, " More prompts           ", 0, 10, 0, NULL, 0, 0);
        wmenuitem(10, 32, " Mail check             ", 0, 12, 0, NULL, 0, 0);
        wmenuitem(11,  1, " New files check        ", 0, 11, 0, NULL, 0, 0);
        wmenuitem(11, 32, " Screen clears          ", 0, 13, 0, NULL, 0, 0);
        wmenuitem(12,  1, " Hotkeys                ", 0, 14, 0, NULL, 0, 0);
        wmenuitem(12, 32, " Ansi graphics          ", 0, 15, 0, NULL, 0, 0);
        wmenuitem(13,  1, " Avatar graphics        ", 0, 16, 0, NULL, 0, 0);
        wmenuitem(13, 32, " Full screen editor     ", 0, 17, 0, NULL, 0, 0);
        wmenuitem(14,  1, " Rookie calls           ", 0, 18, 0, NULL, 0, 0);
        wmenuitem(14, 32, " Random birthdate check ", 0, 19, 0, NULL, 0, 0);
        wmenuend(i, M_OMNI | M_SAVE, 0, 0, LGREY | _BLACK, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY);

        wprints(1, 22, CYAN | _BLACK, get_priv_text(config.logon_level));
        wprints(2, 23, CYAN | _BLACK, get_flagA_text((config.logon_flags >> 24) & 0xFF));
        wprints(3, 23, CYAN | _BLACK, get_flagB_text((config.logon_flags >> 16) & 0xFF));
        wprints(4, 23, CYAN | _BLACK, get_flagC_text((config.logon_flags >> 8) & 0xFF));
        wprints(5, 23, CYAN | _BLACK, get_flagD_text(config.logon_flags & 0xFF));
        wprints(6, 26, CYAN | _BLACK, config.birthdate ? "Yes" : "No");
        wprints(6, 57, CYAN | _BLACK, config.voicephone ? "Yes" : "No");
        wprints(7, 26, CYAN | _BLACK, config.dataphone ? "Yes" : "No");
        wprints(7, 57, CYAN | _BLACK, config.ibmset ? "Yes" : "No");
        wprints(8, 26, CYAN | _BLACK, config.ask_alias ? "Yes" : "No");
        wprints(8, 57, CYAN | _BLACK, config.ask_protocol ? "Yes" : "No");
        wprints(9, 26, CYAN | _BLACK, config.ask_packer ? "Yes" : "No");
        wprints(9, 57, CYAN | _BLACK, config.filebox ? "Yes" : "No");
        wprints(10, 26, CYAN | _BLACK, config.moreprompt == 1 ? "Yes" : (config.moreprompt == 2 ? "Ask" : "No"));
        wprints(10, 57, CYAN | _BLACK, config.mailcheck == 1 ? "Yes" : (config.mailcheck == 2 ? "Ask" : "No"));
        wprints(11, 26, CYAN | _BLACK, config.newfilescheck == 1 ? "Yes" : (config.newfilescheck == 2 ? "Ask" : "No"));
        wprints(11, 57, CYAN | _BLACK, config.screenclears == 1 ? "Yes" : (config.screenclears == 2 ? "Ask" : "No"));
        wprints(12, 26, CYAN | _BLACK, config.hotkeys == 1 ? "Yes" : (config.hotkeys == 2 ? "Ask" : "No"));
        wprints(12, 57, CYAN | _BLACK, config.ansigraphics == 1 ? "Yes" : (config.ansigraphics == 2 ? "Ask" : "No"));
        wprints(13, 26, CYAN | _BLACK, config.avatargraphics == 1 ? "Yes" : (config.avatargraphics == 2 ? "Ask" : "No"));
        wprints(13, 57, CYAN | _BLACK, config.fullscrnedit == 1 ? "Yes" : (config.fullscrnedit == 2 ? "Ask" : "No"));
        sprintf(string, "%d", config.rookie_calls);
        wprints(14, 26, CYAN | _BLACK, string);
        wprints(14, 57, CYAN | _BLACK, config.random_birth ? "Yes" : "No");

        start_update();
        i = wmenuget();

        switch (i) {
            case 1:
                config.logon_level = select_level(config.logon_level, 37, 4);
                break;

            case 2:
                config.logon_flags = window_get_flags(3, 35, 1, config.logon_flags);
                break;

            case 3:
                config.logon_flags = window_get_flags(3, 35, 2, config.logon_flags);
                break;

            case 4:
                config.logon_flags = window_get_flags(3, 35, 3, config.logon_flags);
                break;

            case 5:
                config.logon_flags = window_get_flags(3, 35, 4, config.logon_flags);
                break;

            case 6:
                config.birthdate ^= 1;
                break;

            case 7:
                config.voicephone ^= 1;
                break;

            case 8:
                config.dataphone ^= 1;
                break;

            case 9:
                config.ibmset ^= 1;
                break;

            case 10:
                if (++config.moreprompt == 3) {
                    config.moreprompt = 0;
                }
                break;

            case 11:
                if (++config.newfilescheck == 3) {
                    config.newfilescheck = 0;
                }
                break;

            case 12:
                if (++config.mailcheck == 3) {
                    config.mailcheck = 0;
                }
                break;

            case 13:
                if (++config.screenclears == 3) {
                    config.screenclears = 0;
                }
                break;

            case 14:
                if (++config.hotkeys == 3) {
                    config.hotkeys = 0;
                }
                break;

            case 15:
                if (++config.ansigraphics == 3) {
                    config.ansigraphics = 0;
                }
                break;

            case 16:
                if (++config.avatargraphics == 3) {
                    config.avatargraphics = 0;
                }
                break;

            case 17:
                if (++config.fullscrnedit == 3) {
                    config.fullscrnedit = 0;
                }
                break;

            case 18:
                sprintf(string, "%d", config.rookie_calls);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(14, 26, string, "?????", 0, 1, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    config.rookie_calls = atoi(strbtrim(string));
                }
                break;

            case 19:
                config.random_birth ^= 1;
                break;

            case 20:
                config.ask_alias ^= 1;
                break;

            case 21:
                config.ask_protocol ^= 1;
                break;

            case 22:
                config.ask_packer ^= 1;
                break;

            case 23:
                config.filebox ^= 1;
                break;
        }

        hidecur();

    } while (i != -1);

    wclose();
}

void bbs_general()
{
    int wh, i = 1;
    char string[128];

    wh = wopen(2, 4, 23, 76, 1, LCYAN | _BLACK, CYAN | _BLACK);
    wactiv(wh);
    wshadow(DGREY | _BLACK);
    wtitle(" General options ", TRIGHT, YELLOW | _BLUE);

    do {
        stop_update();
        wclear();

        wmenubegc();
        wmenuitem(1, 1, " Users file       ", 0, 1, 0, NULL, 0, 0);
        wmenuitem(2, 1, " Menu dir         ", 0, 2, 0, NULL, 0, 0);
        wmenuitem(3, 1, " General text dir ", 0, 3, 0, NULL, 0, 0);
        wmenuitem(4, 1, " Full scr. editor ", 0, 4, 0, NULL, 0, 0);
        wmenuitem(5, 1, " Quote header     ", 0, 25, 0, NULL, 0, 0);
        wmenuitem(6, 1, " Upload check     ", 0, 28, 0, NULL, 0, 0);
        wmenuitem(8, 1, " ANSI at logon      ", 0, 5, 0, NULL, 0, 0);
        wmenuitem(8, 30, " Aftercaller exit   ", 0, 9, 0, NULL, 0, 0);
        wmenuitem(9, 1, " Areachange keys    ", 0, 10, 0, NULL, 0, 0);
        wmenuitem(9, 30, " Date format        ", 0, 11, 0, NULL, 0, 0);
        wmenuitem(10, 1, " Snoop on-line user ", 0, 8, 0, NULL, 0, 0);
        wmenuitem(10, 30, " Time format        ", 0, 12, 0, NULL, 0, 0);
        wmenuitem(11, 1, " Download counter   ", 0, 13, 0, NULL, 0, 0);
        wmenuitem(11, 30, " Uploader name      ", 0, 14, 0, NULL, 0, 0);
        wmenuitem(12, 1, " \300 Counter limits   ", 0, 15, 0, NULL, 0, 0);
        wmenuitem(12, 30, " Login check city   ", 0, 16, 0, NULL, 0, 0);
        wmenuitem(13, 1, " Inactivity timeout ", 0, 17, 0, NULL, 0, 0);
        wmenuitem(13, 30, " IEMSI logins       ", 0, 18, 0, NULL, 0, 0);
        wmenuitem(14, 1, " Show missing files ", 0, 19, 0, NULL, 0, 0);
        wmenuitem(14, 30, " ZModem protocol    ", 0, 20, 0, NULL, 0, 0);
        wmenuitem(15, 1, " XModem protocol    ", 0, 21, 0, NULL, 0, 0);
        wmenuitem(15, 30, " 1K-XModem protocol ", 0, 22, 0, NULL, 0, 0);
        wmenuitem(16, 1, " SEAlink protocol   ", 0, 23, 0, NULL, 0, 0);
        wmenuitem(16, 30, " Quote string       ", 0, 24, 0, NULL, 0, 0);
        wmenuitem(17, 1, " Min. upload space  ", 0, 26, 0, NULL, 0, 0);
        wmenuitem(17, 30, " Input date format  ", 0, 27, 0, NULL, 0, 0);
        wmenuitem(18, 1, " CD-ROM Downl. swap ", 0, 29, 0, NULL, 0, 0);

        wmenuend(i, M_OMNI | M_SAVE, 0, 0, LGREY | _BLACK, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY);

        wprints(1, 20, CYAN | _BLACK, config.user_file);
        wprints(2, 20, CYAN | _BLACK, config.menu_path);
        wprints(3, 20, CYAN | _BLACK, config.glob_text_path);
        wprints(4, 20, CYAN | _BLACK, config.external_editor);
        wprints(5, 20, CYAN | _BLACK, config.quote_header);
        wprints(6, 20, CYAN | _BLACK, config.upload_check);
        if (config.ansilogon == 0) {
            wprints(8, 22, CYAN | _BLACK, "No");
        }
        else if (config.ansilogon == 1) {
            wprints(8, 22, CYAN | _BLACK, "Auto");
        }
        else if (config.ansilogon == 2) {
            wprints(8, 22, CYAN | _BLACK, "Ask");
        }
        else if (config.ansilogon == 3) {
            wprints(8, 22, CYAN | _BLACK, "Yes");
        }
        wprints(10, 22, CYAN | _BLACK, config.snooping ? "Yes" : "No");
        sprintf(string, "%d", config.aftercaller_exit);
        wprints(8, 51, CYAN | _BLACK, string);
        wprints(9, 22, CYAN | _BLACK, config.areachange_key);
        wprints(9, 51, CYAN | _BLACK, config.dateformat);
        wprints(10, 51, CYAN | _BLACK, config.timeformat);
        wprints(11, 22, CYAN | _BLACK, config.keep_dl_count ? "Yes" : "No");
        wprints(11, 51, CYAN | _BLACK, config.put_uploader ? "Yes" : "No");
        wprints(12, 22, CYAN | _BLACK, config.dl_counter_limits);
        wprints(12, 51, CYAN | _BLACK, config.check_city ? "Yes" : "No");
        sprintf(string, "%d", config.inactivity_timeout);
        wprints(13, 22, CYAN | _BLACK, string);
        wprints(13, 51, CYAN | _BLACK, config.use_iemsi ? "Yes" : "No");
        wprints(14, 22, CYAN | _BLACK, config.show_missing ? "Yes" : "No");
        wprints(14, 51, CYAN | _BLACK, config.prot_zmodem ? "Yes" : "No");
        wprints(15, 22, CYAN | _BLACK, config.prot_xmodem ? "Yes" : "No");
        wprints(15, 51, CYAN | _BLACK, config.prot_1kxmodem ? "Yes" : "No");
        wprints(16, 22, CYAN | _BLACK, config.prot_sealink ? "Yes" : "No");
        wprints(16, 51, CYAN | _BLACK, config.quote_string);
        sprintf(string, "%d", config.ul_free_space);
        wprints(17, 22, CYAN | _BLACK, string);
        if (config.inp_dateformat == 0) {
            wprints(17, 51, CYAN | _BLACK, "DD-MM-YY");
        }
        else if (config.inp_dateformat == 1) {
            wprints(17, 51, CYAN | _BLACK, "MM-DD-YY");
        }
        else if (config.inp_dateformat == 2) {
            wprints(17, 51, CYAN | _BLACK, "YY-MM-DD");
        }
        wprints(18, 22, CYAN | _BLACK, config.cdrom_swap ? "Yes" : "No");


        start_update();
        i = wmenuget();

        switch (i) {
            case 1:
                strcpy(string, config.user_file);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(1, 20, string, "??????????????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.user_file, strbtrim(string));
                }
                break;

            case 2:
                strcpy(string, config.menu_path);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(2, 20, string, "??????????????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.menu_path, strbtrim(string));
                    if (config.menu_path[0] && config.menu_path[strlen(config.menu_path) - 1] != '\\') {
                        strcat(config.menu_path, "\\");
                    }
                    create_path(config.menu_path);
                }
                break;

            case 3:
                strcpy(string, config.glob_text_path);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(3, 20, string, "??????????????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.glob_text_path, strbtrim(string));
                    if (config.glob_text_path[0] && config.glob_text_path[strlen(config.glob_text_path) - 1] != '\\') {
                        strcat(config.glob_text_path, "\\");
                    }
                    create_path(config.glob_text_path);
                }
                break;

            case 4:
                strcpy(string, config.external_editor);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(4, 20, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.external_editor, strbtrim(string));
                }
                break;

            case 25:
                strcpy(string, config.quote_header);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(5, 20, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.quote_header, strtrim(string));
                }
                break;

            case 28:
                strcpy(string, config.upload_check);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(6, 20, string, "?????????????????????????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.upload_check, strtrim(string));
                }
                break;

            case 5:
                if (config.ansilogon == 1) {
                    config.ansilogon = 3;
                }
                else if (config.ansilogon == 3) {
                    config.ansilogon = 0;
                }
                else if (config.ansilogon == 0) {
                    config.ansilogon = 2;
                }
                else if (config.ansilogon == 2) {
                    config.ansilogon = 1;
                }
                break;

            case 8:
                config.snooping ^= 1;
                break;

            case 9:
                sprintf(string, "%d", config.aftercaller_exit);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(8, 51, string, "???", 0, 1, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    config.aftercaller_exit = atoi(string);
                }
                break;

            case 10:
                strcpy(string, config.areachange_key);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(9, 22, string, "???", 0, 1, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.areachange_key, string);
                }
                break;

            case 11:
                strcpy(string, config.dateformat);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(9, 51, string, "??????????????????", 0, 1, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.dateformat, strbtrim(string));
                }
                break;

            case 12:
                strcpy(string, config.timeformat);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(10, 51, string, "??????????????????", 0, 1, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.timeformat, strbtrim(string));
                }
                break;

            case 13:
                config.keep_dl_count ^= 1;
                break;

            case 14:
                config.put_uploader ^= 1;
                break;

            case 15:
                strcpy(string, config.dl_counter_limits);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(12, 22, string, "??", 0, 1, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(config.dl_counter_limits, strbtrim(string));
                }
                break;

            case 16:
                config.check_city ^= 1;
                break;

            case 17:
                sprintf(string, "%d", config.inactivity_timeout);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(13, 22, string, "???", 0, 1, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    config.inactivity_timeout = atoi(strbtrim(string));
                }
                break;

            case 18:
                config.use_iemsi ^= 1;
                break;

            case 19:
                config.show_missing ^= 1;
                break;

            case 20:
                config.prot_zmodem ^= 1;
                break;

            case 21:
                config.prot_xmodem ^= 1;
                break;

            case 22:
                config.prot_1kxmodem ^= 1;
                break;

            case 23:
                config.prot_sealink ^= 1;
                break;

            case 24:
                strcpy(string, config.quote_string);
                if (winputs(16, 51, string, "?????", 1, '\261', BLUE | _GREEN, BLUE | _GREEN) != W_ESCPRESS) {
                    strcpy(config.quote_string, string);
                }
                break;

            case 26:
                sprintf(string, "%d", config.ul_free_space);
                if (winputs(17, 22, string, "?????", 1, ' ', BLUE | _GREEN, BLUE | _GREEN) != W_ESCPRESS) {
                    config.ul_free_space = atoi(strbtrim(string));
                }
                break;

            case 27:
                if (++config.inp_dateformat == 3) {
                    config.inp_dateformat = 0;
                }
                break;

            case 29:
                config.cdrom_swap ^= 1;
                break;
        }

        hidecur();
    } while (i != -1);

    wclose();
}

void bbs_message()
{
    int fd, fdi, fdx, wh, wh1, i = 1, saved;
    char string[128], filename[80];
    long pos;
    struct _sys sys, bsys;
    struct _sys_idx sysidx;

    sprintf(string, "%sSYSMSG.IDX", config.sys_path);
    fd = sh_open(string, SH_DENYWR, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
    if (fd == -1) {
        return;
    }
    close(fd);

    sprintf(string, "%sSYSMSG.DAT", config.sys_path);
    fd = sh_open(string, SH_DENYWR, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
    if (fd == -1) {
        return;
    }
    memset((char *)&sys, 0, SIZEOF_MSGAREA);
    read(fd, (char *)&sys, SIZEOF_MSGAREA);

    gotoxy_(24, 1);
    clreol_();
    prints(24, 1, LGREY | _BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add New Area  C-Copy  L-List  D-Delete");
    prints(24, 1, YELLOW | _BLACK, "PgUp/PgDn");
    prints(24, 26, YELLOW | _BLACK, "E");
    prints(24, 34, YELLOW | _BLACK, "A");
    prints(24, 50, YELLOW | _BLACK, "C");
    prints(24, 58, YELLOW | _BLACK, "L");
    prints(24, 66, YELLOW | _BLACK, "D");

    wh = wopen(1, 0, 22, 77, 1, LCYAN | _BLACK, CYAN | _BLACK);
    wactiv(wh);
    wshadow(DGREY | _BLACK);
    wtitle(" Message areas ", TRIGHT, YELLOW | _BLUE);

    do {
        stop_update();
        wclear();

        wprints(1, 1, LGREY | _BLACK, " Number       ");
        wprints(1, 28, LGREY | _BLACK, " QWK Name     ");
        wprints(2, 1, LGREY | _BLACK, " Name         ");
        wprints(3, 1, LGREY | _BLACK, " Type         ");
        wprints(3, 28, LGREY | _BLACK, " Echo-Tag     ");
        wprints(4, 1, LGREY | _BLACK, " Flags        ");
        wprints(5, 1, LGREY | _BLACK, " Storage      ");
        wprints(4, 28, LGREY | _BLACK, " Group        ");
        wprints(6, 1, LGREY | _BLACK, " Path/Board   ");
        wprints(7, 1, LGREY | _BLACK, " Aka          ");
        wprints(8, 1, LGREY | _BLACK, " Origin       ");
        wprints(9, 1, LGREY | _BLACK, " Max messages ");
        wprints(10, 22, LGREY | _BLACK, "days");
        wprints(10, 1, LGREY | _BLACK, " Message age  ");
        wprints(10, 28, LGREY | _BLACK, " Age received ");
        wprints(10, 49, LGREY | _BLACK, "days");
        wprints(11, 1, LGREY | _BLACK, " Read level   ");
        wprints(12, 1, LGREY | _BLACK, " A Flag       ");
        wprints(13, 1, LGREY | _BLACK, " B Flag       ");
        wprints(14, 1, LGREY | _BLACK, " C Flag       ");
        wprints(15, 1, LGREY | _BLACK, " D Flag       ");
        wprints(11, 28, LGREY | _BLACK, " Write level  ");
        wprints(12, 28, LGREY | _BLACK, " A Flag       ");
        wprints(13, 28, LGREY | _BLACK, " B Flag       ");
        wprints(14, 28, LGREY | _BLACK, " C Flag       ");
        wprints(15, 28, LGREY | _BLACK, " D Flag       ");
        wprints(11, 55, LGREY | _BLACK, " Afx level ");
        wprints(12, 55, LGREY | _BLACK, " A Flag    ");
        wprints(13, 55, LGREY | _BLACK, " B Flag    ");
        wprints(14, 55, LGREY | _BLACK, " C Flag    ");
        wprints(15, 55, LGREY | _BLACK, " D Flag    ");
        wprints(16, 1, LGREY | _BLACK, " Forward to:");
        wprints(17, 1, LGREY | _BLACK, " 1 ");
        wprints(18, 1, LGREY | _BLACK, " 2 ");
        wprints(19, 1, LGREY | _BLACK, " 3 ");

        sprintf(string, "%d", sys.msg_num);
        wprints(1, 16, CYAN | _BLACK, string);

        wprints(1, 43, CYAN | _BLACK, sys.qwk_name);

        sys.msg_name[55] = '\0';
        wprints(2, 16, CYAN | _BLACK, sys.msg_name);

        if (sys.netmail) {
            wprints(3, 16, CYAN | _BLACK, "Netmail");
        }
        else if (sys.echomail) {
            wprints(3, 16, CYAN | _BLACK, "Echomail");
        }
        else if (sys.internet_mail) {
            wprints(3, 16, CYAN | _BLACK, "Internet");
        }
        else {
            wprints(3, 16, CYAN | _BLACK, "Local");
        }

        wprints(3, 43, CYAN | _BLACK, sys.echotag);

        strcpy(string, "......");   // P=Public, V=Private,
        // A=Allow alias, G=Group restricted,
        // T=Tiny seen-by

        if (sys.public) {
            string[0] = 'P';
        }
        if (sys.private) {
            string[1] = 'V';
        }
        if (sys.anon_ok) {
            string[2] = 'A';
        }
        if (sys.msg_restricted) {
            string[3] = 'G';
        }
        if (sys.sendonly) {
            string[4] = 'S';
        }
        if (sys.receiveonly) {
            string[5] = 'R';
        }
        wprints(4, 16, CYAN | _BLACK, string);

        if (sys.passthrough) {
            wprints(5, 16, CYAN | _BLACK, "Passthrough");
        }
        else if (sys.quick_board) {
            wprints(5, 16, CYAN | _BLACK, "QuickBBS");
        }
        else if (sys.gold_board) {
            wprints(5, 16, CYAN | _BLACK, "GoldBase");
        }
        else if (sys.pip_board) {
            wprints(5, 16, CYAN | _BLACK, "Pip-Base");
        }
        else if (sys.squish) {
            wprints(5, 16, CYAN | _BLACK, "Squish");
        }
        else {
            wprints(5, 16, CYAN | _BLACK, "Fido");
        }

        sprintf(string, "%d", sys.msg_sig);
        wprints(4, 43, CYAN | _BLACK, string);

        if (sys.quick_board) {
            sprintf(string, "%d", sys.quick_board);
        }
        else if (sys.gold_board) {
            sprintf(string, "%d", sys.gold_board);
        }
        else if (sys.pip_board) {
            sprintf(string, "%d", sys.pip_board);
        }
        else {
            strcpy(string, sys.msg_path);
        }
        wprints(6, 16, CYAN | _BLACK, string);

        sprintf(string, "%d:%d/%d.%d", config.alias[sys.use_alias].zone, config.alias[sys.use_alias].net, config.alias[sys.use_alias].node, config.alias[sys.use_alias].point);
        wprints(7, 16, CYAN | _BLACK, string);

        wprints(8, 16, CYAN | _BLACK, sys.origin);

        sprintf(string, "%d", sys.max_msgs);
        wprints(9, 16, CYAN | _BLACK, string);

        sprintf(string, "%d", sys.max_age);
        wprints(10, 16, CYAN | _BLACK, string);

        sprintf(string, "%d", sys.age_rcvd);
        wprints(10, 43, CYAN | _BLACK, string);

        wprints(11, 15, CYAN | _BLACK, get_priv_text(sys.msg_priv));
        wprints(12, 16, CYAN | _BLACK, get_flagA_text((sys.msg_flags >> 24) & 0xFF));
        wprints(13, 16, CYAN | _BLACK, get_flagB_text((sys.msg_flags >> 16) & 0xFF));
        wprints(14, 16, CYAN | _BLACK, get_flagC_text((sys.msg_flags >> 8) & 0xFF));
        wprints(15, 16, CYAN | _BLACK, get_flagD_text(sys.msg_flags & 0xFF));

        wprints(11, 42, CYAN | _BLACK, get_priv_text(sys.write_priv));
        wprints(12, 43, CYAN | _BLACK, get_flagA_text((sys.write_flags >> 24) & 0xFF));
        wprints(13, 43, CYAN | _BLACK, get_flagB_text((sys.write_flags >> 16) & 0xFF));
        wprints(14, 43, CYAN | _BLACK, get_flagC_text((sys.write_flags >> 8) & 0xFF));
        wprints(15, 43, CYAN | _BLACK, get_flagD_text(sys.write_flags & 0xFF));

        sprintf(string, "%d", sys.areafix);
        wprints(11, 67, CYAN | _BLACK, string);
        wprints(12, 67, CYAN | _BLACK, get_flagA_text((sys.afx_flags >> 24) & 0xFF));
        wprints(13, 67, CYAN | _BLACK, get_flagB_text((sys.afx_flags >> 16) & 0xFF));
        wprints(14, 67, CYAN | _BLACK, get_flagC_text((sys.afx_flags >> 8) & 0xFF));
        wprints(15, 67, CYAN | _BLACK, get_flagD_text(sys.afx_flags & 0xFF));

        wprints(17, 4, CYAN | _BLACK, sys.forward1);
        wprints(18, 4, CYAN | _BLACK, sys.forward2);
        wprints(19, 4, CYAN | _BLACK, sys.forward3);
        start_update();

        i = getxch();
        if ((i & 0xFF) != 0) {
            i &= 0xFF;
        }

        switch (i) {
            // PgDn
            case 0x5100:
                read(fd, (char *)&sys, SIZEOF_MSGAREA);
                break;

            // PgUp
            case 0x4900:
                if (tell(fd) > SIZEOF_MSGAREA) {
                    lseek(fd, -2L * SIZEOF_MSGAREA, SEEK_CUR);
                    read(fd, (char *)&sys, SIZEOF_MSGAREA);
                }
                break;

            // E Edit
            case 'E':
            case 'e':
                saved = sys.msg_num;
                strcpy(string, sys.qwk_name);

                edit_single_area(&sys);

                lseek(fd, -1L * SIZEOF_MSGAREA, SEEK_CUR);
                write(fd, (char *)&sys, SIZEOF_MSGAREA);

                if (saved != sys.msg_num || strcmp(string, sys.qwk_name)) {
                    pos = tell(fd) - (long)SIZEOF_MSGAREA;
                    close(fd);

                    update_message();

                    sprintf(string, "%sSYSMSG.IDX", config.sys_path);
                    while ((fdx = sh_open(string, SH_DENYRD, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE)) == -1)
                        ;
                    sprintf(filename, "%sSYSMSG.BAK", config.sys_path);
                    while ((fdi = sh_open(filename, SH_DENYRW, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE)) == -1)
                        ;
                    sprintf(string, "%sSYSMSG.DAT", config.sys_path);
                    while ((fd = sh_open(string, SH_DENYRD, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) == -1)
                        ;
                    saved = 0;

                    while (read(fd, (char *)&bsys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
                        if (!saved && bsys.msg_num > sys.msg_num) {
                            pos = tell(fdi);
                            write(fdi, (char *)&sys, SIZEOF_MSGAREA);
                            memset((char *)&sysidx, 0, sizeof(struct _sys_idx));
                            sysidx.priv = sys.msg_priv;
                            sysidx.flags = sys.msg_flags;
                            sysidx.area = sys.msg_num;
                            sysidx.sig = sys.msg_sig;
                            strcpy(sysidx.key, sys.qwk_name);
                            write(fdx, (char *)&sysidx, sizeof(struct _sys_idx));
                            saved = 1;
                        }
                        if (bsys.msg_num != sys.msg_num) {
                            write(fdi, (char *)&bsys, SIZEOF_MSGAREA);
                            memset((char *)&sysidx, 0, sizeof(struct _sys_idx));
                            sysidx.priv = bsys.msg_priv;
                            sysidx.flags = bsys.msg_flags;
                            sysidx.area = bsys.msg_num;
                            sysidx.sig = bsys.msg_sig;
                            strcpy(sysidx.key, bsys.qwk_name);
                            write(fdx, (char *)&sysidx, sizeof(struct _sys_idx));
                        }
                    }

                    if (!saved) {
                        pos = tell(fdi);
                        write(fdi, (char *)&sys, SIZEOF_MSGAREA);
                        memset((char *)&sysidx, 0, sizeof(struct _sys_idx));
                        sysidx.priv = sys.msg_priv;
                        sysidx.flags = sys.msg_flags;
                        sysidx.area = sys.msg_num;
                        sysidx.sig = sys.msg_sig;
                        strcpy(sysidx.key, sys.qwk_name);
                        write(fdx, (char *)&sysidx, sizeof(struct _sys_idx));
                    }

                    lseek(fd, 0L, SEEK_SET);
                    chsize(fd, 0L);
                    lseek(fdi, 0L, SEEK_SET);

                    while (read(fdi, (char *)&bsys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
                        write(fd, (char *)&bsys, SIZEOF_MSGAREA);
                    }

                    close(fdx);
                    close(fd);
                    close(fdi);

                    unlink(filename);

                    sprintf(string, "%sSYSMSG.DAT", config.sys_path);
                    while ((fd = sh_open(string, SH_DENYNONE, O_RDWR | O_BINARY, S_IREAD | S_IWRITE)) == -1)
                        ;
                    lseek(fd, pos, SEEK_SET);
                    read(fd, (char *)&sys, SIZEOF_MSGAREA);

                    wclose();
                }
                else {
                    pos = tell(fd) - (long)SIZEOF_MSGAREA;
                    i = (int)(pos / (long)SIZEOF_MSGAREA);

                    sprintf(string, "%sSYSMSG.IDX", config.sys_path);
                    while ((fdx = sh_open(string, SH_DENYNONE, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) == -1)
                        ;
                    lseek(fdx, (long)i * sizeof(struct _sys_idx), SEEK_SET);
                    memset((char *)&sysidx, 0, sizeof(struct _sys_idx));
                    sysidx.priv = sys.msg_priv;
                    sysidx.flags = sys.msg_flags;
                    sysidx.area = sys.msg_num;
                    sysidx.sig = sys.msg_sig;
                    strcpy(sysidx.key, sys.qwk_name);
                    write(fdx, (char *)&sysidx, sizeof(struct _sys_idx));
                    close(fdx);
                }

                break;

            // A Add
            case 'A':
            case 'a':
                memcpy((char *)&bsys, (char *)&sys, sizeof(struct _sys));
                memset((char *)&sys, 0, sizeof(struct _sys));
                sys.max_age = 14;
                sys.max_msgs = 200;
                sys.msg_priv = sys.write_priv = TWIT;

            // C Copy
            case 'C':
            case 'c':
                if (i == 'C' || i == 'c') {
                    memcpy((char *)&bsys, (char *)&sys, sizeof(struct _sys));
                }

                sys.msg_num = 0;
                edit_single_area(&sys);

                if (sys.msg_num && strcmp(sys.msg_name, "")) {
                    pos = tell(fd) - (long)SIZEOF_MSGAREA;
                    close(fd);

                    update_message();

                    sprintf(string, "%sSYSMSG.IDX", config.sys_path);
                    while ((fdx = sh_open(string, SH_DENYRD, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE)) == -1)
                        ;
                    sprintf(filename, "%sSYSMSG.BAK", config.sys_path);
                    while ((fdi = sh_open(filename, SH_DENYRW, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE)) == -1)
                        ;
                    sprintf(string, "%sSYSMSG.DAT", config.sys_path);
                    while ((fd = sh_open(string, SH_DENYRD, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) == -1)
                        ;
                    saved = 0;

                    while (read(fd, (char *)&bsys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
                        if (!saved && bsys.msg_num > sys.msg_num) {
                            pos = tell(fdi);
                            write(fdi, (char *)&sys, SIZEOF_MSGAREA);
                            memset((char *)&sysidx, 0, sizeof(struct _sys_idx));
                            sysidx.priv = sys.msg_priv;
                            sysidx.flags = sys.msg_flags;
                            sysidx.area = sys.msg_num;
                            sysidx.sig = sys.msg_sig;
                            strcpy(sysidx.key, sys.qwk_name);
                            write(fdx, (char *)&sysidx, sizeof(struct _sys_idx));
                            saved = 1;
                        }
                        if (bsys.msg_num != sys.msg_num) {
                            write(fdi, (char *)&bsys, SIZEOF_MSGAREA);
                            memset((char *)&sysidx, 0, sizeof(struct _sys_idx));
                            sysidx.priv = bsys.msg_priv;
                            sysidx.flags = bsys.msg_flags;
                            sysidx.area = bsys.msg_num;
                            sysidx.sig = bsys.msg_sig;
                            strcpy(sysidx.key, bsys.qwk_name);
                            write(fdx, (char *)&sysidx, sizeof(struct _sys_idx));
                        }
                    }

                    if (!saved) {
                        pos = tell(fdi);
                        write(fdi, (char *)&sys, SIZEOF_MSGAREA);
                        memset((char *)&sysidx, 0, sizeof(struct _sys_idx));
                        sysidx.priv = sys.msg_priv;
                        sysidx.flags = sys.msg_flags;
                        sysidx.area = sys.msg_num;
                        sysidx.sig = sys.msg_sig;
                        strcpy(sysidx.key, sys.qwk_name);
                        write(fdx, (char *)&sysidx, sizeof(struct _sys_idx));
                    }

                    lseek(fd, 0L, SEEK_SET);
                    chsize(fd, 0L);
                    lseek(fdi, 0L, SEEK_SET);

                    while (read(fdi, (char *)&bsys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
                        write(fd, (char *)&bsys, SIZEOF_MSGAREA);
                    }

                    close(fdx);
                    close(fd);
                    close(fdi);

                    unlink(filename);

                    sprintf(string, "%sSYSMSG.DAT", config.sys_path);
                    while ((fd = sh_open(string, SH_DENYNONE, O_RDWR | O_BINARY, S_IREAD | S_IWRITE)) == -1)
                        ;
                    lseek(fd, pos, SEEK_SET);
                    read(fd, (char *)&sys, SIZEOF_MSGAREA);

                    wclose();
                }
                else {
                    memcpy((char *)&sys, (char *)&bsys, sizeof(struct _sys));
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
                    pos = tell(fd) - (long)SIZEOF_MSGAREA;
                    close(fd);

                    update_message();

                    sprintf(string, "%sSYSMSG.IDX", config.sys_path);
                    while ((fdx = sh_open(string, SH_DENYRD, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE)) == -1)
                        ;
                    sprintf(filename, "%sSYSMSG.BAK", config.sys_path);
                    while ((fdi = sh_open(filename, SH_DENYRW, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE)) == -1)
                        ;
                    sprintf(string, "%sSYSMSG.DAT", config.sys_path);
                    while ((fd = sh_open(string, SH_DENYRD, O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) == -1)
                        ;

                    while (read(fd, (char *)&bsys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
                        if (bsys.msg_num != sys.msg_num) {
                            write(fdi, (char *)&bsys, SIZEOF_MSGAREA);
                            memset((char *)&sysidx, 0, sizeof(struct _sys_idx));
                            sysidx.priv = bsys.msg_priv;
                            sysidx.flags = bsys.msg_flags;
                            sysidx.area = bsys.msg_num;
                            sysidx.sig = bsys.msg_sig;
                            strcpy(sysidx.key, bsys.qwk_name);
                            write(fdx, (char *)&sysidx, sizeof(struct _sys_idx));
                        }
                    }

                    lseek(fd, 0L, SEEK_SET);
                    chsize(fd, 0L);
                    lseek(fdi, 0L, SEEK_SET);

                    while (read(fdi, (char *)&bsys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
                        write(fd, (char *)&bsys, SIZEOF_MSGAREA);
                    }

                    close(fdx);
                    close(fd);
                    close(fdi);

                    unlink(filename);

                    sprintf(string, "%sSYSMSG.DAT", config.sys_path);
                    while ((fd = sh_open(string, SH_DENYNONE, O_RDWR | O_BINARY, S_IREAD | S_IWRITE)) == -1)
                        ;
                    if (lseek(fd, pos, SEEK_SET) == -1 || pos == filelength(fd)) {
                        lseek(fd, -1l * SIZEOF_MSGAREA, SEEK_END);
                        pos = tell(fd);
                    }
                    read(fd, (char *)&sys, SIZEOF_MSGAREA);

                    wclose();
                }
                break;

            // L List
            case 'L':
            case 'l':
                select_area_list(fd, &sys);
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

static void edit_single_area(sys)
struct _sys * sys;
{
    int i = 1, wh1, m, mx;
    char string[128], *akas[MAX_ALIAS], temp[42];
    struct _sys nsys;

    memcpy((char *)&nsys, (char *)sys, SIZEOF_MSGAREA);

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
        wmenuitem(1, 1, " Number       ", 0, 1, 0, NULL, 0, 0);
        wmenuitem(1, 28, " QWK Name     ", 0, 25, 0, NULL, 0, 0);
        wmenuitem(2, 1, " Name         ", 0, 2, 0, NULL, 0, 0);
        wmenuitem(3, 1, " Type         ", 0, 3, 0, NULL, 0, 0);
        wmenuitem(3, 28, " Echo-Tag     ", 0, 24, 0, NULL, 0, 0);
        wmenuitem(4, 1, " Flags        ", 0, 4, 0, NULL, 0, 0);
        wmenuitem(5, 1, " Storage      ", 0, 6, 0, NULL, 0, 0);
        wmenuitem(4, 28, " Group        ", 0, 5, 0, NULL, 0, 0);
        wmenuitem(6, 1, " Path/Board   ", 0, 7, 0, NULL, 0, 0);
        wmenuitem(7, 1, " Aka          ", 0, 8, 0, NULL, 0, 0);
        wmenuitem(8, 1, " Origin       ", 0, 9, 0, NULL, 0, 0);
        wmenuitem(9, 1, " Max messages ", 0, 10, 0, NULL, 0, 0);
        wmenuitem(10, 1, " Message age  ", 0, 11, 0, NULL, 0, 0);
        wprints(10, 22, LGREY | _BLACK, "days");
        wmenuitem(10, 28, " Age received ", 0, 12, 0, NULL, 0, 0);
        wprints(10, 49, LGREY | _BLACK, "days");
        wmenuitem(11, 1, " Read level   ", 0, 13, 0, NULL, 0, 0);
        wmenuitem(12, 1, " A Flag       ", 0, 14, 0, NULL, 0, 0);
        wmenuitem(13, 1, " B Flag       ", 0, 15, 0, NULL, 0, 0);
        wmenuitem(14, 1, " C Flag       ", 0, 16, 0, NULL, 0, 0);
        wmenuitem(15, 1, " D Flag       ", 0, 17, 0, NULL, 0, 0);
        wmenuitem(11, 28, " Write level  ", 0, 18, 0, NULL, 0, 0);
        wmenuitem(12, 28, " A Flag       ", 0, 19, 0, NULL, 0, 0);
        wmenuitem(13, 28, " B Flag       ", 0, 20, 0, NULL, 0, 0);
        wmenuitem(14, 28, " C Flag       ", 0, 21, 0, NULL, 0, 0);
        wmenuitem(15, 28, " D Flag       ", 0, 22, 0, NULL, 0, 0);
        wmenuitem(11, 55, " Afx level ", 0, 23, 0, NULL, 0, 0);
        wmenuitem(12, 55, " A Flag    ", 0, 29, 0, NULL, 0, 0);
        wmenuitem(13, 55, " B Flag    ", 0, 30, 0, NULL, 0, 0);
        wmenuitem(14, 55, " C Flag    ", 0, 31, 0, NULL, 0, 0);
        wmenuitem(15, 55, " D Flag    ", 0, 32, 0, NULL, 0, 0);
        wprints(16, 1, LGREY | _BLACK, " Forward to:");
        wmenuitem(17, 1, " 1 ", 0, 26, 0, NULL, 0, 0);
        wmenuitem(18, 1, " 2 ", 0, 27, 0, NULL, 0, 0);
        wmenuitem(19, 1, " 3 ", 0, 28, 0, NULL, 0, 0);
        wmenuend(i, M_OMNI | M_SAVE, 0, 0, LGREY | _BLACK, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY);

        sprintf(string, "%d", nsys.msg_num);
        wprints(1, 16, CYAN | _BLACK, string);

        wprints(1, 43, CYAN | _BLACK, nsys.qwk_name);

        nsys.msg_name[55] = '\0';
        wprints(2, 16, CYAN | _BLACK, nsys.msg_name);

        if (nsys.netmail) {
            wprints(3, 16, CYAN | _BLACK, "Netmail");
        }
        else if (nsys.echomail) {
            wprints(3, 16, CYAN | _BLACK, "Echomail");
        }
        else if (nsys.internet_mail) {
            wprints(3, 16, CYAN | _BLACK, "Internet");
        }
        else {
            wprints(3, 16, CYAN | _BLACK, "Local");
        }

        wprints(3, 43, CYAN | _BLACK, nsys.echotag);

        strcpy(string, "......");   // P=Public, V=Private,
        // A=Allow alias, G=Group restricted,
        // T=Tiny seen-by

        if (nsys.public) {
            string[0] = 'P';
        }
        if (nsys.private) {
            string[1] = 'V';
        }
        if (nsys.anon_ok) {
            string[2] = 'A';
        }
        if (nsys.msg_restricted) {
            string[3] = 'G';
        }
        if (nsys.sendonly) {
            string[4] = 'S';
        }
        if (nsys.receiveonly) {
            string[5] = 'R';
        }

        wprints(4, 16, CYAN | _BLACK, string);

        if (nsys.passthrough) {
            wprints(5, 16, CYAN | _BLACK, "Passthrough");
        }
        else if (nsys.quick_board) {
            wprints(5, 16, CYAN | _BLACK, "QuickBBS");
        }
        else if (nsys.gold_board) {
            wprints(5, 16, CYAN | _BLACK, "GoldBase");
        }
        else if (nsys.pip_board) {
            wprints(5, 16, CYAN | _BLACK, "Pip-Base");
        }
        else if (nsys.squish) {
            wprints(5, 16, CYAN | _BLACK, "Squish");
        }
        else {
            wprints(5, 16, CYAN | _BLACK, "Fido");
        }

        sprintf(string, "%d", nsys.msg_sig);
        wprints(4, 43, CYAN | _BLACK, string);

        if (nsys.quick_board) {
            sprintf(string, "%d", nsys.quick_board);
        }
        else if (nsys.gold_board) {
            sprintf(string, "%d", nsys.gold_board);
        }
        else if (nsys.pip_board) {
            sprintf(string, "%d", nsys.pip_board);
        }
        else {
            strcpy(string, nsys.msg_path);
        }
        wprints(6, 16, CYAN | _BLACK, string);

        sprintf(string, "%d:%d/%d.%d", config.alias[nsys.use_alias].zone, config.alias[nsys.use_alias].net, config.alias[nsys.use_alias].node, config.alias[nsys.use_alias].point);
        wprints(7, 16, CYAN | _BLACK, string);

        wprints(8, 16, CYAN | _BLACK, nsys.origin);

        sprintf(string, "%d", nsys.max_msgs);
        wprints(9, 16, CYAN | _BLACK, string);

        sprintf(string, "%d", nsys.max_age);
        wprints(10, 16, CYAN | _BLACK, string);

        sprintf(string, "%d", nsys.age_rcvd);
        wprints(10, 43, CYAN | _BLACK, string);

        wprints(11, 15, CYAN | _BLACK, get_priv_text(nsys.msg_priv));
        wprints(12, 16, CYAN | _BLACK, get_flagA_text((nsys.msg_flags >> 24) & 0xFF));
        wprints(13, 16, CYAN | _BLACK, get_flagB_text((nsys.msg_flags >> 16) & 0xFF));
        wprints(14, 16, CYAN | _BLACK, get_flagC_text((nsys.msg_flags >> 8) & 0xFF));
        wprints(15, 16, CYAN | _BLACK, get_flagD_text(nsys.msg_flags & 0xFF));

        wprints(11, 42, CYAN | _BLACK, get_priv_text(nsys.write_priv));
        wprints(12, 43, CYAN | _BLACK, get_flagA_text((nsys.write_flags >> 24) & 0xFF));
        wprints(13, 43, CYAN | _BLACK, get_flagB_text((nsys.write_flags >> 16) & 0xFF));
        wprints(14, 43, CYAN | _BLACK, get_flagC_text((nsys.write_flags >> 8) & 0xFF));
        wprints(15, 43, CYAN | _BLACK, get_flagD_text(nsys.write_flags & 0xFF));

        sprintf(string, "%d", nsys.areafix);
        wprints(11, 67, CYAN | _BLACK, string);
        wprints(12, 67, CYAN | _BLACK, get_flagA_text((nsys.afx_flags >> 24) & 0xFF));
        wprints(13, 67, CYAN | _BLACK, get_flagB_text((nsys.afx_flags >> 16) & 0xFF));
        wprints(14, 67, CYAN | _BLACK, get_flagC_text((nsys.afx_flags >> 8) & 0xFF));
        wprints(15, 67, CYAN | _BLACK, get_flagD_text(nsys.afx_flags & 0xFF));

        wprints(17, 4, CYAN | _BLACK, nsys.forward1);
        wprints(18, 4, CYAN | _BLACK, nsys.forward2);
        wprints(19, 4, CYAN | _BLACK, nsys.forward3);
        start_update();

        i = wmenuget();

        switch (i) {
            case 1:
                sprintf(string, "%d", nsys.msg_num);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(1, 16, string, "?????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    nsys.msg_num = atoi(strbtrim(string));
                }
                break;

            case 2:
                strcpy(string, nsys.msg_name);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(2, 16, string, "????????????????????????????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(nsys.msg_name, strbtrim(string));
                }
                break;

            case 3:
                if (nsys.netmail) {
                    nsys.netmail = 0;
                    nsys.echomail = 1;
                }
                else if (nsys.echomail) {
                    nsys.echomail = 0;
                    nsys.internet_mail = 1;
                }
                else if (nsys.internet_mail) {
                    nsys.internet_mail = 0;
                }
                else {
                    nsys.internet_mail = 0;
                    nsys.netmail = 1;
                }
                break;

            case 4:
                wh1 = wopen(8, 28, 17, 53, 1, LCYAN | _BLACK, CYAN | _BLACK);
                wactiv(wh1);
                wshadow(DGREY | _BLACK);
                wtitle(" Flags ", TRIGHT, YELLOW | _BLUE);
                m = 1;

                do {
                    wmenubegc();
                    wmenuitem(1, 1, " Public only      ", 0, 1, 0, NULL, 0, 0);
                    wmenuitem(2, 1, " Private only     ", 0, 2, 0, NULL, 0, 0);
                    wmenuitem(3, 1, " Allow alias      ", 0, 3, 0, NULL, 0, 0);
                    wmenuitem(4, 1, " Group restricted ", 0, 4, 0, NULL, 0, 0);
//               wmenuitem (4, 1, "Tiny SEEN-BY    ", 0, 5, 0, NULL, 0, 0);
                    wmenuitem(5, 1, " Send only        ", 0, 5, 0, NULL, 0, 0);
                    wmenuitem(6, 1, " Receive only     ", 0, 6, 0, NULL, 0, 0);
                    wmenuend(m, M_OMNI | M_SAVE, 0, 0, LGREY | _BLACK, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY);

                    if (nsys.public) {
                        wprints(1, 20, CYAN | _BLACK, "Yes");
                    }
                    else {
                        wprints(1, 20, CYAN | _BLACK, "No ");
                    }
                    if (nsys.private) {
                        wprints(2, 20, CYAN | _BLACK, "Yes");
                    }
                    else {
                        wprints(2, 20, CYAN | _BLACK, "No ");
                    }
                    if (nsys.anon_ok) {
                        wprints(3, 20, CYAN | _BLACK, "Yes");
                    }
                    else {
                        wprints(3, 20, CYAN | _BLACK, "No ");
                    }
                    if (nsys.msg_restricted) {
                        wprints(4, 20, CYAN | _BLACK, "Yes");
                    }
                    else {
                        wprints(4, 20, CYAN | _BLACK, "No ");
                    }
                    if (nsys.sendonly) {
                        wprints(5, 20, CYAN | _BLACK, "Yes");
                    }
                    else {
                        wprints(5, 20, CYAN | _BLACK, "No ");
                    }
                    if (nsys.receiveonly) {
                        wprints(6, 20, CYAN | _BLACK, "Yes");
                    }
                    else {
                        wprints(6, 20, CYAN | _BLACK, "No ");
                    }

                    m = wmenuget();

                    switch (m) {
                        case 1:
                            nsys.public = nsys.public ? 0 : 1;
                            if (nsys.public) {
                                nsys.private = 0;
                            }
                            break;
                        case 2:
                            nsys.private = nsys.private ? 0 : 1;
                            if (nsys.private) {
                                nsys.public = 0;
                            }
                            break;
                        case 3:
                            nsys.anon_ok = nsys.anon_ok ? 0 : 1;
                            break;
                        case 4:
                            nsys.msg_restricted = nsys.msg_restricted ? 0 : 1;
                            break;
                        case 5:
                            nsys.sendonly = nsys.sendonly ? 0 : 1;
                            break;
                        case 6:
                            nsys.receiveonly = nsys.receiveonly ? 0 : 1;
                            break;
                    }

                } while (m != -1);

                wclose();
                break;

            case 5:
                sprintf(string, "%d", nsys.msg_sig);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(4, 43, string, "???", 0, 1, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    nsys.msg_sig = atoi(strbtrim(string));
                }
                break;

            case 6:
                if (nsys.passthrough) {
                    if ((nsys.quick_board = sys->quick_board) == 0) {
                        nsys.quick_board = 199;
                    }
                    nsys.passthrough = 0;
                }
                else if (nsys.quick_board) {
                    if ((nsys.gold_board = sys->gold_board) == 0) {
                        nsys.gold_board = 499;
                    }
                    nsys.quick_board = 0;
                }
                else if (nsys.gold_board) {
                    if ((nsys.pip_board = sys->pip_board) == 0) {
                        nsys.pip_board = 1;
                    }
                    nsys.gold_board = 0;
                }
                else if (nsys.pip_board) {
                    nsys.pip_board = 0;
                    nsys.squish = 1;
                }
                else if (nsys.squish) {
                    nsys.squish = 0;
                }
                else {
                    nsys.passthrough = 1;
                }
                break;

            case 7:
                if (nsys.quick_board) {
                    do {
                        sprintf(string, "%d", nsys.quick_board);
                        winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                        winpdef(6, 16, string, "???", 0, 2, NULL, 0);
                        if (winpread() != W_ESCPRESS) {
                            nsys.quick_board = atoi(strbtrim(string));
                        }
                        else {
                            break;
                        }
                    } while (nsys.quick_board < 1 || nsys.quick_board > 200);
                }
                else if (nsys.gold_board) {
                    do {
                        sprintf(string, "%d", nsys.gold_board);
                        winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                        winpdef(6, 16, string, "???", 0, 2, NULL, 0);
                        if (winpread() != W_ESCPRESS) {
                            nsys.gold_board = atoi(strbtrim(string));
                        }
                        else {
                            break;
                        }
                    } while (nsys.gold_board < 1 || nsys.gold_board > 500);
                }
                else if (nsys.pip_board) {
                    sprintf(string, "%d", nsys.pip_board);
                    winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                    winpdef(6, 16, string, "????", 0, 2, NULL, 0);
                    if (winpread() != W_ESCPRESS) {
                        nsys.pip_board = atoi(strbtrim(string));
                    }
                }
                else if (nsys.squish) {
                    strcpy(string, nsys.msg_path);
                    winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                    winpdef(6, 16, string, "???????????????????????????????????????", 0, 2, NULL, 0);
                    if (winpread() != W_ESCPRESS) {
                        strcpy(nsys.msg_path, strbtrim(string));
                        if (nsys.msg_path[strlen(nsys.msg_path) - 1] == '\\') {
                            nsys.msg_path[strlen(nsys.msg_path) - 1] = '\0';
                        }
                    }
                }
                else {
                    strcpy(string, nsys.msg_path);
                    winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                    winpdef(6, 16, string, "??????????????????????????????????????", 0, 2, NULL, 0);
                    if (winpread() != W_ESCPRESS) {
                        strcpy(nsys.msg_path, strbtrim(string));
                        if (nsys.msg_path[strlen(nsys.msg_path) - 1] != '\\') {
                            strcat(nsys.msg_path, "\\");
                        }
                        create_path(nsys.msg_path);
                    }
                }
                break;

            case 8:
                mx = 0;
                for (m = 0; m < MAX_ALIAS && config.alias[m].net; m++) {
                    sprintf(string, "%d:%d/%d.%d", config.alias[m].zone, config.alias[m].net, config.alias[m].node, config.alias[m].point);
                    akas[m] = (char *)malloc(strlen(string) + 1);
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
                m = wpickstr(12, 27, 18, 46, 5, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY, akas, nsys.use_alias, NULL);
                /* MM */            if (m != -1) {
                    nsys.use_alias = m;
                }
                wclose();
                for (m = 0; m < MAX_ALIAS && config.alias[m].net; m++) {
                    free(akas[m]);
                }
                break;

            case 9:
                strcpy(string, nsys.origin);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(8, 16, string, "????????????????????????????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(nsys.origin, strbtrim(string));
                }
                break;

            case 10:
                sprintf(string, "%d", nsys.max_msgs);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(9, 16, string, "?????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    nsys.max_msgs = atoi(strbtrim(string));
                }
                break;

            case 11:
                sprintf(string, "%d", nsys.max_age);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(10, 16, string, "?????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    nsys.max_age = atoi(strbtrim(string));
                }
                break;

            case 12:
                sprintf(string, "%d", nsys.age_rcvd);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(10, 43, string, "?????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    nsys.age_rcvd = atoi(strbtrim(string));
                }
                break;

            case 13:
                nsys.msg_priv = select_level(nsys.msg_priv, 20, 8);
                break;

            case 14:
                nsys.msg_flags = window_get_flags(8, 24, 1, nsys.msg_flags);
                break;

            case 15:
                nsys.msg_flags = window_get_flags(8, 24, 2, nsys.msg_flags);
                break;

            case 16:
                nsys.msg_flags = window_get_flags(8, 24, 3, nsys.msg_flags);
                break;

            case 17:
                nsys.msg_flags = window_get_flags(8, 24, 4, nsys.msg_flags);
                break;

            case 18:
                nsys.write_priv = select_level(nsys.write_priv, 47, 8);
                break;

            case 19:
                nsys.write_flags = window_get_flags(8, 46, 1, nsys.write_flags);
                break;

            case 20:
                nsys.write_flags = window_get_flags(8, 46, 2, nsys.write_flags);
                break;

            case 21:
                nsys.write_flags = window_get_flags(8, 46, 3, nsys.write_flags);
                break;

            case 22:
                nsys.write_flags = window_get_flags(8, 46, 4, nsys.write_flags);
                break;

            case 23:
                sprintf(string, "%d", nsys.areafix);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(11, 67, string, "???", 0, 1, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    nsys.areafix = atoi(strbtrim(string));
                }
                break;

            case 24:
                strcpy(string, nsys.echotag);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(3, 43, string, "???????????????????????????????", 0, 1, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(nsys.echotag, strupr(strbtrim(string)));
                }
                break;

            case 25:
                strcpy(string, nsys.qwk_name);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(1, 43, string, "????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(nsys.qwk_name, strbtrim(string));
                }
                break;

            case 26:
                strcpy(string, nsys.forward1);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(17, 4, string, "??????????????????????????????????????????????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(nsys.forward1, strbtrim(string));
                }
                break;

            case 27:
                strcpy(string, nsys.forward2);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(18, 4, string, "??????????????????????????????????????????????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(nsys.forward2, strbtrim(string));
                }
                break;

            case 28:
                strcpy(string, nsys.forward3);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(19, 4, string, "??????????????????????????????????????????????????????????????????????", 0, 2, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    strcpy(nsys.forward3, strbtrim(string));
                }
                break;

            case 29:
                nsys.afx_flags = window_get_flags(8, 66, 1, nsys.afx_flags);
                break;

            case 30:
                nsys.afx_flags = window_get_flags(8, 66, 2, nsys.afx_flags);
                break;

            case 31:
                nsys.afx_flags = window_get_flags(8, 66, 3, nsys.afx_flags);
                break;

            case 32:
                nsys.afx_flags = window_get_flags(8, 66, 4, nsys.afx_flags);
                break;
        }

        hidecur();
    } while (i != -1);

    if (memcmp((char *)&nsys, (char *)sys, SIZEOF_MSGAREA)) {
        wh1 = wopen(10, 25, 14, 54, 0, BLACK | _LGREY, BLACK | _LGREY);
        wactiv(wh1);
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
            memcpy((char *)sys, (char *)&nsys, SIZEOF_MSGAREA);
        }
    }

    gotoxy_(24, 1);
    clreol_();
    prints(24, 1, LGREY | _BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add New Area  C-Copy  L-List  D-Delete");
    prints(24, 1, YELLOW | _BLACK, "PgUp/PgDn");
    prints(24, 26, YELLOW | _BLACK, "E");
    prints(24, 34, YELLOW | _BLACK, "A");
    prints(24, 50, YELLOW | _BLACK, "C");
    prints(24, 58, YELLOW | _BLACK, "L");
    prints(24, 66, YELLOW | _BLACK, "D");
}

static void select_area_list(fd, osys)
int fd;
struct _sys * osys;
{
    int wh, i, x, start;
    char string[80], **array;
    struct _sys sys;
    long pos;

    wh = wopen(4, 0, 20, 77, 1, LCYAN | _BLACK, CYAN | _BLACK);
    wactiv(wh);
    wshadow(DGREY | _BLACK);
    wtitle(" Select area ", TRIGHT, YELLOW | _BLUE);

    wprints(0, 1, LGREY | _BLACK, "Area# Name                                                   Level");
    whline(1, 0, 76, 0, BLUE | _BLACK);

    gotoxy_(24, 1);
    clreol_();
    prints(24, 1, LGREY | _BLACK, "\030\031-Move bar  ENTER-Select");
    prints(24, 1, YELLOW | _BLACK, "\030\031");
    prints(24, 14, YELLOW | _BLACK, "ENTER");

    i = (int)(filelength(fd) / SIZEOF_MSGAREA);
    i += 2;
    if ((array = (char **)malloc(i * sizeof(char *))) == NULL) {
        return;
    }

    pos = tell(fd) - SIZEOF_MSGAREA;
    lseek(fd, 0L, SEEK_SET);
    i = 0;
    start = 0;

    while (read(fd, (char *)&sys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
        sprintf(string, " %-4d %-33.33s %-20.20s%-13s ", sys.msg_num, sys.msg_name, sys.echotag, get_priv_text(sys.msg_priv));
        array[i] = (char *)malloc(strlen(string) + 1);
        if (array[i] == NULL) {
            break;
        }
        if (osys->msg_num == sys.msg_num) {
            start = i;
        }
        strcpy(array[i++], string);
    }
    array[i] = NULL;

    x = wpickstr(7, 2, 19, 75, 5, LGREY | _BLACK, CYAN | _BLACK, BLUE | _LGREY, array, start, NULL);

    if (x == -1) {
        lseek(fd, pos, SEEK_SET);
    }
    else {
        lseek(fd, (long)x * SIZEOF_MSGAREA, SEEK_SET);
    }
    read(fd, (char *)osys, SIZEOF_MSGAREA);

    wclose();

    i = 0;
    while (array[i] != NULL) {
        free(array[i++]);
    }
    free(array);

    gotoxy_(24, 1);
    clreol_();
    prints(24, 1, LGREY | _BLACK, "PgUp/PgDn-Next/Previous  E-Edit  A-Add New Area  C-Copy  L-List  D-Delete");
    prints(24, 1, YELLOW | _BLACK, "PgUp/PgDn");
    prints(24, 26, YELLOW | _BLACK, "E");
    prints(24, 34, YELLOW | _BLACK, "A");
    prints(24, 50, YELLOW | _BLACK, "C");
    prints(24, 58, YELLOW | _BLACK, "L");
    prints(24, 66, YELLOW | _BLACK, "D");
}

void bbs_login_limits()
{
    int wh, i = 1;
    char string[128];

    wh = wopen(7, 15, 17, 48, 1, LCYAN | _BLACK, CYAN | _BLACK);
    wactiv(wh);
    wshadow(DGREY | _BLACK);
    wtitle(" Login limits ", TRIGHT, YELLOW | _BLUE);

    do {
        stop_update();
        wclear();

        wmenubegc();
        wmenuitem(1,  1, " Minimum sec. level ", 0, 1, 0, NULL, 0, 0);
        wmenuitem(2,  1, " Minimum A flags    ", 0, 2, 0, NULL, 0, 0);
        wmenuitem(3,  1, " Minimum B flags    ", 0, 3, 0, NULL, 0, 0);
        wmenuitem(4,  1, " Minimum C flags    ", 0, 4, 0, NULL, 0, 0);
        wmenuitem(5,  1, " Minimum D flags    ", 0, 5, 0, NULL, 0, 0);
        wmenuitem(6,  1, " Minimum age        ", 0, 6, 0, NULL, 0, 0);
        wmenuitem(7,  1, " Time to login      ", 0, 7, 0, NULL, 0, 0);
        wmenuend(i, M_OMNI | M_SAVE, 0, 0, LGREY | _BLACK, LGREY | _BLACK, LGREY | _BLACK, BLUE | _LGREY);

        wprints(1, 21, CYAN | _BLACK, get_priv_text(config.min_login_level));
        wprints(2, 22, CYAN | _BLACK, get_flagA_text((config.min_login_flags >> 24) & 0xFF));
        wprints(3, 22, CYAN | _BLACK, get_flagB_text((config.min_login_flags >> 16) & 0xFF));
        wprints(4, 22, CYAN | _BLACK, get_flagC_text((config.min_login_flags >> 8) & 0xFF));
        wprints(5, 22, CYAN | _BLACK, get_flagD_text(config.min_login_flags & 0xFF));
        sprintf(string, "%d", config.min_login_age);
        wprints(6, 22, CYAN | _BLACK, string);
        sprintf(string, "%d", config.login_timeout);
        wprints(7, 22, CYAN | _BLACK, string);

        start_update();
        i = wmenuget();

        switch (i) {
            case 1:
                config.min_login_level = select_level(config.min_login_level, 37, 4);
                break;

            case 2:
                config.min_login_flags = window_get_flags(5, 45, 1, config.min_login_flags);
                break;

            case 3:
                config.min_login_flags = window_get_flags(5, 45, 2, config.min_login_flags);
                break;

            case 4:
                config.min_login_flags = window_get_flags(5, 45, 3, config.min_login_flags);
                break;

            case 5:
                config.min_login_flags = window_get_flags(5, 45, 4, config.min_login_flags);
                break;

            case 6:
                sprintf(string, "%d", config.min_login_age);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(6, 22, string, "???", 0, 1, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    config.min_login_age = atoi(strbtrim(string));
                }
                break;

            case 7:
                sprintf(string, "%d", config.login_timeout);
                winpbeg(BLUE | _GREEN, BLUE | _GREEN);
                winpdef(7, 22, string, "???", 0, 1, NULL, 0);
                if (winpread() != W_ESCPRESS) {
                    config.login_timeout = atoi(strbtrim(string));
                }
                break;
        }

        hidecur();

    } while (i != -1);

    wclose();
}


