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

    ldi     reg_temp, PORT_ISC_RISING_gc
    sts     PORTF_STRHSTART_PINCTRL, reg_temp
    sts     PORTF_STREQU_PINCTRL, reg_temp
    sts     PORTF_STRVSTART_PINCTRL, reg_temp
    sts     PORTF_STRLONG_PINCTRL, reg_temp

    sbi     VPORTF_INTFLAGS, PORTF_STRHSTART_PIN
    sbi     VPORTF_INTFLAGS, PORTF_STREQU_PIN
    sbi     VPORTF_INTFLAGS, PORTF_STRVSTART_PIN
    sbi     VPORTF_INTFLAGS, PORTF_STRLONG_PIN
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
    ldi     reg_temp, PORTF_STRHSTART_BM | PORTF_STREQU_BM | PORTF_STRVSTART_BM | PORTF_STRLONG_BM
    out     VPORTF_INTFLAGS, reg_temp       ; Clear interrupt flags to get a blank slate.
strobes_wait_for_next_hstart_loop:          ; Loop until next HSTART
    sbis    VPORTF_INTFLAGS, PORTF_STRHSTART_PIN
    rjmp    strobes_wait_for_next_hstart_loop
    in	    reg_temp, VPORTF_INTFLAGS       ; Read all the interrupt flags now.
    out     VPORTF_INTFLAGS, reg_temp       ; Clear the interrupt flags.
    ret
