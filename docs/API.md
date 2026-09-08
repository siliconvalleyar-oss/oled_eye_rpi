# EyePet — API

Documentación de la interfaz pública de la aplicación EyePet.

## Namespace `Eye`

### Clase `Eye::Eye_t`

Encapsula el ciclo de vida del display y el bucle de animación del ojo.

| Método | Descripción |
|--------|-------------|
| `Eye_t()` | Constructor: crea el estado interno del ojo y la semilla aleatoria. |
| `~Eye_t()` | Destructor: apaga el display de forma segura. |
| `int run(int argc, char* argv[])` | Punto de entrada: parsea argumentos, carga configuración e inicia el bucle de animación. |
| `std::string version() const` | Devuelve la versión compilada (macro `VERSION`). |

No se permite copia ni asignación (`Eye_t(const Eye_t&) = delete`).

### Enumeración `Eye::EyeMode_e`

| Valor | Modo |
|-------|------|
| `Normal(0)` | Exploración suave hacia el frente. |
| `Tracking(1)` | Seguimiento de un objetivo circular. |
| `Happy(2)` | Alegría (cejas arqueadas). |
| `Surprised(3)` | Sorpresa (cejas elevadas, pupila pequeña). |
| `Angry(4)` | Enfado (cejas fruncidas). |
| `Sleepy(5)` | Sueño (párpado medio cerrado). |
| `Sleep(6)` | Dormido (ojos cerrados con temblores). |
| `Saccades(7)` | Movimientos sacádicos rápidos. |

### Enumeración `Eye::EyeStyle_e`

| Valor | Estilo |
|-------|--------|
| `Classic(0)` | Ojo redondeado clásico, pupila circular, un brillo. |
| `Anime(1)` | Elipse ancha a pantalla completa, pupila amplia, 2 brillos. |
| `Feline(2)` | Elipse almendra, pupila vertical alargada. |
| `Robot(3)` | Visor rectangular, pupila cuadrada con retícula. |

Se seleccionan con el campo `style` de `EyeConfig_t`, con la clave `style` de
`config/config.cfg` o con `--style <0..3>`.

### Estructura `Eye::EyeConfig_t`

Parámetros de configuración del ojo. Ver `config/config.cfg` y
`config/hardware.cfg` para la lista completa.

### Funciones de configuración

| Función | Descripción |
|---------|-------------|
| `void loadGeneralConfig(EyeConfig_t&, const std::string&)` | Carga `config/config.cfg`. |
| `void loadHardwareConfig(EyeConfig_t&, const std::string&)` | Carga `config/hardware.cfg`. |

### Interfaz `Eye::IDisplay`

Abstracción del display para permitir I2C o SPI:

| Método | Descripción |
|--------|-------------|
| `bool begin()` | Inicializa el display y el bus. |
| `void clearBuffer()` | Limpia el buffer de pantalla. |
| `void update()` | Vuelca el buffer al hardware. |
| `void powerDown()` | Apaga el display. |
| `void setContrast(uint8_t)` | Ajusta contraste. |
| Primitivas (`drawPixel`, `drawLine`, `drawFastVLine`, `drawFastHLine`, `drawRect`, `fillRect`, `drawCircle`, `fillCircle`) | Dibujo sobre el buffer. |

Implementaciones: `Eye::DisplayAdapterI2C` (driver `SSD1306` por `/dev/i2c-N`)
y `Eye::DisplayAdapterSPI` (driver `SSD1306_SPI` por `/dev/spidev`).

## Capa OLED (`include/oled/`)

Driver del SSD1306 (heredado del proyecto BASIC) con API estilo Adrian/Adafruit:

- `SSD1306` clase principal (I2C). `OLEDbegin(...)`, `OLEDclearBuffer()`,
  `OLEDupdate()`, `OLEDPowerDown()`, gráficos (`drawPixel`, `drawLine`,
  `fillCircle`, ...), texto (`setCursor`, `print`, `setFontNum`, ...).
- `SSD1306_I2C`: acceso low-level por `/dev/i2c-N` (`i2c_open`, `i2c_write_byte`, ...).
- `SSD1306_SPI` y `SSD1306_SPI_driver`: driver SPI con la misma API gráfica.
- Fuentes: `SSD1306_OLED_font` (8 fuentes), `SSD1306_OLED_Print` (Print de Arduino).

## Línea de comandos

```
bin/App [--version] [--mode N] [--style N] [--config ARCHIVO] [--hw-config ARCHIVO] [--help]
```

Ver `docs/USAGE.md` para detalles completos.