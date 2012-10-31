#include <stdio.h>
#include <stdlib.h>

#include "language.h"


/*
 * Assume average length of a string at 32 characters
 *
 */

#define MAX_STRINGS 1000
#define MAX_MEMORY (MAX_STRINGS * 32)

char **pointers;
int pointer_size;

char *memory;
int memory_size;


/*
 * Read in a raw text file and write out a compiled BinkleyTerm
 * language file.
 *
 */

void process_language (type, mpath)
char *type, *mpath;
{
    char *malloc_target, fn[50];
    int error;

   /*
    * Print out the copyright notice.
    */

   printf("Compile the %s Language File\n", type);

   /*
    * Allocate space for the raw character array and for the
    * pointer array
    *
    */

    malloc_target = malloc (MAX_MEMORY);
    if (malloc_target == NULL)
        {
        fprintf (stderr, "Unable to allocate string memory\n");
        exit (250);
        }
    memory = malloc_target;
    memory_size = MAX_MEMORY;

    malloc_target = malloc ((MAX_STRINGS + 1) * (sizeof (char *)));
    if (malloc_target == NULL)
        {
        fprintf (stderr, "Unable to allocate pointer array\n");
        exit (250);
        }
    pointers = (char **)malloc_target;
    pointer_size = MAX_STRINGS;


   /*
    * Now read the stuff into our array.
    *
    */

    sprintf(fn, "%s%s.TXT", mpath, type);
    error = get_language (fn);
    if (error != 0)
       exit (240);


   /*
    * Write our stuff out now.
    *
    */

    sprintf(fn, "%s%s.LNG", mpath, type);
    error = put_language (fn);
    if (error != 0)
       exit (230);

    free (memory);
    free (pointers);
}



