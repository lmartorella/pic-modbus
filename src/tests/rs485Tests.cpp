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

static void simulateSend(const std::vector<uint8_t>& data) {
    for (auto it = data.begin(); it != data.end(); ++it) {
        rxQueue.push(*it);
    }
}

static std::vector<uint8_t> simulateReceive(int size) {
    if (size > txQueue.size()) {
        throw std::runtime_error("Not enough data to read");
    }
    std::vector<uint8_t> data(size);
    for (int i = 0; i < size; i++) {
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
        rxEnabled = true;
    }

    void uart_enable_tx() {
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
        md->overrunErr = false;
    }

    void uart_write(uint8_t byte) {
        if (mode != TRANSMIT) {
            throw std::runtime_error("Write called in receive mode");
        }
        if (!txEnabled) {
            throw std::runtime_error("TX not enabled");
        }
        txQueue.push(byte);
    }

    _Bool uart_tx_fifo_empty() {
        return txQueue.empty();
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

TEST_CASE() {
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

    REQUIRE(simulateReceive(1) == std::vector<uint8_t>({ 0x1 }));

    // Write data until buffer is full
    REQUIRE(rs485_poll() == false);

    REQUIRE(simulateReceive(1) == std::vector<uint8_t>({ 0x2 }));

    // Write data until buffer is full
    REQUIRE(rs485_poll() == false);

    REQUIRE(simulateReceive(1) == std::vector<uint8_t>({ 0x3 }));

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
