#include "net/net.h"
#include "persistence.h"
#include "protocol.h"
#include "appio.h"
#include "./eeprom.h"

/**
 * The RAM backed-up data for writings and readings
 */
PersistentData pers_data;

#ifdef _IS_ETH_CARD
#   define __ADDRESS __at(0x1F800)
#   define PERSISTENT_SIZE 0x10
#else
#   define __ADDRESS
#   define PERSISTENT_SIZE (sizeof(PersistentData))
#endif

static EEPROM_MODIFIER PersistentData s_persistentData __ADDRESS = { 
    // Zero GUID by default, it means unassigned
    { 0, 0, 0, 0, 0 }, 

    // Used by bus secondary
    {
        UNASSIGNED_SUB_ADDRESS, 
        0xff
    },
    
#ifdef HAS_PERSISTENT_SINK_DATA
    PERSISTENT_SINK_DATA_DEFAULT_DATA
#endif
};


#ifdef _IS_ETH_CARD
static EEPROM_MODIFIER char s_persistentDataFiller[0x400 - PERSISTENT_SIZE] __at(0x1F800 + PERSISTENT_SIZE);
#define ROM_ADDR ((const void*)&s_persistentData)
#elif defined(_IS_PIC16F628_CARD) || defined(_IS_PIC16F1827_CARD) || defined(_IS_PIC16F887_CARD)
#define ROM_ADDR 0
#endif

void pers_load()
{
    rom_read(ROM_ADDR, (uint8_t*)&pers_data, PERSISTENT_SIZE);
}

void pers_save()
{
    rom_write(ROM_ADDR, (uint8_t*)&pers_data, PERSISTENT_SIZE);
}
