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
#include <ctype.h>
#include <string.h>
#include <dir.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <alloc.h>
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

#define DOWN_FILES  0x0020
#define NO_MESSAGE  0x0040
#define COPYBUFFER  8192

typedef struct {
    char name[13];
    word area;
    long datpos;
} FILEIDX;

extern short tcpip;
char no_external = 0, no_description = 0, no_check = 0, no_precheck = 0;

void stripcrlf(char * linea);
int exist_system_file(char * name);
void m_print2(char * format, ...);
char * translate_ansi(char *);
void check_uploads(FILE * xferinfo, char * path);
void update_filestat(char * rqname);
int download_tagged_files(char * fname);
void tag_files(int);
void ask_birthdate(void);
int m_getch(void);
void update_lastread_pointers(void);
void general_external_protocol(char * name, char * path, int id, int global, int dl);
void display_external_protocol(int id);

static FILE * Infile;
static int fsize2 = 0;

static int prot_read_file(char *);
int file_more_question(int, int);

typedef struct _dirent {
    char name[13];
    unsigned date;
    long size;
} DIRENT;

DIRENT * scandir(char * dir, int * n)
{
    int ndir = 0;
    char filename[80];
    struct ffblk blk;
    DIRENT * d;

    sprintf(filename, "%s*.*", dir);
    if (findfirst(filename, &blk, 0)) {
        return (NULL);
    }

    do {
        ndir++;
    } while (!findnext(&blk));

    d = (DIRENT *)calloc(ndir, sizeof(DIRENT));
    if (d == NULL) {
        return (NULL);
    }

    if (findfirst(filename, &blk, 0)) {
        return (NULL);
    }

    ndir = 0;

    do {
        strcpy(d[ndir].name, blk.ff_name);
        d[ndir].date = blk.ff_fdate;
        d[ndir].size = blk.ff_fsize;
        ndir++;
    } while (!findnext(&blk));

    *n = ndir;
    return (d);
}

void file_list(char * path, char * list)
{
    int fd, m, i, line, z, nnd, yr;
    char filename[80], name[14], desc[70], *fbuf, *p;
    DIRENT * de;

    cls();

    m_print(bbstxt[B_CONTROLS]);
    m_print(bbstxt[B_CONTROLS2]);
    change_attr(CYAN | _BLACK);

    if (list == NULL || !(*list)) {
        i = strlen(path) - 1;
        while (path[i] == ' ') {
            path[i--] = '\0';
        }
        while (*path == ' ' && *path != '\0') {
            path++;
        }

        sprintf(filename, "%sFILES.BBS", path);
        fd = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);
    }
    else {
        status_line(msgtxt[M_READ_FILE_LIST], list);
        fd = sh_open(list, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);
    }

    // Se il file e' di 0 bytes non importa leggere altro
    if (fd == -1 || filelength(fd) == 0L) {
        if (fd != -1) {
            close(fd);
        }
        return;
    }

    line = 3;

    // Alloca il buffer che andra' a contenere l'intero files.bbs per
    // l'area corrente. Se non riesce ad allocarselo scrive una linea
    // nel log con le informazioni utili (spero) per il debugging.
    if ((fbuf = (char *)malloc((unsigned int)filelength(fd) + 1)) == NULL) {
#ifdef __OS2__
        status_line("!MEM: new_file_list (%ld)", filelength(fd));
#else
        status_line("!MEM: new_file_list (%ld / %ld)", filelength(fd), coreleft());
#endif
        close(fd);
        return;
    }

    // Lettura di tutto il files.bbs nel buffer allocato.
    read(fd, fbuf, (unsigned int)filelength(fd));
    fbuf[(unsigned int)filelength(fd)] = '\0';
    close(fd);

    // Legge tutta la directory in memoria. Se non c'e' spazio sufficiente
    // prosegue (scandir si occupa di segnalare nel log gli errori.
    if ((de = scandir(sys.filepath, &nnd)) == NULL) {
        free(fbuf);
        return;
    }

    // La scansione del files.bbs avviene in memoria. Tenendo conto che
    // ogni linea e' separata almeno da un \r (CR, 0x0D) si puo' utilizzare
    // la strtok, trattando l'intero buffer come un insieme di token.
    if ((p = strtok(fbuf, "\r")) != NULL)
        do {
            // Nel caso di \r\n (EOL) e' necessario saltare il \n per evitare
            // problemi nel riconoscimento dei files.
            if (*p == '\n') {
                p++;
            }

            if (*p == '\0') {
                m_print(bbstxt[B_ONE_CR]);
                if ((line = file_more_question(line, 1)) == 0 || !CARRIER) {
                    break;
                }
                continue;
            }

            if (!CARRIER || (!local_mode && RECVD_BREAK())) {
                line = 0;
                break;
            }

            // Se il primo carattere non e' alfanumerico si tratta di un
            // commento, pertanto non e' necessario visualizzarlo.
            if (*p == '-') {
                *p = ' ';
                m_print(bbstxt[B_FILES_DASHLINE], p);

                if ((line = file_more_question(line, 1)) == 0 || !CARRIER) {
                    break;
                }
            }
            else if (*p == ' ') {
                if (p[1] == '>' || p[1] == '|') {
                    if (sys.no_filedate) {
                        m_print(bbstxt[B_FILES_XNODATE], &p[2]);
                    }
                    else {
                        m_print(bbstxt[B_FILES_XDATE], &p[2]);
                    }
                }
                else {
                    m_print(bbstxt[B_FILES_SPACELINE], &p[1]);
                }

                if ((line = file_more_question(line, 1)) == 0 || !CARRIER) {
                    break;
                }
            }
            else {
                // Estrae il nome del file dalla stringa puntata da p
                m = 0;
                while (p[m] && p[m] != ' ' && m < 12) {
                    name[m] = p[m++];
                }
                name[m] = '\0';

                if (m == 0) {
                    m_print(bbstxt[B_ONE_CR]);
                    if ((line = file_more_question(line, 1)) == 0 || !CARRIER) {
                        break;
                    }
                    continue;
                }

                // Cerca il file nella lista in memoria puntata da de.
                for (i = 0; i < nnd; i++) {
                    if (!stricmp(de[i].name, name)) {
                        break;
                    }
                }

                // Se trova il file nella directory prosegue le verifiche
                // altrimenti salta alla linea sucessiva.
                yr = (i >= nnd) ? -1 : 0;

                z = 0;
                while (p[m] == ' ') {
                    m++;
                }
                while (p[m] && z < (sys.no_filedate ? 56 : 48)) {
                    desc[z++] = p[m++];
                }
                if (p[m] != '\0')
                    while (p[m] != ' ' && z > 0) {
                        m--;
                        z--;
                    }
                desc[z] = '\0';

                if (sys.no_filedate) {
                    if (yr == -1) {
                        if (config->show_missing) {
                            m_print(bbstxt[B_FILES_NODATE_MISSING], strupr(name), bbstxt[B_FILES_MISSING], desc);
                        }
                    }
                    else {
                        m_print(bbstxt[B_FILES_NODATE], strupr(name), de[i].size, desc);
                    }
                }
                else {
                    if (yr == -1) {
                        if (config->show_missing) {
                            m_print(bbstxt[B_FILES_FORMAT_MISSING], strupr(name), bbstxt[B_FILES_MISSING], desc);
                        }
                    }
                    else {
                        m_print(bbstxt[B_FILES_FORMAT], strupr(name), de[i].size, show_date(config->dateformat, e_input, de[i].date, 0), desc);
                    }
                }
                if (!(line = file_more_question(line, 1)) || !CARRIER) {
                    break;
                }

                while (p[m] != '\0') {
                    z = 0;
                    while (p[m] != '\0' && z < (sys.no_filedate ? 56 : 48)) {
                        desc[z++] = p[m++];
                    }
                    if (p[m] != '\0')
                        while (p[m] != ' ' && z > 0) {
                            m--;
                            z--;
                        }
                    desc[z] = '\0';

                    if (sys.no_filedate) {
                        m_print(bbstxt[B_FILES_XNODATE], desc);
                    }
                    else {
                        m_print(bbstxt[B_FILES_XDATE], desc);
                    }

                    if ((line = file_more_question(line, 1)) == 0 || !CARRIER) {
                        break;
                    }
                }

                if (line == 0) {
                    break;
                }
            }
        } while ((p = strtok(NULL, "\r")) != NULL);

    free(de);
    free(fbuf);

    m_print(bbstxt[B_ONE_CR]);
    if (line && CARRIER) {
        file_more_question(usr.len, 1);
    }
}

void file_display()
{
    char filename[80], stringa[20];

    if (!get_command_word(stringa, 14)) {
        m_print(bbstxt[B_WHICH_FILE]);
        input(stringa, 14);
        if (!strlen(stringa)) {
            return;
        }
    }

    status_line("+Display '%s'", strupr(stringa));
    sprintf(filename, "%s%s", sys.filepath, stringa);

    cls();

    if (prot_read_file(filename) > 1 && CARRIER) {
        press_enter();
    }
}

void override_path()
{
    char stringa[80];

    if (!get_command_word(stringa, 78)) {
        m_print(bbstxt[B_FULL_OVERRIDE]);
        input(stringa, 78);
        if (!strlen(stringa)) {
            return;
        }
    }

    append_backslash(stringa);
    status_line("*Path override '%s'", strupr(stringa));

    strcpy(sys.filepath, stringa);
    strcpy(sys.uppath, stringa);
    fancy_str(stringa);
    strcpy(sys.file_name, stringa);
}

static int prot_read_file(name)
char * name;
{
    FILE * fp;
    char linea[130], *p;
    int line;

    XON_ENABLE();
    _BRK_ENABLE();

    if (!name || !(*name)) {
        return (0);
    }

    fp = fopen(name, "rb");
    if (fp == NULL) {
        return (0);
    }

    line = 1;
    nopause = 0;

loop:
    change_attr(LGREY | _BLACK);

    while (fgets(linea, 128, fp) != NULL) {
        linea[strlen(linea) - 2] = '\0';

        for (p = linea; (*p); p++) {
            if (!CARRIER || (!local_mode && RECVD_BREAK())) {
                CLEAR_OUTBOUND();
                fclose(fp);
                return (0);
            }

            if (*p == 0x1B) {
                p = translate_ansi(p);
            }
            else {
                if (!local_mode) {
                    BUFFER_BYTE(*p);
                }
                if (snooping) {
                    wputc(*p);
                }
            }
        }

        if (!local_mode) {
            BUFFER_BYTE('\r');
            BUFFER_BYTE('\n');
        }
        if (snooping) {
            wputs("\n");
        }

        if (!(line++ < (usr.len - 1)) && usr.len != 0) {
            line = 1;
            if (!continua()) {
                fclose(fp);
                break;
            }
        }

        if (*p == CTRLZ) {
            fclose(fp);
            UNBUFFER_BYTES();
            FLUSH_OUTPUT();
            break;
        }
    }

    fclose(fp);

    UNBUFFER_BYTES();
    FLUSH_OUTPUT();

    return (line);
}

void raw_dir()
{
    int yr, mo, dy, line, nfiles;
    char stringa[30], filename[80];
    long fsize;
    struct ffblk blk;

    nopause = 0;

    if (!get_command_word(stringa, 14)) {
        m_print(bbstxt[B_DIRMASK]);
        input(stringa, 14);
        if (!CARRIER) {
            return;
        }
    }

    if (!strlen(stringa)) {
        strcpy(stringa, "*.*");
    }

    sprintf(filename, "%s%s", sys.filepath, stringa);

    line = 1;
    nfiles = 0;
    fsize = 0L;

    if (!findfirst(filename, &blk, 0)) {
        cls();
        m_print(bbstxt[B_RAW_HEADER]);

        do {
            yr = (blk.ff_fdate >> 9) & 0x7F;
            mo = (blk.ff_fdate >> 5) & 0x0F;
            dy = blk.ff_fdate & 0x1F;

            m_print(bbstxt[B_RAW_FORMAT], blk.ff_name, blk.ff_fsize, dy, mesi[mo - 1], (yr + 80) % 100);

            nfiles++;
            fsize += blk.ff_fsize;

            if (!(line = file_more_question(line, 1)) || !CARRIER) {
                break;
            }
            if (!local_mode && RECVD_BREAK()) {
                break;
            }
        } while (!findnext(&blk));
    }

    if (CARRIER && line) {
        sprintf(stringa, "%d files", nfiles);
        m_print(bbstxt[B_RAW_FOOTER], stringa, fsize);
        press_enter();
    }
}

void new_file_list(only_one)
int only_one;
{
    int fpd, day, mont, year, line, m, z, fd, nnd, i, len, oldlen = 0;
    word search;
    long totfile, totsize;
    char month[4], name[13], desc[70], *fbuf, header[90];
    char stringa[30], filename[80], *p, sameline, areadone;
    struct _sys tsys, vsys;
    DIRENT * de;

    sameline = (bbstxt[B_AREALIST_HEADER][strlen(bbstxt[B_AREALIST_HEADER]) - 1] == '\n') ? 0 : 1;
    strcpy(header, bbstxt[B_AREALIST_HEADER]);
    header[strlen(header) - 1] = '\0';

    totfile = totsize = 0L;
    areadone = 0;

    if (only_one != 2) {
        m_print(bbstxt[B_NEW_SINCE_LAST]);
    }

    if (only_one != 2 && yesno_question(DEF_YES) == DEF_NO) {
        if (!get_command_word(stringa, 8)) {
            m_print(bbstxt[B_NEW_DATE], bbstxt[B_DATEFORMAT + config->inp_dateformat]);
            input(stringa, 8);
            if (stringa[0] == '\0' || !CARRIER) {
                return;
            }
        }

        sscanf(stringa, "%2d-%2d-%2d", &day, &mont, &year);
        mont--;

        if (config->inp_dateformat == 1) {
            i = day;
            day = mont;
            mont = i;
        }
        else if (config->inp_dateformat == 2) {
            i = year;
            year = day;
            day = i;
        }
    }
    else {
        sscanf(usr.ldate, "%2d %3s %2d", &day, month, &year);
        month[3] = '\0';
        for (mont = 0; mont < 12; mont++) {
            if ((!stricmp(mtext[mont], month)) || (!stricmp(mesi[mont], month))) {
                break;
            }
        }
    }

    if (only_one == 2) {
        only_one = 0;
    }

    if ((mont == 12) || (day > 31) || (day < 1) || (year > 99) || (year < 80) || !CARRIER) {
        return;
    }

    search = ((year - 80) << 9) | ((mont + 1) << 5) | day;

    cls();
    m_print(bbstxt[B_DATE_SEARCH]);
    m_print(bbstxt[B_DATE_UNDERLINE]);

    line = 4;

    sprintf(filename, "%sSYSFILE.DAT", config->sys_path);
    if ((fd = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) == -1) {
        status_line(msgtxt[M_NO_SYSTEM_FILE]);
        return;
    }

    memcpy((char *)&vsys.file_name, (char *)&sys.file_name, SIZEOF_FILEAREA);

    while (read(fd, (char *)&tsys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA && line) {
        if (only_one) {
            memcpy((char *)&tsys.file_name, (char *)&sys.file_name, SIZEOF_FILEAREA);
        }
        else {
            if (usr.priv < tsys.file_priv || tsys.no_global_search) {
                continue;
            }
            if ((usr.flags & tsys.file_flags) != tsys.file_flags) {
                continue;
            }
        }

        if (tsys.cdrom) {
            if (only_one || !CARRIER) {
                break;
            }

            continue;
        }

        memcpy((char *)&sys.file_name, (char *)&tsys.file_name, SIZEOF_FILEAREA);

        if (sameline && (usr.ansi || usr.avatar)) {
            del_line();
        }

        m_print(header, tsys.file_num, tsys.file_name);
        if (!sameline) {
            m_print2("\n");
            if ((line = file_more_question(line, only_one)) == 0) {
                break;
            }
        }
        else {
            areadone = 0;
            if (!usr.ansi && !usr.avatar) {
                len = strlen(tsys.file_name);
                if (oldlen != 0 && len < oldlen) {
                    space(oldlen - len);
                }
                oldlen = len;
            }
            m_print2("\r");
        }

        // Apertura del FILES.BBS utilizzando il path dell'area letta nella
        // struttura locale tsys
        if (!tsys.filelist[0]) {
            sprintf(filename, "%sFILES.BBS", tsys.filepath);
        }
        else {
            strcpy(filename, tsys.filelist);
        }
        if ((fpd = open(filename, O_RDONLY | O_BINARY)) == -1) {
            continue;
        }

        // Se il file e' di 0 bytes non importa leggere altro
        if (filelength(fpd) == 0L) {
            close(fpd);
            continue;
        }

        // Alloca il buffer che andra' a contenere l'intero files.bbs per
        // l'area corrente. Se non riesce ad allocarselo scrive una linea
        // nel log con le informazioni utili (spero) per il debugging.
        if ((fbuf = (char *)malloc((unsigned int)filelength(fpd) + 1)) == NULL) {
#ifdef __OS2__
            status_line("!MEM: new_file_list (%ld)", filelength(fpd));
#else
            status_line("!MEM: new_file_list (%ld / %ld)", filelength(fpd), coreleft());
#endif
            close(fpd);
            continue;
        }

        // Lettura di tutto il files.bbs nel buffer allocato.
        read(fpd, fbuf, (unsigned int)filelength(fpd));
        fbuf[(unsigned int)filelength(fpd)] = '\0';
        close(fpd);

        // Legge tutta la directory in memoria. Se non c'e' spazio sufficiente
        // prosegue (scandir si occupa di segnalare nel log gli errori.
        if ((de = scandir(tsys.filepath, &nnd)) == NULL) {
            free(fbuf);
            continue;
        }

        // La scansione del files.bbs avviene in memoria. Tenendo conto che
        // ogni linea e' separata almeno da un \r (CR, 0x0D) si puo' utilizzare
        // la strtok, trattando l'intero buffer come un insieme di token.
        if ((p = strtok(fbuf, "\r\n")) != NULL)
            do {
                // Nel caso di \r\n (EOL) e' necessario saltare il \n per evitare
                // problemi nel riconoscimento dei files.
                if (p[0] == '\0') {
                    continue;
                }
                if (p[0] == '\n') {
                    p++;
                }

                if (!CARRIER || (!local_mode && RECVD_BREAK())) {
                    line = 0;
                    break;
                }

                // Se il primo carattere non e' alfanumerico si tratta di un
                // commento, pertanto non e' necessario visualizzarlo.
                if (!isalnum(p[0])) {
                    continue;
                }

                // Estrae il nome del file dalla stringa puntata da p
                m = 0;
                while (p[m] != ' ' && m < 12) {
                    name[m] = p[m++];
                }
                name[m] = '\0';

                // Cerca il file nella lista in memoria puntata da de.
                for (i = 0; i < nnd; i++) {
                    if (!stricmp(de[i].name, name)) {
                        break;
                    }
                }

                // Se trova il file nella directory prosegue le verifiche
                // altrimenti salta alla linea sucessiva.
                if (i < nnd && de[i].date >= search) {
                    z = 0;
                    while (p[m] == ' ') {
                        m++;
                    }
                    while (p[m] && z < (tsys.no_filedate ? 56 : 48)) {
                        desc[z++] = p[m++];
                    }
                    desc[z] = '\0';

                    totsize += de[i].size;
                    totfile++;

                    if (sameline && !areadone) {
                        areadone = 1;
                        m_print(bbstxt[B_ONE_CR]);
                        if ((line = file_more_question(line, only_one)) == 0) {
                            break;
                        }
                    }

                    if (sys.no_filedate) {
                        m_print(bbstxt[B_FILES_NODATE], strupr(name), de[i].size, desc);
                    }
                    else {
                        m_print(bbstxt[B_FILES_FORMAT], strupr(name), de[i].size, show_date(config->dateformat, e_input, de[i].date, 0), desc);
                    }

                    if (!(line = file_more_question(line, only_one)) || !CARRIER) {
                        break;
                    }
                }
            } while ((p = strtok(NULL, "\r\n")) != NULL);

        free(de);
        free(fbuf);

        if (only_one || !CARRIER) {
            break;
        }
    }

    close(fd);

    memcpy((char *)&sys.file_name, (char *)&vsys.file_name, SIZEOF_FILEAREA);

    if (sameline && !areadone) {
        m_print(bbstxt[B_ONE_CR]);
    }

    m_print(bbstxt[B_LOCATED_MATCH], totfile, totsize);

    if (line && CARRIER) {
        file_more_question(usr.len, only_one);
    }
}

void download_report(char * rqname, int id, char * filelist)
{
    FILE * fp, *fp2;
    char string[128];
    char drive[80], path[80], name[16], ext[6];
    struct ffblk blk;

    if (rqname != NULL) {
        strupr(rqname);
        fnsplit(rqname, drive, path, name, ext);
        strcat(drive, path);
        strcat(name, ext);
        strupr(name);
    }

    sprintf(string, "DL-REP%d.BBS", line_offset);
    fp = fopen(string, (id == 1) ? "wt" : "at");
    if (fp == NULL) {
        return;
    }

    if (id == 2) {
        strcat(drive, "FILES.BBS");

        if (findfirst(rqname, &blk, 0)) {
            sprintf(string, bbstxt[B_REPORT_SUCCESFUL], name, cps, "* Not found *");
        }

        else {
            if ((fp2 = fopen(drive, "rt")) == NULL && filelist != NULL && *filelist) {
                fp2 = fopen(filelist, "rt");
            }

            if (tagged_dnl) {
                tagged_kb -= (blk.ff_fsize / 1024);
                if (tagged_kb < 0) {
                    tagged_kb = 0;
                }
            }

            sprintf(string, bbstxt[B_REPORT_SUCCESFUL], name, cps, "* No description *");

            if (fp2 != NULL) {
                while (!feof(fp2)) {
                    path[0] = 0;
                    if (fgets(path, 70, fp2) == NULL) {
                        break;
                    }
                    if (!strnicmp(path, name, strlen(name))) {
                        path[strlen(path) - 1] = 0;
                        sprintf(string, bbstxt[B_REPORT_SUCCESFUL], name, cps, &path[13]);
                        break;
                    }
                }

                fclose(fp2);
            }
        }

        fwrite(string, strlen(string), 1, fp);
    }
    else if (id == 3) {
        fprintf(fp, bbstxt[B_REPORT_NOT_SENT], name, cps);
    }

    else if (id == 4) {
        fprintf(fp, "\n");
    }

    fclose(fp);

    if (id == 4) {
        sprintf(string, "DL-REP%d.BBS", line_offset);
        read_file(string);
        unlink(string);
        timer(30);
    }
}

void download_file(st, global)
char * st;
int global;
{
    FILE * fp1, *fp2;

    int fd, i, m, protocol, killafter = 0;
    char filename[80], path[80], dir[80], name[80], ext[5], logoffafter;
    char old_filename[80], old_path[80], *copybuf;
    long rp, t, length, blocks, j;
    struct ffblk blk;
    struct _sys tsys;
    tagged_dnl = 0;

#if defined (__OCC__) || defined (__TCPIP__)
    if (tcpip && usr.priv < SYSOP) {
        m_print("\n\026\001\014Sorry, file transfer not available on TCP/IP\n");
        press_enter();
        return;
    }
#endif

    fnsplit(st, path, dir, name, ext);
    strcat(path, dir);
    strcat(name, ext);

    if (!offline_reader && !sys.freearea && !usr.xfer_prior && config->class[usr_class].ratio && !name[0] && usr.n_dnld >= config->class[usr_class].start_ratio) {
        rp = usr.upld ? (usr.dnld / usr.upld) : usr.dnld;
        if (rp > (long)config->class[usr_class].ratio) {
            cmd_string[0] = '\0';
            status_line(":Dnld req. would exceed ratio");
            read_system_file("RATIO");
            return;
        }
    }

    killafter = 0;

    if (global == -1) {
        global = 0;
        killafter = 1;
    }
    else if (global != -2 && global != -3) {
        if (download_tagged_files(st)) {
            return;
        }
    }
    else {
        global = (global == -2) ? 0 : 1;
    }

    if (user_status != UPLDNLD && user_status != 7) {
        set_useron_record(UPLDNLD, 0, 0);
    }

    if (user_status != 7) {
        read_system_file("PREDNLD");
    }

    if (st == NULL) {
        cls();
    }
    if ((protocol = selprot()) == 0) {
        return;
    }

    if (!dir[0] && st != NULL) {
        strcpy(path, sys.filepath);
    }

    if (!name[0]) {
        if (!cmd_string[0]) {
            ask_for_filename(name, 0);
            if (!name[0] || !CARRIER) {
                return;
            }
        }
        else {
            strcpy(name, cmd_string);
            cmd_string[0] = '\0';
        }
    }

    if (!display_transfer(protocol, path, name, global)) {
        return;
    }

    m_print(bbstxt[B_DL_CONFIRM]);
    if (usr.hotkey) {
        cmd_input(filename, 1);
    }
    else {
        chars_input(filename, 1, 0);
    }

    if (toupper(filename[0]) == '!') {
        logoffafter = 1;
    }
    else {
        logoffafter = 0;
    }

    if (toupper(filename[0]) == 'A') {
        m = 0;
        logoffafter = 0;
        goto abort_xfer;
    }

    m_print(bbstxt[B_READY_TO_SEND]);
    m_print(bbstxt[B_CTRLX_ABORT]);

    if (local_mode) {
        sysop_error();
        m = 0;
        goto abort_xfer;
    }


    download_report(NULL, 1, NULL);

    if (protocol == 1 || protocol == 2 || protocol == 3 || protocol == 6) {
        timer(10);
        m = i = 0;

        while (get_string(name, dir) != NULL) {
            sprintf(filename, "%s%s", path, dir);

            if (findfirst(filename, &blk, 0)) {
                sprintf(filename, "%sSYSFILE.DAT", config->sys_path);
                fd = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);

                if (global) {
                    while (read(fd, (char *)&tsys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA) {
                        if (usr.priv < tsys.file_priv) {
                            continue;
                        }
                        if ((usr.flags & tsys.file_flags) != tsys.file_flags) {
                            continue;
                        }
                        if (usr.priv < tsys.download_priv) {
                            continue;
                        }
                        if ((usr.flags & tsys.download_flags) != tsys.download_flags) {
                            continue;
                        }

                        sprintf(filename, "%s%s", tsys.filepath, dir);

                        if (!findfirst(filename, &blk, 0)) {
                            break;
                        }
                    }
                }

                close(fd);
            }
            else {
                memcpy(&tsys.file_name, &sys.file_name, SIZEOF_FILEAREA);
            }
            if (tsys.cdrom && config->cdrom_swap && !offline_reader) {

                char t_name[20];
                struct ftime ft;
                int c;

                strcpy(old_filename, filename);
                fp1 = fopen(old_filename, "rb");
                if (!fp1) {
                    status_line("!Unable to open %s", old_filename);
                    return;
                }
                sprintf(filename, "%scdrom%d", config->sys_path, line_offset);
                mkdir(filename);
                fnsplit(old_filename, NULL, NULL, t_name, ext);
                strcat(t_name, ext);
                sprintf(filename, "%scdrom%d\\%s", config->sys_path, line_offset, t_name);
                fp2 = fopen(filename, "wb");
                if (!fp2) {
                    status_line("!Unable to open %s", filename);
                    return;
                }
                length = filelength(fileno(fp1));
                blocks = length / (COPYBUFFER);
                copybuf = malloc(COPYBUFFER);
                for (j = 0; j < blocks; j++) {
                    fread(copybuf, COPYBUFFER, 1, fp1);
                    fwrite(copybuf, COPYBUFFER, 1, fp2);
                }
                fread(copybuf, length % COPYBUFFER, 1, fp1);
                fwrite(copybuf, length % COPYBUFFER, 1, fp2);
                /*				while((c=fgetc(fp1))!=EOF)
                					fputc((char)c,fp2);
                */
                getftime(fileno(fp1), &ft);
                setftime(fileno(fp2), &ft);
                free(copybuf);
                fclose(fp1);
                fclose(fp2);
                status_line("+Copied %s -> %s", old_filename, filename);
            }

            switch (protocol) {
                case 1:
                    m = fsend(filename, 'X');
                    break;
                case 2:
                    m = fsend(filename, 'Y');
                    break;
                case 3:
                    m = send_Zmodem(filename, dir, i, 0);
                    break;
                case 6:
                    m = fsend(filename, 'S');
                    break;
            }

            if (tsys.cdrom && config->cdrom_swap && !offline_reader) {
                unlink(filename);
                status_line("+Removed %s", filename);
                strcpy(filename, old_filename);
            }

            if (!m) {
                download_report(filename, 3, tsys.filelist);
                break;
            }

            if (config->keep_dl_count) {
                update_filestat(filename);
            }
            download_report(filename, 2, tsys.filelist);
            i++;

            if (!tsys.freearea || st == NULL) {
                usr.dnld += (int)(blk.ff_fsize / 1024L) + 1;
                usr.dnldl += (int)(blk.ff_fsize / 1024L) + 1;
                usr.n_dnld++;

                if (function_active == 3) {
                    f3_status();
                }
            }

            if (killafter) {
                file_kill(1, dir);
            }
        }

        if (protocol == 3) {
            send_Zmodem(NULL, NULL, ((i) ? END_BATCH : NOTHING_TO_DO), 0);
        }
        else if (protocol == 6) {
            fsend(NULL, 'S');
        }

abort_xfer:
        wactiv(mainview);

        m_print(bbstxt[B_TWO_CR]);

        if (m) {
            m_print(bbstxt[B_TRANSFER_OK]);
        }
        else {
            m_print(bbstxt[B_TRANSFER_ABORT]);
        }
    }

    else if (protocol >= 10) {

        if (sys.cdrom && (global != 1) && config->cdrom_swap && !offline_reader) {

            int c;
            struct ftime ft;

            strcpy(old_path, path);
            sprintf(old_filename, "%s%s", path, name);
            fp1 = fopen(old_filename, "rb");
            if (!fp1) {
                status_line("!Unable to open %s", old_filename);
                return;
            }

            sprintf(filename, "%scdrom%d", config->sys_path, line_offset);
            mkdir(filename);
            sprintf(filename, "%scdrom%d\\%s", config->sys_path, line_offset, name);
            fp2 = fopen(filename, "wb");
            if (!fp2) {
                status_line("!Unable to open %s", filename);
                return;
            }
            length = filelength(fileno(fp1));
            blocks = length / (COPYBUFFER);
            copybuf = malloc(COPYBUFFER);
            for (j = 0; j < blocks; j++) {
                fread(copybuf, COPYBUFFER, 1, fp1);
                fwrite(copybuf, COPYBUFFER, 1, fp2);
            }
            fread(copybuf, length % COPYBUFFER, 1, fp1);
            fwrite(copybuf, length % COPYBUFFER, 1, fp2);/*				while((c=fgetc(fp1))!=EOF)
				fputc((char)c,fp2);
*/
            getftime(fileno(fp1), &ft);
            setftime(fileno(fp2), &ft);
            free(copybuf);
            fclose(fp1);
            fclose(fp2);
            status_line("+Copied %s -> %s", old_filename, filename);
            sprintf(path, "%scdrom%d\\", config->sys_path, line_offset);
        }

        general_external_protocol(name, path, protocol, killafter ? -1 : global, 1);

        if (sys.cdrom && config->cdrom_swap && !offline_reader) {
            unlink(filename);
            status_line("+Removed %s", filename);
            strcpy(path, old_path);
        }
    }

    download_report(NULL, 4, NULL);

    if (logoffafter) {
        if (user_status == 7) {
            update_lastread_pointers();
        }

        rp = time(NULL) + 10L;
        do {
            m = -1;
            m_print(bbstxt[B_DL_LOGOFF], (int)(rp - time(NULL)));
            t = time(NULL);
            while (time(NULL) == t && CARRIER) {
                release_timeslice();
                if ((m = toupper(m_getch())) == 'A') {
                    break;
                }
            }
            if (m == 'A') {
                break;
            }
        } while (time(NULL) < rp && CARRIER);

        if (m != 'A') {
            read_system_file("FGOODBYE");

            hidecur();
            terminating_call();
            get_down(aftercaller_exit, 2);
        }
    }
}

int display_transfer(protocol, path, name, flag)
int protocol;
char * path, *name;
int flag;
{
    int fd, i, byte_sec, min, sec, nfiles, errfl;
    char filename[80], root[14], back[80];
    long fl;
    struct ffblk blk;
    struct _sys tsys;

    back[0] = '\0';
    fl = 0L;
    nfiles = 0;
    errfl = 0;

    while (get_string(name, root) != NULL) {
        sprintf(filename, "%s%s", path, root);

        if (findfirst(filename, &blk, 0)) {
            sprintf(filename, "%sSYSFILE.DAT", config->sys_path);
            fd = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);

            if (flag) {
                i = 0;

                while (read(fd, (char *)&tsys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA) {
                    if (usr.priv < tsys.file_priv) {
                        continue;
                    }
                    if ((usr.flags & tsys.file_flags) != tsys.file_flags) {
                        continue;
                    }
                    if (usr.priv < tsys.download_priv) {
                        continue;
                    }
                    if ((usr.flags & tsys.download_flags) != tsys.download_flags) {
                        continue;
                    }

                    sprintf(filename, "%s%s", tsys.filepath, root);

                    if (findfirst(filename, &blk, 0)) {
                        continue;
                    }

                    do {
                        if ((((fl + blk.ff_fsize) / 1024) + usr.dnldl) > config->class[usr_class].max_dl && !tsys.freearea && !usr.xfer_prior) {
                            status_line(":Dnld req. would exceed limit (%s)", blk.ff_name);
                            errfl = 1;
                            read_system_file("TODAYK");
                            break;
                        }

                        if (strlen(back) + strlen(blk.ff_name) + 1 < 79) {
                            if (nfiles) {
                                strcat(back, " ");
                            }
                            strcat(back, blk.ff_name);
                            fl += blk.ff_fsize;
                            nfiles++;
                            i = 1;
                        }
                    } while (!findnext(&blk));
                }

                if (!i) {
                    m_print(bbstxt[B_FILE_NOT_FOUND], strupr(root));
                }
            }
            else {
                m_print(bbstxt[B_FILE_NOT_FOUND], strupr(root));
            }

            close(fd);
        }
        else
            do {
                if ((((fl + blk.ff_fsize) / 1024) + usr.dnldl) > config->class[usr_class].max_dl && !offline_reader && !sys.freearea && !usr.xfer_prior) {
                    status_line(":Dnld req. would exceed limit (%s)", blk.ff_name);
                    errfl = 1;
                    read_system_file("TODAYK");
                    break;
                }

                if (strlen(back) + strlen(blk.ff_name) + 1 < 79) {
                    if (nfiles) {
                        strcat(back, " ");
                    }
                    strcat(back, blk.ff_name);
                    fl += blk.ff_fsize;
                    nfiles++;
                }
            } while (!findnext(&blk));
    }

    if (!back[0]) {
        return (0);
    }

    cls();

    m_print(bbstxt[B_FILE_MASK], strupr(back));
    m_print(bbstxt[B_LEN_MASK], fl);

    byte_sec = (int)(rate / 11);
    i = (int)((fl + (fl / 1024)) / byte_sec);
    min = i / 60;
    sec = i - min * 60;

    m_print(bbstxt[B_TOTAL_TIME], min, sec);
    if (protocol >= 10) {
        display_external_protocol(protocol);
    }
    else {
        m_print(bbstxt[B_PROTOCOL], &protocols[protocol - 1][1]);
    }

    if (((fl / 1024) + usr.dnldl) > config->class[usr_class].max_dl && !offline_reader && !sys.freearea && !usr.xfer_prior) {
        status_line(":Dnld req. would exceed limit");
        read_system_file("TODAYK");
        return (0);
    }
    else if (errfl && nfiles == 1) {
        read_system_file("TODAYK");
        return (0);
    }

    if (min > time_remain() && !sys.freearea && !usr.xfer_prior) {
        status_line(":Dnld req. would exceed limit");
        read_system_file("NOTIME");
        return (0);
    }

    strcpy(name, back);
    return (1);
}

void ask_for_filename(name, dotag)
char * name;
int dotag;
{
    char stringa[80];

    name[0] = '\0';

    for (;;) {
        if (dotag) {
            m_print(bbstxt[B_TAG_FILE_NAME]);
        }
        else {
            m_print(bbstxt[B_DOWNLOAD_NAME]);
        }
        input(stringa, 79);
        if (!stringa[0] || !CARRIER) {
            return;
        }

        if (invalid_filename(stringa) && usr.priv < SYSOP) {
            m_print(bbstxt[B_INVALID_FILENAME]);
            status_line(msgtxt[M_DL_PATH], stringa);
        }
        else {
            break;
        }
    }

    strcpy(name, stringa);
}

int invalid_filename(f)
char * f;
{
    if (strchr(f, '\\') != NULL) {
        return (1);
    }
    if (strchr(f, ':') != NULL) {
        return (1);
    }

    return (0);
}

int check_upload_names(char * origname, char * path)
{
    FILE * fp;
    int found;
    char filename[80], stringa[80], *p, name[14], our_wildcard[14], their_wildcard[14];
    struct ffblk blk;
    struct _sys tsys;
    FILEIDX fidx;

    strcpy(name, origname);
    sprintf(filename, "%sNOCHECK.CFG", config->sys_path);
    if ((fp = fopen(filename, "rt")) != NULL) {
        while (fgets(stringa, 78, fp) != NULL) {
            stripcrlf(stringa);
            if (!stricmp(stringa, name)) {
                fclose(fp);
                return (0);
            }
        }
        fclose(fp);
    }

    if ((p = strchr(name, '.')) != NULL) {
        if (!stricmp(p, ".ZIP") || !stricmp(p, ".ARJ") || !stricmp(p, ".LZH") || !stricmp(p, ".LHA") || !stricmp(p, ".ARC")) {
            strcpy(p, ".*");
        }
        else if (!stricmp(p, ".GIF") || !stricmp(p, ".JPG") || !stricmp(p, ".ZOO")) {
            strcpy(p, ".*");
        }
    }

    sprintf(filename, "%s%s", sys.uppath, name);
    if (!findfirst(filename, &blk, 0)) {
        m_print(bbstxt[B_ALREADY_HAVE], strupr(origname), sys.file_num, sys.file_name);
        return (-1);
    }

//   sprintf (filename, "%sSYSFILE.DAT", config->sys_path);
//   if ((fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) == -1)
//      return (0);

    found = 0;
    prep_match(name, their_wildcard);
    if ((fp = sh_fopen("FILES.IDX", "rb", SH_DENYNONE)) != NULL) {
        while (fread(&fidx, sizeof(FILEIDX), 1, fp) == 1) {
            prep_match(fidx.name, our_wildcard);
            if (!match(our_wildcard, their_wildcard)) {
                found = 1;
                if (read_system2(fidx.area, 2, &tsys)) {
                    m_print(bbstxt[B_ALREADY_HAVE], name, tsys.file_num, tsys.file_name);
                }
                break;
            }
        }
        fclose(fp);
    }

    if (!found) {
        prep_match(name, their_wildcard);
        if ((fp = sh_fopen("CDROM.IDX", "rb", SH_DENYNONE)) != NULL) {
            while (fread(&fidx, sizeof(FILEIDX), 1, fp) == 1) {
                prep_match(fidx.name, our_wildcard);
                if (!match(our_wildcard, their_wildcard)) {
                    found = 1;
                    if (read_system2(fidx.area, 2, &tsys)) {
                        m_print(bbstxt[B_ALREADY_HAVE], name, tsys.file_num, tsys.file_name);
                    }
                    break;
                }
            }
            fclose(fp);
        }
    }
    /*
    	while (read (fd, (char *)&tsys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA) {
    		if (!stricmp (tsys.filepath, path))
    			continue;
    		sprintf (filename, "%s%s", tsys.filepath, name);
    		if (!findfirst (filename, &blk, 0)) {
    			m_print (bbstxt[B_ALREADY_HAVE], name, tsys.file_num, tsys.file_name);
    			close (fd);
    			return (-1);
    		}
    	}
    */

//   close (fd);
    return (0);
}

void ask_file_description(char * name)
{
    FILE * fp;
    char stringa[80], filename[80];

    m_print(bbstxt[B_DESCRIBE], strupr(name));
    input(stringa, 54);
    if (!CARRIER || !stringa[0]) {
        return;
    }

    sprintf(filename, "DESC%d.TMP", config->line_offset);
    fp = fopen(filename, "at");
    fprintf(fp, "%-12s %s\n", name, stringa);
    fclose(fp);
}

int check_file_description(char * name, char * uppath)
{
    FILE * fp;
    char * p, stringa[80], filename[80];

    sprintf(filename, "DESC%d.TMP", config->line_offset);
    if ((fp = fopen(filename, "rt")) == NULL) {
        return (0);
    }

    while (fgets(stringa, 78, fp) != NULL) {
        stripcrlf(stringa);
        p = strchr(stringa, ' ');
        *p++ = '\0';

        if (!stricmp(stringa, name)) {
            p = strbtrim(p);
            sprintf(filename, "%sFILES.BBS", uppath);

            fp = fopen(filename, "at");
            sprintf(filename, "%c  0%c ", config->dl_counter_limits[0], config->dl_counter_limits[1]);
            fprintf(fp, "%-12s %s%s\n", strupr(name), config->keep_dl_count ? filename : "", p);
            if (config->put_uploader) {
                fprintf(fp, bbstxt[B_UPLOADER_NAME], usr.name);
            }
            fclose(fp);

            return (-1);
        }
    }

    fclose(fp);
    return (0);
}

void upload_file(char * st, char pr)
{
    FILE * xferinfo, *fp;
    int i, protocol;
    char filename[80], path[80], stringa[80], name[20], ext[5], *file, doupload;
    long fpos, start_upload;
    struct ffblk blk;
    PROTOCOL prot;

#if defined (__OCC__) || defined (__TCPIP__)
    if (tcpip && usr.priv < SYSOP) {
        m_print("\n\026\001\014Sorry, file transfer not available on TCP/IP\n");
        press_enter();
        return;
    }
#endif

    if (pr == 0 || pr == -1) {
        if (pr == 0) {
            read_system_file("PREUPLD");
        }

        if ((protocol = selprot()) == 0) {
            return;
        }
    }
    else {
        protocol = pr;
    }

    name[0] = '\0';

    if (st == NULL || !st[0]) {
        strcpy(path, sys.uppath);
    }
    else {
        fnsplit(st, filename, stringa, name, ext);
        strcat(filename, stringa);
        strcat(name, ext);

        if (filename[0]) {
            strcpy(path, filename);
        }
        else {
            strcpy(path, sys.uppath);
        }
    }

    if (zfree(path) <= config->ul_free_space) {
        read_system_file("ULSPACE");
        return;
    }

    if (protocol >= 10) {
        sprintf(filename, "%sPROTOCOL.DAT", config->sys_path);
        i = sh_open(filename, O_RDONLY | O_BINARY, SH_DENYWR, S_IREAD | S_IWRITE);
        lseek(i, (long)(protocol - 10) * sizeof(PROTOCOL), SEEK_SET);
        read(i, &prot, sizeof(PROTOCOL));
        close(i);
    }

    if (!no_precheck && !name[0]) {
        if (!get_command_word(name, 14)) {
            m_print(bbstxt[B_UPLOAD_NAME]);
            input(name, 14);
            if (!name[0] || !CARRIER) {
                return;
            }
        }

        if ((i = check_upload_names(name, "")) == 0) {
            ask_file_description(name);
        }
        doupload = i ? 0 : 1;

        if ((protocol >= 10 && prot.batch) || (protocol != 1 && protocol != 2))
            for (;;) {
                m_print(bbstxt[B_UPLOAD_NAME]);
                input(name, 14);
                if (!name[0] || !CARRIER) {
                    break;
                }

                if ((i = check_upload_names(name, "")) == 0) {
                    ask_file_description(name);
                }
                doupload += i ? 0 : 1;
            }

        if (!doupload || !CARRIER) {
            return;
        }
    }
    else if (!name[0]) {
        if ((protocol >= 10 && !prot.batch) || protocol == 1 || protocol == 2) {
            if (!get_command_word(name, 14)) {
                m_print(bbstxt[B_UPLOAD_NAME]);
                input(name, 14);
                if (!name[0] || !CARRIER) {
                    return;
                }
            }
        }
    }

    if (pr == 0 || pr == -1) {
        if (protocol >= 10) {
            m_print(bbstxt[B_READY_TO_RECEIVE], prot.name);
        }
        else {
            m_print(bbstxt[B_READY_TO_RECEIVE], &protocols[protocol - 1][1]);
        }
        m_print2(bbstxt[B_CTRLX_ABORT]);
    }

    start_upload = time(NULL);

    if (local_mode) {
        sysop_error();
        i = 0;
        file = NULL;
        goto abort_xfer;
    }


    if (protocol == 1 || protocol == 2 || protocol == 3 || protocol == 6) {
        timer(50);

        sprintf(filename, "XFER%d", line_offset);
        xferinfo = fopen(filename, "w+t");

        i = 0;
        file = NULL;

        switch (protocol) {
            case 1:
                who_is_he = 0;
                overwrite = 0;
                file = receive(path, name, 'X');
                if (file != NULL) {
                    fprintf(xferinfo, "%s\n", file);
                }
                break;

            case 2:
                who_is_he = 0;
                overwrite = 0;
                file = receive(path, name, 'Y');
                if (file != NULL) {
                    fprintf(xferinfo, "%s\n", file);
                }
                break;

            case 3:
                i = get_Zmodem(path, xferinfo);
                break;

            case 6:
                i = 0;
                do {
                    who_is_he = 0;
                    overwrite = 0;
                    file = receive(path, NULL, 'S');
                    if (file != NULL) {
                        fprintf(xferinfo, "%s\n", file);
                        i++;
                    }
                } while (file != NULL);
                break;

        }

abort_xfer:
        if (!CARRIER) {
            fclose(xferinfo);
            sprintf(filename, "XFER%d", line_offset);
            unlink(filename);
            sprintf(filename, "DESC%d.TMP", config->line_offset);
            unlink(filename);
            return;
        }

        wactiv(mainview);

        allowed += (int)((time(NULL) - start_upload) / 60);

        CLEAR_INBOUND();
        if (i || file != NULL) {
            m_print(bbstxt[B_TRANSFER_OK]);
            timer(10);
        }
        else {
            m_print(bbstxt[B_TRANSFER_ABORT]);
            fclose(xferinfo);
            sprintf(filename, "XFER%d", line_offset);
            unlink(filename);
            sprintf(filename, "DESC%d.TMP", config->line_offset);
            unlink(filename);
            press_enter();
            return;
        }

        // Chiede le descrizioni dei files mandati
        if (!no_description) {
            rewind(xferinfo);
            fpos = filelength(fileno(xferinfo));

            while (ftell(xferinfo) < fpos) {
                fgets(filename, 78, xferinfo);
                filename[strlen(filename) - 1] = '\0';

                fnsplit(filename, NULL, NULL, name, ext);
                strcat(name, ext);

                sprintf(filename, "%s%s", path, name);
                if (!dexists(filename)) {
                    continue;
                }

                if (!check_file_description(name, path)) {
                    do {
                        m_print(bbstxt[B_DESCRIBE], strupr(name));
                        input(stringa, 54);
                        if (!CARRIER) {
                            fclose(xferinfo);
                            sprintf(filename, "XFER%d", line_offset);
                            unlink(filename);
                            sprintf(filename, "DESC%d.TMP", config->line_offset);
                            unlink(filename);
                            return;
                        }
                    } while (strlen(stringa) < 5);

                    sprintf(filename, "%sFILES.BBS", path);
                    fp = fopen(filename, "at");
                    sprintf(filename, "%c  0%c ", config->dl_counter_limits[0], config->dl_counter_limits[1]);
                    fprintf(fp, "%-12s %s%s\n", name, config->keep_dl_count ? filename : "", stringa);
                    if (config->put_uploader) {
                        fprintf(fp, bbstxt[B_UPLOADER_NAME], usr.name);
                    }
                    fclose(fp);
                }
            }
        }

        // Effettua il controllo dei files ed eventualmente cambia le
        // descrizioni inviate dall'uploader
        if (!no_check) {
            check_uploads(xferinfo, path);
            m_print(bbstxt[B_THANKS], usr.name);
        }

        // Aggiorna i contatori dell'utente con i kbytes mandati e il numero
        // di files totali.
        rewind(xferinfo);
        fpos = filelength(fileno(xferinfo));

        while (ftell(xferinfo) < fpos) {
            fgets(filename, 78, xferinfo);
            filename[strlen(filename) - 1] = '\0';

            fnsplit(filename, NULL, NULL, name, ext);
            strcat(name, ext);

            sprintf(filename, "%s\\%s", path, name);
            if (!findfirst(filename, &blk, 0)) {
                usr.upld += (int)(blk.ff_fsize / 1024L) + 1;
                usr.n_upld++;

                if (function_active == 3) {
                    f3_status();
                }
            }
        }

        fclose(xferinfo);
    }

    else if (protocol >= 10) {
        general_external_protocol(prot.batch ? NULL : name, path, protocol, 0, 0);
    }

    sprintf(filename, "XFER%d", line_offset);
    unlink(filename);
    sprintf(filename, "DESC%d.TMP", config->line_offset);
    unlink(filename);
}

int selprot(void)
{
    int fd, i, cmdpos = 0;
    char c, stringa[4], extcmd[50], filename[50];
    PROTOCOL prot;

    cmdpos = 0;
    sprintf(filename, "%sPROTOCOL.DAT", config->sys_path);
    fd = sh_open(filename, O_RDONLY | O_BINARY, SH_DENYNONE, S_IREAD | S_IWRITE);
    if (fd != -1) {
        while (read(fd, &prot, sizeof(PROTOCOL)) == sizeof(PROTOCOL)) {
            if (prot.active) {
                extcmd[cmdpos++] = prot.hotkey;
            }
            else {
                extcmd[cmdpos++] = ' ';
            }
        }
        close(fd);
    }
    extcmd[cmdpos] = '\0';

    for (;;) {
        if ((c = get_command_letter()) == '\0') {
            if (!usr.protocol) {
                if (!read_system_file("XFERPROT")) {
                    m_print(bbstxt[B_PROTOCOLS]);
                    if (config->prot_xmodem || no_external) {
                        m_print(bbstxt[B_PROTOCOL_FORMAT], protocols[0][0], &protocols[0][1]);
                    }
                    if (config->prot_1kxmodem || no_external) {
                        m_print(bbstxt[B_PROTOCOL_FORMAT], protocols[1][0], &protocols[1][1]);
                    }
                    if (config->prot_zmodem || no_external) {
                        m_print(bbstxt[B_PROTOCOL_FORMAT], protocols[2][0], &protocols[2][1]);
                    }
                    if (config->prot_sealink || no_external) {
                        m_print(bbstxt[B_PROTOCOL_FORMAT], protocols[5][0], &protocols[5][1]);
                    }

                    if (!no_external) {
                        sprintf(filename, "%sPROTOCOL.DAT", config->sys_path);
                        fd = sh_open(filename, O_RDONLY | O_BINARY, SH_DENYWR, S_IREAD | S_IWRITE);
                        if (fd != -1) {
                            while (read(fd, &prot, sizeof(PROTOCOL)) == sizeof(PROTOCOL)) {
                                if (prot.active) {
                                    m_print(bbstxt[B_PROTOCOL_FORMAT], prot.hotkey, prot.name);
                                }
                            }
                            close(fd);
                        }
                    }

                    if (exist_system_file("XFERHELP")) {
                        m_print(bbstxt[B_PROTOCOL_FORMAT], bbstxt[B_HELP][0], &bbstxt[B_HELP][1]);
                    }
                    m_print(bbstxt[B_SELECT_PROT]);
                }

                if (usr.hotkey) {
                    cmd_input(stringa, 1);
                    m_print(bbstxt[B_ONE_CR]);
                }
                else {
                    input(stringa, 1);
                }
                c = stringa[0];
                if (c == '\0' || c == '\r' || !CARRIER) {
                    return (0);
                }
            }
            else {
                c = usr.protocol;
            }
        }

        if (!no_external) {
            for (i = 0; i < cmdpos; i++)
                if (toupper(c) == toupper(extcmd[i])) {
                    break;
                }
            if (i < cmdpos) {
                return (10 + i);
            }
        }

        switch (toupper(c)) {
            case 'X':
                if (config->prot_xmodem || no_external) {
                    return (1);
                }
                break;
            case '1':
                if (config->prot_1kxmodem || no_external) {
                    return (2);
                }
                break;
            case 'Z':
                if (config->prot_zmodem || no_external) {
                    return (3);
                }
                break;
            case 'S':
                if (config->prot_sealink || no_external) {
                    return (6);
                }
                break;
            case '?':
                read_system_file("XFERHELP");
                break;
            default:
                if (c == '\0') {
                    return (0);
                }
                usr.protocol = '\0';
                break;
        }
    }
}

void SendACK()
{
    char temp[20];

    if ((!recv_ackless) || (block_number == 0)) {
        SENDBYTE(ACK);
        if (sliding)
        {
            SENDBYTE((unsigned char)block_number);
            SENDBYTE((unsigned char)(~(block_number)));
        }
    }

    sprintf(temp, "%5d", block_number);
    if (caller || emulator) {
        wgotoxy(1, 1);
    }
    else {
        wgotoxy(10, 1);
    }
    wputs(temp);

    errs = 0;
}

void SendNAK()
{
    int i;
    long t1;

    errs++;
    real_errs++;
    if (errs > 6) {
        return;
    }

    if (++did_nak > 8) {
        recv_ackless = 0;
    }

    CLEAR_INBOUND();

    if (!recv_ackless) {
        t1 = timerset(3000);
        if ((base_block != block_number) || (errs > 1))
            do {
                i = TIMED_READ(1);
                if (!CARRIER) {
                    return;
                }
                if (timeup(t1)) {
                    break;
                }
                time_release();
            } while (i >= 0);
    }

    if (block_number > base_block) {
        SENDBYTE(NAK);
    }
    else {
        if ((errs < 5) && (!do_chksum)) {
            SENDBYTE('C');
        }
        else {
            do_chksum = 1;
            SENDBYTE(NAK);
        }
    }

    if (sliding) {
        SENDBYTE((unsigned char)block_number);
        SENDBYTE((unsigned char)(~(block_number)));
    }
}

void getblock()
{
    register int i;
    char * sptr, rbuffer[1024];
    unsigned int crc;
    int in_char, is_resend, blockerr;
    unsigned char chksum;
    struct zero_block * testa;

    blockerr = 0;
    is_resend = 0;
    chksum = '\0';
    testa = (struct zero_block *)&rbuffer[0];

    in_char = TIMED_READ(5);
    if (in_char != (block_number & 0xff)) {
        if (in_char < (int)block_number) {
            is_resend = 1;
        }
        else if ((block_number) || (in_char != 1)) {
            blockerr++;
        }
        else {
            block_number = 1;
        }
    }

    i = TIMED_READ(5);
    if ((i & 0xff) != ((~in_char) & 0xff)) {
        blockerr++;
    }

    for (sptr = rbuffer, i = 0; i < block_size; i++, sptr++) {
        in_char = TIMED_READ(5);
        if (in_char < 0) {
            if (CARRIER) {
                SendNAK();
            }
            return;
        }
        sptr[0] = (char)in_char;
    }

    if (do_chksum) {
        for (sptr = rbuffer, i = 0; i < block_size; i++, sptr++) {
            chksum += sptr[0];
        }

        if (TIMED_READ(5) != chksum) {
            blockerr++;
        }
    }
    else {
        int lsb, msb;

        for (sptr = rbuffer, i = crc = 0; i < block_size; i++, sptr++) {
            crc = xcrc(crc, (byte)sptr[0]);
        }

        msb = TIMED_READ(5);
        lsb = TIMED_READ(5);
        if ((lsb < 0) || (msb < 0)) {
            if (!block_number) {
                sliding = 0;
            }
            blockerr++;
        }
        else if (((msb << 8) | lsb) != crc) {
            blockerr++;
        }
    }

    if (blockerr) {
        SendNAK();
    }
    else {
        SendACK();

        if (is_resend) {
            return;
        }

        if (block_number) {
            if (block_number < fsize1) {
                fwrite(rbuffer, block_size, 1, Infile);
            }
            else if (block_number == fsize1) {
                fwrite(rbuffer, fsize2, 1, Infile);
            }
        }
        else if (first_block && !may_be_seadog)
        {
            sptr = &(testa->name[0]);
            if (sptr[0])
            {
                sprintf(final_name, "%s%s", _fpath, sptr);
                sptr = final_name;
                fancy_str(sptr);
                i = strlen(sptr) - 1;
                if ((isdigit(sptr[i])) && (sptr[i - 1] == 'o') && (sptr[i - 2] == 'm') && (sptr[i - 3] == '.')) {
                    got_arcmail = 1;
                }
//                                wgotoxy (0, 13);
//                                wclreol ();
//                                wputs (final_name);
            }
        }
        else if (!first_block) {
            sptr = &(testa->name[0]);
            if (netmail) {
                invent_pkt_name(sptr);
            }

            for (i = 0; ((sptr[i]) && (i < 17)); i++)
                if (sptr[i] <= ' ') {
                    sptr[i] = '\0';
                }

            if (sptr[0])
            {
                sprintf(final_name, "%s%s", _fpath, sptr);
                sptr = final_name;
                fancy_str(sptr);
                i = strlen(sptr) - 1;
                if ((isdigit(sptr[i])) && (sptr[i - 1] == 'o') && (sptr[i - 2] == 'm') && (sptr[i - 3] == '.')) {
                    got_arcmail = 1;
                }
//                                wgotoxy (0, 13);
//                                wclreol ();
//                                wputs (final_name);
            }
            else {
                status_line(msgtxt[M_GRUNGED_HEADER]);
            }

            if (testa->size)
            {
                fsize1 = (int)(testa->size / 128L);
                if ((testa->size % 128) != 0) {
                    fsize2 = (int)(testa->size % 128);
                    ++fsize1;
                }
            }

            if ((rate >= 9600) && testa->noacks && !no_overdrive) {
                recv_ackless = 1;
            }

            first_block = 0;
        }

        block_number++;
    }
}

struct zero_block1 {
    long size;
    long time;
    char name[17];
    char moi[15];
    char noacks;
};

char * receive(char * fpath, char * fname, char protocol)
{
    char tmpname[80], announced;
    int in_char, i, wh = -1;
    long t1;

    announced = 0;
    did_nak = 0;
    wh = -1;

    if (protocol == 'F') {
        may_be_seadog = 1;
        netmail = 0;
        protocol = 'S';
        first_block = 0;
    }
    else if (protocol == 'B') {
        protocol = 'S';
        may_be_seadog = 1;
        netmail = 1;
        first_block = 1;
    }
    else if (protocol == 'T') {
        protocol = 'S';
        may_be_seadog = 1;
        netmail = 0;
        first_block = 1;
    }
    else {
        may_be_seadog = 0;
        netmail = 0;
        first_block = 0;
    }

    _BRK_DISABLE();
    XON_DISABLE();

    fsize1 = 32767;
    if (fname) {
        for (in_char = 0; fname[in_char]; in_char++) {
            if ((fname[in_char] == '*') || (fname[in_char] == '?')) {
                fname[0] = '\0';
            }
        }
    }

    _fpath = fpath;
    sliding = 1;
    base_block = 0;
    do_chksum = 0;
    errs = 0;
    block_size = 128;
    fsize2 = 128;

    switch (protocol) {
        case 'X' :
            base_block = 1;
            sliding = 0;
            break;
        case 'Y' :
            base_block = 1;
            sliding = 0;
            fsize2 = block_size = 1024;
            break;
        case 'S' :
            break;
        case 'T' :
            break;
        case 'M' :
            base_block = 1;
            sliding = 0;
            break;
        default  :
            return NULL;
    }

    block_number = base_block;

    sprintf(tmpname, "%s_TMP%d_.$$$", fpath, line_offset);
    sprintf(final_name, "%s%s", fpath, (fname && fname[0]) ? fname : "UNKNOWN.$$$");

    if (caller || emulator) {
        wh = wopen(16, 0, 19, 79, 1, WHITE | _RED, WHITE | _RED);
        wactiv(wh);
        wtitle(" Transfer Status ", TLEFT, WHITE | _RED);
        wgotoxy(0, 1);
    }
    else {
        wfill(9, 1, 10, 77, ' ', WHITE | _BLACK);
        wgotoxy(9, 1);
    }

    Infile = fopen(tmpname, "wb");
    if (Infile == NULL) {
        if ((caller || emulator) && wh != -1) {
            wclose();
        }
        return (NULL);
    }
    if (isatty(fileno(Infile))) {
        if ((caller || emulator) && wh != -1) {
            wclose();
        }
        fclose(Infile);
        unlink(tmpname);
        return (NULL);
    }

    throughput(0, 0L);

    SendNAK();

    t1 = timerset(300);
    real_errs = 0;

loop_top:
    in_char = TIMED_READ(4);

    switch (in_char) {
        case SOH:
            block_size = 128;
            getblock();
            if (!announced) {
                status_line(" %s-%c %s", msgtxt[M_RECEIVING], protocol, final_name);
                if (caller || emulator) {
                    wgotoxy(0, 1);
                }
                else {
                    wgotoxy(9, 1);
                }
                wputs(msgtxt[M_RECEIVING]);
                wputc('-');
                wputc(protocol);
                wputc(' ');
                wputs(final_name);
                announced = 1;
            }
            t1 = timerset(300);
            break;
        case STX:
            block_size = 1024;
            getblock();
            if (!announced) {
                status_line(" %s-%c %s", msgtxt[M_RECEIVING], protocol, final_name);
                if (caller || emulator) {
                    wgotoxy(0, 1);
                }
                else {
                    wgotoxy(9, 1);
                }
                wputs(msgtxt[M_RECEIVING]);
                wputc('-');
                wputc(protocol);
                wputc(' ');
                wputs(final_name);
                announced = 1;
            }
            t1 = timerset(300);
            break;
        case SYN:
            do_chksum = 1;
            getblock();
            do_chksum = 0;
            t1 = timerset(300);
            break;
        case CAN:
            if (TIMED_READ(2) == CAN) {
                goto fubar;
            }
            t1 = timerset(300);
            break;
        case EOT:
            t1 = timerset(20);
            while (!timeup(t1)) {
                TIMED_READ(1);
                time_release();
            }
            if (block_number || protocol == 'S') {
                goto done;
            }
            else {
                goto fubar;
            }
        default:
            if (!CARRIER) {
                if ((caller || emulator) && wh != -1) {
                    wactiv(wh);
                    wclose();
                }
                fclose(Infile);
                unlink(tmpname);
                return (NULL);
            }

            if (timeup(t1)) {
                SendNAK();
                t1 = timerset(300);
            }
            break;
    }

    if (errs > 14) {
        goto fubar;
    }

    goto loop_top;

fubar:
    if (Infile) {
        fclose(Infile);
        unlink(tmpname);
    }

    CLEAR_OUTBOUND();

    if ((caller || emulator) && wh != -1) {
        wactiv(wh);
        wclose();
        wh = -1;
    }

    send_can();
    status_line("!File not received");

    CLEAR_INBOUND();
    return (NULL);

done:
    recv_ackless = 0;
    SendACK();

    if (Infile) {
        int j, k;
        struct ffblk dta;

        fclose(Infile);

        if (!block_number && protocol == 'S') {
            return (NULL);
        }

        i = strlen(tmpname) - 1;
        j = strlen(final_name) - 1;

        if (tmpname[i] == '.') {
            tmpname[i] = '\0';
        }
        if (final_name[j] == '.') {
            final_name[j] = '\0';
            j--;
        }

        i = 0;
        k = is_arcmail(final_name, j);
        if ((!overwrite) || k) {
            while (rename(tmpname, final_name)) {
                if (isdigit(final_name[j])) {
                    final_name[j]++;
                }
                else {
                    final_name[j] = '0';
                }
                if (!isdigit(final_name[j])) {
                    return (final_name);
                }
                i = 1;
            }
        }
        else {
            unlink(final_name);
            rename(tmpname, final_name);
        }
        if (i) {
            status_line("+Dupe file renamed: %s", final_name);
        }

        if (!findfirst(final_name, &dta, 0)) {
            if (real_errs > 4) {
                status_line("+Corrected %d errors in %d blocks", real_errs, fsize1);
            }
            throughput(1, dta.ff_fsize);
            status_line("+UL-%c %s", protocol, strupr(final_name));

            if ((caller || emulator) && wh != -1) {
                wactiv(wh);
                wclose();
                wh = -1;
            }

            strcpy(final_name, dta.ff_name);
            return (final_name);
        }
    }

    if ((caller || emulator) && wh != -1) {
        wactiv(wh);
        wclose();
    }

    return (NULL);
}

int fsend(fname, protocol)
char * fname, protocol;
{
    register int i, j;
    char * b, rbuffer[1024], temps[80];
    int in_char, in_char1, base, head;
    int win_size, full_window, ackerr, wh = -1;
    FILE * fp;
    long block_timer, ackblock, blknum, last_block, temp;
    struct stat st_stat;
    struct ffblk dta;
    struct zero_block1 * testa;

    if (protocol == 'F') {
        protocol = 'S';
        may_be_seadog = 1;
    }
    else if (protocol == 'B') {
        protocol = 'S';
        may_be_seadog = 1;
    }
    else {
        may_be_seadog = 0;
    }

    fp = NULL;
    sliding = 0;
    ackblock = -1;
    do_chksum = 0;
    errs = 0;
    ackerr = 0;
    real_errs = 0;
    wh = -1;

    full_window = (int)(rate / 400);

    if (small_window && full_window > 6) {
        full_window = 6;
    }

    _BRK_DISABLE();
    XON_DISABLE();

    if ((!fname) || (!fname[0])) {

        for (i = 0; i < 5; i++) {
            switch (in_char = TIMED_READ(5)) {
                case 'C':
                case NAK:
                case CAN:
                    SENDBYTE(EOT);
                    break;

                case TSYNC:
                    return (TSYNC);

                default:
                    if (in_char < ' ') {
                        return (0);
                    }
            }
        }
        return (0);
    }

    strlwr(fname);

    if (!findfirst(fname, &dta, 0)) {
        fp = fopen(fname, "rb");
    }
    else {
        send_can();
        return (0);
    }
    if (isatty(fileno(fp))) {
        fclose(fp);
        return (0);
    }

    testa = (struct zero_block1 *)rbuffer;

    block_size = 128;
    head = SOH;
    switch (protocol) {
        case 'Y':
            base = 1;
            block_size = 1024;
            head = STX;
            break;

        case 'X':
            base = 1;
            break;

        case 'S':
            base = 0;
            break;

        case 'T':
            base = 0;
            head = SYN;
            break;

        case 'M':
            base = 1;
            break;

        default:
            fclose(fp);
            return (0);
    }

    blknum = base;
    last_block = (long)((dta.ff_fsize + ((long)block_size - 1L)) / (long)block_size);
    status_line(msgtxt[M_SEND_MSG], fname, last_block, protocol);
    block_timer = timerset(300);

    if (caller || emulator) {
        wh = wopen(16, 0, 19, 79, 1, WHITE | _RED, WHITE | _RED);
        wactiv(wh);
        wtitle(" Transfer Status ", TLEFT, WHITE | _RED);
        wgotoxy(0, 1);
    }
    else {
        wfill(9, 0, 10, 77, ' ', WHITE | _BLACK);
        wgotoxy(9, 1);
    }

    sprintf(temps, msgtxt[M_SEND_MSG], fname, last_block, protocol);
    wputs(temps);

    if (may_be_seadog) {
        throughput(0, 0L);
        goto sendloop;
    }

    do {
        do_chksum = 0;
        i = TIMED_READ(9);
        switch (i) {
            case NAK:
                do_chksum = 1;
            case 'C' :
            {
                int send_tmp;

                if (((send_tmp = TIMED_READ(4)) >= 0) && (TIMED_READ(2) == ((~send_tmp) & 0xff))) {
                    if (send_tmp <= 1) {
                        sliding = 1;
                    }
                    else {
                        SENDBYTE(EOT);
                        continue;
                    }
                }
                if (may_be_seadog) {
                    sliding = 1;
                }
                errs = 0;
                CLEAR_INBOUND();
                throughput(0, 0L);
                goto sendloop;
            }
            case CAN :
                goto fubar;
            default  :
                if ((errs++) > 15) {
                    goto fubar;
                }
                block_timer = timerset(50);
                while (!timeup(block_timer)) {
                    time_release();
                }
                CLEAR_INBOUND();
        }
    } while (CARRIER);

    goto fubar;

sendloop:
    while (CARRIER) {
        win_size = (blknum < 2) ? 2 : (send_ackless ? 220 : full_window);

        if (blknum <= last_block) {
            memset(rbuffer, 0, block_size);
            if (blknum) {
                if (fseek(fp, ((long)(blknum - 1) * (long)block_size), SEEK_SET) == -1) {
                    goto fubar;
                }
                fread(rbuffer, block_size, 1, fp);
            }
            else {
                block_size = 128;
                testa->size = dta.ff_fsize;
                stat(fname, &st_stat);
                testa->time = st_stat.st_atime;

                strcpy(testa->name, dta.ff_name);

                if (protocol == 'T') {
                    for (i = 0; i < HEADER_NAMESIZE; i++)
                        if (!(testa->name[i])) {
                            testa->name[i] = ' ';
                        }
                    testa->time = dta.ff_ftime;
                }

                strcpy(testa->moi, VERSION);
                if ((rate >= 9600) && (sliding)) {
                    testa->noacks = 1;
                    send_ackless = 1;
                }
                else {
                    testa->noacks = 0;
                    send_ackless = 0;
                }

                if (no_overdrive) {
                    testa->noacks = 0;
                    send_ackless = 0;
                }

                ackless_ok = 0;
            }

            SENDBYTE((unsigned char)head);
            SENDBYTE((unsigned char)(blknum & 0xFF));
            SENDBYTE((unsigned char)(~(blknum & 0xFF)));

            for (b = rbuffer, i = 0; i < block_size; i++, b++) {
                SENDBYTE(*b);
            }

            if ((do_chksum) || (head == SYN)) {
                unsigned char chksum = '\0';
                for (b = rbuffer, i = 0; i < block_size; i++, b++) {
                    chksum += (*b);
                }
                SENDBYTE(chksum);
            }
            else {
                word crc;

                for (b = rbuffer, crc = i = 0; i < block_size; i++, b++) {
                    crc = xcrc(crc, (byte)(*b));
                }

                SENDBYTE((unsigned char)(crc >> 8));
                SENDBYTE((unsigned char)(crc & 0xff));
            }
        }

        block_timer = timerset(3000);

slide_reply:
        if (!sliding) {
            FLUSH_OUTPUT();
            block_timer = timerset(3000);
        }
        else if ((blknum < (ackblock + win_size)) && (blknum < last_block) && (PEEKBYTE() < 0)) {
            if ((send_ackless) && (blknum > 0)) {
                ackblock = blknum;

                if (blknum >= last_block) {
                    if (ackless_ok) {
                        sprintf(temps, "%5ld", last_block);
                        if (caller || emulator) {
                            wgotoxy(1, 1);
                        }
                        else {
                            wgotoxy(10, 1);
                        }
                        wputs(temps);
                        goto done;
                    }

                    blknum = last_block + 1;
                    goto sendloop;
                }

                ++blknum;

                if (!(blknum & 0x001F)) {
                    sprintf(temps, "%5ld", blknum);
                    if (caller || emulator) {
                        wgotoxy(1, 1);
                    }
                    else {
                        wgotoxy(10, 1);
                    }
                    wputs(temps);
                }
            }
            else {
                blknum++;
            }
            goto sendloop;
        }

        if (PEEKBYTE() < 0) {
            if (send_ackless) {
                ackblock = blknum;

                if (blknum >= last_block) {
                    if (ackless_ok) {
                        sprintf(temps, "%5ld", last_block);
                        if (caller || emulator) {
                            wgotoxy(1, 1);
                        }
                        else {
                            wgotoxy(10, 1);
                        }
                        wputs(temps);
                        goto done;
                    }

                    blknum = last_block + 1;
                    goto sendloop;
                }

                ++blknum;

                if ((blknum % 20) == 0) {
                    sprintf(temps, "%5ld", blknum);
                    if (caller || emulator) {
                        wgotoxy(1, 1);
                    }
                    else {
                        wgotoxy(10, 1);
                    }
                    wputs(temps);
                }

                goto sendloop;
            }
        }

reply:
        while (CARRIER && !OUT_EMPTY() && PEEKBYTE() < 0) {
            time_release();
            release_timeslice();
        }

        if ((in_char = TIMED_READ(30)) < 0) {
            goto fubar;
        }

        if (in_char == 'C') {
            do_chksum = 0;
            in_char = NAK;
        }

        if (in_char == CAN) {
            goto fubar;
        }

        if ((in_char > 0) && (sliding)) {
            if (++ackerr >= 10) {
                if (send_ackless) {
                    send_ackless = 0;
                }
            }

            if ((in_char == ACK) || (in_char == NAK)) {
                if ((i = TIMED_READ(2)) < 0) {
                    sliding = 0;
                    if (send_ackless) {
                        send_ackless = 0;
                    }
                }
                else {
                    if ((j = TIMED_READ(2)) < 0) {
                        sliding = 0;
                        if (send_ackless) {
                            send_ackless = 0;
                        }
                    }
                    else if (i == (j ^ 0xff)) {
                        temp = blknum - ((blknum - i) & 0xff);
                        if ((temp <= blknum) && (temp > (blknum - win_size - 10))) {
                            if (in_char == ACK) {
                                if ((head == SYN) && (blknum)) {
                                    head = SOH;
                                }
                                if (send_ackless) {
                                    ackless_ok = 1;
                                    goto slide_reply;
                                }
                                else {
                                    ackblock = temp;
                                }

                                blknum++;

                                if (ackblock >= last_block) {
                                    goto done;
                                }
                                errs = 0;
                            }
                            else {
                                blknum = temp;
                                CLEAR_OUTBOUND();
                                errs++;
                                real_errs++;
                            }
                        }
                    }
                }
            }
            else {
                if (timeup(block_timer) && OUT_EMPTY()) {
                    goto fubar;
                }
                else if (!OUT_EMPTY()) {
                    block_timer = timerset(3000);
                }
                goto slide_reply;
            }
        }

        if (!sliding) {
            if (in_char == ACK) {
                if (blknum == 10) {
                    timer(3);
                }
                if (PEEKBYTE() > 0) {
                    int send_tmp;

                    if (((send_tmp = TIMED_READ(1)) >= 0) && (TIMED_READ(1) == ((~send_tmp) & 0xff))) {
                        sliding = 1;
                        ackblock = send_tmp;
                    }
                }

                if (blknum >= last_block) {
                    goto done;
                }
                blknum++;

                if ((head == SYN) && (blknum)) {
                    head = SOH;
                }

                errs = 0;
            }
            else if (in_char == NAK) {
                errs++;
                real_errs++;
                timer(5);
                CLEAR_OUTBOUND();
            }
            else if (CARRIER) {
                if (!timeup(block_timer)) {
                    goto reply;
                }
                else {
                    goto fubar;
                }
            }
            else {
                goto fubar;
            }
        }

        if (errs > 10) {
            goto fubar;
        }

        temp = (blknum <= last_block) ? blknum : last_block;

        sprintf(temps, "%5ld", temp);
        if (caller || emulator) {
            wgotoxy(1, 1);
        }
        else {
            wgotoxy(10, 1);
        }
        wputs(temps);

        if ((sliding) && (ackblock > 0))
            if (!send_ackless) {
                sprintf(temps, ":%-5ld", ackblock);
                wputs(temps);
            }
    }

fubar:
    CLEAR_OUTBOUND();
    send_can();
    status_line("!%s not sent", fname);

    if (fp) {
        fclose(fp);
    }

    if ((caller || emulator) && wh != -1) {
        wactiv(wh);
        wclose();
        wh = -1;
    }

    return (0);

done:
    CLEAR_INBOUND();
    CLEAR_OUTBOUND();

    SENDBYTE(EOT);

    ackerr = 1;
    blknum = last_block + 1;

    for (i = 0; i < 5; i++) {
        if (!CARRIER) {
            ackerr = 1;
            goto gohome;
        }

        switch (TIMED_READ(5)) {
            case 'C':
            case NAK:
            case CAN:
                if (sliding && ((in_char = TIMED_READ(1)) >= 0)) {
                    in_char1 = TIMED_READ(1);
                    if (in_char == (in_char1 ^ 0xff)) {
                        blknum -= ((blknum - in_char) & 0xff);
                        CLEAR_INBOUND();
                        if (blknum <= last_block) {
                            goto sendloop;
                        }
                    }
                }
                CLEAR_INBOUND();
                SENDBYTE(EOT);
                break;
            case TSYNC:
                ackerr = TSYNC;
                goto gohome;
            case ACK:
                if (sliding && ((in_char = TIMED_READ(1)) >= 0)) {
                    in_char1 = TIMED_READ(1);
                }
                ackerr = 1;
//         SENDBYTE(EOT);
                goto gohome;
        }
    }

gohome:
    if (fp) {
        fclose(fp);
    }

    throughput(1, dta.ff_fsize);

    if (real_errs > 4) {
        status_line("+Corrected %d errors in %ld blocks", real_errs, last_block);
    }
    status_line("+DL-%c %s", protocol, strupr(fname));

    if ((caller || emulator) && wh != -1) {
        wactiv(wh);
        wclose();
        wh = -1;
    }

    return (ackerr);
}

void locate_files(only_one)
int only_one;
{
    int fpd, line, m, z, fd, i, nnd, yr, oldlen, len;
    long totsize, totfile;
    char name[13], desc[70], *p, *fbuf, sameline, areadone;
    char filename[80], parola[30], header[90];
    struct _sys tsys, vsys;
    DIRENT * de;

    if (!get_command_word(parola, 29)) {
        m_print(bbstxt[B_KEY_SEARCH]);
        input(parola, 29);
        if (parola[0] == '\0' || !CARRIER) {
            return;
        }
    }

    cls();
    line = 4;
    totsize = totfile = 0L;
    oldlen = 0;
    sameline = (bbstxt[B_AREALIST_HEADER][strlen(bbstxt[B_AREALIST_HEADER]) - 1] == '\n') ? 0 : 1;
    strcpy(header, bbstxt[B_AREALIST_HEADER]);
    header[strlen(header) - 1] = '\0';

    _BRK_ENABLE();

    m_print(bbstxt[B_CONTROLS]);
    m_print(bbstxt[B_CONTROLS2]);

    sprintf(filename, "%sSYSFILE.DAT", config->sys_path);
    fd = sh_open(filename, SH_DENYNONE, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);
    if (fd == -1) {
        status_line(msgtxt[M_NO_SYSTEM_FILE]);
        return;
    }

    memcpy((char *)&vsys.file_name, (char *)&sys.file_name, SIZEOF_FILEAREA);

    while (read(fd, (char *)&tsys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA && line) {
        if (only_one) {
            memcpy((char *)&tsys.file_name, (char *)&sys.file_name, SIZEOF_FILEAREA);
        }
        else {
            if (usr.priv < tsys.file_priv || tsys.no_global_search) {
                continue;
            }
            if ((usr.flags & tsys.file_flags) != tsys.file_flags) {
                continue;
            }
        }

        memcpy((char *)&sys.file_name, (char *)&tsys.file_name, SIZEOF_FILEAREA);

        if (sameline && (usr.ansi || usr.avatar)) {
            del_line();
        }

        m_print(header, tsys.file_num, tsys.file_name);
        if (!sameline) {
            m_print2("\n");
            if ((line = file_more_question(line, only_one)) == 0) {
                break;
            }
        }
        else {
            areadone = 0;
            if (!usr.ansi && !usr.avatar) {
                len = strlen(tsys.file_name);
                if (oldlen != 0 && len < oldlen) {
                    space(oldlen - len);
                }
                oldlen = len;
            }
            m_print2("\r");
        }

        // Apertura del FILES.BBS utilizzando il path dell'area letta nella
        // struttura locale tsys
        if (!tsys.filelist[0]) {
            sprintf(filename, "%sFILES.BBS", tsys.filepath);
        }
        else {
            strcpy(filename, tsys.filelist);
        }
        if ((fpd = open(filename, O_RDONLY | O_BINARY)) == -1) {
            continue;
        }

        // Se il file e' di 0 bytes non importa leggere altro
        if (filelength(fpd) == 0L) {
            close(fpd);
            continue;
        }

        // Alloca il buffer che andra' a contenere l'intero files.bbs per
        // l'area corrente. Se non riesce ad allocarselo scrive una linea
        // nel log con le informazioni utili (spero) per il debugging.
        if ((fbuf = (char *)malloc((unsigned int)filelength(fpd) + 1)) == NULL) {
#ifdef __OS2__
            status_line("!MEM: new_file_list (%ld)", filelength(fpd));
#else
            status_line("!MEM: new_file_list (%ld / %ld)", filelength(fpd), coreleft());
#endif
            close(fpd);
            continue;
        }

        // Lettura di tutto il files.bbs nel buffer allocato.
        read(fpd, fbuf, (unsigned int)filelength(fpd));
        fbuf[(unsigned int)filelength(fpd)] = '\0';
        close(fpd);

        // Legge tutta la directory in memoria. Se non c'e' spazio sufficiente
        // prosegue (scandir si occupa di segnalare nel log gli errori.
        if ((de = scandir(tsys.filepath, &nnd)) == NULL) {
            free(fbuf);
            continue;
        }

        // La scansione del files.bbs avviene in memoria. Tenendo conto che
        // ogni linea e' separata almeno da un \r (CR, 0x0D) si puo' utilizzare
        // la strtok, trattando l'intero buffer come un insieme di token.
        if ((p = strtok(fbuf, "\r\n")) != NULL)
            do {
                // Nel caso di \r\n (EOL) e' necessario saltare il \n per evitare
                // problemi nel riconoscimento dei files.
                if (p[0] == '\0') {
                    continue;
                }
                if (p[0] == '\n') {
                    p++;
                }

                if (!CARRIER || (!local_mode && RECVD_BREAK())) {
                    line = 0;
                    break;
                }

                // Se il primo carattere non e' alfanumerico si tratta di un
                // commento, pertanto non e' necessario visualizzarlo.
                if (!isalnum(p[0])) {
                    continue;
                }

                // Se la stringa di ricerca viene trovata in un punto qualsiasi
                // della linea di descrizione si prosegue.
                if (stristr(p, parola) != NULL) {
                    // Estrae il nome del file dalla stringa puntata da p
                    m = 0;
                    while (p[m] != ' ' && m < 12) {
                        name[m] = p[m++];
                    }
                    name[m] = '\0';

                    // Cerca il file nella lista in memoria puntata da de.
                    for (i = 0; i < nnd; i++) {
                        if (!stricmp(de[i].name, name)) {
                            break;
                        }
                    }

                    // Il file non e' presente nella directory fisica.
                    if (i >= nnd) {
                        if (!config->show_missing) {
                            continue;
                        }
                        yr = -1;
                    }
                    else {
                        yr = 0;
                    }

                    // Estrae la descrizione del file.
                    z = 0;
                    while (p[m] == ' ') {
                        m++;
                    }
                    while (p[m] && z < (tsys.no_filedate ? 56 : 48)) {
                        desc[z++] = p[m++];
                    }
                    desc[z] = '\0';

                    if (yr != -1) {
                        totsize += de[i].size;
                    }
                    totfile++;

                    if (sameline && !areadone) {
                        areadone = 1;
                        m_print(bbstxt[B_ONE_CR]);
                        if ((line = file_more_question(line, only_one)) == 0) {
                            break;
                        }
                    }

                    if (sys.no_filedate) {
                        if (yr == -1) {
                            if (config->show_missing) {
                                m_print(bbstxt[B_FILES_NODATE_MISSING], strupr(name), bbstxt[B_FILES_MISSING], desc);
                            }
                        }
                        else {
                            m_print(bbstxt[B_FILES_NODATE], strupr(name), de[i].size, desc);
                        }
                    }
                    else {
                        if (yr == -1) {
                            if (config->show_missing) {
                                m_print(bbstxt[B_FILES_FORMAT_MISSING], strupr(name), bbstxt[B_FILES_MISSING], desc);
                            }
                        }
                        else {
                            m_print(bbstxt[B_FILES_FORMAT], strupr(name), de[i].size, show_date(config->dateformat, e_input, de[i].date, 0), desc);
                        }
                    }

                    if (!(line = file_more_question(line, only_one)) || !CARRIER) {
                        break;
                    }
                }
            } while ((p = strtok(NULL, "\r\n")) != NULL);

        free(de);
        free(fbuf);

        if (only_one || !CARRIER) {
            break;
        }
    }

    close(fd);

    memcpy((char *)&sys.file_name, (char *)&vsys.file_name, SIZEOF_FILEAREA);

    if (sameline && !areadone) {
        m_print(bbstxt[B_ONE_CR]);
    }

    m_print(bbstxt[B_LOCATED_MATCH], totfile, totsize);

    if (line && CARRIER) {
        file_more_question(usr.len, only_one);
    }
}

void hurl()
{
#define MAX_HURL  4096
    FILE * fps, *fpd;
    struct _sys tempsys;
    int fds, fdd, m;
    char file[16], hurled[128], *b, filename[128];
    struct ftime ftimep;

    if (!get_command_word(file, 12)) {
        m_print(bbstxt[B_HURL_WHAT]);
        input(file, 12);
        if (!file[0]) {
            return;
        }
    }

    sprintf(filename, "%s%s", sys.filepath, file);
    fds = shopen(filename, O_RDWR | O_BINARY);
    if (fds == -1) {
        return;
    }

    if (!get_command_word(hurled, 79)) {
        m_print(bbstxt[B_HURL_AREA]);
        input(hurled, 80);
        if (!hurled[0]) {
            close(fds);
            return;
        }
    }

    if ((m = atoi(hurled)) != 0) {
        if (!read_system2(2, m, &tempsys)) {
            close(fds);
            return;
        }
    }
    else {
        strcpy(tempsys.filepath, hurled);
    }

    m_print(bbstxt[B_HURLING], sys.filepath, file, tempsys.filepath, file);

    sprintf(filename, "%s%s", tempsys.filepath, file);
    fdd = cshopen(filename, O_CREAT | O_TRUNC | O_RDWR | O_BINARY, S_IREAD | S_IWRITE);
    if (fdd == -1) {
        close(fds);
        return;
    }

    b = (char *)malloc(MAX_HURL);
    if (b == NULL) {
        return;
    }

    do {
        m = read(fds, b, MAX_HURL);
        write(fdd, b, m);
    } while (m == MAX_HURL);

    getftime(fds, &ftimep);
    setftime(fdd, &ftimep);

    close(fdd);
    close(fds);

    free(b);

    sprintf(filename, "%s%s", sys.filepath, file);
    unlink(filename);

    sprintf(filename, "%sFILES.BBS", sys.filepath);
    sprintf(hurled, "%sFILES.BAK", sys.filepath);
    unlink(hurled);
    rename(filename, hurled);

    fpd = fopen(filename, "wt");
    fps = fopen(hurled, "rt");

    strupr(file);
    strcat(file, " ");

    while (fgets(filename, 128, fps) != NULL) {
        if (!strncmp(filename, file, strlen(file))) {
            strcpy(hurled, filename);
            continue;
        }
        fputs(filename, fpd);
    }

    fclose(fps);
    fclose(fpd);

    sprintf(filename, "%sFILES.BBS", tempsys.filepath);
    fpd = fopen(filename, "at");
    fputs(hurled, fpd);
    fclose(fpd);
}

int file_more_question(line, only_one)
int line, only_one;
{
    int i;

    if (nopause) {
        return (1);
    }

    if (!(line++ < (usr.len - 1)) && usr.more) {
reask_question:
        line = 1;
        cmd_string[0] = '\0';
//      m_print(bbstxt[B_MORE]);

        m_print(bbstxt[B_FILE_MOREQUESTION]);

        if ((usr.priv < sys.download_priv || sys.download_priv == HIDDEN) || ((usr.flags & sys.download_flags) != sys.download_flags)) {
            i = yesno_question(DEF_YES | EQUAL | NO_LF | NO_MESSAGE);
        }
        else {
            i = yesno_question(DEF_YES | EQUAL | NO_LF | TAG_FILES | DOWN_FILES | NO_MESSAGE);
        }

        m_print("\r");
        space(strlen(bbstxt[B_FILE_MOREQUESTION]) + 2);
        m_print("\r");

        if (i == DEF_NO) {
            nopause = 0;
            return (0);
        }

        line = 1;

        if (i == EQUAL) {
            nopause = 1;
        }

        if (i == DOWN_FILES) {
            nopause = 0;
            if (usr.priv < sys.download_priv || sys.download_priv == HIDDEN) {
                return (line);
            }
            if ((usr.flags & sys.download_flags) != sys.download_flags) {
                return (line);
            }
            download_file(sys.filepath, only_one ? 0 : 1);
            if (user_status != BROWSING) {
                set_useron_record(BROWSING, 0, 0);
            }
            m_print(bbstxt[B_PAUSED_LIST]);
            goto reask_question;
        }

        if (i == TAG_FILES) {
            nopause = 0;
            if (usr.priv < sys.download_priv || sys.download_priv == HIDDEN) {
                return (line);
            }
            if ((usr.flags & sys.download_flags) != sys.download_flags) {
                return (line);
            }
            tag_files(only_one);
            if (user_status != BROWSING) {
                set_useron_record(BROWSING, 0, 0);
            }
            m_print(bbstxt[B_PAUSED_LIST]);
            goto reask_question;
        }
    }

    return (line);
}

void file_kill(int fbox, char * n_file)
{
    FILE * fps, *fpd;
    int i, godelete;
    char filename[80], file[14], *buffer, *p;
    struct ffblk blk;

    if (n_file == NULL) {
        if (!get_command_word(file, 12)) {
            m_print(bbstxt[B_KILL_FILE]);
            input(file, 12);
            if (!file[0]) {
                return;
            }
        }
    }
    else {
        strcpy(filename, n_file);
    }

    if ((buffer = (char *)malloc(2048)) == NULL) {
        return;
    }

    if (fbox) {
        if (n_file != NULL) {
            while ((p = strchr(filename, '\\')) != NULL) {
                strcpy(filename, ++p);
            }
            strcpy(file, filename);
        }
        sprintf(filename, "%s%08lx\\%s", config->boxpath, usr.id, file);
    }
    else if (n_file == NULL) {
        sprintf(filename, "%s%s", sys.filepath, file);
    }

    if (!findfirst(filename, &blk, 0))
        do {
            if (fbox) {
                sprintf(filename, "%s%08lx\\%s", config->boxpath, usr.id, blk.ff_name);
            }
            else {
                sprintf(filename, "%s%s", sys.filepath, blk.ff_name);
            }

            if (!fbox) {
                m_print(bbstxt[B_KILL_REMOVE], blk.ff_name);
                i = yesno_question(DEF_NO);
            }
            else {
                i = DEF_YES;
            }

            if (i == DEF_YES) {
                if (unlink(filename)) {
                    m_print(bbstxt[B_NOTFOUND], blk.ff_name);
                }

                else {
                    if (fbox) {
                        sprintf(filename, "%s%08lx\\FILES.BBS", config->boxpath, usr.id);
                        sprintf(buffer, "%s%08lx\\FILES.BAK", config->boxpath, usr.id);
                    }
                    else {
                        sprintf(filename, "%sFILES.BBS", sys.filepath);
                        sprintf(buffer, "%sFILES.BBS", sys.filepath);
                    }

                    rename(filename, buffer);
                    godelete = 0;

                    fpd = fopen(filename, "wt");
                    fps = fopen(buffer, "rt");

                    while (fgets(buffer, 2040, fps) != NULL) {
                        if (godelete) {
                            if (buffer[1] == '>') {
                                continue;
                            }
                            else {
                                godelete = 0;
                            }
                        }
                        if (!strncmp(buffer, blk.ff_name, strlen(blk.ff_name))) {
                            godelete = 1;
                            continue;
                        }
                        fputs(buffer, fpd);
                    }

                    fclose(fps);
                    fclose(fpd);
                }
            }
        } while (!findnext(&blk));

    else if (!fbox) {
        m_print(bbstxt[B_NOTFOUND], file);
    }

    free(buffer);
}

