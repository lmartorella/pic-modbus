#include <catch2/catch.hpp>
#include <stdbool.h>
#include <queue>

typedef _Bool __bit;
#include "net/uart.h"
#include "net/rs485.h"

using namespace std::string_literals;

bool rxEnabled = false;
bool txEnabled = false;
enum { TRANSMIT, RECEIVE } mode;
std::queue<uint8_t> txQueue;
std::queue<uint8_t> rxQueue;
TICK_TYPE s_timer = 0;
int txQueueSize = 1000; // no max
bool simulateHwRxOverrun = false;

static void initMock(int _txQueueSize) {
    txQueueSize = _txQueueSize;
    simulateHwRxOverrun = false;
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

static void advanceTime(TICK_TYPE delta) {
    s_timer += delta;
}

extern "C" {
    void uart_init() {
        uart_disable_rx();
        uart_disable_tx();
        uart_receive();
    }

    void uart_disable_rx() {
        rxEnabled = false;
        while(!rxQueue.empty()) rxQueue.pop();
    }

    void uart_disable_tx() {
        txEnabled = false;
        while(!txQueue.empty()) txQueue.pop();
    }

    void uart_enable_rx() {
        if (txEnabled) {
            throw std::runtime_error("Not supposed to happen on RS485");
        }
        rxEnabled = true;
    }

    void uart_enable_tx() {
        if (rxEnabled) {
            throw std::runtime_error("Not supposed to happen on RS485");
        }
        txEnabled = true;
    }

    void uart_transmit() {
        mode = TRANSMIT;
    }

    void uart_receive() {
        mode = RECEIVE;
    }

    void uart_read(uint8_t* byte, UART_RX_MD* md) {
        if (mode != RECEIVE) {
            throw std::runtime_error("Read called in transmit mode");
        }
        if (!rxEnabled) {
            throw std::runtime_error("RX not enabled");
        }
        if (rxQueue.empty()) {
            throw std::runtime_error("No data to read");
        }
        *byte = rxQueue.front();
        rxQueue.pop();
        md->frameErr = false;
        md->overrunErr = simulateHwRxOverrun;
    }

    void uart_write(uint8_t byte) {
        if (mode != TRANSMIT) {
            throw std::runtime_error("Write called in receive mode");
        }
        if (!txEnabled) {
            throw std::runtime_error("TX not enabled");
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
    REQUIRE(rxEnabled == true);
    REQUIRE(txEnabled == false);
    REQUIRE(rs485_state == RS485_LINE_RX);

    REQUIRE(rs485_poll() == false);

    std::vector<uint8_t> buffer(3);

    simulateSend({ 0x1 });

    // Read data in a single poll call
    REQUIRE(rs485_poll() == false);
    REQUIRE(rs485_read(&buffer[0], 4) == false);

    simulateSend({ 0x2, 0x3 });

    // Read data in a single poll call
    REQUIRE(rs485_poll() == false);

    REQUIRE(rs485_read(&buffer[0], 3) == true);

    REQUIRE(buffer == std::vector<uint8_t>({ 0x1, 0x2, 0x3 }));

    rs485_write(&buffer[0], 3);
    REQUIRE(rs485_state == RS485_LINE_WAIT_FOR_START_TRANSMIT);

    REQUIRE(rxEnabled == false);
    REQUIRE(txEnabled == true);

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
    REQUIRE(txEnabled == true);
    REQUIRE(rxEnabled == false);

    advanceTime(DISENGAGE_CHANNEL_TIMEOUT + 1);

    REQUIRE(rs485_poll() == false);
    REQUIRE(txEnabled == false);
    REQUIRE(rxEnabled == true);

    REQUIRE(rs485_state == RS485_LINE_RX);
}

TEST_CASE("Max RX buffer") {
    initMock(32);

    rs485_init();
    REQUIRE(rs485_poll() == false);

    for (int i = 0; i < RS485_BUF_SIZE - 1; i++) {
        simulateSend({ (uint8_t)i });
    }

    // Read data in a single poll call
    REQUIRE(rs485_poll() == false);

    std::vector<uint8_t> buffer(RS485_BUF_SIZE - 1);
    REQUIRE(rs485_read(&buffer[0], RS485_BUF_SIZE - 1) == true);

    for (int i = 0; i < RS485_BUF_SIZE - 1; i++) {
        REQUIRE(buffer[i] == i);
    }

    // Test rollover
    for (int i = 0; i < 2; i++) {
        simulateSend({ (uint8_t)(i + 100)});
    }

    REQUIRE(rs485_poll() == false);

    std::vector<uint8_t> buffer2(2);
    REQUIRE(rs485_read(&buffer[0], 2) == true);

    for (int i = 0; i < 2; i++) {
        REQUIRE(buffer[i] == i + 100);
    }

    // Test overrun error
    for (int i = 0; i < RS485_BUF_SIZE; i++) {
        simulateSend({ (uint8_t)i });
    }

    CHECK_THROWS_WITH(rs485_poll(), "Fatal U.rov");
}

TEST_CASE("Max TX buffer") {
    initMock(32);

    rs485_init();
    REQUIRE(rs485_poll() == false);

    std::vector<uint8_t> buffer(RS485_BUF_SIZE - 1);
    for (int i = 0; i < RS485_BUF_SIZE - 1; i++) {
        buffer[i] = (uint8_t)i;
    }

    rs485_write(&buffer[0], RS485_BUF_SIZE - 1);
    // Write data in a single poll call
    REQUIRE(rs485_poll() == true);
    advanceTime(START_TRANSMIT_TIMEOUT + 1);
    // At the end of TX starts disengage timer
    REQUIRE(rs485_poll() == true);
    auto rx = receiveAllData();
    REQUIRE(rx == buffer);

    advanceTime(DISENGAGE_CHANNEL_TIMEOUT + 1);
    REQUIRE(rs485_poll() == false);

    // Test rollover
    std::vector<uint8_t> buffer2(2);
    for (int i = 0; i < 2; i++) {
        buffer2[i] = (uint8_t)(i + 200);
    }

    rs485_write(&buffer2[0], 2);
    // Write data in a single poll call
    REQUIRE(rs485_poll() == true);
    advanceTime(START_TRANSMIT_TIMEOUT + 1);
    // At the end of TX starts disengage timer
    REQUIRE(rs485_poll() == true);
    rx = receiveAllData();
    REQUIRE(rx == buffer2);

    advanceTime(DISENGAGE_CHANNEL_TIMEOUT + 1);
    REQUIRE(rs485_poll() == false);

    // Check overrun
    std::vector<uint8_t> buffer3(RS485_BUF_SIZE);
    CHECK_THROWS_WITH(rs485_write(&buffer3[0], RS485_BUF_SIZE), "Fatal U.wov");;
}

TEST_CASE("Coalesce write chunks if possible") {
    initMock(2);
    rs485_init();
    REQUIRE(rs485_poll() == false);

    std::vector<uint8_t> buffer(1);
    buffer[0] = 0x20;

    rs485_write(&buffer[0], 1);
    REQUIRE(rs485_poll() == true);
    advanceTime(START_TRANSMIT_TIMEOUT + 1);
    // Only one byte written, so the disengage timer starts
    REQUIRE(rs485_poll() == true);
    REQUIRE(rs485_state == RS485_LINE_TX_DISENGAGE);
    auto rx = receiveAllData();
    REQUIRE(rx.size() == 1);
    REQUIRE(rx[0] == 0x20);

    REQUIRE(txEnabled == true);
    REQUIRE(rxEnabled == false);

    // Now write again: the line will not be disengaged
    buffer[0] = 0x21;
    rs485_write(&buffer[0], 1);
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
    REQUIRE(txEnabled == false);
    REQUIRE(rxEnabled == true);

    REQUIRE(rs485_state == RS485_LINE_RX);
}

TEST_CASE("Test read in the middle of transmission (abort)") {
    initMock(1);
    rs485_init();
    REQUIRE(rs485_poll() == false);

    std::vector<uint8_t> buffer(3);
    for (int i = 0; i < 3; i++) {
        buffer[i] = (uint8_t)(i + 0x10);
    }

    rs485_write(&buffer[0], 3);
    REQUIRE(rs485_poll() == true);
    advanceTime(START_TRANSMIT_TIMEOUT + 1);
    // Only one byte written
    REQUIRE(rs485_poll() == false);
    auto rx = receiveAllData();
    REQUIRE(rx.size() == 1);
    REQUIRE(rx[0] == 0x10);

    // Now start read: write will be aborted
    REQUIRE(rs485_read(&buffer[0], 1) == false);

    REQUIRE(rs485_state == RS485_LINE_TX_DISENGAGE);
    REQUIRE(txEnabled == true);
    REQUIRE(rxEnabled == false);

    advanceTime(DISENGAGE_CHANNEL_TIMEOUT + 1);

    REQUIRE(rs485_poll() == false);
    REQUIRE(txEnabled == false);
    REQUIRE(rxEnabled == true);

    REQUIRE(rs485_state == RS485_LINE_RX);
}

TEST_CASE("Test RX hardware overrun") {
    initMock(1);
    rs485_init();
    REQUIRE(rs485_poll() == false);

    simulateSend({ (uint8_t)0 });
    simulateHwRxOverrun = true;

    CHECK_THROWS_WITH(rs485_poll(), "Fatal U.OER");;
}
