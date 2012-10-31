/***************************************************************************
 *                                                                         *
 *  MSGAPI Source Code, Version 2.00                                       *
 *  Copyright 1989-1991 by Scott J. Dudley.  All rights reserved.          *
 *                                                                         *
 *  Bitmapped date union                                                   *
 *                                                                         *
 *  For complete details of the licensing restrictions, please refer to    *
 *  the licence agreement, which is published in its entirety in           *
 *  README.1ST.                                                            *
 *                                                                         *
 *  USE OF THIS FILE IS SUBJECT TO THE RESTRICTIONS CONTAINED IN THE       *
 *  MSGAPI LICENSING AGREEMENT.  IF YOU DO NOT FIND THE TEXT OF THIS       *
 *  AGREEMENT IN ANY OF THE AFOREMENTIONED FILES, OR IF YOU DO NOT HAVE    *
 *  THESE FILES, YOU SHOULD IMMEDIATELY CONTACT THE AUTHOR AT ONE OF THE   *
 *  ADDRESSES LISTED BELOW.  IN NO EVENT SHOULD YOU PROCEED TO USE THIS    *
 *  FILE WITHOUT HAVING ACCEPTED THE TERMS OF THE MSGAPI LICENSING         *
 *  AGREEMENT, OR SUCH OTHER AGREEMENT AS YOU ARE ABLE TO REACH WITH THE   *
 *  AUTHOR.                                                                *
 *                                                                         *
 *  You can contact the author at one of the address listed below:         *
 *                                                                         *
 *  Scott Dudley           FidoNet  1:249/106                              *
 *  777 Downing St.        Internet f106.n249.z1.fidonet.org               *
 *  Kingston, Ont.         BBS      (613) 389-8315   HST/14.4k, 24hrs      *
 *  Canada - K7M 5N3                                                       *
 *                                                                         *
 ***************************************************************************/

/* $Id: stamp.h_v 1.0 1991/11/16 16:16:51 sjd Rel sjd $ */

#ifndef __STAMP_H_DEFINED
#define __STAMP_H_DEFINED

#include "typedefs.h"

struct _stamp2   /* DOS-style datestamp */
{
  struct
  {
    unsigned int da : 5;
    unsigned int mo : 4;
    unsigned int yr : 7;
  } date;

  struct
  {
    unsigned int ss : 5;
    unsigned int mm : 6;
    unsigned int hh : 5;
  } time;
};


struct _dos_st
{
  word date;
  word time;
};

/* Union so we can access stamp as "int" or by individual components */

union stamp_combo   
{
  dword ldate;
  struct _stamp2 msg_st;
  struct _dos_st dos_st;
};

typedef union stamp_combo SCOMBO;

#endif /* __STAMP_H_DEFINED */

