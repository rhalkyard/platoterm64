	
;	Render Glyph
;	Args:
;	cx, cy (coordinates)
;	CharCode (character)
;	curfont (address of glyph data)
;	Flags: bit 7: 1 = reverse video, 0 = normal
;	bit 6: 1 = double size, 0 = normal
;	bit 5: 1 = transparent background mode, 0 = normal
;	bit 4: 1 = render in background colour with transparent background
;
;	320 x 192 screen size is assumed
;	Note: no checks are made for x >= 320 or y >= 192
;	Please add these if necessary

.include "zeropage.inc"
.include "c128.inc"
.import _curfont, _cx, _cy, _CharCode, _Flags, _FONT_SIZE_X, _FONT_SIZE_Y
.export _RenderGlyph

.define PixelOffset tmp1
.define LineCount tmp2
.define ByteExtent tmp3
.define LeftMask tmp4

.define BitmapBuffer regsave
.define ByteOffset regsave+3

.define RightMask sreg
.define MiddleMask sreg+1

SCRBASE := $0

.segment "DATA"
;RightMask:		.res 1  ; right hand mask
;MiddleMask:		.res 1	; middle mask
;ByteOffset:		.res 1  ; horizontal byte offset on screen
EORMask:		.res 1  ; EOR mask (for reverse video rendering)
;BitmapBuffer:	.res 3  ; character data buffer (in shifted position)
VDCBuffer:		.res 3	; buffer for manipulating VDC memory
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
	ldx #%11111111 ; was #%11111000
	and #$10
	beq :+
	ldx #%11111111
:
	stx EORMask
	
	jsr GetScreenAddress	; load up ScreenAddress and get pixel offset
	sta	PixelOffset

	jsr GetGlyphAddress		; figure out address of glyph data

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
	
	ora RightMask	; if character only occupies one byte, OR in the right background mask
	sta LeftMask
	
:
 	lda #$20	; check for transparent background mode
 	and _Flags
 	beq NotTransparent
 	
 	lda #$FF	; for transparent mode, we simply keep all the background bits intact
 	sta LeftMask
 	sta RightMask
 	sta MiddleMask
 	
NotTransparent:	
 	lda #192	; trim off bottom of character if it extends off the foot of the screen
	sec
	sbc _cy
	cmp LineCount	; we now account for double-height glyphs
	bcs Loop1
	sta LineCount

Loop1:
	jsr VDCSetSourceAddr
	ldy #0
	lda ByteExtent
	sta ptr3
:	jsr VDCReadByte
	sta VDCBuffer, y
	iny
	dec ptr3
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
	lda (ptr2),y
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
	sta ptr3

	ldy #0
	ldx #1

	lda #$10
	bit _Flags 	; check for background colour render
	beq NormalColour
	
	lda BitmapBuffer	; bits are already reversed
	and VDCBuffer,y
	sta VDCBuffer,y
	iny
	
	dec ptr3
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
	
	dec ptr3
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
	sta ptr3
:	lda VDCBuffer, y
	jsr VDCWriteByte
	iny
	dec ptr3
	bpl :-

	lsr DoubleFlag	; shift bit 7 into bit 6 (flag will clear on second iteration)

	dec LineCount
	beq Finished

	clc
	lda ptr1
	adc #80
	sta ptr1
	bcc :+
	inc ptr1+1
:

	bit DoubleFlag	; if bit 6 set, we just repeat the previous line using existing buffer content
	bvc :+
	jmp DoRender
:

	inc ptr2
	bne :+
	inc ptr2+1
:
	jmp Loop1
	
Finished:	
	rts
.endproc




;	Work out the address of the target line
;	adapted from the CALC routine in c64-hi.tgi
;	
;	returns with byte address in ptr1, pixel index within byte in A

.proc GetScreenAddress
        lda     _cy+1
        sta     ptr1+1
        lda     _cy
        asl
        rol     ptr1+1
        asl
        rol     ptr1+1          ; Y*4
        clc
        adc     _cy
        sta     ptr1
        lda     _cy+1
        adc     ptr1+1
        sta     ptr1+1          ; Y*4+Y=Y*5
        lda     ptr1
        asl
        rol     ptr1+1
        asl
        rol     ptr1+1
        asl
        rol     ptr1+1
        asl
        rol     ptr1+1
        sta     ptr1            ; Y*5*16=Y*80

        lda     _cx+1
        sta     ptr3
        lda     _cx
        lsr     ptr3
        ror
        lsr     ptr3
        ror
        lsr     ptr3
        ror
        clc
        adc     ptr1
        sta     ptr1
        lda     ptr1+1          ; ptr1 = Y*80+x/8
        adc     ptr3
        sta     ptr1+1
        lda     ptr1+1
        adc     #0
        sta     ptr1+1
        lda     _cx
        and     #7
        rts

.endproc


;	Work out offset into glyph data

.proc GetGlyphAddress
	lda #0
	sta ptr2+1
	lda _CharCode
	
	asl	; work out char * 6
	rol ptr2+1
	sta ptr3
	ldy ptr2+1
	sty ptr3+1
	
	asl
	rol ptr2+1
	
	clc
	adc ptr3	; add offset * 2 to offset * 4
	sta ptr2
	
	lda ptr2+1
	adc ptr3+1
	sta ptr2+1
	
	lda ptr2	; add in base address of font
	clc
	adc _curfont
	sta ptr2
	lda ptr2+1
	adc _curfont+1
	sta ptr2+1
	rts
.endproc




;	Create double width bitmap

.proc CreateDoubledBitmap
	ldy #0
	lda (ptr2),y
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
		lda		ptr1+1
        jsr     VDCWriteReg
        ldx     #VDC_DATA_LO
		lda		ptr1
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
