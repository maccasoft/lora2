#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "defines.h"
#include "lora.h"

struct _tym {
        char name[41];
        char action[16];
};

void main (argc, argv)
int argc;
char *argv[];
{
        int fd, i;
        char filename[80];
        struct _lorainfo lorainfo;
        struct _usr usr;
        struct _tym tym;

        if (argc < 3)
        {
                printf("\nUsage: Tym2Lora <Task_Number> <Path_to_Lorainfo>\n");
                exit (1);
        }

        sprintf(filename, "%s\\LORAINFO.T%02X", argv[2], atoi(argv[1]));
        fd = open (filename, O_RDWR|O_BINARY);
        if (fd == -1)
                return;

        read (fd, (char *)&usr, sizeof(struct _usr));
        read (fd, (char *)&lorainfo, sizeof(struct _lorainfo));
        lseek (fd, 0L, SEEK_SET);

        sprintf(filename, "HAMBANK.TYM");
        i = open (filename, O_RDWR|O_BINARY);
        if (i == -1)
        {
                close (fd);
                return;
        }

        read (i, (char *)&tym, sizeof(struct _tym));
        close (i);

        for (i=40;i >= 0; i--)
        {
                if (tym.name[i] == ' ')
                        tym.name[i] = '\0';
                else
                        break;
        }

        if (!stricmp(usr.name, tym.name) ||
            (!stricmp("Sysop", tym.name) && !stricmp (tym.name, lorainfo.sysop))
           )
        {
                lorainfo.time = atoi(&tym.action[7]);
                write (fd, (char *)&usr, sizeof(struct _usr));
                write (fd, (char *)&lorainfo, sizeof(struct _lorainfo));
        }
        close (fd);

        unlink (filename);
}

