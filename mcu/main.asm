; -------------------------
;  VTek ScanDoubler Rev. 1
;     DUBLINA firmware
;  (c)2026 VTek/Carl Ådahl
; -------------------------

.nolist
.include "AVR32DD32def.inc"
.list

.include "helpers.inc"

#define ENABLE_DBG_TX
;#define ENABLE_OSD1

.equ NBR_MEASURE_FRAMES         = 16    ; Number of frames over which measurements must agree.

.equ NTSC_TOTAL_LINES           = 262
.equ NTSC_VBLANK_LINES          = 21

.equ PAL_TOTAL_LINES            = 312
.equ PAL_VBLANK_LINES           = 29

.def reg_zero                   = r0
.def reg_one                    = r1
.def reg_dbg_tx_rp              = r2
.def reg_dbg_tx_wp              = r3
.def reg_fcount                 = r4
.def reg_A1                     = r5
.def reg_A2                     = r6
.def reg_A3                     = r7
.def reg_A4                     = r8
.def reg_A5                     = r9
.def reg_A6                     = r10
.def reg_A7                     = r11
.def reg_A8                     = r12
.def reg_A9                     = r13
.def reg_A10                    = r14
.def reg_A11                    = r15
.def reg_A12                    = r16
.def reg_lcount                 = r17
.def reg_line_l                 = r18
.def reg_line_h                 = r19
.def reg_temp                   = r20
.def reg_temp2                  = r21
.def reg_temp3                  = r22
.def reg_temp4                  = r23

.equ HSYNC_CLOCKS               = 17*8

DECLARE_PIN(PORTA, OSD2, 2)
DECLARE_PIN(PORTA, HSYNC_EXTRA, 3)
DECLARE_PIN(PORTA, OSD1, 4)

DECLARE_PIN(PORTC, CFG_MOSI, 0)
DECLARE_PIN(PORTC, CFG_MISO, 1)
DECLARE_PIN(PORTC, CFG_SCK, 2)
DECLARE_PIN(PORTC, CFG_nSS, 3)

DECLARE_PIN(PORTD, FMEM_OE, 1)
DECLARE_PIN(PORTD, FMEM_RE, 2)
DECLARE_PIN(PORTD, FMEM_RSTR, 3)
DECLARE_PIN(PORTD, FMEM_RSTW, 4)
DECLARE_PIN(PORTD, FMEM_WE, 5)
DECLARE_PIN(PORTD, FMEM_IE, 6)

DECLARE_PIN(PORTF, STRLONG, 0)
DECLARE_PIN(PORTF, STRHSTART, 1)
DECLARE_PIN(PORTF, STREQU, 2)
DECLARE_PIN(PORTF, STRVSTART, 3)
DECLARE_PIN(PORTF, DBG_TX, 5)

; --------------------------
;  C O D E    S E G M E N T
; --------------------------
    .cseg
    .org 0
    rjmp    reset
    ; INTERRUPT VECTORS GO HERE

    .org 0x100
.include "cfg.inc"
.include "dbg_tx.inc"
.include "osd.inc"
.include "fmem.inc"
.include "strobes.inc"

    .cseg
reset:
    cli

    clr     reg_zero
    clr     reg_one
    inc     reg_one

    fmem_init

    ; HSYNC output (active high)
    ; --------------------------
    ldi     reg_temp, PORTA_HSYNC_EXTRA_BM
    sts     PORTA_OUTCLR, reg_temp
    sts     PORTA_DIRSET, reg_temp

    rcall   strobes_init

    ; Wait 1 uS
    wait    280

    ldi     reg_temp, CPU_CCP_IOREG_gc              ; CCP key, we want to change IO regs.

    ldi     reg_temp2, CLKCTRL_FRQRANGE_32M_gc | CLKCTRL_SELHF_EXTCLOCK_gc | CLKCTRL_ENABLE_bm  ; SELHF bit set = extclk
    out     CPU_CCP, reg_temp                       ; Unlock for four cycles.
    sts     CLKCTRL_XOSCHFCTRLA, reg_temp2

    out     CPU_CCP, reg_temp                       ; Unlock for four cycles.
    sts     CLKCTRL_MCLKCTRLB, reg_zero             ; Turn off prescaler.

    ldi     reg_temp2, CLKCTRL_CLKSEL_EXTCLK_gc     ; Full value for CLKCTRL_MCLKCTRLA.
    out     CPU_CCP, reg_temp                       ; Unlock for four cycles.
    sts     CLKCTRL_MCLKCTRLA, reg_temp2

clock_wait:
    lds     reg_temp2, CLKCTRL_MCLKSTATUS            ; Loop until main clock has switched over.
    sbrc    reg_temp2, CLKCTRL_SOSC_bp
    rjmp    clock_wait

    rcall   cfg_init
#ifdef ENABLE_DBG_TX
    rcall   dbg_tx_init
#endif

#ifdef ENABLE_OSD1
    rcall   osd1_init
#endif


; ------------------------------------------------
; MEASURE - DETERMINE PAL/NTSC and LACED/NON-LACED
; ------------------------------------------------
DECLARE_FLAG(MEASURE_FLAG, INITIALIZED, 0)
DECLARE_FLAG(MEASURE_FLAG, SAW_STREQU, 1)
DECLARE_FLAG(MEASURE_FLAG, SAW_STRLONG, 2)

measure:
#ifdef ENABLE_DBG_TX
    ldi     YL, low(PM(measure_msg))
    ldi     YH, high(PM(measure_msg))
    rcall   dbg_tx_push_str
    rcall   dbg_tx_flush
#endif

    clr     reg_temp3                               ; previous measurement
    ldi     reg_temp2, MEASURE_FLAG_INITIALIZED_BM  ; current measurement

    clr     reg_fcount

measure_frame:
    mov     reg_lcount, reg_one                     ; Track nr of vblank lines * 2 for each frame.

    ; Field 1
    rcall   strobes_wait_for_next_vstart

    sbrc    reg_temp, PORTF_STREQU_PIN              ; STREQU is seen in LACED modes.
    ori     reg_temp2, MEASURE_FLAG_SAW_STREQU_BM   ; Make a note that we saw it.
    sbrc    reg_temp, PORTF_STRLONG_PIN             ; STRLONG is used in NTSC modes.
    ori     reg_temp2, MEASURE_FLAG_SAW_STRLONG_BM  ; Make a note that we saw it.

measure_blank_line_f1:
    rcall   strobes_wait_for_next_hstart
    sbrc    reg_temp, PORTF_STREQU_PIN              ; STREQU is seen in LACED modes.
    ori     reg_temp2, MEASURE_FLAG_SAW_STREQU_BM   ; Make a note that we saw it.
    sbrc    reg_temp, PORTF_STRLONG_PIN             ; STRLONG is used in NTSC modes.
    ori     reg_temp2, MEASURE_FLAG_SAW_STRLONG_BM  ; Make a note that we saw it.
    sbrs    reg_temp, PORTF_STRVSTART_PIN           ; either STRVBL or STRLONG (merged in the GAL).
    rjmp    measure_blank_line_end_f1               ; exit if we aren't in vblank
    inc     reg_lcount                              ; Count the number of lines we spend in VBLANK.
    rjmp    measure_blank_line_f1
measure_blank_line_end_f1:

    ; Field 2
    rcall   strobes_wait_for_next_vstart
    sbrc    reg_temp, PORTF_STREQU_PIN              ; STREQU is seen in LACED modes.
    ori     reg_temp2, MEASURE_FLAG_SAW_STREQU_BM   ; Make a note that we saw it.
    sbrc    reg_temp, PORTF_STRLONG_PIN             ; STRLONG is used in NTSC modes.
    ori     reg_temp2, MEASURE_FLAG_SAW_STRLONG_BM  ; Make a note that we saw it.

measure_blank_line_f2:
    rcall   strobes_wait_for_next_hstart
    sbrc    reg_temp, PORTF_STREQU_PIN              ; STREQU is seen in LACED modes.
    ori     reg_temp2, MEASURE_FLAG_SAW_STREQU_BM   ; Make a note that we saw it.
    sbrc    reg_temp, PORTF_STRLONG_PIN             ; STRLONG is used in NTSC modes.
    ori     reg_temp2, MEASURE_FLAG_SAW_STRLONG_BM  ; Make a note that we saw it.
    sbrs    reg_temp, PORTF_STRVSTART_PIN           ; either STRVBL or STRLONG (merged in the GAL).
    rjmp    measure_blank_line_end_f2               ; exit if we aren't in vblank
    inc     reg_lcount                              ; Count the number of lines we spend in VBLANK.
    rjmp    measure_blank_line_f2
measure_blank_line_end_f2:

    ; Evaluate flags from measured frame.
    sbrs    reg_temp3, MEASURE_FLAG_INITIALIZED_BIT ; If it's the first frame, don't compare measurements yet.
    rjmp    measure_skip_equality_check
    cpse    reg_temp3, reg_temp2                    ; If this measurement == prev measurement, we are good.
    rjmp    measure_again                           ; Otherwise, restart the whole measurement phase.
measure_skip_equality_check:

    ; Equality check passed, prepare for next frame.
    mov     reg_temp3, reg_temp2                    ; prev = this
    ldi     reg_temp2, MEASURE_FLAG_INITIALIZED_BM  ; this = clear

    inc     reg_fcount

    ldi     reg_temp, NBR_MEASURE_FRAMES            ; Require fcount # of valid measurements.
    cpse    reg_fcount, reg_temp
    rjmp    measure_frame

    ; We have NBR_MEASURE_FRAMES of valid measurements.
    ; Flags are now in reg_temp3.

#ifdef ENABLE_DBG_TX
    ; Print the measured nbr of lines of vblank * 2
    mov     reg_temp, reg_lcount
    rcall   dbg_tx_push_hex
    rcall   dbg_tx_flush
#endif

measurement_done:
    sbrc    reg_temp3, MEASURE_FLAG_SAW_STREQU_BIT
    rjmp    measure_is_laced                        ; LACED if we saw STREQU at least once during the frame.
    sbrc    reg_temp3, MEASURE_FLAG_SAW_STRLONG_BIT
    rjmp    ntsc                                    ; NTSC (progressive) if we saw STRLONG at least once during the frame.
    rjmp    pal                                     ; PAL (progressive) if not.
measure_is_laced:
    sbrc    reg_temp3, MEASURE_FLAG_SAW_STRLONG_BIT
    rjmp    ntsc                                    ; NTSC (LACED) if we saw STRLONG at least once during the frame.
    rjmp    pal                                     ; PAL (LACED) if not.

measure_again:
    ; Wait a few frames
    ldi     reg_temp, 3
measure_again_delay:
    rcall   strobes_wait_for_next_vstart
    dec     reg_temp
    brne    measure_again_delay
    rjmp    measure

; --------------
; NTSC NON-LACED
; --------------
ntsc:
#ifdef ENABLE_DBG_TX
    ldi     YL, low(PM(ntsc_msg))
    ldi     YH, high(PM(ntsc_msg))
    rcall   dbg_tx_push_str
    rcall   dbg_tx_flush
#endif

ntsc_frame:
    rcall   strobes_wait_for_next_vstart

    ; VERTICAL BLANK
    ldi     reg_lcount, NTSC_VBLANK_LINES
ntsc_blank_line:
    rcall   strobes_wait_for_next_hstart
    out     VPORTD_OUT, reg_zero
    dec     reg_lcount
    brne    ntsc_blank_line

    clr     reg_line_l
    clr     reg_line_h

    ; DISPLAY AREA
    ldi     reg_lcount, NTSC_TOTAL_LINES-NTSC_VBLANK_LINES
ntsc_visible_line:
    rcall   strobes_wait_for_next_hstart
    rcall   visible_scanline
    inc     reg_line_l
    dec     reg_lcount
    breq    ntsc_end_visible_line
    rjmp    ntsc_visible_line
ntsc_end_visible_line:

    rjmp    ntsc_frame


; --------------
; PAL NON-LACED
; --------------
pal:
#ifdef ENABLE_DBG_TX
    ldi     YL, low(PM(pal_msg))
    ldi     YH, high(PM(pal_msg))
    rcall   dbg_tx_push_str
    rcall   dbg_tx_flush
#endif
pal_frame:
    rcall   strobes_wait_for_next_vstart

    ; VERTICAL BLANK
    ldi     reg_lcount, PAL_VBLANK_LINES
pal_blank_line:
    rcall   strobes_wait_for_next_hstart
    out     VPORTD_OUT, reg_zero
    dec     reg_lcount
    brne    pal_blank_line

    clr     reg_line_l
    clr     reg_line_h

    ; DISPLAY AREA
.if PAL_TOTAL_LINES-PAL_VBLANK_LINES < 255
    .error "Visible PAL lines must be at least 255!"
.endif
    ldi     reg_lcount, 255
pal_visible_line:
    rcall   strobes_wait_for_next_hstart
    rcall   visible_scanline
    inc     reg_line_l
    dec     reg_lcount
    breq    pal_end_visible_line
    rjmp    pal_visible_line
pal_end_visible_line:

    ldi     reg_lcount, PAL_TOTAL_LINES-PAL_VBLANK_LINES-255
pal_visible_line2:
    rcall   strobes_wait_for_next_hstart
    rcall   visible_scanline
    inc     reg_line_l
    adc     reg_line_h, reg_zero
    dec     reg_lcount
    breq    pal_end_visible_line2
    rjmp    pal_visible_line2
pal_end_visible_line2:

    rjmp    pal_frame

measure_msg:
    .db "MEASURE", 13, 10, 0
ntsc_msg:
    .db "NTSC", 13, 10, 0, 0
pal_msg:
    .db "PAL", 13, 10, 0


visible_scanline:
    ; Clobbers:
    ;   ASSUME NOTHING!
    fmem_prepare_write_address
    fmem_init_write

    ; It takes 17 write-cycles (34 of our cycles) before we can enable WE/IE.
    ; Set up the read address and init read meanwhile.

    fmem_prepare_read_address
    fmem_init_read

    ; It takes 55 read-cycles before we can enable RE/OE
    wait    54

    ; Enable input and write
    ; Enable output and read
    ldi     reg_temp, PORTD_FMEM_IE_BM | PORTD_FMEM_WE_BM | PORTD_FMEM_RE_BM | PORTD_FMEM_OE_BM
    out     VPORTD_OUT, reg_temp

    ; Clock out pixels
    wait    720

    ; Disable output and read
    cbi     VPORTD_OUT, PORTD_FMEM_OE_PIN
    cbi     VPORTD_OUT, PORTD_FMEM_RE_PIN

    fmem_prepare_read_address
    fmem_init_read

    ; It takes 55 read-cycles before we can enable RE/OE
    wait    55

    ; Enable output and read
    sbi     VPORTD_OUT, PORTD_FMEM_RE_PIN
    sbi     VPORTD_OUT, PORTD_FMEM_OE_PIN

    ; Clocking out pixels
    wait    720

    ; Disable output and read
    cbi     VPORTD_OUT, PORTD_FMEM_OE_PIN
    cbi     VPORTD_OUT, PORTD_FMEM_RE_PIN

    ; Disable input and write
    cbi     VPORTD_OUT, PORTD_FMEM_WE_PIN
    cbi     VPORTD_OUT, PORTD_FMEM_IE_PIN
    ret
