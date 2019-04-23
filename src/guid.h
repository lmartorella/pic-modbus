#ifndef _VER_APP_H
#define _VER_APP_H

typedef struct 
{
	DWORD data1;
	WORD data2;
	WORD data3;
	DWORD data4a;
	DWORD data4b;
} GUID;

typedef union {
    char str[2];
    struct {
        char c1;
        char c2;
    } chars;
    BYTE bytes[2];
} TWOCC;

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
