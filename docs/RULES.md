# EyePet — Reglas del proyecto (RULES)

Normas internas de desarrollo de EyePet.

## Código

- C++17, compilable con `g++` y `-Wall -Wextra -O2`.
- Nombres descriptivos, `const`-correctness, sin `new`/`delete` explícitos en
  el código de aplicación (usar smart pointers / `std::make_unique`).
- Todo el código comentado (propósito, parámetros, retorno, decisiones).
- No se añaden librerías externas sin justificación; el proyecto debe ser
  autocontenido (solo libc/libm/pthread y el kernel Linux).

## Motor del ojo

- El bucle debe ser **no bloqueante** (cadencia configurable).
- Los modos dibujan** siempre sobre el buffer**; una sola `update()` por
  fotograma.
- Los valores geométricos/tiempos provienen de `config/config.cfg` (nunca
  hardcodeados salvo valores por defecto documentados).

## Hardware

- El acceso al display usa `/dev/i2c-N`/`/dev/spidev` (ioctl). No reintroducir
  `bcm2835` para I2C (incompatible con Pi 5/CM5).
- El destructor/apagado debe ser seguro si el display no se inicializó.

## Git / versionado

- **Todo push lleva su tag.**
- `VERSION` coincide con el último tag (tag `v1.0.0` → `VERSION` = `1.0.0`).
- Commits con conventional commits: `feat:`, `fix:`, `docs:`, `chore:`, etc.
- Un tag por rama/push; secuencia patch 0-9 luego minor; no retroceder
  versiones; no eliminar tags publicados.

## Documentación

- Ningún archivo de `docs/` debe quedar vacío (regla del prompt).
- README siempre al día (compilación, uso, opciones, git, modos).
- Actualizar `CHANGELOG.md` y `ACTIVITY.md` con cada entregable.

## Seguridad

- Nunca guardar contraseñas/tokens en el repositorio (usar `SSHPASS`,
  tokens por CLI, o credenciales globales de `gh`).
- Ver `docs/SECURITY.md`.