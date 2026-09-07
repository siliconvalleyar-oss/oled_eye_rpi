# EyePet — Mapa de memoria (MEMORY_MAP)

Cómo gestiona la memoria la aplicación.

## Almacenamiento dinámico

| Objeto | Gestión | Dónde |
|--------|---------|-------|
| `Eye::Eye_t` | `std::make_unique` | `src/main.cpp` |
| `IDisplay` (adaptador I2C/SPI) | `std::unique_ptr<IDisplay>` | `Eye_t::display_` |
| Driver `SSD1306` / `SSD1306_SPI` | `std::unique_ptr` | adaptadores |
| Frame buffer OLED (1024 B) | `malloc`/`free` | drivers |
| Cadenas y config | `std::string` / `EyeConfig_t` | `Eye_t::cfg_` |

## Detalle del frame buffer del SSD1306

El SSD1306 de 128×64 píxeles se representa como **8 páginas de 128 bytes**
(1 byte = 8 píxeles verticales):

- Tamaño total: `128 * (64/8) = 1024 bytes`.
- Índice de un píxel (x, y): `buffer[ (128 * (y/8)) + x ]`, bit `(y & 7)`.
- `drawPixel` modifica el bit correspondiente; `OLEDclearBuffer()` hace
  `memset(buffer, 0, 1024)`; `OLEDupdate()` transfiere el buffer completo al
  display en horizontal addressing mode.

## Miembros principales de `Eye_t`

| Miembro | Tamaño aprox. | Descripción |
|---------|---------------|-------------|
| `cfg_` (`EyeConfig_t`) | ~120 B | config leída de los *.cfg |
| `display_` (unique_ptr<IDisplay>) | 8 B | puntero al adaptador |
| `pupilDX_/pupilDY_/targetDX_/targetDY_` | 8×2 B | posición de la pupila |
| temporizadores (`blink*_`, `sleep*_`) | ~20 B | estado de la animación |
| `frameCounter_` | 4 B | contador de fotogramas |

## Notas

- No hay fugas esperadas: todo mecanismo de `unique_ptr` libera al salir de
  scope; los drivers liberan su buffer en el destructor.
- El bucle principal no acumula memoria (sin asignación por fotograma).
- `EyeConfig_t` se copia por valor (no hay grandes buffers en stack).