
;	Render Glyph
;	Args:
;	cx, cy (coordinates)
;	CharPtr (address of glyph)
;	Flags: bit 7: 1 = reverse video, 0 = normal
;	bit 6: 1 = double size, 0 = normal
;	bit 5: 1 = transparent background mode, 0 = normal
;	bit 4: 1 = render in background colour with transparent background
;
;	Note: only minimal bounds checks are made

.include "zeropage.inc"
.include "c128.inc"
.import _cx, _cy, _CharPtr, _Flags, _FONT_SIZE_X, _FONT_SIZE_Y, _vdcmode
.export _RenderGlyph

.define PixelOffset tmp1
.define LineCount tmp2
.define ByteExtent tmp3
.define LeftMask tmp4

.define VidPtr ptr1
.define FontPtr ptr2
.define zptemp ptr3			; temporary single-byte variable
.define VDCBuffer ptr3+1	; 3 bytes - spills into ptr4

.define BitmapBuffer regsave ; 3 bytes
.define ByteOffset regsave+3

.define RightMask sreg
.define MiddleMask sreg+1

.segment "DATA"
;RightMask:		.res 1  ; right hand mask
;MiddleMask:		.res 1	; middle mask
;ByteOffset:		.res 1  ; horizontal byte offset on screen
EORMask:		.res 1  ; EOR mask (for reverse video rendering)
;BitmapBuffer:	.res 3  ; character data buffer (in shifted position)
;VDCBuffer:		.res 3	; buffer for manipulating VDC memory
DoubleFlag:		.res 1	; repeat line buffer flag
LeftBit:		.res 1 	; Left bit to shift in for AND mode
.code

.proc _RenderGlyph
	lda _Flags
	asl
	asl
	asl
	and #$80
	sta LeftBit	; if we're rendering black-on-white, bitmask needs to have 1 shifted into top bit

	ldx #0
	stx DoubleFlag	; repeated buffer flag for double-height glyphs
	stx MiddleMask

	lda _Flags
	and #$90
	beq :+
	and #$10
	beq :+
	ldx #%11111111
:
	stx EORMask

	jsr GetScreenAddress	; load up ScreenAddress and get pixel offset
	stx	PixelOffset

	lda _CharPtr
	sta FontPtr
	lda _CharPtr+1
	sta FontPtr+1

	lda _FONT_SIZE_Y
	bit _Flags
	bvc :+
	asl	; double pixel height if we're in double size mode
:	sta  LineCount
	lda _FONT_SIZE_X
	bit _Flags
	bvc :+
	asl	; double pixel width if we're in double size mode
:

	clc
	adc PixelOffset	; calculate total pixel extent including x offset
	pha
	and #$07
	tax
	lda RightBGMaskTable,x
	sta RightMask
	pla
	lsr
	lsr
	lsr
	sta ByteExtent

	ldx PixelOffset	; set up left and right background masks
	lda LeftBGMaskTable,x
	sta LeftMask

	ldx ByteExtent
	bne :+

	; TODO: is this needed with 8-pixel characters?
	; ora RightMask	; if character only occupies one byte, OR in the right background mask
	; sta LeftMask

:
 	lda #$20	; check for transparent background mode
 	and _Flags
 	beq NotTransparent

 	lda #$FF	; for transparent mode, we simply keep all the background bits intact
 	sta LeftMask
 	sta RightMask
 	sta MiddleMask

NotTransparent:
	bit _vdcmode	; skip trimming
	bmi trim_hires

 	lda #192	; trim off bottom of character if it extends off the foot of the screen
	sec
	sbc _cy
	cmp LineCount	; we now account for double-height glyphs
	bcs Loop1
	sta LineCount
	bcc Loop1		; always taken

trim_hires:
	lda _cy+1
	beq Loop1

	lda #<480
	sec
	sbc _cy
	cmp LineCount
	bcs Loop1
	sta LineCount

Loop1:
	jsr VDCSetSourceAddr
	ldy #0
	lda ByteExtent
	sta zptemp
:	jsr VDCReadByte
	sta VDCBuffer, y
	iny
	dec zptemp
	bpl :-

	ldx #0
	lda #$10
	bit _Flags
	beq :+
	dex
:
	stx BitmapBuffer
	stx BitmapBuffer+1
	stx BitmapBuffer+2

	bit _Flags
	bvc NormalSize
	lda #$80
	sta DoubleFlag
	jsr CreateDoubledBitmap
	jmp DoShift

NormalSize:
	ldy #0
	lda (FontPtr),y
	eor EORMask

DoShift:
	ldx PixelOffset
	beq NoShift
:
	lsr
	ora LeftBit
	ror BitmapBuffer+1
	ror BitmapBuffer+2
	dex
	bne :-
NoShift:
	sta BitmapBuffer	; bitmap is now in desired x position

DoRender:
	lda ByteExtent
	sta zptemp

	ldy #0
	ldx #1

	lda #$10
	bit _Flags 	; check for background colour render
	beq NormalColour

	lda BitmapBuffer	; bits are already reversed
	and VDCBuffer,y
	sta VDCBuffer,y
	iny

	dec zptemp
	bmi Done
	beq LastByte2

	lda BitmapBuffer+1
	and VDCBuffer,y
	sta VDCBuffer,y
	iny
	inx

LastByte2:
	lda BitmapBuffer,x
	and VDCBuffer,y
	sta VDCBuffer,y
	jmp Done

NormalColour:
	lda VDCBuffer,y
	and LeftMask
	ora BitmapBuffer
	sta VDCBuffer,y
	iny

	dec zptemp
	bmi Done
	beq LastByte

	lda VDCBuffer,y
	and MiddleMask
	ora BitmapBuffer+1
	sta VDCBuffer,y
	iny
	inx

LastByte:
	lda VDCBuffer,y
	and RightMask
	ora BitmapBuffer,x
	sta VDCBuffer,y
Done:
	jsr VDCSetSourceAddr
	ldy #0
	lda ByteExtent
	sta zptemp
:	lda VDCBuffer, y
	jsr VDCWriteByte
	iny
	dec zptemp
	bpl :-

	lsr DoubleFlag	; shift bit 7 into bit 6 (flag will clear on second iteration)

	dec LineCount
	beq Finished

	bit _vdcmode	; 0 = 640x200, 0x80 = 640x480
	bpl nextrow

	; 640x480 mode is interlaced, with even and odd fields stored as separate bitmaps:
	; 0 2 4 6 8 ... 480 1 3 5 7 ... 479

	; subtract 21360 bytes (odd field offset) from bitmap pointer but don't save
	; it just yet
	sec
	lda VidPtr
	sbc #<21360
	pha
	lda VidPtr+1
	sbc #>21360
	bpl @odd

	; VidPtr < 21360: video pointer was in an even field. Next field is odd.
	pla				; discard subtraction result
	clc
	lda VidPtr
	adc #<21360
	sta VidPtr
	lda VidPtr+1
	adc #>21360
	sta VidPtr+1
	jmp samerow		; same row but in odd-field bitmap

@odd:
	; VidPtr >= 21360: video pointer was in an odd field, next field is even.
	sta VidPtr+1		; save subtraction result
	pla
	sta VidPtr
	; move on to the next bitmap row.

nextrow:
	clc
	lda VidPtr
	adc #80
	sta VidPtr
	bcc samerow
	inc VidPtr+1
samerow:

	bit DoubleFlag	; if bit 6 set, we just repeat the previous line using existing buffer content
	bvc :+
	jmp DoRender
:

	inc FontPtr
	bne :+
	inc FontPtr+1
:
	jmp Loop1

Finished:
	rts
.endproc




;	Work out the address of the target line
;	adapted from the CALC routine in c64-hi.tgi
;
;	returns with byte address in VidPtr, pixel index within byte in A
.proc GetScreenAddress
		bit		_vdcmode
		bpl		lores

; 640x480 interlaced mode is implemented as two 640x240 bitmaps for even and
; odd fields. Temporarily halve Y coordinate and calculate bitmap offset first
        lda     _cy
        pha
        lda     _cy+1
        pha
        lsr
        ror     _cy              ; Y=Y/2

lores:
        sta     _cy+1
        sta     VidPtr+1
        lda     _cy
        asl
        rol     VidPtr+1
        asl
        rol     VidPtr+1          ; Y*4

        clc
        adc     _cy
        sta     VidPtr
        lda     _cy+1
        adc     VidPtr+1
        sta     VidPtr+1          ; Y*4+Y=Y*5

        lda     VidPtr
        asl
        rol     VidPtr+1
        asl
        rol     VidPtr+1
        asl
        rol     VidPtr+1
        asl
        rol     VidPtr+1
        sta     VidPtr            ; Y*5*16=Y*80

        lda     _cx+1
        sta     zptemp
        lda     _cx
        lsr     zptemp
        ror
        lsr     zptemp
        ror
        lsr     zptemp
        ror
        clc
        adc     VidPtr
        sta     VidPtr
        lda     VidPtr+1          ; VidPtr = Y*80+x/8
        adc     zptemp
        sta     VidPtr+1

		bit		_vdcmode
		bpl		@even

; interlaced mode: restore original Y coordinate value and apply offset for
; odd-field bitmap if needed
        pla
        sta     _cy+1
        pla
        sta     _cy
        and     #1
        beq     @even           ; even line - no offset
        lda     VidPtr
        clc
        adc     #<21360
        sta     VidPtr
        lda     VidPtr+1
        adc     #>21360
        sta     VidPtr+1          ; odd lines are 21360 bytes farther

@even:  lda     _cx
        and     #7
        tax
        rts
.endproc


;	Create double width bitmap

.proc CreateDoubledBitmap
	ldy #0
	lda (FontPtr),y
	eor EORMask

	ldx #7	; loop for each pixel
:
	lsr	; shift rightmost bit into c
	php	; save state of carry
	ror BitmapBuffer
	ror BitmapBuffer+1	; this leaves c = 0
	plp	; retrieve carry
	ror BitmapBuffer
	ror BitmapBuffer+1
	dex
	bpl :-
	lda BitmapBuffer
	rts
.endproc

.rodata

LeftBGMaskTable:	; 	LUT for left hand mask
	.byte %00000000
	.byte %10000000
	.byte %11000000
	.byte %11100000
	.byte %11110000
	.byte %11111000
	.byte %11111100
	.byte %11111110


RightBGMaskTable:	; LUT for right hand mask
	.byte %11111111
	.byte %01111111
	.byte %00111111
	.byte %00011111
	.byte %00001111
	.byte %00000111
	.byte %00000011
	.byte %00000001

VDCSetSourceAddr:
        ldx     #VDC_DATA_HI
		lda		VidPtr+1
        jsr     VDCWriteReg
        ldx     #VDC_DATA_LO
		lda		VidPtr
        jmp		VDCWriteReg

VDCReadByte:
        ldx     #VDC_RAM_RW
VDCReadReg:
        stx     VDC_INDEX
@L0:    bit     VDC_INDEX
        bpl     @L0
        lda     VDC_DATA
        rts

VDCWriteByte:
        ldx     #VDC_RAM_RW
VDCWriteReg:
        stx     VDC_INDEX
@L0:    bit     VDC_INDEX
        bpl     @L0
        sta     VDC_DATA
        rts




;; ;	you can probably move some of the most used absolute address variables into ptr4 and tmp1-tmp4

;; PixelOffset		.ds 1	; pixel offset into leftmost byte
;; cy				.ds 1	; y coord
;; cx 				.ds 2	; x coord
;; LineCount		.ds 1	; line counter
;; CharCode		.ds 1	; character to display
;; curfont		.ds 2	; base address of character data
;; Flags			.ds 1	; bit 7: 1 = reverse video, 0 = normal; bit 6: 1 = double size, 0 = normal
;; ByteExtent		.ds 1	; number of bytes fully or partially occupied by glyph on screen
;; LeftMask		.ds 1	; left hand mask
;; RightMask		.ds 1	; right hand mask
;; MiddleMask		.ds 1	; middle mask
;; ByteOffset		.ds 1	; horizontal byte offset on screen
;; EORMask			.ds 1	; EOR mask (for reverse video rendering)
;; BitmapBuffer	.ds 3	; character data buffer (in shifted position)
;; DoubleFlag		.ds 1	; repeat line buffer flag
;; LeftBit			.ds 1	; bit to shift into rotated bitmask

;; 	run Start
