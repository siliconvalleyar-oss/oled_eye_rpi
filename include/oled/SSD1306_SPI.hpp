/**
 * @file SSD1306_SPI.hpp
 * @brief Capa de acceso SPI del SSD1306 vía /dev/spidev y GPIO por sysfs.
 *
 * @details Proporciona acceso al bus SPI de Linux (/dev/spidevX.Y) usando ioctl
 * (igual que los accesos I2C por /dev/i2c-N del proyecto). El pin DC (datos vs
 * comando) y el pin de reset se controlan a través de la interfaz GPIO de
 * sysfs (/sys/class/gpio), que no requiere librerías adicionales y funciona en
 * Raspberry Pi OS de 32 y 64 bits.
 *
 * @author Proyecto EyePet (Raspberry Pi)
 * @version 1.0.0
 */

#ifndef SSD1306_SPI_HPP
#define SSD1306_SPI_HPP

#include <stdint.h>
#include <stdbool.h>

/**
 * @namespace SSD1306LinuxSPI
 * @brief Funciones de acceso SPI y GPIO por sysfs.
 */
namespace SSD1306LinuxSPI {

/** @brief Ruta por defecto del dispositivo SPI (controlador 0, chip select 0). */
extern const char* SPI_DEV_PATH;

/** @brief Ruta base del sysfs de GPIO. */
extern const char* GPIO_SYSFS_PATH;

/**
 * @brief Exporta un GPIO del sysfs para poder trabajar con él.
 * @param pin Número de GPIO (numeración de la cabecera BCM).
 * @return true si se pudo exportar (o ya estaba exportado).
 */
bool gpio_export(unsigned gpio);

/**
 * @brief Configura la dirección de un GPIO (in/out).
 * @param gpio Número de GPIO.
 * @param dir  "in" o "out".
 * @return true si tuvo éxito.
 */
bool gpio_direction(unsigned gpio, const char* dir);

/**
 * @brief Escribe un valor lógico en un GPIO de salida.
 * @param gpio  Número de GPIO.
 * @param value 0 (bajo) o 1 (alto).
 * @return true si tuvo éxito.
 */
bool gpio_write(unsigned gpio, int value);

/**
 * @brief Libera (Unexport) un GPIO del sysfs.
 * @param gpio Número de GPIO.
 */
void gpio_unexport(unsigned gpio);

/**
 * @brief Abre el dispositivo SPI y configura el modo.
 * @param device Ruta del dispositivo SPI (p.ej. "/dev/spidev0.0").
 * @param speed  Velocidad máxima en Hz (p.ej. 1000000 = 1 MHz).
 * @return Descriptor de archivo (>=0) o -1 en caso de error.
 */
int spi_open(const char* device, uint32_t speed);

/**
 * @brief Envía un byte (comando o dato) por SPI con el pin DC adecuado.
 * @param fd   Descriptor del bus SPI.
 * @param dc   Pin DC configurado como GPIO.
 * @param value Byte a transmitir.
 * @param isData true si es dato (DC alto), false si es comando (DC bajo).
 * @return Número de bytes transmitidos o -1 en error.
 */
int spi_transfer_byte(int fd, unsigned dc, uint8_t value, bool isData);

/**
 * @brief Cierra el bus SPI.
 * @param fd Descriptor del bus SPI.
 */
void spi_close(int fd);

} // namespace SSD1306LinuxSPI

#endif // SSD1306_SPI_HPP