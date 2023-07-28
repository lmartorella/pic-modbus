#include <fcntl.h>
#include <memory.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <iostream>

#include "hw.h"
#include "gpio.h"

/**
 * Uses linux dev
 */

static int SPI_FD;

static void err(const char* err) {
    std::cerr << err << ", errno: " << errno << std::endl;
    exit(1);
}

static void spi_init() {
    SPI_FD = open("/dev/spidev0.0", O_RDWR);
    if (SPI_FD < 0) {
        err("Can't open");
    }

    // CKPHA = 1, SPI_CPOL = 0
    uint8_t mode = SPI_MODE_1;
    int ret = ioctl(SPI_FD, SPI_IOC_WR_MODE, &mode);
    if (ret < 0) {
        err("can't set spi mode");
    }

    uint8_t bits = 8;
    ret = ioctl(SPI_FD, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret < 0) {
        err("can't set bits per word");
    }

    // Max is 12Mhz per specs
    uint32_t speed = 500000;
    ret = ioctl(SPI_FD, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret < 0) {
        err("can't set max speed hz");
    }
}

static OutputPin* reset;

void hw_init() {
    spi_init();

    reset = new OutputPin(24);
}

extern "C" {
    void gpio_reset(_Bool asserted) {
        *reset = !asserted;
    }

    void spi_set_reg_msb_first(uint8_t reg, uint16_t val) {
        uint8_t txBuf[3];
        uint8_t rxBuf[3];
        txBuf[0] = reg & ~0x80;
        txBuf[1] = val >> 8;
        txBuf[2] = val & 0xff;

        struct spi_ioc_transfer tr;
        memset(&tr, 0, sizeof(tr));
        tr.tx_buf = (unsigned long long)txBuf;
        tr.rx_buf = (unsigned long long)rxBuf;
        tr.len = 3;
        tr.speed_hz = 500000;
        tr.bits_per_word = 8;
        int status = ioctl(SPI_FD, SPI_IOC_MESSAGE(1), &tr);

        if (status < 0) {
            err("can't receive register");
        }
    }

    uint16_t spi_get_reg_msb_first(uint8_t reg) {
        uint8_t txBuf[3];
        uint8_t rxBuf[3];
        txBuf[0] = reg | 0x80;

        struct spi_ioc_transfer tr;
        memset(&tr, 0, sizeof(tr));
        tr.tx_buf = (unsigned long long)txBuf;
        tr.rx_buf = (unsigned long long)rxBuf;
        tr.len = 3;
        tr.speed_hz = 500000;
        tr.bits_per_word = 8;
        int status = ioctl(SPI_FD, SPI_IOC_MESSAGE(1), &tr);

        if (status < 0) {
            err("can't receive register");
        }
        return (uint16_t)((rxBuf[1] << 8) + rxBuf[2]);
    }
}
