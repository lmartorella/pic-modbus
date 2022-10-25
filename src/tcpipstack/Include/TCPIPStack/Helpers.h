/*********************************************************************
 Header file for Helpers.c
 
 FileName:      Helpers.h
 Dependencies:  See INCLUDES section
 Processor:     PIC18, PIC24, dsPIC, PIC32
 Compiler:      Microchip C18, C30, C32
 Company:       Microchip Technology, Inc.

 Software License Agreement

 Copyright (C) 2002-2011 Microchip Technology Inc.  All rights
 reserved.

 Microchip licenses to you the right to use, modify, copy, and
 distribute:
 (i)  the Software when embedded on a Microchip microcontroller or
      digital signal controller product ("Device") which is
      integrated into Licensee's product; or
 (ii) ONLY the Software driver source files ENC28J60.c, ENC28J60.h,
		ENCX24J600.c and ENCX24J600.h ported to a non-Microchip device
		used in conjunction with a Microchip ethernet controller for
		the sole purpose of interfacing with the ethernet controller.

 You should refer to the license agreement accompanying this
 Software for additional information regarding your rights and
 obligations.

 THE SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT
 WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 LIMITATION, ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 MICROCHIP BE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR
 CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF
 PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS
 BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE
 THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER
 SIMILAR COSTS, WHETHER ASSERTED ON THE BASIS OF CONTRACT, TORT
 (INCLUDING NEGLIGENCE), BREACH OF WARRANTY, OR OTHERWISE.

 ********************************************************************
 File Description:
 
 Change History:
 
  Rev         Description
  ----------  -------------------------------------------------------
  1.0 - 5.31  Initial release
  5.36        Updated compile time check for ultoa();
 ********************************************************************/
#ifndef __HELPERS_H
#define __HELPERS_H


#if !defined(__18CXX) || defined(HI_TECH_C)
	char *strupr(char* s);
#endif

// Implement consistent ultoa() function
#if (defined(__PIC32MX__) && (__C32_VERSION__ < 112)) || (defined (__C30__) && (__C30_VERSION__ < 325)) || defined(__C30_LEGACY_LIBC__) || defined(__C32_LEGACY_LIBC__)
	// C32 < 1.12 and C30 < v3.25 need this 2 parameter stack implemented function
	void ultoa(uint32_t Value, uint8_t* Buffer);
#elif defined(__18CXX) && !defined(HI_TECH_C)
	// C18 already has a 2 parameter ultoa() function
	#include <stdlib.h>
#else
	// HI-TECH PICC-18 PRO 9.63, C30 v3.25+, and C32 v1.12+ already have a ultoa() stdlib 
	// library function, but it requires 3 parameters.  The TCP/IP Stack 
	// assumes the C18 style 2 parameter ultoa() function, so we shall 
	// create a macro to automatically convert the code.
	#include <stdlib.h>
	#define ultoa(val,buf)	ultoa((char*)(buf),(val),10)
#endif

#if defined(DEBUG)
	#define DebugPrint(a)	{putrsUART(a);}
#else
	#define DebugPrint(a)
#endif

uint32_t	LFSRSeedRand(uint32_t dwSeed);
uint16_t	LFSRRand(void);
uint32_t	GenerateRandomDWORD(void);
void 	uitoa(uint16_t Value, uint8_t* Buffer);
void 	UnencodeURL(uint8_t* URL);
uint16_t 	Base64Decode(uint8_t* cSourceData, uint16_t wSourceLen, uint8_t* cDestData, uint16_t wDestLen);
uint16_t	Base64Encode(uint8_t* cSourceData, uint16_t wSourceLen, uint8_t* cDestData, uint16_t wDestLen);
_Bool	StringToIPAddress(uint8_t* str, IP_ADDR* IPAddress);
uint8_t 	ReadStringUART(uint8_t* Dest, uint8_t BufferLen);
uint8_t	hexatob(WORD_VAL AsciiChars);
uint8_t	btohexa_high(uint8_t b);
uint8_t	btohexa_low(uint8_t b);
signed char stricmppgm2ram(uint8_t* a, ROM uint8_t* b);
char * 	strnchr(const char *searchString, size_t count, char c);
size_t  strncpy_m(char* destStr, size_t destSize, int nStrings, ...);

#if defined(__18CXX)
	_Bool	ROMStringToIPAddress(ROM uint8_t* str, IP_ADDR* IPAddress);
#else
	// Non-ROM variant for C30 and C32
	#define ROMStringToIPAddress(a,b)	StringToIPAddress((uint8_t*)a,b)
#endif


uint16_t    swaps(uint16_t v);

#if defined(__C32__)
uint32_t   __attribute__((nomips16)) swapl(uint32_t v);
#else
uint32_t   swapl(uint32_t v);
#endif

uint16_t    CalcIPChecksum(uint8_t* buffer, uint16_t len);


#if defined(__18CXX)
	uint32_t leftRotateDWORD(uint32_t val, uint8_t bits);
#else
	// Rotations are more efficient in C30 and C32
	#define leftRotateDWORD(x, n) (((x) << (n)) | ((x) >> (32-(n))))
#endif

void FormatNetBIOSName(uint8_t Name[16]);


// Protocols understood by the ExtractURLFields() function.  IMPORTANT: If you 
// need to reorder these (change their constant values), you must also reorder 
// the constant arrays in ExtractURLFields().
typedef enum
{
	PROTOCOL_HTTP = 0u,
	PROTOCOL_HTTPS,
	PROTOCOL_MMS,
	PROTOCOL_RTSP
} PROTOCOLS;

uint8_t ExtractURLFields(uint8_t *vURL, PROTOCOLS *protocol, uint8_t *vUsername, uint16_t *wUsernameLen, uint8_t *vPassword, uint16_t *wPasswordLen, uint8_t *vHostname, uint16_t *wHostnameLen, uint16_t *wPort, uint8_t *vFilePath, uint16_t *wFilePathLen);
int16_t Replace(uint8_t *vExpression, ROM uint8_t *vFind, ROM uint8_t *vReplacement, uint16_t wMaxLen, _Bool bSearchCaseInsensitive);

#endif
