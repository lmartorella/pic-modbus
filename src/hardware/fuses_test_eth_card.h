#ifndef _FUSES_INCLUDE_
#define _FUSES_INCLUDE_

// Reset the device with fatal error
void fatal(const char* msg);

// ==== TEST ETH CARD wireframe

// PORTA: 0/5, Digital and analog. RA0/RA1 used by ethernet leds
// PORTB: 0/7, interrupt on change. RB6/7 used by ICSP. RB0/5 used by Mp3
// PORTC: 0/7, RC3/4 used by I2C. 1/2 and 6/7 used by EXTRAM I2C. Used by IO modules. 
// PORTD: 0/2
// PORTE: 0/5: Used by CM1602 module (0 and 2/7)
// PORTF: 0/7: digital and analog. Used by IO modules.
// PORTG: 4: 1/2 used by USART2 + 0 used by MAX485 enable logic

#define SYSTEM_CLOCK 25000000ull
#define PRIO_TYPE low_priority

#undef DEBUGMODE

#define HAS_BUS

// ******* 
// DISPLAY
// Uses PORTE, 0, 2-7
// ******* 
#define HAS_CM1602
#define CM1602_PORT 		PORTE
#define CM1602_TRIS 		TRISE
#define CM1602_PORTADDR         0xF84
#define CM1602_IF_MODE 		4
#define CM1602_IF_NIBBLE_LOW 	0
#define CM1602_IF_NIBBLE_HIGH 	1
#define CM1602_IF_NIBBLE 	CM1602_IF_NIBBLE_HIGH
#define CM1602_IF_TRIS_RW 	TRISEbits.RE2
#define CM1602_IF_TRIS_RS 	TRISEbits.RE0
#define CM1602_IF_TRIS_EN 	TRISEbits.RE3
#define CM1602_IF_BIT_RW 	PORTEbits.RE2
#define CM1602_IF_BIT_RS 	PORTEbits.RE0
#define CM1602_IF_BIT_EN 	PORTEbits.RE3
#define CM1602_LINE_COUNT 	2
#define CM1602_COL_COUNT 	16
#define CM1602_FONT_HEIGHT 	7

// ******* 
// MEMORY, uses PORTC 1, 2 6 and 7 + I2C
// ******* 
#define MEM_PORT	 PORTC
#define MEM_PORTBITS PORTCbits
#define MEM_TRISBITS TRISCbits
#define MEM_BANK0_CS RC6
#define MEM_BANK1_CS RC2
#define MEM_BANK2_CS RC7
#define MEM_BANK3_CS RC1
#define MEM_BANK_CS_MASK 0b11000110

// ******* 
// MP3, uses PORTB 0/5
// ******* 
#undef HAS_VS1011
#define VS1011_PORT  	PORTB
#define VS1011_PORTBITS PORTBbits
#define VS1011_TRISBITS TRISBbits
#define VS1011_RESET 	RB4
#define VS1011_DREQ  	RB5
#define VS1011_GPIO3 	RB2
#define VS1011_GPIO2  	RB3
#define VS1011_XDCS 	RB0
#define VS1011_XCS      RB1
#define VS1011_XTALI    25000           // in mhz
#define VS1011_CLK_DOUBLING    0


// ******* 
// MEM & LOADER & FLASH
// *******
#define MAX_PROG_MEM		0x20000
#define ROM_BLOCK_SIZE		64
#define CONFIGURATION_SIZE 	8

// ******
// SPI
// ******
#undef HAS_SPI

// ******
// SPI RAM
// ******
#undef HAS_SPI_RAM

// ******
// IP: uses PORTA0,1 (leds)
// ******
#define HAS_IP
#define SERVER_CONTROL_UDP_PORT 17008
#define CLIENT_TCP_PORT 20001

// ******
// IO: uses PORTC and PORTF full
// ******
#undef HAS_IO

// ******
// RS485: use USART2 on 18F87J60 (PORTG)
// ******
#define HAS_RS485
#define RS485_BUF_SIZE 64
#define RS485_RCSTA RCSTA2bits
#define RS485_TXSTA TXSTA2bits
#define RS485_TXREG TXREG2
#define RS485_RCREG RCREG2
#define RS485_PIR_TXIF PIR3bits.TX2IF
#define RS485_PIR_RCIF PIR3bits.RC2IF
#define RS485_PIE_TXIE PIE3bits.TX2IE
#define RS485_PIE_RCIE PIE3bits.RC2IE
#define RS485_TRIS_TX TRISGbits.RG1
#define RS485_TRIS_RX TRISGbits.RG2
#define RS485_TRIS_EN TRISGbits.RG0
#define RS485_PORT_EN PORTGbits.RG0
#define RS485_BAUD 19200
    // For 9600:
//#define RS485_INIT_BAUD() \
//    TXSTA2bits.BRGH = 1;\
//    BAUDCON2bits.BRG16 = 0;\
//    SPBRGH2 = 0;\
//    SPBRG2 = 162
#define RS485_INIT_BAUD() \
     TXSTA2bits.BRGH = 1;\
     BAUDCON2bits.BRG16 = 0;\
     SPBRGH2 = 0;\
     SPBRG2 = 80
#define RS485_INIT_INT() \
    IPR3bits.TX2IP = 0; \
    IPR3bits.RC2IP = 0
    
// *****
// Tick timer source
// *****
#define TICK_TMRH TMR0H
#define TICK_TMRL TMR0L
#define TICK_TCON T0CON
// Timer on, 16-bit, internal timer, 1:256 prescalar
// (TMR0ON | T0PS2 | T0PS1 | T0PS0)
#define TICK_TCON_DATA (0x87)
#define TICK_INTCON_IF INTCONbits.TMR0IF
#define TICK_INTCON_IE INTCONbits.TMR0IE
#define TICK_CLOCK_BASE (SYSTEM_CLOCK/4ull)
#define TICK_PRESCALER 256ull

#define TICK_TYPE DWORD

#ifdef HAS_IO
#ifdef HAS_SPI
#error Cannot use SPI and IO togheter
#elif defined(HAS_SPI_RAM)
#error Cannot use SPI RAM and IO togheter
#endif
#endif

#undef HAS_DCF77
#define DCF77_IN_TRIS TRISBbits.RB1
#define DCF77_IN_PORT PORTBbits.RB1
#ifdef HAS_DCF77
#ifdef HAS_VS1011
#error Cannot use DCF77 and VS1011 at the same time
#endif
#endif

#define HAS_LED
#define LED_PORTBIT PORTHbits.RH0
#define LED_TRISBIT TRISHbits.RH0

#undef HAS_FAKE_RS232

#undef HAS_I2C
#define I2C_PORT_SDA PORTCbits.RC4
#define I2C_TRIS_SDA TRISCbits.TRISC4
#define I2C_PORT_SCL PORTCbits.RC3
#define I2C_TRIS_SCL TRISCbits.TRISC3
#define I2C_SSPADD SSPADD
#define I2C_SSPBUF SSPBUF
#define I2C_SSPCON2 SSPCON2
#define I2C_PIR_SSP1IF PIR1bits.SSP1IF
#define I2C_SSPSTAT_SMP SSPSTATbits.SMP
#define I2C_SSPSTAT_CKE SSPSTATbits.CKE
#define I2C_SSPSTAT_BF SSPSTATbits.BF
#define I2C_SSPCON1_WCOL SSPCON1bits.WCOL
#define I2C_SSPCON1_SSPOV SSPCON1bits.SSPOV
#define I2C_SSPCON1_SSPM SSPCON1bits.SSPM
#define I2C_SSPCON1_SSPEN SSPCON1bits.SSPEN
#define I2C_SSPCON2_SEN SSPCON2bits.SEN
#define I2C_SSPCON2_ACKSTAT SSPCON2bits.ACKSTAT
#define I2C_SSPCON2_RCEN SSPCON2bits.RCEN
#define I2C_SSPCON2_PEN SSPCON2bits.PEN
#define I2C_SSPCON2_ACKEN SSPCON2bits.ACKEN
#define I2C_SSPCON2_ACKDT SSPCON2bits.ACKDT
#define I2C_SSPCON2_BUSY_MASK (_SSPCON2_SEN_MASK | _SSPCON2_RSEN_MASK | _SSPCON2_PEN_MASK | _SSPCON2_RCEN_MASK | _SSPCON2_ACKEN_MASK)

#undef HAS_BMP180
#undef HAS_BMP180_APP

#endif

