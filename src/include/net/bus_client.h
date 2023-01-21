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

// RS485 Modbus defines station address in the range 1 to 247. 0 is used for broadcast messages without acknowledge.
// So use 254 as special "unassigned" address. When the AUTO_REGISTER function is called, the only device in auto mode in the bus
// should reply and change his address.
#define UNASSIGNED_STATION_ADDRESS (254)

/**
 * Poll for bus client activities.
 * Returns true if the node is currently active and requires short polling.
 * Returns false if the node is not currently active and it can be polled with larger period,
 * depending on the medium implementation (e.g. 1ms)
 */
__bit bus_cl_poll();

/**
 * Function handler descriptor. The register address is the descriptor index * 256.
 */
typedef struct {
    /**
     * The unique ID of the function, the type
     */
    FOURCC id;

    /**
     * The function handler that produces the function response data to send to the server
     * during a read call. The buffer size is `readSize`.
     */
    void (*onRead)(void* buffer);

    /**
     * The required sink read stream size, in bytes. Must by mutiple of 2. It is 0 if the sink only supports write.
     */
    uint8_t readSize;

    /**
     * The function handler that consumes the function data sent by the server
     * during a write call. The buffer size is `writeSize`.
     */
    void (*onWrite)(const void* buffer);

    /**
     * The required sink write stream size, in bytes. Must by mutiple of 2. It is 0 if the sink only supports reads.
     */
    uint8_t writeSize;
} FunctionDefinition;

/**
 * The function count in the `bus_cl_functions`. Must be filled by the application.
 */
extern const uint8_t bus_cl_function_count;

/**
 * The function definitions, of size `bus_cl_function_count`. Must be filled by the application.
 * The first function will have the address 0x0, the second one the address 0x100, regardless the read/write size.
 */
extern const FunctionDefinition bus_cl_functions[];

/***
 ***
 * Specific state for RTU client
 ***
 **/

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
    // The sink read function is piped to the "Write Register" function request
    BUS_CL_RTU_READ_STREAM,
    // The sink write function is piped to the "Read Register" function response
    BUS_CL_RTU_WRITE_STREAM,
    // When the response is completed and the response CRC should be written
    BUS_CL_RTU_WRITE_RESPONSE_CRC,
    // Wait for the RS485 module to end the transmission
    BUS_CL_RTU_WAIT_FOR_FLUSH
} BUS_CL_RTU_STATE;
extern BUS_CL_RTU_STATE bus_cl_rtu_state;

#ifdef __cplusplus
}
#endif

#endif	/* BUS_SEC_H */
