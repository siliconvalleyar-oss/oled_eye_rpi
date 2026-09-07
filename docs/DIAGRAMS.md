# EyePet — Diagramas (DIAGRAMS)

Diagramas de la arquitectura y del flujo de animación de EyePet.

## Diagrama de capas

```
+---------------------------------------------------------------+
| APP   | src/main.cpp                                           |
|       |   auto eye = std::make_unique<Eye::Eye_t>();           |
|       |   return eye->run(argc, argv);                         |
+---------------------------------------------------------------+
| ENGINE| Eye::Eye_t   (src/engine/Eye_t.cpp)                    |
|       |   parseArgs → loadConfig → initHardware → animationLoop|
+---------------------------------------------------------------+
| IFACE | Eye::IDisplay (include/engine/IDisplay.hpp)            |
+-------------------------------+-------------------------------+
| ADAP | DisplayAdapterI2C      | DisplayAdapterSPI              |
+-------------------------------+-------------------------------+
| DRV  | SSD1306                | SSD1306_SPI_driver             |
|      |   └─ SSD1306_I2C       |   └─ SSD1306LinuxSPI           |
|      |      ├ ioctl(/dev/i2c) |      ├ ioctl(/dev/spidev)      |
|      |      └ I2C_Write_Byte  |      └ GPIO sysfs (DC/RST)     |
+-------------------------------+-------------------------------+
```

## Flujo del bucle principal

```
            ┌─────────────────────────────┐
            │       animationLoop()       │
            └──────────────┬──────────────┘
                           ▼
                       dt = now - last
                           │
               ┌───────────┼───────────┐
               ▼           ▼           ▼
          updateBlink  updateModo   contador dt
               │    (Normal/Tracking/
               │     Saccades/Sleep/...)
               ▼
   ┌────────────────────────────┐
   │        drawEye(...)        │   esclera→iris→pupila→párpados→ceja
   └──────────────┬─────────────┘
                  ▼
         if glint → addGlint(...)
                  ▼
            display->update()        (buffer → OLED)
                  ▼
        sleep(frame_delay_ms)         (cadencia no bloqueante)
```

## Estado del ojo (variables internas de `Eye_t`)

| Variable | Uso |
|----------|-----|
| `pupilDX_`, `pupilDY_` | Posición actual de la pupila. |
| `targetDX_`, `targetDY_` | Objetivo del movimiento suave. |
| `blinkPhase_` | Apertura del párpado (1 abierto → 0 cerrado). |
| `blinkTimerMs_` | Cuenta atrás hasta el siguiente parpadeo. |
| `browOffset_` | Desplazamiento vertical de la ceja. |
| `pupilRCurrent_` | Radio de pupila (dilatación). |
| `frameCounter_` | Nº de fotograma (depuración). |

## Mapa de archivos

```
include/engine/ Eye_t.hpp EyeConfig.hpp IDisplay.hpp DisplayAdapters.hpp
src/engine/    Eye_t.cpp  EyeConfig.cpp DisplayAdapters.cpp
include/oled/  SSD1306_OLED.hpp {I2C,graphics,font,Print}.hpp SSD1306_SPI{,_driver}.hpp SSD1306_SPI.hpp
src/oled/      SSD1306_OLED.cpp SSD1306_I2C.cpp SSD1306_SPI{_driver,}.cpp
               SSD1306_OLED_{graphics,font,Print}.cpp
include/core/  (vacío)          include/drivers/ (vacío)
src/           main.cpp
```