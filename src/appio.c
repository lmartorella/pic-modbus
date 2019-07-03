#include "pch.h"
#include "appio.h"

#ifdef HAS_CM1602
#include "../../samples/beans/hardware/cm1602.h"
#endif

const char* g_lastException;
RESET_REASON g_resetReason;

extern void wait1s();

#if defined(HAS_MAX232_SOFTWARE) && defined(DEBUGMODE)
#define HAS_DEBUG_LINE
#endif

#if defined(HAS_CM1602) || defined(HAS_DEBUG_LINE)
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

#ifdef HAS_DEBUG_LINE
static void _stdout(const char* str) {
    strcpy(max232_buffer1, str);
    max232_send(strlen(str));
}
#endif

void io_init()
{
#ifdef HAS_CM1602
    // reset display
    cm1602_reset();
    cm1602_clear();
    cm1602_setEntryMode(MODE_INCREMENT | MODE_SHIFTOFF);
    cm1602_enable(ENABLE_DISPLAY | ENABLE_CURSOR | ENABLE_CURSORBLINK);

    cm1602_setDdramAddr(0);
    cm1602_writeStr("Boot: ");
    cm1602_writeStr(g_resetReasonMsgs[g_resetReason]);
    if (g_resetReason == RESET_EXC)
    {
        cm1602_setDdramAddr(0x40);
        cm1602_writeStr(g_lastException);
    }

    wait1s();
#elif defined(HAS_DEBUG_LINE)
    _stdout("Boot: ");
    _stdout(g_resetReasonMsgs[g_resetReason]);
    if (g_resetReason == RESET_EXC)
    {
        _stdout(g_lastException);
    }
#endif

#ifdef HAS_LED
    led_init();
    led_off();
#endif
}

#ifdef HAS_CM1602
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
#endif

#ifdef HAS_CM1602
static void _print(const char* str, BYTE addr)
{
	_clr(addr);
	cm1602_writeStr(str);
	CLRWDT();
}
#endif

void io_println(const char* str)
{
#ifdef HAS_CM1602
	_print(str, 0x40);	
#elif defined(HAS_DEBUG_LINE)
    _stdout(str);
#elif defined(__GNU)
    printf("%s\r\n", str);
    fflush(stdout);
#endif
}

void io_io_printlnStatus(const char* str)
{
#ifdef HAS_CM1602
	_print(str, 0x00);	
#elif defined(HAS_DEBUG_LINE)
    _stdout(str);
#elif defined(__GNU)
    printf("%s\r\n", str);
    fflush(stdout);
#endif
}

void io_printChDbg(char ch)
{
#ifdef HAS_CM1602
    cm1602_write(ch);
#elif defined(HAS_DEBUG_LINE)
    max232_buffer1[0] = ch;
    max232_send(1);
#elif defined(__GNU)
    printf("%c", ch);
    fflush(stdout);
#endif
}

#ifdef __GNU
void flog(const char* format, ...) {
    va_list args;
    va_start(args, format);

    vprintf(format, args);
    printf("\n");
    fflush(stdout);
}
#endif
