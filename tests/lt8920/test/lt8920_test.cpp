#include <catch2/catch.hpp>

#define UNIT_TEST
static void check_init_reg(uint8_t reg, uint16_t val, uint16_t mask) {
    // Check that the val doesn't set reserved bits
    REQUIRE((val & mask) == 0);
}

#include "../../src/hw/lt8920.c"

extern "C" {
    void debug_print_init_reg(uint8_t reg, uint16_t init_val, uint16_t set_val) {

    }

    void spi_init() {

    }

    void gpio_init() {

    }

    void gpio_reset(_Bool asserted) {

    }

    uint8_t spi_get_reg8(uint8_t reg) {
        return -1;
    }

    void spi_set_reg8(uint8_t reg, uint8_t val) {

    }

    uint16_t spi_get_reg16_msb_first(uint8_t reg) {
        return -1;
    }

    void spi_set_reg16_msb_first(uint8_t reg, uint16_t val) {

    }
}

TEST_CASE("Check init_registers correctly uses mask") {
    lt8920_init_registers();
}

