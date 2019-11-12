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
#include <fcntl.h>
#include <string.h>

#include "lsetup.h"
#include "language.h"
#include "version.h"

/*
 * Assume average length of a string at 32 characters
 *
 */

#define MAX_STRINGS 1000
#define MAX_MEMORY (MAX_STRINGS * 32)

char ** pointers;
int pointer_size;

char * memory;
int memory_size;


/*
 * Read in a raw text file and write out a compiled BinkleyTerm
 * language file.
 *
 */

void process_language(type, mpath)
char * type, *mpath;
{
    char * malloc_target, fn[50];
    int error;

    /*
     * Print out the copyright notice.
     */

    printf("* Compile the %s Language File\n", type);

    /*
     * Allocate space for the raw character array and for the
     * pointer array
     *
     */

    malloc_target = malloc(MAX_MEMORY);
    if (malloc_target == NULL)
    {
        fprintf(stderr, "* Unable to allocate string memory\n");
        exit(250);
    }
    memory = malloc_target;
    memory_size = MAX_MEMORY;

    malloc_target = malloc((MAX_STRINGS + 1) * (sizeof(char *)));
    if (malloc_target == NULL)
    {
        fprintf(stderr, "* Unable to allocate pointer array\n");
        exit(250);
    }
    pointers = (char **)malloc_target;
    pointer_size = MAX_STRINGS;


    /*
     * Now read the stuff into our array.
     *
     */

    sprintf(fn, "%s%s.TXT", mpath, type);
    error = get_language(fn);
    if (error != 0) {
        exit(240);
    }


    /*
     * Write our stuff out now.
     *
     */

    sprintf(fn, "%s%s.LNG", mpath, type);
    error = put_language(fn);
    if (error != 0) {
        exit(230);
    }

    free(memory);
    free(pointers);
}

struct _configuration config;

void main(argc, argv)
int argc;
char * argv[];
{
    int fd, i, doit;

#ifdef __OS2__
    printf("\nLANGCOMP; LoraBBS-OS/2 Language compiler, Version %s\n", LANGCOMP_VERSION);
#else
    printf("\nLANGCOMP; LoraBBS-DOS Language compiler, Version %s\n", LANGCOMP_VERSION);
#endif
    printf("          Copyright (c) 1991-96 by Marco Maccaferri, All Rights Reserved\n\n");

    fd = open("CONFIG.DAT", O_RDONLY | O_BINARY);
    if (fd != -1) {
        read(fd, &config, sizeof(struct _configuration));
        close(fd);
    }

    doit = 1;

    if (argc > 1) {
        for (i = 1; i < argc; i++) {
            if (!strnicmp(argv[i], "-C", 2)) {
                fd = open((char *)&argv[i][2], O_RDONLY | O_BINARY);
                if (fd != -1) {
                    read(fd, &config, sizeof(struct _configuration));
                    close(fd);
                }
                else {
                    fprintf(stderr, "* Configuration file '%s' not found.\n", (char *)&argv[i][2]);
                }
            }
            else {
                process_language(argv[i], config.menu_path);
                doit = 0;
            }
        }
    }

    if (doit) {
        for (i = 0; i < MAX_LANG; i++) {
            if (config.language[i].basename[0]) {
                process_language(config.language[i].basename, config.menu_path);
            }
        }
    }
}

