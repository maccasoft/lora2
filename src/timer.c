
/*
  
	This file contains routines to implement a simple multiple
	alarm system.  The routines allow setting any number of alarms,
	and then checking if any one of them has expired.  It also allows
	adding time to an alarm.
*/


#include <stdio.h>
#include <dos.h>

#include "lora.h"
#include "timer.h"
#include "prototyp.h"

void dostime (int *hour, int *min, int *sec, int *hdths)
{
   union REGS r;

   r.h.ah = 0x2c;

   (void) intdos (&r, &r);

   *hour  = r.h.ch;
   *min   = r.h.cl;
   *sec   = r.h.dh;
   *hdths = r.h.dl;
}

void dosdate (int *month, int *mday, int *year, int *wday)
{
   union REGS r;

   r.h.ah = 0x2a;

   (void) intdos (&r, &r);

   *month  = r.x.bx;
   *mday   = r.x.dx;
   *year   = r.x.cx;
   *wday   = r.x.ax;
}

/*
	long timerset (t)
	unsigned int t;
  
	This routine returns a timer variable based on the MS-DOS
	time.  The variable returned is a long which corresponds to
	the MS-DOS time at which the timer will expire.  The variable
	need never be used once set, but to find if the timer has in
	fact expired, it will be necessary to call the timeup function.
	The expire time 't' is in hundredths of a second.
  
	Note: This routine as coded had a granularity of one week. I
	have put the code that implements that inside the flag 
	"HIGH_OVERHEAD". If you want it, go ahead and use it. For
	BT's purposes, (minute) granularity should suffice.
  
*/
long timerset (t)
int t;
{
   long l;

#ifdef HIGH_OVERHEAD
   int l2;

#endif
   int hours, mins, secs, ths;

#ifdef HIGH_OVERHEAD
   extern int week_day ();

#endif

   /* Get the DOS time and day of week */
   dostime (&hours, &mins, &secs, &ths);

#ifdef HIGH_OVERHEAD
   l2 = week_day ();
#endif

   /* Figure out the hundredths of a second so far this week */
   l =

#ifdef HIGH_OVERHEAD
      l2 * PER_DAY +
      (hours % 24) * PER_HOUR +
#endif
      (mins % 60) * PER_MINUTE +
      (secs % 60) * PER_SECOND +
      ths;

   /* Add in the timer value */
   l += t;

   /* Return the alarm off time */
   return (l);
}

/*
        int timeup (t)
        long t;
  
        This routine returns a 1 if the passed timer variable corresponds
        to a timer which has expired, or 0 otherwise.
*/
int timeup (t)
long t;
{
   long l;

   /* Get current time in hundredths */
   l = timerset (0);

   /* If current is less then set by more than max int, then adjust */
   if (l < (t - 65536L))
      {
#ifdef HIGH_OVERHEAD
      l += PER_WEEK;
#else
      l += PER_HOUR;
#endif
      }

   /* Return whether the current is greater than the timer variable */
   return ((l - t) >= 0L);
}
/*
	long addtime (t1, t2)
	long t1;
	unsigned int t2;
  
	This routine adds t2 hundredths of a second to the timer variable
	in t1.  It returns a new timer variable.

        Not needed in BinkleyTerm, commented out.

long addtime (t1, t2)
long t1;
unsigned int t2;
{
   return (t1 + t2);
}
*/
