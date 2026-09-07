# EyePet — Pruebas (TESTING)

Estrategia de pruebas de EyePet.

## Pruebas de humo (rápidas)

| Prueba | Comando | Resultado esperado |
|--------|---------|--------------------|
| Versión | `./bin/App --version` | `EyePet v1.0.0`, sale sin abrir el bus |
| Ayuda | `./bin/App --help` | lista de opciones |
| Opción inválida | `./bin/App --wat` | aviso por stderr, continúa |
| Modo inválido | `./bin/App --mode 99 --version` | resetea a Normal |

## Pruebas en hardware (Raspberry Pi + OLED)

1. **Detección**: `sudo i2cdetect -y 1` → dispositivo en `0x3c` (o `0x3d`).
2. **Arranque**: `sudo ./bin/App --mode 0` → ojo normal explorando.
3. **Modos** (`--mode 0..7`): verificar la animación de cada uno.
4. **Parpadeo**: observar cierre/apertura periódica.
5. **Brillo**: con `glint = true` debe verse el punto sobre el iris.
6. **SPI** (si hardware disponible): `protocol = spi` en `hardware.cfg`.
7. **Apagado seguro**: `./bin/App --version` no debe generar errores I2C.

## Configuración

- Modificar `config/config.cfg` y comprobar que los cambios se aplican sin
  recompilar (parpadeo más lento/rápido, rango de pupila, geometría).
- `debug = true` muestra estado cada 30 fotogramas (fotograma, modo, pupila).

## Automatización (futuro)

- Test del parser: alimentar `config.cfg` con valores válidos/inválidos y
  comprobar `EyeConfig_t` resultante (diseñar con un mini-framework o asserts).
- Test del motor sin hardware: render del buffer a imagen PGM (los drivers ya
  separan buffer de transferencia).
- Script `scripts/test_eye.sh` propuesto: compila, prueba `--version`,
  `--help`, y recorre `--mode 0..7` (sin display, los modos trabarían al abrir
  el HW; se admite con `--hw-config` apuntando a un fichero que fuerce error).

## Criterios de aceptación

- `make` sin errores ni advertencias relevantes.
- `./bin/App --version` correcto.
- Animación fluida (>20 FPS objetivo) en Pi Zero 2W y CM5.
- Cambio de modos via CLI y config sin recompilar.