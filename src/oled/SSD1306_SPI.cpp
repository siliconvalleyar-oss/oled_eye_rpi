/**
 * @file SSD1306_SPI.cpp
 * @brief Implementación de la capa de acceso SPI del SSD1306 (spidev + sysfs GPIO).
 *
 * @details Usa ioctl sobre /dev/spidev (el mismo patrón que la capa I2C del
 * proyecto) y GPIO de sysfs para controlar los pines DC y RESET. No depende de
 * librerías externas; funciona en Raspberry Pi OS 32 y 64 bits.
 *
 * @author Proyecto EyePet (Raspberry Pi)
 * @version 1.0.0
 */

#include "SSD1306_SPI.hpp"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <string>

#include <linux/spi/spidev.h>
#include <sys/ioctl.h>

namespace SSD1306LinuxSPI {

const char* SPI_DEV_PATH = "/dev/spidev0.0";
const char* GPIO_SYSFS_PATH = "/sys/class/gpio";

/**
 * @brief Devuelve la ruta sysfs del pin dado.
 * @param gpio Número de GPIO.
 * @param dirname "value", "direction", etc.
 * @return Cadena con la ruta completa.
 */
static std::string gpio_sysfs_path(unsigned gpio, const char* dirname) {
    char buf[128];
    snprintf(buf, sizeof(buf), "%s/gpio%u/%s", GPIO_SYSFS_PATH, gpio, dirname);
    return std::string(buf);
}

bool gpio_export(unsigned gpio) {
    // Intenta exportar el GPIO; si ya está exportado, el acceso fallará con
    // EEXIST y no es un error.
    int fd = open(std::string(GPIO_SYSFS_PATH).append("/export").c_str(), O_WRONLY);
    if (fd < 0) {
        perror("gpio_export: abrir /sys/class/gpio/export");
        return false;
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", gpio);
    if (write(fd, buf, strlen(buf)) < 0) {
        // En Linux, si ya está exportado devuelve -1 con EEXIST; se ignora.
        if (errno == EBUSY || errno == EEXIST) {
            close(fd);
            return true;
        }
        perror("gpio_export: write");
        close(fd);
        return false;
    }
    close(fd);
    return true;
}

bool gpio_direction(unsigned gpio, const char* dir) {
    std::string path = gpio_sysfs_path(gpio, "direction");
    int fd = open(path.c_str(), O_WRONLY);
    if (fd < 0) {
        perror("gpio_direction: open");
        return false;
    }
    ssize_t n = write(fd, dir, strlen(dir));
    close(fd);
    return n >= 0;
}

bool gpio_write(unsigned gpio, int value) {
    std::string path = gpio_sysfs_path(gpio, "value");
    int fd = open(path.c_str(), O_WRONLY);
    if (fd < 0) {
        perror("gpio_write: open");
        return false;
    }
    char c = value ? '1' : '0';
    ssize_t n = write(fd, &c, 1);
    close(fd);
    return n == 1;
}

void gpio_unexport(unsigned gpio) {
    int fd = open(std::string(GPIO_SYSFS_PATH).append("/unexport").c_str(), O_WRONLY);
    if (fd < 0) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", gpio);
    if (write(fd, buf, strlen(buf)) < 0) { /* ignorar si ya estaba liberado */ }
    close(fd);
}

int spi_open(const char* device, uint32_t speed) {
    int fd = open(device, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "SPI: no se pudo abrir '%s': %s\n", device, strerror(errno));
        return -1;
    }

    // Modo SPI 0 (CPOL=0, CPHA=0), el que usa el SSD1306 por defecto.
    uint8_t mode = SPI_MODE_0;
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) {
        perror("SPI: SPI_IOC_WR_MODE");
        close(fd);
        return -1;
    }
    uint8_t bits = 8;
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        perror("SPI: SPI_IOC_WR_BITS_PER_WORD");
        close(fd);
        return -1;
    }
    uint32_t maxSpeed = speed;
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &maxSpeed) < 0) {
        perror("SPI: SPI_IOC_WR_MAX_SPEED_HZ");
        close(fd);
        return -1;
    }
    return fd;
}

int spi_transfer_byte(int fd, unsigned dc, uint8_t value, bool isData) {
    if (fd < 0) return -1;

    // Controla el pin DC: bajo = comando, alto = dato.
    if (!gpio_write(dc, isData ? 1 : 0)) {
        return -1;
    }

    struct spi_ioc_transfer tr;
    memset(&tr, 0, sizeof(tr));
    tr.tx_buf = (uint64_t)&value;
    tr.rx_buf = 0;
    tr.len = 1;
    tr.speed_hz = 0; // usar el configurado en el bus
    tr.bits_per_word = 8;
    tr.delay_usecs = 0;

    int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1) {
        fprintf(stderr, "SPI: error en transferencia ioctl: %s\n", strerror(errno));
        return -1;
    }
    return ret;
}

void spi_close(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}

} // namespace SSD1306LinuxSPI