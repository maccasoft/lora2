#pragma inline
#include <stdio.h>
#include <dos.h>
#include <io.h>
#include <alloc.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>
#include <cxl\cxlstr.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"

static void display_cpu (void);
static void is_4dos (void);
static void compilation_data(void);

#define MAX_INDEX    500

void software_version()
{
   unsigned char c;
   union REGS inregs, outregs;

   cls();

   change_attr(LMAGENTA|_BLACK);
   m_print("%s - The Computer-Based Information System\n", VERSION);
   m_print("CopyRight (c) 1989-92 by Marco Maccaferri. All Rights Res.\n");

   change_attr(LCYAN|_BLACK);
   m_print("\nJanus revision 0.31 - (C) Copyright 1987-90, Bit Bucket Software Co.\n");

   compilation_data();

   c = peekb(0xFFFF,0x000E);

   change_attr(WHITE|_BLACK);
   m_print(bbstxt[B_COMPUTER]);
   switch((byte)c) {
   case 0xFD:
      m_print(bbstxt[B_TYPE_PCJR]);
      break;
   case 0xFE:
      m_print(bbstxt[B_TYPE_XT]);
      break;
   case 0xFF:
      m_print(bbstxt[B_TYPE_PC]);
      break;
   case 0xFC:
      m_print(bbstxt[B_TYPE_AT]);
      break;
   case 0xFA:
      m_print(bbstxt[B_TYPE_PS230]);
      break;
   case 0xF9:
      m_print(bbstxt[B_TYPE_PCCONV]);
      break;
   case 0xF8:
      m_print(bbstxt[B_TYPE_PS280]);
      break;
   case 0x9A:
   case 0x2D:
      m_print(bbstxt[B_TYPE_COMPAQ]);
      break;
   default:
      m_print(bbstxt[B_TYPE_GENERIC]);
      break;
   }

   display_cpu();

   inregs.h.ah=0x30;
   intdos(&inregs,&outregs);
   change_attr(YELLOW|_BLACK);
   m_print(bbstxt[B_OS_DOS],outregs.h.al,outregs.h.ah);
   is_4dos();

   fossil_version();

   change_attr(LGREEN|_BLACK);
   m_print(bbstxt[B_HEAP_RAM], coreleft());

   press_enter ();
}

void is_4dos()
{
   union REGS inregs, outregs;

   inregs.x.ax = 0xD44D;
   inregs.x.bx = 0;
   int86(0x2F, &inregs, &outregs);

   if (outregs.x.ax == 0x44DD)
      m_print(bbstxt[B_DOS_4DOS], outregs.h.bl, outregs.h.bh);
}

void display_cpu()
{
   m_print(" (CPU ");

   asm   pushf
   asm   xor     dx,dx
   asm   xor     ax,ax
   asm   push    ax
   asm   popf
   asm   pushf
   asm   pop     ax
   asm   and     ax,0f000h
   asm   cmp     ax,0f000h
   asm   je      l1
   asm   mov     ax,07000h
   asm   push    ax
   asm   popf
   asm   pushf
   asm   pop     ax
   asm   and     ax,07000h
   asm   jne     l2
   m_print("80286");
   asm   jmp     fine
l2:
   m_print("80386");
   asm   jmp     fine
l1:
   asm   mov     ax,0ffffh
   asm   mov     cl,21h
   asm   shl     ax,cl
   asm   je      l3
   m_print("80186");
   asm   jmp     fine
l3:
   asm   xor     al,al
   asm   mov     al,40h
   asm   mul     al
   asm   je      l4
   m_print("8086/88");
   asm   jmp     fine
l4:
    m_print("V20/30");
fine:
   asm   popf

   m_print(")\n");
}


void display_area_list(type, flag, sig)   /* flag == 1, Normale due colonne */
int type, flag;                           /* flag == 2, Normale una colonna */
{                                         /* flag == 3, Anche nomi, una colonna */
   int fd, i, pari, area, linea, nsys;
   char stringa[13], filename[50], dir[80], first;
   struct _sys tsys;
   struct _sys_idx sysidx[MSG_AREAS];

   if (!type)
       return;

   first = 1;

   if (type == 1)
   {
      sprintf(filename,"%sSYSMSG.IDX", sys_path);
      fd = open(filename, O_RDONLY|O_BINARY);
      if (fd == -1)
         return;
      nsys = read(fd, (char *)&sysidx, sizeof(struct _sys_idx) * MSG_AREAS);
      nsys /= sizeof (struct _sys_idx);
      close(fd);
      area = usr.msg;
   }
   else if (type == 2)
   {
      sprintf(filename,"%sSYSFILE.IDX", sys_path);
      fd = open(filename, O_RDONLY|O_BINARY);
      if (fd == -1)
         return;
      nsys = read(fd, (char *)&sysidx, sizeof(struct _sys_idx) * MSG_AREAS);
      nsys /= sizeof (struct _sys_idx);
      close(fd);
      area = usr.files;
   }

   do {
      if (!get_command_word (stringa, 12))
      {
         if (type == 1)
            m_print(bbstxt[B_SELECT_AREAS],bbstxt[B_MESSAGE]);
         else if (type == 2)
            m_print(bbstxt[B_SELECT_AREAS],bbstxt[B_FILE]);
         input(stringa, 12);
         if (!CARRIER || time_remain() <= 0)
            return;
      }

      if (!stringa[0] && first)
      {
         first = 0;
         stringa[0] = area_change_key[2];
      }

      if (stringa[0] == area_change_key[2] || (!stringa[0] && !area))
      {
         first = 0;
         if (type == 1)
            sprintf(filename,"%sSYSMSG.DAT", sys_path);
         else if (type == 2)
            sprintf(filename,"%sSYSFILE.DAT", sys_path);
         fd = open(filename, O_RDONLY|O_BINARY);
         if (fd == -1)
            return;

         cls();
         m_print(bbstxt[B_AREAS_TITLE], (type == 1) ? bbstxt[B_MESSAGE] : bbstxt[B_FILE]);

         pari = 0;
         linea = 4;

         for (i=0; i < nsys; i++) {
            if (usr.priv < sysidx[i].priv ||
                (usr.flags & sysidx[i].flags) != sysidx[i].flags)
               continue;

            if (type == 1)
            {
               if (read_system_file ("MSGAREA"))
                  break;

               lseek(fd, (long)i * SIZEOF_MSGAREA, SEEK_SET);
               read(fd, (char *)&tsys.msg_name, SIZEOF_MSGAREA);

//               if (sig && tsys.msg_sig != sig)
//                  continue;

               if (flag == 1)
               {
                  strcpy(dir, tsys.msg_name);
                  dir[31] = '\0';
                  m_print("%3d ... ",sysidx[i].area);
                  if (pari)
                  {
                     m_print("%s\n",dir);
                     pari = 0;
                     if (!(linea = more_question(linea)))
                        break;
                  }
                  else
                  {
                     m_print("%-31s ",dir);
                     pari = 1;
                  }
               }
               else if (flag == 2 || flag == 3)
               {
                  m_print("%3d ... ",sysidx[i].area);
                  if (flag == 3)
                     m_print("%-12s ",sysidx[i].key);
                  m_print(" %s\n",tsys.msg_name);

                  if (!(linea = more_question(linea)))
                     break;
               }
            }
            else if (type == 2)
            {
               if (read_system_file ("FILEAREA"))
                  break;

               lseek(fd, (long)i * SIZEOF_FILEAREA, SEEK_SET);
               read(fd, (char *)&tsys.file_name, SIZEOF_FILEAREA);

//               if (sig && tsys.file_sig != sig)
//                  continue;

               if (flag == 1)
               {
                  strcpy(dir, tsys.file_name);
                  dir[31] = '\0';
                  m_print("%3d ... ",sysidx[i].area);
                  if (pari)
                  {
                     m_print("%s\n",dir);
                     pari = 0;
                     if (!(linea = more_question(linea)))
                        break;
                  }
                  else
                  {
                     m_print("%-31s ",dir);
                     pari = 1;
                  }
               }
               else if (flag == 2 || flag == 3)
               {
                  m_print("%3d ... ",sysidx[i].area);
                  if (flag == 3)
                     m_print("%-12s ",sysidx[i].key);
                  m_print(" %s\n",tsys.file_name);

                  if (!(linea = more_question(linea)))
                     break;
               }
            }
         }

         close(fd);

         if (pari)
            m_print("\n");
         area = -1;
      }
      else if (stringa[0] == area_change_key[1])
      {
         for (i=0; i < nsys; i++)
            if (sysidx[i].area == area)
               break;
         if (i == nsys)
            continue;
         do {
            i++;
            if (i >= nsys)
               i = 0;
            if (sysidx[i].area == area)
               break;
         } while (usr.priv < sysidx[i].priv);
         area = sysidx[i].area;
      }
      else if (stringa[0] == area_change_key[0])
      {
         for (i=0; i < nsys; i++)
            if (sysidx[i].area == area)
               break;
         if (i == nsys)
            continue;
         do {
            i--;
            if (i < 0)
               i = nsys - 1;
            if (sysidx[i].area == area)
               break;
         } while (usr.priv < sysidx[i].priv);
         area = sysidx[i].area;
      }
      else if (strlen(stringa) < 1)
         return;
      else {
         area = atoi(stringa);
         if (area < 1 || area > MSG_AREAS)
         {
            area = -1;
            for(i=0;i<nsys;i++)
               if (!stricmp(stringa,sysidx[i].key))
               {
                  area = sysidx[i].area;
                  break;
               }
         }
      }
   } while (area == -1 || !read_system(area, type));

   if (type == 1)
   {
      status_line(msgtxt[M_BBS_EXIT], area, sys.msg_name);
      usr.msg = area;
   }
   else if (type == 2)
   {
      status_line(msgtxt[M_BBS_SPAWN], area, sys.file_name);
      usr.files = area;
   }
}

int read_system(s, type)
int s, type;
{
   int fd, nsys, i;
   char filename[50];
   struct _sys_idx sysidx[MSG_AREAS];

   if (type != 1 && type != 2)
      return (0);

   if (type == 1)
   {
      sprintf(filename,"%sSYSMSG.IDX", sys_path);
      fd = open(filename, O_RDONLY|O_BINARY);
      if (fd == -1)
         return (0);
      nsys = read(fd, (char *)&sysidx, sizeof(struct _sys_idx) * MSG_AREAS);
      nsys /= sizeof (struct _sys_idx);
      close(fd);
   }
   else if (type == 2)
   {
      sprintf(filename,"%sSYSFILE.IDX", sys_path);
      fd = open(filename, O_RDONLY|O_BINARY);
      if (fd == -1)
         return (0);
      nsys = read(fd, (char *)&sysidx, sizeof(struct _sys_idx) * MSG_AREAS);
      nsys /= sizeof (struct _sys_idx);
      close(fd);
   }

   for (i=0; i < nsys; i++)
   {
      if (sysidx[i].area == s)
         break;
   }

   if (i == nsys || sysidx[i].priv > usr.priv)
      return (0);

   if (type == 1)
   {
      sprintf(filename,"%sSYSMSG.DAT", sys_path);
      fd = open(filename, O_RDONLY|O_BINARY);
      if (fd == -1)
         return (0);
      lseek (fd, (long)i * SIZEOF_MSGAREA, SEEK_SET);
      read(fd, (char *)&sys.msg_name, SIZEOF_MSGAREA);
      close(fd);

      if (sys.pip_board)
      {
         sprintf (filename, "%sMPTR%04x.PIP", pip_msgpath, s);
         if (!dexists (filename))
            return (0);
         sprintf (filename, "%sMPKT%04x.PIP", pip_msgpath, s);
         if (!dexists (filename))
            return (0);
      }
      else if (sys.quick_board)
      {
         sprintf (filename, "%sMSG*.BBS", fido_msgpath);
         if (!dexists (filename))
            return (0);
      }
      else
      {
         sys.msg_path[strlen (sys.msg_path) - 1] = '\0';
         if (!dexists (sys.msg_path))
            return (0);
         sys.msg_path[strlen (sys.msg_path)] = '\\';
      }
   }
   else if (type == 2)
   {
      sprintf(filename,"%sSYSFILE.DAT", sys_path);
      fd = open(filename, O_RDONLY|O_BINARY);
      if (fd == -1)
         return (0);
      lseek (fd, (long)i * SIZEOF_FILEAREA, SEEK_SET);
      read(fd, (char *)&sys.file_name, SIZEOF_FILEAREA);
      close(fd);

      sys.filepath[strlen (sys.filepath) - 1] = '\0';
      if (!dexists (sys.filepath))
         return (0);
      sys.filepath[strlen (sys.filepath)] = '\\';
   }

   return (1);
}

int read_system2 (s, type, tsys)
int s, type;
struct _sys *tsys;
{
   int fd, nsys, i;
   char filename[50];
   struct _sys_idx sysidx[MSG_AREAS];

   if (type != 1 && type != 2)
      return (0);

   if (type == 1)
   {
      sprintf(filename,"%sSYSMSG.IDX", sys_path);
      fd = open(filename, O_RDONLY|O_BINARY);
      if (fd == -1)
         return (0);
      nsys = read(fd, (char *)&sysidx, sizeof(struct _sys_idx) * MSG_AREAS);
      nsys /= sizeof (struct _sys_idx);
      close(fd);
   }
   else if (type == 2)
   {
      sprintf(filename,"%sSYSFILE.IDX", sys_path);
      fd = open(filename, O_RDONLY|O_BINARY);
      if (fd == -1)
         return (0);
      nsys = read(fd, (char *)&sysidx, sizeof(struct _sys_idx) * MSG_AREAS);
      nsys /= sizeof (struct _sys_idx);
      close(fd);
   }

   for (i=0; i < nsys; i++)
   {
      if (sysidx[i].area == s)
         break;
   }

   if (i == nsys || sysidx[i].priv > usr.priv)
      return (0);

   if (type == 1)
   {
      sprintf(filename,"%sSYSMSG.DAT", sys_path);
      fd = open(filename, O_RDONLY|O_BINARY);
      if (fd == -1)
         return (0);
      lseek (fd, (long)i * SIZEOF_MSGAREA, SEEK_SET);
      read(fd, (char *)&tsys->msg_name, SIZEOF_MSGAREA);
      close(fd);
   }
   else if (type == 2)
   {
      sprintf(filename,"%sSYSFILE.DAT", sys_path);
      fd = open(filename, O_RDONLY|O_BINARY);
      if (fd == -1)
         return (0);
      lseek (fd, (long)i * SIZEOF_FILEAREA, SEEK_SET);
      read(fd, (char *)&tsys->file_name, SIZEOF_FILEAREA);
      close(fd);
   }

   return (1);
}

void user_list(args)
char *args;
{
   char stringa[40], linea[80], *p, handle, swap;
   int fd, line, act, nn, day, mont, year;
   int mdays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
   long days, now;
   struct _usr tempusr;

   nopause = 0;
   line = 3;
   act = 0;
   nn = 0;
   handle = swap = 0;

   cls();

   change_attr(YELLOW|_BLACK);
   if ((p=strstr (args,"/H")) != NULL)
      handle = 1;
   if ((p=strstr (args,"/S")) != NULL)
      swap = 1;
   if ((p=strstr (args,"/L")) != NULL)
   {
      act = 1;
      if (p[2] == '=')
         nn = atoi(&p[3]);
      else
      {
         if (!get_command_word (stringa, 5))
         {
            m_print(bbstxt[B_MIN_DAYS_LIST]);
            input(stringa,5);
         }
         if (stringa[0] == '\0')
            nn = 15;
         nn = atoi(stringa);
      }
      data (stringa);
      sscanf(stringa, "%2d %3s %2d", &day, linea, &year);
      linea[3] = '\0';
      for (mont = 0; mont < 12; mont++) {
         if ((!stricmp(mtext[mont], linea)) || (!stricmp(mesi[mont], linea)))
            break;
      }
      now = (long)year*365;
      for (line=0;line<mont;line++)
         now += mdays[line];
      now += day;
   }
   else if ((p=strstr (args,"/T")) != NULL)
   {
      act = 2;
      if (p[2] == '=')
         nn = atoi(&p[3]);
      else
      {
         if (!get_command_word (stringa, 5))
         {
            m_print(bbstxt[B_MIN_CALLS_LIST]);
            input(stringa,5);
         }
         if (stringa[0] == '\0')
            nn = 10;
         nn = atoi(stringa);
      }
   }
   else
   {
      if (!get_command_word (stringa, 35))
      {
         m_print(bbstxt[B_ENTER_USERNAME]);
         input(stringa,35);
      }
      act = 0;
   }

   strupr(stringa);

   cls();
   change_attr(YELLOW|_BLACK);
   m_print(bbstxt[B_USERLIST_TITLE]);
   m_print(bbstxt[B_USERLIST_UNDERLINE]);

   sprintf(linea, "%s.BBS", user_file);
   fd=open(linea,O_RDONLY|O_BINARY);

   while(read(fd,(char *)&tempusr,sizeof(struct _usr)) == sizeof (struct _usr))
   {
      if (tempusr.usrhidden || tempusr.deleted || !tempusr.name[0])
         continue;

      if (handle)
         strcpy (tempusr.name, tempusr.handle);

      if (act == 0)
      {
         strupr(tempusr.name);

         if (strstr(tempusr.name,stringa) == NULL)
            continue;

         fancy_str(tempusr.name);
      }
      else if (act == 1)
      {
         sscanf(tempusr.ldate, "%2d %3s %2d", &day, linea, &year);
         linea[3] = '\0';
         for (mont = 0; mont < 12; mont++) {
            if ((!stricmp(mtext[mont], linea)) || (!stricmp(mesi[mont], linea)))
               break;
         }
         days = (long)year*365;
         for (line=0;line<mont;line++)
            days += mdays[line];
         days += day;
         if ((int)(now-days) > nn)
            continue;
      }
      else if (act == 2)
      {
         if (tempusr.times < (long)nn)
            continue;
      }

      if (swap)
      {
         strcpy (linea, tempusr.name);
         p = strchr (linea, '\0');
         while (*p != ' ' && p != linea)
            p--;
         if (p != linea)
            *p++ = '\0';
         strcpy (tempusr.name, p);
         strcat (tempusr.name, " ");
         strcat (tempusr.name, linea);
      }

      m_print(bbstxt[B_USERLIST_FORMAT], tempusr.name, tempusr.city, tempusr.ldate, tempusr.times, tempusr.n_dnld, tempusr.n_upld);
      if (!(line=more_question(line)))
         break;
   }

   close(fd);
   m_print(bbstxt[B_ONE_CR]);

   if (line)
      press_enter();
}

int logoff_procedure()
{
   read_system_file("LOGOFF");

   if (!local_mode && !terminal && CARRIER)
      timer (2);

   return (1);
}

void update_user()
{
   int online, fd, i, m, fflag, posit;
   char filename[80];
   long prev, crc;
   struct _useron useron;
   struct _usridx usridx[MAX_INDEX];

   if (!local_mode)
      FLUSH_OUTPUT();

   if(usr.name[0] && usr.city[0] && usr.pwd[0])
   {
      sprintf(filename, USERON_NAME, sys_path);
      fd = open(filename, O_RDWR|O_BINARY);

      while (fd != -1) {
         prev = tell(fd);

         if (read(fd, (char *)&useron, sizeof(struct _useron)) != sizeof(struct _useron))
            break;

         if (!strcmp(useron.name,usr.name) && useron.line == line_offset)
         {
            lseek(fd,prev,SEEK_SET);
            memset((char *)&useron, 0, sizeof(struct _useron));
            write (fd, (char *)&useron, sizeof(struct _useron));
            break;
         }
      }

      close(fd);

      fflag = 0;
      posit = 0;

      crc = crc_name (usr.name);

      sprintf (filename, "%s.IDX", user_file);
      fd = open (filename, O_RDWR|O_BINARY);

      do {
         i = read(fd,(char *)&usridx,sizeof(struct _usridx) * MAX_INDEX);
         m = i / sizeof (struct _usridx);

         for (i=0; i < m; i++)
            if (usridx[i].id == crc)
            {
               m = 0;
               posit += i;
               fflag = 1;
               break;
            }

         if (!fflag)
            posit += m;
      } while (m == MAX_INDEX && !fflag);

      close (fd);

      sprintf (filename, "%s.BBS", user_file);

      fd = open (filename, O_RDWR|O_BINARY);
      lseek (fd, (long)posit * sizeof (struct _usr), SEEK_SET);

      if (!fflag)
         status_line("!Can't update %s",usr.name);
      else
      {
         online = (int)((time(NULL)-start_time)/60);
         usr.time += online;
         if (lorainfo.logindate[0])
            strcpy (usr.ldate, lorainfo.logindate);
         write(fd,(char *)&usr,sizeof(struct _usr));
         set_last_caller();
      }

      close(fd);
   }

   if(usr.name[0])
   {
      online = (int)((time(NULL)-start_time)/60);
      status_line("+%s off-line. Calls=%ld, Len=%d, Today=%d",usr.name,usr.times,online,usr.time);
   }

   memset(usr.name, 0, 36);
}

void space(s)
int s;
{
   while(s-- > 0)
   {
      if (!local_mode)
         BUFFER_BYTE(' ');
      if (snooping)
         wputc(' ');
   }

   if (!local_mode)
      UNBUFFER_BYTES ();
}

static void compilation_data()
{
  #define COMPILER "Borland C"
  #define COMPVERMAJ    __TURBOC__/0x100
  #define COMPVERMIN    __TURBOC__ % 0x100

  m_print(bbstxt[B_COMPILER], __DATE__,__TIME__,COMPILER,COMPVERMAJ,COMPVERMIN);
}

void show_lastcallers(args)
char *args;
{
   int fd, i, line;
   char linea[82], filename[80];
   struct _lastcall lc;

   sprintf(filename, "%sLASTCALL.BBS", sys_path);
   fd = open(filename, O_RDONLY|O_BINARY);

   memset((char *)&lc, 0, sizeof(struct _lastcall));

   cls();
   sprintf(linea,bbstxt[B_TODAY_CALLERS], system_name);
   i = (80 - strlen(linea)) / 2;
   space(i);
   change_attr(WHITE|_BLACK);
   m_print("%s\n",linea);
   change_attr(LRED|_BLACK);
   strnset(linea, '-', strlen(linea));
   i = (80 - strlen(linea)) / 2;
   space(i);
   m_print("%s\n\n",linea);
   change_attr(LGREEN|_BLACK);
   m_print(bbstxt[B_CALLERS_HEADER]);
   change_attr(GREEN|_BLACK);
   m_print("-------------------------------------------------------------------------------\n");

   line = 5;
   i = atoi(args);

   while (read(fd, (char *)&lc, sizeof(struct _lastcall)) == sizeof(struct _lastcall))
   {
      if (i && lc.line != i)
         continue;

      change_attr(LCYAN|_BLACK);
      sprintf(linea,"%-20.20s ", lc.name);
      m_print(linea);
      change_attr(WHITE|_BLACK);
      m_print("%2d    ", lc.line);
      m_print("%6d ", lc.baud);
      m_print("    %s ", lc.logon);
      m_print("  %s ", lc.logoff);
      m_print(" %5d  ", lc.times);
      change_attr(YELLOW|_BLACK);
      sprintf(linea," %s", lc.city);
      linea[19] = '\0';
      m_print("%s\n",linea);

      if (!(line = more_question(line)) || !CARRIER)
         break;
   }

   close(fd);
   m_print("\n");

   if (line)
      press_enter();
}

void bulletin_menu (args)
char *args;
{
   int max_input;
   char stringa[10], *p, *a, filename[80];

   p = args;
   while ((a=strchr(p,':')) != NULL || (a=strchr(p,'\\')) != NULL)
      p = a+1;

   max_input = 8 - strlen(p);

   for (;;)
   {
      if (!get_command_word (stringa, max_input))
      {
         read_file (args);
         input (stringa, max_input);
         if (!stringa[0] || !CARRIER || time_remain() <= 0)
            return;
      }

      strcpy (filename, args);
      strcat (filename, stringa);
      read_file (filename);
      press_enter ();
   }
}

void message_to_next_caller ()
{
   FILE *fp;
   int i;
   char filename[80];
   long t;
   struct tm *tim;

   line_editor (0);

   m_print(bbstxt[B_DATABASE]);
   if (yesno_question (DEF_YES) == DEF_NO)
   {
      free_message_array ();
      return;
   }

   sprintf(filename,"%sNEXT%d.BBS",sys_path,line_offset);
   fp = fopen(filename,"wb");
   if (fp == NULL)
   {
      free_message_array ();
      return;
   }

   memset((char *)&msg,0,sizeof(struct _msg));
   msg.attr = MSGLOCAL;

   msg.dest_net=alias[sys.use_alias].net;
   msg.dest=alias[sys.use_alias].node;
   strcpy (msg.from, usr.name);
   strcpy (msg.subj, "Logout comment");
   data(msg.date);
   msg.up=0;
   msg_fzone=alias[sys.use_alias].zone;
   msg.orig=alias[sys.use_alias].node;
   msg.orig_net=alias[sys.use_alias].net;
   time(&t);
   tim=localtime(&t);
   msg.date_written.date=(tim->tm_year-80)*512+(tim->tm_mon+1)*32+tim->tm_mday;
   msg.date_written.time=tim->tm_hour*2048+tim->tm_min*32+tim->tm_sec/2;
   msg.date_arrived.date=msg.date_written.date;
   msg.date_arrived.time=msg.date_written.time;

   fwrite((char *)&msg,sizeof(struct _msg),1,fp);

   i = 0;
   while (messaggio[i] != NULL)
   {
      if (messaggio[i][strlen(messaggio[i])-1] == '\r')
         fprintf(fp,"%s\n", messaggio[i++]);
      else
         fprintf(fp,"%s\r\n", messaggio[i++]);
   }

   fclose (fp);
   free_message_array ();

   status_line(":Write message to next caller");
}

void read_comment ()
{
   FILE *fp;
   int line = 1;
   char filename[130], *p;
   struct _msg msgt;

   sprintf(filename,"%sNEXT%d.BBS",sys_path,line_offset);
   fp = fopen(filename,"rb");
   if (fp == NULL)
      return;

   fread((char *)&msgt,sizeof(struct _msg),1,fp);
   strcpy (msgt.to, usr.name);

   cls ();
   line = msg_attrib(&msgt, 0, line, 0);
   m_print(bbstxt[B_ONE_CR]);
   change_attr(LGREY|_BLACK);

   while (fgets(filename,128,fp) != NULL)
   {
      for (p=filename;(*p) && (*p != 0x1B);p++)
      {
         if (!CARRIER || RECVD_BREAK())
         {
            CLEAR_OUTBOUND();
            fclose(fp);
            return;
         }

         if (!local_mode)
            BUFFER_BYTE(*p);
         if (snooping)
            wputc(*p);
      }

      if (!(line=more_question(line)) || !CARRIER)
         break;
   }

   fclose(fp);

   UNBUFFER_BYTES ();
   FLUSH_OUTPUT();

   if (line)
   {
      m_print (bbstxt[B_ONE_CR]);
      press_enter ();
   }

   sprintf(filename,"%sNEXT%d.BBS",sys_path,line_offset);
   unlink (filename);
}

#define IsBis(y) (!(y % 4) && ((y % 100) || !(y % 400)))

void display_usage ()
{
   int i, y_axis[15], z, m, dd, mm, aa, month[13];
   long total, tempo, day_now;
   struct tm *tim;

   month[1] = 31;
   month[2] = 28;
   month[3] = 31;
   month[4] = 30;
   month[5] = 31;
   month[6] = 30;
   month[7] = 31;
   month[8] = 31;
   month[9] = 30;
   month[10] = 31;
   month[11] = 30;
   month[12] = 31;

   tempo = time (&tempo);
   tim = localtime (&tempo);

   total = 365 * tim->tm_year;
   if (IsBis (tim->tm_year))
      month[2] = 29;
   else
      month[2] = 28;
   for (i = 1; i < (tim->tm_mon + 1); i++)
      total += month[i];
   total += tim->tm_mday;
   total *= 60;

   sscanf (linestat.startdate, "%2d-%2d-%2d", &mm, &dd, &aa);

   day_now = 365 * aa;
   if (IsBis (aa))
      month[2] = 29;
   else
      month[2] = 28;
   for (i = 1; i < mm; i++)
      day_now += month[i];
   day_now += dd;
   day_now *= 60;

   total -= day_now;
   total += 60;

   for (i = 0; i < 15; i++)
      y_axis[i] = 0;

   if (total > 0)
      for (i = 0; i < 24; i++)
      {
         m = (int)((linestat.busyperhour[i] * 100L) / total);
         if (m > y_axis[14])
            y_axis[14] = m;
      }

   m = y_axis[14] / 15;

   for (i = 13; i >= 0; i--)
      y_axis[i] = y_axis[i+1] - m;

   cls ();
   m_print (bbstxt[B_PERCENTAGE], linestat.startdate, tim->tm_mon+1, tim->tm_mday, tim->tm_year % 100, line_offset);

   for (i = 14; i >= 0; i--)
   {
      m_print (bbstxt[B_PERCY], y_axis[i]);
      for (z = 0; z < 24; z++)
      {
         if (total > 0)
            m = (int)((linestat.busyperhour[z] * 100L) / total);
         else
            m = -1;
         if (m >= y_axis[i] && m > 0)
            m_print ("** ");
         else
            m_print ("   ");
      }

      m_print ("\n");
   }

   m_print (bbstxt[B_PERCX1]);
   m_print (bbstxt[B_PERCX2]);

   press_enter ();
}

void time_statistics ()
{
   long t, this_call;
   struct tm *tim;

   this_call = (long)((time(&t) - start_time));
   tim = localtime (&t);

   m_print (bbstxt[B_TSTATS_1], tim->tm_hour, tim->tm_min);
   m_print (bbstxt[B_TSTATS_2], tim->tm_mday, mtext[tim->tm_mon], tim->tm_year);
                                                                                
   m_print (bbstxt[B_TSTATS_3], (int)(this_call/60), (int)(this_call % 60));

   this_call += usr.time * 60;
   m_print (bbstxt[B_TSTATS_4], (int)(this_call/60), (int)(this_call % 60));
   m_print (bbstxt[B_TSTATS_5], time_remain ());
   m_print (bbstxt[B_TSTATS_6], class[usr_class].max_call);

   press_enter ();
}

