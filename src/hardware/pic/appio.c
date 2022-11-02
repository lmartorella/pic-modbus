#include "net/net.h"
#include "cm1602.h"

EXC_STRING_T g_lastException = EXC_STRING_NULL;
RESET_REASON g_resetReason;

#if defined(HAS_CM1602)
static const char* g_resetReasonMsgs[] = { 
                "N/A",
				"POR",
				"BOR",
				"CFG",
				"WDT",
				"STK",
				"RST",
				"EXC:"  };
#endif

void io_init() {
#ifdef HAS_CM1602
    // reset display
    cm1602_reset();
    cm1602_clear();
    cm1602_setEntryMode(MODE_INCREMENT | MODE_SHIFTOFF);
    cm1602_enable(ENABLE_DISPLAY | ENABLE_CURSOR | ENABLE_CURSORBLINK);

    cm1602_setDdramAddr(0);
    cm1602_writeStr("Boot: ");
    cm1602_writeStr(g_resetReasonMsgs[g_resetReason]);
    if (g_resetReason == RESET_EXC && g_lastException != 0)
    {
        cm1602_setDdramAddr(0x40);
        cm1602_writeStr((const char*)g_lastException);
    }

    __delaywdt_ms(1000);
#endif
}

#ifdef HAS_CM1602
static void _clr(uint8_t addr) {
	char i;
	cm1602_setDdramAddr(addr);
	for (i = 0; i < 16; i++)
	{
		cm1602_write(' ');
	}
	cm1602_setDdramAddr(addr);
}
#endif

#ifdef HAS_CM1602
static void _print(const char* str, uint8_t addr) {
	_clr(addr);
	cm1602_writeStr(str);
	CLRWDT();
}
#endif

void io_println(const char* str) {
#ifdef HAS_CM1602
	_print(str, 0x40);	
#endif
}

void io_printlnStatus(const char* str) {
#ifdef HAS_CM1602
	_print(str, 0x00);	
#endif
}

void io_printChDbg(char ch) {
#ifdef HAS_CM1602
    cm1602_write(ch);
#endif
}
