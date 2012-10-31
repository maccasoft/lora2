#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include <string.h>
#include <process.h>
#include <alloc.h>

void main (argc, argv)
int argc;
char *argv[];
{
   int i;
   char *p, argv0[80], *args[20], memf[10];

#ifdef __OS2__
   sprintf (memf, "%ld", 0L);
#else
   sprintf (memf, "%ld", farcoreleft ());
#endif

   args[0] = argv0;
   for (i = 1; i < argc; i++)
      args[i] = argv[i];
   args[i++] = memf;
   args[i] = NULL;

   strcpy (argv0, argv[0]);
   p = strstr (strupr (argv0), ".EXE");
   if (p == NULL)
      strcat (argv0, ".OVL");
   else
      strcpy (p, ".OVL");

   do {
      i = spawnvp (P_WAIT, argv0, args);
   } while (i == 255);

   exit (i);
}

