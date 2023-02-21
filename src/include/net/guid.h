#ifndef _VER_APP_H
#define _VER_APP_H

/**
 * Classic GUID definition
 */
typedef struct 
{
	uint32_t data1;
	uint16_t data2;
	uint16_t data3;
	uint32_t data4a;
	uint32_t data4b;
} GUID;

/**
 * Four-cc is used by sink type IDs
 */
typedef union {
    char str[4];
    struct {
        char c1;
        char c2;
        char c3;
        char c4;
    } chars;
    uint8_t bytes[4];
    uint32_t dword;
} FOURCC;

#endif
