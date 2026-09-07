#!/bin/bash
# =============================================================================
# EyePet - Genera la estructura base del proyecto (directorios y archivos).
# -----------------------------------------------------------------------------
# Idempotente: crea los directorios y archivos que falten, sin sobrescribir
# los existentes. Uso:
#   ./generate_basic_src.sh
# =============================================================================

echo "=== EyePet: generando estructura base ==="

# --- Directorios --------------------------------------------------------------
mkdir -p bin config docs examples \
         include/core include/drivers include/engine \
         include/libraries include/nlohmann include/oled include/security \
         obj scripts src/engine src/oled docs/doxygen

# --- Archivos base (solo si no existen) ----------------------------------------
touch -a bin/App 2>/dev/null || true

for f in \
  config/config.cfg config/hardware.cfg \
  docs/ACTIVITY.md docs/API.md docs/ARCHITECTURE.md docs/ARQUITECTURA.md \
  docs/BLUETOOTH.md docs/BUILD.md docs/CHANGELOG.md docs/CONTRIBUTING.md \
  docs/DEPLOY.md docs/DESING.md docs/DIAGRAMS.md docs/HARDWARE.md \
  docs/INSTALL.md docs/LEARNINGS.md docs/MEMORY_MAP.md docs/PROMPT.md \
  docs/REPORT.md docs/ROADMAP.md docs/RULES.md docs/SECURITY.md \
  docs/SETUP.md docs/SKILLS.md docs/TESTING.md docs/TODO.md \
  docs/TROUBLESHOOTING.md docs/USAGE.md docs/WORKFLOW.md \
  include/nlohmann/json.hpp \
  scripts/install_deps.sh \
  src/main.cpp \
  LICENSE Makefile README.md VERSION
do
  [ -e "$f" ] || touch "$f"
done

echo "Estructura base lista."
echo "Contenido por defecto: revisa README.md y docs/ para la documentación."