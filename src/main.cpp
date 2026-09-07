/**
 * @file main.cpp
 * @brief Punto de entrada principal de la aplicación EyePet para Raspberry Pi.
 *
 * @details Crea una instancia de Eye::Eye_t mediante un unique_ptr y ejecuta su
 * método run(), que inicializa la pantalla OLED SSD1306, carga la configuración
 * y ejecuta el bucle de animación del ojo. El parseo de argumentos de línea de
 * comandos (--version, --config, --hw-config, --mode) se realiza dentro de
 * Eye::Eye_t::run().
 *
 * Ejecución recomendada (necesita permisos para acceder a /dev/i2c-*):
 *     sudo ./bin/App
 *     sudo ./bin/App --version
 *     sudo ./bin/App --mode 6
 *
 * La memoria se libera automáticamente al salir del main gracias al unique_ptr.
 *
 * @author Proyecto EyePet (Raspberry Pi)
 * @version 1.0.0
 */

#include <memory>
#include "Eye_t.hpp"   // Ruta adecuada de la clase Eye::Eye_t

/**
 * @brief Punto de entrada de la aplicación.
 * @param argc Número de argumentos de la línea de comandos.
 * @param argv Vector de argumentos de la línea de comandos.
 * @return Código de retorno del proceso (0 = éxito).
 */
int main(int argc, char* argv[]) {
    // Crea el ojo con memoria gestionada automáticamente (unique_ptr):
    // la memoria se libera al salir del main sin new/delete explícitos.
    auto eye = std::make_unique<Eye::Eye_t>();
    return eye->run(argc, argv);
}