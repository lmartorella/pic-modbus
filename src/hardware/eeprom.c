#include "../pch.h"
#include "../appio.h"
#include "eeprom.h"

/**
 * This module defines the virtualization layer for data peristence
 * (e.g. saves node ID)
 */

#ifdef _IS_ETH_CARD
// Source: http://www.microchip.com/forums/m339126.aspx

// full 24-bit pointer support for flash zones
typedef union 
{
	struct
	{
		BYTE lower;
		BYTE higher;
		BYTE upper;
	} bytes;
	UINT32 ptr;
} POINTER;

//----------------------------------------------- 
// Prototype: void setWTablePtr() 
// Scope: Reset the Flash write TBLPTR pointer to the start addr 
// Return Val: NONE 
//----------------------------------------------- 
static void setWTablePtr(POINTER* ptr) 
{ 
	TBLPTRU = ptr->bytes.upper; 
	TBLPTRH = ptr->bytes.higher; 
	TBLPTRL = ptr->bytes.lower; 
} 

//----------------------------------------------- 
// Prototype: void loadWData(const ram BYTE* pData, BYTE nLen) 
// Scope: Load the data into the 64 bytes registers 
//----------------------------------------------- 
static void loadWData(BYTE* pData, BYTE nLen) 
{ 
	BYTE i; 
	for (i = 0; i < nLen; i++) 
	{ 
		 TABLAT = *pData; 
		 pData++;
                 asm("TBLWTPOSTINC");
                 NOP();
	} 
} 

//----------------------------------------------- 
// Prototype: void rowWrite() 
// Scope: Perform the write of 64 bytes. 
// Remark: Call setWTablePtr() prior to this function 
//      to reset the write pointer 
//----------------------------------------------- 
static void rowWrite(void) 
{ 
	// Write, do not clear row
	EECON1bits.WREN = 1; 
	EECON1bits.FREE = 0; 
	// Disable interrupts
	INTCONbits.GIE = 0; 
	EECON2 = 0x55; 
	EECON2 = 0xaa; 
	EECON1bits.WR = 1; // CPU stall 
	
	// Re-enable interrupts and stop write
	INTCONbits.GIE = 1; 
	EECON1bits.WREN = 0; 
} 

//----------------------------------------------- 
// Prototype: void rowErase() 
// Scope: Perform the Row Erase of 1024 bytes. 
// Remark: Call setWTablePtr () prior to this function 
//      to reset the write pointer 
//----------------------------------------------- 
static void rowErase(void) 
{ 
	// Write, clear row
	EECON1bits.WREN = 1; 
	EECON1bits.FREE = 1; 
	// Disable interrupts
	INTCONbits.GIE = 0; 
	EECON2 = 0x55; 
	EECON2 = 0xaa; 
	EECON1bits.WR = 1; // CPU Stall 
	
	// Re-enable interrupts and stop write
	INTCONbits.GIE = 1; 
	EECON1bits.WREN = 0; 
} 

// Erase the entire row (destination should be multiple of ROW_SIZE = 1Kb)
// and then copy the source bytes to the start of row, length should be at max 1Kb
void rom_write(const void* destination, const void* source, WORD length)
{
	POINTER ptr;
	ptr.ptr = (UINT32)(void*)destination;

        // The destination pointer should be row aligned
        if ((ptr.ptr & 0x3FF) != 0)
        {
            fatal("ROWER");
        }
        // Max write the row size
        if (length > 0x400)
        {
            fatal("ROWL");
        }

        // Erase the row
	setWTablePtr(&ptr); 
	rowErase();

	setWTablePtr(&ptr); 
	loadWData(source, length);
	setWTablePtr(&ptr); 
	rowWrite(); 
}

#elif defined(_IS_PIC16F628_CARD) || defined(_IS_PIC16F1827_CARD) || defined(_IS_PIC16F887_CARD)

void rom_read(BYTE sourceAddress, BYTE* destination, BYTE length)
{
#ifdef _IS_PIC16F887_CARD
    EECON1bits.EEPGD = 0;
#endif
    for (; length > 0; length--) { 
        // Wait for previous WR to finish
        while (EECON1bits.WR);
        EEADR = sourceAddress++;
        EECON1bits.RD = 1;
        *(destination++) = EEDATA;
        CLRWDT();
    }
}

// Data still to write
static BYTE s_length;
static BYTE s_destinationAddr;
static const BYTE* s_source;

// Since writing is slow, cannot lose protocol data. Hence polling
void rom_poll() 
{
    // Data to write and previous write operation finished?
    if (s_length > 0 && !EECON1bits.WR) {
        INTCONbits.GIE = 0;
        EECON1bits.WREN = 1;
#ifdef _IS_PIC16F887_CARD
        EECON1bits.EEPGD = 0;
#endif
        EEADR = s_destinationAddr++;
        EEDATA = *(s_source++);
        EECON2 = 0x55;
        EECON2 = 0xAA;
        EECON1bits.WR = 1;
        s_length--;

        INTCONbits.GIE = 1;
        EECON1bits.WREN = 0;
    }
}

void rom_write(BYTE destinationAddr, const BYTE* source, BYTE length)
{
    s_length = length;
    s_destinationAddr = destinationAddr;
    s_source = source;
}

#endif
