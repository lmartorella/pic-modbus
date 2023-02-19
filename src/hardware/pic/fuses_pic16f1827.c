#include "net/net.h"

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = ON        // Watchdog Timer Enable (WDT enabled)
#pragma config PWRTE = ON       // Power-up Timer Enable (PWRT enabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Memory Code Protection (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = ON        // Internal/External Switchover (Internal/External Switchover mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = ON       // PLL Enable (4x PLL enabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)
 
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

void sys_storeResetReason() {
    // Set 16MHz oscillator
    // SCS = 10b: Internal oscillator block
    OSCCONbits.SCS = 2;
    // IRCF = 1111b = 16 MHz 
    OSCCONbits.IRCF = 0xf;
    // PLL disabled
    OSCCONbits.SPLLEN = 0;

    // See datasheet table 7.10
    if (!PCONbits.nPOR) {
        g_resetReason = RESET_POWER;        
    } else if (!PCONbits.nBOR) {
        g_resetReason = RESET_BROWNOUT;
    } else if (!(__resetbits & nTObit)) {
        g_resetReason = RESET_WATCHDOG;
    } else if (!PCONbits.nRMCLR) {
        g_resetReason = RESET_MCLR;
    } else if (!PCONbits.nRI) {
        g_resetReason = RESET_EXC;
        g_lastException = g_exceptionPtr;
    } else if (PCONbits.STKOVF || PCONbits.STKUNF) {
        g_resetReason = RESET_STACKFAIL;
    }

    PCON = 0xf; // reset all reset reasons
    g_exceptionPtr = 0;
}

