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
#include <signal.h>
#include <string.h>
#include <process.h>
#include <stdlib.h>
#include <conio.h>
#include <dir.h>
#include <time.h>
#include <fcntl.h>

#ifdef __OS2__
#define INCL_DOS
#include <os2.h>

void VioUpdate(void);
#endif

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlkey.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

#define IEMSI_ON        0x0001
#define IEMSI_MAILCHECK 0x0002
#define IEMSI_FILECHECK 0x0004
#define IEMSI_NEWS      0x0008

#ifndef __OS2__
extern unsigned int _stklen = 10240U;
unsigned _ovrbuffer = 0xA00;
#endif

extern int blank_timer;
extern short tcpip;
extern char * mday[], *wtext[], * * dos_argv, fulleditor, nopoll, nomailproc;

#ifdef __OS2__
extern HFILE hfComHandle;
#endif

void open_logfile(void);
int spawn_program(int swapout, char * outstring);
void set_prior(int);
void rebuild_call_queue(void);
void edit_outbound_info(void);
void set_flags(void);
void external_bbs(char * s);
int iemsi_session(void);
void get_bad_call(int, int, int, int, int);
void check_new_netmail(void);
char * tcpip_name(char *);

int blanked, outinfo = 0, to_row = 0, iemsi = 0;
long elapsed = 0L, timeout = 0L, exectime = 0L;

int posit = 0, old_event, last_sel = 100;
char interpoint = ':';
long events, clocks, blankto, to, verfile, fram;
struct _minf minf;

//#ifdef __OS2__
//static long pollkey = 0L;
//#endif

static void remote_task(long);
static int execute_events(void);
static void snake_screen_blanker(int init);
static void star_screen_blanker(int init);

void pull_down_menu(void);
void run_external_editor(void);
void shell_to_dos(void);
void shell_to_config(void);
void autoprocess_mail(int argc, char * argv[], long fmem);
void parse_config_line(int argc, char ** argv);

void main(argc, argv)
int argc;
char ** argv;
{
    int j;

    _fmode = SH_DENYNONE | O_BINARY;

#ifndef __OS2__
    signal(SIGINT, SIG_IGN);
    signal(SIGABRT, SIG_IGN);
//   signal (SIGTERM, SIG_IGN);
//   ctrlbrk (ctrlchand);
//   setcbrk (0);
#else
    signal(SIGINT, SIG_IGN);
    signal(SIGABRT, SIG_IGN);
//   signal (SIGTERM, SIG_IGN);
    _wscroll = 0;

//   DosSetPrty (PRTYS_PROCESSTREE, PRTYC_IDLETIME, PRTYD_MAXIMUM, 0);
#endif

    directvideo = 1;
    registered = 1;
    tzset();

    exectime = time(NULL);
    init_system();

    parse_config_line(argc, argv);
    if (!parse_config()) {
        exit(1);
    }

    parse_command_line(argc, argv);

    if (tcpip != 0) {
        caller = 1;
        snooping = 1;
        dos_argv = NULL;
    }

    if (local_mode != 2) {
        setup_screen();
        local_status("Startup");
    }
    else {
        setup_bbs_screen();
    }

    /* Initialize the random number generator */
    j = (int) time(NULL);
    srand((unsigned int) j);

    old_event = -1;
    open_logfile();

    events = time(NULL);
    while (events == time(NULL))
        ;

    events = 0L;
    clocks = 0L;
    verfile = 0L;

    minf.def_zone = config->alias[0].zone;
    MsgOpenApi(&minf);

//   set_prior (2);
    autoprocess_mail(argc, argv, atol(argv[argc - 1]));

    if (!local_mode) {
        SET_DTR_ON();
    }
    remote_task(atol(argv[argc - 1]));

    exit(1);
}

long random_time(int x)
{
    int i;

    if (x == 0) {
        x = 10;
    }

    if (!config->random_redial) {
        return (timerset((unsigned int)(x * 100)));
    }

    /* Number of seconds to delay is random based on x +/- 50% */
    i = (rand() % (x + 1)) + (x / 2);

    return (timerset((unsigned int)(i * 100)));
}

static void remote_task(fmem)
long fmem;
{
    int i, m;
    char buffer[80];
    long ansto = 0L, t;
    struct ffblk blk;

    fram = fmem;
    if (!caller && !local_mode && tcpip == 0) {
        get_call_list();
        display_outbound_info(outinfo);
        check_new_netmail();
    }

    if (caller && remote_net && remote_node) {
        caller = 0;
        poll(1, 1, remote_zone, remote_net, remote_node);
        terminating_call();
        get_down(aftercaller_exit, 2);
        return;
    }

    rate = speed;
    com_baud(speed);

    if (!local_mode && !caller && tcpip == 0) {
        local_status(msgtxt[M_SETTING_BAUD]);

        caller = 0;
        to = timerset(200);
        blankto = timerset(0);
        blankto += blank_timer * 6000L;
        set_useron_record(WFC, 0, 0);

        do {
            if (answer_flag) {
                if ((i = wait_for_connect(1)) == 1) {
                    break;
                }
                if (!answer_flag) {
                    local_status(msgtxt[M_SETTING_BAUD]);
                }
                else if (timeup(ansto)) {
                    answer_flag = 0;
                    status_line("!Answer timeout");
                    initialize_modem();
                    local_status(msgtxt[M_SETTING_BAUD]);
                    to = timerset(30000);
                }
            }
            else {
                if ((i = wait_for_connect(1)) == 1) {
                    break;
                }
                if (answer_flag) {
                    ansto = timerset(6000);
                    sysinfo.today.idletime += time(NULL) - elapsed;
                    sysinfo.week.idletime += time(NULL) - elapsed;
                    sysinfo.month.idletime += time(NULL) - elapsed;
                    sysinfo.year.idletime += time(NULL) - elapsed;
                    local_status("Answering");
                }
            }

            if (timeup(verfile)) {
                if (registered) {
                    if (dexists("ECHOMAIL.RSN") || dexists("NETMAIL.RSN")) {
                        function_active = 0;
                        if (blanked) {
                            stop_blanking();
                        }
                        if (modem_busy != NULL) {
                            mdm_sendcmd(modem_busy);
                        }

                        t = time(NULL);

                        if (dexists("NETMAIL.RSN")) {
                            if (dexists("ECHOMAIL.RSN")) {
                                if (config->mail_method) {
                                    export_mail(NETMAIL_RSN);
                                    export_mail(ECHOMAIL_RSN);
                                }
                                else {
                                    export_mail(NETMAIL_RSN | ECHOMAIL_RSN);
                                }
                            }
                            else {
                                export_mail(NETMAIL_RSN);
                            }
                            unlink("NETMAIL.RSN");
                        }
                        else {
                            export_mail(ECHOMAIL_RSN);
                        }
                        unlink("ECHOMAIL.RSN");

                        sysinfo.today.echoscan += time(NULL) - t;
                        sysinfo.week.echoscan += time(NULL) - t;
                        sysinfo.month.echoscan += time(NULL) - t;
                        sysinfo.year.echoscan += time(NULL) - t;

                        if (cur_event >= 0) {
                            if ((call_list[next_call].type & MAIL_WILLGO) || !(e_ptrs[cur_event]->behavior & MAT_NOOUT)) {
                                events = random_time(e_ptrs[cur_event]->wait_time);
                            }
                        }
                        else if (cur_event < 0) {
                            events = timerset(500);
                        }

                        local_status(msgtxt[M_SETTING_BAUD]);
                        rebuild_call_queue();
                        outinfo = 0;
                        unlink("RESCAN.NOW");
                        display_outbound_info(outinfo);
                        events = 0L;
                        clocks = 0L;
                        to = 0L;
                        old_event = -1;
                        function_active = 99;

                        blankto = timerset(0);
                        blankto += blank_timer * 6000L;
                    }

                    if (dexists("RESCAN.NOW")) {
                        if (blanked) {
                            stop_blanking();
                        }
                        rebuild_call_queue();
                        outinfo = 0;
                        display_outbound_info(outinfo);
                        unlink("RESCAN.NOW");
                        blankto = timerset(0);
                        blankto += blank_timer * 6000L;
                    }

                    sprintf(buffer, "%sLEXIT*.*", config->sys_path);
                    if (!findfirst(buffer, &blk, 0))
                        do {
                            i = m = 0;
                            sscanf(blk.ff_name, "LEXIT%d.%d", &i, &m);
                            if (i == line_offset || i == 0) {
                                unlink(blk.ff_name);
                                get_down(m, 3);
                            }
                        } while (!findnext(&blk));
                }

                verfile = timerset(1000);
            }

            else if (!execute_events()) {
                local_status(msgtxt[M_SETTING_BAUD]);
                to = timerset(30000);
            }

            else if (timeup(to)) {
                if (!answer_flag && !terminal) {
                    CLEAR_INBOUND();
                    initialize_modem();
                }
                to = timerset(30000);
            }

            else if (timeup(blankto) && blank_timer) {
                if (answer_flag) {
                    blankto = timerset(0);
                    blankto += blank_timer * 6000L;
                }
                else {
                    if (!blanked) {
                        begin_blanking();
                    }
                    else {
                        blank_progress();
                    }
                    blankto = timerset(20);
                    time_release();
                    release_timeslice();
                }
            }

            else {
                time_release();
                release_timeslice();
            }

            if (local_kbd != -1) {
                switch (local_kbd) {

                    // ESC Attiva i menu' pull-down
                    case 0x1B:
                        pull_down_menu();
                        break;

                    // Alt-A Manual Answer
                    case 0x1E00:
                        mdm_sendcmd((config->answer[0] == '\0') ? "ATA|" : config->answer);
                        answer_flag = 1;
                        ansto = timerset(6000);
                        sysinfo.today.idletime += time(NULL) - elapsed;
                        sysinfo.week.idletime += time(NULL) - elapsed;
                        sysinfo.month.idletime += time(NULL) - elapsed;
                        sysinfo.year.idletime += time(NULL) - elapsed;
                        local_status("Answering");
                        break;

                    // Alt-D Chiama subito
                    case 0x2000:
                        events = timerset(0);
                        break;

                    // Alt-I Inizializza il modem
                    case 0x1700:
                        initialize_modem();
                        break;

                    // ALT-X Esce dal programma
                    case 0x2D00:
                        if (!caller && !local_mode) {
                            local_status("Shutdown");
                            status_line(msgtxt[M_EXIT_REQUEST]);
                            get_down(-1, 3);
                        }
                        break;

                    // ALT-P Process ECHOmail
                    case 0x1900:
                        process_startup_mail(1);
                        break;

                    // ALT-Q Rebuild outbound
                    case 0x1000:
                        if (!answer_flag && !CARRIER) {
                            rebuild_call_queue();
                            unlink("RESCAN.NOW");
                            outinfo = 0;
                            display_outbound_info(outinfo);
                        }
                        break;

                    // ALT-C Setup
                    case 0x2E00:
                        shell_to_config();
                        break;

                    // ALT-M Poll manuale
                    case 0x3200:
                        manual_poll();
                        break;

                    // Alt-V View/Modify Outbound
                    case 0x2F00:
                        edit_outbound_info();
                        break;

                    // ALT-E Editor esterno
                    case 0x1200:
                        run_external_editor();
                        break;

                    // ALT-T Terminale
                    case 0x1400:
                        terminal_emulator();
                        hidecur();
                        events = 0L;
                        to = 0L;
                        clocks = 0L;
                        blankto = timerset(0);
                        blankto += blank_timer * 6000L;
                        verfile = 0L;
                        break;

                    // ALT-S File attach
                    case 0x1F00:
                        file_attach();
                        break;

                    // ALT-R File request
                    case 0x1300:
                        file_request();
                        break;

                    // Entra nel BBS in modo locale
                    case 0x2500:
                        if (!local_mode) {
                            local_mode = 1;
                        }
                        break;

                    // ALT-J DOS Shell
                    case 0x2400:
                        shell_to_dos();
                        break;

                    // ALT-L Lock keyboard
                    case 0x2600:
                        keyboard_password();
                        if (locked && password != NULL && registered) {
                            prints(23, 43, YELLOW | _BLACK, "LOCKED  ");
                        }
                        break;

                    // Up arrow
                    case 0x4800:
                        if (outinfo > 0) {
                            outinfo--;
                            display_outbound_info(outinfo);
                        }
                        break;

                    // Down arrow
                    case 0x5000:
                        if ((outinfo + 8) < max_call) {
                            outinfo++;
                            display_outbound_info(outinfo);
                        }
                        break;

                    // ALT-F1 - ALT-F10 Exit with errorlevel
                    case 0x6800:
                    case 0x6900:
                    case 0x6A00:
                    case 0x6B00:
                    case 0x6C00:
                    case 0x6D00:
                    case 0x6E00:
                    case 0x6F00:
                    case 0x7000:
                    case 0x7100:
                        i = (int)(((unsigned) local_kbd) >> 8);
                        status_line(msgtxt[M_FUNCTION_KEY], (i - 0x67) * 10);
                        get_down((i - 0x67) * 10, 3);
                        break;
                }

                local_kbd = -1;
            }

        } while (!local_mode);

        if (local_mode) {
            rate = speed;
            local_status("LocalMode");
        }
    }

    if (local_mode) {
        if (modem_busy != NULL) {
            mdm_sendcmd(modem_busy);
        }
        status_line("#Connect Local");
        i = 0;
        config->snooping = 1;
        snooping = 1;
    }
#if defined (__OCC__) || defined (__TCPIP__)
    else if (tcpip) {
        status_line("#Connect TCP/IP (%s)", tcpip_name(buffer));
        i = 0;
        config->snooping = 1;
        snooping = 1;
    }
#endif
    else if (rate == -1) {
        // E' stato ricevuto un CONNECT FAX
        if (mdm_flags == NULL) {
            status_line("#Connect FAX%s%s", "", "");
            mdm_flags = "";
        }
        else {
            status_line("#Connect FAX%s%s", "/", mdm_flags);
        }

        // Forza lo stato delle variabili per evitare di abbattere la chiamata
        // prima di lanciare il programma di ricezione dei fax.
        local_mode = 0;
        modem_busy = NULL;

        // Se e' presente un errorlevel di uscita per il fax, allora esce al
        // DOS con l'errorlevel, altrimenti utilizza il RCVFAX.

        if (config->fax_errorlevel) {
            get_down(config->fax_errorlevel, 0);
        }
        else {
#ifdef __OS2__
            sprintf(e_input, "RCVFAX %d %ld %ld", com_port + 1, rate, hfComHandle);
#else
            sprintf(e_input, "RCVFAX %d %ld", com_port + 1, rate);
#endif
            status_line("+Start FAX receiving program");
            if ((i = spawn_program(registered, e_input)) != 0) {
                status_line(":Return code: %d", i);
            }

            modem_hangup();
            get_down(0, 2);
        }
    }
    else {
        caller = 0;
        function_active = 0;

        if (mdm_flags == NULL) {
            status_line(msgtxt[M_READY_CONNECT], rate, "", "");
            mdm_flags = "";
        }
        else {
            status_line(msgtxt[M_READY_CONNECT], rate, "/", mdm_flags);
        }

        if (!no_logins || !registered || (cur_event >= 0 && (e_ptrs[cur_event]->behavior & MAT_OUTONLY))) {
            i = mail_session();
        }
        else {
            iemsi_session();
            i = 0;
        }
    }

    caller = 1;

    if (!i) {
        if (!config->bbs_batch[0]) {
#ifndef POINT
            snooping = config->snooping;
            if (config->snooping) {
                if (local_mode != 2) {
                    setup_bbs_screen();
                }
                showcur();
            }
            else {
                local_status("Login");
            }

            load_language(0);
            text_path = config->language[0].txt_path;

            if (login_user()) {
                if (!snooping) {
                    prints(5, 65, YELLOW | _BLACK, "User on-line");
                }

                sprintf(buffer, "B%ld", rate);
                read_system_file(buffer);

                sprintf(buffer, "SEC%d", usr.priv);
                if (!read_system_file(buffer)) {
                    read_system_file("SECALL");
                }

                if ((i = check_subscription()) != -1) {
                    status_line("+Subscription left: %d (%s)", i, usr.subscrdate);
                    if (i < 31) {
                        if (i > 0) {
                            read_system_file("SUBWARN");
                        }
                        else {
                            read_system_file("SUBDATE");
                        }
                    }
                }

                if (usr.scanmail || (iemsi & IEMSI_MAILCHECK))
                    if (scan_mailbox()) {
                        mail_read_forward(0);
                        menu_dispatcher("READMAIL");
                    }

                if (!iemsi || (iemsi & IEMSI_MAILCHECK)) {
                    download_filebox(1);
                }

                if (config->newfilescheck) {
                    if (config->newfilescheck == 2 && !(iemsi & IEMSI_FILECHECK)) {
                        m_print(bbstxt[B_CHECK_NEW_FILES]);
                        if (yesno_question(DEF_NO)) {
                            new_file_list(2);
                        }
                    }
                    else if (config->newfilescheck == 1 || (iemsi & IEMSI_FILECHECK)) {
                        new_file_list(2);
                    }
                }

                read_system(usr.msg, 1);
                read_system(usr.files, 2);

                if (!iemsi || (iemsi & IEMSI_NEWS)) {
                    read_system_file("NEWS");
                }

                menu_dispatcher(NULL);
            }
#endif
        }
        else {
            external_bbs(config->bbs_batch);
        }
    }
    else {
        caller = 0;
    }

    hidecur();

    if (!(MODEM_STATUS() & 0x80) && !local_mode) {
        status_line(msgtxt[M_NO_CARRIER]);
    }

    terminating_call();

    get_down(aftercaller_exit, 2);
}

void resume_blanked_screen()
{
    if (blanked) {
        stop_blanking();
    }
    else {
        blankto = timerset(0);
        blankto += blank_timer * 6000L;
    }
}

void time_release(void)
{
    int sc, i, ch, hh, mm, ss, scanc;
    char cmdname[40];
    long t;
    struct time timep;
    struct tm * tp, tpt;

//	if (!emulator && local_mode != 3) {
    if (!emulator) {
        if (kbmhit()) {
            ch = getxch();

            // Se lo schermo e' in blank-mode, ripristina lo schermo del mailer e
            // ritorna immediatamente. Qualunque tasto premuto ha il solo effetto di
            // fermare lo screen blanker se e' attivo.
            if (blanked) {
                resume_blanked_screen();
                return;
            }

            if (!(ch & 0xFF)) {
                if (locked && registered && password != NULL && ch != 0x2500) {
                    ch = -1;
                }
            }
            else if (locked && registered && password != NULL && !local_mode) {
                ch &= 0xFF;

                if (ch == password[posit]) {
                    locked = (password[++posit] == '\0') ? 0 : 1;
                    if (!locked) {
                        if (function_active == 4) {
                            f4_status();
                        }
                        else if (!caller) {
                            prints(23, 43, YELLOW | _BLACK, "UNLOCKED");
//							 (23, 43, 23, 50, ' ', YELLOW|_BLACK);
                        }
                    }
                }
                else {
                    posit = 0;
                }

                ch = -1;
            }
            else {
                scanc = (ch & 0xFF00) >> 8;
                ch &= 0xFF;
            }

            if (ch != -1) {
                if (caller) {
                    switch (ch) {

                        // Grey +
                        case '+':
                            if (scanc != 0x4E) {
                                local_kbd = ch;
                                break;
                            }
                            if (usr.priv == TWIT) {
                                usr.priv = DISGRACE;
                            }
                            else if (usr.priv == DISGRACE) {
                                usr.priv = LIMITED;
                            }
                            else if (usr.priv == LIMITED) {
                                usr.priv = NORMAL;
                            }
                            else if (usr.priv == NORMAL) {
                                usr.priv = WORTHY;
                            }
                            else if (usr.priv == WORTHY) {
                                usr.priv = PRIVIL;
                            }
                            else if (usr.priv == PRIVIL) {
                                usr.priv = FAVORED;
                            }
                            else if (usr.priv == FAVORED) {
                                usr.priv = EXTRA;
                            }
                            else if (usr.priv == EXTRA) {
                                usr.priv = CLERK;
                            }
                            else if (usr.priv == CLERK) {
                                usr.priv = ASSTSYSOP;
                            }
                            else if (usr.priv == ASSTSYSOP) {
                                usr.priv = SYSOP;
                            }
                            else if (usr.priv == SYSOP) {
                                usr.priv = TWIT;
                            }
                            if (function_active == 1) {
                                f1_status();
                            }
                            break;

                        // Grey -
                        case '-':
                            if (scanc != 0x4A) {
                                local_kbd = ch;
                                break;
                            }
                            if (usr.priv == TWIT) {
                                usr.priv = SYSOP;
                            }
                            else if (usr.priv == DISGRACE) {
                                usr.priv = TWIT;
                            }
                            else if (usr.priv == LIMITED) {
                                usr.priv = DISGRACE;
                            }
                            else if (usr.priv == NORMAL) {
                                usr.priv = LIMITED;
                            }
                            else if (usr.priv == WORTHY) {
                                usr.priv = NORMAL;
                            }
                            else if (usr.priv == PRIVIL) {
                                usr.priv = WORTHY;
                            }
                            else if (usr.priv == FAVORED) {
                                usr.priv = PRIVIL;
                            }
                            else if (usr.priv == EXTRA) {
                                usr.priv = FAVORED;
                            }
                            else if (usr.priv == CLERK) {
                                usr.priv = EXTRA;
                            }
                            else if (usr.priv == ASSTSYSOP) {
                                usr.priv = CLERK;
                            }
                            else if (usr.priv == SYSOP) {
                                usr.priv = ASSTSYSOP;
                            }
                            if (function_active == 1) {
                                f1_status();
                            }
                            break;

                        // ALT-H Hangup forzato
                        case 0x2300:
                            hidecur();
                            terminating_call();
                            get_down(aftercaller_exit, 2);
                            break;

                        // ALT-S Set security
                        case 0x1F00:
                            set_security();
                            break;

                        // ALT-F Set flags
                        case 0x2100:
                            set_flags();
                            break;

                        // ALT-L Lock-out user
                        case 0x2600:
                            hidecur();
                            usr.priv = 0;
                            terminating_call();
                            get_down(aftercaller_exit, 2);
                            break;

                        // ALT-N Toggle NERD flag
                        case 0x3100:
                            usr.nerd ^= 1;
                            break;

                        // ALT-J DOS Shell
                        case 0x2400:
                            shell_to_dos();
                            break;

                        case 0x3B00:
                            f1_status();
                            break;

                        case 0x3C00:
                            f2_status();
                            break;

                        case 0x3D00:
                            f3_status();
                            break;

                        case 0x3E00:
                            f4_status();
                            break;

                        case 0x4300:
                            f9_status();
                            break;

                        // UpArr Add one minute
                        case 0x4800:
                            if (fulleditor) {
                                local_kbd = ch;
                                break;
                            }
                            allowed += 1;
                            if (function_active == 1) {
                                f1_status();
                            }
                            break;

                        // DnArr Subtract one minute
                        case 0x5000:
                            if (fulleditor) {
                                local_kbd = ch;
                                break;
                            }
                            allowed -= 1;
                            if (function_active == 1) {
                                f1_status();
                            }
                            break;

                        // ALT-F1 - ALT-F10  Visualizza un file .BBS
                        case 0x6800:
                        case 0x6900:
                        case 0x6A00:
                        case 0x6B00:
                        case 0x6C00:
                        case 0x6D00:
                        case 0x6E00:
                        case 0x6F00:
                        case 0x7000:
                        case 0x7100:
                            i = (int)(((unsigned) ch) >> 8);
                            sprintf(cmdname, "ALTF%d", (i - 0x67) * 10);
                            read_system_file(cmdname);
                            break;

                        default:
                            local_kbd = ch;
                            break;
                    }
                }
                else if (ch != -1) {
                    local_kbd = ch;
                }
            }
        }

        if (timeup(clocks)) {
            clocks = timerset(97);

            if (caller && snooping) {
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
                        sprintf(cmdname, "%02ld:%02ld  ", t / 60L, t % 60L);
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
                    else if (function_active == 99) {
                        sc = time_to_next(0);
                        t = time(NULL);
                        tp = localtime(&t);

                        if (old_event != cur_event) {
                            old_event = cur_event;
                            if (!blanked) {
                                prints(9, 65, YELLOW | _BLACK, "              ");

                                if (cur_event >= 0) {
                                    sprintf(cmdname, "%d / %02d:%02d", cur_event + 1, e_ptrs[cur_event]->minute / 60, e_ptrs[cur_event]->minute % 60);
                                    prints(9, 65, YELLOW | _BLACK, cmdname);
                                }
                                else {
                                    prints(9, 65, YELLOW | _BLACK, "None");
                                }

                                prints(7, 65, YELLOW | _BLACK, "              ");

                                if (next_event >= 0) {
                                    tpt.tm_hour = tp->tm_hour + (sc / 60);
                                    tpt.tm_min = tp->tm_min + (sc % 60);
                                    if (tpt.tm_min >= 60) {
                                        tpt.tm_min -= 60;
                                        tpt.tm_hour++;
                                    }

                                    sprintf(cmdname, "%d / %02d:%02d", next_event + 1, tpt.tm_hour % 24, tpt.tm_min % 60);
                                    prints(7, 65, YELLOW | _BLACK, cmdname);
                                }
                                else {
                                    prints(7, 65, YELLOW | _BLACK, "None");
                                }

                                whline(22, 0, 30, 0, LGREY | _BLACK);
                                prints(22, 1, LCYAN | _BLACK, "EVENT: ");
                                sprintf(cmdname, "%s", e_ptrs[cur_event]->cmd);
                                cmdname[22] = 0;
                                if (!cmdname[0]) {
                                    strcpy(cmdname, "N/A");
                                }
                                prints(22, 8, LGREEN | _BLACK, cmdname);

                                if (e_ptrs[cur_event]->behavior & MAT_BBS) {
                                    prints(23, 2, YELLOW | _BLACK, "HUMANS OK");
                                }
                                else {
                                    prints(23, 2, YELLOW | _BLACK, "MAIL ONLY");
                                }

                                if (e_ptrs[cur_event]->behavior & MAT_NOREQ) {
                                    prints(23, 13, YELLOW | _BLACK, "NO FREQ");
                                }
                                else {
                                    prints(23, 13, YELLOW | _BLACK, "FREQ OK");
                                }

                                if (e_ptrs[cur_event]->behavior & MAT_NOOUT) {
                                    prints(23, 22, YELLOW | _BLACK, "NO OUTC.");
                                }
                                else {
                                    prints(23, 22, YELLOW | _BLACK, "CALL OUT");
                                }

                            }
                        }

                        if (cur_event >= 0) {
                            t = sc * 60L - tp->tm_sec;
                            ss = (int)(t % 60);
                            mm = (int)(t / 60) % 60;
                            hh = (int)(t / 3600) % 24;
                            sprintf(cmdname, "%02d:%02d:%02d", hh, mm, ss);
                            prints(8, 65, YELLOW | _BLACK, cmdname);
                        }
                    }
                }
            }
        }
    }

#ifndef __OS2__
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
#else
    DosSleep(5L);
#endif
}

static int execute_events(void)
{
    int i, rc;
    char filename[80];
    struct ffblk blk;

    i = 1;

    if (events == 0L) {
        find_event();

        if (cur_event >= 0) {
            if ((call_list[next_call].type & MAIL_WILLGO) || !(e_ptrs[cur_event]->behavior & MAT_NOOUT)) {
                events = random_time(e_ptrs[cur_event]->wait_time);
            }
        }
        else if (cur_event < 0) {
            events = timerset(500);
        }
    }

    if (timeup(events) && events > 0L) {
        events = 0L;

        if (answer_flag || nopoll) {
            return (1);
        }

        if (next_call < 0) {
            next_call = 0;
        }
        if (next_call >= max_call) {
            next_call = 0;
        }

        for (; next_call < max_call; next_call++) {
            if (!(call_list[next_call].type & MAIL_WILLGO)) {
                if ((e_ptrs[cur_event]->behavior & MAT_NOOUT)) {
                    continue;
                }
                if ((call_list[next_call].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_NOCM)) {
                    continue;
                }
                if (!(call_list[next_call].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_CM)) {
                    continue;
                }
                if (!(call_list[next_call].type & (MAIL_CRASH | MAIL_DIRECT | MAIL_NORMAL))) {
                    continue;
                }
                if (e_ptrs[cur_event]->res_net && (call_list[next_call].zone != e_ptrs[cur_event]->res_zone || call_list[next_call].net != e_ptrs[cur_event]->res_net || call_list[next_call].node != e_ptrs[cur_event]->res_node)) {
                    continue;
                }
                sprintf(filename, "%s%04X%04X.*", HoldAreaNameMunge(call_list[next_call].zone), call_list[next_call].net, call_list[next_call].node);
                rc = 0;
                if (!findfirst(filename, &blk, 0))
                    do {
                        if (blk.ff_name[9] != 'H' && blk.ff_name[10] == 'L' && blk.ff_name[11] == 'O') {
                            rc = 1;
                            break;
                        }
                        if (blk.ff_name[9] != 'H' && blk.ff_name[10] == 'U' && blk.ff_name[11] == 'T') {
                            rc = 1;
                            break;
                        }
                    } while (!findnext(&blk));
                if (!rc) {
                    for (i = next_call + 1; i < max_call; i++) {
                        memcpy(&call_list[i - 1], &call_list[i], sizeof(struct _call_list));
                    }
                    max_call--;
                    if (next_call >= max_call) {
                        next_call = -1;
                    }
                    else {
                        next_call--;
                    }
                    display_outbound_info(outinfo);
                    continue;
                }
            }

            if (bad_call(call_list[next_call].net, call_list[next_call].node, 0, 0)) {
                continue;
            }
            else {
                if (flag_file(TEST_AND_SET, call_list[next_call].zone, call_list[next_call].net, call_list[next_call].node, call_list[next_call].point, 0)) {
                    continue;
                }

                if (blanked) {
                    stop_blanking();
                }

                i = poll(1, 1, call_list[next_call].zone, call_list[next_call].net, call_list[next_call].node);
                flag_file(CLEAR_FLAG, call_list[next_call].zone, call_list[next_call].net, call_list[next_call].node, call_list[next_call].point, 0);

                if (i <= 0) {
                    call_list[next_call].flags = i;
                    i = 0;
                    get_bad_call(call_list[next_call].zone, call_list[next_call].net, call_list[next_call].node, call_list[next_call].point, next_call);
                }
                else {
                    bad_call(call_list[next_call].net, call_list[next_call].node, -2, 0);
                    get_call_list();
                    outinfo = 0;
                }

                // Se la chiamata appena fatta era di tipo immediato, allora cerca
                // nella coda un altro nodo immediato, oppure sempre lo stesso se
                // non ce ne sono altri.
                if (call_list[next_call].type & MAIL_WILLGO) {
                    next_call++;

                    if (next_call >= max_call) {
                        next_call = 0;
                    }

                    for (; next_call < max_call; next_call++) {
                        if (!(call_list[next_call].type & MAIL_WILLGO)) {
                            continue;
                        }

                        if (flag_file(TEST_FLAG, call_list[next_call].zone, call_list[next_call].net, call_list[next_call].node, call_list[next_call].point, 0)) {
                            continue;
                        }

                        if (bad_call(call_list[next_call].net, call_list[next_call].node, 0, 0)) {
                            call_list[next_call].type &= ~MAIL_WILLGO;
                            continue;
                        }
                        else {
                            break;
                        }
                    }

                    if (next_call >= max_call) {
                        for (next_call = 0; next_call < max_call; next_call++) {
                            if (!(call_list[next_call].type & MAIL_WILLGO)) {
                                continue;
                            }

                            if (flag_file(TEST_FLAG, call_list[next_call].zone, call_list[next_call].net, call_list[next_call].node, call_list[next_call].point, 0)) {
                                continue;
                            }

                            if (bad_call(call_list[next_call].net, call_list[next_call].node, 0, 0)) {
                                call_list[next_call].type &= ~MAIL_WILLGO;
                                continue;
                            }
                            else {
                                break;
                            }
                        }
                    }

                    if (next_call >= max_call) {
                        for (next_call = 0; next_call < max_call; next_call++) {
                            if ((e_ptrs[cur_event]->behavior & MAT_NOOUT)) {
                                continue;
                            }
                            if ((call_list[next_call].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_NOCM)) {
                                continue;
                            }
                            if (!(call_list[next_call].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_CM)) {
                                continue;
                            }
                            if (!(call_list[next_call].type & (MAIL_CRASH | MAIL_DIRECT | MAIL_NORMAL))) {
                                continue;
                            }
                            if (e_ptrs[cur_event]->res_net && (call_list[next_call].zone != e_ptrs[cur_event]->res_zone || call_list[next_call].net != e_ptrs[cur_event]->res_net || call_list[next_call].node != e_ptrs[cur_event]->res_node)) {
                                continue;
                            }

                            if (flag_file(TEST_FLAG, config->alias[0].zone, call_list[next_call].net, call_list[next_call].node, call_list[next_call].point, 0)) {
                                continue;
                            }

                            if (bad_call(call_list[next_call].net, call_list[next_call].node, 0, 0)) {
                                continue;
                            }
                            else {
                                break;
                            }
                        }

                        if (next_call >= max_call) {
                            next_call = -1;
                        }
                    }
                }
                else {
                    next_call++;

                    if (next_call >= max_call) {
                        next_call = 0;
                    }

                    for (; next_call < max_call; next_call++) {
                        if (!(call_list[next_call].type & MAIL_WILLGO)) {
                            if ((e_ptrs[cur_event]->behavior & MAT_NOOUT)) {
                                continue;
                            }
                            if ((call_list[next_call].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_NOCM)) {
                                continue;
                            }
                            if (!(call_list[next_call].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_CM)) {
                                continue;
                            }
                            if (!(call_list[next_call].type & (MAIL_CRASH | MAIL_DIRECT | MAIL_NORMAL))) {
                                continue;
                            }
                            if (e_ptrs[cur_event]->res_net && (call_list[next_call].zone != e_ptrs[cur_event]->res_zone || call_list[next_call].net != e_ptrs[cur_event]->res_net || call_list[next_call].node != e_ptrs[cur_event]->res_node)) {
                                continue;
                            }
                        }

                        if (flag_file(TEST_FLAG, config->alias[0].zone, call_list[next_call].net, call_list[next_call].node, call_list[next_call].point, 0)) {
                            continue;
                        }

                        if (bad_call(call_list[next_call].net, call_list[next_call].node, 0, 0)) {
                            continue;
                        }
                        else {
                            break;
                        }
                    }

                    if (next_call >= max_call) {
                        for (next_call = 0; next_call < max_call; next_call++) {
                            if (!(call_list[next_call].type & MAIL_WILLGO)) {
                                if ((e_ptrs[cur_event]->behavior & MAT_NOOUT)) {
                                    continue;
                                }
                                if ((call_list[next_call].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_NOCM)) {
                                    continue;
                                }
                                if (!(call_list[next_call].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_CM)) {
                                    continue;
                                }
                                if (!(call_list[next_call].type & (MAIL_CRASH | MAIL_DIRECT | MAIL_NORMAL))) {
                                    continue;
                                }
                                if (e_ptrs[cur_event]->res_net && (call_list[next_call].zone != e_ptrs[cur_event]->res_zone || call_list[next_call].net != e_ptrs[cur_event]->res_net || call_list[next_call].node != e_ptrs[cur_event]->res_node)) {
                                    continue;
                                }
                            }

                            if (flag_file(TEST_FLAG, config->alias[0].zone, call_list[next_call].net, call_list[next_call].node, call_list[next_call].point, 0)) {
                                continue;
                            }

                            if (bad_call(call_list[next_call].net, call_list[next_call].node, 0, 0)) {
                                continue;
                            }
                            else {
                                break;
                            }
                        }

                        if (next_call >= max_call) {
                            next_call = -1;
                        }
                    }
                }

                display_outbound_info(outinfo);
                break;
            }
        }
    }

//   if (!called) {
//      if (cur_event > -1 && (e_ptrs[cur_event]->behavior & MAT_DYNAM)) {
//         e_ptrs[cur_event]->behavior |= MAT_SKIP;
//         write_sched ();
//      }
//   }

    if (!i) {
        old_event = -1;
        events = 0L;
        clocks = 0L;
        blankto = timerset(0);
        blankto += blank_timer * 6000L;
    }

    return (i);
}

/*
static int execute_events (void)
{
   int i, m, rc;
   char filename[80], called = 0;
   struct ffblk blk;

   i = 1;

   if (events == 0L) {
      find_event ();

      if (cur_event >= 0) {
         if ( (call_list[next_call].type & MAIL_WILLGO) || !(e_ptrs[cur_event]->behavior & MAT_NOOUT))
            events = random_time (e_ptrs[cur_event]->wait_time);
      }
      else if (cur_event < 0)
         events = timerset (500);
   }

   if (timeup (events) && events > 0L) {
      events = 0L;

      if (answer_flag)
         return (1);

      if (next_call >= max_call)
         next_call = 0;

      for (;next_call < max_call; next_call++) {
         if (!(call_list[next_call].type & MAIL_WILLGO)) {
            if ((call_list[next_call].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_NOCM))
               continue;
            if (!(call_list[next_call].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_CM))
               continue;
            if (!(call_list[next_call].type & (MAIL_CRASH|MAIL_DIRECT|MAIL_NORMAL)))
               continue;
            if (e_ptrs[cur_event]->res_net && (call_list[next_call].net != e_ptrs[cur_event]->res_net ||
                                               call_list[next_call].node != e_ptrs[cur_event]->res_node))
               continue;
            sprintf (filename, "%s%04X%04X.*", HoldAreaNameMunge (call_list[next_call].zone), call_list[next_call].net, call_list[next_call].node);
            rc = 0;
            if (!findfirst (filename, &blk, 0))
               do {
                  if (blk.ff_name[9] != 'H' && blk.ff_name[10] == 'L' && blk.ff_name[11] == 'O') {
                     rc = 1;
                     break;
                  }
                  if (blk.ff_name[9] != 'H' && blk.ff_name[10] == 'U' && blk.ff_name[11] == 'T') {
                     rc = 1;
                     break;
                  }
               } while (!findnext (&blk));
            if (!rc) {
               for (i = next_call + 1; i < max_call; i++)
                  memcpy (&call_list[i - 1], &call_list[i], sizeof (struct _call_list));
               max_call--;
               if (next_call >= max_call)
                  next_call = -1;
               else
                  next_call--;
               display_outbound_info (outinfo);
               continue;
            }
         }

         if (bad_call(call_list[next_call].net,call_list[next_call].node,0,0))
            continue;
         else {
            if (flag_file (TEST_AND_SET, call_list[next_call].zone, call_list[next_call].net, call_list[next_call].node, call_list[next_call].point, 0))
               continue;

            if (blanked)
               stop_blanking ();

            called = 1;
            i = poll(1, 1, call_list[next_call].zone, call_list[next_call].net, call_list[next_call].node);

            flag_file (CLEAR_FLAG, call_list[next_call].zone, call_list[next_call].net, call_list[next_call].node, call_list[next_call].point, 0);
            if (!(call_list[next_call].type & MAIL_WILLGO))
               m = next_call + 1;
            else
               m = next_call;

            if (i <= 0) {
               call_list[next_call].flags = i;
               i = 0;
               get_bad_call (call_list[next_call].zone, call_list[next_call].net, call_list[next_call].node, call_list[next_call].point, next_call);
            }
            else {
               bad_call (call_list[next_call].net, call_list[next_call].node, -2, 0);
               get_call_list ();
               outinfo = 0;
            }

            next_call = m;
            if (next_call >= max_call)
               next_call = 0;

            for (;next_call < max_call; next_call++) {
               if (!(call_list[next_call].type & MAIL_WILLGO)) {
                  if ((call_list[next_call].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_NOCM))
                     continue;
                  if (!(call_list[next_call].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_CM))
                     continue;
                  if (!(call_list[next_call].type & (MAIL_CRASH|MAIL_DIRECT|MAIL_NORMAL)))
                     continue;
                  if (e_ptrs[cur_event]->res_net && (call_list[next_call].net != e_ptrs[cur_event]->res_net || call_list[next_call].node != e_ptrs[cur_event]->res_node))
                     continue;
               }

               if (flag_file (TEST_FLAG, config->alias[0].zone, call_list[next_call].net, call_list[next_call].node, call_list[next_call].point, 0))
                  continue;

               if (bad_call(call_list[next_call].net,call_list[next_call].node,0,0))
                  continue;
               else
                  break;
            }

            if (next_call >= max_call) {
               for (next_call = 0; next_call < max_call; next_call++) {
                  if (!(call_list[next_call].type & MAIL_WILLGO)) {
                     if ((call_list[next_call].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_NOCM))
                        continue;
                     if (!(call_list[next_call].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_CM))
                        continue;
                     if (!(call_list[next_call].type & (MAIL_CRASH|MAIL_DIRECT|MAIL_NORMAL)))
                        continue;
                     if (e_ptrs[cur_event]->res_net && (call_list[next_call].net != e_ptrs[cur_event]->res_net || call_list[next_call].node != e_ptrs[cur_event]->res_node))
                        continue;
                  }

                  if (flag_file (TEST_FLAG, config->alias[0].zone, call_list[next_call].net, call_list[next_call].node, call_list[next_call].point, 0))
                     continue;

                  if (bad_call(call_list[next_call].net,call_list[next_call].node,0,0))
                     continue;
                  else
                     break;
               }

               if (next_call >= max_call)
                  next_call = 0;
            }

            display_outbound_info (outinfo);
            break;
         }
      }
   }

//   if (!called) {
//      if (cur_event > -1 && (e_ptrs[cur_event]->behavior & MAT_DYNAM)) {
//         e_ptrs[cur_event]->behavior |= MAT_SKIP;
//         write_sched ();
//      }
//   }

   if (!i) {
      old_event = -1;
      events = 0L;
		clocks = 0L;
      blankto = timerset (0);
      blankto += blank_timer * 6000L;
   }

   return (i);
}
*/

// []----------------------------------------------------------------------[]
//    Screen blanker
// []----------------------------------------------------------------------[]
static int * scrsv;

static void begin_blanking()
{
    scrsv = ssave();
    wtextattr(LGREY | _BLACK);
    cclrscrn(LGREY | _BLACK);

    switch (config->blanker_type) {
        case 1:
            star_screen_blanker(1);
            break;
        case 2:
            snake_screen_blanker(1);
            break;
    }

    blanked = 1;

#ifdef __OS2__
    VioUpdate();
#endif
}

void stop_blanking(void)
{
    if (blanked) {
        switch (config->blanker_type) {
            case 1:
                star_screen_blanker(0);
                break;
            case 2:
                snake_screen_blanker(0);
                break;
        }

        if (scrsv != NULL) {
            srestore(scrsv);
        }

        blanked = 0;
        blankto = timerset(0);
        blankto += blank_timer * 6000L;

#ifdef __OS2__
        VioUpdate();
#endif
    }
}

static void blank_progress()
{
    if (!blanked) {
        return;
    }

    switch (config->blanker_type) {
        case 1:
            star_screen_blanker(0);
            break;
        case 2:
            snake_screen_blanker(0);
            break;
    }

#ifdef __OS2__
    VioUpdate();
#endif
}

struct _star {
    char x;
    char y;
    char magnitude;
};

#define MAX_STAR  50

static void star_screen_blanker(int init)
{
    static struct _star * star;
    int i, m, count;
    unsigned char c;

    // Inizializzazione dello screen blanker
    if (init == 1) {
        randomize();

        star = (struct _star *)malloc(sizeof(struct _star) * MAX_STAR);

        for (i = 0; i < MAX_STAR; i++) {
            do {
                star[i].x = random(80);
                star[i].y = random(25);
                for (m = 0; m < i; m++)
                    if (star[i].x == star[m].x && star[i].y == star[m].y) {
                        break;
                    }
            } while (m < i);
            star[i].magnitude = 0;
            printc(star[i].y, star[i].x, LCYAN | _BLACK, 249);
        }
    }
    // Eliminazione dello screen blanker
    else if (init == 2) {
        free(star);
    }
    // Se lo screen blanker deve fare qualcosa, con init == 0 puo' essere
    // fatto.
    else if (init == 0) {
        count = 0;

        for (i = 0; i < MAX_STAR; i++) {
            if (star[i].magnitude > 0) {
                switch (star[i].magnitude) {
                    case 0:
                        c = 249;
                        break;
                    case 1:
                        c = 7;
                        break;
                    case 2:
                        c = 4;
                        break;
                    case 3:
                        c = 42;
                        break;
                    case 4:
                        c = 15;
                        break;
                }
                if (c == 249) {
                    printc(star[i].y, star[i].x, LCYAN | _BLACK, c);
                }
                else {
                    printc(star[i].y, star[i].x, WHITE | _BLACK, c);
                }
                star[i].magnitude++;
                if (star[i].magnitude >= 5) {
                    printc(star[i].y, star[i].x, WHITE | _BLACK, ' ');
                    do {
                        star[i].x = random(80);
                        star[i].y = random(25);
                        for (m = 0; m < i; m++)
                            if (star[i].x == star[m].x && star[i].y == star[m].y) {
                                break;
                            }
                    } while (m < i);
                    star[i].magnitude = 0;
                    printc(star[i].y, star[i].x, LCYAN | _BLACK, 249);
                }

                count++;
            }
        }

        for (i = count; i < 4; i++) {
            i = random(MAX_STAR);
            star[i].magnitude++;
        }
    }
}

#define MAX_ELEMENTS   20

static void snake_screen_blanker(int init)
{
    static int x[3], y[3], dir[3], oldx[3], oldy[3];
    static int elemx[3][MAX_ELEMENTS], elemy[3][MAX_ELEMENTS];
    int i, m;

    // Inizializzazione dello screen blanker
    if (init == 1) {
        randomize();

        dir[0] = random(4);
        do {
            dir[1] = random(4);
        } while (dir[1] == dir[0]);
        do {
            dir[2] = random(4);
        } while (dir[2] == dir[0] || dir[2] == dir[1]);

        for (m = 0; m < 3; m++) {
            for (i = 0; i < MAX_ELEMENTS; i++) {
                elemx[m][i] = elemy[m][i] = 0;
            }
        }

        elemx[0][0] = x[0] = 40;
        elemy[0][0] = y[0] = 12;
        elemx[1][0] = x[1] = 20;
        elemy[1][0] = y[1] = 12;
        elemx[2][0] = x[2] = 60;
        elemy[2][0] = y[2] = 12;
        prints(y[0] - 1, x[0] - 1, LGREEN | _BLACK, "\333\333");
        prints(y[1] - 1, x[1] - 1, LRED | _BLACK, "\333\333");
        prints(y[2] - 1, x[2] - 1, BLUE | _BLACK, "\333\333");
    }
    // Eliminazione dello screen blanker
    else if (init == 2) {
        return;
    }
    // Se lo screen blanker deve fare qualcosa, con init == 0 puo' essere
    // fatto.
    else if (init == 0) {
        for (m = 0; m < 3; m++) {
            switch (dir[m]) {
                case 0:
                    y[m]--;
                    x[m]++;
                    break;
                case 1:
                    x[m]++;
                    y[m]++;
                    break;
                case 2:
                    y[m]++;
                    x[m]--;
                    break;
                case 3:
                    x[m]--;
                    y[m]--;
                    break;
            }

            oldx[m] = elemx[m][MAX_ELEMENTS - 1];
            oldy[m] = elemy[m][MAX_ELEMENTS - 1];

            for (i = MAX_ELEMENTS - 1; i > 0; i--) {
                elemx[m][i] = elemx[m][i - 1];
                elemy[m][i] = elemy[m][i - 1];
            }

            elemx[m][0] = x[m];
            elemy[m][0] = y[m];

            if (oldx[m]) {
                prints(oldy[m] - 1, oldx[m] - 1, WHITE | _BLACK, "  ");
            }

            for (i = 0; i < MAX_ELEMENTS; i++) {
                if (elemx[m][i] == 0) {
                    break;
                }

                if (i == 0) {
                    if (m == 0) {
                        prints(elemy[m][i] - 1, elemx[m][i] - 1, LGREEN | _BLACK, "\333\333");
                    }
                    else if (m == 1) {
                        prints(elemy[m][i] - 1, elemx[m][i] - 1, LRED | _BLACK, "\333\333");
                    }
                    else {
                        prints(elemy[m][i] - 1, elemx[m][i] - 1, BLUE | _BLACK, "\333\333");
                    }
                }
                else if (i < MAX_ELEMENTS / 3) {
                    if (m == 0) {
                        prints(elemy[m][i] - 1, elemx[m][i] - 1, LGREEN | _BLACK, "\262\262");
                    }
                    else if (m == 1) {
                        prints(elemy[m][i] - 1, elemx[m][i] - 1, LRED | _BLACK, "\262\262");
                    }
                    else {
                        prints(elemy[m][i] - 1, elemx[m][i] - 1, BLUE | _BLACK, "\262\262");
                    }
                }
                else if (i < (MAX_ELEMENTS / 3) * 2) {
                    if (m == 0) {
                        prints(elemy[m][i] - 1, elemx[m][i] - 1, LGREEN | _BLACK, "\261\261");
                    }
                    else if (m == 1) {
                        prints(elemy[m][i] - 1, elemx[m][i] - 1, LRED | _BLACK, "\261\261");
                    }
                    else {
                        prints(elemy[m][i] - 1, elemx[m][i] - 1, BLUE | _BLACK, "\261\261");
                    }
                }
                else {
                    if (m == 0) {
                        prints(elemy[m][i] - 1, elemx[m][i] - 1, LGREEN | _BLACK, "\260\260");
                    }
                    else if (m == 1) {
                        prints(elemy[m][i] - 1, elemx[m][i] - 1, LRED | _BLACK, "\260\260");
                    }
                    else {
                        prints(elemy[m][i] - 1, elemx[m][i] - 1, BLUE | _BLACK, "\260\260");
                    }
                }
            }

            if (x[m] == 1 && (dir[m] == 2 || dir[m] == 3)) {
                if (y[m] == 1) {
                    dir[m] = 1;
                }
                else if (y[m] == 24) {
                    dir[m] = 0;
                }
                else {
                    dir[m] = (dir[m] == 3) ? 0 : 1;
                }
            }
            else if (x[m] == 79 && (dir[m] == 0 || dir[m] == 1)) {
                if (y[m] == 1) {
                    dir[m] = 2;
                }
                else if (y[m] == 24) {
                    dir[m] = 3;
                }
                else {
                    dir[m] = (dir[m] == 0) ? 3 : 2;
                }
            }
            else if (y[m] == 1 && (dir[m] == 0 || dir[m] == 3)) {
                dir[m] = (dir[m] == 0) ? 1 : 2;
            }
            else if (y[m] == 24 && (dir[m] == 2 || dir[m] == 1)) {
                dir[m] = (dir[m] == 2) ? 3 : 0;
            }
        }
    }
}

