//-----------------------------------------------------------------------------
// F0xx_PCA0_High_Speed_Output.c
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// This program sends a square wave out on an I/O pin, using the PCA's
// High-Speed Output Mode.
//
// In this example, PCA Module 0 is used to generate the waveform.
// Pinout:
//
//    P0.0 -> CEX0 (waveform output)
//
//    all other port pins unused
//
//
// How To Test:
//
// 1) Download code to a 'F0xx device which has an oscilloscope monitoring P0.0
// 2) Run the program - the waveform should be visible on the oscilloscope.
//
// FID:            00X000022
// Target:         C8051F0xx
// Tool chain:     Keil C51 8.0 / Keil EVAL C51
// Command Line:   None
//
//
// Release 1.0
//    -Initial Revision (BD / TP)
//    -3 AUG 2006
//

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include <C8051F000.h>                 // SFR declarations


//-----------------------------------------------------------------------------
// Global Constants
//-----------------------------------------------------------------------------

#define SYSCLK       16000000          // External oscillator frequency in Hz

#define CEX0_FREQUENCY 10000           // (Approx.) Frequency to output in Hz

// SYSCLK cycles per interrupt
#define DIVIDE_RATIO ((SYSCLK/4)/CEX0_FREQUENCY/2)

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

void OSCILLATOR_Init (void);
void PORT_Init (void);
void PCA0_Init (void);

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

unsigned int NEXT_COMPARE_VALUE;       // Next edge to be sent out in HSO mode

//-----------------------------------------------------------------------------
// main() Routine
//-----------------------------------------------------------------------------

void main (void) {

   // Disable watchdog timer
   WDTCN = 0xde;
   WDTCN = 0xad;

   PORT_Init ();                       // Initialize crossbar and GPIO
   OSCILLATOR_Init ();                 // Initialize oscillator
   PCA0_Init ();                       // Initialize PCA0

   EA = 1;                             // Globally enable interrupts

   while (1);                          // Spin here to wait for ISR
}


//-----------------------------------------------------------------------------
// Initialization Subroutines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// OSCILLATOR_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function initializes the system clock to use the internal oscillator
// at 16 MHz.
//
//-----------------------------------------------------------------------------
void OSCILLATOR_Init (void)
{
   OSCICN = 0x07;                      // Set SYSCLK to use 16 MHz internal
                                       // oscillator
}

//-----------------------------------------------------------------------------
// PORT_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function configures the crossbar and GPIO ports.
//
// P0.0   digital   push-pull     PCA0 CEX0
//
//-----------------------------------------------------------------------------
void PORT_Init (void)
{
   XBR0    = 0x08;                     // Route CEX0 to P0.0
   XBR1    = 0x00;
   XBR2    = 0x40;                     // Enable crossbar and weak pull-ups

   PRT0CF |= 0x01;                     // Set CEX0 (P0.0) to push-pull
}

//-----------------------------------------------------------------------------
// PCA0_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function configures the PCA time base, and sets up High-Speed output
// mode for Module 0 (CEX0 pin).
//
// The frequency of the square wave generated at the CEX0 pin is defined by
// the parameter CEX0_FREQUENCY.
//
// The maximum square wave frequency output for this example is about 100 kHz.
// The minimum square wave frequency output for this example is about 35 Hz
//
// The PCA time base in this example is configured to use SYSCLK/4, and SYSCLK
// is set up to use the internal oscillator running at 16 MHz (+- 20%).
// Because of the wide tolerance range of the internal oscillator, the
// actual output frequency may vary from the desired frequency.
//
// Using different PCA clock sources or a different processor clock will
// result in a different frequency for the square wave, and different
// maximum and minimum options.
//
//    -------------------------------------------------------------------------
//    How "High Speed Output Mode" Works:
//
//       The PCA's High Speed Output Mode works by toggling the output pin
//    associated with the module every time the PCA0 register increments and
//    the new 16-bit PCA0 counter value matches the module's capture/compare
//    register (PCA0CPn). When initially enabled in high-speed output mode, the
//    CEXn pin associated with the module will be at a logic high state.  The
//    first match will cause a falling edge on the pin.  The next match will
//    cause a rising edge on the pin, and so on.
//
//    By loading the PCA0CPn register with the next match value every time a
//    match happens, arbitrary waveforms can be generated on the pin with high
//    levels of precision.
//    -------------------------------------------------------------------------
//
// When setting the capture/compare register for the next edge value, the low
//  byte of the PCA0CPn register (PCA0CPLn) should be written first, followed
//  by the high byte (PCA0CPHn).  Writing the low byte clears the ECOMn bit,
//  and writing the high byte will restore it.  This ensures that a match does
//  not occur until the full 16-bit value has been written to the compare
//  register.  Writing the high byte first will result in the ECOMn bit being
//  set to '0' after the 16-bit write, and the next match will not occur at
// the correct time.
//
// It is best to update the capture/compare register as soon after a match
//  occurs as possible so that the PCA counter will not have incremented past
//  the next desired edge value. This code implements the compare register
//  update in the PCA ISR upon a match interrupt.
//
//-----------------------------------------------------------------------------
void PCA0_Init (void)
{
   unsigned short PCA_value = 0;

   // Configure PCA time base; overflow interrupt disabled
   PCA0CN = 0x00;                      // Stop counter; clear all flags
   PCA0MD = 0x02;                      // Use SYSCLK/4 as time base

   PCA0CPM0 = 0x4D;                    // Module 0 = High Speed Output mode,
                                       // Enable Module 0 Interrupt flag,
                                       // Enable ECOM bit

   PCA0L = 0x00;                       // Reset PCA Counter Value to 0x0000
   PCA0H = 0x00;

   PCA0CPL0 = DIVIDE_RATIO & 0x00FF;   // Set up first edge
   PCA0CPH0 = (DIVIDE_RATIO & 0xFF00) >> 8;

   // Set up the variable for the following edge
   PCA_value = PCA0CPL0;
   PCA_value |= PCA0CPH0 << 8;

   NEXT_COMPARE_VALUE = PCA_value + DIVIDE_RATIO;

   EIE1 |= 0x08;                       // Enable PCA interrupts

   CCF0 = 0;                           // Clear the interrupt flag initially

   CR = 1;                             // Start PCA
}

//-----------------------------------------------------------------------------
// Interrupt Service Routines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PCA0_ISR
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This is the ISR for the PCA.  It handles the case when a match occurs on
// channel 0, and updates the PCA0CPn compare register with the value held in
// the global variable "NEXT_COMPARE_VALUE".
//
//-----------------------------------------------------------------------------
void PCA0_ISR (void) interrupt 9
{
   unsigned short PCA_value = 0;

   if (CCF0)                           // If Module 0 caused the interrupt
   {
      CCF0 = 0;                        // Clear module 0 interrupt flag.

      PCA0CPL0 = (NEXT_COMPARE_VALUE & 0x00FF);
      PCA0CPH0 = (NEXT_COMPARE_VALUE & 0xFF00)>>8;

      // Set up the variable for the following edge
      PCA_value = PCA0CPL0;
      PCA_value |= PCA0CPH0 << 8;

      NEXT_COMPARE_VALUE = PCA_value + DIVIDE_RATIO;
   }
   else                                // Interrupt was caused by other bits.
   {
      PCA0CN &= ~0x86;                 // Clear other interrupt flags for PCA
   }
}


//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------