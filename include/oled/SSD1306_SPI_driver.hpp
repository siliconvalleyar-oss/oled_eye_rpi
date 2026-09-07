/**
 * @file SSD1306_SPI.hpp
 * @brief Driver del display OLED SSD1306 en modo SPI (4 hilos).
 *
 * @details Clase equivalente a SSD1306 (modo I2C) pero configurada para operar
 * por el bus SPI: usa la misma capa gráfica SSD1306_graphics, el mismo buffer,
 * y el mismo juego de comandos del SSD1306, pero transmite por /dev/spidev
 * controlando el pin DC (datos/comando) y el pin RESET por GPIO (sysfs).
 *
 * La elección entre I2C y SPI se realiza en config/hardware.cfg (clave protocol).
 *
 * @author Proyecto EyePet (Raspberry Pi)
 * @version 1.0.0
 */

#ifndef SSD1306_SPI_HPP_DRIVER
#define SSD1306_SPI_HPP_DRIVER

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "SSD1306_OLED_graphics.hpp"
#include "SSD1306_SPI.hpp"

// Juego de comandos del SSD1306 (mismo que el driver I2C).
#define SSD1306_SET_CONTRAST_CONTROL                    0x81
#define SSD1306_DISPLAY_ALL_ON_RESUME                   0xA4
#define SSD1306_DISPLAY_ALL_ON                          0xA5
#define SSD1306_NORMAL_DISPLAY                          0xA6
#define SSD1306_INVERT_DISPLAY                          0xA7
#define SSD1306_DISPLAY_OFF                             0xAE
#define SSD1306_DISPLAY_ON                              0xAF
#define SSD1306_RIGHT_HORIZONTAL_SCROLL                 0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL                  0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL    0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL     0x2A
#define SSD1306_DEACTIVATE_SCROLL                       0x2E
#define SSD1306_ACTIVATE_SCROLL                         0x2F
#define SSD1306_SET_VERTICAL_SCROLL_AREA                0xA3
#define SSD1306_SET_LOWER_COLUMN                        0x00
#define SSD1306_SET_HIGHER_COLUMN                       0x10
#define SSD1306_MEMORY_ADDR_MODE                        0x20
#define SSD1306_SET_COLUMN_ADDR                         0x21
#define SSD1306_SET_PAGE_ADDR                           0x22
#define SSD1306_SET_START_LINE                          0x40
#define SSD1306_SET_SEGMENT_REMAP                       0xA0
#define SSD1306_SET_MULTIPLEX_RATIO                     0xA8
#define SSD1306_COM_SCAN_DIR_INC                        0xC0
#define SSD1306_COM_SCAN_DIR_DEC                        0xC8
#define SSD1306_SET_DISPLAY_OFFSET                      0xD3
#define SSD1306_SET_COM_PINS                            0xDA
#define SSD1306_CHARGE_PUMP                             0x8D
#define SSD1306_SET_DISPLAY_CLOCK_DIV_RATIO             0xD5
#define SSD1306_SET_PRECHARGE_PERIOD                    0xD9
#define SSD1306_SET_VCOM_DESELECT                       0xDB

#define SSPI_COMMAND 0 // DC bajo = comando
#define SSPI_DATA    1 // DC alto  = dato

class SSD1306_SPI : public SSD1306_graphics {
  public:
    /** @brief Constructor: toma dimensiones del display. */
    SSD1306_SPI(int16_t oledwidth, int16_t oledheight);
    /** @brief Destructor: libera el buffer y cierra el bus. */
    ~SSD1306_SPI();

    uint8_t* buffer = nullptr;

    virtual void drawPixel(int16_t x, int16_t y, uint8_t color) override;
    void OLEDupdate(void);
    void OLEDclearBuffer(void);

    /** @brief Inicializa el display y el bus SPI.
     *  @param spiDevice Ruta /dev/spidevX.Y.
     *  @param speed     Velocidad del bus en Hz.
     *  @param dcPin     GPIO de sysfs para DC.
     *  @param rstPin    GPIO de sysfs para RESET (0 = sin reset).
     *  @return true si todo se inició correctamente. */
    bool OLEDbegin(const char* spiDevice, uint32_t speed,
                   unsigned dcPin, unsigned rstPin);

    /** @brief Secuencia de inicialización de registros (tras el reset). */
    void OLEDinit(void);

    void OLEDPowerDown(void);
    void OLEDEnable(uint8_t on);
    void OLEDContrast(uint8_t contrast);
    void OLEDInvert(bool on);

    /** @brief Muestra el buffer en pantalla (page by page). */
    void OLEDBuffer(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t* data);

  private:
    bool SPI_Write_Byte(uint8_t value, uint8_t dc);
    void SPI_Reset(void);

    int _SPI_fd = -1;
    unsigned _SPI_DC_pin = 0;
    unsigned _SPI_RST_pin = 0;
    bool _gpio_dc_owned = false;
    bool _gpio_rst_owned = false;

    int16_t _OLED_WIDTH;
    int16_t _OLED_HEIGHT;
    int8_t _OLED_PAGE_NUM;
    uint8_t bufferWidth;
    uint8_t bufferHeight;
};

#endif // SSD1306_SPI_HPP_DRIVER