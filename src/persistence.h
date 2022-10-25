#ifndef PERSISTENCE_INCLUDE_
#define PERSISTENCE_INCLUDE_

#ifdef HAS_EEPROM
#include "hardware/eeprom.h"
#endif

/**
 * The system persistence record
 */
typedef struct
{
    /**
     * The stored device ID
     */
	GUID deviceId;
    
    // Used by bus secondary
#ifdef HAS_RS485_BUS_SECONDARY
    /**
     * The bean node bus address
     */ 
    uint8_t address;
    uint8_t filler;
#endif

#ifdef HAS_PERSISTENT_SINK_DATA
    PERSISTENT_SINK_DATA sinkData;
#endif    
} PersistentData;

// The cached copy of the EEPROM data, read at startup/init
// and then saved explicitly
extern PersistentData pers_data;

// Update copy of persistence
void pers_load();
// Poll long-running writing operations 
#define pers_poll rom_poll
// Program the new content of the UserData
void pers_save();

#endif
