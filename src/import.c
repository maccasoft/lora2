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
#include <ctype.h>
#include <process.h>
#include <time.h>
#include <conio.h>
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
#include "exec.h"
#include "quickmsg.h"

#define isMAX_PRODUCT    0xE2

void open_logfile(void);
void import_newareas(char * newareas);
int spawn_program(int swapout, char * outstring);
FILE * mopen(char * filename, char * mode);
int mclose(FILE * fp);
int mputs(char * s, FILE * fp);
void mprintf(FILE * fp, char * format, ...);
long mseek(FILE * fp, long position, int offset);
int mread(char * s, int n, int e, FILE * fp);
void build_aidx(void);
void track_inbound_messages(FILE * fpd, struct _msg * msgt, int fzone);
int check_board_not_used(char goldbase, FILE * fp);
int open_packet(int zone, int net, int node, int point, int ai);
unsigned long get_buffer_crc(void * buffer, int length);

struct _pkthdr22
{
    short orig_node;         /* originating node               */
    short dest_node;         /* destination node               */
    short orig_point;        /* originating point              */
    short dest_point;        /* Destination point              */
    byte reserved[8];
    short subver;            /* packet subversion              */
    short ver;               /* packet version                 */
    short orig_net;          /* originating network number     */
    short dest_net;          /* destination network number     */
    byte product;           /* product type                   */
    char serial;            /* serial number (some systems)   */
    byte password[8];       /* session/pickup password        */
    short orig_zone;         /* originating zone               */
    short dest_zone;         /* Destination zone               */
    char orig_domain[8];    /* originating domain name        */
    char dest_domain[8];    /* destination domain name        */
    byte filler[4];
};

struct _fwd_alias {
    short zone;
    short net;
    short node;
    short point;
    bit   export;
    bit   receive;
};
/*
   > - Nodo a sola ricezione (il nodo non puo' spedire messaggi echo)
*/

#define MAXDUPES      1000
#define MAX_SEEN      512
#define MAX_PATH      256
#define MAX_FORWARD   128
#define MAX_DUPEINDEX 40

struct _dupecheck {
    char  areatag[48];
    short dupe_pos;
    short max_dupes;
    long  area_pos;
    long  dupes[MAXDUPES];
};

struct _dupeindex {
    char  areatag[48];
    long  area_pos;
};

struct _aidx {
//   char areatag[32];
    unsigned long areatag;
    byte board;
    word gold_board;
};

extern char * suffixes[], nomailproc;
extern struct _msginfo msginfo;
extern struct _gold_msginfo gmsginfo;

extern int maxakainfo;
extern struct _akainfo * akainfo;

int isbundle(char *);
int fido_save_message2(FILE *, char *);
int fido_save_message3(FILE *);
void call_packer(char *, char *, char *, int, int, int, int);
char * packet_fgets(char *, int, FILE * fp);

int maxaidx;
static FILE * f1, *f2, *f3, *f4, *f5, *fpareas;
struct _dupecheck * dupecheck = NULL;
struct _aidx * aidx;
static int ndupes, nbad, pkt_zone, pkt_net, pkt_node, pkt_point, nmy;
static int totalpackets, totalnetmail, totalechomail, fdsys, totaldupes, totalbad;

static int verify_password(char * pwd, int zone, int net, int node, int point);
static void rename_bad_packet(char *, char *, char *);
static int process_type2_packet(FILE *, struct _pkthdr2 *, char *);
static int process_type2plus_packet(FILE *, struct _pkthdr2 *, char *);
static int process_type22_packet(FILE *, struct _pkthdr22 *, struct ffblk *);
static int  process_packet(FILE *, struct _pkthdr2 *);
static char * get_to_null(FILE * fp, char *, int);
static int  read_dupe_check(char *);
static void write_dupe_check(void);
static int  check_and_add_dupe(long);
static long compute_crc(char *, long);
static int passthrough_save_message(FILE *, struct _fwd_alias *, int, struct _fwd_alias *, int);
static void unpack_arcmail(char *);

void mail_batch(char * what)
{
    int * varr, i;
    char curdir[50];

    if (what && what[0]) {
        status_line(":Executing %s", what);

        fclose(logf);
        getcwd(curdir, 49);
        varr = ssave();
        cclrscrn(LGREY | _BLACK);
        showcur();

        i = spawn_program(registered, what);

        if (varr != NULL) {
            srestore(varr);
        }
        setdisk(curdir[0] - 'A');
        chdir(curdir);
        open_logfile();
        hidecur();

        if (i) {
            status_line(":Return code %d", i);
        }
    }
}

void import_sequence(void)
{
    if (nomailproc) {
        return;
    }

    if (e_ptrs[cur_event]->echomail & (ECHO_PROT | ECHO_KNOW | ECHO_NORMAL | ECHO_RESERVED4)) {
        mail_batch(config->pre_import);
    }

    if (e_ptrs[cur_event]->echomail & (ECHO_RESERVED4)) {
        import_tic_files();
    }

    if (e_ptrs[cur_event]->echomail & (ECHO_PROT | ECHO_KNOW | ECHO_NORMAL)) {
        import_mail();
    }
}

/*---------------------------------------------------------------------------
   void import_mail (void)

	Shell da richiamare per importare l'echomail e la netmail dalle
   directory di inbound. L'echomail viene importata automaticamente dalle
   directory di inbound appropriate, a seconda dei settaggi dell'evento
   corrente.
---------------------------------------------------------------------------*/
void import_mail(void)
{
    FILE * fp;
    int wh, i, d;
    char filename[80], pktname[14], *dir, newname[14];
    unsigned int pktdate, pkttime;
    struct ffblk blk;
    struct _pkthdr2 pkthdr;
    struct _wrec_t * wr;

    build_aidx();

    // Scompatta tutti i pacchetti arcmail presenti nella directory indicata.
    for (d = 0; d < 4; d++) {
        if ((e_ptrs[cur_event]->echomail & (ECHO_PROT | ECHO_KNOW | ECHO_NORMAL))) {
            if (d == 0 && config->prot_filepath[0] && (e_ptrs[cur_event]->echomail & ECHO_PROT)) {
                dir = config->prot_filepath;
            }
            if (d == 1 && config->know_filepath[0] && (e_ptrs[cur_event]->echomail & ECHO_KNOW)) {
                dir = config->know_filepath;
            }
            if (d == 2 && config->norm_filepath[0] && (e_ptrs[cur_event]->echomail & ECHO_NORMAL)) {
                dir = config->norm_filepath;
            }
            if (d == 3) {
                dir = ".\\";
            }
        }
        else {
            if (d == 0 && config->prot_filepath[0]) {
                dir = config->prot_filepath;
            }
            if (d == 1 && config->know_filepath[0]) {
                dir = config->know_filepath;
            }
            if (d == 2 && config->norm_filepath[0]) {
                dir = config->norm_filepath;
            }
            if (d == 3) {
                dir = ".\\";
            }
        }

        if (d != 3) {
            unpack_arcmail(dir);
        }
    }

    local_status("Import");
    toss_system();

    wh = wopen(12, 0, 24, 79, 0, LGREY | _BLACK, LCYAN | _BLACK);
    wactiv(wh);
    wtitle("PROCESS ECHOMAIL", TLEFT, LCYAN | _BLACK);
    wprints(0, 0, YELLOW | _BLACK, " Num.  Area tag                         Base          Imp.");
    printc(12, 0, LGREY | _BLACK, '\303');
    printc(12, 52, LGREY | _BLACK, '\301');
    printc(12, 79, LGREY | _BLACK, '\264');
    wr = wfindrec(wh);
    wr->srow++;

    if (config->use_areasbbs) {
        fpareas = fopen(config->areas_bbs, "rt");
    }

    sprintf(filename, SYSMSG_PATH, config->sys_path);
    fdsys = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);

    f1 = f2 = f3 = f4 = f5 = NULL;

    dupecheck = (struct _dupecheck *)malloc(sizeof(struct _dupecheck));
    if (dupecheck == NULL) {
        return;
    }

    memset((char *)&sys, 0, sizeof(struct _sys));
    if (dupes != NULL) {
        strcpy(sys.msg_path, dupes);
        scan_message_base(0, 0);
        ndupes = last_msg;
    }
    else {
        ndupes = 0;
    }
    if (bad_msgs != NULL && *bad_msgs) {
        strcpy(sys.msg_path, bad_msgs);
        scan_message_base(0, 0);
        nbad = last_msg;
    }
    else {
        nbad = 0;
    }

    if (config->my_mail[0]) {
        strcpy(sys.msg_path, config->my_mail);
        scan_message_base(0, 0);
        nmy = last_msg;
    }
    else {
        nmy = 0;
    }

    totalpackets = totalnetmail = totalechomail = totalmsg = totaldupes = totalbad = 0;
    totaltime = timerset(0);

    for (d = 0; d < 4; d++) {
        if ((e_ptrs[cur_event]->echomail & (ECHO_PROT | ECHO_KNOW | ECHO_NORMAL))) {
            if (d == 0 && config->prot_filepath[0] && (e_ptrs[cur_event]->echomail & ECHO_PROT)) {
                dir = config->prot_filepath;
            }
            if (d == 1 && config->know_filepath[0] && (e_ptrs[cur_event]->echomail & ECHO_KNOW)) {
                dir = config->know_filepath;
            }
            if (d == 2 && config->norm_filepath[0] && (e_ptrs[cur_event]->echomail & ECHO_NORMAL)) {
                dir = config->norm_filepath;
            }
            if (d == 3) {
                dir = ".\\";
            }
        }
        else {
            if (d == 0 && config->prot_filepath[0]) {
                dir = config->prot_filepath;
            }
            if (d == 1 && config->know_filepath[0]) {
                dir = config->know_filepath;
            }
            if (d == 2 && config->norm_filepath[0]) {
                dir = config->norm_filepath;
            }
            if (d == 3) {
                dir = ".\\";
            }
        }

        // Se non ci sono PKT ritorna immediatamente.
        sprintf(filename, "%s*.PK?", dir);
        if (findfirst(filename, &blk, 0)) {
            continue;
        }

        status_line("#Tossing msgs from %s", dir);

        for (;;) {
            sprintf(filename, "%s*.PK?", dir);
            if (findfirst(filename, &blk, 0)) {
                break;
            }

            strcpy(pktname, blk.ff_name);
            pktdate = blk.ff_fdate;
            pkttime = blk.ff_ftime;

            // Routinetta per trovare il .pkt piu' vecchio tra quelli che sono
            // stati ricevuti, in modo da estrarre i messaggi sempre nell'ordine
            // giusto.
            while (!findnext(&blk)) {
                if (pktdate > blk.ff_fdate || (pktdate == blk.ff_fdate && pkttime > blk.ff_ftime)) {
                    strcpy(pktname, blk.ff_name);
                    pktdate = blk.ff_fdate;
                    pkttime = blk.ff_ftime;
                }
            }

            strcpy(blk.ff_name, pktname);

            prints(7, 65, YELLOW | _BLACK, blk.ff_name);
            time_release();

            sprintf(filename, "%s%s", dir, blk.ff_name);
            rename_bad_packet(filename, dir, newname);

            sprintf(filename, "%s%s", dir, newname);
            fp = sh_fopen(filename, "rb", SH_DENYRW);
            if (fp == NULL) {
                continue;
            }
            setvbuf(fp, NULL, _IOFBF, 2048);
            fread((char *)&pkthdr, sizeof(struct _pkthdr2), 1, fp);

            if (pkthdr.ver != PKTVER) {
                fclose(fp);
            }
            else {
                if (pkthdr.rate == 2) {
                    i = process_type22_packet(fp, (struct _pkthdr22 *)&pkthdr, &blk);
                }
                else {
                    swab((char *)&pkthdr.cwvalidation, (char *)&i, 2);
                    pkthdr.cwvalidation = i;
                    if (pkthdr.capability != pkthdr.cwvalidation || !(pkthdr.capability & 0x0001)) {
                        i = process_type2_packet(fp, &pkthdr, blk.ff_name);
                    }
                    else {
                        i = process_type2plus_packet(fp, &pkthdr, blk.ff_name);
                    }
                }

                fclose(fp);

                if (i == 0) {
                    status_line("!Error processing: %s%s", dir, blk.ff_name);
                    break;
                }
            }

            if (i == 1) {
                unlink(filename);
            }
        }
    }

    if (sq_ptr != NULL) {
        MsgUnlock(sq_ptr);
        MsgCloseArea(sq_ptr);
        sq_ptr = NULL;
    }

    wr->srow--;
    wclose();

    free(dupecheck);
    dupecheck = NULL;
    unlink("MSGTMP.IMP");

    if (!totalpackets) {
        status_line("+No ECHOmail processed at this time");
    }
    else {
        status_line("+%d packet(s): %d NETmail, %d ECHOmail, %d Dupes, %d Bad", totalpackets, totalnetmail, totalechomail, totaldupes, totalbad);
        sysinfo.today.emailreceived += totalnetmail;
        sysinfo.week.emailreceived += totalnetmail;
        sysinfo.month.emailreceived += totalnetmail;
        sysinfo.year.emailreceived += totalnetmail;
        sysinfo.today.echoreceived += totalechomail;
        sysinfo.week.echoreceived += totalechomail;
        sysinfo.month.echoreceived += totalechomail;
        sysinfo.year.echoreceived += totalechomail;
    }
    memset((char *)&sys, 0, sizeof(struct _sys));

    if (fdsys != -1) {
        close(fdsys);
    }

    if (fpareas != NULL) {
        fclose(fpareas);
        if (dexists("NEWAREAS.BBS")) {
            status_line("+Import new echomail area(s)");
            import_newareas("NEWAREAS.BBS");
            unlink("NEWAREAS.BBS");
        }
    }

    mail_batch(config->after_import);

    if (aidx != NULL) {
        free(aidx);
    }
    if (akainfo != NULL) {
        free(akainfo);
    }

    idle_system();
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static void rename_bad_packet(name, dir, newname)
char * name, *dir, *newname;
{
    int i = -1;
    char filename[128], *p;
    struct ffblk blk;

    sprintf(filename, "%sBAD_PKT.*", dir);
    if (!findfirst(filename, &blk, 0))
        do {
            if ((p = strchr(blk.ff_name, '.')) != NULL) {
                p++;
                if (atoi(p) > i) {
                    i = atoi(p);
                }
            }
        } while (!findnext(&blk));

    i++;
    sprintf(filename, "%sBAD_PKT.%03d", dir, i);
    rename(name, filename);

    if (newname != NULL) {
        sprintf(newname, "BAD_PKT.%03d", i);
    }
}

static int verify_password(pwd, zone, net, node, point)
char * pwd;
int zone, net, node, point;
{
    int fd;
    char outbase[80], found, fp[10];
    NODEINFO ni;

    if (!config->secure) {
        return (1);
    }

    found = 0;

    sprintf(outbase, "%sNODES.DAT", config->net_info);
    if ((fd = open(outbase, O_RDONLY | O_BINARY)) != -1) {
        while (read(fd, (char *)&ni, sizeof(NODEINFO)) == sizeof(NODEINFO))
            if (ni.zone == zone && ni.node == node && ni.net == net && ni.point == point) {
                found = 1;
                break;
            }
        close(fd);
    }

    if (!found || (!ni.pw_packet[0] && !ni.pw_inbound_packet[0])) {
        return (1);
    }

    memset(fp, 0, 10);
    strncpy(fp, ni.pw_inbound_packet[0] ? ni.pw_inbound_packet : ni.pw_packet, 8);

    return (!strncmp(strupr(pwd), strupr(fp), 8));
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static int process_type22_packet(fp, pkthdr, blk)
FILE * fp;
struct _pkthdr22 * pkthdr;
struct ffblk * blk;
{
    char tmpstr[40];
    struct _pkthdr2 pkthdr2;

    memset((char *)&msg, 0, sizeof(struct _msg));
    memset((char *)&pkthdr2, 0, sizeof(struct _pkthdr2));

    pkthdr2.dest_net = msg.dest_net = pkthdr->dest_net;
    pkthdr2.dest_node = msg.dest = pkthdr->dest_node;
    pkthdr2.dest_zone = msg_tzone = pkthdr->dest_zone;
    pkthdr2.dest_point = msg_tpoint = pkthdr->dest_point;

    pkthdr2.orig_net = pkt_net = msg.orig_net = pkthdr->orig_net;
    pkthdr2.orig_node = pkt_node = msg.orig = pkthdr->orig_node;
    pkthdr2.orig_zone = pkt_zone = msg_fzone = pkthdr->orig_zone;
    pkthdr2.orig_point = pkt_point = msg_fpoint = pkthdr->orig_point;

    status_line("* %s, %02d/%02d/%02d, %02d:%02d, by %02X (%s)", blk->ff_name, blk->ff_fdate & 0x1F, (blk->ff_fdate >> 5) & 0x0F, (blk->ff_fdate >> 9) & 0x7F % 100, 0, 0, pkthdr->product, pkthdr->product > isMAX_PRODUCT ? "Unknow" : prodcode[pkthdr->product]);
    status_line("* Orig:%d:%d/%d.%d, Dest:%d:%d/%d.%d, Type=2.2", msg_fzone, msg.orig_net, msg.orig, msg_fpoint, msg_tzone, msg.dest_net, msg.dest, msg_tpoint);

    sprintf(tmpstr, "%02d/%02d/%02d", blk->ff_fdate & 0x1F, (blk->ff_fdate >> 5) & 0x0F, (blk->ff_fdate >> 9) & 0x7F % 100);
    prints(8, 65, YELLOW | _BLACK, tmpstr);
    sprintf(tmpstr, "%02d:%02d:%02d", 0, 0, 0);
    prints(9, 65, YELLOW | _BLACK, tmpstr);
    prints(10, 65, YELLOW | _BLACK, "              ");
    sprintf(tmpstr, "%d:%d/%d.%d", msg_fzone, msg.orig_net, msg.orig, msg_fpoint);
    prints(10, 65, YELLOW | _BLACK, tmpstr);

    if (!verify_password(pkthdr->password, msg_fzone, msg.orig_net, msg.orig, msg_fpoint)) {
        status_line("!Packet password error");
        return (2);
    }

    return (process_packet(fp, &pkthdr2));
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static int process_type2_packet(fp, pkthdr, pktname)
FILE * fp;
struct _pkthdr2 * pkthdr;
char * pktname;
{
    char tmpstr[40];

    memset((char *)&msg, 0, sizeof(struct _msg));
    msg.dest_net = pkthdr->dest_net;
    msg.dest = pkthdr->dest_node;
    msg_tzone = pkthdr->dest_zone;
    msg_tpoint = 0;
    pkthdr->dest_zone2 = 0;

    pkt_net = msg.orig_net = pkthdr->orig_net;
    pkt_node = msg.orig = pkthdr->orig_node;
    pkt_zone = msg_fzone = pkthdr->orig_zone;
    pkt_point = msg_fpoint = 0;
    pkthdr->orig_zone2 = 0;

    status_line("* %s, %02d/%02d/%02d, %02d:%02d, by %02X (%s)", pktname, pkthdr->day, pkthdr->month + 1, pkthdr->year % 100, pkthdr->hour, pkthdr->minute, pkthdr->product, pkthdr->product > isMAX_PRODUCT ? "Unknow" : prodcode[pkthdr->product]);
    status_line("* Orig:%d:%d/%d.%d, Dest:%d:%d/%d.%d, Type=2", msg_fzone, msg.orig_net, msg.orig, msg_fpoint, msg_tzone, msg.dest_net, msg.dest, msg_tpoint);

    sprintf(tmpstr, "%02d/%02d/%02d", pkthdr->day, pkthdr->month + 1, pkthdr->year % 100);
    prints(8, 65, YELLOW | _BLACK, tmpstr);
    sprintf(tmpstr, "%02d:%02d:%02d", pkthdr->hour, pkthdr->minute, pkthdr->second);
    prints(9, 65, YELLOW | _BLACK, tmpstr);
    prints(10, 65, YELLOW | _BLACK, "              ");
    sprintf(tmpstr, "%d:%d/%d.%d", msg_fzone, msg.orig_net, msg.orig, msg_fpoint);
    prints(10, 65, YELLOW | _BLACK, tmpstr);

    if (!verify_password(pkthdr->password, msg_fzone, msg.orig_net, msg.orig, msg_fpoint)) {
        status_line("!Packet password error");
        return (2);
    }

    return (process_packet(fp, pkthdr));
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static int process_type2plus_packet(fp, pkthdr, pktname)
FILE * fp;
struct _pkthdr2 * pkthdr;
char * pktname;
{
    char tmpstr[40];

    memset((char *)&msg, 0, sizeof(struct _msg));

    msg.dest_net = pkthdr->dest_net;
    msg.dest = pkthdr->dest_node;
    msg_tzone = pkthdr->dest_zone2;
    msg_tpoint = pkthdr->dest_point;
    pkthdr->dest_zone = pkthdr->dest_zone2;

    pkt_net = msg.orig_net = pkthdr->orig_net;
    pkt_node = msg.orig = pkthdr->orig_node;
    pkt_zone = msg_fzone = pkthdr->orig_zone2;
    pkt_point = msg_fpoint = pkthdr->orig_point;
    pkthdr->orig_zone = pkthdr->orig_zone2;

    status_line("* %s, %02d/%02d/%02d, %02d:%02d, by %02X (%s)", pktname, pkthdr->day, pkthdr->month + 1, pkthdr->year % 100, pkthdr->hour, pkthdr->minute, pkthdr->product, pkthdr->product > isMAX_PRODUCT ? "Unknow" : prodcode[pkthdr->product]);
    status_line("* Orig:%d:%d/%d.%d, Dest:%d:%d/%d.%d, Type=2+", msg_fzone, msg.orig_net, msg.orig, msg_fpoint, msg_tzone, msg.dest_net, msg.dest, msg_tpoint);

    sprintf(tmpstr, "%02d/%02d/%02d", pkthdr->day, pkthdr->month + 1, pkthdr->year % 100);
    prints(8, 65, YELLOW | _BLACK, tmpstr);
    sprintf(tmpstr, "%02d:%02d:%02d", pkthdr->hour, pkthdr->minute, pkthdr->second);
    prints(9, 65, YELLOW | _BLACK, tmpstr);
    prints(10, 65, YELLOW | _BLACK, "              ");
    sprintf(tmpstr, "%d:%d/%d.%d", msg_fzone, msg.orig_net, msg.orig, msg_fpoint);
    prints(10, 65, YELLOW | _BLACK, tmpstr);

    if (!verify_password(pkthdr->password, msg_fzone, msg.orig_net, msg.orig, msg_fpoint)) {
        status_line("!Packet password error");
        return (2);
    }

    return (process_packet(fp, pkthdr));
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static int sort_func(const void * a1, const void * b1)
{
    struct _fwd_alias * a, *b;
    a = (struct _fwd_alias *)a1;
    b = (struct _fwd_alias *)b1;
    if (a->zone != b->zone) {
        return (a->zone - b->zone);
    }
    if (a->net != b->net) {
        return (a->net - b->net);
    }
    if (a->node != b->node) {
        return (a->node - b->node);
    }
    return ((int)(a->point - b->point));
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static int process_packet(FILE * fp, struct _pkthdr2 * pkthdr)
{
    FILE * fpd;
    int cpoint, cnet, cnode, m, i, n_path, n_forw, nmsg, npkt, n_seen;
    char linea[128], last_path[128], addr[128], *p, dupe, bad, tmpforw[128];
    char havepath, pathreached, seenreached, intl, empty, shead, c, *eot;
    long crc, pos;
    struct _msghdr2 msghdr;
    struct _fwd_alias * path, *forward, *seen;

    path = (struct _fwd_alias *)malloc(MAX_PATH * sizeof(struct _fwd_alias));
    if (path == NULL) {
        return (0);
    }
    forward = (struct _fwd_alias *)malloc(MAX_FORWARD * sizeof(struct _fwd_alias));
    if (forward == NULL) {
        free(path);
        return (0);
    }

    seen = (struct _fwd_alias *)malloc(MAX_SEEN * sizeof(struct _fwd_alias));
    if (seen == NULL) {
        free(path);
        free(forward);
        return (0);
    }

    memset((char *)dupecheck, 0, sizeof(struct _dupecheck));
    memset((char *)&sys, 0, sizeof(struct _sys));
    f1 = f2 = f3 = f4 = f5 = NULL;

    fpd = fopen("MSGTMP.IMP", "rb+");
    if (fpd == NULL) {
        fpd = fopen("MSGTMP.IMP", "wb");
        fclose(fpd);
    }
    else {
        fclose(fpd);
    }
    fpd = mopen("MSGTMP.IMP", "rb+");

    npkt = nmsg = 0;
    wclear();

    for (;;) {
        if (fread((char *)&msghdr, sizeof(struct _msghdr2), 1, fp) != 1) {
            break;
        }

        if (msghdr.ver == 0) {
            break;
        }

        if (msghdr.ver != PKTVER) {
            while ((i = fgetc(fp)) != EOF) {
                if ((i & 0xFF) == '\0') {
                    break;
                }
            }
        }
        else {
            bad = dupe = 0;
            empty = 1;

            if (nmsg) {
                wputs("\n");
            }
            sprintf(addr, "%5d  ", ++nmsg);
            wputs(addr);

            msg.orig_net = msghdr.orig_net;
            msg.orig = msghdr.orig_node;
            msg_fpoint = pkt_point;
            if (pkt_zone) {
                msg_fzone = pkthdr->orig_zone;
            }
            else {
                msg_fzone = msg_tzone = config->alias[sys.use_alias].zone;
            }

            memset((char *)&msg.date, 0, 20);
            strcpy(msg.date, get_to_null(fp, linea, 20));
            if (isalpha(msg.date[0]) && msg.date[0] != ' ') {
                strncpy(msg.date, &msg.date[4], 10);
                strcpy(&msg.date[10], &msg.date[13]);
                strcat(msg.date, ":00");
            }
            memset((char *)&msg.to, 0, 36);
            strcpy(msg.to, get_to_null(fp, linea, 35));
            memset((char *)&msg.from, 0, 36);
            strcpy(msg.from, get_to_null(fp, linea, 35));
            memset((char *)&msg.subj, 0, 72);
            strcpy(msg.subj, get_to_null(fp, linea, 71));
            msg.attr = msghdr.attrib;
            msg.attr &= ~(MSGCRASH | MSGSENT | MSGHOLD | MSGFRQ | MSGKILL);
            msg.cost = msghdr.cost;

            crc = compute_crc(msg.date, 0xFFFFFFFFL);
            crc = compute_crc(msg.to, crc);
            crc = compute_crc(msg.from, crc);
            crc = compute_crc(msg.subj, crc);

            pos = ftell(fp);

            for (;;) {
                eot = packet_fgets(linea, 120, fp);
                linea[120] = '\0';
                if (!eot) {
                    break;
                }
                c = fgetc(fp);
                if (c == EOF) {
                    break;
                }
                ungetc(c, fp);
                if (!c) {
                    break;
                }
                if (!strncmp(linea, "\01MSGID", 6)) {
                    p = strtok(linea, " ");
                    if (p) {
                        p = strtok(NULL, " ");
                    }
                    if (p) {
                        p = strtok(NULL, " ");
                    }
                    if (p) {
                        strupr(p);
                        sscanf(p, "%08lX", &crc);
//									status_line("DMSGID %08lX",crc);
                        sprintf(linea, "%d%d", msg.orig_net, msg.orig);
                        crc = compute_crc(linea, crc);
//									status_line("D -> %08lX",crc);
                    }
                    break;
                }
            }

            fseek(fp, pos, SEEK_SET);

            packet_fgets(linea, 120, fp);
            linea[120] = '\0';

            if (!strncmp(linea, "AREA:", 5)) {
                sys.netmail = 0;
                sys.echomail = 1;
                if (linea[0]) {
                    while (strlen(linea) > 0 && (linea[strlen(linea) - 1] == 0x0D || linea[strlen(linea) - 1] == 0x0A)) {
                        linea[strlen(linea) - 1] = '\0';
                    }
                }

                p = strbtrim(&linea[5]);

                sprintf(addr, "%-32.32s ", strupr(p));
                wputs(addr);

                if (stricmp(p, dupecheck->areatag) != 0) {
                    if (dupecheck->areatag[0]) {
                        if (npkt && strlen(sys.echotag)) {
                            status_line(":   %-20.20s (Toss=%04d)", sys.echotag, npkt);
                        }
                        npkt = 0;
                        write_dupe_check();
                    }

                    if (!read_dupe_check(p)) {
                        sprintf(addr, "%-12.12s ", "(Bad area)");
                        wputs(addr);
                        bad = 1;
                    }
                    else {
                        n_forw = 0;
                        forward[n_forw].zone = config->alias[sys.use_alias].zone;
                        forward[n_forw].net = config->alias[sys.use_alias].net;
                        forward[n_forw].node = config->alias[sys.use_alias].node;
                        forward[n_forw].point = config->alias[sys.use_alias].point;
                        forward[n_forw].receive = 0;

                        strcpy(tmpforw, sys.forward1);
                        p = strtok(tmpforw, " ");
                        if (p != NULL)
                            do {
                                if (n_forw) {
                                    forward[n_forw].zone = forward[n_forw - 1].zone;
                                    forward[n_forw].net = forward[n_forw - 1].net;
                                    forward[n_forw].node = forward[n_forw - 1].node;
                                    forward[n_forw].point = forward[n_forw - 1].point;
                                }
                                forward[n_forw].receive = 0;
                                if (strstr(p, ">")) {
                                    forward[n_forw].receive = 1;
                                }
                                while (*p == '<' || *p == '>' || *p == '!' || *p == 'P' || *p == 'p') {
                                    p++;
                                }
                                parse_netnode2(p, (int *)&forward[n_forw].zone, (int *)&forward[n_forw].net, (int *)&forward[n_forw].node, (int *)&forward[n_forw].point);
                                n_forw++;
                            } while ((p = strtok(NULL, " ")) != NULL);
                        strcpy(tmpforw, sys.forward2);
                        p = strtok(tmpforw, " ");
                        if (p != NULL)
                            do {
                                if (n_forw) {
                                    forward[n_forw].zone = forward[n_forw - 1].zone;
                                    forward[n_forw].net = forward[n_forw - 1].net;
                                    forward[n_forw].node = forward[n_forw - 1].node;
                                    forward[n_forw].point = forward[n_forw - 1].point;
                                }
                                forward[n_forw].receive = 0;
                                if (strstr(p, ">")) {
                                    forward[n_forw].receive = 1;
                                }
                                while (*p == '<' || *p == '>' || *p == '!' || *p == 'P' || *p == 'p') {
                                    p++;
                                }
                                parse_netnode2(p, (int *)&forward[n_forw].zone, (int *)&forward[n_forw].net, (int *)&forward[n_forw].node, (int *)&forward[n_forw].point);
                                n_forw++;
                            } while ((p = strtok(NULL, " ")) != NULL);
                        strcpy(tmpforw, sys.forward3);
                        p = strtok(tmpforw, " ");
                        if (p != NULL)
                            do {
                                if (n_forw) {
                                    forward[n_forw].zone = forward[n_forw - 1].zone;
                                    forward[n_forw].net = forward[n_forw - 1].net;
                                    forward[n_forw].node = forward[n_forw - 1].node;
                                    forward[n_forw].point = forward[n_forw - 1].point;
                                }
                                forward[n_forw].receive = 0;
                                if (strstr(p, ">")) {
                                    forward[n_forw].receive = 1;
                                }
                                while (*p == '<' || *p == '>' || *p == '!' || *p == 'P' || *p == 'p') {
                                    p++;
                                }
                                parse_netnode2(p, (int *)&forward[n_forw].zone, (int *)&forward[n_forw].net, (int *)&forward[n_forw].node, (int *)&forward[n_forw].point);
                                n_forw++;
                            } while ((p = strtok(NULL, " ")) != NULL);
                    }
                }

                if (!bad) {
                    for (i = 0; i < n_forw; i++) {
                        if (forward[i].zone == msg_fzone && forward[i].net == msghdr.orig_net && forward[i].node == msghdr.orig_node && forward[i].point == msg_fpoint) {
                            break;
                        }
                    }


                    if (i == n_forw && config->secure) {
                        sprintf(addr, "%-12.12s ", "(Bad origin)");
                        wputs(addr);
                        bad = 1;
                    }

                    if (i != n_forw && forward[i].receive) {
                        sprintf(addr, "%-12.12s ", "(Bad: R/O  )");
                        wputs(addr);
                        bad = 1;
                    }

                }

                if (!bad) {
                    crc = compute_crc(sys.echotag, crc);

                    if (check_and_add_dupe(crc)) {
                        sprintf(addr, "%-12.12s ", "(Duplicate)");
                        wputs(addr);
                        dupe = 1;
                    }
                    else {
                        dupe = 0;
                    }
                }

                linea[0] = '\0';
                havepath = 1;
                pathreached = 0;
            }
            else {

                int msgid, intl, reply;
                char * eot, c;
                int zo, ne, no, po;

                fseek(fp, pos, SEEK_SET);
                if (!sys.netmail) {

                    if (dupecheck->areatag[0]) {
                        if (npkt && strlen(sys.echotag)) {
                            status_line(":   %-20.20s (Toss=%04d)", sys.echotag, npkt);
                        }
                        npkt = 0;
                        write_dupe_check();
                    }

                    memset((char *)&sys, 0, sizeof(struct _sys));
                    sys.netmail = 1;
                    sys.echomail = 0;
                    strcpy(sys.msg_name, "Netmail");
                    strcpy(sys.msg_path, config->netmail_dir);

                    scan_message_base(0, 0);

                    read_dupe_check("Netmail");
                }

                crc = compute_crc(sys.msg_name, crc);
                check_and_add_dupe(crc);

                sprintf(addr, "%-32.32s ", "Netmail");
                wputs(addr);
                dupe = 0;

                havepath = 0;
                pathreached = 1;
                status_line("*Netmail: %s -> %s", msg.from, msg.to);

                msg.orig_net = msghdr.orig_net;
                msg.orig = msghdr.orig_node;
                msg.dest_net = msghdr.dest_net;
                msg.dest = msghdr.dest_node;
                if (pkthdr->dest_zone) {
                    msg_tzone = pkthdr->dest_zone;
                }
                else if (pkthdr->dest_zone2) {
                    msg_tzone = pkthdr->dest_zone2;
                }
                else {
                    msg_tzone = config->alias[sys.use_alias].zone;
                }
                msg_tpoint = pkthdr->dest_point;
                if (pkthdr->orig_zone) {
                    msg_fzone = pkthdr->orig_zone;
                }
                else if (pkthdr->orig_zone2) {
                    msg_fzone = pkthdr->orig_zone2;
                }
                else {
                    msg_fzone = msg_tzone = config->alias[sys.use_alias].zone;
                }
                msg_fpoint = pkthdr->orig_point;


                intl = msgid = reply = 0;

                do {

                    eot = packet_fgets(linea, 120, fp);
                    linea[120] = '\0';
                    if (!eot) {
                        break;
                    }
                    c = fgetc(fp);
                    if (c == EOF) {
                        break;
                    }
                    ungetc(c, fp);
                    if (!c) {
                        break;
                    }
                    if (!strncmp(linea, "\01INTL", 5)) {
                        p = strtok(linea, " ");
                        if (p) {
                            p = strtok(NULL, " ");
                        }
                        if (p) {
                            parse_netnode(p, &msg_tzone, (int *)&msg.dest_net, (int *)&msg.dest, &i);
                            p = strtok(NULL, " ");
                            if (p) {
                                parse_netnode(p, &msg_fzone, (int *)&msg.orig_net, (int *)&msg.orig, &i);
                            }
                        }
//						sscanf (&linea[6], "%d:%d/%d %d:%d/%d", &msg_tzone, &msg.dest_net, &msg.dest, &msg_fzone, &msg.orig_net, &msg.orig);
                        intl = 1;
                        continue;
                    }

                    if (!strncmp(linea, "\01TOPT", 5)) {
                        sscanf(&linea[6], "%d", &msg_tpoint);
                        continue;
                    }
                    if (!strncmp(linea, "\01MSGID", 6)) {
                        msgid = 1;
                        if (strstr(linea, "/")) {
                            if (!intl) {
                                int i;
//								parse_netnode (&linea[7], &msg_fzone, (int *)&msg.orig_net, (int *)&msg.orig, &po);
                                parse_netnode(&linea[7], &msg_fzone, &i, &i, &po);
                                if (!reply) {
                                    msg_tzone = msg_fzone;
                                }
                            }
                        }
                        continue;
                    }

                    if (!strncmp(linea, "\01FMPT", 5)) {
                        sscanf(&linea[6], "%d", &msg_fpoint);
                        continue;
                    }
                    /*
                    					if (!strncmp(linea,"\01REPLY",6)) {
                    						reply=1;
                    						if (strstr(linea,"/")){
                    							if(!intl){
                                            int i;
                    //                      parse_netnode (&linea[7], &msg_tzone, (int *)&msg.dest_net, (int *)&msg.dest, &po);
                                            parse_netnode (&linea[7], &msg_tzone, &i, &i, &i);

                    								if(!msgid)
                    									msg_fzone = msg_tzone;
                    							}
                    						}
                    						continue;
                    					}
                    */
                } while (c && eot && (c != EOF));

                status_line("*         %d:%d/%d.%d -> %d:%d/%d.%d", msg_fzone, msg.orig_net, msg.orig, msg_fpoint, msg_tzone, msg.dest_net, msg.dest, msg_tpoint);
                fseek(fp, pos, SEEK_SET);
            }

            mseek(fpd, 0L, SEEK_SET);

            if (dupe) {
                if (dupes != NULL) {
                    sprintf(addr, "%5d", ndupes + 1);
                    wputs(addr);
                }
                else {
                    wputs(" ----");
                }

                sysinfo.today.dupes++;
                sysinfo.week.dupes++;
                sysinfo.month.dupes++;
                sysinfo.year.dupes++;
                totaldupes++;
            }

            else if (bad) {
                if (bad_msgs != NULL && *bad_msgs) {
                    sprintf(addr, "%5d", nbad + 1);
                    wputs(addr);
                }
                else {
                    wputs(" ----");
                }

                sysinfo.today.bad++;
                sysinfo.week.bad++;
                sysinfo.month.bad++;
                sysinfo.year.bad++;
                totalbad++;
            }

            else if (sys.passthrough) {
                npkt++;
                wputs("Passthrough   ----");
            }

            else {
                if (sys.quick_board) {
                    sprintf(addr, "QuickBBS     %5d", last_msg + 1);
                    wputs(addr);
                }
                else if (sys.gold_board) {
                    sprintf(addr, "GoldBase     %5d", last_msg + 1);
                    wputs(addr);
                }
                else if (sys.pip_board) {
                    sprintf(addr, "Pip-Base     %5d", last_msg + 1);
                    wputs(addr);
                }
                else if (sys.squish) {
                    sprintf(addr, "Squish<tm>   %5d", last_msg + 1);
                    wputs(addr);
                }
                else {
                    sprintf(addr, "Fido         %5d", last_msg + 1);
                    wputs(addr);
                }
            }

            last_path[0] = '\0';
            if (linea[0]) {
//				mprintf (fpd, "%s", linea);
                if (linea[0] != '\r' && linea[0] != '\n') {
                    if (linea[0] != 0x01 && strncmp(linea, "SEEN-BY: ", 9)) {
                        empty = 0;
                    }
                }
            }

            n_seen = n_path = 0;
            seenreached = 0;
            intl = shead = 0;

            while (packet_fgets(linea, 120, fp) != NULL) {
                if (linea[0] == '\0') {
                    break;
                }
                linea[120] = '\0';

                if (empty && linea[0] != '\r' && linea[0] != '\n') {
                    if (linea[0] != 0x01 && strncmp(linea, "SEEN-BY: ", 9)) {
                        empty = 0;
                    }
                }

                if (sys.netmail) {

                    char * x_flags;

                    x_flags = strstr(linea, "\001FLAGS");
                    if (x_flags && (linea[0] == 0x01) && (strstr(linea, "IMM") || strstr(linea, "DIR"))) {
                        char * pp;

                        if ((pp = strstr(linea, "IMM")) != 0) {
                            *(pp++) = ' ';
                            *(pp++) = ' ';
                            *pp     = ' ';
                            status_line("!Deleted IMM Xflag");
                        }
                        if ((pp = strstr(linea, "DIR")) != 0) {
                            *(pp++) = ' ';
                            *(pp++) = ' ';
                            *pp     = ' ';
                            status_line("!Deleted DIR Xflag");
                        }
                    }

                    if (!intl) {
                        if (linea[0] != 0x01) {
                            if (msg_fzone != config->alias[sys.use_alias].zone || msg_fzone != msg_tzone) {
                                mprintf(fpd, msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig);
                            }
                            intl = 1;
                        }
                        else {
                            if (!strncmp(linea, "\001INTL ", 6)) {
                                intl = 1;
                            }
                        }
                    }

                    if (!shead && linea[0] != 0x01) {
                        shead = 1;
                        if (config->msgtrack) {
                            track_inbound_messages(fpd, &msg, msg_fzone);
                        }
                    }
                }

                if (!seenreached && strncmp(linea, "\001PATH: ", 7) && strncmp(linea, "SEEN-BY: ", 9)) {
                    mprintf(fpd, "%s", linea);
                }
                else if (!strncmp(linea, "\001PATH: ", 7) && !sys.netmail) {
                    pathreached = 1;
                    seenreached = 1;

                    if (!sys.passthrough && !last_path[0] && !config->single_pass) {
                        strcpy(last_path, linea);

                        if (config->alias[sys.use_alias].point && config->alias[sys.use_alias].fakenet) {
                            seen[n_seen].zone = config->alias[sys.use_alias].zone;
                            seen[n_seen].net = config->alias[sys.use_alias].fakenet;
                            seen[n_seen].node = config->alias[sys.use_alias].point;
                            seen[n_seen].point = 0;
                        }
                        else {
                            seen[n_seen].zone = config->alias[sys.use_alias].zone;
                            seen[n_seen].net = config->alias[sys.use_alias].net;
                            seen[n_seen].node = config->alias[sys.use_alias].node;
                            seen[n_seen].point = config->alias[sys.use_alias].point;
                        }
                        n_seen++;

                        seen[n_seen].zone = msg_fzone;
                        seen[n_seen].net = msg.orig_net;
                        seen[n_seen].node = msg.orig;
                        seen[n_seen].point = msg_fpoint;
                        n_seen++;

                        qsort(seen, n_seen, sizeof(struct _fwd_alias), sort_func);

                        cpoint = cnet = cnode = 0;
                        strcpy(linea, "SEEN-BY: ");

                        for (m = 0; m < n_seen; m++) {
                            if (strlen(linea) > 65) {
                                mprintf(fpd, "%s\r\n", linea);
                                cpoint = cnet = cnode = 0;
                                strcpy(linea, "SEEN-BY: ");
                            }
                            if (m && seen[m].net == seen[m - 1].net && seen[m].node == seen[m - 1].node && seen[m].point == seen[m - 1].point && m < n_seen) {
                                continue;
                            }

                            if (cnet != seen[m].net) {
                                if (seen[m].point) {
                                    sprintf(addr, "%d/%d.%d ", seen[m].net, seen[m].node, seen[m].point);
                                }
                                else {
                                    sprintf(addr, "%d/%d ", seen[m].net, seen[m].node);
                                }
                                cnet = seen[m].net;
                                cnode = seen[m].node;
                                cpoint = seen[m].point;
                            }
                            else if (cnet == seen[m].net && cnode != seen[m].node) {
                                if (seen[m].point) {
                                    sprintf(addr, "%d.%d ", seen[m].node, seen[m].point);
                                }
                                else {
                                    sprintf(addr, "%d ", seen[m].node);
                                }
                                cnode = seen[m].node;
                                cpoint = seen[m].point;
                            }
                            else {
                                if (seen[m].point && cpoint != seen[m].point) {
                                    sprintf(addr, ".%d ", seen[m].point);
                                    cpoint = seen[m].point;
                                }
                                else {
                                    strcpy(addr, "");
                                }
                            }

                            strcat(linea, addr);
                        }

                        if (strlen(linea) > 9) {
                            mprintf(fpd, "%s\r\n", linea);
                        }
                        strcpy(linea, last_path);
                    }

                    path[n_path].zone = config->alias[sys.use_alias].zone;
                    if (config->alias[sys.use_alias].point && config->alias[sys.use_alias].fakenet) {
                        path[n_path].net = config->alias[sys.use_alias].fakenet;
                        path[n_path].node = config->alias[sys.use_alias].point;
                        path[n_path].point = 0;
                    }
                    else {
                        path[n_path].net = config->alias[sys.use_alias].net;
                        path[n_path].node = config->alias[sys.use_alias].node;
                        path[n_path].point = config->alias[sys.use_alias].point;
                    }

                    if (linea[0]) {
                        while (strlen(linea) > 0 && (linea[strlen(linea) - 1] == 0x0D || linea[strlen(linea) - 1] == 0x0A)) {
                            linea[strlen(linea) - 1] = '\0';
                        }
                    }
                    p = strtok(linea, " ");
                    while ((p = strtok(NULL, " ")) != NULL) {
                        if (n_path) {
                            path[n_path].net = path[n_path - 1].net;
                        }
                        path[n_path].zone = config->alias[sys.use_alias].zone;
                        parse_netnode2(p, (int *)&path[n_path].zone, (int *)&path[n_path].net, (int *)&path[n_path].node, (int *)&path[n_path].point);
                        if (n_path && path[n_path].net == path[n_path - 1].net && path[n_path].node == path[n_path - 1].node) {
                            continue;
                        }
                        n_path++;
                    }
                }
                else if (!strncmp(linea, "SEEN-BY: ", 9) && !sys.netmail) {
                    seenreached = 1;

                    seen[n_seen].zone = config->alias[sys.use_alias].zone;
                    if (config->alias[sys.use_alias].point && config->alias[sys.use_alias].fakenet) {
                        seen[n_seen].net = config->alias[sys.use_alias].fakenet;
                        seen[n_seen].node = config->alias[sys.use_alias].point;
                        seen[n_seen].point = 0;
                    }
                    else {
                        seen[n_seen].net = config->alias[sys.use_alias].net;
                        seen[n_seen].node = config->alias[sys.use_alias].node;
                        seen[n_seen].point = config->alias[sys.use_alias].point;
                    }

                    if (linea[0]) {
                        while (strlen(linea) > 0 && (linea[strlen(linea) - 1] == 0x0D || linea[strlen(linea) - 1] == 0x0A)) {
                            linea[strlen(linea) - 1] = '\0';
                        }
                    }
                    p = strtok(linea, " ");
                    while ((p = strtok(NULL, " ")) != NULL) {
                        if (n_seen + 5 >= MAX_SEEN) {
                            break;
                        }

                        if (n_seen) {
                            seen[n_seen].net = seen[n_seen - 1].net;
                        }
                        seen[n_seen].zone = config->alias[sys.use_alias].zone;
                        parse_netnode2(p, (int *)&seen[n_seen].zone, (int *)&seen[n_seen].net, (int *)&seen[n_seen].node, (int *)&seen[n_seen].point);
                        if (n_seen && seen[n_seen].zone == seen[n_seen - 1].zone && seen[n_seen].net == seen[n_seen - 1].net && seen[n_seen].node == seen[n_seen - 1].node && seen[n_seen].point == seen[n_seen - 1].point) {
                            continue;
                        }
                        n_seen++;
                    }
                }
            }

            if (!sys.netmail && !sys.passthrough && !config->single_pass) {
                if (havepath && !pathreached) {
                    if (config->alias[sys.use_alias].point && config->alias[sys.use_alias].fakenet) {
                        seen[n_seen].zone = config->alias[sys.use_alias].zone;
                        seen[n_seen].net = config->alias[sys.use_alias].fakenet;
                        seen[n_seen].node = config->alias[sys.use_alias].point;
                        seen[n_seen].point = 0;
                    }
                    else {
                        seen[n_seen].zone = config->alias[sys.use_alias].zone;
                        seen[n_seen].net = config->alias[sys.use_alias].net;
                        seen[n_seen].node = config->alias[sys.use_alias].node;
                        seen[n_seen].point = config->alias[sys.use_alias].point;
                    }
                    n_seen++;

                    seen[n_seen].zone = msg_fzone;
                    seen[n_seen].net = msg.orig_net;
                    seen[n_seen].node = msg.orig;
                    seen[n_seen].point = msg_fpoint;
                    n_seen++;

                    qsort(seen, n_seen, sizeof(struct _fwd_alias), sort_func);

                    cpoint = cnet = cnode = 0;
                    strcpy(linea, "SEEN-BY: ");

                    for (m = 0; m < n_seen; m++) {
                        if (strlen(linea) > 65) {
                            mprintf(fpd, "%s\r\n", linea);
                            cpoint = cnet = cnode = 0;
                            strcpy(linea, "SEEN-BY: ");
                        }
                        if (m && seen[m].net == seen[m - 1].net && seen[m].node == seen[m - 1].node && seen[m].point == seen[m - 1].point && m < n_seen) {
                            continue;
                        }

                        if (cnet != seen[m].net && cnode != seen[m].node) {
                            if (seen[m].point) {
                                sprintf(addr, "%d/%d.%d ", seen[m].net, seen[m].node, seen[m].point);
                            }
                            else {
                                sprintf(addr, "%d/%d ", seen[m].net, seen[m].node);
                            }
                            cnet = seen[m].net;
                            cnode = seen[m].node;
                            cpoint = seen[m].point;
                        }
                        else if (cnet == seen[m].net && cnode != seen[m].node) {
                            if (seen[m].point) {
                                sprintf(addr, "%d.%d ", seen[m].node, seen[m].point);
                            }
                            else {
                                sprintf(addr, "%d ", seen[m].node);
                            }
                            cnode = seen[m].node;
                            cpoint = seen[m].point;
                        }
                        else {
                            if (seen[m].point && cpoint != seen[m].point) {
                                sprintf(addr, ".%d ", seen[m].point);
                                cpoint = seen[m].point;
                            }
                            else {
                                strcpy(addr, "");
                            }
                        }

                        strcat(linea, addr);
                    }

                    if (strlen(linea) > 9) {
                        mprintf(fpd, "%s\r\n", linea);
                    }
                }

                path[n_path].zone = config->alias[sys.use_alias].zone;
                if (config->alias[sys.use_alias].point && config->alias[sys.use_alias].fakenet) {
                    path[n_path].net = config->alias[sys.use_alias].fakenet;
                    path[n_path].node = config->alias[sys.use_alias].point;
                    path[n_path].point = 0;
                }
                else {
                    path[n_path].net = config->alias[sys.use_alias].net;
                    path[n_path].node = config->alias[sys.use_alias].node;
                    path[n_path].point = config->alias[sys.use_alias].point;
                }
                n_path++;

                i = 0;
                do {
                    cpoint = cnet = cnode = 0;
                    strcpy(linea, "\001PATH: ");
                    do {
                        if (cnet != path[i].net) {
                            sprintf(addr, "%d/%d ", path[i].net, path[i].node);
                            cnet = path[i].net;
                            cnode = path[i].node;
                        }
                        else if (cnode != path[i].node) {
                            sprintf(addr, "%d ", path[i].node);
                            cnet = path[i].net;
                            cnode = path[i].node;
                        }
                        else {
                            strcpy(addr, "");
                        }

                        strcat(linea, addr);
                        i++;
                    } while (i < n_path && strlen(linea) < 70);

                    mprintf(fpd, "%s\r\n", linea);
                } while (i < n_path);
            }

            if (sys.netmail) {
                for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++) {
                    if (config->alias[i].zone == msg_fzone) {
                        break;
                    }
                }
                if (!(i < MAX_ALIAS && config->alias[i].net)) {
                    i = sys.use_alias;
                }
                crc = time(NULL);
                sprintf(linea, "\x01Via %s on %d:%d/%d.%d, %s", VERSION, config->alias[i].zone, config->alias[i].net, config->alias[i].node, config->alias[i].point, ctime(&crc));
                while (strlen(linea) > 0 && (linea[strlen(linea) - 1] == 0x0D || linea[strlen(linea) - 1] == 0x0A)) {
                    linea[strlen(linea) - 1] = '\0';
                }
                mprintf(fpd, "%s\r\n", linea);
            }

            if (!sys.passthrough && !config->single_pass) {
                mseek(fpd, 0L, SEEK_SET);
            }

            if (sys.echomail && n_forw == 1) {
                msg.attr |= MSGSENT;
            }

            if (sys.netmail) {
                for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++) {
                    if (config->alias[i].point && config->alias[i].fakenet) {
                        if (config->alias[i].zone == msg_tzone && config->alias[i].fakenet == msg.dest_net && config->alias[i].point == msg.dest) {
                            break;
                        }
                    }
                    if (config->alias[i].zone == msg_tzone && config->alias[i].net == msg.dest_net && config->alias[i].node == msg.dest && config->alias[i].point == msg_tpoint) {
                        break;
                    }
                }
                if (i >= MAX_ALIAS || !config->alias[i].net) {
                    msg.attr |= MSGFWD;
                    if (!config->keeptransit) {
                        msg.attr |= MSGKILL;
                    }
                }
            }

            time_release();
            i = 0;

            if (!empty || config->keep_empty) {
                if (dupe) {
                    if (dupes != NULL) {
                        i = fido_save_message2(fpd, dupes);
                    }
                    else {
                        i = 1;
                    }
                }

                else if (bad) {
                    if (bad_msgs != NULL && *bad_msgs) {
                        i = fido_save_message2(fpd, bad_msgs);
                    }
                    else {
                        i = 1;
                    }
                }

                else if (sys.passthrough) {
                    i = passthrough_save_message(fpd, seen, n_seen, path, n_path);
                }

                else {
                    if (!sys.netmail) {
                        npkt++;
                    }

                    i = 1;
                    if (config->single_pass) {
                        i = passthrough_save_message(fpd, seen, n_seen, path, n_path);
                        mseek(fpd, 0L, SEEK_SET);
                    }

                    if (i) {
                        if (sys.quick_board || sys.gold_board) {
                            i = quick_save_message2(fpd, f1, f2, f3, f4);
                        }

                        else if (sys.pip_board) {
                            i = pip_save_message2(fpd, f1, f2, f3);
                        }

                        else if (sys.squish) {
                            i = squish_save_message2(fpd);
                        }

                        else {
                            i = fido_save_message2(fpd, NULL);
                        }
                    }

                    if (sys.netmail) {
                        totalnetmail++;
                    }
                    else {
                        totalechomail++;
                    }

                    if (!stricmp(config->sysop, msg.to)) {
                        if (config->save_my_mail && config->my_mail[0]) {
                            mseek(fpd, 0L, SEEK_SET);
                            fido_save_message3(fpd);
                        }
                    }
                }
            }

            if (empty) {
                status_line("!Empty message");
                wputs("               Skip");
            }
            else if (i) {
                wputs("                 Ok");
            }
            else {
                wputs("              Error");
                printf("\a");
                sleep(10);
                return (0);
            }

            if ((totalmsg % 4) == 0) {
                time_release();
            }

            totalmsg++;
            sprintf(linea, "%d (%.1f/s) ", totalmsg, (float)totalmsg / ((float)(timerset(0) - totaltime) / 100));
            prints(11, 65, YELLOW | _BLACK, linea);
        }
    }

    if (dupecheck->areatag[0]) {
        write_dupe_check();
    }

    if (npkt && sys.echomail && strlen(sys.echotag)) {
        status_line(":   %-20.20s (Toss=%04d)", sys.echotag, npkt);
    }

    totalpackets++;
    time_release();

    if (f1 != NULL) {
        fclose(f1);
    }
    if (f2 != NULL) {
        fclose(f2);
    }
    if (f3 != NULL) {
        fclose(f3);
    }
    if (f4 != NULL) {
        fclose(f4);
    }
    if (f5 != NULL) {
        fseek(f5, 0L, SEEK_SET);
        if (sys.gold_board) {
            fwrite((char *)&gmsginfo, sizeof(struct _gold_msginfo), 1, f5);
        }
        else {
            fwrite((char *)&msginfo, sizeof(struct _msginfo), 1, f5);
        }
        fclose(f5);
    }
    f1 = f2 = f3 = f4 = f5 = NULL;

    mclose(fpd);
    free(seen);
    free(forward);
    free(path);

    return (1);
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static char * get_to_null(fp, dest, max)
FILE * fp;
char * dest;
int max;
{
    int i, c;

    i = 0;

    do {
        if ((c = fgetc(fp)) == EOF) {
            break;
        }
        if (i < max) {
            dest[i++] = (char)c;
        }
    } while ((char)c != '\0');

    dest[i] = '\0';

    return (dest);
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static int read_dupe_check(area)
char * area;
{
    FILE * fp;
    int fd, found = 0, i, zo, ne, no, po;
    char filename[80], ptype, linea[256], *location, *tag, *forward, *p;
    struct _sys tsys;
    struct _dupeindex dupeindex[MAX_DUPEINDEX];

    if (sys.quick_board) {
        ptype = 1;
    }
    else if (sys.gold_board) {
        ptype = 3;
    }
    else if (sys.pip_board) {
        ptype = 2;
    }
    else if (!sys.passthrough) {
        ptype = 0;
    }
    else {
        ptype = -1;
    }

    if (sys.echomail) {
        if (fpareas != NULL) {
            rewind(fpareas);
            fp = fpareas;

            fgets(linea, 250, fp);

            while (fgets(linea, 250, fp) != NULL) {
                if (linea[0] == ';' || linea[0] == '\0') {
                    continue;
                }
                while (strlen(linea) > 0 && (linea[strlen(linea) - 1] == 0x0D || linea[strlen(linea) - 1] == 0x0A)) {
                    linea[strlen(linea) - 1] = '\0';
                }
                if ((location = strtok(linea, " ")) == NULL) {
                    continue;
                }
                if ((tag = strtok(NULL, " ")) == NULL) {
                    continue;
                }
                if ((forward = strtok(NULL, "")) == NULL) {
                    continue;
                }
                while (*forward == ' ') {
                    forward++;
                }
                if (!stricmp(tag, area)) {
                    memset((char *)&sys, 0, sizeof(struct _sys));

                    sys.echomail = 1;
                    strcpy(sys.echotag, tag);
                    if (forward != NULL) {
                        strcpy(sys.forward1, forward);
                    }

                    zo = config->alias[sys.use_alias].zone;
                    parse_netnode2(sys.forward1, &zo, &ne, &no, &po);
                    if (zo != config->alias[sys.use_alias].zone) {
                        for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++)
                            if (zo == config->alias[i].zone) {
                                break;
                            }
                        if (i < MAX_ALIAS && config->alias[i].net) {
                            sys.use_alias = i;
                        }
                    }

                    if (!strcmp(location, "##")) {
                        sys.passthrough = 1;
                    }
                    else if (atoi(location)) {
                        sys.quick_board = atoi(location);
                    }
                    else if (toupper(location[0]) == 'G' && atoi(&location[1])) {
                        sys.gold_board = atoi(&location[1]);
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

                    found = 1;
                    break;
                }
            }
        }

        if (!found) {
            for (i = 0; i < maxaidx; i++) {
                if (aidx[i].areatag == get_buffer_crc(area, strlen(area))) {
                    break;
                }
            }

            if (i < maxaidx) {
                lseek(fdsys, (long)i * SIZEOF_MSGAREA, SEEK_SET);
                read(fdsys, (char *)&tsys.msg_name, SIZEOF_MSGAREA);
                found = 1;
            }

            if (!found) {
                i = 0;

                if (config->newareas_create[0]) {
                    zo = config->alias[sys.use_alias].zone;
                    ne = config->alias[sys.use_alias].net;
                    no = config->alias[sys.use_alias].node;
                    po = config->alias[sys.use_alias].point;
                    i = 0;

                    strcpy(linea, config->newareas_create);
                    p = strtok(linea, " ");
                    if (p != NULL)
                        do {
                            parse_netnode2(p, &zo, &ne, &no, &po);
                            if (zo == msg_fzone && ne == msg.orig_net && no == msg.orig && po == msg_fpoint) {
                                i = 1;
                                break;
                            }
                        } while ((p = strtok(NULL, " ")) != NULL);
                }

                if (i) {
                    memset((char *)&tsys, 0, sizeof(struct _sys));
                    tsys.msg_priv = SYSOP;
                    strcpy(tsys.echotag, area);
                    strcpy(tsys.msg_name, area);
                    strcat(tsys.msg_name, " (New area)");

                    strcpy(filename, tsys.echotag);
                    strsrep(filename, ".", "_");
                    if (strlen(filename) > 8) {
                        filename[8] = '\0';
                    }

                    strcpy(linea, config->newareas_path);
                    strcat(linea, filename);

                    if (config->newareas_base == 0) {
                        if (mkdir(linea) == -1) {
                            strcpy(filename, tsys.echotag);
                            strsrep(filename, ".", "_");
                            if (strlen(filename) > 8) {
                                filename[8] = '.';
                                filename[12] = '\0';
                            }
                            else {
                                strcat(filename, ".000");
                            }

                            strcpy(linea, config->newareas_path);
                            strcat(linea, filename);

                            if (mkdir(linea) == -1) {
                                linea[strlen(linea) - 1] = 47;
                                do {
                                    linea[strlen(linea) - 1]++;
                                } while (mkdir(linea) == -1);
                            }
                        }

                        strcat(linea, "\\");
                        strcpy(tsys.msg_path, linea);
                    }
                    else if (config->newareas_base == 1) {
                        tsys.quick_board = check_board_not_used(0, fpareas);
                    }
                    else if (config->newareas_base == 2) {
                        tsys.gold_board = check_board_not_used(1, fpareas);
                    }
                    else if (config->newareas_base == 3) {
                        tsys.squish = 1;

                        strcpy(filename, linea);
                        strcat(filename, ".SQD");

                        if (dexists(filename)) {
                            linea[strlen(linea) - 1] = 47;
                            do {
                                linea[strlen(linea) - 1]++;
                                strcpy(filename, linea);
                                strcat(filename, ".SQD");
                            } while (dexists(filename));
                        }

                        strcpy(tsys.msg_path, linea);
                    }
                    else if (config->newareas_base == 4) {
                        tsys.pip_board = 0;
                        do {
                            sprintf(filename, "%sMPKT%04x.PIP", config->pip_msgpath, ++tsys.pip_board);
                        } while (dexists(filename));
                    }
                    else if (config->newareas_base == 5) {
                        tsys.passthrough = 1;
                    }

                    tsys.echomail = 1;
                    sprintf(tsys.forward1, "%d:%d/%d ", msg_fzone, msg.orig_net, msg.orig);
                    if (config->newareas_link[0]) {
                        strcat(tsys.forward1, config->newareas_link);
                    }

                    if (fpareas != NULL) {
                        fclose(fpareas);
                    }

                    if (config->use_areasbbs) {
                        fp = fopen(config->areas_bbs, "at");
                    }
                    else {
                        fp = fopen("NEWAREAS.BBS", "at");
                    }

                    if (fp != NULL) {
                        if (fpareas == NULL) {
                            fprintf(fp, ";\n");
                        }

                        if (tsys.passthrough) {
                            fprintf(fp, "##%-28.28s %-22.22s %s\n", "", tsys.echotag, tsys.forward1);
                        }
                        else if (tsys.quick_board) {
                            fprintf(fp, "%-30d %-22.22s %s\n", tsys.quick_board, tsys.echotag, tsys.forward1);
                        }
                        else if (tsys.gold_board) {
                            fprintf(fp, "G%-29d %-22.22s %s\n", tsys.gold_board, tsys.echotag, tsys.forward1);
                        }
                        else if (tsys.pip_board) {
                            fprintf(fp, "!%-29d %-22.22s %s\n", tsys.pip_board, tsys.echotag, tsys.forward1);
                        }
                        else if (tsys.squish) {
                            fprintf(fp, "$%-29.29s %-22.22s %s\n", tsys.msg_path, tsys.echotag, tsys.forward1);
                        }
                        else {
                            fprintf(fp, "%-30.30s %-22.22s %s\n", tsys.msg_path, tsys.echotag, tsys.forward1);
                        }

                        fclose(fp);
                    }

                    if (config->use_areasbbs) {
                        fpareas = fopen(config->areas_bbs, "rt");
                    }
                    else {
                        fpareas = fopen("NEWAREAS.BBS", "rt");
                    }
                }
                else {
                    strcpy(sys.echotag, area);
                    sys.echomail = 1;
                    return (0);
                }
            }

            memcpy((char *)&sys, (char *)&tsys, sizeof(struct _sys));
        }
    }

    if (!sys.passthrough) {
        if (sys.quick_board) {
            if (ptype != 1) {
                if (ptype == 3) {
                    fseek(f5, 0L, SEEK_SET);
                    fwrite((char *)&gmsginfo, sizeof(struct _gold_msginfo), 1, f5);
                    fclose(f1);
                    fclose(f2);
                    fclose(f3);
                    fclose(f4);
                    fclose(f5);
                }
                else if (ptype == 2) {
                    fclose(f1);
                    fclose(f2);
                    fclose(f3);
                }

                if (sq_ptr != NULL) {
                    MsgUnlock(sq_ptr);
                    MsgCloseArea(sq_ptr);
                    sq_ptr = NULL;
                }
                sprintf(filename, "%sMSGINFO.BBS", fido_msgpath);
                if ((i = sh_open(filename, SH_DENYWR, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE)) == -1) {
                    status_line("!%s message base is busy. Waiting", "QuickBBS");
                    while ((i = sh_open(filename, SH_DENYWR, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE)) == -1)
                        ;
                }
                f5 = fdopen(i, "r+b");
                fread((char *)&msginfo, sizeof(struct _msginfo), 1, f5);
                last_msg = msginfo.totalonboard[sys.quick_board - 1];
                sprintf(filename, "%sMSGIDX.BBS", fido_msgpath);
                f1 = fopen(filename, "ab");
                fseek(f1, 0L, SEEK_END);
                sprintf(filename, "%sMSGTOIDX.BBS", fido_msgpath);
                f2 = fopen(filename, "ab");
                fseek(f2, 0L, SEEK_END);
                setvbuf(f2, NULL, _IOFBF, 2048);
                sprintf(filename, "%sMSGTXT.BBS", fido_msgpath);
                f3 = fopen(filename, "ab");
                fseek(f3, 0L, SEEK_END);
                setvbuf(f3, NULL, _IOFBF, 2048);
                sprintf(filename, "%sMSGHDR.BBS", fido_msgpath);
                f4 = fopen(filename, "ab");
                fseek(f4, 0L, SEEK_END);
                setvbuf(f4, NULL, _IOFBF, 2048);
            }
        }
        else if (sys.gold_board) {
            if (ptype != 3) {
                if (ptype == 1) {
                    fseek(f5, 0L, SEEK_SET);
                    fwrite((char *)&msginfo, sizeof(struct _msginfo), 1, f5);
                    fclose(f1);
                    fclose(f2);
                    fclose(f3);
                    fclose(f4);
                    fclose(f5);
                }
                else if (ptype == 2) {
                    fclose(f1);
                    fclose(f2);
                    fclose(f3);
                }

                if (sq_ptr != NULL) {
                    MsgUnlock(sq_ptr);
                    MsgCloseArea(sq_ptr);
                    sq_ptr = NULL;
                }
                sprintf(filename, "%sMSGINFO.DAT", fido_msgpath);
                if ((i = sh_open(filename, SH_DENYWR, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE)) == -1) {
                    status_line("!%s message base is busy. Waiting", "GoldBase");
                    while ((i = sh_open(filename, SH_DENYWR, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE)) == -1)
                        ;
                }
                f5 = fdopen(i, "r+b");
                fread((char *)&gmsginfo, sizeof(struct _gold_msginfo), 1, f5);
                last_msg = msginfo.totalonboard[sys.gold_board - 1];
                sprintf(filename, "%sMSGIDX.DAT", fido_msgpath);

                f1 = fopen(filename, "ab");
                fseek(f1, 0L, SEEK_END);
                sprintf(filename, "%sMSGTOIDX.DAT", fido_msgpath);

                f2 = fopen(filename, "ab");
                fseek(f2, 0L, SEEK_END);
                setvbuf(f2, NULL, _IOFBF, 2048);
                sprintf(filename, "%sMSGTXT.DAT", fido_msgpath);

                f3 = fopen(filename, "ab");
                fseek(f3, 0L, SEEK_END);
                setvbuf(f3, NULL, _IOFBF, 2048);
                sprintf(filename, "%sMSGHDR.DAT", fido_msgpath);

                f4 = fopen(filename, "ab");
                fseek(f4, 0L, SEEK_END);
                setvbuf(f4, NULL, _IOFBF, 2048);
            }
        }
        else if (sys.pip_board) {
            if (ptype == 1 || ptype == 3) {
                fseek(f5, 0L, SEEK_SET);
                if (ptype == 3) {
                    fwrite((char *)&gmsginfo, sizeof(struct _gold_msginfo), 1, f5);
                }
                else {
                    fwrite((char *)&msginfo, sizeof(struct _msginfo), 1, f5);
                }
            }

            if (sq_ptr != NULL) {
                MsgUnlock(sq_ptr);
                MsgCloseArea(sq_ptr);
                sq_ptr = NULL;
            }

            if (f1 != NULL) {
                fclose(f1);
            }
            if (f2 != NULL) {
                fclose(f2);
            }
            if (f3 != NULL) {
                fclose(f3);
            }
            if (f4 != NULL) {
                fclose(f4);
            }
            if (f5 != NULL) {
                fclose(f5);
            }

            pip_scan_message_base(0, 0);

            sprintf(filename, "%sMPTR%04x.PIP", pip_msgpath, sys.pip_board);
            while ((i = sh_open(filename, SH_DENYWR, O_CREAT | O_RDWR | O_BINARY, S_IREAD | S_IWRITE)) == -1)
                ;
            f1 = fdopen(i, "rb+");
            sprintf(filename, "%sMPKT%04x.PIP", pip_msgpath, sys.pip_board);
            f2 = fopen(filename, "rb+");
            if (f2 == NULL) {
                f2 = fopen(filename, "wb");
                fputc(0, f2);
                fputc(0, f2);
                fclose(f2);
                f2 = fopen(filename, "rb+");
            }
            sprintf(filename, "%sDESTPTR.PIP", pip_msgpath);
            f3 = fopen(filename, "ab");
            f4 = f5 = NULL;
        }
        else if (sys.squish) {
            if (ptype == 1 || ptype == 3) {
                fseek(f5, 0L, SEEK_SET);
                if (ptype == 3) {
                    fwrite((char *)&gmsginfo, sizeof(struct _gold_msginfo), 1, f5);
                }
                else {
                    fwrite((char *)&msginfo, sizeof(struct _msginfo), 1, f5);
                }
            }

            if (f1 != NULL) {
                fclose(f1);
            }
            if (f2 != NULL) {
                fclose(f2);
            }
            if (f3 != NULL) {
                fclose(f3);
            }
            if (f4 != NULL) {
                fclose(f4);
            }
            if (f5 != NULL) {
                fclose(f5);
            }
            f1 = f2 = f3 = f4 = f5 = NULL;

            if (sq_ptr != NULL) {
                MsgUnlock(sq_ptr);
                MsgCloseArea(sq_ptr);
            }
            while ((sq_ptr = MsgOpenArea(sys.msg_path, MSGAREA_CRIFNEC, MSGTYPE_SQUISH)) == NULL)
                ;
            if (MsgLock(sq_ptr) == -1 && msgapierr != MERR_NOMEM) {
                status_line("!%s message base is busy. Waiting", "Squish");
                while (MsgLock(sq_ptr) == -1 && msgapierr != MERR_NOMEM)
                    ;
            }
            num_msg = (int)MsgGetNumMsg(sq_ptr);
            if (num_msg) {
                first_msg = 1;
            }
            else {
                first_msg = 0;
            }
            last_msg = num_msg;
        }
        else {
//			if (ptype == 1 || ptype == 3) {

            int infolenght;

            if (f5) {
                fseek(f5, 0L, SEEK_END);
                infolenght = (int) ftell(f5);
                fseek(f5, 0L, SEEK_SET);
                if (infolenght == sizeof(struct _gold_msginfo)) {
                    fwrite((char *)&gmsginfo, sizeof(struct _gold_msginfo), 1, f5);
                }
                else {
                    fwrite((char *)&msginfo, sizeof(struct _msginfo), 1, f5);
                }
            }

            if (sq_ptr != NULL) {
                MsgUnlock(sq_ptr);
                MsgCloseArea(sq_ptr);
                sq_ptr = NULL;
            }

            if (f1 != NULL) {
                fclose(f1);
            }
            if (f2 != NULL) {
                fclose(f2);
            }
            if (f3 != NULL) {
                fclose(f3);
            }
            if (f4 != NULL) {
                fclose(f4);
            }
            if (f5 != NULL) {
                fclose(f5);
            }
            f1 = f2 = f3 = f4 = f5 = NULL;
            scan_message_base(0, 0);
        }
    }
    else {
//		if (ptype == 1 || ptype == 3) {
        int infolenght;

        if (f5) {
            fseek(f5, 0L, SEEK_END);
            infolenght = (int) ftell(f5);
            fseek(f5, 0L, SEEK_SET);
            if (infolenght == sizeof(struct _gold_msginfo)) {
                fwrite((char *)&gmsginfo, sizeof(struct _gold_msginfo), 1, f5);
            }
            else {
                fwrite((char *)&msginfo, sizeof(struct _msginfo), 1, f5);
            }
        }

        if (sq_ptr != NULL) {
            MsgUnlock(sq_ptr);
            MsgCloseArea(sq_ptr);
            sq_ptr = NULL;
        }

        if (f1 != NULL) {
            fclose(f1);
        }
        if (f2 != NULL) {
            fclose(f2);
        }
        if (f3 != NULL) {
            fclose(f3);
        }
        if (f4 != NULL) {
            fclose(f4);
        }
        if (f5 != NULL) {
            fclose(f5);
        }
        f1 = f2 = f3 = f4 = f5 = NULL;
    }

    found = 0;

    if (dupecheck != NULL) {
        sprintf(filename, "%sDUPES.IDX", config->sys_path);
        while ((fd = sh_open(filename, SH_DENYWR, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE)) == -1)
            ;

        do {
            po = read(fd, (char *)&dupeindex, sizeof(struct _dupeindex) * MAX_DUPEINDEX);
            po /= sizeof(struct _dupeindex);
            for (i = 0; i < po; i++) {
                if (!stricmp(area, dupeindex[i].areatag)) {
                    dupecheck->area_pos = dupeindex[i].area_pos;
                    found = 1;
                    break;
                }
            }
        } while (!found && po == MAX_DUPEINDEX);

        close(fd);

        sprintf(filename, "%sDUPES.DAT", config->sys_path);
        while ((fd = sh_open(filename, SH_DENYWR, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE)) == -1)
            ;
        if (found) {
            lseek(fd, dupecheck->area_pos, SEEK_SET);
            read(fd, (char *)dupecheck, sizeof(struct _dupecheck));
            dupecheck->area_pos = dupeindex[i].area_pos;
        }
        else {
            lseek(fd, 0L, SEEK_END);
            memset((char *)dupecheck, 0, sizeof(struct _dupecheck));
            dupecheck->area_pos = tell(fd);
            strcpy(dupecheck->areatag, area);
            write(fd, (char *)dupecheck, sizeof(struct _dupecheck));
            close(fd);

            sprintf(filename, "%sDUPES.IDX", config->sys_path);
            while ((fd = sh_open(filename, SH_DENYRW, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE)) == -1)
                ;
            memset((char *)&dupeindex[0], 0, sizeof(struct _dupeindex));
            dupeindex[0].area_pos = dupecheck->area_pos;
            strcpy(dupeindex[0].areatag, area);
            lseek(fd, 0L, SEEK_END);
            write(fd, (char *)&dupeindex[0], sizeof(struct _dupeindex));
        }

        close(fd);
    }

    return (1);
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static void write_dupe_check()
{
    int fd;
    char filename[80];

    sprintf(filename, "%sDUPES.DAT", config->sys_path);
    while ((fd = sh_open(filename, SH_DENYWR, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE)) == -1)
        ;
    lseek(fd, dupecheck->area_pos, SEEK_SET);
    write(fd, (char *)dupecheck, sizeof(struct _dupecheck));
    close(fd);
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static int check_and_add_dupe(id)
long id;
{
    int i;

    for (i = 0; i < MAXDUPES; i++) {
        if (dupecheck->dupes[i] == id) {
            return (1);
        }
    }

    i = dupecheck->dupe_pos;
    dupecheck->dupes[i++] = id;
    if (i >= MAXDUPES) {
        i = 0;
    }
    dupecheck->dupe_pos = i;

    return (0);
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
static long compute_crc(string, crc)
char * string;
long crc;
{
    int i;

    for (i = 0; string[i]; i++) {
        crc = Z_32UpdateCRC((byte)string[i], crc);
    }

    return (crc);
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/

struct _msgzone {
    short dest_zone;
    short orig_zone;
    short dest_point;
    short orig_point;
};

int fido_save_message2(txt, bad)
FILE * txt;
char * bad;
{
    FILE * fp;
    int i, dest, m;
    char filename[80], buffer[2050];
    struct _msgzone mz;

    if (bad == NULL) {
        if (!last_msg) {
            last_msg++;
        }
        dest = ++last_msg;
        sprintf(filename, "%s%d.MSG", sys.msg_path, dest);
    }
    else {
        if (bad == bad_msgs) {
            dest = ++nbad;
        }
        else {
            dest = ++ndupes;
        }
        sprintf(filename, "%s%d.MSG", bad, dest);
    }

    fp = fopen(filename, "wb");
    if (fp == NULL) {
        return (0);
    }

    if (sys.public) {
        msg.attr &= ~MSGPRIVATE;
    }
    else if (sys.private) {
        msg.attr |= MSGPRIVATE;
    }

    mz.orig_zone = msg_fzone;
    mz.dest_zone = msg_tzone;
    mz.orig_point = msg_fpoint;
    mz.dest_point = msg_tpoint;
    memcpy(&msg.date_written, &mz, sizeof(struct _msgzone));

    if (fwrite((char *)&msg, sizeof(struct _msg), 1, fp) != 1) {
        return (0);
    }

    if ((bad == config->bad_msgs || bad == config->dupes) && sys.echomail) {
        if (fprintf(fp, "AREA:%s\r\n", sys.echotag) == EOF) {
            return (0);
        }
        if (msg_tzone != msg_fzone) {
            if (fprintf(fp, msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig) == EOF) {
                return (0);
            }
        }
        if (msg_fpoint) {
            if (fprintf(fp, "\001FMPT %d\r\n", msg_fpoint) == EOF) {
                return (0);
            }
        }
        if (msg_tpoint) {
            if (fprintf(fp, msgtxt[M_TOPT], msg_tpoint) == EOF) {
                return (0);
            }
        }
    }

    do {
        i = mread(buffer, 1, 2048, txt);
        for (m = 0; m < i; m++) {
            if (buffer[m] == 0x1A) {
                buffer[m] = ' ';
            }
        }
        if (fwrite(buffer, 1, i, fp) != i) {
            return (0);
        }
    } while (i == 2048);

    fputc('\0', fp);
    fclose(fp);

    return (1);
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
int fido_save_message3(FILE * txt)
{
    FILE * fp;
    int i, dest, m;
    char filename[80], buffer[2050];

    dest = ++nmy;
    sprintf(filename, "%s%d.MSG", config->my_mail, dest);

    fp = fopen(filename, "wb");
    if (fp == NULL) {
        return (0);
    }

    if (sys.public) {
        msg.attr &= ~MSGPRIVATE;
    }
    else if (sys.private) {
        msg.attr |= MSGPRIVATE;
    }

    if (fwrite((char *)&msg, sizeof(struct _msg), 1, fp) != 1) {
        return (0);
    }

    if (sys.echomail)
        if (fprintf(fp, "AREA:%s\r\n", sys.echotag) == EOF) {
            return (0);
        }

    do {
        i = mread(buffer, 1, 2048, txt);
        for (m = 0; m < i; m++) {
            if (buffer[m] == 0x1A) {
                buffer[m] = ' ';
            }
        }
        if (fwrite(buffer, 1, i, fp) != i) {
            return (0);
        }
    } while (i == 2048);

    fputc('\0', fp);
    fclose(fp);

    return (1);
}

/*---------------------------------------------------------------------------
   static void passthrough_save_message (fpd)

   Esporta immediatamente il messaggio per le aree passthrough, cioe'
   quelle aree che non devono rimanere residenti nella base messaggi locale
   ma devono essere immediatamente esportate e cancellate.
---------------------------------------------------------------------------*/
int pass_is_here(int z, int ne, int no, int pp, struct _fwd_alias * forward, int maxnodes)
{
    register int i;

    for (i = 0; i < maxnodes; i++)
        if (forward[i].zone == z && forward[i].net == ne && forward[i].node == no && forward[i].point == pp) {
            return (i);
        }

    return (-1);
}

static int mail_sort_func(const void * a1, const void * b1)
{
    struct _fwd_alias * a, *b;
    a = (struct _fwd_alias *)a1;
    b = (struct _fwd_alias *)b1;
    if (a->net != b->net) {
        return (a->net - b->net);
    }
    return ((int)(a->node - b->node));
}

static int passthrough_save_message(fpd, seen, n_seen, path, n_path)
FILE * fpd;
struct _fwd_alias * seen;
int n_seen;
struct _fwd_alias * path;
int n_path;
{
    int mi, i, z, ne, no, pp, ai, m, cnet, cnode;
    char * p, buffer[2050], buff[80], tmpforw[80], wrp[80], addr[30];
    struct _msghdr2 mhdr;

    mhdr.ver = PKTVER;
    mhdr.cost = 0;
    mhdr.attrib = 0;

    if (sys.private) {
        mhdr.attrib |= MSGPRIVATE;
    }

    for (i = 0; i < n_seen; i++) {
        seen[i].export = 0;
    }

    if (!sys.use_alias) {
        z = config->alias[sys.use_alias].zone;
        parse_netnode2(sys.forward1, &z, &ne, &no, &pp);
        if (z != config->alias[sys.use_alias].zone) {
            for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++)
                if (z == config->alias[i].zone) {
                    break;
                }
            if (i < MAX_ALIAS && config->alias[i].net) {
                sys.use_alias = i;
            }
        }
    }

    z = config->alias[sys.use_alias].zone;
    ne = config->alias[sys.use_alias].net;
    no = config->alias[sys.use_alias].node;
    pp = 0;

    strcpy(tmpforw, sys.forward1);
    p = strtok(tmpforw, " ");
    if (p != NULL)
        do {
            parse_netnode2(p, &z, &ne, &no, &pp);
            if ((i = pass_is_here(z, ne, no, pp, seen, n_seen)) == -1) {
                seen[n_seen].zone = z;
                seen[n_seen].net = ne;
                seen[n_seen].node = no;
                seen[n_seen].point = pp;
                seen[n_seen].export = 1;
                n_seen++;
            }
        } while ((p = strtok(NULL, " ")) != NULL);

    if (!sys.use_alias) {
        z = config->alias[sys.use_alias].zone;
        parse_netnode2(sys.forward2, &z, &ne, &no, &pp);
        if (z != config->alias[sys.use_alias].zone) {
            for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++)
                if (z == config->alias[i].zone) {
                    break;
                }
            if (i < MAX_ALIAS && config->alias[i].net) {
                sys.use_alias = i;
            }
        }
    }

    z = config->alias[sys.use_alias].zone;
    ne = config->alias[sys.use_alias].net;
    no = config->alias[sys.use_alias].node;
    pp = 0;

    strcpy(tmpforw, sys.forward2);
    p = strtok(tmpforw, " ");
    if (p != NULL)
        do {
            parse_netnode2(p, &z, &ne, &no, &pp);
            if ((i = pass_is_here(z, ne, no, pp, seen, n_seen)) == -1) {
                seen[n_seen].zone = z;
                seen[n_seen].net = ne;
                seen[n_seen].node = no;
                seen[n_seen].point = pp;
                seen[n_seen].export = 1;
                n_seen++;
            }
        } while ((p = strtok(NULL, " ")) != NULL);

    if (!sys.use_alias) {
        z = config->alias[sys.use_alias].zone;
        parse_netnode2(sys.forward3, &z, &ne, &no, &pp);
        if (z != config->alias[sys.use_alias].zone) {
            for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++)
                if (z == config->alias[i].zone) {
                    break;
                }
            if (i < MAX_ALIAS && config->alias[i].net) {
                sys.use_alias = i;
            }
        }
    }

    z = config->alias[sys.use_alias].zone;
    ne = config->alias[sys.use_alias].net;
    no = config->alias[sys.use_alias].node;
    pp = 0;

    strcpy(tmpforw, sys.forward3);
    p = strtok(tmpforw, " ");
    if (p != NULL)
        do {
            parse_netnode2(p, &z, &ne, &no, &pp);
            if ((i = pass_is_here(z, ne, no, pp, seen, n_seen)) == -1) {
                seen[n_seen].zone = z;
                seen[n_seen].net = ne;
                seen[n_seen].node = no;
                seen[n_seen].point = pp;
                seen[n_seen].export = 1;
                n_seen++;
            }
        } while ((p = strtok(NULL, " ")) != NULL);

    if (!config->alias[sys.use_alias].point) {
        seen[n_seen].export = 0;
        seen[n_seen].net = config->alias[sys.use_alias].net;
        seen[n_seen++].node = config->alias[sys.use_alias].node;
    }
    else if (config->alias[sys.use_alias].fakenet) {
        seen[n_seen].export = 0;
        seen[n_seen].net = config->alias[sys.use_alias].fakenet;
        seen[n_seen++].node = config->alias[sys.use_alias].point;
    }

    for (i = 0; i < n_seen; i++) {
        if (!seen[i].export) {
            continue;
        }

        ai = sys.use_alias;
        for (m = 0; m < maxakainfo; m++)
            if (akainfo[m].zone == seen[i].zone && akainfo[m].net == seen[i].net && akainfo[m].node == seen[i].node && akainfo[m].point == seen[i].point) {
                if (akainfo[m].aka) {
                    ai = akainfo[m].aka - 1;
                }
                break;
            }
        if (ai != sys.use_alias) {
            if (!config->alias[ai].point) {
                if (config->alias[ai].net != seen[n_seen - 1].net || config->alias[ai].node != seen[n_seen - 1].node) {
                    seen[n_seen].export = 0;
                    seen[n_seen].net = config->alias[ai].net;
                    seen[n_seen++].node = config->alias[ai].node;
                }
            }
            else if (config->alias[ai].fakenet) {
                if (config->alias[ai].fakenet != seen[n_seen - 1].net || config->alias[ai].point != seen[n_seen - 1].node) {
                    seen[n_seen].export = 0;
                    seen[n_seen].net = config->alias[ai].fakenet;
                    seen[n_seen++].node = config->alias[ai].point;
                }
            }
        }
    }

    qsort(seen, n_seen, sizeof(struct _fwd_alias), mail_sort_func);

    cnet = cnode = 0;
    strcpy(wrp, "SEEN-BY: ");

    for (i = 0; i < n_seen; i++) {
        if (strlen(wrp) > 65) {
            mprintf(fpd, "%s\r\n", wrp);
            cnet = cnode = 0;
            strcpy(wrp, "SEEN-BY: ");
        }
        if (i && seen[i].net == seen[i - 1].net && seen[i].node == seen[i - 1].node) {
            continue;
        }

        if (cnet != seen[i].net) {
            sprintf(buffer, "%d/%d ", seen[i].net, seen[i].node);
            cnet = seen[i].net;
            cnode = seen[i].node;
        }
        else if (cnet == seen[i].net && cnode != seen[i].node) {
            sprintf(buffer, "%d ", seen[i].node);
            cnode = seen[i].node;
        }
        else {
            strcpy(buffer, "");
        }

        strcat(wrp, buffer);
    }

    if (strlen(wrp) > 9) {
        mprintf(fpd, "%s\r\n", wrp);
    }


    path[n_path].zone = config->alias[sys.use_alias].zone;
    if (config->alias[sys.use_alias].point && config->alias[sys.use_alias].fakenet) {
        path[n_path].net = config->alias[sys.use_alias].fakenet;
        path[n_path].node = config->alias[sys.use_alias].point;
        path[n_path].point = 0;
    }
    else {
        path[n_path].net = config->alias[sys.use_alias].net;
        path[n_path].node = config->alias[sys.use_alias].node;
        path[n_path].point = config->alias[sys.use_alias].point;
    }

    n_path++;
    i = 0;

    do {
        cnet = cnode = 0;
        strcpy(wrp, "\001PATH: ");

        do {
            if (cnet != path[i].net) {
                sprintf(addr, "%d/%d ", path[i].net, path[i].node);
                cnet = path[i].net;
                cnode = path[i].node;
            }
            else if (cnode != path[i].node) {
                sprintf(addr, "%d ", path[i].node);
                cnet = path[i].net;
                cnode = path[i].node;
            }
            else {
                strcpy(addr, "");
            }

            strcat(wrp, addr);
            i++;
        } while (i < n_path && strlen(wrp) < 70);

        if (strlen(wrp) > 7) {
            mprintf(fpd, "%s\r\n", wrp);
        }
    } while (i < n_path);

    for (i = 0; i < n_seen; i++) {
        if (!seen[i].export) {
            continue;
        }

        ai = sys.use_alias;
        for (m = 0; m < maxakainfo; m++)
            if (akainfo[m].zone == seen[i].zone && akainfo[m].net == seen[i].net && akainfo[m].node == seen[i].node && akainfo[m].point == seen[i].point) {
                if (akainfo[m].aka) {
                    ai = akainfo[m].aka - 1;
                }
                break;
            }

        if (config->alias[ai].point && config->alias[ai].fakenet) {
            mhdr.orig_node = config->alias[ai].point;
            mhdr.orig_net = config->alias[ai].fakenet;
        }
        else {
            mhdr.orig_node = config->alias[ai].node;
            mhdr.orig_net = config->alias[ai].net;
        }

        mhdr.dest_net = seen[i].net;
        mhdr.dest_node = seen[i].node;

        mi = open_packet(seen[i].zone, seen[i].net, seen[i].node, seen[i].point, ai);
        write(mi, (char *)&mhdr, sizeof(struct _msghdr2));

        write(mi, msg.date, strlen(msg.date) + 1);
        write(mi, msg.to, strlen(msg.to) + 1);
        write(mi, msg.from, strlen(msg.from) + 1);
        write(mi, msg.subj, strlen(msg.subj) + 1);

        sprintf(buffer, "AREA:%s\r\n", sys.echotag);
        write(mi, buffer, strlen(buffer));

        mseek(fpd, 0L, SEEK_SET);
        do {
            z = mread(buffer, 1, 2048, fpd);
            if (write(mi, buffer, z) != z) {
                break;
            }
        } while (z == 2048);
        buff[0] = buff[1] = buff[2] = 0;
        if (write(mi, buff, 3) != 3) {
            return (0);
        }
        close(mi);

        time_release();
    }

    return (1);
}

static void unpack_arcmail(dir)
char * dir;
{
    int retval, isvideo, id, f, *varr, m, i;
    char filename[80], outstring[128], extr[80], swapout, *p;
    unsigned char headstr[30], r;
    struct ffblk blk;

    local_status("Unpack");
    unpack_system();

    sprintf(filename, "%s*.*", dir);
    if (findfirst(filename, &blk, 0)) {
        return;
    }

    isvideo = 0;

    do {
        // Controlla se si tratta di un arcmail autentico, guardando il nome
        // (8 caratteri esadecimali) e l'estensione (due lettere del giorno
        // della settimana e un numero decimale).
        if (!isbundle(blk.ff_name)) {
            continue;
        }

        prints(7, 65, YELLOW | _BLACK, blk.ff_name);
        prints(8, 65, YELLOW | _BLACK, "              ");

        sprintf(extr, "%s%s", dir, blk.ff_name);

        if (!isvideo) {
            setup_video_interrupt("UNPACK ARCMAIL");
            isvideo = 1;
        }

        if ((f = sh_open(extr, SH_DENYRW, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) == -1) {
            continue;
        }

        id = -1;

        for (i = 0; i < 10; i++) {
            if (config->packid[i].offset >= 0L) {
                lseek(f, config->packid[i].offset, SEEK_SET);
            }
            else {
                lseek(f, config->packid[i].offset, SEEK_END);
            }
            read(f, headstr, 14);

            if (!config->packers[i].id[0] || !config->packid[i].ident[0]) {
                continue;
            }

            id = i;
            m = 0;
            p = (char *)&config->packid[i].ident[0];

            while (*p) {
                r = 0;
                if (isdigit(*p)) {
                    r *= 16L;
                    r += *p - '0';
                }
                else if (isxdigit(*p)) {
                    r *= 16L;
                    r += *p - 55;
                }
                p++;
                if (isdigit(*p)) {
                    r *= 16L;
                    r += *p - '0';
                }
                else if (isxdigit(*p)) {
                    r *= 16L;
                    r += *p - 55;
                }
                p++;
                if (r != headstr[m++]) {
                    id = -1;
                    break;
                }
            }

            if (id != -1) {
                break;
            }
        }
        close(f);

        if (id == -1) {
            status_line("!Unrecognized method for %s", blk.ff_name);
            id = 0;
            do {
                sprintf(outstring, "%sBAD_ARC.%03d", dir, id++);
            } while (rename(extr, outstring) == -1);
            status_line(":Renamed %s in %s", extr, outstring);
        }
        else {
            status_line("#Un%sing %s", config->packers[id].id, extr);

            activation_key();
            prints(8, 65, YELLOW | _BLACK, config->packers[id].id);

            time_release();
            if (config->packers[id].unpackcmd[0] == '+') {
                strcpy(outstring, &config->packers[id].unpackcmd[1]);
                swapout = 1;
            }
            else {
                strcpy(outstring, config->packers[id].unpackcmd);
                swapout = 0;
            }
            strsrep(outstring, "%1", extr);
            strsrep(outstring, "%2", "*.PKT");

            varr = ssave();
            gotoxy(1, 14);
            wtextattr(LGREY | _BLACK);

            retval = spawn_program(swapout, outstring);

            if (varr != NULL) {
                srestore(varr);
            }

            time_release();

            // Se il compattatore ritorna un errorlevel diverso da 0 significa che
            // si e' rilevato un errore, pertanto non viene cancellato il pacchetto.
            if (retval) {
                status_line(":Return code %d", retval);
                id = 0;
                do {
                    sprintf(outstring, "%sBAD_ARC.%03d", dir, id++);
                } while (rename(extr, outstring) == -1);
                status_line(":Renamed %s in %s", extr, outstring);
            }
            else {
                unlink(extr);
            }
        }
    } while (!findnext(&blk));

    if (isvideo) {
        hidecur();
        wclose();

        printc(12, 0, LGREY | _BLACK, '\303');
        printc(12, 52, LGREY | _BLACK, '\305');
        printc(12, 79, LGREY | _BLACK, '\264');
    }
}


