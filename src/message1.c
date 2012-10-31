
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
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <dir.h>
#include <io.h>
#include <fcntl.h>
#include <ctype.h>

#include <cxl\cxlstr.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

char *firstchar(char *, char *, int);

void gather_origin_netnode (s)
char *s;
{
   int zone, net, node, point;
   char *p;

   p = strchr(s, '\0');

   while (p != s && !isdigit (*p))
      p--;

   while (p > s)
   {
      if (!isdigit(*p) && *p != '.' && *p != ':' && *p != '/')
         break;
      p--;
   }

   if (p == s)
      return;

   p++;
   parse_netnode (p, &zone, &net, &node, &point);

   msg_fzone = zone;
   msg.orig_net = net;
   msg.orig = node;
   msg_fpoint = point;
}


