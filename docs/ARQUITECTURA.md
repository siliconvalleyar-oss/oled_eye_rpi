# EyePet — Arquitectura (versión en español)

Resumen técnico del diseño de EyePet. Ver también `ARCHITECTURE.md` (misma
información, complementaria) y `DIAGRAMS.md` para los diagramas.

## Capas del software

1. **Capa de aplicación** (`src/main.cpp`): crea el objeto `Eye::Eye_t` con
   memoria gestionada (`std::make_unique`) y ejecuta `run(argc, argv)`.
2. **Motor del ojo** (`src/engine/Eye_t.cpp`, `include/engine/Eye_t.hpp`):
   - Estados de animación (parpadeo, expresiones, movimientos).
   - Dibujo del ojo: esclera (círculo blanco), iris (anillos), pupila (círculo
     negro), párpados (recorte superior/inferior), cejas y brillo.
   - Modos de comportamiento: Normal, Tracking, Happy, Surprised, Angry,
     Sleepy, Sleep y Saccades.
3. **Interfaz de display** (`include/engine/IDisplay.hpp`) y adaptadores
   (`DisplayAdapters.cpp`): abstraen el hardware concreto.
4. **Drivers del display** (`src/oled/`):
   - `SSD1306` + `SSD1306_I2C`: modo I2C por `/dev/i2c-N`.
   - `SSD1306_SPI` + `SSD1306LinuxSPI`: modo SPI por `/dev/spidev` + sysfs GPIO.
   - `SSD1306_OLED_graphics`, `SSD1306_OLED_font`, `SSD1306_OLED_Print`:
     primitivas, fuentes e impresión de texto.

## Configuración

| Archivo | Contenido |
|---------|-----------|
| `config/config.cfg` | Modo inicial, velocidad, parpadeo, rango, geometría, brillo, cejas. |
| `config/hardware.cfg` | Protocolo (i2c/spi), dispositivo, dirección, pines. |
| `config/deploy.cfg` | Host/directorio para `make remote` y `make deploy`. |

## Bucle principal

```
bucle (fotogramas):
  dt = tiempo transcurrido
  updateBlink(dt)
  switch(mode): updateNormal | updateTracking | updateSaccades | updateSleep ...
  drawEye(apertura, dxPupila, dyPupila, radioPupila, ceja)
  if glint: addGlint(...)
  display.update()
  sleep(frame_delay_ms)
```

## Compatibilidad

- I2C: ioctl sobre `/dev/i2c-N` → funciona en Pi 5/CM5 (RP1) y anteriores.
- SPI: `/dev/spidev` + GPIO sysfs → requiere `dtparam=spi=on`.
- 32/64 bits: compilación nativa y cruzada (`make crossover32/64`).