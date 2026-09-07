#!/usr/bin/env bash
# =============================================================================
# EyePet - Instalación de dependencias para Raspberry Pi (32 y 64 bits)
# -----------------------------------------------------------------------------
# Instala:
#   - Herramientas de compilación: g++, make, git, build-essential.
#   - Herramientas I2C/SPI: i2c-tools, spidev (kernel), librerías del kernel.
#   - Driver I2C del kernel activo y usuario en el grupo "i2c".
#   - (Opcional) librería bcm2835 (solo SPI/GPIO de bajo nivel; el OLED usa
#     /dev/i2c-N y /dev/spidev, no bcm2835, pero se incluye para compatibilidad).
#
# Uso:
#   sudo ./scripts/install_deps.sh
#   sudo ./scripts/install_deps.sh --with-bcm2835   (incluye instalación de bcm2835)
#
# Compatible con Raspberry Pi OS de 32 bits (armhf) y 64 bits (arm64/aarch64).
# =============================================================================

set -e

# --- Configuración -----------------------------------------------------------
BCM2835_VERSION="1.75"
BCM2835_URL="http://www.airspayce.com/mikem/bcm2835/bcm2835-${BCM2835_VERSION}.tar.gz"
INCLUDE_BCM2835=0

# --- Parsea argumentos -------------------------------------------------------
for arg in "$@"; do
    case "$arg" in
        --with-bcm2835) INCLUDE_BCM2835=1 ;;
        *) echo "EyePet: argumento desconocido '$arg' (use --with-bcm2835)" ;;
    esac
done

echo "=== EyePet: instalando dependencias ==="

# --- Comprueba privilegios ---------------------------------------------------
if [ "$(id -u)" != "0" ]; then
    echo "Error: ejecuta con sudo: sudo ./scripts/install_deps.sh"
    exit 1
fi

# --- Arquitectura detectada (solo informativa) ------------------------------
ARCH="$(uname -m)"
case "$ARCH" in
    aarch64|arm64)  echo "Arquitectura detectada: 64 bits (aarch64)" ;;
    armv7l|armhf|armv6l) echo "Arquitectura detectada: 32 bits (arm)" ;;
    *) echo "Arquitectura detectada: $ARCH (no es una Raspberry Pi típica)" ;;
esac

# --- Actualiza índices de paquetes -------------------------------------------
echo ">>> Actualizando índices de apt..."
apt-get update -y

# --- Paquetes básicos ---------------------------------------------------------
echo ">>> Instalando herramientas de compilación..."
apt-get install -y \
    build-essential \
    g++ \
    make \
    git \
    pkg-config

# --- Herramientas I2C y dependencias del kernel -------------------------------
echo ">>> Instalando herramientas I2C/SPI y grupos..."
apt-get install -y \
    i2c-tools \
    linux-headers-$(uname -r) || true

# Asegura que el usuario "pi" (si existe) esté en el grupo i2c para acceder
# a /dev/i2c-N sin sudo.
if id -u pi >/dev/null 2>&1; then
    echo ">>> Añadiendo usuario 'pi' al grupo i2c y spi..."
    usermod -a -G i2c pi || true
    usermod -a -G spi pi   || true
fi

# --- Habilita I2C y SPI en /boot/config.txt (si procede) ----------------------
CONFIG_FILE="/boot/config.txt"
if [ -f "$CONFIG_FILE" ]; then
    echo ">>> Habilitando I2C en $CONFIG_FILE..."
    grep -q "^dtparam=i2c_arm=on" "$CONFIG_FILE" || \
        echo "dtparam=i2c_arm=on" >> "$CONFIG_FILE"

    echo ">>> Habilitando SPI en $CONFIG_FILE..."
    grep -q "^dtparam=spi=on" "$CONFIG_FILE" || \
        echo "dtparam=spi=on" >> "$CONFIG_FILE"

    echo ">>> Recuerda reiniciar la Raspberry Pi para aplicar los cambios: sudo reboot"
else
    echo "WARN: no se encontró $CONFIG_FILE (no es Raspberry Pi OS)."
fi

# --- Librería bcm2835 (opcional) ---------------------------------------------
if [ "$INCLUDE_BCM2835" = "1" ]; then
    echo ">>> Descargando e instalando bcm2835 v${BCM2835_VERSION}..."
    TMPDIR="$(mktemp -d)"
    cd "$TMPDIR"
    wget -q "${BCM2835_URL}" -O bcm2835.tar.gz || curl -sL "${BCM2835_URL}" -o bcm2835.tar.gz
    tar xzf bcm2835.tar.gz
    cd "bcm2835-${BCM2835_VERSION}"
    ./configure
    make
    make check || true
    make install
    cd /
    rm -rf "$TMPDIR"
    echo ">>> bcm2835 instalado."
else
    echo ">>> Nota: la librería bcm2835 NO es necesaria (el OLED usa /dev/i2c-N y"
    echo "    /dev/spidev). Para instalarla pasa el argumento --with-bcm2835."
fi

# --- Verificación rápida ------------------------------------------------------
echo ""
echo "=== Instalación completada ==="
echo "Comprueba el display con:  sudo i2cdetect -y 1"
echo "Reinicia tras editar /boot/config.txt:  sudo reboot"
echo "Luego compila con:  make && sudo ./bin/App"