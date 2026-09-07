# EyePet — HOJA DE RUTA (ROADMAP)

## v1.0.0 (actual)

- [x] Motor del ojo (8 modos) sobre OLED SSD1306 128×64.
- [x] Parpadeo, brillo, cejas y suavizado de movimiento.
- [x] Configuración por archivos y CLI (`--version`, `--mode`).
- [x] Soporte I2C (por defecto) y SPI.
- [x] Scripts: instalación de dependencias y setup de Git.
- [x] Documentación completa.

## v1.1.0 — Refuerzos

- [ ] Control remoto por Bluetooth LE (ver `docs/BLUETOOTH.md`).
- [ ] Comandos en caliente (cambiar modo/velocidad sin reiniciar) vía FIFO/D-Bus.
- [ ] Opción `--demo` para recorrer todos los modos automáticamente.
- [ ] Test de dibujo por software (render a PGM) para validar sin hardware.

## v1.2.0 — Calidad

- [ ] Unit tests del config parser y del motor (ver `docs/TESTING.md`).
- [ ] CI de compilación cruzada (32 y 64 bits) en GitHub Actions.
- [ ] Medición de FPS reales en Pi Zero 2W y CM5.
- [ ] Perfil de consumo (apagado del display en reposo).

## v1.3.0 — Interacción

- [ ] Seguimiento por joystick analógico o sensor de distancia (HC-SR04/VL53L0X).
- [ ] Modo "alerta" (pupila dilatada, sacadas rápidas) ante estímulo externo.
- [ ] Soporte de estados emocionales persistentes reaccionando a entrada.

## Ideas a largo plazo

- Servidor web local para configuración y visualización del buffer.
- Port a pantallas mayores (SSD1309 / ST7920) reutilizando `IDisplay`.
- Página de documentación generada con Doxygen (`docs/doxygen/`).

_Las fechas se definirán según el ritmo de validación en hardware real._