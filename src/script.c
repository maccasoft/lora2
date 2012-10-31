/*--------------------------------------------------------------------------*/
/* SCRIPT -- interpreter module 					    */
/* Original Module from BT v1.50 -- Arrogantly hacked on by Giampaolo Sica  */
/*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <cxl\cxlwin.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

#define BITS_7          0x02
#define STOP_2          0x04

/*--------------------------------------------------------------------------*/
/*   Define our current script functions for use in our dispatch table.     */
/*--------------------------------------------------------------------------*/

static int      script_baud (void);         /* Set our baud rate to that of remote*/
static int      script_xmit (void);         /* transmit characters out the port   */
static int      script_pattern (void);      /* define a pattern to wait for       */
static int      script_wait (void);         /* wait for a pattern or timeout      */
static int      script_dial (void);         /* dial the whole number at once      */
static int      script_areacode (void);     /* transmit the areacode out the port */
static int      script_phone (void);        /* transmit the phone number          */
static int      script_carrier (void);      /* Test point, must have carrier now  */
static int      script_session (void);      /* Exit script "successfully"         */
static int      script_if (void);           /* Branch based on pattern match      */
static int      script_goto (void);         /* Absolute branch                    */
static int      script_timer (void);        /* Set a master script timeout        */
static int      nextline (char *);
static void     change_bps (int, char *);
static int      get_line (void);

/**		    Added to original module of bt v1.50		     **/
static int      script_dos (void);          /* Execute dos command                */
static int      script_init (void);         /* Send modem init string             */
static int      script_abort (void);        /* Abort script exection in a time wnd*/
static int      script_break (void);        /* Send a line break                  */
static int      script_speed (void);        /* Send in string rate/100            */
static int      script_comm (void);         /* Change communication parameters    */

struct dispatch {
	char    *string;
        int     (*fun) (void);
};

struct dispatch disp_table[] = {
        { "baud", script_baud         },
        { "xmit", script_xmit         },
        { "pattern", script_pattern   },
        { "wait", script_wait         },
        { "dial", script_dial         },
        { "areacode", script_areacode },
        { "phone", script_phone       },
        { "carrier", script_carrier   },
        { "session", script_session   },
        { "if", script_if             },
        { "goto", script_goto         },
        { "timer", script_timer       },
        { "dos", script_dos           },
        { "init", script_init         },
        { "abort", script_abort       },
        { "break", script_break       },
        { "speed", script_speed       },
        { "comm", script_comm         },
        { NULL, NULL                  }
};

char *script_dial_string = NULL;      /* string for 'dial'     */
char *script_phone_string = NULL;     /* string for 'phone'    */
char *script_areacode_string = "          ";  /* string for 'areacode' */

#define PATTERNS 9
#define PATSIZE 22

char pattern[PATTERNS][PATSIZE];     /* 'wait' patterns       */
int  scr_session_flag = 0;   /* set by "session".     */
int  pat_matched = -1;
char *script_function_argument;       /* argument for functions */

#define MAX_LABELS 40
#define MAX_LAB_LEN 20

struct lab {
   char name[MAX_LAB_LEN + 1];
   long foffset;
   int  line;
} labels[MAX_LABELS];

static long offset;
static long script_alarm;   /* used for master timeout */
static int  num_labels = 0;
static int  curline;
static int  rate_at_start;
static FILE *stream;

int do_script (char *phone_number)
{
   int i, j, k, wh;
   char *c, *f;
   char s[64], *t, *tmp;

   /*--------------------------------------------------------------------------*/
   /* Reset everything from possible previous use of this function.            */
   /*--------------------------------------------------------------------------*/

   rate_at_start=rate;
   curline = 0;
   pat_matched = -1;
   num_labels = 0;
   *script_areacode_string = '\0'; /* reset the special strings */
   script_dial_string = script_phone_string = NULL;
   script_alarm = 0L;                      /* reset timeout */
   for (i = 0; i < PATTERNS; i++) {
      pattern[i][0] = 1;
      pattern[i][1] = '\0';   /* and the 'wait' patterns   */
   }
   scr_session_flag = 0;

   /*--------------------------------------------------------------------------*/
   /* Now start doing things with phone number:                                */
   /*     1) construct the name of the script file into e_input                   */
   /*     2) build script_dial_string, script_areacode_string and              */
   /*        script_phone_string                                               */
   /*--------------------------------------------------------------------------*/

   strcpy (e_input, config->net_info);     /* get our current path         */
   t = c = &e_input[strlen (e_input)];/* point past paths             */
   f = phone_number;               /* then get input side       */
   while (*++f != '\"') {  /* look for end of string    */
      if ((*c++ = *f) == '\0')        /* if premature ending,      */
         return (0);
   }
   *c = '\0';                      /* Now we have the file name */
   strcpy (s, t);

   script_dial_string = ++f;       /* dial string is rest of it */

   /* Say what we are doing */
   status_line (":Dialing %s with script \"%s\"", script_dial_string, s);

   c = script_areacode_string;             /* point to area code               */
   tmp = f;
   if (*tmp)                               /* if there's anything left,        */
      for (i = 0; (i < 10) && (*tmp != '\0') && (*tmp != '-'); i++)
         *c++ = *tmp++;          /* copy it for 'areacode'           */
   if (*tmp=='\0')
      *script_areacode_string='\0';   /* There isn't areacode             */
   else {
      *c = '\0';                      /* terminate areacode               */
      f=tmp;
   }
   if (*f && *f++ == '-')  /* If more, and we got '-',  */
      script_phone_string = f;        /* point to phone string     */

   /*--------------------------------------------------------------------------*/
   /* Finally open the script file and start doing some WORK.                  */
   /*--------------------------------------------------------------------------*/

   if ((stream = fopen (e_input, "r")) == NULL) {     /* OK, let's open the file   */
      status_line ("!Could not open script %s", e_input);
      return (0);             /* no file, no work to do    */
   }

   wh = wopen (12, 0, 24, 79, 0, LGREY|_BLACK, LCYAN|_BLACK);
   wactiv (wh);
   wtitle ("SCRIPT PROCESSING", TLEFT, LCYAN|_BLACK);

   k = 0;                  /* default return is "fail"  */
   while (nextline (NULL)) {       /* Now we parse the file ... */
      if (local_kbd == 0x1B) {
         local_kbd = 1;
         break;
      }
      k = 0;                  /* default return is "fail"  */
      for (j = 0; (c = disp_table[j].string) != NULL; j++) {
         i = strlen (c);
         if (strnicmp (e_input, c, i) == 0) {
            script_function_argument = e_input + i + 1;
            k = (*disp_table[j].fun) ();
            break;
         }
      }

      if (script_alarm && timeup(script_alarm))  { /* Check master timer */
         status_line("+Master script timer expired");
         k = 0;
      }

      if (!k || scr_session_flag)     /* get out for failure or    */
         break;                  /* 'session'.                */

      time_release ();
   }

   wclose ();
   fclose (stream);                /* close input file          */

   if (!k)
      status_line ("+Script \"%s\" failed at line %d", s, curline);

   return (k);                       /* return last success/fail  */
}

static int script_xmit()
{
   mdm_sendcmd(script_function_argument);
   return (1);
}

static int script_areacode ()
{
   mdm_sendcmd (script_areacode_string);
   return (1);
}

static int script_phone ()
{
   mdm_sendcmd (script_phone_string);
   return (1);
}

static int script_dial ()
{
   int i;
   char *number;

   if (*script_function_argument)
      number = script_function_argument;
   else
      number = script_dial_string;

   dial_number (0, number);
   i = wait_for_connect (0);

   if (i > 0) {
      if (mdm_flags == NULL)
         status_line (msgtxt[M_READY_CONNECT], rate, "", "");
      else
         status_line (msgtxt[M_READY_CONNECT], rate, "/", mdm_flags);

      if (!lock_baud)
         com_baud (rate);

      return (1);
   }
   else
      return (1);
}

static int script_carrier ()
{
   return (CARRIER);
}

static int script_session ()
{
   ++scr_session_flag;             /* signal end of script */
   return (1);
}

static int script_pattern ()
{
	register int    i, j;
	register char  *c;

	c = script_function_argument;/* copy the pointer   */
	i = atoi (c);                /* get pattern number */
	if (i < 0 || i >= PATTERNS)  /* check bounds */
		return (0);
	c += 2;                      /* skip digit and space */
	for (j = 1; (j <= PATSIZE) && (*c != '\0'); j++)
		pattern[i][j] = *c++;     /* store the pattern */
	pattern[i][j] = '\0';        /* terminate it here */
	return (1);
}

static int script_wait ()
{
	long t1;
	register int i, j;
	register char c;
	int wait;
	int got_it = 0;

	pat_matched = -1;
	wait = 100 * atoi (script_function_argument);

	if (!wait)
		wait = 4000;
	t1 = timerset (wait);

        wtextattr (LGREY|_BLACK);

        while (!timeup (t1) && !KEYPRESS ()) {
		if (script_alarm && timeup(script_alarm))
			break;

		if (!CHAR_AVAIL ()) {
			continue;
		}
		t1 = timerset (wait);     /* reset the timeout         */
		c = MODEM_IN ();          /* get a character           */
		if (!c)
			continue;              /* ignore null characters    */
		if (c >= ' ') {
			putch (c & 0x7f);
		}
		for (i = 0; i < PATTERNS; i++) {
			j = pattern[i][0];
			if (c == pattern[i][j]) {
				++j;
				pattern[i][0] = j;
				if (!pattern[i][j]) {
					++got_it;
					pat_matched = i;
					goto done;
				}
			}
			else {
				pattern[i][0] = 1;  /* back to start of string   */
			}
		}
	}

done:
        wtextattr (LCYAN|_BLACK);

        for (i = 0; i < PATTERNS; i++) {
		pattern[i][0] = 1;        /* reset these for next time */
	}

	return (got_it);
}

static int script_baud ()
{
	int b;

	if(*script_function_argument) {
		if ((b = atoi (script_function_argument)) != 0) {
			if (b%300==0)
				com_baud (rate=b);
			else
			    return(0);
		}
		else
		    com_baud(rate=rate_at_start);
	}

	return (1);
}

static int script_goto ()
{
	int i;

	/* First see if we already found this guy */
	for (i = 0; i < num_labels; i++) {
		if (stricmp(script_function_argument, labels[i].name) == 0) {
			/* We found it */
			fseek(stream, labels[i].foffset, SEEK_SET);
			curline = labels[i].line;
			return (1);
		}
	}

	return(nextline (script_function_argument));
}

static int script_if ()
{
	char *occ;
	int chk_rate;

	if ((occ=strstr(script_function_argument,"BPS"))!=NULL) {
		if (sscanf(occ+3,"%d ",&chk_rate)==EOF)
			return(0);
		if (rate==chk_rate) {
			if ((script_function_argument=strchr(occ,' ')+1)==NULL)
				return(0);
			else
			    return(script_goto());
		}
		else
		    return (1);
	}

	if (atoi (script_function_argument) != pat_matched)
		return (1);

	/* Skip the pattern number and the space */
	script_function_argument += 2;

	return(script_goto ());
}

static int script_timer ()                                 /* Set a master timer */
{
   int i;

   /* If we got a number, set the master timer. Note: this could be
      done many times in the script, allowing you to program timeouts
      on individual parts of the script. */

   i = atoi (script_function_argument);
   if (i)
      script_alarm = timerset (i * 100);

   return (1);
}

static int script_dos ()                                   /* Execute DOS command */
{
   if (system (script_function_argument) == -1)
      return(0);
   return(1);
}

static int script_init()                                   /* Send the normal init string*/
{
   initialize_modem ();
   return(1);
}

static int script_abort ()                               /* Abort script in a time window*/
{
	time_t now;
	struct tm *nws;
	int nh,nm,sh,sm,eh,em,el1,el2;

	if (sscanf(script_function_argument,"%d:%d %d:%d",&sh,&sm,&eh,&em) != 4)
		return(0);

	now=time(0);
	nws=localtime(&now);
	nh=nws->tm_hour;
	nm=nws->tm_min;

	if (sh<=eh)
		el1=(eh-sh)*60+(em-sm);
	else
		el1=(23-sh+eh)*60+(60-sm+em);

	el2=(nh-sh)*60+(nm-sm);

	if (el2<0) {
		if(el1-el2>=1440)
			return(0);
		else
		    return(1);
	}

	return((el2-el1)>0);
}

static int script_break()
{
   long t;
   void do_break (int);

   do_break (1);

   if (*script_function_argument)
      t = timerset (atoi (script_function_argument));
   else
      t = timerset (100);

   while (!timeup (t)) {
      time_release ();
      release_timeslice ();
   }

   do_break (0);

   return (1);
}

static int script_speed()
{
   char str_rate[4];
   register char *c;

   itoa(rate/100,str_rate,10);
   c=str_rate;
   for(;*c;c++)
      SENDBYTE(*c);
   return(1);
}

static int script_comm()
{
   if (strlen(script_function_argument) != 3)
      return(0);

   change_bps (rate,script_function_argument);
   return (1);
}

static int nextline(str)
char *str;
{
	char save[256];

	if (str != NULL)
		strcpy(save, str);
	else
	    save[0] = '\0';

	while (get_line()) {
                if (!isalpha(e_input[0])) {
                        if (e_input[0] != ':') {
				continue;
			}
			else {
				/* It is a label */
				if (num_labels >= MAX_LABELS) {
					status_line("!Too many labels in script");
					return(0);
				}
                                strcpy (labels[num_labels].name, &(e_input[1]));
				labels[num_labels].foffset = offset;
				labels[num_labels].line = curline;
				++num_labels;

                                if (stricmp (&e_input[1], save)) {
					continue;
				}
			}
		}
		else {
			return (1);
		}
	}

	if (!save[0]) {
		return (1);
	}

	return (0);
}

static int get_line()
{
   char string[128];

   if (fgets(e_input, 255, stream) == NULL)
      return (0);

   ++curline;
   e_input[strlen(e_input) - 1] = '\0';
   sprintf(string, "%03d: %s", curline, e_input);
   if (strlen (string) > 77)
      string[77] = '\0';
   wputs (string);
   wputs ("\n");
   offset = ftell(stream);

   return(1);
}

static void change_bps (baud, np)
int baud;
char *np;
{
   register byte prm;

   switch( baud ) {
      case 300:
         prm = BAUD_300;
         break;
      case 1200:
         prm = BAUD_1200;
         break;
      case 2400:
         prm = BAUD_2400;
         break;
      case 4800:
         prm = BAUD_4800;
         break;
      case 9600:
         prm = BAUD_9600;
         break;
      case 19200:
         prm = BAUD_19200;
         break;
   }

   prm |= (*np == '7' ? BITS_7 : BITS_8);
   prm |= (*++np == 'O' ? ODD_PARITY : (*np == 'E' ? EVEN_PARITY : NO_PARITY));
   prm |= (*++np == '2' ? STOP_2 : STOP_1); /*default on error: 8N1*/

   Com_ (0x00, prm);
}
