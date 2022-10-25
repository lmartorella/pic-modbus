/*********************************************************************
 *
 *                  ARP Module Defs for Microchip TCP/IP Stack
 *
 *********************************************************************
 * FileName:        ARP.h
 * Dependencies:    Stacktsk.h
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
 * Nilesh Rajbharti     5/1/01  Original        (Rev 1.0)
 * Nilesh Rajbharti     2/9/02  Cleanup
 * Nilesh Rajbharti     5/22/02 Rev 2.0 (See version.log for detail)
 * Howard Schlunder		8/17/06	Combined ARP.h and ARPTsk.h into ARP.h
 ********************************************************************/
#ifndef __ARP_H
#define __ARP_H

#ifdef STACK_CLIENT_MODE
	void ARPInit(void);
#else
	#define ARPInit()
#endif

#define ARP_OPERATION_REQ       0x0001u		// Operation code indicating an ARP Request
#define ARP_OPERATION_RESP      0x0002u		// Operation code indicating an ARP Response

#define HW_ETHERNET             (0x0001u)	// ARP Hardware type as defined by IEEE 802.3
#define ARP_IP                  (0x0800u)	// ARP IP packet type as defined by IEEE 802.3


// ARP packet structure
__ALIGN2PACK typedef struct
{
    uint16_t        HardwareType;   // Link-layer protocol type (Ethernet is 1).
    uint16_t        Protocol;       // The upper-layer protocol issuing an ARP request (0x0800 for IPv4)..
    uint8_t        MACAddrLen;     // MAC address length (6).
    uint8_t        ProtocolLen;    // Length of addresses used in the upper-layer protocol (4).
    uint16_t        Operation;      // The operation the sender is performing (ARP_REQ or ARP_RESP).
    MAC_ADDR    SenderMACAddr;  // The sender's hardware (MAC) address.
    IP_ADDR     SenderIPAddr;   // The sender's IP address.
    MAC_ADDR    TargetMACAddr;  // The target node's hardware (MAC) address.
    IP_ADDR     TargetIPAddr;   // The target node's IP address.
} ARP_PACKET;

_Bool ARPProcess(void);
void ARPResolve(IP_ADDR* IPAddr);
_Bool ARPIsResolved(IP_ADDR* IPAddr, MAC_ADDR* MACAddr);
void SwapARPPacket(ARP_PACKET* p);

#ifdef STACK_USE_ZEROCONF_LINK_LOCAL
	/* API specific Definitions */
	#define ARP_REQ       0x0001u		// Operation code indicating an ARP Request
	#define ARP_RESP      0x0002u		// Operation code indicating an ARP Response

	struct arp_app_callbacks {
    	_Bool used;
    	void (*ARPPkt_notify)(uint32_t SenderIPAddr, uint32_t TargetIPAddr, 
                          	MAC_ADDR* SenderMACAddr, MAC_ADDR* TargetMACAddr, uint8_t op_req);
	};
	CHAR ARPRegisterCallbacks(struct arp_app_callbacks *app);
	_Bool ARPDeRegisterCallbacks(CHAR id);
#endif
	_Bool ARPSendPkt(uint32_t SrcIPAddr, uint32_t DestIPAddr, uint8_t op_req );
#endif


