;--------------------------------------------------------------------------;
;                                                                          ;
;                                                                          ;
;      ------------         Bit-Bucket Software, Co.                       ;
;      \ 10001101 /         Writers and Distributors of                    ;
;       \ 011110 /          Freely Available<tm> Software.                 ;
;        \ 1011 /                                                          ;
;         ------                                                           ;
;                                                                          ;
;  (C) Copyright 1987-91, Bit Bucket Software Co., a Delaware Corporation. ;
;                                                                          ;
;                                                                          ;
;                   Assembly routines for BinkleyTerm                      ;
;                                                                          ;
;                                                                          ;
;    For complete  details  of the licensing restrictions, please refer    ;
;    to the License  agreement,  which  is published in its entirety in    ;
;    the MAKEFILE and BT.C, and also contained in the file LICENSE.250.    ;
;                                                                          ;
;    USE  OF THIS FILE IS SUBJECT TO THE  RESTRICTIONS CONTAINED IN THE    ;
;    BINKLEYTERM  LICENSING  AGREEMENT.  IF YOU DO NOT FIND THE TEXT OF    ;
;    THIS  AGREEMENT IN ANY OF THE  AFOREMENTIONED FILES,  OR IF YOU DO    ;
;    NOT HAVE THESE FILES,  YOU  SHOULD  IMMEDIATELY CONTACT BIT BUCKET    ;
;    SOFTWARE CO.  AT ONE OF THE  ADDRESSES  LISTED BELOW.  IN NO EVENT    ;
;    SHOULD YOU  PROCEED TO USE THIS FILE  WITHOUT HAVING  ACCEPTED THE    ;
;    TERMS  OF  THE  BINKLEYTERM  LICENSING  AGREEMENT,  OR  SUCH OTHER    ;
;    AGREEMENT AS YOU ARE ABLE TO REACH WITH BIT BUCKET SOFTWARE, CO.      ;
;                                                                          ;
;                                                                          ;
; You can contact Bit Bucket Software Co. at any one of the following      ;
; addresses:                                                               ;
;                                                                          ;
; Bit Bucket Software Co.        FidoNet  1:104/501, 1:343/491             ;
; P.O. Box 460398                AlterNet 7:491/0                          ;
; Aurora, CO 80046               BBS-Net  86:2030/1                        ;
;                                Internet f491.n343.z1.fidonet.org         ;
;                                                                          ;
; Please feel free to contact us at any time to share your comments about  ;
; our software and/or licensing policies.                                  ;
;--------------------------------------------------------------------------;

.xlist
        page    64,132
        
        title   Bink_Asm
        subttl  by Bob Hartman
        
        .sall
.list

;
;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                       ;;  
;;     DATA SEGMENT      ;;
;;                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;
_DATA	SEGMENT  WORD PUBLIC 'DATA'
_DATA	ENDS
;CONST	SEGMENT  WORD PUBLIC 'CONST'
;CONST	ENDS
_BSS	SEGMENT  WORD PUBLIC 'BSS'
_BSS	ENDS
;DGROUP	GROUP	CONST, _BSS, _DATA
DGROUP	GROUP	_BSS, _DATA
;
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;
;
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                       ;;  
;;     CODE SEGMENT      ;;
;;                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;
BINK_ASM_TEXT	SEGMENT  WORD PUBLIC 'CODE'

	ASSUME  CS: BINK_ASM_TEXT, DS: DGROUP, SS: DGROUP

                                
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;cProc   real_flush,<PUBLIC>
;        parmW fhandle
;
;cBegin

	public	_real_flush
_real_flush proc far
	push	bp
	mov	bp,sp

        mov     ah,45h
;       mov     bx,fhandle
	mov	bx, word ptr [bp+6]

        int     21h
        jc      rferr
        mov     bx,ax
        mov     ah,3eh
        int     21h
        xor     ax,ax
        jmp     rfout

rferr:
        mov     ax,1

rfout:

;cEnd
	pop	bp
	ret
_real_flush endp



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;cProc   dv_get_version,<PUBLIC>
;
;cBegin

	public	_dv_get_version
_dv_get_version proc far
	push	bp

        mov     cx,4445h
        mov     dx,5351h
        mov     ax,2b01h
        int     21h
        cmp     al,0ffh
        je      no_dv
        mov     ax,bx
        jmp     short got_dv

no_dv:
        xor     ax,ax

got_dv:

;cEnd
	pop	bp
	ret
_dv_get_version endp



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 
;cProc   windows_active,<PUBLIC>
;
;cBegin

	public	_windows_active
_windows_active proc far
	push	bp
 
	push	es			; Save old ES
	push	bx			; Save old BX
	mov	ax,352Fh		; We are about to clobber them
	int	21h			; DOS get vector for 2Fh

	mov	ax,es			; ES:BX = vector
	or	ax,bx			; So let's see if there is one

	jz	got_windows		; Nope, so return the zero

; Int 2f will work. So let's do it.

	mov	ax, 1600h 		; test for Windows
	int	2fh			; Go do the test
	and	ax, 007fh		; Mask off bit so 80 is same as 0

; Result is now zero if not windows and nonzero if windows. Good enough.

got_windows:

	pop	bx			; Restore old bx and es
	pop	es
 
;cEnd
	pop	bp
	ret
_windows_active endp

 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 
;cProc   mos_active,<PUBLIC>
;
;cBegin

	public	_mos_active
_mos_active proc far
	push	bp
 
        mov     ah,30h
        int     21h
        push    ax       ;save it
        mov     ax,3000h
        mov     bx,ax
        mov     cx,ax
        mov     dx,ax
        int     21h
        pop     bx
        cmp     ax,bx
        jne     got_mos
        xor     ax,ax

got_mos:
 
;cEnd
	pop	bp
	ret
_mos_active endp

 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;cProc   ddos_active,<PUBLIC>
;
;cBegin
	public	_ddos_active
_ddos_active proc far
	push	bp

        mov     ah,0e4h
        int     21h
        cmp     al,1
        je      got_ddos
        cmp     al,2
        je      got_ddos
        xor     ax,ax

got_ddos:

;cEnd
	pop	bp
	ret
_ddos_active endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;cProc   tv_get_version,<PUBLIC>
;
;cBegin

	public	_tv_get_version
_tv_get_version proc far
	push	bp

        mov     ax,1022h
        mov     bx,0
        int     15h
        cmp     bx,0
        je      no_tv
        mov     ax,bx
        jmp     short got_tv

no_tv:
        xor     ax,ax

got_tv:

;cEnd
	pop	bp
	ret
_tv_get_version endp



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;cProc   ml_active,<PUBLIC>,<ds>
; 
;cBegin 

	public	_ml_active
_ml_active proc far
	push	bp
	push	ds
 
        sub     ax,ax
        mov     ds,ax 
        mov     ax,ds:[01feh] 
        cmp     ax,0000 
        je      no_ml 
        jmp     short got_ml 
 
no_ml: 
        xor     ax,ax 
 
got_ml: 
 
;cEnd 
	pop	ds
	pop	bp
	ret
_ml_active endp



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 
;cProc   ml_pause,<PUBLIC> 
; 
;cBegin 

	public	_ml_pause
_ml_pause proc far
	push	bp
 
        mov     ah,02h 
        mov     al,00h 
        int     7fh 
 
;cEnd 
	pop	bp
	ret
_ml_pause endp


 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;cProc   dv_pause,<PUBLIC>
;
;cBegin

	public	_dv_pause
_dv_pause proc far
	push	bp

        mov     ax,101ah
        int     15h
        mov     ax,1000h
        int     15h
        mov     ax,1025h
        int     15h

;cEnd
	pop	bp
	ret
_dv_pause endp



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;cProc   tv_pause,<PUBLIC>
;
;cBegin

	public	_tv_pause
_tv_pause proc far
	push 	bp

        mov     ax,1000h
        int     15h

;cEnd
	pop	bp
	ret
_tv_pause endp



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;cProc   ddos_pause,<PUBLIC>
;
;cBegin

	public	_ddos_pause
_ddos_pause proc far
	push	bp

        int     0f4h

;cEnd
	pop	bp
	ret
_ddos_pause endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;cProc   windows_pause,<PUBLIC>
;
;cBegin

	public _windows_pause
_windows_pause proc far
	push	bp

; The good news is that this code works. The bad news is that we can not
; use it. This is because we will lose interrupts like crazy if we tell
; Windows that we are idle. Incoming 'RING's will be lost. Yoohoo's. You
; name it, it would be messed up. I can't think of any way to only do this
; some of the time -- since 'RING' is asynchronous.
;
; If you have a thirst for danger, just use this stuff.
;
	mov	ax, 1680h	; Tell Windows
	int	2fh		; we're idle

;cEnd
	pop	bp
	ret
_windows_pause endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;cProc   mos_pause,<PUBLIC>
;
;cBegin

	public _mos_pause
_mos_pause proc far
	push	bp

        mov     ax,703h
        mov     bx,3          ; Give up three ticks
        xor     cx,cx         ; Clear wait on IRQ mask
        mov     dx,cx         ; Clear ports to wait on
        int     38h

;cEnd
	pop	bp
	ret
_mos_pause endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; The idea for this code came from Holger Schurig

;cProc   msdos_pause,<PUBLIC>
;
;cBegin

	public	_msdos_pause
_msdos_pause proc far
	push	bp

        int     028h

;cEnd
	pop	bp
	ret
_msdos_pause endp



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;cProc   os2_pause,<PUBLIC>
;
;cBegin

        public  _os2_pause
_os2_pause proc far
        xor     dx,dx     ; Number of milliseconds in DX:AX. A value of 0
        mov     ax,5      ; means that the current timeslice is released.
        hlt               ; Call OS/2 DosSleep() indirectly
        db      35h,0CAh  ; Signature to differentiate between a normal
                          ; HLT-instruction and the call to DosSleep().
;cEnd
	ret
_os2_pause endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

BINK_ASM_TEXT	ENDS
         end


