#include <catch2/catch.hpp>
#include <stdbool.h>
#include <vector>

#include "net/bus_client.h"
#include "net/rs485.h"

using namespace std::string_literals;

extern "C" {
    RS485_LINE_STATE rs485_state;

    uint8_t rs485_readAvail() {
        return 0;
    }
}

static void simulatePacket(const std::vector<uint8_t>& data) {

}

TEST_CASE("Normal operations") {
    bus_cl_init();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_poll() == false);

    simulatePacket({ 0x1, 0x2, 0x3 });
    REQUIRE(bus_cl_poll() == false);
}
