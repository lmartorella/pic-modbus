#include "pic-modbus/net.h"
#include "./eeprom.h"

/**
 * This module defines the virtualization layer for data persistence
 * (e.g. saves node ID)
 */

// Source: http://www.microchip.com/forums/m339126.aspx

// full 24-bit pointer support for flash zones
typedef union 
{
	struct
	{
		uint8_t lower;
		uint8_t higher;
		uint8_t upper;
	} bytes;
	uint32_t ptr;
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
// Prototype: void loadWData(const ram uint8_t* pData, uint8_t nLen) 
// Scope: Load the data into the 64 bytes registers 
//----------------------------------------------- 
static void loadWData(uint8_t* pData, uint8_t nLen) 
{ 
	uint8_t i; 
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
void rom_write(const void* destination, const void* source, uint16_t length)
{
	POINTER ptr;
	ptr.ptr = (uint32_t)(void*)destination;

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

_Bool pers_poll() {
    return false;
}
