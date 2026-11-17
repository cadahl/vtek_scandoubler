; -------------------------
;  VTek ScanDoubler Rev. 1
;     DUBLINA firmware
;  (c)2026 VTek/Carl Ådahl
; -------------------------

.equ CLK_PER	 = 28636360
    ;    28375160 28636360
    ; 4000000   ; PAL = 28375160

; Get the address of a label in program memory, mapped to a data address.
; Probably only works on specific series like Dx where program flash is
; directly mapped.
#define PM(x) ((x * 2) + MAPPED_PROGMEM_START)

; Set convenient equs with _PIN, _BM and _PINCTRL for a named GPIO pin.
#define DECLARE_PIN(port, name, n) \
    .equ port##_##name##_PIN      = n \
    .equ port##_##name##_BM       = (1 << (n)) \
    .equ port##_##name##_PINCTRL  = port##_PIN##n##CTRL

; Set convenient equs with _BIT and _BM for a named flag.
#define DECLARE_FLAG(field, name, n) \
    .equ field##_##name##_BIT      = n \
    .equ field##_##name##_BM       = (1 << (n))


; Generate an efficient NOP sequence for the specified exact nr of cycles.
.macro wait
    ; Args:
    ;	Number of cycles to wait.
    ; Clobbers:
    ;   reg_temp
.if ((@0) / 3) > 255
    wait(255*3)
    wait((@0)-(255*3))
.else
.if (@0) >= 3
    ldi     reg_temp, (@0) / 3	; (1c)
waitloop:
    dec     reg_temp            ; (1c)
    brne    waitloop            ; (2c/1c)
.endif
.if ((@0) % 3) > 0
    nop                         ; (1c)
.endif
.if ((@0) % 3) > 1
    nop                         ; (1c)
.endif
.endif
.endm
