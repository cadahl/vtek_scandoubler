; -------------------------
;  VTek ScanDoubler Rev. 1
;     DUBLINA firmware
;  (c)2026 VTek/Carl Ådahl
; -------------------------
; Control routines for TMS4C2971 field memory

.macro fmem_init
    ; Clobbers:
    ;	reg_temp

    ; TMS4C2971 control pins (active high)
    ldi     reg_temp, PORTD_FMEM_OE_BM | PORTD_FMEM_RE_BM | PORTD_FMEM_RSTR_BM | PORTD_FMEM_RSTW_BM | PORTD_FMEM_WE_BM | PORTD_FMEM_IE_BM
    out     VPORTD_OUT, reg_zero
    sts     PORTD_DIRSET, reg_temp
.endm

; Prepare the address registers for a read from reg_rline_h/l
.macro fmem_prepare_read_address
    ; Inputs:
    ;	reg_rline_h, reg_rline_l
    ; Outputs:
    ;   reg_A1-reg_A12
    ; Clobbers:
    ;	reg_temp, reg_temp2, reg_temp3, reg_temp4

    ; The TMS4C2971 has a block size of 80 pixels. We make the block address
    ; wline * 10, which means every line has room for 800 pixels.
    mov	    reg_temp, reg_line_l
    mov	    reg_temp2, reg_line_h

    lsl	    reg_temp
    rol	    reg_temp2

    mov	    reg_temp3, reg_temp
    mov	    reg_temp4, reg_temp2

    lsl	    reg_temp
    rol	    reg_temp2

    lsl	    reg_temp
    rol	    reg_temp2

    add	    reg_temp, reg_temp3
    adc	    reg_temp2, reg_temp4    ; x * 10 == (x << 3) + (x << 1)

    ldi     reg_A12, PORTD_FMEM_RSTR_BM
    bst     reg_temp2, 4
    bld     reg_A12, PORTD_FMEM_OE_PIN

    clr     reg_A11
    bst     reg_temp2, 3
    bld     reg_A11, PORTD_FMEM_OE_PIN

    clr     reg_A10
    bst     reg_temp2, 2
    bld     reg_A10, PORTD_FMEM_OE_PIN

    clr     reg_A9
    bst     reg_temp2, 1
    bld     reg_A9, PORTD_FMEM_OE_PIN

    clr     reg_A8
    bst     reg_temp2, 0
    bld     reg_A8, PORTD_FMEM_OE_PIN

    clr     reg_A7
    bst     reg_temp, 7
    bld     reg_A7, PORTD_FMEM_OE_PIN

    clr     reg_A6
    bst     reg_temp, 6
    bld     reg_A6, PORTD_FMEM_OE_PIN

    clr     reg_A5
    bst     reg_temp, 5
    bld     reg_A5, PORTD_FMEM_OE_PIN

    clr     reg_A4
    bst     reg_temp, 4
    bld     reg_A4, PORTD_FMEM_OE_PIN

    clr     reg_A3
    bst     reg_temp, 3
    bld     reg_A3, PORTD_FMEM_OE_PIN

    clr     reg_A2
    bst     reg_temp, 2
    bld     reg_A2, PORTD_FMEM_OE_PIN

    clr     reg_A1
    bst     reg_temp, 1
    bld     reg_A1, PORTD_FMEM_OE_PIN

    ; No need for reg_A0 since we have shifted a zero into that position.
.endm

; Prepare the address registers for a write to reg_wline_h/l.
.macro fmem_prepare_write_address
    ; Inputs:
    ;	reg_wline_h, reg_wline_l
    ; Outputs:
    ;   reg_A1-reg_A12
    ; Clobbers:
    ;	reg_temp, reg_temp2, reg_temp3, reg_temp4

    ; The TMS4C2971 has a block size of 80 pixels. We make the block address
    ; wline * 10, which means every line has room for 800 pixels.
    mov	    reg_temp, reg_line_l
    mov	    reg_temp2, reg_line_h

    add	    reg_temp, reg_one
    adc	    reg_temp2, reg_zero

    lsl	    reg_temp
    rol	    reg_temp2

    mov	    reg_temp3, reg_temp
    mov	    reg_temp4, reg_temp2

    lsl	    reg_temp
    rol	    reg_temp2

    lsl	    reg_temp
    rol	    reg_temp2

    add	    reg_temp, reg_temp3
    adc	    reg_temp2, reg_temp4    ; x * 10 == (x << 3) + (x << 1)

    ldi     reg_A12, PORTD_FMEM_RSTW_BM
    bst     reg_temp2, 4
    bld     reg_A12, PORTD_FMEM_IE_PIN

    clr     reg_A11
    bst     reg_temp2, 3
    bld     reg_A11, PORTD_FMEM_IE_PIN

    clr     reg_A10
    bst     reg_temp2, 2
    bld     reg_A10, PORTD_FMEM_IE_PIN

    clr     reg_A9
    bst     reg_temp2, 1
    bld     reg_A9, PORTD_FMEM_IE_PIN

    clr     reg_A8
    bst     reg_temp2, 0
    bld     reg_A8, PORTD_FMEM_IE_PIN

    clr     reg_A7
    bst     reg_temp, 7
    bld     reg_A7, PORTD_FMEM_IE_PIN

    clr     reg_A6
    bst     reg_temp, 6
    bld     reg_A6, PORTD_FMEM_IE_PIN

    clr     reg_A5
    bst     reg_temp, 5
    bld     reg_A5, PORTD_FMEM_IE_PIN

    clr     reg_A4
    bst     reg_temp, 4
    bld     reg_A4, PORTD_FMEM_IE_PIN

    clr     reg_A3
    bst     reg_temp, 3
    bld     reg_A3, PORTD_FMEM_IE_PIN

    clr     reg_A2
    bst     reg_temp, 2
    bld     reg_A2, PORTD_FMEM_IE_PIN

    clr     reg_A1
    bst     reg_temp, 1
    bld     reg_A1, PORTD_FMEM_IE_PIN

    ; No need for reg_A0 since we have shifted a zero into that position.
.endm

; Reset the read side of the field memory and transfer the read address.
.macro fmem_init_read
    ; Inputs:
    ;   reg_A1-reg_A12
    ; Clobbers:
    ;   reg_temp
    ldi     reg_temp, PORTD_FMEM_RSTR_BM | PORTD_FMEM_RE_BM
    out     VPORTD_OUT, reg_temp                            ; RSTR=1 OE=0   RE=1 First clock
    ldi     reg_temp, PORTD_FMEM_OE_BM | PORTD_FMEM_RE_BM   ; RSTR=1 OE=0   RE=1 Second clock
    out     VPORTD_IN, reg_temp                             ; RSTR=1 OE=1   RE=0 Third clock
    nop                                                     ; RSTR=1 OE=1   RE=0 Fourth clock
    out     VPORTD_OUT, reg_A12                             ; RSTR=0 OE=A12 RE=0 Fifth clock
    out     VPORTD_OUT, reg_A11                             ; RSTR=0 OE=A11 RE=0 Sixth clock
    out     VPORTD_OUT, reg_A10                             ; RSTR=0 OE=A10 RE=0 Seventh clock
    out     VPORTD_OUT, reg_A9                              ; RSTR=0 OE=A9  RE=0 Eighth clock
    out     VPORTD_OUT, reg_A8                              ; RSTR=0 OE=A8  RE=0 Ninth clock
    out     VPORTD_OUT, reg_A7                              ; RSTR=0 OE=A7  RE=0 Tenth clock
    out     VPORTD_OUT, reg_A6                              ; RSTR=0 OE=A6  RE=0 Eleventh clock
    out     VPORTD_OUT, reg_A5                              ; RSTR=0 OE=A5  RE=0 Twelfth clock
    out     VPORTD_OUT, reg_A4                              ; RSTR=0 OE=A4  RE=0 Thirteenth clock
    out     VPORTD_OUT, reg_A3                              ; RSTR=0 OE=A3  RE=0 Fourteenth clock
    out     VPORTD_OUT, reg_A2                              ; RSTR=0 OE=A2  RE=0 Fifteenth clock
    out     VPORTD_OUT, reg_A1                              ; RSTR=0 OE=A1  RE=0 Sixteenth clock
    out     VPORTD_OUT, reg_zero                            ; RSTR=0 OE=0   RE=0 Seventeenth clock
.endm

; Reset the write side of the field memory and transfer the write address.
.macro fmem_init_write
    ; Inputs:
    ;   reg_A1-reg_A12
    ; Clobbers:
    ;   reg_temp
    ldi     reg_temp, PORTD_FMEM_RSTW_BM | PORTD_FMEM_WE_BM
    out     VPORTD_OUT, reg_temp                            ; RSTW=1 IE=0   WE=1 First clock
    nop
    nop                                                     ; RSTW=1 IE=0   WE=1 Second clock
    ldi     reg_temp, PORTD_FMEM_IE_BM | PORTD_FMEM_WE_BM
    out     VPORTD_IN, reg_temp                             ; RSTW=1 IE=1   WE=0 Third clock
    nop
    nop                                                     ; RSTW=1 IE=1   WE=0 Fourth clock
    nop
    out     VPORTD_OUT, reg_A12                             ; RSTW=0 IE=A12 WE=0 Fifth clock
    nop
    out     VPORTD_OUT, reg_A11                             ; RSTW=0 IE=A11 WE=0 Sixth clock
    nop
    out     VPORTD_OUT, reg_A10                             ; RSTW=0 IE=A10 WE=0 Seventh clock
    nop
    out     VPORTD_OUT, reg_A9                              ; RSTW=0 IE=A9  WE=0 Eighth clock
    nop
    out     VPORTD_OUT, reg_A8                              ; RSTW=0 IE=A8  WE=0 Ninth clock
    nop
    out     VPORTD_OUT, reg_A7                              ; RSTW=0 IE=A7  WE=0 Tenth clock
    nop
    out     VPORTD_OUT, reg_A6                              ; RSTW=0 IE=A6  WE=0 Eleventh clock
    nop
    out     VPORTD_OUT, reg_A5                              ; RSTW=0 IE=A5  WE=0 Twelfth clock
    nop
    out     VPORTD_OUT, reg_A4                              ; RSTW=0 IE=A4  WE=0 Thirteenth clock
    nop
    out     VPORTD_OUT, reg_A3                              ; RSTW=0 IE=A3  WE=0 Fourteenth clock
    nop
    out     VPORTD_OUT, reg_A2                              ; RSTW=0 IE=A2  WE=0 Fifteenth clock
    nop
    out     VPORTD_OUT, reg_A1                              ; RSTW=0 IE=A1  WE=0 Sixteenth clock
    nop
    out     VPORTD_OUT, reg_zero                            ; RSTW=0 IE=0   WE=0 Seventeenth clock
    nop
.endm
