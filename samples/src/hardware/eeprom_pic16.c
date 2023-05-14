#include "pic-modbus/modbus.h"
#include "./eeprom.h"

/**
 * This module defines the virtualization layer for data persistence
 * (e.g. saves node ID)
 */

// Data still to write
static uint8_t s_length;
static uint8_t s_destinationAddr;
static const uint8_t* s_source;

void pers_load() {
    uint8_t* dest = (uint8_t*)&pers_data;
#ifdef _IS_PIC16F887_CARD
    EECON1bits.EEPGD = 0;
#endif
    s_destinationAddr = (uint8_t)&rom_data;
    for (uint8_t i = sizeof(PersistentData); i != 0; i--) { 
        // Wait for previous WR to finish
        while (EECON1bits.WR);
        EEADR = s_destinationAddr++;
        EECON1bits.RD = 1;
        *(dest++) = EEDATA;
        CLRWDT();
    }
    
    s_length = 0;
}

// Since writing is slow, cannot lose protocol data. Hence polling
_Bool pers_poll() {
    // Data to write and previous write operation finished?
    if (s_length > 0 && !EECON1bits.WR) {
        INTCONbits.GIE = 0;
        EECON1bits.WREN = 1;
#ifdef _IS_PIC16F887_CARD
        EECON1bits.EEPGD = 0;
#endif
        EEADR = s_destinationAddr++;
        EEDATA = *(s_source++);
        EECON2 = 0x55;
        EECON2 = 0xAA;
        EECON1bits.WR = 1;
        s_length--;

        INTCONbits.GIE = 1;
        EECON1bits.WREN = 0;
        return true;
    } else {
        return false;
    }
}

void pers_save() {
    s_length = sizeof(PersistentData);
    s_destinationAddr = (uint8_t)&rom_data;
    s_source = (const uint8_t*)&pers_data;
}
