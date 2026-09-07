# EyePet — Informe (REPORT)

Informe de estado del proyecto EyePet.

## Resumen

EyePet es una aplicación C++17 que emula el ojo de una mascota sobre un display
OLED SSD1306 (128×64) en Raspberry Pi (32/64 bits). Generada íntegramente a
partir del prompt del proyecto (`prompt.md`), ofrece 8 modos del ojo,
configuración por archivos y por CLI, y soporte I2C y SPI.

## Estado de los requisitos del prompt

| Requisito | Estado |
|-----------|--------|
| Estructura de carpetas completa | ✔ |
| `main.cpp` con formato `std::make_unique<Eye::Eye_t>()` + `run()` | ✔ |
| Clase `Eye::Eye_t` con `run()` no bloqueante | ✔ |
| Versión en tiempo de compilación (`-DVERSION`) + `--version` | ✔ |
| 7 funcionalidades del ojo (Normal, parpadeo, tracking, expresiones, sacadas, dormido, brillo) | ✔ |
| Config controlable por `config.cfg` y CLI | ✔ |
| Driver OLED completo (I2C y SPI) | ✔ |
| Makefile con versión, objetivos y 32/64 bits | ✔ |
| `scripts/install_deps.sh` | ✔ |
| `setup_git.sh` interactivo | ✔ |
| Documentación (README y 25+ archivos `docs/`) | ✔ |
| Licencia MIT | ✔ |
| Compilación `make` sin errores | ✔ (nativo verificado) |
| Creación del repositorio Git | Pendiente (ejecutar `setup_git.sh`) |

## Decisiones tomadas (2026-09-07)

- **Preservar el driver I2C funcional** por `/dev/i2c-N` (compatible Pi 5/CM5)
  y **añadir soporte SPI** (`/dev/spidev` + GPIO sysfs), en lugar de migrar a
  `bcm2835` (que no soporta I2C en RP1).
- **Versionado**: se reinicia el proyecto en `v1.0.0`.

## Pendientes

- Validación en hardware real (RPi + OLED).
- Prueba del modo SPI.
- Configuración del repositorio Git (`setup_git.sh`).
- Tests automatizados (ver `docs/TESTING.md`).

## Contacto / licencia

Proyecto local de Raspberry Pi. Licencia MIT (ver `LICENSE`).