//-----------------------------------------------------------------------------
// F02x_External_Interrupts.c
//-----------------------------------------------------------------------------
// Copyright 2007 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// This software shows the necessary configuration to use External Interrupt 0
// (/INT0) or External Interrupt 1 (/INT1) as an interrupt source.  The code
// executes the initialization routines and then spins in an infinite while()
// loop.  If the button P3.7 (on the target board) is pressed, then the
// edge-triggered /INT0 input on P0.0 will cause an interrupt and toggle the
// LED.
//
// Pinout:
//
// P0.0 - /INT0
// P0.1 - /INT1
//
// P1.6 - LED
//
// P3.7 - SWITCH
//
// How To Test:
//
// 1) Compile and download code to a 'F02x target board.
// 2) On the target board, connect the switch side of J1 to either P0.0 for
//    /INT0 or P0.1 for /INT1.
// 3) Verify the J3 jumper is in place.
// 4) Verify the J6 and J9 jumpers are removed.
// 5) Press the P3.7 switch.  Every time the switch is pressed, the P1.6 LED
//    should toggle.
//
// Target:         C8051F02x
// Tool chain:     Keil C51 7.50 / Keil EVAL C51
// Command Line:   None
//
//
// Release 1.0
//    -Initial Revision (SM)
//    -19 JUN 2007
//

//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------

#include <C8051F020.h>

//-----------------------------------------------------------------------------
// Global Constants
//-----------------------------------------------------------------------------

#define SYSCLK       (22118400L)

sbit LED    = P1^6;                    // Green Led
sbit SWITCH = P3^7;                    // Push-button switch on board

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

void Oscillator_Init (void);           // Configure the system clock
void Port_Init (void);                 // Configure the Crossbar and GPIO
void Ext_Interrupt_Init (void);        // Configure External Interrupts (/INT0
                                       // and /INT1)

//-----------------------------------------------------------------------------
// MAIN Routine
//-----------------------------------------------------------------------------
void main (void)
{
   WDTCN = 0xDE;                       // Disable Watchdog timer
   WDTCN = 0xAD;

   Oscillator_Init();                  // Initialize the system clock
   Port_Init ();                       // Initialize crossbar and GPIO
   Ext_Interrupt_Init();               // Initialize External Interrupts

   EA = 1;

   while(1);                           // Infinite while loop waiting for
                                       // an interrupt from /INT0 or /INT1
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
// This function initializes the system clock to use the  external 22.1184MHz
// crystal.
//
//-----------------------------------------------------------------------------
void OSCILLATOR_Init (void)
{
   int i;                              // Software timer

   OSCICN |= 0x80;                     // Enable the missing clock detector

   // Initialize external crystal oscillator to use 22.1184 MHz crystal

   OSCXCN = 0x67;                      // Enable external crystal osc.
   for (i=0; i < 256; i++);            // Wait at least 1ms
   while (!(OSCXCN & 0x80));           // Wait for crystal osc to settle
   OSCICN |= 0x08;                     // Select external clock source
   OSCICN &= ~0x04;                    // Disable the internal osc.
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
// Pinout:
//
// P0.0 - digital   open-drain  /INT0
// P0.1 - digital   open-drain  /INT1
//
// P1.6 - digital   push-pull   LED
//
// P3.7 - digital   open-drain  SWITCH
//
//-----------------------------------------------------------------------------
void Port_Init (void)
{
   XBR0     = 0x00;
   XBR1     = 0x14;                    // No peripherals selected
   XBR2     = 0x40;                    // Enable crossbar and weak pullups

   P1MDOUT  = 0x40;                    // LED is push-pull a output

}

//-----------------------------------------------------------------------------
// Ext_Interrupt_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function configures and enables /INT0 and /INT1 (External Interrupts)
// as negative edge-triggered.
//
//-----------------------------------------------------------------------------
void Ext_Interrupt_Init (void)
{
   TCON = 0x05;                        // /INT 0 and /INT 1 are falling edge
                                       // triggered

   EX0 = 1;                            // Enable /INT0 interrupts
   EX1 = 1;                            // Enable /INT1 interrupts

}

//-----------------------------------------------------------------------------
// Interrupt Service Routines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// /INT0 ISR
//-----------------------------------------------------------------------------
//
// Whenever a negative edge appears on P0.0, the LED is toggled.
// The interrupt pending flag is automatically cleared by vectoring to the ISR
//
//-----------------------------------------------------------------------------
void INT0_ISR (void) interrupt 0
{
   LED = !LED;
}

//-----------------------------------------------------------------------------
// /INT1 ISR
//-----------------------------------------------------------------------------
//
// Whenever a negative edge appears on P0.1, the LED is toggled.
// The interrupt pending flag is automatically cleared by vectoring to the ISR
//
//-----------------------------------------------------------------------------
void INT1_ISR (void) interrupt 2
{
   LED = !LED;
}

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------
