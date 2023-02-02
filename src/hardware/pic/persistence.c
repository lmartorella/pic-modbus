#include "net/net.h"
#include "./eeprom.h"

/**
 * The RAM backed-up data for writings and readings
 */
PersistentData pers_data;

EEPROM_MODIFIER PersistentData rom_data __ADDRESS = { 
    // Zero GUID by default, it means unassigned
    .deviceId = { 0, 0, 0, 0, 0 }, 

    // Used by bus secondary
    .sec = {
        .address = UNASSIGNED_STATION_ADDRESS, 
        .filler = 0xff
    },
    
    .custom = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};
