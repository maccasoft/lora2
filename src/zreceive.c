#include <stdio.h>
#include <io.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlwin.h>

#include "zmodem.h"
#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"
#include "tc_utime.h"


/*--------------------------------------------------------------------------*/
/* Local routines                                                           */
static int RZ_ReceiveData (byte *, int);
static int RZ_32ReceiveData (byte *, int);
static int RZ_InitReceiver (void);
static int RZ_ReceiveBatch (FILE *);
static int RZ_ReceiveFile (FILE *);
static int RZ_GetHeader (void);
static int RZ_SaveToDisk (long *);
static void RZ_AckBibi (void);

/*--------------------------------------------------------------------------*/
/* Private declarations                                                     */
/*--------------------------------------------------------------------------*/
static long DiskAvail;
static long filetime;
static byte realname[64];
static int zreceive_handle;

/*--------------------------------------------------------------------------*/
/* Private data                                                             */
/*--------------------------------------------------------------------------*/

/* Parameters for ZSINIT frame */
#define ZATTNLEN 32

static char Attn[ZATTNLEN + 1];                  /* String rx sends to tx on
                                                  * err            */
static FILE *Outfile;                            /* Handle of file being
                                                  * received           */
static int Tryzhdrtype;                          /* Hdr type to send for Last
                                                  * rx close      */
static char isBinary;                            /* Current file is binary
                                                  * mode             */
static char EOFseen;                             /* indicates cpm eof (^Z)
                                                  * was received     */
static char Zconv;                               /* ZMODEM file conversion
                                                  * request          */
static int RxCount;                              /* Count of data bytes
                                                  * received            */
static byte Upload_path[PATHLEN];                /* Dest. path of file being
                                                  * received   */
static long Filestart;                           /* File offset we started
                                                  * this xfer from   */
extern long filelength ();                       /* returns length of file
                                                  * ref'd in stream  */

/*--------------------------------------------------------------------------*/
/* GET ZMODEM                                                               */
/* Receive a batch of files.                                                */
/* returns TRUE (1) for good xfer, FALSE (0) for bad                        */
/* can be called from f_upload or to get mail from a WaZOO Opus             */
/*--------------------------------------------------------------------------*/
int get_Zmodem (rcvpath, xferinfo)
char *rcvpath;
FILE *xferinfo;
{
   byte namebuf[PATHLEN];
   int i;
   byte *p;
   char *HoldName;
   long t, timerset ();

#ifdef DEBUG
   show_debug_name ("get_Zmodem");
#endif

   filetime = 0;
   zreceive_handle = -1;

   _BRK_DISABLE ();
   IN_XON_ENABLE ();

/*   Secbuf = NULL;*/
   Outfile = NULL;
   z_size = 0;


   Rxtimeout = 100;
   Tryzhdrtype = ZRINIT;

   strcpy (namebuf, rcvpath);
   Filename = namebuf;

   strcpy (Upload_path, rcvpath);
   p = Upload_path + strlen (Upload_path) - 1;
   while (p >= Upload_path && *p != '\\')
      --p;
   *(++p) = '\0';

   HoldName = hold_area;

   sprintf (Abortlog_name, "%s%04x%04x.Z\0",
            HoldName, remote_net, remote_node);

   DiskAvail = zfree (Upload_path);

   if (((i = RZ_InitReceiver ()) == ZCOMPL) ||
       ((i == ZFILE) && ((RZ_ReceiveBatch (xferinfo)) == OK)))
      {
      XON_DISABLE ();
      XON_ENABLE ();                             /* Make sure xmitter is
                                                  * unstuck */
      return 1;
      }

   CLEAR_OUTBOUND ();
   XON_DISABLE ();                               /* Make sure xmitter is
                                                  * unstuck */
   send_can ();                                  /* transmit at least 10 cans    */
   t = timerset (200);                           /* wait no more than 2
                                                  * seconds  */
   while (!timeup (t) && !OUT_EMPTY () && CARRIER)
      time_release ();                           /* Give up slice while */
                                                  /* waiting  */
   XON_ENABLE ();                                /* Turn XON/XOFF back on...     */
/*
   if (Secbuf)
      free (Secbuf);
*/
   if (Outfile)
      fclose (Outfile);

   return 0;
}                                                /* get_Zmodem */

/*--------------------------------------------------------------------------*/
/* RZ RECEIVE DATA                                                          */
/* Receive array buf of max length with ending ZDLE sequence                */
/* and CRC.  Returns the ending character or error code.                    */
/*--------------------------------------------------------------------------*/
static int RZ_ReceiveData (buf, length)
register byte *buf;
register int length;
{
   register int c;
   register word crc;
   char *endpos;
   int d;


#ifdef DEBUG
   show_debug_name ("RZ_ReceiveData");
#endif

   if (Rxframeind == ZBIN32)
      return RZ_32ReceiveData (buf, length);

   crc = RxCount = 0;
   buf[0] = buf[1] = 0;
   endpos = buf + length;

   while (buf <= endpos)
      {
      if ((c = Z_GetZDL ()) & ~0xFF)
         {
   CRCfoo:
         switch (c)
            {
            case GOTCRCE:
            case GOTCRCG:
            case GOTCRCQ:
            case GOTCRCW:
               /*-----------------------------------*/
               /* C R C s                           */
               /*-----------------------------------*/
               crc = Z_UpdateCRC (((d = c) & 0xFF), crc);
               if ((c = Z_GetZDL ()) & ~0xFF)
                  goto CRCfoo;

               crc = Z_UpdateCRC (c, crc);
               if ((c = Z_GetZDL ()) & ~0xFF)
                  goto CRCfoo;

               crc = Z_UpdateCRC (c, crc);
               if (crc & 0xFFFF)
                  {
                  z_message (msgtxt[M_CRC_MSG]);
                  return ERROR;
                  }

               RxCount = length - (int) (endpos - buf);
               return d;

            case GOTCAN:
               /*-----------------------------------*/
               /* Cancel                            */
               /*-----------------------------------*/
               status_line (msgtxt[M_CAN_MSG]);
               return ZCAN;

            case TIMEOUT:
               /*-----------------------------------*/
               /* Timeout                           */
               /*-----------------------------------*/
               z_message (msgtxt[M_TIMEOUT]);
               return c;

            case RCDO:
               /*-----------------------------------*/
               /* No carrier                        */
               /*-----------------------------------*/
               status_line (msgtxt[M_NO_CARRIER]);
               CLEAR_INBOUND ();
               return c;

            default:
               /*-----------------------------------*/
               /* Something bizarre                 */
               /*-----------------------------------*/
               z_message (msgtxt[M_DEBRIS]);
               CLEAR_INBOUND ();
               return c;
            }                                    /* switch */
         }                                       /* if */

      *buf++ = (unsigned char) c;
      crc = Z_UpdateCRC (c, crc);
      }                                          /* while(1) */

   z_message (msgtxt[M_LONG_PACKET]);
   return ERROR;
}                                                /* RZ_ReceiveData */

/*--------------------------------------------------------------------------*/
/* RZ RECEIVE DATA with 32 bit CRC                                          */
/* Receive array buf of max length with ending ZDLE sequence                */
/* and CRC.  Returns the ending character or error code.                    */
/*--------------------------------------------------------------------------*/
static int RZ_32ReceiveData (buf, length)
register byte *buf;
register int length;
{
   register int c;
   unsigned long crc;
   char *endpos;
   int d;


#ifdef DEBUG
   show_debug_name ("RZ_32ReceiveData");
#endif

   crc = 0xFFFFFFFFL;
   RxCount = 0;
   buf[0] = buf[1] = 0;
   endpos = buf + length;

   while (buf <= endpos)
      {
      if ((c = Z_GetZDL ()) & ~0xFF)
         {
   CRCfoo:
         switch (c)
            {
            case GOTCRCE:
            case GOTCRCG:
            case GOTCRCQ:
            case GOTCRCW:
               /*-----------------------------------*/
               /* C R C s                           */
               /*-----------------------------------*/
               d = c;
               c &= 0377;
               crc = Z_32UpdateCRC (c, crc);
               if ((c = Z_GetZDL ()) & ~0xFF)
                  goto CRCfoo;

               crc = Z_32UpdateCRC (c, crc);
               if ((c = Z_GetZDL ()) & ~0xFF)
                  goto CRCfoo;

               crc = Z_32UpdateCRC (c, crc);
               if ((c = Z_GetZDL ()) & ~0xFF)
                  goto CRCfoo;

               crc = Z_32UpdateCRC (c, crc);
               if ((c = Z_GetZDL ()) & ~0xFF)
                  goto CRCfoo;

               crc = Z_32UpdateCRC (c, crc);
               if (crc != 0xDEBB20E3L)
                  {
                  z_message (msgtxt[M_CRC_MSG]);
                  return ERROR;
                  }

               RxCount = length - (int) (endpos - buf);
               return d;

            case GOTCAN:
               /*-----------------------------------*/
               /* Cancel                            */
               /*-----------------------------------*/
               status_line (msgtxt[M_CAN_MSG]);
               return ZCAN;

            case TIMEOUT:
               /*-----------------------------------*/
               /* Timeout                           */
               /*-----------------------------------*/
               z_message (msgtxt[M_TIMEOUT]);
               return c;

            case RCDO:
               /*-----------------------------------*/
               /* No carrier                        */
               /*-----------------------------------*/
               status_line (msgtxt[M_NO_CARRIER]);
               CLEAR_INBOUND ();
               return c;

            default:
               /*-----------------------------------*/
               /* Something bizarre                 */
               /*-----------------------------------*/
               z_message (msgtxt[M_DEBRIS]);
               CLEAR_INBOUND ();
               return c;
            }                                    /* switch */
         }                                       /* if */

      *buf++ = (unsigned char) c;
      crc = Z_32UpdateCRC (c, crc);
      }                                          /* while(1) */

   z_message (msgtxt[M_LONG_PACKET]);
   return ERROR;
}                                                /* RZ_ReceiveData */

/*--------------------------------------------------------------------------*/
/* RZ INIT RECEIVER                                                         */
/* Initialize for Zmodem receive attempt, try to activate Zmodem sender     */
/* Handles ZSINIT, ZFREECNT, and ZCOMMAND frames                            */
/*                                                                          */
/* Return codes:                                                            */
/*    ZFILE .... Zmodem filename received                                   */
/*    ZCOMPL ... transaction finished                                       */
/*    ERROR .... any other condition                                        */
/*--------------------------------------------------------------------------*/
static int RZ_InitReceiver ()
{
   register int n;
   int errors = 0;
   char *sptr = NULL;


#ifdef DEBUG
   show_debug_name ("RZ_InitReceiver");
#endif

   for (n = 12; --n >= 0;)
      {
      /*--------------------------------------------------------------*/
      /* Set buffer length (0=unlimited, don't wait).                 */
      /* Also set capability flags                                    */
      /*--------------------------------------------------------------*/
      Z_PutLongIntoHeader (0L);
      Txhdr[ZF0] = CANFC32 | CANFDX | CANOVIO;
      Z_SendHexHeader (Tryzhdrtype, Txhdr);
      if (Tryzhdrtype == ZSKIP)
         Tryzhdrtype = ZRINIT;

AGAIN:

      switch (Z_GetHeader (Rxhdr))
         {
         case ZFILE:
            Zconv = Rxhdr[ZF0];
            Tryzhdrtype = ZRINIT;
            if (RZ_ReceiveData (Secbuf, WAZOOMAX) == GOTCRCW)
               return ZFILE;
            Z_SendHexHeader (ZNAK, Txhdr);
            if (--n < 0)
               {
               sptr = "ZFILE";
               goto Err;
               }
            goto AGAIN;

         case ZSINIT:
            if (RZ_ReceiveData (Attn, ZATTNLEN) == GOTCRCW)
               {
               Z_PutLongIntoHeader (1L);
               Z_SendHexHeader (ZACK, Txhdr);
               }
            else Z_SendHexHeader (ZNAK, Txhdr);
            if (--n < 0)
               {
               sptr = "ZSINIT";
               goto Err;
               }
            goto AGAIN;

         case ZFREECNT:
            Z_PutLongIntoHeader (DiskAvail);
            Z_SendHexHeader (ZACK, Txhdr);
            goto AGAIN;

         case ZCOMMAND:
            /*-----------------------------------------*/
            /* Paranoia is good for you...             */
            /* Ignore command from remote, but lie and */
            /* say we did the command ok.              */
            /*-----------------------------------------*/
            if (RZ_ReceiveData (Secbuf, WAZOOMAX) == GOTCRCW)
               {
               status_line (msgtxt[M_Z_IGNORING], Secbuf);
               Z_PutLongIntoHeader (0L);
               do
                  {
                  Z_SendHexHeader (ZCOMPL, Txhdr);
                  }
               while (++errors < 10 && Z_GetHeader (Rxhdr) != ZFIN);
               RZ_AckBibi ();
               return ZCOMPL;
               }
            else Z_SendHexHeader (ZNAK, Txhdr);
            if (--n < 0)
               {
               sptr = "CMD";
               goto Err;
               }
            goto AGAIN;

         case ZCOMPL:
            if (--n < 0)
               {
               sptr = "COMPL";
               goto Err;
               }
            goto AGAIN;

         case ZFIN:
            RZ_AckBibi ();
            return ZCOMPL;

         case ZCAN:
            sptr = msgtxt[M_CAN_MSG];
            goto Err;

         case RCDO:
            sptr = &msgtxt[M_NO_CARRIER][1];
            CLEAR_INBOUND ();
            goto Err;
         }                                       /* switch */
      }                                          /* for */

   sptr = msgtxt[M_TIMEOUT];

Err:
   if (sptr != NULL)
      sprintf (e_input, msgtxt[M_Z_INITRECV], sptr);
   status_line (e_input);

   return ERROR;
}                                                /* RZ_InitReceiver */

/*--------------------------------------------------------------------------*/
/* RZFILES                                                                  */
/* Receive a batch of files using ZMODEM protocol                           */
/*--------------------------------------------------------------------------*/
static int RZ_ReceiveBatch (xferinfo)
FILE *xferinfo;
{
   register int c;
   byte namebuf[PATHLEN];


#ifdef DEBUG
   show_debug_name ("RZ_ReceiveBatch");
#endif

   while (1)
      {
      switch (c = RZ_ReceiveFile (xferinfo))
         {
         case ZEOF:
            if (Resume_WaZOO)
               {
                                        remove_abort (Abortlog_name, Resume_name);
               strcpy (namebuf, Upload_path);
               strcat (namebuf, Resume_name);
               unique_name (namebuf);
               rename (Filename, namebuf);
               }
            /* fallthrough */
         case ZSKIP:
            switch (RZ_InitReceiver ())
               {
               case ZCOMPL:
                  return OK;
               default:
                  return ERROR;
               case ZFILE:
                  break;
               }                                 /* switch */
            break;

         default:
            fclose (Outfile);
            Outfile = NULL;
            if (remote_capabilities)
               {
               if (!Resume_WaZOO)
                  {
                                                add_abort (Abortlog_name, Resume_name, Filename, Upload_path, Resume_info);
                  }
               }
            else unlink (Filename);
            return c;
         }                                       /* switch */
      }                                          /* while */
}                                                /* RZ_ReceiveBatch */

/*--------------------------------------------------------------------------*/
/* RZ RECEIVE FILE                                                          */
/* Receive one file; assumes file name frame is preloaded in Secbuf         */
/*--------------------------------------------------------------------------*/
static int RZ_ReceiveFile (xferinfo)
FILE *xferinfo;
{
   register int c;
   int n;
   long rxbytes;
   char *sptr = NULL;
   struct utimbuf utimes;
   char j[50];


#ifdef DEBUG
   show_debug_name ("RZ_ReceiveFile");
#endif

   EOFseen = FALSE;
   c = RZ_GetHeader ();
   if (c == ERROR || c == ZSKIP)
      return (Tryzhdrtype = ZSKIP);

   n = 10;
   rxbytes = Filestart;

   while (1)
      {
      Z_PutLongIntoHeader (rxbytes);
      Z_SendHexHeader (ZRPOS, Txhdr);
NxtHdr:
      switch (c = Z_GetHeader (Rxhdr))
         {
         case ZDATA:
            /*-----------------------------------------*/
            /* Data Packet                             */
            /*-----------------------------------------*/
            if (Rxpos != rxbytes)
               {
               if (--n < 0)
                  {
                  sptr = msgtxt[M_FUBAR_MSG];
                  goto Err;
                  }
               sprintf (j, "%s; %ld/%ld", msgtxt[M_BAD_POS], rxbytes, Rxpos);
               z_message (j);
               Z_PutString (Attn);
               continue;
               }
      MoreData:
            switch (c = RZ_ReceiveData (Secbuf, WAZOOMAX))
               {
               case ZCAN:
                  sptr = msgtxt[M_CAN_MSG];
                  goto Err;

               case RCDO:
                  sptr = &msgtxt[M_NO_CARRIER][1];
                  CLEAR_INBOUND ();
                  goto Err;

               case ERROR:
                  /*-----------------------*/
                  /* CRC error             */
                  /*-----------------------*/
                  if (--n < 0)
                     {
                     sptr = msgtxt[M_FUBAR_MSG];
                     goto Err;
                     }
                  show_loc (rxbytes, n);
                  Z_PutString (Attn);
                  continue;

               case TIMEOUT:
                  if (--n < 0)
                     {
                     sptr = msgtxt[M_TIMEOUT];
                     goto Err;
                     }
                  show_loc (rxbytes, n);
                  continue;

               case GOTCRCW:
                  /*---------------------*/
                  /* End of frame          */
                  /*-----------------------*/
                  n = 10;
                  if (RZ_SaveToDisk (&rxbytes) == ERROR)
                     return ERROR;
                  Z_PutLongIntoHeader (rxbytes);
                  Z_SendHexHeader (ZACK, Txhdr);
                  goto NxtHdr;

               case GOTCRCQ:
                  /*---------------------*/
                  /* Zack expected         */
                  /*-----------------------*/
                  n = 10;
                  if (RZ_SaveToDisk (&rxbytes) == ERROR)
                     return ERROR;
                  Z_PutLongIntoHeader (rxbytes);
                  Z_SendHexHeader (ZACK, Txhdr);
                  goto MoreData;

               case GOTCRCG:
                  /*---------------------*/
                  /* Non-stop              */
                  /*-----------------------*/
                  n = 10;
                  if (RZ_SaveToDisk (&rxbytes) == ERROR)
                     return ERROR;
                  goto MoreData;

               case GOTCRCE:
                  /*---------------------*/
                  /* Header to follow      */
                  /*-----------------------*/
                  n = 10;
                  if (RZ_SaveToDisk (&rxbytes) == ERROR)
                     return ERROR;
                  goto NxtHdr;
               }                                 /* switch */

         case ZNAK:
         case TIMEOUT:
            /*-----------------------------------------*/
            /* Packet was probably garbled             */
            /*-----------------------------------------*/
            if (--n < 0)
               {
               sptr = msgtxt[M_JUNK_BLOCK];
               goto Err;
               }
            show_loc (rxbytes, n);
            continue;

         case ZFILE:
            /*-----------------------------------------*/
            /* Sender didn't see our ZRPOS yet         */
            /*-----------------------------------------*/
            RZ_ReceiveData (Secbuf, WAZOOMAX);
            continue;

         case ZEOF:
            /*-----------------------------------------*/
            /* End of the file                         */
            /* Ignore EOF if it's at wrong place; force */
            /* a timeout because the eof might have    */
            /* gone out before we sent our ZRPOS       */
            /*-----------------------------------------*/
            if (Rxpos != rxbytes)
               goto NxtHdr;

            throughput (2, rxbytes - Filestart);

            fclose (Outfile);

            status_line ("%s-Z%s %s", msgtxt[M_FILE_RECEIVED], Crc32 ? "/32" : "", realname);

            if (filetime)
               {
               utimes.actime = filetime;
               utimes.modtime = filetime;
               utime (Filename, &utimes);
               }

            Outfile = NULL;
            if (xferinfo != NULL)
               {
               fprintf (xferinfo, "%s\n", Filename);
               }

            if (zreceive_handle != -1)
            {
               wactiv(zreceive_handle);
               wclose();
               wunlink(zreceive_handle);
               zreceive_handle = -1;
            }

            return c;

         case ERROR:
            /*-----------------------------------------*/
            /* Too much garbage in header search error */
            /*-----------------------------------------*/
            if (--n < 0)
               {
               sptr = msgtxt[M_JUNK_BLOCK];
               goto Err;
               }
            show_loc (rxbytes, n);
            Z_PutString (Attn);
            continue;

         case ZSKIP:
            return c;

         default:
            sptr = "???";
            CLEAR_INBOUND ();
            goto Err;
         }                                       /* switch */
      }                                          /* while */

Err:
   if (sptr != NULL)
   {
      sprintf (e_input, msgtxt[M_Z_RZ], sptr);
      status_line (e_input);
   }
   return ERROR;
}                                                /* RZ_ReceiveFile */

/*--------------------------------------------------------------------------*/
/* RZ GET HEADER                                                            */
/* Process incoming file information header                                 */
/*--------------------------------------------------------------------------*/
static int RZ_GetHeader ()
{
   register byte *p;
   struct stat f;
   int i;
   byte *ourname;
   byte *theirname;
   unsigned long filesize;
   byte *fileinfo;
   char j[80];


#ifdef DEBUG
   show_debug_name ("RZ_GetHeader");
#endif

   /*--------------------------------------------------------------------*/
   /* Setup the transfer mode                                            */
   /*--------------------------------------------------------------------*/
   isBinary = (char) ((!RXBINARY && Zconv == ZCNL) ? 0 : 1);
   Resume_WaZOO = 0;

   /*--------------------------------------------------------------------*/
   /* Extract and verify filesize, if given.                             */
   /* Reject file if not at least 10K free                               */
   /*--------------------------------------------------------------------*/
   filesize = 0L;
   filetime = 0L;
   fileinfo = Secbuf + 1 + strlen (Secbuf);
   if (*fileinfo)
      sscanf (fileinfo, "%ld %lo", &filesize, &filetime);
   if (filesize + 10240 > DiskAvail)
      {
      status_line (msgtxt[M_OUT_OF_DISK_SPACE]);
      return ERROR;
      }

   /*--------------------------------------------------------------------*/
   /* Get and/or fix filename for uploaded file                          */
   /*--------------------------------------------------------------------*/
   p = Filename + strlen (Filename) - 1;         /* Find end of upload path */
   while (p >= Filename && *p != '\\')
      p--;
   ourname = ++p;

   p = Secbuf + strlen (Secbuf) - 1;             /* Find transmitted simple
                                                  * filename */
   while (p >= Secbuf && *p != '\\' && *p != '/' && *p != ':')
      p--;
   theirname = ++p;

   strcpy (ourname, theirname);                  /* Start w/ our path & their
                                                  * name */
   strcpy (realname, Filename);

   /*--------------------------------------------------------------------*/
   /* Save info on WaZOO transfer in case of abort                       */
   /*--------------------------------------------------------------------*/
   if (remote_capabilities)
      {
      strcpy (Resume_name, theirname);
      sprintf (Resume_info, "%ld %lo", filesize, filetime);
      }

   /*--------------------------------------------------------------------*/
   /* Check if this is a failed WaZOO transfer which should be resumed   */
   /*--------------------------------------------------------------------*/
   if (remote_capabilities && dexists (Abortlog_name))
      {
                Resume_WaZOO = (byte) check_failed (Abortlog_name, theirname, Resume_info, ourname);
      }

   /*--------------------------------------------------------------------*/
   /* Open either the old or a new file, as appropriate                  */
   /*--------------------------------------------------------------------*/
   if (Resume_WaZOO)
      {
      if (dexists (Filename))
         p = "r+b";
      else p = "wb";
      }
   else
      {
      strcpy (ourname, theirname);
      /*--------------------------------------------------------------------*/
      /* If the file already exists:                                        */
      /* 1) And the new file has the same time and size, return ZSKIP    */
      /* 2) And OVERWRITE is turned on, delete the old copy              */
      /* 3) Else create a unique file name in which to store new data    */
      /*--------------------------------------------------------------------*/
      if (dexists (Filename))
         {                                       /* If file already exists...      */
         if ((Outfile = fopen (Filename, "rb")) == NULL)
            {
            return ERROR;
            }
         fstat (fileno (Outfile), &f);
         fclose (Outfile);
         if (filesize == f.st_size && filetime == f.st_mtime)
            {
            status_line (msgtxt[M_ALREADY_HAVE], Filename);
            return ZSKIP;
            }
         i = strlen (Filename) - 1;
         if ((!overwrite) || (is_arcmail (Filename, i)))
            {
            unique_name (Filename);
            }
         else
            {
            unlink (Filename);
            }
         }                                       /* if exist */

      if (strcmp (ourname, theirname))
         {
         status_line (msgtxt[M_RENAME_MSG], ourname);
         }
      p = "wb";
      }
   if ((Outfile = fopen (Filename, p)) == NULL)
      {
      return ERROR;
      }
   if (isatty (fileno (Outfile)))
      {
      fclose (Outfile);
      return (ERROR);
      }

   Filestart = (Resume_WaZOO) ? filelength (fileno (Outfile)) : 0L;
   if (Resume_WaZOO)
      status_line (msgtxt[M_SYNCHRONIZING_OFFSET], Filestart);
   fseek (Outfile, Filestart, SEEK_SET);

   if (remote_capabilities && is_arcmail (theirname, strlen(theirname) - 1) )
      p = "ARCMail";
   else
      p = NULL;

   sprintf (j, "%s %s; %s%ldb, %d min.",
            (p) ? p : msgtxt[M_RECEIVING],
            realname,
            (isBinary) ? "" : "ASCII ",
            filesize,
            (int) ((filesize - Filestart) * 10 / rate + 53) / 54);

   file_length = filesize;

   zreceive_handle = wopen(16,0,19,79,1,WHITE|_RED,WHITE|_RED);
   wactiv(zreceive_handle);
   wtitle(" ZModem Transfer Status ", TLEFT, WHITE|_RED);

   wgotoxy (0, 2);
   wputs (j);
   status_line(" %s", j);
   i = (int) (filesize * 10 / rate + 53) / 54;
   wgotoxy (1, 69);
   sprintf(j, "%3d min", i);
   wputs (j);

   throughput (0, 0L);

   return OK;
}                                                /* RZ_GetHeader */

/*--------------------------------------------------------------------------*/
/* RZ SAVE TO DISK                                                          */
/* Writes the received file data to the output file.                        */
/* If in ASCII mode, stops writing at first ^Z, and converts all            */
/*   solo CR's or LF's to CR/LF pairs.                                      */
/*--------------------------------------------------------------------------*/
static int RZ_SaveToDisk (rxbytes)
long *rxbytes;
{
   static byte lastsent;

   register byte *p;
   register int count;
   int i;
   char j[100];

#ifdef DEBUG
   show_debug_name ("RZ_SaveToDisk");
#endif

   count = RxCount;

/*   if (got_ESC ())
      {
      send_can ();
      while ((i = Z_GetByte (20)) != TIMEOUT && i != RCDO)

         CLEAR_INBOUND ();
      send_can ();
      z_log (msgtxt[M_KBD_MSG]);
      return ERROR;
      } */

   if (count != z_size)
      {
      wgotoxy (1, 12);
      wputs (ultoa (((unsigned long) (z_size = count)), e_input, 10));
      wputs ("    ");
      }

   if (isBinary)
      {
      if (fwrite (Secbuf, 1, count, Outfile) != count)
         goto oops;
      }
   else
      {
      if (EOFseen)
         return OK;
      for (p = Secbuf; --count >= 0; ++p)
         {
         if (*p == CPMEOF)
            {
            EOFseen = TRUE;
            return OK;
            }
         if (*p == '\n')
            {
            if (lastsent != '\r' && putc ('\r', Outfile) == EOF)
               goto oops;
            }
         else
            {
            if (lastsent == '\r' && putc ('\n', Outfile) == EOF)
               goto oops;
            }
         if (putc ((lastsent = *p), Outfile) == EOF)
            goto oops;
         }
      }

   *rxbytes += RxCount;
   i = (int) ((file_length - *rxbytes)* 10 / rate + 53) / 54;
   sprintf (j, "%3d min", i);

   wgotoxy (1, 2);
   wputs (ultoa (((unsigned long) (*rxbytes)), e_input, 10));
   wgotoxy (1, 69);
   sprintf (j, "%3d min", i);
   wputs (j);

   return OK;

oops:
   return ERROR;
}

/*--------------------------------------------------------------------------*/
/* RZ ACK BIBI                                                              */
/* Ack a ZFIN packet, let byegones be byegones                              */
/*--------------------------------------------------------------------------*/
static void RZ_AckBibi ()
{
   register int n;


#ifdef DEBUG
   show_debug_name ("RZ_AckBiBi");
#endif

   Z_PutLongIntoHeader (0L);
   for (n = 4; --n;)
      {
      Z_SendHexHeader (ZFIN, Txhdr);
      switch (Z_GetByte (100))
         {
         case 'O':
            Z_GetByte (1);                       /* Discard 2nd 'O' */

         case TIMEOUT:
         case RCDO:
            return;
         }                                       /* switch */
      }                                          /* for */
}                                                /* RZ_AckBibi */



char *suffixes[8] = {
	"SU", "MO", "TU", "WE", "TH", "FR", "SA", NULL
};

int is_arcmail (p, n)
char *p;
int n;
{
   int i;
   char c[128];

   (void) strcpy (c, p);
   (void) strupr (c);

   for (i = n - 11; i < n - 3; i++) {
      if ((!isdigit (c[i])) && ((c[i] > 'F') || (c[i] < 'A')))
         return (0);
   }

   if ((c[n] == 'Q') && (c[n-1] == 'E') && (c[n-2] == 'R') && (c[n-3] == '.')) {
      made_request = 1;
      return (0);
   }

   for (i = 0; i < 7; i++) {
      if (strnicmp (&c[n - 2], suffixes[i], 2) == 0)
         break;
   }

   if (i >= 7) {
      if ((c[n] != 'T') || (c[n-1] != 'K') || (c[n-2] != 'P') || (c[n-3] != '.'))
         return (0);
   }

   got_arcmail = 1;
   return (1);
}

