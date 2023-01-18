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
        rs485_isMarkCondition = false;
        rs485_state = RS485_LINE_TX;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(buffer);
        for (auto i = 0; i < size; ++i) {
            txQueue.push(data[i]);
        }
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

    void rs485_discard() {
        while (!rxQueue.empty()) {
            rxQueue.pop();
        }
    }
}

static void initRs485() {
    rs485_isMarkCondition = true;
    rs485_state = RS485_LINE_RX;
    rs485_discard();
}

/**
 * Each packet is distant enough from other packets to 
 */
static void simulateData(const std::vector<uint8_t>& data) {
    if (data.size() > 0) {
        for (auto it = data.begin(); it != data.end(); ++it) {
            rxQueue.push(*it);
        }
        rs485_isMarkCondition = false;
    } else {
        rs485_isMarkCondition = true;
    }
}

static void simulateMark() {
    simulateData({ });
}

static std::vector<uint8_t> simulateRead() {
    std::vector<uint8_t> ret;
    while (txQueue.size() > 0) {
        ret.push_back(txQueue.front());
        txQueue.pop();
    }
    return ret;
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

    simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}

TEST_CASE("Addressed but truncated packet") {
    initRs485();
    bus_cl_init();
    bus_cl_stationAddress = 2;

    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    // Addressed and correct function
    simulateData({ 0x2, 0x10 });
    REQUIRE(bus_cl_poll() == false);
    // After 2 bytes read, the client will start skipping data
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_REGISTER_DATA);

    simulateData({ 0x3 });
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_REGISTER_DATA);

    // Mark condition, reset state
    simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    REQUIRE(simulateRead() == std::vector<uint8_t>({ }));
}

TEST_CASE("Wrong function") {
    initRs485();
    bus_cl_init();
    bus_cl_stationAddress = 2;

    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    // Addressed and incorrect function
    simulateData({ 0x2, 0x11 });
    REQUIRE(bus_cl_poll() == false);
    // After 2 bytes read, the client will start skipping data
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_RESPONSE);

    simulateData({ 0x3 });
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_RESPONSE);

    // Mark condition, now the station will respond
    simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    REQUIRE(simulateRead() == std::vector<uint8_t>({ 0x2, 0x91, 0x1, 0xff, 0xff }));

    rs485_state = RS485_LINE_RX;
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE); 
}

static void testWrongAddress(uint8_t addressH, uint8_t addressL) {
    initRs485();
    bus_cl_init();
    bus_cl_stationAddress = 2;

    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    // Addressed and correct function
    simulateData({ 0x2, 0x10 });
    REQUIRE(bus_cl_poll() == false);
    // After 2 bytes read, the client will wait for address
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_REGISTER_DATA);

    simulateData({ addressH, addressL, 0, 1 });
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_RESPONSE);

    // Mark condition, now the station will respond
    simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    // Function error
    REQUIRE(simulateRead() == std::vector<uint8_t>({ 0x2, 0x90, 0x2, 0xff, 0xff }));

    rs485_state = RS485_LINE_RX;
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE); 
}

TEST_CASE("Wrong address: low value") {
    testWrongAddress(0, 1);
}
TEST_CASE("Wrong address, low value -1") {
    testWrongAddress(0, 0xff);
}
TEST_CASE("Wrong address, high value > sink") {
    testWrongAddress(3, 0x0);
}
TEST_CASE("Wrong address, high value -1") {
    testWrongAddress(0xff, 0x0);
}

static void testSizeSetup(uint8_t sinkId, uint8_t sizeH, uint8_t sizeL, uint8_t function) {
    initRs485();
    bus_cl_init();
    bus_cl_stationAddress = 2;

    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    // Addressed and correct function
    simulateData({ 0x2, function });
    REQUIRE(bus_cl_poll() == false);
    // After 2 bytes read, the client will wait for address
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_REGISTER_DATA);

    simulateData({ sinkId, 0x0, sizeH, sizeL });
    REQUIRE(bus_cl_poll() == false);
}

static void testWrongSize(uint8_t sinkId, uint8_t sizeH, uint8_t sizeL) {
    testSizeSetup(sinkId, sizeH, sizeL, 0x10);

    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_RESPONSE);

    // Mark condition, now the station will respond
    simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    // Function size error
    REQUIRE(simulateRead() == std::vector<uint8_t>({ 0x2, 0x90, 0x3, 0xff, 0xff }));

    rs485_state = RS485_LINE_RX;
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}

TEST_CASE("Wrong size: high value") {
    testWrongSize(0, 1, 0);
}
TEST_CASE("Wrong size: high value, 2") {
    testWrongSize(0, 2, 0);
}
TEST_CASE("Wrong size: low value, 0") {
    testWrongSize(1, 0, 0);
}
TEST_CASE("Wrong size: low value, 1") {
    testWrongSize(1, 0, 1);
}
TEST_CASE("Wrong size: low value, 0xff") {
    testWrongSize(1, 0, 0xff);
}

static void testCorrectRead(uint8_t sinkId, uint8_t sizeL) {
    testSizeSetup(sinkId, 0, sizeL, 0x3);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_CHECK_REQUEST_CRC);

    simulateData({ 0xff, 0xff }); // Fake CRC
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_RESPONSE);

    simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WRITE_STREAM);
    // Read response
    REQUIRE(simulateRead() == std::vector<uint8_t>({ 0x2, 0x3, sinkId, 0x0, 0x0, sizeL, (uint8_t)(sizeL * 2) }));
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WRITE_STREAM);

    bus_cl_closeStream();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(simulateRead() == std::vector<uint8_t>({ 0xff, 0xff }));
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    rs485_state = RS485_LINE_RX;
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}

static void testCorrectWrite(uint8_t sinkId, uint8_t sizeL) {
    testSizeSetup(sinkId, 0, sizeL, 0x10);

    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_READ_STREAM);
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_READ_STREAM);

    bus_cl_closeStream();

    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_CHECK_REQUEST_CRC);

    simulateData({ 0xff, 0xff }); // Fake CRC
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_RESPONSE);

    simulateMark();
    REQUIRE(bus_cl_poll() == false);

    // Read empty response
    REQUIRE(simulateRead() == std::vector<uint8_t>({ 0x2, 0x10, sinkId, 0x0, 0x0, sizeL, 0xff, 0xff }));
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    rs485_state = RS485_LINE_RX;
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}

TEST_CASE("Correct size, sink 0, read") {
    testCorrectRead(0, 1);
}
TEST_CASE("Correct size, sink 0, write") {
    testCorrectWrite(0, 0);
}
TEST_CASE("Correct size, sink 1, read") {
    testCorrectRead(1, 0);
}
TEST_CASE("Correct size, sink 1, write") {
    testCorrectWrite(1, 2);
}
TEST_CASE("Correct size, sink 3, read") {
    testCorrectRead(2, 4);
}
TEST_CASE("Correct size, sink 3, write") {
    testCorrectWrite(2, 4);
}
