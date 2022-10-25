#ifndef IP_RASPBIAN_H
#define IP_RASPBIAN_H

#include <sys/socket.h>
#include <netinet/in.h>

typedef uint16_t UDP_PORT;
typedef int UDP_SOCKET;
typedef int TCP_SOCKET;

typedef struct {
    uint32_t MyIPAddr;
} AppConfig_t;
extern AppConfig_t AppConfig;

#define DHCPIsBound(v) (1)

void StackInit();
void StackTask();
#define StackApplications()

void TCPDiscard(TCP_SOCKET socket);
void TCPDisconnect(TCP_SOCKET socket);
uint16_t TCPIsGetReady(TCP_SOCKET socket);
uint16_t TCPIsPutReady(TCP_SOCKET socket);
_Bool TCPIsConnected(TCP_SOCKET socket);
void TCPFlush(TCP_SOCKET socket);
uint16_t TCPGetArray(TCP_SOCKET socket, uint8_t* buf, uint16_t size);
void TCPPutArray(TCP_SOCKET socket, const uint8_t* buf, uint16_t size);
TCP_SOCKET TCPOpen(uint32_t dwRemoteHost, uint8_t vRemoteHostType, uint16_t wPort, uint8_t vSocketPurpose);
#define TCP_OPEN_SERVER		0u
#define TCP_PURPOSE_GENERIC_TCP_SERVER 1
#define INVALID_SOCKET      (0xFE)	// The socket is invalid or could not be opened

UDP_SOCKET UDPOpenEx(uint32_t remoteHost, uint8_t remoteHostType, UDP_PORT localPort, UDP_PORT remotePort);
#define UDP_OPEN_NODE_INFO	4u
#define INVALID_UDP_SOCKET      (0xffu)		// Indicates a UDP socket that is not valid
uint16_t UDPIsPutReady(UDP_SOCKET s);
void UDPPutString(const char *strData);
void UDPPutArray(const uint8_t *cData, uint16_t wDataLen);
void UDPPutW(uint16_t w);
void UDPFlush();

#endif /* IP_RASPBIAN_H */

