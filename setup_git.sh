#!/usr/bin/env bash
# =============================================================================
# EyePet - Configuración del repositorio Git (interactivo)
# -----------------------------------------------------------------------------
# Pregunta al usuario:
#   1. Nombre de usuario en la plataforma (GitHub, GitLab, etc.).
#   2. Credenciales (token de acceso o que estén ya configuradas globalmente).
#   3. Nombre del repositorio (coincide con el nombre del proyecto).
#   4. Visibilidad: pública o privada.
#
# Después ejecuta, si es posible:
#   git init
#   git add .
#   git commit -m "Initial commit"
#   gh repo create <nombre> --public|--private --source=. --remote=origin --push
#   git push -u origin main|master
#
# Si no se dispone de GitHub CLI (gh), el script crea el repositorio localmente
# y muestra los pasos manuales para crearlo en la web y conectarlo.
#
# Uso:
#   ./setup_git.sh
# =============================================================================

set -e

echo "=== EyePet: configuración del repositorio Git ==="

# --- Leer datos del usuario ---------------------------------------------------
read -rp "Nombre de usuario en la plataforma (GitHub/GitLab): " GIT_USER
read -rp "Nombre del repositorio (coincide con el proyecto): " REPO_NAME

if [ -z "$REPO_NAME" ]; then
    REPO_NAME="eye-oled"
fi

echo "Visibilidad del repositorio:"
select VIS in "public" "private"; do
    if [ -n "$VIS" ]; then
        break
    else
        echo "Selecciona 1 (public) o 2 (private)."
    fi
done

echo ""
echo "Credenciales:"
echo "  1) Ya tengo credenciales configuradas globalmente (recomendado)"
echo "  2) Usar un token de acceso ya creado en la plataforma"
read -rp "Opción [1/2]: " AUTH_OPT

GH_TOKEN=""
if [ "$AUTH_OPT" = "2" ]; then
    read -rsp "Pega tu token de acceso (no se mostrará): " GH_TOKEN
    echo ""
    export GH_TOKEN="$GH_TOKEN"
fi

# --- Verificar herramientas ---------------------------------------------------
HAS_GH=0
if command -v gh >/dev/null 2>&1; then
    HAS_GH=1
else
    echo ""
    echo "Aviso: no se encontró 'gh' (GitHub CLI)."
    echo "  Instálalo con:  sudo apt install gh  (o usa los pasos manuales)"
fi

# --- Inicializar el repositorio local -----------------------------------------
if [ -d .git ]; then
    echo "El directorio ya contiene un repositorio Git (.git). Se reutiliza."
else
    echo ">>> git init"
    git init -b main
fi

echo ">>> git add ."
git add .

if git diff --cached --quiet; then
    echo "No hay cambios pendientes para el commit inicial."
else
    echo ">>> git commit"
    git -c user.name="${GIT_USER:-EyePet}" \
        -c user.email="${GIT_USER}@users.noreply.${PLATFORM:-github}.com" \
        commit -m "Initial commit"
fi

# --- Crear repositorio remoto y subir -----------------------------------------
if [ "$HAS_GH" = "1" ]; then
    echo ">>> gh repo create $REPO_NAME --$VIS --source=. --remote=origin --push"
    if gh auth status >/dev/null 2>&1; then
        gh repo create "$REPO_NAME" --"$VIS" --source=. --remote=origin --push || {
            echo "WARN: falló la creación remota (¿el repositorio ya existe?)."
            echo "  Puedes reutilizar uno existente con:"
            echo "    git remote add origin git@github.com:${GIT_USER}/${REPO_NAME}.git"
            echo "    git push -u origin main"
        }
    else
        echo ""
        echo "No hay sesión activa de gh. Inicia sesión con:  gh auth login"
        echo "Después vuelve a ejecutar este script o crea el repo manualmente."
    fi
else
    echo ""
    echo "=== Pasos manuales para el repositorio remoto ==="
    echo "1. Crea en la web un repositorio llamado '$REPO_NAME' ($VIS) para '$GIT_USER'."
    echo "2. Conéctalo con:"
    echo "     git remote add origin git@github.com:${GIT_USER}/${REPO_NAME}.git"
    echo "     git push -u origin main"
    echo ""
    echo "Si el nombre de la rama principal es 'master', usa:"
    echo "     git branch -M master && git push -u origin master"
fi

echo ""
echo "=== Repositorio Git configurado ==="
git log --oneline -1 2>/dev/null || true
echo "Verifica el remoto con:  git remote -v"