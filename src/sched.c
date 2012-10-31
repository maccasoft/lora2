#include <stdio.h>
#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <process.h>
#include <time.h>
#include <alloc.h>
#include <mem.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "tc_utime.h"
#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "zmodem.h"
#include "externs.h"
#include "prototyp.h"

extern int outinfo, blanked;
extern char nomailproc;

void stop_blanking (void);
void rebuild_call_queue (void);
void write_sched (void);

void find_event ()
{
   int cur_day;
   int cur_hour;
   int cur_minute;
   int cur_mday;
   int cur_mon;
   int cur_year;
   int junk;
   int our_time;
   int i;
   char cmds[80];

   /* Get the current day of the week */
   dosdate (&cur_mon, &cur_mday, &cur_year, &cur_day);

   cur_day = 1 << cur_day;

   /* Get the current time in minutes */
   dostime (&cur_hour, &cur_minute, &junk, &junk);
   our_time = (cur_hour % 24) * 60 + (cur_minute % 60);

   cur_event = -1;

   /* Go through the events from top to bottom */
   for (i = 0; i < num_events; i++) {
      if (our_time >= e_ptrs[i]->minute) {
         if ((cur_day & e_ptrs[i]->days) && ((!e_ptrs[i]->day) || (e_ptrs[i]->day == (char)cur_mday)) && ((!e_ptrs[i]->month) || (e_ptrs[i]->month == (char)cur_mon))) {
            if (((our_time - e_ptrs[i]->minute) < e_ptrs[i]->length) || ((our_time == e_ptrs[i]->minute) && (e_ptrs[i]->length == 0)) || ((e_ptrs[i]->behavior & MAT_FORCED) && (e_ptrs[i]->last_ran != cur_mday))) {
               /* Are we not supposed to force old events */
               if (((our_time - e_ptrs[i]->minute) > e_ptrs[i]->length) && (!noforce)) {
                  e_ptrs[i]->last_ran = cur_mday;
                  continue;
               }

               if (e_ptrs[i]->last_ran != cur_mday) {
                  cur_event = i;

                  if (blanked)
                     stop_blanking ();
                  if (e_ptrs[i]->cmd[0])
                     status_line (":Starting Event %d - %s", i + 1, e_ptrs[i]->cmd);
                  else
                     status_line (msgtxt[M_STARTING_EVENT], i + 1);

                  /* Mark that this one is running */
                  e_ptrs[i]->last_ran = cur_mday;

                  /*
                   * Mark that we have not yet skipped it. After all, it just
                   * started! 
                   */
                  e_ptrs[i]->behavior &= ~MAT_SKIP;

                  /* Write out the schedule */
                  write_sched ();
                  write_sysinfo ();
                  read_sysinfo ();

                  if (!nomailproc)
                     if (e_ptrs[i]->echomail & (ECHO_STARTEXPORT|ECHO_STARTIMPORT)) {
                        if (e_ptrs[cur_event]->echomail & (ECHO_RESERVED4))
                           import_tic_files ();
                        if (e_ptrs[i]->echomail & ECHO_STARTIMPORT)
                           process_startup_mail (1);
                        else
                           process_startup_mail (0);
                     }

                  if (e_ptrs[i]->res_net && !(e_ptrs[cur_event]->behavior & MAT_NOOUT)) {
                     if (e_ptrs[cur_event]->echomail & ECHO_FORCECALL) {
                        junk = 'F';
                        if (e_ptrs[i]->behavior & MAT_CM)
                           junk = 'C';
                        sprintf (cmds, "%s%04x%04x.%cLO", HoldAreaNameMunge (e_ptrs[i]->res_zone), e_ptrs[i]->res_net, e_ptrs[i]->res_node, junk);
                        junk = open (cmds, O_WRONLY|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
                        close (junk);
                     }

                     bad_call (e_ptrs[i]->res_net, e_ptrs[i]->res_node, -2, 0);
                  }

                  /* If we are supposed to exit, then do it */
                  if (e_ptrs[i]->errlevel[0]) {
                     status_line (msgtxt[M_EVENT_EXIT], e_ptrs[i]->errlevel[0]);
                     get_down (e_ptrs[i]->errlevel[0], 1);
                  }

                  rebuild_call_queue ();
                  outinfo = 0;
                  display_outbound_info (outinfo);

                  if (e_ptrs[i]->behavior & MAT_RESYNC) {
                     status_line ("+Start remote clock adjustments");
                     poll_galileo (e_ptrs[i]->with_connect, e_ptrs[i]->no_connect);
                     initialize_modem ();
                     local_status(msgtxt[M_SETTING_BAUD]);
                  }

                  cur_event = i;
                  max_connects = e_ptrs[i]->with_connect;
                  max_noconnects = e_ptrs[i]->no_connect;
               }
               else {
                  /* Don't do events that have been exited already */
                  if (e_ptrs[i]->behavior & MAT_SKIP)
                     continue;
               }

               cur_event = i;

               if (e_ptrs[i]->behavior & MAT_NOREQ)
                  no_requests = 1;
               else
                  no_requests = 0;

               if (e_ptrs[i]->behavior & MAT_NOOUTREQ)
                  requests_ok = 0;
               else
                  requests_ok = 1;

               max_connects = e_ptrs[i]->with_connect;
               max_noconnects = e_ptrs[i]->no_connect;
               break;
            }
         }
      }
   }
}

void set_event ()
{
   int cur_day;
   int cur_hour;
   int cur_minute;
   int cur_mday;
   int cur_mon;
   int cur_year;
   int junk;
   int our_time;
   int i;

   /* Get the current day of the week */
   dosdate (&cur_mon, &cur_mday, &cur_year, &cur_day);

   cur_day = 1 << cur_day;

   /* Get the current time in minutes */
   dostime (&cur_hour, &cur_minute, &junk, &junk);
   our_time = (cur_hour % 24) * 60 + (cur_minute % 60);

   cur_event = -1;

/*
   if (cur_mday != hist.which_day)
      {
      }
*/

   /* Go through the events from top to bottom */
   for (i = 0; i < num_events; i++)
      {
      if (our_time >= e_ptrs[i]->minute)
         {
         if ((cur_day & e_ptrs[i]->days) &&
	 ((!e_ptrs[i]->day) || (e_ptrs[i]->day == (char)cur_mday)) &&
	 ((!e_ptrs[i]->month) || (e_ptrs[i]->month == (char)cur_mon)))
            {
            if (((our_time - e_ptrs[i]->minute) < e_ptrs[i]->length) ||
            ((our_time == e_ptrs[i]->minute) && (e_ptrs[i]->length == 0)) ||
                ((e_ptrs[i]->behavior & MAT_FORCED) && (e_ptrs[i]->last_ran != cur_mday)))
               {
               /* Are we not supposed to force old events */
               if (((our_time - e_ptrs[i]->minute) > e_ptrs[i]->length) && (!noforce))
                  {
                  e_ptrs[i]->last_ran = cur_mday;
                  continue;
                  }

               if (e_ptrs[i]->last_ran != cur_mday)
                  {
                  cur_event = i;
                  max_connects = e_ptrs[i]->with_connect;
                  max_noconnects = e_ptrs[i]->no_connect;
                  }
               else
                  {
                  /* Don't do events that have been exited already */
                  if (e_ptrs[i]->behavior & MAT_SKIP)
                     continue;
                  }

               cur_event = i;

               if (e_ptrs[i]->behavior & MAT_NOREQ)
                  {
/*                  matrix_mask &= ~TAKE_REQ;*/
                  no_requests = 1;
                  }
               else
                  {
/*                  matrix_mask |= TAKE_REQ;*/
                  no_requests = 0;
                  }

               if (e_ptrs[i]->behavior & MAT_NOOUTREQ)
                  {
                  requests_ok = 0;
                  }
               else
                  {
                  requests_ok = 1;
                  }

               max_connects = e_ptrs[i]->with_connect;
               max_noconnects = e_ptrs[i]->no_connect;

               break;
               }
            }
         }
      }
}

void read_sched ()
{
   EVENT *sptr;
   struct stat buffer1;
   FILE *f;
   int i;

   if (stat (config->sched_name, &buffer1))
      {
      return;
      }

   if ((sptr = (EVENT *) malloc ((unsigned int) buffer1.st_size)) == NULL)
      {
      return;
      }

   if ((f = fopen (config->sched_name, "rb")) == NULL)
      {
      return;
      }

   (void) fread (sptr, (unsigned int) buffer1.st_size, 1, f);
   got_sched = 1;

   num_events = (int) (buffer1.st_size / sizeof (EVENT));
   for (i = 0; i < num_events; i++)
      {
      e_ptrs[i] = sptr++;
      }

   (void) fclose (f);
   return;
}

void write_sched ()
{
   char temp1[80];
   FILE *f;
   int i;
   struct stat buffer1;
   struct utimbuf times;
   long t;

   /* Get the current time */
   t = time (NULL);

   /* Get the current stat for .Evt file */
   if (!stat (config->sched_name, &buffer1))
      {

      /*
       * If it is newer than current time, we have a problem and we must
       * reset the file date - yucky, but it will probably work 
       */
      if (t < buffer1.st_atime)
         {
         times.actime = buffer1.st_atime;
         times.modtime = buffer1.st_atime;
         }
      else
         {
         times.actime = t;
         times.modtime = t;
         }
      }

   if ((f = fopen (config->sched_name, "wb")) == NULL)
      {
      return;
      }

   for (i = 0; i < num_events; i++)
      {
      /* If it is skipped, but not dynamic, reset it */
      if ((e_ptrs[i]->behavior & MAT_SKIP) &&
          (!(e_ptrs[i]->behavior & MAT_DYNAM)))
         {
         e_ptrs[i]->behavior &= ~MAT_SKIP;
         }

      /* Write this one out */
      (void) fwrite (e_ptrs[i], sizeof (EVENT), 1, f);
      }

   (void) fclose (f);

   (void) utime (temp1, &times);

   return;
}

int time_to_next (skip_bbs)
int skip_bbs;
{
   int cur_day;
   int cur_hour;
   int cur_minute;
   int cur_mday;
   int cur_mon;
   int cur_year;
   int junk;
   int our_time;
   int i;
   int time_to;
   int guess;
   int nmin;

   /* Get the current time in minutes */
   dostime (&cur_hour, &cur_minute, &junk, &junk);
   our_time = cur_hour * 60 + cur_minute;

   /* Get the current day of the week */
   dosdate (&cur_mon, &cur_mday, &cur_year, &cur_day);

   next_event = -1;
   cur_day = 1 << cur_day;

   /* A ridiculous number */
   time_to = 3000;

   /* Go through the events from top to bottom */
   for (i = 0; i < num_events; i++)
      {
      /* If it is the current event, skip it */
      if (cur_event == i)
         continue;

      /* If it is a BBS event, skip it */
      if (skip_bbs && e_ptrs[i]->behavior & MAT_BBS)
         continue;

      /* If it was already run today, skip it */
      if (e_ptrs[i]->last_ran == cur_mday)
         continue;

      /* If it doesn't happen today, skip it */
      if (!(e_ptrs[i]->days & cur_day))
         continue;

      /* If not this day of the month, skip it */
      if ((e_ptrs[i]->day) && (e_ptrs[i]->day != (char)cur_mday))
         continue;
      
      /* If not this month of the year, skip it */
      if ((e_ptrs[i]->month) && (e_ptrs[i]->month != (char)cur_mon))
         continue;

      /* If it is earlier than now, skip it unless it is forced */
      if (e_ptrs[i]->minute <= our_time)
         {
         if (!(e_ptrs[i]->behavior & MAT_FORCED))
            {
            continue;
            }

         /* Hmm, found a forced event that has not executed yet */
         /* Give the guy 2 minutes and call it quits */
         guess = 2;
         }
      else
         {
         /* Calculate how far it is from now */
         guess = e_ptrs[i]->minute - our_time;
         }

      /* If less than closest so far, keep it */
      if (time_to > guess)
         {
         time_to = guess;
         next_event = i;
         }
      }

   /* If we still have nothing, then do it again, starting at midnight */
   if (time_to >= 1441)
      {
      /* Calculate here to midnight */
      nmin = 1440 - our_time;

      /* Go to midnight */
      our_time = 0;

      /* Go to the next possible day */
      cur_day = (int) (((unsigned) cur_day) << 1);
      if (cur_day > DAY_SATURDAY)
         cur_day = DAY_SUNDAY;

      /* Go through the events from top to bottom */
      for (i = 0; i < num_events; i++)
         {
         /* If it is a BBS event, skip it */
         if (skip_bbs && e_ptrs[i]->behavior & MAT_BBS)
            continue;

         /* If it doesn't happen today, skip it */
         if (!(e_ptrs[i]->days & cur_day))
            continue;

         /* If not this day of the month, skip it */
         if ((e_ptrs[i]->day) && (e_ptrs[i]->day != (char)cur_mday))
            continue;
      
         /* If not this month of the year, skip it */
         if ((e_ptrs[i]->month) && (e_ptrs[i]->month != (char)cur_mon))
            continue;

         /* Calculate how far it is from now */
         guess = e_ptrs[i]->minute + nmin;

         /* If less than closest so far, keep it */
         if (time_to > guess)
            {
            time_to = guess;
            next_event = i;
            }
         }
      }

   if (time_to > 1440)
      time_to = 1440;

   if (skip_bbs && (time_to < 1))
      time_to = 1;

   return (time_to);
}

int time_to_next_nonmail (void)
{
   int cur_day;
   int cur_hour;
   int cur_minute;
   int cur_mday;
   int cur_mon;
   int cur_year;
   int junk;
   int our_time;
   int i;
   int time_to;
   int guess;
   int nmin;

   /* Get the current time in minutes */
   dostime (&cur_hour, &cur_minute, &junk, &junk);
   our_time = cur_hour * 60 + cur_minute;

   /* Get the current day of the week */
   dosdate (&cur_mon, &cur_mday, &cur_year, &cur_day);

   next_event = -1;
   cur_day = 1 << cur_day;

   /* A ridiculous number */
   time_to = 3000;

   /* Go through the events from top to bottom */
   for (i = 0; i < num_events; i++)
      {
      /* If it is the current event, skip it */
      if (cur_event == i)
         continue;

      /* If it is a BBS event, skip it */
      if (!(e_ptrs[i]->behavior & MAT_BBS))
         continue;

      /* If it was already run today, skip it */
      if (e_ptrs[i]->last_ran == cur_mday)
         continue;

      /* If it doesn't happen today, skip it */
      if (!(e_ptrs[i]->days & cur_day))
         continue;

      /* If not this day of the month, skip it */
      if ((e_ptrs[i]->day) && (e_ptrs[i]->day != (char)cur_mday))
         continue;
      
      /* If not this month of the year, skip it */
      if ((e_ptrs[i]->month) && (e_ptrs[i]->month != (char)cur_mon))
         continue;

      /* If it is earlier than now, skip it unless it is forced */
      if (e_ptrs[i]->minute <= our_time)
         {
         if (!(e_ptrs[i]->behavior & MAT_FORCED))
            {
            continue;
            }

         /* Hmm, found a forced event that has not executed yet */
         /* Give the guy 2 minutes and call it quits */
         guess = 2;
         }
      else
         {
         /* Calculate how far it is from now */
         guess = e_ptrs[i]->minute - our_time;
         }

      /* If less than closest so far, keep it */
      if (time_to > guess)
         {
         time_to = guess;
         next_event = i;
         }
      }

   /* If we still have nothing, then do it again, starting at midnight */
   if (time_to >= 1441)
      {
      /* Calculate here to midnight */
      nmin = 1440 - our_time;

      /* Go to midnight */
      our_time = 0;

      /* Go to the next possible day */
      cur_day = (int) (((unsigned) cur_day) << 1);
      if (cur_day > DAY_SATURDAY)
         cur_day = DAY_SUNDAY;

      /* Go through the events from top to bottom */
      for (i = 0; i < num_events; i++)
         {
         /* If it is a BBS event, skip it */
         if (!(e_ptrs[i]->behavior & MAT_BBS))
            continue;

         /* If it doesn't happen today, skip it */
         if (!(e_ptrs[i]->days & cur_day))
            continue;

         /* If not this day of the month, skip it */
         if ((e_ptrs[i]->day) && (e_ptrs[i]->day != (char)cur_mday))
            continue;
      
         /* If not this month of the year, skip it */
         if ((e_ptrs[i]->month) && (e_ptrs[i]->month != (char)cur_mon))
            continue;

         /* Calculate how far it is from now */
         guess = e_ptrs[i]->minute + nmin;

         /* If less than closest so far, keep it */
         if (time_to > guess)
            {
            time_to = guess;
            next_event = i;
            }
         }
      }

   if (time_to > 1440)
      time_to = 1440;

   if (time_to < 1)
      time_to = 1;

   return (time_to);
}

