$NOMOD51
;-----------------------------------------------------------------------------
; T60x_Blinky.ASM
;-----------------------------------------------------------------------------
;  Copyright (C) 2007 Silicon Laboratories, Inc.
;  All rights reserved.
;
;  FILE NAME   :  T60x_BLINKY.ASM 
;  DATE        :  26 JAN 2007
;  TARGET MCU  :  C8051T60x 
;  DESCRIPTION :  This program illustrates how to disable the watchdog timer,
;                 configure the Crossbar, configure a port and write to a port
;                 I/O pin.
;
; 	NOTES: 
;
;-----------------------------------------------------------------------------

$include (C8051T600.inc)                  ; Include register definition file.

;-----------------------------------------------------------------------------
; EQUATES
;-----------------------------------------------------------------------------

GREEN_LED   equ   P0.6                    ; Green LED: '1' is ON	

;-----------------------------------------------------------------------------
; RESET and INTERRUPT VECTORS
;-----------------------------------------------------------------------------

            ; Reset Vector
            cseg AT 0
            ljmp Main                     ; Locate a jump to the start of
                                          ; code at the reset vector.

;-----------------------------------------------------------------------------
; CODE SEGMENT
;-----------------------------------------------------------------------------


Blink       segment  CODE

            rseg     Blink                ; Switch to this code segment.
            using    0                    ; Specify register bank for the
                                          ; following program code.

Main:
            ; Disable the WDT.
            anl   PCA0MD, #NOT(040h)      ; clear Watchdog Enable bit

            ; Enable the Port I/O Crossbar
            orl   XBR0, #40h              ; skip LED pin in crossbar
                                          ; assignments
            mov   XBR2, #40h
            orl   P0MDOUT, #40h           ; make LED pin output push-pull
            orl   P0MDIN, #40h            ; make LED pin input mode digital

            ; Initialize LED to OFF
            clr   GREEN_LED

            ; Simple delay loop.
Loop2:      mov   R7, #03h			
Loop1:      mov   R6, #00h
Loop0:      mov   R5, #00h
            djnz  R5, $
            djnz  R6, Loop0
            djnz  R7, Loop1
            cpl   GREEN_LED               ; Toggle LED.
            jmp   Loop2

;-----------------------------------------------------------------------------
; End of file.

END

