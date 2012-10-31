/***************************************************************************
 *                                                                         *
 *  MSGAPI Source Code, Version 2.00                                       *
 *  Copyright 1989-1991 by Scott J. Dudley.  All rights reserved.          *
 *                                                                         *
 *  Compiler-determination and memory-model-determination routines         *
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

/* $Id: compiler.h_v 1.0 1991/11/16 16:16:51 sjd Rel sjd $ */

/* Non-DOS systems...  Just do a "#define __FARCODE__",                     *
 * "#define __FARDATA__" and "#define __LARGE__" in place of this file.     */

#ifndef __COMPILER_H_DEFINED
#define __COMPILER_H_DEFINED

#ifndef __WATCOMC__ /* WATCOM has both M_I86xxx and __modeltype__ macros */

  #if (defined(M_I86SM) || defined(M_I86MM)) || (defined(M_I86CM) || defined(M_I86LM) || defined(M_I86HM))
    #define __MSC__
  #endif

  #ifdef M_I86SM
    #define __SMALL__
  #endif

  #ifdef M_I86MM
    #define __MEDIUM__
  #endif

  #ifdef M_I86CM
    #define __COMPACT__
  #endif

  #ifdef M_I86LM
    #define __LARGE__
  #endif

  #ifdef M_I86HM
    #define __HUGE__
  #endif

#endif /* ! __WATCOMC__ */


/* Handle 386 "flat" memory model */

#ifdef __FLAT__

  /* Other macros may get defined by braindead compilers */

  #ifdef __SMALL__
    #undef __SMALL_
  #endif

  #ifdef __TINY__
    #undef __TINY__
  #endif

  #ifdef __MEDIUM__
    #undef __MEDIUM__
  #endif

  #ifdef __COMPACT__
    #undef __COMPACT__
  #endif

  #ifdef __LARGE__
    #undef __LARGE__
  #endif

  #ifdef __HUGE__
    #undef __HUGE__
  #endif

  /* Code is really "near", but "far" in this context means that we want    *
   * a 32 bit ptr (vice 16 bit).                                            */

  #define __FARCODE__
  #define __FARDATA__

  /* Everything should be "near" in the flat model */

  #ifdef far
    #undef far
  #endif

  #ifdef near
    #undef near
  #endif

  #ifdef huge
    #undef huge
  #endif

  #define far
  #define near
  #define huge
#endif


#if defined(__SMALL__) || defined(__TINY__)
  #define __NEARCODE__
  #define __NEARDATA__
#endif

#ifdef __MEDIUM__
  #define __FARCODE__
  #define __NEARDATA__
#endif

#ifdef __COMPACT__
  #define __NEARCODE__
  #define __FARDATA__
#endif

#if defined(__LARGE__) || defined(__HUGE__)
  #define __FARCODE__
  #define __FARDATA__
#endif



#if !defined(OS_2) && !defined(__MSDOS__)
  #define __MSDOS__
#endif

/* Compiler-specific stuff:                                                 *
 *                                                                          *
 *  _stdc - Standard calling sequence.  This should be the type of          *
 *          function required for function ptrs for qsort() et al.          *
 *  _fast - Fastest calling sequence supported.  If the default             *
 *          calling sequence is the fastest, or if your compiler            *
 *          only has one, define this to nothing.                           *
 *  _intr - For defining interrupt functions.  For some idiotic             *
 *          reason, MSC requires that interrupts be declared                *
 *          as "cdecl interrupt", instead of just "interrupt".              */

#if defined(__TURBOC__)

  #define _stdc     cdecl
  #define _intr     interrupt far
  #define _intcast  void (_intr *)()
  #define _veccast  _intcast
  #define _fast     pascal
  #define _loadds

  #define NW(var) (void)var
  /* structs are packed in TC by default, accd to TURBOC.CFG */

#elif defined(__MSC__)

  #define _stdc     cdecl
  #define _intr     cdecl interrupt far
  #define _intcast  void (_intr *)()
  #define _veccast  _intcast

  #if _MSC_VER >= 600
    #define _fast _fastcall
  #else
    #define _fast pascal
  #endif

  #pragma pack(1)                 /* Structures should NOT be padded */
  #define NW(var)  var = var      /* NW == No Warning */

#elif defined(__WATCOMC__)

  #define _stdc
  #define _intr     cdecl interrupt __far
  #define _intcast  void (_intr *)()
  #define _veccast  void (__interrupt __far *)()
  #define _fast

  #pragma pack(1)                 /* Structures should NOT be padded */
  #define NW(var)   (void)var

#else
  #error Unknown compiler!

  #define _stdc
  #define _intr     interrupt
  #define _intcast  void (_intr *)()
  #define _veccast  _intr
  #define _fast
  #define NW(var)   (void)var
  #define __MSDOS__
#endif

#endif /* ! __COMPILER_H_DEFINED */

