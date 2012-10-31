#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <alloc.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"

/*
 * Read the compiled BinkleyTerm Language file.
 *
 */

int load_language (which)
int which;
{
   int pointer_size;
   char *memory;
   unsigned int memory_size;
   char *malloc_target;
   char LANGpath[128];
   int error;
   int i;
   int read;
   struct stat stbuf;
   FILE           *fpt;                         /* stream pointer           */

   for (i = 0; config->language[i].descr[0]; i++);
   if (which >= i)
      return (0);

   strcpy (LANGpath, config->menu_path);
   strcat (LANGpath, config->language[which].basename);
   strcat (LANGpath, ".LNG");

   /*
    * Get some info about the file
    */

    error = stat (LANGpath, &stbuf);
    if (error != 0)
        {
        status_line ("!Cannot get information on file %s\n",LANGpath);
        exit (250);
        }

   /*
    * Allocate space for the raw character array and for the
    * pointer and fixup arrays
    *
    */

    memory_size = (unsigned int) stbuf.st_size;

    malloc_target = malloc (memory_size);
    if (malloc_target == NULL)
        {
        status_line ("!Unable to allocate string memory\n");
        exit (250);
        }

   /*
    * Open the input file
    *
    */

    fpt = fopen (LANGpath, "rb");               /* Open the file             */
    if (fpt == NULL)                            /* Were we successful?       */
        {
        fprintf (stderr, "Can not open input file %s\n", LANGpath);
        exit (250);
        }

   /*
    * Read the entire file into memory now.
    *
    */

    read = fread (malloc_target, 1, memory_size, fpt);
    if (read != memory_size)
        {
        fprintf (stderr, "Could not read language data from file %s\n",LANGpath);
        fclose (fpt);
        exit (250);
        }

   /*
    * Close the file.
    *
    */

    error = fclose (fpt);
    if (error != 0)
        {
        fprintf (stderr, "Unable to close language file %s\n",LANGpath);
        exit (250);
        }

   /*
    * Do fixups on the string pointer array as follows:
    *
    * 1. Find the NULL pointer in the mess here.
    * 2. Start of the string memory is the "following pointer".
    * 3. Apply arithmetic correction to entire array.
    *
    */

    bbstxt = (char **) malloc_target;
    for (i = 0; bbstxt[i] != NULL; i++)         /* Find NULL marker in array */
        ;

    pointer_size = i - 1;                       /* Count of elements w/o NULL*/
    if (pointer_size != X_TOTAL_MSGS)
        {
        fprintf (stderr, "Count of %d from file does not match %d required\n",
                    pointer_size, X_TOTAL_MSGS);
        exit (250);
        }

    memory = (char *) &bbstxt[++i];             /* Text starts after NULL    */

    for (i = 1; i <= pointer_size; i++)
        {
        bbstxt[i] = memory + (short) (bbstxt[i] - bbstxt[0]);
        }
    bbstxt[0] = memory;

    mesi[0] = bbstxt[B_JAN];
    mesi[1] = bbstxt[B_FEB];
    mesi[2] = bbstxt[B_MAR];
    mesi[3] = bbstxt[B_APR];
    mesi[4] = bbstxt[B_MAY];
    mesi[5] = bbstxt[B_JUN];
    mesi[6] = bbstxt[B_JUL];
    mesi[7] = bbstxt[B_AGO];
    mesi[8] = bbstxt[B_SEP];
    mesi[9] = bbstxt[B_OCT];
    mesi[10] = bbstxt[B_NOV];
    mesi[11] = bbstxt[B_DEC];

    bbstxt[0] = memory;
    return (1);
}
