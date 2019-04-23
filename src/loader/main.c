
#include <p18f87j60.h>
#include "programmer.h"
#include "../hardware/spiram.h"

//#define START_MEM (MAX_PROG_MEM - 256)
#pragma udata my_section_1
static BYTE s_bitmap[256];

#pragma udata my_section_2
static BYTE s_newConfiguration[8];
//static LoaderUserData s_newUserData;

static void DoFlash();

#pragma romdata overlay startzone
static const rom BYTE s_silly;

#pragma romdata loaderrec	// LOADER_PTR = MAX_PROG_MEM - 0xA
static const rom LoaderRecord LREC = 
{
	{0,0},
	(DoFlashHandler)&DoFlash,
	(far void*)&s_silly
};

#pragma code

// Size of block validity bitmap, located at the end of the SPI RAM
#define BITMAP_SIZE (MAX_PROG_MEM / ROM_BLOCK_SIZE / 8)

// Expect SPI ram = PIC ROM
#define SPIRAM_SIZE MAX_PROG_MEM

// The bitmap validity address should be located at the end 
//  (overlays the last 4 blocks, occupied by the loader itself and not useable)
#define SPIRAM_BITMAP_ADDRESS (SPIRAM_SIZE - BITMAP_SIZE)

// Then new CONFIGURATION words
#define SPIRAM_CONF_ADDRESS (SPIRAM_BITMAP_ADDRESS - CONFIGURATION_SIZE)

// Then new UDATA words
//#define SPIRAM_UDATA_ADDRESS (SPIRAM_CONF_ADDRESS - sizeof(LoaderUserData))

// This is the MAX ADDRESS of SPI-RAM that can be used
//  The programmer should check this in addition to the startzone
//  to avoid programmer overwrite
//#define MAX_SPIRAM_USAGE SPIRAM_UDATA_ADDRESS
#define MAX_SPIRAM_USAGE SPIRAM_CONF_ADDRESS

void DoFlash(void)
{	
	// Disable ALL interrupts
	INTCONbits.GIEH = 0;	
	INTCONbits.GIEL = 0;	

	// Read validity bitmap from SPI RAM
	sram_read(s_bitmap, SPIRAM_BITMAP_ADDRESS, BITMAP_SIZE);
	// Read new configuration from SPI RAM
	sram_read(s_newConfiguration, SPIRAM_CONF_ADDRESS, CONFIGURATION_SIZE);
	// Read new userData from SPI RAM
	//sram_read((void*)&s_newUserData, SPIRAM_UDATA_ADDRESS, sizeof(LoaderUserData));

	// Now cycle ram bitmap validity for banks.
	
}

void main()
{
}
