/**
 * @file IDisplay.hpp
 * @brief Interfaz abstracta de display para el motor del ojo.
 *
 * @details Define el contrato mínimo que necesita Eye::Eye_t para dibujar el
 * ojo: limpiar/actualizar el buffer, apagar el display y un conjunto de
 * primitivas gráficas (círculos, líneas, rectángulos, píxeles). Esto permite
 * usar indistintamente el driver I2C (SSD1306) o el driver SPI (SSD1306_SPI)
 * seleccionado en config/hardware.cfg.
 *
 * @author Proyecto EyePet (Raspberry Pi)
 * @version 1.0.0
 */

#ifndef IDISPLAY_HPP
#define IDISPLAY_HPP

#include <cstdint>

namespace Eye {

/**
 * @enum DisplayColor_e
 * @brief Colores disponibles para las primitivas gráficas.
 */
enum DisplayColor_e : uint8_t {
    DisplayBlack = 0,  ///< Píxel apagado.
    DisplayWhite = 1,  ///< Píxel encendido.
    DisplayInverse = 2 ///< Invierte el píxel actual.
};

/**
 * @class IDisplay
 * @brief Interfaz abstracta para el display OLED.
 *
 * @details Todas las operaciones de dibujo se realizan sobre el buffer de
 * pantalla en memoria; llamadas a update() vuelcan el buffer al hardware.
 */
class IDisplay {
public:
    virtual ~IDisplay() = default;

    /** @brief Inicializa el display y el bus (I2C o SPI según implementación).
     *  @return true si la inicialización fue correcta. */
    virtual bool begin() = 0;

    /** @brief Limpia el buffer de pantalla (no escribe en el hardware). */
    virtual void clearBuffer() = 0;

    /** @brief Vuelca el buffer de pantalla al hardware. */
    virtual void update() = 0;

    /** @brief Apaga el display de forma segura. */
    virtual void powerDown() = 0;

    /** @brief Ajusta el contraste del display (0x00..0xFF). */
    virtual void setContrast(uint8_t contrast) = 0;

    // --- Primitivas gráficas sobre el buffer ---

    /** @brief Dibuja un píxel en (x, y) con el color dado. */
    virtual void drawPixel(int16_t x, int16_t y, uint8_t color) = 0;

    /** @brief Dibuja una línea entre dos puntos. */
    virtual void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) = 0;

    /** @brief Dibuja una línea vertical rápida. */
    virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint8_t color) = 0;

    /** @brief Dibuja una línea horizontal rápida. */
    virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint8_t color) = 0;

    /** @brief Dibuja el contorno de un rectángulo. */
    virtual void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) = 0;

    /** @brief Rellena un rectángulo. */
    virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) = 0;

    /** @brief Dibuja el contorno de un círculo. */
    virtual void drawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color) = 0;

    /** @brief Rellena un círculo. */
    virtual void fillCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color) = 0;
};

} // namespace Eye

#endif // IDISPLAY_HPP