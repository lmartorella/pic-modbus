#include "../pch.h"
#include "ip_raspbian.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <linux/tcp.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>

AppConfig_t AppConfig;
in_addr_t ip_bcastAddr;
static int tcp_sock;
static int listen_socket;
#define TCP_BUFSIZE 1024
static BYTE tcp_buffer[TCP_BUFSIZE];
static BYTE* tcp_bufPtr = &tcp_buffer[0];
        
void StackInit() {
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
        fatal("getifaddrs");
    }

    /* Walk through linked list, maintaining head pointer so we
       can free list later */
    for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        int family = ifa->ifa_addr->sa_family;
        if (family != AF_INET) 
            continue;
        if (strcmp(ifa->ifa_name, "lo") == 0)
            continue;

        AppConfig.MyIPAddr = ((struct sockaddr_in*)ifa->ifa_addr)->sin_addr.s_addr;
        ip_bcastAddr = ((struct sockaddr_in*)(ifa->ifa_ifu.ifu_broadaddr))->sin_addr.s_addr;
    }
    freeifaddrs(ifaddr);
    
    listen_socket = -1;
}

TCP_SOCKET TCPOpen(DWORD dwRemoteHost, BYTE vRemoteHostType, WORD wPort, BYTE vSocketPurpose) {
    if (dwRemoteHost != 0 || vRemoteHostType != TCP_OPEN_SERVER || vSocketPurpose != TCP_PURPOSE_GENERIC_TCP_SERVER) {
        fatal("Not supported");
    }
    
    tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock < 0) {
        fatal("Could not create TCP socket");
    }
    if (setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
        fatal("Could not call setsockopt on TCP socket");
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(wPort);

    if (bind(tcp_sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        fatal("TCP bind failed");
    }

    if (listen(tcp_sock, 0) != 0) {
        fatal("TCP listen failed");
    }
    
    return tcp_sock;
}

void TCPDisconnect(TCP_SOCKET socket) {
    close(listen_socket);
    listen_socket = -1;
    tcp_bufPtr = &tcp_buffer[0];
}

void TCPDiscard(TCP_SOCKET socket) {
    //recv(listen_socket, tcp_buffer, TCP_BUFSIZE, MSG_DONTWAIT);
}

WORD TCPIsGetReady(TCP_SOCKET socket) {
    if (listen_socket >= 0) {
        struct pollfd pfd;
        pfd.fd = listen_socket;
        pfd.events = POLLIN | POLLHUP | POLLRDHUP;
        int err = poll(&pfd, 1, 0);
        if (err < 0) {
            char buf[36];
            sprintf(buf, "poll error: %d", errno);
            fatal(buf);
        }
        if (err > 0) {
            if (pfd.revents & (POLLHUP | POLLRDHUP)) {
                // Socket closed
                TCPDisconnect(listen_socket);
            }
            if (pfd.revents & POLLIN) {
                // Data avail!
                BYTE buf[256];
                int ret = recv(listen_socket, buf, 256, MSG_PEEK | MSG_DONTWAIT);
                if (ret < 0) ret = 0;
                return ret;
            }
        }
    } 
    return 0;
}

WORD TCPIsPutReady(TCP_SOCKET socket) {
    return 0x7fff;
}

BOOL TCPIsConnected(TCP_SOCKET socket) {
    return (listen_socket >= 0);
}

void StackTask() {
    if (listen_socket < 0) {
        // clear the socket set
        fd_set readfds;
        FD_ZERO(&readfds);

        // add master socket to set
        FD_SET(tcp_sock, &readfds);
        int max_sd = tcp_sock;    

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        int ret = select(max_sd + 1, &readfds, NULL, NULL, &tv);
        if (ret < 0) {
            fatal("Select error");
        }

        if (ret > 0) {
            struct sockaddr_in address;
            int addrlen = sizeof(address);
            listen_socket = accept(tcp_sock, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (listen_socket < 0) {
                fatal("Error in TCP accept");
            }
            
            // Set nonblocking IO
            int flags = fcntl(listen_socket, F_GETFL, 0);
            if (flags < 0) {
                fatal("Error in fcntl get");
            }

            flags = flags | O_NONBLOCK;
            if (fcntl(listen_socket, F_SETFL, flags) != 0) {
                fatal("Error in fcntl set");
            }
            
            // 1 byte enough for sending (e.g. close), low water 
            int sndlowat = 1;
            setsockopt(listen_socket, SOL_SOCKET, SO_SNDLOWAT, &sndlowat, sizeof(sndlowat));
            int nodelay = 1;
            setsockopt(listen_socket, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
        } 
    }
}

WORD TCPGetArray(TCP_SOCKET socket, BYTE* buf, WORD size) {
    return recv(listen_socket, buf, size, MSG_DONTWAIT);
}

void TCPPutArray(TCP_SOCKET socket, const BYTE* cData, WORD size) {
    memcpy(tcp_bufPtr, cData, size);
    tcp_bufPtr += size;
}

void TCPFlush(TCP_SOCKET socket) {
    int size = tcp_bufPtr - &tcp_buffer[0];
    if (send(listen_socket, tcp_buffer, size, MSG_DONTWAIT) != size) {
        fatal("Socket error, send");
    }
    tcp_bufPtr = &tcp_buffer[0];
}

static int udp_sock;
static struct sockaddr_in udp_broadcastAddr;
#define UDP_BUFSIZE 1024
static BYTE udp_buffer[UDP_BUFSIZE];
static BYTE* udp_bufPtr = &udp_buffer[0];

UDP_SOCKET UDPOpenEx(DWORD remoteHost, BYTE remoteHostType, UDP_PORT localPort, UDP_PORT remotePort) {
    if (remoteHost != 0 || remoteHostType != UDP_OPEN_NODE_INFO || localPort != 0) {
        fatal("Not supported");
    }
    
    udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_sock < 0) {
        fatal("Could not create UDP socket");
    }
    int broadcastEnable = 1;
    if (setsockopt(udp_sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable))) {
        fatal("UDP setsockopt");
    }
    
    udp_broadcastAddr.sin_family = AF_INET;
    udp_broadcastAddr.sin_port = htons(remotePort);
    udp_broadcastAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    return udp_sock;
}

WORD UDPIsPutReady(UDP_SOCKET sock) {
    return UDP_BUFSIZE - (udp_bufPtr - &udp_buffer[0]); 
}

void UDPPutString(const BYTE *strData) {
    UDPPutArray(strData, strlen(strData));
}

void UDPPutArray(const BYTE *cData, WORD wDataLen) {
    memcpy(udp_bufPtr, cData, wDataLen);
    udp_bufPtr += wDataLen;
}

void UDPPutW(WORD w) {
    memcpy(udp_bufPtr, &w, 2);
    udp_bufPtr += 2;
}

void UDPFlush() {
    int l = udp_bufPtr - &udp_buffer[0];
    if (sendto(udp_sock, udp_buffer, l, 0, (struct sockaddr*)&udp_broadcastAddr, sizeof(udp_broadcastAddr)) != l) {
        fatal("UDP send error");
    }
    udp_bufPtr = &udp_buffer[0];
}

