//-----------------------------------------------------------------------------
// F97x_Imeasure_Idle_RTC_Oscillator.c
//-----------------------------------------------------------------------------
// Copyright 2014 Silicon Laboratories, Inc.
// http://developer.silabs.com/legal/version/v11/Silicon_Labs_Software_License_Agreement.txt
//
// Program Description:
//
// This program reproduces the power number shown in the datasheet, Table 1.2,
// in the Digital Supply Current - CPU Inactive (Idle Mode, not fetching
// instructions from Flash) section.
//
//
// How To Test:
//
// 1) Ensure shorting blocks are place on the following:
//    - J1:    3.3v <> VREG (Power)
// 2) Pull off the JP3 jumper and place a multimeter across, positive side on
//    the pin labeled VDD.
// 3) Connect the USB Debug Adapter to H8.
// 4) Place the VDD SELECT switch (SW1) in the VREG position and power the board.
// 5) Compile and download code to the C8051F970-TB by selecting
//      Run -> Debug from the menus,
//      or clicking the Debug button in the quick menu,
//      or pressing F11.
// 6) Run the code by selecting
//      Run -> Resume from the menus,
//      or clicking the Resume button in the quick menu,
//      or pressing F8.
// 7) Measure the supply current using the multimeter.
//
//
// Target:         C8051F97x
// Tool chain:     Simplicity Studio / Keil C51 9.51
// Command Line:   None
//
//
// Release 1.0
//    - Initial Revision (TP/JL)
//    - 27 MAY 2014


//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "SI_C8051F970_Register_Enums.h"

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

void Oscillator_Init (void);
void Port_Init (void);

void RTC_Init (void);
U8 RTC_Read (U8);
void RTC_Write (U8, U8);
void Wait_us(U16 us);

//-----------------------------------------------------------------------------
// MAIN Routine
//-----------------------------------------------------------------------------
void main (void)
{
   PCA0MD &= ~PCA0MD_WDTE__ENABLED;    // Disable watchdog timer

   Port_Init();                        // Initialize Port I/O
   RTC_Init();
   Oscillator_Init ();                 // Initialize Oscillator

   while(1)
   {
      PCON = PCON_IDLE__IDLE;
   }
}

//-----------------------------------------------------------------------------
// Initialization Subroutines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Oscillator_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function initializes the system clock to use the selected oscillator
// and divide value.
//
//-----------------------------------------------------------------------------
void Oscillator_Init (void)
{
   U8 SFRPAGE_save = SFRPAGE;

   SFRPAGE = LEGACY_PAGE;

   REG0CN &= ~REG0CN_OSCBIAS__BMASK;   // Disable precision oscillator bias

   PMU0CF = PMU0CF_CLEAR__ALL_FLAGS;   // Clear all wake-up source flags
                                       // This allows low power oscillator
                                       // to be turned off

   CLKSEL = CLKSEL_CLKSL__RTC |
            CLKSEL_CLKDIV__SYSCLK_DIV_1;

   // Wait for divider to be applied
   while ((CLKSEL & CLKSEL_CLKRDY__BMASK) == CLKSEL_CLKRDY__NOT_SET);

   SFRPAGE = SFRPAGE_save;
}

//-----------------------------------------------------------------------------
// Port_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function configures the crossbar and ports pins.
//
// P0.6 - anolog   XTAL3
// P0.7 - anolog   XTAL4
//-----------------------------------------------------------------------------
void Port_Init (void)
{
   SFRPAGE = CONFIG_PAGE;

   P0 = 0xFF;                          // Set all I/O to analog inputs
   P1 = 0xFF;                          // to save power
   P2 = 0xFF;
   P3 = 0xFF;
   P4 = 0xFF;
   P5 = 0xFF;

   P0MDIN = 0x00;
   P1MDIN = 0x00;
   P2MDIN = 0x00;
   P3MDIN = 0x00;
   P4MDIN = 0x00;
   P5MDIN = 0x00;

   // Uncomment this section to send SYSCLK out a port pin
   /*
   // SYSCLK to P0.1
   P0SKIP |= 0x01;  // P0.0 skipped
   P0MDIN |= 0x02;  // P0.1 is an output
   P0MDOUT |= 0x02; // P0.1 is push-pull


   XBR0 = XBR0_SYSCKE__ENABLED;            // Put SYSCLK onto Crossbar
   */
   XBR1 = XBR1_XBARE__ENABLED |            // Enable Crossbar and
          XBR1_WEAKPUD__PULL_UPS_DISABLED; // disable Weak pullups.
                                           // Less power consumption with
                                           // crossbar enabled...

   SFRPAGE = LEGACY_PAGE;
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
   RTC_Write (RTC0XCN, RTC0XCN_AGCEN__DISABLED
              | RTC0XCN_XMODE__CRYSTAL
              | RTC0XCN_BIASX2__ENABLED
              | RTC0XCN_LFOEN__DISABLED);


   RTC_Write (RTC0XCF, RTC0XCF_AUTOSTP__ENABLED | 0x01);

   RTC_Write (RTC0CN, RTC0CN_RTC0EN__ENABLED);


   //----------------------------------
   // Wait for smaRTClock to start
   //----------------------------------

   // Wait > 20 ms
   Wait_us(25000);

   // Wait for smaRTClock valid
   while ((RTC_Read (RTC0XCN) & RTC0XCN_CLKVLD__BMASK) == RTC0XCN_CLKVLD__NOT_SET);


   // Wait for Load Capacitance Ready
   while ((RTC_Read (RTC0XCF) & RTC0XCF_LOADRDY__BMASK) == RTC0XCF_LOADRDY__NOT_SET);

   //----------------------------------
   // smaRTClock has been started
   //----------------------------------

   RTC_Write (RTC0XCN, RTC0XCN_AGCEN__ENABLED
              | RTC0XCN_XMODE__CRYSTAL
              | RTC0XCN_BIASX2__DISABLED
              | RTC0XCN_LFOEN__DISABLED);


   RTC_Write (RTC0CN, RTC0CN_RTC0EN__ENABLED
              | RTC0CN_MCLKEN__ENABLED); // Enable missing smaRTClock detector
                                         // and leave smaRTClock oscillator
                                         // enabled.
   // Wait > 2 ms
   Wait_us(2500);

   PMU0CF = PMU0CF_CLEAR__ALL_FLAGS;     // Clear PMU wake-up source flags
}

//-----------------------------------------------------------------------------
// Support Subroutines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// RTC_Read
//-----------------------------------------------------------------------------
//
// Return Value : RTC0DAT
// Parameters   : 1) unsigned char reg - address of RTC register to read
//
// This function will read one byte from the specified RTC register.
// Using a register number greater that 0x0F is not permited.
//
//-----------------------------------------------------------------------------
U8 RTC_Read (U8 reg)
{
   reg &= 0x0F;                        // mask low nibble
   RTC0ADR  = reg;                     // pick register
   RTC0ADR |= 0x80;                    // set BUSY bit to read
   while ((RTC0ADR & 0x80) == 0x80);   // poll on the BUSY bit
   return RTC0DAT;                     // return value
}

//-----------------------------------------------------------------------------
// RTC_Write
//-----------------------------------------------------------------------------
//
// Return Value : none
// Parameters   : 1) unsigned char reg - address of RTC register to write
//                2) unsigned char value - the value to write to <reg>
//
// This function will Write one byte to the specified RTC register.
// Using a register number greater that 0x0F is not permited.
//-----------------------------------------------------------------------------
void RTC_Write (U8 reg, U8 value)
{
   reg &= 0x0F;                        // mask low nibble
   RTC0ADR  = reg;                     // pick register
   RTC0DAT = value;                    // write data
   while ((RTC0ADR & 0x80) == 0x80);   // poll on the BUSY bit
}

//-----------------------------------------------------------------------------
// Wait_us
//-----------------------------------------------------------------------------
//
// Insert <us> microsecond delays using an on-chip timer.
//
void Wait_us(U16 us)
{
   CKCON |= CKCON_T2ML__SYSCLK;        // Timer2 uses SYSCLK

   // Setup Timer2 to overflow at a 1 MHz Rate
   TMR2RL = -(20000000/1000000);
   TMR2 = TMR2RL;

   TMR2CN |= TMR2CN_TR2__RUN;          // Enable Timer2 in auto-reload mode

   while(us)
   {
      TMR2CN &= ~TMR2CN_TF2H__BMASK;
      while((TMR2CN & TMR2CN_TF2H__BMASK) == TMR2CN_TF2H__NOT_SET);
      us--;
   }

   TMR2CN &= ~TMR2CN_TR2__BMASK;
}

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------
