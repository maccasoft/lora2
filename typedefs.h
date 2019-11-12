/***************************************************************************
 *                                                                         *
 *  MSGAPI Source Code, Version 2.00                                       *
 *  Copyright 1989-1991 by Scott J. Dudley.  All rights reserved.          *
 *                                                                         *
 *  Platform-specific compiler defn's                                      *
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

/* $Id: typedefs.h_v 1.0 1991/11/16 16:16:51 sjd Rel sjd $ */

#ifndef __TYPEDEFS_H_DEFINED
#define __TYPEDEFS_H_DEFINED

#if defined(__386__) || defined(__FLAT__)
typedef unsigned      bit;

typedef unsigned char byte;
typedef signed char   sbyte;

typedef unsigned short word;
typedef signed short   sword;

//  typedef unsigned int  dword;
typedef signed int    sdword;

typedef unsigned short ushort;
typedef   signed short sshort;

typedef unsigned long  ulong;
typedef   signed long  slong;
#else
typedef unsigned      bit;

typedef signed char   sbyte;

typedef signed int    sword;

//  typedef unsigned long dword;
typedef signed long   sdword;

typedef unsigned short ushort;
typedef   signed short sshort;

typedef unsigned long  ulong;
typedef   signed long  slong;
#endif

#endif

