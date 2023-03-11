#include <net/net.h>
#include "./samples.h"

void __interrupt(PRIO_TYPE) low_isr() {
    // Update tick timers at ~Khz freq
    timers_poll();
#ifdef BEAN_INTERRUPT_VECTOR
    dcnt_interrupt();
#endif
}

void main() {
    // Analyze RESET reason
    regs_init();

    net_init();
                
    sinks_init();
        
    enableInterrupts();

    // I'm alive
    while (1) {
        CLRWDT();
        net_poll();
        sinks_poll();
    }
}

