#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <dir.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <dos.h>
#include <ctype.h>
#include <stdarg.h>
#include <alloc.h>
#include <process.h>
#include <signal.h>
#include <sys/stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>
#include <cxl\cxlkey.h>

#include "defines.h"
#include "lora.h"
#ifndef NO_EXTERNS
#include "externs.h"
#else
#include "sched.h"
#endif
#include "prototyp.h"
#include "zmodem.h"
#include "quickmsg.h"
#include "arc.h"
#include "exec.h"
#include "janus.h"
#include "tc_utime.h"
#include "msgapi.h"
#include "pipbase.h"
