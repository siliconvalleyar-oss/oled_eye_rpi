# EyePet — Arquitectura

Vista general de la arquitectura de la aplicación EyePet.

## Visión general

EyePet es un programa monoproceso y de un único hilo cuyo bucle principal
(no bloqueante) dibuja un ojo de mascota en el buffer de un OLED SSD1306 y lo
volca a la pantalla a una cadencia configurable.

El diseño separa claramente:

- **Núcleo de la aplicación** (`src/engine/`, `include/engine/`): motor del ojo,
  configuración e interfaz de display.
- **Controlador del hardware** (`src/oled/`, `include/oled/`): drivers del
  SSD1306 (I2C y SPI) con acceso directo a dispositivos de Linux (`/dev/i2c-N`,
  `/dev/spidev`).
- **Punto de entrada** (`src/main.cpp`): creación de `Eye::Eye_t` con
  `std::make_unique` y llamada a `run()`.

```
┌─────────────┐
│ src/main.cpp│  Crea Eye::Eye_t (unique_ptr) y llama run(argc, argv)
└─────┬───────┘
      ▼
┌──────────────────────────────┐
│ Eye::Eye_t (motor del ojo)   │  parseArgs → loadConfig → initHardware → loop
│  • dibuja esclera/iris/pupila│
│  • parpadeo, expresiones     │
│  • modos: Normal/Tracking/...│
└─────┬────────────────────────┘
      │  usa Eye::IDisplay (interfaz)
      ▼
┌────────────────────────┬────────────────────────────┐
│ DisplayAdapterI2C      │ DisplayAdapterSPI          │
│  └─ SSD1306            │  └─ SSD1306_SPI            │
│     └─ SSD1306_I2C     │     └─ SSD1306LinuxSPI     │
│        └─ /dev/i2c-N   │        └─ /dev/spidev+GPIO │
└────────────────────────┴────────────────────────────┘
```

## Flujo de ejecución

1. `main()` crea `std::make_unique<Eye::Eye_t>()` y llama `eye->run(argc, argv)`.
2. `Eye::Eye_t::run()`:
   - `parseArgs()` procesa `--version`, `--mode`, `--config`, `--hw-config`.
   - Muestra la versión por consola (`printf("EyePet v%s")`).
   - Carga `config/hardware.cfg` y `config/config.cfg`.
   - `initHardware()` instancia el adaptador I2C o SPI según `protocol`.
   - `animationLoop()` ejecuta el bucle no bloqueante.
3. El destructor de `Eye_t` apaga el display solo si se inicializó
   (`hwReady_`), evitando tocar el bus cuando solo se mostró `--version`.

## Bucle de animación

Cada iteración:

1. Calcula `dt` (ms) con `steady_clock`.
2. Actualiza el parpadeo (`updateBlink`).
3. Actualiza el modo activo (`updateNormal`, `updateTracking`, ...).
4. Dibuja el ojo completo en el buffer (`drawEye`).
5. Añade el brillo si está habilitado (`addGlint`).
6. Vuelca el buffer al display (`update()`).
7. Espera `frame_delay_ms` para mantener la cadencia.

## Gestión de memoria

- Sin `new`/`delete` explícitos en el código de aplicación: se usan
  `std::make_unique` (ojo, adaptadores, drivers) y `std::vector`/`std::string`.
- El driver OLED asigna su frame buffer con `malloc`/`free` internamente para
  evitar el conflicto de la macro `swap` con `<new>`/STL.
- Flags: `hwReady_` (apagado seguro), `ready_` en adaptadores.

## Configuración

- `config/config.cfg` → comportamiento del ojo (`EyeConfig_t`).
- `config/hardware.cfg` → protocolo, dirección I2C, pines SPI.
- Parser: `src/engine/EyeConfig.cpp` (formato `clave = valor`, comentarios `#`).

## Compatibilidad de hardware

- **I2C por `/dev/i2c-N` con ioctl**: funciona en todas las Pi, incluidas
  Pi 5/CM5 (chip RP1) donde `bcm2835` no soporta I2C.
- **SPI por `/dev/spidev`**: 4 hilos (SCK, MOSI, DC, RST); DC/RST por sysfs GPIO.
- Arquitecturas: 32-bit (armhf) y 64-bit (aarch64) — ver `docs/BUILD.md`.

## Decisiones de diseño

1. **Interfaz `IDisplay`**: permite cambiar I2C/SPI en tiempo de ejecución sin
   tocar el motor del ojo.
2. **Driver sin `bcm2835`**: se usa `ioctl` de Linux (más robusto y compatible
   con RP1); `bcm2835` queda como dependencia opcional (solo `--with-bcm2835`).
3. **Bucle no bloqueante**: `sleep_for` corto al final de cada fotograma,
   permitiendo parpadeos y movimientos suaves sin cargar la CPU.
4. **Configuración por archivos**: todos los parámetros de comportamiento y
   hardware son ajustables sin recompilar.