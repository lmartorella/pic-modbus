#include "pch.h"
#include "protocol.h"
#include "appio.h"
#include "ip_client.h"
#include "persistence.h"

#ifdef HAS_IP
#ifdef __XC8
    #include "tcpipstack/Include/Compiler.h"
    #include "tcpipstack/Include/TCPIPStack/TCPIP.h"
    APP_CONFIG AppConfig;
#endif

// UDP broadcast socket
static UDP_SOCKET s_heloSocket;  
// TCP lister control socket
static TCP_SOCKET s_controlSocket;
static bit s_lastDhcpState = FALSE;
static bit s_sendHelo = 0;

/*
	HOME request
*/
__PACK typedef struct
{
	char preamble[4];
	char messageType[4];
	GUID device;
	WORD controlPort;
} HOME_REQUEST;

static void pollControlPort();

void prot_control_close()
{
    TCPFlush(s_controlSocket);
    // Leave the socket open
}

// Close the control port listener
void prot_control_abort()
{
    // Returns in listening state
    TCPDiscard(s_controlSocket);
    TCPDisconnect(s_controlSocket);
}

bit prot_control_readW(WORD* w)
{
    WORD l = TCPIsGetReady(s_controlSocket);
    if (l < 2) { 
        return FALSE;
    }
    TCPGetArray(s_controlSocket, (BYTE*)w, sizeof(WORD));
    return TRUE;
}

bit prot_control_read(void* data, WORD size)
{
    WORD l = TCPIsGetReady(s_controlSocket);
    if (l < size) {
        return FALSE;
    }
    TCPGetArray(s_controlSocket, (BYTE*)data, size);
    return TRUE;
}

void prot_control_writeW(WORD w)
{
    TCPPutArray(s_controlSocket, (BYTE*)&w, sizeof(WORD));
}

void prot_control_write(const void* data, WORD size)
{
    // If I remove & from here, ip_control_read stop working!!
    TCPPutArray(s_controlSocket, (const BYTE*)data, size);
}

// Flush and OVER to other party. TCP is full duplex, so OK to only flush
void prot_control_over()
{
    TCPFlush(s_controlSocket);
}

bit prot_control_isConnected()
{
    return s_lastDhcpState && TCPIsConnected(s_controlSocket);
}

WORD prot_control_readAvail()
{
    return TCPIsGetReady(s_controlSocket);
}

WORD prot_control_writeAvail()
{
    return TCPIsPutReady(s_controlSocket);
}

void ip_prot_init()
{
    io_println("IP/DHCP");
#if defined(__GNU)
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
void ip_prot_slowTimer()
{
    BOOL dhcpOk = DHCPIsBound(0);

    if (dhcpOk != s_lastDhcpState)
    {
        char buffer[16];
        if (dhcpOk)
        {
            unsigned char* p = (unsigned char*)(&AppConfig.MyIPAddr);
            sprintf(buffer, "%d.%d.%d.%d", (int)p[0], (int)p[1], (int)p[2], (int)p[3]);
            io_printlnStatus(buffer);
            s_lastDhcpState = TRUE;
        }
        else
        {
            sprintf(buffer, "DHCP ERR");
            io_printlnStatus(buffer);
            s_lastDhcpState = FALSE;
            //fatal("DHCP.nok");
        }
    }
    if (dhcpOk)
    {
        // Ping server every second. Enqueue HELO packet
        s_sendHelo = 1;
    }
}

void ip_poll() 
{
    if (s_sendHelo) {
        // Still no HOME? Ping HELO
        if (UDPIsPutReady(s_heloSocket) >= sizeof(HOME_REQUEST))
        {
            UDPPutString("HOME");
#ifdef HAS_BUS_SERVER
            UDPPutString(prot_registered ? (bus_hasDirtyChildren ? "CCHN" : "HTBT") : "HEL4");
#else
            UDPPutString(prot_registered ? "HTBT" : "HEL4");
#endif
            UDPPutArray((BYTE*)(&pers_data.deviceId), sizeof(GUID));
            UDPPutW(CLIENT_TCP_PORT);
#ifdef HAS_BUS_SERVER
            if (prot_registered && bus_hasDirtyChildren) {
                UDPPutW(BUFFER_MASK_SIZE);
                UDPPutArray(bus_dirtyChildren, BUFFER_MASK_SIZE);
            }
#endif
            UDPFlush();
            
            s_sendHelo = 0;
        }
    }
}

#endif // HAS_IP
