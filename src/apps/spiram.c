#include "../pch.h"
#include "spiram.h"
#include "../appio.h"

#ifdef HAS_SPI_RAM

#define MEM_CSTRIS0	MEM_TRISBITS.MEM_BANK0_CS
#define MEM_CSTRIS1	MEM_TRISBITS.MEM_BANK1_CS
#define MEM_CSTRIS2	MEM_TRISBITS.MEM_BANK2_CS
#define MEM_CSTRIS3	MEM_TRISBITS.MEM_BANK3_CS

enum STATUSREG
{
	ST_BYTEMODE = 0x0,
	ST_PAGEMODE = 0x80,
	ST_SEQMODE = 0x40,
	HOLD_EN = 0x0,
	HOLD_DIS = 0x1,
};

enum MSG
{
	MSG_READ = 0x3,		// Read data from memory array beginning at selected address
	MSG_WRITE = 0x2,	// Write data to memory array beginning at selected address
	MSG_RDSR = 0x5,		// Read Status register
	MSG_WRSR = 0x1,		// Write Status register
};

static void disableAll(void)
{	
    // Set all CS to 1 (disabled)
    MEM_PORT |= MEM_BANK_CS_MASK;
}

static void enableBank(BYTE b)
{
    // support 4 banks = 128k
    switch (b & 0x3)
    {
        case 0:
            MEM_CS0 = 0;
            break;
        case 1:
            MEM_CS1 = 0;
            break;
        case 2:
            MEM_CS2 = 0;
            break;
        case 3:
            MEM_CS3 = 0;
            break;
    }
}


// This will override the spi_init() call
void sram_init()
{
	char b;
	// Drive SPI RAM chip select pin
	MEM_CSTRIS0 = 0;
	MEM_CSTRIS1 = 0;
	MEM_CSTRIS2 = 0;
	MEM_CSTRIS3 = 0;

	disableAll();

    spi_lock();
	for (b = 0; b < 4; b++)
	{
		enableBank(b);
		// Enter full range mode
		spi_shift(MSG_WRSR);
		spi_shift(ST_SEQMODE | HOLD_DIS);
		disableAll();
	}
    spi_release();
    
    println("ChkRam");
    sram_test_gui(buffer, sizeof(buffer));
}

// Write a vector of bytes in RAM.
// NOTE: do not support SPI cross-bank access
//  - *dest is in banked PIC RAM
//  - address is logic SPIRAM address of the first byte to write
//  - count is the count of byes to write
void sram_write(const BYTE* src, UINT32 address, UINT16 count)
{
	UINT16 raddr = (UINT16)address;
        spi_lock();
        enableBank(address >> 15);
	spi_shift(MSG_WRITE);
	spi_shift16(raddr);
	while (count > 0)
	{
            // Write 1 byte
            spi_shift(*(src++));
            count--;
            ClrWdt();
	}
	disableAll();
        spi_release();
}

// Read a vector of bytes in RAM.
// NOTE: do not support SPI cross-bank access
//  - *dest is in banked PIC RAM
//  - address is logic SPIRAM address of the first byte to read
//  - count is the count of byes to read
void sram_read(BYTE* dest, UINT32 address, UINT16 count)
{
	UINT16 raddr = (UINT16)address;
        spi_lock();
        enableBank(address >> 15);
	spi_shift(MSG_READ);
	spi_shift16(raddr);
        while (count > 0)
	{
            // Read 1 byte
            *(dest++) = spi_shift(0);
            count--;
	}
	disableAll();
        ClrWdt();
        spi_release();
}

// Test all 4 banks, displays the ADDR of the failing test and hang if found one
// bs is the BYTE seed
#define HIGH_MEM 0x20000

// Use a non-256 multiple to test cross-boundary accesses
void sram_test_gui(BYTE* buffer, UINT16 bufferSize)
{
    // To use a pseudo-random seq string that spans on all banks
    // write first, read then

    // Write all
    clearln();
    BYTE b = 0;
    BYTE gui = 0;
    for (UINT32 addr = 0; addr < HIGH_MEM; addr += bufferSize)
    {
        UINT16 toWrite = bufferSize;
        if (addr + toWrite > HIGH_MEM)
        {
            toWrite = HIGH_MEM - addr;
        }
        for (int i = 0; i < toWrite; i++)
        {
            b += 251; // largest prime < 256
            buffer[i] = b;
        }
        sram_write(buffer, addr, toWrite);

        if (((++gui) % 32) == 0)
        {
            printch('.');
        }
    }

    // READ all
    clearln();
    b = 0;
    gui = 0;
    for (UINT32 addr = 0; addr < HIGH_MEM; addr += bufferSize)
    {
        UINT16 toRead = bufferSize;
        if (addr + toRead > HIGH_MEM)
        {
            toRead = HIGH_MEM - addr;
        }

        sram_read(buffer, addr, toRead);
        for (int i = 0; i < toRead; i++)
        {
            b += 251; // largest prime < 256
            if (buffer[i] != b)
            {
                char msg[16];
                sprintf(msg, "FAIL #%8lX", addr + i);
                printlnUp(msg);
                sprintf(msg, "EXP: %2X, RD: %2X", (int)b, (int)buffer[i]);
                println(msg);
                // HANG
                di();
                while (1) CLRWDT();
            }
        }

        if (((++gui) % 32) == 0)
        {
            printch('%');
        }
    }
}

#endif // HAS_SPI_RAM

