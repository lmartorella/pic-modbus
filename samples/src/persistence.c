#include "pic-modbus/modbus.h"
#include "./eeprom.h"

/**
 * The RAM backed-up data for writings and readings
 */
PersistentData pers_data;

// RS485 Modbus defines station address in the range 1 to 247. 0 is used for broadcast messages without acknowledge.
// So use 254 as special "unassigned" address. When the AUTO_REGISTER function is called, the only device in auto mode in the bus
// should reply and change his address.
#define UNASSIGNED_STATION_ADDRESS 254

EEPROM_MODIFIER PersistentData rom_data __ADDRESS = { 
    .address = UNASSIGNED_STATION_ADDRESS
};
