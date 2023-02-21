#include "assert.h"
#include "endian.h"
#include "net/bus_client.h"
#include "net/crc.h"
#include "net/rs485.h"

/**
 * Specific module for client Modbus RTU nodes (RS485), optimized
 * for 8-bit CPUs with low memory.
 */

typedef struct {
    uint8_t address;
    uint8_t function;
} ModbusRtuPacketHeader;

#define READ_HOLDING_REGISTERS (3)
#define WRITE_HOLDING_REGISTERS (16)

typedef struct {
    uint8_t registerAddressH; // big endian
    uint8_t registerAddressL; // big endian
    uint8_t countH;           // big endian
    uint8_t countL;           // big endian
} ModbusRtuHoldingRegisterRequest;

// Optimize number of read operations, for Write Register read also count;
typedef struct {
    ModbusRtuHoldingRegisterRequest req;
    uint8_t countBytes;
} ModbusRtuHoldingRegisterWriteRequest;

typedef struct {
    ModbusRtuPacketHeader header;
    ModbusRtuHoldingRegisterRequest address;
} ModbusRtuPacketResponse;

typedef struct {
    ModbusRtuPacketHeader header;
    uint8_t error;
} ModbusRtuPacketErrorResponse;

typedef struct {
    ModbusRtuPacketHeader header;
    uint8_t countBytes;
} ModbusRtuPacketReadResponse;

/**
 * The current station address. It is UNASSIGNED_STATION_ADDRESS (255) if the station still doesn't have an address (auto-configuration).
 */
uint8_t bus_cl_stationAddress;
uint8_t bus_crcErrors;
static uint8_t s_curRequestAddressL;

// If != NO_ERR, write an error
static uint8_t s_exceptionCode;
// Store the bytes remaining for function data streaming
static uint8_t s_sizeRemaining;
// The current function in use
static uint8_t s_function;

BUS_CL_RTU_STATE bus_cl_rtu_state;

void bus_cl_init() {
    // RS485 already in receive mode
    bus_cl_rtu_state = BUS_CL_RTU_IDLE;
    bus_crcErrors = 0;
}

// Called often
__bit bus_cl_poll() {
    if (rs485_isMarkCondition && bus_cl_rtu_state != BUS_CL_RTU_IDLE) {
        if (bus_cl_rtu_state != BUS_CL_RTU_WAIT_FOR_RESPONSE) {
            // Abort reading, go idle
            bus_cl_rtu_state = BUS_CL_RTU_IDLE;
            return false;
        } else {
            bus_cl_rtu_state = BUS_CL_RTU_RESPONSE;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_IDLE) {
        // Else decode the packet header
        if (rs485_readAvail() < sizeof(ModbusRtuPacketHeader)) {
            // Nothing to do, wait for more data
            return false;
        }
        // Free the buffer
        rs485_discard(sizeof(ModbusRtuPacketHeader));

#define packet_2 ((const ModbusRtuPacketHeader*)rs485_buffer)
        if (packet_2->address == bus_cl_stationAddress) {
            s_function = packet_2->function;
            if (s_function == READ_HOLDING_REGISTERS || s_function == WRITE_HOLDING_REGISTERS) {
                // The message is for reading registers. Address data will follow
                bus_cl_rtu_state = BUS_CL_RTU_WAIT_REGISTER_DATA;
            } else {
                // Invalid function, return error
                s_exceptionCode = ERR_INVALID_FUNCTION;
                bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
                return false;
            }
        } else {
            // No this station, wait for idle
            bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_IDLE;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_WAIT_REGISTER_DATA) {
        // In case of write, read the size too
        uint8_t messageSize = sizeof(ModbusRtuHoldingRegisterRequest);
        if (s_function == WRITE_HOLDING_REGISTERS) {
            messageSize = sizeof(ModbusRtuHoldingRegisterRequest) + 1;
        }
        if (rs485_readAvail() < messageSize) {
            // Nothing to do, wait for more data
            return false;
        }
        // Free the buffer
        rs485_discard(messageSize);

#define packet_1 ((const ModbusRtuHoldingRegisterWriteRequest*)rs485_buffer)
        
        // register address if the function id * 8
        // Max 32 functions, so 256 registers
        if (packet_1->req.registerAddressH != 0 || (packet_1->req.registerAddressL & 0x8) != 0 || (packet_1->req.registerAddressL >> 3) >= bus_cl_functionCount) {
            // Invalid address, return error
            s_exceptionCode = ERR_INVALID_ADDRESS;
            bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
            return false;
        }
        // Functions has fixed size of 16 bytes (8 registers) to save program space
        if (packet_1->req.countH != 0 || packet_1->req.countL != 8) {
            // Invalid size, return error
            s_exceptionCode = ERR_INVALID_SIZE;
            bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
            return false;
        }
        s_sizeRemaining = 16;
        s_curRequestAddressL = packet_1->req.registerAddressL;
        
        if (s_function == READ_HOLDING_REGISTERS) {
            // Ok, function data must be read. Wait for packet to end
            bus_cl_rtu_state = BUS_CL_RTU_CHECK_REQUEST_CRC;
        } else {
            if (packet_1->countBytes != s_sizeRemaining) {
                // Invalid size, return error
                s_exceptionCode = ERR_INVALID_SIZE;
                bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
                return false;
            } else {
                bus_cl_rtu_state = BUS_CL_RTU_READ_STREAM;
            }
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_READ_STREAM) {
        uint8_t avail = rs485_readAvail();
        uint8_t remaining = s_sizeRemaining;
        // A single call can be done to read the rest of the stream?
        if (s_sizeRemaining > RS485_BUF_SIZE) {
            remaining = RS485_BUF_SIZE;
        }

        if (avail < remaining) {
            return false;
        }
        // Free the buffer
        rs485_discard(remaining);
        bus_cl_functionWriteHandlers[s_curRequestAddressL >> 3]();
        s_sizeRemaining -= remaining;

        if (s_sizeRemaining > 0) {
            return false;
        } else {
            // Next state
            bus_cl_rtu_state = BUS_CL_RTU_CHECK_REQUEST_CRC;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_CHECK_REQUEST_CRC) {
        // CRC is LSB first
        uint16_t expectedCrc = le16toh(crc16);

        if (rs485_readAvail() < sizeof(uint16_t)) {
            // Nothing to do, wait for more data
            return false;
        }
        // Free the buffer
        rs485_discard(sizeof(uint16_t));

        bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
        if (expectedCrc != *((const uint16_t*)rs485_buffer)) {
            // Invalid CRC, skip data.
            // TODO: However the function data was already written if piped!
            bus_crcErrors++;
            bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_IDLE;
        } else {
            // Ok, go on with the response
            s_exceptionCode = NO_ERROR;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_IDLE) {
        rs485_discard(rs485_readAvail());
        return false;
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_RESPONSE) {
#define resp_1b ((ModbusRtuPacketHeader*)rs485_buffer)
        resp_1b->address = bus_cl_stationAddress;
        if (s_exceptionCode == NO_ERROR) {
            resp_1b->function = s_function;
            // Transmit packet data in one go
            if (s_function == READ_HOLDING_REGISTERS) {
#define resp_1r ((ModbusRtuPacketReadResponse*)rs485_buffer)
                // Now, if write, open stream
                resp_1r->countBytes = s_sizeRemaining;
                rs485_write(sizeof(ModbusRtuPacketReadResponse));
                bus_cl_rtu_state = BUS_CL_RTU_WRITE_STREAM;
            } else {
#define resp_1w ((ModbusRtuPacketResponse*)rs485_buffer)
                // Response of write registers always contains the address and register count
                resp_1w->address.registerAddressH = 0;
                resp_1w->address.registerAddressL = s_curRequestAddressL;
                resp_1w->address.countH = 0;
                resp_1w->address.countL = 8;
                rs485_write(sizeof(ModbusRtuPacketResponse));
                bus_cl_rtu_state = BUS_CL_RTU_WRITE_RESPONSE_CRC;
            }
        } else {
            // Transmit error data in one go
#define resp_2 ((ModbusRtuPacketErrorResponse*)rs485_buffer)
            resp_2->header.function = s_function | 0x80;
            resp_2->error = s_exceptionCode;
            rs485_write(sizeof(ModbusRtuPacketErrorResponse));
            bus_cl_rtu_state = BUS_CL_RTU_WRITE_RESPONSE_CRC;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_WRITE_STREAM) {
        if (rs485_writeInProgress()) {
            return false;
        }
        uint8_t remaining = s_sizeRemaining;
        // A single call can be done to read the rest of the stream?
        if (s_sizeRemaining > RS485_BUF_SIZE) {
            remaining = RS485_BUF_SIZE;
        }
        bus_cl_functionReadHandlers[s_curRequestAddressL >> 3]();
        rs485_write(remaining);
        s_sizeRemaining -= remaining;
        if (s_sizeRemaining > 0) {
            return false;
        } else {
            // Next state
            bus_cl_rtu_state = BUS_CL_RTU_WRITE_RESPONSE_CRC;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_WRITE_RESPONSE_CRC) {
        if (rs485_writeInProgress()) {
            return false;
        }
        // CRC is LSB first
        *((uint16_t*)rs485_buffer) = htole16(crc16);
        rs485_write(sizeof(uint16_t));
        bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_FLUSH;
        return false;
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH) {
        if (rs485_state == RS485_LINE_RX) {
            bus_cl_rtu_state = BUS_CL_RTU_IDLE;
        }
    }

    return false;
}
