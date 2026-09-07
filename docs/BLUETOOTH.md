# EyePet — Bluetooth (opcional)

Control remoto del ojo mediante Bluetooth LE.

## Estado

**Opcional / no implementado en esta versión.** EyePet v1.0.0 no incluye
control remoto por Bluetooth.

## Posible integración futura

- Usar `BlueZ` + D-Bus (o `libbluetooth-dev`) para recibir comandos:
  cambiar de modo, ajustar velocidad de parpadeo, encender/apagar el brillo.
- Ejecutando EyePet como servicio D-Bus (`dbus-run-session`) se puede
  consumir desde una app móvil o un script.

### Ideas de comandos

| Comando | Acción |
|---------|--------|
| `MODE <0..7>` | Cambia el modo del ojo en caliente. |
| `BLINK <min> <max>` | Ajusta el intervalo de parpadeo. |
| `SPEED <fps>` | Ajusta la cadencia de animación. |
| `POWER OFF` | Apaga el display (bajo consumo). |

## Requisitos (si se implementa)

- Paquete `bluez` y `libbluetooth-dev`.
- Habilitar el adaptador: `sudo hciconfig hci0 up`.
- El bucle principal debe quedar preparado para consultar un buffer de comandos
  sin bloquearse (actualmente `animationLoop()` ya es incremental).

Ver `docs/ROADMAP.md` para el estado de esta funcionalidad.