//----------------------------------------------------------------------------
// F97x_SmaRTClock.c
//----------------------------------------------------------------------------
// Copyright 2014 Silicon Laboratories, Inc.
// http://developer.silabs.com/legal/version/v11/Silicon_Labs_Software_License_Agreement.txt
//
// Program Description:
// --------------------
//
// This file contains some basic routines for RTC operations.
//
//
// Target:         C8051F97x
// Tool chain:     Simplicity Studio / Keil C51 9.51
// Command Line:   None
//
// Release 1.0
//    - Initial Revision (CM2)
//    - 5 MAY 2014
//
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include <SI_C8051F970_Register_Enums.h> // Bit Enums

#include "F97x_SmaRTClock.h"

//-----------------------------------------------------------------------------
// RTC_Init ()
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function will initialize the RTC.
//
//-----------------------------------------------------------------------------
void RTC_Init (U8 useCrystal)
{
   U8 CLKSEL_SAVE = CLKSEL;
   U8 i;

   SFRPAGE = LEGACY_PAGE;

   // No need to unlock interface

   // Configure the RTC to use the internal LFO
   if(useCrystal == RTC_USE_LFO)
   {
           // Configure the smaRTClock to use the LFO
           RTC_Write (RTC0XCN, RTC0XCN_BIASX2__DISABLED |
                               RTC0XCN_LFOEN__ENABLED);

           // Disable Auto Load Cap Stepping
           RTC_Write (RTC0XCF, RTC0XCF_AUTOSTP__DISABLED | 0x07);

   }
   else
   {
           // Configure RTC to use crystal mode
           RTC_Write (RTC0XCN, RTC0XCN_AGCEN__DISABLED |
                               RTC0XCN_XMODE__CRYSTAL |
                               RTC0XCN_BIASX2__ENABLED |
                               RTC0XCN_LFOEN__DISABLED);

           // Enable Automatic Load Capacitance
           // stepping and set the desired
           // load capacitance to 4.5pF plus
           // the stray PCB capacitance
           RTC_Write (RTC0XCF, RTC0XCF_AUTOSTP__ENABLED | 0x01);

           // Enable smaRTClock oscillator
           RTC_Write (RTC0CN, RTC0CN_RTC0EN__ENABLED);


           //----------------------------------
           // Wait for smaRTClock to start
           //----------------------------------

           CLKSEL =  CLKSEL_CLKDIV__SYSCLK_DIV_128 | // Switch to 156 kHz for counting
                     CLKSEL_CLKSL__LPOSC;

           // Wait > 20 ms
           for (i=0x150; i!=0; i--);

           // Wait for smaRTClock valid
           while ((RTC_Read (RTC0XCN) & RTC0XCN_CLKVLD__BMASK) == RTC0XCN_CLKVLD__NOT_SET);

           // Wait for Load Capacitance Ready
           while ((RTC_Read (RTC0XCF) & RTC0XCF_LOADRDY__BMASK) == RTC0XCF_LOADRDY__NOT_SET);

           //----------------------------------
           // smaRTClock has been started
           //----------------------------------

           RTC_Write (RTC0XCN, RTC0XCN_AGCEN__ENABLED |
                               RTC0XCN_XMODE__CRYSTAL |
                               RTC0XCN_BIASX2__DISABLED |
                               RTC0XCN_LFOEN__DISABLED);
   }

   // Enable missing smaRTClock detector and
   // leave smaRTClock oscillator enabled.
   RTC_Write (RTC0CN, RTC0CN_RTC0EN__ENABLED | RTC0CN_MCLKEN__ENABLED);

   // Wait > 2 ms
   for (i=0x40; i!=0; i--);

   PMU0CF = PMU0CF_CLEAR__ALL_FLAGS;   // Clear PMU wake-up source flags

   CLKSEL = CLKSEL_SAVE;               // Restore system clock

   // Wait for divider to be applied
   while ((CLKSEL & CLKSEL_CLKRDY__BMASK) == CLKSEL_CLKRDY__NOT_SET);
}

//-----------------------------------------------------------------------------
// RTC_Read ()
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
// RTC_Write ()
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
// RTC_SetTimer  ()
//-----------------------------------------------------------------------------
//
// Parameters   : U32 Timer set time
// Return Value : U8  returns 0xFF for error.
//
// This function will write to the Capture registers and set the timer.
//
//-----------------------------------------------------------------------------
U8 RTC_SetTimer(U32 time)
{
   UU32 value;
   U8 control;

   value.U32 = time;

   SFRPAGE = LEGACY_PAGE;
   control = RTC_Read(RTC0CN);

   if((control&0x80)==0)
      return 0xFF;                     // error RTC must be enabled

   RTC_Write(CAPTURE0, value.U8[b0]);
   RTC_Write(CAPTURE1, value.U8[b1]);
   RTC_Write(CAPTURE2, value.U8[b2]);
   RTC_Write(CAPTURE3, value.U8[b3]);

   RTC_Write(RTC0CN, (control|0x02));  // set timer
   // wait for set bit to go to zero
   while(((RTC_Read(RTC0CN))&0x02)==0x02);

   return 0;
}
//-----------------------------------------------------------------------------
// RTC_CaptureTimer  ()
//-----------------------------------------------------------------------------
//
// Return Value : U32 capture time
// Parameters   : none
//
// This function will read the 32-bit capture value.
//
//-----------------------------------------------------------------------------
U32 RTC_CaptureTimer (void)
{
   UU32 timer;
   U8 control;

   SFRPAGE = LEGACY_PAGE;

   control = RTC_Read(RTC0CN);

   if((control&0x80)==0)
      return 0xFFFF;                   // error RTC must be enabled

   RTC_Write(RTC0CN, (control|0x01));  // read rtc timer capture
   // wait for capture bit to go to zero
   while(((RTC_Read(RTC0CN))&0x01)==0x01);

   // read using auto-increment
   RTC0ADR = (0xD0 | CAPTURE0);        // read CAPTURE0
   while ((RTC0ADR & 0x80) == 0x80);   // Wait for data to be clocked in
   timer.U8[b0]= RTC0DAT;
   while ((RTC0ADR & 0x80) == 0x80);   // Wait for data to be clocked in
   timer.U8[b1]= RTC0DAT;
   while ((RTC0ADR & 0x80) == 0x80);   // Wait for data to be clocked in
   timer.U8[b2]= RTC0DAT;
   while ((RTC0ADR & 0x80) == 0x80);   // Wait for data to be clocked in
   timer.U8[b3]= RTC0DAT;

   return timer.U32;
}
//-----------------------------------------------------------------------------
// RTC_WriteAlarm  ()
//-----------------------------------------------------------------------------
//
// Parameters   : U32 Alarm time
// Parameters   : None
//
// This function will write to the alarm, but does not enable it.
//
//-----------------------------------------------------------------------------
void RTC_WriteAlarm(U32 time)
{
   UU32 value;

   value.U32 = time;

   SFRPAGE = LEGACY_PAGE;

   RTC_Write(ALARM0, value.U8[b0]);
   RTC_Write(ALARM1, value.U8[b1]);
   RTC_Write(ALARM2, value.U8[b2]);
   RTC_Write(ALARM3, value.U8[b3]);
}
//-----------------------------------------------------------------------------
// RTC_ReadAlarm  ()
//-----------------------------------------------------------------------------
//
// Return Value : U32 Alarm time
// Parameters   : None
//
// This function is provided if you want to read the current alarm value and
// modify the results.
//
//-----------------------------------------------------------------------------
U32 RTC_ReadAlarm ()
{
   UU32 time;

   SFRPAGE = LEGACY_PAGE;

   // read using auto-increment
   RTC0ADR = (0xD0 | ALARM0);          // read ALARM0
   while ((RTC0ADR & 0x80) == 0x80);   // Wait for data to be clocked in
   time.U8[b0]= RTC0DAT;
   while ((RTC0ADR & 0x80) == 0x80);   // Wait for data to be clocked in
   time.U8[b1]= RTC0DAT;
   while ((RTC0ADR & 0x80) == 0x80);   // Wait for data to be clocked in
   time.U8[b2]= RTC0DAT;
   while ((RTC0ADR & 0x80) == 0x80);   // Wait for data to be clocked in
   time.U8[b3]= RTC0DAT;

   return time.U32;
}


//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------
