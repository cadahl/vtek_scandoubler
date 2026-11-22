; -------------------------
;  VTek ScanDoubler Rev. 1
;     DUBLINA firmware
;  (c)2026 VTek/Carl Ådahl
; -------------------------
; OSD routines

#ifdef ENABLE_OSD1
    .cseg
osd1_init:
    ; Clobbers:
    ;	reg_temp
    ldi     reg_temp, PORTA_OSD2_BM
    sts     PORTA_DIRCLR, reg_temp

    ldi     reg_temp, PORTMUX_USART0_ALT1_gc    ; We want MOSI at PA4.
    sts     PORTMUX_USARTROUTEA, reg_temp

    ldi     reg_temp, PORTA_OSD1_BM             ; MOSI pin is output.
    sts     PORTA_DIRSET, reg_temp

    sts     USART0_BAUDL, reg_zero              ; CLK_PER/2 for 14MHz pixel clock.
    sts     USART0_BAUDH, reg_zero

    ldi     reg_temp, USART_CMODE_MSPI_gc       ; USART in MSPI mode.
    sts     USART0_CTRLC, reg_temp

    ldi     reg_temp, USART_TXEN_bm
    sts     USART0_CTRLB, reg_temp

    ret

.macro osd_line
    ldi     XH, high(osd_screen_chars)  ; High byte of screen chars.
    ldi     YH, high(osd_screen_attrs)  ; High byte of screen attribs.
    mov     XL, reg_rline_l             ; YYYYyyyy
    andi    XL, 0xF0                    ; Mask out the lower 4 bits, the font is 16 pixels tall.
    lsl     XL                          ; YYY00000 (note: high Y bit in carry flag, added to XH next)
    adc     XH, reg_zero                ; X now points to the start of the correct line.
    mov     YL, XL                      ; We will use the same low byte offset into the attribute table.

    ldi     ZH, high(PM(font))          ; High byte of address to font. This has ZEROES in the low nibble.
    mov     reg_temp, reg_rline_h       ; Inject the low nibble from the read line counter.
    andi    reg_temp, 0x0F
    or      ZH, reg_temp                ; ZH now points to the correct line of the font. ZL selects glyph.

    ldi     reg_temp2, 32
charloop:
    ld      ZL, X+                      ; Load character. (2c)
    ld	    reg_temp3, Y+               ; Load attribute. (2c)
    ld      reg_temp, Z                 ; Look up 8-pixel font line. (3c)
    eor	    reg_temp, reg_temp3         ; Apply attribute (inversion mask) (1c)
    sts     USART0_TXDATAL, reg_temp    ; Clock out these pixels with MSPI. (2c)
    nop
    nop
    nop
    dec     reg_temp2
    brne    charloop
.endm

    .org 0x4000-0x1000
font:
    .include "trivga.inc"

    .dseg
osd_screen_prealign:
    .org (osd_screen_prealign + 256) & 0xFFFF00
osd_screen_chars:
    .byte   32*16               ; Must be aligned to 256 bytes
osd_screen_attrs:
    .byte   32*16

#endif
