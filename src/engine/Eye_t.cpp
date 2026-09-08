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
#include <algorithm>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <fcntl.h>

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
           "  --style <0..5>         Estilo (versión) del ojo:\n"
           "                           0 Classic, 1 Anime, 2 Feline, 3 Robot,\n"
           "                           4 Squint, 5 Heart.\n"
           "  --demo <segundos>      Duración de la demo de efectos (por defecto 180 s).\n"
           "  --config <archivo>     Ruta alternativa para config/config.cfg.\n"
           "  --hw-config <archivo>  Ruta alternativa para config/hardware.cfg.\n"
           "  --help, -h             Muestra esta ayuda.\n"
           "La mayoría de parámetros se ajustan en config/config.cfg.\n");
}

int Eye_t::parseArgs(int argc, char* argv[]) {
    // Primera pasada: opciones de control y rutas de configuración.
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--version") == 0 || std::strcmp(argv[i], "-v") == 0) {
            // Solo muestra la versión y termina sin tocar el hardware.
            printVersion();
            return 1; // señal de terminar
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            printHelp();
            return 1; // señal de terminar
        } else if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            configPath_ = argv[++i];
        } else if (std::strcmp(argv[i], "--hw-config") == 0 && i + 1 < argc) {
            hwConfigPath_ = argv[++i];
        }
    }

    // Carga la configuración ANTES de aplicar la CLI para que esta la sobreescriba
    // (y así --style/--mode/--demo toman prioridad sobre config/config.cfg).
    loadHardwareConfig(cfg_, hwConfigPath_);
    loadGeneralConfig(cfg_, configPath_);
    demoSeconds_ = static_cast<int>(cfg_.demoSeconds);

    // Segunda pasada: opciones que sobrescriben la configuración cargada.
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--version") == 0 || std::strcmp(argv[i], "-v") == 0 ||
            std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0 ||
            std::strcmp(argv[i], "--config") == 0 || std::strcmp(argv[i], "--hw-config") == 0) {
            continue;
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
                cfg_.style > static_cast<int>(EyeStyle_e::Heart)) {
                fprintf(stderr, "EyePet: estilo inválido '%d' (0..5)\n", cfg_.style);
                cfg_.style = static_cast<int>(EyeStyle_e::Classic);
            }
        } else if (std::strcmp(argv[i], "--demo") == 0 && i + 1 < argc) {
            demoSeconds_ = std::atoi(argv[++i]);
            if (demoSeconds_ < 5) {
                fprintf(stderr, "EyePet: demo demasiado corta '%d' (mínimo 5 s)\n", demoSeconds_);
                demoSeconds_ = 5;
            }
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

    // parseArgs ya cargó config/config.cfg y config/hardware.cfg y aplicó
    // las opciones de línea de comandos (--mode, --style, --demo) por encima.

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
// Asistencia de teclado no bloqueante (Linux/POSIX). Se evita esperar Enter.
namespace {
struct termios gOldTerm;
bool gTermiosActive = false;

void keyboardRaw(bool on) {
    if (on) {
        if (tcgetattr(STDIN_FILENO, &gOldTerm) == 0) {
            struct termios raw = gOldTerm;
            raw.c_lflag &= ~(tcflag_t)(ICANON | ECHO);
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSANOW, &raw);
            gTermiosActive = true;
            // stdin no bloqueante para poder leer por select().
            int fl = fcntl(STDIN_FILENO, F_GETFL, 0);
            fcntl(STDIN_FILENO, F_SETFL, fl | O_NONBLOCK);
        }
    } else if (gTermiosActive) {
        tcsetattr(STDIN_FILENO, TCSANOW, &gOldTerm);
        int fl = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, fl & ~O_NONBLOCK);
        gTermiosActive = false;
    }
}

/** @brief Devuelve true si hay una tecla disponible en stdin. */
bool kbhit() {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv = {0, 0};
    return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0;
}

/** @brief Lee una tecla pendiente o -1 si no hay ninguna. */
int getKey() {
    if (!kbhit()) return -1;
    unsigned char ch = 0;
    if (read(STDIN_FILENO, &ch, 1) == 1) return static_cast<int>(ch);
    return -1;
}
} // namespace anon

void Eye_t::printMenuBanner() const {
    printf("\n=== Ojos animados — menú de la demo (modo %s) ===\n",
           styleRotate_ ? "TODOS" : "un ojo");
    printf("  [0]/[T]  todos los ojos (rotación)   [1..6] elegir un ojo:\n");
    printf("             1 Classic   2 Anime   3 Feline   4 Robot   5 Squint   6 Heart\n");
    printf("  [b] parpadeo on/off   [g] brillo on/off   [e] cejas on/off\n");
    printf("  [+]/- velocidad       [c]/[C] contraste   [p]/[P] pupila +/-   [d] depurar\n");
    printf("  [h] esta ayuda        [q] salir (o espere a que termine la demo de %d s)\n",
           demoSeconds_);
    printState();
}

void Eye_t::printState() const {
    static const char* kStyle[] = {"Classic", "Anime", "Feline", "Robot",
                                   "Squint", "Heart"};
    static const char* kPhase[] = {"Parpadeo", "Mira L->R", "Feliz", "Triste",
                                   "Guiño", "Dormido"};
    int remain = demoSeconds_ - static_cast<int>(demoElapsedMs_ / 1000.0f);
    if (remain < 0) remain = 0;
    printf("  est=%-7s fase=%-9s modo=%d fps=%d parpadeo=%s brillo=%s cejas=%s"
           " pupila=%d contraste=0x%02X restan=%ds\n",
           kStyle[cfg_.style % 6], kPhase[demoPhase_ % DemoPhaseCount],
           cfg_.mode, static_cast<int>(cfg_.frameRate),
           overrideBlink_ ? "demo" : (blinking_ ? "si" : "si"),
           cfg_.glint ? "si" : "no", cfg_.drawEyebrows ? "si" : "no",
           pupilRCurrent_, contrast_, remain);
}

void Eye_t::setStyle(int s) {
    const int maxStyle = static_cast<int>(EyeStyle_e::Heart);
    cfg_.style = s < static_cast<int>(EyeStyle_e::Classic) ? static_cast<int>(EyeStyle_e::Classic)
                : s > maxStyle ? maxStyle : s;
    if (styleRotate_) styleRotate_ = false; // al elegir uno, sale del modo todos
    printf("Ojo seleccionado: %d\n", cfg_.style);
    printState();
}

void Eye_t::handleMenuKey(int ch) {
    switch (ch) {
        case '0': case 't': case 'T':
            styleRotate_ = true;
            printf("MODO TODOS: rotando por los %d estilos...\n",
                   static_cast<int>(EyeStyle_e::Heart) + 1);
            printState();
            break;
        case '1': case '2': case '3':
        case '4': case '5': case '6':
            setStyle(ch - '1');
            break;
        case 'b':
            overrideBlink_ = !overrideBlink_;
            if (!overrideBlink_) { blinking_ = false; blinkPhase_ = 1.0f; }
            printf("Parpadeo %s (la demo fuerza su secuencia)\n",
                   overrideBlink_ ? "OFF" : "ON");
            break;
        case 'g':
            cfg_.glint = !cfg_.glint;
            printf("Brillo: %s\n", cfg_.glint ? "ON" : "OFF");
            break;
        case 'e':
            cfg_.drawEyebrows = !cfg_.drawEyebrows;
            printf("Cejas: %s\n", cfg_.drawEyebrows ? "ON" : "OFF");
            break;
        case '+': case '=':
            cfg_.frameRate += 5.0f;
            if (cfg_.frameRate > 60.0f) cfg_.frameRate = 60.0f;
            frameDelayMs_ = static_cast<int>(1000.0f / cfg_.frameRate);
            printf("Velocidad: %d fps\n", static_cast<int>(cfg_.frameRate));
            break;
        case '-': case '_':
            cfg_.frameRate -= 5.0f;
            if (cfg_.frameRate < 5.0f) cfg_.frameRate = 5.0f;
            frameDelayMs_ = static_cast<int>(1000.0f / cfg_.frameRate);
            printf("Velocidad: %d fps\n", static_cast<int>(cfg_.frameRate));
            break;
        case 'c':
            contrast_ = contrast_ > 16 ? contrast_ - 16 : 0;
            if (display_) display_->setContrast(contrast_);
            printf("Contraste: 0x%02X\n", contrast_);
            break;
        case 'C':
            contrast_ = contrast_ < 0xF0 ? contrast_ + 16 : 0xFF;
            if (display_) display_->setContrast(contrast_);
            printf("Contraste: 0x%02X\n", contrast_);
            break;
        case 'p':
            pupilBoost_ = pupilBoost_ > -4 ? pupilBoost_ - 1 : -4;
            printf("Pupila (dilatación): %d\n", pupilBoost_);
            break;
        case 'P':
            pupilBoost_ = pupilBoost_ < 6 ? pupilBoost_ + 1 : 6;
            printf("Pupila (dilatación): %d\n", pupilBoost_);
            break;
        case 'd':
            cfg_.debug = !cfg_.debug;
            printf("Depuración: %s\n", cfg_.debug ? "ON" : "OFF");
            break;
        case 'h':
            printMenuBanner();
            break;
        case 'q': case 'Q': case 27: // q / Q / Esc
            printf("\nSaliendo de la demo...\n");
            kbdActive_ = false; // señala al bucle que debe terminar
            break;
        default:
            break;
    }
}

void Eye_t::animationLoop() {
    // Ajusta el delay por fotograma según la tasa configurada.
    frameDelayMs_ = cfg_.frameDelayMs > 0 ? cfg_.frameDelayMs
                                          : static_cast<int>(1000.0f / cfg_.frameRate);
    if (frameDelayMs_ < 1) frameDelayMs_ = 1;

    lastTickMs_ = 0;
    contrast_ = 0xFF;
    if (display_) display_->setContrast(contrast_);

    // Tiempo hasta el primer parpadeo.
    blinkTimerMs_ = static_cast<float>(randRange(
        static_cast<int>(cfg_.blinkMinMs), static_cast<int>(cfg_.blinkMaxMs)));

    // Activa el teclado no bloqueante (menú interactivo sin presionar Enter).
    kbdActive_ = isatty(STDIN_FILENO);
    if (kbdActive_) {
        keyboardRaw(true);
        printMenuBanner();
    }

    auto lastReal = std::chrono::steady_clock::now();
    bool finished = false;

    // Bucle no bloqueante: calcula un dt (ms), procesa teclas, actualiza la
    // animación, dibuja y espera el delay restante para mantener la cadencia.
    while (!finished) {
        auto nowReal = std::chrono::steady_clock::now();
        float dtMs = std::chrono::duration<float, std::milli>(nowReal - lastReal).count();
        lastReal = nowReal;
        ++frameCounter_;
        demoElapsedMs_ += dtMs;

        // 1) Procesa las teclas del menú (una por fotograma).
        if (kbdActive_) {
            int key;
            while (kbdActive_ && (key = getKey()) >= 0) handleMenuKey(key);
        }
        if (!kbdActive_) { finished = true; break; }

        // 2) Secuencia de efectos de la demo (parpadeo, mirada, expresiones...).
        updateDemo(dtMs);

        // 3) Parpadeo espontáneo (si la demo no controla el párpado a mano).
        if (!overrideBlink_) updateBlink(dtMs);

        // 4) Dibuja el ojo con el estado actual. La apertura del párpado se
        //    deriva de blinkPhase_ (1.0 abierto -> 0.0 cerrado) para que el
        //    efecto de párpado se vea en pantalla.
        float open = 0.0f;
        switch (static_cast<EyeMode_e>(cfg_.mode)) {
            case EyeMode_e::Sleep:
                open = 0.0f;
                break;
            case EyeMode_e::Sleepy:
                open = blinkPhase_ < 0.6f ? blinkPhase_ : 0.6f;
                break;
            default:
                open = blinkPhase_;
                break;
        }
        drawEye(open, pupilDX_, pupilDY_, pupilRCurrent_, browOffset_);

        // 5) Brillo/reflejo (solo con el ojo suficientemente abierto).
        if (cfg_.glint && blinkPhase_ > 0.5f)
            addGlint(cfg_.eyeCenterX + pupilDX_ + cfg_.glintDX,
                     cfg_.eyeCenterY + pupilDY_ + cfg_.glintDY);

        // 6) Vuelca el buffer final a la pantalla.
        display_->update();

        if (cfg_.debug && (frameCounter_ % 30 == 0)) {
            printf("[EyePet] frame=%u mode=%d fase=%d pupil=(%d,%d) open=%.2f\n",
                   frameCounter_, cfg_.mode, demoPhase_, pupilDX_, pupilDY_, open);
        }

        // 7) Espera y control de la duración de la demo (3 minutos).
        if (demoElapsedMs_ >= static_cast<float>(demoSeconds_) * 1000.0f) {
            finished = true;
            printf("\nDemo de %d segundos finalizada. Apagando el ojo...\n",
                   demoSeconds_);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(frameDelayMs_));
    }

    // Limpia el teclado no bloqueante antes de salir.
    if (kbdActive_) keyboardRaw(false);
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

void Eye_t::updateLookLR(float dtMs) {
    // Mirada panorámica de izquierda a derecha con pausas en los extremos.
    static float tt = 0.0f;
    const float pause = 650.0f;  // pausa en cada extremo (ms)
    const float move = 1150.0f;  // tiempo de un recorrido completo (ms)
    const float cycle = 2.0f * (pause + move);
    tt += dtMs;
    const float ph = std::fmod(tt, cycle);
    float pos;
    if (ph < pause) {
        pos = 0.0f;                                   // izquierda
    } else if (ph < pause + move) {
        pos = static_cast<float>(cfg_.moveRangeX) * (ph - pause) / move; // -> derecha
    } else if (ph < 2.0f * pause + move) {
        pos = static_cast<float>(cfg_.moveRangeX);    // derecha
    } else {
        pos = static_cast<float>(cfg_.moveRangeX) *
              (1.0f - (ph - (2.0f * pause + move)) / move); // -> izquierda
    }
    const int16_t target = static_cast<int16_t>(pos);
    pupilDX_ += static_cast<int16_t>((target - pupilDX_) * 0.15f * dtMs / 16.0f);
    pupilDY_ += static_cast<int16_t>((0 - pupilDY_) * 0.15f * dtMs / 16.0f);
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

// -----------------------------------------------------------------------------
// Demo guiada de 3 minutos: secuencia de efectos (parpadeo, mirada, feliz,
// triste, guiño, dormido) y rotación por todos los estilos de ojo.
// -----------------------------------------------------------------------------
void Eye_t::updateDemo(float dtMs) {
    static const float phaseDurationMs[DemoPhaseCount] = {
        2000.0f,   // DemoBlink: abierto con parpadeo espontáneo.
        2200.0f,   // DemoLook:  apertura + mirada izquierda-derecha.
        1700.0f,   // DemoHappy: feliz (cejas arqueadas, iris brillante).
        1700.0f,   // DemoSad:   triste (ceja caída, párpado medio, mirada baja).
        1500.0f,   // DemoWink:  guiño (párpado cierra y abre lentamente).
        1300.0f    // DemoClosed: párpado cerrado (dormido).
    };

    demoPhaseTimerMs_ += dtMs;
    if (demoPhaseTimerMs_ >= phaseDurationMs[demoPhase_]) {
        demoPhaseTimerMs_ = 0.0f;
        demoPhase_ = (demoPhase_ + 1) % DemoPhaseCount;
        // Si se vuelve al inicio y estamos en modo "todos", rota el estilo.
        if (demoPhase_ == DemoBlink && styleRotate_) {
            setStyle(cfg_.style + 1);
        }
    }

    switch (demoPhase_) {
        case DemoBlink:
            // Ojo abierto, parpadeo espontáneo, ceja neutra, pupila normal.
            cfg_.mode = static_cast<int>(EyeMode_e::Normal);
            overrideBlink_ = false;
            updateNormal(dtMs);
            pupilRCurrent_ = cfg_.pupilR;
            break;
        case DemoLook:
            // Apertura completa y mirada de izquierda a derecha.
            cfg_.mode = static_cast<int>(EyeMode_e::Normal);
            overrideBlink_ = false;
            blinkPhase_ = 1.0f;
            updateLookLR(dtMs);
            pupilRCurrent_ = cfg_.pupilR;
            break;
        case DemoHappy: {
            // Expresión alegre: cejas arqueadas, pupila dilatada, mirada alta.
            cfg_.mode = static_cast<int>(EyeMode_e::Happy);
            overrideBlink_ = false;
            blinkPhase_ = 1.0f;
            targetDX_ = 0;
            targetDY_ = -2;
            pupilDX_ += static_cast<int16_t>((targetDX_ - pupilDX_) * 0.1f * dtMs / 16.0f);
            pupilDY_ += static_cast<int16_t>((targetDY_ - pupilDY_) * 0.1f * dtMs / 16.0f);
            pupilRCurrent_ = static_cast<int16_t>(std::min<int>(
                cfg_.pupilR + 2, cfg_.irisR - 2));
            break;
        }
        case DemoSad:
            // Expresión triste: ceja caída, párpado medio cerrado, mirada baja.
            cfg_.mode = static_cast<int>(EyeMode_e::Sleepy);
            overrideBlink_ = true;
            blinkPhase_ = 0.5f;
            targetDX_ = 2;
            targetDY_ = static_cast<int16_t>(cfg_.moveRangeY * 0.7f);
            pupilDX_ += static_cast<int16_t>((targetDX_ - pupilDX_) * 0.1f * dtMs / 16.0f);
            pupilDY_ += static_cast<int16_t>((targetDY_ - pupilDY_) * 0.1f * dtMs / 16.0f);
            pupilRCurrent_ = std::max<int16_t>(2, cfg_.pupilR - 1);
            browOffset_ = 3; // ceja caída
            break;
        case DemoWink: {
            // Guiño: el párpado cierra y abre lentamente (a cámara lenta).
            cfg_.mode = static_cast<int>(EyeMode_e::Normal);
            overrideBlink_ = true;
            pupilDX_ = 0; pupilDY_ = 0;
            pupilRCurrent_ = cfg_.pupilR;
            browOffset_ = 0;
            if (!demoWinkGoing_) {
                demoWinkGoing_ = true;
                demoWinkTimerMs_ = 0.0f;
            }
            demoWinkTimerMs_ += dtMs;
            const float closeMs = 350.0f, holdMs = 500.0f, openMs = 400.0f;
            const float totalMs = closeMs + holdMs + openMs;
            if (demoWinkTimerMs_ <= closeMs) {
                const float t = demoWinkTimerMs_ / closeMs;
                blinkPhase_ = 1.0f - (t * t);           // 1 -> 0 (easing).
            } else if (demoWinkTimerMs_ <= closeMs + holdMs) {
                blinkPhase_ = 0.0f;                      // cerrado.
            } else if (demoWinkTimerMs_ <= totalMs) {
                const float t = (demoWinkTimerMs_ - closeMs - holdMs) / openMs;
                blinkPhase_ = t * t;                     // 0 -> 1 (easing).
            } else {
                blinkPhase_ = 1.0f;
                demoWinkGoing_ = false;
                demoWinkTimerMs_ = 0.0f;
            }
            break;
        }
        case DemoClosed:
            // Párpado cerrado (dormido).
            cfg_.mode = static_cast<int>(EyeMode_e::Sleep);
            overrideBlink_ = true;
            blinkPhase_ = 0.0f;
            pupilRCurrent_ = 0;
            browOffset_ = 0;
            break;
        default:
            break;
    }

    // Dilatación manual (menú) se aplica por encima del radio de la fase.
    if (pupilBoost_ != 0 && cfg_.mode != static_cast<int>(EyeMode_e::Sleep)) {
        pupilRCurrent_ = static_cast<int16_t>(std::min<int16_t>(
            pupilRCurrent_ + pupilBoost_, std::max<int16_t>(1, cfg_.irisR - 1)));
        if (pupilRCurrent_ < 1) pupilRCurrent_ = 1;
    }
}

void Eye_t::updateBlink(float dtMs) {
    // Anima de verdad la apertura del párpado con easing: 1.0 (abierto) ->
    // 0.0 (cerrado) en blinkCloseMs, mantiene cerrado y vuelve a 1.0 en
    // blinkOpenMs. Si la demo controla el párpado, no interviene.
    if (overrideBlink_) return;

    if (!blinking_) {
        blinkPhase_ = 1.0f;
        blinkTimerMs_ -= dtMs;
        if (blinkTimerMs_ <= 0.0f) {
            blinking_ = true;
            blinkStage_ = 0; // cerrar
            blinkStageTimerMs_ = static_cast<float>(cfg_.blinkCloseMs);
        }
        return;
    }

    blinkStageTimerMs_ -= dtMs;
    float done = 0.0f;
    if (blinkStageTimerMs_ <= 0.0f) blinkStageTimerMs_ = -blinkStageTimerMs_;

    if (blinkStage_ == 0) {          // cerrando: 1 -> 0
        const float total = std::max(1.0f, static_cast<float>(cfg_.blinkCloseMs));
        done = 1.0f - blinkStageTimerMs_ / total;   // -.. : usamos timer ya restado
        done = std::max(0.0f, std::min(1.0f, done));
        blinkPhase_ = 1.0f - done * done;
        if (blinkStageTimerMs_ <= 0.0f || blinkPhase_ <= 0.0f) {
            blinkStage_ = 1;        // mantener cerrado
            blinkStageTimerMs_ = static_cast<float>(cfg_.blinkClosedHoldMs);
            blinkPhase_ = 0.0f;
        }
    } else if (blinkStage_ == 1) {   // mantenimiento cerrado
        blinkPhase_ = 0.0f;
        if (blinkStageTimerMs_ <= 0.0f) {
            blinkStage_ = 2;        // abriendo
            blinkStageTimerMs_ = static_cast<float>(cfg_.blinkOpenMs);
        }
    } else {                          // abriendo: 0 -> 1
        const float total = std::max(1.0f, static_cast<float>(cfg_.blinkOpenMs));
        done = 1.0f - blinkStageTimerMs_ / total;
        done = std::max(0.0f, std::min(1.0f, done));
        blinkPhase_ = done * done;
        if (blinkStageTimerMs_ <= 0.0f || blinkPhase_ >= 1.0f) {
            blinking_ = false;
            blinkPhase_ = 1.0f;
            blinkStage_ = 0;
            blinkTimerMs_ = static_cast<float>(randRange(
                static_cast<int>(cfg_.blinkMinMs),
                static_cast<int>(cfg_.blinkMaxMs)));
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
        case EyeStyle_e::Squint:
            // Un punto pequeño (párpado entrecerrado).
            display_->fillCircle(cx, cy, 2, DisplayWhite);
            break;
        case EyeStyle_e::Heart:
            // Punto superior central (corazón brillante).
            display_->fillCircle(cx, cy - 2, 2, DisplayWhite);
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

void Eye_t::fillTriangleInto(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                             int16_t x2, int16_t y2, uint8_t color) {
    // Rasterización por scanlines: para cada fila calcula los dos puntos de
    // intersección del triángulo con la línea horizontal y rellena el tramo.
    const int16_t pts[3][2] = {{x0, y0}, {x1, y1}, {x2, y2}};
    const int16_t minY = std::min({y0, y1, y2});
    const int16_t maxY = std::max({y0, y1, y2});

    for (int16_t yy = minY; yy <= maxY; ++yy) {
        int16_t cross[2] = {0, 0};
        int n = 0;
        for (int e = 0; e < 3 && n < 2; ++e) {
            const int16_t ax = pts[e][0], ay = pts[e][1];
            const int16_t bx = pts[(e + 1) % 3][0], by = pts[(e + 1) % 3][1];
            if ((ay <= yy && yy < by) || (by <= yy && yy < ay)) {
                const float t = static_cast<float>(yy - ay) /
                                static_cast<float>(by - ay);
                cross[n++] = static_cast<int16_t>(ax + (bx - ax) * t);
            }
        }
        if (n == 2) {
            const int16_t lo = std::min(cross[0], cross[1]);
            const int16_t hi = std::max(cross[0], cross[1]);
            display_->drawFastHLine(lo, yy, hi - lo + 1, color);
        }
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

void Eye_t::drawSquintEye(float openAmount, int16_t dx, int16_t dy,
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

    // Párpados dobles gruesos (entrecerrado): curvas superior e inferior
    // recortadas más una banda gruesa que estrecha la abertura.
    drawEyelidsCurve(cx, cy, R, R, openAmount);
    const int16_t halfEye = static_cast<int16_t>(R * clamp01(openAmount));
    if (halfEye > 2) {
        for (int16_t d = 1; d <= 2; ++d) {
            display_->drawFastHLine(cx - R, cy - halfEye - d, R * 2, DisplayBlack);
            display_->drawFastHLine(cx - R, cy + halfEye + d, R * 2, DisplayBlack);
        }
    }

    if (cfg_.drawEyebrows) drawBrow(*display_, cfg_.mode, cx, cy, R, browOffset);
}

void Eye_t::drawHeartEye(float openAmount, int16_t dx, int16_t dy,
                         int16_t r, int16_t browOffset) {
    const int16_t cx = cfg_.eyeCenterX;
    const int16_t cy = cfg_.eyeCenterY;
    const int16_t R = cfg_.scleraR;
    const int16_t rx = R - 2;
    const int16_t ry = R - 1;

    fillEllipseInto(cx, cy, rx, ry, DisplayWhite);

    if (openAmount <= 0.05f) {
        if (cfg_.drawEyebrows) drawBrow(*display_, cfg_.mode, cx, cy, R, browOffset);
        drawSleepLine(cx, cy, rx);
        return;
    }

    const int16_t irisCX = cx + dx;
    const int16_t irisCY = cy + dy;
    drawIrisArea(*display_, irisCX, irisCY, cfg_.irisR);

    // Pupila en forma de corazón (dos círculos + triángulo).
    if (r > 0) {
        const int16_t s = std::max<int16_t>(2, r);
        display_->fillCircle(irisCX - s / 2, irisCY - s / 2, s * 2 / 3, DisplayBlack);
        display_->fillCircle(irisCX + s / 2, irisCY - s / 2, s * 2 / 3, DisplayBlack);
        fillTriangleInto(irisCX - s, irisCY - s / 2 + s / 3,
                         irisCX + s, irisCY - s / 2 + s / 3,
                         irisCX, irisCY + s, DisplayBlack);
    }

    drawEyelidsCurve(cx, cy, rx, ry, openAmount);

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
        case EyeStyle_e::Squint: drawSquintEye(openF, pupilDX, pupilDY, pupilR, browOffset); break;
        case EyeStyle_e::Heart:  drawHeartEye(openF, pupilDX, pupilDY, pupilR, browOffset); break;
        case EyeStyle_e::Classic:
        default:                 drawClassicEye(openF, pupilDX, pupilDY, pupilR, browOffset); break;
    }
}

} // namespace Eye
