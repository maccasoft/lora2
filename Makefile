.AUTODEPEND

OBJ = OBJ
SRC = SRC
UTI = $(SRC)\UTIL
TCLIB = D:\BORLANDC\LIB
CXLIB = $(TCLIB)\CXL

#		*Translator Definitions*
CC = bcc +TURBOC.CFG -c -v -y
TASM = tasm /MX /ZD
TLINK = tlink /x
LIBPATH = D:\BORLANDC\LIB
INCLUDEPATH = D:\BORLANDC\INCLUDE


#		*Implicit Rules*
.c.obj:
  $(CC) -c -n$(OBJ) {$< }

.cpp.obj:
  $(CC) -c -n$(OBJ) {$< }

.asm.obj:
  $(TASM) {$< }


#		*List Macros*

all: lora loraovl lsetup lorakey lmsg ltop luser lnetmgr langcomp makeinst install usered fileidx

lora:      lora.exe
loraovl:   lora.ovl
lsetup:    lsetup.exe
lorakey:   lorakey.exe
lmsg:      lmsg.exe
ltop:      ltop.exe
luser:     luser.exe
lnetmgr:   lnetmgr.exe
langcomp:  langcomp.exe
makeinst:  makeinst.exe
install:   install.exe
usered:    usered.exe
fileidx:   fileidx.exe

EXE_LSETUP_dependencies = \
  $(OBJ)\lsetup.obj \
  $(OBJ)\lsetup2.obj \
  $(OBJ)\lsetup3.obj \
  $(OBJ)\lsetup4.obj \
  $(OBJ)\lsetup5.obj \
  $(OBJ)\lsetup6.obj \
  $(OBJ)\lsetup7.obj

EXE_dependencies =  \
  $(CXLIB)\cxltcl.lib \
  squish-l.lib \
  $(OBJ)\areafix.obj \
  $(OBJ)\areafix2.obj \
  $(OBJ)\bink_asm.obj \
  $(OBJ)\checkpat.obj \
  $(OBJ)\com_asm.obj \
  $(OBJ)\spawn.obj \
  $(OBJ)\change.obj \
  $(OBJ)\chat.obj \
  $(OBJ)\cmdfile.obj \
  $(OBJ)\colors.obj \
  $(OBJ)\config.obj \
  $(OBJ)\cpu.obj \
  $(OBJ)\data.obj \
  $(OBJ)\door.obj \
  $(OBJ)\editor.obj \
  $(OBJ)\editor1.obj \
  $(OBJ)\exec.obj \
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

EXE_START_dependencies = \
  $(OBJ)\startup.obj

EXE_KEY_dependencies = \
  $(OBJ)\lorakey.obj

EXE_LMSG_dependencies = \
  squish-l.lib \
  $(OBJ)\lmsg.obj

EXE_LTOP_dependencies = \
  $(OBJ)\ltop.obj

EXE_LUSER_dependencies = \
  $(OBJ)\luser.obj

EXE_NET_dependencies = \
  $(OBJ)\bink_asm.obj \
  $(OBJ)\dostime.obj \
  $(OBJ)\lnetmgr.obj

EXE_COMP_dependencies = \
  $(OBJ)\loralng.obj \
  $(OBJ)\get_lang.obj \
  $(OBJ)\put_lang.obj

EXE_MKINST_dependencies = \
  $(OBJ)\makeinst.obj

EXE_INST_dependencies = \
  $(OBJ)\install.obj

EXE_UED_dependencies = \
  $(OBJ)\usered.obj

EXE_IDX_dependencies = \
  $(OBJ)\fileidx.obj

#               *Explicit Rules*

lsetup.exe: turboc.cfg $(EXE_LSETUP_dependencies)
  $(TLINK) @&&|
$(TCLIB)\c0l.obj+
/o- $(OBJ)\lsetup.obj+
    $(OBJ)\lsetup2.obj+
/o+ $(OBJ)\lsetup3.obj+
    $(OBJ)\lsetup4.obj+
    $(OBJ)\lsetup5.obj+
    $(OBJ)\lsetup6.obj+
    $(OBJ)\lsetup7.obj
lsetup,
$(TCLIB)\overlay.lib+
$(CXLIB)\cxltcl.lib+
$(TCLIB)\emu.lib+
$(TCLIB)\mathl.lib+
$(TCLIB)\cl.lib+
|

lora.ovl: turboc.cfg $(EXE_dependencies)
  $(TLINK) /o @&&|
/o- $(TCLIB)\c0l.obj+
    $(OBJ)\bink_asm.obj+
    $(OBJ)\checkpat.obj+
    $(OBJ)\com_asm.obj+
    $(OBJ)\spawn.obj+
    $(OBJ)\gestint.obj+
    $(OBJ)\colors.obj+
    $(OBJ)\lora.obj+
    $(OBJ)\misc.obj+
    $(OBJ)\modem.obj+
    $(OBJ)\sched.obj+
    $(OBJ)\status.obj+
    $(OBJ)\timer.obj+
    $(OBJ)\vdisk.obj+
/o+ $(OBJ)\areafix.obj+
    $(OBJ)\areafix2.obj+
    $(OBJ)\change.obj+
    $(OBJ)\chat.obj+
    $(OBJ)\cmdfile.obj+
    $(OBJ)\config.obj+
    $(OBJ)\cpu.obj+
    $(OBJ)\data.obj+
    $(OBJ)\door.obj +
    $(OBJ)\editor.obj+
    $(OBJ)\editor1.obj+
    $(OBJ)\exec.obj+
    $(OBJ)\export.obj+
    $(OBJ)\files.obj+
    $(OBJ)\files2.obj+
    $(OBJ)\ftsc.obj+
    $(OBJ)\fulled.obj+
    $(OBJ)\general.obj+
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
lora.ovl,
$(TCLIB)\overlay.lib+
$(TCLIB)\emu.lib+
$(TCLIB)\mathl.lib+
$(TCLIB)\cl.lib+
$(CXLIB)\cxltcl.lib+
squish-l.lib
|

lora.exe: turboc.cfg $(EXE_START_dependencies)
  $(TLINK) @&&|
$(TCLIB)\c0l.obj+
$(OBJ)\startup.obj
lora,
$(CXLIB)\cxltcl.lib+
$(TCLIB)\emu.lib+
$(TCLIB)\mathl.lib+
$(TCLIB)\cl.lib+
|

lorakey.exe: turboc.cfg $(EXE_KEY_dependencies)
  $(TLINK) @&&|
$(TCLIB)\c0l.obj+
$(OBJ)\lorakey.obj
lorakey,
$(CXLIB)\cxltcl.lib+
$(TCLIB)\emu.lib+
$(TCLIB)\mathl.lib+
$(TCLIB)\cl.lib+
|

lmsg.exe: turboc.cfg $(EXE_LMSG_dependencies)
  $(TLINK) @&&|
$(TCLIB)\c0l.obj+
$(OBJ)\lmsg.obj
lmsg,
$(TCLIB)\emu.lib+
$(TCLIB)\mathl.lib+
$(TCLIB)\cl.lib+
squish-l.lib
|

ltop.exe: turboc.cfg $(EXE_LTOP_dependencies)
  $(TLINK) @&&|
$(TCLIB)\c0l.obj+
$(OBJ)\ltop.obj
ltop,
$(TCLIB)\emu.lib+
$(TCLIB)\mathl.lib+
$(TCLIB)\cl.lib+
|

luser.exe: turboc.cfg $(EXE_LUSER_dependencies)
  $(TLINK) @&&|
$(TCLIB)\c0l.obj+
$(OBJ)\luser.obj
luser,
$(TCLIB)\emu.lib+
$(TCLIB)\mathl.lib+
$(TCLIB)\cl.lib+
|

lnetmgr.exe: turboc.cfg $(EXE_NET_dependencies)
  $(TLINK) @&&|
$(TCLIB)\c0l.obj+
$(OBJ)\bink_asm.obj+
$(OBJ)\dostime.obj+
$(OBJ)\lnetmgr.obj
lnetmgr,
$(CXLIB)\cxltcl.lib+
$(TCLIB)\emu.lib+
$(TCLIB)\mathl.lib+
$(TCLIB)\cl.lib+
|

langcomp.exe: turboc.cfg $(EXE_COMP_dependencies)
  $(TLINK) @&&|
$(TCLIB)\c0l.obj+
$(OBJ)\loralng.obj+
$(OBJ)\get_lang.obj+
$(OBJ)\put_lang.obj
langcomp,
$(TCLIB)\emu.lib+
$(TCLIB)\mathl.lib+
$(TCLIB)\cl.lib+
|

makeinst.exe: turboc.cfg $(EXE_MKINST_dependencies)
  $(TLINK) @&&|
$(TCLIB)\c0l.obj+
$(OBJ)\makeinst.obj
makeinst,
implode.lib+
$(TCLIB)\emu.lib+
$(TCLIB)\mathl.lib+
$(TCLIB)\cl.lib
|

install.exe: turboc.cfg $(EXE_INST_dependencies)
  $(TLINK) @&&|
$(TCLIB)\c0l.obj+
$(OBJ)\install.obj
install,
implode.lib+
$(CXLIB)\cxltcl.lib+
$(TCLIB)\emu.lib+
$(TCLIB)\mathl.lib+
$(TCLIB)\cl.lib
|

usered.exe: turboc.cfg $(EXE_UED_dependencies)
  $(TLINK) @&&|
$(TCLIB)\c0l.obj+
$(OBJ)\usered.obj
usered,
$(CXLIB)\cxltcl.lib+
$(TCLIB)\emu.lib+
$(TCLIB)\mathl.lib+
$(TCLIB)\cl.lib
|

fileidx.exe: turboc.cfg $(EXE_IDX_dependencies)
  $(TLINK) @&&|
$(TCLIB)\c0l.obj+
$(OBJ)\fileidx.obj
fileidx,
$(TCLIB)\emu.lib+
$(TCLIB)\mathl.lib+
$(TCLIB)\cl.lib
|

#               *Individual File Dependencies*

$(OBJ)\lsetup.obj: $(SRC)\lsetup.c
        $(CC) -n$(OBJ) $(SRC)\lsetup.c

$(OBJ)\lsetup2.obj: $(SRC)\lsetup2.c
        $(CC) -n$(OBJ) $(SRC)\lsetup2.c

$(OBJ)\lsetup3.obj: $(SRC)\lsetup3.c
        $(CC) -Yo -n$(OBJ) $(SRC)\lsetup3.c

$(OBJ)\lsetup4.obj: $(SRC)\lsetup4.c
        $(CC) -Yo -n$(OBJ) $(SRC)\lsetup4.c

$(OBJ)\lsetup5.obj: $(SRC)\lsetup5.c
        $(CC) -Yo -n$(OBJ) $(SRC)\lsetup5.c

$(OBJ)\lsetup6.obj: $(SRC)\lsetup6.c
        $(CC) -Yo -n$(OBJ) $(SRC)\lsetup6.c

$(OBJ)\lsetup7.obj: $(SRC)\lsetup7.c
        $(CC) -Yo -n$(OBJ) $(SRC)\lsetup7.c



$(OBJ)\bink_asm.obj: $(SRC)\bink_asm.asm
        $(TASM) /DMODL=LARGE $(SRC)\bink_asm.asm, $(OBJ)\bink_asm

$(OBJ)\checkpat.obj: $(SRC)\checkpat.asm
        $(TASM) /DMODL=LARGE $(SRC)\checkpat.asm, $(OBJ)\checkpat

$(OBJ)\com_asm.obj: $(SRC)\com_asm.asm
        $(TASM) /DMODL=LARGE $(SRC)\com_asm.asm, $(OBJ)\com_asm

$(OBJ)\dostime.obj: $(SRC)\dostime.asm
        $(TASM) /DMODL=LARGE $(SRC)\dostime.asm, $(OBJ)\dostime

$(OBJ)\spawn.obj: $(SRC)\spawn.asm
        $(TASM) /DMODL=LARGE $(SRC)\spawn.asm, $(OBJ)\spawn

$(OBJ)\areafix.obj: $(SRC)\areafix.c
        $(CC) -n$(OBJ) $(SRC)\areafix.c

$(OBJ)\areafix2.obj: $(SRC)\areafix2.c
        $(CC) -n$(OBJ) $(SRC)\areafix2.c

$(OBJ)\change.obj: $(SRC)\change.c
        $(CC) -n$(OBJ) $(SRC)\change.c

$(OBJ)\chat.obj: $(SRC)\chat.c
        $(CC) -n$(OBJ) $(SRC)\chat.c

$(OBJ)\cmdfile.obj: $(SRC)\cmdfile.c
        $(CC) -n$(OBJ) $(SRC)\cmdfile.c

$(OBJ)\colors.obj: $(SRC)\colors.c
        $(CC) -n$(OBJ) $(SRC)\colors.c

$(OBJ)\config.obj: $(SRC)\config.c
        $(CC) -n$(OBJ) $(SRC)\config.c

$(OBJ)\cpu.obj: $(SRC)\cpu.c
        $(CC) -n$(OBJ) $(SRC)\cpu.c

$(OBJ)\data.obj: $(SRC)\data.c
        $(CC) -n$(OBJ) $(SRC)\data.c

$(OBJ)\door.obj: $(SRC)\door.c
        $(CC) -n$(OBJ) $(SRC)\door.c

$(OBJ)\editor.obj: $(SRC)\editor.c
        $(CC) -n$(OBJ) $(SRC)\editor.c

$(OBJ)\editor1.obj: $(SRC)\editor1.c
        $(CC) -n$(OBJ) $(SRC)\editor1.c

$(OBJ)\exec.obj: $(SRC)\exec.c
        $(CC) -n$(OBJ) $(SRC)\exec.c

$(OBJ)\export.obj: $(SRC)\export.c
        $(CC) -n$(OBJ) $(SRC)\export.c

$(OBJ)\files.obj: $(SRC)\files.c
        $(CC) -n$(OBJ) $(SRC)\files.c

$(OBJ)\files2.obj: $(SRC)\files2.c
        $(CC) -n$(OBJ) $(SRC)\files2.c

$(OBJ)\ftsc.obj: $(SRC)\ftsc.c
        $(CC) -n$(OBJ) $(SRC)\ftsc.c

$(OBJ)\fulled.obj: $(SRC)\fulled.c
        $(CC) -n$(OBJ) $(SRC)\fulled.c

$(OBJ)\general.obj: $(SRC)\general.c
        $(CC) -n$(OBJ) $(SRC)\general.c

$(OBJ)\gestint.obj: $(SRC)\gestint.c
        $(CC) -n$(OBJ) -N- -Z- $(SRC)\gestint.c

$(OBJ)\import.obj: $(SRC)\import.c
        $(CC) -n$(OBJ) $(SRC)\import.c

$(OBJ)\import2.obj: $(SRC)\import2.c
        $(CC) -n$(OBJ) $(SRC)\import2.c

$(OBJ)\input.obj: $(SRC)\input.c
        $(CC) -n$(OBJ) $(SRC)\input.c

$(OBJ)\janus.obj: $(SRC)\janus.c
        $(CC) -n$(OBJ) $(SRC)\janus.c

$(OBJ)\langload.obj: $(SRC)\langload.c
        $(CC) -n$(OBJ) $(SRC)\langload.c

$(OBJ)\login.obj: $(SRC)\login.c
        $(CC) -n$(OBJ) $(SRC)\login.c

$(OBJ)\lora.obj: $(SRC)\lora.c
        $(CC) -n$(OBJ) $(SRC)\lora.c

$(OBJ)\lora_ovl.obj: $(SRC)\lora_ovl.c
        $(CC) -n$(OBJ) $(SRC)\lora_ovl.c

$(OBJ)\lrpn.obj: $(SRC)\lrpn.c
        $(CC) -n$(OBJ) $(SRC)\lrpn.c

$(OBJ)\maintask.obj: $(SRC)\maintask.c
        $(CC) -n$(OBJ) $(SRC)\maintask.c

$(OBJ)\menu.obj: $(SRC)\menu.c
        $(CC) -n$(OBJ) $(SRC)\menu.c

$(OBJ)\message.obj: $(SRC)\message.c
        $(CC) -n$(OBJ) $(SRC)\message.c

$(OBJ)\message1.obj: $(SRC)\message1.c
        $(CC) -n$(OBJ) $(SRC)\message1.c

$(OBJ)\misc.obj: $(SRC)\misc.c
        $(CC) -n$(OBJ) $(SRC)\misc.c

$(OBJ)\modem.obj: $(SRC)\modem.c
        $(CC) -n$(OBJ) $(SRC)\modem.c

$(OBJ)\ovrmisc.obj: $(SRC)\ovrmisc.c
        $(CC) -n$(OBJ) $(SRC)\ovrmisc.c

$(OBJ)\pack.obj: $(SRC)\pack.c
        $(CC) -n$(OBJ) $(SRC)\pack.c

$(OBJ)\pipbase.obj: $(SRC)\pipbase.c
        $(CC) -n$(OBJ) $(SRC)\pipbase.c

$(OBJ)\pipbase2.obj: $(SRC)\pipbase2.c
        $(CC) -n$(OBJ) $(SRC)\pipbase2.c

$(OBJ)\poller.obj: $(SRC)\poller.c
        $(CC) -n$(OBJ) $(SRC)\poller.c

$(OBJ)\quickmsg.obj: $(SRC)\quickmsg.c
        $(CC) -n$(OBJ) $(SRC)\quickmsg.c

$(OBJ)\quickms2.obj: $(SRC)\quickms2.c
        $(CC) -n$(OBJ) $(SRC)\quickms2.c

$(OBJ)\qwkspt.obj: $(SRC)\qwkspt.c
        $(CC) -n$(OBJ) $(SRC)\qwkspt.c

$(OBJ)\reader.obj: $(SRC)\reader.c
        $(CC) -n$(OBJ) $(SRC)\reader.c

$(OBJ)\readmail.obj: $(SRC)\readmail.c
        $(CC) -n$(OBJ) $(SRC)\readmail.c

$(OBJ)\sched.obj: $(SRC)\sched.c
        $(CC) -n$(OBJ) $(SRC)\sched.c

$(OBJ)\script.obj: $(SRC)\script.c
        $(CC) -n$(OBJ) $(SRC)\script.c

$(OBJ)\squish.obj: $(SRC)\squish.c
        $(CC) -n$(OBJ) $(SRC)\squish.c

$(OBJ)\status.obj: $(SRC)\status.c
        $(CC) -n$(OBJ) $(SRC)\status.c

$(OBJ)\terminal.obj: $(SRC)\terminal.c
        $(CC) -n$(OBJ) $(SRC)\terminal.c

$(OBJ)\timer.obj: $(SRC)\timer.c
        $(CC) -n$(OBJ) $(SRC)\timer.c

$(OBJ)\utility.obj: $(SRC)\utility.c
        $(CC) -n$(OBJ) $(SRC)\utility.c

$(OBJ)\video.obj: $(SRC)\video.c
        $(CC) -n$(OBJ) $(SRC)\video.c

$(OBJ)\vdisk.obj: $(SRC)\vdisk.c
        $(CC) -n$(OBJ) $(SRC)\vdisk.c

$(OBJ)\wazoo.obj: $(SRC)\wazoo.c
        $(CC) -n$(OBJ) $(SRC)\wazoo.c

$(OBJ)\yoohoo.obj: $(SRC)\yoohoo.c
        $(CC) -n$(OBJ) $(SRC)\yoohoo.c

$(OBJ)\zmisc.obj: $(SRC)\zmisc.c
        $(CC) -n$(OBJ) $(SRC)\zmisc.c

$(OBJ)\zsend.obj: $(SRC)\zsend.c
        $(CC) -n$(OBJ) $(SRC)\zsend.c

$(OBJ)\zreceive.obj: $(SRC)\zreceive.c
        $(CC) -n$(OBJ) $(SRC)\zreceive.c



$(OBJ)\startup.obj: $(SRC)\startup.c
        $(CC) -n$(OBJ) -N- -Z- $(SRC)\startup.c



$(OBJ)\lorakey.obj: $(SRC)\lorakey.c
        $(CC) -n$(OBJ) $(SRC)\lorakey.c



$(OBJ)\lmsg.obj: $(SRC)\lmsg.c
        $(CC) -n$(OBJ) $(SRC)\lmsg.c



$(OBJ)\ltop.obj: $(SRC)\ltop.c
        $(CC) -n$(OBJ) $(SRC)\ltop.c



$(OBJ)\luser.obj: $(SRC)\luser.c
        $(CC) -n$(OBJ) $(SRC)\luser.c



$(OBJ)\lnetmgr.obj: $(SRC)\lnetmgr.c
        $(CC) -n$(OBJ) $(SRC)\lnetmgr.c



$(OBJ)\loralng.obj: $(SRC)\loralng.c
        $(CC) -n$(OBJ) $(SRC)\loralng.c

$(OBJ)\put_lang.obj: $(SRC)\put_lang.c
        $(CC) -n$(OBJ) $(SRC)\put_lang.c

$(OBJ)\get_lang.obj: $(SRC)\get_lang.c
        $(CC) -n$(OBJ) $(SRC)\get_lang.c



$(OBJ)\makeinst.obj: $(SRC)\makeinst.c
        $(CC) -n$(OBJ) $(SRC)\makeinst.c



$(OBJ)\install.obj: $(SRC)\install.c
        $(CC) -n$(OBJ) $(SRC)\install.c



$(OBJ)\usered.obj: $(SRC)\usered.c
        $(CC) -n$(OBJ) $(SRC)\usered.c



$(OBJ)\fileidx.obj: $(SRC)\fileidx.c
        $(CC) -n$(OBJ) $(SRC)\fileidx.c


#               *Compiler Configuration File*
turboc.cfg: makefile
  copy &&|
-ml
-C
-N
-O
-Ol
-d
-Y
-h
-vi-
-wbbf
-wamb
-wamp
-wasm
-wpro
-wcln
-wdef
-wsig
-wnod
-wstv
-wuse
-weas
-wpre
-w-ccc
-I$(INCLUDEPATH)
-L$(LIBPATH)
| turboc.cfg

