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
#include <io.h>
#include <fcntl.h>
#include <time.h>

#include <cxl\cxlvid.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

void update_last_caller()
{
    int fd, fdb;
    char filename[80], backup[80];
    long stamp;
    struct _lastcall lc;

    stamp = time(NULL);
    memset((char *)&lastcall, 0, sizeof(struct _lastcall));

    sprintf(backup, "%sLASTCALL.BAK", config.sys_path);
    sprintf(filename, "%sLASTCALL.BBS", config.sys_path);
    unlink(backup);
    rename(filename, backup);

    fdb = shopen(backup, O_RDONLY | O_BINARY);
    if (fdb == -1) {
        return;
    }

    fd = cshopen(filename, O_RDONLY | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);

    while (read(fdo, (char *)&lc, sizeof(struct _lastcall)) == sizeof(struct _lastcall)) {
        if (stamp - lc.timestamp < 86400L) {
            write(fd, (char *)&lc, sizeof(struct _lastcall));
        }
    }

    close(fd);
    close(fdo);
}

