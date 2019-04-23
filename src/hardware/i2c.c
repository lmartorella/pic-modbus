#include "../pch.h"
#include "i2c.h"

#ifdef HAS_I2C

// I2C status bytes
static BYTE s_addr;
static BYTE* s_dest;
static BYTE* s_buf;
typedef enum {
    DIR_SEND = 0,
    DIR_RECEIVE = 1,
} I2C_DIRECTION;

// I2C state machine
static enum {
    STATE_IDLE,
    STATE_START,
    STATE_ADDR,
    STATE_TXDATA,
    STATE_RXDATA,
    STATE_ACK,
    STATE_STOP
} s_istate;

void i2c_init() {
    // First remove any deadlock on receivers.
    // To avoid I2C module to be stuck, send out 9 '1' bits manually
    // See AN1028, Software Reset Sequence
    
    // Ports as outputs
    I2C_PORT_SDA = 1;
    I2C_TRIS_SDA = 0;
    I2C_PORT_SCL = 1;
    I2C_TRIS_SCL = 0;

    wait2ms();

    // Start bit
    I2C_PORT_SDA = 0; // SDA low
    wait40us();
    I2C_PORT_SCL = 0; // SCL low
    wait40us();
     
    I2C_PORT_SDA = 1; // SDA = 1 
    // Produce 8 bits + 1 NACK
    for (BYTE i = 0; i < 8; i++) {
        wait40us();
        I2C_PORT_SCL = 1;
        wait40us();
        I2C_PORT_SCL = 0;
    }
    wait40us();

    // Stop bit
    I2C_PORT_SDA = 0; // SDA low
    wait40us();
    I2C_PORT_SCL = 1; // SCL high
    wait40us();
    I2C_PORT_SDA = 1; // SDA high
    wait2ms();
    
    // Ports as inputs
    I2C_TRIS_SDA = 1;
    I2C_TRIS_SCL = 1;
    
    // Baud generator
    I2C_SSPADD = 62; // FOsc = 25Mkz -> 100kHz
    
    // Setup I2C
    I2C_SSPSTAT_SMP = 0;
    I2C_SSPSTAT_CKE = 0;
    I2C_SSPCON1_SSPM = 8; // I2C master
    I2C_SSPCON1_SSPEN = 1;  

    wait2ms();
    
    s_istate = STATE_IDLE;
}

void i2c_sendReceive7(BYTE addr, BYTE size, BYTE* buf) {
    // Check if MSSP module is in use
    if ((I2C_SSPCON2 & I2C_SSPCON2_BUSY_MASK) || s_istate != STATE_IDLE) {
        fatal("I2.U");
    }
    I2C_PIR_SSP1IF = 0;

    // Store regs
    s_addr = addr;
    s_buf = buf;
    s_dest = buf + size;
    
    // Start bit!
    // Start
    I2C_SSPCON2_SEN = 1; 
    s_istate = STATE_START;
}

bit i2c_poll() {
loop:
    if (s_istate == STATE_IDLE) {
        return TRUE; 
    }

    // Something happened?
    if (!I2C_PIR_SSP1IF) {
        return FALSE;
    }
    
    I2C_PIR_SSP1IF = 0;
    if (I2C_SSPCON1_WCOL) {
        fatal("I2.CL");
    }
    if (I2C_SSPCON1_SSPOV) {
        fatal("I2.OV");
    }
    
    switch (s_istate) {
        case STATE_START:
            // Send address
            I2C_SSPBUF = s_addr;
            s_istate = STATE_ADDR;
            break;
        case STATE_ADDR:
            if (I2C_SSPCON2_ACKSTAT) {
                // ACK not received. Err.
                fatal("I2.AA");
            }
            
            // Start send/receive
            if ((s_addr & 0x1) == DIR_RECEIVE) {
                I2C_SSPCON2_RCEN = 1;
                s_istate = STATE_RXDATA;
            } else {
                I2C_SSPBUF = *s_buf;
                s_buf++;
                s_istate = STATE_TXDATA;
            }
            break;
        case STATE_RXDATA:
            if (!I2C_SSPSTAT_BF) {
                fatal("I.BF");
            }
            *s_buf = I2C_SSPBUF;
            s_buf++;
            // Again?
            if (s_buf >= s_dest) {
                // Finish: send NACK
                I2C_SSPCON2_ACKDT = 1;
            } else {
                // Again: send ACK
                I2C_SSPCON2_ACKDT = 0;
            }
            I2C_SSPCON2_ACKEN = 1;
            s_istate = STATE_ACK;
            break;
        case STATE_TXDATA:
            if (I2C_SSPCON2_ACKSTAT) {
                // ACK not received? Err. (even the last byte, see BPM180 specs)
                fatal("I2.AI");
            }
            if (s_buf >= s_dest) {
                // Send STOP
                I2C_SSPCON2_PEN = 1;
                s_istate = STATE_STOP;
            } else {
                // TX again
                I2C_SSPBUF = *s_buf;
                s_buf++;
            }
            break;
        case STATE_ACK:
            if (s_buf >= s_dest) {
                // Send STOP
                I2C_SSPCON2_PEN = 1;
                s_istate = STATE_STOP;
            } else {
                I2C_SSPCON2_RCEN = 1;
                s_istate = STATE_RXDATA;
            }
            break;
        case STATE_STOP:
            s_istate = STATE_IDLE;
            break;
    }
    // It is possible that the IF flag is ready right now
    goto loop;
}
        
#endif
