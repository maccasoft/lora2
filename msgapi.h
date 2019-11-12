/***************************************************************************
 *                                                                         *
 *  MSGAPI Source Code, Version 2.00                                       *
 *  Copyright 1989-1991 by Scott J. Dudley.  All rights reserved.          *
 *                                                                         *
 *  MsgAPI main header file                                                *
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

/* $Id: msgapi.h_v 1.0 1991/11/16 16:16:51 sjd Rel sjd $ */

#ifndef __SQAPI_H_DEFINED
#define __SQAPI_H_DEFINED

/* #define TSR*/

#include "stamp.h"
#include "typedefs.h"
#include "compiler.h"

#define MSGAPI

#ifdef __OS2__
#ifndef EXPENTRY
#define EXPENTRY pascal far _loadds
#endif

#define OS2LOADDS _loadds
#else
#ifndef EXPENTRY
#define EXPENTRY pascal
#endif
#define OS2LOADDS
#endif


#define MSGAREA_NORMAL  0x00
#define MSGAREA_CREATE  0x01
#define MSGAREA_CRIFNEC 0x02

#define MSGTYPE_SDM     0x01
#define MSGTYPE_SQUISH  0x02
#define MSGTYPE_ECHO    0x80

#define MSGNUM_CUR      -1L
#define MSGNUM_PREV     -2L
#define MSGNUM_NEXT     -3L

#define MSGNUM_current  MSGNUM_CUR
#define MSGNUM_previous MSGNUM_PREV
#define MSGNUM_next     MSGNUM_NEXT

#define MOPEN_CREATE    0
#define MOPEN_READ      1
#define MOPEN_WRITE     2
#define MOPEN_RW        3

struct _msgapi;
struct _msgh;
struct _netaddr;

typedef struct _msgapi MSG;
typedef struct _msgh MSGH;
typedef dword UMSGID;
typedef struct _netaddr NETADDR;



struct _minf
{
    word req_version;
    word def_zone;
    word haveshare;  /* filled in by msgapi routines - no need to set this */
};


/* The network address structure.  The z/n/n/p fields are always             *
 * maintained in parallel to the 'ascii' field, which is simply an ASCII     *
 * representation of the address.  In addition, the 'ascii' field can        *
 * be used for other purposes (such as internet addresses), so the           *
 * contents of this field are implementation-defined, but for most cases,    *
 * should be in the format "1:123/456.7" for Fido addresses.                 */

struct _netaddr
{
    word zone;
    word net;
    word node;
    word point;
};


/* The eXtended message structure.  Translation between this structure, and *
 * the structure used by the individual message base formats, is done       *
 * on-the-fly by the API routines.                                          */

typedef struct
{
    dword attr;

    /* Bitmasks for 'attr' */

#define MSGPRIVATE 0x0001
#define MSGCRASH   0x0002
#define MSGREAD    0x0004
#define MSGSENT    0x0008
#define MSGFILE    0x0010
#define MSGFWD     0x0020
#define MSGORPHAN  0x0040
#define MSGKILL    0x0080
#define MSGLOCAL   0x0100
#define MSGHOLD    0x0200
#define MSGXX2     0x0400
#define MSGFRQ     0x0800
#define MSGRRQ     0x1000
#define MSGCPT     0x2000
#define MSGARQ     0x4000
#define MSGURQ     0x8000
#define MSGSCANNED 0x00010000L


#define XMSG_FROM_SIZE  36
#define XMSG_TO_SIZE    36
#define XMSG_SUBJ_SIZE  72

    byte from[XMSG_FROM_SIZE];
    byte to[XMSG_TO_SIZE];
    byte subj[XMSG_SUBJ_SIZE];

    NETADDR orig;        /* Origination and destination addresses             */
    NETADDR dest;

    struct _stamp2 date_written;   /* When user wrote the msg (UTC)            */
    struct _stamp2 date_arrived;   /* When msg arrived on-line (UTC)           */
    sword utc_ofs;                /* Offset from UTC of message writer, in    *
                                 * minutes.                                 */

#define MAX_REPLY 10          /* Max number of stored replies to one msg  */

    UMSGID replyto;
    UMSGID replies[MAX_REPLY];

    byte ftsc_date[20];  /* Obsolete date information.  If it weren't for the *
                        * fact that FTSC standards say that one cannot      *
                        * modify an in-transit message, I'd be VERY         *
                        * tempted to axe this field entirely, and recreate  *
                        * an FTSC-compatible date field using               *
                        * the information in 'date_written' upon            *
                        * export.  Nobody should use this field, except     *
                        * possibly for tossers and scanners.  All others    *
                        * should use one of the two binary datestamps,      *
                        * above.                                            */
} XMSG;





/* This is a 'message area handle', as returned by MsgOpenArea(), and       *
 * required by calls to all other message functions.  This structure        *
 * must always be accessed through the API functions, and never             *
 * modified directly.                                                       */

struct _msgapi
{
#define MSGAPI_ID   0x0201414dL

    dword id;                       /* Must always equal MSGAPI_ID */

    word len;                       /* LENGTH OF THIS STRUCTURE! */
    word type;

    dword num_msg;
    dword cur_msg;
    dword high_msg;
    dword high_water;

    word sz_xmsg;

    byte locked;                    /* Base is locked from use by other tasks */
    byte isecho;                    /* Is this an EchoMail area?              */

    /* Function pointers for manipulating messages within this area.          */
    struct _apifuncs
    {
        sword(EXPENTRY * CloseArea)(MSG * mh);
        MSGH * (EXPENTRY * OpenMsg)(MSG * mh, word mode, dword n);
        sword(EXPENTRY * CloseMsg)(MSGH * msgh);
        dword(EXPENTRY * ReadMsg)(MSGH * msgh, XMSG * msg, dword ofs,
                                  dword bytes, byte * text, dword cbyt,
                                  byte * ctxt);
        sword(EXPENTRY * WriteMsg)(MSGH * msgh, word append, XMSG * msg,
                                   byte * text, dword textlen, dword totlen,
                                   dword clen, byte * ctxt);
        sword(EXPENTRY * KillMsg)(MSG * mh, dword msgnum);
        sword(EXPENTRY * Lock)(MSG * mh);
        sword(EXPENTRY * Unlock)(MSG * mh);
        sword(EXPENTRY * SetCurPos)(MSGH * msgh, dword pos);
        dword(EXPENTRY * GetCurPos)(MSGH * msgh);
        UMSGID(EXPENTRY * MsgnToUid)(MSG * mh, dword msgnum);
        dword(EXPENTRY * UidToMsgn)(MSG * mh, UMSGID umsgid, word type);
        dword(EXPENTRY * GetHighWater)(MSG * mh);
        sword(EXPENTRY * SetHighWater)(MSG * mh, dword hwm);
        dword(EXPENTRY * GetTextLen)(MSGH * msgh);
        dword(EXPENTRY * GetCtrlLen)(MSGH * msgh);
    } * api;

    /* Pointer to application-specific data.  API_SQ.C and API_SDM.C use      *
     * this for different things, so again, no applications should muck       *
     * with anything in here.                                                 */

    void /*far*/ *apidata;
};




/* This is a 'dummy' message handle.  The other message handlers (contained *
 * in API_SQ.C and API_SDM.C) will define their own structures, with some   *
 * application-specified variables instead of other[].  Applications should *
 * not mess with anything inside the _msgh (or MSGH) structure.             */

#define MSGH_ID  0x0302484dL

#if !defined(MSGAPI_HANDLERS) && !defined(NO_MSGH_DEF)
struct _msgh
{
    MSG * sq;
    dword id;

    dword bytes_written;
    dword cur_pos;
};
#endif



#include "api_brow.h"





/* This variable is modified whenever an error occurs with the Msg...()     *
 * functions.  If msgapierr==0, then no error occurred.                     */

extern word _stdc msgapierr;
/*extern word _stdc haveshare;*/
extern struct _minf _stdc mi;

#ifdef TSR
#define IMS_ID    0x4453u
#define IMS_INTR  0x32
#endif

#if defined(TSR) && defined(MSGAPI_PROC)
extern void * (_stdc * memalloc)(word len);
extern void (_stdc * memfree)(void * block);

#define palloc(s)     (*memalloc)(s)
#define pfree(s)      (*memfree)(s)
#define farpalloc(s)  palloc(s)
#define farpfree(s)   pfree(s)
#else
#define palloc(s)     malloc(s)
#define pfree(s)      free(s)
#define farpalloc(s)  farmalloc(s)
#define farpfree(s)   farfree(s)
#endif


/* Constants for 'type' argument of MsgUidToMsgn() */

#define UID_EXACT     0x00
#define UID_NEXT      0x01
#define UID_PREV      0x02


/* Values for 'msgapierr', above. */

#define MERR_NONE   0     /* No error                                       */
#define MERR_BADH   1     /* Invalid handle passed to function              */
#define MERR_BADF   2     /* Invalid or corrupted file                      */
#define MERR_NOMEM  3     /* Not enough memory for specified operation      */
#define MERR_NODS   4     /* Maybe not enough disk space for operation      */
#define MERR_NOENT  5     /* File/message does not exist                    */
#define MERR_BADA   6     /* Bad argument passed to msgapi function         */
#define MERR_EOPEN  7     /* Couldn't close - messages still open           */


/* Now, a set of macros, which call the specified API function.  These      *
 * will map calls for 'MsgOpenMsg()' into 'SquishOpenMsg()',                *
 * 'SdmOpenMsg()', or '<insert fave message type here>'.  Applications      *
 * should always call these macros, instead of trying to call the           *
 * manipulation functions directly.                                         */

#define MsgCloseArea(mh)       (*(mh)->api->CloseArea) (mh)
#define MsgOpenMsg(mh,mode,n)  (*(mh)->api->OpenMsg)          (mh,mode,n)
#define MsgCloseMsg(msgh)      ((*(((struct _msgh *)msgh)->sq->api->CloseMsg))(msgh))
#define MsgReadMsg(msgh,msg,ofs,b,t,cl,ct) (*(((struct _msgh *)msgh)->sq->api->ReadMsg))(msgh,msg,ofs,b,t,cl,ct)
#define MsgWriteMsg(gh,a,m,t,tl,ttl,cl,ct) (*(((struct _msgh *)gh)->sq->api->WriteMsg))(gh,a,m,t,tl,ttl,cl,ct)
#define MsgKillMsg(mh,msgnum)  (*(mh)->api->KillMsg)(mh,msgnum)
#define MsgLock(mh)            (*(mh)->api->Lock)(mh)
#define MsgUnlock(mh)          (*(mh)->api->Unlock)(mh)
#define MsgGetCurPos(msgh)     (*(((struct _msgh *)msgh)->sq->api->GetCurPos))(msgh)
#define MsgSetCurPos(msgh,pos) (*(((struct _msgh *)msgh)->sq->api->SetCurPos))(msgh,pos)
#define MsgMsgnToUid(mh,msgn)  (*(mh)->api->MsgnToUid)(mh,msgn)
#define MsgUidToMsgn(mh,umsgid,t) (*(mh)->api->UidToMsgn)(mh,umsgid,t)
#define MsgGetHighWater(mh)   (*(mh)->api->GetHighWater)(mh)
#define MsgSetHighWater(mh,n) (*(mh)->api->SetHighWater)(mh,n)
#define MsgGetTextLen(msgh)   (*(((struct _msgh *)msgh)->sq->api->GetTextLen))(msgh)
#define MsgGetCtrlLen(msgh)   (*(((struct _msgh *)msgh)->sq->api->GetCtrlLen))(msgh)

/* These don't actually call any functions, but are macros used to access    *
 * private data inside the _msgh structure.                                  */

#define MsgCurMsg(mh)         ((mh)->cur_msg)
#define MsgNumMsg(mh)         ((mh)->num_msg)
#define MsgHighMsg(mh)        ((mh)->high_msg)

#define MsgGetCurMsg(mh)      ((mh)->cur_msg)
#define MsgGetNumMsg(mh)      ((mh)->num_msg)
#define MsgGetHighMsg(mh)     ((mh)->high_msg)

sword EXPENTRY MsgOpenApi(struct _minf * minf);
sword EXPENTRY MsgCloseApi(void);

MSG * EXPENTRY MsgOpenArea(byte * name, word mode, word type);
sword EXPENTRY MsgValidate(word type, byte * name);
sword EXPENTRY MsgBrowseArea(BROWSE * b);

sword MSGAPI InvalidMsgh(MSGH * msgh);
sword MSGAPI InvalidMh(MSG * mh);

void EXPENTRY SquishSetMaxMsg(MSG * sq, dword max_msgs, dword skip_msgs, dword age);
dword EXPENTRY SquishHash(byte * f);



MSG * MSGAPI SdmOpenArea(byte * name, word mode, word type);
sword MSGAPI SdmValidate(byte * name);

MSG * MSGAPI SquishOpenArea(byte * name, word mode, word type);
sword MSGAPI SquishValidate(byte * name);


byte * EXPENTRY CvtCtrlToKludge(byte * ctrl);
byte * EXPENTRY GetCtrlToken(byte * where, byte * what);
byte * EXPENTRY CopyToControlBuf(byte * txt, byte ** newtext, unsigned * length);
void EXPENTRY ConvertControlInfo(byte * ctrl, NETADDR * orig, NETADDR * dest);
word EXPENTRY NumKludges(char * txt);
void EXPENTRY RemoveFromCtrl(byte * ctrl, byte * what);


sword far pascal farread(sword handle, byte far * buf, word len);
sword far pascal farwrite(sword handle, byte far * buf, word len);
byte * _fast Address(NETADDR * a);
byte * StripNasties(byte * str);

#ifdef __MSDOS__
sword far pascal shareloaded(void);
#else
#define shareloaded() TRUE
#endif

#endif





