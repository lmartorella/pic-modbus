#ifndef _PROGRAMMER_BOOTSTRAP_H_
#define _PROGRAMMER_BOOTSTRAP_H_

#include "../hardware/utilities.h"
#include "../hardware/fuses.h"


// Pointe to the FLASH utility (that program and reset the device at the end)
// The LoaderUDataToProgram and ConfigurationToProgram should be loaded with the new
// data to flash. Pre-load the data with the current content using the loader_loadCurrentUDConf() function.
typedef far rom void (*DoFlashHandler)(UINT16 startBlock, UINT16 lastBlock);

typedef struct
{
	BYTE major;
	BYTE minor;
} VERSION;

/********************
  LOADER RECORD (READ-ONLY)
********************/
typedef struct
{
		// Read the programmer version (for logs)
	VERSION programmerVersion;				
		// Pointe to the FLASH utility (that program and reset the device at the end)
		// The LoaderUDataToProgram and ConfigurationToProgram should be loaded with the new
		// data to flash. Pre-load the data with the current content using the loader_loadCurrentUDConf() function.
    DoFlashHandler flashHandler;			
  		// Start of loader memory data, this is the max application code programmable.
		//  (makes programmer code safe). The only memory exclusion are the configuration
 		//  words and the LoaderUserData memory.
	far void* blockFree;					
} LoaderRecord;
extern far rom LoaderRecord* LOADER_REC_PTR;


// Addition PIC RAM memory, contains the loader user data to write
//extern far ram struct LoaderUserData* LoaderUDataToProgram;

// Addition PIC RAM memory, contains the configuration words to write
//extern far ram BYTE ConfigurationToProgram[8];

// *******
// Prepare the data to write 
// *******

// Loads the current USERDATA+CONFIGURATION rom in the relevant ram sections 
// (LoaderUDataToProgram and ConfigurationToProgram)
// in order to overwrite it with the new data.
//void loader_loadCurrentUDConf();
// Clear the SPI RAM bitmap
//void loader_clearBitmap();
// Load a HEX line in SPI RAM and update the bitmap validity
//void loader_processHexLine(const ram char* line);
#endif
