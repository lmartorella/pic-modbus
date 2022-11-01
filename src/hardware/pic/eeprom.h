#ifndef _ROM_INCLUDE_H_
#define _ROM_INCLUDE_H_

/**
 * This module defines the virtualization layer for data persistence
 * (e.g. saves node ID)
 */

#ifdef _IS_ETH_CARD

// PIC18 has memcpy
#define rom_read(rom,ram,size) memcpy(ram,rom,size)

// Erase the entire row (destination should be multiple of ROW_SIZE = 1Kb)
// and then copy the source bytes to the start of row, length should be at max 1Kb
void rom_write(const void* destination, const void* source, uint16_t length);
#define pers_poll()

#define EEPROM_MODIFIER const

#elif defined(_IS_PIC16F628_CARD) || defined(_IS_PIC16F1827_CARD) || defined(_IS_PIC16F887_CARD)

void rom_read(uint8_t sourceAddress, uint8_t* destination, uint8_t length);
void rom_write(uint8_t destinationAddr, const uint8_t* source, uint8_t length);

#define EEPROM_MODIFIER __eeprom

#endif
#endif
