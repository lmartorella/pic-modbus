#include "../pch.h"
#include "../appio.h"

#ifdef HAS_DCF77 

static bit s_lastState;
static TICK_TYPE s_lastTick;
static TICK_TYPE s_lastValidBit;

static char s_pos;
// Process one byte (2 BCD digit + parity) at a time
static BYTE s_acc;
static bit s_err;

static BYTE s_minutesL, s_minutesH;
static BYTE s_hoursL, s_hoursH;

// From DA6180B datasheet
#define ZERO_MIN (TICK_TYPE)(TICKS_PER_SECOND * 0.04)
#define ZERO_MAX (TICK_TYPE)(TICKS_PER_SECOND * 0.13)
#define ONE_MIN (TICK_TYPE)(TICKS_PER_SECOND * 0.14)
#define ONE_MAX (TICK_TYPE)(TICKS_PER_SECOND * 0.25)

#define SPACE_MIN (TICK_TYPE)(TICKS_PER_SECOND * 0.8)
#define SPACE_MAX (TICK_TYPE)(TICKS_PER_SECOND * 1.2)
#define SPACE2_MIN (TICK_TYPE)(TICKS_PER_SECOND * 1.8)
#define SPACE2_MAX (TICK_TYPE)(TICKS_PER_SECOND * 2.2)

#define FLASH_TIME (TICK_TYPE)(TICKS_PER_SECOND * 0.15)

static enum {
    BIT_TYPE_INVALID = '-',
    BIT_TYPE_ZERO = '0',
    BIT_TYPE_ONE = '1'
} s_lastMark;

static void reset() {
    s_pos = 0;
    s_err = 0;
    printlnUp("...");
}

static void err(const char* msg) {
    s_err = 1;
    s_pos = 0;
    printlnUp(msg);
}

static void printMin() {
    println("");
    printch('m');
    printch(':');
    printch(s_minutesH + '0');
    printch(s_minutesL + '0');
}

static void printHrs() {
    printch(' ');
    printch('h');
    printch(':');
    printch(s_hoursH + '0');
    printch(s_hoursL + '0');
}

static void addBit() {
    if (s_err) {
        return;
    }

    if (s_pos == 0) {
        // Start of packet, always 0
        if (s_lastMark != BIT_TYPE_ZERO) {
            err("NZ0");
        }
    } else if (s_pos == 20) {
        // Start of encoded time. Always 1.
        if (s_lastMark != BIT_TYPE_ONE) {
            err("NH1");
        } else {
            s_acc = 0;
        }
    } else if (s_pos >= 21 && s_pos <= 35) {
        // Accumulate
        s_acc >>= 1;
        if (s_lastMark == BIT_TYPE_ONE) {
            s_acc |= 0x80;
        }
    } 

    if (s_err) {
        return;
    }
        
    if (s_pos == 28 || s_pos == 35) {
        // Check even parity
        BYTE p = s_acc;
        p = p ^ (p >> 4 | p << 4);
        p = p ^ (p >> 2);
        p = p ^ (p >> 1);
        if ((p & 1) == 0) {
            if (s_pos == 28) {
                // Have minutes!
                s_minutesL = s_acc & 0x0f;
                s_minutesH = (s_acc & 0x70) >> 4;
                printMin();
            } else {
                // Have hours!
                s_acc >>= 1;
                s_hoursL = s_acc & 0x0f;
                s_hoursH = (s_acc & 0x30) >> 4;
                printMin();
                printHrs();
                
                // End of string
                err("OK");
            }
            s_acc = 0;
        } else {
            err("PAR");
        }
    }
    // Advance
    s_pos++;
}

void dcf77_init() {
    s_lastState = 0;    // At reset the receiver line is zero
    s_lastMark = BIT_TYPE_INVALID;
    DCF77_IN_TRIS = 1;  // input
    s_lastValidBit = TickGet();
    reset();
}

void dcf77_poll() {
    BOOL s = DCF77_IN_PORT;
    TICK_TYPE now = TickGet();
    TICK_TYPE len;
    
    if (s && !s_lastState) {
        s_lastState = 1;
        s_lastTick = TickGet();
    }    
    else if (!s && s_lastState) {
        s_lastState = 0;
        len = now - s_lastTick;
        s_lastMark = BIT_TYPE_INVALID;
        if (len < ZERO_MIN) {
            // Invalid
        }
        else if (len < ZERO_MAX) {
            s_lastMark = BIT_TYPE_ZERO;
        }
        else if (len < ONE_MIN) {
            // Invalid
        }
        else if (len < ONE_MAX) {
            s_lastMark = BIT_TYPE_ONE;
        }
       
        if (s_lastMark != BIT_TYPE_INVALID) {
            // Get time from last bit
            len = now - s_lastValidBit;
            // 1 second? Accumulate bit into the string
            if (len > SPACE2_MIN && len < SPACE2_MAX) {
                // First bit after space
                reset();
                addBit();
            }
            else if (len < SPACE_MIN || len > SPACE_MAX) {
                // Receive error
                err("TMG");
            } 
            else {
                // else exactly 1 second? Accumulate bit into the string
                addBit();
            }
            
            led_on();
            s_lastValidBit = now;
        }
    }    
    
    if (led_isOn() && now - s_lastValidBit > FLASH_TIME) {
        led_off();
    }
}

#endif