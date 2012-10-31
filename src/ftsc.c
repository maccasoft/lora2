#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dir.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"

extern long elapsed, timeout;

char *n_frproc(char *, int *, int);
int n_getpassword(int, int, int, int);
long get_phone_cost (int, int, int, long);
void import_sequence (void);

static int recvmdm7(char *);
int xfermdm7(char *);
static int FTSC_sendmail(void);
static int FTSC_recvmail(void);
static int SEA_sendreq(void);
static int SEA_recvreq(void);
static int try_sealink(void);
static int req_out(char *,char *);
static int get_req_str(char *,char *,int *);
static int gen_req_name(char *,char *,int *);

struct _pkthdr22
{
   int  orig_node;         /* originating node               */
   int  dest_node;         /* destination node               */
   int  orig_point;        /* originating point              */
   int  dest_point;        /* Destination point              */
   byte reserved[8];
   int  subver;            /* packet subversion              */
   int  ver;               /* packet version                 */
   int  orig_net;          /* originating network number     */
   int  dest_net;          /* destination network number     */
   char product;           /* product type                   */
   char serial;            /* serial number (some systems)   */
   byte password[8];       /* session/pickup password        */
   int  orig_zone;         /* originating zone               */
   int  dest_zone;         /* Destination zone               */
   char orig_domain[8];    /* originating domain name        */
   char dest_domain[8];    /* destination domain name        */
   byte filler[4];
};

/*------------------------------------------------------------------------*/
/*                 Protocolli Trasferimento Network                       */
/*------------------------------------------------------------------------*/
#define WAZOO_SECTION
#define MATRIX_SECTION

#define PRODUCT_CODE 0x4E
#define isLORA       0x4E
#define no_zapzed    0
#define NUM_FLAGS    5

#include "version.h"

#define Z_PUTHEX(i,c) {i=(c);SENDBYTE(hex[((i)&0xF0)>>4]);SENDBYTE(hex[(i)&0xF]);}
#define ZATTNLEN     32

static int net_problems;


int FTSC_sender (wz)
int wz;
{
   int j, wh;
   char req[120];
   long t1, t, olc;

   XON_DISABLE ();
   first_block = 0;

   if (!wz) {
      status_line(msgtxt[M_SEND_FALLBACK]);

      timeout = 0L;
      filetransfer_system ();
      update_filesio (0, 0);

      wh = wopen (12, 0, 24, 79, 0, LGREY|_BLACK, WHITE|_BLACK);
      wactiv (wh);
      wtitle ("OUTBOUND CALL STATUS", TLEFT, LCYAN|_BLACK);
      printc (12, 0, LGREY|_BLACK, 'Ã');
      printc (12, 52, LGREY|_BLACK, 'Á');
      printc (12, 79, LGREY|_BLACK, '´');
      whline (8, 0, 80, 0, LGREY|_BLACK);

      sprintf (req, "%u:%u/%u.%u, %s, %s, %s", remote_zone, remote_net, remote_node, remote_point, nodelist.sysop, nodelist.name, nodelist.city);
      if (strlen (req) > 78)
         req[78] = '\0';
      wcenters (0, LGREY|_BLACK, req);
      sprintf (req, "Connected at %u baud", rate);
      wcenters (1, LGREY|_BLACK, req);
      wcenters (2, LGREY|_BLACK, "AKAs: No aka presented");

      wprints (5, 2, LCYAN|_BLACK, "Files");
      wprints (6, 2, LCYAN|_BLACK, "Bytes");

      wprints (4, 9, LCYAN|_BLACK, " ÚÄÄÄÄMailPKTÄÄÄÄÄÄÄDataÄÄÄÄÄ¿");
      wprints (5, 9, LCYAN|_BLACK, "úúúúúúúúúúúúúúúúúúúúúúúúúúúúúúú");
      wprints (6, 9, LCYAN|_BLACK, "úúúúúúúúúúúúúúúúúúúúúúúúúúúúúúú");
      wprints (7, 9, LCYAN|_BLACK, " ÀÄÄÄÄÄÄINBOUND TRAFFICÄÄÄÄÄÄÙ");

      wrjusts (5, 20, YELLOW|_BLACK, "N/A");
      wrjusts (6, 20, YELLOW|_BLACK, "N/A");
      wrjusts (5, 31, YELLOW|_BLACK, "N/A");
      wrjusts (6, 31, YELLOW|_BLACK, "N/A");

      wprints (4, 44, LCYAN|_BLACK, " ÚÄÄÄÄMailPKTÄÄÄÄÄÄÄDataÄÄÄÄÄ¿");
      wprints (5, 44, LCYAN|_BLACK, "úúúúúúúúúúúúúúúúúúúúúúúúúúúúúúú");
      wprints (6, 44, LCYAN|_BLACK, "úúúúúúúúúúúúúúúúúúúúúúúúúúúúúúú");
      wprints (7, 44, LCYAN|_BLACK, " ÀÄÄÄÄÄOUTBOUND TRAFFICÄÄÄÄÄÄÙ");

      sprintf (req, "%d", call_list[next_call].n_mail);
      wrjusts (5, 55, YELLOW|_BLACK, req);
      sprintf (req, "%ld", call_list[next_call].b_mail);
      wrjusts (6, 55, YELLOW|_BLACK, req);
      sprintf (req, "%d", call_list[next_call].n_data);
      wrjusts (5, 66, YELLOW|_BLACK, req);
      sprintf (req, "%ld", call_list[next_call].b_data);
      wrjusts (6, 66, YELLOW|_BLACK, req);

      prints (7, 65, YELLOW|_BLACK, "FSC-0001");
      who_is_he = 0;
   }

   FTSC_sendmail ();
   t1 = timerset (1000);

   while ((!timeup (t1)) && CARRIER) {
      if ((j = PEEKBYTE()) >= 0) {
         switch (j) {
            case TSYNC:
               CLEAR_INBOUND();
               if (FTSC_recvmail())
                  goto get_out;
               t1 = timerset (1000);
               break;

            case SYN:
               CLEAR_INBOUND();
               SEA_recvreq();
               t1 = timerset (1000);
               break;

            case ENQ:
               CLEAR_INBOUND();
               SEA_sendreq();
               goto get_out;

            case NAK:
            case 'C':
               TIMED_READ(0);
               TIMED_READ(1);
               TIMED_READ(1);
               SENDBYTE(EOT);
               t1 = timerset (1000);
               break;

            default:
               TIMED_READ(0);
               SENDBYTE(EOT);
               break;
         }
      }
   }

   if (!CARRIER) {
      status_line(msgtxt[M_HE_HUNG_UP]);
      CLEAR_INBOUND();
      if (!wz)
         wclose ();
      return FALSE;
   }

   if (timeup(t1)) {
      FTSC_recvmail();
      status_line (msgtxt[M_TOO_LONG]);
   }

get_out:
   t1 = timerset (100);
   while (!timeup (t1));

   if (!wz) {
      if (cur_event > -1 && (e_ptrs[cur_event]->behavior & MAT_DYNAM)) {
         e_ptrs[cur_event]->behavior |= MAT_SKIP;
         write_sched ();
      }

      status_line (msgtxt[M_0001_END]);
      terminating_call();
      wclose ();

      t = time (NULL) - elapsed + 20L;
      olc = get_phone_cost (remote_zone, remote_net, remote_node, t);
      status_line ("*Session with %d:%d/%d.%d Time: %ld:%02ld, Cost: $%ld.%02ld", remote_zone, remote_net, remote_node, remote_point, t / 60L, t % 60L, olc / 100L, olc % 100L);

      HoldAreaNameMunge (call_list[next_call].zone);
      bad_call (call_list[next_call].net, call_list[next_call].node, -2, 0);
      sysinfo.today.completed++;
      sysinfo.week.completed++;
      sysinfo.month.completed++;
      sysinfo.year.completed++;
      sysinfo.today.outconnects += t;
      sysinfo.week.outconnects += t;
      sysinfo.month.outconnects += t;
      sysinfo.year.outconnects += t;
      sysinfo.today.cost += olc;
      sysinfo.week.cost += olc;
      sysinfo.month.cost += olc;
      sysinfo.year.cost += olc;

      if (got_arcmail) {
         if (cur_event > -1 && e_ptrs[cur_event]->errlevel[2])
            aftermail_exit = e_ptrs[cur_event]->errlevel[2];

         if (aftermail_exit) {
            status_line(msgtxt[M_EXIT_AFTER_MAIL],aftermail_exit);
            get_down (aftermail_exit, 3);
         }

         if (cur_event > -1 && (e_ptrs[cur_event]->echomail & (ECHO_PROT|ECHO_KNOW|ECHO_NORMAL|ECHO_EXPORT))) {
            if (modem_busy != NULL)
               mdm_sendcmd (modem_busy);

            t = time (NULL);

            import_sequence ();

            if (e_ptrs[cur_event]->echomail & ECHO_EXPORT) {
               if (config->mail_method) {
                  export_mail (NETMAIL_RSN);
                  export_mail (ECHOMAIL_RSN);
               }
               else
                  export_mail (NETMAIL_RSN|ECHOMAIL_RSN);
            }

            sysinfo.today.echoscan += time (NULL) - t;
            sysinfo.week.echoscan += time (NULL) - t;
            sysinfo.month.echoscan += time (NULL) - t;
            sysinfo.year.echoscan += time (NULL) - t;
         }
      }

      get_down(0, 2);
   }

   return(TRUE);
}

int FTSC_receiver (wz)
int wz;
{
   char fname[64], i, *HoldName, req[120];
   int havemail, done, wh;
   unsigned char j;
   long t1, t2, t;
   struct ffblk dt1;
   struct stat buf;

   first_block = 0;
   XON_DISABLE ();
   HoldName = HoldAreaNameMunge (called_zone);

   if (!wz) {
      status_line(msgtxt[M_RECV_FALLBACK]);

      timeout = 0L;
      filetransfer_system ();
      update_filesio (0, 0);

      wh = wopen (12, 0, 24, 79, 0, LGREY|_BLACK, WHITE|_BLACK);
      wactiv (wh);
      wtitle ("INBOUND CALL STATUS", TLEFT, LCYAN|_BLACK);
      printc (12, 0, LGREY|_BLACK, 'Ã');
      printc (12, 52, LGREY|_BLACK, 'Á');
      printc (12, 79, LGREY|_BLACK, '´');
      whline (8, 0, 80, 0, LGREY|_BLACK);

      sprintf (req, "Connected at %u baud", rate);
      wcenters (1, LGREY|_BLACK, req);

      wprints (5, 2, LCYAN|_BLACK, "Files");
      wprints (6, 2, LCYAN|_BLACK, "Bytes");

      wprints (4, 9, LCYAN|_BLACK, " ÚÄÄÄÄMailPKTÄÄÄÄÄÄÄDataÄÄÄÄÄ¿");
      wprints (5, 9, LCYAN|_BLACK, "úúúúúúúúúúúúúúúúúúúúúúúúúúúúúúú");
      wprints (6, 9, LCYAN|_BLACK, "úúúúúúúúúúúúúúúúúúúúúúúúúúúúúúú");
      wprints (7, 9, LCYAN|_BLACK, " ÀÄÄÄÄÄÄINBOUND TRAFFICÄÄÄÄÄÄÙ");

      wrjusts (5, 20, YELLOW|_BLACK, "N/A");
      wrjusts (6, 20, YELLOW|_BLACK, "N/A");
      wrjusts (5, 31, YELLOW|_BLACK, "N/A");
      wrjusts (6, 31, YELLOW|_BLACK, "N/A");

      wprints (4, 44, LCYAN|_BLACK, " ÚÄÄÄÄMailPKTÄÄÄÄÄÄÄDataÄÄÄÄÄ¿");
      wprints (5, 44, LCYAN|_BLACK, "úúúúúúúúúúúúúúúúúúúúúúúúúúúúúúú");
      wprints (6, 44, LCYAN|_BLACK, "úúúúúúúúúúúúúúúúúúúúúúúúúúúúúúú");
      wprints (7, 44, LCYAN|_BLACK, " ÀÄÄÄÄÄOUTBOUND TRAFFICÄÄÄÄÄÄÙ");

      prints (7, 65, YELLOW|_BLACK, "FSC-0001");
      who_is_he = 1;
   }

   CLEAR_INBOUND();

   done = 0;
   if (FTSC_recvmail ()) {
      if (!wz) {
         status_line (msgtxt[M_0001_END]);
         wclose ();
      }
      return 1;
   }

   HoldName = HoldAreaNameMunge (called_zone);
   sprintf (fname,"%s%04x%04x.?UT",HoldName,called_net,called_node);
   havemail = findfirst(fname,&dt1,0);

   if (havemail) {
      sprintf(fname,"%s%04x%04x.?LO",HoldName,called_net,called_node);
      havemail=findfirst(fname,&dt1,0);
   }

   if (havemail) {
      sprintf(fname,"%s%04x%04x.REQ",filepath,config->alias[assumed].net,config->alias[assumed].node);
      havemail=findfirst(fname,&dt1,0);
   }

   if (havemail && remote_point) {
      sprintf (fname,"%s%04x%04x.PNT\\%08x.?UT",HoldName,remote_net,remote_node, remote_point);
      havemail = findfirst(fname,&dt1,0);

      if (havemail) {
         sprintf(fname,"%s%04x%04x.PNT\\%08x.?LO",HoldName,remote_net,remote_node,remote_point);
         havemail=findfirst(fname,&dt1,0);
      }
   }

   if (havemail)
      status_line ("*No mail waiting for %d:%d/%d.%d", remote_zone, remote_net, remote_node, remote_point);
   else {
      status_line (msgtxt[M_GIVING_MAIL], remote_zone, remote_net, remote_node, remote_point);
      t1 = timerset(3000);
      j = 0;
      done = 0;
      while (!timeup(t1) && CARRIER && !done) {
         SENDBYTE(TSYNC);

         t2 = timerset (300);
         while (CARRIER && (!timeup(t2)) && !done) {
            i = TIMED_READ (0);

            switch (i) {
               case 'C':
               case 0x00:
               case 0x01:
                  if (j == 'C') {
                     done = 1;
                     FTSC_sendmail ();
                  }
                  break;

               case 0xfe:
                  if (j == 0x01) {
                     done = 1;
                     FTSC_sendmail ();
                  }
                  break;

               case 0xff:
                  if (j == 0x00) {
                     done = 1;
                     FTSC_sendmail ();
                  }
                  break;

               case NAK:
                  if (j == NAK) {
                     done = 1;
                     FTSC_sendmail ();
                  }
                  break;
            }
            if (i != -1)
               j = i;
         }
      }
   }

   sprintf( fname, "%s%04x%04x.REQ",HoldName,called_net,called_node);
   if (!stat(fname,&buf)) {
      t1 = timerset(3000);
      done = 0;
      while (!timeup(t1) && CARRIER && !done) {
         SENDBYTE(SYN);

         t2 = timerset (300);
         while (CARRIER && (!timeup(t2)) && !done) {
            i = TIMED_READ (0);

            switch (i) {
               case ENQ:
                  SEA_sendreq ();

               case CAN:
                  done = 1;
                  break;

               case 'C':
               case NAK:
                  SENDBYTE (EOT);
                  break;
            }
         }
      }
   }

   if (!no_requests)
      SEA_recvreq ();

   if (!wz) {
      if (cur_event > -1 && (e_ptrs[cur_event]->behavior & MAT_DYNAM)) {
         e_ptrs[cur_event]->behavior |= MAT_SKIP;
         write_sched ();
      }

      status_line (msgtxt[M_0001_END]);
      terminating_call();
      wclose ();

      t = time (NULL) - elapsed;
      status_line ("*Session with %d:%d/%d.%d Time: %ld:%02ld", remote_zone, remote_net, remote_node, remote_point, t / 60L, t % 60L);

      sysinfo.today.incalls++;
      sysinfo.week.incalls++;
      sysinfo.month.incalls++;
      sysinfo.year.incalls++;
      sysinfo.today.inconnects += t;
      sysinfo.week.inconnects += t;
      sysinfo.month.inconnects += t;
      sysinfo.year.inconnects += t;

      if (got_arcmail) {
         if (cur_event > -1 && e_ptrs[cur_event]->errlevel[2])
            aftermail_exit = e_ptrs[cur_event]->errlevel[2];

         if (aftermail_exit) {
            status_line(msgtxt[M_EXIT_AFTER_MAIL],aftermail_exit);
            get_down (aftermail_exit, 3);
         }

         if (cur_event > -1 && (e_ptrs[cur_event]->echomail & (ECHO_PROT|ECHO_KNOW|ECHO_NORMAL|ECHO_EXPORT))) {
            if (modem_busy != NULL)
               mdm_sendcmd (modem_busy);

            t = time (NULL);

            import_sequence ();

            if (e_ptrs[cur_event]->echomail & ECHO_EXPORT) {
               if (config->mail_method) {
                  export_mail (NETMAIL_RSN);
                  export_mail (ECHOMAIL_RSN);
               }
               else
                  export_mail (NETMAIL_RSN|ECHOMAIL_RSN);
            }

            sysinfo.today.echoscan += time (NULL) - t;
            sysinfo.week.echoscan += time (NULL) - t;
            sysinfo.month.echoscan += time (NULL) - t;
            sysinfo.year.echoscan += time (NULL) - t;
         }
      }

      get_down(0, 2);
   }

   return 1;
}

static int FTSC_sendmail ()
{
   FILE *fp;
   char fname[80], s[80], *sptr, *password, p, *HoldName;
   int c, i, j;
   long t1;
   struct stat buf;
   struct _pkthdr2 *pkthdr;
   struct date datep;
   struct time timep;
   long current, last_start;

   XON_DISABLE ();
   HoldName = HoldAreaNameMunge (called_zone);

   sptr = s;
   *ext_flags  = 'O';
   for (c = 0; c < NUM_FLAGS; c++) {
      if (caller && (ext_flags[c] == 'H'))
         continue;

      sprintf( fname, "%s%04x%04x.%cUT", HoldName, called_net, called_node, ext_flags[c]);

      if (!stat(fname,&buf))
         break;
   }

   if (c == NUM_FLAGS && remote_point) {
      sptr = s;
      *ext_flags  = 'O';
      for (c = 0; c < NUM_FLAGS; c++) {
         if (caller && (ext_flags[c] == 'H'))
            continue;

         sprintf( fname, "%s%04x%04x.PNT\\%08x.%cUT", HoldName, remote_net, remote_node, remote_point, ext_flags[c]);

         if (!stat(fname,&buf))
            break;
      }
   }

   invent_pkt_name (s);
   gettime (&timep);
   getdate (&datep);

   status_line(" Sending bundle to %d:%d/%d", called_zone, called_net, called_node);

   if (c == NUM_FLAGS) {
      sprintf (fname, "%s%04x%04x.OUT", HoldName, called_net, called_node);
      fp = fopen (fname, "wb");
      if (fp == NULL)
         return 1;

      pkthdr = (struct _pkthdr2 *) calloc (sizeof (struct _pkthdr2), 1);
      if (pkthdr == NULL) {
         status_line ("!Mem err in sending");
         fclose (fp);
         return 1;
      }

      memset ((char *)pkthdr, 0, sizeof (struct _pkthdr2));

      pkthdr->hour = timep.ti_hour;
      pkthdr->minute = timep.ti_min;
      pkthdr->second = timep.ti_sec;
      pkthdr->year = datep.da_year;
      pkthdr->month = datep.da_mon - 1;
      pkthdr->day = datep.da_day;
      pkthdr->ver = PKTVER;
      pkthdr->product = 0x4E;
      pkthdr->serial = 2 * 16 + 10;
      pkthdr->capability = 1;
      pkthdr->cwvalidation = 256;
      if (config->alias[assumed].point && config->alias[assumed].fakenet) {
         pkthdr->orig_node = config->alias[assumed].point;
         pkthdr->orig_net = config->alias[assumed].fakenet;
         pkthdr->orig_point = 0;
      }
      else {
         pkthdr->orig_node = config->alias[assumed].node;
         pkthdr->orig_net = config->alias[assumed].net;
         pkthdr->orig_point = config->alias[assumed].point;
      }
      pkthdr->orig_zone = config->alias[assumed].zone;
      pkthdr->orig_zone2 = config->alias[assumed].zone;
      pkthdr->dest_point = remote_point;
      pkthdr->dest_node = called_node;
      pkthdr->dest_net = called_net;
      pkthdr->dest_zone = called_zone;
      pkthdr->dest_zone2 = called_zone;

      if (get_bbs_record (called_zone, called_net, called_node, remote_point)) {
         strcpy (remote_password, nodelist.password);
         if (remote_password[0]) {
            strncpy (pkthdr->password, strupr (remote_password), 8);
         }
      }
      fwrite ((char *) pkthdr, sizeof (struct _pkthdr2), 1, fp);
      free (pkthdr);
      fwrite ("\0\0", 2, 1, fp);
      fclose (fp);
   }
   else {
      if (get_bbs_record (called_zone, called_net, called_node, remote_point)) {
         strcpy (remote_password, nodelist.password);
         fp = fopen (fname, "rb+");
         if (fp == NULL)
            return 1;
         pkthdr = (struct _pkthdr2 *) calloc (sizeof (struct _pkthdr2), 1);
         if (pkthdr == NULL) {
            status_line ("!Mem err in sending");
            return 1;
         }
         fread (pkthdr, 1, sizeof (struct _pkthdr2), fp);
         pkthdr->hour = timep.ti_hour;
         pkthdr->minute = timep.ti_min;
         pkthdr->second = timep.ti_sec;
         pkthdr->year = datep.da_year;
         pkthdr->month = datep.da_mon - 1;
         pkthdr->day = datep.da_day;
         if (remote_password[0])
            strncpy (pkthdr->password, strupr (remote_password), 8);

         fseek (fp, 0L, SEEK_SET);
         fwrite (pkthdr, 1, sizeof (struct _pkthdr), fp);
         fclose (fp);
         free (pkthdr);
      }
   }

   net_problems = send (fname, 'B');
   if ((net_problems == TSYNC) || (net_problems == 0)) {
      if (c == NUM_FLAGS)
         unlink (fname);
      return (net_problems);
   }

   unlink (fname);

   *ext_flags  = 'F';
   status_line (" Outbound file attaches");
   for(c=0; c<NUM_FLAGS+1; c++) {
      if (caller && (ext_flags[c] == 'H'))
         continue;

      if (c < NUM_FLAGS)
         sprintf( fname, "%s%04x%04x.%cLO",HoldName,called_net,called_node,ext_flags[c]);
      else
         sprintf( fname, "%s%04x%04x.REQ",filepath,config->alias[assumed].net, config->alias[assumed].node);

      if (!stat(fname,&buf)) {
         fp = fopen( fname, "rb+" );
         if (fp == NULL)
            continue;

         current  = 0L;
         while(!feof(fp)) {
            s[0] = 0;
            last_start = current;
            fgets(s,79,fp);

            sptr = s;
            password = NULL;

            for(i=0; sptr[i]; i++)
               if (sptr[i]=='!')
                  password = sptr+i+1;

            if (password) {
               password = sptr+i+1;
               for(i=0; password[i]; i++)
                  if (password[i]<=' ')
                     password[i]=0;
               if (strcmp(strupr(password),strupr(remote_password))) {
                  status_line("!RemotePwdErr %s %s",password,remote_password);
                  continue;
               }
            }

            for(i=0; sptr[i]; i++)
               if (sptr[i]<=' ')
                  sptr[i]=0;

            current = ftell(fp);

            if (sptr[0] == '#') {
               sptr++;
               i = TRUNC_AFTER;
            }
            else if (sptr[0] == '^') {
               sptr++;
               i = DELETE_AFTER;
            }
            else
               i = NOTHING_AFTER;

            if (!sptr[0])
               continue;

            if (sptr[0] != '~') {
               if (stat(sptr,&buf))
                       continue;
               else
                   if (!buf.st_size)
                       continue;

               j = xfermdm7 (sptr);

               p = 'B';
               if (j == 0) {
                  net_problems = 1;
                  return FALSE;
               }
               else if (j == 2)
                  p = 'F';

               if (!send (sptr, p)) {
                  fclose(fp);
                  net_problems   = 1;
                  return FALSE;
               }

               fseek( fp, last_start, SEEK_SET );
               putc('~',fp);
               fflush (fp);
               rewind(fp);
               fseek( fp, current, SEEK_SET );

               if (i == TRUNC_AFTER) {
                  i = cshopen(sptr,O_TRUNC,S_IWRITE);
                  close(i);
               }
               else if (i == DELETE_AFTER)
                  unlink (sptr);
            }
         }

         fclose(fp);
         unlink(fname);
      }
   }

   if (remote_point) {
      *ext_flags  = 'F';
      for (c = 0; c < NUM_FLAGS; c++) {
         if (caller && (ext_flags[c] == 'H'))
            continue;

         sprintf (fname, "%s%04x%04x.PNT\\%08x.%cLO", HoldName, remote_net, remote_node, remote_point, ext_flags[c]);

         if (!stat(fname,&buf)) {
            fp = fopen( fname, "rb+" );
            if (fp == NULL)
               continue;

            current  = 0L;
            while(!feof(fp)) {
               s[0] = 0;
               last_start = current;
               fgets(s,79,fp);

               sptr = s;
               password = NULL;

               for(i=0; sptr[i]; i++)
                  if (sptr[i]=='!')
                     password = sptr+i+1;

               if (password) {
                  password = sptr+i+1;
                  for(i=0; password[i]; i++)
                     if (password[i]<=' ')
                        password[i]=0;
                  if (strcmp(strupr(password),strupr(remote_password))) {
                     status_line("!RemotePwdErr %s %s",password,remote_password);
                     continue;
                  }
               }

               for(i=0; sptr[i]; i++)
                  if (sptr[i]<=' ')
                     sptr[i]=0;

               current = ftell(fp);

               if (sptr[0]=='#') {
                  sptr++;
                  i = TRUNC_AFTER;
               }
               else if (sptr[0] == '^') {
                  sptr++;
                  i = DELETE_AFTER;
               }
               else
                  i = NOTHING_AFTER;

               if (!sptr[0])
                  continue;

               if (sptr[0] != '~') {
                  if (stat(sptr,&buf))
                     continue;
                  else
                     if (!buf.st_size)
                        continue;

                  j = xfermdm7 (sptr);

                  p = 'B';
                  if (j == 0) {
                     net_problems = 1;
                     return FALSE;
                  }
                  else if (j == 2)
                     p = 'F';

                  if (!send (sptr, p)) {
                     fclose(fp);
                     net_problems   = 1;
                     return FALSE;
                  }

                  fseek( fp, last_start, SEEK_SET );
                  putc('~',fp);
                  fflush (fp);
                  rewind(fp);
                  fseek( fp, current, SEEK_SET );

                  if (i == TRUNC_AFTER) {
                     i = cshopen(sptr,O_TRUNC,S_IWRITE);
                     close(i);
                  }
                  else if (i == DELETE_AFTER)
                     unlink (sptr);
               }
            }

            fclose(fp);
            unlink(fname);
         }
      }
   }

   *sptr = 0;
   status_line (" End of outbound file attaches");
   t1 = timerset (100);
   while (CARRIER && !timeup(t1)) {
      j = TIMED_READ(0);
      if ((j == 'C') || (j == NAK)) {
         SENDBYTE (EOT);
         t1 = timerset (100);
      }
   }
   return TRUE;
}

static int FTSC_recvmail ()
{
   char fname[80], fname1[80], req[120], *p;
   struct _pkthdr2 pkthdr;
   struct _pkthdr22 *pkthdr22;
   FILE *fp;
   char done, i;
   int j;

   status_line (msgtxt[M_RECV_MAIL]);

   if (!CARRIER) {
      status_line(msgtxt[M_HE_HUNG_UP]);
      CLEAR_INBOUND();
      return (1);
   }

   XON_DISABLE ();

   status_line (" Inbound bundle");
   invent_pkt_name (fname1);

   CLEAR_INBOUND();
   SENDBYTE ('C');
   SENDBYTE (0x01);
   SENDBYTE (0xfe);
   if ((p = receive (filepath, fname1, 'B')) == NULL)
      return (1);

   if (!remote_capabilities) {
      sprintf (fname, "%s%s", filepath, p);
      fp = fopen (fname, "rb");
      if (fp == NULL) {
         status_line (msgtxt[M_PWD_ERR_ASSUMED]);
         return (1);
      }
      fread (&pkthdr, 1, sizeof (struct _pkthdr2), fp);
      fclose (fp);

      if (pkthdr.rate == 2) {
         pkthdr22 = (struct _pkthdr22 *)&pkthdr;
         remote_net = pkthdr22->orig_net;
         remote_node = pkthdr22->orig_node;
         remote_zone = pkthdr22->orig_zone;
         remote_point = pkthdr22->orig_point;
      }
      else {
         swab ((char *)&pkthdr.cwvalidation, (char *)&i, 2);
         pkthdr.cwvalidation = i;
         if (pkthdr.capability != pkthdr.cwvalidation || !(pkthdr.capability & 0x0001)) {
            remote_net = pkthdr.orig_net;
            remote_node = pkthdr.orig_node;
            remote_zone = pkthdr.orig_zone;
            remote_point = 0;
            if (!remote_zone) {
               for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++) {
                  if (config->alias[i].net == remote_net)
                     break;
               }

               if (i < MAX_ALIAS && config->alias[i].net) {
                  remote_zone = config->alias[i].zone;
                  assumed = i;
               }
               else
                  remote_zone = config->alias[0].zone;
            }
            else
               for (i = 0; i < MAX_ALIAS && config->alias[i].net; i++) {
                  if (config->alias[i].zone == remote_zone) {
                     assumed = i;
                     break;
                  }
               }
         }
         else {
            remote_net = pkthdr.orig_net;
            remote_node = pkthdr.orig_node;
            remote_zone = pkthdr.orig_zone2;
            remote_point = pkthdr.orig_point;
         }
      }

      if (get_bbs_record (remote_zone, remote_net, remote_node, remote_point)) {
         strcpy (remote_password, nodelist.password);
         sprintf (req, "%u:%u/%u.%u, %s, %s, %s", remote_zone, remote_net, remote_node, remote_point, nodelist.sysop, nodelist.name, nodelist.city);
         if (strlen (req) > 78)
            req[78] = '\0';
         wcenters (0, LGREY|_BLACK, req);
         wcenters (2, LGREY|_BLACK, "AKAs: No aka presented");
         status_line("%s: %s (%u:%u/%u)",msgtxt[M_REMOTE_SYSTEM],nodelist.name,remote_zone,remote_net,remote_node);
         if (config->know_filepath[0])
            filepath = config->know_filepath;
         if (config->know_okfile[0])
            request_list = config->know_okfile;
         max_requests = config->know_max_requests;
         max_kbytes = config->know_max_kbytes;
      }
      else {
         remote_password[0] = '\0';
         sprintf (req, "%u:%u/%u.%u, %s", remote_zone, remote_net, remote_node, remote_point, msgtxt[M_UNKNOWN_MAILER]);
         if (strlen (req) > 78)
            req[78] = '\0';
         wcenters (0, LGREY|_BLACK, req);
         wcenters (2, LGREY|_BLACK, "AKAs: No aka presented");
         status_line("%s: %s (%u:%u/%u)",msgtxt[M_REMOTE_SYSTEM],msgtxt[M_UNKNOWN_MAILER],remote_zone,remote_net,remote_node);
      }
      sprintf (req, "Connected at %u baud with %s", rate, prodcode[pkthdr.product]);
      wcenters (1, LGREY|_BLACK, req);
      if (remote_password[0] && stricmp (remote_password, pkthdr.password)) {
         status_line ("!Password Error: expected '%s' got '%s'",remote_password, pkthdr.password);
         strcpy (fname1, fname);
         j = strlen (fname) - 3;
         strcpy (&(fname[j]), "Bad");
         if (rename (fname1, fname))
            status_line (msgtxt[M_CANT_RENAME_MAIL], fname1);
         else
            status_line (msgtxt[M_MAIL_PACKET_RENAMED], fname);
         return (1);
      }
      else {
         status_line (msgtxt[M_PROTECTED_SESSION]);
         if (config->prot_filepath[0])
            filepath = config->prot_filepath;
         if (config->prot_okfile[0])
            request_list = config->prot_okfile;
         max_requests = config->prot_max_requests;
         max_kbytes = config->prot_max_kbytes;
      }
   }

   called_zone = remote_zone;
   if (remote_point) {
      called_net = config->alias[assumed].fakenet;
      called_node = remote_point;
   }
   else {
      called_net = remote_net;
      called_node = remote_node;
   }

   done = 0;
   status_line (" Inbound file attaches");
   do {
      if ((i = try_sealink ()) == 0) {
         if (!recvmdm7 (fname))
            done = 1;
         else {
            if (!receive (filepath, fname, 'T'))
               done = 1;
            else
               got_arcmail = 1;
         }
      }
      else {
         if (i == 1) {
            if (!receive (filepath, NULL, 'F'))
               done = 1;
            else
               got_arcmail = 1;
         }
         else
            done = 1;
      }
   } while (!done && CARRIER);

   status_line (" End of inbound file attaches");
   CLEAR_INBOUND();
   return (0);
}

static int SEA_sendreq ()
{
	char fname[80], reqf[80];
        char *p, *name, *pw, *HoldName;
	int i, j, done, done1, nfiles;
	FILE *fp;
	long t1;

	t1 = timerset (1000);
        HoldName = HoldAreaNameMunge (called_zone);

        sprintf(fname,"%s%04x%04x.REQ",HoldName,called_net,called_node);

	if (!dexists(fname))
		status_line (":No outgoing file requests");
	else {
                status_line (msgtxt[M_MAKING_FREQ]);
		if ((fp = fopen (fname, "r")) == NULL) {
			SENDBYTE(ETB);
			return (1);
		}

                while ((fgets (reqf, 79, fp) != NULL) && (CARRIER)) {
			p = reqf+strlen(reqf)-1;
			while ((p>=reqf)&&(isspace(*p)))
				*p-- = '\0';

			p = reqf;
			while ((*p) && (isspace (*p)))
				p++;
			name = p;

			if (*name == ';')
				continue;

			while ((*p) && (!isspace (*p)))
				p++;
			if (*p) {
				*p = '\0';
				++p;
				while ((*p) && (*p != '!'))
					p++;
				if (*p == '!')
					*p = ' ';
				pw = p;
			}
			else
				pw = p;

			if (req_out (name, pw))
				continue;

			t1 = timerset (1000);
			done = 0;
                        while ((!timeup (t1)) && CARRIER && !done) {
				j = TIMED_READ(0);
				if (j >= 0) {
					if (j == ACK) {
						nfiles = 0;
						done1 = 0;
						do {
							if ((i = try_sealink ()) == 0) {
								if (!recvmdm7 (reqf))
									done1 = 1;
								else {
                                                                        if (!receive (filepath, reqf, 'T'))
										done1 = 1;
									else
										++nfiles;
								}
							}
							else
							    if (i == 1) {
                                                                if (!receive (filepath, NULL, 'F'))
									done1 = 1;
								else
									++nfiles;
							}
							else
							    done1 = 1;

						} 
                                                while (CARRIER && !done1);

						status_line (":Received %d files", nfiles);
						done = 1;
						t1 = timerset (1000);

                                                while ((TIMED_READ(0) != ENQ) && (!timeup (t1)) && CARRIER);
					}
					else
						if (j == ENQ)
							req_out (name, pw);
				}
			}
		}
		fclose (fp);
		unlink (fname);
		status_line (":End of outbound file requests");
	}

	SENDBYTE(ETB);
	return(0);
}

static int SEA_recvreq ()
{
	int done, i, j, recno, retval, nfiles, nfiles1, w_event;
        char p, reqs[64], req[64];
	long t1;

	w_event = -1;
	t1 = timerset (2000);

	if (no_requests) {
		SENDBYTE(CAN);
                status_line (msgtxt[M_REFUSING_IN_FREQ]);
		return TRUE;
	}

	done = 0;
	nfiles = 0;
	status_line (":Inbound file requests");
        while (CARRIER && !done && (!timeup (t1))) {
		SENDBYTE(ENQ);

		j = TIMED_READ(2);

		switch (j) {
		case ACK:
			recno = -1;
			nfiles1 = 0;
			if ((retval = get_req_str (reqs, req, &recno)) > 0) {
                                if ((w_event == 32000)) { /* || (nfiles > n_requests)) {*/
					status_line ("!File Request denied");
					SENDBYTE (ACK);
					send (NULL, 'S');
					recno = -1;
				}
				else {
					SENDBYTE(ACK);
					do {
						if (reqs[0])
							i = xfermdm7 (reqs);
						else
						    i = 2;

						p = 'T';
						if (i == 0) {
							net_problems = 1;
							continue;
						}
						else
						    if (i == 2)
							p = 'F';

						if (retval == 1) {
							send (reqs, p);
							++nfiles;
							++nfiles1;
						}
                                                if (nfiles > max_requests && max_requests) {
                                                        status_line (msgtxt[M_FREQ_LIMIT]);
							recno = -1;
						}
						else
						    if (gen_req_name (reqs, req, &recno) == 2)
							recno = -1;
					} 
                                        while (CARRIER && (recno >= 0));
				}

				if (retval != 1)
					send (NULL, 'S');
				status_line (":%d matching files sent", nfiles1);
			}
			t1 = timerset (2000);
			break;

		case ETB:
		case ENQ:
			done = 1;
			break;

		case 'C':
		case NAK:
			SENDBYTE(EOT);
			CLEAR_INBOUND();
			break;
		}
	}
	status_line (":End of inbound file requests");
	return TRUE;
}

static int try_sealink ()
{
	int i, j;
	long t1;

	for (i = 0; i < 5; i++) {
		SENDBYTE ('C');

		t1 = timerset (100);
                while (!timeup (t1) && CARRIER) {
			if ((j = PEEKBYTE()) >= 0) {
				if (j == SOH)
					return (1);
				j = TIMED_READ(0);
				if (j == EOT)
					return (2);
				else
				    if (j == TSYNC)
					return (0);
			}
		}

                if (!CARRIER)
			break;
	}

	return (0);
}

static int req_out (name, pw)
char *name, *pw;
{
        char *p;
	unsigned int crc;

	p = name;
	if (!*p)
		return (1);

	status_line ("*Requesting '%s' %s%s", name, (*pw)?"with password":"", pw);
	SENDBYTE(ACK);
	crc = 0;
	while (*p) {
		SENDBYTE(*p);
		crc = xcrc(crc,(byte )(*p));
		++p;
	}

	SENDBYTE(' ');
	crc = xcrc(crc,(byte )(' '));
	SENDBYTE('0');
	crc = xcrc(crc,(byte )('0'));
	p = pw;
	while (*p) {
		SENDBYTE(*p);
		crc = xcrc(crc,(byte )(*p));
		++p;
	}

	SENDBYTE(ETX);
	SENDBYTE( crc&0xff );
	SENDBYTE( crc>>8   );
	return (0);
}

static int get_req_str (reqs, req, recno)
char *reqs, *req;
int *recno;
{
	unsigned int crc, crc1, crc2, crc3;
	int i,j;

	crc = i = 0;
        while (CARRIER) {
		j = TIMED_READ(2);
		if (j < 0)
			return (0);

		if (j == ETX) {
			crc1 = TIMED_READ(2);
			crc2 = TIMED_READ(2);
			crc3 = (crc2<<8)+crc1;
			if (crc3 != crc) {
				status_line("!Bad crc - trying again");
				return (0);
			}
			req[i] = '\0';
			return (gen_req_name (reqs, req, recno));
		}
		else {
			req[i++] = j&0xff;
			crc = xcrc (crc,j&0xff);
		}
	}
	return (0);
}

static int gen_req_name(reqs,req,recno)
char *reqs, *req;
int *recno;
{
	char *q, *q1;
	struct stat  st;
	char buf[32];
        char *rqname;
	long fsecs;
	int save_rec;

	save_rec = *recno;
	q = req;
	q1 = buf;
	while((*q) && (!isspace(*q)))
		*q1++ = *q++;

	if(*q) {
		++q;

		fsecs = atol(q);

		while((*q) && isdigit (*q))
			++q;

		if(*q) {
			++q;

			*q1++ = ' ';
			*q1++ = '!';
			while(*q)
				*q1++ = *q++;
			*q1++ = '\0';
		}
	}

	for(;;) {
                if ((rqname = n_frproc (buf, recno, (fsecs == 0) ? 0 : 1)) != NULL) {
			if(!stat(rqname,&st)) {
				if(st.st_atime - timezone <= fsecs)
					continue;
			}
			strcpy(reqs,rqname);
			return(1);
		}
		else {
			q = buf;
			while((*q) && !isspace (*q))
				++q;
			*q = '\0';
			if(save_rec == -1)
				status_line("!No files matched '%s'",buf);
			reqs[0] = '\0';
			return(2);
		}
	}
}

int xfermdm7(fn)
char *fn;
{
   unsigned char checksum;
   int i, j;
   unsigned char mdm7_head[13];
   char num_tries = 0;
   char *fname;
   struct ffblk dta;

   XON_DISABLE();
   _BRK_DISABLE();

   findfirst (fn, &dta, 0);
   fname = dta.ff_name;

   memset (mdm7_head, ' ', 12);
   for (i = j = 0; ((fname[i]) && (i < 12) && (j < 12));i ++) {
      if (fname[i] == '.')
         j = 8;
      else
         mdm7_head[j++] = (char )toupper (fname[i]);
   }

   checksum = SUB;
   for (i = 0; i < 11; i++)
      checksum += mdm7_head[i];

top:
   if (!CARRIER)
      return 0;
   else {
      if (num_tries++ > 10) {
         send_can ();
         return (0);
      }
      else
         if (num_tries)
            SENDBYTE ('u');
   }

   for (i = 0; i < 15; i++) {
      if (!CARRIER)
         return (0);

      j = TIMED_READ (10);

      switch (j) {
         case 'C' :
            SENDBYTE (SOH);
            return 2;
         case NAK :
            i=16;
            break;
         case CAN :
            return (0);
      }
   }

   SENDBYTE (ACK);

   for (i = 0; i < 11; i++) {
      SENDBYTE(mdm7_head[i]);

      switch (j = TIMED_READ(10)) {
         case ACK :
            break;
         case CAN :
            SENDBYTE (ACK);
            return (0);
         default  :
            goto top;
      }
   }

try_sub:
   SENDBYTE (SUB);
   if ((i = TIMED_READ (10)) != checksum)
      goto top;

   SENDBYTE (ACK);
   return (1);
}

static int recvmdm7(fname)
char *fname;
{
	register int i, j;
	int got_dot, retbyte, tries, got_eot;
	char tempname[30];
	unsigned char xchksum;

        _BRK_DISABLE();
	XON_DISABLE();

	tries = got_eot = 0;

	if(PEEKBYTE() == -1)
		SENDBYTE(NAK);


top:
	i = got_dot = 0;
	tries++;

	memset(tempname,0,30);
	memset(fname,0,30);

        while((CARRIER) && (tries<8)) {
		switch(retbyte = TIMED_READ(3)) {
		case SUB :
			if(!i) {
				if(tries<4)
					goto top;
				else
					return(0);
			}

			for(xchksum=SUB,i=0;tempname[i];i++)
				xchksum += tempname[i];
			CLEAR_INBOUND();
			SENDBYTE(xchksum);
			retbyte = TIMED_READ(5);
			if(retbyte==ACK) {
				for(i=j=got_dot=0;((tempname[i]) && (i<30));i++) {
					if(tempname[i]=='.')
						got_dot=1;
					else
					    if ((i==8)&&(!got_dot))
						fname[j++] = '.';
					fname[j++] = tempname[i];
				}
				return(1);
			}

			got_eot = 0;
			SENDBYTE(NAK);
			goto top;

		case 'u' :
		case ACK :
			goto top;

		case EOT :
			SENDBYTE(ACK);
			if (got_eot>2)
				return(0);

			got_eot++;
			goto top;

		case CAN :
			goto fubar;

		default  :
			if(retbyte<' ') {
				if(got_eot>2)
					return(0);
				else
				    got_eot++;

				CLEAR_INBOUND();

				SENDBYTE(NAK);
				goto top;
			}
			if(i >= 30)
				goto fubar;
			if((retbyte>=' ') && (retbyte<='~'))
				tempname[i++]= (char )retbyte;
			SENDBYTE(ACK);
			break;
		}
	}

fubar:
	return(0);
}


