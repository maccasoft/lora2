#include <stdio.h>
#include <io.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <stdlib.h>

#include <cxl\cxlwin.h>

#include "zmodem.h"
#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"


/*--------------------------------------------------------------------------*/
/* Private routines                                                         */
/*--------------------------------------------------------------------------*/
static void ZS_SendBinaryHeader (unsigned short, byte *);
static void ZS_32SendBinaryHeader (unsigned short, byte *);
static void ZS_SendData (byte *, int, unsigned short);
static void ZS_32SendData (byte *, int, unsigned short);
static void ZS_SendByte (byte);
static int ZS_GetReceiverInfo (void);
static int ZS_SendFile (int, int);
static int ZS_SendFileData (int);
static int ZS_SyncWithReceiver (int);
static void ZS_EndSend (void);

/*--------------------------------------------------------------------------*/
/* Private data                                                             */
/*--------------------------------------------------------------------------*/
static FILE *Infile;                             /* Handle of file being sent */
static long Strtpos;                             /* Starting byte position of */

 /* download                 */
static long LastZRpos;                           /* Last error location      */
static long ZRPosCount;                          /* ZRPOS repeat count       */
static long Txpos;                               /* Transmitted file position */
static int Rxbuflen;                             /* Receiver's max buffer    */

 /* length                   */
static int Rxflags;                              /* Receiver's flags         */
static int zsend_handle;



/*--------------------------------------------------------------------------*/
/* SEND ZMODEM (send a file)                                                */
/*   returns TRUE (1) for good xfer, FALSE (0) for bad                      */
/*   sends one file per call; 'fsent' flags start and end of batch          */
/*--------------------------------------------------------------------------*/
int send_Zmodem (fname, alias, fsent, wazoo)
char *fname;
char *alias;
int fsent;
int wazoo;
{
   register byte *p;
   register byte *q;
   struct stat f;
   int i;
   int rc;
   char j[100];

#ifdef DEBUG
   show_debug_name ("send_Zmodem");
#endif

   IN_XON_ENABLE ();

   z_size = 0;
   Infile = NULL;
   zsend_handle = -1;

   switch (fsent)
      {
      case 0:
         Z_PutString ("rz\r");
         Z_PutLongIntoHeader (0L);
         Z_SendHexHeader (ZRQINIT, Txhdr);
         /* Fall through */

      case NOTHING_TO_DO:
         Rxtimeout = 200;
         if (ZS_GetReceiverInfo () == ERROR)
            {
            XON_DISABLE ();
            XON_ENABLE ();

            return FALSE;
            }
      }

   Rxtimeout = (int) (614400L / (long) rate);

   if (Rxtimeout < 100)
      Rxtimeout = 100;

   if (fname == NULL)
      goto Done;

   /*--------------------------------------------------------------------*/
   /* Prepare the file for transmission.  Just ignore file open errors   */
   /* because there may be other files that can be sent.                 */
   /*--------------------------------------------------------------------*/
   Filename = fname;
   if ((Infile = fopen (Filename, "rb")) == NULL)
      {
      XON_DISABLE ();
      XON_ENABLE ();

      return OK;
      }
   if (isatty (fileno (Infile)))
      {
      errno = 1;
      fclose (Infile);
      XON_DISABLE ();
      XON_ENABLE ();

      return OK;
      }


   /*--------------------------------------------------------------------*/
   /* Send the file                                                      */
   /*--------------------------------------------------------------------*/
   rc = TRUE;

   /*--------------------------------------------------------------------*/
   /* Display outbound filename, size, and ETA for sysop                 */
   /*--------------------------------------------------------------------*/
   fstat (fileno (Infile), &f);

   zsend_handle = wopen(16,0,19,79,1,WHITE|_RED,WHITE|_RED);
   wactiv(zsend_handle);
   wtitle(" ZModem Transfer Status ", TLEFT, WHITE|_RED);

   i = (int) (f.st_size * 10 / rate + 53) / 54;
   sprintf (j, "Z-Send %s, %ldb, %d min.", Filename, f.st_size, i);
   file_length = f.st_size;

   wgotoxy (0, 2);
   wputs (j);
   status_line(" %s", j);
   wgotoxy (1, 69);
   sprintf (j, "%3d min", i);
   wputs (j);

   /*--------------------------------------------------------------------*/
   /* Get outgoing file name; no directory path, lower case              */
   /*--------------------------------------------------------------------*/
   for (p = (alias != NULL) ? alias : Filename, q = Txbuf; *p;)
      {
      if ((*p == '/') || (*p == '\\') || (*p == ':'))
         q = Txbuf;
      else *q++ = tolower (*p);

      p++;
      }

   *q++ = '\0';
   p = q;

   /*--------------------------------------------------------------------*/
   /* Zero out remainder of file header packet                           */
   /*--------------------------------------------------------------------*/
   while (q < (Txbuf + KSIZE))
      *q++ = '\0';

   /*--------------------------------------------------------------------*/
   /* Store filesize, time last modified, and file mode in header packet */
   /*--------------------------------------------------------------------*/
   sprintf (p, "%lu %lo %o", f.st_size, f.st_mtime, f.st_mode);

   /*--------------------------------------------------------------------*/
   /* Transmit the filename block and { the download                 */
   /*--------------------------------------------------------------------*/
   throughput (0, 0L);

   /*--------------------------------------------------------------------*/
   /* Check the results                                                  */
   /*--------------------------------------------------------------------*/
   switch (ZS_SendFile (1 + strlen (p) + (p - Txbuf), wazoo))
      {
      case ERROR:
         /*--------------------------------------------------*/
         /* Something tragic happened                        */
         /*--------------------------------------------------*/
         goto Err_Out;

      case OK:
         /*--------------------------------------------------*/
         /* File was sent                                    */
         /*--------------------------------------------------*/
         fclose (Infile);
         Infile = NULL;

         status_line ("%s-Z%s %s", msgtxt[M_FILE_SENT], Crc32t ? "/32" : "", Filename);
         goto Done;

      case ZSKIP:
         status_line (msgtxt[M_REMOTE_REFUSED], Filename);
         rc = SPEC_COND;                         /* Success but don't
                                                  * truncate! */
         goto Done;

      default:
         /*--------------------------------------------------*/
         /* Ignore the problem, get next file, trust other   */
         /* error handling mechanisms to deal with problems  */
         /*--------------------------------------------------*/
         goto Done;
      }                                          /* switch */

Err_Out:
   rc = FALSE;

Done:
   if (zsend_handle != -1)
   {
      wactiv(zsend_handle);
      wclose();
      wunlink(zsend_handle);
      zsend_handle = -1;
   }

   if (Infile)
      fclose (Infile);

   if (fsent < 0)
      ZS_EndSend ();

   XON_DISABLE ();
   XON_ENABLE ();

   return rc;
}                                                /* send_Zmodem */

/*--------------------------------------------------------------------------*/
/* ZS SEND BINARY HEADER                                                    */
/* Send ZMODEM binary header hdr of type type                               */
/*--------------------------------------------------------------------------*/
static void ZS_SendBinaryHeader (type, hdr)
unsigned short type;
register byte *hdr;
{
   register unsigned short crc;
   int n;

#ifdef DEBUG
   show_debug_name ("ZS_SendBinaryHeader");
#endif

   BUFFER_BYTE (ZPAD);
   BUFFER_BYTE (ZDLE);

   if ((Crc32t = Txfcs32) != 0)
      ZS_32SendBinaryHeader (type, hdr);
   else
      {
      BUFFER_BYTE (ZBIN);
      ZS_SendByte ((byte) type);

      crc = Z_UpdateCRC (type, 0);

      for (n = 4; --n >= 0;)
         {
         ZS_SendByte (*hdr);
         crc = Z_UpdateCRC (((unsigned short) (*hdr++)), crc);
         }
      ZS_SendByte ((byte) (crc >> 8));
      ZS_SendByte ((byte) crc);

      UNBUFFER_BYTES ();
      }

   if (type != ZDATA)
      {
      while (CARRIER && !OUT_EMPTY ())
         time_release ();
      if (!CARRIER)
         CLEAR_OUTBOUND ();
      }
}                                                /* ZS_SendBinaryHeader */

/*--------------------------------------------------------------------------*/
/* ZS SEND BINARY HEADER                                                    */
/* Send ZMODEM binary header hdr of type type                               */
/*--------------------------------------------------------------------------*/
static void ZS_32SendBinaryHeader (type, hdr)
unsigned short type;
register byte *hdr;
{
   unsigned long crc;
   int n;

#ifdef DEBUG
   show_debug_name ("ZS_32SendBinaryHeader");
#endif

   BUFFER_BYTE (ZBIN32);
   ZS_SendByte ((byte) type);

   crc = 0xFFFFFFFFL;
   crc = Z_32UpdateCRC (type, crc);

   for (n = 4; --n >= 0;)
      {
      ZS_SendByte (*hdr);
      crc = Z_32UpdateCRC (((unsigned short) (*hdr++)), crc);
      }

   crc = ~crc;
   for (n = 4; --n >= 0;)
      {
      ZS_SendByte ((byte) crc);
      crc >>= 8;
      }

   UNBUFFER_BYTES ();
}                                                /* ZS_SendBinaryHeader */

/*--------------------------------------------------------------------------*/
/* ZS SEND DATA                                                             */
/* Send binary array buf with ending ZDLE sequence frameend                 */
/*--------------------------------------------------------------------------*/
static void ZS_SendData (buf, length, frameend)
register byte *buf;
int length;
unsigned short frameend;
{
   register unsigned short crc;

#ifdef DEBUG
   show_debug_name ("ZS_SendData");
#endif

   if (Crc32t)
      ZS_32SendData (buf, length, frameend);
   else
      {
      crc = 0;
      for (; --length >= 0;)
         {
         ZS_SendByte (*buf);
         crc = Z_UpdateCRC (((unsigned short) (*buf++)), crc);
         }

      BUFFER_BYTE (ZDLE);
      BUFFER_BYTE ((unsigned char) frameend);
      crc = Z_UpdateCRC (frameend, crc);
      ZS_SendByte ((byte) (crc >> 8));
      ZS_SendByte ((byte) crc);

      UNBUFFER_BYTES ();

      }

   if (frameend == ZCRCW)
      {
      SENDBYTE (XON);
      while (CARRIER && !OUT_EMPTY ())
         time_release ();
      if (!CARRIER)
         CLEAR_OUTBOUND ();
      }
}                                                /* ZS_SendData */

/*--------------------------------------------------------------------------*/
/* ZS SEND DATA with 32 bit CRC                                             */
/* Send binary array buf with ending ZDLE sequence frameend                 */
/*--------------------------------------------------------------------------*/
static void ZS_32SendData (buf, length, frameend)
register byte *buf;
int length;
unsigned short frameend;
{
   unsigned long crc;

#ifdef DEBUG
   show_debug_name ("ZS_32SendData");
#endif

   crc = 0xFFFFFFFFL;
   for (; --length >= 0; ++buf)
      {
      ZS_SendByte (*buf);
      crc = Z_32UpdateCRC (((unsigned short) (*buf)), crc);
      }

   BUFFER_BYTE (ZDLE);
   BUFFER_BYTE ((unsigned char) frameend);
   crc = Z_32UpdateCRC (frameend, crc);

   crc = ~crc;

   for (length = 4; --length >= 0;)
      {
      ZS_SendByte ((byte) crc);
      crc >>= 8;
      }

   UNBUFFER_BYTES ();
}                                                /* ZS_SendData */

/*--------------------------------------------------------------------------*/
/* ZS SEND BYTE                                                             */
/* Send character c with ZMODEM escape sequence encoding.                   */
/* Escape XON, XOFF. Escape CR following @ (Telenet net escape)             */
/*--------------------------------------------------------------------------*/
static void ZS_SendByte (c)
register byte c;
{
   static byte lastsent;

   switch (c)
      {
      case 015:
      case 0215:
         if ((lastsent & 0x7F) != '@')
            goto SendIt;
      case 020:
      case 021:
      case 023:
      case 0220:
      case 0221:
      case 0223:
      case ZDLE:
         /*--------------------------------------------------*/
         /* Quoted characters                                */
         /*--------------------------------------------------*/
         BUFFER_BYTE (ZDLE);
         c ^= 0x40;

      default:
         /*--------------------------------------------------*/
         /* Normal character output                          */
         /*--------------------------------------------------*/
   SendIt:
         BUFFER_BYTE (lastsent = c);

      }                                          /* switch */
}                                                /* ZS_SendByte */

/*--------------------------------------------------------------------------*/
/* ZS GET RECEIVER INFO                                                     */
/* Get the receiver's init parameters                                       */
/*--------------------------------------------------------------------------*/
static int ZS_GetReceiverInfo ()
{
   int n;

#ifdef DEBUG
   show_debug_name ("ZS_GetReceiverInfo");
#endif

   for (n = 10; --n >= 0;)
      {
      switch (Z_GetHeader (Rxhdr))
         {
         case ZCHALLENGE:
            /*--------------------------------------*/
            /* Echo receiver's challenge number     */
            /*--------------------------------------*/
            Z_PutLongIntoHeader (Rxpos);
            Z_SendHexHeader (ZACK, Txhdr);
            continue;

         case ZCOMMAND:
            /*--------------------------------------*/
            /* They didn't see our ZRQINIT          */
            /*--------------------------------------*/
            Z_PutLongIntoHeader (0L);
            Z_SendHexHeader (ZRQINIT, Txhdr);
            continue;

         case ZRINIT:
            /*--------------------------------------*/
            /* */
            /*--------------------------------------*/
            Rxflags = 0377 & Rxhdr[ZF0];
            Rxbuflen = ((word) Rxhdr[ZP1] << 8) | Rxhdr[ZP0];
            Txfcs32 = Rxflags & CANFC32;
            return OK;

         case ZCAN:
         case RCDO:
         case TIMEOUT:
            return ERROR;

         case ZRQINIT:
            if (Rxhdr[ZF0] == ZCOMMAND)
               continue;

         default:
            Z_SendHexHeader (ZNAK, Txhdr);
            continue;
         }                                       /* switch */
      }                                          /* for */

   return ERROR;
}                                                /* ZS_GetReceiverInfo */

/*--------------------------------------------------------------------------*/
/* ZS SEND FILE                                                             */
/* Send ZFILE frame and begin sending ZDATA frame                           */
/*--------------------------------------------------------------------------*/
static int ZS_SendFile (blen, wazoo)
int blen;
int wazoo;
{
   register int c;
   long timerset ();

#ifdef DEBUG
   show_debug_name ("ZS_SendFile");
#endif

   while (1)
      {
/*      if (got_ESC ())
         {
         CLEAR_OUTBOUND ();
         XON_DISABLE ();

         send_can ();
         t = timerset (200);

         while (!timeup (t) && !OUT_EMPTY () && CARRIER)
            time_release ();

         XON_ENABLE ();
         status_line (msgtxt[M_KBD_MSG]);
         return ERROR;
         }
      else */
      if (!CARRIER)
         return ERROR;

      Txhdr[ZF0] = LZCONV;                       /* Default file conversion
                                                  * mode */
      Txhdr[ZF1] = LZMANAG;                      /* Default file management
                                                  * mode */
      Txhdr[ZF2] = LZTRANS;                      /* Default file transport
                                                  * mode */
      Txhdr[ZF3] = 0;
      ZS_SendBinaryHeader (ZFILE, Txhdr);
      ZS_SendData (Txbuf, blen, ZCRCW);

Again:
      switch (c = Z_GetHeader (Rxhdr))
         {
         case ZRINIT:
            while ((c = Z_GetByte (50)) > 0)
               if (c == ZPAD)
                  goto Again;

            /* Fall thru to */

         default:
            continue;

         case ZCAN:
         case RCDO:
         case TIMEOUT:
         case ZFIN:
         case ZABORT:
            return ERROR;

         case ZSKIP:
            /*-----------------------------------------*/
            /* Other system wants to skip this file    */
            /*-----------------------------------------*/
            return c;

         case ZRPOS:
            /*-----------------------------------------*/
            /* Resend from this position...            */
            /*-----------------------------------------*/
            fseek (Infile, Rxpos, SEEK_SET);
            if (Rxpos != 0L)
               {
               status_line (msgtxt[M_SYNCHRONIZING_OFFSET], Rxpos);
               CLEAR_OUTBOUND ();                /* Get rid of queued data */
               XON_DISABLE ();                   /* End XON/XOFF restraint */
               SENDBYTE (XON);                   /* Send XON to remote     */
               XON_ENABLE ();                    /* Start XON/XOFF again   */
               }
            LastZRpos = Strtpos = Txpos = Rxpos;
            ZRPosCount = 10;
            CLEAR_INBOUND ();
            return ZS_SendFileData (wazoo);
         }                                       /* switch */
      }                                          /* while */
}                                                /* ZS_SendFile */

/*--------------------------------------------------------------------------*/
/* ZS SEND FILE DATA                                                        */
/* Send the data in the file                                                */
/*--------------------------------------------------------------------------*/
static int ZS_SendFileData (wazoo)
int wazoo;
{
   register int c, e;
   int i;
   char j[100];
   word newcnt, blklen, maxblklen, goodblks, goodneeded = 1;

   long timerset ();

#ifdef DEBUG
   show_debug_name ("ZS_SendFileData");
#endif

   maxblklen = ((rate >= 0) && (rate < 300)) ? 128 : rate / 300 * 256;

   if (maxblklen > WAZOOMAX)
      maxblklen = WAZOOMAX;
   if (!wazoo && maxblklen > KSIZE)
      maxblklen = KSIZE;
   if (Rxbuflen && maxblklen > Rxbuflen)
      maxblklen = Rxbuflen;

   if (wazoo && (remote_capabilities & ZED_ZIPPER))
      maxblklen = KSIZE;

   blklen = maxblklen;

SomeMore:

   if (CHAR_AVAIL ())
      {
WaitAck:

      switch (c = ZS_SyncWithReceiver (1))
         {
         case ZSKIP:
            /*-----------------------------------------*/
            /* Skip this file                          */
            /*-----------------------------------------*/
            return c;

         case ZACK:
            break;

         case ZRPOS:
            /*-----------------------------------------*/
            /* Resume at this position                 */
            /*-----------------------------------------*/
            blklen = ((blklen >> 2) > 64) ? blklen >> 2 : 64;
            goodblks = 0;
            goodneeded = ((goodneeded << 1) > 16) ? 16 : goodneeded << 1;
            break;

         case ZRINIT:
            /*-----------------------------------------*/
            /* Receive init                            */
            /*-----------------------------------------*/
            throughput (1, Txpos - Strtpos);
            return OK;

         case TIMEOUT:
            /*-----------------------------------------*/
            /* Timed out on message from other side    */
            /*-----------------------------------------*/
            break;

         default:
            status_line (msgtxt[M_CAN_MSG]);
            fclose (Infile);
            return ERROR;
         }                                       /* switch */

      /*
       * Noise probably got us here. Odds of surviving are not good. But we
       * have to get unstuck in any event. 
       *
       */

      Z_UncorkTransmitter ();                    /* Get our side free if need
                                                  * be      */
      SENDBYTE (XON);                            /* Send an XON to release
                                                  * other side */

      while (CHAR_AVAIL ())
         {
         switch (MODEM_IN ())
            {
            case CAN:
            case RCDO:
            case ZPAD:
               goto WaitAck;
            }                                    /* switch */
         }                                       /* while */
      }                                          /* while */

   newcnt = Rxbuflen;
   Z_PutLongIntoHeader (Txpos);
   ZS_SendBinaryHeader (ZDATA, Txhdr);

   do
      {
/*      if (got_ESC ())
         {
         CLEAR_OUTBOUND ();
         XON_DISABLE ();

         send_can ();
         t = timerset (200);

         while (!timeup (t) && !OUT_EMPTY () && CARRIER)
            time_release ();

         XON_ENABLE ();
         status_line (msgtxt[M_KBD_MSG]);
         goto oops;
         }*/

      if (!CARRIER)
         goto oops;

      if ((c = fread (Txbuf, 1, blklen, Infile)) != z_size)
         {
         wgotoxy (1, 12);
         wputs (ultoa (((unsigned long) (z_size = c)), e_input, 10));
         wputs ("    ");
         }

      if (c < blklen)
         e = ZCRCE;
      else if (Rxbuflen && (newcnt -= c) <= 0)
         e = ZCRCW;
      else e = ZCRCG;

      ZS_SendData (Txbuf, c, e);

      Txpos += c;

      i = (int) ((file_length - Txpos) * 10 / rate + 53) / 54;
      sprintf (j, "%3d min", i);

      wgotoxy (1, 2);
      wputs (ultoa (((unsigned long) Txpos), e_input, 10));
      wputs ("  ");
      wgotoxy (1, 69);
      wputs (j);

      if (blklen < maxblklen && ++goodblks > goodneeded)
         {
         blklen = ((blklen << 1) < maxblklen) ? blklen << 1 : maxblklen;
         goodblks = 0;
         }

      if (e == ZCRCW)
         goto WaitAck;

      while (CHAR_AVAIL ())
         {
         switch (MODEM_IN ())
            {
            case CAN:
            case RCDO:
            case ZPAD:
               /*--------------------------------------*/
               /* Interruption detected;               */
               /* stop sending and process complaint   */
               /*--------------------------------------*/
               z_message (msgtxt[M_TROUBLE]);
               CLEAR_OUTBOUND ();
               ZS_SendData (Txbuf, 0, ZCRCE);
               goto WaitAck;
            }                                    /* switch */
         }                                       /* while */

      }                                          /* do */
   while (e == ZCRCG);

   while (1)
      {
      Z_PutLongIntoHeader (Txpos);
      ZS_SendBinaryHeader (ZEOF, Txhdr);

      switch (ZS_SyncWithReceiver (7))
         {
         case ZACK:
            continue;

         case ZRPOS:
            /*-----------------------------------------*/
            /* Resume at this position...              */
            /*-----------------------------------------*/
            goto SomeMore;

         case ZRINIT:
            /*-----------------------------------------*/
            /* Receive init                            */
            /*-----------------------------------------*/
            throughput (1, Txpos - Strtpos);
            return OK;

         case ZSKIP:
            /*-----------------------------------------*/
            /* Request to skip the current file        */
            /*-----------------------------------------*/
            status_line (msgtxt[M_SKIP_MSG]);
            fclose (Infile);
            return c;

         default:
      oops:
            status_line (msgtxt[M_CAN_MSG]);
            fclose (Infile);
            return ERROR;
         }                                       /* switch */
      }                                          /* while */
}                                                /* ZS_SendFileData */

/*--------------------------------------------------------------------------*/
/* ZS SYNC WITH RECEIVER                                                    */
/* Respond to receiver's complaint, get back in sync with receiver          */
/*--------------------------------------------------------------------------*/
static int ZS_SyncWithReceiver (num_errs)
int num_errs;
{
   register int c;
   char j[50];

#ifdef DEBUG
   show_debug_name ("ZS_SyncWithReceiver");
#endif

   while (1)
      {
      c = Z_GetHeader (Rxhdr);
      CLEAR_INBOUND ();
      switch (c)
         {
         case TIMEOUT:
            z_message (msgtxt[M_TIMEOUT]);
            if ((num_errs--) >= 0)
               break;

         case ZCAN:
         case ZABORT:
         case ZFIN:
         case RCDO:
            status_line (msgtxt[M_ERROR]);
            return ERROR;

         case ZRPOS:
            if (Rxpos == LastZRpos)              /* Same as last time?    */
               {
               if (!(--ZRPosCount))              /* Yup, 10 times yet?    */
                  return ERROR;                  /* Too many, get out     */
               }
            else ZRPosCount = 10;                /* Reset repeat count    */
            LastZRpos = Rxpos;                   /* Keep track of this    */

            rewind (Infile);                     /* In case file EOF seen */
            fseek (Infile, Rxpos, SEEK_SET);
            Txpos = Rxpos;
            sprintf (j, msgtxt[M_RESENDING_FROM],
                     ultoa (((unsigned long) (Txpos)), e_input, 10));
            z_message (j);
            return c;

         case ZSKIP:
            status_line (msgtxt[M_SKIP_MSG]);

         case ZRINIT:
            fclose (Infile);
            return c;

         case ZACK:
            z_message (NULL);
            return c;

         default:
            z_message ("Debris");
            ZS_SendBinaryHeader (ZNAK, Txhdr);
            continue;
         }                                       /* switch */
      }                                          /* while */
}                                                /* ZS_SyncWithReceiver */




/*--------------------------------------------------------------------------*/
/* ZS END SEND                                                              */
/* Say BIBI to the receiver, try to do it cleanly                           */
/*--------------------------------------------------------------------------*/
static void ZS_EndSend ()
{

#ifdef DEBUG
    show_debug_name ("ZS_EndSend");
#endif

   while (1)
      {
      Z_PutLongIntoHeader (0L);
      ZS_SendBinaryHeader (ZFIN, Txhdr);

      switch (Z_GetHeader (Rxhdr))
         {
         case ZFIN:
            SENDBYTE ('O');
            SENDBYTE ('O');
            while (CARRIER && !OUT_EMPTY ())
               time_release ();
            if (!CARRIER)
               CLEAR_OUTBOUND ();
            /* fallthrough... */
         case ZCAN:
         case RCDO:
         case TIMEOUT:
            return;
         }                                       /* switch */
      }                                          /* while */
}                                                /* ZS_EndSend */

