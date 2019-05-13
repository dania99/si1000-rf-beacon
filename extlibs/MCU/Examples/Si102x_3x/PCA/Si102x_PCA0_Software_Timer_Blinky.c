//-----------------------------------------------------------------------------
// Si102x_PCA0_Software_Timer_Blinky.c
//-----------------------------------------------------------------------------
// Copyright 2011 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// This program uses the PCA's Software Timer Mode to periodically schedule
// an interrupt.  Inside the ISR, a port pin is toggled to blink an LED on the
// target board.
//
// In this example, PCA Module 0 is used to generate the interrupt, and the
// port pin driving the LED is configured for push-pull mode.
//
// How To Test:
//
// 1) Download code to an Si102x/3x device on a UPMP-F960-MLCD target board.
// 2) Run the program - the LED should blink at 5 Hz.
//
// Target:         Si102x/3x
// Tool chain:     Generic
// Command Line:   None
//
// Release 1.0
//    - Initial Revision (MRF)
//    - 27 OCTOBER 2011
//

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include <compiler_defs.h>
#include <Si1020_defs.h>               // SFR declarations

//-----------------------------------------------------------------------------
// Global Constants
//-----------------------------------------------------------------------------

#define SYSCLK       24500000L         // Internal oscillator frequency in Hz

#define LED_FREQUENCY 5L               // Frequency to blink LED at in Hz
#define T0_CLOCKS 245L                 // Use 245 clocks per T0 Overflow

// SYSCLK cycles per interrupt
#define PCA_TIMEOUT ((SYSCLK/T0_CLOCKS)/LED_FREQUENCY/2)

//-----------------------------------------------------------------------------
// Define Hardware
//-----------------------------------------------------------------------------
#define UDP_F960_MCU_MUX_LCD
//#define UDP_F960_MCU_EMIF
//-----------------------------------------------------------------------------
// Hardware Dependent definitions
//-----------------------------------------------------------------------------
#ifdef UDP_F960_MCU_MUX_LCD
SBIT (LED1, SFR_P0, 0);
SBIT (LED2, SFR_P0, 1);
SBIT (LED3, SFR_P0, 2);
SBIT (LED4, SFR_P0, 3);
#endif

#ifdef UDP_F960_MCU_EMIF
SBIT (LED1, SFR_P3, 0);
SBIT (LED2, SFR_P3, 1);
SBIT (LED3, SFR_P3, 2);
SBIT (LED4, SFR_P3, 3);
#endif

//-----------------------------------------------------------------------------
// Global VARIABLES
//-----------------------------------------------------------------------------
LOCATED_VARIABLE_NO_INIT (reserved, U8, SEG_XDATA, 0x0000);

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

void OSCILLATOR_Init (void);
void Port_Init (void);
void PCA0_Init (void);
INTERRUPT_PROTO (PCA0_ISR, INTERRUPT_PCA0);

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

unsigned int Next_Compare_Value;       // Next edge to be sent out in HSO mode

//-----------------------------------------------------------------------------
// main() Routine
//-----------------------------------------------------------------------------

void main (void)
{
   PCA0MD = 0x00;                      // Disable watchdog timer

   Port_Init ();                       // Initialize crossbar and GPIO
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
// This function initializes the system clock to use the internal precision
// oscillator.
//
//-----------------------------------------------------------------------------
void OSCILLATOR_Init (void)
{
   SFRPAGE = CONFIG_PAGE;
   FLSCL     = 0x40;                   // Set BYPASS bit for >12.5 MHz.

   SFRPAGE = LEGACY_PAGE;
   REG0CN |= 0x10;                     // Enable the precision osc. bias
   OSCICN |= 0x80;                     // Enable the precision internal osc.

   RSTSRC = 0x06;                      // Enable missing clock detector and
                                       // leave VDD Monitor enabled.

   CLKSEL = 0x00;                      // Select precision internal osc.

}

//-----------------------------------------------------------------------------
// Port_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function configures the crossbar and GPIO ports.
//
// No crossbar peripherals are used for this example.
//
//-----------------------------------------------------------------------------
void Port_Init (void)
{
   SFRPAGE = LEGACY_PAGE;

   XBR0    = 0x00;
   XBR2    = 0x40;                     // No peripherals routed to pins
                                       // Enable crossbar and weak pull-ups

}

//-----------------------------------------------------------------------------
// PCA0_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function configures the PCA time base to use T0 Overflows as a clock
// source.  Timer0 is also configured by this routine for 8-bit auto-reload
// mode, where T0_CLOCKS is the number of System clocks per T0 Overflow.
// The code then sets up Software Timer mode for Module 0 (CEX0 pin).
//
// On every interrupt from Module 0, software toggles the port I/O pin for the
// LED. The frequency of the LED toggling is determined by the parameter
// CEX0_FREQUENCY.
//
// The maximum frequency output for this example is about 50 kHz.
// The minimum frequency output for this example is about 1 Hz
//
// The PCA time base in this example is configured to use T0 overflows, and
// SYSCLK is set up for the internal oscillator running at 24.5 MHz.
// Using different PCA clock sources, different T0 reload values, or a
// different processor clock will result in a different frequency for the LED
// square wave, and different maximum and minimum options.
//
//    -------------------------------------------------------------------------
//    How "Software Timer Mode" Works:
//
//       The PCA's Software Timer Mode works by generating an interrupt for the
//    associated module every time the PCA0 register increments and the new
//    16-bit PCA0 counter value matches the module's capture/compare
//    register (PCA0CPn).
//
//    By loading the PCA0CPn register with the next match value every time a
//    match happens, arbitrarily timed interrupts can be generated.
//    -------------------------------------------------------------------------
//
// When setting the capture/compare register for the next match value, the low
// byte of the PCA0CPn register (PCA0CPLn) should be written first, followed
// by the high byte (PCA0CPHn).  Writing the low byte clears the ECOMn bit,
// and writing the high byte will restore it.  This ensures that a match does
// not occur until the full 16-bit value has been written to the compare
// register.  Writing the high byte first will result in the ECOMn bit being
// set to '0' after the 16-bit write, and the next match will not occur at
// the correct time.
//
// It is best to update the capture/compare register as soon after a match
// occurs as possible so that the PCA counter will not have incremented past
// the next desired edge value. This code implements the compare register
// update in the PCA ISR upon a match interrupt.
//
//-----------------------------------------------------------------------------
void PCA0_Init (void)
{
   SFRPAGE = LEGACY_PAGE;

   // Configure Timer 0 for 8-bit auto-reload mode, using SYSCLK as time base
   TMOD &= 0xF0;                       // Clear all T0 control bits
   TMOD |= 0x02;                       // 8-bit auto-reload timer
   CKCON |= 0x04;                      // T0 uses SYSCLK
   TH0 = (unsigned char) -T0_CLOCKS;   // Set up reload value
   TL0 = (unsigned char) -T0_CLOCKS;   // Set up initial value

   // Configure PCA time base; overflow interrupt disabled
   PCA0CN = 0x00;                      // Stop counter; clear all flags
   PCA0MD = 0x04;                      // Use Timer 0 as time base

   PCA0CPM0 = 0x49;                    // Module 0 = Software Timer Mode,
                                       // Enable Module 0 Interrupt flag,
                                       // Enable ECOM bit

   PCA0L = 0x00;                       // Reset PCA Counter Value to 0x0000
   PCA0H = 0x00;

   PCA0CPL0 = PCA_TIMEOUT & 0x00FF;    // Set up first match
   PCA0CPH0 = (PCA_TIMEOUT & 0xFF00) >> 8;

   // Set up the variable for the following match
   Next_Compare_Value = PCA0CP0 + PCA_TIMEOUT;

   EIE1 |= 0x10;                       // Enable PCA interrupts

   CR = 1;                             // Start PCA
   TR0 = 1;                            // Start Timer 0
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
// the global variable "Next_Compare_Value".
//
//-----------------------------------------------------------------------------
INTERRUPT(PCA0_ISR, INTERRUPT_PCA0)
{
   static U8 count = 0;

   SFRPAGE = LEGACY_PAGE;
   if (CCF0)                           // If Module 0 caused the interrupt
   {
      CCF0 = 0;                        // Clear module 0 interrupt flag.

      PCA0CPL0 = (Next_Compare_Value & 0x00FF);
      PCA0CPH0 = (Next_Compare_Value & 0xFF00)>>8;

      LED1 = 1;                      // Invert the LED pin
      LED2 = 1;
      LED3 = 1;
      LED4 = 1;
      count++;

      if(count == 8)
      {
      LED1 = 0;                      // Invert the LED pin
      LED2 = 0;
      LED3 = 0;
      LED4 = 0;
      count = 0;
      }
      // Set up the variable for the following edge
      Next_Compare_Value = PCA0CP0 + PCA_TIMEOUT;
   }
   else                                // Interrupt was caused by other bits.
   {
      PCA0CN &= ~0x86;                 // Clear other interrupt flags for PCA
   }
}

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------
