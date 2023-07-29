#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include "radio.h"
#include "hw.h"
#include "lt8920.h"

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
    while (radio_poll()) {
        usleep(500);
    }

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm lTime;
    localtime_r(&now, &lTime);
    std::stringstream str;
    str << std::put_time(&lTime, "%H:%M:%S");
    auto text = str.str();
    std::strcpy(reinterpret_cast<char*>(radio_buffer), text.c_str());
    radio_write_packet(text.size());
    while (radio_write_in_progress()) {
        radio_poll();
        usleep(500);
    }

    std::cout << "Transmitted: " << text << " (" << text.size() << " bytes)\n";
    // Now back to receive
}

static void receive() {
    // Wait for data available and no more active
    bool active;
    do {
        usleep(500);
        radio_poll();
    } while (!radio_packet_ready());

    int l = radio_read_avail();
    std::string text(reinterpret_cast<const char*>(radio_buffer), l);

    std::cout << "Received: " << text << " (" << l << " bytes)\n";
}

static void primary() {
    // Start with transmitting
    while (true) {
        transmit();
        exit(0);
        receive();
    }
}

static void secondary() {
    // Start with receiving
    while (true) {
        receive();
        exit(0);
        transmit();
    }
}

int main(int argc, const char** argv) {
    if (argc < 2) {
        std::cerr << "You need to specify 'primary' or 'secondary' as argument\n";
        return 1;
    }

    int deviceIndex;
    if (argv[1] == "primary"s) {
        deviceIndex = 0;
    } else if (argv[1] == "secondary"s) {
        deviceIndex = 1;
    } else {
        std::cerr << "You need to specify 'primary' or 'secondary' as argument\n";
        return 1;
    }

    hw_init(deviceIndex);
    radio_init();

    LT8920_REVISION_INFO rev;
    lt8920_get_rev(&rev);
    std::cout << "Lower bits of JEDEC JEP106K Manufacture’s ID code: 0x" << std::hex << (rev.reg31.b.ID_CODE_JEDEC_MCODE_M << 16) + rev.reg30.b.ID_CODE_JEDEC_MCODE_L;
    std::cout << ", RF_CODE_ID: 0x" << rev.reg31.b.RF_CODE_ID << ", RF_VER_ID: 0x" << rev.reg29.b.RF_VER_ID;
    std::cout << ", MCU_VER_ID: 0x" << rev.reg29.b.MCU_VER_ID << std::dec << std::endl;

    if (deviceIndex == 0) {
        primary();
    } else {
        secondary();
    }
    return 0;
}