#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include "lt8920.h"

static void transmit() {
    // Wait for MODBUS mark condition
    while (!lt8920_isMarkCondition) {
        lt8920_poll();
        sleep(1);
    }

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm lTime;
    localtime_r(&now, &lTime);
    std::stringstream str;
    str << std::put_time(&lTime, "%H:%M:%S");
    auto text = str.str();
    std::strcpy(reinterpret_cast<char*>(lt8920_buffer), text.c_str());
    lt8920_write(text.size());
    while (lt8920_writeInProgress()) {
        lt8920_poll();
        sleep(1);
    }

    std::cout << "Transmitted: " << text << "\n";
    // Now back to receive
}

static void receive() {
    // Wait for MODBUS mark condition
    while (!lt8920_isMarkCondition) {
        lt8920_poll();
        sleep(1);
    }

    int l = lt8920_readAvail();
    lt8920_discard(l);

    // Now wait for data
    while (lt8920_readAvail() <= 0) {
        lt8920_poll();
        sleep(1);
    }

    // Wait for MODBUS mark condition
    while (!lt8920_isMarkCondition) {
        lt8920_poll();
        sleep(1);
    }

    l = lt8920_readAvail();
    std::string text(reinterpret_cast<const char*>(lt8920_buffer), l);
    lt8920_discard(l);

    std::cout << "Received: " << text << "\n";

    sleep(1000);
}

static void primary() {
    // Start with transmitting
    while (true) {
        transmit();
        receive();
    }
}

static void secondary() {
    // Start with receiving
    while (true) {
        receive();
        transmit();
    }
}

int main(int argc, const char** argv) {
    if (argc < 1) {
        std::cerr << "You need to specify 'primary' or 'secondary' as argument";
        return 1;
    }

    lt8920_init();

    if (argv[1] == u8"primary") {
        primary();
    } else if (argv[1] == u8"secondary") {
        secondary();
    } else {
        std::cerr << "You need to specify 'primary' or 'secondary' as argument";
        return 1;
    }
    return 0;
}