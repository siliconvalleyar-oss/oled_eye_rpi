/**
 * @file EyeConfig.hpp
 * @brief Declaraciones para la carga de configuración del ojo.
 *
 * @details Expone las funciones que cargan config/config.cfg (parámetros
 * generales) y config/hardware.cfg (protocolo I2C/SPI y pines) en la
 * estructura Eye::EyeConfig_t.
 *
 * @author Proyecto EyePet (Raspberry Pi)
 * @version 1.0.0
 */

#ifndef EYECONFIG_HPP
#define EYECONFIG_HPP

#include <string>

#include "Eye_t.hpp"

namespace Eye {

/**
 * @brief Carga la configuración de hardware (protocolo I2C/SPI, pines, dispositivo).
 * @param cfg  Estructura donde se depositan los parámetros.
 * @param path Ruta del archivo de configuración de hardware (p.ej. config/hardware.cfg).
 */
void loadHardwareConfig(EyeConfig_t& cfg, const std::string& path);

/**
 * @brief Carga la configuración general (modo inicial, velocidad, parpadeo, geometría).
 * @param cfg  Estructura donde se depositan los parámetros.
 * @param path Ruta del archivo de configuración general (p.ej. config/config.cfg).
 */
void loadGeneralConfig(EyeConfig_t& cfg, const std::string& path);

} // namespace Eye

#endif // EYECONFIG_HPP