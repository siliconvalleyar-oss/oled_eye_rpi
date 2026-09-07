/**
 * @file DisplayAdapters.hpp
 * @brief Adaptadores del driver OLED a la interfaz Eye::IDisplay.
 *
 * @details Implementa Eye::IDisplay sobre los drivers SSD1306 (I2C) y
 * SSD1306_SPI (SPI) sin modificar dichos drivers. El adaptador elegido depende
 * de la clave "protocol" de config/hardware.cfg.
 *
 * @author Proyecto EyePet (Raspberry Pi)
 * @version 1.0.0
 */

#ifndef DISPLAYADAPTERS_HPP
#define DISPLAYADAPTERS_HPP

#include <memory>
#include <string>

#include "IDisplay.hpp"
#include "SSD1306_OLED.hpp"
#include "SSD1306_SPI_driver.hpp"

#ifdef swap
#undef swap
#endif

namespace Eye {

/**
 * @class DisplayAdapterI2C
 * @brief Adaptador del driver SSD1306 (I2C por /dev/i2c-N) a IDisplay.
 */
class DisplayAdapterI2C : public IDisplay {
public:
    /** @brief Construye el adaptador sobre el driver I2C. */
    DisplayAdapterI2C();

    /** @brief Configura la dirección I2C del display (p.ej. 0x3C o 0x3D). */
    void setI2CAddress(uint8_t address);

    bool begin() override;
    void clearBuffer() override;
    void update() override;
    void powerDown() override;
    void setContrast(uint8_t contrast) override;
    void drawPixel(int16_t x, int16_t y, uint8_t color) override;
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) override;
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint8_t color) override;
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint8_t color) override;
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) override;
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) override;
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color) override;
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color) override;

private:
    std::unique_ptr<SSD1306> driver_; ///< Driver I2C subyacente.
    uint8_t address_ = 0x3C;          ///< Dirección I2C del display.
    bool ready_ = false;              ///< Indica si el driver está iniciado.
};

/**
 * @class DisplayAdapterSPI
 * @brief Adaptador del driver SSD1306_SPI (SPI por /dev/spidev) a IDisplay.
 */
class DisplayAdapterSPI : public IDisplay {
public:
    /** @brief Construye el adaptador sobre el driver SPI. */
    DisplayAdapterSPI();

    /** @brief Configura el bus SPI (dispositivo, velocidad, DC y RESET). */
    void setSPIParams(const std::string& device, uint32_t speed,
                      unsigned dcPin, unsigned rstPin);

    bool begin() override;
    void clearBuffer() override;
    void update() override;
    void powerDown() override;
    void setContrast(uint8_t contrast) override;
    void drawPixel(int16_t x, int16_t y, uint8_t color) override;
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) override;
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint8_t color) override;
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint8_t color) override;
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) override;
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) override;
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color) override;
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color) override;

private:
    std::unique_ptr<SSD1306_SPI> driver_; ///< Driver SPI subyacente.
    std::string device_ = "/dev/spidev0.0"; ///< Bus SPI.
    uint32_t speed_ = 1000000;              ///< Velocidad del bus (Hz).
    unsigned dcPin_ = 24;                   ///< GPIO DC (sysfs).
    unsigned rstPin_ = 25;                  ///< GPIO RESET (sysfs).
    bool ready_ = false;                    ///< Indica si el driver está iniciado.
};

} // namespace Eye

#endif // DISPLAYADAPTERS_HPP