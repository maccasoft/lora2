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

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

static int vwh = -1;

void setup_video_interrupt(title)
char * title;
{
    vwh = wopen(13, 0, 24, 79, 5, LGREY | _BLACK, LCYAN | _BLACK);
    wactiv(vwh);
    wtitle(title, TLEFT, LCYAN | _BLACK);
    printc(12, 0, LGREY | _BLACK, '\300');
    printc(12, 52, LGREY | _BLACK, '\301');
    printc(12, 79, LGREY | _BLACK, '\331');

    gotoxy_(13, 0);
    showcur();
}

#ifndef __OS2__
static int restnodirect;

static void interrupt newint10();
static void interrupt newint10_2();

void interrupt(*oldfunc)(void);

static struct _wrec_t * wr;

void blank_video_interrupt()
{
#ifndef __NOFOSSIL__
    if (_vinfo.usebios) {
        restnodirect = 1;
        setvparam(VP_DMA);
    }
    else {
        restnodirect = 0;
    }
    oldfunc  = getvect(16);
    setvect(16, newint10_2);
#endif
}

void restore_video_interrupt()
{
#ifndef __NOFOSSIL__
    hidecur();
    setvect(16, oldfunc);
    if (vwh != -1) {
        wclose();
        vwh = -1;
    }

    if (restnodirect) {
        setvparam(VP_BIOS);
    }
#endif
}

#ifndef __NOFOSSIL__
#pragma -wpar-
void interrupt newint10_2(unsigned bp, unsigned di, unsigned si,
                          unsigned ds, unsigned es, unsigned dx,
                          unsigned cx, unsigned bx, unsigned ax)
{
    static unsigned char f;

    f = ax >> 8;

    if (f == 0x09 || f == 0x0E);
    else if (f == 0x02);
    else if (f == 0x03) {
        _AX = ax;
        _BX = bx;
        oldfunc();
        ax = _AX;
        cx = _CX;
    }
    else {
        _AX = ax;
        _BX = bx;
        _CX = cx;
        _DX = dx;
        oldfunc();
        ax = _AX;
        bx = _BX;
        cx = _CX;
        dx = _DX;
    }
}

void interrupt newint10(unsigned bp, unsigned di, unsigned si,
                        unsigned ds, unsigned es, unsigned dx,
                        unsigned cx, unsigned bx, unsigned ax)
{
    static unsigned char f, c;

    f = ax >> 8;
    c = ax & 0xFF;

    if (f == 0x09 || f == 0x0E) {
        wputc(c);
//      r = wr->row * 256 + wr->column;
//      _AX = 0x0200;
//      _BX = 0;
//      _DX = r;
//      oldfunc ();
    }
    else if (f == 0x02) {
        f = dx >> 8;
        c = dx & 0xFF;
        if (c == 0) {
            wputc(0x0D);
        }
        else if (c < wr->column) {
            wputc(0x08);
        }
        if (f > wr->row) {
            wputc(0x0A);
        }
        _AX = ax;
        _BX = bx;
        _CX = cx;
        _DX = dx;
        oldfunc();
        ax = _AX;
        bx = _BX;
        cx = _CX;
        dx = _DX;
    }
    else if (f == 0x03) {
        _AX = ax;
        _BX = bx;
        oldfunc();
        ax = _AX;
        cx = _CX;
        dx = wr->row * 256 + wr->column;
    }
    else {
        _AX = ax;
        _BX = bx;
        _CX = cx;
        _DX = dx;
        oldfunc();
        ax = _AX;
        bx = _BX;
        cx = _CX;
        dx = _DX;
    }
}
#pragma +wpar

#endif  // __NOFOSSIL__
#endif  // __OS2__

