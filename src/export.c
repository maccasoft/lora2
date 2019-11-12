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

#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <dir.h>
#include <dos.h>
#include <string.h>
#include <process.h>
#include <time.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "exec.h"
#include "quickmsg.h"

#define isLORA       0x4E
#include "version.h"

struct _aidx {
//   char areatag[32];
    unsigned long areatag;
    byte board;
    word gold_board;
};

extern int maxaidx, nodes_num;
extern char nomailproc, nonetmail;
extern struct _aidx * aidx;

struct _node2name * nametable; // Gestione nomi per aree PRIVATE
int nodes_num;

int maxakainfo;
struct _akainfo * akainfo = NULL;

#define MAXDUPES      1000
#define MAX_FORWARD   128

#define PACK_ECHOMAIL  0x0001
#define PACK_NETMAIL   0x0002

unsigned long get_buffer_crc(void * buffer, int length);
void mail_batch(char * what);
void open_logfile(void);
int spawn_program(int swapout, char * outstring);
void pack_outbound(int);
void build_aidx(void);
void rescan_areas(void);

void read_nametable(void)
{
    NODEINFO ni;
    int i = 0, fd;
    char string[128];


    sprintf(string, "%sNODES.DAT", config->net_info);

    fd = open(string, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);
    if (fd == -1) {
        return;
    }
    nodes_num = (int)(filelength(fd) / (long)sizeof(NODEINFO));

    nametable = (struct _node2name *)malloc(sizeof(NODE2NAME) * nodes_num);
    if (!nametable) {
        return;
    }

    while (read(fd, (char *)&ni, sizeof(NODEINFO)) == sizeof(NODEINFO)) {
        strncpy(nametable[i].name, ni.sysop_name, 36);
        nametable[i].zone = ni.zone;
        nametable[i].net = ni.net;
        nametable[i].node = ni.node;
        nametable[i].point = ni.point;
        i++;
    }
    close(fd);
}

void copy_mail_packet(char * from, char * to)
{
    FILE * fps, *fpd;
    int i;
    char string[512];

    if (rename(from, to) == 0) {
        return;
    }

    if ((fps = sh_fopen(from, "rb", SH_DENYRW)) == NULL) {
        return;
    }

    if ((fpd = sh_fopen(to, "ab", SH_DENYRW)) == NULL) {
        fclose(fps);
        return;
    }

    if (filelength(fileno(fpd)) > 0L) {
        fseek(fps, sizeof(struct _pkthdr2), SEEK_SET);
        fseek(fpd, -2L, SEEK_END);
    }

    do {
        i = fread(string, 1, 512, fps);
        fwrite(string, 1, i, fpd);
    } while (i == 512);

    fclose(fpd);
    fclose(fps);

    unlink(from);
}

void export_mail(who)
int who;
{
    FILE * fp;
    int maxnodes, wh, i, fd;
    char filename[80], *p, linea[256], *tag, *location, *forw;
    struct _fwrd * forward;
    struct date datep;
    struct time timep;
    struct _pkthdr2 pkthdr;
    struct _wrec_t * wr;

    if (nomailproc) {
        return;
    }

    local_status("Export");
    scan_system();
    read_nametable();


    if (who & ECHOMAIL_RSN) {
        mail_batch(config->pre_export);

        if (!registered) {
            config->replace_tear = 3;
        }
        status_line("#Scanning messages");

        forward = (struct _fwrd *)malloc(MAX_FORWARD * sizeof(struct _fwrd));
        if (forward == NULL) {
            free(nametable);
            nametable = NULL;
            return;
        }

        build_aidx();

        wh = wopen(12, 0, 24, 79, 0, LGREY | _BLACK, LCYAN | _BLACK);
        wactiv(wh);
        wtitle("PROCESS ECHOMAIL", TLEFT, LCYAN | _BLACK);
        wprints(0, 0, YELLOW | _BLACK, " Num.  Area tag               Base         Forward to");
        printc(12, 0, LGREY | _BLACK, '\303');
        printc(12, 52, LGREY | _BLACK, '\301');
        printc(12, 79, LGREY | _BLACK, '\264');
        wr = wfindrec(wh);
        wr->srow++;
        wr->row++;

        maxnodes = 0;
        totalmsg = 0;
        totaltime = timerset(0);

        if (config->use_areasbbs) {
            time_release();

            fp = fopen(config->areas_bbs, "rt");
            if (fp != NULL) {
                fgets(linea, 255, fp);

                while (fgets(linea, 255, fp) != NULL) {
                    if (linea[0] == ';') {
                        continue;
                    }
                    while (linea[strlen(linea) - 1] == 0x0D || linea[strlen(linea) - 1] == 0x0A || linea[strlen(linea) - 1] == ' ') {
                        linea[strlen(linea) - 1] = '\0';
                    }
                    if ((location = strtok(linea, " ")) == NULL) {
                        continue;
                    }
                    if ((tag = strtok(NULL, " ")) == NULL) {
                        continue;
                    }
                    if ((forw = strtok(NULL, "")) == NULL) {
                        continue;
                    }
                    while (*forw == ' ') {
                        forw++;
                    }
                    sys.echomail = 1;
                    strcpy(sys.echotag, tag);
                    strcpy(sys.forward1, forw);
                    if (*location == '$') {
                        strcpy(sys.msg_path, ++location);
                        sys.squish = 1;
                        if (sq_ptr != NULL) {
                            MsgUnlock(sq_ptr);
                            MsgCloseArea(sq_ptr);
                        }
                        if ((sq_ptr = MsgOpenArea(sys.msg_path, MSGAREA_CRIFNEC, MSGTYPE_SQUISH)) == NULL) {
                            status_line("!MsgApi Error: %s", sys.msg_path);
                            continue;
                        }
                        while (MsgLock(sq_ptr) == -1) {
                            time_release();
                            release_timeslice();
                        }
                        num_msg = (int)MsgGetNumMsg(sq_ptr);
                        if (num_msg) {
                            first_msg = 1;
                        }
                        else {
                            first_msg = 0;
                        }
                        last_msg = num_msg;
                        maxnodes = squish_export_mail(maxnodes, forward);
                        if (maxnodes == -1) {
                            status_line("!Error exporting mail");
                            free(nametable);
                            nametable = NULL;
                            return;
                        }
                    }
                    else if (*location == '!') {
                        sys.pip_board = atoi(++location);
                        maxnodes = pip_export_mail(maxnodes, forward);
                        if (maxnodes == -1) {
                            status_line("!Error exporting mail");
                            free(nametable);
                            nametable = NULL;
                            return;
                        }
                    }
                    else if (!atoi(location) && stricmp(location, "##")) {
                        strcpy(sys.msg_path, location);
                        scan_message_base(0, 0);
                        maxnodes = fido_export_mail(maxnodes, forward);
                        if (maxnodes == -1) {
                            status_line("!Error exporting mail");
                            free(nametable);
                            nametable = NULL;
                            return;
                        }
                    }
                }

                fclose(fp);
            }
        }

        time_release();
        maxnodes = quick_export_mail(maxnodes, forward, 0);
        if (maxnodes == -1) {
            status_line("!Error exporting mail");
            free(nametable);
            nametable = NULL;
            return;
        }

        time_release();
        maxnodes = quick_export_mail(maxnodes, forward, 1);
        if (maxnodes == -1) {
            status_line("!Error exporting mail");
            free(nametable);
            nametable = NULL;
            return;
        }

        time_release();
        sprintf(filename, SYSMSG_PATH, config->sys_path);
        fd = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        if (fd != -1) {
            while (read(fd, (char *)&sys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
                if (sys.squish && sys.echomail && !sys.passthrough) {
                    if (sq_ptr != NULL) {
                        MsgUnlock(sq_ptr);
                        MsgCloseArea(sq_ptr);
                    }
                    if ((sq_ptr = MsgOpenArea(sys.msg_path, MSGAREA_CRIFNEC, MSGTYPE_SQUISH)) == NULL) {
                        status_line("!MsgApi Error: Area %d, %s", sys.msg_num, sys.msg_path);
                        continue;
                    }
                    while (MsgLock(sq_ptr) == -1)
                        ;
                    MsgLock(sq_ptr);
                    num_msg = (int)MsgGetNumMsg(sq_ptr);
                    if (num_msg) {
                        first_msg = 1;
                    }
                    else {
                        first_msg = 0;
                    }
                    last_msg = num_msg;
                    maxnodes = squish_export_mail(maxnodes, forward);
                    if (maxnodes == -1) {
                        status_line("!Error exporting mail");
                        close(fd);
                        free(nametable);
                        nametable = NULL;
                        return;
                    }
                }
            }

            if (sq_ptr != NULL) {
                MsgCloseArea(sq_ptr);
                sq_ptr = NULL;
            }
            close(fd);
        }

        time_release();
        sprintf(filename, SYSMSG_PATH, config->sys_path);
        fd = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        if (fd != -1) {
            while (read(fd, (char *)&sys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
                if (sys.pip_board && sys.echomail && !sys.passthrough) {
                    maxnodes = pip_export_mail(maxnodes, forward);
                    if (maxnodes == -1) {
                        status_line("!Error exporting mail");
                        close(fd);
                        free(nametable);
                        nametable = NULL;
                        return;
                    }
                }
            }

            close(fd);
        }

        time_release();
        sprintf(filename, SYSMSG_PATH, config->sys_path);
        fd = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        if (fd != -1) {
            while (read(fd, (char *)&sys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
                if (!sys.pip_board && !sys.squish && !sys.quick_board && sys.echomail && !sys.passthrough) {
                    scan_message_base(0, 0);
                    maxnodes = fido_export_mail(maxnodes, forward);
                    if (maxnodes == -1) {
                        status_line("!Error exporting mail");
                        close(fd);
                        free(nametable);
                        nametable = NULL;
                        return;
                    }
                }
            }

            close(fd);
        }

        gettime(&timep);
        getdate(&datep);

        for (i = 0; i < maxnodes; i++) {
            p = HoldAreaNameMunge(forward[i].zone);

//         if (forward[i].point)
//            sprintf (linea, "%s%04X%04X.PNT\\%08X.XPR", p, forward[i].net, forward[i].node, forward[i].point);
//         else
//            sprintf (linea, "%s%04X%04X.XPR", p, forward[i].net, forward[i].node);

            if (forward[i].point) {
                sprintf(filename, "%s%04X%04X.PNT\\%08X.XPR", p, forward[i].net, forward[i].node, forward[i].point);
            }
            else {
                sprintf(filename, "%s%04X%04X.XPR", p, forward[i].net, forward[i].node);
            }

//         copy_mail_packet (linea, filename);

            fd = open(filename, O_RDWR | O_BINARY);
            if (fd != -1) {
                if (filelength(fd) == 0L) {
                    close(fd);
                    unlink(filename);
                }
                else {
                    read(fd, (char *)&pkthdr, sizeof(struct _pkthdr2));
                    if (!pkthdr.year && !pkthdr.month && !pkthdr.day) {
                        pkthdr.hour = timep.ti_hour;
                        pkthdr.minute = timep.ti_min;
                        pkthdr.second = timep.ti_sec;
                        pkthdr.year = datep.da_year;
                        pkthdr.month = datep.da_mon - 1;
                        pkthdr.day = datep.da_day;
                        lseek(fd, 0L, SEEK_SET);
                        write(fd, (char *)&pkthdr, sizeof(struct _pkthdr2));
                    }
                    close(fd);
                }
            }
        }

        wclose();
        free(forward);
        if (aidx != NULL) {
            free(aidx);
        }
        if (akainfo != NULL) {
            free(akainfo);
        }

        local_status("Export");
        scan_system();

        mail_batch(config->after_export);

        if (totalmsg) {
            status_line("+%d ECHOmail message(s) forwarded", totalmsg);
            sysinfo.today.echosent += totalmsg;
            sysinfo.week.echosent += totalmsg;
            sysinfo.month.echosent += totalmsg;
            sysinfo.year.echosent += totalmsg;
        }
        else {
            status_line("+No ECHOmail messages forwarded");
        }
    }

    if ((who & NETMAIL_RSN) && nonetmail == 0) {
        mail_batch(config->pre_pack);

        wh = wopen(12, 0, 24, 79, 0, LGREY | _BLACK, LCYAN | _BLACK);
        wactiv(wh);
        wtitle("NETMAIL", TLEFT, LCYAN | _BLACK);
        wprints(0, 0, YELLOW | _BLACK, " Num.  Area tag               Base         Forward to");
        printc(12, 0, LGREY | _BLACK, '\303');
        printc(12, 52, LGREY | _BLACK, '\301');
        printc(12, 79, LGREY | _BLACK, '\264');
        wr = wfindrec(wh);
        wr->srow++;
        wr->row++;

        memset((char *)&sys, 0, sizeof(struct _sys));
        sys.netmail = 1;
        strcpy(sys.msg_path, netmail_dir);
        if (fido_export_netmail() == -1) {
            status_line("!Error exporting mail");
            free(nametable);
            nametable = NULL;
            return;
        }

        sprintf(filename, SYSMSG_PATH, config->sys_path);
        fd = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        if (fd != -1) {
            while (read(fd, (char *)&sys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
                if (sys.netmail && !stricmp(sys.msg_path, netmail_dir)) {
                    continue;
                }
                if (sys.netmail || (sys.internet_mail && config->export_internet)) {
                    if (fido_export_netmail() == -1) {
                        status_line("!Error exporting mail");
                        free(nametable);
                        nametable = NULL;
                        return;
                    }
                }
            }

            close(fd);
        }

        wclose();
        rescan_areas();

        mail_batch(config->after_pack);
    }

    if (!(who & NO_PACK)) {
        if (!(who & ECHOMAIL_RSN)) {
            pack_outbound(PACK_NETMAIL);
        }
        else {
            pack_outbound(PACK_ECHOMAIL);
        }
    }

    idle_system();
    memset((char *)&sys, 0, sizeof(struct _sys));
    free(nametable);
    nametable = NULL;
}

void build_aidx()
{
    int fd, i;
    char filename[80];
    struct _sys tsys;
    NODEINFO ni;

    sprintf(filename, "%sNODES.DAT", config->net_info);
    if ((fd = sh_open(filename, SH_DENYWR, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) != -1) {
        akainfo = (struct _akainfo *)malloc((int)((filelength(fd) / sizeof(NODEINFO)) * sizeof(struct _akainfo)));

        i = 0;
        while (read(fd, (char *)&ni, sizeof(NODEINFO)) == sizeof(NODEINFO)) {
            akainfo[i].zone = ni.zone;
            akainfo[i].net = ni.net;
            akainfo[i].node = ni.node;
            akainfo[i].point = ni.point;
            akainfo[i].aka = ni.aka;
            i++;
        }

        close(fd);
        maxakainfo = i;
    }
    else {
        maxakainfo = 0;
    }

    sprintf(filename, SYSMSG_PATH, config->sys_path);
    fd = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
    if (fd != -1) {
        maxaidx = 0;

        aidx = (struct _aidx *)malloc((int)(((filelength(fd) / SIZEOF_MSGAREA) + 10) * sizeof(struct _aidx)));
        if (aidx == NULL) {
            close(fd);
            return;
        }

        i = 0;

        while (read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
            aidx[i].board = tsys.quick_board;
            aidx[i].gold_board = tsys.gold_board;
            aidx[i++].areatag = get_buffer_crc(tsys.echotag, strlen(tsys.echotag));
        }

        close(fd);

        maxaidx = i;
    }
}

