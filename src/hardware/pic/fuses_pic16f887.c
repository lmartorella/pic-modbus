#include "net/net.h"
#include "appio.h"
   
// Configuration in garden source code
    
void sys_enableInterrupts() {
    // Disable low/high interrupt mode
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;
}

// Get a copy version of STATUS
extern __bank0 unsigned char __resetbits;
#define nTObit 0x10

// The pointer is pointing to ROM space that will not be reset
// otherwise after the RESET the variable content can be lost.
__persistent EXC_STRING_T g_exceptionPtr;

void sys_storeResetReason()
{
    // See datasheet table 14-5
    if (PCONbits.nPOR) {
        if (!PCONbits.nBOR) {
            g_resetReason = RESET_BROWNOUT;
            PCONbits.nBOR = 1;
        }
        else {
            if (!(__resetbits & nTObit)) {
                // Watchdog is used for RESET() on pic16!
                if (g_exceptionPtr != 0) { 
                    g_resetReason = RESET_EXC;
                    g_lastException = g_exceptionPtr;
                }
                else {
                    g_resetReason = RESET_WATCHDOG;
                }
            }
            else {
                g_resetReason = RESET_MCLR;
            }
        }
    }
    else {
        g_resetReason = RESET_POWER;        
        PCONbits.nPOR = 1;
        PCONbits.nBOR = 1;
    }
    g_exceptionPtr = 0;
}

