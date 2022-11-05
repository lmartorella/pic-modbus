#include "net/net.h"

#if _CONF_POSIX
#include "./hardware/posix/ip.h"
#endif

#ifdef __XC8
#include "tcpipstack/Include/Compiler.h"
#include "tcpipstack/Include/TCPIPStack/TCPIP.h"
APP_CONFIG AppConfig;
#endif
    
#ifdef DEBUG
// 17008 is the debug port
#define SERVER_CONTROL_UDP_PORT 17008
#else
// 17007 is the release port
#define SERVER_CONTROL_UDP_PORT 17007
#endif
#define CLIENT_TCP_PORT 20000

// UDP broadcast socket
static UDP_SOCKET s_heloSocket;  
// TCP lister control socket
static TCP_SOCKET s_controlSocket;
static __bit s_lastDhcpState = false;
static __bit s_sendHelo = 0;

static void pollControlPort();

// Close the control port listener
void prot_prim_control_abort() {
    // Returns in listening state
    TCPDiscard(s_controlSocket);
    TCPDisconnect(s_controlSocket);
}

__bit prot_prim_control_readW(uint16_t* w) {
    uint16_t l = TCPIsGetReady(s_controlSocket);
    if (l < 2) { 
        return false;
    }
    TCPGetArray(s_controlSocket, (uint8_t*)w, sizeof(uint16_t));
    return true;
}

__bit prot_prim_control_read(void* data, uint16_t size) {
    uint16_t l = TCPIsGetReady(s_controlSocket);
    if (l < size) {
        return false;
    }
    TCPGetArray(s_controlSocket, (uint8_t*)data, size);
    return true;
}

void prot_prim_control_writeW(uint16_t w) {
    TCPPutArray(s_controlSocket, (uint8_t*)&w, sizeof(uint16_t));
}

void prot_prim_control_write(const void* data, uint16_t size) {
    // If I remove & from here, ip_control_read stop working!!
    TCPPutArray(s_controlSocket, (const uint8_t*)data, size);
}

// Flush and OVER to other party. TCP is full duplex, so OK to only flush
void prot_prim_control_over() {
    TCPFlush(s_controlSocket);
}

__bit prot_prim_control_isConnected() {
    return s_lastDhcpState && TCPIsConnected(s_controlSocket);
}

uint16_t prot_prim_control_readAvail() {
    return TCPIsGetReady(s_controlSocket);
}

uint16_t prot_prim_control_writeAvail() {
    return TCPIsPutReady(s_controlSocket);
}

void ip_prot_init() {
    io_println("IP/DHCP");
#if defined(_CONF_POSIX)
    printf("Listen port: %d\n", SERVER_CONTROL_UDP_PORT);
#endif
    
#ifdef __XC8
    memset(&AppConfig, 0, sizeof(AppConfig));
    AppConfig.Flags.bIsDHCPEnabled = 1;
    AppConfig.MyMACAddr.v[0] = MY_DEFAULT_MAC_BYTE1;
    AppConfig.MyMACAddr.v[1] = MY_DEFAULT_MAC_BYTE2;
    AppConfig.MyMACAddr.v[2] = MY_DEFAULT_MAC_BYTE3;
    AppConfig.MyMACAddr.v[3] = MY_DEFAULT_MAC_BYTE4;
    AppConfig.MyMACAddr.v[4] = MY_DEFAULT_MAC_BYTE5;
    AppConfig.MyMACAddr.v[5] = MY_DEFAULT_MAC_BYTE6;

#endif
    // Init ETH loop data
    StackInit();  
    
    s_heloSocket = UDPOpenEx(0, UDP_OPEN_NODE_INFO, 0, SERVER_CONTROL_UDP_PORT);
    if (s_heloSocket == INVALID_UDP_SOCKET)
    {
        fatal("SOC.opn1");
    }

    // Open the sever TCP channel
    s_controlSocket = TCPOpen(0, TCP_OPEN_SERVER, CLIENT_TCP_PORT, TCP_PURPOSE_GENERIC_TCP_SERVER);
    if (s_controlSocket == INVALID_SOCKET)
    {
        fatal("SOC.opn2");
    }
    
    s_sendHelo = 0;
}

/*
	Manage slow timer (heartbeats)
*/
void ip_prot_slowTimer() {
    _Bool dhcpOk = DHCPIsBound(0);

    if (dhcpOk != s_lastDhcpState)
    {
        char buffer[16];
        if (dhcpOk)
        {
            unsigned char* p = (unsigned char*)(&AppConfig.MyIPAddr);
            sprintf(buffer, "%d.%d.%d.%d", (int)p[0], (int)p[1], (int)p[2], (int)p[3]);
            io_printlnStatus(buffer);
            s_lastDhcpState = true;
        }
        else
        {
            sprintf(buffer, "DHCP ERR");
            io_printlnStatus(buffer);
            s_lastDhcpState = false;
            //fatal("DHCP.nok");
        }
    }
    if (dhcpOk)
    {
        // Ping server every second. Enqueue HELO packet
        s_sendHelo = 1;
    }
}

/**
 * HOME request
 */
__PACK typedef struct {
	char preamble[4];
	char messageType[4];
	GUID device;
	uint16_t controlPort;
} HOME_REQUEST;

void ip_poll() {
    // Do ETH stuff
    StackTask();
    // This tasks invokes each of the core stack application tasks
    StackApplications();

    if (s_sendHelo) {
        // Still no HOME? Ping HELO
        if (UDPIsPutReady(s_heloSocket) >= (sizeof(HOME_REQUEST) + BUFFER_MASK_SIZE + 2))
        {
            UDPPutString("HOME");
            UDPPutString(prot_registered ? (bus_prim_hasDirtyChildren ? "CCHN" : "HTB2") : "HEL4");
            UDPPutArray((uint8_t*)(&pers_data.deviceId), sizeof(GUID));
            UDPPutW(CLIENT_TCP_PORT);
            if (prot_registered) {
                UDPPutW(BUFFER_MASK_SIZE);
                if (bus_prim_hasDirtyChildren) {
                    // CCHN: mask of changed children
                    UDPPutArray(bus_prim_dirtyChildren, BUFFER_MASK_SIZE);
                } else {
                    // HTB2: list of alive children
                    UDPPutArray(bus_prim_knownChildren, BUFFER_MASK_SIZE);
                }
            }
            UDPFlush();
            
            s_sendHelo = 0;
        }
    }
}

void ip_flush() {
    TCPFlush(s_controlSocket);
    // Leave the socket open
}

void ip_waitEvent() {
#ifdef _CONF_POSIX
    // Wait max 100ms for data
    TCPWaitEvent(100);
#endif
}