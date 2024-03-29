#include <pic-modbus/modbus.h>
#include "./samples.h"

void __interrupt() low_isr() {
    timers_isr();
}

static void enableInterrupts() {
    // Disable low/high interrupt mode
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;
}

void main() {
    // Analyze RESET reason
    sys_init();
    modbus_init();
    samples_init();
    enableInterrupts();
    // I'm alive
    while (1) {
        CLRWDT();
        modbus_poll();
        samples_poll();
    }
}

