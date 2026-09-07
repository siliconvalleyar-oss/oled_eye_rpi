/**
 * @file DisplayAdapters.cpp
 * @brief Implementación de los adaptadores IDisplay (I2C y SPI).
 *
 * @details Envuelven los drivers SSD1306 y SSD1306_SPI para ofrecer la interfaz
 * común Eye::IDisplay al motor del ojo. Cada adaptador traslada llamadas de
 * primitivas gráficas y de control de pantalla al driver subyacente.
 *
 * @author Proyecto EyePet (Raspberry Pi)
 * @version 1.0.0
 */

#include "DisplayAdapters.hpp"

namespace Eye {

// ------------------------- Adaptador I2C -------------------------

DisplayAdapterI2C::DisplayAdapterI2C()
    : driver_(std::make_unique<SSD1306>(128, 64)) {}

void DisplayAdapterI2C::setI2CAddress(uint8_t address) {
    address_ = address;
}

bool DisplayAdapterI2C::begin() {
    // OLEDbegin(I2C_speed, address). La velocidad de bus la controla el kernel
    // (/boot/config.txt -> dtparam=i2c_arm_baudrate), así que se pasa 0.
    driver_->OLEDbegin(0, static_cast<uint8_t>(address_));
    driver_->OLEDclearBuffer();
    driver_->OLEDupdate();
    ready_ = true;
    return true;
}

void DisplayAdapterI2C::clearBuffer() {
    driver_->OLEDclearBuffer();
}

void DisplayAdapterI2C::update() {
    driver_->OLEDupdate();
}

void DisplayAdapterI2C::powerDown() {
    if (ready_) {
        driver_->OLEDPowerDown();
        ready_ = false;
    }
}

void DisplayAdapterI2C::setContrast(uint8_t contrast) {
    driver_->OLEDContrast(contrast);
}

void DisplayAdapterI2C::drawPixel(int16_t x, int16_t y, uint8_t color) {
    driver_->drawPixel(x, y, color);
}

void DisplayAdapterI2C::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                 uint8_t color) {
    driver_->drawLine(x0, y0, x1, y1, color);
}

void DisplayAdapterI2C::drawFastVLine(int16_t x, int16_t y, int16_t h, uint8_t color) {
    driver_->drawFastVLine(x, y, h, color);
}

void DisplayAdapterI2C::drawFastHLine(int16_t x, int16_t y, int16_t w, uint8_t color) {
    driver_->drawFastHLine(x, y, w, color);
}

void DisplayAdapterI2C::drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
                                 uint8_t color) {
    driver_->drawRect(x, y, w, h, color);
}

void DisplayAdapterI2C::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                                 uint8_t color) {
    driver_->fillRect(x, y, w, h, color);
}

void DisplayAdapterI2C::drawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color) {
    driver_->drawCircle(x0, y0, r, color);
}

void DisplayAdapterI2C::fillCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color) {
    driver_->fillCircle(x0, y0, r, color);
}

// ------------------------- Adaptador SPI -------------------------

DisplayAdapterSPI::DisplayAdapterSPI()
    : driver_(std::make_unique<SSD1306_SPI>(128, 64)) {}

void DisplayAdapterSPI::setSPIParams(const std::string& device, uint32_t speed,
                                     unsigned dcPin, unsigned rstPin) {
    device_ = device;
    speed_ = speed;
    dcPin_ = dcPin;
    rstPin_ = rstPin;
}

bool DisplayAdapterSPI::begin() {
    if (!driver_->OLEDbegin(device_.c_str(), speed_, dcPin_, rstPin_)) {
        fprintf(stderr, "DisplayAdapterSPI: no se pudo iniciar el display SPI\n");
        return false;
    }
    driver_->OLEDclearBuffer();
    driver_->OLEDupdate();
    ready_ = true;
    return true;
}

void DisplayAdapterSPI::clearBuffer() {
    driver_->OLEDclearBuffer();
}

void DisplayAdapterSPI::update() {
    driver_->OLEDupdate();
}

void DisplayAdapterSPI::powerDown() {
    if (ready_) {
        driver_->OLEDPowerDown();
        ready_ = false;
    }
}

void DisplayAdapterSPI::setContrast(uint8_t contrast) {
    driver_->OLEDContrast(contrast);
}

void DisplayAdapterSPI::drawPixel(int16_t x, int16_t y, uint8_t color) {
    driver_->drawPixel(x, y, color);
}

void DisplayAdapterSPI::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                 uint8_t color) {
    driver_->drawLine(x0, y0, x1, y1, color);
}

void DisplayAdapterSPI::drawFastVLine(int16_t x, int16_t y, int16_t h, uint8_t color) {
    driver_->drawFastVLine(x, y, h, color);
}

void DisplayAdapterSPI::drawFastHLine(int16_t x, int16_t y, int16_t w, uint8_t color) {
    driver_->drawFastHLine(x, y, w, color);
}

void DisplayAdapterSPI::drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
                                 uint8_t color) {
    driver_->drawRect(x, y, w, h, color);
}

void DisplayAdapterSPI::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                                 uint8_t color) {
    driver_->fillRect(x, y, w, h, color);
}

void DisplayAdapterSPI::drawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color) {
    driver_->drawCircle(x0, y0, r, color);
}

void DisplayAdapterSPI::fillCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color) {
    driver_->fillCircle(x0, y0, r, color);
}

} // namespace Eye