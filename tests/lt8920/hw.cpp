#include <fcntl.h>
#include <memory.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <iostream>
#include <string>

#include "hw.h"
#include "gpio.h"

using namespace std::literals;

/**
 * Uses linux dev
 */

static int SPI_FD;

static void err(const char* err) {
    std::cerr << err << ", errno: " << errno << std::endl;
    exit(1);
}

static void spi_init(int deviceIndex) {
    auto deviceName = "/dev/spidev0."s + std::to_string(deviceIndex);
    std::cout << "Opening " << deviceName << std::endl;

    SPI_FD = open(deviceName.c_str(), O_RDWR);
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

static OutputPin reset_0(24, false);
static OutputPin reset_1(23, false);
static int _deviceIndex;

void hw_init(int deviceIndex) {
    _deviceIndex = deviceIndex;
    spi_init(deviceIndex);
}

extern "C" {
    void gpio_reset(_Bool asserted) {
        if (_deviceIndex == 0) {
            reset_0 = !asserted;
        } else {
            reset_1 = !asserted;
        }
    }

    uint8_t spi_get_reg8(uint8_t reg) {
        uint8_t txBuf[2];
        uint8_t rxBuf[2];
        txBuf[0] = reg | 0x80;

        struct spi_ioc_transfer tr;
        memset(&tr, 0, sizeof(tr));
        tr.tx_buf = (unsigned long long)txBuf;
        tr.rx_buf = (unsigned long long)rxBuf;
        tr.len = 2;
        tr.speed_hz = 500000;
        tr.bits_per_word = 8;
        int status = ioctl(SPI_FD, SPI_IOC_MESSAGE(1), &tr);

        if (status < 0) {
            err("can't receive register");
        }
        return rxBuf[1];
    }

    void spi_set_reg8(uint8_t reg, uint8_t val) {
        uint8_t txBuf[2];
        uint8_t rxBuf[2];
        txBuf[0] = reg & ~0x80;
        txBuf[1] = val;

        struct spi_ioc_transfer tr;
        memset(&tr, 0, sizeof(tr));
        tr.tx_buf = (unsigned long long)txBuf;
        tr.rx_buf = (unsigned long long)rxBuf;
        tr.len = 2;
        tr.speed_hz = 500000;
        tr.bits_per_word = 8;
        int status = ioctl(SPI_FD, SPI_IOC_MESSAGE(1), &tr);

        if (status < 0) {
            err("can't receive register");
        }
    }

    uint16_t spi_get_reg16_msb_first(uint8_t reg) {
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

    void spi_set_reg16_msb_first(uint8_t reg, uint16_t val) {
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
}
