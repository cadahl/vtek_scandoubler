; -------------------------
;  VTek ScanDoubler Rev. 1
;     DUBLINA firmware
;  (c)2026 VTek/Carl Ådahl
; -------------------------
; Debug TX FIFO
; - Must start at 256 byte boundary.
; - Size must be power of two.
; - If size == 256 bytes, both operations use one less cycle.
; - Use dbg_tx_push_one to queue characters.
; - Call dbg_tx_transmit_one occasionally, to send outgoing characters at a reasonable pace.
#ifdef ENABLE_DBG_TX

.equ DBG_TX_FIFO_SIZE           = 256
.equ DBG_TX_BAUD                = 9600
.equ DBG_TX_CYCLES_PER_BIT = CLK_PER / DBG_TX_BAUD

.if DBG_TX_FIFO_SIZE < 2
    .error "DBG_TX_FIFO_SIZE must be >= 2"
.endif

.if DBG_TX_FIFO_SIZE > 256
    .error "DBG_TX_FIFO_SIZE must be <= 256"
.endif

.if (DBG_TX_FIFO_SIZE & (DBG_TX_FIFO_SIZE-1)) != 0
    .error "DBG_TX_FIFO_SIZE must be a power of two"
.endif

    .cseg
; this is placed here to be near the branch.
dbg_tx_transmit_one_fifo_empty:
    ldi	    reg_temp, 4                     ; EOT (FIFO empty)
    ret

; Transmit one character from the FIFO (if there are any).
dbg_tx_transmit_one:
    ; Clobbers:
    ;   reg_temp, reg_temp2, XL, XH
    ; Outputs:
    ;	reg_temp is the character sent, 00h: NUL char (not sent) or 04h: EOT (FIFO empty)

    mov     XL, reg_dbg_tx_rp
    ldi     XH, high(dbg_tx_fifo)
    cp      XL, reg_dbg_tx_wp
    breq    dbg_tx_transmit_one_fifo_empty  ; ...bail if FIFO empty (read pointer == write pointer).
    ld      reg_temp3, X+
    tst	    reg_temp3
    breq    dbg_tx_transmit_one_skip_send   ; Skip send if this is a null character.

    ; time window 1 (ends when next bit is sent)
    cbi	    VPORTF_OUT, PORTF_DBG_TX_PIN    ; (1c)
    wait    DBG_TX_CYCLES_PER_BIT-1-6	    ; 1 cycle above, 6 cycles below

    push    reg_temp2                       ; (2c)
    push    reg_temp3

    ldi	    reg_temp2, 8                    ; (1c)
dbg_tx_transmit_one_bit_loop:
    ror     reg_temp3                       ; (1c)
    brcs    dbg_tx_transmit_one_set_high    ; (2c/1c)
    nop                                     ; (1c)
    ; time window 2a (ends when next bit is sent)
    cbi     VPORTF_OUT, PORTF_DBG_TX_PIN    ; (1c)
    rjmp    dbg_tx_transmit_one_bit_done    ; (2c)
dbg_tx_transmit_one_set_high:
    ; time window 2b (ends when next bit is sent)
    sbi     VPORTF_OUT, PORTF_DBG_TX_PIN    ; (1c)
    nop                                     ; (1c)
    nop                                     ; (1c)
dbg_tx_transmit_one_bit_done:
    wait    DBG_TX_CYCLES_PER_BIT-3-3-3	    ; 3 cycles above, 3 cycles below and 3 cycles from start of loop to next bit sent

    dec	    reg_temp2                       ; (1c)
    brne    dbg_tx_transmit_one_bit_loop    ; (2c/1c)

    nop                                     ; (1c)
    nop                                     ; (1c)
    nop                                     ; (1c)
    nop                                     ; (1c)
    nop                                     ; (1c)

    sbi     VPORTF_OUT, PORTF_DBG_TX_PIN    ; (1c) Stop bit.

    ror	    reg_temp3                       ; (1c) Restore reg_temp to return the character sent.

    wait    DBG_TX_CYCLES_PER_BIT-5-4

    mov	    reg_temp, reg_temp3

    pop	    reg_temp2                       ; (2c)
    pop	    reg_temp3

dbg_tx_transmit_one_skip_send:
.if DBG_TX_FIFO_SIZE != 256
    andi    XL, DBG_TX_FIFO_SIZE-1          ; (1c) wrap pointer to power of two FIFO size.
.endif
    mov     reg_dbg_tx_rp, XL               ; (1c) store the advanced and wrapped read pointer.

    ret


dbg_tx_flush:
    rcall   dbg_tx_transmit_one
    cpi	    reg_temp, 4                     ; loop until EOT
    brne    dbg_tx_flush
    ret

; Push character in reg_temp to the debug tx FIFO.
.macro dbg_tx_push_one
    ; Inputs:
    ;   reg_temp    Character to push.
    ; Clobbers:
    ;   XL, XH
    mov     XL, reg_dbg_tx_wp
    ldi     XH, high(dbg_tx_fifo)           ; X = points into FIFO at write position.
    st      X+, reg_temp                    ; Store the character, even if the FIFO will turn out to be full below.
.if DBG_TX_FIFO_SIZE != 256
    andi    XL, DBG_TX_FIFO_SIZE-1          ; Wrap pointer to power of two FIFO size.
.endif
    cpse    XL, reg_dbg_tx_rp               ; If full, we skip storing the wrapped and advanced write pointer.
    mov     reg_dbg_tx_wp, XL               ; This is safe because in a full FIFO, write pointer + 1 == read pointer.
.endm

; Initialize debug tx.
dbg_tx_init:
    clr	    reg_dbg_tx_rp                   ; FIFO read pointer
    clr	    reg_dbg_tx_wp                   ; FIFO write pointer
    sbi     VPORTF_OUT, PORTF_DBG_TX_PIN    ; The TX line will idle high.
    ldi     reg_temp, PORTF_DBG_TX_BM
    sts     PORTF_DIRSET, reg_temp          ; Set it as an output.

;    ldi     reg_temp, low(DBG_TX_BAUDVAL)
;    sts     USART1_BAUDL, reg_temp
;    ldi     reg_temp, high(DBG_TX_BAUDVAL)
;    sts     USART1_BAUDH, reg_temp

;    ldi     reg_temp, USART_NORMAL_CHSIZE_8BIT_gc      ; 8N1
;    sts     USART1_CTRLC, reg_temp

;    ldi     reg_temp, USART_TXEN_bm
;    sts     USART1_CTRLB, reg_temp
    ret

; Push value in reg_temp as its two character hex string representation,
; followed by newline, to the debug tx FIFO.
dbg_tx_push_hex:
    ; Inputs:
    ;	reg_temp
    ; Clobbers:
    ;	reg_temp, XL, XH, YL, YH, ZL, ZH
    ldi     ZH, high(PM(hex_tbl))	;
    mov     YL, reg_temp
    mov     ZL, YL
    swap    ZL
    andi    ZL, 15
    ld      reg_temp, Z
    dbg_tx_push_one
    mov     ZL, YL
    andi    ZL, 15
    ld      reg_temp, Z
    dbg_tx_push_one
    ldi     reg_temp, 10
    dbg_tx_push_one
    ret

; Push string pointed to by Y to the debug tx FIFO.
dbg_tx_push_str:
    ld      reg_temp, Y+
    tst     reg_temp
    breq    dbg_tx_push_str_end
    dbg_tx_push_one
    rjmp    dbg_tx_push_str
dbg_tx_push_str_end:
    ldi     reg_temp, 10
    dbg_tx_push_one
    ret

hex_tbl_prealign:
    .org (hex_tbl_prealign + 128) & 0xFFFF80    ; With avrasm2, this is a word address and aligned to 256 bytes.
hex_tbl:
    .db "0123456789ABCDEF"

; --------------------------
;  D A T A    S E G M E N T
; --------------------------
    .dseg
dbg_tx_fifo_prealign:
    .org (dbg_tx_fifo_prealign + 256) & 0xFFFF00
dbg_tx_fifo:
    .byte   DBG_TX_FIFO_SIZE                    ; Must be aligned to 256 bytes
dbg_tx_fifo_end:

    .cseg
#endif
