#include "../../../src/nodes/include/net.h"
#include "../../../src/nodes/include/persistence.h"
#include "counter.h"

#ifdef HAS_DIGITAL_COUNTER

static DCNT_DATA s_data;
static __bit s_counterDirty;
static uint32_t s_lastCounter;

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
    s_lastCounter = s_data.counter = pers_data.sinkData.dcnt_counter;
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
        
        s_data.flow = currCounter - s_lastCounter;
        s_lastCounter = currCounter;
        
        if ((++s_persTimer) == 0) {          
            pers_data.sinkData.dcnt_counter = s_lastCounter;
            pers_save();
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


#endif
