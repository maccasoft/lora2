#include <stdio.h>
#include <ctype.h>
#include <dos.h>
#include <io.h>
#include <share.h>
#include <errno.h>
#include <string.h>
#include <dir.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys\stat.h>

#ifdef __OS2__
#include <conio.h>

#define INCL_DOS
#define INCL_VIO
#include <os2.h>
#else
#include <bios.h>
#endif

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlkey.h>

#include "tc_utime.h"
#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

#define isLORA       0x4E
#include "version.h"

char *firstchar(char *, char *, int);

#ifdef __OS2__
void pokeb (unsigned segment, unsigned offset, unsigned char value);
unsigned char peekb (unsigned segment, unsigned offset);
void VioUpdate (void);

extern int errno = 0;
#endif

int com_online (void)
{
   long t;

   if (!config->dcd_timeout)
      return (local_mode || (Com_(0x03) & config->carrier_mask));

   if (local_mode || (Com_(0x03) & config->carrier_mask))
      return (-1);

   t = timerset (config->dcd_timeout * 100);
   while (!timeup (t)) {
      if (Com_(0x03) & config->carrier_mask)
         return (-1);
   }

   return (0);
}

int esc_pressed (void)
{
   time_release ();
   if (local_kbd == 0x1B) {
      local_kbd = -1;
      return (1);
   }

   return (0);
}

/*
#ifdef __OS2__
   if (kbhit ())
      if (getch () == 0x1B)
         return (1);

   return (0);
#else
   if (bioskey (1))
      if ((bioskey (0) & 0xFF) == 0x1B)
         return (1);

   return (0);
#endif
}
*/

/* always uses modification time */
int cdecl utime(char *name, struct utimbuf *times)
{
   int handle;
   struct date d;
   struct time t;
   struct ftime ft;

   unixtodos(times->modtime, &d, &t);
   ft.ft_tsec = t.ti_sec / 2;
   ft.ft_min = t.ti_min;
   ft.ft_hour = t.ti_hour;
   ft.ft_day = d.da_day;
   ft.ft_month = d.da_mon;
   ft.ft_year = d.da_year - 1980;
   if ((handle = shopen(name, O_RDONLY)) == -1)
      return -1;

   setftime(handle, &ft);
   close(handle);
   return 0;
}

char *fancy_str(s)
char *s;
{
	int m, flag=0;

    if (s == NULL)
       return (NULL);

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

    return (s);
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
        if(time_gone > config->class[usr_class].max_time)
                time_gone=config->class[usr_class].max_time-this_call;
*/

	return(time_gone);
}

void show_controls(i)
int i;
{
   XON_ENABLE();
   _BRK_ENABLE();

   change_attr(i);
   m_print(bbstxt[B_CONTROLS]);
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
   if (zone != (int) config->alias[0].zone && zone) {
      sprintf(q, ".%03x", zone);
      q[4] = '\0';
   }

//   mkdir (hold_area);
   strcat (hold_area, "\\");

   return (hold_area);
}

char *HoldAreaNameMungeCreate (zone)
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
   if (zone != (int) config->alias[0].zone && zone)
      sprintf(q, ".%03x", zone);

   mkdir (hold_area);
   strcat (hold_area, "\\");

   return (hold_area);
}

char *stristr (s, p)
char *s, *p;
{
   int i, m;

   if (s == NULL || p == NULL)
      return (NULL);

   m = strlen (p);

   for (i = 0; s[i] != '\0'; i++) {
      if (toupper (s[i]) != toupper (*p))
         continue;
      if (!strnicmp (&s[i], p, m))
         break;
   }

   if (s[i] != '\0')
      return (&s[i]);

   return (NULL);
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
   char string[16];

   if (v)
      t = (i * 100L) / v;
   else
      t = 100;

   sprintf (string, "\b\b\b\b%3ld%%", t);
   m_print (string);

#ifdef __OS2__
   UNBUFFER_BYTES ();
   VioUpdate ();
#endif
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

void parse_netnode (char *netnode, int *zo, int *ne, int *no, int *po)
{
   char *p;
   unsigned short *zone, *net, *node, *point;

   zone = (unsigned short *)zo;
   net = (unsigned short *)ne;
   node = (unsigned short *)no;
   point = (unsigned short *)po;

   *zone = config->alias[0].zone;
   *net = config->alias[0].net;
   *node = 0;
   *point = 0;

   p = netnode;
   while (!isdigit (*p) && *p != '.' && *p)
      p++;
   if (*p == '\0')
      return;

   /* If we have a zone (and the caller wants the zone to be passed back).. */

   if (strchr(netnode,':') && zone) {
      *zone = (unsigned short)atoi(p);
      p = firstchar(p,":",2);
   }

   /* If we have a net number... */

   if (p && strchr(netnode,'/') && net) {
      *net=(unsigned short)atoi(p);
      p=firstchar(p,"/",2);
   }

   /* We *always* need a node number... */

   if (p && node)
      *node=(unsigned short)atoi(p);

   /* And finally check for a point number... */

   if (p && strchr(netnode,'.') && point) {
      p=firstchar(p,".",2);

      if (p)
         *point=(unsigned short)atoi(p);
      else
         *point=0;
   }
}

void parse_netnode2 (char *netnode, int *zo, int *ne, int *no, int *po)
{
   char *p;
   short *zone, *net, *node, *point;

   zone = (short *)zo;
   net = (short *)ne;
   node = (short *)no;
   point = (short *)po;

   p = netnode;
   while (!isdigit (*p) && *p != '.' && *p)
      p++;
   if (*p == '\0')
      return;

   /* If we have a zone (and the caller wants the zone to be passed back).. */

   if (strchr (netnode, ':')) {
      *zone = (unsigned short)atoi(p);
      p = firstchar(p, ":", 2);
   }

   /* If we have a net number... */

   if (p && strchr (p, '/')) {
      *net = (unsigned short)atoi (p);
      p = firstchar (p, "/", 2);
   }
   else if (!stricmp (p, "ALL")) {
      *net = -1;
      *node = -1;
      *point = -1;
      return;
   }

   /* We *always* need a node number... */

   if (p && *p != '.')
      if (!stricmp (p, "ALL"))
         *node = -1;
      else
         *node = (unsigned short)atoi(p);
   else if (p == NULL)
      *node = 0;

   /* And finally check for a point number... */

   if (p && strchr (p, '.')) {
      if (*p == '.')
         p=firstchar (p, ".", 1);
      else
         p=firstchar (p, ".", 2);

      if (p) {
         if (!stricmp (p, "ALL"))
            *point = -1;
         else
            *point = (unsigned short)atoi(p);
      }
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

int scrollbox(int sy,int sx,int ey,int ex,int num,int direction)
{
#ifdef __OS2__
    register unsigned i,y;
    unsigned p,d,inrow;

    inrow=_vinfo.numcols*2;
    d=(ex-sx+1)*2;

    if (direction==SUP)
        while (num--) {
            for(y=sy;y<ey;y++) {
                p=((y*_vinfo.numcols)+sx)*2;
                for(i=0;i<d;i++) {
                    pokeb(0,p,peekb(0,p+inrow));
                    p++;
                }
            }
            p=((y*_vinfo.numcols)+sx)*2;
            d=ex-sx+1;
            for(i=0;i<d;i++) {
                pokeb(0,p++,' ');
                pokeb(0,p++,7);
            }
        }
    else {
        while (num--) {
            for(y=ey;y>sy;y--) {
                p=((y*_vinfo.numcols)+sx)*2;
                for(i=0;i<d;i++) {
                    pokeb(0,p,peekb(0,p-inrow));
                    p++;
                }
            }
            p=((y*_vinfo.numcols)+sx)*2;
            d=ex-sx+1;
            for(i=0;i<d;i++) {
                pokeb(0,p++,' ');
                pokeb(0,p++,7);
            }
        }
    }

    VioUpdate();

   /* return with no error */
   return(W_NOERROR);
#else
    union REGS regs;

    /* use BIOS function call 6 (up) or 7 (down) to scroll window */
    regs.h.bh=0x07;
    regs.h.ch=sy;
    regs.h.cl=sx;
    regs.h.dh=ey;
    regs.h.dl=ex;
    regs.h.al=num;
    regs.h.ah=((direction==SDOWN)?7:6);
    int86(0x10,&regs,&regs);

    /* return with no error */
    return(W_NOERROR);
#endif
}

int isbundle (char *name)
{
   int i, n;

   if ((n = strlen (name)) < 12)
      return (0);

   strupr (name);

   for (i = n - 12; i < n - 4; i++) {
      if ((!isdigit (name[i])) && ((name[i] > 'F') || (name[i] < 'A')))
         return (0);
   }

   return ((strstr (name, ".MO") || strstr (name, ".TU") || strstr (name, ".WE") || strstr (name, ".TH") || strstr (name, ".FR") || strstr (name, ".SA") || strstr (name, ".SU")));
}

void release_timeslice ()
{
#ifndef __OS2__
   if (have_dv)
      dv_pause ();
   else if (have_ddos)
      ddos_pause ();
   else if (have_tv)
      tv_pause ();
   else if (have_ml)
      ml_pause ();
   else if (have_os2)
      os2_pause ();
   else
      msdos_pause ();
#else
   DosSleep (5L);
#endif
}

void stripcrlf (char *linea)
{
   while (linea[strlen (linea) -1] == 0x0D || linea[strlen (linea) - 1] == 0x0A)
      linea[strlen (linea) -1] = '\0';
}

FILE *sh_fopen (char *filename, char *access, int shmode)
{
   FILE *fp;
   long t1, t2;
   long time (long *);

   t1 = time (NULL);

   while (time (NULL) < t1 + 20) {
      if ((fp = _fsopen (filename, access, shmode)) != NULL || errno != EACCES)
         break;
      t2 = time (NULL);
#ifdef __OS2__
      while (time (NULL) < t2 + 1)
         DosSleep (100L);
#else
      while (time (NULL) < t2 + 1)
         ;
#endif
   }

   return (fp);
}

int sh_open (char *file, int shmode, int omode, int fmode)
{
   int i;
   long t1, t2;
   long time (long *);

   t1 = time (NULL);
   while (time (NULL) < t1 + 20) {
      if ((i = sopen (file, omode, shmode, fmode)) != -1 || errno != EACCES)
         break;
      t2 = time (NULL);
#ifdef __OS2__
      while (time (NULL) < t2 + 1)
         DosSleep (100L);
#else
      while (time (NULL) < t2 + 1)
         ;
#endif
   }

   return (i);
}

/*---------------------------------------------------------------------------

---------------------------------------------------------------------------*/
char *packet_fgets (dest, max, fp)
char *dest;
int max;
FILE *fp;
{
   int i, c;

   i = 0;

   while (i < max) {
      if ((c = fgetc (fp)) == EOF) {
         if (i == 0)
            return (NULL);
         else
            break;
      }

      dest[i] = (char )c;
      if ((char )c == '\0') {
         if (i > 0)
            ungetc (c, fp);
         break;
      }

      i++;
      if ((char )c == 0x0D) {
         if ((c = fgetc (fp)) == 0x0A)
            dest[i++] = (char )c;
         else
            ungetc (c, fp);
         break;
      }
   }

   dest[i] = '\0';
   return (dest);
}

int m_getch (void)
{
   int i;

   if (!local_mode) {
      if (CHAR_AVAIL ())
         return (TIMED_READ (1) & 0xFF);
      if (local_kbd == -1)
         time_release ();
      else {
         i = local_kbd;
         local_kbd = -1;
         return (i);
      }
   }
   else {
      if (local_kbd == -1)
         time_release ();
      else {
         i = local_kbd;
         local_kbd = -1;
         return (i);
      }
   }

   return (-1);
}

int get_emsi_field (char *s)
{
   char c;
   int i = 0, start = 0, value;
   long t;

   t = timerset (100);

   while (CARRIER && !timeup (t)) {
      while (PEEKBYTE () == -1) {
         if (!CARRIER || timeup (t))
            return (0);
      }

      c = (char)TIMED_READ (1);
      t = timerset (100);

      if (!start && c != '{')
         continue;

      if (c == '{' && !start) {
         start = 1;
         continue;
      }

      if (c == '}' && start) {
         if (PEEKBYTE () != '}')
            break;
         else
            c = (char)TIMED_READ (1);
      }

      if (c == ']') {
         if (PEEKBYTE () == ']')
            c = (char)TIMED_READ (1);
      }

      if (c == '\\') {
         if ((c = (char)TIMED_READ (1)) != '\\') {
            c = toupper (c);
            value = (c >= 'A') ? (c - 55) : (c - '0');
            value *= 16;
            c = (char)TIMED_READ (1);
            c = toupper (c);
            value += (c >= 'A') ? (c - 55) : (c - '0');
            c = (char)value;
         }
      }

      s[i++] = c;
   }

   s[i] = '\0';
   return (1);
}

#define x32crc(c,crc) (cr3tab[((int) crc ^ c) & 0xff] ^ ((crc >> 8) & 0x00FFFFFFL))

unsigned long get_buffer_crc (void *buffer, int length)
{
   int i;
   unsigned long crc = 0xFFFFFFFFL;
   unsigned char *b;

   b = (unsigned char *)buffer;

   for (i = 0; i < length; i++)
      crc = x32crc (*b++, crc);

   return (crc);
}

int open_packet (int zone, int net, int node, int point, int ai)
{
   int mi;
   char buff[80], *p;
   struct _pkthdr2 pkthdr;

   p = HoldAreaNameMungeCreate (zone);
   if (point)
      sprintf (buff, "%s%04x%04x.PNT\\%08X.XPR", p, net, node, point);
   else
      sprintf (buff, "%s%04x%04x.XPR", p, net, node);

   mi = open (buff, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
   if (mi == -1 && point) {
      sprintf (buff, "%s%04x%04x.PNT", p, net, node);
      mkdir (buff);
      sprintf (buff, "%s%04x%04x.PNT\\%08X.XPR", p, net, node, point);
      mi = open (buff, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
   }

   if (filelength (mi) > 0L)
      lseek (mi, filelength (mi) - 2, SEEK_SET);
   else {
      memset ((char *)&pkthdr, 0, sizeof (struct _pkthdr2));
      pkthdr.ver = PKTVER;
      pkthdr.product = 0x4E;
      pkthdr.serial = ((MAJVERSION << 4) | MINVERSION);
      pkthdr.capability = 1;
      pkthdr.cwvalidation = 256;
      if (config->alias[ai].point && config->alias[ai].fakenet) {
         pkthdr.orig_node = config->alias[ai].point;
         pkthdr.orig_net = config->alias[ai].fakenet;
         pkthdr.orig_point = 0;
      }
      else {
         pkthdr.orig_node = config->alias[ai].node;
         pkthdr.orig_net = config->alias[ai].net;
         pkthdr.orig_point = config->alias[ai].point;
      }
      pkthdr.orig_zone = config->alias[ai].zone;
      pkthdr.orig_zone2 = config->alias[ai].zone;

      pkthdr.dest_point = point;
      pkthdr.dest_node = node;
      pkthdr.dest_net = net;
      pkthdr.dest_zone = zone;
      pkthdr.dest_zone2 = zone;

      add_packet_pw (&pkthdr);
      write (mi, (char *)&pkthdr, sizeof (struct _pkthdr2));
   }

   return (mi);
}

int prep_match (char *template, char *dep)
{
   register int i,delim;
   register char *sptr;
   int start;

   memset (dep, 0, 15);
   sptr = template;

   for(start=i=0; sptr[i]; i++)
      if ((sptr[i]=='\\') || (sptr[i]==':'))
         start = i+1;

   if(start)
      sptr += start;
   delim = 8;

   strupr(sptr);

   for(i=0; *sptr && i < 12; sptr++)
      switch(*sptr) {
         case '.':
            if (i>8)
               return(-1);
            while(i<8)
               dep[i++] = ' ';
            dep[i++] = *sptr;
            delim = 12;
            break;
         case '*':
            while(i<delim)
               dep[i++] = '?';
            break;
         default:
            dep[i++] = *sptr;
            break;
      }

   while(i<12) {
      if (i == 8)
         dep[i++] = '.';
      else
         dep[i++] = ' ';
   }
   dep[i] = '\0';

   return 0;
}

int match (char *s1, char *s2)
{
   register char *i,*j;

   i = s1;
   j = s2;

   while(*i) {
      if((*j != '?') && (*i != *j))
         return(1);
      i++;
      j++;
   }

   return 0;
}


