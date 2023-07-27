#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include "hw.h"

/**
 * Uses linux dev
 */

void hw_init() {
    // int fd = open("/dev/spidev0.0", O_RDWR);

    // struct spi_ioc_transfer tr =
    // {
    //     .tx_buf = (unsigned long)tx,
    //     .rx_buf = (unsigned long)rx,
    //     .len = 1,
    //     .delay_usecs = 0,
    //     .speed_hz = 500000,
    //     .bits_per_word = 8,
    // };

    // int status = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    // if (status < 0)
    //     printf("can't send data");
}

extern "C" {
    void gpio_reset(_Bool asserted) {
        
    }

    void spi_set_reg(uint8_t reg, uint16_t val) {

    }

    uint16_t spi_get_reg(uint8_t reg) {
        return -1;
    }
}
