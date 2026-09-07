#!/bin/bash
# =============================================================================
# deploy.sh - Copia el binario compilado a la Raspberry Pi
# -----------------------------------------------------------------------------
# Transfiere bin/App hacia REMOTE_DIR/bin/ en la Raspberry Pi mediante scp.
# La contraseña se lee de la variable de entorno SSHPASS (nunca se guarda).
# =============================================================================
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEPLOY_CFG="$ROOT_DIR/config/deploy.cfg"

if ! command -v sshpass >/dev/null 2>&1; then
    echo "ERROR: 'sshpass' no está instalado (sudo apt install sshpass)."
    exit 1
fi

if [[ -z "${SSHPASS:-}" ]]; then
    echo "ERROR: La variable SSHPASS no está definida."
    exit 1
fi

# shellcheck disable=SC1090
source "$DEPLOY_CFG"

for var in REMOTE_USER REMOTE_HOST REMOTE_DIR; do
    if [[ -z "${!var:-}" ]]; then
        echo "ERROR: '$var' no está definida en $DEPLOY_CFG"
        exit 1
    fi
done

REMOTE_TARGET="$REMOTE_USER@$REMOTE_HOST"
LOCAL_BIN="$ROOT_DIR/bin/App"

if [[ ! -f "$LOCAL_BIN" ]]; then
    echo "ERROR: No existe $LOCAL_BIN. Compila primero (make o make ARCH=cross)."
    exit 1
fi

# Crea el directorio remoto si no existe y copia el binario.
sshpass -e ssh -o StrictHostKeyChecking=no -o ConnectTimeout=20 \
    "$REMOTE_TARGET" "mkdir -p $REMOTE_DIR/bin"
sshpass -e scp -o StrictHostKeyChecking=no \
    "$LOCAL_BIN" "$REMOTE_TARGET:$REMOTE_DIR/bin/App"

echo ">>> Binario desplegado en $REMOTE_TARGET:$REMOTE_DIR/bin/App"
