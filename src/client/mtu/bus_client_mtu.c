#include "net/appio.h"
#include "net/bus_client.h"
#include "net/leds.h"
#include "net/persistence.h"
#include "net/protocol.h"
#include "net/rs485.h"

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
        s_stationAddress = pers_data.sec.address = UNASSIGNED_SUB_ADDRESS;       
        pers_save();
    }

    if (s_stationAddress == UNASSIGNED_SUB_ADDRESS) {
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

    return false;
}
