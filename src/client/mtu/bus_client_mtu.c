#include "net/net.h"

/**
 * Specific module for client Modbus RTU nodes (RS485), optimized
 * for 8-bit CPUs with low memory.
 */

static enum {
    // The client bus is idle, waiting for a frame request
    STATE_IDLE,
    // Request frame header decoding
    STATE_REQ_HEADER,
    // Request's frame header decoded and the current node was addressed: reading function type and register address 
    STATE_REQ_FUNCTION_ADDRESS,
    // Request: function type and register address valid, now reading input data stream and piping data to the sink. 
    STATE_PIPE_READ,
    // Request: function type and register address valid, wait for response to start (wait for line direction to change)
    STATE_WAIT_RESPONSE,
    // Writing response header
    STATE_RESP_HEADER,
    // In response body: writing output data stream
    STATE_PIPE_WRITE,
    // Writing response checksum
    STATE_RESP_CHECKSUM,
    // Read mode: skipping input data and waiting for the next idle state.
    STATE_SKIP_DATA
} s_state;

/**
 * The current station address. It is 0 if the station still doesn't have an address (auto-configuration).
 */
static uint8_t s_stationAddress;

/**
 * Was the node be acknowledged by the server? (auto-configuration)
 */
static __bit s_acknowledged;

void bus_cl_init() {
    // Prepare address
    s_stationAddress = pers_data.sec.address;
    s_acknowledged = false;

    // Address should be reset?
    if (g_resetReason == RESET_MCLR
#ifdef _IS_ETH_CARD
            // Bug of HW spec of PIC18?
            || g_resetReason == RESET_POWER
#endif
    ) {
        // Reset address
        pers_data.sec.address = UNASSIGNED_STATION_ADDRESS;       
        pers_save();
        s_availForAddressAssign = true;
    }

    if (s_stationAddress == UNASSIGNED_STATION_ADDRESS) {
        // Signal unattended secondary client, but doesn't auto-assign to avoid line clash at multiple boot
        led_on();
    }

    // RS485 already in receive mode
    s_state = STATE_SKIP_DATA;
}

static void storeAddress(uint8_t address) {
    pers_data.sec.address = s_stationAddress;
    pers_save();
    led_off();
}

// Called often
__bit bus_cl_poll() {
    if (s_state == STATE_WAIT_RESPONSE) {
        // Line ready for transmission?
        if (rs485_state == RS485_LINE_TX) {
            s_state = STATE_RESP_HEADER;
        }
    }
    uint8_t readSize = rs485_readAvail();
    if (readSize == 0) {
        // Nothing to do
        return s_state == STATE_IDLE;
    }
    switch (s_state) {
        case 
            // Header decode
            if (rs485_readAvail() > 0) {            

                // RC9 will be 1
                rs485_read(&buf, 1);
                // Waiting for header?
                if (s_state < STATE_HEADER_3) {
                    // Keep an eye to address if in assign state
                    if (s_state == STATE_HEADER_ADDRESS && s_availForAddressAssign) {
                        // Store the byte, in case of s_skipNextAddressCheck
                        s_tempAddressForAssignment = buf;
                        // Go ahead
                        s_state++;
                    }
                    else if (buf == s_header[s_state]) {
                        // Header matches. Go next.
                        s_state++;
                    }
                    else {
                        // Not my address, or protocol error. Restart from 0x55 but don't try to skip glitches
                        reinit_after_disengage();
                    }
                }
                else {
                    _Bool isManaged = false;
                    // Header correct. Now read the command and respond
                    switch (buf) { 
                        case BUS_MSG_TYPE_ADDRESS_ASSIGN:
                            if (s_availForAddressAssign) {
                                // Master now knows me
                                s_acknowledged = 1;
                                // Store the new address in memory
                                storeAddress();
                                sendAck(BUS_ACK_TYPE_HEARTBEAT);
                                isManaged = true;
                            }
                            break;

                        case BUS_MSG_TYPE_HEARTBEAT:
                            // Only respond to heartbeat if has address
                            if (s_stationAddress != UNASSIGNED_STATION_ADDRESS) {
                                sendAck(s_acknowledged ? 
                                    (g_resetReason == RESET_NONE ? BUS_ACK_TYPE_HEARTBEAT : BUS_ACK_TYPE_READ_STATUS) 
                                        : BUS_ACK_TYPE_HELLO);
                                isManaged = true;
                            }
                            break;

                        case BUS_MSG_TYPE_READY_FOR_HELLO:
                            // Only respond to hello if ready to program
                            if (s_availForAddressAssign) {
                                sendAck(BUS_ACK_TYPE_HELLO);
                                isManaged = true;
                            }
                            break;

                        case BUS_MSG_TYPE_CONNECT:
                            if (s_stationAddress != UNASSIGNED_STATION_ADDRESS) {
                                // Master now knows me
                                s_acknowledged = 1;
                                // Start reading data with rc9 not set
                                rs485_skipData = rs485_lastRc9 = false;
                                // Socket, direct connect
                                s_state = STATE_SOCKET_OPEN;
                                isManaged = true;
                            }
                            break;
                        default:
                            // Unknown command. Reset
                            // Restart from 0x55
                            reinit_after_disengage();
                            break;
                    }
                
                    if (!isManaged) {
                        // If not managed, reinit bus for the next message
                        reinit_after_disengage();
                    }
                }
            }
    }

    return s_state != STATE_HEADER_0;
}
