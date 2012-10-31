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

   stamp = time (NULL);
   memset ((char *)&lastcall, 0, sizeof (struct _lastcall));

   sprintf (backup, "%sLASTCALL.BAK", config.sys_path);
   sprintf (filename, "%sLASTCALL.BBS", config.sys_path);
   unlink (backup);
   rename (filename, backup);

   fdb = shopen (backup, O_RDONLY|O_BINARY);
   if (fdb == -1)
      return;

   fd = cshopen (filename, O_RDONLY|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);

   while (read(fdo, (char *)&lc, sizeof(struct _lastcall)) == sizeof(struct _lastcall)) {
      if (stamp - lc.timestamp < 86400L)
         write (fd, (char *)&lc, sizeof (struct _lastcall));
   }

   close (fd);
   close (fdo);
}

