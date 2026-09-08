/**
 * @file Eye_t.hpp
 * @brief Clase principal del ojo de mascota (emulación en OLED SSD1306).
 *
 * @details Define la clase Eye::Eye_t que gestiona la animación completa de un
 * ojo estilizado de mascota sobre una pantalla OLED SSD1306 (128x64). Contiene
 * todos los modos de comportamiento (normal, parpadeo, seguimiento, expresiones,
 * movimientos sacádicos, dormido y brillo) así como la configuración cargada desde
 * config/config.cfg y config/hardware.cfg.
 *
 * El bucle principal run() es no bloqueante: actualiza la animación del ojo en
 * un buffer de memoria y vuelca el resultado a la pantalla a un ritmo controlado,
 * permitiendo movimientos suaves, parpadeos y expresiones.
 *
 * @author Proyecto EyePet (Raspberry Pi)
 * @version 1.0.0
 */

#ifndef EYE_T_HPP
#define EYE_T_HPP

#include <memory>
#include <cstdint>
#include <string>
#include <vector>

// Interfaz abstracta del display (permite usar I2C o SPI según hardware.cfg).
#include "IDisplay.hpp"

/**
 * @namespace Eye
 * @brief Espacio de nombres que agrupa los componentes del ojo de mascota.
 */
namespace Eye {

/**
 * @enum EyeMode_e
 * @brief Modos de comportamiento del ojo.
 */
enum class EyeMode_e : int {
    Normal   = 0, ///< Ojo abierto explorando al frente.
    Tracking = 1, ///< Seguimiento de un objetivo simulado.
    Happy    = 2, ///< Expresión de alegría (ceja arqueada, pupila dilatada).
    Surprised= 3, ///< Expresión de sorpresa (pupila pequeña, ojo muy abierto).
    Angry    = 4, ///< Expresión de enfado (ceja fruncida).
    Sleepy   = 5, ///< Expresión de sueño (párpado a medio cerrar).
    Sleep    = 6, ///< Modo dormido (ojo cerrado con línea).
    Saccades = 7  ///< Movimientos sacádicos rápidos.
};

/**
 * @enum EyeStyle_e
 * @brief Estilo (versión) de dibujo del ojo.
 *
 * @details Cada estilo cambia la forma de la esclera, el iris, la pupila y el
 * patrón de brillo, permitiendo emular ojos muy distintos sobre el mismo OLED
 * monocromo de 128x64.
 */
enum class EyeStyle_e : int {
    Classic = 0, ///< Ojo clásico redondeado (pupila circular, un brillo).
    Anime   = 1, ///< Estilo anime: elipse grande, pupila amplia y 2 brillos.
    Feline  = 2, ///< Felino: pupila vertical alargada, iris marcado.
    Robot   = 3  ///< Robótico: esclera rectangular, pupila cuadrada de rejilla.
};

/**
 * @struct EyeConfig_t
 * @brief Estructura con todos los parámetros de configuración del ojo.
 *
 * @details Se carga desde config/config.cfg y config/hardware.cfg mediante
 * Eye::loadConfig(). Incluye parámetros de velocidad, parpadeo, modo inicial,
 * geometría del ojo, protocolo de hardware y opciones de renderizado.
 */
struct EyeConfig_t {
    // --- Protocolo de hardware (config/hardware.cfg) ---
    std::string protocol    = "i2c";     ///< "i2c" o "spi".
    std::string i2c_device  = "/dev/i2c-1";
    unsigned    i2c_address = 0x3C;      ///< Dirección I2C (0x3C o 0x3D).
    std::string spi_device  = "/dev/spidev0.0";
    unsigned    spi_speed   = 1000000;   ///< Velocidad SPI en Hz.
    unsigned    spi_dc_pin  = 24;        ///< Pin GPIO DC/reset (hardware SPI).
    unsigned    spi_rst_pin = 25;        ///< Pin GPIO RESET.
    bool        spi_reset_active_high = false;

    // --- Modo y velocidad (config/config.cfg) ---
    int       mode        = static_cast<int>(EyeMode_e::Normal); ///< Modo inicial.
    int       style       = static_cast<int>(EyeStyle_e::Classic); ///< Estilo del ojo.
    float     frameRate   = 30.0f;       ///< Fotogramas por segundo.
    int       frameDelayMs= 33;          ///< Delay por fotograma (ms).
    unsigned  moveRangeX  = 14;          ///< Rango horizontal de la pupila.
    unsigned  moveRangeY  = 8;           ///< Rango vertical de la pupila.

    // --- Parpadeo ---
    unsigned  blinkMinMs = 1500;   ///< Intervalo mínimo entre parpadeos (ms).
    unsigned  blinkMaxMs = 4500;   ///< Intervalo máximo entre parpadeos (ms).
    unsigned  blinkCloseMs = 90;   ///< Tiempo para cerrar el párpado (ms).
    unsigned  blinkOpenMs  = 90;   ///< Tiempo para abrir el párpado (ms).
    unsigned  blinkClosedHoldMs = 100; ///< Tiempo con el ojo cerrado (ms).

    // --- Geometría del ojo (ocupa toda la pantalla 128x64) ---
    int16_t eyeCenterX = 64;  ///< Centro del ojo en X.
    int16_t eyeCenterY = 32;  ///< Centro del ojo en Y.
    int16_t scleraR    = 30;  ///< Radio de la esclera.
    int16_t irisR      = 16;  ///< Radio del iris.
    int16_t pupilR     = 6;   ///< Radio de la pupila.

    // --- Brillo / reflejo ---
    bool    glint = true;      ///< Mostrar punto de brillo.
    int16_t glintDX = -6;      ///< Offset X del brillo respecto al centro del iris.
    int16_t glintDY = -5;      ///< Offset Y del brillo.
    uint8_t glintR = 2;        ///< Radio del brillo.

    // --- Expresiones ---
    bool    drawEyebrows = true; ///< Dibujar cejas.

    // --- Otras opciones ---
    bool    debug = false;     ///< Salida de depuración por consola.
};

/**
 * @class Eye_t
 * @brief Motor de animación del ojo de mascota.
 *
 * @details Encapsula el ciclo de vida del display OLED, la carga de la
 * configuración y el bucle de animación no bloqueante que dibuja el ojo en el
 * buffer y lo vuelca a la pantalla.
 */
class Eye_t {
public:
    /** @brief Ancho del display en píxeles. */
    static constexpr int16_t OLED_WIDTH  = 128;
    /** @brief Alto del display en píxeles. */
    static constexpr int16_t OLED_HEIGHT = 64;

    /**
     * @brief Constructor de la clase.
     * @details Crea la instancia del display OLED y carga la configuración.
     */
    Eye_t();

    /**
     * @brief Destructor de la clase.
     * @details Apaga la pantalla OLED antes de liberar la memoria.
     */
    ~Eye_t();

    // Elimina copia y asignación para evitar dobles liberaciones del display.
    Eye_t(const Eye_t&) = delete;
    Eye_t& operator=(const Eye_t&) = delete;

    /**
     * @brief Ejecuta el bucle principal de animación del ojo.
     * @details Parsea los argumentos de línea de comandos (--version,
     * --config, --hw-config, --mode), carga la configuración e inicia el
     * bucle de animación no bloqueante.
     * @param argc Número de argumentos de la línea de comandos.
     * @param argv Vector de argumentos de la línea de comandos.
     * @return Código de salida (0 = éxito).
     */
    int run(int argc, char* argv[]);

    /**
     * @brief Devuelve la versión de la aplicación compilada.
     * @return Cadena con el número de versión.
     */
    std::string version() const;

private:
    // --- Inicialización y ciclo ---
    bool initHardware();       ///< Inicializa el display según protocolo configurado.
    void shutdownHardware();   ///< Apaga el display de forma segura.
    int  parseArgs(int argc, char* argv[]); ///< Parsea argumentos de CLI.
    void printVersion() const; ///< Muestra la versión por consola.
    void printHelp() const;    ///< Muestra la ayuda de uso.
    void animationLoop();      ///< Bucle principal no bloqueante.

    // --- Dibujo del ojo ---
    void drawEye(float openAmount, int16_t pupilDX, int16_t pupilDY,
                 int16_t pupilR, int16_t browOffset);
    void clearScreenBuffer();

    // --- Variantes (estilos) del ojo ---
    void drawClassicEye(float openAmount, int16_t dx, int16_t dy,
                        int16_t r, int16_t browOffset);
    void drawAnimeEye(float openAmount, int16_t dx, int16_t dy,
                      int16_t r, int16_t browOffset);
    void drawFelineEye(float openAmount, int16_t dx, int16_t dy,
                       int16_t r, int16_t browOffset);
    void drawRobotEye(float openAmount, int16_t dx, int16_t dy,
                      int16_t r, int16_t browOffset);

    // --- Utilidades de dibujo geométrico ---
    void fillEllipseInto(int16_t cx, int16_t cy, int16_t rx, int16_t ry,
                         uint8_t color); ///< Elipse rellena por barrido horizontal.
    void drawEyelidsCurve(int16_t cx, int16_t cy, int16_t rx, int16_t ry,
                          float openF);  ///< Recorte curvo (blink/eclosionado).
    void drawEyelidsFlat(int16_t cx, int16_t cy, int16_t rx, int16_t ry,
                         float openF);   ///< Recorte recto (estilo robot).
    void drawClosedEyeLine(int16_t cx, int16_t cy, int16_t rx); ///< Línea de dormido.
    void drawSleepLine(int16_t cx, int16_t cy, int16_t rx);     ///< Línea + muesca.

    // --- Modos de comportamiento ---
    void updateNormal(float dtMs);
    void updateTracking(float dtMs);
    void updateSaccades(float dtMs);
    void updateSleep(float dtMs);

    // --- Utilidades de animación ---
    bool  shouldBlink(uint32_t nowMs); ///< Determina si toca parpadear.
    void  updateBlink(float dtMs);     ///< Anima el estado actual del párpado.
    void  addGlint(int16_t cx, int16_t cy); ///< Añade el punto de brillo.
    void  doSleepTwitch(float dtMs);   ///< Pequeño movimiento en modo dormido.

    // --- Configuración ---
    EyeConfig_t cfg_;                  ///< Configuración cargada.
    std::string configPath_ = "config/config.cfg";
    std::string hwConfigPath_ = "config/hardware.cfg";

    /** @brief Instancia del display OLED (I2C o SPI según configuración). */
    std::unique_ptr<IDisplay> display_;
    /** @brief Indica si el display se inicializó (apagado seguro en destructor). */
    bool hwReady_ = false;

    // --- Estado de animación ---
    uint32_t lastTickMs_ = 0;             ///< Último tick de tiempo simulado.
    float    blinkTimerMs_ = 0;           ///< Temporizador para el siguiente parpadeo.
    float    blinkStageTimerMs_ = 0;      ///< Temporizador de la fase ciega actual.
    float    blinkPhase_ = 0.0f;          ///< 1.0 abierto, 0.0 cerrado.
    bool     blinking_ = false;           ///< true si se está parpadeando.
    int      frameDelayMs_ = 33;          ///< Delay por fotograma (ms).
    int16_t  pupilDX_ = 0;                ///< Desplazamiento X actual de la pupila.
    int16_t  pupilDY_ = 0;                ///< Desplazamiento Y actual de la pupila.
    int16_t  targetDX_ = 0;               ///< Posición objetivo X de la pupila.
    int16_t  targetDY_ = 0;               ///< Posición objetivo Y de la pupila.
    float    moveSpeedPx_ = 0.02f;        ///< Velocidad de suavizado (px/ms).
    int16_t  browOffset_ = 0;             ///< Desplazamiento vertical de la ceja.
    int16_t  pupilRCurrent_ = 5;          ///< Radio actual de la pupila.
    float    sleepTimerMs_ = 0;           ///< Temporizador para modo dormido.
    uint32_t frameCounter_ = 0;           ///< Contador de fotogramas.
};

} // namespace Eye

#endif // EYE_T_HPP
