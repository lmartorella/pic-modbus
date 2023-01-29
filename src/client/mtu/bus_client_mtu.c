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

/**
 * The current station address. It is UNASSIGNED_STATION_ADDRESS (255) if the station still doesn't have an address (auto-configuration).
 */
uint8_t bus_cl_stationAddress;

// If != NO_ERR, write an error
static uint8_t s_exceptionCode;
// Store the total reg count (low byte) of the last command
static uint8_t s_currentSize;
// Store the bytes remaining for function data streaming
static uint8_t s_sizeRemaining;
// The current function in use
static uint8_t s_function;
// The current sink ID addressed
static uint8_t s_currentAddrL, s_currentAddrH;

BUS_CL_RTU_STATE bus_cl_rtu_state;

#define INITIAL_CRC_VALUE (0xffff)

void bus_cl_init() {
    // RS485 already in receive mode
    bus_cl_rtu_state = BUS_CL_RTU_IDLE;
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
        ModbusRtuPacketHeader packet;
        if (!rs485_read(&packet, sizeof(ModbusRtuPacketHeader))) {
            // Nothing to do, wait for more data
            return false;
        }
        if (packet.address == bus_cl_stationAddress) {
            s_function = packet.function;
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
        ModbusRtuHoldingRegisterWriteRequest packet;
        // In case of write, read the size too
        uint8_t size = s_function == READ_HOLDING_REGISTERS ? sizeof(ModbusRtuHoldingRegisterRequest) : sizeof(ModbusRtuHoldingRegisterRequest) + 1;
        if (!rs485_read(&packet, size)) {
            // Nothing to do, wait for more data
            return false;
        }
        
        // register address if the sink id * 256
        s_currentAddrL = packet.req.registerAddressL;
        s_currentAddrH = packet.req.registerAddressH;
        s_currentSize = packet.req.countL;
        if (s_currentAddrH == 0) {
            if ((s_currentAddrL & 0xf) != 0 || (s_currentAddrL >> 4) >= bus_cl_sysFunctionCount) {
                // Invalid address, return error
                s_exceptionCode = ERR_INVALID_ADDRESS;
                bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
                return false;
            }
            // Sys functions has fixed size to save program space
            s_sizeRemaining = 16;
        } else {
            if (s_currentAddrL != 0 || (s_currentAddrH - 1) >= bus_cl_appFunctionCount) {
                // Invalid address, return error
                s_exceptionCode = ERR_INVALID_ADDRESS;
                bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
                return false;
            }
            s_sizeRemaining = (s_function == READ_HOLDING_REGISTERS) ? bus_cl_appFunctions[s_currentAddrH - 1].readSize : bus_cl_appFunctions[s_currentAddrH - 1].writeSize;
        }
        if (packet.req.countH != 0 || packet.req.countL != s_sizeRemaining / 2 || s_sizeRemaining == 0) {
            // Invalid size, return error
            s_exceptionCode = ERR_INVALID_SIZE;
            bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
            return false;
        }

        if (s_function == READ_HOLDING_REGISTERS) {
            // Ok, sink data must be read. Wait for packet to end
            bus_cl_rtu_state = BUS_CL_RTU_CHECK_REQUEST_CRC;
        } else {
            if (packet.countBytes != s_sizeRemaining) {
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
        uint8_t buf[STREAM_BUFFER_SIZE];
        uint8_t remaining = s_sizeRemaining;
        // A single call can be done to read the rest of the stream?
        if (s_sizeRemaining > STREAM_BUFFER_SIZE) {
            remaining = STREAM_BUFFER_SIZE;
        }

        if (avail < remaining) {
            return false;
        }
        rs485_read(buf, remaining);
        const FunctionDefinition* def = (s_currentAddrH == 0) ? &bus_cl_sysFunctions[s_currentAddrL >> 4] : &bus_cl_appFunctions[s_currentAddrH - 1].def;
        def->onWrite(buf);
        s_sizeRemaining -= remaining;

        if (s_sizeRemaining > 0) {
            return false;
        } else {
            // Next state
            bus_cl_rtu_state = BUS_CL_RTU_CHECK_REQUEST_CRC;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_CHECK_REQUEST_CRC) {
        uint16_t readCrc;
        // CRC is LSB first
        uint16_t expectedCrc = le16toh(crc16);

        if (!rs485_read(&readCrc, sizeof(uint16_t))) {
            // Nothing to do, wait for more data
            return false;
        }

        bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
        if (expectedCrc != readCrc) {
            // Invalid CRC, skip data.
            // TODO: However the sink data was already written if piped!
            bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_IDLE;
        } else {
            // Ok, go on with the response
            s_exceptionCode = NO_ERROR;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_IDLE) {
        rs485_discard();
        return false;
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_RESPONSE) {
        if (s_exceptionCode == NO_ERROR) {
            // Transmit packet data in one go
            ModbusRtuPacketHeader header;
            header.address = bus_cl_stationAddress;
            header.function = s_function;
            rs485_write(&header, sizeof(ModbusRtuPacketHeader));
            // Response of read/write registers always contains the address and register count
            ModbusRtuHoldingRegisterRequest sizes;
            sizes.registerAddressH = s_currentAddrH;
            sizes.registerAddressL = s_currentAddrL;
            sizes.countH = 0;
            sizes.countL = s_currentSize;
            rs485_write(&sizes, sizeof(ModbusRtuHoldingRegisterRequest));
            // Now, if write, open stream
            if (s_function == READ_HOLDING_REGISTERS) {
                uint8_t size = s_sizeRemaining;
                rs485_write(&size, 1);
                bus_cl_rtu_state = BUS_CL_RTU_WRITE_STREAM;
            } else {
                bus_cl_rtu_state = BUS_CL_RTU_WRITE_RESPONSE_CRC;
            }
        } else {
            // Transmit error data in one go
            ModbusRtuPacketHeader header;
            header.address = bus_cl_stationAddress;
            header.function = s_function | 0x80;
            rs485_write(&header, sizeof(ModbusRtuPacketHeader));
            rs485_write(&s_exceptionCode, sizeof(uint8_t));
            bus_cl_rtu_state = BUS_CL_RTU_WRITE_RESPONSE_CRC;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_WRITE_STREAM) {
        uint8_t avail = rs485_writeAvail();
        uint8_t buf[STREAM_BUFFER_SIZE];
        uint8_t remaining = s_sizeRemaining;
        // A single call can be done to read the rest of the stream?
        if (s_sizeRemaining > STREAM_BUFFER_SIZE) {
            remaining = STREAM_BUFFER_SIZE;
        }
        if (avail < remaining) {
            return false;
        }
        const FunctionDefinition* def = (s_currentAddrH == 0) ? &bus_cl_sysFunctions[s_currentAddrL >> 4] : &bus_cl_appFunctions[s_currentAddrH - 1].def;
        def->onRead(buf);
        rs485_write(buf, remaining);
        s_sizeRemaining -= remaining;
        if (s_sizeRemaining > 0) {
            return false;
        } else {
            // Next state
            bus_cl_rtu_state = BUS_CL_RTU_WRITE_RESPONSE_CRC;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_WRITE_RESPONSE_CRC) {
        // CRC is LSB first
        uint16_t crc = htole16(crc16);
        rs485_write(&crc, sizeof(uint16_t));
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
