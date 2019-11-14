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
#include <dos.h>
#include <string.h>
#include <process.h>
#include <stdlib.h>
#include <conio.h>
#include <dir.h>
#include <time.h>
#include <alloc.h>
#include <fcntl.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlkey.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "exec.h"

void mail_batch(char * what);
void open_logfile(void);
void rebuild_call_queue(void);
int spawn_program(int swapout, char * outstring);
void edit_outbound_info(void);
void check_new_netmail(void);
void run_external_editor(void);
void shell_to_config(void);
void shell_to_dos(void);
void new_echomail_link(void);
void new_tic_link(void);
void request_echomail_link(void);
void rescan_echomail_link(void);
void time_usage(void);
void activity_chart(void);
long random_time(int x);

extern int blank_timer, freeze;
extern char * config_file, *mday[], *wtext[];

extern int blanked, outinfo, to_row;
extern long elapsed, timeout;

extern int old_event, last_sel;
extern char interpoint;
extern long events, clocks, blankto, to, verfile, fram;

void autoprocess_mail(argc, argv, fmem)
int argc;
char * argv[];
long fmem;
{
    int m, i, doexit = 0, oc;
    long t;

    function_active = 99;
    oc = caller;
//   caller = 0;
    read_sched();
    set_event();

    time_release();

    show_statistics(fmem);

    read_sysinfo();
    if (local_mode != 2) {
        if (locked && password != NULL && registered) {
            prints(23, 43, YELLOW | _BLACK, "LOCKED  ");
        }
        else {
            prints(23, 43, YELLOW | _BLACK, "UNLOCKED");
        }

    }

    t = time(NULL);

    for (i = 1; i < argc; i++) {
        if (!stricmp(argv[i], "-NB")) {
            config->pre_import[0] = '\0';
            config->after_import[0] = '\0';
            config->pre_export[0] = '\0';
            config->after_export[0] = '\0';
            config->pre_pack[0] = '\0';
            config->after_pack[0] = '\0';
        }
    }

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            continue;
        }

        if (!stricmp(argv[i], "POLL")) {
#ifndef __OS2__
            blank_video_interrupt();
#endif
            if (!com_install(com_port)) {
#ifndef __OS2__
                restore_video_interrupt();
                wopen(9, 12, 16, 70, 0, LRED | _BLACK, LGREY | _BLACK);
                wtitle(" SYSTEM MALFUNCTION ", TCENTER, YELLOW | _BLACK);
                wcenters(1, LCYAN | _BLACK, "There doesn't appear to be a FOSSIL driver loaded.");
                wcenters(2, LCYAN | _BLACK, "LoraBBS requires a FOSSIL driver.");
                wcenters(3, LCYAN | _BLACK, "Please take care of this before attempting");
                wcenters(4, LCYAN | _BLACK, "to run Lora BBS again.");
#else
                wopen(9, 12, 16, 70, 0, LRED | _BLACK, LGREY | _BLACK);
                wtitle(" SYSTEM MALFUNCTION ", TCENTER, YELLOW | _BLACK);
                wcenters(1, LCYAN | _BLACK, "The COM port isn't available.");
                wcenters(2, LCYAN | _BLACK, "LoraBBS requires at least one COM port available.");
                wcenters(3, LCYAN | _BLACK, "Please take care of this before attempting");
                wcenters(4, LCYAN | _BLACK, "to run Lora BBS again.");
#endif

                t = timerset(500);
                while (!timeup(t) && !kbmhit()) {
                    time_release();
                }
                if (kbmhit()) {
                    getxch();
                }

                clrscrn();
                showcur();
                exit(250);
            }

#ifndef __OS2__
            restore_video_interrupt();
#endif
            rate = speed = config->speed;
            com_baud(rate);

            parse_netnode(argv[++i], &called_zone, &called_net, &called_node, &m);
            poll(1, 1, called_zone, called_net, called_node);

            doexit = 1;
            MDM_DISABLE();
        }
        else if (!stricmp(argv[i], "TOSS") || !stricmp(argv[i], "IMPORT")) {
            function_active = 0;
            timeout = 0L;
            mail_batch(config->pre_import);
            import_mail();
            doexit = 1;
        }
        else if (!stricmp(argv[i], "SCAN") || !stricmp(argv[i], "EXPORT")) {
            function_active = 0;
            timeout = 0L;
            if (!stricmp(argv[i + 1], "PACK")) {
                export_mail(ECHOMAIL_RSN | NO_PACK);
            }
            else {
                export_mail(ECHOMAIL_RSN);
            }
            doexit = 1;
            unlink("ECHOMAIL.RSN");
        }
        else if (!stricmp(argv[i], "PACK")) {
            function_active = 0;
            timeout = 0L;
            export_mail(NETMAIL_RSN);
            doexit = 1;
            unlink("NETMAIL.RSN");
        }
        else if (!stricmp(argv[i], "TIC")) {
            function_active = 0;
            timeout = 0L;
            import_tic_files();
            doexit = 1;
        }
        else if (!stricmp(argv[i], "NODELIST")) {
            function_active = 0;
            timeout = 0L;
            build_nodelist_index(1);
            doexit = 1;
        }
        else if (!strnicmp(argv[i], "TERM", 4)) {
#ifndef __OS2__
            blank_video_interrupt();
#endif
            if (!com_install(com_port)) {
#ifndef __OS2__
                restore_video_interrupt();
                wopen(9, 12, 16, 70, 0, LRED | _BLACK, LGREY | _BLACK);
                wtitle(" SYSTEM MALFUNCTION ", TCENTER, YELLOW | _BLACK);
                wcenters(1, LCYAN | _BLACK, "There doesn't appear to be a FOSSIL driver loaded.");
                wcenters(2, LCYAN | _BLACK, "LoraBBS requires a FOSSIL driver.");
                wcenters(3, LCYAN | _BLACK, "Please take care of this before attempting");
                wcenters(4, LCYAN | _BLACK, "to run Lora BBS again.");
#else
                wopen(9, 12, 16, 70, 0, LRED | _BLACK, LGREY | _BLACK);
                wtitle(" SYSTEM MALFUNCTION ", TCENTER, YELLOW | _BLACK);
                wcenters(1, LCYAN | _BLACK, "The COM port isn't available.");
                wcenters(2, LCYAN | _BLACK, "LoraBBS requires at least one COM port available.");
                wcenters(3, LCYAN | _BLACK, "Please take care of this before attempting");
                wcenters(4, LCYAN | _BLACK, "to run Lora BBS again.");
#endif

                t = timerset(500);
                while (!timeup(t) && !kbmhit()) {
                    time_release();
                }
                if (kbmhit()) {
                    getxch();
                }

                clrscrn();
                showcur();
                exit(250);
            }

#ifndef __OS2__
            restore_video_interrupt();
#endif
            com_baud(rate);

            terminal_emulator();
            doexit = 1;
            MDM_DISABLE();
        }
        else if (!isdigit(argv[i][0])) {
            status_line("!Unrecognized option: '%s'", argv[i]);
        }
    }

    if (doexit) {
        sysinfo.today.echoscan += time(NULL) - t;
        sysinfo.week.echoscan += time(NULL) - t;
        sysinfo.month.echoscan += time(NULL) - t;
        sysinfo.year.echoscan += time(NULL) - t;

        local_mode = 1;
        get_down(1, 2);
    }

    if (!local_mode) {
#ifndef __OS2__
        blank_video_interrupt();
#endif
        if (!com_install(com_port)) {
#ifndef __OS2__
            restore_video_interrupt();
            wopen(9, 12, 16, 70, 0, LRED | _BLACK, LGREY | _BLACK);
            wtitle(" SYSTEM MALFUNCTION ", TCENTER, YELLOW | _BLACK);
            wcenters(1, LCYAN | _BLACK, "There doesn't appear to be a FOSSIL driver loaded.");
            wcenters(2, LCYAN | _BLACK, "Lora BBS requires a FOSSIL driver.");
            wcenters(3, LCYAN | _BLACK, "Please take care of this before attempting");
            wcenters(4, LCYAN | _BLACK, "to run Lora BBS again.");
#else
            wopen(9, 12, 16, 70, 0, LRED | _BLACK, LGREY | _BLACK);
            wtitle(" SYSTEM MALFUNCTION ", TCENTER, YELLOW | _BLACK);
            wcenters(1, LCYAN | _BLACK, "The COM port isn't available.");
            wcenters(2, LCYAN | _BLACK, "LoraBBS requires at least one COM port available.");
            wcenters(3, LCYAN | _BLACK, "Please take care of this before attempting");
            wcenters(4, LCYAN | _BLACK, "to run Lora BBS again.");
#endif
            t = timerset(500);
            while (!timeup(t) && !kbmhit()) {
                time_release();
            }
            if (kbmhit()) {
                getxch();
            }

            clrscrn();
            showcur();
            exit(250);
        }

#ifndef __OS2__
        restore_video_interrupt();
#endif
        com_baud(rate);
    }

    get_last_caller();
    system_crash();
    caller = oc;
}

void run_external_editor()
{
    int fd, *varr;

    if (config->local_editor[0]) {
        status_line("+Shelling to editor");
        fclose(logf);
        if (modem_busy != NULL) {
            mdm_sendcmd(modem_busy);
        }
        varr = ssave();
        cclrscrn(LGREY | _BLACK);

        spawn_program(1, config->local_editor);

        if (varr != NULL) {
            srestore(varr);
        }
        if (cur_event >= 0) {
            if ((call_list[next_call].type & MAIL_WILLGO) || !(e_ptrs[cur_event]->behavior & MAT_NOOUT)) {
                events = random_time(e_ptrs[cur_event]->wait_time);
            }
        }
        else if (cur_event < 0) {
            events = timerset(500);
        }
        clocks = 0L;
        to = 0L;
        blankto = timerset(0);
        blankto += blank_timer * 6000L;
        verfile = 0L;
        hidecur();

        fd = open(config_file, O_RDONLY | O_BINARY);
        if (fd != -1) {
            read(fd, (char *)config, sizeof(struct _configuration));
            close(fd);
        }

        open_logfile();
        check_new_netmail();
        if (modem_busy != NULL) {
            modem_hangup();
        }
    }
}

void shell_to_config()
{
    FILE * fp;
    int * varr;
    char cpath[80], *p, linea[128], *pc, cmd[128];

    if (blanked) {
        stop_blanking();
    }

    if ((varr = ssave()) == NULL) {
        return;
    }

    getcwd(cpath, 79);
    fclose(logf);
    if (modem_busy != NULL) {
        mdm_sendcmd(modem_busy);
    }

    pc = config_file;
    if (!dexists(config_file)) {
        if ((p = getenv("LORA")) != NULL) {
            strcpy(linea, p);
            append_backslash(linea);
            strcat(linea, config_file);
            config_file = linea;
        }
    }

//   clown_clear ();

    strcpy(cmd, "LSETUP ");
    strcat(cmd, config_file);
    spawn_program(1, cmd);

    setdisk(cpath[0] - 'A');
    chdir(cpath);
    srestore(varr);

    fp = fopen(config_file, "rb");
    if (fp != NULL) {
        fread((char *)config, sizeof(struct _configuration), 1, fp);
        fclose(fp);
    }

    open_logfile();

    hidecur();
    freeze = 1;
    events = 0L;
    to = 0L;
    config_file = pc;
    clocks = 0L;
    blankto = timerset(0);
    blankto += blank_timer * 6000L;
    verfile = 0L;
    check_new_netmail();
    if (modem_busy != NULL) {
        modem_hangup();
    }
}

void process_startup_mail(import)
int import;
{
    long t;

    function_active = 0;

    if (blanked) {
        stop_blanking();
    }

    if (modem_busy != NULL) {
        mdm_sendcmd(modem_busy);
    }

    t = time(NULL);

    if (import) {
        mail_batch(config->pre_import);
        import_mail();
    }
    if (config->mail_method) {
        export_mail(NETMAIL_RSN);
        export_mail(ECHOMAIL_RSN);
    }
    else {
        export_mail(NETMAIL_RSN | ECHOMAIL_RSN);
    }

    sysinfo.today.echoscan += time(NULL) - t;
    sysinfo.week.echoscan += time(NULL) - t;
    sysinfo.month.echoscan += time(NULL) - t;
    sysinfo.year.echoscan += time(NULL) - t;

    local_status(msgtxt[M_SETTING_BAUD]);
    get_call_list();
    outinfo = 0;
    unlink("RESCAN.NOW");
    unlink("ECHOMAIL.RSN");
    unlink("NETMAIL.RSN");
    display_outbound_info(outinfo);
    events = 0L;
    clocks = 0L;
    blankto = timerset(0);
    blankto += blank_timer * 6000L;
    function_active = 99;
    old_event = -1;
    check_new_netmail();
    if (modem_busy != NULL) {
        modem_hangup();
    }
}

void shell_to_dos()
{
    int fd, *varr;
    char * cmd, cmdname[128], cpath[80];
    long freeram;

    if (blanked) {
        stop_blanking();
    }

#ifdef __OS2__
    freeram = 0L;
#else
    if (registered) {
        freeram = fram - 4752;
    }
    else {
        freeram = farcoreleft();
    }
#endif

    getcwd(cpath, 79);
    cmd = getenv("COMSPEC");
#ifdef __OS2__
    strcpy(cmdname, (cmd == NULL) ? "cmd.exe" : cmd);
#else
    strcpy(cmdname, (cmd == NULL) ? "command.com" : cmd);
#endif
    status_line(msgtxt[M_SHELLING]);
    fclose(logf);

    if (caller) {
        read_system_file("SHELLBYE");
    }
    else if (modem_busy != NULL) {
        mdm_sendcmd(modem_busy);
    }

    if ((varr = ssave()) == NULL) {
        return;
    }
    clown_clear();
    showcur();

    gotoxy(1, 8);
    printf(msgtxt[M_TYPE_EXIT], freeram);

    spawn_program(registered, cmdname);

    setdisk(cpath[0] - 'A');
    chdir(cpath);
    srestore(varr);

    fd = open(config_file, O_RDONLY | O_BINARY);
    if (fd != -1) {
        read(fd, (char *)config, sizeof(struct _configuration));
        close(fd);
    }

    open_logfile();

    hidecur();
    freeze = 1;
    status_line(msgtxt[M_BINKLEY_BACK]);

    if (caller) {
        read_system_file("SHELLHI");
    }
    else {
        get_call_list();
        outinfo = 0;
        unlink("RESCAN.NOW");
        display_outbound_info(outinfo);
        events = 0L;
        to = 0L;
        blankto = timerset(0);
        blankto += blank_timer * 6000L;
        check_new_netmail();
        if (modem_busy != NULL) {
            modem_hangup();
        }
    }
}

static void keyboard_loop(void)
{
    int sc, i;
    char cmdname[40];
    long t;
    struct time timep;
    struct tm * tp;

    if (timeup(clocks)) {
        clocks = timerset(97);

        if (caller) {
            if (local_mode != 2) {
                hidecur();
                i = whandle();
                wactiv(status);

                gettime((struct time *)&timep);
                sprintf(cmdname, "%02d%c%02d", timep.ti_hour % 24, interpoint, timep.ti_min % 60);
                wprints(0, 73, BLACK | _LGREY, cmdname);
                interpoint = (interpoint == ':') ? ' ' : ':';

                if (function_active == 1) {
                    sc = time_remain();
                    sprintf(cmdname, "%d mins ", sc);
                    wprints(1, 26, BLACK | _LGREY, cmdname);
                }
                else if (function_active == 4) {
                    sc = time_to_next(0);
                    if (old_event != cur_event && !blanked) {
                        wgotoxy(1, 1);
                        wdupc(' ', 34);
                        old_event = cur_event;
                    }

                    if (next_event >= 0) {
                        sprintf(cmdname, msgtxt[M_NEXT_EVENT], next_event + 1, sc / 60, sc % 60);
                        wprints(1, 1, BLACK | _LGREY, cmdname);
                    }
                    else {
                        wprints(1, 1, BLACK | _LGREY, msgtxt[M_NONE_EVENTS]);
                    }
                }

                wactiv(i);
                showcur();
            }
        }
        else {
            if (!blanked) {
                t = time(NULL);
                tp = localtime(&t);
                sprintf(cmdname, "%s, %s %d %d", wtext[tp->tm_wday], mday[tp->tm_mon], tp->tm_mday, tp->tm_year + 1900);
                prints(2, 54 + ((25 - strlen(cmdname)) / 2), YELLOW | _BLACK, cmdname);
                sprintf(cmdname, "%02d:%02d:%02d", tp->tm_hour % 24, tp->tm_min % 60, tp->tm_sec % 60);
                prints(3, 54 + ((25 - strlen(cmdname)) / 2), YELLOW | _BLACK, cmdname);

                if (elapsed) {
                    t -= elapsed;
                    sprintf(cmdname, "%02ld:%02ld", t / 60L, t % 60L);
                    prints(6, 65, YELLOW | _BLACK, cmdname);
                }

                if (timeout) {
                    t = (timeout - timerset(0)) / 100;
                    if (t < 0L) {
                        t = 0L;
                    }
                    sprintf(cmdname, "%02ld:%02ld", t / 60L, t % 60L);
                    prints(to_row, 65, YELLOW | _BLACK, cmdname);
                }
            }
        }
    }
#ifndef __OS2__
    else {
        if (have_dv) {
            dv_pause();
        }
        else if (have_ddos) {
            ddos_pause();
        }
        else if (have_tv) {
            tv_pause();
        }
        else if (have_ml) {
            ml_pause();
        }
        else if (have_os2) {
            os2_pause();
        }
        else {
            msdos_pause();
        }
    }
#endif
}

static void set_selection(void)
{
    struct _item_t * item;

    item = wmenuicurr();
    last_sel = item->tagid;
}

static void addshadow(void)
{
    wshadow(DGREY | _BLACK);
}

static long timecount;

static void reset_timecount(void)
{
    timecount = timerset(6090);
}

static void timeout_keyboard_loop(void)
{
    long t;
    char cmdname[10];

    if ((t = (timecount - timerset(0)) / 100L) < 0L) {
        t = 0L;
    }

    sprintf(cmdname, "%02ld:%02ld", t / 60L, t % 60L);
    prints(0, 73, BLUE | _LGREY, cmdname);

    if (timeup(timecount)) {
        kbput(0x011B);
        kbput(0x011B);
    }

    keyboard_loop();
}

void pull_down_menu()
{
    sysinfo.today.idletime += time(NULL) - elapsed;
    sysinfo.week.idletime += time(NULL) - elapsed;
    sysinfo.month.idletime += time(NULL) - elapsed;
    sysinfo.year.idletime += time(NULL) - elapsed;

    local_status("Interaction");

    setkbloop(timeout_keyboard_loop);
    setonkey(0x5000, reset_timecount, 0x5000);
    setonkey(0x4800, reset_timecount, 0x4800);
    setonkey(0x4B00, reset_timecount, 0x4B00);
    setonkey(0x4D00, reset_timecount, 0x4D00);
    reset_timecount();

    wmenubeg(0, 0, 0, 79, 5, BLACK | _LGREY, BLACK | _LGREY, NULL);
    wmenuitem(0, 1, " Programs ", 'P', 100, M_HASPD, NULL, 0, 0);
    wmenubeg(1, 1, 8, 29, 3, RED | _LGREY, BLUE | _LGREY, addshadow);
    wmenuitem(0, 0, " Message Editor      Alt-E ", 0, 101, M_CLALL, set_selection, 0, 0);
    wmenuitem(1, 0, " Terminal Emulator   Alt-T ", 0, 102, M_CLALL, set_selection, 0, 0);
    wmenuitem(2, 0, " Configure           Alt-C ", 0, 103, M_CLALL, set_selection, 0, 0);
    wmenuitem(3, 0, " Process Nodelist          ", 0, 104, M_CLALL, set_selection, 0, 0);
#ifdef __OS2__
    wmenuitem(4, 0, " OS/2 Shell          Alt-J ", 0, 105, M_CLALL, set_selection, 0, 0);
#else
    wmenuitem(4, 0, " DOS Shell           Alt-J ", 0, 105, M_CLALL, set_selection, 0, 0);
#endif
    wmenuitem(5, 0, " Leave Lora          Alt-X ", 0, 106, M_CLALL, set_selection, 0, 0);
    wmenuend(101, M_PD | M_SAVE, 0, 0, BLUE | _LGREY, WHITE | _LGREY, DGREY | _LGREY, YELLOW | _BLACK);
    wmenuitem(0, 11, " Utility ", 'U', 200, M_HASPD, NULL, 0, 0);
    wmenubeg(1, 11, 11, 39, 3, RED | _LGREY, BLUE | _LGREY, addshadow);
    wmenuitem(0, 0, " Process ECHOmail    Alt-P ", 0, 201, M_CLALL, set_selection, 0, 0);
    wmenuitem(1, 0, " Process TIC Files         ", 0, 209, M_CLALL, set_selection, 0, 0);
    wmenuitem(2, 0, " Inbound History           ", 0, 202, M_CLALL, set_selection, 0, 0);
    wmenuitem(3, 0, " Outbound History          ", 0, 203, M_CLALL, set_selection, 0, 0);
    wmenuitem(4, 0, " Rebuild Outbound    Alt-Q ", 0, 204, M_CLALL, set_selection, 0, 0);
    wmenuitem(5, 0, " Lock keyboard       Alt-L ", 0, 205, M_CLALL, set_selection, 0, 0);
    wmenuitem(6, 0, " Activity chart            ", 0, 206, M_CLALL, set_selection, 0, 0);
    wmenuitem(7, 0, " Time usage                ", 0, 207, M_CLALL, set_selection, 0, 0);
    wmenuitem(8, 0, " Clock Adjustment          ", 0, 208, M_CLALL, set_selection, 0, 0);
    wmenuend(201, M_PD | M_SAVE, 0, 0, BLUE | _LGREY, WHITE | _LGREY, DGREY | _LGREY, YELLOW | _BLACK);
    wmenuitem(0, 20, " Mail ", 'M', 300, M_HASPD, NULL, 0, 0);
    wmenubeg(1, 20, 13, 52, 3, RED | _LGREY, BLUE | _LGREY, addshadow);
    wmenuitem(0, 0, " Import mail                   ", 0, 310, M_CLALL, set_selection, 0, 0);
    wmenuitem(1, 0, " Export echomail               ", 0, 311, M_CLALL, set_selection, 0, 0);
    wmenuitem(2, 0, " Pack netmail                  ", 0, 312, M_CLALL, set_selection, 0, 0);
    wmenuitem(3, 0, " Request File(s)         Alt-R ", 0, 301, M_CLALL, set_selection, 0, 0);
    wmenuitem(4, 0, " Send File(s)            Alt-S ", 0, 302, M_CLALL, set_selection, 0, 0);
    wmenuitem(5, 0, " Forced Poll             Alt-M ", 0, 304, M_CLALL, set_selection, 0, 0);
    wmenuitem(6, 0, " View/Modify Outbound    Alt-V ", 0, 305, M_CLALL, set_selection, 0, 0);
    wmenuitem(7, 0, " Request ECHOmail link         ", 0, 306, M_CLALL, set_selection, 0, 0);
    wmenuitem(8, 0, " New ECHOmail link             ", 0, 307, M_CLALL, set_selection, 0, 0);
    wmenuitem(9, 0, " Rescan ECHOmail               ", 0, 308, M_CLALL, set_selection, 0, 0);
    wmenuitem(10, 0, " New TIC file link             ", 0, 309, M_CLALL, set_selection, 0, 0);
    wmenuend(310, M_PD | M_SAVE, 0, 0, BLUE | _LGREY, WHITE | _LGREY, DGREY | _LGREY, YELLOW | _BLACK);
    wmenuitem(0, 26, " BBS ", 'B', 400, M_HASPD, NULL, 0, 0);
    wmenubeg(1, 26, 4, 50, 3, RED | _LGREY, BLUE | _LGREY, addshadow);
    wmenuitem(0, 0, " Local login     Alt-K ", 0, 401, M_CLALL, set_selection, 0, 0);
    wmenuitem(1, 0, " Fast login            ", 0, 402, M_CLALL, set_selection, 0, 0);
    wmenuend(401, M_PD | M_SAVE, 0, 0, BLUE | _LGREY, WHITE | _LGREY, DGREY | _LGREY, YELLOW | _BLACK);

    if (last_sel == -1) {
        last_sel = 100;
    }
    else if (last_sel > 3040) {
        last_sel = 300;
    }
    else {
        last_sel = (last_sel / 100) * 100;
    }

    wmenuend(last_sel, M_HORZ, 0, 0, BLUE | _LGREY, WHITE | _LGREY, DGREY | _LGREY, YELLOW | _BLACK);

    if (!config->local_editor[0]) {
        wmenuidsab(101);
    }

    kbput(0x1C0D);
    if (wmenuget() != -1) {
        freonkey();
        setkbloop(keyboard_loop);

        switch (last_sel) {
            // Editor esterno
            case 101:
                run_external_editor();
                break;

            // Terminale
            case 102:
                terminal_emulator();
                hidecur();
                events = 0L;
                to = 0L;
                clocks = 0L;
                blankto = timerset(0);
                blankto += blank_timer * 6000L;
                verfile = 0L;
                break;

            // Configure
            case 103:
                shell_to_config();
                break;

            // Forza la compilazione della nodelist
            case 104:
                build_nodelist_index(1);
                old_event = -1;
                events = 0L;
                to = 0L;
                clocks = 0L;
                blankto = timerset(0);
                blankto += blank_timer * 6000L;
                verfile = 0L;
                break;

            // DOS Shell
            case 105:
                shell_to_dos();
                break;

            // Quit
            case 106:
                if (!caller && !local_mode) {
                    local_status("Shutdown");
                    status_line(msgtxt[M_EXIT_REQUEST]);
                    get_down(-1, 3);
                }
                break;

            // Process ECHOmail
            case 201:
                process_startup_mail(1);
                break;

            // Inbound history
            case 202:
                display_history(1);
                break;

            // Outbound history
            case 203:
                display_history(0);
                break;

            // Rebuild outbound
            case 204:
                if (!answer_flag && !CARRIER) {
                    rebuild_call_queue();
                    unlink("RESCAN.NOW");
                    outinfo = 0;
                    display_outbound_info(outinfo);
                }
                break;

            // Lock keyboard
            case 205:
                keyboard_password();
                if (locked && password != NULL && registered) {
                    prints(23, 43, YELLOW | _BLACK, "LOCKED  ");
                }
                break;

            // Activity chart
            case 206:
                activity_chart();
                break;

            // Time usage
            case 207:
                time_usage();
                break;

            // Remote clock
            case 208:
                poll_galileo(5, 100);
                old_event = -1;
                events = 0L;
                to = 0L;
                clocks = 0L;
                blankto = timerset(0);
                blankto += blank_timer * 6000L;
                verfile = 0L;
                break;

            case 209:
                function_active = 0;
                timeout = 0L;
                import_tic_files();
                get_call_list();
                outinfo = 0;
                unlink("RESCAN.NOW");
                display_outbound_info(outinfo);
                events = 0L;
                clocks = 0L;
                blankto = timerset(0);
                blankto += blank_timer * 6000L;
                function_active = 99;
                old_event = -1;
                break;

            // File request
            case 301:
                file_request();
                break;

            // File attach
            case 302:
                file_attach();
                break;

            // Poll manuale
            case 304:
                manual_poll();
                break;

            // View/Modify Outbound
            case 305:
                edit_outbound_info();
                break;

            // Request echomail link
            case 306:
                request_echomail_link();
                break;

            // New echomail link
            case 307:
                new_echomail_link();
                break;

            // Rescan echomail
            case 308:
                rescan_echomail_link();
                break;

            // New echomail link
            case 309:
                new_tic_link();
                break;

            case 310:
                function_active = 0;
                timeout = 0L;
                mail_batch(config->pre_import);
                import_mail();
                events = 0L;
                clocks = 0L;
                blankto = timerset(0);
                blankto += blank_timer * 6000L;
                function_active = 99;
                old_event = -1;
                break;

            case 311:
                function_active = 0;
                timeout = 0L;
                export_mail(ECHOMAIL_RSN);
                get_call_list();
                outinfo = 0;
                unlink("ECHOMAIL.RSN");
                display_outbound_info(outinfo);
                events = 0L;
                clocks = 0L;
                blankto = timerset(0);
                blankto += blank_timer * 6000L;
                function_active = 99;
                old_event = -1;
                check_new_netmail();
                break;

            case 312:
                function_active = 0;
                timeout = 0L;
                export_mail(NETMAIL_RSN);
                get_call_list();
                outinfo = 0;
                unlink("RESCAN.NOW");
                unlink("NETMAIL.RSN");
                display_outbound_info(outinfo);
                events = 0L;
                clocks = 0L;
                blankto = timerset(0);
                blankto += blank_timer * 6000L;
                function_active = 99;
                old_event = -1;
                check_new_netmail();
                break;

            // Entra nel BBS in modo locale
            case 401:
                if (!local_mode) {
                    local_mode = 1;
                    local_kbd = -1;
                }
                break;

            // Entra nel BBS in modo locale (fast)
            case 402:
                strcpy(usr.name, config->sysop);
                if (!local_mode) {
                    local_mode = 1;
                    local_kbd = -1;
                }
                break;
        }
    }
    else {
        freonkey();
    }

    kbclear();
    CLEAR_INBOUND();

    if (cur_event >= 0) {
        if ((call_list[next_call].type & MAIL_WILLGO) || !(e_ptrs[cur_event]->behavior & MAT_NOOUT)) {
            events = random_time(e_ptrs[cur_event]->wait_time);
        }
    }
    else if (cur_event < 0) {
        events = timerset(500);
    }

    clocks = 0L;

    sysinfo.today.interaction += time(NULL) - elapsed;
    sysinfo.week.interaction += time(NULL) - elapsed;
    sysinfo.month.interaction += time(NULL) - elapsed;
    sysinfo.year.interaction += time(NULL) - elapsed;

    local_status("Idle");
    blankto = timerset(0);
    blankto += blank_timer * 6000L;
    to = timerset(30000);

    setkbloop(NULL);
}

