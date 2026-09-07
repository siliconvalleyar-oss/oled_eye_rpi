# EyePet — Solución de problemas (TROUBLESHOOTING)

Problemas frecuentes y sus soluciones.

## I2C

### El display no aparece con `i2cdetect` (bus vacío)
- **Causa**: problema eléctrico o dirección distinta.
- **Soluciones**:
  - Verifica cableado: SDA→GPIO2 (pin 3), SCL→GPIO3 (pin 5), VCC→3.3V, GND.
  - Prueba dirección `0x3D` (algunos módulos): `sudo i2cdetect -y 1`.
  - Retira cualquier "bonnet" que cubra los pines 3 y 5.
  - Si el bus sigue vacío, el problema es eléctrico (no de configuración).

### `Permission denied` al abrir /dev/i2c-1
- **Causa**: usuario sin permiso sobre `/dev/i2c-N`.
- **Solución**: ejecuta con `sudo` o añade tu usuario al grupo `i2c`
  (`sudo usermod -a -G i2c $USER`) y reinicia sesión.

### Error "Cannot start I2C"
- **Causa**: el archivo `/dev/i2c-1` no existe o I2C no está habilitado.
- **Solución**: habilita con `dtparam=i2c_arm=on` en `/boot/config.txt` y reinicia
  (`scripts/install_deps.sh` hace esto automáticamente).

### NACK en cada escritura en Pi 5 / CM5
- **Causa**: se está usando `bcm2835` (no compatible con I2C en chip RP1).
- **Solución**: EyePet usa `/dev/i2c-N` (ioctl) → no usa `bcm2835`. Asegúrate de
  que `config/hardware.cfg` tenga `protocol = i2c`.

## SPI

### No se encuentra `/dev/spidev0.0`
- **Causa**: SPI no habilitado.
- **Solución**: `dtparam=spi=on` en `/boot/config.txt` y reinicia.

### No se puede exportar GPIO (sysfs)
- **Causa**: el sysfs GPIO está deprecado o require permisos de root.
- **Solución**: ejecuta con `sudo`; verifica que el GPIO no esté usado por otra
  capa (overlay). Ajusta `spi_dc_pin`/`spi_rst_pin` en `hardware.cfg`.

## Compilación

### `g++: command not found`
- Instala herramientas: `sudo apt install build-essential g++ make`.

### Compilación cruzada falla (missing sysroot)
- Crea `sysroot/<triple>/` con `scripts/fetch_sysroot.sh` o instala solo las
  librerías de sistema del destino. El proyecto solo usa libc/libm/pthread.

### Conflicto de la macro `swap`
- Es un problema histórico del driver OLED; ya resuelto en el proyecto
  (`#undef swap` y `malloc` en el buffer). No introduces `#include <new>`
  antes de las cabeceras OLED en código nuevo.

## Ejecución

### `EyePet v1.0.0` no se muestra
- Verifica que `make` se ejecutó tras actualizar `VERSION` (la versión se
  inyecta en tiempo de compilación).

### La animación no se ve (pantalla en blanco/parpadeo)
- Verifica contraste (`OLEDContrast`) y que `glint = true` no tape el ojo.
- Comprueba `debug = true` en `config.cfg` (imprime el estado del bucle).
- Prueba `sudo ./bin/App --fill`? (No existe en EyePet; usa los modos 0..7).

### `Segfault` o `Aborted` al iniciar
- Ocurrió en versiones previas si el buffer era NULL o el destructor tocaba el
  I2C sin init. En EyePet: `hwReady_`/`ready_` lo evitan. Si reaparece, abre un
  issue indicando la salida de `dmesg`.

## Versionado / Git

### `git push` pide credenciales
- Usa las credenciales de la PC: `gh auth login` o configura el credential
  helper (`git config --global credential.helper store`). El token con scope
  `repo` basta.

### `VERSION` y tag no coinciden
- Regla: tag `v1.0.0` → `VERSION` = `1.0.0`. Actualiza `VERSION` a mano si
  difieren antes de pushear.

## Despliegue remoto

### `make remote` falla (SSH)
- Configura `config/deploy.cfg` (user/host/dir/branch).
- Exporta la contraseña: `read -s SSHPASS; export SSHPASS` y ejecuta `make remote`.

## Otros

### El display parpadea débil
- Baja la frecuencia de actualización (`frame_rate`) o sube el contraste
  (`OLEDContrast(0xFF)`).

### Alto consumo / CPU
- Reduce `frame_rate`; en reposo usa `--mode 6` (dormido).