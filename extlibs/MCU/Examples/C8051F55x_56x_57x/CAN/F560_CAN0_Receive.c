//-----------------------------------------------------------------------------
// F560_CAN0_Receive.c
//-----------------------------------------------------------------------------
// Copyright 2009 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// This program receives data from another CAN node using a configurable number
// of message objects and datasize.  The message identifier is set based on the
// message object number.  The corresponding transmit firmware is
// F560_CAN0_Transmit.c.  The following two #defines should be set to the
// same values in both the transmit and receive code:
//
// #define MESSAGE_OBJECTS
// #define MESSAGE_SIZE
//
// #define RX_COMPLETE_MASK should also be set.  See definition for details
//
// The purpose of this example is to show how to initialize a message object
// for receive mode and how to handle certain error conditions.
//
// How To Test:
//
// 1) Verify the LED and switch pin jumpers are populated
//    (J19 for device A and J11 for device B).
//
// 2) Make sure the CAN jumpers in J17 (for A side) and J12 (for B side)
//    are connected.
//
// 3) Download the code to a F560-TB (either device A or device B) that is
//    connected as above to another device running the F560_CAN0_Transmit
//    code.
//
// 4) Run the code on the Receiver first, then run the code on the Transmitter.
//
// 5) While the receiver is waiting for transmissions, the LED will blink.
//    Once transmissions are complete, the LED will remain lit.
//
// Target:         C8051F550 (Side B of a C8051F560-TB)
// Tool chain:     Raisonance / Keil / SDCC
// Command Line:   None
//
// Release 1.1 / 11 MAR 2010 (GP)
//    -Tested with Raisonance
//
// Release 1.0 / 15 JAN 2009 (GP)
//    - Initial Revision
//
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "compiler_defs.h"
#include "C8051F560_defs.h"            // SFR declarations

//-----------------------------------------------------------------------------
// Type Definitions
//-----------------------------------------------------------------------------

typedef union UU64
{
   U32 U32[2];
   U16 U16[4];
   U8 U8[8];
} UU64;

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

void OSCILLATOR_Init (void);
void PORT_Init (void);
void CAN0_Init (void);

INTERRUPT_PROTO (CAN0_ISR, INTERRUPT_CAN0);

//-----------------------------------------------------------------------------
// Global Constants
//-----------------------------------------------------------------------------

#define SYSCLK       24000000          // System clock speed in Hz

#define MESSAGE_OBJECTS    32          // Number of message objects to use
                                       // Range is 1-32
#define MESSAGE_SIZE        8          // Size in bytes of each CAN message
                                       // Range is 1-8

#define RX_COMPLETE_MASK  0xFFFFFFFF   // Set this to (2^MESSAGE_OBJECTS - 1)

//-----------------------------------------------------------------------------
// Bit Definition Masks
//-----------------------------------------------------------------------------

// CAN0STAT
#define BOff  0x80                     // Busoff Status
#define EWarn 0x40                     // Warning Status
#define EPass 0x20                     // Error Passive
#define RxOk  0x10                     // Receive Message Successfully
#define TxOk  0x08                     // Transmitted Message Successfully
#define LEC   0x07                     // Last Error Code

//-----------------------------------------------------------------------------
// Pin Definitions
//-----------------------------------------------------------------------------

SBIT (LED, SFR_P1, 3);                 // LED = 1 turns on the LED

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

bit CAN_ERROR = 0;                     // 0 = No Errors occurred
                                       // 1 = Some error(s) occurred

UU32 CAN_RX_COMPLETE;                  // Bit status register that is updated
                                       // when a RX complete is received for
                                       // a specific message object.  Should be
                                       // equal to RX_COMPLETE_MASK when done

//-----------------------------------------------------------------------------
// MAIN Routine
//-----------------------------------------------------------------------------

void main (void)
{
   U16 iter;                           // Loop counter

   SFRPAGE = ACTIVE_PAGE;              // Set for PCA0MD

   PCA0MD &= ~0x40;                    // Disable Watchdog Timer

   OSCILLATOR_Init ();                 // Initialize oscillator
   PORT_Init ();                       // Initialize crossbar and GPIO
   CAN0_Init ();                       // Start CAN peripheral

   CAN_RX_COMPLETE.U32 = 0x0000;       // Initialize as no messages received

   EIE2 |= 0x02;                       // Enable CAN interupts
   EA = 1;                             // Enable global interrupts

   // While all messages have not been received, blink the LED
   while ((CAN_RX_COMPLETE.U32 != RX_COMPLETE_MASK) || (CAN_ERROR))
   {
      LED = !LED;
      for (iter = 0; iter < 65000; iter++);
   }

   // Once all transmissions are complete, keep the LED on
   LED = 1;                            // Turn on LED

   while (1);
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
// Initialize the internal oscillator to 24 MHz
//
//-----------------------------------------------------------------------------
void OSCILLATOR_Init (void)
{
   U8 SFRPAGE_save = SFRPAGE;
   SFRPAGE = CONFIG_PAGE;

   OSCICN = 0x87;                      // Set internal oscillator divider to 1

   SFRPAGE = SFRPAGE_save;
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
// P0.6   digital  push-pull        CAN TX
// P0.7   digital  open-drain       CAN RX
//
// P1.3   digital  push-pull        LED
//
//-----------------------------------------------------------------------------

void PORT_Init (void)
{
   U8 SFRPAGE_save = SFRPAGE;
   SFRPAGE  = CONFIG_PAGE;             // Port SFR's on Configuration page

   P0MDOUT  |= 0x40;                   // P0.6 (CAN0 TX) is push-pull
   P1MDOUT  |= 0x08;                   // P1.3 (LED) is push-pull

   XBR0     = 0x02;                    // Enable CAN0 on Crossbar
   XBR2     = 0x40;                    // Enable Crossbar and weak pull-ups

   SFRPAGE = SFRPAGE_save;
}

//-----------------------------------------------------------------------------
// CAN0_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function initializes the CAN peripheral and all of the message objects
//
// The following settings are chosen for the peripheral and message objects:
//
// CAN Bit Clock : 1 Mbps
// Auto Retransmit : Automatic Retransmission is enabled
// MsgVal        : Set to Valid based on the #define MESSAGE_OBJECTS
// Filtering     : Enabled for all valid message objects
// Message Identifier : 11-bit standard; Each message object is only used by
//                      one message ID and the ID is equal to the object number
// Direction     : All valid message objects are configured for receive
// End of Buffer : All message objects are treated as separate buffers
//
// The following interrupts are enabled and are handled by CAN0_ISR
//
// Error Interrupts
// Status Change Interrupt
// Receive Interrupt
//
//-----------------------------------------------------------------------------

void CAN0_Init (void)
{
   U8 iter;

   U8 SFRPAGE_save = SFRPAGE;
   SFRPAGE  = CAN0_PAGE;               // All CAN register are on page 0x0C

   CAN0CN |= 0x01;                     // Start Intialization mode

   //---------Initialize general CAN peripheral settings

   CAN0CN |= 0x4E;                     // Enable Status, Error,
                                       // Module Interrupts
                                       // Enable access to bit timing register

   // See the CAN Bit Timing Spreadsheet for how to calculate this value
   CAN0BT = 0x1402;                    // Based on 24 Mhz CAN clock, set the
                                       // CAN bit rate to 1 Mbps

   //---------Initialize settings common to all message objects

   // Command Mask Register
   CAN0IF1CM = 0x00F0;                 // Write Operation
                                       // Transfer ID Mask, MDir, MXtd
                                       // Transfer ID, Dir, Xtd, MsgVal
                                       // Transfer Control Bits
                                       // Don't set TxRqst or transfer data

   // Mask Registers
   CAN0IF1M1 = 0x0000;                 // Mask Bits 15-0 not used for filtering
   CAN0IF1M2 = 0x5FFC;                 // Ignore Extended Identifier for
                                       // filtering
                                       // Used Direction bit for filtering
                                       // Use ID bits 28-18 for filtering

   // Arbitration Registers
   CAN0IF1A1 = 0x0000;                 // 11-bit ID, so lower 16-bits not used

   // Message Control Registers
   CAN0IF1MC = 0x1480 | MESSAGE_SIZE;  // Enable Receive Interrupt
                                       // Message Object is a Single Message
                                       // Message Size set by #define


   //---------Initialize unique settings for each valid message object

   for (iter = 0; iter < MESSAGE_OBJECTS; iter++)
   {
      // For example purposes, set 11-bit identifier based on the message
      // object that is used to receive it.

      // Arbitration Registers
      CAN0IF1A2 = 0x8000 | (iter << 2);  // Set MsgVal to valid
                                         // Set Object Direction to read
                                         // Set 11-bit Identifier to iter

      CAN0IF1CR = iter;                  // Start command request

      while (CAN0IF1CRH & 0x80);         // Poll on Busy bit
   }

   //---------Initialize settings for unused message objects

   for (iter = MESSAGE_OBJECTS; iter < 32; iter++)
   {
      // Set remaining message objects to be Ignored
      CAN0IF1A2 = 0x0000;              // Set MsgVal to 0 to Ignore
      CAN0IF1CR = iter;                // Start command request

      while (CAN0IF1CRH & 0x80);       // Poll on Busy bit
   }

   CAN0CN &= ~0x41;                    // Return to Normal Mode and disable
                                       // access to bit timing register

   SFRPAGE = SFRPAGE_save;
}


//-----------------------------------------------------------------------------
// Interrupt Service Routines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// CAN0_ISR
//-----------------------------------------------------------------------------
//
// The ISR is triggered upon any CAN errors or upon reception of message
//
// If an error occurs, a global counter is updated
//
//-----------------------------------------------------------------------------

INTERRUPT (CAN0_ISR, INTERRUPT_CAN0)
{
   U8 iter;
   UU64 rx_data;

   // SFRPAGE is set to CAN0_Page automatically when ISR starts

   U8 status = CAN0STAT;               // Read status, which clears the Status
                                       // Interrupt bit pending in CAN0IID

   U8 Interrupt_ID = CAN0IID;          // Read which message object caused
                                       // the interrupt

   // new_data variable will now tell which message object is new.

   CAN0IF1CM = 0x007F;                 // Read all of message object to IF1
                                       // Clear IntPnd and newData

   CAN0IF1CR = Interrupt_ID;           // Start command request

   while (CAN0IF1CRH & 0x80);          // Poll on Busy bit

   if (status & RxOk)                  // If transmit completed successfully
   {
       // Read all 8 data bytes to rxdata, even though they might not be valid
       rx_data.U16[0] = CAN0IF1DA1;    // First pair of bytes
       rx_data.U16[1] = CAN0IF1DA2;    // Second pair of bytes
       rx_data.U16[2] = CAN0IF1DB1;    // Third pair of bytes
       rx_data.U16[3] = CAN0IF1DB2;    // Fourth and last pair of bytes

       // Message Object 0 reports as 0x20 in the CAN0IID register, so
       // convert it to 0x00
       if (Interrupt_ID == 0x20)
       {
          Interrupt_ID = 0x00;
       }

       for (iter = 0; iter < MESSAGE_SIZE; iter++)
	    {
          if (rx_data.U8[iter] != (Interrupt_ID + iter))
          {
             CAN_ERROR = 1;
          }
	    }

       // Set variable to indicate this message object's transfer completed
       // Bit-shifting doesn't work with an operator greater than 15, so
       // account for it
       if (Interrupt_ID <= 15)
       {
	       CAN_RX_COMPLETE.U32 |= (U16) (0x01 << Interrupt_ID);
       }
       else if (Interrupt_ID <= 0x1F)
       {
          CAN_RX_COMPLETE.U16[MSB] |= (U16) (0x01 << (Interrupt_ID - 16));
       }
   }

   // If an error occured, simply update the global variable and continue
   if (status & LEC)
   {
       // The LEC bits identify the type of error, but those are grouped here
       if ((status & LEC) != 0x07)
       {
          CAN_ERROR = 1;
       }
   }

   if (status & BOff)
   {
      CAN_ERROR = 1;
   }

   if (status & EWarn)
   {
      CAN_ERROR = 1;
   }

   // Old SFRPAGE is popped off stack when ISR exits
}

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------

