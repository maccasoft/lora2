/*
	--- Version 3.1 91-08-17 23:05 ---

   EXEC.H: EXEC function with memory swap - Main function header file.

   Public domain software by

        Thomas Wagner
        Ferrari electronic GmbH
        Beusselstrasse 27
        D-1000 Berlin 21
        Germany
*/


extern int do_exec (char *xfn, char *pars, int spawn, unsigned needed,
						  char **envp);

/*>e
   The EXEC function.

      Parameters:

         xfn      is a string containing the name of the file
                  to be executed. If the string is empty,
                  the COMSPEC environment variable is used to
                  load a copy of COMMAND.COM or its equivalent.
                  If the filename does not include a path, the
                  current PATH is searched after the default.
                  If the filename does not include an extension,
                  the path is scanned for a COM, EXE, or BAT file 
                  in that order.

         pars     The program parameters.

         spawn    If 0, the function will terminate after the 
                  EXECed program returns, the function will not return.

                  NOTE: If the program file is not found, the function 
                        will always return with the appropriate error 
                        code, even if 'spawn' is 0.

                  If non-0, the function will return after executing the
                  program. If necessary (see the "needed" parameter),
                  memory will be swapped out before executing the program.
                  For swapping, spawn must contain a combination of the 
                  following flags:

                     USE_EMS  (0x01)  - allow EMS swap
                     USE_XMS  (0x02)  - allow XMS swap
                     USE_FILE (0x04)  - allow File swap

                  The order of trying the different swap methods can be
                  controlled with one of the flags

                     EMS_FIRST (0x00) - EMS, XMS, File (default)
                     XMS_FIRST (0x10) - XMS, EMS, File

                  If swapping is to File, the attribute of the swap file
                  can be set to "hidden", so users are not irritated by
                  strange files appearing out of nowhere with the flag

                     HIDE_FILE (0x40)    - create swap file as hidden

                  and the behaviour on Network drives can be changed with

                     NO_PREALLOC (0x100) - don't preallocate
                     CHECK_NET (0x200)   - don't preallocate if file on net.

                  This checking for Network is mainly to compensate for
                  a strange slowdown on Novell networks when preallocating
                  a file. You can either set NO_PREALLOC to avoid allocation
                  in any case, or let the prep_swap routine decide whether
                  to do preallocation or not depending on the file being
                  on a network drive (this will only work with DOS 3.1 or 
                  later).

         needed   The memory needed for the program in paragraphs (16 Bytes).
                  If not enough memory is free, the program will 
                  be swapped out. 
                  Use 0 to never swap, 0xffff to always swap. 
                  If 'spawn' is 0, this parameter is irrelevant.

         envp     The environment to be passed to the spawned
                  program. If this parameter is NULL, a copy
                  of the parent's environment is used (i.e.
                  'putenv' calls have no effect). If non-NULL,
                  envp must point to an array of pointers to
                  strings, terminated by a NULL pointer (the
                  standard variable 'environ' may be used).

      Return value:

         0x0000..00FF: The EXECed Program's return code

         0x0101:       Error preparing for swap: no space for swapping
         0x0102:       Error preparing for swap: program too low in memory

         0x0200:       Program file not found
         0x0201:       Program file: Invalid drive
         0x0202:       Program file: Invalid path
         0x0203:       Program file: Invalid name
         0x0204:       Program file: Invalid drive letter
         0x0205:       Program file: Path too long
         0x0206:       Program file: Drive not ready
         0x0207:       Batchfile/COMMAND: COMMAND.COM not found
         0x0208:       Error allocating temporary buffer

         0x03xx:       DOS-error-code xx calling EXEC

         0x0400:       Error allocating environment buffer

         0x0500:       Swapping requested, but prep_swap has not 
                       been called or returned an error.
         0x0501:       MCBs don't match expected setup
         0x0502:       Error while swapping out

         0x0600:       Redirection syntax error
         0x06xx:       DOS error xx on redirection
<*/

/*>d
   Die EXEC Funktion.

      Parameter:

         xfn      ist ein String mit dem Namen der auszufÅhrenden Datei.
                  Ist der String leer, wird die COMSPEC Umgebungsvariable
                  benutzt um COMMAND.COM oder das Equivalent zu laden.
                  Ist kein Pfad angegeben, wird nach dem aktuellen Pfad
                  der in der PATH Umgebungsvariablen angegebene Pfad
                  durchsucht.
                  Ist kein Dateityp angegeben, wird der Pfad nach
                  einer COM oder EXE Datei (in dieser Reihenfolge) abgesucht.

         pars     Die Kommandozeile

         spawn    Wenn 0, wird der Programmlauf beendet wenn das
                  aufgerufene Programm zurÅckkehrt, die Funktion kehrt
                  nicht zurÅck.

                  HINWEIS: Wenn die auszufÅhrende Datei nicht gefunden
                        wird, kehrt die Funktion mit einem Fehlercode
                        zurÅck, auch wenn der 'spawn' Parameter 0 ist.

                  Wenn nicht 0, kehrt die Funktion nach AusfÅhrung des
                  Programms zurÅck. Falls notwendig (siehe den Parameter
                  "needed") wird der Programmspeicherbereich vor Aufruf 
                  ausgelagert.
                  Zur Auslagerung mu· der Parameter eine Kombination der
                  folgenden Flags enthalten:

                     USE_EMS  (0x01)  - Auslagerung auf EMS zulassen
                     USE_XMS  (0x02)  - Auslagerung auf XMS zulassen
                     USE_FILE (0x04)  - Auslagerung auf Datei zulassen

                  Die Reihenfolge der Versuche, auf die verschiedenen
                  Medien auszulagern kann mit einem der folgenden
                  Flags beeinflu·t werden:

                     EMS_FIRST (0x00) - EMS, XMS, Datei (Standard)
                     XMS_FIRST (0x10) - XMS, EMS, Datei

                  Wenn die Auslagerung auf Datei erfolgt, kann das
                  Attribut dieser Datei auf "hidden" gesetzt werden,
                  damit der Benutzer nicht durch unversehends auftauchende
                  Dateien verwirrt wird:

                     HIDE_FILE (0x40) - Auslagerungsdatei "hidden" erzeugen

                  Au·erdem kann das Verhalten auf Netzwerk-Laufwerken 
                  beeinflu·t werden mit

                     NO_PREALLOC (0x100) - nicht PrÑallozieren
                     CHECK_NET (0x200)   - nicht PrÑallozieren wenn Netz.

                  Diese PrÅfung auf Netzwerk ist hauptsÑchlich sinnvoll
                  fÅr Novell Netze, bei denen eine PrÑallozierung eine
                  erhebliche Verzîgerung bewirkt. Sie kînnen entweder mit
                  NO_PREALLOC eine PrÑallozierung in jedem Fall ausschlie·en,
                  oder die Entscheidung mit CHECK_NET prep_swap Åberlassen.
                  In diesem Fall wird nicht prÑalloziert wenn die Datei
                  auf einem Netzwerk-Laufwerk liegt (funktioniert nur
                  mit DOS Version 3.1 und spÑteren).

         needed   Der zur AusfÅhrung des Programms benîtigte Speicher
                  in Paragraphen (16 Bytes). Wenn nicht ausreichend 
                  freier Speicher vorhanden ist, wird der Programm-
                  speicherbereich ausgelagert.
                  Bei Angabe von 0 wird nie ausgelagert, bei Angabe
                  von 0xffff wird immer ausgelagert.
                  Ist der Parameter 'spawn' 0, hat 'needed' keine Bedeutung.

         envp     Die dem gerufenen Programm zu Åbergebenden 
                  Umgebungsvariablen. Ist der Parameter NULL,
                  wird eine Kopie der Vater-Umgebung benutzt,
                  d.h. da· Aufrufe von "putenv" keinen Effekt haben.
                  Wenn nicht NULL mu· envp auf ein Array von Zeigern
                  auf Strings zeigen, das durch einen NULL Zeiger
                  abgeschlossen wird. HierfÅr kann die Standardvariable 
                  "environ" benutzt werden.

      Liefert:

         0x0000..00FF: RÅckgabewert des aufgerufenen Programms

         0x0101:       Fehler bei Vorbereitung zum Auslagern
                        kein Speicherplatz in XMS/EMS/Datei
         0x0102:       Fehler bei Vorbereitung zum Auslagern
                        der Programmcode ist zu nah am Beginn des
                        Programms.

         0x0200:       AuszufÅhrende Programmdatei nicht gefunden
         0x0201:       Programmdatei: UngÅltiges Laufwerk
         0x0202:       Programmdatei: UngÅltiger Pfad
         0x0203:       Programmdatei: UngÅltiger Dateiname
         0x0204:       Programmdatei: UngÅltiger Laufwerksbuchstabe
         0x0205:       Programmdatei: Pfad zu lang
         0x0206:       Programmdatei: Laufwerk nicht bereit
         0x0207:       Batchfile/COMMAND: COMMAND.COM nicht gefunden
         0x0208:       Fehler beim allozieren eines temporÑren Puffers

         0x03xx:       DOS-Fehler-Code xx bei Aufruf von EXEC

         0x0400:       Fehler beim allozieren der Umgebungsvariablenkopie

         0x0500:       Auslagerung angefordert, aber prep_swap wurde nicht
                       aufgerufen oder lieferte einen Fehler
         0x0501:       MCBs entsprechen nicht dem erwarteten Aufbau
         0x0502:       Fehler beim Auslagern

         0x0600:       Redirection Syntaxfehler
         0x06xx:       DOS-Fehler xx bei Redirection
<*/

/*e Return codes (only upper byte significant) */
/*d Fehlercodes (nur das obere Byte signifikant) */

#define RC_PREPERR   0x0100
#define RC_NOFILE    0x0200
#define RC_EXECERR   0x0300
#define RC_ENVERR    0x0400
#define RC_SWAPERR   0x0500
#define RC_REDIRERR  0x0600

/*e Swap method and option flags */
/*d Auslagerungsmethoden ond Optionen */

#define USE_EMS      0x01
#define USE_XMS      0x02
#define USE_FILE     0x04
#define EMS_FIRST    0x00
#define XMS_FIRST    0x10
#define HIDE_FILE    0x40
#define NO_PREALLOC  0x100
#define CHECK_NET    0x200

#define USE_ALL      (USE_EMS | USE_XMS | USE_FILE)

