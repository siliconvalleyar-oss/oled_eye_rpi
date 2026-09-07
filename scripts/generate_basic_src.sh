#!/bin/bash

# Crear estructura de directorios y archivos vacíos

# Directorios (incluye docs/doxygen como directorio)
mkdir -p bin config docs examples \
         include/core include/drivers include/engine \
         include/libraries include/nlohmann include/oled include/security \
         obj scripts src/engine src/oled docs/doxygen

# Archivos vacíos
touch bin/App
touch config/config.cfg config/hardware.cfg
touch docs/ACTIVITY.md docs/API.md docs/ARCHITECTURE.md docs/ARQUITECTURA.md \
      docs/BLUETOOTH.md docs/BUILD.md docs/CHANGELOG.md docs/CONTRIBUTING.md \
      docs/DEPLOY.md docs/DESING.md docs/DIAGRAMS.md docs/HARDWARE.md \
      docs/INSTALL.md docs/LEARNINGS.md docs/MEMORY_MAP.md docs/PROMPT.md \
      docs/REPORT.md docs/ROADMAP.md docs/RULES.md docs/SECURITY.md \
      docs/SETUP.md docs/SKILLS.md docs/TESTING.md docs/TODO.md \
      docs/TROUBLESHOOTING.md docs/USAGE.md docs/WORKFLOW.md
touch include/HMC5883L.hpp
touch include/nlohmann/json.hpp
touch scripts/install_deps.sh
touch src/main.cpp
touch LICENSE Makefile README.md VERSION

echo "✅ Estructura creada correctamente."
