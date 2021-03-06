//-----------------------------------------------------------------------------
// Si102x_Oscillator_SmaRTClock.c
//-----------------------------------------------------------------------------
// Copyright 2011 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// This program demonstrates how to use the SmaRTClock oscillator as a low
// frequency system clock.
//
//
// How To Test:
//
// 1) Ensure that a 32.768 kHz crystal is installed at XTAL3 and XTAL4.
// 2) Download code to a target board.
// 3) Measure the frequency output on P0.0.
//
//
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
// Include Files
//-----------------------------------------------------------------------------

#include <compiler_defs.h>
#include <Si1020_defs.h>               // SFR declarations

//-----------------------------------------------------------------------------
// Global VARIABLES
//-----------------------------------------------------------------------------
LOCATED_VARIABLE_NO_INIT (reserved, U8, SEG_XDATA, 0x0000);

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

void PORT_Init (void);
void RTC_Init (void);
void OSCILLATOR_Init (void);

unsigned char RTC_Read (unsigned char);
void RTC_Write (unsigned char, unsigned char);

//-----------------------------------------------------------------------------
// main() Routine
//-----------------------------------------------------------------------------

void main (void)
{
   PCA0MD &= ~0x40;                    // WDTE = 0 (clear watchdog timer
                                       // enable)

   PORT_Init();                        // Initialize Port I/O
   RTC_Init ();                        // Initialize RTC
   OSCILLATOR_Init ();                 // Initialize Oscillator


   while (1);                          // Infinite Loop

}

//-----------------------------------------------------------------------------
// Initialization Subroutines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PORT_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function configures the crossbar and ports pins.
//
//
// P0.0   digital    push-pull      /SYSCLK
//-----------------------------------------------------------------------------
void PORT_Init (void)
{

   // Buffered System Clock Output
   P0MDOUT |= 0x01;                    // P0.0 is push-pull

   P1MDIN &= ~0x0C;                    // P1.2 and P1.3 (RTC Pins) are
                                       // analog
   // Crossbar Initialization
   XBR0    = 0x08;                     // Route /SYSCLK to first available pin
   XBR2    = 0x40;                     // Enable Crossbar and weak pull-ups

}

//-----------------------------------------------------------------------------
// RTC_Init ()
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function will initialize the RTC. First it unlocks the RTC interface,
// enables the RTC, clears ALRMn and CAPTUREn bits. Then it sets up the RTC
// to operate using a 32.768khZ crystal. Lastly it enables the alarm and
// interrupt. This function uses the RTC primitive functions to access
// the internal RTC registers.
//
//-----------------------------------------------------------------------------
void RTC_Init (void)
{
   unsigned int i;

   unsigned char CLKSEL_SAVE = CLKSEL;


   RTC0KEY = 0xA5;                     // Unlock the SmaRTClock interface
   RTC0KEY = 0xF1;

   RTC_Write (RTC0XCN, 0x60);          // Configure the SmaRTClock
                                       // oscillator for crystal mode
                                       // Bias Doubling Enabled
                                       // AGC Disabled


   RTC_Write (RTC0XCF, 0x8A);          // Enable Automatic Load Capacitance
                                       // stepping and set the desired
                                       // load capacitance

   RTC_Write (RTC0CN, 0x80);           // Enable SmaRTClock oscillator


   //----------------------------------
   // Wait for SmaRTClock to start
   //----------------------------------

   CLKSEL    =  0x74;                  // Switch to 156 kHz low power
                                       // internal oscillator
   // Wait > 20 ms
   for (i=0x150;i!=0;i--);

   // Wait for SmaRTClock valid
   while ((RTC_Read (RTC0XCN) & 0x10)== 0x00);


   // Wait for Load Capacitance Ready
   while ((RTC_Read (RTC0XCF) & 0x40)== 0x00);

   //----------------------------------
   // SmaRTClock has been started
   //----------------------------------

   RTC_Write (RTC0XCN, 0x40);          // Disable bias doubling

   // Wait > 20 ms
   for (i=0x150;i!=0;i--);


   RTC_Write (RTC0XCN, 0x40);          // Enable Automatic Gain Control
                                       // and disable bias doubling

   RTC_Write (RTC0CN, 0xC0);           // Enable missing SmaRTClock detector
                                       // and leave SmaRTClock oscillator
                                       // enabled.
   // Wait > 2 ms
   for (i=0x40;i!=0;i--);

   PMU0CF = 0x20;                      // Clear PMU wake-up source flags

   CLKSEL = CLKSEL_SAVE;               // Restore system clock
   while(!(CLKSEL & 0x80));            // Poll CLKRDY

}

//-----------------------------------------------------------------------------
// OSCILLATOR_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function initializes the system clock to use the SmaRTClock oscillator.
//
//
//-----------------------------------------------------------------------------
void OSCILLATOR_Init (void)
{


   RSTSRC = 0x06;                      // Enable missing clock detector and
                                       // leave VDD Monitor enabled.

   CLKSEL &= ~0x70;                    // Specify a clock divide value of 1

   while(!(CLKSEL & 0x80));            // Wait for CLKRDY to ensure the
                                       // divide by 1 has been applied

   CLKSEL = 0x03;                      // Select SmaRTClock oscillator
                                       // as the system clock

}


//-----------------------------------------------------------------------------
// Support Subroutines
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// RTC_Write ()
//-----------------------------------------------------------------------------
//
// Return Value : none
// Parameters   : reg, value
//
// This function will Write one byte from the specified RTC register.
// Using a register number greater that 0x0F is not permited,
//
// This function uses the new F960 fast write mode.
//
//-----------------------------------------------------------------------------
void RTC_Write (U8 reg, U8 value)
{
   U8  restoreSFRPAGE;
   restoreSFRPAGE = SFRPAGE;

   SFRPAGE = LEGACY_PAGE;
   reg &= 0x0F;                        // mask low nibble
   RTC0ADR  = reg;                     // pick register
   RTC0DAT = value;                    // write data

   SFRPAGE= restoreSFRPAGE;
}
//-----------------------------------------------------------------------------
// RTC_Read ()
//-----------------------------------------------------------------------------
//
// Return Value : RTC0DAT
// Parameters   : reg
//
// This function will read one byte from the specified RTC register.
// Using a register number greater that 0x0F is not permited,
//
//
// This function uses the new F960 fast read mode.
//
//-----------------------------------------------------------------------------
U8 RTC_Read (U8 reg)
{
   U8 value;
   U8  restoreSFRPAGE;

   restoreSFRPAGE = SFRPAGE;
   SFRPAGE = LEGACY_PAGE;
   
   reg &= 0x0F;                        // mask low nibble
   RTC0ADR  = (reg |0x80);             // write address setting READ bit
   value = RTC0DAT;
   SFRPAGE= restoreSFRPAGE;

   return value;                       // return value
}
//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------
