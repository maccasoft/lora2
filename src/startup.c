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
#include <conio.h>
#include <string.h>
#include <process.h>
#include <alloc.h>

void main(argc, argv)
int argc;
char * argv[];
{
    int i;
    char * p, argv0[80], *args[20], memf[10];

#ifdef __OS2__
    sprintf(memf, "%ld", 0L);
#else
    sprintf(memf, "%ld", farcoreleft());
#endif

    args[0] = argv0;
    for (i = 1; i < argc; i++) {
        args[i] = argv[i];
    }
    args[i++] = memf;
    args[i] = NULL;

    strcpy(argv0, argv[0]);
    p = strstr(strupr(argv0), ".EXE");
    if (p == NULL) {
        strcat(argv0, ".OVL");
    }
    else {
        strcpy(p, ".OVL");
    }

    do {
        i = spawnvp(P_WAIT, argv0, args);
    } while (i == 255);

    exit(i);
}

