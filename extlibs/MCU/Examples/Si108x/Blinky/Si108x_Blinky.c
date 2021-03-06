//-----------------------------------------------------------------------------
// Si108x_Blinky.c
//-----------------------------------------------------------------------------
// Copyright (C) 2013 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// This program flashes the yellow LED1 on the wireless motherboard about
// five times a second using the interrupt handler for Timer2.
//
//
// How To Test:
//
// 1) Download code to the target board 
// 2) Run the program.  If the Yellow LED1 flashes, the program is working
//
//
// Target:         Si108x
// Tool chain:     Keil
// Command Line:   None
//
// Release 1.0
//    -Initial Revision (FB)
//    -22 OCT 2013
//

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <compiler_defs.h>             // compiler declarations
#include <Si1080_defs.h>               // SFR declarations

//-----------------------------------------------------------------------------
// Pin Declarations
//-----------------------------------------------------------------------------

SBIT (LED1,    SFR_P0, 0);          // '0' means ON, '1' means OFF


//-----------------------------------------------------------------------------
// Global CONSTANTS
//-----------------------------------------------------------------------------

#define SYSCLK       20000000/8        // SYSCLK frequency in Hz

#define LED_ON           0
#define LED_OFF          1

//-----------------------------------------------------------------------------
// Function PROTOTYPES
//-----------------------------------------------------------------------------
void PORT_Init (void);
void Timer2_Init (int counts);

//-----------------------------------------------------------------------------
// MAIN Routine
//-----------------------------------------------------------------------------
void main (void)
{
   PCA0MD &= ~0x40;                    // WDTE = 0 (clear watchdog timer
                                       // enable)
   PORT_Init ();

   Timer2_Init (SYSCLK / 12 / 10);     // Init Timer2 to generate interrupts
                                       // at a 10 Hz rate.

   EA = 1;                             // Enable global interrupts

   while (1) {}                        // Spin forever
}

//-----------------------------------------------------------------------------
// PORT_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function configures the crossbar and ports pins.
//
// P0.0   digital   open-drain    
//-----------------------------------------------------------------------------
void PORT_Init (void)
{

   XBR2    = 0x40;                     // Enable crossbar and enable
                                       // weak pull-ups
}

//-----------------------------------------------------------------------------
// Timer2_Init
//-----------------------------------------------------------------------------
//
// Configure Timer2 to 16-bit auto-reload and generate an interrupt at
// interval specified by <counts> using SYSCLK/48 as its time base.
//
void Timer2_Init (int counts)
{
   TMR2CN  = 0x00;                        // Stop Timer2; Clear TF2;
                                          // use SYSCLK/12 as timebase
   CKCON  &= ~0x60;                       // Timer2 clocked based on T2XCLK;

   TMR2RL  = -counts;                     // Init reload values
   TMR2    = 0xffff;                      // set to reload immediately
   ET2     = 1;                           // enable Timer2 interrupts
   TR2     = 1;                           // start Timer2
}

//-----------------------------------------------------------------------------
// Interrupt Service Routines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Timer2_ISR
//-----------------------------------------------------------------------------
// This routine changes the state of the LED whenever Timer2 overflows.
//
INTERRUPT(Timer2_ISR, INTERRUPT_TIMER2)
{
   TF2H = 0;                              // clear Timer2 interrupt flag
   LED1 = !LED1;                          // change state of LEDs

}

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------
