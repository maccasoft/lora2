#include <sys\types.h>
#include <sys\stat.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <conio.h>
#include <stdlib.h>
#include <alloc.h>

#include <cxl\cxlwin.h>
#include <cxl\cxlvid.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "zmodem.h"
#include "externs.h"
#include "prototyp.h"
#include "defines.h"
#include "janus.h"
#include "tc_utime.h"

#define MAX_EMSI_ADDR   30

void update_filesio (int, int);
void set_prior (int);
int is_mailpkt (char *p, int n);

extern int emsi;
extern long tot_sent, tot_rcv;
extern char oksend[MAX_EMSI_ADDR];
extern struct _alias addrs[MAX_EMSI_ADDR + 1];

/* Private routines */
static void getfname(word);
static void sendpkt(byte *,int,int);
static void sendpkt32(byte *,int,int);
static void txbyte(byte);
static long procfname(void);
static byte rcvpkt(void);
static void rxclose(word);
static void endbatch(void);
static void j_message(word,char *,...);
static void j_status(char *,...);
static int j_error (byte *msg, byte *fname);
static void long_set_timer(long *,word);
static int long_time_gone(long *);
static int rcvrawbyte(void);
static int rxbyte(void);
static void xfer_summary(char *,char *,long *,int);
static void update_status(long *,long *,long,int *,int);
static void through(long *,long *);
static int get_filereq(byte);
static byte get_reqname(byte);
static void mark_done(char *);

static void open_janus_filetransfer(void);
static void close_janus_filetransfer(void);


/* Private data. I know, proper software design says you shouldn't make data */
/* global unless you really need to.  In this case speed and code size make  */
/* it more important to avoid constantly pushing & popping arguments.        */

static byte *GenericError = "!%s";
static byte *ReqTmp = "JANUSREQ.TMP";
static char *Rxbuf;       /* Address of packet reception buffer              */
static char *Txfname;     /* Full path of file we're sending                 */
static char *Rxfname;     /* Full path of file we're receiving               */
static byte *Rxbufptr;    /* Current position within packet reception buffer */
static byte *Rxbufmax;    /* Upper bound of packet reception buffer          */
static byte Do_after;     /* What to do with file being sent when we're done */
static byte jfsent;       /* Did we manage to send anything this session?    */
static byte WaitFlag;     /* Tells rcvrawbyte() whether or not to wait       */
static byte SharedCap;    /* Capability bits both sides have in common       */
static int Txfile;        /* File handle of file we're sending               */
static int Rxfile;        /* File handle of file we're receiving             */
static int ReqRecorded;   /* Number of files obtained by this request        */
static word TimeoutSecs;  /* How long to wait for various things             */
static word Rxblklen;     /* Length of data in last data block packet recvd  */
static word Tx_y;         /* Line number of file transmission status display */
static word Rx_y;         /* Line number of file reception status display    */
static long Txlen;        /* Total length of file we're sending              */
static long Rxlen;        /* Total length of file we're receiving            */
static long Rxfiletime;   /* Timestamp of file we're receiving               */
static long Diskavail;    /* Bytes available in upload directory             */
static long TotalBytes;   /* Total bytes xferred in this session             */
static long Txsttime;     /* Time at which we started sending current file   */
static long Rxsttime;     /* Time at which we started receiving current file */

// static int janus_handle;


/*****************************************************************************/
/* Super-duper neato-whizbang full-duplex streaming ACKless batch file       */
/* transfer protocol for use in WaZOO mail sessions                          */
/*****************************************************************************/
void Janus(void) {
   byte xstate;           /* Current file transmission state                 */
   byte rstate;           /* Current file reception state                    */
   byte pkttype;          /* Type of packet last received                    */
   byte tx_inhibit;       /* Flag to wait and send after done receiving      */
   byte *holdname;        /* Name of hold area                               */
   byte sending_req;      /* Are we currently sending requested files?       */
   byte attempting_req;   /* Are we waiting for the sender to start our req? */
   byte req_started;      /* Has the sender started servicing our request?   */
   int txoldeta;          /* Last transmission ETA displayed                 */
   int rxoldeta;          /* Last reception ETA displayed                    */
   int nreceived;         /* Number of received files                        */
   int nsent;             /* Number of sent files                            */
   word blklen;           /* Length of last data block sent                  */
   word txblklen;         /* Size of data block to try to send this time     */
   word txblkmax;         /* Max size of data block to send at this speed    */
   word goodneeded;       /* # good bytes to send before upping txblklen     */
   word goodbytes;        /* Number of good bytes sent at this block size    */
   word rpos_count;       /* Number of RPOS packets sent at this position    */
   word req_sent;         /* File requests sent this session           (M^M) */
   long xmit_retry;       /* Time to retransmit lost FNAMEPKT or EOF packet  */
   long txpos;            /* Current position within file we're sending      */
   long lasttx;           /* Position within file of last data block we sent */
   long starttime;        /* Time at which we started this Janus session     */
   long txstpos;          /* Initial data position of file we're sending     */
   long rxstpos;          /* Initial data position of file we're receiving   */
   long txoldpos;         /* Last transmission file position displayed       */
   long rxoldpos;         /* Last reception file position displayed          */
   long rpos_retry;       /* Time at which to retry RPOS packet              */
   long brain_dead;       /* Time at which to give up on other computer      */
   long rpos_sttime;      /* Time at which we started current RPOS sequence  */
   long last_rpostime;    /* Timetag of last RPOS which we performed         */
   long last_blkpos;      /* File position of last out-of-sequence BLKPKT    */
   FILE *reqfile;         /* File handle for .REQ file                       */

//   set_prior (3);                               /* Time Critical             */
   XON_DISABLE();
   sprintf (ReqTmp, "JANUS%03x.TMP", line_offset);

   CurrentReqLim = max_requests;
//   janus_handle = -1;
   Tx_y = 9;
   Rx_y = 10;
   SharedCap = 0;
   TotalBytes = 0;
   lasttx = 0L;
   last_blkpos = 0L;
   rxstpos = 0L;
   req_sent = 0;
   nreceived = nsent = 0;
   time(&starttime);
   update_filesio (nsent, nreceived);

   /*------------------------------------------------------------------------*/
   /* Allocate memory                                                        */
   /*------------------------------------------------------------------------*/
   Rxbuf = (char *) Txbuf + 4096 + 8;
   Txfname = Rxfname = NULL;
   if (!(Txfname = malloc(PATHLEN)) || !(Rxfname = malloc(PATHLEN))) {
      status_line(msgtxt[M_MEM_ERROR]);
      modem_hangup();
      goto freemem;
   }
   Rxbufmax = (byte *) (Rxbuf + BUFMAX + 8);

   open_janus_filetransfer();

   /*------------------------------------------------------------------------*/
   /* Initialize file transmission variables                                 */
   /*------------------------------------------------------------------------*/
   tx_inhibit = FALSE;
   last_rpostime = xmit_retry = 0L;
   TimeoutSecs = (unsigned int)(40960U / rate);
   if (TimeoutSecs < 30)
      TimeoutSecs = 30;
   long_set_timer(&brain_dead,120);
   if (rate > 64000L)
      txblkmax = BUFMAX;
   else {
      txblkmax = (word)(rate / 300 * 128);
      if (txblkmax > BUFMAX)
         txblkmax = BUFMAX;
   }
   txblklen = txblkmax;
   goodbytes = goodneeded = 0;
   Txfile = -1;
   sending_req = jfsent = FALSE;
   xstate = XSENDFNAME;
   getfname(INITIAL_XFER);

   /*------------------------------------------------------------------------*/
   /* Initialize file reception variables                                    */
   /*------------------------------------------------------------------------*/
   holdname = HoldAreaNameMunge (config->alias[0].zone);
   sprintf(Abortlog_name,"%sRECOVERY.Z\0",holdname);
//   holdname = HoldAreaNameMunge (remote_zone);
//   sprintf(Abortlog_name,"%s%04x%04x.Z\0",holdname,called_net,called_node);

   Diskavail = (filepath[1] == ':') ? zfree(filepath) : 0x7FFFFFFFL;
   Rxbufptr = NULL;
   rpos_retry = rpos_count = 0;
   attempting_req = req_started = FALSE;
   rstate = RRCVFNAME;

   /*------------------------------------------------------------------------*/
   /* Send and/or receive stuff until we're done with both                   */
   /*------------------------------------------------------------------------*/
   do {                                          /* while (xstate || rstate) */

      /*---------------------------------------------------------------------*/
      /* If nothing useful (i.e. sending or receiving good data block) has   */
      /* happened within the last 2 minutes, give up in disgust              */
      /*---------------------------------------------------------------------*/
      if (long_time_gone(&brain_dead)) {
         j_status(msgtxt[M_OTHER_DIED]);           /* "He's dead, Jim." */
         goto giveup;
      }

      /*---------------------------------------------------------------------*/
      /* If we're tired of waiting for an ACK, try again                     */
      /*---------------------------------------------------------------------*/
      if (xmit_retry) {
         if (long_time_gone(&xmit_retry)) {
            j_message(Tx_y,msgtxt[M_TIMEOUT]);
            xmit_retry = 0L;

            switch (xstate) {
               case XRCVFNACK:
                  xstate = XSENDFNAME;
                  break;
               case XRCVFRNAKACK:
                  xstate = XSENDFREQNAK;
                  break;
               case XRCVEOFACK:
                  errno = 0;
                  txpos=lasttx;
                  lseek(Txfile, txpos, SEEK_SET);
                  if (j_error(msgtxt[M_SEEK_MSG],Txfname))
                     goto giveup;
                  xstate = XSENDBLK;
                  break;
            }
         }
      }

      /*---------------------------------------------------------------------*/
      /* Transmit next part of file, if any                                  */
      /*---------------------------------------------------------------------*/
      switch (xstate) {
         case XSENDBLK:
            if (tx_inhibit)
               break;
            *((long *)Txbuf) = lasttx = txpos;
            errno = 0;
            blklen = read(Txfile, Txbuf+sizeof(txpos), txblklen);
            if (j_error(msgtxt[M_READ_MSG],Txfname))
               goto giveup;
            txpos += blklen;
            sendpkt(Txbuf, sizeof(txpos)+blklen, BLKPKT);
            update_status(&txpos,&txoldpos,Txlen-txpos,&txoldeta,Tx_y);
            jfsent = TRUE;
            if (txpos >= Txlen || blklen < txblklen) {
               long_set_timer(&xmit_retry,TimeoutSecs);
               xstate = XRCVEOFACK;
            } else
               long_set_timer(&brain_dead,120);

            if (txblklen < txblkmax && (goodbytes+=txblklen) >= goodneeded) {
               txblklen <<= 1;
               goodbytes = 0;
            }
            break;

         case XSENDFNAME:
            blklen = (int)(strchr( strchr(Txbuf,'\0')+1, '\0') - Txbuf + 1);
            Txbuf[blklen++] = OURCAP;
            sendpkt(Txbuf,blklen,FNAMEPKT);
            txoldpos = txoldeta = -1;
            long_set_timer(&xmit_retry,TimeoutSecs);
            xstate = XRCVFNACK;
            break;

         case XSENDFREQNAK:
            sendpkt(NULL,0,FREQNAKPKT);
            long_set_timer(&xmit_retry,TimeoutSecs);
            xstate = XRCVFRNAKACK;
            break;
      }

      /*---------------------------------------------------------------------*/
      /* Catch up on our reading; receive and handle all outstanding packets */
      /*---------------------------------------------------------------------*/
      while ((pkttype = rcvpkt()) != 0) {
         if (pkttype != BADPKT)
            long_set_timer(&brain_dead,120);
         switch (pkttype) {

            /*---------------------------------------------------------------*/
            /* File data block or munged block                               */
            /*---------------------------------------------------------------*/
            case BADPKT:
            case BLKPKT:
               if (rstate == RRCVBLK) {
                  if (pkttype == BADPKT || *((long *)Rxbuf) != Rxpos) {
                     if (pkttype == BLKPKT) {
                        if (*((long *)Rxbuf) < last_blkpos)
                           rpos_retry = rpos_count = 0;
                        last_blkpos = *((long *)Rxbuf);
                     }
                     if (long_time_gone(&rpos_retry)) {
                        /* If we're the called machine, and we're trying
                           to send stuff, and it seems to be screwing up
                           our ability to receive stuff, maybe this
                           connection just can't hack full-duplex.  Try
                           waiting till the sending system finishes before
                           sending our stuff to it */
                        if (rpos_count > 4) {
                           if (xstate && !isOriginator && !tx_inhibit) {
                              tx_inhibit = TRUE;
                              j_status(msgtxt[M_GOING_ONE_WAY]);
                           }
                           rpos_count = 0;
                        }
                        if (++rpos_count == 1)
                           time(&rpos_sttime);
                        j_message(Rx_y,msgtxt[M_J_BAD_PACKET],Rxpos);
                        *((long *)Rxbuf) = Rxpos;
                        *((long *)(Rxbuf + sizeof(Rxpos))) = rpos_sttime;
                        sendpkt(Rxbuf, sizeof(Rxpos)+sizeof(rpos_sttime), RPOSPKT);
                        long_set_timer(&rpos_retry,TimeoutSecs/2);
                     }
                  } else {
                     last_blkpos = Rxpos;
                     rpos_retry = rpos_count = 0;
                     errno = 0;
                     write(Rxfile, Rxbuf+sizeof(Rxpos), Rxblklen -= sizeof(Rxpos));
                     if (j_error(msgtxt[M_WRITE_MSG],Rxfname))
                        goto giveup;
                     Diskavail -= Rxblklen;
                     Rxpos += Rxblklen;
                     update_status(&Rxpos,&rxoldpos,Rxlen-Rxpos,&rxoldeta,Rx_y);
                     if (Rxpos >= Rxlen) {
                        rxclose(GOOD_XFER);
                        Rxlen -= rxstpos;
                        through(&Rxlen,&Rxsttime);
                        tot_rcv += Rxlen;
                        sysinfo.today.bytereceived += Rxlen;
                        sysinfo.week.bytereceived += Rxlen;
                        sysinfo.month.bytereceived += Rxlen;
                        sysinfo.year.bytereceived += Rxlen;
                        sysinfo.today.filereceived++;
                        sysinfo.week.filereceived++;
                        sysinfo.month.filereceived++;
                        sysinfo.year.filereceived++;
                        nreceived++;
                        update_filesio (nsent, nreceived);
                        j_status("%s-J%s %s",msgtxt[M_FILE_RECEIVED], (SharedCap&CANCRC32)?"/32":" ",Rxfname);
//                        prints (10, 65, YELLOW|_BLACK, "              ");
//                        prints (11, 65, YELLOW|_BLACK, "              ");
                        wgotoxy (Rx_y, 0);
                        wclreol();

                        rstate = RRCVFNAME;
                     }
                  }
               }
               if (rstate == RRCVFNAME)
                  sendpkt(NULL,0,EOFACKPKT);
               break;

            /*---------------------------------------------------------------*/
            /* Name and other data for next file to receive                  */
            /*---------------------------------------------------------------*/
            case FNAMEPKT:
               if (rstate == RRCVFNAME)
                  Rxpos = rxstpos = procfname();
               if (!Rxfname[0] && get_filereq(req_started)) {
                  sendpkt(Rxbuf,strlen(Rxbuf)+2,FREQPKT);
                  attempting_req = TRUE;
                  req_started = FALSE;
               } else {
                  if (attempting_req) {
                     attempting_req = FALSE;
                     req_started = TRUE;
                  }
                  *((long *)Rxbuf) = Rxpos;
                  Rxbuf[sizeof(Rxpos)] = SharedCap;
                  sendpkt(Rxbuf,sizeof(Rxpos)+1,FNACKPKT);
                  rxoldpos = rxoldeta = -1;
                  if (Rxpos > -1)
                     rstate = (byte)((Rxfname[0]) ? RRCVBLK : RDONE);
                  else
                     j_status(msgtxt[M_REFUSING],Rxfname);
                  if (!rstate)
                     tx_inhibit = FALSE;
                  if (!(xstate || rstate))
                     goto breakout;
               }
               break;

            /*---------------------------------------------------------------*/
            /* ACK to filename packet we just sent                           */
            /*---------------------------------------------------------------*/
            case FNACKPKT:
               if (xstate == XRCVFNACK) {
                  xmit_retry = 0L;
                  if (Txfname[0]) {
                     SharedCap = (Rxblklen > sizeof(long)) ? Rxbuf[sizeof(long)] : 0;
                     if ((txpos = *((long *)Rxbuf)) > -1L) {
                        if (txpos)
                           status_line(msgtxt[M_SYNCHRONIZING],txpos);
                        errno = 0;
                        lseek(Txfile, txstpos = txpos, SEEK_SET);
                        if (j_error(msgtxt[M_SEEK_MSG],Txfname))
                           goto giveup;
                        xstate = XSENDBLK;
                     } else {
                        j_status(msgtxt[M_REMOTE_REFUSED],Txfname);
//                        prints (8, 65, YELLOW|_BLACK, "              ");
//                        prints (9, 65, YELLOW|_BLACK, "              ");
                        wgotoxy (Tx_y, 0);
                        wclreol();
                        if (sending_req) {
                           if (!(sending_req = get_reqname(FALSE)))
                              getfname(GOOD_XFER);
                        } else {
                           Do_after = NOTHING_AFTER;
                           getfname(GOOD_XFER);
                        }
                        xstate = XSENDFNAME;
                     }
                  } else {
/*                     sent_mail = 1;*/
                     xstate = XDONE;
                  }
               }
               if (!(xstate || rstate))
                  goto breakout;
               break;

            /*---------------------------------------------------------------*/
            /* Request to send more stuff rather than end batch just yet     */
            /*---------------------------------------------------------------*/
            case FREQPKT:
               if (xstate == XRCVFNACK) {
                  xmit_retry = 0L;
                  SharedCap = *(strchr(Rxbuf,'\0')+1);
                  if (!max_requests || (CurrentReqLim && CurrentReqLim > 0)) {
                     sprintf (Txbuf, "%s%04X%04X.REQ", filepath, config->alias[assumed].net, config->alias[assumed].node);
                     errno = 0;
                     reqfile = fopen(Txbuf,"wt");
                     j_error(msgtxt[M_OPEN_MSG],Txbuf);
                     fputs(Rxbuf,reqfile);
                     fputs("\n",reqfile);
                     fclose(reqfile);
                     unlink(ReqTmp);
                     ReqRecorded = 0;  /* counted by record_reqfile */
                     req_sent = respond_to_file_requests(req_sent);
                     CurrentReqLim -= ReqRecorded;
                     if ((sending_req = get_reqname(TRUE)) != 0)
                        xstate = XSENDFNAME;
                     else
                        xstate = XSENDFREQNAK;
                  } else
                     xstate = XSENDFREQNAK;
               }
               break;

            /*---------------------------------------------------------------*/
            /* Our last file request didn't match anything; move on to next  */
            /*---------------------------------------------------------------*/
            case FREQNAKPKT:
               attempting_req = FALSE;
               req_started = TRUE;
               sendpkt(NULL,0,FRNAKACKPKT);
               break;

            /*---------------------------------------------------------------*/
            /* ACK to no matching files for request error; try to end again  */
            /*---------------------------------------------------------------*/
            case FRNAKACKPKT:
               if (xstate == XRCVFRNAKACK) {
                  xmit_retry = 0L;
                  getfname(GOOD_XFER);
                  xstate = XSENDFNAME;
               }
               break;

            /*---------------------------------------------------------------*/
            /* ACK to last data block in file                                */
            /*---------------------------------------------------------------*/
            case EOFACKPKT:
               if (xstate == XRCVEOFACK || xstate == XRCVFNACK) {
                  xmit_retry = 0L;
                  if (xstate == XRCVEOFACK) {
                     Txlen -= txstpos;
                     through(&Txlen,&Txsttime);
                     tot_sent += Txlen;
                     sysinfo.today.bytesent += Txlen;
                     sysinfo.week.bytesent += Txlen;
                     sysinfo.month.bytesent += Txlen;
                     sysinfo.year.bytesent += Txlen;
                     sysinfo.today.filesent++;
                     sysinfo.week.filesent++;
                     sysinfo.month.filesent++;
                     sysinfo.year.filesent++;
                     nsent++;
                     update_filesio (nsent, nreceived);
                     j_status("%s-J%s %s",msgtxt[M_FILE_SENT],(SharedCap&CANCRC32)?"/32":" ",Txfname);
//                     prints (8, 65, YELLOW|_BLACK, "              ");
//                     prints (9, 65, YELLOW|_BLACK, "              ");
                     wgotoxy (Tx_y, 0);
                     wclreol();

                     if (sending_req) {
                        sysinfo.today.filerequest++;
                        sysinfo.week.filerequest++;
                        sysinfo.month.filerequest++;
                        sysinfo.year.filerequest++;
                        if (!(sending_req = get_reqname(FALSE)))
                           getfname(GOOD_XFER);
                     } else
                        getfname(GOOD_XFER);
                  }
                  xstate = XSENDFNAME;
               }
               break;

            /*---------------------------------------------------------------*/
            /* Receiver says "let's try that again."                         */
            /*---------------------------------------------------------------*/
            case RPOSPKT:
               if (xstate == XSENDBLK || xstate == XRCVEOFACK) {
                  if (*((long *)(Rxbuf+sizeof(txpos))) != last_rpostime) {
                     last_rpostime = *((long *)(Rxbuf+sizeof(txpos)));
                     xmit_retry = 0L;
                     CLEAR_OUTBOUND();
                     errno = 0;
                     txpos = lasttx = *((long *)Rxbuf);
                     lseek(Txfile, lasttx, SEEK_SET);
                     if (j_error(msgtxt[M_SEEK_MSG],Txfname))
                        goto giveup;
                     status_line(msgtxt[M_SYNCHRONIZING],txpos);
                     txblklen >>= 2;
                     if (txblklen < 64)
                        txblklen = 64;
                     goodbytes = 0;
                     goodneeded += 1024;
                     if (goodneeded > 8192)
                        goodneeded = 8192;
                     xstate = XSENDBLK;
                  }
               }
               break;

            /*---------------------------------------------------------------*/
            /* Debris from end of previous Janus session; ignore it          */
            /*---------------------------------------------------------------*/
            case HALTACKPKT:
               break;

            /*---------------------------------------------------------------*/
            /* Abort the transfer and quit                                   */
            /*---------------------------------------------------------------*/
            default:
               j_status(msgtxt[M_UNKNOWN_PACKET],pkttype);
               /* fallthrough */
            case HALTPKT:
giveup:        j_status(msgtxt[M_SESSION_ABORT]);
               if (Txfname[0])
                  getfname(ABORT_XFER);
               if (rstate == RRCVBLK) {
                  TotalBytes += (Rxpos-rxstpos);
                  rxclose(FAILED_XFER);
               }
               goto abortxfer;

         }                                    /* switch (pkttype)  */
      }                                       /* while (pkttype)  */
   } while (xstate || rstate);

   /*------------------------------------------------------------------------*/
   /* All done; make sure other end is also finished (one way or another)    */
   /*------------------------------------------------------------------------*/
breakout:
   if (!jfsent && !emsi)
      j_status(msgtxt[M_NOTHING_TO_SEND], remote_zone,remote_net,remote_node,remote_point);
abortxfer:
   through(&TotalBytes,&starttime);
   endbatch();
   close_janus_filetransfer();
   unlink(ReqTmp);

   /*------------------------------------------------------------------------*/
   /* Release allocated memory                                               */
   /*------------------------------------------------------------------------*/
freemem:
   if (Txfname)
      free(Txfname);
   if (Rxfname)
      free(Rxfname);
//   if (!emsi)
//      flag_file (CLEAR_FLAG, called_zone, called_net, called_node, 1);
//   set_prior (4);                               /* Always High                                     */
}



/*****************************************************************************/
/* Get name and info for next file to be transmitted, if any, and build      */
/* FNAMEPKT.  Packet contents as per ZModem filename info packet, to allow   */
/* use of same method of aborted-transfer recovery.  If there are no more    */
/* files to be sent, build FNAMEPKT with null filename.  Also open file and  */
/* set up for transmission.  Set Txfname, Txfile, Txlen.  Txbuf must not be  */
/* modified until FNACKPKT is received.                                      */
/*****************************************************************************/
static void getfname(word xfer_flag) {
   static byte point4d, floflag, bad_xfers, outboundname[PATHLEN];
   static long floname_pos;
   static FILE *flofile;
   static int have_lock, whichaka = 0;
   char *holdname;

   char *p;
   int i, v;
   long curr_pos;
   struct stat f;

   /*------------------------------------------------------------------------*/
   /* Initialize static variables on first call of the batch                 */
   /*------------------------------------------------------------------------*/
emsi_send:
   if (xfer_flag == INITIAL_XFER) {
      point4d = floflag = outboundname[0] = '\0';
      flofile = NULL;
      if (emsi) {
         if (whichaka != -1) {
            called_zone = remote_zone = addrs[whichaka].zone;
            called_net = remote_net = addrs[whichaka].net;
            called_node = remote_node = addrs[whichaka].node;
            remote_point = addrs[whichaka].point;

            if (addrs[whichaka].point) {
               for (v = 0; v < MAX_ALIAS && config->alias[v].net; v++) {
                  if (!config->alias[v].net)
                     break;
                  if (addrs[whichaka].zone == config->alias[v].zone && addrs[whichaka].net == config->alias[v].net && addrs[whichaka].node == config->alias[v].node && config->alias[v].fakenet)
                     break;
               }

               if (v < MAX_ALIAS && config->alias[v].net) {
                  called_node = addrs[whichaka].point;
                  called_net = config->alias[v].fakenet;
               }
               else
                  called_net = -1;
            }
         }
         else {
            remote_zone = called_zone;
            remote_net = called_net;
            remote_node = called_node;
            remote_point = 0;
         }

         if (get_bbs_record (called_zone, called_net, called_node, 0)) {
            if (nodelist.password[0] && strcmp(strupr(nodelist.password),strupr(remote_password))) {
               status_line(msgtxt[M_PWD_ERROR],remote_zone,remote_net,remote_node,remote_point,remote_password,nodelist.password);
               whichaka++;
               if (addrs[whichaka].net) {
                  xfer_flag = INITIAL_XFER;
                  jfsent = 0;
                  goto emsi_send;
               }
               else {
                  whichaka--;
                  goto end_send;
               }
            }
         }
         else if (remote_point) {
            if (get_bbs_record (remote_zone, remote_net, remote_node, remote_point)) {
               if (nodelist.password[0] && strcmp(strupr(nodelist.password),strupr(remote_password))) {
                  status_line(msgtxt[M_PWD_ERROR],remote_zone,remote_net,remote_node,remote_point,remote_password,nodelist.password);
                  whichaka++;
                  if (addrs[whichaka].net) {
                     xfer_flag = INITIAL_XFER;
                     jfsent = 0;
                     goto emsi_send;
                  }
                  else {
                     whichaka--;
                     goto end_send;
                  }
               }
            }
         }
      }

      if (emsi && whichaka != -1) {
         if (oksend[whichaka])
            have_lock = FALSE;
         else
            have_lock = TRUE;
      }

   /*------------------------------------------------------------------------*/
   /* If we were already sending a file, close it and clean up               */
   /*------------------------------------------------------------------------*/
   } else if (Txfile != -1) {
      errno = 0;
      close(Txfile);
      j_error(msgtxt[M_CLOSE_MSG],Txfname);
      Txfile = -1;
      /*---------------------------------------------------------------------*/
      /* If xfer completed, do post-xfer cleanup                             */
      /*---------------------------------------------------------------------*/
      if (xfer_flag == GOOD_XFER) {
         /*------------------------------------------------------------------*/
         /* Perform post-xfer file massaging if neccessary                   */
         /*------------------------------------------------------------------*/
         switch (Do_after) {
            case DELETE_AFTER:
            case '^':
               j_status(msgtxt[M_UNLINKING_MSG],Txfname);
               unlink(Txfname);
               j_error(msgtxt[M_UNLINK_MSG],Txfname);
               break;
            case TRUNC_AFTER:
               j_status(msgtxt[M_TRUNC_MSG],Txfname);
               unlink (Txfname);
               Txfile = open (Txfname, O_CREAT|O_TRUNC|O_RDWR, S_IREAD|S_IWRITE);
               if (Txfile != -1) {
                  errno = 0;
                  j_error(msgtxt[M_TRUNC_MSG],Txfname);
                  close(Txfile);
               }
               Txfile = -1;
         }
         /*------------------------------------------------------------------*/
         /* If processing .?LO file, flag filename as sent (name[0] = '~')   */
         /*------------------------------------------------------------------*/
skipname:
         if (floflag) {
            curr_pos = ftell(flofile);
            j_error(msgtxt[M_SEEK_MSG],outboundname);
            fseek(flofile,floname_pos,SEEK_SET);
            j_error(msgtxt[M_SEEK_MSG],outboundname);
            fputc(Txfname[0] = '~',flofile);
            j_error(msgtxt[M_WRITE_MSG],outboundname);
            fseek(flofile,curr_pos,SEEK_SET);
            j_error(msgtxt[M_SEEK_MSG],outboundname);
         }
      } else {
abort:
         ++bad_xfers;
      }
   }

   /*------------------------------------------------------------------------*/
   /* Find next file to be sent and build FNAMEPKT.  If reading .FLO-type    */
   /* file get next entry from it; otherwise check for next .OUT/.FLO file   */
   /*------------------------------------------------------------------------*/
   if (have_lock)
      goto end_send;

   holdname = HoldAreaNameMunge (called_zone);
   if (!floflag) {
      /*---------------------------------------------------------------------*/
      /* If first getfname() for this batch, init filename to .OUT           */
      /*---------------------------------------------------------------------*/
      if (!outboundname[0]) {
         sprintf(outboundname,"%s%04x%04x.OUT",holdname,called_net,called_node);
         *ext_flags = 'O';
      /*---------------------------------------------------------------------*/
      /* Increment outbound filename until match found or all checked        */
      /* .OUT->.DUT->.CUT->.HUT->.FLO->.DLO->.CLO->.HLO->null name           */
      /*---------------------------------------------------------------------*/
      } else {
nxtout:
         p = strchr(outboundname,'\0') - 3;
         for (i = 0; i < NUM_FLAGS; ++i)
            if (ext_flags[i] == *p)
               break;
         if (i < NUM_FLAGS - 1) {
            *p = ext_flags[i+1];
#ifndef JACK_DECKER
            if (isOriginator && *p == 'H')
               goto nxtout;
#endif
         } else if (!point4d && remote_point) {
            if (floflag) {
               sprintf (outboundname, "%s%04x%04x.PNT\\%08x.FLO", holdname, remote_net, remote_node, remote_point);
               *ext_flags = 'F';
            }
            else {
               sprintf (outboundname, "%s%04x%04x.PNT\\%08x.OUT", holdname, remote_net, remote_node, remote_point);
               *ext_flags = 'O';
            }
            ++point4d;
         } else {
            /*---------------------------------------------------------------*/
            /* Finished ?,D,C,H sequence; wrap .OUT->.FLO, or .FLO->done     */
            /*---------------------------------------------------------------*/
            if (!floflag) {
               if (point4d) {
                  sprintf(outboundname,"%s%04x%04x.OUT",holdname,called_net,called_node);
                  p = strchr(outboundname,'\0') - 3;
                  --point4d;
               }
               *p++ = *ext_flags = 'F';
               *p++ = 'L';
               *p = 'O';
               ++floflag;
            } else {
end_send:
               outboundname[0] = Txfname[0] = Txbuf[0] = Txbuf[1] = floflag = '\0';
               if (emsi) {
                  whichaka++;
                  if (addrs[whichaka].net) {
                     if (!jfsent) j_status(msgtxt[M_NOTHING_TO_SEND], remote_zone,remote_net,remote_node,remote_point);
                     xfer_flag = INITIAL_XFER;
                     jfsent = 0;
                     goto emsi_send;
                  }
               }
            }
         }
      }
      /*---------------------------------------------------------------------*/
      /* Check potential outbound name; if file doesn't exist keep looking   */
      /*---------------------------------------------------------------------*/
      if (outboundname[0]) {
         if (!dexists(outboundname))
            goto nxtout;
         if (floflag)
            goto rdflo;
         strcpy(Txfname,outboundname);
         /*------------------------------------------------------------------*/
         /* Start FNAMEPKT using .PKT alias                                  */
         /*------------------------------------------------------------------*/
         invent_pkt_name(Txbuf);
         Do_after = DELETE_AFTER;
      }
   /*------------------------------------------------------------------------*/
   /* Read and process next entry from .?LO-type file                        */
   /*------------------------------------------------------------------------*/
   } else {
rdflo:
      /*---------------------------------------------------------------------*/
      /* Open .?LO file for processing if neccessary                         */
      /*---------------------------------------------------------------------*/
      if (!flofile) {
         bad_xfers = 0;
         errno = 0;
#ifndef IBM_C
         _fmode = O_TEXT;        /* Gyrations are to avoid MSC 4.0 "r+t" bug */
         flofile = fopen(outboundname,"r+");
         _fmode = O_BINARY;
         setvbuf (flofile, NULL, _IONBF, 0); /* Added by Angelo Besani */
         if (j_error(msgtxt[M_OPEN_MSG],outboundname))
#else
         flofile = fopen(outboundname,"rt+");
         setvbuf (flofile, NULL, _IONBF, 0); /* else ftell fails if file ends with ^Z */
         if (j_error(msgtxt[M_OPEN_MSG],outboundname))
#endif
            goto nxtout;
      }
      errno = 0;
      floname_pos = ftell(flofile);
      j_error(msgtxt[M_SEEK_MSG],outboundname);
      if (fgets(p = Txfname, PATHLEN, flofile)) {
         /*------------------------------------------------------------------*/
         /* Got an attached file name; check for handling flags, fix up name */
         /*------------------------------------------------------------------*/
         while (*p > ' ')
            ++p;
         *p = '\0';
         switch (Txfname[0]) {
            case '\0':
            case '~':
            case ';':
               goto rdflo;
            case TRUNC_AFTER:
            case DELETE_AFTER:
            case '^':
               if (dexists (Txfname+1)) {
                  Do_after = Txfname[0];
                  strcpy(Txfname,Txfname+1);
               }
               else
                  goto rdflo;
               break;
            default:
               if (dexists (Txfname))
                  Do_after = NOTHING_AFTER;
               else
                  goto rdflo;
               break;
         }
         /*------------------------------------------------------------------*/
         /* Start FNAMEPKT with simple filename                              */
         /*------------------------------------------------------------------*/
         while (p >= Txfname && *p != '\\' && *p != ':')
            --p;
         strcpy(Txbuf,++p);
      } else {
         /*------------------------------------------------------------------*/
         /* Finished reading this .?LO file; clean up and look for another   */
         /*------------------------------------------------------------------*/
         errno = 0;
         fclose(flofile);
         j_error(msgtxt[M_CLOSE_MSG],outboundname);
         flofile = NULL;
         if (!bad_xfers) {
            unlink(outboundname);
            j_error(msgtxt[M_UNLINK_MSG],outboundname);
         }
         goto nxtout;
      }
   }

   /*------------------------------------------------------------------------*/
   /* If we managed to find a valid file to transmit, open it, finish        */
   /* FNAMEPKT, and print nice message for the sysop.                        */
   /*------------------------------------------------------------------------*/
   if (Txfname[0]) {
      if (xfer_flag == ABORT_XFER)
         goto abort;
      j_status(msgtxt[M_SENDING],Txfname);
      errno = 0;
      Txfile = shopen(Txfname,O_RDONLY|O_BINARY);
      if (Txfile != -1)
         errno = 0;
      if (j_error(msgtxt[M_OPEN_MSG],Txfname))
         goto skipname;
      if (isatty (Txfile))           /* Angelo B added 5 Oct 90 */
       {
        close (Txfile);
        errno = 1;
        j_error (msgtxt[M_DEVICE_MSG],Txfname);
        goto skipname;
       }
      stat(Txfname,&f);
      sprintf(strchr(Txbuf,'\0')+1,"%lu %lo %o",Txlen = f.st_size,f.st_mtime,f.st_mode);

      p = strchr(Txfname,'\0');
      while (p >= Txfname && *p != ':' && *p != '\\')
         --p;
      xfer_summary(msgtxt[M_SEND],++p,&Txlen,Tx_y);

      time(&Txsttime);
   }
}


/*****************************************************************************/
/* Build and send a packet of any type.                                      */
/* Packet structure is: PKTSTRT,contents,packet_type,PKTEND,crc              */
/* CRC is computed from contents and packet_type only; if PKTSTRT or PKTEND  */
/* get munged we'll never even find the CRC.                                 */
/*****************************************************************************/
static void sendpkt(byte *buf,int len,int type) {
   unsigned short crc;

   if ((SharedCap & CANCRC32) && type != FNAMEPKT)
      sendpkt32(buf,len,type);
   else {
      BUFFER_BYTE(DLE);
      BUFFER_BYTE(PKTSTRTCHR ^ 0x40);

      crc = 0;
      while (--len >= 0) {
         txbyte(*buf);
         crc = xcrc(crc, ((byte)(*buf++)) );
      }

      BUFFER_BYTE((byte)type);
      crc = xcrc(crc,(byte)type);

      BUFFER_BYTE(DLE);
      BUFFER_BYTE(PKTENDCHR ^ 0x40);

      txbyte((byte)(crc >> 8));
      txbyte((byte)(crc & 0xFF));

      UNBUFFER_BYTES();
   }
}


/*****************************************************************************/
/* Build and send a packet using 32-bit CRC; same as sendpkt in other ways   */
/*****************************************************************************/
static void sendpkt32(byte *buf,int len,int type) {
   unsigned long crc32;

   BUFFER_BYTE(DLE);
   BUFFER_BYTE(PKTSTRTCHR32 ^ 0x40);

   crc32 = 0xFFFFFFFFL;
   while (--len >= 0) {
      txbyte(*buf);
      crc32 = Z_32UpdateCRC(((byte)*buf),crc32);
      ++buf;
   }

   BUFFER_BYTE((byte)type);
   crc32 = Z_32UpdateCRC((byte)type,crc32);

   BUFFER_BYTE(DLE);
   BUFFER_BYTE(PKTENDCHR ^ 0x40);

   txbyte((byte)(crc32 >> 24));
   txbyte((byte)((crc32 >> 16) & 0xFF));
   txbyte((byte)((crc32 >> 8) & 0xFF));
   txbyte((byte)(crc32 & 0xFF));

   UNBUFFER_BYTES();
}



/*****************************************************************************/
/* Transmit cooked escaped byte(s) corresponding to raw input byte.  Escape  */
/* DLE, XON, and XOFF using DLE prefix byte and ^ 0x40. Also escape          */
/* CR-after-'@' to avoid Telenet/PC-Pursuit problems.                        */
/*****************************************************************************/
static void txbyte(byte c) {
   static byte lastsent;

   switch (c) {
      case CR:
         if (lastsent != '@')
            goto sendit;
         /* fallthrough */
      case DLE:
      case XON:
      case XOFF:
         BUFFER_BYTE(DLE);
         c ^= 0x40;
         /* fallthrough */
      default:
sendit:  BUFFER_BYTE(lastsent = c);
   }
}


/*****************************************************************************/
/* Process FNAMEPKT of file to be received.  Check for aborted-transfer      */
/* recovery and solve filename collisions.    Check for enough disk space.   */
/* Return initial file data position to start receiving at, or -1 if error   */
/* detected to abort file reception.  Set Rxfname, Rxlen, Rxfile.            */
/*****************************************************************************/
static long procfname(void) {
   byte *p;
   char linebuf[128], *fileinfo, *badfname;
   long filestart, bytes;
   FILE *abortlog;
   struct stat f;
   int i;

   /*------------------------------------------------------------------------*/
   /* Initialize for file reception                                          */
   /*------------------------------------------------------------------------*/
   Rxfname[0] = Resume_WaZOO = 0;

   /*------------------------------------------------------------------------*/
   /* Save info on WaZOO transfer in case of abort                           */
   /*------------------------------------------------------------------------*/
   strcpy(Resume_name,fancy_str(Rxbuf));
   fileinfo = strchr(Rxbuf,'\0') + 1;
   p = strchr(fileinfo,'\0') + 1;
   SharedCap = (Rxblklen > p-Rxbuf) ? *p & OURCAP : 0;

   /*------------------------------------------------------------------------*/
   /* If this is a null FNAMEPKT, return OK immediately                      */
   /*------------------------------------------------------------------------*/
   if (!Rxbuf[0])
      return 0L;

   strcpy(linebuf,Rxbuf);
   strlwr(linebuf);
   if (is_mailpkt (linebuf, strlen(linebuf) - 1) )
      p = "MailPKT";
   else if (is_arcmail (linebuf, strlen(linebuf) - 1) )
      p = "ARCMail";
   else
      p = NULL;
   j_status("#%s %s %s",msgtxt[M_RECEIVING],(p) ? p : " ",Rxbuf);

   /*------------------------------------------------------------------------*/
   /* Extract and validate filesize                                          */
   /*------------------------------------------------------------------------*/
   Rxlen = -1;
   Rxfiletime = 0;
   if (sscanf(fileinfo,"%ld %lo",&Rxlen,&Rxfiletime) < 1 || Rxlen < 0) {
      j_status(msgtxt[M_NO_LENGTH]);
      return -1L;
   }
   sprintf(Resume_info,"%ld %lo",Rxlen,Rxfiletime);

   /*------------------------------------------------------------------------*/
   /* Check if this is a failed WaZOO transfer which should be resumed       */
   /*------------------------------------------------------------------------*/
   if (dexists(Abortlog_name)) {
      errno = 0;
      abortlog = fopen(Abortlog_name,"rt");
      if (!j_error(msgtxt[M_OPEN_MSG],Abortlog_name)) {
         while (!feof(abortlog)) {
            linebuf[0] = '\0';
            if (!fgets(p = linebuf,sizeof(linebuf),abortlog))
               break;
            while (*p >= ' ')
               ++p;
            *p = '\0';
            p = strchr(linebuf,' ');
            *p = '\0';
            if (!stricmp(linebuf,Resume_name)) {
               p = strchr( (badfname = ++p), ' ');
               *p = '\0';
               if (!stricmp(++p,Resume_info)) {
                  ++Resume_WaZOO;
                  break;
               }
            }
         }
         errno = 0;
         fclose(abortlog);
         j_error(msgtxt[M_CLOSE_MSG],Abortlog_name);
      }
   }

   /*------------------------------------------------------------------------*/
   /* Open either the old or a new file, as appropriate                      */
   /*------------------------------------------------------------------------*/
   p = strchr(strcpy(Rxfname,filepath),'\0');
   errno = 0;
   if (Resume_WaZOO) {
      strcpy (p, badfname);
      Rxfile = sh_open (Rxfname, SH_DENYRW, O_CREAT|O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
   } else {
      strcpy (p, Rxbuf);
      /*---------------------------------------------------------------------*/
      /* If the file already exists:                                         */
      /* 1) And the new file has the same time and size, skip it             */
      /* 2) And OVERWRITE is turned on, delete the old copy                  */
      /* 3) Else create a unique file name in which to store new data        */
      /*---------------------------------------------------------------------*/
      if (dexists(Rxfname)) {
         stat(Rxfname,&f);
         if (Rxlen == f.st_size && Rxfiletime == f.st_mtime) {
            j_status(msgtxt[M_ALREADY_HAVE],Rxfname);
            return -1L;
         }
         i = strlen(Rxfname) - 1;
         if ((!overwrite) || (is_arcmail(Rxfname,i))) {
            unique_name(Rxfname);
            j_status(msgtxt[M_RENAME_MSG],Rxfname);
         } else {
            unlink(Rxfname);
            j_error(msgtxt[M_UNLINK_MSG],Rxfname);
         }
      }
      Rxfile = sh_open (Rxfname, SH_DENYRW, O_CREAT|O_EXCL|O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
   }
   if (Rxfile != -1)
      errno = 0;
   if (j_error(msgtxt[M_OPEN_MSG],Rxfname))
      return -1L;

   /*------------------------------------------------------------------------*/
   /* Determine initial file data position                                   */
   /*------------------------------------------------------------------------*/
   if (Resume_WaZOO) {
      stat(Rxfname,&f);
      j_status(msgtxt[M_SYNCHRONIZING_OFFSET],filestart = f.st_size);
      p = Rxbuf;
      errno = 0;
      lseek(Rxfile,filestart,SEEK_SET);
      if (j_error(msgtxt[M_SEEK_MSG],Rxfname)) {
         close(Rxfile);
         return -1L;
      }
   } else
      filestart = 0L;

   /*------------------------------------------------------------------------*/
   /* Check for enough disk space                                            */
   /*------------------------------------------------------------------------*/
   bytes = Rxlen - filestart + 10240;
   if (bytes > Diskavail) {
      j_status(msgtxt[M_OUT_OF_DISK_SPACE]);
      if (filelength (Rxfile) == 0L) {
         close (Rxfile);
         unlink (Rxfname);
      }
      else
         close (Rxfile);
      return -1L;
   }

   /*------------------------------------------------------------------------*/
   /* Print status message for the sysop                                     */
   /*------------------------------------------------------------------------*/
   xfer_summary(msgtxt[M_RECV],p,&Rxlen,Rx_y);

   time(&Rxsttime);

   return filestart;
}


/*****************************************************************************/
/* Receive, validate, and extract a packet if available.  If a complete      */
/* packet hasn't been received yet, receive and store as much of the next    */
/* packet as possible.    Each call to rcvpkt() will continue accumulating a */
/* packet until a complete packet has been received or an error is detected. */
/* Rxbuf must not be modified between calls to rcvpkt() if NOPKT is returned.*/
/* Returns type of packet received, NOPKT, or BADPKT.  Sets Rxblklen.        */
/*****************************************************************************/
static byte rcvpkt() {
   static byte rxcrc32;
   static unsigned short crc;
   static unsigned long crc32;
   byte *p;
   short i, c;
   unsigned long pktcrc;

   /*------------------------------------------------------------------------*/
   /* Abort transfer if operator pressed ESC                                 */
   /*------------------------------------------------------------------------*/
   if (local_kbd == 0x1B) {
      local_kbd = -1;
      j_status(GenericError,msgtxt[M_KBD_MSG]);
      return HALTPKT;
   }

   /*------------------------------------------------------------------------*/
   /* If not accumulating packet yet, find start of next packet              */
   /*------------------------------------------------------------------------*/
   WaitFlag = FALSE;
   p = Rxbufptr;
   if (!p) {
      do {
         c = rxbyte ();
      } while (c >= 0 || c == PKTEND);

      switch (c) {
         case PKTSTRT:
            rxcrc32 = FALSE;
            p = (byte *) Rxbuf;
            crc = 0;
            break;
         case PKTSTRT32:
            rxcrc32 = TRUE;
            p = (byte *) Rxbuf;
            crc32 = 0xFFFFFFFFL;
            break;
         case NOCARRIER:
            j_status(GenericError,&(msgtxt[M_NO_CARRIER][1]));
            return HALTPKT;
         default:
            return NOPKT;
      }
   }

   /*------------------------------------------------------------------------*/
   /* Accumulate packet data until we empty buffer or find packet delimiter  */
   /*------------------------------------------------------------------------*/
   if (rxcrc32) {
      while ((c = rxbyte()) >= 0 && p < Rxbufmax) {
         *p++ = (byte)c;
         crc32 = Z_32UpdateCRC((byte)c,crc32);
      }
   } else {
      while ((c = rxbyte()) >= 0 && p < Rxbufmax) {
         *p++ = (byte)c;
         crc = xcrc(crc,(byte)c);
      }
   }

   /*------------------------------------------------------------------------*/
   /* Handle whichever end-of-packet condition occurred                      */
   /*------------------------------------------------------------------------*/
   switch (c) {
      /*---------------------------------------------------------------------*/
      /* PKTEND found; verify valid CRC                                      */
      /*---------------------------------------------------------------------*/
      case PKTEND:
         WaitFlag = TRUE;
         pktcrc = 0;
         for (i = (rxcrc32) ? 4 : 2; i; --i) {
            if ((c = rxbyte()) < 0)
               break;
            pktcrc = (pktcrc << 8) | c;
         }
         if (!i) {
            if ((rxcrc32 && pktcrc == crc32) || pktcrc == crc) {
               /*------------------------------------------------------------*/
               /* Good packet verified; compute packet data length and       */
               /* return packet type                                         */
               /*------------------------------------------------------------*/
               Rxbufptr = NULL;
               Rxblklen = (word) (--p - (byte *) Rxbuf);
               return *p;
            }
         }
         /* fallthrough */

      /*---------------------------------------------------------------------*/
      /* Bad CRC, carrier lost, or buffer overflow from munged PKTEND        */
      /*---------------------------------------------------------------------*/
      default:
         if (c == NOCARRIER) {
            j_status(GenericError,&(msgtxt[M_NO_CARRIER][1]));
            return HALTPKT;
         } else {
            Rxbufptr = NULL;
            return BADPKT;
         }

      /*---------------------------------------------------------------------*/
      /* Emptied buffer; save partial packet and let sender do something     */
      /*---------------------------------------------------------------------*/
      case BUFEMPTY:
         Rxbufptr = p;
         return NOPKT;

      /*---------------------------------------------------------------------*/
      /* PKTEND was trashed; discard partial packet and prep for next one    */
      /*---------------------------------------------------------------------*/
      case PKTSTRT:
         rxcrc32 = FALSE;
         Rxbufptr = Rxbuf;
         crc = 0;
         return BADPKT;

      case PKTSTRT32:
         rxcrc32 = TRUE;
         Rxbufptr = Rxbuf;
         crc32 = 0xFFFFFFFFL;
         return BADPKT;
   }
}



/*****************************************************************************/
/* Close file being received and perform post-reception aborted-transfer     */
/* recovery cleanup if neccessary.                                           */
/*****************************************************************************/
static void rxclose(word xfer_flag) {
   byte *p;
   byte namebuf[PATHLEN], linebuf[128], c;
   FILE *abortlog, *newlog;
   struct utimbuf utimes;

   /*------------------------------------------------------------------------*/
   /* Close file we've been receiving                                        */
   /*------------------------------------------------------------------------*/
   errno = 0;
   close(Rxfile);
   j_error(msgtxt[M_CLOSE_MSG],Rxfname);
   if (Rxfiletime) {
      utimes.actime = Rxfiletime;
      utimes.modtime = Rxfiletime;
      utime(Rxfname,&utimes);
   }

   /*------------------------------------------------------------------------*/
   /* If we completed a previously-aborted transfer, kill log entry & rename */
   /*------------------------------------------------------------------------*/
   if (xfer_flag == GOOD_XFER && Resume_WaZOO) {
      abortlog = fopen(Abortlog_name,"rt");
      if (!j_error(msgtxt[M_OPEN_MSG],Abortlog_name)) {
         c = 0;
         strcpy(strchr(strcpy(namebuf,Abortlog_name),'\0')-1,"TMP");
         newlog = fopen(namebuf,"wt");
         if (!j_error(msgtxt[M_OPEN_MSG],namebuf)) {
            while (!feof(abortlog)) {
               linebuf[0] = '\0';
               if (!fgets(p = linebuf,sizeof(linebuf),abortlog))
                  break;
               while (*p > ' ')
                  ++p;
               *p = '\0';
               if (stricmp(linebuf,Resume_name)) {
                  *p = ' ';
                  fputs(linebuf,newlog);
                  if (j_error(msgtxt[M_WRITE_MSG],namebuf))
                     break;
                  ++c;
               }
            }
            errno = 0;
            fclose(abortlog);
            j_error(msgtxt[M_CLOSE_MSG],Abortlog_name);
            fclose(newlog);
            j_error(msgtxt[M_CLOSE_MSG],namebuf);
            unlink(Abortlog_name);
            j_error(msgtxt[M_UNLINK_MSG],Abortlog_name);
            if (c) {
               if (!rename(namebuf,Abortlog_name))
                  errno = 0;
               j_error(msgtxt[M_RENAME_MSG],namebuf);
            } else {
               unlink(namebuf);
               j_error(msgtxt[M_UNLINK_MSG],namebuf);
            }
         } else {
            fclose(abortlog);
            j_error(msgtxt[M_CLOSE_MSG],Abortlog_name);
         }
      }
      j_status(msgtxt[M_FINISHED_PART],Resume_name);
      unique_name(strcat(strcpy(namebuf,filepath),Resume_name));
      if (!rename(Rxfname,namebuf)) {
         errno = 0;
         strcpy(Rxfname,namebuf);
      } else
         j_error(msgtxt[M_RENAME_MSG],Rxfname);
   /*------------------------------------------------------------------------*/
   /* If transfer failed and was not an attempted resumption, log for later  */
   /*------------------------------------------------------------------------*/
   } else if (xfer_flag == FAILED_XFER && !Resume_WaZOO) {
      j_status(msgtxt[M_SAVING_PART],Rxfname);
      unique_name(strcat(strcpy(namebuf,filepath),"BadWaZOO.001"));
      if (!rename(Rxfname,namebuf))
         errno = 0;
      j_error(msgtxt[M_RENAME_MSG],Rxfname);

      abortlog = fopen(Abortlog_name,"at");
      if (!j_error(msgtxt[M_OPEN_MSG],Abortlog_name)) {
         fprintf(abortlog,"%s %s %s\n",Resume_name,namebuf+strlen(filepath),Resume_info);
         j_error(msgtxt[M_WRITE_MSG],Abortlog_name);
         fclose(abortlog);
         j_error(msgtxt[M_CLOSE_MSG],Abortlog_name);
      } else {
         unlink(namebuf);
         j_error(msgtxt[M_UNLINK_MSG],namebuf);
      }
   }
}


/*****************************************************************************/
/* Try REAL HARD to disengage batch session cleanly                          */
/*****************************************************************************/
static void endbatch(void) {
   int done, timeouts;
   long timeval, brain_dead;

   /*------------------------------------------------------------------------*/
   /* Tell the other end to halt if it hasn't already                        */
   /*------------------------------------------------------------------------*/
   done = timeouts = 0;
   long_set_timer(&brain_dead,120);
   sendpkt (NULL, 0, HALTPKT);
   long_set_timer (&timeval, TimeoutSecs);

   /*------------------------------------------------------------------------*/
   /* Wait for the other end to acknowledge that it's halting                */
   /*------------------------------------------------------------------------*/
   while (!done) {
      if (long_time_gone(&brain_dead))
         break;

      switch (rcvpkt()) {
         case NOPKT:
         case BADPKT:
            if (long_time_gone(&timeval)) {
               if (++timeouts > 2)
                  ++done;
               else
                  goto reject;
            }
            break;

         case HALTPKT:
         case HALTACKPKT:
            ++done;
            break;

         default:
            timeouts = 0;
reject:     sendpkt(NULL,0,HALTPKT);
            long_set_timer(&timeval,TimeoutSecs);
            break;
      }
   }

   /*------------------------------------------------------------------------*/
   /* Announce quite insistently that we're done now                         */
   /*------------------------------------------------------------------------*/
   for (done = 0; done < 10; ++done)
      sendpkt(NULL,0,HALTACKPKT);

   FLUSH_OUTPUT();
}


/*****************************************************************************/
/* Print a message in the message field of a transfer status line            */
/*****************************************************************************/
static void j_message(word pos,char *va_alist,...)
{
   va_list arg_ptr;
   int l;
   char buf[128];

   va_start (arg_ptr, va_alist);
   wgotoxy (pos, MSG_X);

   (void) vsprintf (buf, va_alist, arg_ptr);

   for (l = 25 - strlen (buf); l > 0; --l)
      strcat (buf, " ");
   wputs (buf);
   va_end (arg_ptr);
}


/*****************************************************************************/
/* Print & log status message without messing up display                     */
/*****************************************************************************/
static void j_status(char *va_alist,...)
{
   va_list arg_ptr;

   char buf[128];
   va_start(arg_ptr, va_alist);
   (void) vsprintf(buf,va_alist,arg_ptr);

   status_line(buf);
   va_end(arg_ptr);
}


/*****************************************************************************/
/* Print & log error message without messing up display                      */
/*****************************************************************************/
static int j_error (byte *msg, byte *fname)
{
   return (0);
}


/*****************************************************************************/
/* Compute future timehack for later reference                               */
/*****************************************************************************/
static void long_set_timer(long *Buffer,word Duration)
{
   time (Buffer);
   *Buffer += (long)Duration;
}


/*****************************************************************************/
/* Return TRUE if timehack has been passed, FALSE if not                     */
/*****************************************************************************/
static int long_time_gone(long *TimePtr) {
   return (time(NULL) > *TimePtr);
}


/*****************************************************************************/
/* Receive cooked escaped byte translated to avoid various problems.         */
/* Returns raw byte, BUFEMPTY, PKTSTRT, PKTEND, or NOCARRIER.                */
/*****************************************************************************/
static int rxbyte(void) {
   int c, w;

   if ((c = rcvrawbyte()) == DLE) {
      w = WaitFlag++;
      if ((c = rcvrawbyte()) >= 0) {
         switch (c ^= 0x40) {
            case PKTSTRTCHR:
               c = PKTSTRT;
               break;
            case PKTSTRTCHR32:
               c = PKTSTRT32;
               break;
            case PKTENDCHR:
               c = PKTEND;
               break;
         }
      }
      WaitFlag = (byte)w;
   }
   return c;
}


/*****************************************************************************/
/* Receive raw non-escaped byte.  Returns byte, BUFEMPTY, or NOCARRIER.      */
/* If waitflag is true, will wait for a byte for Timeoutsecs; otherwise      */
/* will return BUFEMPTY if a byte isn't ready and waiting in inbound buffer. */
/*****************************************************************************/
static int rcvrawbyte(void)
{
   long timeval;

   if ((int) PEEKBYTE () >= 0)
      return MODEM_IN ();

   if (!CARRIER)
      return NOCARRIER;
   if (!WaitFlag)
      return BUFEMPTY;

   timeval = time (NULL) + TimeoutSecs;

   while ((int) PEEKBYTE () < 0) {
      if (!CARRIER)
         return NOCARRIER;
      if (time (NULL) > timeval)
         return BUFEMPTY;
      time_release ();
      release_timeslice ();
   }

   return MODEM_IN ();
}


/*****************************************************************************/
/* Display start-of-transfer summary info                                    */
/*****************************************************************************/
static void xfer_summary(char *xfertype,char *fname,long *len,int y) {
/*
   char buf[128];

   if (*len);

   if (xfertype);
   if (y == 0) {
      prints (8, 65, YELLOW|_BLACK, "              ");
      prints (9, 65, YELLOW|_BLACK, "              ");
      sprintf (buf, "%-12.12s", strupr (fname));
      prints (8, 65, YELLOW|_BLACK, buf);
      prints (9, 65, YELLOW|_BLACK, "0 (00%)");
   }
   else if (y == 1) {
      prints (10, 65, YELLOW|_BLACK, "              ");
      prints (11, 65, YELLOW|_BLACK, "              ");
      sprintf (buf, "%-12.12s", strupr (fname));
      prints (10, 65, YELLOW|_BLACK, buf);
      prints (11, 65, YELLOW|_BLACK, "0 (00%)");
   }

   time_release ();
*/
   char buf[128];

   wgotoxy(y,2);
   sprintf(buf,"%s %12.12s;        0/%8ldb,%4d min.                       ",
      xfertype, fname, *len, (int)((*len*10/rate*100/JANUS_EFFICIENCY+59)/60));
   wputs(buf);

   time_release ();
   release_timeslice ();
}


/*****************************************************************************/
/* Update any status line values which have changed                          */
/*****************************************************************************/
static void update_status(long *pos,long *oldpos,long left,int *oldeta,int y) {
/*
  char buf[16];
  int eta, i;

   if (y == 0) {
      if (*pos != *oldpos) {
         *oldpos = *pos;
         sprintf (buf, "%ld (%02.1f%%)", *pos, (float)((float)(*pos) * 100 / (*pos + left)));
         prints (9, 65, YELLOW|_BLACK, buf);
      }
      i = (int)(left * 10 / rate * 100 / JANUS_EFFICIENCY);
      eta = (i + 59) / 60;
      sprintf(buf,"%4d",*oldeta = eta);
   }
   else if (y == 1) {
      if (*pos != *oldpos) {
         *oldpos = *pos;
         sprintf (buf, "%ld (%02.1f%%)", *pos, (float)((float)(*pos) * 100 / (*pos + left)));
         prints (11, 65, YELLOW|_BLACK, buf);
      }
      i = (int)(left * 10 / rate * 100 / JANUS_EFFICIENCY);
      eta = (i + 59) / 60;
      sprintf(buf,"%4d",*oldeta = eta);
   }
*/
  char buf[16];
  int eta;

  if (*pos != *oldpos) {
     sprintf(buf,"%8ld",*oldpos = *pos);
     wgotoxy(y,POS_X);
     wputs(buf);
  }

  eta = (int)((left*10/rate*100/JANUS_EFFICIENCY+59)/60);
  if (eta != *oldeta) {
     sprintf(buf,"%4d",*oldeta = eta);
     wgotoxy(y,ETA_X);
     wputs(buf);
  }
}

/*****************************************************************************/
/* Compute and print throughput                                              */
/*****************************************************************************/
static void through(long *bytes,long *started) {
   static byte *scrn = "+CPS: %u (%lu bytes)  Efficiency: %lu%%%%";
   unsigned long elapsed;
   word cps;

   elapsed = time(NULL) - *started;
   cps = (elapsed) ? (word)(*bytes/elapsed) : 0;
   j_status(scrn, cps, *bytes, cps*1000L/rate);
   TotalBytes += *bytes;
}


/*****************************************************************************/
/* Get next file to request, if any                                          */
/*****************************************************************************/
static int get_filereq(byte req_started) {
   byte reqname[PATHLEN], linebuf[128];
   byte *p;
   int gotone = FALSE, i;
   FILE *reqfile;

//   if (flag_file (TEST_AND_SET, called_zone, called_net, called_node, 1))
//      return FALSE;

   if (!(remote_capabilities & WZ_FREQ) || (cur_event > -1 && (e_ptrs[cur_event]->behavior & MAT_NOOUTREQ)) ) {
      status_line (msgtxt[M_FREQ_DECLINED]);
      return (FALSE);
   }

   if (emsi) {
      for (i = 0; i < MAX_EMSI_ADDR; i++) {
         if (addrs[i].net == 0)
            break;
         if (flag_file (TEST_AND_SET, addrs[i].zone, addrs[i].net, addrs[i].node, addrs[i].point, 0))
            continue;
         if (addrs[i].point)
            sprintf (reqname, "%s%04x%04x.PNT\\%08d.REQ", HoldAreaNameMunge (addrs[i].zone), addrs[i].net, addrs[i].node, addrs[i].point);
         else
            sprintf (reqname, request_template, HoldAreaNameMunge (addrs[i].zone), addrs[i].net, addrs[i].node);
         if (dexists(reqname))
            break;
      }

      if (i >= MAX_EMSI_ADDR || !addrs[i].net)
         return (FALSE);
   }
   else
      sprintf (reqname, request_template, HoldAreaNameMunge (called_zone), called_net, called_node);

   if (req_started)
      mark_done(reqname);

   if (dexists(reqname)) {
      if (!(SharedCap & CANFREQ))
         j_status(msgtxt[M_REMOTE_CANT_FREQ]);
      else {
         errno = 0;
         reqfile = fopen(reqname,"rt");
         if (!j_error(msgtxt[M_OPEN_MSG],reqname)) {
            while (!feof(reqfile)) {
               linebuf[0] = '\0';
               if (!fgets(p = linebuf,sizeof(linebuf),reqfile))
                  break;
               while (*p >= ' ')
                  ++p;
               *p = '\0';
               if (linebuf[0] != ';') {
                  strcpy(Rxbuf,linebuf);
/* Angelo B */    j_status ("%s '%s'",msgtxt[M_MAKING_FREQ],Rxbuf);
                  *(strchr(Rxbuf,'\0')+1) = SharedCap;
                  gotone = TRUE;
                  break;
               }
            }
            fclose(reqfile);
            j_error(msgtxt[M_CLOSE_MSG],reqname);
            if (!gotone) {
               unlink(reqname);
               j_error(msgtxt[M_UNLINK_MSG],reqname);
            }
         }
      }
   }

   return gotone;
}


/*****************************************************************************/
/* Record names of files to send in response to file request; callback       */
/* routine for respond_to_file_requests()                                    */
/*****************************************************************************/
int record_reqfile(char *fname) {
   FILE *tmpfile;

   errno = 0;
   tmpfile = fopen(ReqTmp,"at");
   if (!j_error(msgtxt[M_OPEN_MSG],ReqTmp)) {
      fputs(fname,tmpfile);
      j_error(msgtxt[M_WRITE_MSG],ReqTmp);
      fputs("\n",tmpfile);
      j_error(msgtxt[M_WRITE_MSG],ReqTmp);
      fclose(tmpfile);
      j_error(msgtxt[M_CLOSE_MSG],ReqTmp);
      ++ReqRecorded;
      return TRUE;
   }
   return FALSE;
}


/*****************************************************************************/
/* Get next file which was requested, if any                                 */
/*****************************************************************************/
static byte get_reqname(byte first_req) {
   byte *p;
   byte gotone = FALSE;
   FILE *tmpfile;
   struct stat f;

   if (!first_req) {
      errno = 0;
      close(Txfile);
      j_error(msgtxt[M_CLOSE_MSG],Txfname);
      Txfile = -1;
      mark_done(ReqTmp);
   }

   if (dexists(ReqTmp)) {
      errno = 0;
      tmpfile = fopen(ReqTmp,"rt");
      if (!j_error(msgtxt[M_OPEN_MSG],ReqTmp)) {
         while (!feof(tmpfile)) {
            Txfname[0] = '\0';
            if (!fgets(p = Txfname,PATHLEN,tmpfile))
               break;
            while (*p >= ' ')
               ++p;
            *p = '\0';
            if (Txfname[0] != ';') {
               j_status(msgtxt[M_SENDING],Txfname);
               errno = 0;
               Txfile = shopen(Txfname,O_RDONLY|O_BINARY);
               if (Txfile != -1)
                  errno = 0;
               if (j_error(msgtxt[M_OPEN_MSG],Txfname))
                  continue;
               while (p >= Txfname && *p != '\\' && *p != ':')
                  --p;
               strcpy(Txbuf,++p);
               stat(Txfname,&f);
               sprintf(strchr(Txbuf,'\0')+1,"%lu %lo %o",Txlen = f.st_size,f.st_mtime,f.st_mode);
               xfer_summary(msgtxt[M_SEND],p,&Txlen,Tx_y);
               time(&Txsttime);
               gotone = TRUE;
               break;
            }
         }
         fclose(tmpfile);
         j_error(msgtxt[M_CLOSE_MSG],ReqTmp);
         if (!gotone) {
            unlink(ReqTmp);
            j_error(msgtxt[M_UNLINK_MSG],ReqTmp);
         }
      }
   }

   return gotone;
}


/*****************************************************************************/
/* Mark first unmarked line of file as done (comment it out)                 */
/*****************************************************************************/
static void mark_done(char *fname) {
   byte linebuf[128];
   FILE *fh;
   long pos;

   if (dexists(fname)) {
      errno = 0;
      _fmode = O_TEXT;
      fh = fopen(fname,"r+");
      _fmode = O_BINARY;
      if (!j_error(msgtxt[M_OPEN_MSG],fname)) {
         while (!feof(fh)) {
            pos = ftell(fh);
            j_error(msgtxt[M_SEEK_MSG],fname);
            if (!fgets(linebuf,sizeof(linebuf),fh))
               break;
            if (linebuf[0] != ';') {
               fseek(fh,pos,SEEK_SET);
               j_error(msgtxt[M_SEEK_MSG],fname);
               fputc(';',fh);
               j_error(msgtxt[M_WRITE_MSG],fname);
               break;
            }
         }
         fclose(fh);
         j_error(msgtxt[M_CLOSE_MSG],fname);
      }
   }
}

static void open_janus_filetransfer()
{
//        janus_handle = wopen(16,0,19,79,1,WHITE|_RED,WHITE|_RED);
//        wactiv(janus_handle);
//        wtitle(" Janus Transfer Status ", TLEFT, WHITE|_RED);
}

static void close_janus_filetransfer()
{
//        if (janus_handle != -1) {
//                wactiv(janus_handle);
//                wclose();
//                janus_handle = -1;
//        }
}

