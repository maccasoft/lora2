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

#define PER_WEEK    60480000L
#define PER_DAY      8640000L
#define PER_HOUR      360000L
#define PER_MINUTE      6000L
#define PER_SECOND       100L

void dostime(int * hour, int * min, int * sec, int * hdths)
{
#ifdef __OS2__
    struct time timep;

    gettime(&timep);

    *hour = timep.ti_hour;
    *min = timep.ti_min;
    *sec = timep.ti_sec;
    *hdths = timep.ti_hund;
#else
    union REGS r;

    r.h.ah = 0x2c;

    (void) intdos(&r, &r);

    *hour  = r.h.ch;
    *min   = r.h.cl;
    *sec   = r.h.dh;
    *hdths = r.h.dl;
#endif
}

void dosdate(int * month, int * mday, int * year, int * wday)
{
#ifdef __OS2__
    struct dosdate_t datep;

    _dos_getdate(&datep);

    *month = datep.month;
    *mday = datep.day;
    *year = datep.year;
    *wday = datep.dayofweek;
#else
    union REGS r;

    r.h.ah = 0x2a;

    (void) intdos(&r, &r);

    *month  = r.x.bx;
    *mday   = r.x.dx;
    *year   = r.x.cx;
    *wday   = r.x.ax;
#endif
}

long timerset(int t)
{
    long l;
    int hours, mins, secs, ths;

    /* Get the DOS time and day of week */
    dostime(&hours, &mins, &secs, &ths);

    /* Figure out the hundredths of a second so far this week */
    l = (mins % 60) * PER_MINUTE +
        (secs % 60) * PER_SECOND +
        ths;

    /* Add in the timer value */
    l += t;

    /* Return the alarm off time */
    return (l);
}

int timeup(long t)
{
    long l;

    /* Get current time in hundredths */
    l = timerset(0);

    /* If current is less then set by more than max int, then adjust */
    if (l < (t - 65536L)) {
        l += PER_HOUR;
    }

    /* Return whether the current is greater than the timer variable */
    return ((l - t) >= 0L);
}
