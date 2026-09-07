/**
 * @file SSD1306_I2C.cpp
 * @brief Implementación por ioctl de Linux I2C (/dev/i2c-N).
 *
 * @details Usa el controlador I2C estándar de Linux vía ioctl (I2C_SLAVE) y
 * write(). No depende de bcm2835, por lo que funciona en Raspberry Pi 5/CM5.
 *
 * @author Proyecto BASIC (Raspberry Pi)
 * @version 1.1.0
 */

#include "SSD1306_I2C.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

namespace SSD1306LinuxI2C {

const char* I2C_DEV_PATH = "/dev/i2c-1";

int i2c_open(const char* device, uint8_t address) {
    int fd = open(device, O_RDWR);
    if (fd < 0) {
        printf("Error: No se pudo abrir %s: %s\n", device, strerror(errno));
        return -1;
    }
    if (!i2c_set_address(fd, address)) {
        close(fd);
        return -1;
    }
    return fd;
}

bool i2c_set_address(int fd, uint8_t address) {
    if (ioctl(fd, I2C_SLAVE, address) < 0) {
        printf("Error: No se pudo configurar dirección 0x%02X: %s\n",
               address, strerror(errno));
        return false;
    }
    return true;
}

bool i2c_write_byte(int fd, uint8_t value, uint8_t cmd) {
    uint8_t buf[2] = { cmd, value };
    if (write(fd, buf, 2) == 2) {
        return true;   // ACK (el dispositivo respondió)
    }
    return false;      // NACK o error de escritura
}

void i2c_close(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}

} // namespace SSD1306LinuxI2C
