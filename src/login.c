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
#include <io.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <alloc.h>
#include <sys\stat.h>

#include <cxl\cxlstr.h>
#include <cxl\cxlvid.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"

extern short tcpip;
extern int iemsi;
extern char usr_rip;

#define MAX_INDEX    200

void f1_special_status(char *, char *, char *);
int get_user_age(void);
int ask_random_birth(void);
void ask_birthdate(void);
void ask_default_protocol(void);
void ask_default_archiver(void);
int iemsi_login(void);
void update_sysinfo_calls(void);
char * tcpip_name(char *);

static int is_trashpwd(char *);
static int is_trashcan(char *);

int login_user(void)
{
    int fd, fflag, posit, m, original_time, i, xmq;
    char stringa[36], filename[80], c, mayberip;
    long crc;
    struct _usr tusr;
    struct _usridx usridx[MAX_INDEX];

    usr_rip = 0;
    tagged_kb = 0;
    cmd_string[0] = '\0';
    memset((char *)&lorainfo, 0, sizeof(struct _lorainfo));

    start_time = time(NULL);
    set_useron_record(LOGIN, 0, 0);

    xmq = allowed;
    if (!allowed) {
        allowed = time_to_next(1);
    }
    if (config->login_timeout == 0) {
        config->login_timeout = 255;
    }
    if (allowed > config->login_timeout) {
        allowed = config->login_timeout;
    }
    usr_class = get_class(config->logon_level);
    user_status = 0;

    CLEAR_INBOUND();

    if (!iemsi && config->ansilogon) {
        if (config->ansilogon == 3) {
            usr.ibmset = usr.ansi = usr.color = usr.formfeed = 1;
        }
        else if (local_mode || config->ansilogon == 2) {
            m_print(bbstxt[B_ANSI_LOGON]);
            if (yesno_question(DEF_YES) == DEF_YES) {
                usr.ibmset = usr.ansi = usr.color = usr.formfeed = 1;
            }
        }
        else if (config->ansilogon == 1) {
            // Riconoscimento del RIPSCRIPT
            SENDBYTE(0x1B);
            SENDBYTE('[');
            SENDBYTE('!');

            // Riconoscimento dell'ANSI (posizione del cursore).
            SENDBYTE(0x1B);
            SENDBYTE('[');
            SENDBYTE('6');
            SENDBYTE('n');

            mayberip = 0;
            i = 0;

            while ((m = TIMED_READ(2)) != -1) {
                if (!mayberip) {
                    if (i == 0 && m == 'R') {
                        mayberip = 1;
                        i++;
                    }
                    else if (i == 0 && m != 0x1B) {
                        i = 0;
                    }
                    else if (i == 1 && m != '[') {
                        i = 0;
                    }
                    else if (i == 2 && m == 'R') {
                        break;
                    }
                    else if (i == 2 && !isdigit(m) && m != ';') {
                        i = 0;
                    }
                    else if (i < 2) {
                        i++;
                    }
                }
                else {
                    if (i == 0 && m != 'R') {
                        i = 0;
                        mayberip = 0;
                    }
                    else if (i == 1 && m != 'I') {
                        i = 0;
                        mayberip = 0;
                    }
                    else if (i == 2 && m == 'P') {
                        break;
                    }
                    else if (i < 2) {
                        i++;
                    }
                }
            }

            if (i == 2 && m == 'R') {
                usr.ibmset = usr.ansi = usr.color = usr.formfeed = 1;
            }

            if (mayberip && i == 2 && m == 'P') {
                usr.ibmset = usr.ansi = usr.color = usr.formfeed = 1;
                usr_rip = 1;
            }
        }
    }

    read_system_file("LOGO");

    if (iemsi) {
        posit = iemsi_login();
        if (posit >= 0) {
            goto loginok;
        }
        if (posit == -2) {
            return (0);
        }
    }

new_login:
    stringa[0] = 0;

    m_print("\n\026\001\016%s%s login.\n", VERSION, registered ? "" : NOREG);
    CLEAR_INBOUND();

    if (!usr.name[0])
        do {
            m_print(bbstxt[B_FULLNAME]);
            chars_input(stringa, 35, INPUT_FANCY | INPUT_FIELD);
            if (!CARRIER || time_remain() <= 0) {
                return (0);
            }
        } while (strlen(stringa) < 3);
    else {
        m_print(bbstxt[B_FULLNAME]);
        m_print("%-35.35s\n", usr.name);
        strcpy(stringa, usr.name);
    }

    fancy_str(stringa);

    if (is_trashcan(stringa)) {
        status_line("!Trashcan user `%s'", stringa);
        read_system_file("TRASHCAN");
        return (0);
    }

    status_line(msgtxt[M_USER_CALLING], stringa);
    crc = crc_name(stringa);

    sprintf(filename, "%s.IDX", config->user_file);
    fd = sh_open(filename, SH_DENYWR, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);

    fflag = 0;
    posit = 0;

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

    if (!fflag) {
        lseek(fd, 0L, SEEK_SET);
        posit = 0;

        do {
            i = read(fd, (char *)&usridx, sizeof(struct _usridx) * MAX_INDEX);
            m = i / sizeof(struct _usridx);

            for (i = 0; i < m; i++)
                if (usridx[i].alias_id == crc) {
                    m = 0;
                    posit += i;
                    fflag = 1;
                    break;
                }

            if (!fflag) {
                posit += m;
            }
        } while (m == MAX_INDEX && !fflag);
    }

    close(fd);

    sprintf(filename, "%s.BBS", config->user_file);

    fd = sh_open(filename, SH_DENYWR, O_RDWR | O_BINARY, S_IREAD | S_IWRITE);
    lseek(fd, (long)posit * sizeof(struct _usr), SEEK_SET);
    read(fd, (char *)&tusr, sizeof(struct _usr));
    close(fd);

    if (!fflag) {
        status_line(msgtxt[M_NOT_IN_LIST], stringa);

        m_print(bbstxt[B_TWO_CR]);

        if (check_multilogon(stringa)) {
            read_system_file("1ATATIME");
            return (0);
        }

        m_print(bbstxt[B_NAME_NOT_FOUND]);
        m_print(bbstxt[B_NAME_ENTERED], stringa);

        do {
            m_print(bbstxt[B_NEW_USER]);
            c = yesno_question(DEF_NO | QUESTION);
            if (c == QUESTION) {
                read_system_file("WHY_NEW");
            }
            if (!CARRIER) {
                return (1);
            }
        } while (c != DEF_NO && c != DEF_YES);

        if (c == DEF_NO) {
            goto new_login;
        }

        if (config->logon_level == HIDDEN) {
            read_system_file("PREREG");
            return (0);
        }

        if (!new_user(stringa)) {
            goto new_login;
        }
        if (!CARRIER) {
            return (0);
        }
    }
    else {
        m = 1;

        free(bbstxt);
        if (!load_language(tusr.language)) {
            tusr.language = 0;
            load_language(tusr.language);
        }
        if (config->language[tusr.language].txt_path[0]) {
            text_path = config->language[tusr.language].txt_path;
        }
        else {
            text_path = config->language[0].txt_path;
        }
        usr.ansi = tusr.ansi;
        usr.avatar = tusr.avatar;
        usr.color = tusr.color;

        if (config->check_city) {
            m_print(bbstxt[B_ASK_CORRECT_CITY], tusr.name, tusr.city);
            if (yesno_question(DEF_YES) == DEF_NO) {
                usr.name[0] = '\0';
                goto new_login;
            }
        }

        for (;;) {
            m_print(bbstxt[B_PASSWORD]);
            chars_input(stringa, 15, INPUT_PWD | INPUT_FIELD);
            if (!CARRIER) {
                return (0);
            }

            if (!stricmp(tusr.pwd, strcode(strupr(stringa), tusr.name))) {
                break;
            }

            m_print(bbstxt[B_BAD_PASSWORD]);
            status_line(msgtxt[M_BAD_PASSWORD], strcode(stringa, tusr.name));

            if (++m > 4) {
                if (!read_system_file("BADPWD")) {
                    m_print(bbstxt[B_DENIED]);
                }
                status_line(msgtxt[M_INVALID_PASSWORD]);

                if (registered) {
                    tusr.badpwd = 1;
                    memcpy((char *)&usr, (char *)&tusr, sizeof(struct _usr));
                }

                return (0);
            }
        }

        if (check_multilogon(tusr.name)) {
            read_system_file("1ATATIME");
            return (0);
        }

        memcpy((char *)&usr, (char *)&tusr, sizeof(struct _usr));
    }

loginok:
    // Se data di oggi e' diversa da quella dell'ultimo collegamento, azzera
    // tutti i parametri dei limiti giornalieri.
    data(stringa);
    if (strncmp(stringa, usr.ldate, 9)) {
        usr.time = 0;
        usr.dnldl = 0;
        usr.chat_minutes = 0;
    }

#if defined (__OCC__) || defined (__TCPIP__)
    if (tcpip) {
        strcpy(usr.comment, tcpip_name(usr.comment));
    }
#endif

    if (!usr.priv) {
        read_system_file("LOCKOUT");
        return (0);
    }

    usr_class = get_class(usr.priv);

    if (usr.ovr_class.max_time) {
        config->class[usr_class].max_time = usr.ovr_class.max_time;
    }
    if (usr.ovr_class.max_call) {
        config->class[usr_class].max_call = usr.ovr_class.max_call;
    }
    if (usr.ovr_class.max_dl) {
        config->class[usr_class].max_dl = usr.ovr_class.max_dl;
    }
    else {
        switch (rate) {
            case 300:
                if (config->class[usr_class].dl_300) {
                    config->class[usr_class].max_dl = config->class[usr_class].dl_300;
                }
                break;

            case 1200:
                if (config->class[usr_class].dl_1200) {
                    config->class[usr_class].max_dl = config->class[usr_class].dl_1200;
                }
                break;

            case 2400:
            case 4800:
            case 7200:
                if (config->class[usr_class].dl_2400) {
                    config->class[usr_class].max_dl = config->class[usr_class].dl_2400;
                }
                break;

            default:
                if (config->class[usr_class].dl_9600) {
                    config->class[usr_class].max_dl = config->class[usr_class].dl_9600;
                }
                break;
        }
    }

    if (usr.ovr_class.ratio) {
        config->class[usr_class].ratio = usr.ovr_class.ratio;
    }
    if (usr.ovr_class.min_baud) {
        config->class[usr_class].min_baud = usr.ovr_class.min_baud;
    }
    if (usr.ovr_class.min_file_baud) {
        config->class[usr_class].min_file_baud = usr.ovr_class.min_file_baud;
    }
    if (usr.ovr_class.start_ratio) {
        config->class[usr_class].start_ratio = usr.ovr_class.start_ratio;
    }

    original_time = config->class[usr_class].max_call;
    if ((usr.time + original_time) > config->class[usr_class].max_time) {
        original_time = config->class[usr_class].max_time - usr.time;
    }

    if (usr.times) {
        status_line(msgtxt[M_USER_LAST_TIME], usr.ldate);
    }

    if (strncmp(stringa, usr.ldate, 9)) {
        status_line(msgtxt[M_TIMEDL_ZEROED]);
    }

    f1_status();
    update_sysinfo_calls();
    status_line(":System call #%ld", sysinfo.total_calls);

    strcpy(lorainfo.logindate, stringa);
    usr.times++;

    if (original_time <= 0) {
        read_system_file("DAYLIMIT");
        FLUSH_OUTPUT();
        modem_hangup();
        return (0);
    }

    allowed = xmq ? xmq : time_to_next(1);

    if (original_time < allowed) {
        allowed = original_time;
    }
    else {
        read_system_file("TIMEWARN");
    }

    status_line(msgtxt[M_GIVEN_LEVEL], time_remain(), get_priv_text(usr.priv));

    if (config->min_login_level && usr.priv < config->min_login_level) {
        status_line("!Level too low to login");
        read_system_file("LOGONSEC");
        return (0);
    }

    if (config->min_login_flags && ((usr.flags & config->min_login_flags) != config->min_login_flags)) {
        status_line("!Incorrect login flags");
        read_system_file("LOGONFLA");
        return (0);
    }

    if (config->min_login_age && get_user_age() && get_user_age() < config->min_login_age) {
        status_line("!User too young to login");
        read_system_file("LOGONAGE");
        return (0);
    }

    if ((config->birthdate || config->random_birth) && !usr.birthdate[0]) {
        ask_birthdate();
    }
    else if (config->random_birth) {
        if (!ask_random_birth()) {
            usr.security = 1;
            return (0);
        }
        usr.security = 0;
    }

    if (usr.badpwd && registered) {
        read_system_file("WARNPWD");
        usr.badpwd = 0;
    }

    if (usr.baud_rate && rate) {
        if (usr.baud_rate < rate) {
            read_system_file("MOREBAUD");
        }
        if (usr.baud_rate > rate) {
            read_system_file("LESSBAUD");
        }
    }

    usr.baud_rate = local_mode ? 0 : rate;

    if (!CARRIER) {
        return (0);
    }

    sprintf(filename, "%s.BBS", config->user_file);
    fd = sh_open(filename, SH_DENYWR, O_RDWR | O_BINARY, S_IREAD | S_IWRITE);
    lseek(fd, (long)posit * sizeof(struct _usr), SEEK_SET);
    write(fd, (char *)&usr, sizeof(struct _usr));
    close(fd);

    lorainfo.posuser = posit;

    if (rate && rate < config->class[usr_class].min_baud) {
        read_system_file("TOOSLOW");
        FLUSH_OUTPUT();
        modem_hangup();
        return (0);
    }

    read_system_file("WELCOME");
    if (!CARRIER) {
        return (0);
    }

    read_hourly_files();
    read_comment();

    data(stringa);
    if (!strncmp(stringa, usr.birthdate, 6)) {
        read_system_file("BIRTHDAY");
    }

    if (usr.times > 1) {
        if (usr.times < config->rookie_calls) {
            read_system_file("ROOKIE");
        }

        sprintf(filename, "%s%d", config->sys_path, posit);
        if (dexists(filename)) {
            read_file(filename);
        }
    }
    else {
        read_system_file("NEWUSER2");
    }

    return ((!CARRIER) ? 0 : 1);
}

int new_user(char * buff)
{
    int c, fd;
    char stringa[40], filename[80];
    struct _usr tusr;
    struct _usridx usridx;

    cmd_string[0] = '\0';
    memset((char *)&tusr, 0, sizeof(struct _usr));
    memset((char *)&usr, 0, sizeof(struct _usr));

    while ((c = select_language()) == -1 && CARRIER);
    tusr.language = (byte) c;

    if (!CARRIER) {
        return (0);
    }
    free(bbstxt);
    if (!load_language(tusr.language)) {
        tusr.language = 0;
        load_language(tusr.language);
    }
    if (config->language[tusr.language].txt_path[0]) {
        text_path = config->language[tusr.language].txt_path;
    }
    else {
        text_path = config->language[0].txt_path;
    }

    read_system_file("APPLIC");

    strcpy(tusr.name, buff);
    strcpy(tusr.handle, buff);

    if (rate && rate >= speed_graphics) {
        if (config->ansigraphics == 2) {
            do {
                m_print(bbstxt[B_ASK_ANSI]);
                c = yesno_question(DEF_NO | QUESTION);
                if (c == QUESTION) {
                    read_system_file("WHY_ANSI");
                }
                if (!CARRIER) {
                    return (1);
                }
            } while (c != DEF_NO && c != DEF_YES);

            if (c == DEF_YES) {
                tusr.ansi = 1;
            }
        }
        else {
            tusr.ansi = config->ansigraphics;
        }

        if (!tusr.ansi) {
            if (config->avatargraphics == 2) {
                do {
                    m_print(bbstxt[B_ASK_AVATAR]);
                    c = yesno_question(DEF_NO | QUESTION);
                    if (c == QUESTION) {
                        read_system_file("WHY_AVT");
                    }
                    if (!CARRIER) {
                        return (1);
                    }
                } while (c != DEF_NO && c != DEF_YES);

                if (c == DEF_YES) {
                    tusr.avatar = 1;
                }
            }
            else {
                tusr.avatar = config->avatargraphics;
            }
        }

        if (!CARRIER) {
            return (1);
        }

        if (tusr.ansi || tusr.avatar) {
            do {
                m_print(bbstxt[B_ASK_COLOR]);
                c = yesno_question(DEF_YES | QUESTION);
                if (c == QUESTION) {
                    read_system_file("WHY_COL");
                }
                if (!CARRIER) {
                    return (1);
                }
            } while (c != DEF_NO && c != DEF_YES);

            if (c == DEF_YES) {
                tusr.color = 1;
            }
        }

        memcpy((char *)&usr, (char *)&tusr, sizeof(struct _usr));
        usr.name[0] = '\0';

        tusr.use_lore = 1;

        if (tusr.ansi || tusr.avatar) {
            if (config->fullscrnedit == 2) {
                do {
                    m_print(bbstxt[B_ASK_EDITOR]);
                    c = yesno_question(DEF_YES | QUESTION);
                    if (c == QUESTION) {
                        read_system_file("WHY_OPED");
                    }
                    if (!CARRIER) {
                        return (1);
                    }
                } while (c != DEF_NO && c != DEF_YES);

                if (c == DEF_YES) {
                    tusr.use_lore = 0;
                }
            }
            else if (config->fullscrnedit) {
                tusr.use_lore = 0;
            }
            else {
                tusr.use_lore = 1;
            }
        }

        if (tusr.ansi || tusr.avatar) {
            do {
                m_print(bbstxt[B_ASK_FULLREAD]);
                c = yesno_question(DEF_YES | QUESTION);
                if (c == QUESTION) {
                    read_system_file("WHY_FULR");
                }
                if (!CARRIER) {
                    return (1);
                }
            } while (c != DEF_NO && c != DEF_YES);

            if (c == DEF_YES) {
                tusr.full_read = 1;
            }
        }
    }

    if (config->ask_alias) {
        handle_change();
        strcpy(tusr.handle, usr.handle);
    }

    if (config->hotkeys == 2) {
        m_print(bbstxt[B_USE_HOTKEY]);
        if (yesno_question(DEF_YES) == DEF_YES) {
            usr.hotkey = tusr.hotkey = 1;
        }
    }
    else {
        usr.hotkey = tusr.hotkey = config->hotkeys;
    }

    if (config->ibmset) {
        do {
            m_print(bbstxt[B_ASK_IBMSET]);
            c = yesno_question((usr.ansi || usr.avatar) ? DEF_YES | QUESTION : DEF_NO | QUESTION);
            if (c == QUESTION) {
                read_system_file("WHY_IBM");
            }
            if (!CARRIER) {
                return (1);
            }
        } while (c != DEF_NO && c != DEF_YES);

        if (c == DEF_YES) {
            tusr.ibmset = 1;
        }
    }
    else {
        tusr.ibmset = 1;
    }

    usr.len = 24;
    screen_change();
    tusr.len = usr.len;

    if (config->moreprompt == 2) {
        do {
            m_print(bbstxt[B_ASK_MORE]);
            c = yesno_question(DEF_YES);
            if (!CARRIER) {
                return (1);
            }
        } while (c != DEF_NO && c != DEF_YES);

        if (c == DEF_YES) {
            tusr.more = 1;
        }
    }
    else {
        tusr.more = config->moreprompt;
    }

    if (config->screenclears == 2) {
        do {
            m_print(bbstxt[B_ASK_CLEAR]);
            c = yesno_question(DEF_YES);
            if (!CARRIER) {
                return (1);
            }
        } while (c != DEF_NO && c != DEF_YES);

        if (c == DEF_YES) {
            tusr.formfeed = 1;
        }
    }
    else {
        tusr.formfeed = config->screenclears;
    }

    memcpy((char *)&usr, (char *)&tusr, sizeof(struct _usr));
    usr.name[0] = '\0';

    do {
        m_print(bbstxt[B_CITY_STATE]);
        chars_input(stringa, 35, INPUT_FIELD);
        if (!CARRIER) {
            return (1);
        }
    } while (!strlen(stringa));

    strcpy(tusr.city, fancy_str(stringa));

    if (config->birthdate) {
        ask_birthdate();
        strcpy(tusr.birthdate, usr.birthdate);
    }

    if (config->voicephone) {
        voice_phone_change();
        strcpy(tusr.voicephone, usr.voicephone);
        if (!CARRIER) {
            return (1);
        }
    }

    if (config->dataphone) {
        data_phone_change();
        strcpy(tusr.dataphone, usr.dataphone);
        if (!CARRIER) {
            return (1);
        }
    }

    if (config->mailcheck == 2) {
        do {
            m_print(bbstxt[B_LOGON_CHECKMAIL]);
            c = yesno_question(DEF_YES);
            if (!CARRIER) {
                return (1);
            }
        } while (c != DEF_NO && c != DEF_YES);

        if (c == DEF_YES) {
            tusr.scanmail = 1;
        }
    }
    else {
        tusr.scanmail = config->mailcheck;
    }

    if (config->ask_protocol) {
        ask_default_protocol();
        tusr.protocol = usr.protocol;
    }

    if (config->ask_packer) {
        ask_default_archiver();
        tusr.archiver = usr.archiver;
    }

    memcpy((char *)&usr, (char *)&tusr, sizeof(struct _usr));
    usr.name[0] = '\0';

    strcpy(tusr.ldate, " 1 Jan 80  00:00:00");
    data(tusr.firstdate);
    tusr.priv = config->logon_level;
    tusr.flags = config->logon_flags;
    tusr.width = 80;
    tusr.tabs = 1;
    tusr.ptrquestion = -1L;
    if (config->filebox) {
        tusr.havebox = 1;
    }

    memcpy((char *)&usr, (char *)&tusr, sizeof(struct _usr));
    usr.name[0] = '\0';

    read_system_file("NEWUSER1");
    change_attr(LRED | _BLACK);

    for (;;) {
        do {
            m_print(bbstxt[B_SELECT_PASSWORD]);
            chars_input(stringa, 15, INPUT_PWD | INPUT_FIELD);
            if (!CARRIER) {
                return (1);
            }
        } while (strlen(stringa) < 4);

        if (is_trashpwd(stringa)) {
            status_line("!Trashcan password `%s'", stringa);
            read_system_file("TRASHPWD");
            continue;
        }

        strcpy(tusr.pwd, stringa);

        do {
            m_print(bbstxt[B_VERIFY_PASSWORD]);
            chars_input(stringa, 15, INPUT_PWD | INPUT_FIELD);
            if (!CARRIER) {
                return (1);
            }
        } while (!strlen(stringa));

        if (!stricmp(stringa, tusr.pwd)) {
            break;
        }

        m_print(bbstxt[B_WRONG_PWD1], strupr(tusr.pwd));
        m_print(bbstxt[B_WRONG_PWD2], strupr(stringa));
    }

    strcpy(tusr.pwd, strcode(strupr(stringa), tusr.name));
    data(stringa);
    stringa[9] = '\0';
    strcpy(usr.lastpwdchange, stringa);

    usridx.id = crc_name(tusr.name);
    usridx.alias_id = crc_name(tusr.handle);
    tusr.id = usridx.id;
    tusr.alias_id = usridx.alias_id;
    memcpy((char *)&usr, (char *)&tusr, sizeof(struct _usr));

    sprintf(filename, "%s.IDX", config->user_file);
    fd = open(filename, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
    lseek(fd, 0L, SEEK_END);
    write(fd, (char *)&usridx, sizeof(struct _usridx));
    close(fd);

    sprintf(filename, "%s.BBS", config->user_file);
    fd = open(filename, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
    lseek(fd, 0L, SEEK_END);
    write(fd, (char *)&usr, sizeof(struct _usr));
    close(fd);

    return (1);
}

long crc_name(name)
char * name;
{
    int i;
    long crc;

    crc = 0xFFFFFFFFL;
    for (i = 0; i < strlen(name); i++) {
        crc = Z_32UpdateCRC(((unsigned short) name[i]), crc);
    }

    return (crc);
}

static int is_trashpwd(s)
char * s;
{
    FILE * fp;
    char buffer[40];

    fp = fopen("PWDTRASH.CFG", "rt");
    if (fp != NULL) {
        while (fgets(buffer, 38, fp) != NULL) {
            buffer[strlen(buffer) - 1] = '\0';
            if (!stricmp(buffer, s)) {
                return (1);
            }
        }

        fclose(fp);
    }

    return (0);
}

static int is_trashcan(s)
char * s;
{
    FILE * fp;
    char buffer[40];

    fp = fopen("TRASHCAN.CFG", "rt");
    if (fp != NULL) {
        while (fgets(buffer, 38, fp) != NULL) {
            buffer[strlen(buffer) - 1] = '\0';
            if (!stricmp(buffer, s)) {
                return (1);
            }
        }

        fclose(fp);
    }

    return (0);
}

int iemsi_login(void)
{
    int fd, fflag, posit, m, i;
    char stringa[36], filename[80], c;
    long crc;
    struct _usr tusr;
    struct _usridx usridx[MAX_INDEX];
    struct _usridx uidx;

    status_line(":IEMSI login detected");

    strcpy(stringa, strbtrim(usr.name));
    strcpy(usr.name, fancy_str(stringa));

    if (is_trashcan(stringa)) {
        status_line("!Trashcan user `%s'", stringa);
        read_system_file("TRASHCAN");
        return (-2);
    }

    status_line(msgtxt[M_USER_CALLING], stringa);
    crc = crc_name(stringa);

    sprintf(filename, "%s.IDX", config->user_file);
    fd = sh_open(filename, SH_DENYWR, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);

    fflag = 0;
    posit = 0;

    do {
        i = read(fd, (char *)&usridx, sizeof(struct _usridx) * MAX_INDEX);
        m = i / sizeof(struct _usridx);

        for (i = 0; i < m; i++, posit++)
            if (usridx[i].id == crc) {
                fflag = 1;
                break;
            }
    } while (m == MAX_INDEX && !fflag);

    if (!fflag) {
        lseek(fd, 0L, SEEK_SET);
        posit = 0;

        do {
            i = read(fd, (char *)&usridx, sizeof(struct _usridx) * MAX_INDEX);
            m = i / sizeof(struct _usridx);

            for (i = 0; i < m; i++, posit++)
                if (usridx[i].alias_id == crc) {
                    fflag = 1;
                    break;
                }
        } while (m == MAX_INDEX && !fflag);
    }

    close(fd);

    if (!fflag) {
        status_line(msgtxt[M_NOT_IN_LIST], stringa);

        m_print(bbstxt[B_TWO_CR]);

        if (check_multilogon(stringa)) {
            read_system_file("1ATATIME");
            return (-2);
        }

        if (config->logon_level == HIDDEN) {
            read_system_file("PREREG");
            return (-2);
        }

        while ((c = select_language()) == -1 && CARRIER)
            ;
        usr.language = (byte) c;

        if (!CARRIER) {
            return (0);
        }
        free(bbstxt);
        if (!load_language(usr.language)) {
            usr.language = 0;
            load_language(usr.language);
        }
        if (config->language[usr.language].txt_path[0]) {
            text_path = config->language[usr.language].txt_path;
        }
        else {
            text_path = config->language[0].txt_path;
        }

        read_system_file("APPLIC");

        if (config->birthdate) {
            ask_birthdate();
        }

        if (config->ask_protocol) {
            ask_default_protocol();
        }

        if (config->ask_packer) {
            ask_default_archiver();
        }

        strcode(strupr(usr.pwd), usr.name);

        strcpy(usr.ldate, " 1 Jan 80  00:00:00");
        data(stringa);
        strcpy(usr.firstdate, stringa);
        stringa[9] = '\0';
        strcpy(usr.lastpwdchange, stringa);
        usr.priv = config->logon_level;
        usr.flags = config->logon_flags;
        usr.tabs = 1;
        usr.ptrquestion = -1L;
        if (config->filebox) {
            usr.havebox = 1;
        }

        uidx.id = crc_name(usr.name);
        uidx.alias_id = crc_name(usr.handle);
        usr.id = uidx.id;
        usr.alias_id = uidx.alias_id;

        sprintf(filename, "%s.IDX", config->user_file);
        fd = sh_open(filename, SH_DENYWR, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        lseek(fd, 0L, SEEK_END);
        posit = (int)(tell(fd) / sizeof(struct _usridx));
        write(fd, (char *)&uidx, sizeof(struct _usridx));
        close(fd);

        sprintf(filename, "%s.BBS", config->user_file);
        fd = sh_open(filename, SH_DENYWR, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        lseek(fd, 0L, SEEK_END);
        write(fd, (char *)&usr, sizeof(struct _usr));
        close(fd);
    }
    else {
        sprintf(filename, "%s.BBS", config->user_file);

        fd = sh_open(filename, SH_DENYWR, O_RDWR | O_BINARY, S_IREAD | S_IWRITE);
        lseek(fd, (long)posit * sizeof(struct _usr), SEEK_SET);
        read(fd, (char *)&tusr, sizeof(struct _usr));
        close(fd);

        m = 1;

        free(bbstxt);
        if (!load_language(tusr.language)) {
            tusr.language = 0;
            load_language(tusr.language);
        }
        if (config->language[tusr.language].txt_path[0]) {
            text_path = config->language[tusr.language].txt_path;
        }
        else {
            text_path = config->language[0].txt_path;
        }

        if (stricmp(tusr.pwd, strcode(strupr(usr.pwd), tusr.name))) {
            m_print(bbstxt[B_BAD_PASSWORD]);
            status_line(msgtxt[M_BAD_PASSWORD], strcode(stringa, tusr.name));

            return (-1);
        }

        if (check_multilogon(tusr.name)) {
            read_system_file("1ATATIME");
            return (-2);
        }

        strcpy(tusr.handle, usr.handle);
        strcpy(tusr.city, usr.city);
        strcpy(tusr.dataphone, usr.dataphone);
        strcpy(tusr.voicephone, usr.voicephone);
        tusr.ansi = usr.ansi;
        tusr.avatar = usr.avatar;
        tusr.color = usr.color;
        tusr.len = usr.len;
        tusr.width = usr.width;
        tusr.nulls = usr.nulls;
        tusr.ibmset = usr.ibmset;
        tusr.more = usr.more;
        tusr.hotkey = usr.hotkey;
        tusr.formfeed = usr.formfeed;
        tusr.use_lore = usr.use_lore;

        memcpy((char *)&usr, (char *)&tusr, sizeof(struct _usr));
    }

    return (posit);
}

