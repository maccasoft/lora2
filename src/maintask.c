#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <dir.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <dos.h>
#include <sys/stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"
#include "quickmsg.h"

static void create_quickbase_file (void);

void display_outbound_info (int);

void get_call_list()
{
   FILE *fp;
   int i, net, node, m, z;
   char c, filename[100], outbase[100], *p;
   struct ffblk blk, blk2, blko;

   for(i=0;i<MAX_OUT;i++)
   {
      call_list[i].zone = 0;
      call_list[i].net  = 0;
      call_list[i].node = 0;
      call_list[i].type = 0;
      call_list[i].size = 0L;
   }

   max_call = 0;

   strcpy (filename, hold_area);
   filename[strlen(filename) - 1] = '\0';
   strcpy (outbase, filename);
   strcat (filename, ".*");

   if (!findfirst (filename, &blko, FA_DIREC))
      do {
         p = strchr (blko.ff_name, '.');

         sprintf (filename, "%s%s\\*.*", outbase, p == NULL ? "" : p);
         c = '\0';

         if(!findfirst(filename,&blk,0))
            do {
               m = strlen (blk.ff_name);
               if ((blk.ff_name[m-1] == 'T' && blk.ff_name[m-2] == 'U') ||
                   (blk.ff_name[m-1] == 'O' && blk.ff_name[m-2] == 'L') ||
                   (blk.ff_name[m-1] == 'Q' && blk.ff_name[m-2] == 'E' && blk.ff_name[m-3] == 'R'))
               {
                  sscanf(blk.ff_name,"%4x%4x.%c",&net,&node,&c);

                  i = no_dups(net,node);

                  if (p == NULL)
                     call_list[i].zone = alias[0].zone;
                  else {
                     sscanf (&p[1], "%3x", &z);
                     call_list[i].zone = z;
                  }

                  call_list[i].net  = net;
                  call_list[i].node = node;

                  if (blk.ff_name[m-1] == 'T' && blk.ff_name[m-2] == 'U')
                     call_list[i].size += blk.ff_fsize;

                  if (blk.ff_name[m-1] == 'O' && blk.ff_name[m-2] == 'L')
                  {
                     sprintf(filename,"%s%s",hold_area,blk.ff_name);
                     fp = fopen(filename,"rt");
                     while (fgets(filename, 99, fp) != NULL)
                     {
                        if (filename[strlen(filename)-1] == '\n')
                           filename[strlen(filename)-1] = '\0';
                        if (!findfirst (filename, &blk2, 0))
                           do {
                              call_list[i].size += blk2.ff_fsize;
                           } while (!findnext(&blk2));
                        else if (!findfirst (&filename[1], &blk2, 0))
                           do {
                              call_list[i].size += blk2.ff_fsize;
                           } while (!findnext(&blk2));
                     }
                     fclose (fp);
                  }

                  if (blk.ff_name[m-3] == 'C' && blk.ff_name[m-2] == 'U')
                     call_list[i].type |= MAIL_CRASH;
                  else if (blk.ff_name[m-3] == 'C' && blk.ff_name[m-2] == 'L')
                     call_list[i].type |= MAIL_CRASH;
                  else if (blk.ff_name[m-3] == 'F' && blk.ff_name[m-2] == 'L')
                     call_list[i].type |= MAIL_NORMAL;
                  else if (blk.ff_name[m-3] == 'D')
                     call_list[i].type |= MAIL_DIRECT;
                  else if (blk.ff_name[m-3] == 'H')
                     call_list[i].type |= MAIL_HOLD;
                  else if (blk.ff_name[m-3] == 'O' && blk.ff_name[m-2] == 'U')
                     call_list[i].type |= MAIL_NORMAL;
                  else if (blk.ff_name[m-1] == 'Q' && blk.ff_name[m-2] == 'E' && blk.ff_name[m-3] == 'R')
                  {
                     call_list[i].type |= MAIL_REQUEST;
                     call_list[i].size += blk.ff_fsize;
                  }

                  if (max_call <= i)
                     max_call = i + 1;
               }
            } while(!findnext(&blk));
      } while (!findnext (&blko));

   next_call = 0;
}

void display_outbound_info (start)
int start;
{
   int i, row, v;
   char j[30];

   row = 4;
   wfill (4, 16, 6, 46, ' ', LCYAN|_BLUE);

   for (i=start;i<max_call;i++)
   {
      sprintf (j, "%d:%d/%d", call_list[i].zone, call_list[i].net, call_list[i].node);
      wprints (row, 18, LCYAN|_BLUE, j);
      if (call_list[i].size < 1024L)
         sprintf (j, "%ldb", call_list[i].size);
      else
         sprintf (j, "%ldKb", call_list[i].size / 1024L);
      wrjusts (row, 38, LCYAN|_BLUE, j);
      v = 0;
      if (call_list[i].type & MAIL_CRASH)
         j[v++] = 'C';
      if (call_list[i].type & MAIL_NORMAL)
         j[v++] = 'N';
      if (call_list[i].type & MAIL_DIRECT)
         j[v++] = 'D';
      if (call_list[i].type & MAIL_HOLD)
         j[v++] = 'H';
      if (call_list[i].type & MAIL_REQUEST)
         j[v++] = 'R';
      j[v] = '\0';
      wrjusts (row, 45, LCYAN|_BLUE, j);
      row++;
   }
}

int no_dups(net, node)
int net, node;
{
   int i;

   for(i = 0; call_list[i].net; i++)
   {
      if (call_list[i].net == net && call_list[i].node == node)
         break;
   }

   return (i);
}

void sysop_error()
{
   int wh;

   if (!local_mode)
      return;

   hidecur();
   wh = wopen(10,19,13,60,1,LCYAN|_BLUE,LCYAN|_BLUE);
   wactiv(wh);

   wprints(0,2,LCYAN|_BLUE,"ERR 999: Sysop confused.");
   wprints(1,2,LCYAN|_BLUE,"Press any key to return to reality.");

   if (getch() == 0)
      getch();

   wclose();
   wunlink(wh);
   showcur();
}

void clear_status()
{
   int wh;

   wh = whandle();
   wactiv(status);

   wfill(0,0,1,64,' ',BLACK|_LGREY);

   wactiv(wh);
   function_active = 0;
}

void f1_status()
{
   int wh, i;
   char j[80], jn[36];

   if (!usr.name[0])
      return;

   wh = whandle();
   wactiv(status);
   wfill(0,0,1,64,' ',BLACK|_LGREY);

   strcpy(jn, usr.name);
   jn[22] = '\0';
   sprintf(j, "%s of %s at %d baud", jn,
                                     usr.city,
                                     local_mode ? 0 : rate);
   wprints(0,1,BLACK|_LGREY,j);

   if (lorainfo.wants_chat)
      wprints(0,58,BLACK|_LGREY|BLINK,"[CHAT]");

   wprints(1,1,BLACK|_LGREY,"Level:");
   for (i=0;i<12;i++)
      if (levels[i].p_length == usr.priv)
      {
         wprints(1,8,BLACK|_LGREY,levels[i].p_string);
         break;
      }

   sprintf(j, "Time: %d mins", time_remain());
   wprints(1,20,BLACK|_LGREY,j);

   if (usr.ansi)
      wprints(1,36,BLACK|_LGREY,"[ANSI]");
   else if (usr.avatar)
      wprints(1,36,BLACK|_LGREY,"[AVT] ");
   else
      wprints(1,36,BLACK|_LGREY,"      ");

   if (usr.color)
      wprints(1,43,BLACK|_LGREY,"[COLOR]");
   else
      wprints(1,43,BLACK|_LGREY,"       ");

   sprintf(j, "[Line: %02d]", line_offset);
   wprints(1,54,BLACK|_LGREY,j);

   wactiv(wh);
   function_active = 1;
}

void f1_special_status(pwd, name, city)
char *pwd, *name, *city;
{
   int wh;
   char j[80], jn[36];

   wh = whandle();
   wactiv(status);
   wfill(0,0,1,64,' ',BLACK|_LGREY);

   strcpy(jn, name);
   jn[22] = '\0';
   sprintf(j, "%s of %s at %d baud", jn,
                                     city,
                                     local_mode ? 0 : rate);
   wprints(0,1,BLACK|_LGREY,j);

   wprints(1,1,BLACK|_LGREY,"Pwd:");
   strcpy (j, pwd);
   strcode (strupr (j), name);
   wprints(1,6,BLACK|_LGREY,j);

   sprintf(j, "Time: %d mins", time_remain());
   wprints(1,20,BLACK|_LGREY,j);

   if (usr.ansi)
      wprints(1,36,BLACK|_LGREY,"[ANSI]");
   else if (usr.avatar)
      wprints(1,36,BLACK|_LGREY,"[AVT] ");
   else
      wprints(1,36,BLACK|_LGREY,"      ");

   if (usr.color)
      wprints(1,43,BLACK|_LGREY,"[COLOR]");
   else
      wprints(1,43,BLACK|_LGREY,"       ");

   sprintf(j, "[Line: %02d]", line_offset);
   wprints(1,54,BLACK|_LGREY,j);

   wactiv(wh);
}

void f2_status()
{
   int wh;
   char j[80];

   wh = whandle();
   wactiv(status);
   wfill(0,0,1,64,' ',BLACK|_LGREY);

   sprintf(j, "Voice#: %-14.14s Last Call on: %s", usr.voicephone, usr.ldate);
   wprints(0,1,BLACK|_LGREY,j);

   sprintf(j, "Data #: %-14.14s Times Called: %-5ld First Call: %-9.9s", usr.dataphone, usr.times, usr.firstdate);
   wprints(1,1,BLACK|_LGREY,j);

   wactiv(wh);
   function_active = 2;
}

void f3_status()
{
   int wh, i, x;
   char j[80];
   long flag;

   wh = whandle();
   wactiv(status);
   wfill(0,0,1,64,' ',BLACK|_LGREY);

   sprintf(j, "Uploads: %ldK, %d files  Downloads: %ldK, %d files", usr.upld, usr.n_upld, usr.dnld, usr.n_dnld);
   wprints(0,1,BLACK|_LGREY,j);

   flag = usr.flags;
   wprints(1,1,BLACK|_LGREY,"Flags: ");

   x = 0;

   for (i=0;i<8;i++)
   {
      if (flag & 0x80000000L)
         j[x++] = '0' + i;
      else
         j[x++] = '-';
      flag = flag << 1;
   }

   for (i=0;i<8;i++)
   {
      if (flag & 0x80000000L)
         j[x++] = ((i<2)?'8':'?') + i;
      else
         j[x++] = '-';
      flag = flag << 1;
   }

   for (i=0;i<8;i++)
   {
      if (flag & 0x80000000L)
         j[x++] = 'G' + i;
      else
         j[x++] = '-';
      flag = flag << 1;
   }

   for (i=0;i<8;i++)
   {
      if (flag & 0x80000000L)
         j[x++] = 'O' + i;
      else
         j[x++] = '-';
      flag = flag << 1;
   }

   j[x] = '\0';

   wprints(1,8,BLACK|_LGREY,j);

   wactiv(wh);
   function_active = 3;
}

void f4_status()
{
   int wh, sc;
   char j[80];

   wh = whandle();
   wactiv(status);
   wfill(0,0,1,64,' ',BLACK|_LGREY);

   sprintf(j, "Last Caller: %-28.28s  Total Calls: %lu", lastcall.name, sysinfo.total_calls);
   wprints(0,1,BLACK|_LGREY,j);

   if (locked && password != NULL && registered)
      wprints(1,44,BLACK|_LGREY,"[LOCK]");

   wprints(1,54,BLACK|_LGREY,"[F9]=Help");

   sc = time_to_next (0);
   if (next_event >= 0)
   {
      sprintf(j, msgtxt[M_NEXT_EVENT], next_event + 1, sc / 60, sc % 60);
      wprints(1,1,BLACK|_LGREY,j);
   }
   else
      wprints(1,1,BLACK|_LGREY,msgtxt[M_NONE_EVENTS]);

   wactiv(wh);
   function_active = 4;
}

void f9_status()
{
   int wh;

   wh = whandle();
   wactiv(status);
   wfill(0,0,1,64,' ',BLACK|_LGREY);

   wprints(0,1,BLACK|_LGREY,"ALT: [H]angup [J]Shell [L]ockOut [D]Snoop [M]Poll [C]hat e[X]it ");
   wprints(1,1,BLACK|_LGREY,"     [K]eyboard [S]ecurity -Inc/-Dec Time [F1]-[F4]=Extra     ");

   wactiv(wh);
   function_active = 9;
}

void system_crash()
{
   int fd, fdidx;
   char filename[80], nusers;
   long prev, crc;
   struct _useron useron;
   struct stat statbuf, statidx;
   struct _usr usr;
   struct _usridx usridx;

   sprintf(filename, USERON_NAME, sys_path);
   fd = open(filename, O_RDWR|O_BINARY);

   prev = 0L;

   sprintf (filename, "%sLORAINFO.T%02X", sys_path, line_offset);
   unlink (filename);

   while (read(fd, (char *)&useron, sizeof(struct _useron)) == sizeof(struct _useron))
   {
      if (useron.line == line_offset && useron.name[0])
      {
         status_line("!System crash detected on task %d", line_offset);
         status_line("!User on-line at time of crash was %s", useron.name);

         memset((char *)&useron, 0, sizeof(struct _useron));
         lseek(fd, prev, SEEK_SET);
         write(fd, (char *)&useron, sizeof(struct _useron));
         break;
      }

      prev = tell(fd);
   }

   close(fd);

   sprintf (filename, "%s.BBS", user_file);
   fd = open(filename, O_RDWR|O_BINARY);
   if (fd == -1)
      fd = open (filename, O_CREAT|O_TRUNC|O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
   fstat (fd, &statbuf);

   sprintf (filename, "%s.IDX", user_file);
   fdidx = open(filename, O_RDWR|O_BINARY);
   if (fdidx == -1)
      fdidx = open (filename, O_CREAT|O_TRUNC|O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
   fstat (fdidx, &statidx);


   if ( (statbuf.st_size / sizeof (struct _usr)) != (statidx.st_size / sizeof (struct _usridx)) )
   {
      status_line("!%s", "Rebuilding user index file");
      wputs ("* ");
      wputs ("Rebuilding user index file");
      wputs ("\n");
      chsize (fdidx, 0L);

      nusers = 0;

      for (;;)
      {
         if (!registered && nusers >= 50)
            break;

         prev = tell (fd);
         if (read (fd, (char *)&usr, sizeof(struct _usr)) != sizeof(struct _usr))
            break;

         if (!usr.deleted)
         {
            crc = crc_name (usr.name);
            usr.id = crc;
            usridx.id = crc;
         }
         else
            crc = 0L;

         lseek (fd, prev, SEEK_SET);
         write (fd, (char *)&usr, sizeof(struct _usr));
         write (fdidx, (char *)&usridx, sizeof(struct _usridx));

         nusers++;
      }

      close (fd);
      close (fdidx);
   }

   create_quickbase_file ();
}

void get_last_caller()
{
   int fd;
   char filename[80];
   struct _lastcall lc;

   memcpy ((char *)&lastcall, 0, sizeof (struct _lastcall));

   sprintf(filename, "%sLASTCALL.BBS", sys_path);
   fd = open(filename, O_RDONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);

   while (read(fd, (char *)&lc, sizeof(struct _lastcall)) == sizeof(struct _lastcall))
   {
      if (lc.line == line_offset)
         memcpy ((char *)&lastcall, (char *)&lc, sizeof (struct _lastcall));
   }

   if (!lastcall.name[0])
      strcpy(lastcall.name, msgtxt[M_NEXT_NONE]);

   close (fd);
}

void set_last_caller()
{
   int fd, i, m;
   char filename[80];
   struct _lastcall lc;

   sprintf(filename, "%sLASTCALL.BBS", sys_path);
   fd = open(filename, O_APPEND|O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);

   memset((char *)&lc, 0, sizeof(struct _lastcall));

   lc.line = line_offset;
   strcpy(lc.name, usr.name);
   strcpy(lc.city, usr.city);
   lc.baud = local_mode ? 0 : rate;
   lc.times = usr.times;
   strncpy(lc.logon, &usr.ldate[11], 5);

   data (filename);
   strncpy(lc.logoff, &filename[11], 5);

   write(fd, (char *)&lc, sizeof(struct _lastcall));

   close (fd);

   lc.logon[2] = '\0';
   lc.logoff[2] = '\0';

   i = atoi (lc.logon);
   m = atoi (lc.logoff);
   if (m < i)
      m += 24;

   for (; i <= m; i++)
   {
      if (linestat.busyperhour[i % 24] < 65435U)
      {
         if (atoi(lc.logon) == atoi(lc.logoff))
            linestat.busyperhour[i % 24] += atoi (&lc.logoff[3]) - atoi (&lc.logon[3]) + 1;
         else if (i == atoi (lc.logon))
            linestat.busyperhour[i % 24] += 60 - atoi (&lc.logon[3]);
         else if (i == atoi (lc.logoff))
            linestat.busyperhour[i % 24] += atoi (&lc.logoff[3]) + 1;
         else
            linestat.busyperhour[i % 24] += 60;
      }
   }
}

void manual_poll ()
{
   int wh, i, zone, net, node, point;
   char stringa1[20];

   i = (80 - strlen (msgtxt[M_PHONE_OR_NODE]) - 21) / 2;
   wh = wopen(18,i,20,i + strlen (msgtxt[M_PHONE_OR_NODE]) + 21,1,LCYAN|_BLUE,LCYAN|_BLUE);
   wactiv(wh);

   showcur ();

   wputs (msgtxt[M_PHONE_OR_NODE]);

   winpbeg (WHITE|_RED, WHITE|_RED);
   winpdef (0, strlen (msgtxt[M_PHONE_OR_NODE]), stringa1,
            "???????????????????", '?', 0, NULL, 0);

   if (winpread () == W_ESCPRESS || !strlen (stringa1))
   {
      hidecur ();
      wclose ();
      wunlink (wh);
      return;
   }
   strtrim (stringa1);

   wclose ();
   wunlink (wh);
   hidecur ();

   parse_netnode(stringa1, &zone, &net, &node, &point);

   if (point)
   {
      sysop_error ();
      return;
   }

   if (!get_bbs_record (zone, net, node))
   {
      if ( strchr (stringa1, ':') || strchr (stringa1, '/') || strchr (stringa1, '.') )
         return;
   }

   status_line (msgtxt[M_POLL_MODE]);
   (void) poll(30, 0, zone, net, node);
   status_line (msgtxt[M_POLL_COMPLETED]);
   local_status("Waiting for a call");
}

static void create_quickbase_file ()
{
   int fd, creating, fdidx, fdto;
   word nhdr, nidx, nto;
   char filename[80];
   struct _msghdr msghdr;
   struct _msgidx msgidx;
   struct _msginfo msginfo;

   if (fido_msgpath == NULL)
      return;

   creating = 0;

   sprintf(filename,"%sMSGINFO.BBS",fido_msgpath);
   fd = open(filename,O_RDONLY|O_BINARY);
   if (fd == -1)
   {
      fd = open(filename,O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
      if (fd == -1)
         return;
      memset((char *)&msginfo, 0, sizeof(struct _msginfo));
      write(fd, (char *)&msginfo, sizeof(struct _msginfo));
      creating = 1;
   }
   else
      read(fd, (char *)&msginfo, sizeof(struct _msginfo));
   close (fd);

   sprintf(filename,"%sMSGIDX.BBS",fido_msgpath);
   fd = open(filename,O_RDONLY|O_BINARY);
   if (fd == -1)
   {
      fd = open(filename,O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
      if (fd == -1)
         return;
      creating = 1;
   }
   nidx = (word)(filelength(fd) / sizeof (struct _msgidx));
   close (fd);

   sprintf(filename,"%sMSGHDR.BBS",fido_msgpath);
   fd = open(filename,O_RDONLY|O_BINARY);
   if (fd == -1)
   {
      fd = open(filename,O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
      if (fd == -1)
         return;
      creating = 1;
   }
   nhdr = (word)(filelength(fd) / sizeof (struct _msghdr));
   close (fd);

   sprintf(filename,"%sMSGTOIDX.BBS",fido_msgpath);
   fd = open(filename,O_RDONLY|O_BINARY);
   if (fd == -1)
   {
      fd = open(filename,O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
      if (fd == -1)
         return;
      creating = 1;
   }
   nto = (word)(filelength(fd) / sizeof (struct _msgtoidx));
   close (fd);

   sprintf(filename,"%sMSGTXT.BBS",fido_msgpath);
   fd = open(filename,O_RDONLY|O_BINARY);
   if (fd == -1)
   {
      fd = open(filename,O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
      if (fd == -1)
         return;
      creating = 1;
   }
   close (fd);

   if (creating)
   {
      wputs ("* Creating QuickBBS message base files\n");
      status_line ("!Creating QuickBBS message base files");
   }

   if (msginfo.totalmsgs == nhdr &&
       msginfo.totalmsgs == nidx &&
       msginfo.totalmsgs == nto)
      return;

/*
   wputs ("* Rebuilding QuickBBS message base files\n");
   status_line ("!Rebuilding QuickBBS message base files");

   sprintf(filename,"%sMSGHDR.BBS",fido_msgpath);
   fd = open(filename,O_RDONLY|O_BINARY);
   if (fd == -1)
   {
      fd = open(filename,O_RDONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
      if (fd == -1)
         return;
      creating = 1;
   }

   memset((char *)&msginfo, 0, sizeof(struct _msginfo));
   msginfo.lowmsg = 65535U;

   sprintf(filename,"%sMSGIDX.BBS",fido_msgpath);
   fdidx = open(filename,O_WRONLY|O_BINARY|O_TRUNC);

   sprintf(filename,"%sMSGTOIDX.BBS",fido_msgpath);
   fdto = open(filename,O_WRONLY|O_BINARY|O_TRUNC);

   while (read(fd, (char *)&msghdr, sizeof (struct _msghdr)) == sizeof (struct _msghdr))
   {
      if (msghdr.msgattr & Q_RECKILL)
      {
         msgidx.msgnum = 0;
         write (fdto, "\x0B* Deleted *                        ", 36);
      }
      else
      {
         write (fdto, (char *)&msghdr.whoto, sizeof (struct _msgtoidx));
         msgidx.msgnum = msghdr.msgnum;
         msginfo.totalonboard[msgidx.board-1]++;
      }
      msgidx.board = msghdr.board;
      write (fdidx, (char *)&msgidx, sizeof (struct _msgidx));
      msginfo.totalmsgs++;
      if (msginfo.lowmsg > msgidx.msgnum)
         msginfo.lowmsg = msgidx.msgnum;
      if (msginfo.highmsg < msgidx.msgnum)
         msginfo.highmsg = msgidx.msgnum;
   }

   close (fdto);
   close (fdidx);
   close (fd);

   sprintf(filename,"%sMSGINFO.BBS",fido_msgpath);
   fd = open(filename,O_WRONLY|O_BINARY);
   write(fd, (char *)&msginfo, sizeof(struct _msginfo));
   close (fd);
*/
}

void keyboard_password ()
{
   int wh, i;
   char stringa1[40], stringa2[40];

   i = (80 - strlen (msgtxt[M_INPUT_COMMAND]) - 32) / 2;
   wh = wopen(18,i,20,i + strlen (msgtxt[M_INPUT_COMMAND]) + 32,1,LCYAN|_BLUE,LCYAN|_BLUE);
   wactiv(wh);

   read_sysinfo();
   showcur ();

   wputs (msgtxt[M_INPUT_COMMAND]);

   winpbeg (WHITE|_RED, WHITE|_RED);
   winpdef (0, strlen (msgtxt[M_INPUT_COMMAND]), stringa1,
            "??????????????????????????????", 'P', 0, NULL, 0);

   if (winpread () == W_ESCPRESS)
   {
      hidecur ();
      wclose ();
      wunlink (wh);
      return;
   }
   strtrim (stringa1);

   wputs (msgtxt[M_ELEMENT_CHOSEN]);

   winpbeg (WHITE|_RED, WHITE|_RED);
   winpdef (0, strlen (msgtxt[M_INPUT_COMMAND]), stringa2,
            "??????????????????????????????", 'P', 0, NULL, 0);

   if (winpread () == W_ESCPRESS)
   {
      hidecur ();
      wclose ();
      wunlink (wh);
      return;
   }
   strtrim (stringa2);

   wclose ();
   wunlink (wh);
   hidecur ();

   if (strcmp(stringa1, stringa2))
   {
      wputs ("\a");
      return;
   }

   strcpy (sysinfo.pwd, stringa1);
   write_sysinfo ();

   if (sysinfo.pwd[0])
      password = sysinfo.pwd;
   else
      password = NULL;

   if (password != NULL)
   {
      locked = 1;
      if (function_active == 4)
         f4_status ();
   }
}

void set_security ()
{
   int wh, i, x;
   char stringa[12], stringa2[33], *sf = "Flags: ";
   long flag;

   i = (80 - strlen (msgtxt[M_SET_SECURITY]) - 13) / 2;
   x = (80 - strlen (sf) - 35) / 2;
   if (x < i)
   {
      i = x;
      x = strlen (sf) + 35;
   }
   else
      x = strlen (msgtxt[M_SET_SECURITY]) + 13;

   wh = wopen(18,i,21,i + x,1,LCYAN|_BLUE,LCYAN|_BLUE);
   wactiv(wh);

   strcpy (stringa, get_priv_text(usr.priv) );

   x = 0;
   flag = usr.flags;

   for (i=0;i<8;i++)
   {
      if (flag & 0x80000000L)
         stringa2[x++] = '0' + i;
      flag = flag << 1;
   }

   for (i=0;i<8;i++)
   {
      if (flag & 0x80000000L)
         stringa2[x++] = (i<2)?'8':'?' + i;
      flag = flag << 1;
   }

   for (i=0;i<8;i++)
   {
      if (flag & 0x80000000L)
         stringa2[x++] = 'G' + i;
      flag = flag << 1;
   }

   for (i=0;i<8;i++)
   {
      if (flag & 0x80000000L)
         stringa2[x++] = 'O' + i;
      flag = flag << 1;
   }

   stringa2[x] = '\0';

   showcur ();

   wprints (0,1,LCYAN|_BLUE,msgtxt[M_SET_SECURITY]);
   wprints (1,1,LCYAN|_BLUE,sf);

   winpbeg (WHITE|_RED, WHITE|_RED);
   winpdef (0, strlen (msgtxt[M_SET_SECURITY])+1, stringa,
            "???????????", 'M', 1, NULL, 0);
   winpdef (1, strlen (sf)+1, stringa2,
            "????????????????????????????????", 'U', 1, NULL, 0);

   if (winpread () == W_ESCPRESS)
   {
      hidecur ();
      wclose ();
      wunlink (wh);
      return;
   }

   hidecur ();
   wclose ();
   wunlink (wh);

   strtrim (stringa);
   strtrim (stringa2);

   usr.priv = get_priv (stringa);
   usr.flags = get_flags (stringa2);
   usr_class = get_class(usr.priv);

   if (function_active == 1)
      f1_status ();
   else if (function_active == 3)
      f3_status ();
}

