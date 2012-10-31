
// LoraBBS Version 2.41 Free Edition
// Copyright (C) 1987-98 Marco Maccaferri
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <stdio.h>

#include "language.h"

/*
 * put_language -- store compiled language file
 *
 * This is a simple four step operation
 *
 * 1. Open file for write
 * 2. Write out the used part of the fixup array
 * 3. Write out the used part of the memory block
 * 4. Close the file
 *
 */

int put_language (name_of_file)
char *name_of_file;
{
    FILE           *fpt;                        /* stream pointer            */
    int             error;                      /* Internal error value      */
    int             wanna_write;                /* How many we wanna write   */
    int             written;                    /* How many we really write  */

   /*
    * Open the file for output now.
    *
    */


    fpt = fopen (name_of_file, "wb");           /* Open the file             */
    if (fpt == NULL)                            /* Were we successful?       */
        {
        fprintf (stderr, "Can not open output file %s\n", name_of_file);
        return (-1);                            /* Return failure to caller  */
        }

   /*
    * OK. Looking good so far. Write out the pointer array.
    * Don't forget that last NULL pointer to terminate it!
    *
    */

    wanna_write = 1 + pointer_size;             /* Number of things to write */
    written = fwrite ((char *)pointers, sizeof (char *), wanna_write, fpt);
    if (written != wanna_write)
        {
        fprintf (stderr, "Unable to write fixup array to output file\n");
        fclose (fpt);
        return (-2);
        }

   /*
    * Pointer array is there. Now write out the characters.
    *
    */

    wanna_write = memory_size;                  /* Number of chars to write  */
    written = fwrite (memory, sizeof (char), wanna_write, fpt);
    if (written != wanna_write)
        {
        fprintf (stderr, "Unable to write characters to output file\n");
        fclose (fpt);
        return (-3);
        }

   /*
    * Everything's there now. Close the file.
    */

    error = fclose (fpt);
    if (error != 0)
        {
        fprintf (stderr, "Unable to properly close output file %s\n",name_of_file);
        return (-4);
        }

    return (0);
}
