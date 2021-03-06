//-----------------------------------------------------------------------------
// LockByte.c
//-----------------------------------------------------------------------------
// Copyright 2008 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// The code sets the value of the Lock Byte.  The 'T622/3 and 'T326/7 Security
// Lock Byte write and read locks the EPROM memory.  If any of the write lock
// bits (7-4) or read lock bits (3-0) are cleared, the entire EPROM memory will
// be write or read locked.
//
// This code can be located using the Keil command-line linker flags.  Please
// see the project settings under Project->Tool Chain Integration...->Linker
// tab for more information.
//
// This code is part of the "T622_LockByte" project.
//
// How To Test:
//
// 1) Open the "T622_LockByte.wsp" project in the Silicon Labs IDE.
// 2) Compile and download the code to a 'T622 on a 'T62x Development Board.
// 3) Make sure the P1.2 LED jumper is in place on J10.
// 4) Run the code.
// 5) Verify the code is running; P1.2 LED should blink once a second.
// 6) Stop the code in the IDE.
// 7) Open the View->Debug Windows->Code Memory window in the IDE.
// 8) Verify the LockByte settings are in effect:
//       - If the EPROM is read-locked, the IDE should be unable to display the
//         code at address 0x0000.
//       - If the EPROM is write-locked, the IDE should be unable to write to
//         the code memory.
//
// Note: This code is intended to be used with Keil using the Keil command-line
// flags to locate the LockByte.
//
// Target:         C8051T622/3, 'T326/7
// Tool chain:     Keil / Raisonance
// Command Line:   None
//
// Release 1.0
//    -Initial Revision (TP)
//    -02 SEP 2008
//

unsigned char code LockByte = 0xF0;    // Read lock the device
