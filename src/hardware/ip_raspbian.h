#ifndef IP_RASPBIAN_H
#define IP_RASPBIAN_H

#include <sys/socket.h>
#include <netinet/in.h>

typedef WORD UDP_PORT;
typedef int UDP_SOCKET;
typedef int TCP_SOCKET;

typedef struct {
    DWORD MyIPAddr;
} AppConfig_t;
extern AppConfig_t AppConfig;

#define DHCPIsBound(v) (1)

void StackInit();
void StackTask();
#define StackApplications()

void TCPDiscard(TCP_SOCKET socket);
void TCPDisconnect(TCP_SOCKET socket);
WORD TCPIsGetReady(TCP_SOCKET socket);
BOOL TCPIsConnected(TCP_SOCKET socket);
void TCPFlush(TCP_SOCKET socket);
WORD TCPGetArray(TCP_SOCKET socket, BYTE* buf, WORD size);
void TCPPutArray(TCP_SOCKET socket, const BYTE* buf, WORD size);
TCP_SOCKET TCPOpen(DWORD dwRemoteHost, BYTE vRemoteHostType, WORD wPort, BYTE vSocketPurpose);
#define TCP_OPEN_SERVER		0u
#define TCP_PURPOSE_GENERIC_TCP_SERVER 1
#define INVALID_SOCKET      (0xFE)	// The socket is invalid or could not be opened

UDP_SOCKET UDPOpenEx(DWORD remoteHost, BYTE remoteHostType, UDP_PORT localPort, UDP_PORT remotePort);
#define UDP_OPEN_NODE_INFO	4u
#define INVALID_UDP_SOCKET      (0xffu)		// Indicates a UDP socket that is not valid
WORD UDPIsPutReady(UDP_SOCKET s);
void UDPPutString(const BYTE *strData);
void UDPPutArray(const BYTE *cData, WORD wDataLen);
void UDPPutW(WORD w);
void UDPFlush();

#endif /* IP_RASPBIAN_H */

