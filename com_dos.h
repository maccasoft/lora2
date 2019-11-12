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

unsigned ComInit(unsigned char);
int      READBYTE();
#define M_INSTALL(b)       {Cominit(com_port);Com_(0x0f,0);Com_(0x0f,handshake_mask);new_baud(0,b);}


/*-----------------------------------------------*/
/* Service 0: SET BAUD(etc)                      */
/*-----------------------------------------------*/
/*#define MDM_ENABLE(b)      (Com_(0x00,b|NO_PARITY|STOP_1|BITS_8))*/

/*-----------------------------------------------*/
/* Service 1: SEND CHAR (wait)                   */
/*-----------------------------------------------*/
/*#define SENDBYTE(x)	   (Com_(0x01,x))*/
/*-----------------------------------------------*/
/* Service 2: GET CHAR (wait)                    */
/*-----------------------------------------------*/
/*#define MODEM_IN()         (Com_(0x02)&0x00ff)*/

/*-----------------------------------------------*/
/* Service 3: GET STATUS                         */
/*-----------------------------------------------*/
/*#define MODEM_STATUS()     (Com_(0x03))*/
#define CARRIER            com_online()
#define CHAR_AVAIL()       (MODEM_STATUS()&DATA_READY)
#define OUT_EMPTY()        (Com_(0x03)&TX_SHIFT_EMPTY)
#define OUT_FULL()         (!(Com_(0x03)&TX_HOLD_EMPTY))

/*-----------------------------------------------*/
/* Service 4: INIT/INSTALL                       */
/*-----------------------------------------------*/

/*-----------------------------------------------*/
/* Service 5: UNINSTALL                          */
/*-----------------------------------------------*/
/*#define MDM_DISABLE()      (Com_(0x05,BAUD_2400|NO_PARITY|STOP_1|BITS_8))*/

/*-----------------------------------------------*/
/* Service 6: SET DTR                            */
/*-----------------------------------------------*/
#ifdef __OS2__
void SET_DTR_OFF(void);
void SET_DTR_ON(void);
#else
#define SET_DTR_OFF()      ((void) Com_(0x06,0))
#define SET_DTR_ON()       ((void) Com_(0x06,1))
#endif

/*-----------------------------------------------*/
/* Service 7: GET TIMER TICK PARMS               */
/*-----------------------------------------------*/

/*-----------------------------------------------*/
/* Service 8: FLUSH OUTBOUND RING-BUFFER         */
/*-----------------------------------------------*/

/*-----------------------------------------------*/
/* Service 9: NUKE OUTBOUND RING-BUFFER          */
/*-----------------------------------------------*/
/*#define CLEAR_OUTBOUND()   (Com_(0x09))*/

/*-----------------------------------------------*/
/* Service a: NUKE INBOUND RING-BUFFER           */
/*-----------------------------------------------*/
/*#define CLEAR_INBOUND()    (Com_(0x0a))*/

/*-----------------------------------------------*/
/* Service b: SEND CHAR (no wait)                */
/*-----------------------------------------------*/
#define Com_Tx_NW(c)       (Com_(0x0b,c))

/*-----------------------------------------------*/
/* Service c: GET CHAR (no wait)                 */
/*-----------------------------------------------*/
/*#define PEEKBYTE()         (Com_(0x0c))*/

/*-----------------------------------------------*/
/* Service d: GET KEYBOARD STATUS                */
/*-----------------------------------------------*/
#define KEYPRESS()         (Com_(0x0d)!=(0xffff))
#define FOSSIL_PEEKKB()    (Com_(0x0d))

/*-----------------------------------------------*/
/* Service e: GET KEYBOARD CHARACTER (wait)      */
/*-----------------------------------------------*/
#define READKB()           (Com_(0x0e)&0xff)
#define READSCAN()         (Com_(0x0e))
#define FOSSIL_CHAR()      (Com_(0x0e))


/*-----------------------------------------------*/
/* Service f: SET/GET FLOW CONTROL STATUS        */
/*-----------------------------------------------*/
#ifdef __OS2__
void XON_ENABLE(void);
void XON_DISABLE(void);
#define IN_XON_ENABLE()    XON_ENABLE()
#define IN_XON_DISABLE()   XON_DISABLE()
#else
#define XON_ENABLE()       ((void) Com_(0x0f,handshake_mask))
#define XON_DISABLE()      ((void) Com_(0x0f,(handshake_mask&(~USE_XON))))
#define IN_XON_ENABLE()    ((void) Com_(0x0f,handshake_mask|OTHER_XON))
#define IN_XON_DISABLE()   ((void) Com_(0x0f,(handshake_mask&(~OTHER_XON))))
#endif

/*-----------------------------------------------*/
/* Service 10: SET/GET CTL-BREAK CONTROLS        */
/*             Note that the "break" here refers */
/*             to ^C and ^K rather than the      */
/*             tradition modem BREAK.            */
/*-----------------------------------------------*/
#define _BRK_ENABLE()      (Com_(0x10,BRK))
#define _BRK_DISABLE()     (Com_(0x10,0))
#define RECVD_BREAK()      (Com_(0x10,BRK)&BRK)

/*-----------------------------------------------*/
/* Service 11: SET LOCAL VIDEO CURSOR POSITION   */
/*-----------------------------------------------*/

/*-----------------------------------------------*/
/* Service 12: GET LOCAL VIDEO CURSOR POSITION   */
/*-----------------------------------------------*/

/*-----------------------------------------------*/
/* Service 13: WRITE LOCAL ANSI CHARACTER        */
/*-----------------------------------------------*/
#define WRITE_ANSI(c)	   ((void) Com_(0x13,c))
/*-----------------------------------------------*/
/* Service 14: WATCHDOG on/off                   */
/*-----------------------------------------------*/
#define FOSSIL_WATCHDOG(x) (Com_(0x14,x))
/*-----------------------------------------------*/
/* Service 15: BIOS write to screen              */
/*-----------------------------------------------*/
#define WRITE_BIOS(c) (Com_(0x15,c))


/*--------------------------------------------------------------------------*/
/*                                                                          */
/* A no-stall ReadByte routine might look like this:                        */
/*                                                                          */
/*    int READBYTE()                                                        */
/*       {                                                                  */
/*       return( CHAR_AVAIL() ? MODEM_IN() : (-1) );                        */
/*       }                                                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* END OF FILE: com.h */

