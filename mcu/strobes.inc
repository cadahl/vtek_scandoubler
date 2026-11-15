
    .cseg

strobes_init:
    ; GAL address decoder strobe inputs (active high)
    ; -----------------------------------------------
    ldi     reg_temp, PORTF_STRHSTART_BM | PORTF_STREQU_BM | PORTF_STRVSTART_BM | PORTF_STRLONG_BM
    sts     PORTF_DIRCLR, reg_temp
    ret

strobes_wait_for_next_vstart:
    ; Output:
    ;   reg_temp
wait_for_strvbl_clear:
    in	    reg_temp, VPORTF_IN
    sbrc    reg_temp, PORTF_STRVSTART_PIN
    rjmp    wait_for_strvbl_clear
wait_for_strvbl_set:
    in	    reg_temp, VPORTF_IN
    sbrs    reg_temp, PORTF_STRVSTART_PIN
    rjmp    wait_for_strvbl_set
    ret

strobes_wait_for_next_hstart:
    ; Output:
    ;   reg_temp
wait_for_strhstart_clear:
    in	    reg_temp, VPORTF_IN
    sbrc    reg_temp, PORTF_STRHSTART_PIN
    rjmp    wait_for_strhstart_clear
wait_for_strhstart_set:
    in	    reg_temp, VPORTF_IN
    sbrs    reg_temp, PORTF_STRHSTART_PIN
    rjmp    wait_for_strhstart_set
    ret

