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
#include <cxl\cxlvid.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"

#define NUM_FLAGS      5
#define isITALY        0x4E
#define MAX_EMSI_ADDR  30

void reset_mailon (void);
void set_mailon (int, int, int, int, char *);
void request_report (char *, int);
long get_phone_cost (int, int, int, long);
void update_filestat (char *rqname);
void import_sequence (void);

static void mail_history (int, int, int, int, int, int, long, long, long);

extern char remote_system[60], remote_sysop[36], remote_program[60], remote_location[40];
extern char nomailproc;
extern struct _alias addrs[MAX_EMSI_ADDR + 1];
extern long elapsed, timeout;

int emsi, n_mail = 0, n_data = 0, got_maildata = 0, freceived;
long b_mail = 0, b_data = 0, tot_rcv = 0, tot_sent = 0;

static int emsi_sent;

void WaZOO(originator)
int originator;
{
   int stat, np, wh, i, v, ntt_mail = 0, ntt_data = 0;
   char req[256];
   long t, btt_mail = 0, btt_data = 0, olc;

   timeout = 0L;
   tot_rcv = tot_sent = 0;
   stat = 0;
   got_arcmail = 0;
   made_request = 0;
   emsi = 0;
   freceived = 0;
   fsent = 0;

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

   filetransfer_system ();
   update_filesio (fsent, freceived);

   wh = wopen (12, 0, 24, 79, 0, LGREY|_BLACK, WHITE|_BLACK);
   wactiv (wh);
   if (originator)
      wtitle ("OUTBOUND CALL STATUS", TLEFT, LCYAN|_BLACK);
   else
      wtitle ("INBOUND CALL STATUS", TLEFT, LCYAN|_BLACK);
   printc (12, 0, LGREY|_BLACK, 'ֳ');
   printc (12, 52, LGREY|_BLACK, 'ֱ');
   printc (12, 79, LGREY|_BLACK, '´');
   whline (8, 0, 80, 0, LGREY|_BLACK);

   set_mailon (remote_zone, remote_net, remote_node, remote_point, remote_location);

   sprintf (req, "%u:%u/%u.%u, %s, %s, %s", remote_zone, remote_net, remote_node, remote_point, remote_sysop, remote_system, remote_location);
   if (strlen (req) > 78)
      req[78] = '\0';
   wcenters (0, LGREY|_BLACK, req);
   sprintf (req, "Connected at %lu baud with %s", rate, remote_program);
   wcenters (1, LGREY|_BLACK, req);
   wcenters (2, LGREY|_BLACK, "AKAs: No aka presented");

   wprints (5, 2, LCYAN|_BLACK, "Files");
   wprints (6, 2, LCYAN|_BLACK, "Bytes");

   wprints (4, 9, LCYAN|_BLACK, " ÚִִִִMailPKTִִִִִִִDataִִִִִ¿");
   wprints (5, 9, LCYAN|_BLACK, "תתתתתתתתתתתתתתתתתתתתתתתתתתתתתתת");
   wprints (6, 9, LCYAN|_BLACK, "תתתתתתתתתתתתתתתתתתתתתתתתתתתתתתת");
   wprints (7, 9, LCYAN|_BLACK, " ְִִִִִִINBOUND TRAFFICִִִִִִÙ");

   if (got_maildata) {
      sprintf (req, "%d", n_mail);
      wrjusts (5, 20, YELLOW|_BLACK, req);
      sprintf (req, "%ld", b_mail);
      wrjusts (6, 20, YELLOW|_BLACK, req);
      sprintf (req, "%d", n_data);
      wrjusts (5, 31, YELLOW|_BLACK, req);
      sprintf (req, "%ld", b_data);
      wrjusts (6, 31, YELLOW|_BLACK, req);
   }
   else {
      wrjusts (5, 20, YELLOW|_BLACK, "N/A");
      wrjusts (6, 20, YELLOW|_BLACK, "N/A");
      wrjusts (5, 31, YELLOW|_BLACK, "N/A");
      wrjusts (6, 31, YELLOW|_BLACK, "N/A");
   }

   wprints (4, 44, LCYAN|_BLACK, " ÚִִִִMailPKTִִִִִִִDataִִִִִ¿");
   wprints (5, 44, LCYAN|_BLACK, "תתתתתתתתתתתתתתתתתתתתתתתתתתתתתתת");
   wprints (6, 44, LCYAN|_BLACK, "תתתתתתתתתתתתתתתתתתתתתתתתתתתתתתת");
   wprints (7, 44, LCYAN|_BLACK, " ְִִִִִOUTBOUND TRAFFICִִִִִִÙ");

   if (originator) {
      sprintf (req, "%d", call_list[next_call].n_mail);
      wrjusts (5, 55, YELLOW|_BLACK, req);
      sprintf (req, "%ld", call_list[next_call].b_mail);
      wrjusts (6, 55, YELLOW|_BLACK, req);
      sprintf (req, "%d", call_list[next_call].n_data);
      wrjusts (5, 66, YELLOW|_BLACK, req);
      sprintf (req, "%ld", call_list[next_call].b_data);
      wrjusts (6, 66, YELLOW|_BLACK, req);
   }
   else {
      for (i = 0; i < max_call; i++) {
         if (remote_zone == call_list[i].zone && remote_net == call_list[i].net && remote_node == call_list[i].node && remote_point == call_list[i].point) {
            ntt_mail += call_list[i].n_mail;
            btt_mail += call_list[i].b_mail;
            ntt_data += call_list[i].n_data;
            btt_data += call_list[i].b_data;
            break;
         }
      }

      if (remote_point) {
         for (v = 0; v < MAX_ALIAS && config->alias[v].net; v++) {
            if (remote_zone == config->alias[v].zone && remote_net == config->alias[v].net && remote_node == config->alias[v].node && config->alias[v].fakenet)
               break;
         }

         if (v < MAX_ALIAS && config->alias[v].net) {
            for (i = 0; i < max_call; i++) {
               if (remote_zone == call_list[i].zone && config->alias[v].fakenet == call_list[i].net && remote_point == call_list[i].node && !call_list[i].point) {
                  ntt_mail += call_list[i].n_mail;
                  btt_mail += call_list[i].b_mail;
                  ntt_data += call_list[i].n_data;
                  btt_data += call_list[i].b_data;
                  break;
               }
            }
         }
      }

      sprintf (req, "%d", ntt_mail);
      wrjusts (5, 55, YELLOW|_BLACK, req);
      sprintf (req, "%ld", btt_mail);
      wrjusts (6, 55, YELLOW|_BLACK, req);
      sprintf (req, "%d", ntt_data);
      wrjusts (5, 66, YELLOW|_BLACK, req);
      sprintf (req, "%ld", btt_data);
      wrjusts (6, 66, YELLOW|_BLACK, req);
   }

   if (remote_net == config->alias[assumed].net && remote_node == config->alias[assumed].node && remote_point) {
      called_net = config->alias[assumed].fakenet;
      called_node = remote_point;
   }
   else if (remote_point)
      called_net = -1;

   if ((remote_capabilities & DOES_IANUS) && (mdm_flags == NULL || strstr (mdm_flags, "Hst") == NULL) && config->janus) {
      prints (7, 65, YELLOW|_BLACK, "WaZOO/Janus");
      status_line (msgtxt[M_WAZOO_METHOD], "Janus");
   }
   else if (remote_capabilities & ZED_ZAPPER) {
      prints (7, 65, YELLOW|_BLACK, "WaZOO/ZedZap");
      status_line(msgtxt[M_WAZOO_METHOD], "ZedZap");
   }
   else if (remote_capabilities & ZED_ZIPPER) {
      prints (7, 65, YELLOW|_BLACK, "WaZOO/ZedZip");
      status_line(msgtxt[M_WAZOO_METHOD], "ZedZip");
   }
   else {
      prints (7, 65, YELLOW|_BLACK, "WaZOO/DietIfna");
      status_line(msgtxt[M_WAZOO_METHOD], "DietIfna");
   }

   if (flag_file (TEST_AND_SET, called_zone, called_net, called_node, 0, 1))
      goto endwazoo;

   if ((remote_capabilities & DOES_IANUS) && (mdm_flags == NULL || strstr (mdm_flags, "Hst") == NULL) && config->janus) {
      isOriginator = originator;
      Janus ();
      goto endwazoo;
   }

   if (remote_capabilities & ZED_ZAPPER)
      remote_capabilities &= ~ZED_ZIPPER;
   else if (remote_capabilities & ZED_ZIPPER)
      remote_capabilities &= ~ZED_ZAPPER;
   else {
      if (originator)
         FTSC_sender(1);
      else
         FTSC_receiver(1);
      goto endwazoo;
   }

   for (np = 0; np < 10; np++) {
      if (config->alias[np].net == 0)
         break;

      sprintf(req,request_template,filepath,config->alias[np].net,config->alias[np].node);
      unlink(req);
   }

   if (originator) {
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
      made_request = 0;
      if (!CARRIER)
         goto endwazoo;
      send_WaZOO(originator);
      if (!CARRIER || !made_request)
         goto endwazoo;
      get_Zmodem(filepath, NULL);
   }

endwazoo:
   local_mode = caller = 0;

   if (!CARRIER)
      status_line(msgtxt[M_NO_CARRIER]);

   flag_file (CLEAR_FLAG,called_zone,called_net,called_node,0,1);

   wactiv(wh);
   wclose();

   status_line (msgtxt[M_WAZOO_END]);

   t = time (NULL) - elapsed + 20L;
   if (originator) {
      olc = get_phone_cost (remote_zone, remote_net, remote_node, t);
      status_line ("*Session with %d:%d/%d.%d Time: %ld:%02ld, Cost: $%ld.%02ld", remote_zone, remote_net, remote_node, remote_point, t / 60L, t % 60L, olc / 100L, olc % 100L);
   }
   else
      status_line ("*Session with %d:%d/%d.%d Time: %ld:%02ld", remote_zone, remote_net, remote_node, remote_point, t / 60L, t % 60L);

   if (cur_event > -1 && (e_ptrs[cur_event]->behavior & MAT_DYNAM)) {
      e_ptrs[cur_event]->behavior |= MAT_SKIP;
      write_sched ();
   }

   mail_history (originator, remote_zone, remote_net, remote_node, remote_point, (int)t, tot_sent, tot_rcv, olc);
   reset_mailon ();
   memset ((char *)&usr, 0, sizeof (struct _usr));

   local_status ("Hangup");
   modem_hangup ();

   if (originator) {
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
   }
   else {
      sysinfo.today.incalls++;
      sysinfo.week.incalls++;
      sysinfo.month.incalls++;
      sysinfo.year.incalls++;
      sysinfo.today.inconnects += t;
      sysinfo.week.inconnects += t;
      sysinfo.month.inconnects += t;
      sysinfo.year.inconnects += t;
   }

   if (!nomailproc && got_arcmail) {
      if (cur_event > -1 && e_ptrs[cur_event]->errlevel[2])
         aftermail_exit = e_ptrs[cur_event]->errlevel[2];

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

      if (aftermail_exit) {
         status_line(msgtxt[M_EXIT_AFTER_MAIL],aftermail_exit);
         get_down (aftermail_exit, 3);
      }
   }

   get_down (0, 2);
}

/*--------------------------------------------------------------------------*/
/* SEND WaZOO (send another WaZOO-capable Opus its mail)                    */
/*   returns TRUE (1) for good xfer, FALSE (0) for bad                      */
/*   use instead of n_bundle and n_attach for WaZOO Opera                   */
/*--------------------------------------------------------------------------*/
int send_WaZOO (int caller)
{
   FILE *fp;
   char fname[80], s[80], *sptr, *password, *HoldName;
   int c, i, rzcond, nounlink;
   struct stat buf;
   long current, last_start;

   if (emsi)
      fsent = emsi_sent;
   else
      fsent = 0;

   HoldName = HoldAreaNameMunge (called_zone);

   if (!remote_point || called_zone != remote_zone || called_net != remote_net || called_node != remote_node) {
      *ext_flags  = 'O';
      for (c = 0; c < NUM_FLAGS; c++) {
         if (caller && (ext_flags[c] == 'H'))
            continue;

         sprintf(fname,"%s%04x%04x.%cUT",HoldName,called_net,called_node,ext_flags[c]);

         if (!stat(fname,&buf)) {
            invent_pkt_name(s);
            if (!send_Zmodem(fname,s,fsent++,DO_WAZOO))
               goto bad_send;
            update_filesio (fsent, freceived);
            unlink(fname);
         }
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
            update_filesio (fsent, freceived);
            unlink(fname);
         }
      }
   }

   if (!remote_point || called_zone != remote_zone || called_net != remote_net || called_node != remote_node) {
      nounlink = 0;
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

                  if ((rzcond=send_Zmodem(sptr,NULL,fsent++,DO_WAZOO)) == 0) {
                     fclose(fp);
                     goto bad_send;
                  }

                  update_filesio (fsent, freceived);
                  if (rzcond != SPEC_COND) {
                     if (i == DELETE_AFTER)
                        unlink(sptr);
                     else if (i == TRUNC_AFTER) {
                        unlink(sptr);
                        i = creat(sptr,S_IREAD|S_IWRITE);
                        close(i);
                     }
                  }
                  else
                     nounlink = 1;

                  fseek(fp,last_start,SEEK_SET);
                  putc('~',fp);
                  fflush(fp);
                  rewind(fp);
                  fseek(fp,current,SEEK_SET);
               }
            }
            fclose(fp);
            if (!nounlink || ext_flags[c] != 'H')
               unlink(fname);
         }
      }
   }

   if (remote_point) {
      nounlink = 0;
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

                  if ((rzcond=send_Zmodem (sptr, NULL, fsent++, DO_WAZOO)) == 0) {
                     fclose (fp);
                     goto bad_send;
                  }

                  update_filesio (fsent, freceived);
                  if (rzcond != SPEC_COND) {
                     if (i == DELETE_AFTER)
                        unlink(sptr);
                     else if (i == TRUNC_AFTER) {
                        unlink (sptr);
                        i = creat(sptr,S_IREAD|S_IWRITE);
                        close (i);
                     }
                  }
                  else
                     nounlink = 1;

                  fseek(fp,last_start,SEEK_SET);
                  putc('~',fp);
                  fflush(fp);
                  rewind(fp);
                  fseek(fp,current,SEEK_SET);
               }
            }
            fclose(fp);
            if (!nounlink)
               unlink(fname);
         }
      }
   }

   sprintf (fname, request_template, HoldName, called_net, called_node);

   if (!stat (fname, &buf)) {
      if (!(remote_capabilities & WZ_FREQ) || (cur_event > -1 && (e_ptrs[cur_event]->behavior & MAT_NOOUTREQ)) )
         status_line (msgtxt[M_FREQ_DECLINED]);
      else {
         status_line (msgtxt[M_MAKING_FREQ]);
         if (send_Zmodem (fname, NULL, fsent++, DO_WAZOO)) {
            unlink (fname);
            update_filesio (fsent, freceived);
            made_request = 1;
         }
      }
   }

   if (!emsi && !caller)
      fsent = respond_to_file_requests (fsent);

   if (!fsent)
      status_line (msgtxt[M_NOTHING_TO_SEND], remote_zone, remote_net, remote_node, remote_point);

done_send:
   if (!emsi)
      send_Zmodem(NULL,NULL,((fsent)?END_BATCH:NOTHING_TO_DO),DO_WAZOO);
   else
      emsi_sent = fsent;
   return(TRUE);

bad_send:
   if (emsi)
      emsi_sent = fsent;
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
   int np, nfiles, junk, notf, i;
   char *rqname;

   if (cur_event > -1 && (e_ptrs[cur_event]->behavior & MAT_NOREQ))
      return (fsent);

   nfiles = 0;

   for(np = 0; np < MAX_ALIAS; np++) {
      if (config->alias[np].net == 0)
              break;

      sprintf(req,request_template,filepath,config->alias[np].net,config->alias[np].node);

      if(!stat(req,&buf)) {
         fp = fopen(req,"rt");
         if(fp == NULL)
            goto done;

         if (!nfiles)
            request_report (NULL, 1);

         while(!feof(fp)) {
            req[0] = 0;
            if(fgets(req,79,fp) == NULL)
               break;

            if(req[0] == ';')
               continue;

            junk = -1;
            notf = 1;

            do {
               if (nfiles > max_requests && max_requests) {
                  status_line (msgtxt[M_FREQ_LIMIT]);
                  request_report (req, 4);
                  break;
               }

               if ((rqname = n_frproc (req, &junk, 0)) != NULL) {
                  if (stat (rqname, &buf2) == -1) {
                     request_report (req, 2);
                     continue;
                  }

                  if ((long)max_kbytes  > (buf2.st_size / 1024L) || !max_kbytes) {
                     if (max_kbytes)
                        max_kbytes -= (int)(buf2.st_size / 1024L);

                     if ((remote_capabilities & DOES_IANUS) && (mdm_flags == NULL || strstr (mdm_flags, "Hst") == NULL) && config->janus) {
                        if (record_reqfile(rqname) == FALSE)
                           goto janus_done;

                        if (config->keep_dl_count)
                           update_filestat (rqname);
                        request_report (rqname, 5);
                        ++fsent;
                        sysinfo.today.filerequest++;
                        sysinfo.week.filerequest++;
                        sysinfo.month.filerequest++;
                        sysinfo.year.filerequest++;
                        notf = 0;
                     }
                     else {
                        if((i = send_Zmodem (rqname, NULL, ++fsent, DO_WAZOO)) == 0)
                           goto janus_done;
                        if (i == SPEC_COND)
                           request_report (rqname, 7);
                        else {
                           if (config->keep_dl_count)
                              update_filestat (rqname);
                           request_report (rqname, 5);
                        }
                        sysinfo.today.filerequest++;
                        sysinfo.week.filerequest++;
                        sysinfo.month.filerequest++;
                        sysinfo.year.filerequest++;
                        notf = 0;
                        update_filesio (fsent, freceived);
                     }

                     ++nfiles;
                     fsent++;
                  }
                  else {
                     status_line (msgtxt[M_BYTE_LIMIT]);
                     request_report (rqname, 3);
                     notf = 0;
                     break;
                  }
               }

               if (notf)
                  request_report (req, 2);
            } while ((junk >= 0));

            if (nfiles > max_requests && max_requests)
               break;
         }

         fclose (fp);
         fp = NULL;

         sprintf(req,request_template,filepath,config->alias[np].net,config->alias[np].node);
         unlink(req);
      }

      if (nfiles > max_requests && max_requests)
         break;
   }

done:
   sprintf (req, "REP%d.TXT", line_offset);
   if (dexists (req)) {
      request_report (NULL, 6);
      if (!(remote_capabilities & DOES_IANUS) || (mdm_flags != NULL && strstr (mdm_flags, "Hst")) || !config->janus) {
         status_line (msgtxt[M_ILLEGAL_CARRIER]);
         invent_pkt_name (s);
         send_Zmodem (req, s, fsent++, DO_WAZOO);
         fsent++;
         update_filesio (fsent, freceived);
         unlink(req);
      }
      else {
         unlink(req);
//         invent_pkt_name (s);
//         rename (req, s);
//         record_reqfile (s);
      }
   }

janus_done:
   sprintf(req,request_template,filepath,config->alias[np].net,config->alias[np].node);
   unlink(req);

   if(fp != NULL)
      fclose(fp);

   return (fsent);
}

void invent_pkt_name(string)
char *string;
{
   static unsigned long last_pkt = 0L;
   unsigned long pkt;
   struct dostime_t time;
   struct dosdate_t date;

   do {
      _dos_gettime (&time);
      _dos_getdate (&date);

      pkt = time.hsecond;
      pkt += (long)time.second * (long)100;
      pkt += (long)time.minute * (long)6000;
      pkt += (long)time.hour * (long)360000;
      pkt += (long)date.day * (long)24 * (long)360000;
   } while (pkt == last_pkt);

   sprintf (string, "%08lX.PKT", pkt);
   last_pkt = pkt;
}

typedef struct {
   char name[13];
   word area;
   long datpos;
} FILEIDX;

char *n_frproc (char *request, int *recno, int updreq)
{
   static char file[80];
   FILE *approved, *fp;
   int i, j;
   char s1[80], *sptr, *their_pwd, required_pwd[32], *after_pwd;
   char our_wildcard[15], their_wildcard[15], magic[15];
   struct ffblk dta;
   FILEIDX fidx;

   approved = NULL;
   their_pwd = NULL;
   after_pwd = NULL;
   strcpy (s1, request);

   for (i = 0; request[i]; i++) {
      if ((request[i] == ' ') || (request[i] == 0x09)) {
         request[i] = '\0';
         if (request[i+1]=='!') {
            their_pwd = request+i+2;
            if(strlen(their_pwd)>31)
               their_pwd[31] = '\0';
               strupr(their_pwd);
         }
      }
      else
         if (request[i] <= ' ')
            request[i] = '\0';
   }

   if (!request[0])
      return (NULL);

   if (their_pwd)
      for (i = 0; their_pwd[i]; i++)
         if (their_pwd[i] <= ' ')
            their_pwd[i] = '\0';

   i = 0;
   sptr = NULL;

   strupr (request);
   if (*recno == -1)
      status_line ("*%s request (%s)", updreq ? "Update" : "File", request);

   if (*recno < 0) {
      if (!strcmp (request, "FILES")) {
         if (config->files[0])
            strcpy (file, config->files);
         else {
            file[0] = '\0';
            sptr = msgtxt[M_NO_AVAIL];
         }
         goto avail;
      }
      else if (!strcmp (request, "ABOUT")) {
         if (config->about[0])
            strcpy (file, config->about);
         else {
            file[0] = '\0';
            sptr = msgtxt[M_NO_ABOUT];
         }
         goto avail;
      }
   }

   prep_match (request, their_wildcard);

   if ((approved = fopen (request_list, "rt")) == NULL)
      goto err;

   j = -1;
   magic[0] = '\0';

   while (!feof (approved)) {
      if (magic[0]) {
         strcpy (their_wildcard, magic);
         magic[0] = '\0';
      }

      file[0] = '\0';
      required_pwd[0] = '\0';

      if (fgets (file, 78, approved) == NULL)
         break;

      for (i = 0; file[i]; i++) {
         if (file[i] == 0x09)
            file[i] = ' ';
         else
            if (file[i] < ' ')
               file[i] = '\0';
      }

      if (!file[0])
         continue;

      if (file[0] == ';')
         continue;

      for (i = 0; file[i]; i++) {
         if (file[i] == ' ') {
            file[i] = '\0';
            if (file[i + 1] == '!') {
               strncpy (required_pwd, &file[i + 2], 31);
               if (strlen (required_pwd) > 31)
                  required_pwd[31] = '\0';

               after_pwd = &file[i+1];
               while (*after_pwd && (!isspace (*after_pwd)))
                  ++after_pwd;

               if (*after_pwd)
                  ++after_pwd;

               for (i = 0; required_pwd[i]; i++)
                  if (required_pwd[i] <= ' ')
                     required_pwd[i] = '\0';
                  break;
            }
            else
               after_pwd = &file[i + 1];
         }
         else
            if (file[i] < ' ')
               file[i] = '\0';
      }

      if (!file[0])
         continue;

      if (file[0] == '@') {
         if (stricmp (&file[1], request) == 0) {
            strcpy (magic, their_wildcard);
            strcpy (file, after_pwd);
            strcpy (their_wildcard, "????????.???");
         }
         else
            continue;
      }
      else if (file[0] == '$') {
         if ((fp = sh_fopen (&file[1], "rb", SH_DENYNONE)) != NULL) {
            while (fread (&fidx, sizeof (FILEIDX), 1, fp) == 1) {
               prep_match (fidx.name, our_wildcard);
               if (!match (our_wildcard, their_wildcard)) {
                  if (read_system (fidx.area, 2)) {
                     if (!sys.prot_req && filepath == config->prot_filepath)
                        continue;
                     if (!sys.know_req && filepath == config->know_filepath)
                        continue;
                     if (!sys.norm_req && filepath == config->norm_filepath)
                        continue;

                     sprintf (file, "%s%s", sys.filepath, fidx.name);
                     if (!dexists (file))
                        continue;

                     if (required_pwd[0]) {
                        strupr (required_pwd);
                        if ((strcmp (required_pwd, their_pwd)) && (strcmp (required_pwd, remote_password)))
                           status_line (msgtxt[M_FREQ_PW_ERR], required_pwd, their_pwd, remote_password);
                        else {
                           if (++j > *recno)
                              goto gotfile;
                        }
                     }
                     else if (++j > *recno)
                        goto gotfile;
                  }
               }
            }
            fclose (fp);
         }
         file[0] = '\0';
      }

      if (file[0] && !findfirst (file, &dta, 0)) {
         do {
            prep_match (dta.ff_name, our_wildcard);
            if (!match (our_wildcard, their_wildcard)) {
               for (i = strlen (file); i; i--)
                  if (file[i] == '\\') {
                     file[i + 1] = 0;
                     break;
                  }
               strcat (file, dta.ff_name);
               if (required_pwd[0]) {
                  strupr (required_pwd);
                  if ((strcmp (required_pwd, their_pwd)) && (strcmp (required_pwd, remote_password)))
                     status_line (msgtxt[M_FREQ_PW_ERR], required_pwd, their_pwd, remote_password);
                  else {
                     if (++j > *recno)
                        goto gotfile;
                  }
               }
               else if (++j > *recno)
                  goto gotfile;
            }
         } while (!findnext (&dta));
      }
      else if (file[0])
         status_line (msgtxt[M_OKFILE_ERR], file);

      file[0] = '\0';
   }

   file[0] = '\0';

avail:
   strcpy (request, s1);

   if (approved) {
      fclose (approved);
      approved = NULL;
   }

   if (*recno > -1) {
      *recno = -1;
      return (NULL);
   }

   if (!file[0]) {
      *recno = -1;
      return (NULL);
   }

gotfile:
   strcpy (request, s1);

   ++(*recno);
   if (approved)
      fclose (approved);

   return (&file[0]);

err:
   strcpy (request, s1);

   if (approved)
      fclose (approved);

   *recno = -1;
   if (sptr)
      status_line ("!%s Request Err: %s", updreq ? "Update": "File", sptr);

   return (NULL);
}

/*

   void request_report (char *rqname, int fsent);

   Genera il report del file request. ID identifica l'operazione da compiere
   e puo' assumere i seguenti valori:

   1  - Inizializzazione
   2  - File non trovato
   3  - Rifiutato perche' troppo grande
   4  - Rifiutato perche' gia' prelevati troppi files
   5  - File mandato
   6  - Costruzione pacchetto mail da spedire
   7  - File rifiutato

*/
void request_report (rqname, id)
char *rqname;
int id;
{
   FILE *fp, *fp2;
   int fd;
   char string[128], *rp = "Report on File-Request";
   char *hd1 = "The following is the result of a file-request to %d:%d/%d ...\r\n\r\n";
   char *hd2 = "Filename        Bytes   Description\r\n--------        -----   -----------\r\n";
   char *hd3 = "              -------\r\n";
   char drive[80], path[80], name[16], ext[6];
   struct ffblk blk;
   struct _msghdr2 mhdr;
   struct _pkthdr2 pkthdr;
   struct date datep;
   struct time timep;
   NODEINFO ni;
   static long totals = 0L;
   static int  nfiles = 0;

   name[0] = ext[0] = drive[0] = path[0] = '\0';
   if (rqname) {
      strupr (rqname);
      fnsplit (rqname, drive, path, name, ext);
      strcat (drive, path);
      strcat (name, ext);
      strupr (name);
   }

   sprintf (string,"REP%d.TXT", line_offset);
   fp = fopen (string, (id == 1) ? "wb" : "ab");
   if (fp == NULL)
      return;

   if (id == 1) {
      gettime (&timep);
      getdate (&datep);

      memset ((char *)&pkthdr, 0, sizeof (struct _pkthdr2));
      pkthdr.ver = PKTVER;
      pkthdr.product = 0x4E;
      pkthdr.serial = 2 * 16 + 11;
      pkthdr.capability = 1;
      pkthdr.cwvalidation = 256;
      pkthdr.orig_node = config->alias[assumed].node;
      pkthdr.orig_net = config->alias[assumed].net;
      pkthdr.orig_zone = config->alias[assumed].zone;
      pkthdr.orig_zone2 = config->alias[assumed].zone;
      pkthdr.orig_point = 0;
      if (emsi) {
         pkthdr.dest_node = addrs[0].node;
         pkthdr.dest_net = addrs[0].net;
         pkthdr.dest_zone = addrs[0].zone;
         pkthdr.dest_zone2 = addrs[0].zone;
         pkthdr.dest_point = addrs[0].point;
      }
      else {
         pkthdr.dest_node = remote_node;
         pkthdr.dest_net = remote_net;
         pkthdr.dest_zone = remote_zone;
         pkthdr.dest_zone2 = remote_zone;
         pkthdr.dest_point = remote_point;
      }

      sprintf (string, "%sNODES.DAT", config->net_info);
      if ((fd = open (string, O_RDONLY|O_BINARY)) != -1) {
         while (read (fd, (char *)&ni, sizeof (NODEINFO)) == sizeof (NODEINFO)) {
            if (ni.zone == pkthdr.dest_zone && ni.node == pkthdr.dest_node && ni.net == pkthdr.dest_net && ni.point == pkthdr.dest_point)
               break;
         }
         close (fd);
      }
      if (ni.zone == pkthdr.dest_zone && ni.node == pkthdr.dest_node && ni.net == pkthdr.dest_net && ni.point == pkthdr.dest_point) {
         strcpy (pkthdr.password, ni.pw_packet);
         pkthdr.orig_zone = config->alias[assumed].zone;
      }

      pkthdr.hour = timep.ti_hour;
      pkthdr.minute = timep.ti_min;
      pkthdr.second = timep.ti_sec;
      pkthdr.year = datep.da_year;
      pkthdr.month = datep.da_mon - 1;
      pkthdr.day = datep.da_day;
      fwrite ((char *)&pkthdr, sizeof (struct _pkthdr2), 1, fp);

      mhdr.ver = PKTVER;
      mhdr.orig_node = config->alias[assumed].node;
      mhdr.orig_net = config->alias[assumed].net;
      mhdr.dest_node = remote_node;
      mhdr.dest_net = remote_net;
      mhdr.cost = 0;
      mhdr.attrib = 0;
      fwrite ((char *)&mhdr, sizeof (struct _msghdr2), 1, fp);

      data (string);
      fwrite (string, strlen (string) + 1, 1, fp);
      fwrite (remote_sysop, strlen (remote_sysop) + 1, 1, fp);
      fwrite (VERSION, strlen (VERSION) + 1, 1, fp);
      fwrite (rp, strlen (rp) + 1, 1, fp);

      if (remote_point) {
         sprintf (string, "\x01TOPT %d\r\n", remote_point);
         fwrite (string, strlen (string), 1, fp);
      }

      sprintf (string, msgtxt[M_PID], VERSION, registered ? "" : NOREG);
      fwrite (string, strlen (string), 1, fp);

      sprintf (string, msgtxt[M_MSGID], config->alias[assumed].zone, config->alias[assumed].net, config->alias[assumed].node, config->alias[assumed].point, time (NULL));
      fwrite (string, strlen (string), 1, fp);

      sprintf (string, hd1, config->alias[assumed].zone, config->alias[assumed].net, config->alias[assumed].node);
      fwrite (string, strlen (string), 1, fp);
      fwrite (hd2, strlen (hd2), 1, fp);

      totals = 0L;
      nfiles = 0;
   }
   else if (id == 2) {
      sprintf (string, "%-12s            %s\r\n", name, "* Not found *");
      fwrite (string, strlen (string), 1, fp);
   }
   else if (id == 3) {
      sprintf (string, "%-12s            %s\r\n", name, "* Too big *");
      fwrite (string, strlen (string), 1, fp);
   }
   else if (id == 4) {
      sprintf (string, "%-12s            %s\r\n", name, "* F/Req. limit exceeded *");
      fwrite (string, strlen (string), 1, fp);
   }
   else if (id == 5) {
      if (!stricmp (drive, sys.filepath) && sys.filelist[0])
         strcpy (drive, sys.filelist);
      else
         strcat (drive, "FILES.BBS");

      if (findfirst (rqname, &blk, 0))
         sprintf (string, "%-12s            %s\r\n", name, "* Not found *");

      else {
         fp2 = fopen (drive, "rt");

         sprintf (string, "%-12s %8ld   %s\r\n", name, blk.ff_fsize, "* No description *");
         totals += blk.ff_fsize;
         nfiles++;

         if (fp2 != NULL) {
            while (!feof (fp2)) {
               path[0] = 0;
               if (fgets (path, 70, fp2) == NULL)
                  break;
               if (!strnicmp (path, name, strlen (name))) {
                  path[strlen (path) - 1] = 0;
                  if (strlen (&path[13]) > 54)
                     path[54 + 13] = '\0';
                  sprintf (string, "%-12s %8ld   %s\r\n", name, blk.ff_fsize, &path[13]);
                  break;
               }
            }

            fclose (fp2);
         }
      }

      fwrite (string, strlen (string), 1, fp);
   }
   else if (id == 6) {
      sprintf (string, "%sTotal        %8ld\r\n", hd3, totals);
      fwrite (string, strlen (string), 1, fp);

      if ((fp2 = fopen ("REPORT.APP", "rb")) == NULL) {
         sprintf (string, "\r\n%s, %d:%d/%d\r\n", config->sysop, config->alias[assumed].zone, config->alias[assumed].net, config->alias[assumed].node);
         fwrite (string, strlen (string), 1, fp);
         sprintf (string, "%s\r\n", system_name);
         fwrite (string, strlen (string), 1, fp);
      }
      else {
         while (fgets (path, 255, fp2) != NULL)
            fputs (path, fp);
         fclose (fp2);
      }
      fwrite ("\0\0\0", 3, 1, fp);
   }
   else if (id == 7) {
      sprintf (string, "%-12s            %s\r\n", name, "* Refused by remote *");
      fwrite (string, strlen (string), 1, fp);
   }

   fclose (fp);
}

char oksend[MAX_EMSI_ADDR];

void emsi_handshake (originator)
{
   int np, wh, stat, i, m, v, ntt_mail = 0, ntt_data = 0;
   char req[512], ttm[50]; // , oksend[MAX_EMSI_ADDR];
   long t, btt_mail = 0, btt_data = 0, olc;

   if (originator) {
      if (!send_emsi_handshake (originator))
         return;
      if (!CARRIER)
         return;
      if (!get_emsi_handshake (originator))
         return;
      if (!CARRIER)
         return;
   }
   else {
      if (!get_emsi_handshake (originator))
         return;
      if (!CARRIER)
         return;
      if (!send_emsi_handshake (originator))
         return;
      if (!CARRIER)
         return;
   }

   got_arcmail = 0;
   made_request = 0;
   timeout = 0L;
   tot_rcv = tot_sent = 0;
   freceived = 0;
   fsent = 0;

   remote_zone = addrs[0].zone;
   remote_net = addrs[0].net;
   remote_node = addrs[0].node;
   remote_point = addrs[0].point;

   if (!originator) {
      called_zone = remote_zone;
      called_net = remote_net;
      called_node = remote_node;
   }
   else {
      for (np = 1; np < MAX_EMSI_ADDR; np++)
         if (addrs[np].net == 0) {
            addrs[np].zone = called_zone;
            addrs[np].net = called_net;
            addrs[np].node = called_node;
            addrs[np].point = 0;
            break;
         }
   }

   if (originator && ((remote_zone != called_zone) || (remote_net != called_net) || (remote_node != called_node)))
      status_line (msgtxt[M_CALLED], called_zone,called_net,called_node,remote_zone,remote_net,remote_node);
   else {
      called_zone = remote_zone;
      called_net = remote_net;
      called_node = remote_node;
   }

   if (!CARRIER)
      return;

   filetransfer_system ();
   update_filesio (fsent, freceived);

   wh = wopen (12, 0, 24, 79, 0, LGREY|_BLACK, WHITE|_BLACK);
   wactiv (wh);
   if (originator)
      wtitle ("OUTBOUND CALL STATUS", TLEFT, LCYAN|_BLACK);
   else
      wtitle ("INBOUND CALL STATUS", TLEFT, LCYAN|_BLACK);
   printc (12, 0, LGREY|_BLACK, 'ֳ');
   printc (12, 52, LGREY|_BLACK, 'ֱ');
   printc (12, 79, LGREY|_BLACK, '´');
   whline (8, 0, 80, 0, LGREY|_BLACK);

   set_mailon (remote_zone, remote_net, remote_node, remote_point, remote_location);

   sprintf (req, "%u:%u/%u.%u, %s, %s, %s", remote_zone, remote_net, remote_node, remote_point, remote_sysop, remote_system, remote_location);
   if (strlen (req) > 78)
      req[78] = '\0';
   wcenters (0, LGREY|_BLACK, req);

   sprintf (req, "Connected at %lu baud with %s", rate, remote_program);
   if (strlen (req) > 78)
      req[78] = '\0';
   wcenters (1, LGREY|_BLACK, req);

   if ((originator && !addrs[2].net) || (!originator && !addrs[1].net))
      wcenters (2, LGREY|_BLACK, "AKAs: No aka presented");
   else {
      strcpy (req, "AKAs:");

      for (np = 1; np < MAX_EMSI_ADDR; np++) {
         if (!originator) {
            if (addrs[np].net == 0)
               break;
         }
         else {
            if (addrs[np + 1].net == 0)
               break;
         }

         sprintf (ttm, " %u:%u/%u.%u", addrs[np].zone, addrs[np].net, addrs[np].node, addrs[np].point);
         if (np > 1) {
            if (strlen (req) + strlen (ttm) > 78)
               break;
            strcat (req, ",");
         }
         strcat (req, ttm);
      }

      if (strlen (req) > 78)
         req[78] = '\0';
      wcenters (2, LGREY|_BLACK, req);
   }

   wprints (5, 2, LCYAN|_BLACK, "Files");
   wprints (6, 2, LCYAN|_BLACK, "Bytes");

   wprints (4, 9, LCYAN|_BLACK, " ÚִִִִMailPKTִִִִִִִDataִִִִִ¿");
   wprints (5, 9, LCYAN|_BLACK, "תתתתתתתתתתתתתתתתתתתתתתתתתתתתתתת");
   wprints (6, 9, LCYAN|_BLACK, "תתתתתתתתתתתתתתתתתתתתתתתתתתתתתתת");
   wprints (7, 9, LCYAN|_BLACK, " ְִִִִִִINBOUND TRAFFICִִִִִִÙ");

   if (got_maildata) {
      sprintf (req, "%d", n_mail);
      wrjusts (5, 20, YELLOW|_BLACK, req);
      sprintf (req, "%ld", b_mail);
      wrjusts (6, 20, YELLOW|_BLACK, req);
      sprintf (req, "%d", n_data);
      wrjusts (5, 31, YELLOW|_BLACK, req);
      sprintf (req, "%ld", b_data);
      wrjusts (6, 31, YELLOW|_BLACK, req);
   }
   else {
      wrjusts (5, 20, YELLOW|_BLACK, "N/A");
      wrjusts (6, 20, YELLOW|_BLACK, "N/A");
      wrjusts (5, 31, YELLOW|_BLACK, "N/A");
      wrjusts (6, 31, YELLOW|_BLACK, "N/A");
   }

   wprints (4, 44, LCYAN|_BLACK, " ÚִִִִMailPKTִִִִִִִDataִִִִִ¿");
   wprints (5, 44, LCYAN|_BLACK, "תתתתתתתתתתתתתתתתתתתתתתתתתתתתתתת");
   wprints (6, 44, LCYAN|_BLACK, "תתתתתתתתתתתתתתתתתתתתתתתתתתתתתתת");
   wprints (7, 44, LCYAN|_BLACK, " ְִִִִִOUTBOUND TRAFFICִִִִִִÙ");

   if (originator) {
      sprintf (req, "%d", call_list[next_call].n_mail);
      wrjusts (5, 55, YELLOW|_BLACK, req);
      sprintf (req, "%ld", call_list[next_call].b_mail);
      wrjusts (6, 55, YELLOW|_BLACK, req);
      sprintf (req, "%d", call_list[next_call].n_data);
      wrjusts (5, 66, YELLOW|_BLACK, req);
      sprintf (req, "%ld", call_list[next_call].b_data);
      wrjusts (6, 66, YELLOW|_BLACK, req);
   }
   else {
      for (m = 0; m < MAX_EMSI_ADDR; m++) {
         if (addrs[m].net == 0)
            break;

         for (i = 0; i < max_call; i++) {
            if (addrs[m].zone == call_list[i].zone && addrs[m].net == call_list[i].net && addrs[m].node == call_list[i].node && addrs[m].point == call_list[i].point) {
               ntt_mail += call_list[i].n_mail;
               btt_mail += call_list[i].b_mail;
               ntt_data += call_list[i].n_data;
               btt_data += call_list[i].b_data;
               break;
            }
         }

         if (addrs[m].point) {
            for (v = 0; v < MAX_ALIAS && config->alias[v].net; v++) {
               if (addrs[m].zone == config->alias[v].zone && addrs[m].net == config->alias[v].net && addrs[m].node == config->alias[v].node && config->alias[v].fakenet)
                  break;
            }

            if (v < MAX_ALIAS && config->alias[v].net) {
               for (i = 0; i < max_call; i++) {
                  if (addrs[m].zone == call_list[i].zone && config->alias[v].fakenet == call_list[i].net && addrs[m].point == call_list[i].node && !call_list[i].point) {
                     ntt_mail += call_list[i].n_mail;
                     btt_mail += call_list[i].b_mail;
                     ntt_data += call_list[i].n_data;
                     btt_data += call_list[i].b_data;
                     break;
                  }
               }
            }
         }
      }

      sprintf (req, "%d", ntt_mail);
      wrjusts (5, 55, YELLOW|_BLACK, req);
      sprintf (req, "%ld", btt_mail);
      wrjusts (6, 55, YELLOW|_BLACK, req);
      sprintf (req, "%d", ntt_data);
      wrjusts (5, 66, YELLOW|_BLACK, req);
      sprintf (req, "%ld", btt_data);
      wrjusts (6, 66, YELLOW|_BLACK, req);
   }

   if (remote_net == config->alias[assumed].net && remote_node == config->alias[assumed].node) {
      called_net = config->alias[assumed].fakenet;
      called_node = remote_point;
   }
   else if (remote_point)
      called_net = -1;

   emsi = 1;
   emsi_sent = 0;
   made_request = 0;

   if ((remote_capabilities & DOES_IANUS) && (mdm_flags == NULL || strstr (mdm_flags, "Hst") == NULL) && config->janus) {
      prints (7, 65, YELLOW|_BLACK, "EMSI/Janus");
      status_line (":EMSI method: %s", "Janus");
   }
   else if (remote_capabilities & ZED_ZAPPER) {
      prints (7, 65, YELLOW|_BLACK, "EMSI/ZedZap");
      status_line(":EMSI method: %s", "ZedZap");
   }
   else if (remote_capabilities & ZED_ZIPPER) {
      prints (7, 65, YELLOW|_BLACK, "EMSI/ZedZap");
      status_line(":EMSI method: %s", "ZedZip");
   }

   for (np = 0; np < 10; np++) {
      if (config->alias[np].net == 0)
         break;

      sprintf(req,request_template,filepath,config->alias[np].net,config->alias[np].node);
      unlink(req);
   }

   for (np = 0; np < MAX_EMSI_ADDR; np++) {
      if (addrs[np].net == 0)
         break;
      if (flag_file (TEST_AND_SET, addrs[np].zone, addrs[np].net, addrs[np].node, addrs[np].point, 1))
         oksend[np] = 0;
      else
         oksend[np] = 1;
   }

   if ((remote_capabilities & DOES_IANUS) && (mdm_flags == NULL || strstr (mdm_flags, "Hst") == NULL) && config->janus) {
      isOriginator = originator;
      Janus ();
      goto endwazoo;
   }

   if (remote_capabilities & ZED_ZAPPER)
      remote_capabilities &= ~ZED_ZIPPER;
   else if (remote_capabilities & ZED_ZIPPER)
      remote_capabilities &= ~ZED_ZAPPER;
   else
      goto endwazoo;

   if (originator) {
      send_WaZOO (originator);

      for (m = 0; m < MAX_EMSI_ADDR; m++) {
         if (addrs[m].net == 0)
            break;

         if (addrs[m + 1].net == 0) {
            if (addrs[m].zone == addrs[0].zone && addrs[m].net == addrs[0].net && addrs[m].node == addrs[0].node && addrs[m].point == addrs[0].point)
               break;
         }

         called_zone = remote_zone = addrs[m].zone;
         called_net = remote_net = addrs[m].net;
         called_node = remote_node = addrs[m].node;
         remote_point = addrs[m].point;

         if (addrs[m].point) {
            for (v = 0; v < MAX_ALIAS && config->alias[v].net; v++) {
               if (!config->alias[v].net)
                  break;
               if (addrs[m].zone == config->alias[v].zone && addrs[m].net == config->alias[v].net && addrs[m].node == config->alias[v].node && config->alias[v].fakenet)
                  break;
            }

            if (v < MAX_ALIAS && config->alias[v].net) {
               called_node = addrs[m].point;
               called_net = config->alias[v].fakenet;
            }
         }

         send_WaZOO(originator);
      }

      send_Zmodem(NULL,NULL,((emsi_sent)?END_BATCH:NOTHING_TO_DO),DO_WAZOO);
      if (!CARRIER)
         goto endwazoo;

      get_Zmodem(filepath, NULL);
      if (!CARRIER)
         goto endwazoo;

      if (made_request) {
         called_zone = remote_zone = addrs[0].zone;
         if (!called_zone)
            called_zone = remote_zone = config->alias[0].zone;
         called_net = remote_net = addrs[0].net;
         called_node = remote_node = addrs[0].node;
         remote_point = addrs[0].point;

         for (i = 0; i < MAX_ALIAS; i++) {
            if (config->alias[i].net == 0)
               break;
            if (remote_zone == config->alias[i].zone && remote_net == config->alias[i].net && remote_node == config->alias[i].node && remote_point && config->alias[i].fakenet)
               break;
         }
         if (remote_zone == config->alias[i].zone && remote_net == config->alias[i].net && remote_node == config->alias[i].node && remote_point && config->alias[i].fakenet) {
            called_net = config->alias[i].fakenet;
            called_node = remote_point;
         }
         else if (remote_point)
            called_net = -1;

         stat = respond_to_file_requests(fsent);
         send_Zmodem(NULL,NULL,((stat)?END_BATCH:NOTHING_TO_DO),DO_WAZOO);
      }
   }
   else {
      get_Zmodem(filepath, NULL);
      if (!CARRIER)
         goto endwazoo;
      made_request = 0;

      for (m = 0; m < MAX_EMSI_ADDR; m++) {
         if (addrs[m].net == 0)
            break;

         called_zone = remote_zone = addrs[m].zone;
         called_net = remote_net = addrs[m].net;
         called_node = remote_node = addrs[m].node;
         remote_point = addrs[m].point;

         if (addrs[m].point) {
            for (v = 0; v < MAX_ALIAS && config->alias[v].net; v++) {
               if (!config->alias[v].net)
                  break;
               if (addrs[m].zone == config->alias[v].zone && addrs[m].net == config->alias[v].net && addrs[m].node == config->alias[v].node && config->alias[v].fakenet)
                  break;
            }

            if (v < MAX_ALIAS && config->alias[v].net) {
               called_node = addrs[m].point;
               called_net = config->alias[v].fakenet;
            }
            else
               called_net = -1;
         }

         if (get_bbs_record (called_zone, called_net, called_node, 0)) {
            if (nodelist.password[0] && strcmp(strupr(nodelist.password),strupr(remote_password))) {
               status_line(msgtxt[M_PWD_ERROR],remote_zone,remote_net,remote_node,remote_point,remote_password,nodelist.password);
               continue;
            }
         }
         else if (remote_point) {
            if (get_bbs_record (remote_zone, remote_net, remote_node, remote_point)) {
               if (nodelist.password[0] && strcmp(strupr(nodelist.password),strupr(remote_password))) {
                  status_line(msgtxt[M_PWD_ERROR],remote_zone,remote_net,remote_node,remote_point,remote_password,nodelist.password);
                  continue;
               }
            }
         }

         send_WaZOO(originator);
      }

      called_zone = remote_zone = addrs[0].zone;
      if (!called_zone)
         called_zone = remote_zone = config->alias[0].zone;
      called_net = remote_net = addrs[0].net;
      called_node = remote_node = addrs[0].node;
      remote_point = addrs[0].point;

      for (i = 0; i < MAX_ALIAS; i++) {
         if (config->alias[i].net == 0)
            break;
         if (remote_zone == config->alias[i].zone && remote_net == config->alias[i].net && remote_node == config->alias[i].node && remote_point && config->alias[i].fakenet)
            break;
      }
      if (remote_zone == config->alias[i].zone && remote_net == config->alias[i].net && remote_node == config->alias[i].node && remote_point && config->alias[i].fakenet) {
         called_net = config->alias[i].fakenet;
         called_node = remote_point;
      }
      else if (remote_point)
         called_net = -1;

      emsi_sent = respond_to_file_requests(fsent);
      send_Zmodem(NULL,NULL,((emsi_sent)?END_BATCH:NOTHING_TO_DO),DO_WAZOO);
      if (!CARRIER || !made_request)
         goto endwazoo;

      get_Zmodem(filepath, NULL);
   }

endwazoo:
   local_mode = caller = 0;

   if (!CARRIER)
      status_line(msgtxt[M_NO_CARRIER]);

   for (np = 0; np < MAX_EMSI_ADDR; np++) {
      if (addrs[np].net == 0)
         break;
      if (originator && addrs[np + 1].net == 0)
         break;
      if (oksend[np])
         flag_file (CLEAR_FLAG, addrs[np].zone, addrs[np].net, addrs[np].node, addrs[np].point, 1);
   }

   wactiv(wh);
   wclose();

   status_line ("*End of EMSI Session");

   t = time (NULL) - elapsed + 20L;
   if (originator) {
      olc = get_phone_cost (addrs[0].zone, addrs[0].net, addrs[0].node, t);
      status_line ("*Session with %d:%d/%d.%d Time: %ld:%02ld, Cost: $%ld.%02ld", addrs[0].zone, addrs[0].net, addrs[0].node, addrs[0].point, t / 60L, t % 60L, olc / 100L, olc % 100L);
   }
   else
      status_line ("*Session with %d:%d/%d.%d Time: %ld:%02ld", addrs[0].zone, addrs[0].net, addrs[0].node, addrs[0].point, t / 60L, t % 60L);

   if (cur_event > -1 && (e_ptrs[cur_event]->behavior & MAT_DYNAM)) {
      e_ptrs[cur_event]->behavior |= MAT_SKIP;
      write_sched ();
   }

   mail_history (originator, addrs[0].zone, addrs[0].net, addrs[0].node, addrs[0].point, (int)t, tot_sent, tot_rcv, olc);
   reset_mailon ();
   memset ((char *)&usr, 0, sizeof (struct _usr));

   local_status ("Hangup");
   modem_hangup ();

   if (originator) {
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
   }
   else {
      sysinfo.today.incalls++;
      sysinfo.week.incalls++;
      sysinfo.month.incalls++;
      sysinfo.year.incalls++;
      sysinfo.today.inconnects += t;
      sysinfo.week.inconnects += t;
      sysinfo.month.inconnects += t;
      sysinfo.year.inconnects += t;
   }

   if (!nomailproc && got_arcmail) {
      if (cur_event > -1 && e_ptrs[cur_event]->errlevel[2])
         aftermail_exit = e_ptrs[cur_event]->errlevel[2];

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

      if (aftermail_exit) {
         status_line (msgtxt[M_EXIT_AFTER_MAIL], aftermail_exit);
         get_down (aftermail_exit, 3);
      }
   }

   get_down (0, 2);
}

static void mail_history (originator, zo, ne, no, po, dur, sent, recv, mailcost)
int originator, zo, ne, no, po;
long sent, recv, mailcost;
{
   FILE *fp;
   char filename[80], addr[60];
   long tempo;
   struct tm *tim;

   tempo = time (NULL);
   tim   = localtime (&tempo);

   if (originator)
      sprintf (filename, "%sOUTBOUND.HIS", config->sys_path);
   else
      sprintf (filename, "%sINBOUND.HIS", config->sys_path);

   if (!dexists (filename)) {
      fp = fopen (filename, "at");
      fprintf (fp, "");
      fprintf (fp, "Date      Time   %-45.45s    Sent  Received    E/T", originator ? "Sent to" : "Received from");
      if (originator)
         fprintf (fp, "  Cost\n");
      else
         fprintf (fp, "\n");
   }
   else
      fp = fopen (filename, "at");

   fprintf (fp, "%02d/%02d/%02d  %02d:%02d  ", tim->tm_mon + 1, tim->tm_mday, tim->tm_year % 100, tim->tm_hour, tim->tm_min);
   if (originator || !po)
      sprintf (addr, "%d:%d/%d ", zo, ne, no);
   else
      sprintf (addr, "%d:%d/%d.%d ", zo, ne, no, po);
   if (strlen (addr) + strlen (remote_system) > 45)
      remote_system[45 - strlen (addr)] = '\0';
   strcat (addr, remote_system);
   fprintf (fp, "%-45.45s %7ld    %7ld  %02d:%02d  ", addr, sent, recv, dur / 60, dur % 60);
   if (originator)
      fprintf (fp, "%ld.%02ld\n", mailcost / 100L, mailcost % 100L);
   else
      fprintf (fp, "\n");
   fclose (fp);
}

char *message_to_sysop (zo, ne, no, po, text, subj)
int zo, ne, no, po;
char *text, *subj;
{
   FILE *fpd;
   int mi, z;
   char buff[80], *p, tmp[40], buffer[2050];
   struct _msghdr2 mhdr;
   struct _pkthdr2 pkthdr;
   struct date datep;
   struct time timep;

   fpd = fopen (text, "rb");
   if (fpd == NULL)
      return (NULL);

   gettime (&timep);
   getdate (&datep);

   p = HoldAreaNameMunge (zo);
   sprintf (buff, "%s%04x%04x.OUT", p, ne, no);
   strcpy (e_input, buff);
   mi = open (buff, O_RDWR|O_CREAT|O_BINARY, S_IREAD|S_IWRITE);
   if (filelength (mi) > 0L)
      lseek(mi,filelength(mi)-2,SEEK_SET);
   else {
      memset ((char *)&pkthdr, 0, sizeof (struct _pkthdr2));
      pkthdr.ver = PKTVER;
      pkthdr.product = 0x4E;
      pkthdr.serial = 2 * 16 + 21;
      pkthdr.capability = 1;
      pkthdr.cwvalidation = 256;
      pkthdr.orig_node = config->alias[assumed].node;
      pkthdr.orig_net = config->alias[assumed].net;
      pkthdr.orig_zone = config->alias[assumed].zone;
      pkthdr.orig_point = config->alias[assumed].point;
      pkthdr.dest_node = no;
      pkthdr.dest_net = ne;
      pkthdr.dest_zone = zo;
      pkthdr.dest_point = po;
      pkthdr.hour = timep.ti_hour;
      pkthdr.minute = timep.ti_min;
      pkthdr.second = timep.ti_sec;
      pkthdr.year = datep.da_year;
      pkthdr.month = datep.da_mon - 1;
      pkthdr.day = datep.da_day;
      write (mi, (char *)&pkthdr, sizeof (struct _pkthdr2));
   }

   mhdr.ver = PKTVER;
   mhdr.orig_node = config->alias[assumed].node;
   mhdr.orig_net = config->alias[assumed].net;
   mhdr.dest_node = no;
   mhdr.dest_net = ne;
   mhdr.cost = 0;
   mhdr.attrib = 0;

   write (mi, (char *)&mhdr, sizeof (struct _msghdr2));

   data (tmp);
   write (mi, tmp, strlen (tmp) + 1);
   write (mi, remote_sysop, strlen (remote_sysop) + 1);
   write (mi, sysop, strlen (sysop) + 1);
   write (mi, subj, strlen (subj) + 1);

   do {
      z = fread(buffer, 1, 2048, fpd);
      write(mi, buffer, z);
   } while (z == 2048);

   buff[0] = buff[1] = buff[2] = 0;
   write (mi, buff, 3);

   close (mi);
   fclose (fpd);

   sysinfo.today.emailsent++;
   sysinfo.week.emailsent++;
   sysinfo.month.emailsent++;
   sysinfo.year.emailsent++;

   return (e_input);
}

