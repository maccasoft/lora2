#include <stdio.h>
#include <alloc.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <sys\stat.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "quickmsg.h"
#include "pipbase.h"


static char *pip[128]={
"PREFIX","SEEN-BY: ","MSGID: ","PATH: ",": ",
"zion","ment","---","che","chi","ghe","ghi","str","","il","al","ed",
"pr","st","..","  ",", ",". ","; ","++",
"a'","e'","i'","o'","u'","a ","e ",
"i ","o ","u ","nt","hi",
"bb","ba","be","bi","bo","bu",
"cc","ca","ce","ci","co","cu",
"dd","da","de","di","do","du",
"ff","fa","fe","fi","fo","fu",
"gg","ga","ge","gi","go","gu",
"ll","la","le","li","lo","lu",
"mm","ma","me","mi","mo","mu",
"nn","na","ne","ni","no","nu",
"pp","pa","pe","pi","po","pu",
"rr","ra","re","ri","ro","ru",
"ss","sa","se","si","so","su",
"tt","ta","te","ti","to","tu",
"vv","va","ve","vi","vo","vu",
"zz","za","ze","zi","zo","zu",
"==",":-","' ","ha","ho","qu","#"};

#define SET_MARK 1
#define SET_DEL 2
#define SET_NEW 4

static int read0(unsigned char *, FILE *);
static void pipstring(unsigned char *, FILE *);
static void write0(unsigned char *, FILE *);
static int full_pip_msg_read(unsigned int, unsigned int, char);

static void pipstring(s, f)
unsigned char *s;
FILE *f;
{
  register int best;
  char l;
  while (*s)
    {
      if (*s==10)
        s++;
      else
        {
          best=127; l=1;
          switch(*s)
            {
              case ' ': if (*(s+1)==' ') {best=20; l=2;} break;
              case '-':if ((*(s+1)=='-') && (*(s+2)=='-')) {best=7; l=3;} break;
              case '\'': if (*(s+1)==' ') {best=123; l=2;} break;
              case '+': if (*(s+1)=='+') {best=24; l=2;} break;
              case ';': if (*(s+1)==' ') {best=23; l=2;} break;
              case ',': if (*(s+1)==' ') {best=21; l=2;} break;
              case '=': if (*(s+1)=='=') {best=121; l=2;} break;
              case ':': switch(*(s+1)) {
                          case '-': best=122; l=2; break;
                          case ' ': best=4; l=2; break;} break;
              case '.': switch(*(s+1)) {
                          case '.': best=19; l=2; break;
                          case ' ': best=22; l=2; break;} break;
              case 'M':if ((*(s+1)=='S') &&
                           (*(s+2)=='G') &&
                           (*(s+3)=='I') &&
                           (*(s+4)=='D') &&
                           (*(s+5)==':') &&
                           (*(s+6)==' ')) {best=2; l=7;} break;
              case 'P':if ((*(s+1)=='A') &&
                           (*(s+2)=='T') &&
                           (*(s+3)=='H') &&
                           (*(s+4)==':') &&
                           (*(s+5)==' ')) {best=3; l=6;} break;
              case 'S':if ((*(s+1)=='E') &&
                           (*(s+2)=='E') &&
                           (*(s+3)=='N') &&
                           (*(s+4)=='-') &&
                           (*(s+5)=='B') &&
                           (*(s+6)=='Y') &&
                           (*(s+7)==':') &&
                           (*(s+8)==' ')) {best=1; l=9;} break;
              case 'a': switch(*(s+1)) {
                          case ' ': best=30; l=2; break;
                          case 'l': best=15; l=2; break;
                          case '\'': best=25; l=2; break;} break;
              case 'b': switch(*(s+1)) {
                          case 'b': best=37; l=2; break;
                          case 'a': best=38; l=2; break;
                          case 'e': best=39; l=2; break;
                          case 'i': best=40; l=2; break;
                          case 'o': best=41; l=2; break;
                          case 'u': best=42; l=2; break;} break;
              case 'c': switch(*(s+1)) {
                          case 'h': if (*(s+2)=='i') {best=9; l=3;} else if (*(s+2)=='e') {best=8; l=3;}; break;
                          case 'c': best=43; l=2; break;
                          case 'a': best=44; l=2; break;
                          case 'e': best=45; l=2; break;
                          case 'i': best=46; l=2; break;
                          case 'o': best=47; l=2; break;
                          case 'u': best=48; l=2; break;} break;
              case 'd': switch(*(s+1)) {
                          case 'd': best=49; l=2; break;
                          case 'a': best=50; l=2; break;
                          case 'e': best=51; l=2; break;
                          case 'i': best=52; l=2; break;
                          case 'o': best=53; l=2; break;
                          case 'u': best=54; l=2; break;} break;
              case 'e': switch(*(s+1)) {
                          case ' ': best=31; l=2; break;
                          case 'd': best=16; l=2; break;
                          case '\'': best=26; l=2; break;} break;
              case 'f': switch(*(s+1)) {
                          case 'f': best=55; l=2; break;
                          case 'a': best=56; l=2; break;
                          case 'e': best=57; l=2; break;
                          case 'i': best=58; l=2; break;
                          case 'o': best=59; l=2; break;
                          case 'u': best=60; l=2; break;} break;
              case 'g': switch(*(s+1)) {
                          case 'h': if (*(s+2)=='i') {best=11; l=3;} else if (*(s+2)=='e') {best=10; l=3;}; break;
                          case 'g': best=61; l=2; break;
                          case 'a': best=62; l=2; break;
                          case 'e': best=63; l=2; break;
                          case 'i': best=64; l=2; break;
                          case 'o': best=65; l=2; break;
                          case 'u': best=66; l=2; break;} break;
              case 'h': switch(*(s+1)) {
                          case 'i': best=36; l=2; break;
                          case 'a': best=124; l=2; break;
                          case 'o': best=125; l=2; break;} break;
              case 'i': switch(*(s+1)) {
                          case ' ': best=32; l=2; break;
                          case 'l': best=14; l=2; break;
                          case '\'': best=27; l=2; break;} break;
              case 'l': switch(*(s+1)) {
                          case 'l': best=67; l=2; break;
                          case 'a': best=68; l=2; break;
                          case 'e': best=69; l=2; break;
                          case 'i': best=70; l=2; break;
                          case 'o': best=71; l=2; break;
                          case 'u': best=72; l=2; break;} break;
              case 'm': switch(*(s+1)) {
                          case 'm': best=73; l=2; break;
                          case 'a': best=74; l=2; break;
                          case 'e': if((*(s+2)=='n') && (*(s+3)=='t')) {best=6; l=4;} else {best=75; l=2;} break;
                          case 'i': best=76; l=2; break;
                          case 'o': best=77; l=2; break;
                          case 'u': best=78; l=2; break;} break;
              case 'n': switch(*(s+1)) {
                          case 'n': best=79; l=2; break;
                          case 't': best=35; l=2; break;
                          case 'a': best=80; l=2; break;
                          case 'e': best=81; l=2; break;
                          case 'i': best=82; l=2; break;
                          case 'o': best=83; l=2; break;
                          case 'u': best=84; l=2; break;} break;
              case 'o': switch(*(s+1)) {
                          case ' ': best=33; l=2; break;
                          case '\'': best=28; l=2; break;} break;
              case 'p': switch(*(s+1)) {
                          case 'p': best=85; l=2; break;
                          case 'r': best=17; l=2; break;
                          case 'a': best=86; l=2; break;
                          case 'e': best=87; l=2; break;
                          case 'i': best=88; l=2; break;
                          case 'o': best=89; l=2; break;
                          case 'u': best=90; l=2; break;} break;
              case 'q': if (*(s+1)=='u') {best=126; l=2;} break;
              case 'r': switch(*(s+1)) {
                          case 'r': best=91; l=2; break;
                          case 'a': best=92; l=2; break;
                          case 'e': best=93; l=2; break;
                          case 'i': best=94; l=2; break;
                          case 'o': best=95; l=2; break;
                          case 'u': best=96; l=2; break;} break;
              case 's': switch(*(s+1)) {
                          case 's': best=97; l=2; break;
                          case 't': best=18; l=2; if (*(s+2)=='r') {best=12; l=3;} break;
                          case 'a': best=98; l=2; break;
                          case 'e': best=99; l=2; break;
                          case 'i': best=100; l=2; break;
                          case 'o': best=101; l=2; break;
                          case 'u': best=102; l=2; break;} break;
              case 't': switch(*(s+1)) {
                          case 't': best=103; l=2; break;
                          case 'a': best=104; l=2; break;
                          case 'e': best=105; l=2; break;
                          case 'i': best=106; l=2; break;
                          case 'o': best=107; l=2; break;
                          case 'u': best=108; l=2; break;} break;
              case 'u': switch(*(s+1)) {
                          case ' ': best=34; l=2; break;
                          case '\'': best=29; l=2; break;} break;
              case 'v': switch(*(s+1)) {
                          case 'v': best=109; l=2; break;
                          case 'a': best=110; l=2; break;
                          case 'e': best=111; l=2; break;
                          case 'i': best=112; l=2; break;
                          case 'o': best=113; l=2; break;
                          case 'u': best=114; l=2; break;} break;
              case 'z': switch(*(s+1)) {
                          case 'z': best=115; l=2; break;
                          case 'a': best=116; l=2; break;
                          case 'e': best=117; l=2; break;
                          case 'i': best=118; l=2; if ((*(s+2)=='o') && (*(s+3)=='n')) {best=5; l=4;} break;
                          case 'o': best=119; l=2; break;
                          case 'u': best=120; l=2; break;} break;
            }
          if (best==127)
            {if ((*s<128) || (*s==141)) fputc(*s,f); else {fputc(128,f); fputc(*s-127,f);} s++;}
          else
            {fputc(128+best,f); s+=l;}
        }
    }
}


static int read0(s, f)
unsigned char *s;
FILE *f;
{
   register int i = 1;

   while ((*s=fgetc(f)) != 0 && !feof(f) && i < BUFSIZE)
   {
      s++;
      i++;
   }
   if (*s == 0)
      i = 1;
   else
      i = 0;
   *s=0;
   return (i);
}


void write0(s, f)
unsigned char *s;
FILE *f;
{
   while (*s)
   {
      if (*s!=10)
         fputc(*s,f);
      s++;
   }
   fputc(0,f);
}


void pip_scan_message_base(area)
int area;
{
   int f1, i;
   char fn[80];

   sprintf(fn, "%sMPTR%04x.PIP", pip_msgpath, area);
   f1 = shopen(fn, O_RDONLY|O_BINARY);
   num_msg = (int)(filelength(f1) / sizeof(MSGPTR));
   close(f1);

   first_msg = 1;
   last_msg = num_msg;

   for (i=0;i<MAXLREAD;i++)
      if (usr.lastread[i].area == area)
         break;
   if (i != MAXLREAD) {
      if (usr.lastread[i].msg_num > last_msg)
         usr.lastread[i].msg_num = last_msg;
      lastread = usr.lastread[i].msg_num;
   }
   else {
      for (i=0;i<MAXDLREAD;i++)
         if (usr.dynlastread[i].area == area)
            break;
      if (i != MAXDLREAD) {
         if (usr.dynlastread[i].msg_num > last_msg)
            usr.dynlastread[i].msg_num = last_msg;
         lastread = usr.dynlastread[i].msg_num;
      }
      else {
         lastread = 0;
         for (i=1;i<MAXDLREAD;i++) {
            usr.dynlastread[i-1].area = usr.dynlastread[i].area;
            usr.dynlastread[i-1].msg_num = usr.dynlastread[i].msg_num;
         }

         usr.dynlastread[i-1].area = area;
         usr.dynlastread[i-1].msg_num = 0;
      }
   }
}

static int full_pip_msg_read(area, msg, mark)
unsigned int area, msg;
char mark;
{
   FILE *f1,*f2;
   MSGPTR hdr;
   MSGPKTHDR mpkt;
   char buff[86], wrp[80], *p, c, fn[80];
   byte a;
   int line, i, colf, shead, m, z;
   struct _msg msgt;

   msg--;

   sprintf(fn,"%sMPTR%04x.PIP", pip_msgpath,area);
   f1 = fopen(fn,"rb+");

   if (f1 == NULL)
      return (0);

   if (fseek(f1,sizeof(hdr)*msg,SEEK_SET))
   {
      fclose(f1);
      return (0);
   }
   if (fread(&hdr,sizeof(hdr),1,f1)==0)
   {
      fclose(f1);
      return (0);
   }
   if (hdr.status & SET_MPTR_DEL)
   {
      fclose(f1);
      return (0);
   }

   sprintf(fn,"%sMPKT%04x.PIP", pip_msgpath,area);
   f2 = fopen(fn,"rb+");
   if (f2 == NULL)
   {
      fclose(f1);
      return (0);
   }

   if (!(mark&SET_NEW) || !((hdr.status&SET_MPTR_RCVD) || (hdr.status&SET_MPTR_DEL)))
   {
      fseek(f2,hdr.pos,SEEK_SET);
      fread(&mpkt,sizeof(mpkt),1,f2);

      line = 1;
      shead = 0;
      colf = 0;
      memset ((char *)&msgt, 0, sizeof (struct _msg));
      msg_fzone = msg_tzone = alias[sys.use_alias].zone;
      msg_fpoint = msg_tpoint = 0;

      read0(msgt.date,f2);
      read0(msgt.to,f2);
      read0(msgt.from,f2);
      read0(msgt.subj,f2);

      msgt.orig_net = mpkt.fromnet;
      msgt.orig = mpkt.fromnode;
      if (!(mpkt.attribute & SET_PKT_FROMUS))
         msg_fpoint = mpkt.point;

      if (mpkt.attribute & SET_PKT_PRIV)
         msgt.attr |= MSGPRIVATE;
      if (mpkt.attribute & SET_PKT_CRASH)
         msgt.attr |= MSGCRASH;
      if (mpkt.attribute & SET_PKT_ATTACH)
         msgt.attr |= MSGFILE;
      if (mpkt.attribute & SET_PKT_TRANSIT)
         msgt.attr |= MSGFWD;
      if (mpkt.attribute & SET_PKT_KILL)
         msgt.attr |= MSGKILL;
      if (mpkt.attribute & SET_PKT_HOLD)
         msgt.attr |= MSGHOLD;
      if (mpkt.attribute & SET_PKT_REQUEST)
         msgt.attr |= MSGFRQ;
      if (mpkt.attribute & SET_PKT_UPDATE)
         msgt.attr |= MSGURQ;
      if (hdr.status & SET_MPTR_RCVD)
         msgt.attr |= MSGREAD;
      if (hdr.status & SET_MPTR_SENT)
         msgt.attr |= MSGSENT;

      if(msgt.attr & MSGPRIVATE)
      {
         if(stricmp(msgt.from,usr.name) && stricmp(msgt.to,usr.name))
         {
            fclose(f1);
            fclose(f2);
            return(0);
         }
      }

      msgt.dest_net = mpkt.tonet;
      msgt.dest = mpkt.tonode;
      if ( (mpkt.attribute & SET_PKT_FROMUS) )
         msg_tpoint = mpkt.point;

      cls ();
      change_attr(BLUE|_LGREY);
      del_line();
      m_print(" * %s\n", sys.msg_name);

      allow_reply = 1;
      i = 0;

      while ((a=(byte)fgetc(f2)) != 0 && !feof(f2))
      {
         c = a;

         switch(mpkt.pktype)
         {
            case 2:
               c = a;
               break;
            case 10:
               if (a!=10)
               {
                  if (a == 141)
                     a='\r';

                  if (a<128)
                     c=a;
                  else
                  {
                     if (a==128)
                     {
                        a=(byte)fgetc(f2);
                        c=(a)+127;
                     }
                     else
                     {
                        buff[i] = '\0';
                        strcat(buff,pip[a-128]);
                        i += strlen(pip[a-128]);
                        c = buff[--i];
                     }
                  }
               }
               else
                  c='\0';
               break;
            default:
               return(1);
         }

         if (c == '\0')
            continue;
         buff[i++] = c;

         if (c == 0x0D)
         {
            buff[i-1]='\0';

            if(buff[0] == 0x01)
            {
               if (!strncmp(&buff[1],"INTL",4) && !shead)
                  sscanf(&buff[6],"%d:%d/%d %d:%d/%d",&msg_tzone,&i,&i,&msg_fzone,&i,&i);
               if (!strncmp(&buff[1],"TOPT",4) && !shead)
                  sscanf(&buff[6],"%d",&msg_tpoint);
               if (!strncmp(&buff[1],"FMPT",4) && !shead)
                  sscanf(&buff[6],"%d",&msg_fpoint);
               i=0;
               continue;
            }
            else if (!shead)
            {
               line = msg_attrib(&msgt,msg+1,line,0);
               change_attr(LGREY|_BLUE);
               del_line();
               change_attr(CYAN|_BLACK);
               m_print(bbstxt[B_ONE_CR]);
               shead = 1;
               line++;
            }

            if(!strncmp(buff,"SEEN-BY",7))
            {
               i=0;
               continue;
            }

            if(buff[0] == 0x01)
            {
               change_attr(YELLOW|_BLUE);
               m_print("%s\n",&buff[1]);
               change_attr(LGREY|BLACK);
            }
            else
            {
               p = &buff[0];

               if(((strchr(buff,'>') - p) < 6) && (strchr(buff,'>')))
               {
                  if(!colf)
                  {
                     change_attr(LGREY|BLACK);
                     colf=1;
                  }
               }
               else
               {
                  if(colf)
                  {
                     change_attr(CYAN|BLACK);
                     colf=0;
                  }
               }

               if (!strncmp(buff,msgtxt[M_TEAR_LINE],4) || !strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
                  m_print("%s\n",buff);
               else
                  m_print("%s\n",buff);

               if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
                  gather_origin_netnode (buff);
            }

            i=0;

            if(!(++line < (usr.len-1)) && usr.more)
            {
               if(!continua())
               {
                  line = 0;
                  break;
               }
               else
               {
                  while (line > 5) {
                     cpos(line--, 1);
                     del_line();
                  }
               }
            }
         }
         else
         {
            if(i<(usr.width-1))
               continue;

            buff[i]='\0';
            while(i>0 && buff[i] != ' ')
               i--;

            m=0;

            if(i != 0)
               for(z=i+1;z<(usr.width-1);z++)
               {
                  wrp[m++]=buff[z];
                  buff[i]='\0';
               }

            wrp[m]='\0';

            if (!shead)
            {
               line = msg_attrib(&msgt,msg+1,line,0);
               change_attr(LGREY|_BLUE);
               del_line();
               change_attr(CYAN|_BLACK);
               m_print(bbstxt[B_ONE_CR]);
               shead = 1;
               line++;
            }

            if (((strchr(buff,'>') - buff) < 6) && (strchr(buff,'>')))
            {
               if(!colf)
               {
                  change_attr(LGREY|_BLACK);
                  colf=1;
               }
            }
            else
            {
               if(colf)
               {
                  change_attr(CYAN|_BLACK);
                  colf=0;
               }
            }

            if (!strncmp(buff,msgtxt[M_TEAR_LINE],4) || !strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
               m_print("%s\n",buff);
            else
               m_print("%s\n",buff);

            if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
               gather_origin_netnode (buff);

            if(!(++line < (usr.len-1)) && usr.more)
            {
               if(!continua())
               {
                  line = 0;
                  break;
               }
               else
               {
                  while (line > 5) {
                     cpos(line--, 1);
                     del_line();
                  }
               }
            }

            strcpy(buff,wrp);
            i=strlen(wrp);
         }
      }

      if (mark&(SET_MARK|SET_DEL))
      {
          hdr.status|=(mark&SET_DEL)?SET_MPTR_DEL:SET_MPTR_RCVD; /* bit received/deleted set to on*/
          fseek(f1,sizeof(hdr)*msg,SEEK_SET);
          if (fwrite(&hdr,sizeof(hdr),1,f1)==0)
             return (1);
          mpkt.attribute|=(mark&SET_MARK)?SET_PKT_RCVD:0;
          fseek(f2,hdr.pos,SEEK_SET);
          fwrite(&mpkt,sizeof(mpkt),1,f2);
      }
   }

   fclose(f1);
   fclose(f2);

   if (line)
      press_enter ();

   return (1);
}

int pip_msg_read(area, msg, mark, flag)
unsigned int area, msg;
char mark;
int flag;
{
   FILE *f1,*f2;
   MSGPTR hdr;
   MSGPKTHDR mpkt;
   char buff[86], wrp[80], *p, c, fn[80];
   byte a;
   int line, i, colf, shead, m, z;
   struct _msg msgt;

   if (usr.full_read && !flag)
      return(full_pip_msg_read(area, msg, mark));

   msg--;

   sprintf(fn,"%sMPTR%04x.PIP",pip_msgpath,area);
   f1 = fopen(fn,"rb+");

   if (f1 == NULL)
      return (0);

   if (fseek(f1,sizeof(hdr)*msg,SEEK_SET))
   {
      fclose(f1);
      return (0);
   }
   if (fread(&hdr,sizeof(hdr),1,f1)==0)
   {
      fclose(f1);
      return (0);
   }
   if (hdr.status & SET_MPTR_DEL)
   {
      fclose(f1);
      return (0);
   }

   sprintf(fn,"%sMPKT%04x.PIP",pip_msgpath,area);
   f2 = fopen(fn,"rb+");
   if (f2 == NULL)
   {
      fclose(f1);
      return (0);
   }

   if (!(mark&SET_NEW) || !((hdr.status&SET_MPTR_RCVD) || (hdr.status&SET_MPTR_DEL)))
   {
      fseek(f2,hdr.pos,SEEK_SET);
      fread(&mpkt,sizeof(mpkt),1,f2);

      line = 1;
      shead = 0;
      colf = 0;
      memset ((char *)&msgt, 0, sizeof (struct _msg));
      msg_fzone = msg_tzone = alias[sys.use_alias].zone;
      msg_fpoint = msg_tpoint = 0;

      read0(msgt.date,f2);
      read0(msgt.to,f2);
      read0(msgt.from,f2);
      read0(msgt.subj,f2);

      msgt.orig_net = mpkt.fromnet;
      msgt.orig = mpkt.fromnode;
      if (!(mpkt.attribute & SET_PKT_FROMUS))
         msg_fpoint = mpkt.point;

      if (mpkt.attribute & SET_PKT_PRIV)
         msgt.attr |= MSGPRIVATE;
      if (mpkt.attribute & SET_PKT_CRASH)
         msgt.attr |= MSGCRASH;
      if (mpkt.attribute & SET_PKT_ATTACH)
         msgt.attr |= MSGFILE;
      if (mpkt.attribute & SET_PKT_TRANSIT)
         msgt.attr |= MSGFWD;
      if (mpkt.attribute & SET_PKT_KILL)
         msgt.attr |= MSGKILL;
      if (mpkt.attribute & SET_PKT_HOLD)
         msgt.attr |= MSGHOLD;
      if (mpkt.attribute & SET_PKT_REQUEST)
         msgt.attr |= MSGFRQ;
      if (mpkt.attribute & SET_PKT_UPDATE)
         msgt.attr |= MSGURQ;
      if (hdr.status & SET_MPTR_RCVD)
         msgt.attr |= MSGREAD;
      if (hdr.status & SET_MPTR_SENT)
         msgt.attr |= MSGSENT;

      if(msgt.attr & MSGPRIVATE)
      {
         if(stricmp(msgt.from,usr.name) && stricmp(msgt.to,usr.name))
         {
            fclose(f1);
            fclose(f2);
            return(0);
         }
      }

      if (!flag)
         cls ();

      msgt.dest_net = mpkt.tonet;
      msgt.dest = mpkt.tonode;
      if ( (mpkt.attribute & SET_PKT_FROMUS) )
         msg_tpoint = mpkt.point;

      change_attr(CYAN|_BLACK);
      allow_reply = 1;
      i = 0;

      while ((a=(byte)fgetc(f2)) != 0 && !feof(f2))
      {
         c = a;

         switch(mpkt.pktype)
         {
            case 2:
               c = a;
               break;
            case 10:
               if (a!=10)
               {
                  if (a == 141)
                     a='\r';

                  if (a<128)
                     c=a;
                  else
                  {
                     if (a==128)
                     {
                        a=(byte)fgetc(f2);
                        c=(a)+127;
                     }
                     else
                     {
                        buff[i] = '\0';
                        strcat(buff,pip[a-128]);
                        i += strlen(pip[a-128]);
                        c = buff[--i];
                     }
                  }
               }
               else
                  c='\0';
               break;
            default:
               return(1);
         }

         if (c == '\0')
            continue;
         buff[i++] = c;

         if (c == 0x0D)
         {
            buff[i-1]='\0';

            if(buff[0] == 0x01)
            {
               if (!strncmp(&buff[1],"INTL",4) && !shead)
                  sscanf(&buff[6],"%d:%d/%d %d:%d/%d",&msg_tzone,&i,&i,&msg_fzone,&i,&i);
               if (!strncmp(&buff[1],"TOPT",4) && !shead)
                  sscanf(&buff[6],"%d",&msg_tpoint);
               if (!strncmp(&buff[1],"FMPT",4) && !shead)
                  sscanf(&buff[6],"%d",&msg_fpoint);
               i=0;
               continue;
            }
            else if (!shead)
            {
               line = msg_attrib(&msgt,msg+1,line,0);
               m_print(bbstxt[B_ONE_CR]);
               shead = 1;
            }

            if(!strncmp(buff,"SEEN-BY",7))
            {
               i=0;
               continue;
            }

            if(buff[0] == 0x01)
            {
               change_attr(YELLOW|_BLUE);
               m_print("%s\n",&buff[1]);
               change_attr(LGREY|BLACK);
            }
            else
            {
               p = &buff[0];

               if(((strchr(buff,'>') - p) < 6) && (strchr(buff,'>')))
               {
                  if(!colf)
                  {
                     change_attr(LGREY|BLACK);
                     colf=1;
                  }
               }
               else
               {
                  if(colf)
                  {
                     change_attr(CYAN|BLACK);
                     colf=0;
                  }
               }

               if (!strncmp(buff,msgtxt[M_TEAR_LINE],4) || !strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
                  m_print("%s\n",buff);
               else
                  m_print("%s\n",buff);

               if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
                  gather_origin_netnode (buff);
            }

            i=0;

            if(flag == 1)
                    line=1;

            if(!(++line < (usr.len-1)) && usr.more)
            {
               line = 1;
               if(!continua())
               {
                  flag=1;
                  break;
               }
            }
         }
         else
         {
            if(i<(usr.width-1))
               continue;

            buff[i]='\0';
            while(i>0 && buff[i] != ' ')
               i--;

            m=0;

            if(i != 0)
               for(z=i+1;z<(usr.width-1);z++)
               {
                  wrp[m++]=buff[z];
                  buff[i]='\0';
               }

            wrp[m]='\0';

            if (!shead)
            {
               line = msg_attrib(&msgt,msg+1,line,0);
               m_print(bbstxt[B_ONE_CR]);
               shead = 1;
            }

            if (((strchr(buff,'>') - buff) < 6) && (strchr(buff,'>')))
            {
               if(!colf)
               {
                  change_attr(LGREY|_BLACK);
                  colf=1;
               }
            }
            else
            {
               if(colf)
               {
                  change_attr(CYAN|_BLACK);
                  colf=0;
               }
            }

            if (!strncmp(buff,msgtxt[M_TEAR_LINE],4) || !strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
               m_print("%s\n",buff);
            else
               m_print("%s\n",buff);

            if (!strncmp(buff,msgtxt[M_ORIGIN_LINE],11))
               gather_origin_netnode (buff);

            if(flag == 1)
               line=1;

            if(!(++line < (usr.len-1)) && usr.more)
            {
               line = 1;
               if(!continua())
               {
                  flag=1;
                  break;
               }
            }

            strcpy(buff,wrp);
            i=strlen(wrp);
         }
      }

      if (mark&(SET_MARK|SET_DEL))
      {
          hdr.status|=(mark&SET_DEL)?SET_MPTR_DEL:SET_MPTR_RCVD; /* bit received/deleted set to on*/
          fseek(f1,sizeof(hdr)*msg,SEEK_SET);
          if (fwrite(&hdr,sizeof(hdr),1,f1)==0)
             return (1);
          mpkt.attribute|=(mark&SET_MARK)?SET_PKT_RCVD:0;
          fseek(f2,hdr.pos,SEEK_SET);
          fwrite(&mpkt,sizeof(mpkt),1,f2);
      }
   }

   fclose(f1);
   fclose(f2);

   if (line)
      press_enter ();

   return (1);
}

void pip_save_message(txt)
char *txt;
{
   FILE *f1,*f2;
   int i, dest;
   char fn[128];
   MSGPTR hdr;
   MSGPKTHDR mpkt;
   DESTPTR pt;

   m_print(bbstxt[B_SAVE_MESSAGE]);
   pip_scan_message_base(sys.pip_board);
   dest = last_msg + 1;
   activation_key ();
   m_print(" #%d ...",dest);

   sprintf(fn,"%sMPTR%04x.PIP", pip_msgpath, sys.pip_board);
   f1=fopen(fn,"rb+");
   if (f1==NULL)
      f1=fopen(fn,"wb");
   sprintf(fn,"%sMPKT%04x.PIP", pip_msgpath, sys.pip_board);
   f2=fopen(fn,"rb+");
   if (f2==NULL)
      f2=fopen(fn,"wb");

   mpkt.fromnode = alias[sys.use_alias].node;
   mpkt.fromnet = alias[sys.use_alias].net;
   mpkt.pktype = 10;
   mpkt.attribute = SET_PKT_FROMUS;
   mpkt.tonode = msg.dest;
   mpkt.tonet = msg.dest_net;
   mpkt.point = msg_tpoint;

   fseek(f2,filelength(fileno(f2))-2,SEEK_SET);
   hdr.pos=ftell(f2);
   hdr.next=hdr.prev=0;
   hdr.status=SET_MPTR_FROMUS; /* from us */
   fseek(f1,filelength(fileno(f1)),SEEK_SET);
   pt.area=sys.pip_board;
   pt.msg=(int)(ftell(f1)/sizeof(hdr));
   strcpy(pt.to,msg.to);
   pt.next=0;

   if (msg.attr & MSGPRIVATE)
      mpkt.attribute |= SET_PKT_PRIV;
   if (msg.attr & MSGCRASH)
      mpkt.attribute |= SET_PKT_CRASH;
   if (msg.attr & MSGFILE)
      mpkt.attribute |= SET_PKT_ATTACH;
   if (msg.attr & MSGFWD)
      mpkt.attribute |= SET_PKT_TRANSIT;
   if (msg.attr & MSGKILL)
      mpkt.attribute |= SET_PKT_KILL;
   if (msg.attr & MSGHOLD)
      mpkt.attribute |= SET_PKT_HOLD;
   if (msg.attr & MSGFRQ)
      mpkt.attribute |= SET_PKT_REQUEST;
   if (msg.attr & MSGURQ)
      mpkt.attribute |= SET_PKT_UPDATE;
   if (msg.attr & MSGREAD)
      hdr.status |= SET_MPTR_RCVD;
   if (msg.attr & MSGSENT)
      hdr.status |= SET_MPTR_SENT;

   fwrite(&hdr,sizeof hdr,1,f1);
   fwrite(&mpkt,sizeof mpkt,1,f2);
   write0(msg.date,f2);
   write0(msg.to,f2);
   write0(msg.from,f2);
   write0(msg.subj,f2);

   if(sys.netmail)
   {
      if (msg_tpoint)
         fprintf(f2,msgtxt[M_TOPT], msg_tpoint);
      if (msg_tzone != msg_fzone)
         fprintf(f2,msgtxt[M_INTL], msg_tzone, msg.dest_net, msg.dest, msg_fzone, msg.orig_net, msg.orig);
   }

   if(sys.echomail)
   {
      fprintf(f2,msgtxt[M_PID], VERSION, registered ? "" : NOREG);
      fprintf(f2,msgtxt[M_MSGID], alias[sys.use_alias].zone, alias[sys.use_alias].net, alias[sys.use_alias].node, time(NULL));
   }

   if (txt == NULL)
   {
      i = 0;
      while(messaggio[i] != NULL)
      {
         pipstring(messaggio[i++],f2);
         if(messaggio[i][strlen(messaggio[i])-1] == '\r')
            pipstring("\n",f2);
      }
   }
   else
   {
      int fptxt, m;
      char buffer[2050];

      fptxt = shopen(txt, O_RDONLY|O_BINARY);
      if (fptxt != -1)
      {
         do {
            i = read(fptxt, buffer, 2048);
            buffer[i] = '\0';
            for (m=0;m<i;m++)
            {
               if (buffer[m] == 0x1A)
                  buffer[m] = ' ';
            }
            pipstring(buffer,f2);
         } while (i == 2048);

         close(fptxt);
      }
   }

   pipstring("\r\n",f2);

   if (strlen(usr.signature) && registered)
   {
      sprintf(fn,msgtxt[M_SIGNATURE],usr.signature);
      pipstring(fn,f2);
   }

   if(sys.echomail)
   {
      sprintf(fn,msgtxt[M_TEAR_LINE],VERSION, registered ? "" : NOREG);
      pipstring(fn,f2);
      if(strlen(sys.origin))
         sprintf(fn,msgtxt[M_ORIGIN_LINE],sys.origin,alias[sys.use_alias].zone,alias[sys.use_alias].net,alias[sys.use_alias].node);
      else
         sprintf(fn,msgtxt[M_ORIGIN_LINE],system_name,alias[sys.use_alias].zone,alias[sys.use_alias].net,alias[sys.use_alias].node);
      pipstring(fn,f2);
   }

   fputc(0,f2);
   fputc(0,f2);
   fputc(0,f2);

   fclose(f1);
   fclose(f2);

   sprintf(fn,"%sDestPtr.Pip", pip_msgpath);
   f1=fopen(fn,"ab+");
   fwrite(&pt,sizeof pt,1,f1);
   fclose(f1);

   if(sys.echomail && sys.echotag[0])
   {
      f2 = fopen ("ECHOTOSS.LOG", "at");
      fprintf (f2, "%s\n", sys.echotag);
      fclose (f2);
   }

   m_print(bbstxt[B_ONE_CR]);
   status_line(msgtxt[M_INSUFFICIENT_DATA],dest);
   last_msg = dest;
}

void pip_list_headers (m, pip_board, verbose)
int m, pip_board, verbose;
{
   FILE *f1, *f2;
   int i, line=3;
   char fn[80];
   MSGPTR hdr;
   MSGPKTHDR mpkt;
   struct _msg msgt;

   sprintf(fn,"%sMPTR%04x.PIP",pip_msgpath,pip_board);
   f1=fopen(fn,"rb+");
   sprintf(fn,"%sMPKT%04x.PIP",pip_msgpath,pip_board);
   f2=fopen(fn,"rb+");

   for(i = m; i <= last_msg; i++)
   {
      if (fseek(f1,sizeof(hdr)*(i-1),SEEK_SET))
         break;
      if (fread(&hdr,sizeof(hdr),1,f1)==0)
         break;

      memset ((char *)&msgt, 0, sizeof (struct _msg));
      msg_fzone = msg_tzone = alias[sys.use_alias].zone;
      msg_fpoint = msg_tpoint = 0;

      fseek(f2,hdr.pos,SEEK_SET);
      fread(&mpkt,sizeof(mpkt),1,f2);

      read0(msgt.date,f2);
      read0(msgt.to,f2);
      read0(msgt.from,f2);
      read0(msgt.subj,f2);

      msgt.orig_net = mpkt.fromnet;
      msgt.orig = mpkt.fromnode;
      if (!(mpkt.attribute & SET_PKT_FROMUS))
         msg_fpoint = mpkt.point;

      if (mpkt.attribute & SET_PKT_PRIV)
         msgt.attr |= MSGPRIVATE;
      if (mpkt.attribute & SET_PKT_CRASH)
         msgt.attr |= MSGCRASH;
      if (mpkt.attribute & SET_PKT_ATTACH)
         msgt.attr |= MSGFILE;
      if (mpkt.attribute & SET_PKT_TRANSIT)
         msgt.attr |= MSGFWD;
      if (mpkt.attribute & SET_PKT_KILL)
         msgt.attr |= MSGKILL;
      if (mpkt.attribute & SET_PKT_HOLD)
         msgt.attr |= MSGHOLD;
      if (mpkt.attribute & SET_PKT_REQUEST)
         msgt.attr |= MSGFRQ;
      if (mpkt.attribute & SET_PKT_UPDATE)
         msgt.attr |= MSGURQ;
      if (hdr.status & SET_MPTR_RCVD)
         msgt.attr |= MSGREAD;
      if (hdr.status & SET_MPTR_SENT)
         msgt.attr |= MSGSENT;

      msgt.dest_net = mpkt.tonet;
      msgt.dest = mpkt.tonode;
      if ( (mpkt.attribute & SET_PKT_FROMUS) )
         msg_tpoint = mpkt.point;

      if (verbose)
      {
         if ((line = msg_attrib(&msgt,i,line,0)) == 0)
            break;

         m_print(bbstxt[B_ONE_CR]);
      }
      else
         m_print ("%-4d %c%-20.20s %c%-20.20s %-32.32s\n",
                   i,
                   stricmp (msgt.from, usr.name) ? 'Š' : 'Ž',
                   msgt.from,
                   stricmp (msgt.to, usr.name) ? 'Š' : 'Œ',
                   msgt.to,
                   msgt.subj);

      if (!(line=more_question(line)) || !CARRIER && !RECVD_BREAK())
         break;

      time_release();
   }

   fclose(f1); fclose(f2);

   if (line && CARRIER)
      press_enter();
}

int pip_mail_list_header (i, pip_board, line)
int i, pip_board, line;
{
   FILE *f1, *f2;
   char fn[80];
   MSGPTR hdr;
   MSGPKTHDR mpkt;
   struct _msg msgt;

   sprintf(fn,"%sMPTR%04x.PIP",pip_msgpath,pip_board);
   f1=fopen(fn,"rb+");
   sprintf(fn,"%sMPKT%04x.PIP",pip_msgpath,pip_board);
   f2=fopen(fn,"rb+");

   if (fseek(f1,sizeof(hdr)*(i-1),SEEK_SET))
      return (line);
   if (fread(&hdr,sizeof(hdr),1,f1)==0)
      return (line);

   memset ((char *)&msgt, 0, sizeof (struct _msg));
   msg_fzone = msg_tzone = alias[sys.use_alias].zone;
   msg_fpoint = msg_tpoint = 0;

   fseek(f2,hdr.pos,SEEK_SET);
   fread(&mpkt,sizeof(mpkt),1,f2);

   read0(msgt.date,f2);
   read0(msgt.to,f2);
   read0(msgt.from,f2);
   read0(msgt.subj,f2);

   msgt.orig_net = mpkt.fromnet;
   msgt.orig = mpkt.fromnode;
   if (!(mpkt.attribute & SET_PKT_FROMUS))
      msg_fpoint = mpkt.point;

   if (mpkt.attribute & SET_PKT_PRIV)
      msgt.attr |= MSGPRIVATE;
   if (mpkt.attribute & SET_PKT_CRASH)
      msgt.attr |= MSGCRASH;
   if (mpkt.attribute & SET_PKT_ATTACH)
      msgt.attr |= MSGFILE;
   if (mpkt.attribute & SET_PKT_TRANSIT)
      msgt.attr |= MSGFWD;
   if (mpkt.attribute & SET_PKT_KILL)
      msgt.attr |= MSGKILL;
   if (mpkt.attribute & SET_PKT_HOLD)
      msgt.attr |= MSGHOLD;
   if (mpkt.attribute & SET_PKT_REQUEST)
      msgt.attr |= MSGFRQ;
   if (mpkt.attribute & SET_PKT_UPDATE)
      msgt.attr |= MSGURQ;
   if (hdr.status & SET_MPTR_RCVD)
      msgt.attr |= MSGREAD;
   if (hdr.status & SET_MPTR_SENT)
      msgt.attr |= MSGSENT;

   msgt.dest_net = mpkt.tonet;
   msgt.dest = mpkt.tonode;
   if ( (mpkt.attribute & SET_PKT_FROMUS) )
      msg_tpoint = mpkt.point;

   if ((line = msg_attrib(&msgt,i,line,0)) == 0)
      return (0);

   fclose(f1);
   fclose(f2);

   return (line);
}

int pip_write_message_text(msg_num, flags, quote_name, sm)
int msg_num, flags;
char *quote_name;
FILE *sm;
{
   FILE *f1,*f2, *fpq;
   MSGPTR hdr;
   MSGPKTHDR mpkt;
   char buff[86], wrp[80], c, fn[80], shead, qwkbuffer[130];
   byte a;
   int i, m, z, pos, blks;
   long qpos;
   struct _msg msgt;
   struct QWKmsghd QWK;

   msg_num--;
   shead = 0;
   blks = 1;
   pos = 0;

   sprintf(fn,"%sMPTR%04x.PIP",pip_msgpath,sys.pip_board);
   f1 = fopen(fn,"rb+");

   if (f1 == NULL)
      return (0);

   if (fseek(f1,sizeof(hdr)*msg_num,SEEK_SET))
   {
      fclose(f1);
      return (0);
   }
   if (fread(&hdr,sizeof(hdr),1,f1)==0)
   {
      fclose(f1);
      return (0);
   }
   if (hdr.status & SET_MPTR_DEL)
   {
      fclose(f1);
      return (0);
   }

   sprintf(fn,"%sMPKT%04x.PIP",pip_msgpath,sys.pip_board);
   f2 = fopen(fn,"rb+");
   if (f2 == NULL)
   {
      fclose(f1);
      return (0);
   }

   if (sm == NULL)
   {
      fpq = fopen(quote_name, (flags & APPEND_TEXT) ? "at" : "wt");
      if (fpq == NULL)
      {
         fclose (f1);
         fclose (f2);
         return(0);
      }
   }
   else
      fpq = sm;


   if (!((hdr.status&SET_MPTR_RCVD) || (hdr.status&SET_MPTR_DEL)))
   {
      fseek(f2,hdr.pos,SEEK_SET);
      fread(&mpkt,sizeof(mpkt),1,f2);

      memset ((char *)&msgt, 0, sizeof (struct _msg));
      msg_fzone = msg_tzone = alias[sys.use_alias].zone;
      msg_fpoint = msg_tpoint = 0;

      read0(msgt.date,f2);
      read0(msgt.to,f2);
      read0(msgt.from,f2);
      read0(msgt.subj,f2);

      msgt.orig_net = mpkt.fromnet;
      msgt.orig = mpkt.fromnode;
      if (!(mpkt.attribute & SET_PKT_FROMUS))
         msg_fpoint = mpkt.point;

      if (mpkt.attribute & SET_PKT_PRIV)
         msgt.attr |= MSGPRIVATE;
      if (mpkt.attribute & SET_PKT_CRASH)
         msgt.attr |= MSGCRASH;
      if (mpkt.attribute & SET_PKT_ATTACH)
         msgt.attr |= MSGFILE;
      if (mpkt.attribute & SET_PKT_TRANSIT)
         msgt.attr |= MSGFWD;
      if (mpkt.attribute & SET_PKT_KILL)
         msgt.attr |= MSGKILL;
      if (mpkt.attribute & SET_PKT_HOLD)
         msgt.attr |= MSGHOLD;
      if (mpkt.attribute & SET_PKT_REQUEST)
         msgt.attr |= MSGFRQ;
      if (mpkt.attribute & SET_PKT_UPDATE)
         msgt.attr |= MSGURQ;
      if (hdr.status & SET_MPTR_RCVD)
         msgt.attr |= MSGREAD;
      if (hdr.status & SET_MPTR_SENT)
         msgt.attr |= MSGSENT;

      if(msgt.attr & MSGPRIVATE)
      {
         if(stricmp(msgt.from,usr.name) && stricmp(msgt.to,usr.name))
         {
            fclose(f1);
            fclose(f2);
            if (sm != NULL)
               fclose (fpq);
            return(0);
         }
      }

      msgt.dest_net = mpkt.tonet;
      msgt.dest = mpkt.tonode;
      if ( (mpkt.attribute & SET_PKT_FROMUS) )
         msg_tpoint = mpkt.point;

      i = 0;
      if (flags & QUOTE_TEXT)
      {
         strcpy(buff, " > ");
         i = strlen(buff);
      }

      while ((a=(byte)fgetc(f2)) != 0 && !feof(f2))
      {
         c = a;

         switch(mpkt.pktype)
         {
            case 2:
               c = a;
               break;
            case 10:
               if (a!=10)
               {
                  if (a == 141)
                     a='\r';

                  if (a<128)
                     c=a;
                  else
                  {
                     if (a==128)
                     {
                        a=(byte)fgetc(f2);
                        c=(a)+127;
                     }
                     else
                     {
                        buff[i] = '\0';
                        strcat(buff,pip[a-128]);
                        i += strlen(pip[a-128]);
                        c = buff[--i];
                     }
                  }
               }
               else
                  c='\0';
               break;
            default:
               return(1);
         }

         if (c == '\0')
            continue;
         buff[i++] = c;

         if (c == 0x0D)
         {
            buff[i-1]='\0';

            if (buff[0] == 0x01)
            {
               if (!strncmp(&buff[1],"INTL",4) && !shead)
                  sscanf(&buff[6],"%d:%d/%d %d:%d/%d",&msg_tzone,&i,&i,&msg_fzone,&i,&i);
               if (!strncmp(&buff[1],"TOPT",4) && !shead)
                  sscanf(&buff[6],"%d",&msg_tpoint);
               if (!strncmp(&buff[1],"FMPT",4) && !shead)
                  sscanf(&buff[6],"%d",&msg_fpoint);
               i=0;
               continue;
            }
            else if (!shead)
            {
               if (flags & INCLUDE_HEADER)
                  text_header (&msgt,msg_num,fpq);
               else if (flags & QWK_TEXTFILE)
                  qwk_header (&msgt,&QWK,msg_num,fpq,&qpos);
               shead = 1;
            }

            if(buff[0] == 0x01 || !strncmp(buff,"SEEN-BY",7))
            {
               i=0;
               continue;
            }

            if (flags & QWK_TEXTFILE)
            {
               write_qwk_string (buff, qwkbuffer, &pos, &blks, fpq);
               write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpq);
            }
            else
               fprintf(fpq,"%s\n",buff);

            if (flags & QUOTE_TEXT) {
               strcpy(buff, " > ");
               i = strlen(buff);
            }
            else
               i = 0;
         }
         else {
            if(i<(usr.width-1))
               continue;

            buff[i]='\0';
            while(i>0 && buff[i] != ' ')
               i--;

            m=0;

            if(i != 0)
               for(z=i+1;z<(usr.width-1);z++) {
                  wrp[m++]=buff[z];
                  buff[i]='\0';
               }

            wrp[m]='\0';

            if (!shead) {
               if (flags & INCLUDE_HEADER)
                  text_header (&msgt,msg_num,fpq);
               else if (flags & QWK_TEXTFILE)
                  qwk_header (&msgt,&QWK,msg_num,fpq,&qpos);
               shead = 1;
            }

            if (flags & QWK_TEXTFILE) {
               write_qwk_string (buff, qwkbuffer, &pos, &blks, fpq);
               write_qwk_string ("\r\n", qwkbuffer, &pos, &blks, fpq);
            }
            else
               fprintf(fpq,"%s\n",buff);

            if (flags & QUOTE_TEXT)
               strcpy(buff, " > ");
            strcat(buff,wrp);
            i = strlen(wrp);
         }
      }
   }

   fprintf(fpq,bbstxt[B_ONE_CR],buff);

   fclose(f1);
   fclose(f2);

   if (flags & QWK_TEXTFILE)
   {
      fwrite(qwkbuffer, 128, 1, fpq);
      blks++;

      fseek(fpq,qpos,SEEK_SET);          /* Restore back to header start */
      sprintf(buff,"%d",blks);
      ljstring(QWK.Msgrecs,buff,6);
      fwrite((char *)&QWK,128,1,fpq);           /* Write out the header */
      fseek(fpq,0L,SEEK_END);               /* Bump back to end of file */

      if (sm == NULL)
         fclose(fpq);
      return (blks);
   }

   if (sm == NULL)
      fclose(fpq);

   return (1);
}

int pip_kill_message (msg)
int msg;
{
   FILE *f1,*f2;
   MSGPTR hdr;
   MSGPKTHDR mpkt;
   char fn[80];
   struct _msg msgt;

   msg--;

   sprintf (fn, "%sMPTR%04x.PIP", pip_msgpath, sys.pip_board);
   f1 = fopen (fn, "rb+");

   if (f1 == NULL)
      return (0);

   if (fseek(f1,sizeof(hdr)*msg,SEEK_SET)) {
      fclose(f1);
      return (0);
   }
   if (fread(&hdr,sizeof(hdr),1,f1)==0) {
      fclose(f1);
      return (0);
   }
   if (hdr.status & SET_MPTR_DEL) {
      fclose(f1);
      return (0);
   }

   sprintf(fn,"%sMPKT%04x.PIP", pip_msgpath,sys.pip_board);
   f2 = fopen(fn,"rb+");
   if (f2 == NULL) {
      fclose(f1);
      return (0);
   }

   if (!((hdr.status&SET_MPTR_RCVD) || (hdr.status&SET_MPTR_DEL))) {
      fseek(f2,hdr.pos,SEEK_SET);
      fread(&mpkt,sizeof(mpkt),1,f2);

      memset ((char *)&msgt, 0, sizeof (struct _msg));

      read0(msgt.date,f2);
      read0(msgt.to,f2);
      read0(msgt.from,f2);
      read0(msgt.subj,f2);

      if (!stricmp (msgt.from, usr.name) || !stricmp (msgt.to, usr.name) || usr.priv == SYSOP) {
         hdr.status |= SET_MPTR_DEL;
         fseek (f1, sizeof(hdr) * msg, SEEK_SET);
         fwrite (&hdr, sizeof(hdr), 1, f1);
         fclose (f1);
         fclose (f2);
         return (1);
      }
   }

   fclose (f1);
   fclose (f2);

   return (0);
}

