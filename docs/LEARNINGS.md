# LEARNINGS — Aprendizajes y reglas del proyecto EyePet

Conocimiento acumulado durante el desarrollo del proyecto. **Última actualización:
2026-09-07 (generación inicial).**

## Hardware / I2C / OLED

- **`bcm2835` NO funciona para I2C en Raspberry Pi 5 / Compute Module 5.**
  La librería accede a los registros del SoC BCM2835/2711, que en la Pi 5/CM5
  residen en el chip RP1 con un mapa de registros distinto. El síntoma es
  `bcm2835I2CReasonCodes :: Error code 1` (NACK) en cada escritura, aunque
  `i2cset`/`i2cget`/`i2cdetect` funcionen bien.
- **Solución: acceder al I2C por `/dev/i2c-N` con ioctl de Linux** (igual que
  hacen `i2cset`/`i2cget`). Funciona en todas las Raspberry Pi. Este proyecto
  usa esa capa (`SSD1306_I2C`) y no enlaza contra `bcm2835`.
- **Soporte SPI** con la misma filosofía: `/dev/spidev` (ioctl) + pines DC/RST
  por GPIO de sysfs (`/sys/class/gpio`). También funciona en todas las Pi.
- **`i2cdetect` vacío ≠ problema de configuración**: si el bus no responde en
  ninguna dirección, es un problema eléctrico (SDA↔GPIO2, SCL↔GPIO3, VCC, GND)
  o una dirección distinta (`0x3D`).
- **Un "bonnet" ST7789 SPI sobre el header de 40 pines cubre GPIO2/GPIO3**;
  si ocupa los pines 3 y 5, retíralo para liberar el I2C.
- Después de editar `/boot/config.txt` hay que **reiniciar** y verificar con
  `sudo i2cdetect -y 1`.

## C++ / librería OLED

- **El buffer del `SSD1306` debe asignarse en el constructor** (era `nullptr`);
  `OLEDclearBuffer()` hacía `memset` sobre NULL → segfault. Se usa `malloc/free`
  para evitar el conflicto de la macro `swap` con `<new>`/STL.
- **Danger: la macro global `swap(a,b)`** definida en `SSD1306_OLED_graphics.hpp`
  rompe `std::swap`/`<memory>`. Se evita con `#undef swap` tras incluir las
  cabeceras OLED (y por eso el driver asigna su buffer con `malloc`, no `new[]`).
- **El destructor no debe tocar el I2C si no hubo init**: usar una flag
  (`hwReady_` / `ready_`) para que `--version` salga sin tocar el hardware.
- **Bucle no bloqueante**: `sleep_for` corto al final del fotograma; los
  movimientos se interpolan hacia objetivos (suavizado ≈ "orgánico").

## Git / versionado (reglas del proyecto)

- **Todo push lleva su tag.** No se pushea sin tag.
- `VERSION` (archivo en la raíz) debe coincidir con el último tag (con y sin `v`):
  `git tag v1.0.0` → `VERSION` = `1.0.0`.
- Un tag por rama/push; los commits significativos llevan tag secuencial
  (patch 0-9, luego minor: `v1.0.9` → `v1.1.0`). No retroceder versiones.
- Commits con *conventional commits*: `feat:`, `fix:`, `docs:`, `refactor:`,
  `chore:`, etc.
- Flujo: actualizar `VERSION` → `git add -A` → `git commit -m "tipo: ..."` →
  `git push origin <rama>` → `git tag -a vX.Y.Z -m "..."` → `git push origin vX.Y.Z`.
- No eliminar tags publicados; ante error, crear el siguiente número.

## Compilación / despliegue

- Compilación local: `make`, `make crossover` (auto-detecta), `make crossover64`
  (aarch64), `make crossover32` (arm-linux-gnueabihf).
- Remoto: `make remote` (`scripts/build_remote.sh`) o `make deploy`
  (`scripts/deploy.sh`); ambos usan `sshpass -e` con la variable `SSHPASS`
  (nunca guardar la contraseña en el repo).
- `sudo ./bin/App --version` valida la versión sin abrir el bus I2C.

## Decisiones de diseño registradas (para no repetir)

1. Mantener el driver I2C funcional (léase: no reescribir con `bcm2835`).
2. Añadir SPI como driver independiente (no acoplar al I2C existente).
3. Separar el motor del ojo del hardware mediante la interfaz `IDisplay`.
4. Reiniciar el versionado del proyecto en `1.0.0` al re-structurar el proyecto.