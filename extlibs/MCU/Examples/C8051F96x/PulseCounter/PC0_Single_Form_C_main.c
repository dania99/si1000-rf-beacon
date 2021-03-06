//=============================================================================
// PC0_Single_Form_C_main.c
//=============================================================================
// Copyright 2010 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// C File Description:
//
//
// Target:
//    C8051F960
//
// IDE:
//    Silicon Laboratories IDE
//
// Tool Chains:
//    Keil
//    SDCC
//    Raisonance
//
// Project Name:
//    C8051F960 Code Examples
//
// Description:
//
// This software must be used in accordance with the End User License Agreement.
//
//=============================================================================
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <compiler_defs.h>
#include <C8051F960_defs.h>
#include "PulseCounter.h"
#include "SmaRTClockF960.h"
//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
bit PC0_Comparator0Flag;
//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------
void PortInit (void);
//-----------------------------------------------------------------------------
// Main Prototypes
//-----------------------------------------------------------------------------
void main (void)
{
   U8 value;
   U32 count;
   U32 current;

   SFRPAGE = LEGACY_PAGE;
   PCA0MD  &= ~0x40;                   // disable watchdog timer

   RTC_Init();
   PortInit();

   PC0_Init (PC0_SINGLE_MODE, PC0_SAMPLING_RATE_2MS, 0);

   // set debounce time to 4 ms
   value = PC0_SetDebounceTimeHigh (4000);
   value = PC0_SetDebounceTimeLow (4000);

   // Disable Pull-up for Form C
   current = PC0_SetPullUpCurrent (0);

   PC0_WriteComparator0 (0x000010);

   // Enable comparator and overflow interrupts
   SFRPAGE = DPPE_PAGE;
   PC0INT0 = PC0_CMP0EN|PC0_OVRF;

   // Clear software flags
   PC0_Comparator0Flag = 0;

   EIE2 |= 0x10;                       // Enable PC0 interrupt
   EA = 1;                             // Enable global interrupts

   while (PC0_Comparator0Flag==0)
   {
      count = PC0_ReadCounter0 ();
   }

   while(1);
}
//-----------------------------------------------------------------------------
// PC0 ISR
//-----------------------------------------------------------------------------
INTERRUPT(PC0_ISR, INTERRUPT_PC0)
{
   U8 source;

   EIE2 &= ~0x10;                       // disable further interrupts

   SFRPAGE = DPPE_PAGE;

   source = PC0_ReadPC0INT0();         // read using accessor function

   if((source&PC0_CMP0F)==PC0_CMP0F)
   {
      PC0INT0 = source & ~PC0_CMP0F;   // clear flag
      // Set software flag.
      PC0_Comparator0Flag = 1;
      // Do something on comparator event.
   }
   else if((source&PC0_OVRF)==PC0_OVRF)
   {
      PC0INT0 = source & ~PC0_CMP1F;   // clear flag
      // Do something on overflow event.
   }
}
//-----------------------------------------------------------------------------
// PortInit
//-----------------------------------------------------------------------------
void PortInit (void)
{
   SFRPAGE = LEGACY_PAGE;
   P1MDIN = ~0x01;                     // enable PC0 as analog input

   // The crossbar does not have to be enabled to use the Pulse Counter
   //XBR2 = 0x40;

}

//=============================================================================
