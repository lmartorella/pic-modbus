#include "net/net.h"
#include "./eeprom.h"

/**
 * The RAM backed-up data for writings and readings
 */
PersistentData pers_data;

EEPROM_MODIFIER PersistentData rom_data __ADDRESS = { 
    .address = UNASSIGNED_STATION_ADDRESS
};
