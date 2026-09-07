#!/bin/bash
# =============================================================================
# fetch_sysroot.sh - Trae bcm2835 de la Raspberry Pi para compilación cruzada
# -----------------------------------------------------------------------------
# Copia desde una Raspberry Pi remota:
#   - /usr/include/bcm2835.h
#   - /usr/lib/aarch64-linux-gnu/libbcm2835.a  (lib estática aarch64)
# a un "sysroot" local (sysroot/<triple>/) que el Makefile usa para compilar
# en cruz (make ARCH=cross) en un PC x86-64.
#
# Requisitos:
#   - sshpass instalado.
#   - Variable SSHPASS con la contraseña del usuario remoto (sin divulgarse).
# =============================================================================
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEPLOY_CFG="$ROOT_DIR/config/deploy.cfg"

# triple por defecto (ARM64 Raspberry Pi de 64 bits)
TRIPLE="${1:-aarch64-linux-gnu}"

if ! command -v sshpass >/dev/null 2>&1; then
    echo "ERROR: 'sshpass' no está instalado (sudo apt install sshpass)."
    exit 1
fi

if [[ -z "${SSHPASS:-}" ]]; then
    echo "ERROR: La variable SSHPASS no está definida."
    echo "  Ejemplo:  read -s SSHPASS; export SSHPASS"
    exit 1
fi

# shellcheck disable=SC1090
source "$DEPLOY_CFG"

for var in REMOTE_USER REMOTE_HOST; do
    if [[ -z "${!var:-}" ]]; then
        echo "ERROR: '$var' no está definida en $DEPLOY_CFG"
        exit 1
    fi
done

# --- Comprobación de sshpass ---------------------------------------------------
if ! command -v sshpass >/dev/null 2>&1; then
    echo "ERROR: 'sshpass' no está instalado (sudo apt install sshpass)."
    exit 1
fi

if [[ -z "${SSHPASS:-}" ]]; then
    echo "ERROR: La variable SSHPASS no está definida."
    echo "  Ejemplo:  read -s SSHPASS; export SSHPASS"
    exit 1
fi

# shellcheck disable=SC1090
source "$DEPLOY_CFG"

for var in REMOTE_USER REMOTE_HOST; do
    if [[ -z "${!var:-}" ]]; then
        echo "ERROR: '$var' no está definida en $DEPLOY_CFG"
        exit 1
    fi
done

REMOTE_TARGET="$REMOTE_USER@$REMOTE_HOST"

# --- Directorios local del sysroot según el triple -----------------------------
SYSROOT="$ROOT_DIR/sysroot/$TRIPLE"
INC_DIR="$SYSROOT/usr/include"
LIB_DIR="$SYSROOT/usr/lib/$TRIPLE"

# Nombres "corregidos" una vez traídos localmente
INC_LOCAL="$INC_DIR/bcm2835.h"
LIB_LOCAL="$LIB_DIR/libbcm2835.a"

# --- Mapear triple → ruta de la librería en la Raspberry Pi --------------------
# Se prueban varias rutas según cómo esté instalada bcm2835 en la Pi:
#   - /usr/local/lib  (instalación manual, p.ej. rpi2w con bcm2835 en /usr/local)
#   - /usr/lib/<triple>  (instalación por paquete libbcm2835-dev)
# Solo se busca una librería que corresponda con el triple pedido (32 vs 64 bit).
case "$TRIPLE" in
    aarch64-linux-gnu)
        REMOTE_LIB_CANDIDATES="/usr/local/lib/libbcm2835.a /usr/lib/aarch64-linux-gnu/libbcm2835.a"
        ;;
    arm-linux-gnueabihf|arm-linux-gnueabi)
        REMOTE_LIB_CANDIDATES="/usr/local/lib/libbcm2835.a /usr/lib/$TRIPLE/libbcm2835.a"
        ;;
    *)
        echo "ERROR: Triple no soportado: '$TRIPLE'"
        echo "  Usa: aarch64-linux-gnu  |  arm-linux-gnueabihf"
        exit 1
        ;;
esac

echo ">>> Localizando bcm2835 ($TRIPLE) en $REMOTE_TARGET ..."
# Encuentra la primera ruta que exista en la Raspberry Pi.
REMOTE_LIB_PATH=$(sshpass -e ssh -o StrictHostKeyChecking=no -o ConnectTimeout=20 \
    "$REMOTE_TARGET" "for f in $REMOTE_LIB_CANDIDATES; do [ -f \"\$f\" ] && { echo \"\$f\"; break; }; done")

if [[ -z "$REMOTE_LIB_PATH" ]]; then
    echo "ERROR: No se encontró la librería para el triple '$TRIPLE' en '$REMOTE_TARGET'."
    echo "  Rutas probadas: $REMOTE_LIB_CANDIDATES"
    echo "  Si usas crossover32 necesitas una Raspberry Pi de 32 bits"
    echo "  (o instala: sudo apt install libbcm2835-dev) y luego:"
    echo "      ./scripts/fetch_sysroot.sh $TRIPLE"
    exit 1
fi

# --- Crear estructura local y descargar ----------------------------------------
mkdir -p "$INC_DIR" "$LIB_DIR"

echo ">>> Descargando cabecera bcm2835.h ..."
scp -q -o StrictHostKeyChecking=no "$REMOTE_TARGET:/usr/include/bcm2835.h" "$INC_LOCAL"

echo ">>> Descargando $REMOTE_LIB_PATH ..."
scp -q -o StrictHostKeyChecking=no "$REMOTE_TARGET:$REMOTE_LIB_PATH" "$LIB_LOCAL"

echo ""
echo ">>> Sysroot listo para '$TRIPLE':"
echo "    Include:   $INC_LOCAL"
echo "    Librería:  $LIB_LOCAL"
echo ""
echo "    Ahora puedes compilar en cruz con:  make crossover"
echo "    (o, para este triple concreto:      make ARCH=cross CROSS_TRIPLE=$TRIPLE)"
