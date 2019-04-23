#ifndef PERSISTENCE_INCLUDE_
#define PERSISTENCE_INCLUDE_

#include "bus.h"

#ifdef HAS_EEPROM
#include "hardware/eeprom.h"
#endif

/********************
  USER-DATA PART OF LOADER RECORD (PROGRAMMABLE)
  The record is immediately before the Configuration word, in the higher program memory.
  (loader do supports changing configuration words)
*/
typedef struct
{
  	// GUID:  application instance ID (used by user code)
	GUID deviceId;
    
    // Used by bus_client
#ifdef HAS_BUS_CLIENT
    BYTE address;
    BYTE filler;
#endif

    // Used by counter
#ifdef HAS_DIGITAL_COUNTER
    DWORD dcnt_counter;
#endif
    
} PersistentData;

// The cached copy of the EEPROM data, read at startup/init
// and then saved explicitly
extern PersistentData pers_data;

// Update copy of persistence
void pers_init();
// Poll WR 
#define pers_poll rom_poll
// Program the new content of the UserData
void pers_save();

#endif
