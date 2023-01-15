#include "assert.h"
#include "net/bus_client.h"
#include "net/sinks.h"
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

typedef enum {
    NO_ERROR = 0,
    ERR_INVALID_FUNCTION = 1,
    ERR_INVALID_ADDRESS = 2,
    ERR_INVALID_SIZE = 3
} EXCEPTION_CODES;

/**
 * The current station address. It is UNASSIGNED_STATION_ADDRESS (255) if the station still doesn't have an address (auto-configuration).
 */
uint8_t bus_cl_stationAddress;

// If != NO_ERR, write an error
static uint8_t s_exceptionCode;
// The current calculated CRC
static uint16_t s_crc;
// Store the reg count (low byte) of the last command
static uint8_t s_regCount;
// The current function in use
static uint8_t s_function;
// The current sink ID addressed
static uint8_t s_currentSink;

BUS_CL_RTU_STATE bus_cl_rtu_state;

void bus_cl_init() {
    // RS485 already in receive mode
    bus_cl_rtu_state = BUS_CL_RTU_IDLE;
}

// Called often
__bit bus_cl_poll() {
    if (rs485_isMarkCondition) {
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
            if (packet.function == READ_HOLDING_REGISTERS || packet.function == WRITE_HOLDING_REGISTERS) {
                // The message is for reading registers. Address data will follow
                bus_cl_rtu_state = BUS_CL_RTU_WAIT_REGISTER_DATA;
                s_function = packet.function;
            } else {
                // Invalid function, return error
                s_exceptionCode = ERR_INVALID_FUNCTION;
                bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
                return false;
            }
        } else {
            // No this station, wait for idle
            bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_IDLE;
            return false;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_WAIT_REGISTER_DATA) {
        ModbusRtuHoldingRegisterRequest packet;
        if (!rs485_read(&packet, sizeof(ModbusRtuHoldingRegisterRequest))) {
            // Nothing to do, wait for more data
            return false;
        }
        
        // register address if the sink id * 256
        if (packet.registerAddressL != 0 || packet.registerAddressH > SINK_IDS_COUNT) {
            // Invalid address, return error
            s_exceptionCode = ERR_INVALID_ADDRESS;
            bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
            return false;
        }
        s_currentSink = packet.registerAddressL;
        uint8_t s_regCount = ((s_function == READ_HOLDING_REGISTERS) ? sink_readSizes[s_currentSink] : sink_writeSizes[s_currentSink]) / 2;
        if (packet.countH != 0 || packet.countL != s_regCount) {
            // Invalid size, return error
            s_exceptionCode = ERR_INVALID_SIZE;
            bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
            return false;
        }

        if (s_function == READ_HOLDING_REGISTERS) {
            // Ok, sink data must be read. Wait for packet to end
            bus_cl_rtu_state = BUS_CL_RTU_CHECK_REQUEST_CRC;
        } else {
            bus_cl_rtu_state = BUS_CL_RTU_WRITE_STREAM;
            // Waits for the sink to call bus_cl_closeStream()
            return false;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_CHECK_REQUEST_CRC) {
        uint16_t crc;
        if (!rs485_read(&crc, sizeof(uint16_t))) {
            // Nothing to do, wait for more data
            return false;
        }
        bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
        if (crc != s_crc) {
            // Invalid CRC, skip data.
            // TODO: However the sink data was already written if piped!
            bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_IDLE;
            return false;
        } else {
            // Ok, go on with the response
            s_exceptionCode = NO_ERROR;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_RESPONSE) {
        if (s_exceptionCode != NO_ERROR) {
            // Transmit packet data in one go
            ModbusRtuPacketHeader header;
            header.address = bus_cl_stationAddress;
            header.function = s_function;
            rs485_write(&header, sizeof(ModbusRtuPacketHeader));
            // Response of read/write registers always contains the address and register count
            ModbusRtuHoldingRegisterRequest sizes;
            sizes.registerAddressH = s_currentSink;
            sizes.registerAddressL = 0;
            sizes.countH = 0;
            sizes.countL = s_regCount;
            rs485_write(&sizes, sizeof(ModbusRtuHoldingRegisterRequest));
            // Now, if write, open stream
            if (s_function == READ_HOLDING_REGISTERS) {
                uint8_t size = s_regCount * 2;
                rs485_write(&size, 1);
                bus_cl_rtu_state = BUS_CL_RTU_WRITE_STREAM;
                // Waits for the sink to call bus_cl_closeStream()
                return false;
            } else {
                rs485_write(&s_crc, sizeof(uint16_t));
                bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_FLUSH;
                return false;
            }
        } else {
            // Transmit error data in one go
            ModbusRtuPacketHeader header;
            header.address = bus_cl_stationAddress;
            header.function = s_function | 0x80;
            rs485_write(&header, sizeof(ModbusRtuPacketHeader));
            rs485_write(&s_exceptionCode, sizeof(uint8_t));
            rs485_write(&s_crc, sizeof(uint16_t));
            bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_FLUSH;
            return false;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_WRITE_RESPONSE_CRC) {
        rs485_write(&s_crc, sizeof(uint16_t));
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
