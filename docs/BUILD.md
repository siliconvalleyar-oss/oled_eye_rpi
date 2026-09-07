# EyePet — Compilación (BUILD)

Instrucciones para compilar EyePet en distintas plataformas.

## Requisitos

- Compilador C++17 (`g++`), `make`, `git`, `pkg-config`.
- En la Raspberry Pi: `i2c-tools` (para diagnóstico). Sin dependencias de
  librerías externas (el acceso al HW es por `ioctl` de Linux).

## Compilación nativa (en la Raspberry Pi)

```sh
make                 # o make -j4 para paralelizar
```

Resultado: `bin/App`.

## Compilación cruzada (desde un PC x86_64)

```sh
make crossover       # detecta arquitectura automáticamente
make crossover64     # ARM 64-bit (aarch64-linux-gnu)
make crossover32     # ARM 32-bit (arm-linux-gnueabihf)
```

El cruce requiere el sysroot de la arquitectura de destino. Se obtiene desde la
propia Raspberry Pi con:

```sh
scripts/fetch_sysroot.sh aarch64-linux-gnu    # para 64 bits
scripts/fetch_sysroot.sh arm-linux-gnueabihf  # para 32 bits
```

Instala los toolchains cruzados con la distribución del PC, p. ej. en Debian/Ubuntu:

```sh
sudo apt install g++-aarch64-linux-gnu g++-arm-linux-gnueabihf
```

> **Nota**: el proyecto NO requiere `libbcm2835.a` ni `bcm2835.h` para compilar
> (el driver OLED usa `/dev/i2c-N` y `/dev/spidev`). El sysroot solo aporta
> las librerías de sistema del destino; con un sysroot vacío bastará si el
> código no usa más librerías que la libc/libm/pthread.

## Compilación remota (en la propia Pi vía SSH)

```sh
make remote
```

Usa `scripts/build_remote.sh`, que hace `git pull` + `make clean` + `make -j4`
en el host definido en `config/deploy.cfg`. Requiere `sshpass` y la variable
`SSHPASS` (contiene la contraseña) para `ssh`.

## Volcado del binario a la Pi

```sh
make deploy
```

Copia `bin/App` al host de `config/deploy.cfg` con `scp` (idéntico mecanismo
`sshpass -e`).

## Objetivos del Makefile

| Objetivo | Descripción |
|----------|-------------|
| `all` | Compila `bin/App`. |
| `clean` | Elimina `obj/` y `bin/App`. |
| `distclean` | Además borra `sysroot/`. |
| `crossover` / `crossover32` / `crossover64` | Cruce automático / 32 / 64 bits. |
| `remote` / `deploy` | Compila o despliega en la Pi remota. |
| `run` / `runnosudo` | Ejecuta `bin/App` (con sudo opcional). |
| `info` | Muestra versión, arquitectura y lista de objetos. |

## Versión en tiempo de compilación

El Makefile lee el número del archivo `VERSION` y lo inyecta con
`-DVERSION="<núm>"`. La app lo muestra al iniciar y con `--version`. Para
versionar, actualiza `VERSION` antes de compilar.

## Verificación

- `./bin/App --version` debe imprimir `EyePet v1.0.0` y salir sin tocar el HW.
- En la Pi, `make run` ejecuta la animación (requiere display conectado).
- Diagnóstico rápido de I2C: `sudo i2cdetect -y 1` (el display aparece en 0x3C).