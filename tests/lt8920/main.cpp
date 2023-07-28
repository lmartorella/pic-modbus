#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include "lt8920.h"

static void transmit() {
    // Wait for IDLE
    while (lt8920_poll()) {
        sleep(1);
    }

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm lTime;
    localtime_r(&now, &lTime);
    std::stringstream str;
    str << std::put_time(&lTime, "%H:%M:%S");
    auto text = str.str();
    std::strcpy(reinterpret_cast<char*>(lt8920_buffer), text.c_str());
    lt8920_write_packet(text.size());
    while (lt8920_writeInProgress()) {
        lt8920_poll();
        sleep(1);
    }

    std::cout << "Transmitted: " << text << "\n";
    // Now back to receive
}

static void receive() {
    // Wait for IDLE
    while (lt8920_poll()) {
        sleep(1);
    }

    int l = lt8920_readAvail();
    lt8920_discard(l);

    // Now wait for data available and no more active
    bool active;
    while ((active = lt8920_poll()), active || lt8920_readAvail() <= 0) {
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

    LT8920_REVISION_INFO rev;
    lt8920_get_rev(&rev);
    std::cout << "Lower bits of JEDEC JEP106K Manufacture’s ID code: " << (rev.reg31.b.ID_CODE_JEDEC_MCODE_M << 16) + rev.reg30.b.ID_CODE_JEDEC_MCODE_L;
    std::cout << ", RF_CODE_ID: " << rev.reg31.b.RF_CODE_ID << ", RF_VER_ID: " << rev.reg29.b.RF_VER_ID;
    std::cout << ", MCU_VER_ID: " << rev.reg29.b.MCU_VER_ID << std::endl;

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