#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <dir.h>
#include <ctype.h>
#include <process.h>
#include <sys\stat.h>

#include <cxl\cxlwin.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"

#define NUM_FLAGS      4
#define isITALY        0x4E
#define MAX_EMSI_ADDR  20

char *n_frproc(char *, int *, int);
int prep_match(char *,char *);
int match(char *,char *);
int send_WaZOO(int);
void packet_bundle(char *, int);
void mail_report(char *, char *, long, int);
int get_emsi_handshake (int);
int send_emsi_handshake (int);
void get_emsi_id (char *, int);

static void description(char *, int);

extern char remote_system[50];
extern char remote_sysop[20];
extern char remote_program[60];
extern struct _alias addrs[MAX_EMSI_ADDR];

static int emsi, emsi_sent;

void WaZOO(originator)
int originator;
{
   int stat, np, wh, i;
   char req[80];

   stat = 0;
   got_arcmail = 0;
   made_request = 0;
   emsi = 0;

   if (originator && ((remote_zone != called_zone) ||
                      (remote_net != called_net) ||
                      (remote_node != called_node)))
      status_line (msgtxt[M_CALLED], called_zone,called_net,called_node,
                                     remote_zone,remote_net,remote_node);
   else {
      called_zone = remote_zone;
      called_net = remote_net;
      called_node = remote_node;
   }

   if (!CARRIER)
           return;

   wh = wopen(7,12,13,66,1,LCYAN|_BLUE,LCYAN|_BLUE);
   wactiv(wh);
   whline(3,0,53,2,LCYAN|_BLUE);

   wtitle(" WaZOO Session ", TLEFT, LCYAN|_BLUE);
   sprintf(req, "System: %s", remote_system);
   wprints(0,1,LCYAN|_BLUE,req);
   sprintf(req, "Sysop:  %s", remote_sysop);
   wprints(1,1,LCYAN|_BLUE,req);
   sprintf(req, "Address: %u:%u/%u.%u", remote_zone, remote_net, remote_node, remote_point);
   wrjusts(1,51,LCYAN|_BLUE,req);
   wprints(2,1,LCYAN|_BLUE,"Method:");
   wprints(4,1,LCYAN|_BLUE,remote_program);

   if (remote_net == alias[assumed].net && remote_node == alias[assumed].node && remote_point) {
      called_net = alias[assumed].fakenet;
      called_node = remote_point;
   }
   else if (remote_point)
      called_net = -1;

   if ((remote_capabilities & DOES_IANUS) && strstr (mdm_flags, "Hst") == NULL) {
      status_line (msgtxt[M_WAZOO_METHOD], "Janus");
      wprints(2,9,LCYAN|_BLUE,"Janus");
      isOriginator = originator;
      Janus ();
      goto endwazoo;
   }

   if (remote_capabilities & ZED_ZAPPER) {
      status_line(msgtxt[M_WAZOO_METHOD], "ZedZap");
      wprints(2,9,LCYAN|_BLUE,"ZedZap");
      remote_capabilities &= ~ZED_ZIPPER;
   }
   else if (remote_capabilities & ZED_ZAPPER) {
      status_line(msgtxt[M_WAZOO_METHOD], "ZedZip");
      wprints(2,9,LCYAN|_BLUE,"ZedZip");
      remote_capabilities &= ~ZED_ZAPPER;
   }
   else {
      status_line(msgtxt[M_WAZOO_METHOD], "DietIfna");
      wprints(2,9,LCYAN|_BLUE,"DietIfna");
      if (originator)
         FTSC_sender(1);
      else
         FTSC_receiver(1);
      goto endwazoo;
   }

   for (np = 0; np < 10; np++) {
      if (alias[np].net == 0)
         break;

      sprintf(req,request_template,filepath,alias[np].net,alias[np].node);
      unlink(req);
   }

   flag_file (SET_SESSION_FLAG, called_zone, called_net, called_node, 0);

   if(originator) {
      send_WaZOO(originator);
      if (!CARRIER)
         goto endwazoo;
      if (!get_Zmodem(filepath, NULL))
         goto endwazoo;
      if (!CARRIER)
         goto endwazoo;
      stat  = respond_to_file_requests(fsent);
      if (stat)
         send_Zmodem(NULL,NULL,((stat)?END_BATCH:NOTHING_TO_DO),DO_WAZOO);
   }
   else {
      if (!get_Zmodem(filepath, NULL))
         goto endwazoo;
      if (!CARRIER)
         goto endwazoo;
      send_WaZOO(originator);
      if (!CARRIER || !made_request)
         goto endwazoo;
      get_Zmodem(filepath, NULL);
   }

endwazoo:
   memset ((char *)&usr, 0, sizeof (struct _usr));
   caller = 0;

   if (!CARRIER)
           status_line(msgtxt[M_NO_CARRIER]);

   if (!(remote_capabilities & DOES_IANUS))
      flag_file (CLEAR_SESSION_FLAG,called_zone,called_net,called_node,0);

   wactiv(wh);
   wclose();
   wunlink(wh);

   status_line (msgtxt[M_WAZOO_END]);
   terminating_call();

   if (got_arcmail) {
      if (cur_event > -1 && e_ptrs[cur_event]->errlevel[2])
         aftermail_exit = e_ptrs[cur_event]->errlevel[2];

      if (!aftermail_exit)
         aftermail_exit = aftercaller_exit;

      if (cur_event > -1 && (e_ptrs[cur_event]->echomail & ECHO_IMPORT)) {
         if (modem_busy != NULL)
            mdm_sendcmd (modem_busy);

         i = 1;
         if ( (e_ptrs[cur_event]->echomail & (ECHO_PROT|ECHO_KNOW|ECHO_NORMAL)) ) {
            if (prot_filepath != NULL && (e_ptrs[cur_event]->echomail & ECHO_PROT))
               import_mail (prot_filepath, &i);
            if (know_filepath != NULL && (e_ptrs[cur_event]->echomail & ECHO_KNOW))
               import_mail (know_filepath, &i);
            if (norm_filepath != NULL && (e_ptrs[cur_event]->echomail & ECHO_NORMAL))
               import_mail (norm_filepath, &i);
         }
         else {
            if (prot_filepath != NULL)
               import_mail (prot_filepath, &i);
            if (know_filepath != NULL)
               import_mail (know_filepath, &i);
            if (norm_filepath != NULL)
               import_mail (norm_filepath, &i);
         }

         i = 2;
         import_mail (".\\", &i);
         if (e_ptrs[cur_event]->echomail & ECHO_EXPORT)
            export_mail (NETMAIL_RSN|ECHOMAIL_RSN);
      }
      else {
         status_line(msgtxt[M_EXIT_AFTER_MAIL],aftermail_exit);
         get_down (aftermail_exit, 3);
      }
   }

   get_down(aftercaller_exit, 2);
}

/*--------------------------------------------------------------------------*/
/* SEND WaZOO (send another WaZOO-capable Opus its mail)                    */
/*   returns TRUE (1) for good xfer, FALSE (0) for bad                      */
/*   use instead of n_bundle and n_attach for WaZOO Opera                   */
/*--------------------------------------------------------------------------*/
int send_WaZOO(caller)
int caller;
{
   FILE *fp;
   char fname[80], s[80], *sptr, *password, *HoldName;
   int c, i;
   struct stat buf;
   long current, last_start;

   if (emsi)
      fsent = emsi_sent;
   else
      fsent = 0;

   HoldName = HoldAreaNameMunge(called_zone);

   if (!emsi) {
      if (flag_file (TEST_AND_SET, called_zone, called_net, called_node, 1))
         goto done_send;
   }

   *ext_flags  = 'O';
   for (c = 0; c < NUM_FLAGS; c++) {
      if (caller && (ext_flags[c] == 'H'))
         continue;

      sprintf(fname,"%s%04x%04x.%cUT",HoldName,called_net,called_node,ext_flags[c]);

      if (!stat(fname,&buf)) {
         invent_pkt_name(s);
         if (!send_Zmodem(fname,s,fsent++,DO_WAZOO))
            goto bad_send;
         unlink(fname);
      }
   }

   if (remote_point) {
      for (c = 0; c < NUM_FLAGS; c++) {
         if (caller && (ext_flags[c] == 'H'))
            continue;

         sprintf(fname,"%s%04x%04x.PNT\\%08x.%cUT",HoldName,remote_net,remote_node,remote_point,ext_flags[c]);

         if (!stat(fname,&buf)) {
            invent_pkt_name(s);
            if (!send_Zmodem(fname,s,fsent++,DO_WAZOO))
               goto bad_send;
            unlink(fname);
         }
      }
   }

   *ext_flags  = 'F';
   for(c=0; c<NUM_FLAGS; c++) {
      if (caller && (ext_flags[c] == 'H'))
         continue;

      sprintf(fname,"%s%04x%04x.%cLO",HoldName,called_net,called_node,ext_flags[c]);

      if(!stat(fname,&buf)) {
         fp = fopen(fname,"rb+");
         if(fp == NULL)
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
                  if (sptr[i]<=' ') sptr[i]=0;

               current = ftell(fp);

               if (sptr[0]=='#') {
                  sptr++;
                  i = TRUNC_AFTER;
               }
               else if (sptr[0]=='^') {
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

                  if (!send_Zmodem(sptr,NULL,fsent++,DO_WAZOO)) {
                     fclose(fp);
                     goto bad_send;
                  }

                  if (i == DELETE_AFTER)
                     unlink(sptr);
                  else if (i == TRUNC_AFTER) {
                     unlink(sptr);
                     i = creat(sptr,S_IREAD|S_IWRITE);
                     close(i);
                  }

                  fseek(fp,last_start,SEEK_SET);
                  putc('~',fp);
                  fflush(fp);
                  rewind(fp);
                  fseek(fp,current,SEEK_SET);
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

         if (!stat (fname, &buf)) {
            fp = fopen(fname,"rb+");
            if (fp == NULL)
               continue;

            current = 0L;
            while (!feof (fp)) {
               s[0] = 0;
               last_start = current;
               fgets(s,79,fp);
               sptr = s;
               password = NULL;

               for (i = 0; sptr[i]; i++)
                  if (sptr[i] == '!')
                     password = sptr + i + 1;

               if (password) {
                  password = sptr + i + 1;

                  for(i=0; password[i]; i++)
                     if (password[i] <= ' ')
                        password[i] = 0;

                  if (strcmp (strupr (password), strupr (remote_password))) {
                     status_line ("!RemotePwdErr %s/%s", password, remote_password);
                     continue;
                  }
               }

               for (i = 0; sptr[i]; i++)
                  if (sptr[i] <= ' ')
                     sptr[i] = 0;

               current = ftell(fp);

               if (sptr[0]=='#') {
                  sptr++;
                  i = TRUNC_AFTER;
               }
               else if (sptr[0]=='^') {
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

                  if (!send_Zmodem (sptr, NULL, fsent++, DO_WAZOO)) {
                     fclose (fp);
                     goto bad_send;
                  }

                  if (i == DELETE_AFTER)
                     unlink(sptr);
                  else if (i == TRUNC_AFTER) {
                     unlink (sptr);
                     i = creat(sptr,S_IREAD|S_IWRITE);
                     close (i);
                  }

                  fseek(fp,last_start,SEEK_SET);
                  putc('~',fp);
                  fflush(fp);
                  rewind(fp);
                  fseek(fp,current,SEEK_SET);
               }
            }
            fclose(fp);
            unlink(fname);
         }
      }
   }

   sprintf(fname,request_template,HoldName,called_net,called_node);

   if (!stat(fname,&buf)) {
      if(!(remote_capabilities & WZ_FREQ) || (cur_event > -1 && (e_ptrs[cur_event]->behavior & MAT_NOOUTREQ)) )
         status_line(msgtxt[M_FREQ_DECLINED]);
      else {
         status_line(msgtxt[M_MAKING_FREQ]);
         if(send_Zmodem(fname,NULL,fsent++,DO_WAZOO)) {
            unlink(fname);
            made_request = 1;
         }
      }
   }

   if (!emsi)
      fsent = respond_to_file_requests(fsent);

   if (!fsent)
      status_line (msgtxt[M_NOTHING_TO_SEND], remote_zone, remote_net, remote_node, remote_point);

done_send:
   if (!emsi)
      flag_file (CLEAR_FLAG, called_zone, called_net, called_node, 1);

   if (!emsi)
      send_Zmodem(NULL,NULL,((fsent)?END_BATCH:NOTHING_TO_DO),DO_WAZOO);
   else
      emsi_sent = fsent;
   return(TRUE);

bad_send:
   if (!emsi)
      flag_file (CLEAR_FLAG, called_zone, called_net, called_node, 1);

   return (FALSE);
}

/*--------------------------------------------------------------------------*/
/* RESPOND TO FILE REQUEST                                                  */
/*--------------------------------------------------------------------------*/
int respond_to_file_requests(fsent)
int fsent;
{
   char  req[80],s[14];
   struct stat  buf, buf2;
   FILE *fp = NULL;
   int np, nfiles, junk;
   char *rqname;

   nfiles = 0;

   for(np = 0; np < MAX_ALIAS; np++) {
      if (alias[np].net == 0)
              break;

      sprintf(req,request_template,filepath,alias[np].net,alias[np].node);

      if(!stat(req,&buf)) {
         fp = fopen(req,"rt");
         if(fp == NULL)
            goto done;

         while(!feof(fp)) {
            req[0] = 0;
            if(fgets(req,79,fp) == NULL)
               break;

            if(req[0] == ';')
               continue;

            junk=-1;

            do {
               if(nfiles > max_requests && max_requests) {
                  status_line (msgtxt[M_FREQ_LIMIT]);
                  description (req, -1);
                  break;
               }

               if ((rqname = n_frproc (req, &junk, 0)) != NULL) {
                  if (stat(rqname,&buf2) == -1)
                     continue;

                  if ((long)max_kbytes  > (buf2.st_size / 1024L) || !max_kbytes) {
                     if (max_kbytes)
                        max_kbytes -= (int)(buf2.st_size / 1024L);

                     if ((remote_capabilities & DOES_IANUS) && strstr (mdm_flags, "Hst") == NULL) {
                        if (record_reqfile(rqname) == FALSE)
                           goto janus_done;
                        description (rqname, nfiles);
                        ++fsent;
                     }
                     else {
                        if(!send_Zmodem (rqname, NULL, ++fsent, DO_WAZOO))
                           goto janus_done;
                        description (rqname, nfiles);
                     }

                     ++nfiles;
                  }
                  else {
                     status_line (msgtxt[M_BYTE_LIMIT]);
                     description (rqname, -1);
                  }
               }
               else {
                  if ((rqname = strchr (req, '\n')) != NULL && junk >= 0) {
                     *rqname = '\0';
                     description (req, -2);
                  }
               }
            } while ((junk >= 0));

            if (nfiles > max_requests && max_requests)
               break;
         }

         fclose (fp);
         fp = NULL;

         sprintf(req,request_template,filepath,alias[np].net,alias[np].node);
         unlink(req);
      }

      if (nfiles > max_requests && max_requests)
         break;
   }

done:
   if (nfiles && !(remote_capabilities & DOES_IANUS)) {
      status_line(msgtxt[M_ILLEGAL_CARRIER]);
      mail_report(NULL, NULL, 0L, -1);
      packet_bundle(req, 1);
      invent_pkt_name(s);
      send_Zmodem(req,s,fsent++,DO_WAZOO);
      unlink(req);
   }

janus_done:
   sprintf(req,request_template,filepath,alias[np].net,alias[np].node);
   unlink(req);

   sprintf(req,"TMP%d.TXT", line_offset);
   unlink(req);

   if(fp != NULL)
      fclose(fp);

   return(fsent);
}

void invent_pkt_name(string)
char *string;
{
   long t;

   t = time (NULL);
   sprintf (string, "%08lX.PKT", t);
}

char *n_frproc(request, recno, updreq)
char *request;
int *recno, updreq;
{
   register int i, j;
   char s1[80], *sptr, *their_pwd, required_pwd[10], *after_pwd;
   char our_wildcard[15], their_wildcard[15], magic[15];
   static char file[80];
   FILE *approved;
   struct ffblk dta;

   approved = NULL;
   their_pwd = NULL;
   after_pwd = NULL;
   strcpy (s1, request);

   for (i = 0; request[i]; i++) {
      if ((request[i] == ' ') || (request[i] == 0x09)) {
         request[i] = '\0';
         if (request[i+1]=='!') {
            their_pwd = request+i+2;
            if(strlen(their_pwd)>6)
               their_pwd[6] = '\0';
               strupr(their_pwd);
         }
      }
      else
         if(request[i]<=' ')
            request[i] = '\0';
   }

   if(!request[0])
      return(NULL);

   if(their_pwd)
      for(i=0;their_pwd[i];i++)
         if(their_pwd[i] <= ' ')
            their_pwd[i] = '\0';

   i = 0;
   sptr = NULL;

   strupr(request);
   if(*recno == -1)
      status_line("*%s request (%s)", updreq?"Update":"File", request);

   if(*recno < 0) {
      if(!strcmp(request,"FILES")) {
         if(availist)
            strcpy(file,availist);
         else {
            file[0] = '\0';
            sptr = msgtxt[M_NO_AVAIL];
         }
         goto avail;
      }
      else if(!strcmp(request,"ABOUT")) {
         if(about)
            strcpy(file,about);
         else {
            file[0] = '\0';
            sptr = msgtxt[M_NO_ABOUT];
         }
         goto avail;
      }
   }

   prep_match(request,their_wildcard);

   approved = fopen(request_list,"rt");
   if(approved == NULL)
      goto err;

   j = -1;
   magic[0] = '\0';
   while(!feof(approved)) {
      if(magic[0]) {
         strcpy(their_wildcard,magic);
         magic[0] = '\0';
      }

      file[0] = '\0';
      required_pwd[0] = '\0';

      if(fgets(file,78,approved) == NULL)
         break;

      for(i=0;file[i];i++) {
         if(file[i]==0x09)
            file[i] = ' ';
         else
            if(file[i]<' ')
               file[i] = '\0';
      }

      if(!file[0])
         continue;

      if((file[0] == '$') || (file[0] == ';'))
         continue;

      for(i=0;file[i];i++) {
         if(file[i]==' ') {
            file[i] = '\0';
            if(file[i+1]=='!') {
               strncpy(required_pwd,&file[i+2],8);
               if(strlen(required_pwd)>6)
                  required_pwd[6] = '\0';

               after_pwd = &file[i+1];
               while(*after_pwd && (!isspace(*after_pwd)))
                  ++after_pwd;

               if(*after_pwd)
                  ++after_pwd;

               for(i=0;required_pwd[i];i++)
                  if(required_pwd[i]<=' ')
                     required_pwd[i] = '\0';

                  break;
            }
            else
               after_pwd = &file[i+1];
         }
         else
            if(file[i]<' ')
               file[i] = '\0';
      }

      if(!file[0])
         continue;

      if(file[0] == '@') {
         if(stricmp(&file[1],request) == 0) {
            strcpy(magic,their_wildcard);
            strcpy(file,after_pwd);
            strcpy(their_wildcard, "????????.???");
         }
         else
            continue;
      }

      if(!findfirst(file,&dta,0)) {
         do {
            prep_match(dta.ff_name,our_wildcard);
            if(!match(our_wildcard,their_wildcard)) {
               for(i=strlen(file);i;i--)
                  if (file[i]=='\\') {
                     file[i+1]=0;
                     break;
                  }
               strcat(file,dta.ff_name);
               if(required_pwd[0]) {
                  strupr(required_pwd);
                  if((strcmp(required_pwd,their_pwd)) && (strcmp(required_pwd,remote_password)))
                     status_line(msgtxt[M_FREQ_PW_ERR],required_pwd,their_pwd,remote_password);
                  else
                     if(++j > *recno)
                        goto gotfile;
               }
               else
                  if(++j > *recno)
                     goto gotfile;
            }
         } while(!findnext(&dta));
      }
      else
         status_line(msgtxt[M_OKFILE_ERR],file);

      file[0] = '\0';
   }

avail:
   strcpy (request, s1);

   if(approved)
      fclose(approved);

   if(*recno > -1) {
      *recno = -1;
      return(NULL);
   }

   if (!file[0]) {
      *recno = -1;
      return (NULL);
   }

gotfile:
   strcpy(request,s1);

   ++(*recno);
   if (approved)
      fclose(approved);

   return(&file[0]);

err:
   strcpy(request,s1);

   if(approved)
      fclose(approved);

   *recno = -1;
   if(sptr)
      status_line("!%s Request Err: %s", updreq?"Update":"File", sptr);

   return(NULL);
}

int prep_match(template,dep)
char *template, *dep;
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

int match(s1,s2)
char *s1, *s2;
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

void packet_bundle(fname, f)
char *fname;
int f;
{
   FILE *fp, *fpt;
   int i;
   char s[256], filename[80], *HoldName;
   struct _pkthdr pkt;
   struct date datep;
   struct time timep;

   HoldName = HoldAreaNameMunge (called_zone);
   memset ((char *)&pkt, 0, sizeof (struct _pkthdr));

   pkt.orig_node = alias[assumed].node;
   pkt.dest_node = remote_node;
   pkt.orig_net = alias[assumed].net;
   pkt.dest_net = remote_net;
   pkt.product = isITALY;
   pkt.rate = rate;
   pkt.ver = PKTVER;
   gettime (&timep);
   getdate (&datep);
   pkt.hour = timep.ti_hour;
   pkt.minute = timep.ti_min;
   pkt.second = timep.ti_sec;
   pkt.year = datep.da_year;
   pkt.month = datep.da_mon;
   pkt.day = datep.da_day;

   sprintf(fname,"%s%04x%04x.OUT",HoldName,called_net,called_node);
   fp = fopen(fname,"wb");
   fwrite((char *)&pkt, sizeof(struct _pkthdr), 1, fp);

   if (f) {
      sprintf(filename,"TMP%d.TXT", line_offset);
      fpt = fopen(filename,"rb");

      do {
         i = fread(s, 1, 255, fpt);
         fwrite(s, 1, i, fp);
      } while (i == 255);

      if (fpt != NULL)
         fclose (fpt);
      unlink(filename);
   }

   s[0]=0;
   s[1]=0;
   fwrite(s, 2, 1, fp);
   fclose(fp);
}

struct _pkmsg {
   int ver;
   int orig_node;
   int dest_node;
   int orig_net;
   int dest_net;
   int attrib;
   int cost;
};

void mail_report(fname, descr, len, fsent)
char *fname, *descr;
long len;
int fsent;
{
   FILE *fp;
   char date[128], *rp = "Report on File-Request", filename[80];
   char *hd1 = "The following is the result of a file-request to %d:%d/%d ...\r\n\r\n";
   char *hd2 = "Filename        Bytes   Description\r\n--------        -----   -----------\r\n";
   char *hd3 = "              -------\r\n";
   static long totals = 0L;
   static char passed = 0;
   struct _pkmsg pmsg;

   if (fsent == 0 || !passed) {
      pmsg.ver = PKTVER;
      pmsg.orig_node = alias[assumed].node;
      pmsg.dest_node = remote_node;
      pmsg.orig_net = alias[assumed].net;
      pmsg.dest_net = remote_net;
      pmsg.attrib = MSGPRIVATE|MSGLOCAL;
      pmsg.cost = 0;
      totals = 0L;
   }

   sprintf(filename,"TMP%d.TXT", line_offset);
   fp = fopen(filename, (fsent == 0) ? "wb" : "ab");
   if (fp == NULL)
      return;

   if (fsent == 0 || !passed) {
      fwrite((char *)&pmsg, sizeof(struct _pkmsg), 1, fp);
      data(date);
      fwrite(date, strlen(date)+1, 1, fp);
      fwrite(remote_sysop, strlen(remote_sysop)+1, 1, fp);
      fwrite(VERSION, strlen(VERSION)+1, 1, fp);
      fwrite(rp, strlen(rp)+1, 1, fp);
      if (remote_point) {
         sprintf(date,"\x01TOPT %d\r\n", remote_point);
         fwrite(date, strlen(date), 1, fp);
      }
      sprintf(date, hd1, alias[assumed].zone, alias[assumed].net, alias[assumed].node);
      fwrite(date, strlen(date), 1, fp);
      fwrite(hd2, strlen(hd2), 1, fp);
   }
   else if (fsent == -1) {
      sprintf(date, "%sTotal        %8ld\r\n", hd3, totals);
      fwrite(date, strlen(date), 1, fp);
      sprintf(date, "\r\n%s, %d:%d/%d\r\n",sysop,alias[assumed].zone,alias[assumed].net,alias[assumed].node);
      fwrite(date, strlen(date), 1, fp);
      sprintf(date, "%s\r\n", system_name);
      fwrite(date, strlen(date), 1, fp);
   }

   if (fsent >= 0 || fsent == -2) {
      if (len)
         sprintf (date, "%-12s %8ld   %s\r\n", fname, len, descr);
      else
         sprintf (date, "%-12s            %s\r\n", fname, descr);
      fwrite(date, strlen(date), 1, fp);
      totals += len;
   }

   passed = 1;
   fclose (fp);
}

static void description(rqname, fsent)
char *rqname;
int fsent;
{
   FILE *fp;
   char drive[80], path[80], name[16], ext[6];
   struct ffblk blk;

   fnsplit (rqname, drive, path, name, ext);
   strcat (drive, path);
   strcat (drive, "FILES.BBS");
   strcat (name, ext);
   strupr (name);

   if (!path[0] || findfirst (rqname, &blk, 0) || fsent == -2) {
      mail_report (name, "* Not found *", 0L, -2);
      return;
   }

   if (fsent >= 0) {
      fp = fopen (drive, "rt");

      if (fp == NULL)
         mail_report (name, "* No description *", blk.ff_fsize, fsent);
      else {
         while(!feof(fp)) {
            path[0] = 0;
            if (fgets (path, 255, fp) == NULL)
               break;
            if (!strnicmp (path, name, strlen(name))) {
               path[strlen(path)-1] = 0;
               mail_report (name, &path[13], blk.ff_fsize, fsent);
               break;
            }
         }

         fclose (fp);
      }
   }
   else if (fsent == -1)
      mail_report (name, "* F/Req. limit exceeded *", 0L, -2);
}

void emsi_handshake (caller)
{
   int np, wh, stat, i;
   char req[80], oksend[MAX_EMSI_ADDR];

   if (caller) {
      if (!send_emsi_handshake (caller))
         return;
      if (!CARRIER)
         return;
      if (!get_emsi_handshake (caller))
         return;
      if (!CARRIER)
         return;
   }
   else {
      if (!get_emsi_handshake (caller))
         return;
      if (!CARRIER)
         return;
      if (!send_emsi_handshake (caller))
         return;
      if (!CARRIER)
         return;
   }

   got_arcmail = 0;
   made_request = 0;

   remote_zone = addrs[0].zone;
   remote_net = addrs[0].net;
   remote_node = addrs[0].node;
   remote_point = addrs[0].point;

   if (!caller) {
      called_zone = remote_zone;
      called_net = remote_net;
      called_node = remote_node;
   }

   if (caller && ((remote_zone != called_zone) || (remote_net != called_net) || (remote_node != called_node)))
      status_line (msgtxt[M_CALLED], called_zone,called_net,called_node,remote_zone,remote_net,remote_node);
   else {
      called_zone = remote_zone;
      called_net = remote_net;
      called_node = remote_node;
   }

   if (!CARRIER)
      return;

   wh = wopen(7,12,13,66,1,LCYAN|_BLUE,LCYAN|_BLUE);
   wactiv(wh);
   whline(3,0,53,2,LCYAN|_BLUE);

   wtitle(" EMSI Session ", TLEFT, LCYAN|_BLUE);
   sprintf(req, "System: %s", remote_system);
   wprints(0,1,LCYAN|_BLUE,req);
   sprintf(req, "Sysop:  %s", remote_sysop);
   wprints(1,1,LCYAN|_BLUE,req);
   sprintf(req, "Address: %u:%u/%u.%u", remote_zone, remote_net, remote_node, remote_point);
   wrjusts(1,51,LCYAN|_BLUE,req);
   wprints(2,1,LCYAN|_BLUE,"Method:");
   wprints(4,1,LCYAN|_BLUE,remote_program);

   if (remote_net == alias[assumed].net && remote_node == alias[assumed].node) {
      called_net = alias[assumed].fakenet;
      called_node = remote_point;
   }
   else if (remote_point)
      called_net = -1;

   if (remote_capabilities & ZED_ZAPPER) {
      status_line(":EMSI method: %s", "ZedZap");
      wprints(2,9,LCYAN|_BLUE,"ZedZap");
   }
   else
      goto endwazoo;

   for (np = 0; np < 10; np++) {
      if (alias[np].net == 0)
         break;

      sprintf(req,request_template,filepath,alias[np].net,alias[np].node);
      unlink(req);
   }

   for (np = 0; np < MAX_EMSI_ADDR; np++) {
      if (addrs[np].net == 0)
         break;
      flag_file (SET_SESSION_FLAG, addrs[np].zone, addrs[np].net, addrs[np].node, 0);
      if (flag_file (TEST_AND_SET, addrs[np].zone, addrs[np].net, addrs[np].node, 1))
         oksend[np] = 0;
      else
         oksend[np] = 1;
   }

   emsi = 1;
   emsi_sent = 0;

   if(caller) {
      for (np = 0; np < MAX_EMSI_ADDR; np++) {
         if (addrs[np].net == 0)
            break;
         if (!oksend[np])
            continue;

         called_zone = remote_zone = addrs[np].zone;
         if (!called_zone)
            called_zone = remote_zone = alias[0].zone;
         called_net = remote_net = addrs[np].net;
         called_node = remote_node = addrs[np].node;
         remote_point = addrs[np].point;

         for (i = 0; i < MAX_ALIAS; i++) {
            if (alias[i].net == 0)
               break;
            if (remote_zone == alias[i].zone && remote_net == alias[i].net && remote_node == alias[i].node && remote_point && alias[i].fakenet)
               break;
         }
         if (remote_zone == alias[i].zone && remote_net == alias[i].net && remote_node == alias[i].node && remote_point && alias[i].fakenet) {
            called_net = alias[assumed].fakenet;
            called_node = remote_point;
         }
         else if (remote_point)
            called_net = -1;

         if (get_bbs_record (called_zone, called_net, called_node, 0)) {
            if (nodelist.password[0] && strcmp(strupr(nodelist.password),strupr(remote_password))) {
               status_line(msgtxt[M_PWD_ERROR],remote_net,remote_node,nodelist.password,remote_password);
               continue;
            }
         }

         send_WaZOO(caller);
      }
      send_Zmodem(NULL,NULL,((emsi_sent)?END_BATCH:NOTHING_TO_DO),DO_WAZOO);
      if (!CARRIER)
         goto endwazoo;
      get_Zmodem(filepath, NULL);
      if (!CARRIER)
         goto endwazoo;
      if (made_request) {
         stat = respond_to_file_requests(fsent);
         send_Zmodem(NULL,NULL,((stat)?END_BATCH:NOTHING_TO_DO),DO_WAZOO);
      }
   }
   else {
      get_Zmodem(filepath, NULL);
      if (!CARRIER)
         goto endwazoo;
      for (np = 0; np < MAX_EMSI_ADDR; np++) {
         if (addrs[np].net == 0)
            break;
         if (!oksend[np])
            continue;

         called_zone = remote_zone = addrs[np].zone;
         if (!called_zone)
            called_zone = remote_zone = alias[0].zone;
         called_net = remote_net = addrs[np].net;
         called_node = remote_node = addrs[np].node;
         remote_point = addrs[np].point;

         for (i = 0; i < MAX_ALIAS; i++) {
            if (alias[i].net == 0)
               break;
            if (remote_zone == alias[i].zone && remote_net == alias[i].net && remote_node == alias[i].node && remote_point && alias[i].fakenet)
               break;
         }
         if (remote_zone == alias[i].zone && remote_net == alias[i].net && remote_node == alias[i].node && remote_point && alias[i].fakenet) {
            called_net = alias[i].fakenet;
            called_node = remote_point;
         }
         else if (remote_point)
            called_net = -1;

         if (get_bbs_record (called_zone, called_net, called_node, 0)) {
            if (nodelist.password[0] && strcmp(strupr(nodelist.password),strupr(remote_password))) {
               status_line(msgtxt[M_PWD_ERROR],remote_net,remote_node,nodelist.password,remote_password);
               continue;
            }
         }
/*
         else if (remote_password[0]) {
            status_line (msgtxt[M_NOTHING_TO_SEND], remote_zone, remote_net, remote_node, remote_point);
            continue;
         }
*/

         send_WaZOO(caller);
      }
      emsi_sent = respond_to_file_requests(fsent);
      send_Zmodem(NULL,NULL,((emsi_sent)?END_BATCH:NOTHING_TO_DO),DO_WAZOO);
      if (!CARRIER || !made_request)
         goto endwazoo;
      get_Zmodem(filepath, NULL);
   }

endwazoo:
   memset ((char *)&usr, 0, sizeof (struct _usr));
   caller = 0;

   if (!CARRIER)
      status_line(msgtxt[M_NO_CARRIER]);

   if (!(remote_capabilities & DOES_IANUS))
      for (np = 0; np < MAX_EMSI_ADDR; np++) {
         if (addrs[np].net == 0)
            break;
         flag_file (CLEAR_SESSION_FLAG, addrs[np].zone, addrs[np].net, addrs[np].node, 0);
         if (oksend[np])
            flag_file (CLEAR_FLAG, addrs[np].zone, addrs[np].net, addrs[np].node, 1);
      }

   wactiv(wh);
   wclose();
   wunlink(wh);

   status_line ("*End of EMSI Session");
   terminating_call();

   if (got_arcmail) {
      if (cur_event > -1 && e_ptrs[cur_event]->errlevel[2])
         aftermail_exit = e_ptrs[cur_event]->errlevel[2];

      if (!aftermail_exit)
         aftermail_exit = aftercaller_exit;

      if (cur_event > -1 && (e_ptrs[cur_event]->echomail & ECHO_IMPORT)) {
         if (modem_busy != NULL)
            mdm_sendcmd (modem_busy);

         i = 1;
         if ( (e_ptrs[cur_event]->echomail & (ECHO_PROT|ECHO_KNOW|ECHO_NORMAL)) ) {
            if (prot_filepath != NULL && (e_ptrs[cur_event]->echomail & ECHO_PROT))
               import_mail (prot_filepath, &i);
            if (know_filepath != NULL && (e_ptrs[cur_event]->echomail & ECHO_KNOW))
               import_mail (know_filepath, &i);
            if (norm_filepath != NULL && (e_ptrs[cur_event]->echomail & ECHO_NORMAL))
               import_mail (norm_filepath, &i);
         }
         else {
            if (prot_filepath != NULL)
               import_mail (prot_filepath, &i);
            if (know_filepath != NULL)
               import_mail (know_filepath, &i);
            if (norm_filepath != NULL)
               import_mail (norm_filepath, &i);
         }                               

         i = 2;
         import_mail (".\\", &i);
         if (e_ptrs[cur_event]->echomail & ECHO_EXPORT)
            export_mail (NETMAIL_RSN|ECHOMAIL_RSN);
      }
      else {
         status_line(msgtxt[M_EXIT_AFTER_MAIL],aftermail_exit);
         get_down (aftermail_exit, 3);
      }
   }

   get_down(aftercaller_exit, 2);
}


