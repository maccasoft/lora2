#=============================================================
#
#	LORA.MAK - Makefile for project C:\BCOS2\LORA\LORA.PRJ
#		Created on 05/21/93 at 22:25
#
#=============================================================

.AUTODEPEND

#=============================================================
#		Translator Definitions
#=============================================================
CC = bcc +BCOS2.CFG
TASM = tasm.exe
TLIB = tlib.exe
TLINK = tlink
RC = brcc.exe
RB = rc.exe
LIBPATH = C:\BCOS2\LIB
INCLUDEPATH = C:\BCOS2\INCLUDE
OBJ = OBJ
SRC = C:\BORLANDC\LORA\SRC


#=============================================================
#		Implicit Rules
#=============================================================
.c.obj:
  $(CC) -c {$< }

.cpp.obj:
  $(CC) -c {$< }

.asm.obj:
  $(TASM) -Mx $*.asm,$*.obj

.rc.res:
  $(RC) -r $*.rc

#=============================================================
#		List Macros
#=============================================================
ALL: LORA LORAOVL LSETUP LMSG LTOP LUSER LNETMGR LANGCOMP MAKEINST INSTALL FILEIDX

LORA:      LORAP.EXE
LORAOVL:   LORAP.OVL
LSETUP:    LSETUP.EXE
LMSG:      LMSG.EXE
LTOP:      LTOP.EXE
LUSER:     LUSER.EXE
LNETMGR:   LNETMGR.EXE
LANGCOMP:  LANGCOMP.EXE
MAKEINST:  MAKEINST.EXE
INSTALL:   INSTALL.EXE
FILEIDX:   FILEIDX.EXE

LORAOVL_DEPENDENCIES =  \
C:\BCOS2\SQUISH\SQUISH2.LIB \
C:\BCOS2\CXL\CXLOS2.LIB \
  $(OBJ)\areafix.obj \
  $(OBJ)\areafix2.obj \
  $(OBJ)\change.obj \
  $(OBJ)\chat.obj \
  $(OBJ)\cmdfile.obj \
  $(OBJ)\colors.obj \
  $(OBJ)\config.obj \
  $(OBJ)\data.obj \
  $(OBJ)\door.obj \
  $(OBJ)\editor.obj \
  $(OBJ)\editor1.obj \
  $(OBJ)\export.obj \
  $(OBJ)\files.obj \
  $(OBJ)\files2.obj \
  $(OBJ)\ftsc.obj \
  $(OBJ)\fulled.obj \
  $(OBJ)\general.obj \
  $(OBJ)\gestint.obj \
  $(OBJ)\import.obj \
  $(OBJ)\import2.obj \
  $(OBJ)\input.obj \
  $(OBJ)\janus.obj \
  $(OBJ)\langload.obj \
  $(OBJ)\login.obj \
  $(OBJ)\lora.obj \
  $(OBJ)\lora_ovl.obj \
  $(OBJ)\lrpn.obj \
  $(OBJ)\maintask.obj \
  $(OBJ)\menu.obj \
  $(OBJ)\message.obj\
  $(OBJ)\message1.obj\
  $(OBJ)\misc.obj \
  $(OBJ)\modem.obj \
  $(OBJ)\ovrmisc.obj \
  $(OBJ)\pack.obj \
  $(OBJ)\pipbase.obj \
  $(OBJ)\pipbase2.obj \
  $(OBJ)\poller.obj \
  $(OBJ)\quickmsg.obj \
  $(OBJ)\quickms2.obj \
  $(OBJ)\qwkspt.obj \
  $(OBJ)\reader.obj \
  $(OBJ)\readmail.obj \
  $(OBJ)\sched.obj \
  $(OBJ)\script.obj \
  $(OBJ)\squish.obj \
  $(OBJ)\status.obj \
  $(OBJ)\terminal.obj \
  $(OBJ)\timer.obj \
  $(OBJ)\utility.obj \
  $(OBJ)\vdisk.obj \
  $(OBJ)\video.obj \
  $(OBJ)\wazoo.obj \
  $(OBJ)\yoohoo.obj \
  $(OBJ)\zmisc.obj \
  $(OBJ)\zsend.obj \
  $(OBJ)\zreceive.obj

LMSG_DEPENDENCIES = \
C:\BCOS2\SQUISH\SQUISH2.LIB \
$(OBJ)\lmsg.obj

LSETUP_DEPENDENCIES =  \
C:\BCOS2\CXL\CXLOS2.LIB \
 $(OBJ)\lsetup.obj \
 $(OBJ)\lsetup2.obj \
 $(OBJ)\lsetup3.obj \
 $(OBJ)\lsetup4.obj \
 $(OBJ)\lsetup5.obj \
 $(OBJ)\lsetup6.obj \
 $(OBJ)\lsetup7.obj

LORA_DEPENDENCIES =  \
  $(OBJ)\startup.obj

LTOP_DEPENDENCIES =  \
  $(OBJ)\ltop.obj

LUSER_DEPENDENCIES =  \
  $(OBJ)\luser.obj

LNETMGR_DEPENDENCIES =  \
  $(OBJ)\lnetmgr.obj

LANGCOMP_DEPENDENCIES =  \
  $(OBJ)\loralng.obj \
  $(OBJ)\get_lang.obj \
  $(OBJ)\put_lang.obj

MAKEINST_DEPENDENCIES =  \
  $(OBJ)\makeinst.obj

INSTALL_DEPENDENCIES =  \
C:\BCOS2\CXL\CXLOS2.LIB \
  $(OBJ)\install.obj

IDX_DEPENDENCIES = \
  $(OBJ)\fileidx.obj

#=============================================================
#		Explicit Rules
#=============================================================
lorap.ovl: bcos2.cfg $(LORAOVL_DEPENDENCIES)
  $(TLINK) /Toe /ap /s /L$(LIBPATH) @&&|
C:\BCOS2\LIB\C02.OBJ+
$(OBJ)\colors.obj+
$(OBJ)\lora.obj+
$(OBJ)\misc.obj+
$(OBJ)\modem.obj+
$(OBJ)\sched.obj+
$(OBJ)\status.obj+
$(OBJ)\timer.obj+
$(OBJ)\vdisk.obj+
$(OBJ)\areafix.obj+
$(OBJ)\areafix2.obj+
$(OBJ)\change.obj+
$(OBJ)\chat.obj+
$(OBJ)\cmdfile.obj+
$(OBJ)\config.obj+
$(OBJ)\data.obj+
$(OBJ)\door.obj +
$(OBJ)\editor.obj+
$(OBJ)\editor1.obj+
$(OBJ)\export.obj+
$(OBJ)\files.obj+
$(OBJ)\files2.obj+
$(OBJ)\ftsc.obj+
$(OBJ)\fulled.obj+
$(OBJ)\general.obj+
$(OBJ)\gestint.obj+
$(OBJ)\import.obj+
$(OBJ)\import2.obj+
$(OBJ)\input.obj+
$(OBJ)\janus.obj+
$(OBJ)\langload.obj+
$(OBJ)\login.obj+
$(OBJ)\lora_ovl.obj+
$(OBJ)\lrpn.obj+
$(OBJ)\maintask.obj+
$(OBJ)\menu.obj+
$(OBJ)\message.obj+
$(OBJ)\message1.obj+
$(OBJ)\ovrmisc.obj+
$(OBJ)\pack.obj+
$(OBJ)\pipbase.obj+
$(OBJ)\pipbase2.obj+
$(OBJ)\poller.obj+
$(OBJ)\quickmsg.obj+
$(OBJ)\quickms2.obj+
$(OBJ)\qwkspt.obj+
$(OBJ)\reader.obj+
$(OBJ)\readmail.obj+
$(OBJ)\script.obj+
$(OBJ)\squish.obj+
$(OBJ)\terminal.obj+
$(OBJ)\utility.obj+
$(OBJ)\video.obj+
$(OBJ)\wazoo.obj+
$(OBJ)\yoohoo.obj+
$(OBJ)\zmisc.obj+
$(OBJ)\zsend.obj+
$(OBJ)\zreceive.obj
lorap.ovl,
C:\BCOS2\CXL\CXLOS2.LIB+
C:\BCOS2\SQUISH\SQUISH2.LIB+
C:\BCOS2\LIB\C2.LIB+
C:\BCOS2\LIB\OS2.LIB

|

lsetup.exe: bcos2.cfg $(LSETUP_DEPENDENCIES)
  $(TLINK) /Toe /ap /s /L$(LIBPATH) @&&|
C:\BCOS2\LIB\C02.OBJ+
$(OBJ)\lsetup.obj+
$(OBJ)\lsetup2.obj+
$(OBJ)\lsetup3.obj+
$(OBJ)\lsetup4.obj+
$(OBJ)\lsetup5.obj+
$(OBJ)\lsetup6.obj+
$(OBJ)\lsetup7.obj
lsetup,
C:\BCOS2\CXL\CXLOS2.LIB+
C:\BCOS2\LIB\C2.LIB+
C:\BCOS2\LIB\OS2.LIB

|


lmsg.exe: bcos2.cfg $(LMSG_DEPENDENCIES)
  $(TLINK) /Toe /ap /x /L$(LIBPATH) @&&|
C:\BCOS2\LIB\C02.OBJ+
$(OBJ)\lmsg.obj
lmsg,
C:\BCOS2\SQUISH\SQUISH2.LIB+
C:\BCOS2\LIB\C2.LIB+
C:\BCOS2\LIB\OS2.LIB

|


lorap.exe: bcos2.cfg $(LORA_DEPENDENCIES)
  $(TLINK) /Toe /ap /x /L$(LIBPATH) @&&|
C:\BCOS2\LIB\C02.OBJ+
$(OBJ)\startup.obj
lorap,
C:\BCOS2\LIB\C2.LIB+
C:\BCOS2\LIB\OS2.LIB

|


ltop.exe: bcos2.cfg $(LTOP_DEPENDENCIES)
  $(TLINK) /Toe /ap /x /L$(LIBPATH) @&&|
C:\BCOS2\LIB\C02.OBJ+
$(OBJ)\ltop.obj
ltop,
C:\BCOS2\LIB\C2.LIB+
C:\BCOS2\LIB\OS2.LIB

|


luser.exe: bcos2.cfg $(LUSER_DEPENDENCIES)
  $(TLINK) /Toe /ap /x /L$(LIBPATH) @&&|
C:\BCOS2\LIB\C02.OBJ+
$(OBJ)\luser.obj
luser,
C:\BCOS2\LIB\C2.LIB+
C:\BCOS2\LIB\OS2.LIB

|


lnetmgr.exe: bcos2.cfg $(LNETMGR_DEPENDENCIES)
  $(TLINK) /Toe /ap /x /L$(LIBPATH) @&&|
C:\BCOS2\LIB\C02.OBJ+
$(OBJ)\lnetmgr.obj
lnetmgr,
C:\BCOS2\CXL\CXLOS2.LIB+
C:\BCOS2\LIB\C2.LIB+
C:\BCOS2\LIB\OS2.LIB

|


langcomp.exe: bcos2.cfg $(LANGCOMP_DEPENDENCIES)
  $(TLINK) /Toe /ap /x /L$(LIBPATH) @&&|
C:\BCOS2\LIB\C02.OBJ+
$(OBJ)\loralng.obj+
$(OBJ)\get_lang.obj+
$(OBJ)\put_lang.obj
langcomp,
C:\BCOS2\LIB\C2.LIB+
C:\BCOS2\LIB\OS2.LIB

|


makeinst.exe: bcos2.cfg $(MAKEINST_DEPENDENCIES)
  $(TLINK) /Toe /ap /x /L$(LIBPATH) @&&|
C:\BCOS2\LIB\C02.OBJ+
$(OBJ)\makeinst.obj
makeinst,
C:\BCOS2\LIB\C2.LIB+
C:\BCOS2\LIB\OS2.LIB

|


install.exe: bcos2.cfg $(INSTALL_DEPENDENCIES)
  $(TLINK) /Toe /ap /x /L$(LIBPATH) @&&|
C:\BCOS2\LIB\C02.OBJ+
$(OBJ)\install.obj
install,
C:\BCOS2\CXL\CXLOS2.LIB+
C:\BCOS2\LIB\C2.LIB+
C:\BCOS2\LIB\OS2.LIB

|


fileidx.exe: bcos2.cfg $(IDX_DEPENDENCIES)
  $(TLINK) /Toe /ap /x /L$(LIBPATH) @&&|
C:\BCOS2\LIB\C02.OBJ+
$(OBJ)\fileidx.obj
fileidx,
C:\BCOS2\LIB\C2.LIB+
C:\BCOS2\LIB\OS2.LIB

|


#=============================================================
#		Individual File Dependencies
#=============================================================
$(OBJ)\areafix.obj: $(SRC)\areafix.c
        $(CC) -c $(SRC)\areafix.c

$(OBJ)\areafix2.obj: $(SRC)\areafix2.c
        $(CC) -c $(SRC)\areafix2.c

$(OBJ)\change.obj: $(SRC)\change.c
        $(CC) -c $(SRC)\change.c

$(OBJ)\chat.obj: $(SRC)\chat.c
        $(CC) -c $(SRC)\chat.c

$(OBJ)\cmdfile.obj: $(SRC)\cmdfile.c
        $(CC) -c $(SRC)\cmdfile.c

$(OBJ)\colors.obj: $(SRC)\colors.c
        $(CC) -c $(SRC)\colors.c

$(OBJ)\config.obj: $(SRC)\config.c
        $(CC) -c $(SRC)\config.c

$(OBJ)\data.obj: $(SRC)\data.c
        $(CC) -c $(SRC)\data.c

$(OBJ)\door.obj: $(SRC)\door.c
        $(CC) -c $(SRC)\door.c

$(OBJ)\editor.obj: $(SRC)\editor.c
        $(CC) -c $(SRC)\editor.c

$(OBJ)\editor1.obj: $(SRC)\editor1.c
        $(CC) -c $(SRC)\editor1.c

$(OBJ)\export.obj: $(SRC)\export.c
        $(CC) -c $(SRC)\export.c

$(OBJ)\files.obj: $(SRC)\files.c
        $(CC) -c $(SRC)\files.c

$(OBJ)\files2.obj: $(SRC)\files2.c
        $(CC) -c $(SRC)\files2.c

$(OBJ)\ftsc.obj: $(SRC)\ftsc.c
        $(CC) -c $(SRC)\ftsc.c

$(OBJ)\fulled.obj: $(SRC)\fulled.c
        $(CC) -c $(SRC)\fulled.c

$(OBJ)\general.obj: $(SRC)\general.c
        $(CC) -c $(SRC)\general.c

$(OBJ)\gestint.obj: $(SRC)\gestint.c
        $(CC) -c $(SRC)\gestint.c

$(OBJ)\import.obj: $(SRC)\import.c
        $(CC) -c $(SRC)\import.c

$(OBJ)\import2.obj: $(SRC)\import2.c
        $(CC) -c $(SRC)\import2.c

$(OBJ)\input.obj: $(SRC)\input.c
        $(CC) -c $(SRC)\input.c

$(OBJ)\janus.obj: $(SRC)\janus.c
        $(CC) -c $(SRC)\janus.c

$(OBJ)\langload.obj: $(SRC)\langload.c
        $(CC) -c $(SRC)\langload.c

$(OBJ)\login.obj: $(SRC)\login.c
        $(CC) -c $(SRC)\login.c

$(OBJ)\lora.obj: $(SRC)\lora.c
        $(CC) -c $(SRC)\lora.c

$(OBJ)\lora_ovl.obj: $(SRC)\lora_ovl.c
        $(CC) -c $(SRC)\lora_ovl.c

$(OBJ)\lrpn.obj: $(SRC)\lrpn.c
        $(CC) -c $(SRC)\lrpn.c

$(OBJ)\maintask.obj: $(SRC)\maintask.c
        $(CC) -c $(SRC)\maintask.c

$(OBJ)\menu.obj: $(SRC)\menu.c
        $(CC) -c $(SRC)\menu.c

$(OBJ)\message.obj: $(SRC)\message.c
        $(CC) -c $(SRC)\message.c

$(OBJ)\message1.obj: $(SRC)\message1.c
        $(CC) -c $(SRC)\message1.c

$(OBJ)\misc.obj: $(SRC)\misc.c
        $(CC) -c $(SRC)\misc.c

$(OBJ)\modem.obj: $(SRC)\modem.c
        $(CC) -c $(SRC)\modem.c

$(OBJ)\ovrmisc.obj: $(SRC)\ovrmisc.c
        $(CC) -c $(SRC)\ovrmisc.c

$(OBJ)\pack.obj: $(SRC)\pack.c
        $(CC) -c $(SRC)\pack.c

$(OBJ)\pipbase.obj: $(SRC)\pipbase.c
        $(CC) -c $(SRC)\pipbase.c

$(OBJ)\pipbase2.obj: $(SRC)\pipbase2.c
        $(CC) -c $(SRC)\pipbase2.c

$(OBJ)\poller.obj: $(SRC)\poller.c
        $(CC) -c $(SRC)\poller.c

$(OBJ)\quickmsg.obj: $(SRC)\quickmsg.c
        $(CC) -c $(SRC)\quickmsg.c

$(OBJ)\quickms2.obj: $(SRC)\quickms2.c
        $(CC) -c $(SRC)\quickms2.c

$(OBJ)\qwkspt.obj: $(SRC)\qwkspt.c
        $(CC) -c $(SRC)\qwkspt.c

$(OBJ)\reader.obj: $(SRC)\reader.c
        $(CC) -c $(SRC)\reader.c

$(OBJ)\readmail.obj: $(SRC)\readmail.c
        $(CC) -c $(SRC)\readmail.c

$(OBJ)\sched.obj: $(SRC)\sched.c
        $(CC) -c $(SRC)\sched.c

$(OBJ)\script.obj: $(SRC)\script.c
        $(CC) -c $(SRC)\script.c

$(OBJ)\squish.obj: $(SRC)\squish.c
        $(CC) -c $(SRC)\squish.c

$(OBJ)\status.obj: $(SRC)\status.c
        $(CC) -c $(SRC)\status.c

$(OBJ)\terminal.obj: $(SRC)\terminal.c
        $(CC) -c $(SRC)\terminal.c

$(OBJ)\timer.obj: $(SRC)\timer.c
        $(CC) -c $(SRC)\timer.c

$(OBJ)\utility.obj: $(SRC)\utility.c
        $(CC) -c $(SRC)\utility.c

$(OBJ)\video.obj: $(SRC)\video.c
        $(CC) -c $(SRC)\video.c

$(OBJ)\vdisk.obj: $(SRC)\vdisk.c
        $(CC) -c $(SRC)\vdisk.c

$(OBJ)\wazoo.obj: $(SRC)\wazoo.c
        $(CC) -c $(SRC)\wazoo.c

$(OBJ)\yoohoo.obj: $(SRC)\yoohoo.c
        $(CC) -c $(SRC)\yoohoo.c

$(OBJ)\zmisc.obj: $(SRC)\zmisc.c
        $(CC) -c $(SRC)\zmisc.c

$(OBJ)\zsend.obj: $(SRC)\zsend.c
        $(CC) -c $(SRC)\zsend.c

$(OBJ)\zreceive.obj: $(SRC)\zreceive.c
        $(CC) -c $(SRC)\zreceive.c


$(OBJ)\LSETUP.obj: $(SRC)\LSETUP.C
        $(CC) -c $(SRC)\LSETUP.C

$(OBJ)\LSETUP2.obj: $(SRC)\LSETUP2.C
        $(CC) -c $(SRC)\LSETUP2.C

$(OBJ)\LSETUP3.obj: $(SRC)\LSETUP3.C
        $(CC) -c $(SRC)\LSETUP3.C

$(OBJ)\LSETUP4.obj: $(SRC)\LSETUP4.C
        $(CC) -c $(SRC)\LSETUP4.C

$(OBJ)\LSETUP5.obj: $(SRC)\LSETUP5.C
        $(CC) -c $(SRC)\LSETUP5.C

$(OBJ)\LSETUP6.obj: $(SRC)\LSETUP6.C
        $(CC) -c $(SRC)\LSETUP6.C

$(OBJ)\LSETUP7.obj: $(SRC)\LSETUP7.C
        $(CC) -c $(SRC)\LSETUP7.C


$(OBJ)\LMSG.obj: $(SRC)\LMSG.C
        $(CC) -c $(SRC)\LMSG.C


$(OBJ)\startup.obj: $(SRC)\startup.c
        $(CC) -c $(SRC)\startup.c

      
$(OBJ)\ltop.obj: $(SRC)\ltop.c
        $(CC) -c $(SRC)\ltop.c


$(OBJ)\luser.obj: $(SRC)\luser.c
        $(CC) -c $(SRC)\luser.c


$(OBJ)\lnetmgr.obj: $(SRC)\lnetmgr.c
        $(CC) -c $(SRC)\lnetmgr.c


$(OBJ)\loralng.obj: $(SRC)\loralng.c
        $(CC) -c $(SRC)\loralng.c

$(OBJ)\put_lang.obj: $(SRC)\put_lang.c
        $(CC) -c $(SRC)\put_lang.c

$(OBJ)\get_lang.obj: $(SRC)\get_lang.c
        $(CC) -c $(SRC)\get_lang.c


$(OBJ)\makeinst.obj: $(SRC)\makeinst.c
        $(CC) -c $(SRC)\makeinst.c


$(OBJ)\install.obj: $(SRC)\install.c
        $(CC) -c $(SRC)\install.c


$(OBJ)\fileidx.obj: $(SRC)\fileidx.c
        $(CC) -c $(SRC)\fileidx.c

#=============================================================
#		Compiler Configuration File
#=============================================================
bcos2.cfg: makefile
  copy &&|
-a1
-vi-
-d
-O1
-N
-w-ccc
-wnod
-wuse
-wstv
-wdef
-wcln
-wasm
-wamp
-wamb
-wbbf
-I$(INCLUDEPATH)
-L$(LIBPATH)
-nOBJ
| bcos2.cfg


