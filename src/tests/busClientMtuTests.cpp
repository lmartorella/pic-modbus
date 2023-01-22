#include <catch2/catch.hpp>
#include <stdbool.h>
#include <queue>

#include "net/bus_client.h"
#include "net/crc.h"
#include "net/rs485.h"

using namespace std::string_literals;

std::queue<uint8_t> txQueue;
std::queue<uint8_t> rxQueue;

static std::vector<uint8_t> operator+ (const std::vector<uint8_t>& vec1, const std::vector<uint8_t>& vec2) {
    std::vector<uint8_t> ret(vec1);
    ret.insert(ret.end(), vec2.begin(), vec2.end());
    return ret;
}

// In crc16.cpp
extern uint16_t calcCrc16(uint16_t prevCrc, const uint8_t* buffer, int wLength);

/**
 * The function handler that produces the function response data to send to the server
 * during a read call. The buffer size is `readSize`.
 */
void onFunctionRead(int functionId, void* buffer, int size);

/**
 * The function handler that consumes the function data sent by the server
 * during a write call. The buffer size is `writeSize`.
 */
void onFunctionWrite(int functionId, const void* buffer, int size);

template<typename T>
void functionError(T param) {
    throw std::runtime_error("It should not be called");
}

extern "C" {
    const uint8_t bus_cl_function_count = 3;
    const FunctionDefinition bus_cl_functions[bus_cl_function_count] = {
        {
            .id = { .str = "FC1"},
            .onRead = [](void* buf) { onFunctionRead(0, buf, 2); },
            .readSize = 2,
            .onWrite = functionError,
            .writeSize = 0,
        },
        {
            .id = { .str = "FC2" },
            .onRead = functionError,
            .readSize = 0,
            .onWrite = [](const void* buf) { onFunctionWrite(1, buf, 4); },
            .writeSize = 4,
        },
        {
            .id = { .str = "FC3" },
            .onRead = [](void* buf) { onFunctionRead(2, buf, 8); },
            .readSize = 8,
            .onWrite = [](const void* buf) { onFunctionWrite(2, buf, 8); },
            .writeSize = 8,
        },
    };

    RS485_LINE_STATE rs485_state;
    bool rs485_isMarkCondition;

    uint8_t rs485_writeAvail() {
        return 255; // Infinite
    }

    void rs485_write(const void* buffer, uint8_t size) {
        if (rs485_state != RS485_LINE_TX) {
            crc_reset();
        }
        rs485_isMarkCondition = false;
        rs485_state = RS485_LINE_TX;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(buffer);
        for (auto i = 0; i < size; ++i) {
            txQueue.push(data[i]);
            crc_update(data[i]);
        }
    }

    uint8_t rs485_readAvail() {
        return (uint8_t)rxQueue.size();
    }

    bool rs485_read(void* buffer, uint8_t size) {
        if (rs485_state != RS485_LINE_RX) {
            crc_reset();
        }
        rs485_state = RS485_LINE_RX;
        if (size > rxQueue.size()) {
            return false;
        }
        uint8_t* di = reinterpret_cast<uint8_t*>(buffer);
        for (int i = 0; i < size; i++, di++) {
            *di = rxQueue.front();
            crc_update(*di);
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

static int s_expectedFuncIdInRead;
static int s_functionIdInWrite;
static std::vector<uint8_t> s_functionBufferToSend;
static std::vector<uint8_t> s_functionBufferReceived;

static void initRs485() {
    rs485_isMarkCondition = true;
    rs485_state = RS485_LINE_RX;
    rs485_discard();
    crc_reset();

    s_expectedFuncIdInRead = -1;
    s_functionIdInWrite = -1;
    s_functionBufferToSend.clear();
    s_functionBufferReceived.clear();
}

void prepareFunctionDataToSend(int funcId, const std::vector<uint8_t>& data) {
    REQUIRE(s_expectedFuncIdInRead == -1);
    REQUIRE(s_functionBufferToSend.size() == 0);

    s_expectedFuncIdInRead = funcId;
    s_functionBufferToSend = data;
}

/**
 * The function handler that produces the function response data to send to the server
 * during a read call. The buffer size is `readSize`.
 */
void onFunctionRead(int functionId, void* buffer, int size) {
    REQUIRE(functionId == s_expectedFuncIdInRead);
    REQUIRE(size == s_functionBufferToSend.size());
    uint8_t* di = reinterpret_cast<uint8_t*>(buffer);
    for (int i = 0; i < size; i++, di++) {
        *di = s_functionBufferToSend[i];
    }

    // Done
    s_expectedFuncIdInRead = -1;
    s_functionBufferToSend.clear();
}

/**
 * The function handler that consumes the function data sent by the server
 * during a write call. The buffer size is `writeSize`.
 */
void onFunctionWrite(int functionId, const void* buffer, int size) {
    REQUIRE(s_functionIdInWrite == -1);
    REQUIRE(s_functionBufferReceived.size() == 0);
    s_functionBufferReceived.resize(size);

    s_functionIdInWrite = functionId;
    const uint8_t* si = reinterpret_cast<const uint8_t*>(buffer);
    for (int i = 0; i < size; i++, si++) {
        s_functionBufferReceived[i] = *si;
    }
}

void checkFunctionDataReceived(int funcId, const std::vector<uint8_t>& data) {
    REQUIRE(s_functionIdInWrite == funcId);
    REQUIRE(s_functionBufferReceived == data);
}

// For CRC calculation
static std::vector<uint8_t> packetData;

/**
 * Each packet is distant enough from other packets to 
 */
static void simulateData(const std::vector<uint8_t>& data) {
    if (data.size() > 0) {
        for (auto it = data.begin(); it != data.end(); ++it) {
            rxQueue.push(*it);
            packetData.push_back(*it);
        }
        rs485_isMarkCondition = false;
    } else {
        rs485_isMarkCondition = true;
        crc_reset();
        packetData.clear();
    }
}

static void simulateMark() {
    simulateData({ });
}

static std::vector<uint8_t> crcOf(const std::vector<uint8_t>& data) {
    uint16_t crc = calcCrc16(0xffff, &data[0], data.size());
    // CRC is LSB first
    return std::vector<uint8_t>({ (uint8_t)(crc & 0xff), (uint8_t)(crc >> 8) });
}
static std::vector<uint8_t> crcOf(const std::vector<uint8_t>& data1, const std::vector<uint8_t>& data2) {
    uint16_t crc = calcCrc16(0xffff, &data1[0], data1.size());
    crc = calcCrc16(crc, &data2[0], data2.size());
    // CRC is LSB first
    return std::vector<uint8_t>({ (uint8_t)(crc & 0xff), (uint8_t)(crc >> 8) });
}

static std::vector<uint8_t> padWithCrc(const std::vector<uint8_t>& data) {
    return data + crcOf(data);
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

    REQUIRE(simulateRead() == padWithCrc({ 0x2, 0x91, 0x1 }));

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
    simulateData({ 0x2, 0x3 });
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
    REQUIRE(simulateRead() == padWithCrc({ 0x2, 0x83, 0x2 }));

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

static void testWrongSizeRead(uint8_t sinkId, uint8_t sizeH, uint8_t sizeL) {
    testSizeSetup(sinkId, sizeH, sizeL, 0x03);

    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_RESPONSE);

    // Mark condition, now the station will respond
    simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    // Function size error
    REQUIRE(simulateRead() == padWithCrc({ 0x2, 0x83, 0x3 }));

    rs485_state = RS485_LINE_RX;
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}

static void testWrongSizeWrite(uint8_t sinkId, uint8_t sizeH, uint8_t sizeL, uint8_t sizeBytes) {
    testSizeSetup(sinkId, sizeH, sizeL, 0x10);
    simulateData({ sizeBytes });
    REQUIRE(bus_cl_poll() == false);

    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_RESPONSE);

    // Mark condition, now the station will respond
    simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    // Function size error
    REQUIRE(simulateRead() == padWithCrc({ 0x2, 0x90, 0x3 }));

    rs485_state = RS485_LINE_RX;
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}

TEST_CASE("Wrong size read: high value") {
    testWrongSizeRead(0, 1, 0);
}
TEST_CASE("Wrong size read: high value, 2") {
    testWrongSizeRead(0, 2, 0);
}
TEST_CASE("Wrong size read: low value, 0") {
    testWrongSizeRead(0, 0, 0);
}
TEST_CASE("Wrong size read: low value, 2 (sink wants 1)") {
    testWrongSizeRead(0, 0, 2);
}
TEST_CASE("Wrong size read: low value, 1 on sink 1") {
    testWrongSizeRead(1, 0, 1);
}
TEST_CASE("Wrong size read: low value, 0xff") {
    testWrongSizeRead(0, 0, 0xff);
}
TEST_CASE("Wrong size read: write-only register with correct size of 0") {
    testWrongSizeRead(1, 0, 0);
}

TEST_CASE("Wrong size write: high value") {
    testWrongSizeWrite(0, 1, 0, 1);
}
TEST_CASE("Wrong size write: high value, 2") {
    testWrongSizeWrite(0, 2, 0, 2);
}
TEST_CASE("Wrong size write: low value, 0") {
    testWrongSizeWrite(1, 0, 0, 0);
}
TEST_CASE("Wrong size write: low value, 1") {
    testWrongSizeWrite(1, 0, 1, 2);
}
TEST_CASE("Wrong size write: low value, 0xff") {
    testWrongSizeWrite(1, 0, 0xff, 0xff);
}
TEST_CASE("Wrong size write: low value wight but not in bytes") {
    testWrongSizeWrite(1, 0, 2, 5);
}
TEST_CASE("Wrong size read: read-only register with correct size of 0") {
    testWrongSizeRead(0, 0, 0);
}

TEST_CASE("Wrong CRC in read") {
    testSizeSetup(0, 0, 1, 0x3);

    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_CHECK_REQUEST_CRC);
    simulateData({ 0xde, 0xad });
    REQUIRE(bus_cl_poll() == false);

    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_IDLE);
    // Mark condition, now the station will NOT respond due to CRC error
    simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}

TEST_CASE("Wrong CRC in write") {
    testSizeSetup(1, 0, 2, 0x10); // write sink 1
    simulateData({ 4 });
    REQUIRE(bus_cl_poll() == false);

    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_READ_STREAM);
    simulateData({ 0xf1, 0xf2, 0xf3, 0xf4 });
    REQUIRE(bus_cl_poll() == false);
    checkFunctionDataReceived(1, { 0xf1, 0xf2, 0xf3, 0xf4 });

    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_CHECK_REQUEST_CRC);
    simulateData({ 0xde, 0xad });
    REQUIRE(bus_cl_poll() == false);

    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_IDLE);
    // Mark condition, now the station will NOT respond due to CRC error
    simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}

static void testCorrectRead(uint8_t sinkId, uint8_t sizeL) {
    testSizeSetup(sinkId, 0, sizeL, 0x3);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_CHECK_REQUEST_CRC);

    simulateData(crcOf(packetData));
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_RESPONSE);

    simulateMark();

    std::vector<uint8_t> dataToSend;
    for (int i = 0; i < sizeL * 2; i++) {
        dataToSend.push_back((uint8_t)(i + 0x40));
    }
    prepareFunctionDataToSend(sinkId, dataToSend);

    // Since TX buffer is infinite, the whole message will be pushed out
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    // Read response
    std::vector<uint8_t> expectedMessageHeader({ 0x2, 0x3, sinkId, 0x0, 0x0, sizeL, (uint8_t)(sizeL * 2) });
    REQUIRE(simulateRead() == expectedMessageHeader + dataToSend + crcOf(expectedMessageHeader, dataToSend));
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    rs485_state = RS485_LINE_RX;
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}

static void testCorrectWrite(uint8_t sinkId, uint8_t sizeL) {
    testSizeSetup(sinkId, 0, sizeL, 0x10);
    // Still missing the content size
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_REGISTER_DATA);

    // Write sizeL * 2 bytes
    simulateData({ (uint8_t)(sizeL * 2) });
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_READ_STREAM);
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_READ_STREAM);

    // Avoid call simulateData with zero size, it is a mark condition
    if (sizeL > 0) {
        // Push stream data to keep CRC correct
        std::vector<uint8_t> dataToSend;
        for (int i = 0; i < sizeL * 2; i++) {
            dataToSend.push_back((uint8_t)i);
        }
        simulateData(dataToSend);
        REQUIRE(bus_cl_poll() == false);

        checkFunctionDataReceived(sinkId, dataToSend);
    }
    
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_CHECK_REQUEST_CRC);

    simulateData(crcOf(packetData));
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_RESPONSE);

    simulateMark();
    REQUIRE(bus_cl_poll() == false);

    // Read empty response
    REQUIRE(simulateRead() == padWithCrc({ 0x2, 0x10, sinkId, 0x0, 0x0, sizeL }));
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    rs485_state = RS485_LINE_RX;
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}

TEST_CASE("Correct size, sink 0, read") {
    testCorrectRead(0, 1);
}
TEST_CASE("Correct size, sink 1, write") {
    testCorrectWrite(1, 2);
}
TEST_CASE("Correct size, sink 2, read") {
    testCorrectRead(2, 4);
}
TEST_CASE("Correct size, sink 2, write") {
    testCorrectWrite(2, 4);
}
