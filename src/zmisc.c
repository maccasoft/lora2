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
#include <string.h>

#include <cxl\cxlwin.h>

#include "zmodem.h"
#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

static int Rxtype;                               /* Type of header received                 */

static char hex[] = "0123456789abcdef";

/* Send a byte as two hex digits */
#define Z_PUTHEX(i,c) {i=(c);SENDBYTE(hex[((i)&0xF0)>>4]);SENDBYTE(hex[(i)&0xF]);}

/*--------------------------------------------------------------------------*/
/* Private routines                                                         */
/*--------------------------------------------------------------------------*/
static int _Z_GetBinaryHeader(unsigned char *);
static int _Z_32GetBinaryHeader(unsigned char *);
static int _Z_GetHexHeader(unsigned char *);
static int _Z_GetHex(void);
static int _Z_TimedRead(void);
static long _Z_PullLongFromHeader(unsigned char *);

void show_loc(l, w)
unsigned long l;
unsigned int w;
{
    char j[100];

    sprintf(j, "Ofs=%ld Retries=%d        ", l, w);

    if (caller || emulator) {
        wgotoxy(1, 37);
        wputs(j);
    }
    else {
        wprints(10, 37, WHITE | _BLACK, j);
    }
}

void z_message(s)
char * s;
{
    if (caller || emulator) {
        if (s)
        {
            wgotoxy(1, 27);
            wputs(s);
        }
        wputs("              ");
    }
    else {
        wprints(10, 27, WHITE | _BLACK, s);
        wprints(10, 27 + strlen(s), WHITE | _BLACK, "              ");
    }
}

int Z_GetByte(tenths)
int tenths;
{
    long timeout, timerset();

    if (PEEKBYTE() >= 0) {
        return (MODEM_IN());
    }

    timeout = timerset(tenths * 10);

    do
    {
        if (PEEKBYTE() >= 0) {
            return MODEM_IN();
        }

        if (!(MODEM_STATUS() & carrier_mask)) {
            return RCDO;
        }

        if (local_kbd == 0x1B)
        {
            local_kbd = -1;
            return -1;
        }

        time_release();
    } while (!timeup(timeout));

    return TIMEOUT;
}

/*--------------------------------------------------------------------------*/
/* Z PUT STRING                                                             */
/* Send a string to the modem, processing for \336 (sleep 1 sec)            */
/* and \335 (break signal, ignored)                                         */
/*--------------------------------------------------------------------------*/
void Z_PutString(s)
register unsigned char * s;
{
    register int c;

    while (*s)
    {
        switch (c = *s++)
        {
            case '\336':
                big_pause(2);
            case '\335':
                /* Should send a break on this */
                break;
            default:
                SENDBYTE((unsigned char) c);
        }                                       /* switch */

    }                                          /* while */

    Z_UncorkTransmitter();                        /* Make sure all is well */
}                                                /* Z_PutString */

/*--------------------------------------------------------------------------*/
/* Z SEND HEX HEADER                                                        */
/* Send ZMODEM HEX header hdr of type type                                  */
/*--------------------------------------------------------------------------*/
void Z_SendHexHeader(type, hdr)
unsigned int type;
register unsigned char * hdr;
{
    register int n;
    register int i;
    register word crc;

#ifdef DEBUG
    show_debug_name("Z_SendHexHeader");
#endif

    Z_UncorkTransmitter();                        /* Get our transmitter going */

    SENDBYTE(ZPAD);
    SENDBYTE(ZPAD);
    SENDBYTE(ZDLE);
    SENDBYTE(ZHEX);
    Z_PUTHEX(i, type);

    Crc32t = 0;
    crc = Z_UpdateCRC(type, 0);
    for (n = 4; --n >= 0;)
    {
        Z_PUTHEX(i, (*hdr));
        crc = Z_UpdateCRC(((unsigned short)(*hdr++)), crc);
    }
    Z_PUTHEX(i, (crc >> 8));
    Z_PUTHEX(i, crc);

    /* Make it printable on remote machine */
    SENDBYTE('\r');
    SENDBYTE('\n');

    /* Uncork the remote in case a fake XOFF has stopped data flow */
    if (type != ZFIN && type != ZACK) {
        SENDBYTE(021);
    }

    if (!CARRIER) {
        CLEAR_OUTBOUND();
    }

}                                                /* Z_SendHexHeader */

/*--------------------------------------------------------------------------*/
/* Z UNCORK TRANSMITTER                                                     */
/* Wait a reasonable amount of time for transmitter buffer to clear.        */
/*   When it does, or when time runs out, turn XON/XOFF off then on.        */
/*   This should release a transmitter stuck by line errors.                */
/*--------------------------------------------------------------------------*/

void Z_UncorkTransmitter()
{
    long t, timerset();

    if (!OUT_EMPTY())
    {
        t = timerset(5 * Rxtimeout);               /* Wait for silence */
        while (!timeup(t) && !OUT_EMPTY() && CARRIER) {
            time_release();    /* Give up slice while */
        }
        /* waiting  */
    }
    XON_DISABLE();                                /* Uncork the transmitter */
    XON_ENABLE();
}


/*--------------------------------------------------------------------------*/
/* Z GET HEADER                                                             */
/* Read a ZMODEM header to hdr, either binary or hex.                       */
/*   On success, set Zmodem to 1 and return type of header.                 */
/*   Otherwise return negative on error                                     */
/*--------------------------------------------------------------------------*/
int Z_GetHeader(hdr)
byte * hdr;
{

    register int c;
    register int n;
    int cancount;

#ifdef DEBUG
    show_debug_name("Z_GetHeader");
#endif

    n = rate;                                     /* Max characters before
                                                  * start of frame */
    cancount = 5;

Again:

    if (local_kbd == 0x1B)
    {
        local_kbd = -1;
        send_can();
        status_line(msgtxt[M_KBD_MSG]);
        return ZCAN;
    }

    Rxframeind = Rxtype = 0;

    switch (c = _Z_TimedRead())
    {
        case ZPAD:
        case ZPAD | 0200:
            /*-----------------------------------------------*/
            /* This is what we want.                         */
            /*-----------------------------------------------*/
            break;

        case RCDO:
        case TIMEOUT:
            goto Done;

        case CAN:

GotCan:

            if (--cancount <= 0)
            {
                c = ZCAN;
                goto Done;
            }
            switch (c = Z_GetByte(1))
            {
                case TIMEOUT:
                    goto Again;

                case ZCRCW:
                    c = ERROR;
                /* fallthrough... */

                case RCDO:
                    goto Done;

                case CAN:
                    if (--cancount <= 0)
                    {
                        c = ZCAN;
                        goto Done;
                    }
                    goto Again;
            }
        /* fallthrough... */

        default:

Agn2:

            if (--n <= 0)
            {
                status_line(msgtxt[M_FUBAR_MSG]);
                return ERROR;
            }

            if (c != CAN) {
                cancount = 5;
            }
            goto Again;

    }                                          /* switch */

    cancount = 5;

Splat:

    switch (c = _Z_TimedRead())
    {
        case ZDLE:
            /*-----------------------------------------------*/
            /* This is what we want.                         */
            /*-----------------------------------------------*/
            break;

        case ZPAD:
            goto Splat;

        case RCDO:
        case TIMEOUT:
            goto Done;

        default:
            goto Agn2;

    }                                          /* switch */


    switch (c = _Z_TimedRead())
    {

        case ZBIN:
            Rxframeind = ZBIN;
            Crc32 = 0;
            c = _Z_GetBinaryHeader(hdr);
            break;

        case ZBIN32:
            Crc32 = Rxframeind = ZBIN32;
            c = _Z_32GetBinaryHeader(hdr);
            break;

        case ZHEX:
            Rxframeind = ZHEX;
            Crc32 = 0;
            c = _Z_GetHexHeader(hdr);
            break;

        case CAN:
            goto GotCan;

        case RCDO:
        case TIMEOUT:
            goto Done;

        default:
            goto Agn2;

    }                                          /* switch */

    Rxpos = _Z_PullLongFromHeader(hdr);

Done:

    return c;
}                                                /* Z_GetHeader */

/*--------------------------------------------------------------------------*/
/* Z GET BINARY HEADER                                                      */
/* Receive a binary style header (type and position)                        */
/*--------------------------------------------------------------------------*/
static int _Z_GetBinaryHeader(hdr)
register unsigned char * hdr;
{
    register int c;
    register unsigned int crc;
    register int n;

#ifdef DEBUG
    show_debug_name("Z_GetBinaryHeader");
#endif

    if ((c = Z_GetZDL()) & ~0xFF) {
        return c;
    }
    Rxtype = c;
    crc = Z_UpdateCRC(c, 0);

    for (n = 4; --n >= 0;)
    {
        if ((c = Z_GetZDL()) & ~0xFF) {
            return c;
        }
        crc = Z_UpdateCRC(c, crc);
        *hdr++ = (unsigned char)(c & 0xff);
    }
    if ((c = Z_GetZDL()) & ~0xFF) {
        return c;
    }

    crc = Z_UpdateCRC(c, crc);
    if ((c = Z_GetZDL()) & ~0xFF) {
        return c;
    }

    crc = Z_UpdateCRC(c, crc);
    if (crc & 0xFFFF)
    {
        z_message(msgtxt[M_CRC_MSG]);
        return ERROR;
    }

    return Rxtype;
}                                                /* _Z_GetBinaryHeader */


/*--------------------------------------------------------------------------*/
/* Z GET BINARY HEADER with 32 bit CRC                                      */
/* Receive a binary style header (type and position)                        */
/*--------------------------------------------------------------------------*/
static int _Z_32GetBinaryHeader(hdr)
register unsigned char * hdr;
{
    register int c;
    register unsigned long crc;
    register int n;

#ifdef DEBUG
    show_debug_name("Z_32GetBinaryHeader");
#endif

    if ((c = Z_GetZDL()) & ~0xFF) {
        return c;
    }
    Rxtype = c;
    crc = 0xFFFFFFFFL;
    crc = Z_32UpdateCRC(c, crc);

    for (n = 4; --n >= 0;)
    {
        if ((c = Z_GetZDL()) & ~0xFF) {
            return c;
        }
        crc = Z_32UpdateCRC(c, crc);
        *hdr++ = (unsigned char)(c & 0xff);
    }

    for (n = 4; --n >= 0;)
    {
        if ((c = Z_GetZDL()) & ~0xFF) {
            return c;
        }

        crc = Z_32UpdateCRC(c, crc);
    }

    if (crc != 0xDEBB20E3L)
    {
        z_message(msgtxt[M_CRC_MSG]);
        return ERROR;
    }

    return Rxtype;
}                                                /* _Z_32GetBinaryHeader */

/*--------------------------------------------------------------------------*/
/* Z GET HEX HEADER                                                         */
/* Receive a hex style header (type and position)                           */
/*--------------------------------------------------------------------------*/
static int _Z_GetHexHeader(hdr)
register unsigned char * hdr;
{
    register int c;
    register unsigned int crc;
    register int n;

#ifdef DEBUG
    show_debug_name("Z_GetHexHeader");
#endif

    if ((c = _Z_GetHex()) < 0) {
        return c;
    }
    Rxtype = c;
    crc = Z_UpdateCRC(c, 0);

    for (n = 4; --n >= 0;)
    {
        if ((c = _Z_GetHex()) < 0) {
            return c;
        }
        crc = Z_UpdateCRC(c, crc);
        *hdr++ = (unsigned char) c;
    }

    if ((c = _Z_GetHex()) < 0) {
        return c;
    }
    crc = Z_UpdateCRC(c, crc);
    if ((c = _Z_GetHex()) < 0) {
        return c;
    }
    crc = Z_UpdateCRC(c, crc);
    if (crc & 0xFFFF)
    {
        z_message(msgtxt[M_CRC_MSG]);
        return ERROR;
    }
    if (Z_GetByte(1) == '\r') {
        Z_GetByte(1);    /* Throw away possible cr/lf */
    }

    return Rxtype;
}

/*--------------------------------------------------------------------------*/
/* Z GET HEX                                                                */
/* Decode two lower case hex digits into an 8 bit byte value                */
/*--------------------------------------------------------------------------*/
static int _Z_GetHex()
{
    register int c, n;

#ifdef DEBUG
    show_debug_name("Z_GetHex");
#endif

    if ((n = _Z_TimedRead()) < 0) {
        return n;
    }
    n -= '0';
    if (n > 9) {
        n -= ('a' - ':');
    }
    if (n & ~0xF) {
        return ERROR;
    }

    if ((c = _Z_TimedRead()) < 0) {
        return c;
    }
    c -= '0';
    if (c > 9) {
        c -= ('a' - ':');
    }
    if (c & ~0xF) {
        return ERROR;
    }

    return ((n << 4) | c);
}

/*--------------------------------------------------------------------------*/
/* Z GET ZDL                                                                */
/* Read a byte, checking for ZMODEM escape encoding                         */
/* including CAN*5 which represents a quick abort                           */
/*--------------------------------------------------------------------------*/
int Z_GetZDL()
{
    register int c;

    if ((c = Z_GetByte(Rxtimeout)) != ZDLE) {
        return c;
    }

    switch (c = Z_GetByte(Rxtimeout))
    {
        case CAN:
            return ((c = Z_GetByte(Rxtimeout)) < 0) ? c :
                   ((c == CAN) && ((c = Z_GetByte(Rxtimeout)) < 0)) ? c :
                   ((c == CAN) && ((c = Z_GetByte(Rxtimeout)) < 0)) ? c : (GOTCAN);

        case ZCRCE:
        case ZCRCG:
        case ZCRCQ:
        case ZCRCW:
            return (c | GOTOR);

        case ZRUB0:
            return 0x7F;

        case ZRUB1:
            return 0xFF;

        default:
            return (c < 0) ? c :
                   ((c & 0x60) == 0x40) ? (c ^ 0x40) : ERROR;

    }                                          /* switch */
}                                                /* Z_GetZDL */

/*--------------------------------------------------------------------------*/
/* Z TIMED READ                                                             */
/* Read a character from the modem line with timeout.                       */
/*  Eat parity, XON and XOFF characters.                                    */
/*--------------------------------------------------------------------------*/
static int _Z_TimedRead()
{
    register int c;

#ifdef DEBUG
    show_debug_name("Z_TimedRead");
#endif

    for (;;)
    {
        if ((c = Z_GetByte(Rxtimeout)) < 0) {
            return c;
        }

        switch (c &= 0x7F)
        {
            case XON:
            case XOFF:
                continue;

            default:
                if (!(c & 0x60)) {
                    continue;
                }

            case '\r':
            case '\n':
            case ZDLE:
                return c;
        }                                       /* switch */

    }                                          /* for */
}                                                /* _Z_TimedRead */

/*--------------------------------------------------------------------------*/
/* Z LONG TO HEADER                                                         */
/* Store long integer pos in Txhdr                                          */
/*--------------------------------------------------------------------------*/
void Z_PutLongIntoHeader(pos)
long pos;
{
#ifndef GENERIC
    *((long *) Txhdr) = pos;
#else
    Txhdr[ZP0] = pos;
    Txhdr[ZP1] = pos >> 8;
    Txhdr[ZP2] = pos >> 16;
    Txhdr[ZP3] = pos >> 24;
#endif
}                                                /* Z_PutLongIntoHeader */

/*--------------------------------------------------------------------------*/
/* Z PULL LONG FROM HEADER                                                  */
/* Recover a long integer from a header                                     */
/*--------------------------------------------------------------------------*/
static long _Z_PullLongFromHeader(hdr)
unsigned char * hdr;
{
#ifndef GENERIC
    if (hdr);
    return (*((long *) Rxhdr)); /*PLF Fri  05-05-1989  06:42:41 */
#else
    long l;

    l = hdr[ZP3];
    l = (l << 8) | hdr[ZP2];
    l = (l << 8) | hdr[ZP1];
    l = (l << 8) | hdr[ZP0];
    return l;
#endif
}                                                /* _Z_PullLongFromHeader */

