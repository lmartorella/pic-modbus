#ifndef _MODBUS_CLIENT_H
#define	_MODBUS_CLIENT_H

#include "configuration.h"
#include "net/guid.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Common interface for Modbus server (secondary) nodes.
 * It can be implemented, for instance, by RTU nodes (RS485 client), TCP clients or 
 * wireless radio client stations.
 *
 * Routers (e.g. from TCP to RTU) will run both client and server instances.
 */

/**
 * Initialize the client node
 */
void bus_cl_init();

/**
 * The current station address.
 */
extern uint8_t bus_cl_stationAddress;

/**
 * Poll for bus client activities.
 * Returns true if the node is currently active and requires short polling.
 * Returns false if the node is not currently active and it can be polled with larger period,
 * depending on the medium implementation (e.g. 1ms)
 */
__bit bus_cl_poll();

/**
 * The external function handler that produces the function response data to send to the server
 * during a read call. The buffer to use is `rs485_buffer`
 */
typedef void (*ReadHandler)();

/**
 * The external function handler that consumes the function data sent by the server
 * during a write call. The buffer to use is `rs485_buffer`
 */
typedef void (*WriteHandler)();

/**
 * The total function count (system + apps). Must be filled by the application.
 */
extern const uint8_t bus_cl_functionCount;

/**
 * The function definitions, of size `bus_cl_functionCount`.
 * Every function will span 8 16-bit registers: the first function will have the address 0x0,
 * the second one the address 0x08, etc...
 */
extern const ReadHandler bus_cl_functionReadHandlers[];
extern const WriteHandler bus_cl_functionWriteHandlers[];

/***
 ***
 * Specific state for RTU client
 ***
 **/

/**
 * Count of un-adertised CRC errors in receiving message
 */
extern uint8_t bus_crcErrors;

// RS485 Modbus defines station address in the range 1 to 247. 0 is used for broadcast messages without acknowledge.
// So use 254 as special "unassigned" address. When the AUTO_REGISTER function is called, the only device in auto mode in the bus
// should reply and change his address.
#define UNASSIGNED_STATION_ADDRESS 254

typedef enum {
    // The client bus is idle, waiting for a complete frame header
    BUS_CL_RTU_IDLE,
    // Read mode: skipping input data and waiting for the next idle state.
    BUS_CL_RTU_WAIT_FOR_IDLE,
    // Read the address and size of the data to read/write
    BUS_CL_RTU_WAIT_REGISTER_DATA,
    // Wait the checksum to validate the message
    BUS_CL_RTU_CHECK_REQUEST_CRC,
    // Wait the end of the packet and then start response
    BUS_CL_RTU_WAIT_FOR_RESPONSE,
    // Transmit response
    BUS_CL_RTU_RESPONSE,
    // The function read function is piped to the "Write Register" function request
    BUS_CL_RTU_READ_STREAM,
    // The function write function is piped to the "Read Register" function response
    BUS_CL_RTU_WRITE_STREAM,
    // When the response is completed and the response CRC should be written
    BUS_CL_RTU_WRITE_RESPONSE_CRC,
    // Wait for the RS485 module to end the transmission
    BUS_CL_RTU_WAIT_FOR_FLUSH
} BUS_CL_RTU_STATE;
extern BUS_CL_RTU_STATE bus_cl_rtu_state;

typedef enum {
    NO_ERROR = 0,
    ERR_INVALID_FUNCTION = 1,
    ERR_INVALID_ADDRESS = 2,
    ERR_INVALID_SIZE = 3
} BUL_CL_RTU_EXCEPTION_CODE;

#ifdef __cplusplus
}
#endif

#endif	/* BUS_SEC_H */
