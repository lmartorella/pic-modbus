#include "../pch.h"
#include "spi.h"

#ifdef HAS_SPI

#define SPIRAM_SPI_IF (PIR1bits.SSP1IF)

//#pragma code spi_section

#define ClearSPIDoneFlag()  SPIRAM_SPI_IF = 0

static void WaitForDataByte(void)   
{
    while (!SPIRAM_SPI_IF);
    ClearSPIDoneFlag();
}

#define SPI_ON_BIT     (SPIRAM_SPICON1bits.SSPEN)

BYTE s_spiInUse;

// Init SPI as master
void spi_init(enum SPI_INIT value)
{
	// Enable I/O (not automatic)
	// (TCP/IP stack sources does it before enabling SPI)
	TRISCbits.RC3 = 0;		// Enable SCK1
	TRISCbits.RC4 = 1;		// SDI1 as input
	TRISCbits.RC5 = 0;		// Enable SDO1

	// Cycling SSPEN 1->0->1 will reset SPI
	SSP1CON1 = value & 0x1F;	// reset WCOL and SSPOV and SSPEN
	Nop();
	SSP1CON1bits.SSPEN = 1;

	ClearSPIDoneFlag();

	SSP1STAT = value;		// only get 7-6 bits, other are not writable
        s_spiInUse = 0;
}

// Send/read a byte MSB
BYTE spi_shift(BYTE data)
{
        spi_lock();
	// now write data to send
	SSP1BUF = data;
	// now wait until BF is set (data flushed and received)
	WaitForDataByte();
	BYTE ret = SSP1BUF;
        spi_release();
        return ret;
}

// Send/read a word MSB
UINT16 spi_shift16(UINT16 data)
{
    spi_lock();
    BYTE bm = spi_shift(data >> 8);
    BYTE bl = spi_shift((BYTE)data);
    spi_release();
    return (bm << 8) + bl;
}

// Send an array and ignore in data
void spi_shift_array_8(const BYTE* buffer, BYTE size)
{
    spi_lock();
    while (size > 0)
    {
        spi_shift(*(buffer++));
        size--;
    }
    spi_release();
}

// Send a repeated byte
void spi_shift_repeat_8(BYTE data, BYTE size)
{
    spi_lock();
    while (size > 0)
    {
        spi_shift(data);
        size--;
    }
    spi_release();
}

#endif //#ifdef HAS_SPI
