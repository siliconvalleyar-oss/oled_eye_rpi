/**
 * @file SSD1306_I2C.hpp
 * @brief Capa de acceso I2C vía /dev/i2c-N usando ioctl de Linux.
 *
 * @details Reemplaza el acceso I2C de la librería bcm2835, que no soporta
 * Raspberry Pi 5 / Compute Module 5 (chip RP1). Usar ioctl sobre /dev/i2c-N
 * (como hacen i2cset/i2cget) funciona en todas las Raspberry Pi.
 *
 * @author Proyecto BASIC (Raspberry Pi)
 * @version 1.1.0
 */

#ifndef SSD1306_I2C_HPP
#define SSD1306_I2C_HPP

#include <stdint.h>
#include <stdbool.h>

/**
 * @namespace SSD1306LinuxI2C
 * @brief Funciones de acceso I2C por /dev/i2c-N.
 */
namespace SSD1306LinuxI2C {

/** @brief Ruta del adaptador I2C por defecto. */
extern const char* I2C_DEV_PATH;

/**
 * @brief Abre el adaptador I2C y configura la dirección del esclavo.
 * @param device Ruta del adaptador (p.ej. "/dev/i2c-1").
 * @param address Dirección I2C del esclavo (p.ej. 0x3C).
 * @return Descriptor de archivo (>=0) o -1 en caso de error.
 */
int i2c_open(const char* device, uint8_t address);

/**
 * @brief Cambia la dirección del esclavo en un adaptador ya abierto.
 * @param fd Descriptor del adaptador.
 * @param address Nueva dirección I2C.
 * @return true si tuvo éxito.
 */
bool i2c_set_address(int fd, uint8_t address);

/**
 * @brief Escribe un byte (o un comando) al dispositivo I2C.
 * @param fd Descriptor del adaptador.
 * @param value Byte de datos.
 * @param cmd Byte de control (0x00 comando, 0x40 dato) o modo continuo.
 * @return true si el dispositivo respondió (ACK).
 */
bool i2c_write_byte(int fd, uint8_t value, uint8_t cmd);

/**
 * @brief Cierra el adaptador I2C.
 * @param fd Descriptor del adaptador a cerrar.
 */
void i2c_close(int fd);

} // namespace SSD1306LinuxI2C

#endif // SSD1306_I2C_HPP
