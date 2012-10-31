/*
	--- Version 3.1 91-08-17 23:07 ---

   CHECKPAT.H: Path check function header file.

   Public domain software by

        Thomas Wagner
        Ferrari electronic GmbH
        Beusselstrasse 27
        D-1000 Berlin 21
        Germany
*/

int checkpath (char *string,
           char *drive,
           char *dir,
           char *fname,
           char *ext,
           char *fullpath);

/*
   This routine accepts a file name and path, checks and resolves the
   path, and splits the name into its components.

   A relative path, or no path at all, is resolved to a full path
   specification. An invalid disk drive will not cause the routine 
   to fail.
*/

/* Error returns: */

#define ERR_DRIVE       -1    /* Invalid drive */
#define ERR_PATH        -2    /* Invalid path */
#define ERR_FNAME       -3    /* Malformed filename */
#define ERR_DRIVECHAR   -4    /* Illegal drive letter */
#define ERR_PATHLEN     -5    /* Path too long */
#define ERR_CRITICAL    -6    /* Critical error (invalid drive) */

/* Good returns (values ORed): */

#define HAS_WILD        1     /* Filename/ext has wildcard characters */
#define HAS_EXT         2     /* Extension specified */
#define HAS_FNAME       4     /* Filename specified */
#define HAS_PATH        8     /* Path specified */
#define HAS_DRIVE       0x10  /* Drive specified */
#define FILE_EXISTS     0x20  /* File exists, upper byte has attributes */
#define IS_DIR        0x1000  /* Directory, upper byte has attributes */

/* The file attributes returned if FILE_EXISTS or IS_DIR is set */

#define IS_READ_ONLY 0x0100
#define IS_HIDDEN    0x0200
#define IS_SYSTEM    0x0400
#define IS_ARCHIVED  0x2000
#define IS_DEVICE    0x4000

