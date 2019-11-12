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

#include <alloc.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "sched.h"
#include "lsetup.h"

#define MAX_DISK_BUFFER  16384

static char * buffer = NULL;
static int  inbuffer, firstaction;
static long indisk, maxread, readed, dupindisk;

FILE * mopen(char * filename, char * mode);
int mclose(FILE * fp);
int mputs(char * s, FILE * fp);
void mprintf(FILE * fp, char * format, ...);
long mseek(FILE * fp, long position, int offset);
int mread(char * s, int n, int e, FILE * fp);
long memlength(void);

FILE * mopen(char * filename, char * mode)
{
    if (buffer != NULL) {
        free(buffer);
    }

    buffer = (char *)malloc(MAX_DISK_BUFFER);
    if (buffer == NULL) {
        return (NULL);
    }

    inbuffer = 0;
    maxread = readed = dupindisk = indisk = 0L;
    firstaction = 1;

    return (fopen(filename, mode));
}

int mclose(FILE * fp)
{
    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }

    if (fp != NULL) {
        return (fclose(fp));
    }

    return (EOF);
}

int mputs(char * s, FILE * fp)
{
    if (firstaction) {
        maxread = 0L;
        firstaction = 0;
        dupindisk = indisk = 0L;
        fseek(fp, 0L, SEEK_SET);
    }

    if (inbuffer + strlen(s) >= MAX_DISK_BUFFER) {
        fwrite(buffer, inbuffer, 1, fp);
        indisk += inbuffer;
        dupindisk += inbuffer;
        inbuffer = 0;
    }

    strcpy(&buffer[inbuffer], s);
    inbuffer += strlen(s);
    maxread += strlen(s);

    return (s[strlen(s)]);
}

void mprintf(FILE * fp, char * format, ...)
{
    va_list var_args;
    char string[256];

    if (strlen(format) > 256) {
        return;
    }

    va_start(var_args, format);
    vsprintf(string, format, var_args);
    va_end(var_args);

    mputs(string, fp);
}

long mseek(FILE * fp, long position, int offset)
{
    if (dupindisk) {
        if (indisk) {
            fwrite(buffer, inbuffer, 1, fp);
            indisk += inbuffer;
            dupindisk += inbuffer;
            inbuffer = 0;
        }

        fseek(fp, position, offset);

        fread(buffer, MAX_DISK_BUFFER, 1, fp);
        indisk = dupindisk - (long)MAX_DISK_BUFFER;
    }

    inbuffer = 0;
    readed = 0L;
    firstaction = 1;

    return (0L);
}

int mread(char * s, int n, int e, FILE * fp)
{
    if (firstaction) {
        firstaction = 0;
    }

    if (readed + e > maxread) {
        e = (int)(maxread - readed);
    }

    if (inbuffer + e >= MAX_DISK_BUFFER) {
        n = MAX_DISK_BUFFER - inbuffer;
        memcpy(s, &buffer[inbuffer], n);
        if (indisk > (long)MAX_DISK_BUFFER) {
            fread(buffer, MAX_DISK_BUFFER, 1, fp);
            indisk -= (long)MAX_DISK_BUFFER;
        }
        else {
            fread(buffer, (int)indisk, 1, fp);
            indisk = 0L;
        }
        memcpy(&s[n], buffer, e - n);
        inbuffer = e - n;
    }
    else {
        memcpy(s, &buffer[inbuffer], e);
        inbuffer += e;
    }

    readed += e;

    return (e);
}

long memlength(void)
{
    return (maxread);
}

