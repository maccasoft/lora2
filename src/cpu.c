#pragma inline
#include <stdio.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "zmodem.h"
#include "msgapi.h"

void display_cpu (void)
{
   short type;

   asm .386
   asm pushf                           // Save old flags

   asm mov     dx, 86                  // Test for 8086
   asm push    sp                      // If SP decrements before
   asm pop     ax                      // a value is PUSHed
   asm cmp     sp, ax                  // it's a real-mode chip
   asm jne     Exit                    // 8088,8086,80188,80186,NECV20/V30

   asm mov     dx, 286                 // Test for 286
   asm pushf                           // If NT (Nested Task)
   asm pop     ax                      // bit (bit 14) in the
   asm or      ax, 4000h               // flags register
   asm push    ax                      // can't be set (in real mode)
   asm popf                            // then it's a 286
   asm pushf
   asm pop     ax
   asm test    ax, 4000h
   asm jz      Exit

   asm mov     dx, 386                 // Test for 386/486
//   asm .386                            // do some 32-bit stuff
   asm mov     ebx, esp                // Zero lower 2 bits of ESP
   asm and     esp, 0FFFFFFFCh         // to avoid AC fault on 486
   asm pushfd                          // Push EFLAGS register
   asm pop     eax                     // EAX = EFLAGS
   asm mov     ecx, eax                // ECX = EFLAGS
   asm xor     eax, 40000h             // Toggle AC bit(bit 18)
                                                // in EFLAGS register
   asm push    eax                     // Push new value
   asm popfd                           // Put it in EFLAGS
   asm pushfd                          // Push EFLAGS
   asm pop     eax                     // EAX = EFLAGS
   asm and     eax, 40000h             // Isolate bit 18 in EAX
   asm and     ecx, 40000h             // Isolate bit 18 in ECX
   asm cmp     eax, ecx                // Is EAX and ECX equal?
   asm je      A_386                   // Yup, it's a 386
   asm mov     dx, 486                 // Nope,it's a 486
A_386:
   asm push    ecx                     // Restore
   asm popfd                           // EFLAGS
   asm mov     esp, ebx                // Restore ESP
Exit:
   asm mov     type, dx                // Put CPU type in type
   asm popf                            // Restore old flags

   m_print (" (CPU 80%d)\n", type);
}
