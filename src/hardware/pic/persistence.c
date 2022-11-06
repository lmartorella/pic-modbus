#include "net/net.h"
#include "./eeprom.h"

/**
 * The RAM backed-up data for writings and readings
 */
PersistentData pers_data;

#ifdef _IS_ETH_CARD
// pic18 doesn't have data EEPROM. Use a whole EEPROM code page, and allocate it whole
#define __ADDRESS __at(0x1F800)
static EEPROM_MODIFIER char s_persistentDataFiller[0x400 - sizeof(PersistentData)] __at(0x1F800 + sizeof(PersistentData));
#define ROM_ADDR ((const void*)&s_persistentData)
#else
// pic16 XC8 doesn'c still support addressing __eeprom data. Assume it allocated at 0
#define __ADDRESS
#define ROM_ADDR 0
#endif

static EEPROM_MODIFIER PersistentData s_persistentData __ADDRESS = { 
    // Zero GUID by default, it means unassigned
    .deviceId = { 0, 0, 0, 0, 0 }, 

    // Used by bus secondary
    .sec = {
        .address = UNASSIGNED_SUB_ADDRESS, 
        .filler = 0xff
    },
    
    .custom = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

void pers_load() {
    rom_read(ROM_ADDR, (void*)&pers_data, sizeof(PersistentData));
}

void pers_save() {
    rom_write(ROM_ADDR, (void*)&pers_data, sizeof(PersistentData));
}
