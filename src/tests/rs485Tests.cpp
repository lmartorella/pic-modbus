#include <catch2/catch.hpp>
#include <stdbool.h>
#include <queue>

#include "pic-modbus/crc.h"
#include "pic-modbus/uart.h"
#include "pic-modbus/rs485.h"
#include "pic-modbus/sys.h"

using namespace std::string_literals;

enum { TRANSMIT, RECEIVE } mode;
std::queue<uint8_t> txQueue;
std::queue<uint8_t> rxQueue;
TICK_TYPE s_timer = 0;
int txQueueSize = 1000; // no max
bool simulateHwRxOverrun = false;
bool simulateHwRxFrameError = false;

static void initMock(int _txQueueSize) {
    txQueueSize = _txQueueSize;
    simulateHwRxOverrun = simulateHwRxFrameError = false;
}

static void simulateSend(const std::vector<uint8_t>& data) {
    for (auto it = data.begin(); it != data.end(); ++it) {
        rxQueue.push(*it);
    }
}

static std::vector<uint8_t> receiveAllData() {
    std::vector<uint8_t> data(txQueue.size());
    for (int i = 0; i < data.size(); i++) {
        data[i] = txQueue.front();
        txQueue.pop();
    }
    return data;
}

static std::vector<uint8_t> bufferAsVector(int size) {
    std::vector<uint8_t> data(size);
    for (int i = 0; i < size; i++) {
        data[i] = rs485_buffer[i];
    }
    return data;
}

static void advanceTime(TICK_TYPE delta) {
    s_timer += delta;
}

extern "C" {
    // Internal
    extern _Bool rs485_frameError;
    SYS_RESET_REASON sys_resetReason;
    UART_LAST_CH uart_lastCh;

    void uart_init() {
        while(!rxQueue.empty()) rxQueue.pop();
        while(!txQueue.empty()) txQueue.pop();
        uart_receive();
    }

    void uart_transmit() {
        mode = TRANSMIT;
    }

    void uart_receive() {
        mode = RECEIVE;
    }

    void uart_read() {
        if (mode != RECEIVE) {
            throw std::runtime_error("Read called in transmit mode");
        }
        if (rxQueue.empty()) {
            throw std::runtime_error("No data to read");
        }
        uart_lastCh.data = rxQueue.front();
        rxQueue.pop();
        uart_lastCh.errs.FERR = simulateHwRxFrameError;
        uart_lastCh.errs.OERR = simulateHwRxOverrun;
    }

    void uart_write(uint8_t byte) {
        if (mode != TRANSMIT) {
            throw std::runtime_error("Write called in receive mode");
        }
        if (txQueue.size() >= txQueueSize) {
            throw std::runtime_error("Buffer overrun during TX");
        }
        txQueue.push(byte);
    }

    _Bool uart_tx_fifo_empty() {
        return txQueue.size() < txQueueSize;
    }

    _Bool uart_rx_fifo_empty() {
        return rxQueue.empty();
    }

    TICK_TYPE timers_get() {
        return s_timer;
    }

    void fatal(const char* msg) {
        throw std::runtime_error("Fatal "s + msg);
    }
}

TEST_CASE("Normal operations") {
    initMock(1);

    rs485_init();
    REQUIRE(rs485_state == RS485_LINE_RX);

    REQUIRE(rs485_poll() == false);

    simulateSend({ 0x1 });

    // Read data in a single poll call
    REQUIRE(rs485_poll() == false);
    REQUIRE(rs485_readAvail() == 1);
    REQUIRE(!rs485_isMarkCondition);

    simulateSend({ 0x2, 0x3 });

    // Read data in a single poll call
    REQUIRE(rs485_poll() == false);

    REQUIRE(rs485_readAvail() == 3);
    REQUIRE(!rs485_isMarkCondition);

    REQUIRE(bufferAsVector(3) == std::vector<uint8_t>({ 0x1, 0x2, 0x3 }));

    // Write back the same buffer
    rs485_write(3);
    REQUIRE(rs485_state == RS485_LINE_WAIT_FOR_START_TRANSMIT);

    // It waits and require polling
    REQUIRE(rs485_poll() == true);
    REQUIRE(rs485_poll() == true);

    advanceTime(START_TRANSMIT_TIMEOUT + 1);

    // Write data until buffer is full
    REQUIRE(rs485_poll() == false);
    REQUIRE(rs485_state == RS485_LINE_TX);

    REQUIRE(receiveAllData() == std::vector<uint8_t>({ 0x1 }));

    // Write data until buffer is full
    REQUIRE(rs485_poll() == false);

    REQUIRE(receiveAllData() == std::vector<uint8_t>({ 0x2 }));

    // Write data until buffer is full
    REQUIRE(rs485_poll() == false);

    REQUIRE(receiveAllData() == std::vector<uint8_t>({ 0x3 }));

    REQUIRE(rs485_poll() == true);
    REQUIRE(rs485_poll() == true);

    REQUIRE(rs485_state == RS485_LINE_TX_DISENGAGE);

    advanceTime(DISENGAGE_CHANNEL_TIMEOUT + 1);

    REQUIRE(rs485_poll() == false);

    REQUIRE(rs485_state == RS485_LINE_RX);
    REQUIRE(rs485_isMarkCondition);
}

TEST_CASE("Max RX buffer") {
    initMock(32);

    rs485_init();
    REQUIRE(rs485_poll() == false);

    for (int i = 0; i < RS485_BUF_SIZE; i++) {
        simulateSend({ (uint8_t)i });
    }

    // Read data in a single poll call
    REQUIRE(rs485_poll() == false);
    REQUIRE(rs485_readAvail() == RS485_BUF_SIZE);
    for (int i = 0; i < RS485_BUF_SIZE; i++) {
        REQUIRE(rs485_buffer[i] == i);
    }
    rs485_discard(RS485_BUF_SIZE);

    // Test rollover
    for (int i = 0; i < 2; i++) {
        simulateSend({ (uint8_t)(i + 100)});
    }

    REQUIRE(rs485_poll() == false);

    REQUIRE(rs485_readAvail() == 2);

    for (int i = 0; i < 2; i++) {
        REQUIRE(rs485_buffer[i] == i + 100);
    }
    rs485_discard(2);

    // Test overrun error
    for (int i = 0; i < RS485_BUF_SIZE; i++) {
        simulateSend({ (uint8_t)i });
    }
    REQUIRE(rs485_poll() == false);
    REQUIRE(rs485_readAvail() == RS485_BUF_SIZE);
    rs485_discard(RS485_BUF_SIZE);

    // Test overrun error
    for (int i = 0; i < RS485_BUF_SIZE + 1; i++) {
        simulateSend({ (uint8_t)i });
    }
    CHECK_THROWS_WITH(rs485_poll(), "Fatal EXC_CODE_RS485_READ_OVERRUN");
}

TEST_CASE("Max TX buffer") {
    initMock(33);

    rs485_init();
    REQUIRE(rs485_poll() == false);

    for (int i = 0; i < RS485_BUF_SIZE; i++) {
        rs485_buffer[i] = (uint8_t)i;
    }

    rs485_write(RS485_BUF_SIZE);
    // Write data in a single poll call
    REQUIRE(rs485_poll() == true);
    advanceTime(START_TRANSMIT_TIMEOUT + 1);
    // At the end of TX starts disengage timer
    REQUIRE(rs485_poll() == true);
    auto rx = receiveAllData();
    REQUIRE(rx == bufferAsVector(RS485_BUF_SIZE));

    advanceTime(DISENGAGE_CHANNEL_TIMEOUT + 1);
    REQUIRE(rs485_poll() == false);

    // Test rollover
    for (int i = 0; i < 2; i++) {
        rs485_buffer[i] = (uint8_t)(i + 200);
    }

    rs485_write(2);
    // Write data in a single poll call
    REQUIRE(rs485_poll() == true);
    advanceTime(START_TRANSMIT_TIMEOUT + 1);
    // At the end of TX starts disengage timer
    REQUIRE(rs485_poll() == true);
    rx = receiveAllData();
    REQUIRE(rx == bufferAsVector(2));

    advanceTime(DISENGAGE_CHANNEL_TIMEOUT + 1);
    REQUIRE(rs485_poll() == false);
}

TEST_CASE("Coalesce write chunks if possible") {
    initMock(2);
    rs485_init();
    REQUIRE(rs485_poll() == false);

    rs485_buffer[0] = 0x20;

    rs485_write(1);
    REQUIRE(rs485_poll() == true);
    advanceTime(START_TRANSMIT_TIMEOUT + 1);
    // Only one byte written, so the disengage timer starts
    REQUIRE(rs485_poll() == true);
    REQUIRE(rs485_state == RS485_LINE_TX_DISENGAGE);
    auto rx = receiveAllData();
    REQUIRE(rx.size() == 1);
    REQUIRE(rx[0] == 0x20);

    // Now write again: the line will not be disengaged
    rs485_buffer[0] = 0x21;
    rs485_write(1);
    REQUIRE(rs485_state == RS485_LINE_TX);
    // Only one byte written, so the disengage timer starts
    REQUIRE(rs485_poll() == true);
    REQUIRE(rs485_state == RS485_LINE_TX_DISENGAGE);
    rx = receiveAllData();
    REQUIRE(rx.size() == 1);
    REQUIRE(rx[0] == 0x21);

    advanceTime(DISENGAGE_CHANNEL_TIMEOUT - 1);
    REQUIRE(rs485_poll() == true);
    advanceTime(2);

    REQUIRE(rs485_poll() == false);

    REQUIRE(rs485_state == RS485_LINE_RX);
}

TEST_CASE("Test RX hardware overrun") {
    initMock(1);
    rs485_init();
    REQUIRE(rs485_poll() == false);

    simulateSend({ (uint8_t)0 });
    simulateHwRxOverrun = true;

    CHECK_THROWS_WITH(rs485_poll(), "Fatal EXC_CODE_RS485_READ_UNDERRUN");;
}

TEST_CASE("Test RX hardware frame error") {
    initMock(1);
    rs485_init();
    REQUIRE(rs485_poll() == false);
    REQUIRE(!rs485_frameError);

    simulateSend({ (uint8_t)0x1 });
    simulateHwRxFrameError = true;

    REQUIRE(rs485_poll() == false);
    REQUIRE(rs485_frameError);
    // Can't read
    REQUIRE(rs485_readAvail() == 0);
    REQUIRE(rs485_poll() == false);
    // Can't read
    REQUIRE(rs485_readAvail() == 0);
    REQUIRE(rs485_frameError);

    rs485_frameError = false;

    simulateSend({ (uint8_t)0x2 });
    simulateHwRxFrameError = false;

    REQUIRE(rs485_poll() == false);
    // First data lost
    REQUIRE(rs485_readAvail() == 1);
    REQUIRE(rs485_buffer[0] == 0x2);
    REQUIRE(!rs485_frameError);
}

TEST_CASE("Test skip data") {
    initMock(1);
    rs485_init();
    REQUIRE(rs485_poll() == false);
    REQUIRE(!rs485_frameError);

    simulateSend({ (uint8_t)0x1, (uint8_t)0x2 });

    REQUIRE(rs485_poll() == false);
    REQUIRE(!rs485_frameError);
    REQUIRE(rs485_readAvail() == 2);
    REQUIRE(rs485_poll() == false);

    rs485_discard(2);
    REQUIRE(rs485_poll() == false);

    REQUIRE(!rs485_frameError);
    REQUIRE(rs485_readAvail() == 0);
    REQUIRE(rs485_poll() == false);

    simulateSend({ (uint8_t)0x2 });
    simulateHwRxFrameError = false;

    REQUIRE(rs485_poll() == false);
    // First data lost
    REQUIRE(rs485_readAvail() == 1);
    REQUIRE(rs485_buffer[0] == 0x2);
    REQUIRE(!rs485_frameError);
}

TEST_CASE("Test mark condition") {
    initMock(1);
    rs485_init();
    REQUIRE(rs485_poll() == false);
    REQUIRE(rs485_isMarkCondition);

    // Packed data
    for (int i = 0; i < 50; i++) {
        advanceTime(TICKS_PER_CHAR);
        simulateSend({ (uint8_t)i });
        REQUIRE(rs485_poll() == false);
        REQUIRE(!rs485_isMarkCondition);
        REQUIRE(rs485_readAvail() == 1);
        REQUIRE(rs485_buffer[0] == i);
        rs485_discard(1);
    }

    // More slack data
    for (int i = 0; i < 50; i++) {
        advanceTime(TICKS_PER_CHAR * 2);
        simulateSend({ (uint8_t)i });
        REQUIRE(rs485_poll() == false);
        REQUIRE(!rs485_isMarkCondition);
        REQUIRE(rs485_readAvail() == 1);
        REQUIRE(rs485_buffer[0] == i);
        rs485_discard(1);
    }

    // Timeout
    advanceTime(TICKS_PER_CHAR * 4);
    REQUIRE(rs485_poll() == false);
    REQUIRE(rs485_isMarkCondition);
}

TEST_CASE("Test CRC on read") {
    initMock(1);
    rs485_init();
    REQUIRE(rs485_poll() == false);

    // Packed data
    advanceTime(TICKS_PER_CHAR);
    simulateSend({ (uint8_t)0x1, (uint8_t)0x2 });
    REQUIRE(rs485_poll() == false);
    REQUIRE(!rs485_isMarkCondition);
    REQUIRE(rs485_readAvail() == 2);

    // Calculated on read
    REQUIRE(crc16 == 0xffff);
    rs485_discard(2);
    REQUIRE(crc16 == 0xE181);
}

TEST_CASE("Test CRC on write") {
    initMock(16);
    rs485_init();
    REQUIRE(rs485_poll() == false);

    rs485_buffer[0] = 0x01;
    rs485_buffer[1] = 0x02;
    rs485_write(2);
    REQUIRE(crc16 == 0xFFFF);
    REQUIRE(rs485_poll() == true);
    advanceTime(START_TRANSMIT_TIMEOUT + 1);
    REQUIRE(rs485_poll() == true);
    REQUIRE(rs485_state == RS485_LINE_TX_DISENGAGE);
    REQUIRE(crc16 == 0xE181);
}
