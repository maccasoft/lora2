;
;                          Message Base Renumberer
;
;              This module was originally written by Bob Hartman
;                       Sysop of FidoNet node 1:132/101
;
;   Spark Software, 427-3 Amherst St, CS 2032, Suite 232, Nashua, NH 03061
;
; This program source code is being released with the following provisions:
;
; 1.  You are  free to make  changes to this source  code for use on your own
; machine,  however,  altered source files may not be distributed without the 
; consent of Spark Software.
;
; 2.  You may distribute "patches"  or  "diff" files for any changes that you
; have made, provided that the "patch" or "diff" files are also sent to Spark
; Software for inclusion in future releases of the entire package.   A "diff"
; file for the source archives may also contain a compiled version,  provided
; it is  clearly marked as not  being created  from the original source code.
; No other  executable  versions may be  distributed without  the  consent of
; Spark Software.
;
; 3.  You are free to include portions of this source code in any program you
; develop, providing:  a) Credit is given to Spark Software for any code that
; may is used, and  b) The resulting program is free to anyone wanting to use
; it, including commercial and government users.
;
; 4.  There is  NO  technical support  available for dealing with this source
; code, or the accompanying executable files.  This source  code  is provided
; as is, with no warranty expressed or implied (I hate legalease).   In other
; words, if you don't know what to do with it,  don't use it,  and if you are
; brave enough to use it, you're on your own.
;
; Spark Software may be contacted by modem at (603) 888-8179 (node 1:132/101)
; on the public FidoNet network, or at the address given above.
;

; ---------------------------------------------------------------------------
;
; This version has been patched to work on extended (>32MB) partitions.
; The changes were made by Alberto Pasquale (SysOp of FidoNet node 2:332/504).
;
; ---------------------------------------------------------------------------
;
; Assembled by Turbo Assembler 1.0
;
; ---------------------------------------------------------------------------

.xlist
        page    64,132
        
        title   DiskIO
        subttl  by Bob Hartman
        
        name    DiskIO
        ;
        ;
        ;
        ; The following macro files come with the MicroSoft "C" compiler
        ;
        include version.inc
        include cmacros.inc
        
.list

sBegin   code

        assumes  cs,code
        assumes  ds,data

prtn    dw      2       ; 1=oldstyle 0=extended 2=not established

packet:
sect    dd      ?       ; packet for extended call
num     dw      1
addr    dd      ?


estprtn proc
        push    ax
        mov     ah,30h
        int     21h
        xchg    ah,al
        cmp     ax,031fh ; DOS 3.31
        mov     prtn,1
        jb      io_exit
        mov     prtn,0
io_exit:pop     ax
        ret
estprtn endp

cProc   read_sect,<PUBLIC>

        parmDP  sect_ptr
        parmD   sector
        parmW   disk

cBegin
        cmp     prtn,2
        jne     chkx25

        call    estprtn

chkx25: push    ds
        cmp     prtn,0
        je      x25

        mov     cx,1
        mov     dx,word ptr sector
        mov     bx,word ptr sect_ptr
        mov     ax,word ptr sect_ptr+2
        mov     ds,ax
        jmp     short do25

x25:    mov     ax,word ptr sector
        mov     word ptr sect,ax
        mov     ax,word ptr sector+2
        mov     word ptr sect+2,ax
        mov     ax,word ptr sect_ptr
        mov     word ptr addr,ax
        mov     ax,word ptr sect_ptr+2
        mov     word ptr addr+2,ax


        push    cs
        pop     ds
        mov     bx,offset packet
        mov     cx,0ffffh

do25:   mov     ax,disk
        push    bp
        int     25h
        jc      rs_exit
        xor     ax,ax

rs_exit:
        add     sp,2
        pop     bp
        pop     ds

cEnd

cProc   write_sect,<PUBLIC>

        parmDP  sect_ptr
        parmD   sector
        parmW   disk

cBegin
        cmp     prtn,2
        jne     chkx26

        call    estprtn

chkx26: push    ds
        cmp     prtn,0
        je      x26

        mov     cx,1
        mov     dx,word ptr sector
        mov     bx,word ptr sect_ptr
        mov     ax,word ptr sect_ptr+2
        mov     ds,ax
        jmp     short do26

x26:    mov     ax,word ptr sector
        mov     word ptr sect,ax
        mov     ax,word ptr sector+2
        mov     word ptr sect+2,ax
        mov     ax,word ptr sect_ptr
        mov     word ptr addr,ax
        mov     ax,word ptr sect_ptr+2
        mov     word ptr addr+2,ax

        push    cs
        pop     ds
        mov     bx,offset packet
        mov     cx,0ffffh

do26:   mov     ax,disk
        push    bp
        int     26h
        jc      ws_exit
        xor     ax,ax

ws_exit:
        add     sp,2
        pop     bp
        pop     ds

cEnd

cProc   get_dir_entry,<PUBLIC>

        parmDP  extfcb
        parmDP  dta

cBegin
        push    ds
        mov     dx,word ptr dta
        mov     ax,word ptr dta+2
        mov     ds,ax
        mov     ah,1ah
        int     21h

        mov     dx,word ptr extfcb
        mov     ax,word ptr extfcb+2
        mov     ds,ax
        mov     ah,11h
        int     21h
        or      al,al
        jnz     gde_exit
        xor     ax,ax

gde_exit:
        xor     ah,ah
        pop     ds
cEnd

cProc   disk_reset,<PUBLIC>

cBegin

        mov     ah,0dh
        int     21h

cEnd

cProc   fcb_delete,<PUBLIC>

        parmDP  fcb

cBegin

        push    ds
        mov     dx,word ptr fcb
        mov     ax,word ptr fcb+2
        mov     ds,ax
        mov     ah,13h
        int     21h
        xor     ah,ah
        pop     ds
cEnd

sEnd
        end
