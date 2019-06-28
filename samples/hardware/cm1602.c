#include "../../../src/nodes/pch.h"
#include "cm1602.h"

#ifdef HAS_CM1602

#define CMD_CLEAR 		0x1
#define CMD_HOME 		0x2
#define CMD_ENTRY 		0x4
#define CMD_ENABLE 		0x8
#define CMD_SHIFT 		0x10
#define CMD_FUNCSET		0x20
#define CMD_SETCGRAM	0x40
#define CMD_SETDDRAM	0x80

#define CMD_FUNCSET_DL_8	0x10
#define CMD_FUNCSET_DL_4	0x00
#define CMD_FUNCSET_LN_2	0x08
#define CMD_FUNCSET_LN_1	0x00
#define CMD_FUNCSET_FS_11	0x04
#define CMD_FUNCSET_FS_7	0x00

#ifndef CM1602_PORT
#error CM1602_PORT not set
#endif
#ifndef CM1602_IF_BIT_RW
#error CM1602_IF_BIT_RW not set
#endif
#ifndef CM1602_IF_BIT_EN
#error CM1602_IF_BIT_EN not set
#endif
#ifndef CM1602_IF_BIT_RS
#error CM1602_IF_BIT_RS not set
#endif

typedef union {
    struct  {
        unsigned R0 :1;
        unsigned R1 :1;
        unsigned R2 :1;
        unsigned R3 :1;
        unsigned R4 :1;
        unsigned R5 :1;
        unsigned R6 :1;
        unsigned R7 :1;
    };
    BYTE v;
} PORTXBITS_t;

extern volatile PORTXBITS_t CM1602_PORTBITS @ CM1602_PORTADDR;

// Clock the control bits in order to push the 4/8 bits to the display.
// In case of 4-bit, the lower data is sent and the HIGH part should be ZERO
static void pulsePort(BYTE v)
{
    PORTXBITS_t data;
    data.v = v;
    
    CM1602_IF_BIT_EN = 1;

#if CM1602_IF_MODE == 4
    // wait for eventual bits in port to stabilize (BIT_EN could be on the same port)
    NOP();

    #if CM1602_IF_NIBBLE == CM1602_IF_NIBBLE_LOW
        CM1602_PORTBITS.R0 = data.R0;
        CM1602_PORTBITS.R1 = data.R1;
        CM1602_PORTBITS.R2 = data.R2;
        CM1602_PORTBITS.R3 = data.R3;
    #elif CM1602_IF_NIBBLE == CM1602_IF_NIBBLE_HIGH
        CM1602_PORTBITS.R4 = data.R0;
        CM1602_PORTBITS.R5 = data.R1;
        CM1602_PORTBITS.R6 = data.R2;
        CM1602_PORTBITS.R7 = data.R3;
    #else
            #error CM1602_IF_NIBBLE should be set to CM1602_IF_NIBBLE_LOW or CM1602_IF_NIBBLE_HIGH
    #endif
#else
    CM1602_PORT = v;
#endif
    CM1602_IF_BIT_EN = 0;
}

// write a byte to the port
static void writeByte(BYTE data)
{
#if CM1602_IF_MODE == 4
	// 4-bit interface. Send high first
	pulsePort(data >> 4);
	pulsePort(data & 0xf);
#elif CM1602_IF_MODE == 8
	// 8-bit interface. Send all
	pulsePort(data);
#else
	#error CM1602_IF_MODE should be set to 4 or 8
#endif
}

static void writeCmd(BYTE data)
{
	CM1602_IF_BIT_RS = 0;
	writeByte(data);
}

static void writeData(BYTE data)
{
	CM1602_IF_BIT_RS = 1;
	writeByte(data);
}

// From http://elm-chan.org/docs/lcd/hd44780_e.html
/*
 The HD44780 does not have an external reset signal. It has an 
 * integrated power-on reset circuit and can be initialized to the 
 * 8-bit mode by proper power-on condition. However the reset circuit 
 * can not work properly if the supply voltage rises too slowly or fast. 
 * Therefore the state of the host interface can be an unknown state, 
 * 8-bit mode, 4-bit mode or half of 4-bit cycle at program started. 
 * To initialize the HD44780 correctly even if it is unknown state, 
 * the software reset procedure shown in Figure 4 is recommended 
 * prior to initialize the HD44780 to the desired function.*/

void cm1602_reset(void)
{
    // Enable all PORTE as output (display)
    // During tIOL, the I/O ports of the interface (control and data signals) should be kept at ?Low?. see ST7066U datasheet
#if CM1602_IF_MODE == 4
    CM1602_IF_TRIS_RW = 0;
    CM1602_IF_TRIS_RS = 0;
    CM1602_IF_TRIS_EN = 0;
    #if CM1602_IF_NIBBLE == CM1602_IF_NIBBLE_LOW
        CM1602_TRIS &= 0xf0;
        NOP();
        CM1602_PORT &= 0xF0;
    #else
        CM1602_TRIS &= 0x0F;
        NOP();
        CM1602_PORT &= 0x0F;
    #endif
    CM1602_IF_BIT_RW = 0;
    CM1602_IF_BIT_RS = 0;
    CM1602_IF_BIT_EN = 0;
#else
    CM1602_TRIS = 0;
    NOP();
    CM1602_PORT = 0x0;
#endif

    // max 100ms @5v , see ST7066U datasheet
    wait30ms();
    wait30ms();
    wait30ms();

    // Push fake go-to-8bit state 3 times
#if CM1602_IF_MODE == 4
    BYTE cmd = (CMD_FUNCSET | CMD_FUNCSET_DL_8) >> 4;
#else
    BYTE cmd = CMD_FUNCSET | CMD_FUNCSET_DL_8;
#endif
    pulsePort(cmd); 
    wait30ms();
    pulsePort(cmd);
    wait100us();
    pulsePort(cmd);

    // Now the device is proper reset, and the mode is 8-bit

    cmd = CMD_FUNCSET |
#if CM1602_LINE_COUNT == 1
            CMD_FUNCSET_LN_1
#elif CM1602_LINE_COUNT == 2
            CMD_FUNCSET_LN_2
#else
#error CM1602_LINE_COUNT should be set to 1 or 2
#endif
    |
#if CM1602_IF_MODE == 8
            CMD_FUNCSET_DL_8
#else
            CMD_FUNCSET_DL_4
#endif
    |
#if CM1602_FONT_HEIGHT == 7
            CMD_FUNCSET_FS_7;
#elif CM1602_FONT_HEIGHT == 10
            CMD_FUNCSET_FS_11;
#else
#error CM1602_FONT_HEIGHT should be set to 7 or 10
#endif

    wait100us();

#if CM1602_IF_MODE == 4
    // Translating to 8-bit (default at power-up) to 4-bit
    // requires a dummy 8-bit command to be given that contains
    // the MSB of the go-to-4bit command
    // After a soft reset, this is not needed again, hence the persistent flag
    pulsePort(cmd >> 4);		// Enables the 4-bit mode
#endif
    writeCmd(cmd);
    wait100us();
}

void cm1602_clear(void)
{
	writeCmd(CMD_CLEAR);
	wait2ms();
}

void cm1602_home(void)
{
	writeCmd(CMD_HOME);
	wait2ms();
}

void cm1602_setEntryMode(enum CM1602_ENTRYMODE mode)
{
	writeCmd(CMD_ENTRY | mode);
	wait40us();
}

void cm1602_enable(enum CM1602_ENABLE enable)
{
	writeCmd(CMD_ENABLE | enable);
	wait40us();
}

void cm1602_shift(enum CM1602_SHIFT data)
{
	writeCmd(CMD_SHIFT | data);
	wait40us();
}

void cm1602_setCgramAddr(BYTE address)
{
	writeCmd(CMD_SETCGRAM | address);
	wait40us();
}

void cm1602_setDdramAddr(BYTE address)
{
	writeCmd(CMD_SETDDRAM | address);
	wait40us();
}

void cm1602_write(BYTE data)
{
	writeData(data);
	wait40us();
}

void cm1602_writeStr(const char* data)
{
	while (*data != 0)
	{
		cm1602_write(*data);
		data++;
	}
}

#endif //#ifdef HAS_CM1602

