/*********************************************************************
 *
 *               Microchip File System
 *
 *********************************************************************
 * FileName:        MPFS2.h
 * Dependencies:    StackTsk.H
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F, PIC32
 * Compiler:        Microchip C32 v1.05 or higher
 *					Microchip C30 v3.12 or higher
 *					Microchip C18 v3.30 or higher
 *					HI-TECH PICC-18 PRO 9.63PL2 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * Copyright (C) 2002-2009 Microchip Technology Inc.  All rights
 * reserved.
 *
 * Microchip licenses to you the right to use, modify, copy, and
 * distribute:
 * (i)  the Software when embedded on a Microchip microcontroller or
 *      digital signal controller product ("Device") which is
 *      integrated into Licensee's product; or
 * (ii) ONLY the Software driver source files ENC28J60.c, ENC28J60.h,
 *		ENCX24J600.c and ENCX24J600.h ported to a non-Microchip device
 *		used in conjunction with a Microchip ethernet controller for
 *		the sole purpose of interfacing with the ethernet controller.
 *
 * You should refer to the license agreement accompanying this
 * Software for additional information regarding your rights and
 * obligations.
 *
 * THE SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION, ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * MICROCHIP BE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF
 * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS
 * BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE
 * THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER
 * SIMILAR COSTS, WHETHER ASSERTED ON THE BASIS OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE), BREACH OF WARRANTY, OR OTHERWISE.
 *
 *
 *V5.36  ----  STACK_USE_MPFS has been removed
 ********************************************************************/

#ifndef __MPFS2_H
#define __MPFS2_H


/****************************************************************************
  Section:
	Storage Type Configurations
  ***************************************************************************/

/*	
	#if defined(STACK_USE_MPFS) && defined(STACK_USE_MPFS2)
		#error Both MPFS and MPFS2 are included
	#endif
*/

	#if defined(MPFS_USE_EEPROM)
		#if defined(USE_EEPROM_25LC1024)
			#define MPFS_WRITE_PAGE_SIZE		(256u)	// Defines the size of a page in EEPROM
		#else
			#define MPFS_WRITE_PAGE_SIZE		(64u)	// Defines the size of a page in EEPROM
		#endif
	#endif

/****************************************************************************
  Section:
	Type Definitions
  ***************************************************************************/
	#define MPFS2_FLAG_ISZIPPED		((uint16_t)0x0001)	// Indicates a file is compressed with GZIP compression
	#define MPFS2_FLAG_HASINDEX		((uint16_t)0x0002)	// Indicates a file has an associated index of dynamic variables
	#define MPFS_INVALID			(0xffffffffu)	// Indicates a position pointer is invalid
	#define MPFS_INVALID_FAT		(0xffffu)		// Indicates an invalid FAT cache
	#define MPFS_INVALID_HANDLE 	(0xffu)			// Indicates that a handle is not valid
	typedef uint32_t MPFS_PTR;							// MPFS Pointers are currently DWORDs
	typedef uint8_t MPFS_HANDLE;						// MPFS Handles are currently stored as BYTEs


	// Stores each file handle's information
	// Handles are free when addr = MPFS_INVALID
	typedef struct
	{
		MPFS_PTR addr;		// Current address in the file system
		uint32_t bytesRem;		// How many bytes remain in this file
		uint16_t fatID;			// ID of which file in the FAT was accessed
	} MPFS_STUB;
	
	// Indicates the method for MPFSSeek
	typedef enum
	{
		MPFS_SEEK_START		= 0u,	// Seek forwards from the front of the file
		MPFS_SEEK_END,				// Seek backwards from the end of the file
		MPFS_SEEK_FORWARD,			// Seek forward from the current position
		MPFS_SEEK_REWIND			// See backwards from the current position
	} MPFS_SEEK_MODE;
	
	// Stores the data for an MPFS2 FAT record
	typedef struct
	{
		uint32_t string;		// Pointer to the file name
		uint32_t data;			// Address of the file data
		uint32_t len;			// Length of file data
		uint32_t timestamp;	// Timestamp of file
		uint32_t microtime;	// Microtime stamp of file
		uint16_t flags;			// Flags for this file
	} MPFS_FAT_RECORD;

/****************************************************************************
  Section:
	Function Definitions
  ***************************************************************************/

void MPFSInit(void);

MPFS_HANDLE MPFSOpen(uint8_t* cFile);
#if defined(__18CXX)
	MPFS_HANDLE MPFSOpenROM(ROM uint8_t* cFile);
#else
	// Non-ROM variant for C30 / C32
	#define MPFSOpenROM(a)	MPFSOpen((uint8_t*) a);
#endif
MPFS_HANDLE MPFSOpenID(uint16_t hFatID);
void MPFSClose(MPFS_HANDLE hMPFS);

_Bool MPFSGet(MPFS_HANDLE hMPFS, uint8_t* c);
uint16_t MPFSGetArray(MPFS_HANDLE hMPFS, uint8_t* cData, uint16_t wLen);
_Bool MPFSGetLong(MPFS_HANDLE hMPFS, uint32_t* ul);
_Bool MPFSSeek(MPFS_HANDLE hMPFS, uint32_t dwOffset, MPFS_SEEK_MODE tMode);
#if defined(__C30__)
	// Assembly function to read all three bytes of program memory for 16-bit parts
	extern uint32_t ReadProgramMemory(uint32_t address);
#endif

MPFS_HANDLE MPFSFormat(void);
void MPFSPutEnd(_Bool final);
uint16_t MPFSPutArray(MPFS_HANDLE hMPFS, uint8_t* cData, uint16_t wLen);

uint32_t MPFSGetTimestamp(MPFS_HANDLE hMPFS);
uint32_t MPFSGetMicrotime(MPFS_HANDLE hMPFS);
uint16_t MPFSGetFlags(MPFS_HANDLE hMPFS);
uint32_t MPFSGetSize(MPFS_HANDLE hMPFS);
uint32_t MPFSGetBytesRem(MPFS_HANDLE hMPFS);
MPFS_PTR MPFSGetStartAddr(MPFS_HANDLE hMPFS);
MPFS_PTR MPFSGetEndAddr(MPFS_HANDLE hMPFS);
_Bool MPFSGetFilename(MPFS_HANDLE hMPFS, uint8_t* cName, uint16_t wLen);
uint32_t MPFSGetPosition(MPFS_HANDLE hMPFS);
uint16_t MPFSGetID(MPFS_HANDLE hMPFS);

// Alias of MPFSGetPosition
#define MPFSTell(a)	MPFSGetPosition(a)

#endif
