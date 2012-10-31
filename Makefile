.AUTODEPEND

OBJ = OBJ
SRC = SRC
UTI = $(SRC)\UTIL
TCLIB = C:\BORLANDC\LIB
CXLIB = $(TCLIB)\CXL

#		*Translator Definitions*
CC = bcc +TURBOC.CFG
TASM = TASM /MX /ZD
TLINK = tlink /P-/x


#		*Implicit Rules*
.c.obj:
  $(CC) -c -n$(OBJ) {$< }

.cpp.obj:
  $(CC) -c -n$(OBJ) {$< }

.asm.obj:
  $(TASM) {$< }


#		*List Macros*

all: lora loracomp lorakey tym2lora usered lmsg ltop luser

lora:      lora.exe
loracomp:  loracomp.exe
lorakey:   lorakey.exe
tym2lora:  tym2lora.exe
usered:    usered.exe
lmsg:      lmsg.exe
ltop:      ltop.exe
luser:     luser.exe


EXE_dependencies =  \
  $(CXLIB)\cxltcm.lib \
  squish-m.lib \
  $(OBJ)\bink_asm.obj \
  $(OBJ)\checkpath.obj \
  $(OBJ)\com_asm.obj \
  $(OBJ)\dostime.obj \
  $(OBJ)\spawn.obj \
  $(OBJ)\change.obj \
  $(OBJ)\chat.obj \
  $(OBJ)\cmdfile.obj \
  $(OBJ)\colors.obj \
  $(OBJ)\config.obj \
  $(OBJ)\data.obj \
  $(OBJ)\door.obj \
  $(OBJ)\editor.obj \
  $(OBJ)\editor1.obj \
  $(OBJ)\exec.obj \
  $(OBJ)\files.obj \
  $(OBJ)\files2.obj \
  $(OBJ)\ftsc.obj \
  $(OBJ)\general.obj \
  $(OBJ)\input.obj \
  $(OBJ)\janus.obj \
  $(OBJ)\langload.obj \
  $(OBJ)\login.obj \
  $(OBJ)\lora.obj \
  $(OBJ)\maintask.obj \
  $(OBJ)\menu.obj \
  $(OBJ)\message.obj\
  $(OBJ)\message1.obj\
  $(OBJ)\misc.obj \
  $(OBJ)\modem.obj \
  $(OBJ)\ovrmisc.obj \
  $(OBJ)\pipbase.obj \
  $(OBJ)\poller.obj \
  $(OBJ)\quickmsg.obj \
  $(OBJ)\qwkspt.obj \
  $(OBJ)\readmail.obj \
  $(OBJ)\sched.obj \
  $(OBJ)\squish.obj \
  $(OBJ)\status.obj \
  $(OBJ)\tcfuncs.obj \
  $(OBJ)\timer.obj \
  $(OBJ)\video.obj \
  $(OBJ)\wazoo.obj \
  $(OBJ)\yoohoo.obj \
  $(OBJ)\zmisc.obj \
  $(OBJ)\zsend.obj \
  $(OBJ)\zreceive.obj


EXE_COMP_dependencies = \
  $(OBJ)\loracomp.obj \
  $(OBJ)\loralng.obj \
  $(OBJ)\get_lang.obj \
  $(OBJ)\put_lang.obj


EXE_KEY_dependencies = \
  $(OBJ)\lorakey.obj


EXE_TYM_dependencies = \
  $(OBJ)\tym2lora.obj


EXE_USER_dependencies = \
  $(OBJ)\usered.obj


EXE_LMSG_dependencies = \
  squish-l.lib \
  $(OBJ)\lmsg.obj


EXE_LTOP_dependencies = \
  $(OBJ)\ltop.obj


EXE_LUSER_dependencies = \
  $(OBJ)\luser.obj


#               *Explicit Rules*

lora.exe: turboc.cfg $(EXE_dependencies)
  $(TLINK) /o @&&|
/o- $(TCLIB)\c0m.obj+
    $(OBJ)\bink_asm.obj+
    $(OBJ)\checkpath.obj+
    $(OBJ)\com_asm.obj+
    $(OBJ)\dostime.obj+
    $(OBJ)\spawn.obj+
    $(OBJ)\colors.obj+
    $(OBJ)\data.obj+
    $(OBJ)\lora.obj+
    $(OBJ)\misc.obj+
    $(OBJ)\modem.obj+
    $(OBJ)\sched.obj+
    $(OBJ)\status.obj+
    $(OBJ)\tcfuncs.obj+
    $(OBJ)\timer.obj+
/o+ $(OBJ)\change.obj+
    $(OBJ)\chat.obj+
    $(OBJ)\cmdfile.obj+
    $(OBJ)\config.obj+
    $(OBJ)\door.obj +
    $(OBJ)\editor.obj+
    $(OBJ)\editor1.obj+
    $(OBJ)\exec.obj+
    $(OBJ)\files.obj+
    $(OBJ)\files2.obj+
    $(OBJ)\ftsc.obj+
    $(OBJ)\general.obj+
    $(OBJ)\input.obj+
    $(OBJ)\janus.obj+
    $(OBJ)\langload.obj+
    $(OBJ)\login.obj+
    $(OBJ)\maintask.obj+
    $(OBJ)\menu.obj+
    $(OBJ)\message.obj+
    $(OBJ)\message1.obj+
    $(OBJ)\ovrmisc.obj+
    $(OBJ)\pipbase.obj+
    $(OBJ)\poller.obj+
    $(OBJ)\quickmsg.obj+
    $(OBJ)\qwkspt.obj+
    $(OBJ)\readmail.obj+
    $(OBJ)\squish.obj+
    $(OBJ)\video.obj+
    $(OBJ)\wazoo.obj+
    $(OBJ)\yoohoo.obj+
    $(OBJ)\zmisc.obj+
    $(OBJ)\zsend.obj+
    $(OBJ)\zreceive.obj
lora,
$(TCLIB)\overlay.lib+
$(TCLIB)\emu.lib+
$(TCLIB)\mathm.lib+
$(TCLIB)\cm.lib+
$(CXLIB)\cxltcm.lib+
squish-m.lib
|


loracomp.exe: turboc.cfg $(EXE_COMP_dependencies)
  $(TLINK) @&&|
$(TCLIB)\c0m.obj+
$(OBJ)\loracomp.obj+
$(OBJ)\loralng.obj+
$(OBJ)\get_lang.obj+
$(OBJ)\put_lang.obj
loracomp,
$(CXLIB)\cxltcm.lib+
$(TCLIB)\emu.lib+
$(TCLIB)\mathm.lib+
$(TCLIB)\cm.lib+

|


lorakey.exe: turboc.cfg $(EXE_KEY_dependencies)
  $(TLINK) @&&|
$(TCLIB)\c0s.obj+
$(OBJ)\lorakey.obj
lorakey,
$(CXLIB)\cxltcs.lib+
$(TCLIB)\emu.lib+
$(TCLIB)\maths.lib+
$(TCLIB)\cs.lib+

|



tym2lora.exe: turboc.cfg $(EXE_TYM_dependencies)
  $(TLINK) @&&|
$(TCLIB)\c0s.obj+
$(OBJ)\tym2lora.obj
tym2lora,
$(TCLIB)\emu.lib+
$(TCLIB)\maths.lib+
$(TCLIB)\cs.lib+

|



usered.exe: turboc.cfg $(EXE_USER_dependencies)
  $(TLINK) @&&|
$(TCLIB)\c0s.obj+
$(OBJ)\usered.obj
usered,
$(CXLIB)\cxltcs.lib+
$(TCLIB)\emu.lib+
$(TCLIB)\maths.lib+
$(TCLIB)\cs.lib+

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
$(TCLIB)\c0s.obj+
$(OBJ)\ltop.obj
ltop,
$(TCLIB)\emu.lib+
$(TCLIB)\maths.lib+
$(TCLIB)\cs.lib+

|



luser.exe: turboc.cfg $(EXE_LUSER_dependencies)
  $(TLINK) @&&|
$(TCLIB)\c0s.obj+
$(OBJ)\luser.obj
luser,
$(TCLIB)\emu.lib+
$(TCLIB)\maths.lib+
$(TCLIB)\cs.lib+

|



#               *Individual File Dependencies*

$(OBJ)\bink_asm.obj: $(SRC)\bink_asm.asm
        $(TASM) $(SRC)\bink_asm.asm, $(OBJ)\bink_asm

$(OBJ)\checkpath.obj: $(SRC)\checkpath.asm
        $(TASM) /DMODL=medium $(SRC)\checkpath.asm, $(OBJ)\checkpath

$(OBJ)\com_asm.obj: $(SRC)\com_asm.asm
        $(TASM) $(SRC)\com_asm.asm, $(OBJ)\com_asm

$(OBJ)\dostime.obj: $(SRC)\dostime.asm
        $(TASM) $(SRC)\dostime.asm, $(OBJ)\dostime

$(OBJ)\spawn.obj: $(SRC)\spawn.asm
        $(TASM) /DMODL=medium $(SRC)\spawn.asm, $(OBJ)\spawn

$(OBJ)\change.obj: $(SRC)\change.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\change.c

$(OBJ)\chat.obj: $(SRC)\chat.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\chat.c

$(OBJ)\cmdfile.obj: $(SRC)\cmdfile.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\cmdfile.c

$(OBJ)\colors.obj: $(SRC)\colors.c
        $(CC) -n$(OBJ) -mm -c $(SRC)\colors.c

$(OBJ)\config.obj: $(SRC)\config.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\config.c

$(OBJ)\data.obj: $(SRC)\data.c
        $(CC) -n$(OBJ) -mm -c $(SRC)\data.c

$(OBJ)\door.obj: $(SRC)\door.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\door.c

$(OBJ)\editor.obj: $(SRC)\editor.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\editor.c

$(OBJ)\editor1.obj: $(SRC)\editor1.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\editor1.c

$(OBJ)\exec.obj: $(SRC)\exec.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\exec.c

$(OBJ)\files.obj: $(SRC)\files.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\files.c

$(OBJ)\files2.obj: $(SRC)\files2.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\files2.c

$(OBJ)\ftsc.obj: $(SRC)\ftsc.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\ftsc.c

$(OBJ)\general.obj: $(SRC)\general.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\general.c

$(OBJ)\input.obj: $(SRC)\input.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\input.c

$(OBJ)\janus.obj: $(SRC)\janus.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\janus.c

$(OBJ)\langload.obj: $(SRC)\langload.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\langload.c

$(OBJ)\login.obj: $(SRC)\login.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\login.c

$(OBJ)\lora.obj: $(SRC)\lora.c
        $(CC) -n$(OBJ) -mm -c $(SRC)\lora.c

$(OBJ)\maintask.obj: $(SRC)\maintask.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\maintask.c

$(OBJ)\menu.obj: $(SRC)\menu.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\menu.c

$(OBJ)\message.obj: $(SRC)\message.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\message.c

$(OBJ)\message1.obj: $(SRC)\message1.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\message1.c

$(OBJ)\misc.obj: $(SRC)\misc.c
        $(CC) -n$(OBJ) -mm -c $(SRC)\misc.c

$(OBJ)\modem.obj: $(SRC)\modem.c
        $(CC) -n$(OBJ) -mm -c $(SRC)\modem.c

$(OBJ)\ovrmisc.obj: $(SRC)\ovrmisc.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\ovrmisc.c

$(OBJ)\pipbase.obj: $(SRC)\pipbase.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\pipbase.c

$(OBJ)\poller.obj: $(SRC)\poller.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\poller.c

$(OBJ)\quickmsg.obj: $(SRC)\quickmsg.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\quickmsg.c

$(OBJ)\qwkspt.obj: $(SRC)\qwkspt.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\qwkspt.c

$(OBJ)\readmail.obj: $(SRC)\readmail.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\readmail.c

$(OBJ)\sched.obj: $(SRC)\sched.c
        $(CC) -n$(OBJ) -mm -c $(SRC)\sched.c

$(OBJ)\squish.obj: $(SRC)\squish.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\squish.c

$(OBJ)\status.obj: $(SRC)\status.c
        $(CC) -n$(OBJ) -mm -c $(SRC)\status.c

$(OBJ)\tcfuncs.obj: $(SRC)\tcfuncs.c
        $(CC) -n$(OBJ) -mm -c $(SRC)\tcfuncs.c

$(OBJ)\timer.obj: $(SRC)\timer.c
        $(CC) -n$(OBJ) -mm -c $(SRC)\timer.c

$(OBJ)\video.obj: $(SRC)\video.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\video.c

$(OBJ)\wazoo.obj: $(SRC)\wazoo.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\wazoo.c

$(OBJ)\yoohoo.obj: $(SRC)\yoohoo.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\yoohoo.c

$(OBJ)\zmisc.obj: $(SRC)\zmisc.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\zmisc.c

$(OBJ)\zsend.obj: $(SRC)\zsend.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\zsend.c

$(OBJ)\zreceive.obj: $(SRC)\zreceive.c
        $(CC) -Y -n$(OBJ) -mm -c $(SRC)\zreceive.c





$(OBJ)\loracomp.obj: $(SRC)\loracomp.c
        $(CC) -n$(OBJ) -mm -c $(SRC)\loracomp.c

$(OBJ)\loralng.obj: $(SRC)\loralng.c
        $(CC) -n$(OBJ) -mm -c $(SRC)\loralng.c

$(OBJ)\put_lang.obj: $(SRC)\put_lang.c
        $(CC) -n$(OBJ) -mm -c $(SRC)\put_lang.c

$(OBJ)\get_lang.obj: $(SRC)\get_lang.c
        $(CC) -n$(OBJ) -mm -c $(SRC)\get_lang.c




$(OBJ)\lorakey.obj: $(SRC)\lorakey.c
        $(CC) -n$(OBJ) -c -ms $(SRC)\lorakey.c




$(OBJ)\tym2lora.obj: $(SRC)\tym2lora.c
        $(CC) -n$(OBJ) -c -ms $(SRC)\tym2lora.c



$(OBJ)\usered.obj: $(SRC)\usered.c
        $(CC) -n$(OBJ) -c -ms $(SRC)\usered.c




$(OBJ)\lmsg.obj: $(SRC)\lmsg.c
        $(CC) -n$(OBJ) -c -ml $(SRC)\lmsg.c




$(OBJ)\ltop.obj: $(SRC)\ltop.c
        $(CC) -n$(OBJ) -c -ms $(SRC)\ltop.c




$(OBJ)\luser.obj: $(SRC)\luser.c
        $(CC) -n$(OBJ) -c -ms $(SRC)\luser.c




#               *Compiler Configuration File*
turboc.cfg: makefile
  copy &&|
-N
-G
-O
-Z
-ff-
-d
-V
-vi-
-wbbf
-wpin
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
-IC:\BORLANDC\INCLUDE
-LC:\BORLANDC\LIB
-j100
| turboc.cfg

