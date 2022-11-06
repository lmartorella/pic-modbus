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
} PersistentNetData;

// The cached copy of the EEPROM data, read at startup/init
// and then saved explicitly
extern PersistentNetData pers_net_data;

// Update copy of persistence (system space)
void pers_net_load(void);
// Program the new content of the syatem data
void pers_net_save(void);

// Update copy of user persistence
void pers_data_load(void* buffer, uint8_t size);
// Program the new content of the user data
void pers_data_save(void* buffer, uint8_t size);

/**
 * Poll long-running writing operations 
 * Returns true if active and require polling
 * */
_Bool pers_poll(void);

#endif
