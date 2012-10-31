#include <stdio.h>
#include <ctype.h>
#include <dos.h>
#include <string.h>
#include <dir.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <alloc.h>
#include <sys\stat.h>

#include <cxl\cxlstr.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"

static int qbbs_get_bbs_record(int, int, int);
static int v5_get_bbs_record(int, int, int);
static int v6_get_bbs_record(int, int, int);
static int v7_get_bbs_record(int, int, int);
static void unpk(char *, char *, int);

static char unwrk[] = " EANROSTILCHBDMUGPKYWFVJXZQ-'0123456789";

void throughput (opt, bytes)
int opt;
unsigned long bytes;

{
   static long started = 0L;
   static long elapsed;

   if (!opt)
      started = time (NULL);
   else if (started)
      {
      elapsed = time (NULL);
      /* The next line tests for day wrap without the date rolling over */
      if (elapsed < started)
         elapsed += 86400L;
      elapsed -= started;
      if (elapsed == 0L)
         elapsed = 1L;
      cps = (long) (bytes / (unsigned long) elapsed);
      started = (cps * 1000L) / ((long) rate);
      status_line ((char *) msgtxt[M_CPS_MESSAGE], cps, bytes, started);
      }
}                                                /* throughput */

long zfree (drive)
char *drive;
{
   union REGS r;

   unsigned char driveno;
   long stat;

   if (drive[0] != '\0' && drive[1] == ':')
      {
      driveno = (unsigned char) (islower (*drive) ? toupper (*drive) : *drive);
      driveno = (unsigned char) (driveno - 'A' + 1);
      }
   else driveno = 0;                             /* Default drive    */

   r.x.ax = 0x3600;                              /* get free space   */
   r.h.dl = driveno;                             /* on this drive    */
   (void) int86 (0x21, &r, &r);                         /* go do it      */

   if (r.x.ax == 0xffff)                         /* error return??   */
      return (0);

   stat = (long) r.x.bx                          /* bx = clusters avail  */
      * (long) r.x.ax                            /* ax = sectors/clust   */
      * (long) r.x.cx;                           /* cx = bytes/sector    */

   return (stat);

}

void send_can ()
{
   int i;

   CLEAR_INBOUND();
   CLEAR_OUTBOUND();

   for (i = 0; i < 10; i++)
      SENDBYTE (0x18);
   for (i = 0; i < 10; i++)
      SENDBYTE (0x08);
}

void remove_abort (fname, rname)
char *fname, *rname;
{
        FILE *abortlog, *newlog;
        char namebuf[100];
        char linebuf[100];
        char *p;
        int c;

        if (!dexists (fname))
                return;

   if ((abortlog = fopen (fname, "ra")) != NULL)
      {
      strcpy (namebuf, fname);
      strcpy (namebuf + strlen (namebuf) - 1, "TMP");
      c = 0;
      if ((newlog = fopen (namebuf, "wa")) == NULL)
         {
         fclose (abortlog);
         }
      else
         {
         while (!feof (abortlog))
            {
            linebuf[0] = '\0';
            if (!fgets (linebuf, 64, abortlog))
               break;
            p = linebuf;
            while (*p > ' ')
               ++p;
            *p = '\0';
            if (stricmp (linebuf, rname))
               {
               *p = ' ';
               fputs (linebuf, newlog);
               ++c;
               }
            }
         fclose (abortlog);
         fclose (newlog);
         unlink (fname);
         if (c)
            rename (namebuf, fname);
         else unlink (namebuf);
         }
      }
}

void unique_name (fname)
char *fname;
{
   static char suffix[] = ".001";
   register char *p;
   register int n;

   if (dexists (fname))
      {                                          /* If file already exists...      */
      p = fname;
      while (*p && *p != '.')
         p++;                                    /* ...find the extension, if
                                                  * any  */
      for (n = 0; n < 4; n++)                    /* ...fill it out if
                                                  * neccessary   */
         if (!*p)
            {
            *p = suffix[n];
            *(++p) = '\0';
            }
         else p++;

      while (dexists (fname))                    /* ...If 'file.ext' exists
                                                  * suffix++ */
         {
         p = fname + strlen (fname) - 1;
         for (n = 3; n--;)
            {
            if (!isdigit (*p))
               *p = '0';
            if (++(*p) <= '9')
               break;
            else *p-- = '0';
            }                                    /* for */
         }                                       /* while */
      }                                          /* if exist */
}                                                /* unique_name */

int check_failed (fname, theirname, info, ourname)
char *fname, *theirname, *info, *ourname;
{
   FILE *abortlog;
   char linebuf[64];
        char *p, *badname;
        int ret;

        ret = 0;
   if ((abortlog = fopen (fname, "ra")) != NULL)
      {
      while (!feof (abortlog))
         {
         linebuf[0] = '\0';
         if (!fgets ((p = linebuf), 64, abortlog))
            break;
         while (*p >= ' ')
            ++p;
         *p = '\0';
         p = strchr (linebuf, ' ');
         *p = '\0';
         if (!stricmp (linebuf, theirname))
            {
            p = strchr ((badname = ++p), ' ');
            *p = '\0';
            if (!stricmp (++p, info))
               {
               strcpy (ourname, badname);
                                        ret = 1;
               break;
               }
            }
         }
      fclose (abortlog);
      }

        return (ret);
}

void add_abort (fname, rname, cname, cpath, info)
char *fname, *rname, *cname, *cpath, *info;
{
        FILE *abortlog;
        char namebuf[100];

   strcpy (namebuf, cpath);
   strcat (namebuf, "BadWaZOO.001");
   unique_name (namebuf);
   rename (cname, namebuf);
   if ((abortlog = fopen (fname, "at")) == NULL)
      {
      unlink (namebuf);
      }
   else
      {
      fprintf (abortlog, "%s %s %s\n", rname, namebuf + strlen (cpath), info);
      fclose (abortlog);
      }
}

int get_bbs_record (zone, net, node)
int zone, net, node;
{
        char stringa[80];

        sprintf (stringa, "%sNODEX.DAT", net_info);
        if (dexists (stringa))
                return (v7_get_bbs_record (zone, net, node));

        sprintf (stringa, "%sNODELIST.DAT", net_info);
        if (dexists (stringa))
                return (v6_get_bbs_record (zone, net, node));

        sprintf (stringa, "%sQNL_IDX.BBS", net_info);
        if (dexists (stringa))
                return (qbbs_get_bbs_record (zone, net, node));

        sprintf (stringa, "%sNODELIST.SYS", net_info);
        if (dexists (stringa))
                return (v5_get_bbs_record (zone, net, node));

        return (0);
}

static int v7_get_bbs_record(zone, net, node)
int zone, net, node;
{
   int fdi, m;
   char stringa[128], pack[128];
   struct _vers7 fnode;

   sprintf(stringa,"%sNODEX.DAT",net_info);
   fdi = shopen(stringa, O_RDONLY|O_BINARY);
   if(fdi == -1)
   {
      status_line(msgtxt[M_UNABLE_TO_OPEN],stringa);
      return (0);
   }

   while (read(fdi, (char *)&fnode, sizeof (struct _vers7)) == sizeof (struct _vers7))
   {
      if (fnode.Node == node &&
          fnode.Net == net &&
          (fnode.Zone == zone || !zone)
         )
      {
         read (fdi, (char *)&nodelist.phone, fnode.Phone_len);
         nodelist.password[fnode.Phone_len] = '\0';
         read (fdi, (char *)&nodelist.password, fnode.Password_len);
         nodelist.password[fnode.Password_len] = '\0';
         read (fdi, pack, fnode.pack_len);
         unpk (pack, stringa, fnode.pack_len);

         m = 0;

         strncpy (nodelist.name, &stringa[m], fnode.Bname_len);
         nodelist.name[fnode.Bname_len] = '\0';
         m += fnode.Bname_len;

         m += fnode.Sname_len;

         strncpy (nodelist.city, &stringa[m], fnode.Cname_len);
         nodelist.city[fnode.Cname_len] = '\0';
         m += fnode.Cname_len;

         nodelist.rate = fnode.BaudRate;
         nodelist.modem = fnode.ModemType;

         close (fdi);
         return (1);
      }
      else
         lseek (fdi, (long)(fnode.pack_len +
                            fnode.Phone_len +
                            fnode.Password_len), SEEK_CUR);
   }

   close (fdi);

   return (0);
}

static void unpk(char *instr,char *outp,int count)
{
    struct chars {
           unsigned char c1;
           unsigned char c2;
           };

    union {
          unsigned w1;
          struct chars d;
          } u;

   register int i, j;
   char obuf[4];

   outp[0] = '\0';

   while (count) {
       u.d.c1 = *instr++;
       u.d.c2 = *instr++;
       count -= 2;
       for(j=2;j>=0;j--) {
           i = u.w1 % 40;
           u.w1 /= 40;
            obuf[j] = unwrk[i];
           }
       obuf[3] = '\0';
       (void) strcat (outp, obuf);
       }
}


static int v6_get_bbs_record(zone, net, node)
int zone, net, node;
{
   int fd, fdi, m, total, real_rd, last_zone;
   char *ndx, stringa[60];
   long node_record;
   struct _ndi *index;

   last_zone = zone;

   sprintf(stringa,"%sNODELIST.DAT",net_info);
   fd = shopen(stringa,O_RDONLY|O_BINARY);
   if(fd == -1)
   {
      status_line(msgtxt[M_UNABLE_TO_OPEN],stringa);
      return (0);
   }

   sprintf(stringa,"%sNODELIST.IDX",net_info);
   fdi=shopen(stringa,O_RDONLY|O_BINARY);
   if(fdi == -1)
   {
      status_line(msgtxt[M_UNABLE_TO_OPEN],stringa);
      return (0);
   }

   ndx = (char *)malloc(INDEX_BUFFER);
   if (ndx == NULL)
   {
      status_line(msgtxt[M_NODELIST_MEM]);
      close(fdi);
      close(fd);
      return (0);
   }

   node_record = 0L;

   do {
      real_rd = read(fdi,ndx,INDEX_BUFFER);
      total = real_rd/sizeof(struct _ndi);

      for (m=0;m<total;m++)
      {
         index = (struct _ndi *)&ndx[m*4];

         if (index->node == -2)
            last_zone = index->net;

         if (index->node == node &&
             index->net == net &&
             (last_zone == zone || !zone)
            )
         {
            lseek(fd,node_record*sizeof(struct _node),SEEK_SET);
            read(fd,(char *)&nodelist,sizeof(struct _node));

            close (fdi);
            close (fd);

            free(ndx);

            return (1);
         }

         node_record++;
      }
   } while (real_rd == INDEX_BUFFER);

   close (fdi);
   close (fd);

   free(ndx);

   return (0);
}

static int v5_get_bbs_record(zone, net, node)
int zone, net, node;
{
   if (zone || net || node);
   return (0);
}

/*--------------------------------------------------------------------------*/
/* QuickBBS 2.00 QNL_IDX.BBS                                                */
/* (File is terminated by EOF)                                              */
/*--------------------------------------------------------------------------*/

struct QuickNodeIdxRecord
{
   int QI_Zone;
   int QI_Net;
   int QI_Node;
   byte QI_NodeType;
};


/*--------------------------------------------------------------------------*/
/* QuickBBS 2.00 QNL_DAT.BBS                                                */
/* (File is terminated by EOF)                                              */
/*--------------------------------------------------------------------------*/

struct QuickNodeListRecord
{
   byte QL_NodeType;
   int QL_Zone;
   int QL_Net;
   int QL_Node;
   byte QL_Name[21];                             /* Pascal! 1 byte count, up
                                                  * to 20 chars */
   byte QL_City[41];                             /* 1 + 40 */
   byte QL_Phone[41];                            /* 1 + 40 */
   byte QL_Password[9];                          /* 1 + 8 */
   word QL_Flags;                                /* Same as flags in new
                                                  * nodelist structure */
   int QL_BaudRate;
   int QL_Cost;
};

static int qbbs_get_bbs_record(zone, net, node)
int zone, net, node;
{
   #define QINDEX_BUFFER  100
   int fd, fdi, m, total, real_rd;
   char *ndx, stringa[60];
   long node_record;
   struct QuickNodeIdxRecord *index;
   struct QuickNodeListRecord Rec;

   sprintf(stringa,"%sQNL_DAT.BBS",net_info);
   fd = shopen(stringa,O_RDONLY|O_BINARY);
   if(fd == -1)
   {
      status_line(msgtxt[M_UNABLE_TO_OPEN],stringa);
      return (0);
   }

   sprintf(stringa,"%sQNL_IDX.BBS",net_info);
   fdi=shopen(stringa,O_RDONLY|O_BINARY);
   if(fdi == -1)
   {
      status_line(msgtxt[M_UNABLE_TO_OPEN],stringa);
      return (0);
   }

   ndx = (char *)malloc(QINDEX_BUFFER*sizeof(struct QuickNodeIdxRecord));
   if (ndx == NULL)
   {
      status_line(msgtxt[M_NODELIST_MEM]);
      close(fdi);
      close(fd);
      return (0);
   }

   node_record = 0L;

   do {
      real_rd = read(fdi,ndx,QINDEX_BUFFER*sizeof(struct QuickNodeIdxRecord));
      total = real_rd/sizeof(struct QuickNodeIdxRecord);

      for (m=0;m<total;m++)
      {
         index = (struct QuickNodeIdxRecord *)&ndx[m*sizeof(struct QuickNodeIdxRecord)];

         if (index->QI_Node == node &&
             index->QI_Net == net &&
             (index->QI_Zone == zone || !zone)
            )
         {
            lseek(fd,node_record*sizeof(struct QuickNodeListRecord),SEEK_SET);
            read(fd,(char *)&Rec,sizeof(struct QuickNodeListRecord));

            close (fdi);
            close (fd);

            free(ndx);

            strncpy(nodelist.password, &Rec.QL_Password[1], Rec.QL_Password[0]);
            strncpy(nodelist.phone, &Rec.QL_Phone[1], Rec.QL_Phone[0]);
            strncpy(nodelist.name, &Rec.QL_Name[1], Rec.QL_Name[0]);
            strncpy(nodelist.city, &Rec.QL_City[1], Rec.QL_City[0]);
            nodelist.rate = Rec.QL_BaudRate;
            nodelist.cost = Rec.QL_Cost;

            return (1);
         }

         node_record++;
      }
   } while (real_rd == QINDEX_BUFFER);

   close (fdi);
   close (fd);

   free(ndx);

   return (0);
}


