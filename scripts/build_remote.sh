#!/bin/bash
# =============================================================================
# build_remote.sh - Compilación remota en la Raspberry Pi
# -----------------------------------------------------------------------------
# Realiza la compilación de forma remota en la Raspberry Pi mediante SSH:
#     1. Cambia a la rama de despliegue (config/deploy.cfg).
#     2. Ejecuta `git pull` para traer los últimos cambios.
#     3. Ejecuta `make clean && make -j4` en la Pi.
#
# Requisitos:
#   - sshpass instalado.
#   - La variable de entorno SSHPASS con la contraseña del usuario remoto.
#     (NO se guarda la contraseña en ningún archivo del repositorio.)
# =============================================================================
set -euo pipefail

# --- Directorio base (raíz del proyecto) --------------------------------------
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEPLOY_CFG="$ROOT_DIR/config/deploy.cfg"

# --- Comprobación de sshpass ---------------------------------------------------
if ! command -v sshpass >/dev/null 2>&1; then
    echo "ERROR: 'sshpass' no está instalado."
    echo "  Instálalo con: sudo apt install sshpass"
    exit 1
fi

# --- Comprobación de la contraseña (variable SSHPASS, sin divulgar su valor) ---
if [[ -z "${SSHPASS:-}" ]]; then
    echo "ERROR: La variable de entorno SSHPASS no está definida."
    echo "  Ejemplo seguro:  read -s SSHPASS; export SSHPASS"
    echo "  o desde un gestor de secretos / agente."
    exit 1
fi

# --- Leer configuración de despliegue ------------------------------------------
if [[ ! -f "$DEPLOY_CFG" ]]; then
    echo "ERROR: No se encontró $DEPLOY_CFG"
    exit 1
fi

# shellcheck disable=SC1090
source "$DEPLOY_CFG"

# --- Validar los valores mínimos ------------------------------------------------
for var in REMOTE_USER REMOTE_HOST REMOTE_DIR REMOTE_BRANCH; do
    if [[ -z "${!var:-}" ]]; then
        echo "ERROR: La variable '$var' no está definida en $DEPLOY_CFG"
        exit 1
    fi
done

REMOTE_TARGET="$REMOTE_USER@$REMOTE_HOST"

echo ">>> Compilación remota en $REMOTE_TARGET"
echo ">>> Ruta: $REMOTE_DIR (rama: $REMOTE_BRANCH)"

# --- Comando que se ejecutará dentro de la Raspberry Pi -------------------------
# Cambia a la rama de despliegue y asegura el upstream hacia origin para que
# "git pull" funcione tanto la primera vez como en ejecuciones posteriores.
REMOTE_CMD="cd $REMOTE_DIR \
  && git fetch origin \
  && if git rev-parse --verify -q $REMOTE_BRANCH >/dev/null; then \
       git checkout $REMOTE_BRANCH; \
     else \
       git checkout -b $REMOTE_BRANCH origin/$REMOTE_BRANCH; \
     fi \
  && (git branch --set-upstream-to=origin/$REMOTE_BRANCH $REMOTE_BRANCH 2>/dev/null || true) \
  && git pull \
  && make clean \
  && make -j4"

# --- Ejecutar vía SSH usando sshpass -e (lee la contraseña de $SSHPASS) ---------
sshpass -e ssh -o StrictHostKeyChecking=no -o ConnectTimeout=20 \
    "$REMOTE_TARGET" "$REMOTE_CMD"

echo ">>> Compilación remota finalizada correctamente."
