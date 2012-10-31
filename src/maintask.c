
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
#include <ctype.h>
#include <dir.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <dos.h>
#include <alloc.h>
#include <sys/stat.h>

#ifdef __OS2__
#define INCL_DOS
#define INCL_VIO
#include <os2.h>
#endif

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>
#include <cxl\cxlkey.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "quickmsg.h"

extern int outinfo;
extern char *wtext[];

static void create_quickbase_file (void);
int no_dups(int, int, int, int);
void get_bad_call (int, int, int, int, int);
int isbundle (char *);

static int node_sort_func (const void *a1, const void *b1)
{
   struct _call_list *a, *b;

   a = (struct _call_list *)a1;
   b = (struct _call_list *)b1;

   if (a->zone != b->zone)  return (a->zone - b->zone);
   if (a->net != b->net)    return (a->net - b->net);
   if (a->node != b->node)    return (a->node - b->node);
   return (a->point - b->point);
}

void sort_call_list ()
{
   qsort (&call_list[0], max_call, sizeof (struct _call_list), node_sort_func);
}

void build_node_queue (int zone, int net, int node, int point, int i)
{
   FILE *fp;
   int fd, m;
   char filename[120], something = 0;
   struct ffblk blk, blk2;

   if (i != -1) {
      call_list[i].type = 0;
      call_list[i].size = call_list[i].b_mail = call_list[i].b_data = 0;
      call_list[i].n_mail = call_list[i].n_data = 0;
   }

   if (!point)
      sprintf (filename, "%s%04X%04X.*", HoldAreaNameMunge (zone), net, node);
   else
      sprintf (filename, "%s%04X%04X.PNT\\%08X.*", HoldAreaNameMunge (zone), net, node, point);

   if (findfirst (filename, &blk, 0))
      return;

   if (i == -1)
      i = no_dups (zone, net, node, point);

   memset (&call_list[i], 0, sizeof (struct _call_list));
   call_list[i].zone = zone;
   call_list[i].net = net;
   call_list[i].node = node;
   call_list[i].point = point;

   do {
      m = strlen (strupr (blk.ff_name));
      if (blk.ff_name[m - 1] == 'T' && blk.ff_name[m - 2] == 'U') {
         if (blk.ff_fdate > call_list[i].fdate || blk.ff_ftime > call_list[i].ftime) {
            call_list[i].fdate = blk.ff_fdate;
            call_list[i].ftime = blk.ff_ftime;
         }
         something = 1;
         call_list[i].size += blk.ff_fsize;
         call_list[i].n_mail++;
         call_list[i].b_mail += blk.ff_fsize;
      }
      else if (blk.ff_name[m - 1] == 'O' && blk.ff_name[m - 2] == 'L') {
         if (blk.ff_fdate > call_list[i].fdate || blk.ff_ftime > call_list[i].ftime) {
            call_list[i].fdate = blk.ff_fdate;
            call_list[i].ftime = blk.ff_ftime;
         }
         something = 1;

         if (!point)
            sprintf (filename, "%s%s", HoldAreaNameMunge (zone), blk.ff_name);
         else
            sprintf (filename, "%s%04X%04X.PNT\\%s", HoldAreaNameMunge (zone), net, node, blk.ff_name);

         fp = sh_fopen (filename, "rt", SH_DENYNONE);
         while (fp != NULL && fgets (filename, 99, fp) != NULL) {
            if (filename[strlen (filename) - 1] == '\n')
               filename[strlen (filename) - 1] = '\0';
            if (strchr (filename, '*') == NULL && strchr (filename, '?') == NULL) {
               if ((fd = open (filename, O_RDONLY|O_BINARY)) != -1) {
                  call_list[i].size += filelength (fd);
                  if (isbundle (&filename[strlen (filename) - 12])) {
                     call_list[i].n_mail++;
                     call_list[i].b_mail += filelength (fd);
                  }
                  else {
                     call_list[i].n_data++;
                     call_list[i].b_data += filelength (fd);
                  }
                  close (fd);
               }
               else if ((fd = open (&filename[1], O_RDONLY)) != -1) {
                  call_list[i].size += filelength (fd);
                  if (isbundle (&filename[strlen (filename) - 12])) {
                     call_list[i].n_mail++;
                     call_list[i].b_mail += filelength (fd);
                  }
                  else {
                     call_list[i].n_data++;
                     call_list[i].b_data += filelength (fd);
                  }
                  close (fd);
               }
            }
            else {
               if (!findfirst (filename, &blk2, 0))
                  do {
                     call_list[i].size += blk2.ff_fsize;
                     if (isbundle (blk2.ff_name)) {
                        call_list[i].n_mail++;
                        call_list[i].b_mail += blk2.ff_fsize;
                     }
                     else {
                        call_list[i].n_data++;
                        call_list[i].b_data += blk2.ff_fsize;
                     }
                  } while (!findnext(&blk2));
               else if (!findfirst (&filename[1], &blk2, 0))
                  do {
                     call_list[i].size += blk2.ff_fsize;
                     if (isbundle (blk2.ff_name)) {
                        call_list[i].n_mail++;
                        call_list[i].b_mail += blk2.ff_fsize;
                     }
                     else {
                        call_list[i].n_data++;
                        call_list[i].b_data += blk2.ff_fsize;
                     }
                  } while (!findnext(&blk2));
            }
         }
         fclose (fp);
      }
      if (blk.ff_name[m-3] == 'C' && blk.ff_name[m-2] == 'U')
         call_list[i].type |= MAIL_CRASH;
      else if (blk.ff_name[m-3] == 'C' && blk.ff_name[m-2] == 'L')
         call_list[i].type |= MAIL_CRASH;
      else if (blk.ff_name[m-3] == 'I' && blk.ff_name[m-2] == 'L')
         call_list[i].type |= MAIL_WILLGO;
      else if (blk.ff_name[m-3] == 'F' && blk.ff_name[m-2] == 'L')
         call_list[i].type |= MAIL_NORMAL;
      else if (blk.ff_name[m-3] == 'D')
         call_list[i].type |= MAIL_DIRECT;
      else if (blk.ff_name[m-3] == 'H')
         call_list[i].type |= MAIL_HOLD;
      else if (blk.ff_name[m-3] == 'O' && blk.ff_name[m-2] == 'U')
         call_list[i].type |= MAIL_NORMAL;
      else if (blk.ff_name[m-1] == 'Q' && blk.ff_name[m-2] == 'E' && blk.ff_name[m-3] == 'R') {
         if (!(call_list[i].type & MAIL_REQUEST)) {
            call_list[i].type |= MAIL_REQUEST;
            call_list[i].size += blk.ff_fsize;
         }
         something = 1;
      }
   } while (!findnext (&blk));

   if (something && max_call <= i)
      max_call = i + 1;
}

void build_call_queue (int force)
{
   FILE *fp;
   int fd, i, net, node, m, z, point;
   char c, filename[100], outbase[100], *p, noinside = 0;
   struct ffblk blk, blk2, blko, blkp;

   memset ((char *)&call_list, 0, sizeof (struct _call_list) * MAX_OUT);
   max_call = 0;
   next_call = 0;

   wfill (14, 1, 21, 51, ' ', LCYAN|_BLACK);
   time_release ();

   if (!force) {
      sprintf (filename, "%sQUEUE.DAT", config->sys_path);
      fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
      if (fd != -1) {
         max_call = 0;
         read (fd, (char *)&max_call, sizeof (short));
         read (fd, (char *)&call_list, sizeof (struct _call_list) * max_call);
         close (fd);
         noinside = 1;

         for (i = 0; i < max_call; i++)
            call_list[i].type &= ~(MAIL_EXISTS|MAIL_INSPECT|MAIL_WILLGO);
      }
   }

   strcpy (filename, HoldAreaNameMunge (config->alias[0].zone));
   filename[strlen(filename) - 1] = '\0';
   strcpy (outbase, filename);
   strcat (filename, ".*");

   if (!findfirst (filename, &blko, FA_DIREC))
      do {
         p = strchr (strupr (blko.ff_name), '.');

         sprintf (filename, "%s%s\\*.*", outbase, p == NULL ? "" : p);
         c = '\0';

         if(!findfirst(filename,&blk,FA_DIREC))
            do {
               m = strlen (strupr (blk.ff_name));
               if (m < 3)
                  continue;

               // Si tratta di .?UT .?LO o .REQ
               if ((blk.ff_name[m-1] == 'T' && blk.ff_name[m-2] == 'U') || (blk.ff_name[m-1] == 'O' && blk.ff_name[m-2] == 'L') || (blk.ff_name[m-1] == 'Q' && blk.ff_name[m-2] == 'E' && blk.ff_name[m-3] == 'R')) {
                  sscanf(blk.ff_name,"%4x%4x.%c",&net,&node,&c);

                  if (p == NULL) {
                     i = no_dups (config->alias[0].zone, net, node, 0);
                     call_list[i].zone = config->alias[0].zone;
                  }
                  else {
                     sscanf (&p[1], "%3x", &z);
                     i = no_dups (z, net, node, 0);
                     call_list[i].zone = z;
                  }

                  call_list[i].net  = net;
                  call_list[i].node = node;
                  call_list[i].type |= MAIL_EXISTS;

                  if (blk.ff_fdate > call_list[i].fdate || blk.ff_ftime > call_list[i].ftime) {
                     call_list[i].fdate = blk.ff_fdate;
                     call_list[i].ftime = blk.ff_ftime;
                     if (noinside && max_call > i)
                        call_list[i].type |= MAIL_INSPECT;
                  }

                  if (!noinside || max_call <= i) {
                     if (!get_bbs_record (call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point))
                        call_list[i].type |= MAIL_UNKNOWN;

                     if (blk.ff_name[m-1] == 'T' && blk.ff_name[m-2] == 'U') {
                        call_list[i].size += blk.ff_fsize;
                        call_list[i].n_mail++;
                        call_list[i].b_mail += blk.ff_fsize;
                     }
                     else if (blk.ff_name[m-1] == 'O' && blk.ff_name[m-2] == 'L') {
                        sprintf(filename,"%s%s",HoldAreaNameMunge (call_list[i].zone),blk.ff_name);
                        fp = sh_fopen (filename, "rt", SH_DENYNONE);
                        while (fp != NULL && fgets(filename, 99, fp) != NULL) {
                           if (filename[strlen(filename)-1] == '\n')
                              filename[strlen(filename)-1] = '\0';
                           if (strchr (filename, '*') == NULL && strchr (filename, '?') == NULL) {
                              if ((fd = open (filename, O_RDONLY)) != -1) {
                                 call_list[i].size += filelength (fd);
                                 if (isbundle (&filename[strlen (filename) - 12])) {
                                    call_list[i].n_mail++;
                                    call_list[i].b_mail += filelength (fd);
                                 }
                                 else {
                                    call_list[i].n_data++;
                                    call_list[i].b_data += filelength (fd);
                                 }
                                 close (fd);
                              }
                              else if ((fd = open (&filename[1], O_RDONLY)) != -1) {
                                 call_list[i].size += filelength (fd);
                                 if (isbundle (&filename[strlen (filename) - 12])) {
                                    call_list[i].n_mail++;
                                    call_list[i].b_mail += filelength (fd);
                                 }
                                 else {
                                    call_list[i].n_data++;
                                    call_list[i].b_data += filelength (fd);
                                 }
                                 close (fd);
                              }
                           }
                           else {
                              if (!findfirst (filename, &blk2, 0))
                                 do {
                                    call_list[i].size += blk2.ff_fsize;
                                    if (isbundle (blk2.ff_name)) {
                                       call_list[i].n_mail++;
                                       call_list[i].b_mail += blk2.ff_fsize;
                                    }
                                    else {
                                       call_list[i].n_data++;
                                       call_list[i].b_data += blk2.ff_fsize;
                                    }
                                 } while (!findnext(&blk2));
                              else if (!findfirst (&filename[1], &blk2, 0))
                                 do {
                                    call_list[i].size += blk2.ff_fsize;
                                    if (isbundle (blk2.ff_name)) {
                                       call_list[i].n_mail++;
                                       call_list[i].b_mail += blk2.ff_fsize;
                                    }
                                    else {
                                       call_list[i].n_data++;
                                       call_list[i].b_data += blk2.ff_fsize;
                                    }
                                 } while (!findnext(&blk2));
                           }
                        }
                        fclose (fp);
                     }
                  }

                  if (blk.ff_name[m-3] == 'C' && blk.ff_name[m-2] == 'U')
                     call_list[i].type |= MAIL_CRASH;
                  else if (blk.ff_name[m-3] == 'C' && blk.ff_name[m-2] == 'L')
                     call_list[i].type |= MAIL_CRASH;
                  else if (blk.ff_name[m-3] == 'I' && blk.ff_name[m-2] == 'L')
                     call_list[i].type |= MAIL_WILLGO;
                  else if (blk.ff_name[m-3] == 'F' && blk.ff_name[m-2] == 'L')
                     call_list[i].type |= MAIL_NORMAL;
                  else if (blk.ff_name[m-3] == 'D')
                     call_list[i].type |= MAIL_DIRECT;
                  else if (blk.ff_name[m-3] == 'H')
                     call_list[i].type |= MAIL_HOLD;
                  else if (blk.ff_name[m-3] == 'O' && blk.ff_name[m-2] == 'U')
                     call_list[i].type |= MAIL_NORMAL;
                  else if (blk.ff_name[m-1] == 'Q' && blk.ff_name[m-2] == 'E' && blk.ff_name[m-3] == 'R') {
                     call_list[i].type |= MAIL_REQUEST;
                     if (!noinside || max_call <= i)
                        call_list[i].size += blk.ff_fsize;
                  }

                  if (max_call <= i)
                     max_call = i + 1;
               }

               // E' stata trovata la directory per i point .PNT
               else if (blk.ff_name[m-1] == 'T' && blk.ff_name[m-2] == 'N' && blk.ff_name[m-3] == 'P' && blk.ff_name[m-4] == '.') {
                  sscanf(blk.ff_name,"%4x%4x.", &net, &node);
                  sprintf (filename, "%s%s\\%s\\*.*", outbase, p == NULL ? "" : p, blk.ff_name);

                  if (!findfirst (filename, &blkp, 0))
                     do {
                        m = strlen (strupr (blkp.ff_name));

                        // Si tratta di .?UT .?LO o .REQ
                        if ((blkp.ff_name[m-1] == 'T' && blkp.ff_name[m-2] == 'U') || (blkp.ff_name[m-1] == 'O' && blkp.ff_name[m-2] == 'L') || (blkp.ff_name[m-1] == 'Q' && blkp.ff_name[m-2] == 'E' && blkp.ff_name[m-3] == 'R')) {
                           sscanf(blkp.ff_name,"%4x%4x.%c", &i, &point, &c);

                           if (p == NULL) {
                              i = no_dups (config->alias[0].zone, net, node, point);
                              call_list[i].zone = config->alias[0].zone;
                           }
                           else {
                              sscanf (&p[1], "%3x", &z);
                              i = no_dups (z, net, node, point);
                              call_list[i].zone = z;
                           }

                           call_list[i].net  = net;
                           call_list[i].node = node;
                           call_list[i].point = point;
                           call_list[i].type |= MAIL_EXISTS;

                           if (blkp.ff_fdate > call_list[i].fdate || blkp.ff_ftime > call_list[i].ftime) {
                              call_list[i].fdate = blkp.ff_fdate;
                              call_list[i].ftime = blkp.ff_ftime;
                              if (noinside && max_call > i)
                                 call_list[i].type |= MAIL_INSPECT;
                           }

                           if (!noinside || max_call <= i) {
                              if (!get_bbs_record (call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point)) {
                                 if (!get_bbs_record (call_list[i].zone, call_list[i].net, call_list[i].node, 0))
                                    call_list[i].type |= MAIL_UNKNOWN;
                              }

                              if (blkp.ff_name[m-1] == 'T' && blkp.ff_name[m-2] == 'U') {
                                 call_list[i].size += blkp.ff_fsize;
                                 call_list[i].n_mail++;
                                 call_list[i].b_mail += blkp.ff_fsize;
                              }
                              else if (blkp.ff_name[m-1] == 'O' && blkp.ff_name[m-2] == 'L') {
                                 sprintf(filename,"%s\\%04x%04x.PNT\\%s",HoldAreaNameMunge (call_list[i].zone),call_list[i].net,call_list[i].node,blkp.ff_name);
                                 fp = sh_fopen (filename, "rt", SH_DENYNONE);
                                 while (fp != NULL && fgets(filename, 99, fp) != NULL) {
                                    if (filename[strlen(filename)-1] == '\n')
                                       filename[strlen(filename)-1] = '\0';
                                    if (strchr (filename, '*') == NULL && strchr (filename, '?') == NULL) {
                                       if ((fd = open (filename, O_RDONLY)) != -1) {
                                          call_list[i].size += filelength (fd);
                                          if (isbundle (&filename[strlen (filename) - 12])) {
                                             call_list[i].n_mail++;
                                             call_list[i].b_mail += filelength (fd);
                                          }
                                          else {
                                             call_list[i].n_data++;
                                             call_list[i].b_data += filelength (fd);
                                          }
                                          close (fd);
                                       }
                                       else if ((fd = open (&filename[1], O_RDONLY)) != -1) {
                                          call_list[i].size += filelength (fd);
                                          if (isbundle (&filename[strlen (filename) - 12])) {
                                             call_list[i].n_mail++;
                                             call_list[i].b_mail += filelength (fd);
                                          }
                                          else {
                                             call_list[i].n_data++;
                                             call_list[i].b_data += filelength (fd);
                                          }
                                          close (fd);
                                       }
                                    }
                                    else {
                                       if (!findfirst (filename, &blk2, 0))
                                          do {
                                             call_list[i].size += blk2.ff_fsize;
                                             if (isbundle (blk2.ff_name)) {
                                                call_list[i].n_mail++;
                                                call_list[i].b_mail += blk2.ff_fsize;
                                             }
                                             else {
                                                call_list[i].n_data++;
                                                call_list[i].b_data += blk2.ff_fsize;
                                             }
                                          } while (!findnext(&blk2));
                                       else if (!findfirst (&filename[1], &blk2, 0))
                                          do {
                                             call_list[i].size += blk2.ff_fsize;
                                             if (isbundle (blk2.ff_name)) {
                                                call_list[i].n_mail++;
                                                call_list[i].b_mail += blk2.ff_fsize;
                                             }
                                             else {
                                                call_list[i].n_data++;
                                                call_list[i].b_data += blk2.ff_fsize;
                                             }
                                          } while (!findnext(&blk2));
                                    }
                                 }
                                 fclose (fp);
                              }
                           }

                           if (blkp.ff_name[m-3] == 'C' && blkp.ff_name[m-2] == 'U')
                              call_list[i].type |= MAIL_CRASH;
                           else if (blkp.ff_name[m-3] == 'C' && blkp.ff_name[m-2] == 'L')
                              call_list[i].type |= MAIL_CRASH;
                           else if (blkp.ff_name[m-3] == 'F' && blkp.ff_name[m-2] == 'L')
                              call_list[i].type |= MAIL_NORMAL;
                           else if (blkp.ff_name[m-3] == 'D')
                              call_list[i].type |= MAIL_DIRECT;
                           else if (blkp.ff_name[m-3] == 'H')
                              call_list[i].type |= MAIL_HOLD;
                           else if (blkp.ff_name[m-3] == 'O' && blkp.ff_name[m-2] == 'U')
                              call_list[i].type |= MAIL_NORMAL;
                           else if (blkp.ff_name[m-1] == 'Q' && blkp.ff_name[m-2] == 'E' && blkp.ff_name[m-3] == 'R') {
                              call_list[i].type |= MAIL_REQUEST;
                              call_list[i].size += blkp.ff_fsize;
                           }

                           if (max_call <= i)
                              max_call = i + 1;
                        }

                        // Si tratta del contatore di tentativi .$$?
                        else if (blk.ff_name[m-2] == '$' && blk.ff_name[m-3] == '$' && blk.ff_name[m-4] == '.') {
                           sscanf(blkp.ff_name,"%4x%4x", &i, &point);

                           if (p == NULL) {
                              i = no_dups (config->alias[0].zone, net, node, point);
                              call_list[i].zone = config->alias[0].zone;
                           }
                           else {
                              sscanf (&p[1], "%3x", &z);
                              i = no_dups (z, net, node, point);
                              call_list[i].zone = z;
                           }

                           call_list[i].net  = net;
                           call_list[i].node = node;
                           call_list[i].point = point;

                           call_list[i].call_wc = blk.ff_name[m - 1] - '0';

                           sprintf(filename,"%s\\%04x%04x.PNT\\%s",HoldAreaNameMunge (call_list[i].zone),call_list[i].net,call_list[i].node,blkp.ff_name);
                           fd = shopen (filename, O_RDONLY|O_BINARY);
                           read (fd, (char *) &call_list[i].call_nc, sizeof (short));
                           read (fd, (char *) &call_list[i].flags, sizeof (short));
                           close (fd);

                           if (max_call <= i)
                              max_call = i + 1;
                        }
                     } while (!findnext (&blkp));
               }

               // Si tratta del contatore dei tentativi di chiamata .$$?
               else if (blk.ff_name[m-2] == '$' && blk.ff_name[m-3] == '$' && blk.ff_name[m-4] == '.') {
                  sscanf(blk.ff_name,"%4x%4x",&net,&node);

                  if (p == NULL) {
                     i = no_dups (config->alias[0].zone, net, node, 0);
                     call_list[i].zone = config->alias[0].zone;
                  }
                  else {
                     sscanf (&p[1], "%3x", &z);
                     i = no_dups (z, net, node, 0);
                     call_list[i].zone = z;
                  }

                  call_list[i].net  = net;
                  call_list[i].node = node;

                  call_list[i].call_wc = blk.ff_name[m - 1] - '0';

                  sprintf(filename,"%s%s",HoldAreaNameMunge (call_list[i].zone),blk.ff_name);
                  fd = shopen (filename, O_RDONLY|O_BINARY);
                  read (fd, (char *) &call_list[i].call_nc, sizeof (short));
                  read (fd, (char *) &call_list[i].flags, sizeof (short));
                  close (fd);

                  if (max_call <= i)
                     max_call = i + 1;
               }
            } while (!findnext (&blk));
      } while (!findnext (&blko));

   time_release ();

   m = 0;
   for (i = 0; i < max_call; i++) {
      if ((call_list[i].type & ~MAIL_EXISTS) && (call_list[i].type & MAIL_EXISTS)) {
         memcpy (&call_list[m], &call_list[i], sizeof (struct _call_list));
         m++;
      }
   }
   max_call = m;

   qsort (&call_list[0], max_call, sizeof (struct _call_list), node_sort_func);
   next_call = -1;

   for (m = 0; m < max_call; m++)
      if (call_list[m].type & MAIL_WILLGO) {
         next_call = m;
         break;
      }

   if (next_call == -1)
      for (m = 0; m < max_call; m++) {
         if ( call_list[m].type & MAIL_UNKNOWN )
            continue;
         else if ( !(call_list[m].type & (MAIL_CRASH|MAIL_DIRECT|MAIL_NORMAL)) )
            continue;
         else if (cur_event >= 0 && (e_ptrs[cur_event]->behavior & MAT_NOOUT))
            continue;
         else if ((call_list[m].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_NOCM))
            continue;
         else if (!(call_list[m].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_CM))
            continue;
         else if (cur_event >= 0 && e_ptrs[cur_event]->res_net && (call_list[m].net != e_ptrs[cur_event]->res_net || call_list[m].node != e_ptrs[cur_event]->res_node))
            continue;
         else {
            next_call = m;
            break;
         }
      }

   if (next_call == -1)
      next_call = 0;

   sprintf (filename, "%sQUEUE.DAT", config->sys_path);
   fd = sh_open (filename, SH_DENYRW, O_WRONLY|O_BINARY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE);
   if (fd != -1) {
      write (fd, (char *)&max_call, sizeof (short));
      write (fd, (char *)&call_list, sizeof (struct _call_list) * max_call);
      close (fd);
   }
}

void get_call_list (void)
{
   int fd = 0, i;
   char filename[80];

   build_call_queue (0);
   for (i = 0; i < max_call; i++) {
      if (call_list[i].type & MAIL_INSPECT) {
         build_node_queue (call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point, i);
         fd = 1;
      }
   }

   if (fd) {
      sprintf (filename, "%sQUEUE.DAT", config->sys_path);
      fd = sh_open (filename, SH_DENYRW, O_WRONLY|O_BINARY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE);
      if (fd != -1) {
         write (fd, (char *)&max_call, sizeof (short));
         write (fd, (char *)&call_list, sizeof (struct _call_list) * max_call);
         close (fd);
      }
   }
}

void rebuild_call_queue (void)
{
   status_line ("+Building the outbound queue");
   build_call_queue (1);
   status_line ("+%d queue record(s) in database", max_call);
}

void get_bad_call (bzone, bnet, bnode, bpoint, rwd)
int bzone, bnet, bnode, bpoint, rwd;
{
   int res, i, j, flags;
   struct ffblk bad_dta;
   char *HoldName, fname[80];

   HoldName = HoldAreaNameMunge (bzone);
   if (bpoint)
      sprintf (fname, "%s%04x%04x.PNT\\%08X.$$?", HoldName, bnet, bnode, bpoint);
   else
      sprintf (fname, "%s%04x%04x.$$?", HoldName, bnet, bnode);
   j = strlen (fname) - 1;
   res = 0;
   flags = 0;

   i = 0;
   if (!findfirst(fname,&bad_dta,0))
      do {
         if (isdigit (bad_dta.ff_name[11])) {
            fname[j] = bad_dta.ff_name[11];
            res = fname[j] - '0';
            break;
         }
      } while (!findnext(&bad_dta));

   call_list[rwd].call_wc = res;

   res = 0;
   flags = 0;

   if ((i = sh_open (fname, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) != -1) {
      read (i, (char *) &res, sizeof (short));
      read (i, (char *) &flags, sizeof (short));
      close (i);
   }

   call_list[rwd].call_nc = res;
   call_list[rwd].flags = flags;
}


void display_outbound_info (start)
int start;
{
   int i, row, v, mc, mnc;
   char j[30];

   if (cur_event > -1) {
      mc = e_ptrs[cur_event]->with_connect ? e_ptrs[cur_event]->with_connect : max_connects;
      mnc = e_ptrs[cur_event]->no_connect ? e_ptrs[cur_event]->no_connect : max_noconnects;
   }
   else
      mc = mnc = 32767;

   row = 14;
   wfill (14, 1, 21, 51, ' ', LCYAN|_BLACK);

   if (max_call) {
      for (i=start;i<max_call && row<22;i++,row++) {
         sprintf (j, "%d:%d/%d.%d", call_list[i].zone, call_list[i].net, call_list[i].node, call_list[i].point);
         wprints (row, 2, LCYAN|_BLACK, j);
         if (call_list[i].size < 10240L)
            sprintf (j, "%ldb ", call_list[i].size);
         else
            sprintf (j, "%ldKb", call_list[i].size / 1024L);
         wrjusts (row, 38, LCYAN|_BLACK, j);
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
         if (call_list[i].type & MAIL_WILLGO)
            j[v++] = 'I';
         j[v] = '\0';
         wrjusts (row, 29, LCYAN|_BLACK, j);

         sprintf (j, "%d", call_list[i].call_nc);
         wrjusts (row, 20, LCYAN|_BLACK, j);
         sprintf (j, "%d", call_list[i].call_wc);
         wrjusts (row, 23, LCYAN|_BLACK, j);

         if (next_call == i)
            wprints (row, 40, YELLOW|_BLACK, "");

         if (cur_event >= 0) {
            if ( !(call_list[i].type & MAIL_WILLGO) ) {
               if ( call_list[i].type & MAIL_UNKNOWN ) {
                  wprints (row, 41, LCYAN|_BLACK, "Unlisted");
                  wprints (row, 40, LCYAN|_BLACK, "*");
               }
               else if ( !(call_list[i].type & (MAIL_CRASH|MAIL_DIRECT|MAIL_NORMAL)) ) {
                  wprints (row, 41, LCYAN|_BLACK, "Hold");
                  wprints (row, 40, LCYAN|_BLACK, "*");
               }
               else if ( e_ptrs[cur_event]->behavior & MAT_NOOUT )
                  wprints (row, 41, LCYAN|_BLACK, "Temp.hold");
               else if ((call_list[i].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_NOCM))
                  wprints (row, 41, LCYAN|_BLACK, "Temp.hold");
               else if (!(call_list[i].type & MAIL_CRASH) && (e_ptrs[cur_event]->behavior & MAT_CM))
                  wprints (row, 41, LCYAN|_BLACK, "Temp.hold");
               else if (e_ptrs[cur_event]->res_net && (call_list[i].net != e_ptrs[cur_event]->res_net || call_list[i].node != e_ptrs[cur_event]->res_node))
                  wprints (row, 41, LCYAN|_BLACK, "Temp.hold");
               else if ( call_list[i].call_nc >= mnc || call_list[i].call_wc >= mc ) {
                  wprints (row, 41, LCYAN|_BLACK, "Undialable");
                  wprints (row, 40, LCYAN|_BLACK, "*");
               }
               else {
                  switch (call_list[i].flags) {
                     case NO_CARRIER:
                        wprints (row, 41, LCYAN|_BLACK, "No Carrier");
                        break;
                     case NO_DIALTONE:
                        wprints (row, 41, LCYAN|_BLACK, "No Dialtone");
                        break;
                     case BUSY:
                        wprints (row, 41, LCYAN|_BLACK, "Busy");
                        break;
                     case NO_ANSWER:
                        wprints (row, 41, LCYAN|_BLACK, "No Answer");
                        break;
                     case VOICE:
                        wprints (row, 41, LCYAN|_BLACK, "Voice");
                        break;
                     case ABORTED:
                        wprints (row, 41, LCYAN|_BLACK, "Aborted");
                        break;
                     case TIMEDOUT:
                        wprints (row, 41, LCYAN|_BLACK, "Timeout");
                        break;
                     default:
                        wprints (row, 41, LCYAN|_BLACK, "------");
                        break;
                  }
               }
            }
            else {
               if ( call_list[i].type & MAIL_UNKNOWN ) {
                  wprints (row, 41, LCYAN|_BLACK, "Unlisted");
                  wprints (row, 40, LCYAN|_BLACK, "*");
               }
               else if ( call_list[i].call_nc >= mnc || call_list[i].call_wc >= mc ) {
                  wprints (row, 41, LCYAN|_BLACK, "Undialable");
                  wprints (row, 40, LCYAN|_BLACK, "*");
               }
               else {
                  switch (call_list[i].flags) {
                     case NO_CARRIER:
                        wprints (row, 41, LCYAN|_BLACK, "No Carrier");
                        break;
                     case NO_DIALTONE:
                        wprints (row, 41, LCYAN|_BLACK, "No Dialtone");
                        break;
                     case BUSY:
                        wprints (row, 41, LCYAN|_BLACK, "Busy");
                        break;
                     case NO_ANSWER:
                        wprints (row, 41, LCYAN|_BLACK, "No Answer");
                        break;
                     case VOICE:
                        wprints (row, 41, LCYAN|_BLACK, "Voice");
                        break;
                     case ABORTED:
                        wprints (row, 41, LCYAN|_BLACK, "Aborted");
                        break;
                     case TIMEDOUT:
                        wprints (row, 41, LCYAN|_BLACK, "Timeout");
                        break;
                     default:
                        wprints (row, 41, LCYAN|_BLACK, "------");
                        break;
                  }
               }
            }
         }
      }
   }
   else
      wprints (17, 13, LCYAN|_BLACK, "Nothing in outbound area");
}

int no_dups (zone, net, node, point)
int zone, net, node, point;
{
   int i;

   for(i = 0; i < max_call; i++) {
      if (call_list[i].zone == zone && call_list[i].net == net && call_list[i].node == node && call_list[i].point == point)
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
   showcur();
}

void clear_status()
{
   int wh;

   if (!snooping || local_mode == 2)
      return;

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

   if (!usr.name[0] || !snooping || local_mode == 2)
      return;

   wh = whandle ();
   wactiv (status);
   wfill (0, 0, 1, 64, ' ', BLACK|_LGREY);

   strcpy (jn, usr.name);
   jn[20] = '\0';
   sprintf (j, "%s from %s at %lu baud", jn, usr.city, local_mode ? 0L : rate);
   wprints (0, 1, BLACK|_LGREY, j);

   if (lorainfo.wants_chat)
      wprints(0,58,BLACK|_LGREY|BLINK,"[CHAT]");

   wprints(1,1,BLACK|_LGREY,"Level:");
   for (i=0;i<12;i++)
      if (levels[i].p_length == usr.priv) {
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

   if (!snooping || local_mode == 2)
      return;

   wh = whandle();
   wactiv(status);
   wfill(0,0,1,64,' ',BLACK|_LGREY);

   strcpy(jn, name);
   jn[20] = '\0';
   sprintf(j, "%s of %s at %lu baud", jn, city, local_mode ? 0 : rate);
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

   if (!snooping || local_mode == 2)
      return;

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

char *get_flagA_text (long f)
{
   strcpy (e_input, "--------");
   if (f & 0x80)
      e_input[0] = '0';
   if (f & 0x40)
      e_input[1] = '1';
   if (f & 0x20)
      e_input[2] = '2';
   if (f & 0x10)
      e_input[3] = '3';
   if (f & 0x08)
      e_input[4] = '4';
   if (f & 0x04)
      e_input[5] = '5';
   if (f & 0x02)
      e_input[6] = '6';
   if (f & 0x01)
      e_input[7] = '7';

   return (e_input);
}

char *get_flagB_text (long f)
{
   strcpy (e_input, "--------");
   if (f & 0x80)
      e_input[0] = '8';
   if (f & 0x40)
      e_input[1] = '9';
   if (f & 0x20)
      e_input[2] = 'A';
   if (f & 0x10)
      e_input[3] = 'B';
   if (f & 0x08)
      e_input[4] = 'C';
   if (f & 0x04)
      e_input[5] = 'D';
   if (f & 0x02)
      e_input[6] = 'E';
   if (f & 0x01)
      e_input[7] = 'F';

   return (e_input);
}

char *get_flagC_text (long f)
{
   strcpy (e_input, "--------");
   if (f & 0x80)
      e_input[0] = 'G';
   if (f & 0x40)
      e_input[1] = 'H';
   if (f & 0x20)
      e_input[2] = 'I';
   if (f & 0x10)
      e_input[3] = 'J';
   if (f & 0x08)
      e_input[4] = 'K';
   if (f & 0x04)
      e_input[5] = 'L';
   if (f & 0x02)
      e_input[6] = 'M';
   if (f & 0x01)
      e_input[7] = 'N';

   return (e_input);
}

char *get_flagD_text (long f)
{
   strcpy (e_input, "--------");
   if (f & 0x80)
      e_input[0] = 'O';
   if (f & 0x40)
      e_input[1] = 'P';
   if (f & 0x20)
      e_input[2] = 'Q';
   if (f & 0x10)
      e_input[3] = 'R';
   if (f & 0x08)
      e_input[4] = 'S';
   if (f & 0x04)
      e_input[5] = 'T';
   if (f & 0x02)
      e_input[6] = 'U';
   if (f & 0x01)
      e_input[7] = 'V';

   return (e_input);
}

void f3_status()
{
   int wh;
   char j[80];
   long flag;

   if (!snooping || local_mode == 2)
      return;

   wh = whandle ();
   wactiv (status);
   wfill (0, 0, 1, 64, ' ', BLACK|_LGREY);

   sprintf (j, "Uploads: %ldK, %d files  Downloads: %ldK, %d files", usr.upld, usr.n_upld, usr.dnld, usr.n_dnld);
   wprints (0, 1, BLACK|_LGREY, j);

   flag = usr.flags;

   strcpy (j, "Flags: A: ");
   strcat (j, get_flagA_text ((flag >> 24) & 0xFF));
   strcat (j, " B: ");
   strcat (j, get_flagB_text ((flag >> 16) & 0xFF));
   strcat (j, " C: ");
   strcat (j, get_flagC_text ((flag >> 8) & 0xFF));
   strcat (j, " D: ");
   strcat (j, get_flagD_text (flag & 0xFF));

   wprints (1, 1, BLACK|_LGREY, j);

   wactiv (wh);
   function_active = 3;
}

void f4_status()
{
   int wh, sc;
   char j[80];

   if (!snooping || local_mode == 2)
      return;

   wh = whandle();
   wactiv(status);
   wfill(0,0,1,64,' ',BLACK|_LGREY);

   sprintf(j, "Last Caller: %-28.28s  Total Calls: %lu", lastcall.name, sysinfo.total_calls);
   wprints(0,1,BLACK|_LGREY,j);

   if (locked && password != NULL && registered)
      wprints(1,37,BLACK|_LGREY,"[LOCK]");

   wprints(1,44,BLACK|_LGREY,"[F9]=Help");

   sprintf(j, "[Line: %02d]", line_offset);
   wprints(1,55,BLACK|_LGREY,j);

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

   if (!snooping || local_mode == 2)
      return;

   wh = whandle();
   wactiv(status);
   wfill(0,0,1,64,' ',BLACK|_LGREY);

   wprints(0,1,BLACK|_LGREY,"ALT: [H]angup [J]Shell [L]ockOut [C]hat [S]ecurity [N]erd");
   wprints(1,1,BLACK|_LGREY,"                       -Inc/-Dec Time [F1]-[F4]=Extra");

   wactiv(wh);
   function_active = 9;
}

#define MAXDUPES     1000
#define DUPE_HEADER  (48 + 2 + 2 + 4)
#define DUPE_FOOTER  (MAXDUPES * 4)

struct _dupecheck {
   char  areatag[48];
   short dupe_pos;
   short max_dupes;
   long  area_pos;
//   long  dupes[MAXDUPES];
};

struct _dupeindex {
   char  areatag[48];
   long  area_pos;
};

void system_crash (void)
{
   int fd, fdidx;
   char filename[80], nusers;
   long prev, crc;
   struct _useron useron;
   struct stat statbuf, statidx;
   struct _usr usr;
   struct _usridx usridx;
   struct _dupeindex dupeindex;
   struct _dupecheck dupecheck;
   struct _sys tsys;
   struct _sys_idx sysidx;

   sprintf (filename, USERON_NAME, config->sys_path);
   fd = sh_open (filename, SH_DENYNONE, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);

   prev = 0L;

   sprintf (filename, "%sLORAINFO.T%02X", config->sys_path, line_offset);
   unlink (filename);

   while (read(fd, (char *)&useron, sizeof(struct _useron)) == sizeof(struct _useron)) {
      if (useron.line == line_offset && useron.name[0]) {
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

   sprintf (filename, "%s.BBS", config->user_file);
   fd = shopen(filename, O_RDWR|O_BINARY);
   if (fd == -1)
      fd = cshopen (filename, O_CREAT|O_TRUNC|O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
   fstat (fd, &statbuf);

   if ((statbuf.st_size % (long)sizeof (struct _usr)) != 0L) {
      close (fd);
      status_line ("!Error in %s file", filename);
      get_down (1, 3);
   }

   sprintf (filename, "%s.IDX", config->user_file);
   fdidx = shopen(filename, O_RDWR|O_BINARY);
   if (fdidx == -1)
      fdidx = cshopen (filename, O_CREAT|O_TRUNC|O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
   fstat (fdidx, &statidx);

   if ( (statbuf.st_size / sizeof (struct _usr)) != (statidx.st_size / sizeof (struct _usridx)) ) {
      status_line("!Rebuilding user index file");
      chsize (fdidx, 0L);

      nusers = 0;

      for (;;) {
         prev = tell (fd);
         if (read (fd, (char *)&usr, sizeof(struct _usr)) != sizeof(struct _usr))
            break;

         if (!usr.deleted) {
            usr.id = usridx.id = crc_name (usr.name);
            usr.alias_id = usridx.alias_id = crc_name (usr.handle);
         }
         else {
            usr.id = usridx.id = 0L;
            usr.alias_id = usridx.alias_id = 0L;
         }

         lseek (fd, prev, SEEK_SET);
         write (fd, (char *)&usr, sizeof (struct _usr));
         write (fdidx, (char *)&usridx, sizeof (struct _usridx));

         nusers++;
      }
   }

   close (fd);
   close (fdidx);

   sprintf (filename, "%sDUPES.DAT", config->sys_path);
   fd = sh_open (filename, SH_DENYNONE, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE);
   if (fd != -1) {
      fstat (fd, &statbuf);
      sprintf (filename, "%sDUPES.IDX", config->sys_path);
      fdidx = open (filename, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
      fstat (fdidx, &statidx);

      if ( (statbuf.st_size / (sizeof (struct _dupecheck) + DUPE_FOOTER)) != (statidx.st_size / sizeof (struct _dupeindex)) ) {
         status_line("!Rebuilding duplicate index file");
         chsize (fdidx, 0L);

         while (read (fd, (char *)&dupecheck, DUPE_HEADER) == DUPE_HEADER) {
            memcpy (dupeindex.areatag, dupecheck.areatag, 48);
            dupeindex.area_pos = dupecheck.area_pos;
            write (fdidx, (char *)&dupeindex, sizeof(struct _dupeindex));
            lseek (fd, (long)DUPE_FOOTER, SEEK_CUR);
         }
/*
         while (read (fd, (char *)&dupecheck, sizeof(struct _dupecheck)) == sizeof(struct _dupecheck)) {
            memcpy (dupeindex.areatag, dupecheck.areatag, 48);
            dupeindex.area_pos = dupecheck.area_pos;
            write (fdidx, (char *)&dupeindex, sizeof(struct _dupeindex));
            lseek (fd, (long)DUPE_FOOTER, SEEK_CUR);
         }
*/
      }

      close (fd);
      close (fdidx);
   }

   sprintf (filename, SYSMSG_PATH, config->sys_path);
   fd = sh_open(filename, SH_DENYWR, O_RDONLY|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
   fstat (fd, &statbuf);

   sprintf (filename, "%sSYSMSG.IDX", config->sys_path);
   fdidx = sh_open(filename, SH_DENYWR, O_WRONLY|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
   fstat (fdidx, &statidx);

   if ((statbuf.st_size % (long)SIZEOF_MSGAREA) != 0L) {
      close (fd);
      close (fdidx);
      status_line ("!Error in %s file", filename);
      get_down (1, 3);
   }

   if ( (statbuf.st_size / SIZEOF_MSGAREA) != (statidx.st_size / sizeof (struct _sys_idx)) ) {
      status_line("!Rebuilding message areas index");
      chsize (fdidx, 0L);

      while (read (fd, (char *)&tsys, SIZEOF_MSGAREA) == SIZEOF_MSGAREA) {
         memset ((char *)&sysidx, 0, sizeof (struct _sys_idx));
         sysidx.priv = tsys.msg_priv;
         sysidx.flags = tsys.msg_flags;
         sysidx.area = tsys.msg_num;
         strcpy (sysidx.key, tsys.qwk_name);
         sysidx.sig = tsys.msg_sig;
         write (fdidx, (char *)&sysidx, sizeof (struct _sys_idx));
      }
   }

   close (fd);
   close (fdidx);

   sprintf (filename, "%sSYSFILE.DAT", config->sys_path);
   fd = sh_open(filename, SH_DENYWR, O_RDONLY|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
   fstat (fd, &statbuf);

   sprintf (filename, "%sSYSFILE.IDX", config->sys_path);
   fdidx = sh_open(filename, SH_DENYWR, O_WRONLY|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
   fstat (fdidx, &statidx);

   if ((statbuf.st_size % (long)SIZEOF_FILEAREA) != 0L) {
      close (fd);
      close (fdidx);
      status_line ("!Error in %s file", filename);
      get_down (1, 3);
   }

   if ( (statbuf.st_size / SIZEOF_FILEAREA) != (statidx.st_size / sizeof (struct _sys_idx)) ) {
      status_line("!Rebuilding file areas index");
      chsize (fdidx, 0L);

      while (read (fd, (char *)&tsys.file_name, SIZEOF_FILEAREA) == SIZEOF_FILEAREA) {
         memset ((char *)&sysidx, 0, sizeof (struct _sys_idx));
         sysidx.priv = tsys.file_priv;
         sysidx.flags = tsys.file_flags;
         sysidx.area = tsys.file_num;
         strcpy (sysidx.key, tsys.short_name);
         sysidx.sig = tsys.file_sig;
         write (fdidx, (char *)&sysidx, sizeof (struct _sys_idx));
      }
   }

   close (fd);
   close (fdidx);

   create_quickbase_file ();
   build_nodelist_index (0);
}

void get_last_caller (void)
{
   int fd, fdd, v;
   char filename[80], destname[80];
   struct _lastcall lc;
   long tempo;

   tempo = time (NULL);

   memset ((char *)&lastcall, 0, sizeof (struct _lastcall));

   sprintf (filename, "%sLASTCALL.BBS", config->sys_path);
   fd = cshopen(filename, O_RDWR|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
   if (filelength (fd) > 0L) {
      v = sizeof (struct _lastcall);
      if ((filelength (fd) % (long)v) != 0L)
         v = sizeof (struct _lastcall) - 4;

      sprintf(destname, "%sLASTCALL.NEW", config->sys_path);
      fdd = cshopen(destname, O_RDWR|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);

      memset ((char *)&lc, 0, sizeof (struct _lastcall));

      while (read(fd, (char *)&lc, v) == v) {
         if (lc.timestamp) {
            if (tempo - lc.timestamp < 86400L)
               write (fdd, (char *)&lc, sizeof (struct _lastcall));
         }
      }

      lseek (fd, 0L, SEEK_SET);
      lseek (fdd, 0L, SEEK_SET);

      while (read(fdd, (char *)&lc, sizeof(struct _lastcall)) == sizeof(struct _lastcall))
         write (fd, (char *)&lc, sizeof (struct _lastcall));
      chsize (fd, tell (fd));

      close (fd);
      close (fdd);

      unlink (destname);
   }
   else if (fd != -1)
      close (fd);

   memset ((char *)&lastcall, 0, sizeof (struct _lastcall));

   sprintf(filename, "%sLASTCALL.BBS", config->sys_path);
   fd = cshopen(filename, O_RDONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
   if (filelength (fd) > 0L) {
      while (read(fd, (char *)&lc, sizeof(struct _lastcall)) == sizeof(struct _lastcall)) {
         if (lc.line == line_offset)
            memcpy ((char *)&lastcall, (char *)&lc, sizeof (struct _lastcall));
      }

      close (fd);
   }
   else if (fd != -1)
      close (fd);

   if (!lastcall.name[0])
      strcpy(lastcall.name, msgtxt[M_NEXT_NONE]);
}

void set_last_caller (void)
{
   int fd, i, m;
   char filename[80];
   struct _lastcall lc;

   if (local_mode && usr.priv == SYSOP)
      return;

   sprintf(filename, "%sLASTCALL.BBS", config->sys_path);
   fd = cshopen(filename, O_APPEND|O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);

   memset((char *)&lc, 0, sizeof(struct _lastcall));

   lc.timestamp = time (NULL);
   lc.line = line_offset;
   strcpy(lc.name, usr.name);
   strcpy(lc.city, usr.city);
   lc.baud = local_mode ? 0 : (int)(rate / 100);
   lc.times = usr.times;
   strncpy (lc.logon, &usr.ldate[11], 5);

   data (filename);
   strncpy (lc.logoff, &filename[11], 5);

   write(fd, (char *)&lc, sizeof(struct _lastcall));

   close (fd);

   lc.logon[2] = '\0';
   lc.logoff[2] = '\0';

   i = atoi (lc.logon);
   m = atoi (lc.logoff);
   if (m < i)
      m += 24;

   for (; i <= m; i++) {
      if (linestat.busyperhour[i % 24] < 65435U) {
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

static void create_quickbase_file ()
{
   int fd, creating;
//   int fdidx, fdto;
   word nhdr, nidx, nto;
   char filename[80];
//   struct _msghdr msghdr;
//   struct _msgidx msgidx;
   struct _msginfo msginfo;

   if (fido_msgpath == NULL)
      return;

   creating = 0;

   sprintf(filename,"%sMSGINFO.BBS",fido_msgpath);
   fd = shopen(filename,O_RDONLY|O_BINARY);
   if (fd == -1) {
      fd = cshopen(filename,O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
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
   fd = shopen(filename,O_RDONLY|O_BINARY);
   if (fd == -1) {
      fd = cshopen(filename,O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
      if (fd == -1)
         return;
      creating = 1;
   }
   nidx = (word)(filelength(fd) / sizeof (struct _msgidx));
   close (fd);

   sprintf(filename,"%sMSGHDR.BBS",fido_msgpath);
   fd = shopen(filename,O_RDONLY|O_BINARY);
   if (fd == -1) {
      fd = cshopen(filename,O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
      if (fd == -1)
         return;
      creating = 1;
   }
   nhdr = (word)(filelength(fd) / sizeof (struct _msghdr));
   close (fd);

   sprintf(filename,"%sMSGTOIDX.BBS",fido_msgpath);
   fd = shopen(filename,O_RDONLY|O_BINARY);
   if (fd == -1) {
      fd = cshopen(filename,O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
      if (fd == -1)
         return;
      creating = 1;
   }
   nto = (word)(filelength(fd) / sizeof (struct _msgtoidx));
   close (fd);

   sprintf(filename,"%sMSGTXT.BBS",fido_msgpath);
   fd = shopen(filename,O_RDONLY|O_BINARY);
   if (fd == -1) {
      fd = cshopen(filename,O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
      if (fd == -1)
         return;
      creating = 1;
   }
   close (fd);

   if (creating)
      status_line ("!Creating QuickBBS message base files");
}

void set_security ()
{
   word ch;
   byte news;

   if (local_mode == 2)
      return;

   wactiv (status);
   clear_status ();
   news = usr.priv;

   wprints (0, 1, BLACK|_LGREY, "Current security: ");
   wprints (0, 19, BLACK|_LGREY, get_priv_text (usr.priv));
   wprints (1, 1, BLACK|_LGREY, "New security: ");
   wprints (1, 15, BLACK|_LGREY, get_priv_text (news));

   for (;;) {
      ch = getxch ();
      if ( (ch & 0xFF) != 0 ) {
         ch &= 0xFF;
         ch = toupper (ch);
      }

      if (ch == 0x0D || ch == 0x1B)
         break;

      switch (ch) {
         case 'T':
            news = TWIT;
            wprints (1, 15, BLACK|_LGREY, "           ");
            wprints (1, 15, BLACK|_LGREY, get_priv_text (news));
            break;

         case 'D':
            news = DISGRACE;
            wprints (1, 15, BLACK|_LGREY, "           ");
            wprints (1, 15, BLACK|_LGREY, get_priv_text (news));
            break;

         case 'L':
            news = LIMITED;
            wprints (1, 15, BLACK|_LGREY, "           ");
            wprints (1, 15, BLACK|_LGREY, get_priv_text (news));
            break;

         case 'N':
            news = NORMAL;
            wprints (1, 15, BLACK|_LGREY, "           ");
            wprints (1, 15, BLACK|_LGREY, get_priv_text (news));
            break;

         case 'W':
            news = WORTHY;
            wprints (1, 15, BLACK|_LGREY, "           ");
            wprints (1, 15, BLACK|_LGREY, get_priv_text (news));
            break;

         case 'P':
            news = PRIVIL;
            wprints (1, 15, BLACK|_LGREY, "           ");
            wprints (1, 15, BLACK|_LGREY, get_priv_text (news));
            break;

         case 'F':
            news = FAVORED;
            wprints (1, 15, BLACK|_LGREY, "           ");
            wprints (1, 15, BLACK|_LGREY, get_priv_text (news));
            break;

         case 'E':
            news = EXTRA;
            wprints (1, 15, BLACK|_LGREY, "           ");
            wprints (1, 15, BLACK|_LGREY, get_priv_text (news));
            break;

         case 'C':
            news = CLERK;
            wprints (1, 15, BLACK|_LGREY, "           ");
            wprints (1, 15, BLACK|_LGREY, get_priv_text (news));
            break;

         case 'A':
            news = ASSTSYSOP;
            wprints (1, 15, BLACK|_LGREY, "           ");
            wprints (1, 15, BLACK|_LGREY, get_priv_text (news));
            break;

         case 'S':
            news = SYSOP;
            wprints (1, 15, BLACK|_LGREY, "           ");
            wprints (1, 15, BLACK|_LGREY, get_priv_text (news));
            break;

         case 0x4800:
            if (news == TWIT)
               news = DISGRACE;
            else if (news == DISGRACE)
               news = LIMITED;
            else if (news == LIMITED)
               news = NORMAL;
            else if (news == NORMAL)
               news = WORTHY;
            else if (news == WORTHY)
               news = PRIVIL;
            else if (news == PRIVIL)
               news = FAVORED;
            else if (news == FAVORED)
               news = EXTRA;
            else if (news == EXTRA)
               news = CLERK;
            else if (news == CLERK)
               news = ASSTSYSOP;
            else if (news == ASSTSYSOP)
               news = SYSOP;
            else if (news == SYSOP)
               news = TWIT;
            wprints (1, 15, BLACK|_LGREY, "           ");
            wprints (1, 15, BLACK|_LGREY, get_priv_text (news));
            break;

         case 0x5000:
            if (news == TWIT)
               news = SYSOP;
            else if (news == DISGRACE)
               news = TWIT;
            else if (news == LIMITED)
               news = DISGRACE;
            else if (news == NORMAL)
               news = LIMITED;
            else if (news == WORTHY)
               news = NORMAL;
            else if (news == PRIVIL)
               news = WORTHY;
            else if (news == FAVORED)
               news = PRIVIL;
            else if (news == EXTRA)
               news = FAVORED;
            else if (news == CLERK)
               news = EXTRA;
            else if (news == ASSTSYSOP)
               news = CLERK;
            else if (news == SYSOP)
               news = ASSTSYSOP;
            wprints (1, 15, BLACK|_LGREY, "           ");
            wprints (1, 15, BLACK|_LGREY, get_priv_text (news));
            break;
      }
   }

   if (ch == 0x0D) {
      usr.priv = news;
      usr_class = get_class(usr.priv);
   }

   f3_status ();
   wactiv (mainview);
}

long recover_flags (char *s)
{
   int i;
   long result;

   result = 0L;

   for (i = 0; i < 8; i++) {
      if (s[i] != '-')
         result |= 1L << (7 - i);
   }

   return (result);
}

void set_flags (void)
{
   int i, x;
   char j[10], f1[10], f2[10], f3[10], f4[10];
   long news, flag;

   if (local_mode == 2)
      return;

   wactiv (status);
   clear_status ();

   flag = news = usr.flags;

   wprints (0, 1, BLACK|_LGREY, "Current flags:");
   wprints (0, 16, BLACK|_LGREY, "A:          B:          C:          D:");

   x = 0;
   for (i = 0; i < 8; i++) {
      if (flag & 0x80000000L)
         j[x++] = '0' + i;
      else
         j[x++] = '-';
      flag = flag << 1;
   }
   j[x] = '\0';
   wprints (0, 19, BLACK|_LGREY, j);
   strcpy (f1, j);

   x = 0;
   for (i=0;i<8;i++) {
      if (flag & 0x80000000L)
         j[x++] = ((i<2)?'8':'?') + i;
      else
         j[x++] = '-';
      flag = flag << 1;
   }
   j[x] = '\0';
   wprints (0, 31, BLACK|_LGREY, j);
   strcpy (f2, j);

   x = 0;
   for (i=0;i<8;i++) {
      if (flag & 0x80000000L)
         j[x++] = 'G' + i;
      else
         j[x++] = '-';
      flag = flag << 1;
   }
   j[x] = '\0';
   wprints (0, 43, BLACK|_LGREY, j);
   strcpy (f3, j);

   x = 0;
   for (i=0;i<8;i++) {
      if (flag & 0x80000000L)
         j[x++] = 'O' + i;
      else
         j[x++] = '-';
      flag = flag << 1;
   }
   j[x] = '\0';
   wprints (0, 55, BLACK|_LGREY, j);
   strcpy (f4, j);

   wprints (1, 1, BLACK|_LGREY, "New flags: ");
   wprints (1, 16, BLACK|_LGREY, "A:          B:          C:          D:");

   winpbeg (BLACK|_LGREY, BLACK|_LGREY);
   winpdef (1, 19, f1, "????????", 0, 1, NULL, 0);
   winpdef (1, 31, f2, "????????", 0, 1, NULL, 0);
   winpdef (1, 43, f3, "????????", 0, 1, NULL, 0);
   winpdef (1, 55, f4, "????????", 0, 1, NULL, 0);

   if (winpread () != W_ESCPRESS) {
      news &= 0xFFFFFF00L;
      news |= recover_flags (f4);
      news &= 0xFFFF00FFL;
      news |= recover_flags (f3) << 8;
      news &= 0xFF00FFFFL;
      news |= recover_flags (f2) << 16;
      news &= 0x00FFFFFFL;
      news |= recover_flags (f1) << 24;
      usr.flags = news;
   }

   f3_status ();
   wactiv (mainview);
}

int share_loaded (void)
{
#ifndef __OS2__
   union REGS inregs, outregs;

   inregs.x.ax = 0x1000;
   int86(0x2F, &inregs, &outregs);

   if (outregs.h.al != 0xFF)
      return (0);
#endif

   return (-1);
}

void show_statistics (fmem)
long fmem;
{
   int c;
   double t, u;
   unsigned pp;
#ifndef __OS2__
   long freeram;
#endif
   long t1;
   struct diskfree_t df;
   struct tm *tim;

   if (frontdoor) {
      t1 = time (NULL);
      tim = localtime (&t1);
      fprintf (logf, "\n----------  %s %02d %s %02d, %s\n", wtext[tim->tm_wday], tim->tm_mday, mtext[tim->tm_mon], tim->tm_year % 100, VERSION);
   }
   else
      status_line("+Begin, %s, (task %d)", &VERSION[8], line_offset);

#ifdef __OS2__
   c = getdisk ();
   if (!_dos_getdiskfree (c + 1, &df)) {
      t = (float)df.total_clusters * df.bytes_per_sector * df.sectors_per_cluster / 1048576L;
      u = (float)df.avail_clusters * df.bytes_per_sector * df.sectors_per_cluster / 1048576L;
      pp = (unsigned )(u * 100 / t);
      status_line (":STATS: %c: %.1f of %.1f Mb free, %d%%", (char)(c + 'A'), u, t, pp);
   }
#else
   t1 = farcoreleft ();
   freeram = fmem - t1;
   status_line (":STATS: %ldK used, %ldK available", freeram / 1024L, t1 / 1024L);

   c = getdisk ();
   if (!_dos_getdiskfree (c + 1, &df)) {
      t = (float)df.total_clusters * df.bytes_per_sector * df.sectors_per_cluster / 1048576L;
      u = (float)df.avail_clusters * df.bytes_per_sector * df.sectors_per_cluster / 1048576L;
      pp = (unsigned )(u * 100 / t);
      status_line (":       %c: %.1f of %.1f Mb free, %d%%", (char)(c + 'A'), u, t, pp);
   }
#endif

   if (share_loaded ())
      status_line (":Message base sharing enabled");
}

