//-----------------------------------------------------------------------------
// F0xx_Timer0_13bitExtTimer.c
//-----------------------------------------------------------------------------
// Copyright 2005 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// This program presents an example of use of the Timer0 of the C8051F0xx's in
// 13-bit counter/timer in counter mode from an external pin. It uses two I/O
// pins; one to create the input pulses (SIGNAL) to be counted and another one
// to enable the counting process (GTE).
//
// This code uses the 'F0xxDK as HW platform.
//
// Pinout:
//
//    P0.0 -> T0 (Timer0 External Input)
//    P0.1 -> /INT0
//
//    P1.1 -> SIGNAL (digital, push-pull)
//    P1.5 -> GTE (digital, push-pull)
//    P1.6 -> LED
//
//    P1.7 -> BUTTON1 (switch)
//
//    all other port pins unused
//
// How To Test:
//
// 1) Open the F0xx_Timer0_13bitExtTimer.c file in the Silicon Labs IDE.
// 2) To change the number of input pulses/interrupt, modify
//    PULSES_PER_TOGGLE.
// 3) To change the speed of the SIGNAL waveform, modify
//    SOFTWARE_DELAY
// 4) Compile the project
// 5) Download code to a 'F0xx device
// 6) Verify J1 and J3 are populated on the 'F005 TB.
// 7) Connnect the following pins:
//
//    P0.0 <--> P1.1 (T0 with SIGNAL) (pin 17 to pin 10 on the 'F005 TB header)
//    P0.1 <--> P1.5 (/INT0 with GTE) (pin 18 to pin 6 on the 'F005 TB header)
//
// 8) Run the code
// 9) To enable the counting, press and hold BUTTON1 (switch), which will be
//    polled to enable the timer.
// 10) The LED will blink and SIGNAL can be observed on an oscilloscope.
//
//
// FID:            00X000030
// Target:         C8051F0xx
// Tool chain:     KEIL C51 7.20 / KEIL EVAL C51
// Command Line:   None
//
// Release 1.0
//    -Initial Revision (CG)
//    -08 NOV 2005
//

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include <C8051F000.h>                 // SFR declarations

//-----------------------------------------------------------------------------
// Global Constants
//-----------------------------------------------------------------------------

#define SYSCLK             16000000/8  // SYSCLK in Hz (16 MHz internal
                                       // oscillator / 8)
                                       // the internal oscillator has a
                                       // tolerance of +/- 20%

#define PULSES_PER_TOGGLE       1000   // Arbitrary number of pulses in the
                                       // input pin necessary to create an
                                       // interrupt.
                                       // Limited to 0x1FFF or 8191d for a
                                       // 13-bit timer

#define SOFTWARE_DELAY  SYSCLK/100000  // Software timer to generate the
                                       // SIGNAL output
                                       // Generate a signal in the kHz range

#define AUX0 0x1FFF-PULSES_PER_TOGGLE+1
#define AUX1 AUX0&0x001F               // 5 LSBs of timer value in TL0[4:0]
#define AUX2 ((AUX0&0x1FFF)>>5)        // High 8 bits of timer in TH0

#define TIMER0_RELOAD_HIGH      AUX2   // Reload value for Timer0 high byte
#define TIMER0_RELOAD_LOW       AUX1   // Reload value for Timer0 5 LSBs

sbit GTE = P1^5;                       // Gate control signal for Timer0
sbit LED = P1^6;                       // LED='1' means ON
sbit SIGNAL = P1^1;                    // SIGNAL is used to input pulses into
                                       // T0 pin
sbit BUTTON = P1^7;                    // Button that enables counting

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

void Port_Init (void);                 // Port initialization routine
void Timer0_Init(void);                // Timer0 initialization routine

//-----------------------------------------------------------------------------
// main() Routine
//-----------------------------------------------------------------------------

void main (void)
{
   unsigned int counter;

   WDTCN = 0xDE;                       // Disable watchdog timer
   WDTCN = 0xAD;

   Timer0_Init ();                     // Initialize the Timer0
   Port_Init ();                       // Init Ports

   LED = 0;
   EA = 1;                             // Enable global interrupts

   while (1)
   {
      if (BUTTON == 1)                 // If button pressed, enable counting
      {
         GTE = 1;
      }
      else
      {
         GTE = 0;
      }

      // Wait a certain time before toggling signal
      for (counter=0; counter < SOFTWARE_DELAY; counter++);

      SIGNAL = ~SIGNAL;                // Toggle the SIGNAL pin
   }
}

//-----------------------------------------------------------------------------
// Initialization Subroutines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Port_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function configures the crossbar and GPIO ports.
//
// Pinout:
//
//    P0.0 -> T0 (Timer0 External Input)
//    P0.1 -> /INT0
//
//    P1.1 -> SIGNAL (digital, push-pull)
//    P1.5 -> GTE (digital, push-pull)
//    P1.6 -> LED
//
//    P1.7 -> BUTTON1 (switch)
//
//    all other port pins unused
//
//-----------------------------------------------------------------------------
void Port_Init (void)
{
   XBR1 = 0x06;                        // INT0 and T0 available at the I/O pins
   XBR2 = 0x40;                        // Enable crossbar
   PRT1CF = 0x62;                      // Set P1.5/6/1 to be used as push-pull
}

//-----------------------------------------------------------------------------
// Timer0_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function configures the Timer0 as a 13-bit timer, interrupt enabled.
// Using an external signal as clock source 1:8 and reloading the
// TH0 and TL0 registers it will interrupt and then toggle the LED upon roll-
// over of the timer every 1sec.
//
// Note: In this example the GATE0 gate control is used.
//-----------------------------------------------------------------------------
void Timer0_Init(void)
{
   TMOD = 0x0C;                        // Timer0 in 13-bit mode ext. counter
                                       // gated counting T0 input
   ET0 = 1;                            // Timer0 interrupt enabled
   TCON = 0x11;                        // Timer0 ON with INT0 edge active
   TH0 = TIMER0_RELOAD_HIGH;           // Reinit Timer0 High register
   TL0 = TIMER0_RELOAD_LOW;            // Reinit Timer0 Low register
}

//-----------------------------------------------------------------------------
// Interrupt Service Routines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Timer0_ISR
//-----------------------------------------------------------------------------
//
// Here we process the Timer0 interrupt and toggle the LED
//
//-----------------------------------------------------------------------------
void Timer0_ISR (void) interrupt 1
{
   LED = ~LED;                         // Toggle the LED
   TH0 = TIMER0_RELOAD_HIGH;           // Reinit Timer0 High register
   TL0 = TIMER0_RELOAD_LOW;            // Reinit Timer0 Low register
}

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------