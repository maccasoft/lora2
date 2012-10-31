#include <stdio.h>
#include <ctype.h>
#include <dos.h>
#include <string.h>
#include <dir.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <alloc.h>
#include <stdlib.h>

#include <cxl\cxlstr.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"

char *firstchar(char *, char *, int);


void timer (decs)
int decs;
{
	long timeout;

	timeout = timerset ((unsigned int) (decs * 10));
	while (!timeup (timeout))
                time_release();
}

char *fancy_str(s)
char *s;
{
	int m, flag=0;

	for(m=0;m<80;m++) {
		if(s[m] == '\0')
			return (s);
                if(s[m] == ' ' || s[m] == '_' || s[m] == '\'' ||
                   s[m] == '(' || s[m] == '/' || s[m] == '\\')
                {
			flag=0;
			continue;
		}
		if(flag == 0) {
			s[m]=toupper(s[m]);
			flag=1;
		}
		else
			s[m]=tolower(s[m]);
	}

	return(s);
}

void press_enter()
{
	char stringa[2];

        m_print("%s", bbstxt[B_PRESS_ENTER]);
        chars_input(stringa,0,INPUT_NOLF);

        m_print("\r");
        space (strlen (bbstxt[B_PRESS_ENTER]) + 1);
        m_print("\r");
        restore_last_color ();
}

int continua()
{
        int i;

        if (nopause)
		return(1);

        m_print(bbstxt[B_MORE]);
        i = yesno_question(DEF_YES|EQUAL|NO_LF);

        m_print("\r");
        space (strlen (bbstxt[B_MORE]) + 14);
        m_print("\r");
        restore_last_color ();

        if(i == DEF_NO)
        {
                nopause = 0;
		return(0);
	}

        if (i == EQUAL)
                nopause = 1;

        return(1);
}

int more_question(line)
int line;
{
        if(!(line++ < (usr.len-1)) && usr.more) {
		line = 1;
                if(!continua())
			return(0);
	}

	return(line);
}


char *data(d)
char *d;
{
	long tempo;
	struct tm *tim;

	tempo=time(0);
	tim=localtime(&tempo);

        sprintf(d,"%2d %3s %02d  %2d:%02d:%02d",tim->tm_mday,mtext[tim->tm_mon],\
			tim->tm_year,tim->tm_hour,tim->tm_min,tim->tm_sec);
	return(d);
}

int dexists (filename)
char *filename;
{
   struct ffblk dta;

   return (!findfirst (filename, &dta, FA_DIREC));
}

char *get_string(s1, s2)
char *s1, *s2;
{
	int i=0, m=0;

	while(s1[m] == ' ')
		m++;
	while(s1[m] != ' ' && s1[m] != '\0')
		s2[i++]=s1[m++];
	s2[i]='\0';
	if(i == 0)
		return(NULL);
	i=0;
	while(s1[m] != '\0')
		s1[i++]=s1[m++];
	s1[i]='\0';

	return(s2);
}

char *get_fancy_string(s1, s2)
char *s1, *s2;
{
	return(fancy_str(get_string(s1, s2)));
}

char *get_number(s1, s2)
char *s1, *s2;
{
	int i=0, m=0;

	while(s1[m] == ' ')
		m++;
	while(isdigit(s1[m]) && s1[m] != '\0')
		s2[i++]=s1[m++];
	s2[i]='\0';
	if(i == 0)
		return(NULL);
	i=0;
	while(s1[m] != '\0')
		s1[i++]=s1[m++];
	s1[i]='\0';

	return(s2);
}

int time_remain()
{
	int this_call, time_gone;
        long t;

        this_call=(int)((time(&t) - start_time)/60);
        time_gone=allowed - this_call;
/*
        if(time_gone > class[usr_class].max_time)
                time_gone=class[usr_class].max_time-this_call;
*/

	return(time_gone);
}

void big_pause (secs)
int secs;
{
   long timeout, timerset ();

   timeout = timerset ((unsigned int) (secs * 100));
   while (!timeup (timeout))
      {
      if (PEEKBYTE() != -1)
         break;
      }
}

void show_controls(i)
int i;
{
   XON_ENABLE();
   _BRK_ENABLE();

   change_attr(i);
   m_print(bbstxt[B_CONTROLS]);
}

int yesno_question(def)
int def;
{
        char stringa[4], c, bigyes[4], bigno[4];

        sprintf(bigyes, "%c/%c", toupper(bbstxt[B_YES][0]), tolower(bbstxt[B_NO][0]));
        sprintf(bigno, "%c/%c", tolower(bbstxt[B_YES][0]), toupper(bbstxt[B_NO][0]));

        m_print(" [%s", (def & DEF_YES) ? bigyes : bigno);
        m_print("%s", (def & EQUAL) ? "/=" : "");
        m_print("%s", (def & TAG_FILES) ? "/d" : "");
        m_print("%s", (def & QUESTION) ? "/?" : "");
        m_print("]?  ");

        do {
                m_print ("\b \b");
                if (!cmd_string[0])
                {
                        if (usr.hotkey)
                                cmd_input (stringa, 1);
                        else
                                chars_input (stringa, 1, INPUT_NOLF);

                        c = toupper(stringa[0]);
                }
                else
                {
                        c = toupper(cmd_string[0]);
                        strcpy (&cmd_string[0], &cmd_string[1]);
                        strtrim (cmd_string);
                }
                if (c == '?' && (def & QUESTION))
                        break;
                if (c == '=' && (def & EQUAL))
                        break;
                if (c == 'D' && (def & TAG_FILES))
                        break;
                if (time_remain() <= 0 || !CARRIER)
                        return (DEF_NO);
        } while (c != bigyes[0] && c != bigno[2] && c != '\0');

        if (!(def & NO_LF))
                m_print(bbstxt[B_ONE_CR]);

        if (c == '?' && (def & QUESTION))
                return (QUESTION);

        if (c == '=' && (def & EQUAL))
                return (EQUAL);

        if (c == 'D' && (def & TAG_FILES))
                return (TAG_FILES);

        if (def & DEF_YES) {
                if (c == bigno[2])
                        return (DEF_NO);
                else
                        return (DEF_YES);
        }
        else {
                if (c == bigyes[0])
                        return (DEF_YES);
                else
                        return (DEF_NO);
        }
}

word xcrc(crc,b)
unsigned int crc;
unsigned char b;
{
   register newcrc;
   int i;

   newcrc = crc ^ ((int)b << 8);
   for(i = 0; i < 8; ++i)
      if(newcrc & 0x8000)
         newcrc = (newcrc << 1) ^ 0x1021;
   else
      newcrc = newcrc << 1;
   return(newcrc & 0xFFFF);
}

int get_command_word (dest, len)
char *dest;
{
   char tmp_buf[MAX_CMDLEN];

   if (get_string (cmd_string, tmp_buf) == NULL)
      return (0);

   tmp_buf[len] = '\0';
   strcpy (dest, tmp_buf);

   return (1);
}

int get_entire_string (dest, len)
char *dest;
{
   register int m = 0;
   char tmp_buf[MAX_CMDLEN];

   while (cmd_string[m] == ' ')
      m++;

   if (!cmd_string[m])
      return (0);

   strcpy (tmp_buf, &cmd_string[m]);
   tmp_buf[len] = cmd_string[0] = '\0';
   strcpy (dest, tmp_buf);

   return (1);
}

char get_command_letter ()
{
   register int m = 0;
   char c;

   while (cmd_string[m] == ' ')
      m++;

   if (!cmd_string[m])
      return (0);

   c = cmd_string[m];
   strcpy (cmd_string, &cmd_string[++m]);

   return (c);
}

char *HoldAreaNameMunge (zone)
int zone;
{
   register char *q;

   q = hold_area;

   while (*q)
      q++;

   if ( *(q - 5) == '.')
      q -= 5;
   else
      q--;

   *q = '\0';
   if (zone != (int) alias[0].zone && zone)
      sprintf(q, ".%03x", zone);

   mkdir (hold_area);
   strcat (hold_area, "\\");

   return (hold_area);
}

int stristr (s, p)
char *s, *p;
{
        char *a;

        strlwr(s);
        strlwr(p);

        while((a=strchr(s,*p)) != NULL) {
                if(!strncmp(a,p,strlen(p)))
                        return(1);
                s=++a;
        }
        return(0);
}

void ljstring(char *dest,char *src,int len)
{
   int x;
   char *p;

   strncpy(dest,src,len);
   x = strlen(dest);
   p = dest + x;
   while (x < len) {
      *p = ' ';
      p++;
      x++;
   }
}

void display_percentage (i, v)
int i, v;
{
   long t;
   char filename[10], *backs = "\b\b\b\b\b";

   t = (i * 100L) / v;
   sprintf (filename, "%ld%%", t);
   strncat (filename, backs, strlen (filename));
   m_print (filename);
}

int is_here (z, ne, no, pp, forward, maxnodes)
int z, ne, no, pp;
struct _fwrd *forward;
int maxnodes;
{
   register int i;

   for (i = 0; i < maxnodes; i++)
      if (forward[i].zone == z && forward[i].net == ne && forward[i].node == no && forward[i].point == pp)
         return (i);

   return (-1);
}

int mail_sort_func (const void *a1, const void *b1)
{
   struct _fwrd *a, *b;
   a = (struct _fwrd *)a1;
   b = (struct _fwrd *)b1;
   if (a->net != b->net)   return (a->net - b->net);
   return ( (int)(a->node - b->node) );
}

void parse_netnode2(netnode, zone, net, node, point)
char *netnode;
int *zone, *net, *node, *point;
{
   char *p;

   p = netnode;

   /* If we have a zone (and the caller wants the zone to be passed back).. */

   if (strchr (netnode, ':')) {
      *zone = atoi(p);
      p = firstchar(p, ":", 2);
   }

   /* If we have a net number... */

   if (p && strchr (p, '/')) {
      *net = atoi (p);
      p = firstchar (p, "/", 2);
   }
   else if (!stricmp (p, "ALL")) {
      *net = 0;
      *node = 0;
      *point = 0;
      return;
   }

   /* We *always* need a node number... */

   if (p && *p != '.')
      *node = atoi(p);
   else if (p == NULL || !stricmp (p, "ALL"))
      *node = 0;

   /* And finally check for a point number... */

   if (p && strchr (p, '.')) {
      if (*p == '.')
         p=firstchar (p, ".", 1);
      else
         p=firstchar (p, ".", 2);

      if (p)
         *point = atoi(p);
      else
         *point = 0;
   }
   else
      *point = 0;
}


char *firstchar(strng, delim, findword)
char *strng, *delim;
int findword;
{
   int x, isw, sl_d, sl_s, wordno=0;
   char *string, *oldstring;

   /* We can't do *anything* if the string is blank... */

   if (! *strng)
      return NULL;

   string=oldstring=strng;

   sl_d=strlen(delim);

   for (string=strng;*string;string++)
   {
      for (x=0,isw=0;x <= sl_d;x++)
         if (*string==delim[x])
            isw=1;

      if (isw==0) {
         oldstring=string;
         break;
      }
   }

   sl_s=strlen(string);

   for (wordno=0;(string-oldstring) < sl_s;string++)
   {
      for (x=0,isw=0;x <= sl_d;x++)
         if (*string==delim[x])
         {
            isw=1;
            break;
         }

      if (!isw && string==oldstring)
         wordno++;

      if (isw && (string != oldstring))
      {
         for (x=0,isw=0;x <= sl_d;x++) if (*(string+1)==delim[x])
         {
            isw=1;
            break;
         }

         if (isw==0)
            wordno++;
      }

      if (wordno==findword)
         return((string==oldstring || string==oldstring+sl_s) ? string : string+1);
   }

   return NULL;
}


