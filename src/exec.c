/*
   --- Version 3.1 91-08-17 23:06 ---

   EXEC.C: EXEC function with memory swap - Prepare parameters.

   Public domain software by

        Thomas Wagner
        Ferrari electronic GmbH
        Beusselstrasse 27
        D-1000 Berlin 21
        Germany
*/

#include "compat.h"
#include "exec.h"
#include "checkpat.h"
#include <bios.h>

#include "defines.h"
#include "lora.h"
#include "externs.h"

/*>e
   Set REDIRECT to 1 to support redirection, else to 0.
   CAUTION: The definition in 'spawn.asm' must match this definition!!
<*/


#define REDIRECT  1

#define SWAP_FILENAME "$$AAAAAA.A%02X"

/*e internal flags for prep_swap */

#define CREAT_TEMP      0x0080
#define DONT_SWAP_ENV   0x4000

#define ERR_COMSPEC     -7
#define ERR_NOMEM       -8

/*e local variables */

static char drive [MAXDRIVE], dir [MAXDIR];
static char name [MAXFILE], ext [MAXEXT];
static char cmdpath [MAXPATH] = "";
static char cmdpars [80] = "";


int do_spawn (int swapping,
          char *xeqfn,
          char *cmdtail,
          unsigned envlen,
          char *envp
#if (REDIRECT)
          ,char *rstdin,
          char *rstdout,
          char *rstderr
#endif
          );
int prep_swap (int method,
               char *swapfn);
int exists (char *fname);

/* --------------------------------------------------------------------- */

/*>e Try '.COM', '.EXE', and '.BAT' on current filename, modify 
   filename if found. <*/

int tryext (char *fn)
{
   char *ext;

   ext = strchr (fn, '\0');
   strcpy (ext, ".COM");
   if (exists (fn))
      return 1;
   strcpy (ext, ".EXE");
   if (exists (fn))
      return 1;
   strcpy (ext, ".BAT");
   if (exists (fn))
      return 2;
   *ext = 0;
   return 0;
}

/*>e Try to find the file 'fn' in the current path. Modifies the filename
   accordingly. <*/

int findfile (char *fn)
{
   char *path, *penv;
   char *prfx;
   int found, check, hasext;

   if (!*fn)
      return (cmdpath [0]) ? 3 : ERR_COMSPEC;

   check = checkpath (fn, drive, dir, name, ext, fn);
   if (check < 0)
      return check;

   if ((check & HAS_WILD) || !(check & HAS_FNAME))
      return ERR_FNAME;

   hasext = (check & HAS_EXT) ? ((!stricmp (ext, ".bat")) ? 2 : 1) : 0;

   if (hasext)
      {
      if (check & FILE_EXISTS)
         found = hasext;
      else
         found = 0;
      }
   else
      found = tryext (fn);

   if (found || (check & (HAS_PATH | HAS_DRIVE)))
      return found;

   penv = getenv ("PATH");
   if (!penv)
      return 0;
   path = (char *)malloc (strlen (penv) + 1);
   if (path == NULL)
      return ERR_NOMEM;

   strcpy (path, penv);
   prfx = strtok (path, ";");

   while (!found && prfx != NULL)
      {
      while (isspace (*prfx))
         prfx++;
      if (*prfx)
         {
         strcpy (fn, prfx);
         prfx = strchr (fn, '\0');
         prfx--;
         if (*prfx != '\\' && *prfx != '/' && *prfx != ':')
            {
            *++prfx = '\\';
            }
         prfx++;
         strcpy (prfx, name);
         strcat (prfx, ext);
         check = checkpath (fn, drive, dir, name, ext, fn);
         if (check > 0 && (check & HAS_FNAME))
            {
            if (hasext)
               {
               if (check & FILE_EXISTS)
                  found = hasext;
               }
            else
               found = tryext (fn);
            }
         }
      prfx = strtok (NULL, ";");
      }
   free (path);
   return found;
}


/*>e 
   Get name and path of the command processor via the COMSPEC 
   environmnt variable. Any parameters after the program name
   are copied and inserted into the command line.
<*/

static void getcmdpath (void)
{
   char *pcmd;
   int found = 0;

   if (cmdpath [0])
      return;
   pcmd = getenv ("COMSPEC");
   if (pcmd)
      {
      strcpy (cmdpath, pcmd);
      pcmd = cmdpath;
      while (isspace (*pcmd))
         pcmd++;
      if (NULL != (pcmd = strpbrk (pcmd, ";,=+/\"[]|<> \t")))
         {
         while (isspace (*pcmd))
            *pcmd++ = 0;
         if (strlen (pcmd) >= 79)
            pcmd [79] = 0;
         strcpy (cmdpars, pcmd);
         strcat (cmdpars, " ");
         }
      found = findfile (cmdpath);
      }
   if (!found)
      {
      cmdpars [0] = 0;
      strcpy (cmdpath, "COMMAND.COM");
      found = findfile (cmdpath);
      if (!found)
         cmdpath [0] = 0;
      }
}


/*>e
   tempdir: Set temporary file path.
            Read "TMP/TEMP" environment. If empty or invalid, clear path.
            If TEMP is drive or drive+backslash only, return TEMP.
            Otherwise check if given path is a valid directory.
            If so, add a backslash, else clear path.
<*/

int tempdir (char *outfn)
{
   int i, res;
   char *stmp [4];

   stmp [0] = getenv ("TMP");
   stmp [1] = getenv ("TEMP");
   stmp [2] = ".\\";
   stmp [3] = "\\";

   for (i = 0; i < 4; i++)
      if (stmp [i])
         {
         strcpy (outfn, stmp [i]);
         res = checkpath (outfn, drive, dir, name, ext, outfn);
         if (res > 0 && (res & IS_DIR) && !(res & IS_READ_ONLY))
            return 1;
         }
   return 0;
}


#if (REDIRECT)

int redirect (char *par, char **rstdin, char **rstdout, char **rstderr)
{
   char ch, sav;
   char *fn, *fnp;
   int app;

   do
      {
      app = 0;
      ch = *par;
      *par++ = 0;
      if (ch != '<')
         {
         if (*par == '&')
            {
            ch = '&';
            par++;
            }
         if (*par == '>')
            {
            app = 1;
            par++;
            }
         }

      while (isspace (*par))
         par++;
      fn = par;
      if ((fnp = strpbrk (par, ";,=+/\"[]|<> \t")) != NULL)
         par = fnp;
      else
         par = strchr (par, '\0');
      sav = *par;
      *par = 0;

      if (!strlen (fn))
         return 0;
      fnp = (char *)malloc (strlen (fn) + app + 1);
      if (fnp == NULL)
         return 0;
      if (app)
         {
         strcpy (fnp, ">");
         strcat (fnp, fn);
         }
      else
         strcpy (fnp, fn);

      switch (ch)
         {
         case '<':   if (*rstdin != NULL)
                        return 0;
                     *rstdin = fnp;
                     break;
         case '>':   if (*rstdout != NULL)
                        return 0;
                     *rstdout = fnp;
                     break;
         case '&':   if (*rstderr != NULL)
                        return 0;
                     *rstderr = fnp;
                     break;
         }

      *par = sav;
      while (isspace (*par))
         par++;
      }
   while (*par == '>' || *par == '<');

   return 1;
}

#endif


int do_exec (char *exfn, char *epars, int spwn, unsigned needed, char **envp)
{
   static char swapfn [MAXPATH];
   static char execfn [MAXPATH];
   unsigned avail;
   union REGS regs;
   unsigned envlen;
   int rc;
   int idx;
   char **env;
   char *ep, *envptr, *envbuf;
   char *progpars;
   int swapping;
#if (REDIRECT)
   char *rstdin = NULL, *rstdout = NULL, *rstderr = NULL;
#endif

   strcpy (execfn, exfn);
   getcmdpath ();

   /*e First, check if the file to execute exists. */
   /*d Zun„chst prfen ob die auszufhrende Datei existiert. */

   if ((rc = findfile (execfn)) <= 0)
      return RC_NOFILE | -rc;

   if (rc > 1)   /* COMMAND.COM or Batch file */
      {
      if (!cmdpath [0])
         return RC_NOFILE | -ERR_COMSPEC;

      idx = (rc == 2) ? strlen (execfn) + 5 : 1;
      progpars = (char *)malloc (strlen (epars) + strlen (cmdpars) + idx);
      if (progpars == NULL)
         return RC_NOFILE | -ERR_NOMEM;
      strcpy (progpars, cmdpars);
      if (rc == 2)
         {
         strcat (progpars, "/c ");
         strcat (progpars, execfn);
         strcat (progpars, " ");
         }
      strcat (progpars, epars);
      strcpy (execfn, cmdpath);
      }
   else
      {
      progpars = (char *)malloc (strlen (epars) + 1);
      if (progpars == NULL)
         return RC_NOFILE | -ERR_NOMEM;
      strcpy (progpars, epars);
      }

#if (REDIRECT)
   if ((ep = strpbrk (progpars, "<>")) != NULL)
      if (!redirect (ep, &rstdin, &rstdout, &rstderr))
         {
         rc = RC_REDIRERR;
         goto exit;
         }
#endif

   /*e Now create a copy of the environment if the user wants it. */
   /*d Nun eine Kopie der Umgebungsvariablen anlegen wenn angefordert. */

   envlen = 0;
   envptr = NULL;

   if (envp != NULL)
      for (env = envp; *env != NULL; env++)
         envlen += strlen (*env) + 1;

   if (envlen)
      {
      /*e round up to paragraph, and alloc another paragraph leeway */
      /*d Auf Paragraphengrenze runden, plus einen Paragraphen zur Sicherheit */
      envlen = (envlen + 32) & 0xfff0;
      envbuf = (char *)malloc (envlen);
      if (envbuf == NULL)
         {
         rc = RC_ENVERR;
         goto exit;
         }

      /*e align to paragraph */
      /*d Auf Paragraphengrenze adjustieren */
      envptr = envbuf;
      if (FP_OFF (envptr) & 0x0f)
         envptr += 16 - (FP_OFF (envptr) & 0x0f);
      ep = envptr;

      for (env = envp; *env != NULL; env++)
         {
         ep = stpcpy (ep, *env) + 1;
         }
      *ep = 0;
      }

   if (!spwn)
      swapping = -1;
   else
      {
      /*e Determine amount of free memory */
      /*d Freien Speicherbereich feststellen */

      regs.x.ax = 0x4800;
      regs.x.bx = 0xffff;
      intdos (&regs, &regs);
      avail = regs.x.bx;

      /*e No swapping if available memory > needed */
      /*d Keine Auslagerung wenn freier Speicher > ben”tigter */

      if (needed < avail)
         swapping = 0;
      else
         {
         /*>e Swapping necessary, use 'TMP' or 'TEMP' environment variable
           to determine swap file path if defined. <*/

         swapping = spwn;
         if (spwn & USE_FILE)
            {
            if (!tempdir (swapfn))
               {
               spwn &= ~USE_FILE;
               swapping = spwn;
               }
            else if (OS_MAJOR >= 3)
               swapping |= CREAT_TEMP;
            else
               {
               sprintf (e_input, SWAP_FILENAME, line_offset);
               strcat (swapfn, e_input);
               idx = strlen (swapfn) - 1;
               while (exists (swapfn))
                  {
                  if (swapfn [idx] == 'Z')
                     idx--;
                  if (swapfn [idx] == '.')
                     idx--;
                  swapfn [idx]++;
                  }
               }
            }
         }
      }

   /*e All set up, ready to go. */
   /*d Alles vorbereitet, jetzt kann's losgehen. */

   if (swapping > 0)
      {
      if (!envlen)
         swapping |= DONT_SWAP_ENV;

      rc = prep_swap (swapping, swapfn);
      if (rc < 0)
         rc = RC_PREPERR | -rc;
      else
         rc = 0;
      }
   else
      rc = 0;

   if (!rc)
#if (REDIRECT)
      rc = do_spawn (swapping, execfn, progpars, envlen, envptr, rstdin, rstdout, rstderr);
#else
      rc = do_spawn (swapping, execfn, progpars, envlen, envptr);
#endif

   /*e Free the environment buffer if it was allocated. */
   /*d Den Umgebungsvariablenblock freigeben falls er alloziert wurde. */

exit:
   free (progpars);
#if (REDIRECT)
   if (rstdin)
      free (rstdin);
   if (rstdout)
      free (rstdout);
   if (rstderr)
      free (rstderr);
#endif
   if (envlen)
      free (envbuf);
   if (spwn)
      unlink (swapfn);

   return rc;
}

