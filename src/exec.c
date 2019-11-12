/*
   --- Version 3.2 91-09-03 22:20 ---

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

/*
   Set REDIRECT to 1 to support redirection, else to 0.
   CAUTION: The definition in 'spawn.asm' must match this definition!!
*/


#define REDIRECT  0

#define SWAP_FILENAME "$$AAAAAA.AAA"

/* internal flags for prep_swap */

#define CREAT_TEMP      0x0080
#define DONT_SWAP_ENV   0x4000

#define ERR_COMSPEC     -7
#define ERR_NOMEM       -8


spawn_check_proc * spawn_check = NULL;

/* local variables */

static char drive [MAXDRIVE], dir [MAXDIR];
static char name [MAXFILE], ext [MAXEXT];
static char cmdpath [MAXPATH] = "";
static char cmdpars [80] = "";


#ifdef __cplusplus
extern "C" int
#else
extern int _cdecl
#endif
do_spawn(int swapping,      /* swap if non-0 */
         char * xeqfn,     /* file to execute */
         char * cmdtail,   /* command tail string */
         unsigned envlen,  /* environment length */
         char * envp       /* environment pointer */
#if (REDIRECT)
         , char * rstdin,  /* redirection file names */
         char * rstdout,
         char * rstderr
#endif
        );

#ifdef __cplusplus
extern "C" int
#else
extern int _cdecl
#endif
prep_swap(int method,       /* swap method */
          char * swapfn);  /* swap file name and/or path */

#ifdef __cplusplus
extern "C" int
#else
extern int _cdecl
#endif
exists(char * fname);

/* --------------------------------------------------------------------- */

/* Try '.COM', '.EXE', and '.BAT' on current filename, modify
   filename if found. */

static int tryext(char * fn)
{
    char * ext;

    ext = strchr(fn, '\0');
    strcpy(ext, ".COM");
    if (exists(fn)) {
        return 1;
    }
    strcpy(ext, ".EXE");
    if (exists(fn)) {
        return 1;
    }
    strcpy(ext, ".BAT");
    if (exists(fn)) {
        return 2;
    }
    *ext = 0;
    return 0;
}

/* Try to find the file 'fn' in the current path. Modifies the filename
   accordingly. */

static int findfile(char * fn)
{
    char * path, *penv;
    char * prfx;
    int found, check, hasext;

    if (!*fn) {
        return (cmdpath [0]) ? 3 : ERR_COMSPEC;
    }

    check = checkpath(fn, drive, dir, name, ext, fn);
    if (check < 0) {
        return check;
    }

    if ((check & HAS_WILD) || !(check & HAS_FNAME)) {
        return ERR_FNAME;
    }

    hasext = (check & HAS_EXT) ? ((!stricmp(ext, ".bat")) ? 2 : 1) : 0;

    if (hasext)
    {
        if (check & FILE_EXISTS) {
            found = hasext;
        }
        else {
            found = 0;
        }
    }
    else {
        found = tryext(fn);
    }

    if (found || (check & (HAS_PATH | HAS_DRIVE))) {
        return found;
    }

    penv = getenv("PATH");
    if (!penv) {
        return 0;
    }
    path = (char *)malloc(strlen(penv) + 1);
    if (path == NULL) {
        return ERR_NOMEM;
    }

    strcpy(path, penv);
    prfx = strtok(path, ";");

    while (!found && prfx != NULL)
    {
        while (isspace(*prfx)) {
            prfx++;
        }
        if (*prfx)
        {
            strcpy(fn, prfx);
            prfx = strchr(fn, '\0');
            prfx--;
            if (*prfx != '\\' && *prfx != '/' && *prfx != ':')
            {
                *++prfx = '\\';
            }
            prfx++;
            strcpy(prfx, name);
            strcat(prfx, ext);
            check = checkpath(fn, drive, dir, name, ext, fn);
            if (check > 0 && (check & HAS_FNAME))
            {
                if (hasext)
                {
                    if (check & FILE_EXISTS) {
                        found = hasext;
                    }
                }
                else {
                    found = tryext(fn);
                }
            }
        }
        prfx = strtok(NULL, ";");
    }
    free(path);
    return found;
}


/*
   Get name and path of the command processor via the COMSPEC
   environmnt variable. Any parameters after the program name
   are copied and inserted into the command line.
*/

static void getcmdpath(void)
{
    char * pcmd;
    int found = 0;

    if (cmdpath [0]) {
        return;
    }
    pcmd = getenv("COMSPEC");
    if (pcmd)
    {
        strcpy(cmdpath, pcmd);
        pcmd = cmdpath;
        while (isspace(*pcmd)) {
            pcmd++;
        }
        if (NULL != (pcmd = strpbrk(pcmd, ";,=+/\"[]|<> \t")))
        {
            while (isspace(*pcmd)) {
                *pcmd++ = 0;
            }
            if (strlen(pcmd) >= 79) {
                pcmd [79] = 0;
            }
            strcpy(cmdpars, pcmd);
            strcat(cmdpars, " ");
        }
        found = findfile(cmdpath);
    }
    if (!found)
    {
        cmdpars [0] = 0;
        strcpy(cmdpath, "COMMAND.COM");
        found = findfile(cmdpath);
        if (!found) {
            cmdpath [0] = 0;
        }
    }
}


/*
   tempdir: Set temporary file path.
            Read "TMP/TEMP" environment. If empty or invalid, clear path.
            If TEMP is drive or drive+backslash only, return TEMP.
            Otherwise check if given path is a valid directory.
            If so, add a backslash, else clear path.
*/

static int tempdir(char * outfn)
{
    int i, res;
    char * stmp [4];

    stmp [0] = getenv("TMP");
    stmp [1] = getenv("TEMP");
    stmp [2] = ".\\";
    stmp [3] = "\\";

    for (i = 0; i < 4; i++)
        if (stmp [i])
        {
            strcpy(outfn, stmp [i]);
            res = checkpath(outfn, drive, dir, name, ext, outfn);
            if (res > 0 && (res & IS_DIR) && !(res & IS_READ_ONLY)) {
                return 1;
            }
        }
    return 0;
}


#if (REDIRECT)

static int redirect(char * par, char ** rstdin, char ** rstdout, char ** rstderr)
{
    char ch, sav;
    char * fn, *fnp;
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

        while (isspace(*par)) {
            par++;
        }
        fn = par;
        if ((fnp = strpbrk(par, ";,=+/\"[]|<> \t")) != NULL) {
            par = fnp;
        }
        else {
            par = strchr(par, '\0');
        }
        sav = *par;
        *par = 0;

        if (!strlen(fn)) {
            return 0;
        }
        fnp = (char *)malloc(strlen(fn) + app + 1);
        if (fnp == NULL) {
            return 0;
        }
        if (app)
        {
            strcpy(fnp, ">");
            strcat(fnp, fn);
        }
        else {
            strcpy(fnp, fn);
        }

        switch (ch)
        {
            case '<':
                if (*rstdin != NULL) {
                    return 0;
                }
                *rstdin = fnp;
                break;
            case '>':
                if (*rstdout != NULL) {
                    return 0;
                }
                *rstdout = fnp;
                break;
            case '&':
                if (*rstderr != NULL) {
                    return 0;
                }
                *rstderr = fnp;
                break;
        }

        *par = sav;
        while (isspace(*par)) {
            par++;
        }
    } while (*par == '>' || *par == '<');

    return 1;
}

#endif


int do_exec(char * exfn, char * epars, int spwn, unsigned needed, char ** envp)
{
    static char swapfn [MAXPATH];
    static char execfn [MAXPATH];
    unsigned avail;
    union REGS regs;
    unsigned envlen;
    int rc, ffrc;
    int idx;
    char ** env;
    char * ep, *envptr, *envbuf;
    char * progpars;
    int swapping;
#if (REDIRECT)
    char * rstdin = NULL, *rstdout = NULL, *rstderr = NULL;
#endif

    envlen = 0;
    envptr = NULL;
    envbuf = NULL;

    strcpy(execfn, exfn);
    getcmdpath();

    /* First, check if the file to execute exists. */

    if ((ffrc = findfile(execfn)) <= 0) {
        return RC_NOFILE | -ffrc;
    }

    if (ffrc > 1)   /* COMMAND.COM or Batch file */
    {
        if (!cmdpath [0]) {
            return RC_NOFILE | -ERR_COMSPEC;
        }

        idx = (ffrc == 2) ? strlen(execfn) + 5 : 1;
        progpars = (char *)malloc(strlen(epars) + strlen(cmdpars) + idx);
        if (progpars == NULL) {
            return RC_NOFILE | -ERR_NOMEM;
        }
        strcpy(progpars, cmdpars);
        if (ffrc == 2)
        {
            strcat(progpars, "/c ");
            strcat(progpars, execfn);
            strcat(progpars, " ");
        }
        strcat(progpars, epars);
        strcpy(execfn, cmdpath);
    }
    else
    {
        progpars = (char *)malloc(strlen(epars) + 1);
        if (progpars == NULL) {
            return RC_NOFILE | -ERR_NOMEM;
        }
        strcpy(progpars, epars);
    }

#if (REDIRECT)
    if ((ep = strpbrk(progpars, "<>")) != NULL)
        if (!redirect(ep, &rstdin, &rstdout, &rstderr))
        {
            rc = RC_REDIRERR;
            goto exit;
        }
#endif

    /* Now create a copy of the environment if the user wants it. */

    if (envp != NULL)
        for (env = envp; *env != NULL; env++) {
            envlen += strlen(*env) + 1;
        }

    if (envlen)
    {
        /* round up to paragraph, and alloc another paragraph leeway */
        envlen = (envlen + 32) & 0xfff0;
        envbuf = (char *)malloc(envlen);
        if (envbuf == NULL)
        {
            rc = RC_ENVERR;
            goto exit;
        }

        /* align to paragraph */
        envptr = envbuf;
        if (FP_OFF(envptr) & 0x0f) {
            envptr += 16 - (FP_OFF(envptr) & 0x0f);
        }
        ep = envptr;

        for (env = envp; *env != NULL; env++)
        {
            ep = stpcpy(ep, *env) + 1;
        }
        *ep = 0;
    }

    if (!spwn) {
        swapping = -1;
    }
    else
    {
        /* Determine amount of free memory */

        regs.x.ax = 0x4800;
        regs.x.bx = 0xffff;
        intdos(&regs, &regs);
        avail = regs.x.bx;

        /* No swapping if available memory > needed */

        if (needed < avail) {
            swapping = 0;
        }
        else
        {
            /* Swapping necessary, use 'TMP' or 'TEMP' environment variable
              to determine swap file path if defined. */

            swapping = spwn;
            if (spwn & USE_FILE)
            {
                if (!tempdir(swapfn))
                {
                    spwn &= ~USE_FILE;
                    swapping = spwn;
                }
                else if (OS_MAJOR >= 3) {
                    swapping |= CREAT_TEMP;
                }
                else
                {
                    strcat(swapfn, SWAP_FILENAME);
                    idx = strlen(swapfn) - 1;
                    while (exists(swapfn))
                    {
                        if (swapfn [idx] == 'Z') {
                            idx--;
                        }
                        if (swapfn [idx] == '.') {
                            idx--;
                        }
                        swapfn [idx]++;
                    }
                }
            }
        }
    }

    /* All set up, ready to go. */

    if (swapping > 0)
    {
        if (!envlen) {
            swapping |= DONT_SWAP_ENV;
        }

        rc = prep_swap(swapping, swapfn);
        if (rc < 0) {
            rc = RC_PREPERR | -rc;
        }
        else {
            rc = 0;
        }
    }
    else {
        rc = 0;
    }

    if (!rc)
    {
        if (spawn_check != NULL) {
            rc = spawn_check(ffrc, swapping, execfn, progpars);
        }
        if (!rc)
#if (REDIRECT)
            rc = do_spawn(swapping, execfn, progpars, envlen, envptr, rstdin, rstdout, rstderr);
#else
            rc = do_spawn(swapping, execfn, progpars, envlen, envptr);
#endif
    }

    /* Free the environment buffer if it was allocated. */

exit:
    free(progpars);
#if (REDIRECT)
    if (rstdin) {
        free(rstdin);
    }
    if (rstdout) {
        free(rstdout);
    }
    if (rstderr) {
        free(rstderr);
    }
#endif
    if (envlen) {
        free(envbuf);
    }

    return rc;
}

