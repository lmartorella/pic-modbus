#include "../hardware/fuses.h"
#include "../hardware/utilities.h"
#include "../hardware/cm1602.h"
#include <stdio.h>

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
    int i;

    reset();

    // Write #1/#2 on both rows
    _print("#1", 0);
    _print("#2", 0x40);

    // Wait 1 sec
    for (i = 0; i < 33; i++) wait30ms();

    // Now simulate a reset
    reset();
    
    // Write #A/#B on both rows
    _print("#A", 0);
    _print("#B", 0x40);

    for (i = 0; i < 33; i++) wait30ms();
    RESET();
}
