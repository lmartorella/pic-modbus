#include "../hardware/fuses.h"
#include "../hardware/utilities.h"
#include "../hardware/cm1602.h"
#include "../hardware/spi.h"
#include "../hardware/spiram.h"

static void _clr(BYTE addr)
{
	char i;
	cm1602_setDdramAddr(addr);
	for (i = 0; i < 16; i++)
	{
		cm1602_write(' ');
	}
	cm1602_setDdramAddr(addr);
}

static void _print(const char* str, BYTE addr)
{
	_clr(addr);
	cm1602_writeStr(str);
	ClrWdt();
}

static void reset()
{
    // reset display
    cm1602_reset();
    cm1602_clear();
    cm1602_setEntryMode(MODE_INCREMENT | MODE_SHIFTOFF);
    cm1602_enable(ENABLE_DISPLAY | ENABLE_CURSOR | ENABLE_CURSORBLINK);
}

void main()
{
    reset();

    // Write #1/#2 on both rows
    _print("Running test...", 0);

    // Enable SPI
    // from 23k256 datasheet and figure 20.3 of PIC datasheet
    // CKP = 0, CKE = 1
    // Output: data sampled at clock falling.
    // Input: data sampled at clock falling, at the end of the cycle.
    spi_init(SPI_SMP_END | SPI_CKE_IDLE | SPI_CKP_LOW | SPI_SSPM_CLK_F4);

    sram_init();

    BYTE buffer[32];
    while (1) sram_test_gui(buffer, sizeof(buffer));
}
