# EyePet — TODO

Lista de tareas pendientes y en curso.

## Pendientes prioritarios

- [ ] **Validar en hardware real** (Raspberry Pi + OLED SSD1306): arranque,
      parpadeo, brillo y modos en I2C.
- [ ] **Probar el modo SPI** con un display real (4 hilos) y ajustar pines.
- [ ] **Configurar el repositorio Git** con `./setup_git.sh` (usuario, token,
      visibilidad) y hacer el primer push + tag `v1.0.0`.
- [ ] Comprobar FPS reales y ajustar valores por defecto de `config.cfg`.

## Mejoras de calidad

- [ ] Añadir `--demo` — recorrer todos los modos automáticamente.
- [ ] Tests del parser de configuración.
- [ ] Render del buffer a PGM para validación sin hardware.
- [ ] Comandos en caliente (FIFO/D-Bus) para cambiar modo en ejecución.

## Documentación

- [ ] Generar Doxygen en `docs/doxygen/`.
- [ ] Ampliar `docs/OLED.md` con ejemplos del motor del ojo.
- [ ] Añadir GIFs/capturas de la animación (cuando haya hardware).

## Infraestructura

- [ ] CI de compilación cruzada (32/64) en GitHub Actions.
- [ ] `scripts/test_eye.sh` para pruebas de humo automáticas.

## Nota

El historial de cambios completado se registra en `ACTIVITY.md` y
`CHANGELOG.md`.