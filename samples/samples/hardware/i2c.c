#include <pic-modbus/net.h>
#include "i2c.h"

#ifdef HAS_I2C

#define I2C_PORT_SDA PORTBbits.RB1
#define I2C_TRIS_SDA TRISBbits.TRISB1
#define I2C_PORT_SCL PORTBbits.RB4
#define I2C_TRIS_SCL TRISBbits.TRISB4
#define I2C_SSPADD SSP1ADD
#define I2C_SSPBUF SSP1BUF
#define I2C_SSPCON2 SSP1CON2
#define I2C_PIR_SSP1IF PIR1bits.SSP1IF
#define I2C_SSPSTAT_SMP SSP1STATbits.SMP
#define I2C_SSPSTAT_CKE SSP1STATbits.CKE
#define I2C_SSPSTAT_BF SSP1STATbits.BF
#define I2C_SSPCON1_WCOL SSP1CON1bits.WCOL
#define I2C_SSPCON1_SSPOV SSP1CON1bits.SSPOV
#define I2C_SSPCON1_SSPM SSP1CON1bits.SSPM
#define I2C_SSPCON1_SSPEN SSP1CON1bits.SSPEN
#define I2C_SSPCON2_SEN SSP1CON2bits.SEN
#define I2C_SSPCON2_ACKSTAT SSP1CON2bits.ACKSTAT
#define I2C_SSPCON2_RCEN SSP1CON2bits.RCEN
#define I2C_SSPCON2_PEN SSP1CON2bits.PEN
#define I2C_SSPCON2_ACKEN SSP1CON2bits.ACKEN
#define I2C_SSPCON2_ACKDT SSP1CON2bits.ACKDT
#define I2C_SSPCON2_BUSY_MASK (_SSPCON2_SEN_MASK | _SSPCON2_RSEN_MASK | _SSPCON2_PEN_MASK | _SSPCON2_RCEN_MASK | _SSPCON2_ACKEN_MASK)

// I2C status bytes
static uint8_t s_addr;
static uint8_t* s_dest;
static uint8_t* s_buf;
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
    
    // Disable analog functions on i2c pins
    ANSELBbits.ANSB1 = 0;
    ANSELBbits.ANSB4 = 0;

    __delay_ms(2);

    // Start bit
    I2C_PORT_SDA = 0; // SDA low
    __delay_us(40);
    I2C_PORT_SCL = 0; // SCL low
    __delay_us(40);
     
    I2C_PORT_SDA = 1; // SDA = 1 
    // Produce 8 bits + 1 NACK
    for (uint8_t i = 0; i < 8; i++) {
        __delay_us(40);
        I2C_PORT_SCL = 1;
        __delay_us(40);
        I2C_PORT_SCL = 0;
    }
    __delay_us(40);

    // Stop bit
    I2C_PORT_SDA = 0; // SDA low
    __delay_us(40);
    I2C_PORT_SCL = 1; // SCL high
    __delay_us(40);
    I2C_PORT_SDA = 1; // SDA high
    __delay_ms(2);
    
    // Ports as inputs
    I2C_TRIS_SDA = 1;
    I2C_TRIS_SCL = 1;
    
    // Baud generator
    I2C_SSPADD = 62; // FOsc = 25Mkz -> 100kHz
    
    // Setup I2C
    I2C_SSPSTAT_SMP = 0; // Slew rate control enabled for high speed mode (400 kHz)
    I2C_SSPSTAT_CKE = 0; // Disable SM bus? specific inputs
    I2C_SSPCON1_SSPM = 8; // I2C Master mode, clock = FOSC / (4 * (I2C_SSPADD + 1))
    I2C_SSPCON1_SSPEN = 1;  // Enables the serial port and configures the SDAx and SCLx pins as the source of the serial port pins

    __delay_ms(2);
    
    s_istate = STATE_IDLE;
}

void i2c_sendReceive7(uint8_t addr, uint8_t size, uint8_t* buf) {
    // Check if MSSP module is in use
    if ((I2C_SSPCON2 & I2C_SSPCON2_BUSY_MASK) || s_istate != STATE_IDLE) {
        sys_fatal(ERR_DEVICE_HW_FAIL);
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

__bit i2c_poll() {
loop:
    if (s_istate == STATE_IDLE) {
        return true; 
    }

    // Something happened?
    if (!I2C_PIR_SSP1IF) {
        return false;
    }
    
    I2C_PIR_SSP1IF = 0;
    // Write collision
    if (I2C_SSPCON1_WCOL) {
        sys_fatal(ERR_DEVICE_HW_FAIL);
    }
    if (I2C_SSPCON1_SSPOV) {
        sys_fatal(ERR_DEVICE_READ_OVERRUN);
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
                sys_fatal(ERR_DEVICE_HW_FAIL);
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
                sys_fatal(ERR_DEVICE_READ_OVERRUN);
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
                sys_fatal(ERR_DEVICE_HW_NOT_ACK);
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
