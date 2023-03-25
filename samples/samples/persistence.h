#ifndef PERSISTENCE_INCLUDE_
#define PERSISTENCE_INCLUDE_

#ifdef __cplusplus
extern "C" {
#endif

    #ifdef _IS_ETH_CARD

// pic18 doesn't have data EEPROM. Use a whole EEPROM code page, and allocate it whole
#define __ADDRESS __at(0x1F800)
static EEPROM_MODIFIER char s_persistentDataFiller[0x400 - sizeof(PersistentData)] __at(0x1F800 + sizeof(PersistentData));
#define ROM_ADDR ((const void*)&s_persistentData)

// PIC18 has memcpy
#define rom_read(rom,ram,size) memcpy(ram,rom,size)

// Erase the entire row (destination should be multiple of ROW_SIZE = 1Kb)
// and then copy the source bytes to the start of row, length should be at max 1Kb
void rom_write();

#elif defined(_IS_PIC16F628_CARD) || defined(_IS_PIC16F1827_CARD) || defined(_IS_PIC16F887_CARD)

// pic16 XC8 doesn'c still support addressing __eeprom data. Assume it allocated at 0
#define EEPROM_MODIFIER __eeprom
#define __ADDRESS
//#define ROM_ADDR 0

#endif

/**
 * The system persistence record
 */
typedef struct {
    /**
     * The modbus slave bus address
     */
    uint8_t address;
} PersistentData;

// The cached copy of the EEPROM data, read at startup/init
// and then saved explicitly
extern PersistentData pers_data;
extern EEPROM_MODIFIER PersistentData rom_data __ADDRESS;

// Update copy of persistence (system space)
void pers_load();
// Program the new content of the syatem data
void pers_save();

/**
 * Poll long-running writing operations 
 * Returns true if active and require polling
 * */
_Bool pers_poll();

#ifdef __cplusplus
}
#endif

#endif
