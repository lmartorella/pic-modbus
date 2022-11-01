#ifndef PERSISTENCE_INCLUDE_
#define PERSISTENCE_INCLUDE_

#include "guid.h"

/**
 * The system persistence record
 */
typedef struct
{
    /**
     * The stored device ID
     */
	GUID deviceId;
    
    /**
     * The bean node bus address (only used by bus secondary)
     */
    struct {
        uint8_t address;
        uint8_t filler;
    } sec;

#ifdef HAS_PERSISTENT_SINK_DATA
    PERSISTENT_SINK_DATA sinkData;
#endif    
} PersistentData;

// The cached copy of the EEPROM data, read at startup/init
// and then saved explicitly
extern PersistentData pers_data;

// Update copy of persistence
void pers_load();
/**
 * Poll long-running writing operations 
 * Returns true if active and require polling
 * */
_Bool pers_poll();
// Program the new content of the UserData
void pers_save();

#endif
