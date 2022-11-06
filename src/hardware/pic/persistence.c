#include "net/net.h"
#include "./eeprom.h"

/**
 * The RAM backed-up data for writings and readings
 */
PersistentNetData pers_net_data;

#ifdef _IS_ETH_CARD
#   define __ADDRESS __at(0x1F800)
#else
#   define __ADDRESS
#endif

#define PERSISTENT_SIZE (sizeof(PersistentNetData))

static EEPROM_MODIFIER PersistentNetData s_persistentData __ADDRESS = { 
    // Zero GUID by default, it means unassigned
    { 0, 0, 0, 0, 0 }, 

    // Used by bus secondary
    {
        UNASSIGNED_SUB_ADDRESS, 
        0xff
    }   
};


#ifdef _IS_ETH_CARD
static EEPROM_MODIFIER char s_persistentDataFiller[0x400 - PERSISTENT_SIZE] __at(0x1F800 + PERSISTENT_SIZE);
#define ROM_ADDR ((const void*)&s_persistentData)
#elif defined(_IS_PIC16F628_CARD) || defined(_IS_PIC16F1827_CARD) || defined(_IS_PIC16F887_CARD)
#define ROM_ADDR 0
#endif

void pers_net_load() {
    rom_read(ROM_ADDR, (void*)&pers_net_data, PERSISTENT_SIZE);
}

void pers_net_save() {
    rom_write(ROM_ADDR, (void*)&pers_net_data, PERSISTENT_SIZE);
}

void pers_data_load(void* buffer, uint8_t size) {
    
}

void pers_data_save(void* buffer, uint8_t size) {
    
}
