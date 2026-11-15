; -------------------------
;  VTek ScanDoubler Rev. 1
;     DUBLINA firmware
;  (c)2026 VTek/Carl Ådahl
; -------------------------
; CFG routines - SPI slave on PC0-PC3

cfg_init:
    ldi	    reg_temp, PORTMUX_SPI0_ALT5_gc
    sts	    PORTMUX_SPIROUTEA, reg_temp
    ret
