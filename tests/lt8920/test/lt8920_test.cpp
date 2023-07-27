#include <catch2/catch.hpp>

#define UNIT_TEST
static void check_init_reg(uint8_t reg, uint16_t val, uint16_t mask) {
    // Check that the val doesn't set reserved bits
    REQUIRE((val & mask) == 0);
}

#include "../lt8920.c"

extern "C" {
    void gpio_reset() {

    }

    void spi_set_reg(uint8_t reg, uint16_t val) {

    }

    uint16_t spi_get_reg(uint8_t reg) {
        return -1;
    }
}

TEST_CASE("Check init_registers correctly uses mask") {
    init_registers();
}

