# EyePet — Skills (destrezas del proyecto)

Habilidades y conocimientos aplicados en EyePet. Complementa a
`docs/LEARNINGS.md` (que registra los errores resueltos).

## C/C++ embebido en Linux

- Programación C++17 de bajo nivel sin frameworks (Makefile + g++).
- Gestión de memoria moderna: `std::make_unique`, `std::vector`, `std::string`.
- **Evitar el conflicto de macros** (ej. `swap` del driver OLED vs. STL).

## Acceso a periféricos sin librerías privativas

- **I2C** por `ioctl` sobre `/dev/i2c-N` (estilo `i2cset`/`i2cget`).
- **SPI** por `ioctl` sobre `/dev/spidevX.Y` (`SPI_IOC_MESSAGE`).
- **GPIO** por sysfs (`/sys/class/gpio/export`, `direction`, `value`).
- Conocimiento clave: `bcm2835` NO funciona con I2C en Pi 5/CM5 (chip RP1).

## SSD1306 (OLED)

- Juego de comandos del controlador (init, modo horizontal, buffer por páginas).
- Frame buffer en memoria (1 byte = 8 píxeles verticales) con un único `update()`.
- Primitivas gráficas Adafruit-style (líneas, círculos, rectángulos, texto,
  fuentes).

## Animación

- Bucle no bloqueante con cadencia estable (`sleep_for` corto por fotograma).
- Interpolación suave hacia objetivos (movimiento "orgánico").
- Máquinas de estado simples para parpadeo/expresiones.
- Patrones pseudoaleatorios con semilla por tiempo.

## Versionado y despliegue

- Conventional commits + tags por push (`VERSION` en raíz).
- Compilación cruzada ARM32/ARM64 desde PC.
- Despliegue remoto seguro con `sshpass`/`SSHPASS` (sin secretos en el repo).
- GitHub CLI (`gh`) para crear repos y pushes automáticos.

## Documentación

- Estructura completa de `docs/` (API, arquitectura, uso, build, troubleshooting).
- Doxygen para generar documentación (el directorio `docs/doxygen/` está listo).