/*********************************************************************
 *
 *                  UDP Module Defs for Microchip TCP/IP Stack
 *
 *********************************************************************
 * FileName:        UDP.h
 * Dependencies:    StackTsk.h
 *                  MAC.h
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F, PIC32
 * Compiler:        Microchip C32 v1.05 or higher
 *					Microchip C30 v3.12 or higher
 *					Microchip C18 v3.30 or higher
 *					HI-TECH PICC-18 PRO 9.63PL2 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * Copyright (C) 2002-2009 Microchip Technology Inc.  All rights
 * reserved.
 *
 * Microchip licenses to you the right to use, modify, copy, and
 * distribute:
 * (i)  the Software when embedded on a Microchip microcontroller or
 *      digital signal controller product ("Device") which is
 *      integrated into Licensee's product; or
 * (ii) ONLY the Software driver source files ENC28J60.c, ENC28J60.h,
 *		ENCX24J600.c and ENCX24J600.h ported to a non-Microchip device
 *		used in conjunction with a Microchip ethernet controller for
 *		the sole purpose of interfacing with the ethernet controller.
 *
 * You should refer to the license agreement accompanying this
 * Software for additional information regarding your rights and
 * obligations.
 *
 * THE SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION, ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * MICROCHIP BE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF
 * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS
 * BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE
 * THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER
 * SIMILAR COSTS, WHETHER ASSERTED ON THE BASIS OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE), BREACH OF WARRANTY, OR OTHERWISE.
 *
 *
 * Author               Date    Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Nilesh Rajbharti     3/19/01  Original        (Rev 1.0)
 ********************************************************************/
#ifndef __UDP_H
#define __UDP_H

// Stores a UDP Port Number
typedef uint16_t UDP_PORT;

// Provides a handle to a UDP Socket
typedef uint8_t UDP_SOCKET;

// UDP States 
typedef enum
{
	UDP_DNS_IS_RESOLVED,		// Special state for UDP client mode sockets
	UDP_DNS_RESOLVE,		// Special state for UDP client mode sockets
	UDP_GATEWAY_SEND_ARP,	// Special state for UDP client mode sockets
	UDP_GATEWAY_GET_ARP,		// Special state for UDP client mode sockets	
    UDP_CLOSED,				// Socket is idle and unallocated
	UDP_OPENED
} UDP_STATE;


// Stores information about a current UDP socket
typedef struct
{
	union
	{
		NODE_INFO	remoteNode;		// 10 bytes for MAC and IP address
		uint32_t		remoteHost;		// RAM or ROM pointer to a hostname string (ex: "www.microchip.com")
	} remote;
    //NODE_INFO   remoteNode;		// IP and MAC of remote node
    UDP_PORT    remotePort;		// Remote node's UDP port number
    UDP_PORT    localPort;		// Local UDP port number, or INVALID_UDP_PORT when free
    UDP_STATE smState;			// State of this socket
    uint32_t retryInterval;
	uint8_t retryCount;
	struct
	{
		unsigned char bRemoteHostIsROM : 1;	// Remote host is stored in ROM
	}flags;
	uint16_t eventTime;
} UDP_SOCKET_INFO;


#define INVALID_UDP_SOCKET      (0xffu)		// Indicates a UDP socket that is not valid
#define INVALID_UDP_PORT        (0ul)		// Indicates a UDP port that is not valid

/****************************************************************************
  Section:
	External Global Variables
  ***************************************************************************/
#if !defined(__UDP_C)
    extern UDP_SOCKET activeUDPSocket;
    extern UDP_SOCKET_INFO  UDPSocketInfo[MAX_UDP_SOCKETS];
	extern uint16_t UDPTxCount;
	extern uint16_t UDPRxCount;
#endif

// Stores the header of a UDP packet
typedef struct
{
    UDP_PORT    SourcePort;				// Source UDP port
    UDP_PORT    DestinationPort;		// Destination UDP port
    uint16_t        Length;					// Length of data
    uint16_t        Checksum;				// UDP checksum of the data
} UDP_HEADER;


// Create a server socket and ignore dwRemoteHost.
#define UDP_OPEN_SERVER		0u
#if defined(STACK_CLIENT_MODE)
	#if defined(STACK_USE_DNS)
		// Create a client socket and use dwRemoteHost as a RAM pointer to a hostname string.
		#define UDP_OPEN_RAM_HOST	1u
		// Create a client socket and use dwRemoteHost as a ROM pointer to a hostname string.
		#define UDP_OPEN_ROM_HOST	2u
	#else
		// Emit an undeclared identifier diagnostic if code tries to use UDP_OPEN_RAM_HOST while the DNS client module is not enabled. 
		#define UDP_OPEN_RAM_HOST	You_need_to_enable_STACK_USE_DNS_to_use_UDP_OPEN_RAM_HOST
		// Emit an undeclared identifier diagnostic if code tries to use UDP_OPEN_ROM_HOST while the DNS client module is not enabled. 
		#define UDP_OPEN_ROM_HOST	You_need_to_enable_STACK_USE_DNS_to_use_UDP_OPEN_ROM_HOST
	#endif
	// Create a client socket and use dwRemoteHost as a literal IP address.
	#define UDP_OPEN_IP_ADDRESS	3u
#endif

// Create a client socket and use dwRemoteHost as a pointer to a NODE_INFO structure containing the exact remote IP address and MAC address to use.
#define UDP_OPEN_NODE_INFO	4u


/****************************************************************************
  Section:
	Function Prototypes
  ***************************************************************************/
void UDPInit(void);
void UDPTask(void);
UDP_SOCKET UDPOpenEx(uint32_t remoteHost, uint8_t remoteHostType, UDP_PORT localPort,UDP_PORT remotePort);

//UDP_SOCKET UDPOpen(UDP_PORT localPort, NODE_INFO *remoteNode, UDP_PORT remotePort);
void UDPClose(UDP_SOCKET s);
_Bool UDPProcess(NODE_INFO *remoteNode, IP_ADDR *localIP, uint16_t len);

void UDPSetTxBuffer(uint16_t wOffset);
void UDPSetRxBuffer(uint16_t wOffset);
uint16_t UDPIsPutReady(UDP_SOCKET s);
_Bool UDPPut(uint8_t v);
_Bool UDPPutW(uint16_t w);
uint16_t UDPPutArray(const uint8_t *cData, uint16_t wDataLen);
uint8_t* UDPPutString(const uint8_t *strData);
void UDPFlush(void);

// ROM function variants for PIC18
#if defined(__18CXX)
	uint16_t UDPPutROMArray(ROM uint8_t *cData, uint16_t wDataLen);
	rom char* UDPPutROMString(rom const char* strData);
#else
	#define UDPPutROMArray(a,b)	UDPPutArray((uint8_t*)a,b)
	#define UDPPutROMString(a)	UDPPutString((uint8_t*)a)
#endif

uint16_t UDPIsGetReady(UDP_SOCKET s);
_Bool UDPGet(uint8_t *v);
uint16_t UDPGetArray(uint8_t *cData, uint16_t wDataLen);
void UDPDiscard(void);
_Bool UDPIsOpened(UDP_SOCKET socket);

/*****************************************************************************
  Function:
    UDP_SOCKET UDPOpen(UDP_PORT localPort, NODE_INFO* remoteNode, 
                        UDP_PORT remotePort)

  Summary:
    Macro of the legacy version of UDPOpen.
    
  Description:
    UDPOpen is a macro replacement of the legacy implementation of UDPOpen.
    Creates a UDP socket handle for transmiting or receiving UDP packets.  
    Call this function to obtain a handle required by other UDP function.

  Precondition:
    UDPInit() must have been previously called.

  Parameters:
    localPort - UDP port number to listen on.  If 0, stack will dynamically 
        assign a unique port number to use.
    remoteNode - Pointer to remote node info (MAC and IP address) for this
        connection.  If this is a server socket (receives the first packet) 
        or the destination is the broadcast address, then this parameter
        should be NULL.
    remotePort - For client sockets, the remote port number.

  Return Values:
    Success -
        A UDP socket handle that can be used for subsequent UDP API calls.
    Failure -
        INVALID_UDP_SOCKET.  This function fails when no more UDP socket 
        handles are available.  Increase MAX_UDP_SOCKETS to make more sockets 
        available.
    
  Remarks:
    When finished using the UDP socket handle, call the UDPClose() function 
    to free the socket and delete the handle.
  ***************************************************************************/
#define UDPOpen(localPort,remoteNode,remotePort)  UDPOpenEx((uint32_t)remoteNode,UDP_OPEN_NODE_INFO,localPort,remotePort)

#endif
