/***************************************************************************
 *                                                                         *
 *  MAXIMUS-CBCS Source Code, Version 1.02                                 *
 *  Copyright 1989, 1990 by Scott J. Dudley.  All rights reserved.         *
 *                                                                         *
 *  SILT include file                                                      *
 *                                                                         *
 *  For complete details of the licensing restrictions, please refer to    *
 *  the licence agreement, which is published in its entirety in MAX.C     *
 *  and LICENCE.MAX.                                                       *
 *                                                                         *
 *  USE OF THIS FILE IS SUBJECT TO THE RESTRICTIONS CONTAINED IN THE       *
 *  MAXIMUS-CBCS LICENSING AGREEMENT.  IF YOU DO NOT FIND THE TEXT OF THIS *
 *  AGREEMENT IN ANY OF THE  AFOREMENTIONED FILES, OR IF YOU DO NOT HAVE   *
 *  THESE FILES, YOU SHOULD IMMEDIATELY CONTACT THE AUTHOR AT ONE OF THE   *
 *  ADDRESSES LISTED BELOW.  IN NO EVENT SHOULD YOU PROCEED TO USE THIS    *
 *  FILE WITHOUT HAVING ACCEPTED THE TERMS OF THE MAXIMUS-CBCS LICENSING   *
 *  AGREEMENT, OR SUCH OTHER AGREEMENT AS YOU ARE ABLE TO REACH WITH THE   *
 *  AUTHOR.                                                                *
 *                                                                         *
 *  You can contact the author at one of the address listed below:         *
 *                                                                         *
 *  Scott Dudley           FidoNet  1:249/106                              *
 *  777 Downing St.        IMEXnet  89:483/202                             *
 *  Kingston, Ont.         Internet f106.n249.z1.fidonet.org               *
 *  Canada - K7M 5N3       BBS      (613) 389-8315 - HST/14.4K, 24hrs      *
 *                                                                         *
 ***************************************************************************/

#ifdef __TURBOC__
#pragma warn -rch
#endif

#define HEAP_SIZE 0x2000
#define MAX_BUFR 512

int Aidx_Cmp(struct _aidx *a1,struct _aidx *a2);

#define Make_String(item,value)   (item)=Make_Strng(value,FALSE);
#define Make_Filename(item,value) (item)=Make_Strng(value,TRUE);
#define Make_Path(item, str)      (item)=Make_Pth(str)

int Make_Strng(char *value,int type);
int Make_Pth(char *value);
void Strip_Comment(char *l);


Parse_Ctlfile(char *ctlname);
Initialize_Prm(void);
Parse_System(FILE *ctlfile);
Parse_Equipment(FILE *ctlfile);
Parse_Matrix(FILE *ctlfile);
Parse_Session(FILE *ctlfile);
Parse_Area(FILE *ctlfile,char *name);
Deduce_Priv(char *p);
Deduce_Attribute(char *a);
Deduce_Class(int priv);
Unknown_Ctl(int linenum,char *p);
Compiling(char type,char *string,char *name);
Blank_Sys(struct _sys *sys,int mode);
Add_Backslash(char *s);
Remove_Backslash(char *s);
int Parse_Weekday(char *s);
pascal Parse_Menu(FILE *ctlfile,char *name);
Parse_Option(option opt,char *line);
Init_Menu(void);
byte Deduce_Lock(char *p);
void Attrib_Or(int clnum,int attr,struct _area *area);
void Attrib_And(int clnum,int attr,struct _area *area);
void pascal Add_Path(char *s,int warn);
void pascal Add_Filename(char *s);
char *fchar(char *str,char *delim,int wordno);
int Add_To_Heap(char *s,int fancy);
void strnncpy(char *to,char *from,int len);
void Blank_Area(struct _area *area);
void Fmt(void);
byte MaxPrivToOpus(int maxpriv);
void pascal Add_Specific_Path(char *frompath,char *topath,char *add_path);
int makedir(char *d);

