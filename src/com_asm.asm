.xlist
        page    64,132
        title   FOSSIL Communications routines for BinkleyTerm
        subttl  by Vince Perriello and Bob Hartman
        name    Com_Asm
        .sall
.list
;--------------------------------------------------------------------------;
;                                                                          ;
;                                                                          ;
;      ------------         Bit-Bucket Software, Co.                       ;
;      \ 10001101 /         Writers and Distributors of                    ;
;       \ 011110 /          Freely Available<tm> Software.                 ;
;        \ 1011 /                                                          ;
;         ------                                                           ;
;                                                                          ;
;  (C) Copyright 1987-90, Bit Bucket Software Co., a Delaware Corporation. ;
;                                                                          ;
;                                                                          ;
;                 Assembly routines for BinkleyTerm 2.40                   ;
;                                                                          ;
;                                                                          ;
;    For complete  details  of the licensing restrictions, please refer    ;
;    to the License  agreement,  which  is published in its entirety in    ;
;    the MAKEFILE and BT.C, and also contained in the file LICENSE.240.    ;
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
; Bit Bucket Software Co.        FidoNet  1:104/501, 1:132/491, 1:141/491  ;
; P.O. Box 460398                AlterNet 7:491/0                          ;
; Aurora, CO 80046               BBS-Net  86:2030/1                        ;
;                                Internet f491.n132.z1.fidonet.org         ;
;                                                                          ;
; Please feel free to contact us at any time to share your comments about  ;
; our software and/or licensing policies.                                  ;
;--------------------------------------------------------------------------;

        %out    FOSSIL routines for BinkleyTerm
	%out	(C) Copyright 1987-90, Bit Bucket Software Co.
;
;
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; This module was born at the hands of Wynn Wagner. He wrote the original
; code for use by OPUS-CBCS. We've rewritten and changed it a great deal.
; We added block read and write support (both Rev 4 and 5 versions) with
; backwards compatibility to Rev 3 FOSSIL drivers, and added several
; routines to support this that previously had been just Com_ calls.
;
; We owe Wynn more than we can ever repay. Thank you, Wynn.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;
;
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                       ;;  
;;     DEFINITIONS       ;;
;;                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;
MODEM    equ      14H      ; interrupt taken over by FOSSIL programs
;
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;
;
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                       ;;  
;;       EXTERNAL        ;;
;;     DECLARATIONS      ;;
;;                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;
  EXTRN   _fossil_buffer:byte
  EXTRN   _fossil_fetch_pointer:word
  EXTRN   _fossil_count:word
  EXTRN   _out_buffer:byte
  EXTRN   _out_send_pointer:word
  EXTRN   _out_count:word
  EXTRN   _carrier_mask:word
  EXTRN   _old_fossil:word
  EXTRN   _time_release:far
  EXTRN   _comm_bits:word
  EXTRN   _parity:word
  EXTRN   _stop_bits:word
  EXTRN   _ctrlc_ctr:byte
;
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;
;
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                       ;;  
;;     DATA SEGMENT      ;;
;;                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;
_DATA	SEGMENT  WORD PUBLIC 'DATA'
Port	dw	0
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
COM_ASM_TEXT	SEGMENT  WORD PUBLIC 'CODE'

	ASSUME  CS: COM_ASM_TEXT, DS: DGROUP, SS: DGROUP

                                
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: _Com_                           ;;
         ;;                                          ;;
         ;; PURPOSE: General calls to FOSSIL         ;;
         ;;                                          ;;
         ;; USAGE:   int Com_(request,parm1);        ;;
         ;;          byte request, parm1;            ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;cProc    Com_,<PUBLIC>,<si,di>
;
;         parmB    request
;         parmB    parm1
;
;cBegin

	public	_Com_
_Com_ proc far
	push	bp
	mov	bp,sp
	push	si
	push	di

;	mov	ah, request
	mov	ah, byte ptr [bp+6]

;	mov	al, parm1
	mov	al, byte ptr [bp+8]

	mov	dx, Port
	int	MODEM
	cld
                
;cEnd
	pop	di
	pop	si
	pop	bp
	ret
_Com_ endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: _ComInit                        ;;
         ;;                                          ;;
         ;; PURPOSE: Initialize the FOSSIL system    ;;
         ;;                                          ;;
         ;; USAGE:   int ComInit(WhichPort);         ;;
         ;;          byte WhichPort                  ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;cProc    ComInit,<PUBLIC>,<si,di>
;
;         parmB    WhichPort
;
;cBegin

	public	_ComInit
_ComInit proc far
        push    bp
        mov     bp,sp
	push	si
	push	di

	mov	_fossil_fetch_pointer,offset _fossil_buffer
	mov	_fossil_count,0		; Reset all buffer info
	mov	ah,4			; initialization service request
	xor	dh,dh

;	mov	dl,WhichPort
	mov	dl,byte ptr [bp+6]

	mov	Port,dx
        mov     bx,4f50h                ; setup for the ^C flag
        push    es
        push    ds
        pop     es
        mov     cx,offset _ctrlc_ctr
	int	MODEM
        pop     es

	push	ax

	mov	ax,0f01H		; Use XON/XOFF
	mov	dx,Port
	int	MODEM

	cld
	pop	ax

;cEnd
	pop	di
	pop	si
	pop	bp
	ret
_ComInit endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: _modem_in                       ;;
         ;;                                          ;;
         ;; PURPOSE: Get a character from com port   ;;
         ;;                                          ;;
         ;; USAGE:   int modem_in();                 ;;
         ;;                                          ;;
         ;; Waits if necessary, returns char in AX.  ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	public	_modem_in
_modem_in	proc far
	cmp	_fossil_count,0		; Anything there?
	je	try_again		; No, go get something.
get_one:
	dec	_fossil_count		; Lower count by one
	mov	bx,_fossil_fetch_pointer; Copy current pointer
	inc	_fossil_fetch_pointer	; Update it for next time
	mov	al,byte ptr [bx]	; Get the current character
	sub	ah,ah			; Clean up the high byte
	ret				; And back to the caller
try_again:
	call	near ptr _fill_buffer	; Fill up the local buffer
	cmp	_fossil_count,0		; See if we got anything
	jne	get_one			; Yes, go give it away
	call	far ptr _time_release	; No, give away a timeslice
	jmp	short try_again		; Then try again
_modem_in	endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: _peekbyte                       ;;
         ;;                                          ;;
         ;; PURPOSE: Non-Destructive Read, no wait   ;;
         ;;                                          ;;
         ;; USAGE:   int _peekbyte();                ;;
         ;;                                          ;;
         ;; Returns with char or 0xFFFF in AX.       ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



	public	_peekbyte
_peekbyte	proc far
	cmp	_fossil_count,0		; Anything there?
	je	tommy_can_you		; No, go get something.
see_me:
	mov	bx,_fossil_fetch_pointer; Copy current pointer
	mov	al,byte ptr [bx]	; Get the current character
	sub	ah,ah			; Clean up the high byte
	ret				; And back to the caller
tommy_can_you:
	call	near ptr _fill_buffer	; Fill up the local buffer
	cmp	_fossil_count,0		; See if we got anything
	jne	see_me			; Yes, go give it away
	mov	ax,-1			; No, then say we're empty
	ret				; Let the caller know
_peekbyte	endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: _fill_buffer (local routine)    ;;
         ;;                                          ;;
         ;; PURPOSE: get chars from FOSSIL to buffer ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_fill_buffer	proc near

        push    si
	mov	si,offset _fossil_buffer; Point to buffer
	mov	_fossil_fetch_pointer,si; Tell high level what to do
        cmp     _old_fossil,0           ; Rev 3 or Rev 4/5?
        jne     old_fill                ; Rev 3

        push    di

	mov	cx,127			; Length of buffer to read
        mov     dx,Port			; Put in the port number
	mov	ax,ds
	mov	es,ax			; Get ES set up
        mov     di,si			; Now ES:DI = DS:SI
	mov	ax,1800H		; Block Read function
        int     MODEM			; Call the FOSSIL
	mov	_fossil_count,ax	; Store the count of chars xferred

        pop     di
        pop     si
        cld
	ret	
old_fill:

	mov	ax,0C00H			; Request one character
        mov     dx, Port
        int     MODEM

        cmp     ax,-1                   ; did we get something?
        je      fbuff_done              ; no, so get out

        mov     ax,200H                 ; we got a character, so bring it in
        mov     dx,Port
        int     MODEM

	inc	_fossil_count		; one character
	mov	[si],al			; Store in buffer for consistency

fbuff_done:
        pop     si			; Restore the character
        cld
	ret	

_fill_buffer	endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: _clear_inbound                  ;;
         ;;                                          ;;
         ;; PURPOSE: Zero out modem input buffers    ;;
         ;;                                          ;;
         ;; USAGE:   void _clear_inbound();          ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	public	_clear_inbound
_clear_inbound	proc far
	mov	_fossil_fetch_pointer,offset _fossil_buffer
	mov	_fossil_count,0		; Reset all buffer info
	mov	ax,0A00H
	mov	dx, Port
	int	MODEM			; Then tell FOSSIL to do it too
	cld
	ret	

_clear_inbound	endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: _clear_outbound                 ;;
         ;;                                          ;;
         ;; PURPOSE: Zero modem output buffers       ;;
         ;;                                          ;;
         ;; USAGE:   void _clear_outbound();         ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	public	_clear_outbound
_clear_outbound	proc far
	mov	_out_send_pointer,offset _out_buffer
	mov	_out_count,0		; Reset all buffer info
	mov	ax,0900H
	mov	dx, Port
	int	MODEM			; Then tell FOSSIL to do it too
	cld
	ret	

_clear_outbound	endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: _modem_status                   ;;
         ;;                                          ;;
         ;; PURPOSE: Return FOSSIL and buffer status ;;
         ;;                                          ;;
         ;; USAGE:   int _modem_status();            ;;
         ;;                                          ;;
         ;; Returns with status mask in AX.          ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	public	_modem_status
_modem_status	proc far
	mov	ax,300H
	mov	dx, Port
	int	MODEM			; Get low level FOSSIL status
	cmp	_fossil_count,0		; Do we have characters here?
	je	nothing_here		; No
	or	ax,256			; Yes, OR in the DATA_READY bit
nothing_here:
	cld
	ret				; Status back to the caller

_modem_status	endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: _do_break                       ;;
         ;;                                          ;;
         ;; PURPOSE: Start or Stop a BREAK signal    ;;
         ;;                                          ;;
         ;; USAGE:   void _do_break (0/1);           ;;
         ;;                                          ;;
         ;; Starts break if 1, stops if 0.           ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;cProc	do_break,<PUBLIC>
;	parmW	on_or_off
;
;cBegin

	public	_do_break
_do_break proc far
	push	bp
	mov	bp,sp

;
;       mov     ax,on_or_off            ; get on or off status
	mov	ax,word ptr [bp+6]	; get on or off status

	mov	ah,1AH                  ; FOSSIL break call
	mov	dx, Port                ; port to use
	int	MODEM			; Do it

;cEnd
	pop	bp
	ret
_do_break endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: mdm_enable                      ;;
         ;;                                          ;;
         ;; PURPOSE: Reset buffers and baud rate     ;;
         ;;                                          ;;
         ;; USAGE:   void mdm_enable(baud_mask);     ;;
         ;;          word baud_mask;                 ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;cProc	mdm_enable,<PUBLIC>
;	parmW	BaudRate
;
;cBegin

	public	_mdm_enable
_mdm_enable proc far
	push	bp
	mov	bp,sp

	mov	_fossil_fetch_pointer,offset _fossil_buffer
	mov	_fossil_count,0		; Reset buffers here

;	mov	ax,BaudRate		; Get user's baud rate
	mov	ax,word ptr [bp+6]	; Get user's baud rate

        or      ax,_comm_bits           ; add in bit settings
        or      ax,_parity              ; and parity settings
        or      ax,_stop_bits           ; and stop bits
	sub	ah,ah			; Function ZERO sets baudrate
	mov	dx, Port		; Use the current port
	int	MODEM			; Then tell FOSSIL to do it
	cld
;cEnd
	pop	bp
	ret
_mdm_enable endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: _mdm_disable                    ;;
         ;;                                          ;;
         ;; PURPOSE: Turn off FOSSIL on current port ;;
         ;;                                          ;;
         ;; USAGE:   void mdm_disable();             ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	public	_mdm_disable
_mdm_disable	proc far

	mov	_fossil_fetch_pointer,offset _fossil_buffer
	mov	_fossil_count,0		; Reset buffers here
	mov	ax,500H			; Function FIVE turns off port
	mov	dx, Port		; Use the current port
	int	MODEM			; Then tell FOSSIL to do it
	cld
	ret	
_mdm_disable	endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: _unbuffer_bytes                 ;;
         ;;                                          ;;
         ;; PURPOSE: Flush local and FOSSIL buffers  ;;
         ;;                                          ;;
         ;; USAGE:   void _unbuffer_bytes();         ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


        public  _unbuffer_bytes
_unbuffer_bytes proc far

        cmp     _out_count,0
        je      all_done
        mov     ax,1
        push    ax
        push    _out_count
        mov     ax,offset _out_buffer
        push    ax
        call    far ptr _sendchars
        add     sp,6
        mov     _out_count,0                            ; clear chars
        mov     _out_send_pointer,offset _out_buffer    ; reset pointer
all_done:
        ret

_unbuffer_bytes endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: _buffer_byte                    ;;
         ;;                                          ;;
         ;; PURPOSE: Store byte, block write if full ;;
         ;;                                          ;;
         ;; USAGE:   void _buffer_byte(asc);         ;;
         ;;          byte asc;                       ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	public	_buffer_byte
_buffer_byte    proc far
        push    bp
        mov     bp,sp

	cmp	_out_count,128          ; Is it full
	je	flush_out 	        ; Yes, so flush it out
looks_good:
	inc	_out_count		; Increment count by one
	mov	bx,_out_send_pointer    ; Copy current pointer
	inc	_out_send_pointer	; Update it for next time

;	mov	ax, asc
        mov     ax,[bp+6]

	mov	byte ptr [bx],al        ; Get the current character
        mov     sp,bp
        pop     bp
	ret				; And back to the caller
flush_out:
        mov     ax,1
        push    ax
        push    _out_count
        mov     ax,offset _out_buffer
        push    ax
        call    far ptr _sendchars
        add     sp,6
        mov     _out_count,0                            ; clear chars
        mov     _out_send_pointer,offset _out_buffer    ; reset pointer
	jmp	short looks_good	; Then buffer this character
_buffer_byte    endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: sendbyte                        ;;
         ;;                                          ;;
         ;; PURPOSE: Flush buffer, send one byte     ;;
         ;;                                          ;;
         ;; USAGE:   void sendbyte(asc);             ;;
         ;;          byte asc;                       ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;cProc	sendbyte,<PUBLIC>
;	parmW	Char
;
;cBegin

	public	_sendbyte
_sendbyte proc far
	push	bp
	mov	bp,sp

try_to_send:
        cmp     _out_count,0             ; anything here to send
        je      send_normal             ; nope, so do just one char
        mov     ax,1
        push    ax
        push    _out_count
        mov     ax,offset _out_buffer
        push    ax
        call    far ptr _sendchars
        add     sp,6
        mov     _out_count,0 
	mov	_out_send_pointer,offset _out_buffer

send_normal:
;	mov	ax,Char			; Get character
	mov	ax,word ptr [bp+6]	; Get character

	mov	ah,0BH			; Write char, no wait
	mov	dx,Port			; To current Port
	int	MODEM			; Tell FOSSIL to do it
	or	ax,ax			; Were we successful?
	jne	sent_it			; Yes, get out
	call	far ptr _time_release	; No, give away a timeslice
	jmp	short try_to_send	; Then try again
sent_it:

;cEnd
	pop	bp
	ret
_sendbyte endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: sendchars                       ;;
         ;;                                          ;;
         ;; PURPOSE: Explicit Block Write routine    ;;
         ;;                                          ;;
         ;; USAGE:   void sendchars(str,len,car);    ;;
         ;;          char *str;  /* characters */    ;;
         ;;          unsigned len; /* how many   */  ;;
         ;;          int car;    /* 1=check DCD*/    ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;cProc	sendchars,<PUBLIC>
;	parmW	String
;        parmW   StrLen
;        parmW   CarChek
;
;cBegin

	public	_sendchars
_sendchars proc far
	push	bp
	mov	bp,sp


	push	di
	push	si

;	mov	si,String		; Pointer to string
	mov	si,word ptr [bp+6]	; Pointer to string

;	mov	di,StrLen		; StrLen of string (for Rev 3)
	mov	di,word ptr [bp+8]	; StrLen of string (for Rev 3)

wbloop:

        cmp     _old_fossil,0           ; Rev 3 FOSSIL?
        jne     wbloop2                 ; Yes, go do it that way.

        mov     ax,ds			; Copy DS
        mov     es,ax			; Into ES
	mov	di,si			; Now DS:SI and ES:DI equal.

;	mov	bx,StrLen		; StrLen of string
	mov	bx,word ptr [bp+8]	; StrLen of string

	mov	cx,bx			; Copy count
	mov	ax,1900H		; Write block is 19H.
	mov	dx,Port			; To current Port

	push	si			; Save this in case

	int	MODEM			; Call the FOSSIL

	pop	si			; Restore original pointer
	
	sub	bx,ax			; Subtract number copied
	cmp	bx,0			; Is it zero?
	je	write_block_done	; Yes, exit.

	add	si,ax			; Bump the pointer

;	mov	StrLen,bx		; Save current length
	mov	word ptr [bp+8],bx	; Save current length

wbwait:
	call	far ptr _time_release	; Give up a slice

;	cmp	CarChek,0		; Do we want to do carrier check?
	cmp	word ptr [bp+10],0	; Do we want to do carrier check?

	je	wbloop			; No, loop back now
	mov	ax,300H			; Status call is THREE.
	mov	dx,Port
	int	MODEM			; Do status call.
	mov	bx,word ptr _carrier_mask; Get carrier mask
	test	ax,bx		 	; Check for carrier
	jne	wbloop			; Still there, continue.
	jmp	short write_block_done	; Lost carrier, exit


wbloop2:
	mov	al,byte ptr [si]	; Get character
	mov	ah,0BH			; Write char, no wait
	mov	dx,Port			; To current Port
	int	MODEM			; Tell FOSSIL to do it
	or	ax,ax			; Were we successful?
	je	wbwait          	; No, do time waste processing

	dec	di			; Lower count by 1
	inc	si			; Move pointer by 1
	cmp	di,0			; All done yet?
	jne	wbloop2			; No, keep plugging away.

write_block_done:

	pop	si			; Restore old registers
	pop	di

;cEnd

	pop	bp
	ret
_sendchars endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: fossil_block                    ;;
         ;;                                          ;;
         ;; PURPOSE: Generic Block Read/Write code   ;;
         ;;                                          ;;
         ;; USAGE:   int fossil_block(cnum,pntr,len);;;
         ;;          int cnum;    /* Call number */  ;;
         ;;          far *pntr;   /* loc of buff */  ;;
         ;;          int len;     /* how many    */  ;;
         ;;                                          ;;
         ;; Returns with status mask in AX.          ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;cProc    fossil_block,<PUBLIC>
;        parmW   cnum
;        parmD   pntr
;        parmW   len
;
;cBegin

	public	_fossil_block
_fossil_block proc far
	push	bp
	mov	bp,sp

        push    si
        push    di

;	MOV	CX,len			; StrLen of buffer to read
	mov	cx,word ptr [bp+12]	; StrLen of buffer to read

;	LES	DI,pntr			; Get ES:DI set up
	LES	DI,dword ptr [bp+8]	; Get ES:DI set up

;	LDS	SI,pntr			; Just in case it is rev 4
	LDS	SI,dword ptr [bp+8]	; Just in case it is rev 4

;	MOV	AX,cnum			; Call number
	MOV	AX,word ptr [bp+6]	; Call number

	MOV	DX,Port			; Put in the port number
	int	MODEM			; Call the FOSSIL
        pop     di
        pop     si

;cEnd
	pop	bp
	ret
_fossil_block endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


         page

         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: _fossil_gotoxy                  ;;
         ;;                                          ;;
         ;; PURPOSE: Send the local cursor to a      ;;
         ;;          screen location.  Column and    ;;
         ;;          row are both 0-based values.    ;;
         ;;                                          ;;
         ;; USAGE:   void fossil_gotoxy(column,row); ;;
         ;;          unsigned char column, row;      ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;cProc    fossil_gotoxy,<PUBLIC>
;
;         parmB    column
;         parmB    row
;
;cBegin

	public	_fossil_gotoxy
_fossil_gotoxy proc far
	push	bp
	mov	bp,sp

;	mov	dh, row
	mov	dh, byte ptr [bp+8]

;	mov	dl, column
	mov	dl, byte ptr [bp+6]

	mov	ah, 11h
	int	MODEM
	cld
                
;cEnd
	pop	bp
	ret
_fossil_gotoxy endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: _fossil_whereami                ;;
         ;;                                          ;;
         ;; PURPOSE: Return the current screen       ;;
         ;;          location as two nybbles.        ;;
         ;;                                          ;;
         ;; USAGE:   int fossil_whereami(void);      ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;cProc    fossil_whereami,<PUBLIC>
;
;cBegin

	public	_fossil_whereami
_fossil_whereami proc far

	mov	ah, 12h
	int	MODEM
	mov	ax, dx
	cld
                
;cEnd
	ret
_fossil_whereami endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: _fossil_wherey                  ;;
         ;;                                          ;;
         ;; PURPOSE: Return the current column on    ;;
         ;;          the local monitor.              ;;
         ;;                                          ;;
         ;; USAGE:   int wherey(void);               ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;cProc    fossil_wherey,<PUBLIC>
;
;cBegin

	 public	_fossil_wherey
_fossil_wherey proc far

	mov	ah, 12h
	int	MODEM
	mov	al, dh
	xor	ah, ah
	cld
                
;cEnd
	ret
_fossil_wherey endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

         page
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
         ;;                                          ;;
         ;; ROUTINE: _fossil_wherex                  ;;
         ;;                                          ;;
         ;; PURPOSE: Return the current row on the   ;;
         ;;          local monitor.                  ;;
         ;;                                          ;;
         ;; USAGE:   int wherex(void);               ;;
         ;;                                          ;;
         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;cProc    fossil_wherex,<PUBLIC>
;
;cBegin

	public	_fossil_wherex
_fossil_wherex proc far

	mov	ah, 12h
	int	MODEM
	mov	al, dl
	xor	ah, ah
	cld
                
;cEnd
	ret
_fossil_wherex endp


COM_ASM_TEXT	ENDS
        end

;; END OF FILE: Com_Asm.Asm ;;

