
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
#include <mem.h>
#include <string.h>

#include "language.h"

int get_language (char *name_of_file)
{
   int len, count_from_file, file_version, escape_on, internal_count, total_size, error;
   char *p, *q, c, *storage, **load_pointers, linebuf[512];
   FILE *fpt;

   internal_count = 0;                     /* zero out internal counter */
   count_from_file = 0;                    /* zero out internal counter */
   total_size = 0;                         /* Initialize storage size   */
   error = 0;                              /* Initialize error value    */

   load_pointers = pointers;               /* Start at the beginning    */
   storage = memory;                       /* A very good place to start*/

   if ((fpt = fopen (name_of_file, "r")) == NULL) {
      fprintf (stderr, "Can not open input file %s\n", name_of_file);
      return (-1);
   }

   while (fgets (linebuf, 500, fpt) != NULL) {
      escape_on = 0;
      p = q = linebuf;

/*
      if (count_from_file) {
         while (*p != 0 && *p != '"')
            p++;
         if (*p == '"')
            strcpy (linebuf, ++p);
         else
            strcpy (linebuf, p);
         p = q = linebuf;
      }
*/

      while ((c = *p++) != 0) {
         switch (c) {
/*
            case '"':
               c = '\n';
               *q = *p = '\0';
               break;
*/

            case ';':
               if (escape_on) {
                  *q++ = ';';
                  --escape_on;
                  break;
               }

            case '\n':
               *q = *p = '\0';
               break;

            case '\\':
               if (escape_on) {
                  *q++ = '\\';
                  --escape_on;
               }
               else
                  ++escape_on;
               break;

            case 'n':
               if (escape_on) {
                  *q++ = '\n';
                  --escape_on;
               }
               else
                  *q++ = c;
               break;

            case 'r':
               if (escape_on) {
                  *q++ = '\r';
                  --escape_on;
               }
               else
                  *q++ = c;
               break;

            case '_':
               if (escape_on) {
                  *q++ = '_';
                  --escape_on;
               }
               else
                  *q++ = ' ';
               break;

            default:
               *q++ = c;
               escape_on = 0;
               break;
         }
      }

      if (!(len = (int)(q - linebuf)))
         continue;

      if (!count_from_file) {
         sscanf (linebuf,"%d %d",&count_from_file, &file_version);
         if (count_from_file <= pointer_size)
            continue;

         fprintf (stderr, "Messages in file = %d, Pointer array size = %d\n", count_from_file, pointer_size);
         error = -2;
         break;
      }

      ++len;
      if (((total_size += len) < memory_size) &&  (internal_count < pointer_size)) {
         memcpy (storage, linebuf, len);     /* Copy it now (with term)   */
         *load_pointers++ = storage;         /* Point to start of string  */
         storage += len;                     /* Move pointer into memory  */
      }

      ++internal_count;                       /* bump count */
   }

   fclose (fpt);

   if (internal_count > pointer_size) {
      fprintf (stderr, "%d messages read exceeds pointer array size of %d\n", internal_count, pointer_size);
      error = -3;
   }

   if (total_size > memory_size) {
      fprintf (stderr, "Required memory of %d bytes exceeds %d bytes available\n", total_size, memory_size);
      error = -4;
   }

   if (count_from_file != internal_count) {
      fprintf (stderr, "Count of %d lines does not match %d lines actually read\n", count_from_file, internal_count);
      error = -5;
   }

   if (!error) {
      pointer_size = internal_count;          /* Store final usage counts  */
      memory_size = total_size;
      *load_pointers = NULL;                  /* Terminate pointer table   */
   }

   return (error);
}
