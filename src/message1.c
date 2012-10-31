#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <dir.h>
#include <io.h>
#include <fcntl.h>
#include <ctype.h>

#include <cxl\cxlstr.h>

#include "defines.h"
#include "lora.h"
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

void parse_netnode(netnode, zone, net, node, point)
char *netnode;
int *zone, *net, *node, *point;
{
   char *p;

   *zone = alias[0].zone;
   *net = alias[0].net;
   *node = 0;
   *point = 0;

   p = netnode;

   /* If we have a zone (and the caller wants the zone to be passed back).. */

   if (strchr(netnode,':') && zone) {
      *zone=atoi(p);
      p = firstchar(p,":",2);
   }

   /* If we have a net number... */

   if (p && strchr(netnode,'/') && net) {
      *net=atoi(p);
      p=firstchar(p,"/",2);
   }

   /* We *always* need a node number... */

   if (p && node)
      *node=atoi(p);

   /* And finally check for a point number... */

   if (p && strchr(netnode,'.') && point) {
      p=firstchar(p,".",2);

      if (p)
         *point=atoi(p);
      else
         *point=0;
   }
}


