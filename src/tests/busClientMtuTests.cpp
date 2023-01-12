#include <catch2/catch.hpp>
#include <stdbool.h>
#include <queue>

#include "net/appio.h"
#include "net/bus_client.h"
#include "net/leds.h"
#include "net/persistence.h"
#include "net/rs485.h"

using namespace std::string_literals;

extern "C" {
    RESET_REASON g_resetReason;
    PersistentData pers_data;
    RS485_LINE_STATE rs485_state;

    void pers_load() {

    }

    void pers_save() {
        
    }

    void led_off(void) {

    }

    void led_on(void) {

    }

    uint8_t rs485_readAvail() {
        return 0;
    }
}

TEST_CASE("Normal operations") {
    bus_cl_init();
}
