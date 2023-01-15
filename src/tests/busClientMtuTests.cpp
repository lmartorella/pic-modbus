#include <catch2/catch.hpp>
#include <stdbool.h>
#include <queue>

#include "net/bus_client.h"
#include "net/rs485.h"
#include "net/sinks.h"

using namespace std::string_literals;

std::queue<uint8_t> txQueue;
std::queue<uint8_t> rxQueue;

extern "C" {
    const uint16_t SINK_IDS_COUNT = 3;
    const uint8_t sink_readSizes[SINK_IDS_COUNT] = { 2, 0, 8 };
    const uint8_t sink_writeSizes[SINK_IDS_COUNT] = { 0, 4, 8 };

    RS485_LINE_STATE rs485_state;
    bool rs485_isMarkCondition;

    void rs485_write(const void* buffer, uint8_t size) {
        throw std::runtime_error("TODO");
    }

    bool rs485_read(void* buffer, uint8_t size) {
        if (size > rxQueue.size()) {
            return false;
        }
        uint8_t* di = reinterpret_cast<uint8_t*>(buffer);
        for (int i = 0; i < size; i++, di++) {
            *di = rxQueue.front();
            rxQueue.pop();
        }
        return true;
    }
}

static void initRs485() {
    rs485_isMarkCondition = true;
    rs485_state = RS485_LINE_RX;
}

/**
 * Each packet is distant enough from other packets to 
 */
static void simulateData(const std::vector<uint16_t>& data) {
    if (data.size() > 0) {
        for (auto it = data.begin(); it != data.end(); ++it) {
            rxQueue.push(*it);
        }
        rs485_isMarkCondition = false;
    } else {
        if (rxQueue.size() > 0) {
            throw std::runtime_error("Can't set mark state when pending data is in the buffer");
        }
        rs485_isMarkCondition = true;
    }
}

TEST_CASE("No addressing, skip packet") {
    initRs485();
    bus_cl_init();
    bus_cl_stationAddress = 2;

    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    // Not addressed
    simulateData({ 0x1 });
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    simulateData({ 0x2 });
    REQUIRE(bus_cl_poll() == false);
    // After 2 bytes read, the client will start skipping data
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_IDLE);

    simulateData({ 0x3, 0x5 });
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_IDLE);

    simulateData({ });
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}
