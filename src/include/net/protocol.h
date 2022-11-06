#ifndef PROTOCOL_H
#define	PROTOCOL_H

/**
 * Protocol implementation module
 */

typedef enum { 
    // Message to beat a bean
    BUS_MSG_TYPE_HEARTBEAT = 1,
    // Message to ask unknown bean to present (broadcast)
    BUS_MSG_TYPE_READY_FOR_HELLO = 2,
    // Message to ask the only unknown bean to register itself
    BUS_MSG_TYPE_ADDRESS_ASSIGN = 3,
    // Command/data will follow: socket open
    BUS_MSG_TYPE_CONNECT = 4
} BUS_MSG_TYPE;

typedef enum { 
    // Bean: ack heartbeat
    BUS_ACK_TYPE_HEARTBEAT = 0x20,
    // Bean: notify unknown (response to BUS_MSG_TYPE_READY_FOR_HELLO)
    BUS_ACK_TYPE_HELLO = 0x21,
    // Bean: notify status dirty (e.g. exception/error to read). Only sent to regular heartbeat (known)
    BUS_ACK_TYPE_READ_STATUS = 0x22
} BUS_ACK_TYPE;

// Low-level socket state. The byte can be used on the wire as break char
typedef enum {
    // Normal state when not connected
    SOCKET_NOT_CONNECTED = -2,
    // Socket data timeout (no data in BUS_SOCKET_TIMEOUT)
    SOCKET_ERR_TIMEOUT = -3,
    // When a frame error on the wire
    SOCKET_ERR_FRAME_ERR = -4,
    // When the server (IP) closes the socket
    SOCKET_ERR_CLOSED_BY_PARENT = -5
} SOCKET_STATE;

#define UNASSIGNED_SUB_ADDRESS 0xff

void prot_init();
/**
 * Returns true if active and require polling
 */
_Bool prot_prim_poll();
_Bool prot_sec_poll();

// Has the slow timer ticked?
extern __bit prot_slowTimer;
extern __bit prot_registered;

/**
 * Implementation dependent calls
 */
// Directly with define in order to minimize stack usage
#define prot_sec_control_readW(w) rs485_read((uint8_t*)w, 2) 
#define prot_sec_control_read(data, size) rs485_read((uint8_t*)data, (uint8_t)size)
#define prot_sec_control_writeW(w) rs485_write(false, (uint8_t*)&w, 2)
#define prot_sec_control_write(data, size) rs485_write(false, (uint8_t*)data, (uint8_t)size)
#define prot_sec_control_over() set_rs485_over()
#define prot_sec_control_idle(buf) rs485_write(true, buf, 1)
#define prot_sec_control_readAvail() rs485_readAvail()
#define prot_sec_control_writeAvail() rs485_writeAvail()
#define prot_sec_control_isConnected() bus_sec_isConnected()
#define prot_sec_control_close() bus_sec_abort()

__bit prot_prim_control_readW(uint16_t* w);
__bit prot_prim_control_read(void* data, uint16_t size);
void prot_prim_control_writeW(uint16_t w);
void prot_prim_control_write(const void* data, uint16_t size);
void prot_prim_control_over(void);
#define prot_prim_control_idle()
uint16_t prot_prim_control_readAvail(void);
uint16_t prot_prim_control_writeAvail(void);
#define prot_prim_control_isConnected() ip_isConnected()
#define prot_prim_control_close() ip_flush()
void prot_prim_control_abort(void);


#if defined(HAS_RS485_BUS_PRIMARY)
#define prot_control_writeAvail prot_prim_control_writeAvail
#define prot_control_write prot_prim_control_write
#define prot_control_writeW prot_prim_control_writeW
#define prot_control_readAvail prot_prim_control_readAvail
#define prot_control_read prot_prim_control_read
#define prot_control_readW prot_prim_control_readW
#elif defined(HAS_RS485_BUS_SECONDARY)
#define prot_control_writeAvail prot_sec_control_writeAvail
#define prot_control_write prot_sec_control_write
#define prot_control_writeW prot_sec_control_writeW
#define prot_control_readAvail prot_sec_control_readAvail
#define prot_control_read prot_sec_control_read
#define prot_control_readW prot_sec_control_readW
#endif

#endif	/* PROTOCOL_H */
