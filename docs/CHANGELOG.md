# EyePet — CHANGELOG

Historial de versiones de EyePet.

## [v1.0.0] — 2026-09-07

Versión inicial de EyePet (generación completa a partir del prompt).

### Añadido

- Motor de animación del ojo (`Eye::Eye_t`) con 8 modos:
  Normal, Tracking, Happy, Surprised, Angry, Sleepy, Sleep y Saccades.
- Parpadeo espontáneo configurable (intervalo y duración de fases).
- Efecto de brillo/reflejo sobre el iris.
- Cejas por expresión (arqueadas, fruncidas, elevadas, neutras).
- Bucle de animación no bloqueante con cadencia configurable.
- Carga de configuración desde `config/config.cfg` y `config/hardware.cfg`.
- Soporte de protocolo I2C (por defecto) y SPI:
  - I2C por `/dev/i2c-N` (ioctl), compatible con Pi 5/CM5 (chip RP1).
  - SPI por `/dev/spidev` + GPIO de sysfs (DC/RST).
- Interfaz `Eye::IDisplay` con adaptadores I2C y SPI.
- `main.cpp` con el formato exacto del prompt (`std::make_unique<Eye::Eye_t>()`).
- Versión en tiempo de compilación desde `VERSION` (`-DVERSION`), mostrada al
  iniciar y con `--version`.
- Argumentos de línea de comandos: `--version`, `--mode`, `--config`,
  `--hw-config`, `--help`.
- Scripts: `install_deps.sh` (32/64 bits), `setup_git.sh` (interactivo),
  `generate_basic_src.sh`.
- Documentación completa en `docs/` y `README.md`.
- Licencia MIT.

### Arreglado

- El proyecto anterior (BASIC) no incluía animación del ojo; se sustituyó la
  demo estática por el motor completo de EyePet.

### Compatibilidad

- Raspberry Pi 32 bits (armhf) y 64 bits (aarch64).
- Display OLED SSD1306 128×64 por I2C o SPI.