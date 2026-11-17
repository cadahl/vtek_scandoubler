; -------------------------
;  VTek ScanDoubler Rev. 1
;     DUBLINA firmware
;  (c)2026 VTek/Carl Ådahl
; -------------------------
; Synchronization with CCK signal and a method of synchronizing execution
; with arbitrary signals that we POLL with the CPU.

; It is assumed that such polling loops take 3 cycles per iteration.
; Without synchronization, exiting such a polling loop (e.g. detecting a rising
; edge), execution might be randomly offset by 0, 1 or 2 cycles.

; We use two timers to eliminate this random offset:
; TCB0 continuously counts 0, 1, 2, 0, 1, 2, ... on MCU clocks (f_CCK * 8)
; TCA0 is set up as the clock source for TCB0.
; TCA0 is started by using CCK going high.
; Synchronization involves a second loop where we poll for TCB_CNT=0.

cck_sync_init:
    ldi     reg_temp, PORTF_CCK_BM
    sts     PORTF_DIRCLR, reg_temp              ; CCK pin is an input

    ldi     reg_temp, EVSYS_CHANNEL4_PORTF_PIN4_gc
    sts     EVSYS_CHANNEL4, reg_temp            ; Wire CCK to EVSYS ch 4

    ; TCA0
    ldi     reg_temp, EVSYS_USER_CHANNEL4_gc    ; TCA0 event input B
    sts     EVSYS_USERTCA0CNTB, reg_temp        ; is wired to CCK

    sts     TCA0_SINGLE_PERL, reg_zero
    sts     TCA0_SINGLE_PERH, reg_zero          ; OVF every tick

    ldi     reg_temp, TCA_SINGLE_WGMODE_NORMAL_gc
    sts     TCA0_SINGLE_CTRLB, reg_temp

    ldi     reg_temp, TCA_SINGLE_EVACTB_RESTART_POSEDGE_gc
    sts     TCA0_SINGLE_EVCTRL, reg_temp        ; Event B rising edge starts the clock.

    ldi     reg_temp, TCA_SINGLE_CLKSEL_DIV1_gc
    sts     TCA0_SINGLE_CTRLA, reg_temp         ; Set divider but don't enable yet.

    ; TCB0
    ldi     reg_temp, 2
    sts     TCB0_CCMP, reg_temp                 ; Count to 2, then reset.

    ldi     reg_temp, TCB_CNTMODE_INT_gc
    sts     TCB0_CTRLB, reg_temp

    sts     TCB0_CNTL, reg_zero
    sts     TCB0_CNTH, reg_zero                 ; Starting count.

    ldi     reg_temp, TCB_CLKSEL_TCA0_gc | TCB_SYNCUPD_bm | TCB_ENABLE_bm
    sts     TCB0_CTRLA, reg_temp

; Call this to resynchronize with CCK.
cck_sync_reset:
    lds     reg_temp, TCA0_SINGLE_CTRLA
    andi    reg_temp, ~TCA_SINGLE_ENABLE_bm
    sts     TCA0_SINGLE_CTRLA, reg_temp

    sts     TCA0_SINGLE_CNTL, reg_zero
    sts     TCA0_SINGLE_CNTH, reg_zero
    sts     TCB0_CNTL, reg_zero
    sts     TCB0_CNTH, reg_zero

    lds     reg_temp, TCA0_SINGLE_EVCTRL
    ori     reg_temp, TCA_SINGLE_CNTBEI_bm
    sts     TCA0_SINGLE_EVCTRL, reg_temp

    lds     reg_temp, TCA0_SINGLE_CTRLA
    ori     reg_temp, TCA_SINGLE_ENABLE_bm
    sts     TCA0_SINGLE_CTRLA, reg_temp
    ret

; After an external signal has been polled in a loop taking 3 cycles per
; iteration, invoking this macro will align execution a FIXED nr of
; clocks after the rising edge of the signal.
.macro cck_sync_now
cck_sync_now_loop:
    lds     reg_temp, TCB0_CNT      ; (2c) This loop takes 5 cycles and will
    tst     reg_temp                ; (1c) eventually find that TCB0_CNT = 0.
    brne    cck_sync_now_loop       ; (2c/1c) This means we are synced.
.endm
