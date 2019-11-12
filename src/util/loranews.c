#include <stdio.h>
#include <stdlib.h>
#include <dir.h>
#include <io.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

#include "lora.h"

int new_file_list(void);

#define MSG_AREAS 201

byte exclude_list [MSG_AREAS];

void main(argc, argv)
int argc;
char ** argv;
{
    int i, m;

    for (m = 1; m <= MSG_AREAS; m++) {
        exclude_list [m] = 0;
    }

    for (i = 1; i < argc; i++)
    {
        if (toupper(argv[i][0]) == 'X')
        {
            if (!stricmp(&argv[i][1], "ALL"))
            {
                for (m = 1; m <= MSG_AREAS; m++) {
                    exclude_list [m] = 1;
                }
            }
            else if (isdigit(argv[i][1])) {
                exclude_list [atoi(&argv[i][1])] = 1;
            }
        }
        else if (toupper(argv[i][0]) == 'I')
        {
            if (!stricmp(&argv[i][1], "ALL"))
            {
                for (m = 1; m <= MSG_AREAS; m++) {
                    exclude_list [m] = 0;
                }
            }
            else if (isdigit(argv[i][1])) {
                exclude_list [atoi(&argv[i][1])] = 0;
            }
        }
    }

    if (new_file_list()) {
        exit(2);
    }

    exit(0);
}

int new_file_list()
{
    FILE * fp, *fpn;
    int m, z, fd, first, news;
    char name[13], desc[46];
    char stringa[120], filename[80];
    struct ffblk blk;
    struct ffblk lblk;
    struct _sys tsys;

    news = 0;

    if (findfirst("LORANEWS.DAT", &lblk, 0))
    {
        m = creat("LORANEWS.DAT", S_IREAD | S_IWRITE);
        close(m);
        findfirst("LORANEWS.DAT", &lblk, 0);
    }

    fd = open("SYSFILE.DAT", O_RDONLY | O_BINARY);
    if (fd == -1)
    {
        printf("WARNING: No System File.\n");
        exit(1);
    }

    fpn = fopen("LORANEWS.DAT", "wt");

    while (read(fd, (char *)&tsys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA)
    {
        if (exclude_list [tsys.file_num]) {
            continue;
        }

        printf("%-3d ... %s\n", tsys.file_num, tsys.file_name);
        first = 1;

        sprintf(filename, "%sFILES.BBS", tsys.filepath);
        fp = fopen(filename, "rt");
        if (fp == NULL) {
            continue;
        }

        while (fgets(stringa, 110, fp) != NULL)
        {
            stringa[strlen(stringa) - 1] = 0;

            if (!isalnum(stringa[0])) {
                continue;
            }

            m = 0;
            while (stringa[m] != ' ' && m < 12) {
                name[m] = stringa[m++];
            }

            name[m] = '\0';

            sprintf(filename, "%s%s", tsys.filepath, name);
            if (findfirst(filename, &blk, 0)) {
                continue;
            }

            if ((blk.ff_fdate > lblk.ff_fdate) ||
                    (blk.ff_fdate == lblk.ff_fdate && blk.ff_ftime > lblk.ff_ftime)
               )
            {
                z = 0;
                while (stringa[m] == ' ') {
                    m++;
                }
                while (stringa[m] && z < 45) {
                    desc[z++] = stringa[m++];
                }
                desc[z] = '\0';

                if (first)
                {
                    fprintf(fpn, "Area #%d - %s\n", tsys.file_num, tsys.file_name);
                    first = 0;
                }

                printf("  %-12s %4dKb - %s\n", strupr(name),
                       (int)(blk.ff_fsize / 1024L),
                       desc);
                fprintf(fpn, "  %-12s %4dKb - %s\n", strupr(name),
                        (int)(blk.ff_fsize / 1024L),
                        desc);
                news++;
            }
        }

        if (!first) {
            fprintf(fpn, "\n");
        }

        fclose(fp);
    }

    close(fd);

    return (news);
}

