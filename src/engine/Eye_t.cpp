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

        // Actualiza el brillo (depende de la posición de la pupila).
        if (cfg_.glint) addGlint(cfg_.eyeCenterX + pupilDX_ + cfg_.glintDX,
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
    // Dibuja un pequeño círculo blanco de brillo sobre el iris.
    display_->fillCircle(cx, cy, cfg_.glintR, DisplayWhite);
    // Pequeño núcleo más brillante (opcional, realismo).
    if (cfg_.glintR >= 2) {
        display_->fillCircle(cx, cy, std::max<int16_t>(1, cfg_.glintR - 1), DisplayWhite);
    }
}

void Eye_t::drawEye(float openAmount, int16_t pupilDX, int16_t pupilDY,
                    int16_t pupilR, int16_t browOffset) {
    clearScreenBuffer();

    const int16_t cx = cfg_.eyeCenterX;
    const int16_t cy = cfg_.eyeCenterY;
    const int16_t scleraR = cfg_.scleraR;

    // Compute la apertura vertical efectiva del párpado (0..scleraR).
    const float openF = openAmount < 0.0f ? 0.0f : (openAmount > 1.0f ? 1.0f : openAmount);
    const int16_t halfEye = static_cast<int16_t>(scleraR * openF);

    // 1) Esclera (blanco del ojo): círculo.
    display_->fillCircle(cx, cy, scleraR, DisplayWhite);

    if (openF <= 0.05f) {
        // Ojo cerrado: muestra solo la línea del párpado.
        display_->drawFastHLine(cx - scleraR, cy, scleraR * 2, DisplayBlack);
        // Dibuja la ceja (si procede) y termina.
        if (cfg_.drawEyebrows) {
            display_->drawFastHLine(cx - 10, cy - scleraR - 6 + browOffset, 20, DisplayBlack);
        }
        return;
    }

    // 2) Párpados superior e inferior (tapan la esclera según la apertura).
    //    La zona por encima del párpado superior y por debajo del inferior se
    //    rellena con negro, encajando la forma del ojo.
    //    Párpado superior: y < cy - halfEye
    display_->fillRect(cx - scleraR, cy - scleraR - scleraR,
                    scleraR * 2, (scleraR - halfEye) + scleraR, DisplayBlack);

    // 3) Iris (círculo de color) centrado en la esclera, desplazado con la pupila.
    const int16_t irisCX = cx + pupilDX;
    const int16_t irisCY = cy + pupilDY;
    const int16_t irisR = cfg_.irisR;
    display_->fillCircle(irisCX, irisCY, irisR, DisplayWhite); // limpiar zona del iris

    // Iris con color (gris medio: 0x40 = patrón punteado para distinguirlo).
    // Usamos un patrón de rejilla para dar textura al iris en pantalla monocromo.
    for (int16_t y = irisCY - irisR; y <= irisCY + irisR; ++y) {
        for (int16_t x = irisCX - irisR; x <= irisCX + irisR; ++x) {
            const int16_t dx = x - irisCX;
            const int16_t dy = y - irisCY;
            if (dx * dx + dy * dy <= irisR * irisR) {
                // Patrón de anillos radiales para simular el iris.
                const float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                const int ring = static_cast<int>(dist / (irisR / 3.0f));
                if (ring % 2 == 0) display_->drawPixel(x, y, DisplayWhite);
                // else: dejamos la esclera (ya blanca) -> efecto de iris claro.
            }
        }
    }

    // 4) Pupila (círculo negro central).
    if (pupilR > 0) {
        display_->fillCircle(irisCX, irisCY, pupilR, DisplayBlack);
    }

    // 5) Contorno del ojo.
    display_->drawCircle(cx, cy, scleraR, DisplayBlack);

    // 6) Párpados que recortan el ojo según la apertura (curva superior/inferior).
    //    Párpado superior (curva): rellena la zona superior.
    for (int16_t x = cx - scleraR; x <= cx + scleraR; ++x) {
        const int16_t dx = x - cx;
        const int16_t halfH = static_cast<int16_t>(std::sqrt(
            static_cast<float>(scleraR * scleraR - dx * dx)));
        // Línea de párpado superior a la altura de halfEye desde el centro.
        const int16_t lidY = cy - halfEye;
        // Rellena desde el borde superior del círculo hasta el párpado.
        const int16_t topY = cy - halfH;
        if (lidY > topY && halfEye >= 0) {
            display_->drawFastVLine(x, topY, lidY - topY, DisplayBlack);
        }
        // Párpado inferior: rellena desde lidY inferior hasta el borde del círculo.
        const int16_t bottomLidY = cy + halfEye;
        const int16_t bottomY = cy + halfH;
        if (bottomLidY < bottomY && halfEye >= 0) {
            display_->drawFastVLine(x, bottomLidY, bottomY - bottomLidY, DisplayBlack);
        }
    }

    // 7) Línea de unión de los párpados (a la altura del rabillo).
    const int16_t lidY = cy - halfEye;
    const int16_t bottomLidY = cy + halfEye;
    display_->drawFastHLine(cx - scleraR, bottomLidY, scleraR * 2, DisplayBlack);
    display_->drawFastHLine(cx - scleraR, lidY, scleraR * 2, DisplayBlack);

    // 8) Cejas (expresión).
    if (cfg_.drawEyebrows && openF > 0.05f) {
        const int16_t browY = cy - scleraR - 6;
        switch (static_cast<EyeMode_e>(cfg_.mode)) {
            case EyeMode_e::Happy:
                // Cejas arqueadas hacia arriba.
                display_->drawLine(cx - 10, browY + 2, cx - 4, browY - 3, DisplayBlack);
                display_->drawLine(cx - 4, browY - 3, cx + 4, browY - 3, DisplayBlack);
                display_->drawLine(cx + 4, browY - 3, cx + 10, browY + 2, DisplayBlack);
                break;
            case EyeMode_e::Angry:
                // Cejas fruncidas (inclinadas hacia dentro).
                display_->drawLine(cx - 10, browY - 3, cx - 2, browY + 2, DisplayBlack);
                display_->drawLine(cx + 2, browY + 2, cx + 10, browY - 3, DisplayBlack);
                break;
            case EyeMode_e::Surprised:
                // Cejas muy elevadas.
                display_->drawFastHLine(cx - 10, browY - 4 + browOffset, 20, DisplayBlack);
                break;
            default:
                // Cejas neutras con un desplazamiento (sueño hacia abajo).
                display_->drawFastHLine(cx - 10, browY + browOffset, 20, DisplayBlack);
                break;
        }
    }
}

} // namespace Eye
