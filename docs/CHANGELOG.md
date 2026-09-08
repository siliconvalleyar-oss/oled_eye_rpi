# EyePet — CHANGELOG

Historial de versiones de EyePet.

## [v1.2.0] — 2026-09-07

### Añadido

- Dos estilos de ojo nuevos (total 6, `--style 0..5`):
  - `4 = Squint`: entrecerrado con párpados dobles gruesos.
  - `5 = Heart`: pupila en forma de corazón (dos círculos + triángulo).
- Menú interactivo por teclado (sin pulsar Enter) durante la ejecución:
  - `0`/`T`: rotar por todos los estilos; `1..6`: elegir un ojo concreto.
  - `b` parpadeo, `g` brillo, `e` cejas, `+`/`-` velocidad (fps),
    `c`/`C` contraste, `p`/`P` dilatación de la pupila, `d` depuración,
    `h` ayuda, `q`/`Q`/`Esc` salir.
- Demo guiada de 3 minutos (configurable con `demo_seconds` en `config.cfg`
  y `--demo <segundos>`) que muestra todos los efectos: parpadeo espontáneo,
  mirada de izquierda a derecha, feliz (pupila dilatada), triste (párpado
  medio y mirada baja), guiño lento y párpado cerrado (dormido).
- Parpadeo real con fases animadas (cerrar → cerrado → abrir con easing);
  la demo controla `blinkPhase_` cuando corresponde.
- `--style`/`--mode`/`--demo` de la CLI ahora tienen prioridad sobre
  `config/config.cfg` (antes la configuración pisaba la opción de consola).

### Cambiado

- `config.cfg`: comentarios de estilos ampliados a 0..5 y nueva clave
  `demo_seconds = 180`.
- `--help` muestra los 6 estilos y la opción `--demo`.

## [v1.1.0] — 2026-09-07

### Añadido

- Estilos (versiones) de ojo configurables con `style` en `config.cfg` y
  `--style 0..3`:
  - `0 = Classic`: ojo redondeado clásico, pupila circular, un brillo.
  - `1 = Anime`: elipse grande a pantalla completa, pupila amplia y 2 brillos.
  - `2 = Feline`: felino con iris marcado y pupila vertical alargada.
  - `3 = Robot`: visor rectangular, pupila cuadrada con retícula.
- El ojo ahora ocupa toda la pantalla (128×64): geometría por defecto con
  `sclera_r = 30` centrada en `(64, 32)` (antes `20` en `(64, 36)`).
- Ojo cerrado con arco suave (`∪`) en lugar de línea recta (estética de
  párpado relajado); los estilos Anime y Feline usan su propia variante.
- El brillo/reflejo solo se dibuja si el ojo está suficientemente abierto
  (no aparece durante el parpadeo).

### Cambiado

- Radios por defecto ajustados para el tamaño a pantalla completa
  (`iris_r = 16`, `pupil_r = 6`, `move_range_x = 14`, `move_range_y = 8`).

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