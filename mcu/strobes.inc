; -------------------------
;  VTek ScanDoubler Rev. 1
;     DUBLINA firmware
;  (c)2026 VTek/Carl Ådahl
; -------------------------
; Strobes from GAL

    .cseg
strobes_init:
    ; Clobbers:
    ;   reg_temp
    ldi     reg_temp, PORTF_STRHSTART_BM | PORTF_STREQU_BM | PORTF_STRVSTART_BM | PORTF_STRLONG_BM
    sts     PORTF_DIRCLR, reg_temp

;    ldi     reg_temp, PORT_ISC_RISING_gc
 ;   sts     PORTF_STRHSTART_PINCTRL, reg_temp
  ;  sts     PORTF_STREQU_PINCTRL, reg_temp
   ; sts     PORTF_STRVSTART_PINCTRL, reg_temp
    ;sts     PORTF_STRLONG_PINCTRL, reg_temp

    ;ldi     reg_temp, PORTF_STRHSTART_BM | PORTF_STREQU_BM | PORTF_STRVSTART_BM | PORTF_STRLONG_BM
    ;out     VPORTF_INTFLAGS, reg_temp
    ret

strobes_wait_for_next_vstart:
    ; Clobbers:
    ;   reg_temp
wait_until_not_in_vblank:
    rcall   strobes_wait_for_next_hstart
    sbrc    reg_temp, PORTF_STRVSTART_PIN
    rjmp    wait_until_not_in_vblank
wait_until_in_vblank:
    rcall   strobes_wait_for_next_hstart
    sbrs    reg_temp, PORTF_STRVSTART_PIN
    rjmp    wait_until_in_vblank
    ret

strobes_wait_for_next_hstart:
    ; Output:
    ;   reg_temp
strobes_wait_for_next_hstart_loop:          ; Loop until next HSTART
    in	    reg_temp, VPORTF_IN       ; Read all the interrupt flags now.
    sbrs    reg_temp, PORTF_STRHSTART_PIN
    rjmp    strobes_wait_for_next_hstart_loop
    ;ext_sync_now                            ; Eliminate phase offset added by the polling loop.

    nop
    nop
    nop
    nop

    nop
    nop
    nop
    nop

    nop
    nop
    nop
    nop

    nop
    in	    reg_temp4, VPORTF_IN       ; Read all the interrupt flags now.
    or      reg_temp, reg_temp4
    ret
