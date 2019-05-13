//-----------------------------------------------------------------------------
// F93x_SMBus_Slave_Multibyte_HWACK.c
//-----------------------------------------------------------------------------
// Copyright 2009 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// Example software to demonstrate the C8051F93x SMBus interface in Slave mode
// - Interrupt-driven SMBus implementation
// - Only slave states defined
// - Multi-byte SMBus data holders used for both transmit and receive
// - Timer1 used as SMBus clock rate (used only for free timeout detection)
// - Timer3 used by SMBus for SCL low timeout detection
// - ARBLOST support included
// - supports multiple-byte writes and multiple-byte reads
// - Pinout:
//    P0.0 -> SDA (SMBus)
//    P0.1 -> SCL (SMBus)
//
//    P1.6 -> LED
//
//    P2.0 -> C2D (debug interface)
//
//    all other port pins unused
//
// How To Test:
//
// 1) Verify that J13 and J14  are not populated.
// 2) Download code to a 'F93x device that is connected to a SMBus master.
// 3) Run the code.  The slave code will copy the write data to the read
//    data, so a successive write and read will effectively echo the data
//    written.  To verify that the code is working properly, verify on the
//    master that the data received is the same as the data written.
//
// Target:         C8051F93x
// Tool chain:     Generic
// Command Line:   None
//
//
// Release 1.2 
//    - Updated to be compatible with Raisonance Toolchain (FB)
//    - 14 APR 2010
//
// Release 1.1 
//    - Compiled and tested for C8051F930-TB (JH)
//    - 07 JULY 2009
//
// Release 1.0 
//    - 11 MAY 2009 (FB)

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include <compiler_defs.h>
#include <C8051F930_defs.h>            // SFR declarations

//-----------------------------------------------------------------------------
// Global Constants
//-----------------------------------------------------------------------------

#define  SYSCLK         24500000       // System clock frequency in Hz

#define  SMB_FREQUENCY  10000         // Target SMBus frequency
                                       // This example supports between 10kHz
                                       // and 100kHz

#define  EXTHOLD_ENABLE  1             // Sets EXTHOLD to 1 to prevent the MCU
                                       // from recieving an interrupt when 
                                       // another slave is being addressed.

#define  WRITE          0x00           // SMBus WRITE command
#define  READ           0x01           // SMBus READ command

#define LED_ON           0             // Turns the LED on
#define LED_OFF          1             // Turns the LED off

#define  SLAVE_ADDR     0xF0           // Device addresses (7 bits,
                                       // lsb is a don't care)

// Status vector - top 4 bits only
#define  SMB_SRADD      0x20           // (SR) slave address received
                                       //    (also could be a lost
                                       //    arbitration)
#define  SMB_SRSTO      0x10           // (SR) STOP detected while SR or ST,
                                       //    or lost arbitration
#define  SMB_SRDB       0x00           // (SR) data byte received, or
                                       //    lost arbitration
#define  SMB_STDB       0x40           // (ST) data byte transmitted
#define  SMB_STSTO      0x50           // (ST) STOP detected during a
                                       //    transaction; bus error
// End status vector definition

#define  NUM_BYTES_WR   3              // Number of bytes to write
                                       // Slave <- Master
#define  NUM_BYTES_RD   3              // Number of bytes to read
                                       // Slave -> Master

//-----------------------------------------------------------------------------
// Global VARIABLES
//-----------------------------------------------------------------------------

// Global holder for SMBus data.
// All receive data is written here
// NUM_BYTES_WR used because an SMBus write is Master->Slave
U8 SMB_DATA_IN[NUM_BYTES_WR];

// Global holder for SMBus data.
// All transmit data is read from here
// NUM_BYTES_RD used because an SMBus read is Slave->Master
U8 SMB_DATA_OUT[NUM_BYTES_RD];

bit DATA_READY = 0;                    // Set to '1' by the SMBus ISR
                                       // when a new data byte has been
                                       // received.

#if(EXTHOLD_ENABLE)
U8 first_entry = 1;                    // Set to 1 to indicate that 
                                       // the start bit should be set
                                       // on the next SMBus interrupt
#endif

SBIT (YELLOW_LED, SFR_P1, 6);          // YELLOW_LED==LED_ON means ON

//-----------------------------------------------------------------------------
// Function PROTOTYPES
//-----------------------------------------------------------------------------

void SMBus_Init (void);
void Timer0_Init (void);
void Timer1_Init (void);
void Timer3_Init (void);
void Port_Init (void);

INTERRUPT_PROTO (TIMER3_ISR, INTERRUPT_TIMER3);
INTERRUPT_PROTO (SMBUS0_ISR, INTERRUPT_SMBUS0);
INTERRUPT_PROTO (TIMER0_ISR, INTERRUPT_TIMER0);

//-----------------------------------------------------------------------------
// MAIN Routine
//-----------------------------------------------------------------------------
//
// Main routine performs all configuration tasks, then waits for SMBus
// communication.
//
//-----------------------------------------------------------------------------
void main (void)
{
   U8 i;

   PCA0MD &= ~0x40;                    // WDTE = 0 (Disable watchdog
                                       // timer)


   OSCICN |= 0x80;                     // Enable precision internal oscillator
   CLKSEL =  0x00;                     // Select precision internal oscillator
                                       // divided by 1 as system clock

   Port_Init();                        // Initialize Crossbar and GPIO
   Timer0_Init();                      // Configure Timer0
   Timer1_Init();                      // Configure Timer1 for use
                                       // with SMBus baud rate

   Timer3_Init ();                     // Configure Timer3 for use with
                                       // SCL low timeout detect

   SMBus_Init ();                      // Configure and enable SMBus

   EIE1 |= 0x01;                       // Enable the SMBus interrupt
   
   YELLOW_LED = LED_OFF;

   EA = 1;                             // Global interrupt enable

   // Initialize the outgoing data array in case a read is done before a
   // write
   for (i = 0; i < NUM_BYTES_RD; i++)
   {
      SMB_DATA_OUT[i] = 0xFD;
   }

   while(1)
   {
      while(!DATA_READY);              // New SMBus data received?
      DATA_READY = 0;

      // Copy the data from the input array to the output array
      for (i = 0; i < NUM_BYTES_RD; i++)
      {
         SMB_DATA_OUT[i] = SMB_DATA_IN[i];
      }

      YELLOW_LED = !YELLOW_LED;
   }
}

//-----------------------------------------------------------------------------
// SMBus_Init()
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// SMBus configured as follows:
// - SMBus enabled
// - Slave mode not inhibited
// - Timer1 used as clock source. The maximum SCL frequency will be
//   approximately 1/3 the Timer1 overflow rate
// - Setup and hold time extensions enabled
// - Bus Free and SCL Low timeout detection enabled
//
//-----------------------------------------------------------------------------
void SMBus_Init (void)
{
   SMB0CF = 0x1D;                      // Use Timer1 overflows as SMBus clock
                                       // source;
                                       // Enable slave mode;
                                       // Enable setup & hold time
                                       // extensions;
                                       // Enable SMBus Free timeout detect;
                                       // Enable SCL low timeout detect;

   SMB0CF |= 0x80;                     // Enable SMBus;


   #if(EXTHOLD_ENABLE)
   SMB0CF |= 0x10;                     // Set EXTHOLD 
   #endif
      

   SMB0ADM = 0xFD;                     // Enable Hardware Acknowledge
                                       // Mask slave SLVM[6:1]

   SMB0ADR = SLAVE_ADDR;               // Ack addresses 0xF2, 0xF0
}


//-----------------------------------------------------------------------------
// Timer0_Init()
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Timer 0 configured to overflow at 100 kHz.
//
//-----------------------------------------------------------------------------
void Timer0_Init (void)
{

   CKCON |= 0x04;                      // Timer0 counts SYSCLK 

   TMOD |= 0x02;                       // Timer0 in 8-bit auto-reload mode

   // Timer 0 configured to overflow every 10uS
   TH0 = -(SYSCLK/100000);

   TL0 = 0xFF;                         // Init Timer0
   ET0 = 1;                            // Enable Timer0 Interrupt
   TR0 = 1;                            // Timer0 enabled
}

//-----------------------------------------------------------------------------
// Timer1_Init()
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Timer1 configured as the SMBus clock source as follows:
// - Timer1 in 8-bit auto-reload mode
// - SYSCLK or SYSCLK / 4 as Timer1 clock source
// - Timer1 overflow rate => 3 * SMB_FREQUENCY
// - The resulting SCL clock rate will be ~1/3 the Timer1 overflow rate
// - Timer1 enabled
//
//-----------------------------------------------------------------------------
void Timer1_Init (void)
{

// Make sure the Timer can produce the appropriate frequency in 8-bit mode
// Supported SMBus Frequencies range from 10kHz to 100kHz.  The CKCON register
// settings may need to change for frequencies outside this range.
#if ((SYSCLK/SMB_FREQUENCY/3) < 255)
   #define SCALE 1
      CKCON |= 0x08;                   // Timer1 clock source = SYSCLK
#elif ((SYSCLK/SMB_FREQUENCY/4/3) < 255)
   #define SCALE 4
      CKCON |= 0x01;
      CKCON &= ~0x0A;                  // Timer1 clock source = SYSCLK / 4
#endif

   TMOD |= 0x20;                       // Timer1 in 8-bit auto-reload mode

   // Timer1 configured to overflow at 1/3 the rate defined by SMB_FREQUENCY
   TH1 = (unsigned char) -(SYSCLK/SMB_FREQUENCY/SCALE/3);

   TL1 = TH1;                          // Init Timer1

   TR1 = 1;                            // Timer1 enabled
}

//-----------------------------------------------------------------------------
// Timer3_Init()
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Timer3 configured for use by the SMBus low timeout detect feature as
// follows:
// - Timer3 in 16-bit auto-reload mode
// - SYSCLK/12 as Timer3 clock source
// - Timer3 reload registers loaded for a 25ms overflow period
// - Timer3 pre-loaded to overflow after 25ms
// - Timer3 enabled
//
//-----------------------------------------------------------------------------
void Timer3_Init (void)
{
   TMR3CN = 0x00;                      // Timer3 configured for 16-bit auto-
                                       // reload, low-byte interrupt disabled

   CKCON &= ~0x40;                     // Timer3 uses SYSCLK/12

   TMR3RL = (U16) -(SYSCLK/12/40);     // Timer3 configured to overflow after
   TMR3 = TMR3RL;                      // ~25ms (for SMBus low timeout detect):
                                       // 1/.025 = 40

   EIE1 |= 0x80;                       // Timer3 interrupt enable
   TMR3CN |= 0x04;                     // Start Timer3
}

//-----------------------------------------------------------------------------
// Port_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Configure the Crossbar and GPIO ports.
//
// P0.0   digital   open-drain    SMBus SDA
// P0.1   digital   open-drain    SMBus SCL
//
// P1.6   digital   push-pull     YELLOW_LED
//
// all other port pins unused
//
//-----------------------------------------------------------------------------
void Port_Init (void)
{

   P1MDOUT |= 0x40;                    // Make the LED (P1.6) a push-pull
                                       // output

   XBR0 = 0x04;                        // Enable SMBus pins
   XBR2 = 0x40;                        // Enable crossbar and weak pull-ups


}

//-----------------------------------------------------------------------------
// Interrupt Service Routines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Timer0 Interrupt Service Routine (ISR)
//-----------------------------------------------------------------------------
INTERRUPT(TIMER0_ISR, INTERRUPT_TIMER0)
{
   ACK = 0;
}

//-----------------------------------------------------------------------------
// SMBus Interrupt Service Routine (ISR)
//-----------------------------------------------------------------------------
//
// SMBus ISR state machine
// - Slave only implementation - no master states defined
// - All incoming data is written to global variable <SMB_data_IN>
// - All outgoing data is read from global variable <SMB_data_OUT>
//
//-----------------------------------------------------------------------------
INTERRUPT(SMBUS0_ISR, INTERRUPT_SMBUS0)
{
   static U8 sent_byte_counter;
   static U8 rec_byte_counter;

   #if(EXTHOLD_ENABLE)
   if(first_entry)
   {
      first_entry = 0;
      STA = 1;
      SMB0CF &= ~0x10;                  // Clear EXTHOLD to allow detection 
      TR0 = 0;                          // of a repeated start
      TF0 = 0;                          // Stop Timer0 and clear flag
   
   }                                    

   #endif

   if (ARBLOST == 0)
   {
      switch (SMB0CN & 0xF0)           // Decode the SMBus status vector
      {
         // Slave Receiver: Start+Address received
         case  SMB_SRADD:

            STA = 0;                   // Clear STA bit
            
            if((SMB0DAT & SMB0ADM & 0xFE) == (SMB0ADR & SMB0ADM & 0xFE))
            {
               sent_byte_counter = 1;     // Reinitialize the data counters
               rec_byte_counter = 1;      // Data counters are valid starting
                                          // with the second interrupt

               if((SMB0DAT&0x01) == READ) // If the transfer is a master READ,
               {
                  // Prepare outgoing byte
                  SMB0DAT = SMB_DATA_OUT[sent_byte_counter-1];
                  sent_byte_counter++;
               } else
               {
                  ACK = 1;                // Auto-ACK the next master WRITE
               }
            } else 
            {
               SI = 0;
               return;
            }
            break;

         // Slave Receiver: Data received
         case  SMB_SRDB:
            
            // If there are two or more bytes remaining to be received
            if (rec_byte_counter < NUM_BYTES_WR)
            {
               // Store incoming data
               SMB_DATA_IN[rec_byte_counter-1] = SMB0DAT;
               rec_byte_counter++;
               
               ACK = 1;                // Auto-ACK next data byte

            }
            // Otherwise, we are receiving the last byte
            else  // (rec_byte_counter == NUM_BYTES_WR)
            {  
               // Store incoming data
               SMB_DATA_IN[rec_byte_counter-1] = SMB0DAT;

               DATA_READY = 1;         // Indicate new data fully received
               ACK = 0;                // Auto-NACK next data byte
            }

            break;

         // Slave Receiver: Stop received while either a Slave Receiver or
         // Slave Transmitter
         case  SMB_SRSTO:

            STO = 0;                   // STO must be cleared by software when
                                       // a STOP is detected as a slave

            #if(EXTHOLD_ENABLE)
            first_entry = 1;
            SMB0CF |= 0x10;            // Enable EXTHOLD to prevent being 
                                       // interrupted on an unmatching slave
                                       // address
            TR0 = 1;                   // Start Timer
            #endif

            break;

         // Slave Transmitter: Data byte transmitted
         case  SMB_STDB:

            if (ACK == 1)              // If Master ACK's, send the next byte
            {
               if (sent_byte_counter <= NUM_BYTES_RD)
               {
                  // Prepare next outgoing byte
                  SMB0DAT = SMB_DATA_OUT[sent_byte_counter-1];
                  sent_byte_counter++;
               }
            }                          // Otherwise, do nothing
            break;

         // Slave Transmitter: Arbitration lost, Stop detected
         //
         // This state will only be entered on a bus error condition.
         // In normal operation, the slave is no longer sending data or has
         // data pending when a STOP is received from the master, so the TXMODE
         // bit is cleared and the slave goes to the SRSTO state.
         case  SMB_STSTO:

            STO = 0;                   // STO must be cleared by software when
                                       // a STOP is detected as a slave

            #if(EXTHOLD_ENABLE)
            first_entry = 1;           // Mark next entry as a new transfer
            SMB0CF |= 0x10;            // Enable EXTHOLD to prevent being 
                                       // interrupted on an unmatching slave
                                       // address
            TR0 = 1;                   // Start Timer
            #endif

            break;

         // Default: all other cases undefined
         default:

            SMB0CF &= ~0x80;           // Reset communication
            SMB0CF |= 0x80;
            STA = 0;
            STO = 0;
            ACK = 0;
            
            #if(EXTHOLD_ENABLE)
            first_entry = 1;
            SMB0CF |= 0x10;            // Enable EXTHOLD to prevent being 
                                       // interrupted on an unmatching slave
                                       // address
            TR0 = 1;                   // Start Timer
            #endif

            break;
      }
   }
   // ARBLOST = 1, Abort failed transfer
   else
   {
      STA = 0;
      STO = 0;
      ACK = 0;
      
      #if(EXTHOLD_ENABLE)
      first_entry = 1;           // Mark next entry as a new transfer
      SMB0CF |= 0x10;            // Enable EXTHOLD to prevent being 
                                 // interrupted on an unmatching slave
                                 // address
      TR0 = 1;                   // Start Timer
      #endif
   }

   SI = 0;                             // Clear SMBus interrupt flag
}


//-----------------------------------------------------------------------------
// Timer3 Interrupt Service Routine (ISR)
//-----------------------------------------------------------------------------
//
// A Timer3 interrupt indicates an SMBus SCL low timeout.
// The SMBus is disabled and re-enabled here
//
//-----------------------------------------------------------------------------
INTERRUPT(TIMER3_ISR, INTERRUPT_TIMER3)
{
   SMB0CF &= ~0x80;                    // Disable SMBus
   SMB0CF |= 0x80;                     // Re-enable SMBus
   TMR3CN &= ~0x80;                    // Clear Timer3 interrupt-pending flag

   #if(EXTHOLD_ENABLE)
      first_entry = 1;                 // Mark next entry as a new transfer
      SMB0CF |= 0x10;                  // Enable EXTHOLD to prevent being 
                                       // interrupted on an unmatching slave
                                       // address
      TR0 = 1;                         // Start Timer
   #endif
}

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------
