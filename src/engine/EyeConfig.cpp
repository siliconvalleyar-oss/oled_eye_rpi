/**
 * @file EyeConfig.cpp
 * @brief Carga de configuración del ojo desde config/config.cfg y config/hardware.cfg.
 *
 * @details Implementa un parser sencillo de archivos de configuración en formato
 * "clave = valor" (con soporte de comentarios con '#'). Carga los parámetros en
 * la estructura Eye::EyeConfig_t. No depende de librerías externas para mantener
 * el proyecto autocontenido.
 *
 * @author Proyecto EyePet (Raspberry Pi)
 * @version 1.0.0
 */

#include "EyeConfig.hpp"
#include "Eye_t.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cstdio>

namespace Eye {

namespace {

/**
 * @brief Recorta espacios y saltos de línea de ambos extremos de una cadena.
 * @param s Cadena a recortar (se modifica in-place).
 * @return Referencia a la cadena recortada.
 */
std::string& trim(std::string& s) {
    // Elimina espacios, tabuladores y saltos de línea al inicio.
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) {
        return !std::isspace(c);
    }));
    // Elimina espacios, tabuladores y saltos de línea al final.
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) {
        return !std::isspace(c);
    }).base(), s.end());
    return s;
}

/**
 * @brief Convierte una cadena a un valor entero, con un valor por defecto.
 */
int toInt(const std::string& value, int def) {
    try { return std::stoi(value); } catch (...) { return def; }
}

/**
 * @brief Convierte una cadena a un valor flotante, con un valor por defecto.
 */
float toFloat(const std::string& value, float def) {
    try { return std::stof(value); } catch (...) { return def; }
}

/**
 * @brief Convierte una cadena a un valor booleano (true/false/1/0/yes/no).
 */
bool toBool(const std::string& value, bool def) {
    std::string v = value;
    trim(v);
    std::transform(v.begin(), v.end(), v.begin(), ::tolower);
    if (v == "true" || v == "1" || v == "yes" || v == "on") return true;
    if (v == "false" || v == "0" || v == "no" || v == "off") return false;
    return def;
}

/**
 * @brief Convierte una cadena a un entero (decimal, octal o hexadecimal 0x..).
 */
unsigned toHexOrInt(const std::string& value, unsigned def) {
    std::string v = value;
    trim(v);
    if (v.size() > 2 && v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) {
        try { return static_cast<unsigned>(std::stoul(v, nullptr, 16)); }
        catch (...) { return def; }
    }
    bool hasHex = std::any_of(v.begin(), v.end(), [](unsigned char c) {
        return std::isalpha(c) != 0;
    });
    if (hasHex) {
        try { return static_cast<unsigned>(std::stoul(v, nullptr, 16)); }
        catch (...) { return def; }
    }
    return static_cast<unsigned>(toInt(v, static_cast<int>(def)));
}

/**
 * @brief Aplica un par clave=valor a un segmento de la configuración.
 * @param cfg   Estructura destino.
 * @param key   Clave leída del archivo.
 * @param value Valor leído del archivo.
 * @param hw    true si es configuración de hardware, false si es general.
 */
void applyToConfig(EyeConfig_t& cfg, const std::string& key,
                   const std::string& value, bool hw) {
    if (hw) {
        if      (key == "protocol")       cfg.protocol    = value;
        else if (key == "i2c_device")     cfg.i2c_device  = value;
        else if (key == "i2c_address")    cfg.i2c_address = toHexOrInt(value, cfg.i2c_address);
        else if (key == "spi_device")     cfg.spi_device  = value;
        else if (key == "spi_speed")      cfg.spi_speed   = static_cast<unsigned>(toInt(value, static_cast<int>(cfg.spi_speed)));
        else if (key == "spi_dc_pin")     cfg.spi_dc_pin  = static_cast<unsigned>(toInt(value, static_cast<int>(cfg.spi_dc_pin)));
        else if (key == "spi_rst_pin")    cfg.spi_rst_pin = static_cast<unsigned>(toInt(value, static_cast<int>(cfg.spi_rst_pin)));
        else if (key == "spi_reset_active_high") cfg.spi_reset_active_high = toBool(value, cfg.spi_reset_active_high);
    } else {
        if      (key == "mode")                cfg.mode         = toInt(value, cfg.mode);
        else if (key == "style")               cfg.style        = toInt(value, cfg.style);
        else if (key == "frame_rate")          cfg.frameRate    = toFloat(value, cfg.frameRate);
        else if (key == "frame_delay_ms")      cfg.frameDelayMs = toInt(value, cfg.frameDelayMs);
        else if (key == "move_range_x")        cfg.moveRangeX   = static_cast<unsigned>(toInt(value, (int)cfg.moveRangeX));
        else if (key == "move_range_y")        cfg.moveRangeY   = static_cast<unsigned>(toInt(value, (int)cfg.moveRangeY));
        else if (key == "blink_min_ms")        cfg.blinkMinMs   = static_cast<unsigned>(toInt(value, (int)cfg.blinkMinMs));
        else if (key == "blink_max_ms")        cfg.blinkMaxMs   = static_cast<unsigned>(toInt(value, (int)cfg.blinkMaxMs));
        else if (key == "blink_close_ms")      cfg.blinkCloseMs = static_cast<unsigned>(toInt(value, (int)cfg.blinkCloseMs));
        else if (key == "blink_open_ms")       cfg.blinkOpenMs  = static_cast<unsigned>(toInt(value, (int)cfg.blinkOpenMs));
        else if (key == "blink_closed_hold_ms") cfg.blinkClosedHoldMs = static_cast<unsigned>(toInt(value, (int)cfg.blinkClosedHoldMs));
        else if (key == "eye_center_x")        cfg.eyeCenterX   = static_cast<int16_t>(toInt(value, cfg.eyeCenterX));
        else if (key == "eye_center_y")        cfg.eyeCenterY   = static_cast<int16_t>(toInt(value, cfg.eyeCenterY));
        else if (key == "sclera_r")            cfg.scleraR      = static_cast<int16_t>(toInt(value, cfg.scleraR));
        else if (key == "iris_r")              cfg.irisR        = static_cast<int16_t>(toInt(value, cfg.irisR));
        else if (key == "pupil_r")             cfg.pupilR       = static_cast<int16_t>(toInt(value, cfg.pupilR));
        else if (key == "glint")               cfg.glint        = toBool(value, cfg.glint);
        else if (key == "glint_dx")            cfg.glintDX      = static_cast<int16_t>(toInt(value, cfg.glintDX));
        else if (key == "glint_dy")            cfg.glintDY      = static_cast<int16_t>(toInt(value, cfg.glintDY));
        else if (key == "glint_r")             cfg.glintR       = static_cast<uint8_t>(toInt(value, cfg.glintR));
        else if (key == "draw_eyebrows")       cfg.drawEyebrows = toBool(value, cfg.drawEyebrows);
        else if (key == "debug")               cfg.debug        = toBool(value, cfg.debug);
    }
}

/**
 * @brief Lee un archivo "clave = valor" y aplica la función de carga adecuada.
 * @param path     Ruta del archivo de configuración.
 * @param cfg      Estructura destino.
 * @param isHardware true para config/hardware.cfg, false para config/config.cfg.
 */
void loadKeyValueFile(const std::string& path, EyeConfig_t& cfg, bool isHardware) {
    std::ifstream file(path);
    if (!file.is_open()) {
        fprintf(stderr, "EyeConfig: no se pudo abrir '%s' (usando valores por defecto)\n",
                path.c_str());
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        // Elimina comentarios (todo lo que esté tras '#').
        auto hashPos = line.find('#');
        if (hashPos != std::string::npos) line = line.substr(0, hashPos);
        trim(line);
        if (line.empty()) continue;

        auto eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = line.substr(0, eqPos);
        std::string value = line.substr(eqPos + 1);
        trim(key);
        trim(value);
        if (key.empty() || value.empty()) continue;
        applyToConfig(cfg, key, value, isHardware);
    }
}

} // namespace anon

/**
 * @brief Carga la configuración de hardware desde un archivo.
 * @param cfg  Estructura destino.
 * @param path Ruta del archivo.
 */
void loadHardwareConfig(EyeConfig_t& cfg, const std::string& path) {
    loadKeyValueFile(path, cfg, true);
}

/**
 * @brief Carga la configuración general desde un archivo.
 * @param cfg  Estructura destino.
 * @param path Ruta del archivo.
 */
void loadGeneralConfig(EyeConfig_t& cfg, const std::string& path) {
    loadKeyValueFile(path, cfg, false);
}

} // namespace Eye