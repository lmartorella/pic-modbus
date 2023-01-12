#ifndef PERSISTENCE_INCLUDE_
#define PERSISTENCE_INCLUDE_

#include "guid.h"

#ifdef __cplusplus
extern "C" {
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
    
    /**
     * The bean node bus address (only used by bus secondary)
     */
    struct {
        uint8_t address;
        uint8_t filler;
    } sec;
    
    /**
     * Custom data (16 bytes)
     */
    struct {
        uint8_t custom[16];        
    } custom;
} PersistentData;

// The cached copy of the EEPROM data, read at startup/init
// and then saved explicitly
extern PersistentData pers_data;

// Update copy of persistence (system space)
void pers_load(void);
// Program the new content of the syatem data
void pers_save(void);

/**
 * Poll long-running writing operations 
 * Returns true if active and require polling
 * */
_Bool pers_poll(void);

#ifdef __cplusplus
}
#endif

#endif
