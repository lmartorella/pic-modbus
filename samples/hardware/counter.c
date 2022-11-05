#include <net/net.h>
#include "./counter.h"

static DCNT_DATA s_data;
static __bit s_counterDirty;
static uint32_t s_lastCounter;

#define DCNT_IF INTCONbits.INTF
#define DCNT_IE INTCONbits.INTE

// Custom persistence data
typedef struct {
    // Used by counter
    uint32_t dcnt_counter;
} PERSISTENT_SINK_DATA;
static EEPROM_MODIFIER PERSISTENT_SINK_DATA s_persistentData /*__DATA_ADDRESS*/ = { 
    0
};

// Only save once every 255 seconds (4 minutes), a good balance between EEPROM data endurance and potential data 
// loss due to reset. Obviously no flow -> no write
static uint8_t s_persTimer;

void dcnt_interrupt() {
    if (DCNT_IF) {
        // Interrupt stack
        // This is access atomically since in interrupt
        s_data.counter++;
        s_counterDirty = 1;
        
        DCNT_IF = 0;
    }
    CLRWDT();
}

void dcnt_init() {
    pers_load(&s_data.counter);
    s_lastCounter = s_data.counter;
    s_data.flow = 0;
    s_counterDirty = 0;
    s_persTimer = 0;
    
    // Init interrupt on edge (RB0)
    DCNT_IF = 0;
    DCNT_IE = 1;
    // Edge not relevant
    
    CLRWDT();
}

// Called every seconds.
// Check when the volatile counter should be stored back in EEPROM
// to minimize write operations
void dcnt_poll() {
    if (s_counterDirty) {
        
        DCNT_IE = 0;
        uint32_t currCounter = s_data.counter;
        DCNT_IE = 1;
        
        s_data.flow = (uint16_t)(currCounter - s_lastCounter);
        s_lastCounter = currCounter;
        
        if ((++s_persTimer) == 0) {          
            pers_save(&s_lastCounter);
            s_counterDirty = 0;
        }
    } else {
        s_data.flow = 0;
    }
    CLRWDT();
}

void dcnt_getDataCopy(DCNT_DATA* data) {
    DCNT_IE = 0;
    *data = s_data;
    DCNT_IE = 1;
}
