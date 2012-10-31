struct QWKmsghd {               /* Messages.dat header */
   char  Msgstat;    		/* message status byte */
   char  Msgnum[7];  		/* message number in ascii */
   char  Msgdate[8]; 		/* date in ascii */
   char  Msgtime[5]; 		/* time in ascii */
   char  Msgto[25];  		/* Person addressed to */
   char  Msgfrom[25];		/* Person who wrote message */
   char  Msgsubj[25];		/* Subject of message */
   char  Msgpass[12];		/* password */
   char  Msgrply[8]; 		/* message # this refers to */
   char  Msgrecs[6];		   /* # of 128 byte recs in message,inc header */
   char  Msglive; 		   /* always 0xe1 */
   char  Msgarealo;     	/* area number, low byte */
   char  Msgareahi;     	/* area number, hi byte */
   char  Msgfiller[3];		/* fill out to 128 bytes */
};

union Converter {
   unsigned char  uc[10];
   unsigned int   ui[5];
   unsigned long  ul[2];
   float           f[2];
   double          d[1];
};

struct repmsg {
   int count;
   int areano;
   word flags;
   struct repmsg _far *next;
};

#define RP_CMDFLAG  0x0001

static struct QWKmsghd *QMsgHead;

void qwkinit (void)
{
   char filename[80];
   struct ffblk fbuf;

   sprintf (filename, "%s*.*", QWKDir);
   if (!findfirst (filename, &fbuf, 0))
   {
      unlink (fbuf.ff_name);
      while (!findnext (&fbuf))
         unlink (fbuf.ff_name);
   }
}

int open_qwk (void)
{
   FILE *datfile;
   int Aindex, x, totals, code = 0;
   char temp[80], *p;
   time_t aclock;
   struct tm  *newtime;
   struct _sys tsys;

   QMsgHead = (struct QWKmsghd *) malloc(sizeof(struct QWKmsghd));
   if (QMsgHead == NULL)
      return 1;

   memset(QMsgHead->Msgpass,' ',12);
   memset(QMsgHead->Msgfiller,' ',3);
   QMsgHead->Msglive = 0xe1;

   sprintf (temp, "%sCONTROL.DAT", QWKDir);
   datfile = fopen (temp, "wt");
   if (datfile != NULL)
   {
      m_print ("\nCreating the Control.Dat file for your QWK packet...\n");
      totals = 0;
      strcpy (temp, system_name);
      p = strtok(temp," \r\n");
      fputs (p, datfile);         /* Line #1 */
      fputs ("\n", datfile);
      fputs ("               \n", datfile);
      fputs ("               ", datfile);
      fputs ("\n", datfile);
      fputs (sysop, datfile);
      fputs ("\n", datfile);
      sprintf (temp,"00000,%s\n", BBSid);         /* Line #5 */
      fputs (temp, datfile);
      sprintf (temp, "%s%s.QWK", QWKDir, BBSid);
      unlink (temp);
      time (&aclock);
      newtime = localtime (&aclock);
      sprintf (temp,"%02d-%02d-19%02d,%02d:%02d:%02d\n",
         newtime->tm_mon+1, newtime->tm_mday, newtime->tm_year,
         newtime->tm_hour, newtime->tm_min, newtime->tm_sec);
      fputs(temp, datfile);              /* Line #6 */
      fputs (strupr(usr.name), datfile);
      fputs ("\n", datfile);
      fputs (" \n", datfile);             /* Line #8 */
      fputs ("0\n", datfile);
      fputs ("0\n", datfile);             /* Line #10 */
      sprintf (temp, "%sSYSMSG.DAT", sys_path);
      Aindex = open (temp, O_RDONLY|O_BINARY);
      for (x=0; x < 1000; x++)
      {
         if ((read (Aindex, (char *)&tsys.msg_name, SIZEOF_MSGAREA)) != SIZEOF_MSGAREA)
            break;
         if (usr.priv < tsys.msg_priv)
            continue;
         if ((usr.flags & tsys.msg_flags) != tsys.msg_flags)
            continue;
         totals++;
      }
      sprintf (temp, "%d\n", totals-1);
      fputs (temp, datfile);
      lseek (Aindex, 0L, SEEK_SET);
      for (x=0; x < 1000; x++)
      {
         if ((read (Aindex, (char *)&tsys.msg_name, SIZEOF_MSGAREA)) != SIZEOF_MSGAREA)
            break;
         if (usr.priv < tsys.msg_priv)
            continue;
         if ((usr.flags & tsys.msg_flags) != tsys.msg_flags)
            continue;
         sprintf (temp, "%d\n", x);
         fputs (temp, datfile);
         memset (temp, 0, 11);
         strncpy (temp, tsys.msg_name, 10);
         fputs (temp, datfile);
         fputs ("\n", datfile);
      }

      fputs ("WELCOME\n", datfile);          /* This is QWK welcome file */
      fputs ("NEWS\n", datfile);          /* This is QWK news file */
      fputs ("GOODBYE\n", datfile);          /* This is QWK bye file */

      fclose (datfile);

      sprintf (temp, "%sDOOR.ID", QWKDir);
      datfile = fopen (temp, "wt");
      if (datfile != NULL) {
         sprintf(temp,"DOOR = LoraMail  \n");
         fputs(temp,datfile);		/* Line #1 */

         sprintf(temp,"VERSION = 2.00   \n");
         fputs(temp,datfile);		/* Line #2 */

         sprintf(temp,"SYSTEM = Lora-CBIS \n");
         fputs(temp,datfile);		/* Line #3 */

         sprintf(temp,"CONTROLNAME = LORAMAIL\n");
         fputs(temp,datfile);		/* Line #4 */

         sprintf(temp,"CONTROLTYPE = ADD\n");
         fputs(temp,datfile);		/* Line #5 */

         sprintf(temp,"CONTROLTYPE = DROP\n");
         fputs(temp,datfile);		/* Line #6 */
         sprintf(temp,"CONTROLTYPE = REQUEST\n");
         fputs(temp,datfile);		/* Line #6 */
         fclose(datfile);
      }

      sprintf (temp, "%sMESSAGES.DAT", QWKDir);
      MsgFile = fopen(temp,"wb");
      if (MsgFile != NULL) {		/* Write out the header */
         fwrite("Produced by Qmail...",20,1,MsgFile);
         fwrite("Copywright (c) 1987 by Sparkware.  ",35,1,MsgFile);
         fwrite("All Rights Reserved",19,1,MsgFile);
         for (x=0; x < 54; x++)
            fwrite(" ",1,1,MsgFile);		/* Fill out rest of 128 byte record */
      }
      else code = 1;
   }
   else code = 1;

   TotRecs = 1;
   return (code);
}

/* This function is called whenever a new message
   area to scan is started */
void qwk_newarea (areano)
int areano;
{
   char tname[80];

   if (Indx != -1)
      close(Indx);

   sprintf (tname, "%s%03d.ndx", QWKDir, areano);
   Indx = open (tname, O_CREAT|O_TRUNC|O_BINARY|O_RDWR, S_IWRITE|S_IREAD);
}

void closeqwk ()
{
   if (Indx != -1)
   {
      close(Indx);
      Indx = -1;
   }

   if (PIndx != -1)
   {
      close(PIndx);
      PIndx = -1;
   }

   if (MsgFile != NULL)
   {
      fclose(MsgFile);
      MsgFile = NULL;
   }
}

/* Convert an IEEE floating point number to MSBIN
   floating point decimal */

float _pascal IEEToMSBIN(f)
float f;
{
   union Converter t;
   int   sign,exp;

   t.f[0] = f;

/* Extract sign & change exponent bias from 0x7f to 0x81 */

   sign = t.uc[3] / 0x80;
   exp  = ((t.ui[1] >> 7) - 0x7f + 0x81) & 0xff;

/* reassemble them in MSBIN format */
   t.ui[1] = (t.ui[1] & 0x7f) | (sign << 7) | (exp << 8);
   return t.f[0];
}

char *namefixup(name)
char *name;
{
   int x,y;
   static char tname[81];

   strncpy(tname,name,80);
   tname[20] = 0;
   strtrim(tname);
   strlwr(tname);

   tname[0] = (char) toupper((int) tname[0]);
   y = (int) strlen(tname);

   for (x = 0; x < y; x++) {
      if (isspace(tname[x])) {
         x++;
         tname[x] = (char) toupper((int) tname[x]);
      }
   }
   return (&tname[0]);
}

