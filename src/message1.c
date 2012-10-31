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


