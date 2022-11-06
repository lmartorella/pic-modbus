#include "net/net.h"
#include "./eeprom.h"

/**
 * This module defines the virtualization layer for data persistence
 * (e.g. saves node ID)
 */

void rom_read(uint8_t sourceAddress, void* destination, uint8_t length) {
    uint8_t* dest = destination;
#ifdef _IS_PIC16F887_CARD
    EECON1bits.EEPGD = 0;
#endif
    for (; length > 0; length--) { 
        // Wait for previous WR to finish
        while (EECON1bits.WR);
        EEADR = sourceAddress++;
        EECON1bits.RD = 1;
        *(dest++) = EEDATA;
        CLRWDT();
    }
}

// Data still to write
static uint8_t s_length;
static uint8_t s_destinationAddr;
static const uint8_t* s_source;

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

void rom_write(uint8_t destinationAddr, const void* source, uint8_t length)
{
    s_length = length;
    s_destinationAddr = destinationAddr;
    s_source = source;
}
