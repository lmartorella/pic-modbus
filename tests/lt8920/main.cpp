#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include "lt8920.h"
#include "hw.h"

using namespace std::literals;

extern "C" {
    void debug_print_init_reg(uint8_t reg, uint16_t init_val, uint16_t set_val) {
        // save default formatting
        std::ios init(NULL);
        init.copyfmt(std::cout);

        std::cout << "Reg " << (int)(reg)
            << std::setfill('0') << std::setw(4) << std::right << std::hex
            << ": value 0x" << init_val << " -> 0x" << set_val << std::endl;

        // restore default formatting
        std::cout.copyfmt(init);
    }
}

static void transmit() {
    // Wait for IDLE
    while (lt8920_poll()) {
        usleep(500);
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
        usleep(500);
    }

    std::cout << "Transmitted: " << text << "\n";
    // Now back to receive
}

static void receive() {
    // Wait for IDLE
    while (lt8920_poll()) {
        usleep(500);
    }

    int l = lt8920_readAvail();
    lt8920_discard(l);
    lt8920_buffer[0] = 0;

    // Now wait for data available and no more active
    bool active;
    while ((active = lt8920_poll()), active || lt8920_readAvail() <= 0) {
        usleep(500);
    }

    l = lt8920_readAvail();
    std::string text(reinterpret_cast<const char*>(lt8920_buffer), l);
    lt8920_discard(l);

    std::cout << "Received: " << text << "\n";

    sleep(1);
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
    if (argc < 2) {
        std::cerr << "You need to specify 'primary' or 'secondary' as argument\n";
        return 1;
    }

    hw_init();
    lt8920_init();

    LT8920_REVISION_INFO rev;
    lt8920_get_rev(&rev);
    std::cout << "Lower bits of JEDEC JEP106K Manufacture’s ID code: 0x" << std::hex << (rev.reg31.b.ID_CODE_JEDEC_MCODE_M << 16) + rev.reg30.b.ID_CODE_JEDEC_MCODE_L;
    std::cout << ", RF_CODE_ID: 0x" << rev.reg31.b.RF_CODE_ID << ", RF_VER_ID: 0x" << rev.reg29.b.RF_VER_ID;
    std::cout << ", MCU_VER_ID: 0x" << rev.reg29.b.MCU_VER_ID << std::dec << std::endl;

    if (argv[1] == "primary"s) {
        primary();
    } else if (argv[1] == "secondary"s) {
        secondary();
    } else {
        std::cerr << "You need to specify 'primary' or 'secondary' as argument\n";
        return 1;
    }
    return 0;
}