/**
 * @file Eye_t.cpp
 * @brief Implementación del motor de animación del ojo de mascota (Eye::Eye_t).
 *
 * @details Dibuja un ojo estilizado sobre el buffer del OLED SSD1306 y lo
 * actualiza en un bucle no bloqueante. Implementa los modos de comportamiento
 * solicitados en el prompt: Normal, Parpadeo, Seguimiento, Expresiones,
 * Movimientos sacádicos, Dormido y Efecto de brillo.
 *
 * @author Proyecto EyePet (Raspberry Pi)
 * @version 1.0.0
 */

#include "Eye_t.hpp"
#include "EyeConfig.hpp"
#include "DisplayAdapters.hpp"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <thread>
#include <chrono>

namespace Eye {

namespace {

/**
 * @brief Genera un número aleatorio en el rango [min, max].
 * @param min Límite inferior (inclusive).
 * @param max Límite superior (inclusive).
 * @return Número entero pseudoaleatorio.
 */
int randRange(int min, int max) {
    if (max <= min) return min;
    return min + (std::rand() % (max - min + 1));
}

/**
 * @brief Limita un valor al rango [0, 1].
 */
float clamp01(float v) {
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

/**
 * @brief Dibuja el iris con un patrón de anillos para dar textura monocroma.
 * @details El iris se rellena en blanco y luego se oscurecen anillos alternos
 * (dither) para distinguirlo de la esclera en un OLED de 1 bit.
 */
void drawIrisArea(IDisplay& d, int16_t cx, int16_t cy, int16_t irisR) {
    if (irisR <= 0) return;
    d.fillCircle(cx, cy, irisR, DisplayWhite);
    const float layer = std::max(1.0f, static_cast<float>(irisR) / 3.0f);
    for (int16_t y = cy - irisR; y <= cy + irisR; ++y) {
        for (int16_t x = cx - irisR; x <= cx + irisR; ++x) {
            const int16_t dx = x - cx;
            const int16_t dy = y - cy;
            if (dx * dx + dy * dy <= irisR * irisR) {
                const float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                if (static_cast<int>(dist / layer) % 2 == 0) {
                    d.drawPixel(x, y, DisplayBlack);
                }
            }
        }
    }
}

/**
 * @brief Dibuja la ceja según el modo (expresión) sobre el borde superior del ojo.
 * @details Con el ojo a pantalla completa, la ceja se dibuja encajada en la
 * parte superior de la esclera (browY = cy - ry + 1 + dy). Se conservan las
 * formas: arco (Happy), fruncida (Angry), elevada (Surprised) y neutra.
 */
void drawBrow(IDisplay& d, int mode, int16_t cx, int16_t cy, int16_t ry, int16_t dy) {
    const int16_t browY = cy - ry + 1 + dy;
    switch (static_cast<EyeMode_e>(mode)) {
        case EyeMode_e::Happy:
            d.drawLine(cx - 8, browY + 3, cx - 3, browY, DisplayBlack);
            d.drawLine(cx + 3, browY, cx + 8, browY + 3, DisplayBlack);
            break;
        case EyeMode_e::Angry:
            d.drawLine(cx - 8, browY, cx - 3, browY + 3, DisplayBlack);
            d.drawLine(cx + 3, browY + 3, cx + 8, browY, DisplayBlack);
            break;
        case EyeMode_e::Surprised:
            d.drawFastHLine(cx - 8, browY, 16, DisplayBlack);
            break;
        default:
            d.drawFastHLine(cx - 8, browY + 1, 16, DisplayBlack);
            break;
    }
}

} // namespace anon

// ------------------------- Constructor / Destructor ---------------------------

Eye_t::Eye_t() : pupilRCurrent_(cfg_.pupilR) {
    // Semilla para la generación de números aleatorios (movimientos y parpadeos).
    std::srand(static_cast<unsigned>(std::time(nullptr)));
}

Eye_t::~Eye_t() {
    shutdownHardware();
}

// ------------------------------- API pública ----------------------------------

std::string Eye_t::version() const {
    // VERSION se define en tiempo de compilación mediante -DVERSION="...".
    return std::string(VERSION);
}

void Eye_t::printVersion() const {
    printf("EyePet v%s\n", version().c_str());
}

void Eye_t::printHelp() const {
    printf("Uso: bin/App [opciones]\n"
           "Opciones:\n"
           "  --version, -v          Muestra la versión de EyePet y termina.\n"
           "  --mode <0..7>          Modo inicial del ojo:\n"
           "                           0 Normal, 1 Tracking, 2 Happy, 3 Surprised,\n"
           "                           4 Angry, 5 Sleepy, 6 Sleep, 7 Saccades.\n"
           "  --style <0..3>         Estilo (versión) del ojo:\n"
           "                           0 Classic, 1 Anime, 2 Feline, 3 Robot.\n"
           "  --config <archivo>     Ruta alternativa para config/config.cfg.\n"
           "  --hw-config <archivo>  Ruta alternativa para config/hardware.cfg.\n"
           "  --help, -h             Muestra esta ayuda.\n"
           "La mayoría de parámetros se ajustan en config/config.cfg.\n");
}

int Eye_t::parseArgs(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--version") == 0 || std::strcmp(argv[i], "-v") == 0) {
            // Solo muestra la versión y termina sin tocar el hardware.
            printVersion();
            return 1; // señal de terminar
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            printHelp();
            return 1; // señal de terminar
        } else if (std::strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            cfg_.mode = std::atoi(argv[++i]);
            // Valida el rango del modo.
            if (cfg_.mode < static_cast<int>(EyeMode_e::Normal) ||
                cfg_.mode > static_cast<int>(EyeMode_e::Saccades)) {
                fprintf(stderr, "EyePet: modo inválido '%d' (0..7)\n", cfg_.mode);
                cfg_.mode = static_cast<int>(EyeMode_e::Normal);
            }
        } else if (std::strcmp(argv[i], "--style") == 0 && i + 1 < argc) {
            cfg_.style = std::atoi(argv[++i]);
            // Valida el rango del estilo.
            if (cfg_.style < static_cast<int>(EyeStyle_e::Classic) ||
                cfg_.style > static_cast<int>(EyeStyle_e::Robot)) {
                fprintf(stderr, "EyePet: estilo inválido '%d' (0..3)\n", cfg_.style);
                cfg_.style = static_cast<int>(EyeStyle_e::Classic);
            }
        } else if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            configPath_ = argv[++i];
        } else if (std::strcmp(argv[i], "--hw-config") == 0 && i + 1 < argc) {
            hwConfigPath_ = argv[++i];
        } else {
            fprintf(stderr, "EyePet: argumento desconocido '%s' (use --help)\n", argv[i]);
        }
    }
    return 0; // continuar
}

/**
 * @brief Ejecuta el bucle principal de animación del ojo.
 * @details Parsea los argumentos de línea de comandos, muestra la versión,
 * carga la configuración e inicia el bucle de animación no bloqueante.
 * @param argc Número de argumentos de la línea de comandos.
 * @param argv Vector de argumentos de la línea de comandos.
 * @return Código de salida (0 = éxito, 1 = terminación solicitada).
 */
int Eye_t::run(int argc, char* argv[]) {
    // Parsea argumentos de línea de comandos (--version, --mode, --config...).
    if (parseArgs(argc, argv) != 0) {
        return 0;
    }

    // Muestra la versión de la aplicación al iniciar.
    printVersion();

    // Carga la configuración desde los archivos de configuración.
    loadHardwareConfig(cfg_, hwConfigPath_);
    loadGeneralConfig(cfg_, configPath_);

    if (!initHardware()) {
        fprintf(stderr, "EyePet: no se pudo inicializar la pantalla OLED\n");
        return 1;
    }

    animationLoop();
    return 0;
}

// --------------------------- Inicialización de HW -----------------------------

bool Eye_t::initHardware() {
    // Valida el protocolo configurado en config/hardware.cfg.
    bool useI2C = (cfg_.protocol == "i2c");
    if (!useI2C && cfg_.protocol != "spi") {
        fprintf(stderr, "EyePet: protocolo desconocido '%s' (se esperaba 'i2c' o 'spi')\n",
                cfg_.protocol.c_str());
        return false;
    }

    if (useI2C) {
        // Adaptador I2C: acceso por /dev/i2c-N (capa SSD1306_I2C).
        auto adapter = std::make_unique<DisplayAdapterI2C>();
        adapter->setI2CAddress(static_cast<uint8_t>(cfg_.i2c_address));
        if (!adapter->begin()) {
            fprintf(stderr, "EyePet: no se pudo iniciar el display I2C (0x%02X)\n",
                    static_cast<unsigned>(cfg_.i2c_address));
            return false;
        }
        display_ = std::move(adapter);
    } else {
        // Adaptador SPI: acceso por /dev/spidev + GPIO (sysfs) para DC/RESET.
        auto adapter = std::make_unique<DisplayAdapterSPI>();
        adapter->setSPIParams(cfg_.spi_device, cfg_.spi_speed,
                              cfg_.spi_dc_pin, cfg_.spi_rst_pin);
        if (!adapter->begin()) {
            fprintf(stderr,
                    "EyePet: no se pudo iniciar el display SPI (%s, %u Hz, DC=%u, RST=%u)\n",
                    cfg_.spi_device.c_str(), cfg_.spi_speed, cfg_.spi_dc_pin, cfg_.spi_rst_pin);
            return false;
        }
        display_ = std::move(adapter);
    }

    hwReady_ = true;
    return true;
}

void Eye_t::shutdownHardware() {
    // Apaga la pantalla solo si se inicializó (evita tocar el bus sin init).
    if (hwReady_ && display_) {
        display_->powerDown();
        hwReady_ = false;
    }
}

// ------------------------------ Bucle principal -------------------------------

void Eye_t::animationLoop() {
    // Ajusta el delay por fotograma según la tasa configurada.
    frameDelayMs_ = cfg_.frameDelayMs > 0 ? cfg_.frameDelayMs
                                          : static_cast<int>(1000.0f / cfg_.frameRate);
    if (frameDelayMs_ < 1) frameDelayMs_ = 1;

    lastTickMs_ = 0;
    // Tiempo hasta el primer parpadeo.
    blinkTimerMs_ = static_cast<float>(randRange(
        static_cast<int>(cfg_.blinkMinMs), static_cast<int>(cfg_.blinkMaxMs)));

    auto lastReal = std::chrono::steady_clock::now();

    // Bucle no bloqueante: calcula un dt (ms), actualiza la animación, dibuja y
    // espera el delay restante para mantener una cadencia estable.
    while (true) {
        auto nowReal = std::chrono::steady_clock::now();
        float dtMs = std::chrono::duration<float, std::milli>(nowReal - lastReal).count();
        lastReal = nowReal;
        ++frameCounter_;

        // Genera una posición objetivo periódicamente para movimiento de exploración.
        updateBlink(dtMs);

        // Actualiza el modo actual.
        switch (static_cast<EyeMode_e>(cfg_.mode)) {
            case EyeMode_e::Tracking:  updateTracking(dtMs); break;
            case EyeMode_e::Saccades:  updateSaccades(dtMs); break;
            case EyeMode_e::Sleep:     updateSleep(dtMs);    break;
            case EyeMode_e::Happy:
            case EyeMode_e::Surprised:
            case EyeMode_e::Angry:
            case EyeMode_e::Sleepy:
                // Para expresiones fijas, el dibujo se resuelve en drawEye con
                // los offsets configurados; no hay movimiento puro.
                break;
            case EyeMode_e::Normal:
            default:                   updateNormal(dtMs);   break;
        }

        // Dibuja el ojo en el buffer con el estado actual.
        float open = cfg_.mode == static_cast<int>(EyeMode_e::Sleep) ? 0.0f : 1.0f;
        drawEye(open, pupilDX_, pupilDY_, pupilRCurrent_, browOffset_);

        // Actualiza el brillo (depende de la posición de la pupila). Solo se
        // dibuja si el ojo está suficientemente abierto (no en el parpadeo).
        if (cfg_.glint && blinkPhase_ > 0.5f)
            addGlint(cfg_.eyeCenterX + pupilDX_ + cfg_.glintDX,
                     cfg_.eyeCenterY + pupilDY_ + cfg_.glintDY);

        // Vuelca el buffer final a la pantalla.
        display_->update();

        if (cfg_.debug && (frameCounter_ % 30 == 0)) {
            printf("[EyePet] frame=%u mode=%d pupil=(%d,%d) blink=%.2f\n",
                   frameCounter_, cfg_.mode, pupilDX_, pupilDY_, blinkPhase_);
        }

        // Espera para mantener la cadencia de fotogramas.
        std::this_thread::sleep_for(std::chrono::milliseconds(frameDelayMs_));
    }
}

// ---------------------------- Actualización de modos --------------------------

bool Eye_t::shouldBlink(uint32_t nowMs) {
    (void)nowMs;
    return false; // La lógica de parpadeo se gestiona en updateBlink.
}

void Eye_t::updateNormal(float dtMs) {
    // Cada cierto tiempo, elige un nuevo objetivo suave para la pupila.
    static float exploreTimer = 0.0f;
    exploreTimer += dtMs;
    if (exploreTimer > 500.0f) { // nuevo objetivo cada ~0.5 s
        exploreTimer = 0.0f;
        targetDX_ = static_cast<int16_t>(randRange(
            -static_cast<int>(cfg_.moveRangeX), static_cast<int>(cfg_.moveRangeX)));
        targetDY_ = static_cast<int16_t>(randRange(
            -static_cast<int>(cfg_.moveRangeY), static_cast<int>(cfg_.moveRangeY)));
    }
    // Suavizado: aproxima la pupila al objetivo.
    pupilDX_ += static_cast<int16_t>((targetDX_ - pupilDX_) * moveSpeedPx_ * dtMs);
    pupilDY_ += static_cast<int16_t>((targetDY_ - pupilDY_) * moveSpeedPx_ * dtMs);

    // Estado estándar: párpado abierto, pupila y ceja neutras.
    pupilRCurrent_ = cfg_.pupilR;
    browOffset_ = 0;
}

void Eye_t::updateTracking(float dtMs) {
    // Simula el seguimiento de un objeto que se mueve en un patrón circular.
    static float angle = 0.0f;
    const float step = 0.002f * dtMs; // rotación en radianes por ms
    angle += step;
    float radiusX = cfg_.moveRangeX;
    float radiusY = cfg_.moveRangeY;
    targetDX_ = static_cast<int16_t>(std::sin(angle) * radiusX);
    targetDY_ = static_cast<int16_t>(std::cos(angle) * radiusY * 0.7f);
    pupilDX_ += static_cast<int16_t>((targetDX_ - pupilDX_) * 0.10f * dtMs / 16.0f);
    pupilDY_ += static_cast<int16_t>((targetDY_ - pupilDY_) * 0.10f * dtMs / 16.0f);

    // Al mirar, el párpado se mantiene abierto y la ceja ligeramente arqueada.
    pupilRCurrent_ = cfg_.pupilR;
    browOffset_ = -2;
}

void Eye_t::updateSaccades(float dtMs) {
    // Movimientos sacádicos: la pupila salta bruscamente entre posiciones.
    static float holdTimer = 0.0f;
    static float dwellMs = 120.0f; // milisegundos entre sácadas.
    holdTimer += dtMs;
    if (holdTimer > dwellMs) {
        holdTimer = 0.0f;
        dwellMs = static_cast<float>(randRange(60, 180));
        targetDX_ = static_cast<int16_t>(randRange(
            -static_cast<int>(cfg_.moveRangeX), static_cast<int>(cfg_.moveRangeX)));
        targetDY_ = static_cast<int16_t>(randRange(
            -static_cast<int>(cfg_.moveRangeY), static_cast<int>(cfg_.moveRangeY)));
    }
    // Avanza muy rápido hacia el objetivo (sácada).
    pupilDX_ += static_cast<int16_t>((targetDX_ - pupilDX_) * 0.5f);
    pupilDY_ += static_cast<int16_t>((targetDY_ - pupilDY_) * 0.5f);
    pupilRCurrent_ = cfg_.pupilR;
    browOffset_ = 0;
}

void Eye_t::updateSleep(float dtMs) {
    // Modo dormido: el ojo está cerrado. Ocasionalmente un pequeño "temblor".
    static float twitchTimer = 0.0f;
    static int prevTwitch = 0;
    twitchTimer += dtMs;
    if (twitchTimer > 3500.0f) { // temblor cada ~3.5 s
        twitchTimer = 0.0f;
        prevTwitch = randRange(0, 10);
        if (prevTwitch < 4) {
            // Pequeño movimiento de la pupila (repentino) simulando el sueño.
            targetDX_ = static_cast<int16_t>(randRange(-4, 4));
            targetDY_ = static_cast<int16_t>(randRange(-2, 3));
            pupilDX_ += static_cast<int16_t>((targetDX_ - pupilDX_) * 0.4f);
            pupilDY_ += static_cast<int16_t>((targetDY_ - pupilDY_) * 0.4f);
        }
    } else {
        pupilDX_ = 0; pupilDY_ = 0;
    }
    // Párpado completamente cerrado.
    blinkPhase_ = 0.0f;
    pupilRCurrent_ = 0;
    browOffset_ = 0;
}

void Eye_t::updateBlink(float dtMs) {
    // Controla la animación cíclica del parpadeo (cerrar y abrir).
    if (!blinking_) {
        blinkTimerMs_ -= dtMs;
        if (blinkTimerMs_ <= 0.0f) {
            blinking_ = true;
            blinkPhase_ = 1.0f;
            // Fase de cierre.
            blinkStageTimerMs_ = static_cast<float>(cfg_.blinkCloseMs);
        }
    } else {
        // Fases: cerrar -> mantener cerrado -> abrir.
        blinkStageTimerMs_ -= dtMs;
        if (blinkStageTimerMs_ <= 0.0f) {
            if (cfg_.mode != static_cast<int>(EyeMode_e::Sleep)) {
                // Abre de nuevo.
                blinkStageTimerMs_ = static_cast<float>(cfg_.blinkOpenMs);
                blinking_ = false;
                blinkPhase_ = 1.0f;
                blinkTimerMs_ = static_cast<float>(randRange(
                    static_cast<int>(cfg_.blinkMinMs),
                    static_cast<int>(cfg_.blinkMaxMs)));
            }
        }
    }

    // Anima la apertura del párpado: 1.0 (abierto) -> 0.0 (cerrado).
    if (!blinking_) {
        // No se parpadea: párpado abierto salvo en modo dormido.
        if (cfg_.mode != static_cast<int>(EyeMode_e::Sleep)) {
            blinkPhase_ = 1.0f;
        } else {
            blinkPhase_ = 0.0f;
        }
    }
}

void Eye_t::doSleepTwitch(float dtMs) {
    (void)dtMs; // Nota: la lógica del temblor está en updateSleep.
}

// ------------------------------- Dibujo del ojo -------------------------------

void Eye_t::clearScreenBuffer() {
    display_->clearBuffer();
}

void Eye_t::addGlint(int16_t cx, int16_t cy) {
    // Brillo según el estilo del ojo.
    switch (static_cast<EyeStyle_e>(cfg_.style)) {
        case EyeStyle_e::Anime:
            // Dos brillos: uno grande arriba a la izquierda y otro pequeño.
            display_->fillCircle(cx, cy, 3, DisplayWhite);
            display_->fillCircle(cx + 8, cy + 8, 1, DisplayWhite);
            break;
        case EyeStyle_e::Feline:
            // Brill anticlástico: pequeña línea vertical.
            display_->drawFastVLine(cx, cy, 3, DisplayWhite);
            break;
        case EyeStyle_e::Robot:
            // Círculo + línea de escaneo horizontal.
            display_->fillCircle(cx, cy, 2, DisplayWhite);
            display_->drawFastHLine(cx - 6, cy - 1, 3, DisplayWhite);
            break;
        case EyeStyle_e::Classic:
        default:
            display_->fillCircle(cx, cy, cfg_.glintR, DisplayWhite);
            break;
    }
}

// -----------------------------------------------------------------------------
// Geometría de ayuda (elipses y párpados)
// -----------------------------------------------------------------------------

void Eye_t::fillEllipseInto(int16_t cx, int16_t cy, int16_t rx, int16_t ry,
                            uint8_t color) {
    if (rx <= 0 || ry <= 0) return;
    for (int16_t y = cy - ry; y <= cy + ry; ++y) {
        const int16_t dy = y - cy;
        const float frac = 1.0f - static_cast<float>(dy * dy) /
                                        static_cast<float>(ry * ry);
        if (frac < 0.0f) continue;
        const int16_t xh = static_cast<int16_t>(static_cast<float>(rx) *
                                        std::sqrt(frac));
        display_->drawFastHLine(cx - xh, y, xh * 2 + 1, color);
    }
}

void Eye_t::drawEyelidsCurve(int16_t cx, int16_t cy, int16_t rx, int16_t ry,
                             float openF) {
    if (rx <= 0 || ry <= 0) return;
    if (openF <= 0.05f) { drawClosedEyeLine(cx, cy, rx); return; }

    const int16_t halfEye = static_cast<int16_t>(static_cast<float>(ry) * openF);
    for (int16_t x = cx - rx; x <= cx + rx; ++x) {
        const int16_t dx = x - cx;
        const float frac = 1.0f - static_cast<float>(dx * dx) /
                                        static_cast<float>(rx * rx);
        if (frac < 0.0f) continue;
        const int16_t halfH = static_cast<int16_t>(static_cast<float>(ry) *
                                        std::sqrt(frac));
        const int16_t topY = cy - halfH;
        const int16_t botY = cy + halfH;
        const int16_t lidTop = cy - halfEye;
        const int16_t lidBot = cy + halfEye;
        if (lidTop > topY) display_->drawFastVLine(x, topY, lidTop - topY, DisplayBlack);
        if (lidBot < botY) display_->drawFastVLine(x, lidBot, botY - lidBot, DisplayBlack);
    }
    display_->drawFastHLine(cx - rx, cy - halfEye, rx * 2, DisplayBlack);
    display_->drawFastHLine(cx - rx, cy + halfEye, rx * 2, DisplayBlack);
}

void Eye_t::drawEyelidsFlat(int16_t cx, int16_t cy, int16_t rx, int16_t ry,
                            float openF) {
    if (rx <= 0 || ry <= 0) return;
    if (openF <= 0.05f) { drawClosedEyeLine(cx, cy, rx); return; }

    const int16_t halfEye = static_cast<int16_t>(static_cast<float>(ry) * openF);
    // Párpado superior (recto) y párpado inferior.
    display_->fillRect(cx - rx, cy - ry - ry, rx * 2, (ry - halfEye) + ry, DisplayBlack);
    display_->fillRect(cx - rx, cy + halfEye, rx * 2, ry - halfEye, DisplayBlack);
    display_->drawFastHLine(cx - rx, cy - halfEye, rx * 2, DisplayBlack);
    display_->drawFastHLine(cx - rx, cy + halfEye, rx * 2, DisplayBlack);
}

void Eye_t::drawClosedEyeLine(int16_t cx, int16_t cy, int16_t rx) {
    // Ojo cerrado: arco suave (∪) con los rabillos algo elevados y el centro
    // bajo, como un párpado cerrado relajado.
    const float a = std::max(2.0f, static_cast<float>(rx) / 6.0f);
    for (int16_t x = cx - rx; x <= cx + rx; ++x) {
        const float t = static_cast<float>(x - cx) / static_cast<float>(rx);
        const int16_t y = cy + static_cast<int16_t>(a * (1.0f - t * t));
        display_->drawPixel(x, y, DisplayBlack);
        display_->drawPixel(x, y + 1, DisplayBlack);
    }
}

void Eye_t::drawSleepLine(int16_t cx, int16_t cy, int16_t rx) {
    // Párpado cerrado con suaves muescas en los extremos (aspecto relajado).
    const float a = std::max(2.0f, static_cast<float>(rx) / 6.0f);
    for (int16_t x = cx - rx; x <= cx + rx; ++x) {
        const float t = static_cast<float>(x - cx) / static_cast<float>(rx);
        const int16_t y = cy + static_cast<int16_t>(a * (1.0f - t * t));
        display_->drawPixel(x, y, DisplayBlack);
        display_->drawPixel(x, y + 1, DisplayBlack);
    }
}

// -----------------------------------------------------------------------------
// Variantes del ojo (estilos)
// -----------------------------------------------------------------------------

void Eye_t::drawClassicEye(float openAmount, int16_t dx, int16_t dy,
                           int16_t r, int16_t browOffset) {
    const int16_t cx = cfg_.eyeCenterX;
    const int16_t cy = cfg_.eyeCenterY;
    const int16_t R = cfg_.scleraR;

    // Esclera circular.
    display_->fillCircle(cx, cy, R, DisplayWhite);

    if (openAmount <= 0.05f) {
        if (cfg_.drawEyebrows) drawBrow(*display_, cfg_.mode, cx, cy, R, browOffset);
        drawClosedEyeLine(cx, cy, R);
        return;
    }

    const int16_t irisCX = cx + dx;
    const int16_t irisCY = cy + dy;
    drawIrisArea(*display_, irisCX, irisCY, cfg_.irisR);
    if (r > 0) display_->fillCircle(irisCX, irisCY, r, DisplayBlack);

    // Párpados que recortan el ojo según la apertura (curva).
    drawEyelidsCurve(cx, cy, R, R, openAmount);
    display_->drawCircle(cx, cy, R, DisplayBlack);

    if (cfg_.drawEyebrows) drawBrow(*display_, cfg_.mode, cx, cy, R, browOffset);
}

void Eye_t::drawAnimeEye(float openAmount, int16_t dx, int16_t dy,
                         int16_t r, int16_t browOffset) {
    const int16_t cx = cfg_.eyeCenterX;
    const int16_t cy = cfg_.eyeCenterY;
    const int16_t R = cfg_.scleraR;
    const int16_t rx = R + 2;   // elipse más ancha
    const int16_t ry = R - 1;

    // Esclera elíptica grande (ocupa la pantalla).
    fillEllipseInto(cx, cy, rx, ry, DisplayWhite);

    if (openAmount <= 0.05f) {
        if (cfg_.drawEyebrows) drawBrow(*display_, cfg_.mode, cx, cy, R, browOffset);
        drawSleepLine(cx, cy, rx);
        return;
    }

    const int16_t irisCX = cx + dx;
    const int16_t irisCY = cy + dy;
    const int16_t irisR = static_cast<int16_t>(cfg_.irisR * 1.15f);
    drawIrisArea(*display_, irisCX, irisCY, irisR);
    // Pupila grande (estilo anime).
    const int16_t pr = static_cast<int16_t>(r * 1.6f);
    if (pr > 0) display_->fillCircle(irisCX, irisCY, pr, DisplayBlack);

    drawEyelidsCurve(cx, cy, rx, ry, openAmount);

    if (cfg_.drawEyebrows) drawBrow(*display_, cfg_.mode, cx, cy, R, browOffset);
}

void Eye_t::drawFelineEye(float openAmount, int16_t dx, int16_t dy,
                          int16_t r, int16_t browOffset) {
    const int16_t cx = cfg_.eyeCenterX;
    const int16_t cy = cfg_.eyeCenterY;
    const int16_t R = cfg_.scleraR;
    const int16_t rx = R - 2;   // elipse almendra
    const int16_t ry = R + 2;

    fillEllipseInto(cx, cy, rx, ry, DisplayWhite);

    if (openAmount <= 0.05f) {
        if (cfg_.drawEyebrows) drawBrow(*display_, cfg_.mode, cx, cy, R, browOffset);
        drawClosedEyeLine(cx, cy, rx);
        return;
    }

    const int16_t irisCX = cx + dx;
    const int16_t irisCY = cy + dy;
    drawIrisArea(*display_, irisCX, irisCY, cfg_.irisR);
    // Pupila vertical alargada (elipse estrecha y alta).
    const int16_t pw = std::max<int16_t>(2, static_cast<int16_t>(r * 0.6f));
    const int16_t ph = static_cast<int16_t>(r * 2.2f);
    fillEllipseInto(irisCX, irisCY, pw, ph, DisplayBlack);

    drawEyelidsCurve(cx, cy, rx, ry, openAmount);

    if (cfg_.drawEyebrows) drawBrow(*display_, cfg_.mode, cx, cy, R, browOffset);
}

void Eye_t::drawRobotEye(float openAmount, int16_t dx, int16_t dy,
                         int16_t r, int16_t browOffset) {
    const int16_t cx = cfg_.eyeCenterX;
    const int16_t cy = cfg_.eyeCenterY;
    const int16_t R = cfg_.scleraR;
    const int16_t hw = R + 6;   // media anchura del visor
    const int16_t hh = R + 2;   // media altura del visor

    // Esclera rectangular (visor).
    display_->fillRect(cx - hw, cy - hh, hw * 2, hh * 2, DisplayWhite);

    if (openAmount <= 0.05f) {
        if (cfg_.drawEyebrows) drawBrow(*display_, cfg_.mode, cx, cy, R, browOffset);
        drawClosedEyeLine(cx, cy, hw);
        return;
    }

    const int16_t irisCX = cx + dx;
    const int16_t irisCY = cy + dy;
    drawIrisArea(*display_, irisCX, irisCY, cfg_.irisR);
    // Pupila cuadrada con retícula.
    const int16_t ps = std::max<int16_t>(3, static_cast<int16_t>(r * 1.3f));
    display_->fillRect(irisCX - ps, irisCY - ps, ps * 2, ps * 2, DisplayBlack);
    for (int16_t gy = irisCY - ps + 2; gy <= irisCY + ps - 2; gy += 2) {
        for (int16_t gx = irisCX - ps + 2; gx <= irisCX + ps - 2; gx += 2) {
            display_->drawPixel(gx, gy, DisplayWhite);
        }
    }

    // Párpados rectos y contorno de la visera.
    drawEyelidsFlat(cx, cy, hw, hh, openAmount);
    display_->drawRect(cx - hw, cy - hh, hw * 2, hh * 2, DisplayBlack);

    if (cfg_.drawEyebrows) drawBrow(*display_, cfg_.mode, cx, cy, R, browOffset);
}

// -----------------------------------------------------------------------------
// Dispatcher principal de dibujo (selecciona el estilo)
// -----------------------------------------------------------------------------

void Eye_t::drawEye(float openAmount, int16_t pupilDX, int16_t pupilDY,
                    int16_t pupilR, int16_t browOffset) {
    clearScreenBuffer();

    const float openF = clamp01(openAmount);
    switch (static_cast<EyeStyle_e>(cfg_.style)) {
        case EyeStyle_e::Anime:  drawAnimeEye(openF, pupilDX, pupilDY, pupilR, browOffset); break;
        case EyeStyle_e::Feline: drawFelineEye(openF, pupilDX, pupilDY, pupilR, browOffset); break;
        case EyeStyle_e::Robot:  drawRobotEye(openF, pupilDX, pupilDY, pupilR, browOffset); break;
        case EyeStyle_e::Classic:
        default:                 drawClassicEye(openF, pupilDX, pupilDY, pupilR, browOffset); break;
    }
}

} // namespace Eye
