# EyePet — Configuración del entorno (SETUP)

Pasos para preparar el entorno de desarrollo y ejecución.

## En la Raspberry Pi

```sh
# 1. Sistema actualizado
sudo apt update && sudo apt upgrade -y

# 2. Dependencias de EyePet
sudo ./scripts/install_deps.sh

# 3. Reiniciar (habilita I2C/SPI en /boot/config.txt)
sudo reboot

# 4. Verificar el display
sudo i2cdetect -y 1      # debe mostrar: 0x3c (o 0x3d)

# 5. Compilar y ejecutar
make -j4
sudo ./bin/App --version
sudo ./bin/App
```

## En el PC de desarrollo (compilación cruzada)

```sh
# Toolchains
sudo apt install g++-aarch64-linux-gnu g++-arm-linux-gnueabihf sshpass

# Sysroots desde la Pi (opcional; el código solo usa libc/libm/pthread)
scripts/fetch_sysroot.sh aarch64-linux-gnu

# Compilar
make crossover64
make crossover32
```

## Configuración del repositorio Git

```sh
./setup_git.sh
```

Pregunta usuario, credenciales, nombre del repo y visibilidad; inicializa el
repo, hace el commit inicial y crea el remoto con `gh` (o da los pasos manuales).

- Instala `gh`: `sudo apt install gh && gh auth login`.

## Edición de la configuración

| Archivo | Qué contiene |
|---------|--------------|
| `config/config.cfg` | Modo inicial, velocidad, parpadeo, rango, geometría, brillo. |
| `config/hardware.cfg` | Protocolo (i2c/spi), dirección, pines. |
| `config/deploy.cfg` | Host remoto para `make remote`/`make deploy`. |

No es necesario recompilar para cambiar parámetros de `config.cfg`; sí para
cambiar protocolo si requiere nueva compilación del adaptador (aunque el selector
es dinámico en tiempo de ejecución).

## Verificación final

```sh
./bin/App --version        # EyePet v1.0.0
sudo ./bin/App --mode 0    # ojo normal explorando
```