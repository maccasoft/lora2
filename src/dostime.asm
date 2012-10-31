.xlist
        page    64,132
        
        title   DosTime
        subttl  by Wynn Wagner III
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
;                                                                          ;
;  This module is based largely on a similar module in OPUS-CBCS V1.03b.   ;
;  The original work is (C) Copyright 1987, Wynn Wagner III. The original  ;
;  author has graciously allowed us to use his code in this work.          ;
;                                                                          ;
;--------------------------------------------------------------------------;
        
        name    DosDate
        
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
DOSTIME_ASM_TEXT	SEGMENT  WORD PUBLIC 'CODE'

	ASSUME  CS: DOSTIME_ASM_TEXT, DS: DGROUP, SS: DGROUP


;
;        --------------------------------------------------------------------
;        void dostime(&hour,&min,&sec,&ths);
;
;        int hour;         0-23  military time
;        int min;	   0-59
;        int sec;          0-59
;        int ths;          0-99
;        --------------------------------------------------------------------
;cProc    dostime,<PUBLIC>,<di>
;
;         parmW    Hour
;         parmW    Minutes
;         parmW    Seconds
;         parmW    Thousandths
;
;cBegin

	public	_dostime
_dostime proc far
	push	bp
	mov	bp,sp
	push	di

	mov	ah,2cH			;DOS 'Get Time' function
	int	21H			;Call DOS

	mov	al,ch
	xor	ah,ah
	mov	di, word ptr [bp+6]	;Address of Hour
	mov	[di],ax

	mov	bl,cl
	xor	bh,bh
	mov	di, word ptr [bp+8]	;Address of Minutes
	mov	[di],bx

	mov	cl,dh
	xor	ch,ch
	mov	di, word ptr [bp+10]	;Address of Seconds
	mov	[di],cx

	xor	dh,dh
	mov	di, word ptr [bp+12]	;Address of Thousandths
	mov	[di],dx

;cEnd
	pop	di
	pop	bp
	ret
_dostime endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;        --------------------------------------------------------------------
;        void dosdate(&month,&mday,&year,&wday);
;
;        int month;	   1-12
;        int mday;	   1-31	 day of month
;        int year;	   1980-2099
;        int wday;	   0-6   day of week (0=Sun,6=Sat)
;        --------------------------------------------------------------------
;cProc    dosdate,<PUBLIC>,<di>
;
;         parmW    Month
;         parmW    Mday
;         parmW    Year
;         parmW    Wday
;
;cBegin

	public	_dosdate
_dosdate proc far
	push	bp
	mov	bp,sp
	push	di

	mov	ah,2aH			;DOS 'Get Date' function
	int	21H			;Call DOS

	mov	bl,dh
	xor	bh,bh
	mov	di, word ptr [bp+6]	;Address of Month
	mov	[di],bx

	xor	dh,dh
	mov	di, word ptr [bp+8]	;Address of Day-of-Month
	mov	[di],dx

	mov	di, word ptr [bp+10]	;Address of Year
	mov	[di],cx

	xor	ah,ah
	mov	di, word ptr [bp+12]	;Address of Day-of-Week
	mov	[di],ax
;cEnd
	pop	di
	pop	bp
	ret
_dosdate endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                  
DOSTIME_ASM_TEXT	ENDS
         end


