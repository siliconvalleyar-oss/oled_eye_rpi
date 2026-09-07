# EyePet — Registro de actividad (ACTIVITY)

Bitácora de actividades de desarrollo del proyecto.

## 2026-09-07

- Inicio del proyecto EyePet a partir del prompt (generación completa de código,
  configuración, documentación y scripts).
- **Fuentes y cabeceras**:
  - Motor del ojo: `Eye::Eye_t` (modos Normal, Tracking, Happy, Surprised,
    Angry, Sleepy, Sleep, Saccades; parpadeo; brillo; cejas).
  - Carga de configuración: `EyeConfig.cpp` (config.cfg y hardware.cfg).
  - Interfaz `Eye::IDisplay` con adaptadores I2C y SPI.
  - Driver SPI del SSD1306 (`SSD1306_SPI` vía `/dev/spidev` + GPIO sysfs).
  - `main.cpp` con el formato exacto del prompt (`std::make_unique<Eye::Eye_t>`).
- **Configuración**: `config.cfg` y `hardware.cfg` completos y documentados.
- **Scripts**: `install_deps.sh` (32/64 bits), `setup_git.sh` (interactivo),
  `generate_basic_src.sh` en la raíz.
- **Compilación**: make nativo OK; versionado 1.0.0 (`VERSION`).
- **Documentación**: completados README y los archivos de `docs/`.

## Pendiente

- Compilar y validar en hardware real (Raspberry Pi).
- Probar el modo SPI con un display real.
- Configuración del repositorio git (ejecutar `setup_git.sh`).