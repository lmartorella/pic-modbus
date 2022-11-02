#ifndef _CM1602_H
#define _CM1602_H

// Low-level interface, HW dependent
// CM1602_IF_MODE should be set to 4 or 8
// CM1602_IF_PORT should be set to the PORT to use
// CM1602_IF_BIT_RS should be set to the PORT bit for the RS signal
// CM1602_IF_BIT_RW should be set to the PORT bit for the RW signal
// CM1602_IF_BIT_EN should be set to the PORT bit for the EN signal
// CM1602_IF_NIBBLE: if CM1602_IF_MODE == 4, it should be set to 'low', 'high'
// CM1602_LINE_COUNT should be set to 1 or 2
// CM1602_FONT_HEIGHT should be set to 7 (5x7) or 10 (5x10)

void cm1602_reset(void);

// Low-level commands
enum CM1602_ENTRYMODE
{
	// RAM PTR increment after each read/write
	MODE_INCREMENT = 0x2,
	// RAM PTR decrement after each read/write
	MODE_DECREMENT = 0,
	// Display will not shift left/right after each read/write
	MODE_SHIFTOFF = 0,
	// Display will shift left/right after each read/write
	MODE_SHIFTON = 1,
};

enum CM1602_ENABLE
{
	ENABLE_DISPLAY = 0x4,
	ENABLE_CURSOR = 0x2,
	ENABLE_CURSORBLINK = 0x1
};

enum CM1602_SHIFT
{
	// Shift cursor
	SHIFT_CURSOR = 0,
	// Shift display
	SHIFT_DISPLAY = 0x8,
	// Shift to left
	SHIFT_LEFT = 0,
	// Shift to right
	SHIFT_RIGHT = 0x4
};

// Clear whole display
void cm1602_clear(void);
// Cursor go home
void cm1602_home(void);
// Set entry mode
void cm1602_setEntryMode(enum CM1602_ENTRYMODE mode);
void cm1602_enable(enum CM1602_ENABLE enable);
void cm1602_shift(enum CM1602_SHIFT data);
void cm1602_setCgramAddr(uint8_t address);
void cm1602_setDdramAddr(uint8_t address);
void cm1602_write(uint8_t data);
void cm1602_writeStr(const char* str);

// Read should be implemented ONLY on port tolerant to 5V 

#endif
