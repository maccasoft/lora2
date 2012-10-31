#include <stdio.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <process.h>
#include <stdlib.h>
#include <signal.h>

#include <cxl\cxlvid.h>

#include "defines.h"
#include "lora.h"
#include "sched.h"

extern unsigned int _stklen = 20480U;

struct _menu_header {
        char name[14];
        int  n_elem;
};

struct parse_list levels[] = {
        TWIT, "Twit",
        DISGRACE, "Disgrace",
        LIMITED, "Limited",
        NORMAL, "Normal",
        WORTHY, "Worthy",
        PRIVIL, "Privel",
        FAVORED, "Favored",
        EXTRA, "Extra",
        CLERK, "Clerk",
        ASSTSYSOP, "Asstsysop",
        SYSOP, "Sysop",
        HIDDEN, "Hidden"
};

struct parse_list days[] = {
        DAY_WEEK|DAY_WKEND, "ALL",
        DAY_SUNDAY, "SUN",
        DAY_MONDAY, "MON",
        DAY_TUESDAY, "TUE",
        DAY_WEDNESDAY, "WED",
        DAY_THURSDAY, "THU",
        DAY_FRIDAY, "FRI",
        DAY_SATURDAY, "SAT",
        DAY_WEEK|DAY_WKEND, "TUTTI",
        DAY_SUNDAY, "DOM",
        DAY_MONDAY, "LUN",
        DAY_TUESDAY, "MAR",
        DAY_WEDNESDAY, "MER",
        DAY_THURSDAY, "GIO",
        DAY_FRIDAY, "VEN",
        DAY_SATURDAY, "SAB",
        DAY_WKEND,"WEEKEND",
        DAY_WEEK,"WEEKDAYS",
        0, NULL
};

static char *BinkSched = "LoraScheduler01";      /* Version of scheduler   */
static char *schedpath, *systempath, *menupath;

int get_day(char *);
int get_priv(char *);
long get_flags (char *);
char *replace_blank(char *);
void process_language (char *, char *);
void append_backslash (char *);
void strip_backslash (char *);
void system_section (void);
void zero_dupes (char *);

struct _sys_idx sysidx;
char idxmkey[13], idxfkey[13];


        FILE *fp, *fpt;
        char linea[128], opt[3][60], filename[90], *cfgname = "LORA.CFG", *p;
        int fd, line, def_menu, def_system, def_sched, v;
        int def_first, def_color, t1, t2, fs;
        int fdmd, fdmi, fdfd, fdfi;
        long prev_head;
        struct _sys sys;
        EVENT sched;
        struct _cmd cmd;
        struct _menu_header m_header;

void main(argc, argv)
int argc;
char *argv[];
{
        fpt = NULL;
        def_first = YELLOW|_BLACK;
        def_color = CYAN|_BLACK;
        systempath = NULL;
        schedpath = NULL;
        menupath = NULL;
        fs = 1;
        prev_head = 0L;
        fd = -1;
        fdmd = fdmi = fdfd = fdfi = -1;

        printf("\nLORACOMP; LoraBBS System File Compiler Version 2.20\n");
        printf("          CopyRight (c) 1991-92 by Marco Maccaferri. All Rights Reserved.\n\n");

        if (argc < 2) {
                printf("Usage: LORACOMP [CfgFile] [-Z<tag>] [-E] [-S] [-M]\n\n");
                printf("Where:\n");
                printf("       CfgFile   - Optional .CFG file to read system information from\n");
                printf("                   Defaults to LORA.CFG\n");
                printf("       -Z<tag>   - Reset dupe checking for area <tag>\n");
                printf("       -E        - Compile the Event Schedule Definitions\n");
                printf("       -S        - Compile the Message/File Areas Definitions\n");
                printf("       -M        - Compile the Menu/Language Structure Defintions\n\n");
                printf("       -E -S -M  - Compile all Defintions\n\n");
                exit (0);
        }

        for (v = 1; v < argc; v++) {
                argv[v][1] = tolower(argv[v][1]);
                if (argv[v][0] != '-') {
                        cfgname = argv[v];
                        continue;
                }

                if (argv[v][1] == 'z') {
                        printf("Reset dupe checking for area %s\n", &argv[v][2]);
                        zero_dupes (&argv[v][2]);
                }

                if (argv[v][1] == 'e')
                        printf("Compile the Event Schedule Definitions\n");
                if (argv[v][1] == 's')
                        printf("Compile the Message/File Areas Definitions\n");

                fp = fopen(cfgname,"rt");
                if (fp == NULL) {
                        printf ("Couldn't read %s file\n", strupr(cfgname));
                        exit (1);
                }

                line = 0;
                def_menu = 0;
                def_system = 0;
                def_sched = 0;

                if (argv[v][1] == 'e')
                        memset((char *)&sched,0,sizeof(EVENT));

continue_scan:
                while (fgets(linea, 255, fp) != NULL) {
                        ++line;

                        linea[strlen(linea)-1] = '\0';
                        if (!strlen(linea) || linea[0] == ';')
                                continue;

                        sscanf(linea,"%s %s %s",opt[0], opt[1], opt[2]);

                        if (!stricmp(opt[0],"INCLUDE"))
                        {
                                fpt = fp;
                                sprintf (filename, "%s.CFG", opt[1]);
                                fp = fopen(filename,"rt");
                                line = 0;
                                continue;
                        }

                        if (!stricmp(opt[0],"MENU_PATH"))
                        {
                                append_backslash (opt[1]);
                                menupath = (char *)malloc(strlen(opt[1])+1);
                                strcpy(menupath, opt[1]);
                                continue;
                        }

                        if (!stricmp(opt[0],"SCHED_NAME"))
                        {
                                schedpath = (char *)malloc(strlen(opt[1])+1);
                                strcpy(schedpath, opt[1]);
                                if (fd != -1)
                                        close(fd);
                                if (argv[v][1] == 'e') {
                                        memset((char *)&sched,0,sizeof(EVENT));
                                        fd = open(schedpath,O_WRONLY|O_BINARY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
                                        write(fd, BinkSched, 16);
                                        close(fd);
                                        fd = -1;
                                }
                                continue;
                        }

                        if (!stricmp(opt[0],"SYSTEM_PATH"))
                        {
                                append_backslash (opt[1]);
                                systempath = (char *)malloc(strlen(opt[1])+1);
                                strcpy(systempath, opt[1]);
                                continue;
                        }

                        if (!stricmp(opt[0],"BEGIN_SCHEDULE") && argv[v][1] == 'e') {
                                if (def_sched)
                                        printf("Misplaced BEGIN_SCHEDULE in line %d\n", line);

                                if (def_system || def_menu)
                                        continue;
                                def_sched = atoi(opt[1]);
                                memset((char *)&sched,0,sizeof(EVENT));
                                continue;
                        }

                        if (!stricmp(opt[0],"LANGUAGE") && argv[v][1] == 'm') {
                                if (fd != -1)
                                        close(fd);

                                process_language (opt[1], menupath);

                                fpt = fp;
                                sprintf (filename, "%s%s.CFG", menupath, opt[1]);
                                fp = fopen(filename,"rt");
                                if (fp != NULL)
                                {
                                        if (fd != -1)
                                                close(fd);
                                        sprintf (filename, "%s%s.MNU", menupath, opt[1]);
                                        fd = open(filename,O_WRONLY|O_BINARY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
                                        memset((char *)&m_header,0,sizeof(struct _menu_header));
                                        prev_head = tell(fd);
                                }
                                continue;
                        }

                        if (!stricmp(opt[0],"BEGIN_MENU") && argv[v][1] == 'm')
                        {
                                if (def_menu)
                                        printf("Misplaced BEGIN_MENU in line %d\n", line);

                                if (def_system || def_sched)
                                        continue;
                                def_menu = 1;
                                strcpy (m_header.name, opt[1]);
                                write(fd, (char *)&m_header, sizeof(struct _menu_header));
                                memset((char *)&cmd,0,sizeof(struct _cmd));
                                cmd.color = def_color;
                                cmd.first_color = def_first;
                                continue;
                        }

                        if (!stricmp(opt[0],"BEGIN_SYSTEM") && argv[v][1] == 's') {
                                if (def_system || def_sched || def_menu)
                                {
                                        printf("Misplaced BEGIN_SYSTEM in line %d\n", line);
                                        continue;
                                }

                                if (fs)
                                {
                                        if (fd != -1)
                                                close(fd);

                                        memset((char *)&sysidx,0,sizeof(struct _sys_idx));
                                        idxmkey[0] = idxfkey[0] = '\0';

                                        sprintf(filename, "%sSYSMSG.DAT", systempath);
                                        fdmd = open(filename,O_WRONLY|O_BINARY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);

                                        sprintf(filename, "%sSYSMSG.IDX", systempath);
                                        fdmi = open(filename,O_WRONLY|O_BINARY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);

                                        sprintf(filename, "%sSYSFILE.DAT", systempath);
                                        fdfd = open(filename,O_WRONLY|O_BINARY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);

                                        sprintf(filename, "%sSYSFILE.IDX", systempath);
                                        fdfi = open(filename,O_WRONLY|O_BINARY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);

                                        fd = -1;
                                        fs = 0;
                                }

                                def_system = atoi(opt[1]);
                                memset((char *)&sys,0,sizeof(struct _sys));
                                continue;
                        }

                        if (def_sched) {
                                if (!stricmp(opt[0],"WEEKDAY")) {
                                        sched.days = get_day(opt[1]);
                                        continue;
                                }
                                if (!stricmp(opt[0],"TIME_DURATION")) {
                                        sscanf(opt[1], "%d:%d",&t1,&t2);
                                        sched.minute = t1 * 60 + t2;
                                        sscanf(opt[2], "%d:%d",&t1,&t2);
                                        sched.length = (t1 * 60 + t2) - sched.minute + 1;
                                        if (sched.length < 0)
                                                sched.length = 1;
                                        continue;
                                }
                                if (!stricmp(opt[0],"RESERVED_FOR")) {
                                        sscanf(opt[1], "%d:%d/%d",&sched.res_zone,&sched.res_net,&sched.res_node);
                                        sched.behavior |= MAT_RESERV;
                                        continue;
                                }
                                if (!stricmp(opt[0],"EXPORT_MAIL")) {
                                        sched.echomail |= ECHO_EXPORT;
                                        continue;
                                }
                                if (!stricmp(opt[0],"START_EXPORT")) {
                                        sched.echomail |= ECHO_STARTEXPORT;
                                        continue;
                                }
                                if (!stricmp(opt[0],"IMPORT_MAIL")) {
                                        sched.echomail |= ECHO_IMPORT;
                                        continue;
                                }
                                if (!stricmp(opt[0],"START_IMPORT")) {
                                        sched.echomail |= ECHO_STARTIMPORT;
                                        continue;
                                }
                                if (!stricmp(opt[0],"IMPORT_KNOW_MAIL")) {
                                        sched.echomail |= ECHO_KNOW;
                                        continue;
                                }
                                if (!stricmp(opt[0],"IMPORT_PROT_MAIL")) {
                                        sched.echomail |= ECHO_PROT;
                                        continue;
                                }
                                if (!stricmp(opt[0],"IMPORT_NORM_MAIL")) {
                                        sched.echomail |= ECHO_NORMAL;
                                        continue;
                                }
                                if (!stricmp(opt[0],"START_EXIT")) {
                                        sched.errlevel[0] = atoi(opt[1]);
                                        continue;
                                }
                                if (!stricmp(opt[0],"AFTERMAIL_EXIT")) {
                                        sched.errlevel[1] = atoi(opt[1]);
                                        continue;
                                }
                                if (!stricmp(opt[0],"MAX_NOCONNECTS")) {
                                        sched.no_connect = atoi(opt[1]);
                                        continue;
                                }
                                if (!stricmp(opt[0],"MAX_CONNECTS")) {
                                        sched.with_connect = atoi(opt[1]);
                                        continue;
                                }
                                if (!stricmp(opt[0],"WAIT_TIME")) {
                                        sched.wait_time = atoi(opt[1]);
                                        continue;
                                }
                                if (!stricmp(opt[0],"CRASHMAIL_ONLY")) {
                                        sched.behavior |= MAT_CM;
                                        continue;
                                }
                                if (!stricmp(opt[0],"RESYNC_CLOCK")) {
                                        sched.behavior |= MAT_RESYNC;
                                        continue;
                                }
//                                if (!stricmp(opt[0],"DYNAMIC"))
//                                       sched.behavior |= MAT_DYNAM;
                                if (!stricmp(opt[0],"BBS_ALLOWED")) {
                                        sched.behavior |= MAT_BBS;
                                        continue;
                                }
                                if (!stricmp(opt[0],"NO_REQUESTS")) {
                                        sched.behavior |= MAT_NOREQ;
                                        continue;
                                }
                                if (!stricmp(opt[0],"NO_INBOUND_CRASHMAIL")) {
                                        sched.behavior |= MAT_OUTONLY;
                                        continue;
                                }
                                if (!stricmp(opt[0],"RECEIVE_ONLY")) {
                                        sched.behavior |= MAT_NOOUT;
                                        continue;
                                }
                                if (!stricmp(opt[0],"FORCED")) {
                                        sched.behavior |= MAT_FORCED;
                                        continue;
                                }
                                if (!stricmp(opt[0],"NO_OUT_REQUESTS")) {
                                        sched.behavior |= MAT_NOOUTREQ;
                                        continue;
                                }
                                if (!stricmp(opt[0],"NO_OUT_CRASHMAIL")) {
                                        sched.behavior |= MAT_NOCM;
                                        continue;
                                }
                                if (!stricmp(opt[0],"ROUTE_TAG")) {
                                        strcpy (sched.route_tag, opt[1]);
                                        continue;
                                }
                                if (!stricmp(opt[0],"END_SCHEDULE")) {
                                        if (!def_sched)
                                                continue;
                                        if (fd != -1)
                                                close(fd);
                                        fd = open(schedpath,O_WRONLY|O_BINARY|O_APPEND);
                                        write(fd,(char *)&sched,sizeof(EVENT));
                                        close(fd);
                                        fd = -1;
                                        def_sched = 0;
                                        continue;
                                }

                                printf("Configuration error. Line %d. (%s)\n", line, opt[0]);
                        }

                        if (def_menu) {
                                if (!stricmp(opt[0],"ITEM_COLORS")) {
                                        cmd.first_color = atoi(opt[1]);
                                        cmd.color = atoi(opt[2]);
                                        continue;
                                }
                                if (!stricmp(opt[0],"DEFAULT_COLORS")) {
                                        def_first = cmd.first_color = atoi(opt[1]);
                                        def_color = cmd.color = atoi(opt[2]);
                                        continue;
                                }
                                if (!stricmp(opt[0],"DISPLAY")) {
                                        p = strstr (linea, opt[0]) + strlen (opt[0]);
                                        p = strstr (p, opt[1]);
                                        strcpy(cmd.name, replace_blank(p));
                                        continue;
                                }
                                if (!stricmp(opt[0],"OPTION_PRIV")) {
                                        cmd.priv = get_priv(opt[1]);
                                        continue;
                                }
                                if (!stricmp(opt[0],"OPTION_FLAGS")) {
                                        cmd.flags = get_flags(opt[1]);
                                        continue;
                                }
                                if (!stricmp(opt[0],"HOT_KEY")) {
                                        cmd.hotkey = toupper(opt[1][0]);
                                        continue;
                                }
                                if (!stricmp(opt[0],"MENU_TYPE")) {
                                        cmd.flag_type = atoi(opt[1]);
                                        continue;
                                }
                                if (!stricmp(opt[0],"AUTOMATIC")) {
                                        cmd.automatic = 1;
                                        continue;
                                }
                                if (!stricmp(opt[0],"NO_CLEAR")) {
                                        cmd.nocls = 1;
                                        continue;
                                }
                                if (!stricmp(opt[0],"FIRST_TIME")) {
                                        cmd.first_time = 1;
                                        continue;
                                }
                                if (!stricmp(opt[0],"ARGUMENTS")) {
                                        strcpy(cmd.argument, replace_blank(opt[1]));
                                        continue;
                                }
                                if (!stricmp(opt[0],"END_ITEM")) {
                                        if (!def_menu)
                                                continue;
                                        write(fd,(char *)&cmd,sizeof(struct _cmd));
                                        memset((char *)&cmd,0,sizeof(struct _cmd));
                                        cmd.color = def_color;
                                        cmd.first_color = def_first;
                                        m_header.n_elem++;
                                        continue;
                                }
                                if (!stricmp(opt[0],"END_MENU")) {
                                        if (!def_menu)
                                                continue;
                                        def_menu = 0;
                                        strcpy(filename,"");
                                        lseek(fd, prev_head, SEEK_SET);
                                        write(fd, (char *)&m_header, sizeof(struct _menu_header));
                                        lseek(fd, 0L, SEEK_END);
                                        prev_head = tell(fd);
                                        memset((char *)&m_header, 0, sizeof(struct _menu_header));
                                        continue;
                                }

                                printf("Configuration error. Line %d. (%s)\n", line, opt[0]);
                        }

                        if (def_system) {
                                if (!stricmp(opt[0],"MESSAGE_KEYWORD")) {
                                        strcpy(idxmkey, replace_blank(opt[1]));
                                        continue;
                                }
                                if (!stricmp(opt[0],"MESSAGE_PATH"))
                                {
                                        if (!sys.squish)
                                                append_backslash (opt[1]);
                                        else
                                                strip_backslash (opt[1]);
                                        strcpy(sys.msg_path, opt[1]);
                                        continue;
                                }
                                if (!stricmp(opt[0],"ORIGIN_LINE")) {
                                        strcpy(sys.origin, replace_blank(opt[1]));
                                        continue;
                                }
                                if (!stricmp(opt[0],"MAX_MESSAGES")) {
                                        sys.max_msgs = atoi (opt[1]);
                                        continue;
                                }
                                if (!stricmp(opt[0],"MAX_AGE")) {
                                        sys.max_age = atoi (opt[1]);
                                        continue;
                                }
                                if (!stricmp(opt[0],"AGE_RECEIVED"))
                                        sys.age_rcvd = atoi (opt[1]);
                                else
                                        system_section ();
                        }
                }

                if (fdmd == -1)
                {
                        close(fdmd);
                        close(fdmi);
                        close(fdfd);
                        close(fdfi);
                }

                if (fd != -1)
                        close(fd);

                fclose(fp);

                if (fpt != NULL)
                {
                        fp = fpt;
                        fpt = NULL;

                        if (def_sched)
                                printf("Scheduler definitions NOT ended\n");
                        if (def_system)
                                printf("Message/File definitions NOT ended\n");
                        if (def_menu)
                                printf("Menu/Language definitions NOT ended\n");

                        goto continue_scan;
                }
        }
}

void system_section ()
{
                                if (!stricmp(opt[0],"MESSAGE_NAME")) {
                                        strcpy(sys.msg_name, replace_blank(opt[1]));
                                        return;
                                }
                                if (!stricmp(opt[0],"MESSAGE_GROUP")) {
                                        sys.msg_sig = atoi (opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"MESSAGE_GROUP_ONLY")) {
                                        sys.msg_restricted = 1;
                                        return;
                                }
                                if (!stricmp(opt[0],"FORWARD1")) {
                                        strcpy(sys.forward1, replace_blank(opt[1]));
                                        return;
                                }
                                if (!stricmp(opt[0],"FORWARD2")) {
                                        strcpy(sys.forward2, replace_blank(opt[1]));
                                        return;
                                }
                                if (!stricmp(opt[0],"FORWARD3")) {
                                        strcpy(sys.forward3, replace_blank(opt[1]));
                                        return;
                                }
                                if (!stricmp(opt[0],"USE_ALIAS")) {
                                        sys.use_alias = atoi (opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"FILE_NAME")) {
                                        strcpy(sys.file_name, replace_blank(opt[1]));
                                        return;
                                }
                                if (!stricmp(opt[0],"FILE_KEYWORD")) {
                                        strcpy(idxfkey, replace_blank(opt[1]));
                                        return;
                                }
                                if (!stricmp(opt[0],"ECHOTAG")) {
                                        strcpy(sys.echotag, opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"QUICK_BOARD")) {
                                        sys.quick_board = atoi(opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"SQUISH_MESSAGE")) {
                                        sys.squish = 1;
                                        strip_backslash (sys.msg_path);
                                        return;
                                }
                                if (!stricmp(opt[0],"FILE_GROUP")) {
                                        sys.file_sig = atoi (opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"FILE_GROUP_ONLY")) {
                                        sys.file_restricted = 1;
                                        return;
                                }
                                if (!stricmp(opt[0],"PIP_BOARD")) {
                                        sys.pip_board = atoi(opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"MESSAGE_PRIV"))
                                {
                                        sys.msg_priv = get_priv(opt[1]);
                                        if (!sys.write_priv)
                                                sys.write_priv = get_priv(opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"MESSAGE_FLAGS"))
                                {
                                        sys.msg_flags = get_flags(opt[1]);
                                        if (!sys.write_flags)
                                                sys.write_flags = get_flags(opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"WRITE_PRIV")) {
                                        sys.write_priv = get_priv(opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"WRITE_FLAGS")) {
                                        sys.write_flags = get_flags(opt[1]);
                                        return;
                                }
/*
                                if (!stricmp(opt[0],"FILE_REQUEST"))
                                {
                                        if (strstr (opt[1], "ALL") != NULL)
                                        {
                                                sys.norm_req = 1;
                                                sys.know_req = 1;
                                                sys.prot_req = 1;
                                        }
                                        if (strstr (opt[1], "NONE") != NULL)
                                        {
                                                sys.norm_req = 0;
                                                sys.know_req = 0;
                                                sys.prot_req = 0;
                                        }
                                        if (strstr (opt[1], "NORM") != NULL)
                                                sys.norm_req = 1;
                                        if (strstr (opt[1], "PROT") != NULL)
                                                sys.prot_req = 1;
                                        if (strstr (opt[1], "KNOW") != NULL)
                                                sys.know_req = 1;
                                }
*/
                                if (!stricmp(opt[0],"FILE_PRIV"))
                                {
                                        sys.file_priv = get_priv(opt[1]);
                                        if (!sys.download_priv)
                                                sys.download_priv = get_priv(opt[1]);
                                        if (!sys.upload_priv)
                                                sys.upload_priv = get_priv(opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"FILE_FLAGS"))
                                {
                                        sys.file_flags = get_flags(opt[1]);
                                        if (!sys.download_flags)
                                                sys.download_flags = get_flags(opt[1]);
                                        if (!sys.upload_flags)
                                                sys.upload_flags = get_flags(opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"LIST_PRIV")) {
                                        sys.list_priv = get_priv(opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"LIST_FLAGS")) {
                                        sys.list_flags = get_flags(opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"DOWNLOAD_PRIV")) {
                                        sys.download_priv = get_priv(opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"DOWNLOAD_FLAGS")) {
                                        sys.download_flags = get_flags(opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"UPLOAD_PRIV")) {
                                        sys.upload_priv = get_priv(opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"UPLOAD_FLAGS")) {
                                        sys.upload_flags = get_flags(opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"PUBLIC_ONLY")) {
                                        sys.public = 1;
                                        return;
                                }
                                if (!stricmp(opt[0],"PRIVATE_ONLY")) {
                                        sys.private = 1;
                                        return;
                                }
                                if (!stricmp(opt[0],"NO_MATRIX_REPLY")) {
                                        sys.no_matrix = 1;
                                        return;
                                }
                                if (!stricmp(opt[0],"ECHOMAIL")) {
                                        sys.echomail = 1;
                                        return;
                                }
                                if (!stricmp(opt[0],"PASSTHROUGH")) {
                                        sys.passthrough = 1;
                                        return;
                                }
                                if (!stricmp(opt[0],"NETMAIL")) {
                                        sys.netmail = 1;
                                        return;
                                }
                                if (!stricmp(opt[0],"OK_ALIAS")) {
                                        sys.anon_ok = 1;
                                        return;
                                }
                                if (!stricmp(opt[0],"FREE_DOWNLOAD")) {
                                        sys.freearea = 1;
                                        return;
                                }
                                if (!stricmp(opt[0],"NO_GLOBAL_SEARCH")) {
                                        sys.no_global_search = 1;
                                        return;
                                }
                                if (!stricmp(opt[0],"NO_FILE_DATE")) {
                                        sys.no_filedate = 1;
                                        return;
                                }
                                if (!stricmp(opt[0],"DOWNLOAD_PATH")) {
                                        append_backslash (opt[1]);
                                        strcpy(sys.filepath, opt[1]);
                                        strcpy(sys.uppath, opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"UPLOAD_PATH"))
                                {
                                        append_backslash (opt[1]);
                                        strcpy(sys.uppath, opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"FILE_LIST")) {
                                        strcpy(sys.filelist, opt[1]);
                                        return;
                                }
                                if (!stricmp(opt[0],"END_SYSTEM"))
                                {
                                        if (!def_system)
                                                return;

                                        if (fd != -1)
                                                close (fd);

                                        if (sys.msg_name[0] && sys.msg_priv)
                                        {
                                           sys.msg_num = def_system;
                                           write(fdmd,(char *)&sys.msg_name, SIZEOF_MSGAREA);

                                           sysidx.priv = sys.msg_priv;
                                           memcpy ((char *)&sysidx.flags, (char *)&sys.msg_flags, MAXFLAGS);
                                           strcpy (sysidx.key, "");
                                           sysidx.area = def_system;
                                           if (idxmkey[0])
                                               strcpy (sysidx.key, idxmkey);

                                           write(fdmi,(char *)&sysidx, sizeof (struct _sys_idx));
                                           idxmkey[0] = '\0';
                                        }

                                        if (sys.file_name[0] && sys.file_priv)
                                        {
                                           sys.file_num = def_system;
                                           write(fdfd,(char *)&sys.file_name, SIZEOF_FILEAREA);

                                           sysidx.priv = sys.file_priv;
                                           memcpy ((char *)&sysidx.flags, (char *)&sys.file_flags, MAXFLAGS);
                                           strcpy (sysidx.key, "");
                                           sysidx.area = def_system;
                                           if (idxfkey[0])
                                               strcpy (sysidx.key, idxfkey);

                                           write(fdfi,(char *)&sysidx, sizeof (struct _sys_idx));
                                           idxfkey[0] = '\0';
                                        }

                                        fd = -1;
                                        def_system = 0;
                                        return;
                                }

                                printf("Configuration error. Line %d. (%s)\n", line, opt[0]);
}

int get_priv(txt)
char *txt;
{
   int i, priv;

   priv = HIDDEN;

   for (i = 0; i < 12; i++)
      if (toupper(levels[i].p_string[0]) == toupper(txt[0])) {
         priv = levels[i].p_length;
         break;
      }

   return (priv);
}

int get_day(txt)
char *txt;
{
        int i, priv;

        priv = 0;
        strupr(txt);

        for (i=0;days[i].p_string != NULL;i++)
                if (strstr(txt, days[i].p_string) != NULL) {
                        priv |= days[i].p_length;
                        break;
                }

        return (priv);
}

char *replace_blank(s)
char *s;
{
	char *p;

	while ((p=strchr(s,'_')) != NULL)
		*p = ' ';

	return (s);
}

void append_backslash (s)
char *s;
{
        int i;

        i = strlen(s) - 1;
        if (s[i] != '\\')
                strcat (s, "\\");
}

void strip_backslash (s)
char *s;
{
        int i;

        i = strlen(s) - 1;
        if (s[i] == '\\')
                s[i] = '\0';
}

long get_flags (char *p)
{
   long flag;

   flag = 0L;

   while (*p)
      switch (toupper(*p++))
      {
      case 'V':
         flag |= 0x00000001L;
         break;
      case 'U':
         flag |= 0x00000002L;
         break;
      case 'T':
         flag |= 0x00000004L;
         break;
      case 'S':
         flag |= 0x00000008L;
         break;
      case 'R':
         flag |= 0x00000010L;
         break;
      case 'Q':
         flag |= 0x00000020L;
         break;
      case 'P':
         flag |= 0x00000040L;
         break;
      case 'O':
         flag |= 0x00000080L;
         break;
      case 'N':
         flag |= 0x00000100L;
         break;
      case 'M':
         flag |= 0x00000200L;
         break;
      case 'L':
         flag |= 0x00000400L;
         break;
      case 'K':
         flag |= 0x00000800L;
         break;
      case 'J':
         flag |= 0x00001000L;
         break;
      case 'I':
         flag |= 0x00002000L;
         break;
      case 'H':
         flag |= 0x00004000L;
         break;
      case 'G':
         flag |= 0x00008000L;
         break;
      case 'F':
         flag |= 0x00010000L;
         break;
      case 'E':
         flag |= 0x00020000L;
         break;
      case 'D':
         flag |= 0x00040000L;
         break;
      case 'C':
         flag |= 0x00080000L;
         break;
      case 'B':
         flag |= 0x00100000L;
         break;
      case 'A':
         flag |= 0x00200000L;
         break;
      case '9':
         flag |= 0x00400000L;
         break;
      case '8':
         flag |= 0x00800000L;
         break;
      case '7':
         flag |= 0x01000000L;
         break;
      case '6':
         flag |= 0x02000000L;
         break;
      case '5':
         flag |= 0x04000000L;
         break;
      case '4':
         flag |= 0x08000000L;
         break;
      case '3':
         flag |= 0x10000000L;
         break;
      case '2':
         flag |= 0x20000000L;
         break;
      case '1':
         flag |= 0x40000000L;
         break;
      case '0':
         flag |= 0x80000000L;
         break;
      }

   return (flag);
}

#define MAXDUPES     1000
#define DUPE_HEADER  (48 + 2 + 2 + 4)
#define DUPE_FOOTER  (MAXDUPES * 4)

struct _dupecheck {
   char areatag[48];
   int  dupe_pos;
   int  max_dupes;
   long area_pos;
   long dupes[MAXDUPES];
};

void zero_dupes (area)
char *area;
{
   int fd;
   struct _dupecheck *dupecheck;

   fd = open ("DUPES.DAT", O_RDWR|O_BINARY);
   if (fd == -1)
      return;

   dupecheck = (struct _dupecheck *)malloc (sizeof (struct _dupecheck));
   if (dupecheck == NULL)
      return;

   while (read (fd, (char *)dupecheck, DUPE_HEADER) == DUPE_HEADER) {
      if (!stricmp (area, dupecheck->areatag)) {
         memset ((char *)dupecheck, 0, sizeof (struct _dupecheck));
         dupecheck->area_pos = tell (fd) - DUPE_HEADER;
         strcpy (dupecheck->areatag, area);
         lseek (fd, -1L * DUPE_HEADER, SEEK_CUR);
         write (fd, (char *)dupecheck, DUPE_HEADER);
         write (fd, (char *)dupecheck->dupes, DUPE_FOOTER);
         break;
      }
      else
         lseek (fd, (long)DUPE_FOOTER, SEEK_CUR);
   }

   free (dupecheck);
   close (fd);
}

