#ifndef _VER_APP_H
#define _VER_APP_H

/**
 * Classic GUID definition
 */
typedef struct 
{
	DWORD data1;
	WORD data2;
	WORD data3;
	DWORD data4a;
	DWORD data4b;
} GUID;

/**
 * Two-cc is used by protocol
 */
typedef union {
    char str[2];
    struct {
        char c1;
        char c2;
    } chars;
    BYTE bytes[2];
} TWOCC;

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
    BYTE bytes[4];
} FOURCC;

#endif
