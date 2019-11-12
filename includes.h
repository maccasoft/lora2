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
