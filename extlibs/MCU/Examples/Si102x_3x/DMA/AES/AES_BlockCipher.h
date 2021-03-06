#ifndef AES_BLOCK_CIPHER_H
#define AES_BLOCK_CIPHER_H
//=============================================================================
// AES_BlockCipher.h
//-----------------------------------------------------------------------------
// Copyright 2011 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// C File Description:
//
//
// Target:
//    Si102x/3x
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
//    Si102x/3x AES Library
//
// This software must be used in accordance with the End User License Agreement.
//
//=============================================================================
#ifndef COMPILER_DEFS_H
#include <compiler_defs.h>
#endif
#ifndef AES_DEFS_H
#include <AES_defs.h>
#endif
//=============================================================================
//-----------------------------------------------------------------------------
// typedefs for operation and status
//-----------------------------------------------------------------------------
typedef U8 AES_BLOCK_CIPHER_OPERATION;
typedef U8 AES_BLOCK_CIPHER_STATUS;
//-----------------------------------------------------------------------------
// ENCRYPT_DECRYPT_AND_SIZE_Enum defined in AES_defs.h
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// return codes defined in AES_defs.h
//-----------------------------------------------------------------------------
//=============================================================================
// Function Prototypes (API)
//=============================================================================
AES_BLOCK_CIPHER_STATUS AES_BlockCipher (AES_BLOCK_CIPHER_OPERATION operation,
   VARIABLE_SEGMENT_POINTER(plainText, U8, SEG_XDATA),
   VARIABLE_SEGMENT_POINTER(cipherText, U8, SEG_XDATA),
   VARIABLE_SEGMENT_POINTER(key, U8, SEG_XDATA),
    U16 blocks);
//=============================================================================
// End of file
//=============================================================================
#endif  // #ifdef AES_BLOCK_CIPHER_H
//=============================================================================

