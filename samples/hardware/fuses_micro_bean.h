#ifndef FUSES_MICRO_BEAN_H
#define	FUSES_MICRO_BEAN_H

#define SYSTEM_CLOCK 4000000ul
#define _XTAL_FREQ SYSTEM_CLOCK
#define PRIO_TYPE

#define HAS_BUS

// *****
// Tick timer source. Uses TMR0 (8-bit prescales to 1:256), that resolve from 0.25ms to 16.7secs
// *****
#define TICK_TMR TMR0
#define TICK_TCON OPTION_REG

// Timer on, internal timer, 1:256 prescalar
// (!T0CS | !PSA , PS2:PS0)
#define TICK_TCON_1DATA (0x07)
#define TICK_TCON_0DATA (0x28)

#define TICK_INTCON_IF INTCONbits.T0IF
#define TICK_INTCON_IE INTCONbits.T0IE
#define TICK_CLOCK_BASE (SYSTEM_CLOCK / 4)
#define TICK_PRESCALER 256

#define TICK_TYPE WORD

#define HAS_LED
#define LED_PORTBIT PORTAbits.RA7
#define LED_TRISBIT TRISAbits.TRISA7

// ******
// RS485: use USART1 on 16F628 (PORTB)
// ******
#define HAS_RS485
#define RS485_BUF_SIZE 32
#define RS485_RCSTA RCSTAbits
#define RS485_TXSTA TXSTAbits
#define RS485_TXREG TXREG
#define RS485_RCREG RCREG
#define RS485_PIR_TXIF PIR1bits.TXIF
#define RS485_PIR_RCIF PIR1bits.RCIF
#define RS485_PIE_TXIE PIE1bits.TXIE
#define RS485_PIE_RCIE PIE1bits.RCIE
#define RS485_TRIS_TX TRISBbits.TRISB5
#define RS485_TRIS_RX TRISBbits.TRISB2
#define RS485_TRIS_EN TRISAbits.TRISA6
#define RS485_PORT_EN PORTAbits.RA6
#define RS485_BAUD 19200
    // For 9600:
//#define RS485_INIT_BAUD() \
//    TXSTAbits.BRGH = 1;\
//    SPBRG = 25
#define RS485_INIT_BAUD() \
     TXSTAbits.BRGH = 1;\
     BAUDCONbits.SCKP = 0;\
     BAUDCONbits.BRG16 = 0;\
     SPBRG = 12;\
     ANSELBbits.ANSB2 = 0;\
     ANSELBbits.ANSB5 = 0;\
     APFCON0bits.RXDTSEL = 1;\
     APFCON1bits.TXCKSEL = 1;\

//RXDTSEL:1  RX/DT function is on RB2
//TXCKSEL:1  TX/CK function is on RB5

#undef HAS_I2C
/*
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
*/

// Digital flow counter
#define HAS_DIGITAL_COUNTER
#define DCNT_IF INTCONbits.INTF
#define DCNT_IE INTCONbits.INTE

// Reset the device with fatal error
extern persistent BYTE g_exceptionPtr;
#define fatal(msg) { g_exceptionPtr = (BYTE)msg; RESET(); }

// Custom persistence data
#define HAS_PERSISTENT_SINK_DATA
typedef struct
{
    // Used by counter
    DWORD dcnt_counter;
} PERSISTENT_SINK_DATA;
#define PERSISTENT_SINK_DATA_DEFAULT_DATA { 0 }

#define HAS_INTERRUPT_VECTOR
#define INTERRUPT_VECTOR dcnt_interrupt();

#endif	/* FUSES_MICRO_BEAN_H */

