#include "pch.h"
#include "persistence.h"
#include "appio.h"

/**
 * The RAM backed-up data for writings and readings
 */
PersistentData pers_data;

#ifdef _IS_ETH_CARD
#   define __ADDRESS @ 0x1F800
#   define PERSISTENT_SIZE 0x10
#else
#   define __ADDRESS
#   define PERSISTENT_SIZE (sizeof(PersistentData))
#endif

static EEPROM_MODIFIER PersistentData s_persistentData __ADDRESS = { 
    // Zero GUID by default, it means unassigned
    { 0, 0, 0, 0, 0 }, 

#ifdef HAS_RS485_BUS_CLIENT
    // Used by bus_client
    UNASSIGNED_SUB_ADDRESS, 
    0xff,
#endif
    
#ifdef HAS_PERSISTENT_SINK_DATA
    PERSISTENT_SINK_DATA_DEFAULT_DATA
#endif
};


#ifdef _IS_ETH_CARD
static EEPROM_MODIFIER char s_persistentDataFiller[0x400 - PERSISTENT_SIZE] @ (0x1F800 + PERSISTENT_SIZE);
#define ROM_ADDR ((const void*)&s_persistentData)
#elif defined(_IS_PIC16F628_CARD) || defined(_IS_PIC16F1827_CARD) || defined(_IS_PIC16F887_CARD)
#define ROM_ADDR 0
#endif

#if defined(_CONF_RASPBIAN)
#include <unistd.h>
#include <limits.h>

static FILE* pers_fopen(const char* mode) {
    char cwd[PATH_MAX];
    flog("Opening: %shome.mem", getcwd(cwd, PATH_MAX));
    return fopen("home.mem", mode);
}
#endif

void pers_load()
{
#if defined(HAS_EEPROM)
    rom_read(ROM_ADDR, (BYTE*)&pers_data, PERSISTENT_SIZE);
#elif defined(_CONF_RASPBIAN)
    FILE* file = pers_fopen("rb");
    if (file) {
        if (fread(&pers_data, PERSISTENT_SIZE, 1, file) == 1) {
            flog("Persistence file read");
        }
        fclose(file);
    } else {
        flog("Persistence file read err: %d", errno);
        pers_data = s_persistentData;
    }
#endif
}

void pers_save()
{
#if defined(HAS_EEPROM)
    rom_write(ROM_ADDR, (BYTE*)&pers_data, PERSISTENT_SIZE);
#elif defined(_CONF_RASPBIAN)
    FILE* file = pers_fopen("wb");
    if (file) {
        if (fwrite(&pers_data, PERSISTENT_SIZE, 1, file) == 1) {
            flog("Persistence file written");
        }
        fclose(file);
    } else {
        flog("Persistence file write err: %d", errno);
    }
#endif
}
