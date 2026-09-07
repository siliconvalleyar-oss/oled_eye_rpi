# OLED SSD1306 - Guía de uso en el Proyecto BASIC (Raspberry Pi)

Documentación completa del uso de la pantalla OLED SSD1306 (128x64 I2C) en el
proyecto BASIC para Raspberry Pi.

Esta guía se basa en la librería
[SSD1306_OLED_RPI](https://github.com/gavinlyonsrepo/SSD1306_OLED_RPI)
(de Gavin Lyons), adaptada e integrada en este proyecto dentro de
`include/oled/` y `src/oled/`.

> **Importante (diferencia clave):** a diferencia de la librería original, este
> proyecto **no usa `bcm2835`** para el acceso I2C. En su lugar vale de la capa
> `SSD1306_I2C` (`include/oled/SSD1306_I2C.hpp`), que accede al controlador I2C
> de Linux mediante `ioctl` sobre `/dev/i2c-N`. Esto permite funcionar en todas
> las Raspberry Pi, incluidas la **Pi 5 y Compute Module 5 (chip RP1)**, donde
> `bcm2835` no es compatible con el controlador I2C.

---

## Tabla de contenidos

- [1. Hardware y conexiones](#1-hardware-y-conexiones)
- [2. Estructura de archivos en el proyecto](#2-estructura-de-archivos-en-el-proyecto)
- [3. Flujo básico de uso](#3-flujo-básico-de-uso)
- [4. Inicialización del display](#4-inicialización-del-display)
- [5. Buffer y actualización de la pantalla](#5-buffer-y-actualización-de-la-pantalla)
- [6. Texto](#6-texto)
- [7. Fuentes](#7-fuentes)
- [8. Gráficos](#8-gráficos)
- [9. Bitmaps](#9-bitmaps)
- [10. Funciones de control](#10-funciones-de-control)
- [11. Colores y códigos de retorno](#11-colores-y-códigos-de-retorno)
- [12. Capa I2C Linux (`SSD1306_I2C`)](#12-capa-i2c-linux-ssd1306_i2c)
- [13. Ejemplo completo](#13-ejemplo-completo)
- [14. Troubleshooting](#14-troubleshooting)

---

## 1. Hardware y conexiones

| Pin RPi (I2C) | Señal | Display OLED |
|----------------|-------|--------------|
| GPIO 2 (pin 3) | SDA | SDA |
| GPIO 3 (pin 5) | SCL | SCL |
| 3.3V (pin 1) | VCC | VCC |
| GND (pin 6) | GND | GND |

Detalles:

- **Dirección I2C:** `0x3C` por defecto (alternativa `0x3D`, definida por los
  puentes `SA0/SA1` del módulo).
- **Bus I2C:** `/dev/i2c-1` en la mayoría de las Pi (incluida la CM5 con display
  en el bus 1). Puede haber otros buses (`i2c-13`, `i2c-14`) en el chip RP1 de la
  Pi 5/CM5; en este proyecto el bus por defecto es `/dev/i2c-1`.
- **Tamaños soportados:** 128x64 (estándar), 128x32; el código de este proyecto
  usa 128x64 (`OLED_WIDTH = 128`, `OLED_HEIGHT = 64`).
- **Permisos:** la app necesita `sudo` (o pertenecer al grupo `i2c`) para abrir
  `/dev/i2c-1`.

---

## 2. Estructura de archivos en el proyecto

| Archivo | Descripción |
|---------|-------------|
| `include/oled/SSD1306_OLED.hpp` | Clase `SSD1306`: buffer, comandos, init, buffer/screen, funciones de control |
| `include/oled/SSD1306_OLED_graphics.hpp` | Clase `SSD1306_graphics`: primitivas de texto y gráficos |
| `include/oled/SSD1306_OLED_font.hpp` | Enum de tipos de fuente y punteros a las tablas de glyphs |
| `include/oled/SSD1306_OLED_Print.hpp` | Clase `Print`: `print()`/`println()` polimórficos |
| `include/oled/SSD1306_I2C.hpp` | Capa de acceso I2C por ioctl de Linux (`/dev/i2c-N`) |
| `src/oled/SSD1306_OLED.cpp` | Implementación de `SSD1306` |
| `src/oled/SSD1306_OLED_graphics.cpp` | Implementación de primitivas gráficas |
| `src/oled/SSD1306_OLED_Print.cpp` | Implementación de `print()`/`println()` |
| `src/oled/SSD1306_OLED_font.cpp` | Tablas de fuentes (glyphs) |
| `src/oled/SSD1306_I2C.cpp` | Implementación de la capa I2C Linux |
| `src/engine/Device_t.cpp` / `include/core/Device_t.hpp` | Clase de alto nivel que integra el OLED |

---

## 3. Flujo básico de uso

En este proyecto, la pantalla se encapsula en la clase `Device::Device_t`. El
flujo general (patrón recomendado para usar el OLED directamente):

```cpp
#include "SSD1306_OLED.hpp"

// 1. Crear la pantalla con sus dimensiones (128x64).
SSD1306 oled(128, 64);

// 2. Inicializar el display (abre /dev/i2c-1, configura y enciende).
oled.OLEDbegin();

// 3. Bucle de dibujo:
oled.OLEDclearBuffer();   // limpiar el buffer interno
// ...dibujar texto / gráficos en el buffer...
oled.OLEDupdate();        // volcar el buffer al display físico

// 4. Al terminar:
oled.OLEDPowerDown();     // apagar el display
```

> El `OLEDPowerDown()` apaga el display; el destructor de `SSD1306` además
> libera el frame buffer y cierra el adaptador I2C (`/dev/i2c-1`) si quedó
> abierto.

---

## 4. Inicialización del display

### Constructor

```cpp
SSD1306(int16_t oledwidth, int16_t oledheight);
```

Asigna el frame buffer interno. Ej.: `SSD1306 oled(128, 64);`.

### `OLEDbegin` (inicializa el display)

```cpp
void OLEDbegin(uint16_t I2C_speed = 0, uint8_t I2c_address = SSD1306_ADDR);
```

- Parámetros: velocidad I2C (opcional) y dirección `0x3C` por defecto.
- Abre el adaptador `/dev/i2c-1` y configura la dirección del esclavo.
- Ejecuta la secuencia de encendido y registro del SSD1306 (termina con
  `DISPLAY_ON`).
- La velocidad real de la línea I2C la controla el driver del kernel
  (`/boot/config.txt` → `dtparam=i2c_arm_baudrate`), no el parámetro.

```cpp
oled.OLEDbegin();                        // defaults 0x3C, /dev/i2c-1
oled.OLEDbegin(0, 0x3C);                 // explícito
```

### `OLEDReset` y `OLEDinit`

```cpp
void OLEDReset(void);    // reinicia el controlador
void OLEDinit(void);     // secuencia de registro (llamada por OLEDbegin)
```

### Apagado

```cpp
void OLEDPowerDown(void);   // apaga el display (0xAE)
```

---

## 5. Buffer y actualización de la pantalla

El SSD1306 no mantiene el contenido de forma persistente en el host: se dibuja
en un **buffer interno** (memoria del host) y luego se envía al display.

| Función | Descripción |
|---------|-------------|
| `void OLEDclearBuffer(void)` | Limpia el buffer interno (no toca la pantalla) |
| `void OLEDupdate(void)` | Vuelca el buffer interno al display físico |
| `void OLEDBuffer(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t* data)` | Escribe una región del buffer a la pantalla |
| `void OLEDFillScreen(uint8_t pixel, uint8_t mircodelay)` | Rellena y envía toda la pantalla con un patrón |
| `void OLEDFillPage(uint8_t page_num, uint8_t pixels, uint8_t delay)` | Rellena una página (8 filas) |

El **frame buffer** es un miembro público:

```cpp
uint8_t* buffer;   // tamaño = width * (height/8) = 128 * 8 = 1024 bytes (128x64)
```

Uso típico del ciclo de dibujo:

```cpp
oled.OLEDclearBuffer();
oled.setTextColor(WHITE);
oled.setCursor(0, 0);
oled.print("Hello");
oled.OLEDupdate();
```

Rellenar toda la pantalla de blanco (patrón de validación visual):

```cpp
memset(oled.buffer, 0xFF, 128 * (64 / 8));
oled.OLEDupdate();
```

`OLEDFillScreen` como splash o para limpiar la pantalla física:

```cpp
oled.OLEDFillScreen(0xF0, 0);   // patrón de barras
oled.OLEDFillScreen(0x00, 0);   // pantalla en negro
```

---

## 6. Texto

### `print` / `println` (clase `Print`)

Imprimen texto y números formateados en la posición del cursor.

```cpp
void setCursor(int16_t x, int16_t y);   // fija la posición del cursor

oled.setCursor(0, 0);
oled.print("Texto");                     // const char[]
oled.print(123);                         // int
oled.println("Línea");                   // con salto de línea
oled.print(19.6657, 3);                  // double con 3 decimales -> "19.666"
oled.print(47, DEC);                     // "47"
oled.print(47, HEX);                     // "2F"
oled.print(47, BIN);                     // "101111"
oled.print(47, OCT);                     // "57"
```

### `setTextColor`

```cpp
void setTextColor(uint8_t c);        // solo color de frente
void setTextColor(uint8_t c, uint8_t bg);   // frente + fondo (para invertir)
```

```cpp
oled.setTextColor(WHITE);            // texto blanco
oled.setTextColor(BLACK, WHITE);     // texto negro sobre fondo blanco (invertido)
```

### `setTextSize`

```cpp
void setTextSize(uint8_t s);   // escala (1, 2, 3, ...). Aplica a fuentes escalables.
```

### `setTextWrap`

```cpp
void setTextWrap(bool w);   // true = ajusta el texto al borde derecho
```

### `setFontNum`

```cpp
void setFontNum(OLEDFontType_e FontNumber);   // selecciona la fuente activa
```

### `drawChar` / `drawText` (dibujo directo)

```cpp
// Un carácter en una posición fija (fuentes escalables, con size).
virtual void drawChar(int16_t x, int16_t y, unsigned char c,
                      uint8_t color, uint8_t bg, uint8_t size);

// Cadena de texto (fuentes numéricas fijas, sin size) y con size.
void drawCharNumFont(uint8_t x, uint8_t y, uint8_t c, uint8_t color, uint8_t bg);
void drawTextNumFont(uint8_t x, uint8_t y, char *pText, uint8_t color, uint8_t bg);
void drawText(uint8_t x, uint8_t y, char *pText, uint8_t color, uint8_t bg, uint8_t size);
```

```cpp
oled.drawChar(10, 10, 'H', WHITE, BLACK, 3);    // "H" tamaño 3
oled.drawChar(0, 0, '8', BLACK, WHITE);          // '8' invertido (fuente numérica)

char buf[] = "1234";
oled.drawText(0, 0, buf, WHITE, BLACK, 2);       // texto escalado
oled.drawText(0, 30, buf, WHITE, BLACK);         // sin size (fuente numérica fija)
```

> `drawChar`/`drawText` devuelven un `OLED_Return_Codes_e` (ver sección 11):
> `OLED_Success` = 0 si todo va bien.

---

## 7. Fuentes

`typedef enum { ... } OLEDFontType_e;` en `SSD1306_OLED_font.hpp`.

Este proyecto incluye **8 fuentes**:

| # | Enum | Tamaño | ASCII | Notas |
|---|------|--------|-------|-------|
| 1 | `OLEDFontType_Default` | 5x8 | 0x00–0xFF (completo) | Escalable, por defecto |
| 2 | `OLEDFontType_Thick` | 7x8 | 0x20–0x5A | Sin minúsculas |
| 3 | `OLEDFontType_SevenSeg` | 4x8 | 0x20–0x7A | Estilo 7 segmentos |
| 4 | `OLEDFontType_Wide` | 8x8 | 0x20–0x5A | Sin minúsculas |
| 5 | `OLEDFontType_Tiny` | 3x8 | 0x20–0x7E | Muy compacta |
| 6 | `OLEDFontType_Homespun` | 7x8 | 0x20–0x7E | |
| 7 | `OLEDFontType_Bignum` | 16x32 | 0x2D–0x3A (números, `- . / :`) | Fija (no escalable) |
| 8 | `OLEDFontType_Mednum` | 16x16 | 0x2D–0x3A (números, `- . / :`) | Fija (no escalable) |

Reglas de uso:

- Fuentes **1–6** son escalables mediante `setTextSize` y `drawChar(..., size)`.
- Fuentes **7–8** (numéricas grandes) **no se escalan**; usar `print`/`drawChar`
  sin tamaño. `print` con fuente numérica muestra dígitos, `:`, `.`, `/`, `-`.

```cpp
oled.setFontNum(OLEDFontType_Default);
oled.setTextSize(2);
oled.setCursor(0, 0);
oled.print("Hola");

oled.setFontNum(OLEDFontType_Bignum);
oled.setCursor(0, 0);
oled.print("12:30");
```

> Las fuentes 7–11 (ArialRound, ArialBold, Mia, Dedica) y 12 no están incluidas
> en esta versión del proyecto (solo las 8 de arriba).

---

## 8. Gráficos

Todas las primitivas dibujan sobre el **buffer** y requieren `OLEDupdate()` para
verse en pantalla. Los parámetros de color se pasan como `uint8_t` (`BLACK`/`WHITE`).

### Punto y líneas

```cpp
virtual void drawPixel(int16_t x, int16_t y, uint8_t color);
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
void drawFastVLine(int16_t x, int16_t y, int16_t h, uint8_t color);   // línea vertical
void drawFastHLine(int16_t x, int16_t y, int16_t w, uint8_t color);   // línea horizontal
```

```cpp
oled.drawPixel(5, 5, WHITE);
oled.drawLine(0, 0, 127, 63, WHITE);
oled.drawFastVLine(64, 0, 63, WHITE);
oled.drawFastHLine(0, 32, 128, WHITE);
```

### Rectángulos

```cpp
void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);  // contorno
void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);  // relleno
void drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint8_t color);
void fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint8_t color);
void fillScreen(uint8_t color);   // llena todo el buffer
```

```cpp
oled.drawRect(10, 10, 50, 30, WHITE);
oled.fillRect(20, 20, 30, 10, WHITE);
oled.fillRoundRect(0, 40, 40, 20, 10, WHITE);
```

### Círculos

```cpp
void drawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color);   // contorno
void fillCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color);   // relleno
void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint8_t color);
void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint8_t color);
```

```cpp
oled.drawCircle(64, 32, 20, WHITE);
oled.fillCircle(40, 20, 10, WHITE);
```

### Triángulos

```cpp
void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                  int16_t x2, int16_t y2, uint8_t color);
void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                  int16_t x2, int16_t y2, uint8_t color);
```

```cpp
oled.fillTriangle(80, 25, 90, 5, 100, 25, WHITE);
```

### Ancho / alto / rotación

```cpp
int16_t height(void) const;   // alto (según rotación)
int16_t width(void) const;    // ancho (según rotación)
void setRotation(uint8_t r);  // 0, 1, 2, 3 (0°, 90°, 180°, 270°)
uint8_t getRotation(void) const;
```

```cpp
oled.setRotation(1);          // 90°
oled.setRotation(0);          // volver a 0°
```

---

## 9. Bitmaps

```cpp
void OLEDBitmap(int16_t x, int16_t y, int16_t w, int16_t h,
                const uint8_t* data, bool invert);
```

Dibuja un bitmap en el buffer. `invert = true` invierte los colores.

```cpp
oled.OLEDBitmap(0, 0, 64, 64, logo_array, false);
oled.OLEDBitmap(70, 10, 16, 8, icon, true);   // invertido
oled.OLEDupdate();
```

> Datos de bitmap monocromos (1 bit por píxel) en orientación **horizontal**
> (ancho múltiplo de 8). Herramienta para convertir imágenes:
> [image2cpp](https://javl.github.io/image2cpp/).

---

## 10. Funciones de control

### Encendido / apagado del display

```cpp
void OLEDEnable(uint8_t on);   // 1 = encender, 0 = apagar
```

```cpp
oled.OLEDEnable(0);   // apagar
oled.OLEDEnable(1);   // encender
```

### Contraste

```cpp
void OLEDContrast(uint8_t OLEDcontrast);   // 0x00 a 0xFF (por defecto 0x80 o 0xCF)
```

```cpp
oled.OLEDContrast(0x00);   // contraste mínimo
oled.OLEDContrast(0xFF);   // contraste máximo
```

### Invertir colores

```cpp
void OLEDInvert(bool on);   // true = invertir, false = normal
```

```cpp
oled.OLEDInvert(1);   // pantalla invertida
oled.OLEDInvert(0);   // volver a normal
```

### Scroll

```cpp
void OLED_StartScrollRight(uint8_t start, uint8_t stop);     // scroll horizontal derecha
void OLED_StartScrollLeft(uint8_t start, uint8_t stop);      // scroll horizontal izquierda
void OLED_StartScrollDiagRight(uint8_t start, uint8_t stop); // scroll diagonal derecha
void OLED_StartScrollDiagLeft(uint8_t start, uint8_t stop);  // scroll diagonal izquierda
void OLED_StopScroll(void);                                  // detener scroll
```

`start`/`stop` son números de página (0–7 para 128x64).

```cpp
oled.OLED_StartScrollRight(0, 0x0F);
delay(3000);
oled.OLED_StopScroll();
```

### Rotación

Ver sección 8 (`setRotation`).

---

## 11. Colores y códigos de retorno

### Colores

```cpp
#define BLACK   0
#define WHITE   1
#define INVERSE 2
```

### Códigos de retorno (`OLED_Return_Codes_e`)

Usados por `drawChar`, `drawText` y `OLEDBitmap`:

```cpp
OLED_Success                = 0    // éxito
OLED_WrongFont              = 2    // fuente incorrecta para el método
OLED_CharScreenBounds       = 3    // carácter fuera de los límites de pantalla
OLED_CharFontASCIIRange     = 4    // carácter fuera del rango ASCII de la fuente
OLED_CharArrayNullptr       = 5    // puntero de array de caracteres nulo
OLED_BitmapNullptr          = 7    // puntero de datos de bitmap nulo
OLED_BitmapScreenBounds     = 8    // punto inicial del bitmap fuera de pantalla
OLED_BitmapLargerThanScreen = 9    // bitmap más grande que la pantalla
OLED_BitmapVerticalSize     = 10   // alto del bitmap vertical no divisible por 8
OLED_BitmapHorizontalSize   = 11   // ancho del bitmap horizontal no divisible por 8
OLED_BitmapSize             = 12   // tamaño del bitmap incorrecto
OLED_CustomCharLen          = 13   // array de carácter personalizado debe ser de 5 bytes
```

---

## 12. Capa I2C Linux (`SSD1306_I2C`)

A diferencia de la librería original (que usaba `bcm2835`), este proyecto accede
al I2C mediante `ioctl` sobre `/dev/i2c-N`. API de `include/oled/SSD1306_I2C.hpp`:

```cpp
namespace SSD1306LinuxI2C {
    const char* I2C_DEV_PATH = "/dev/i2c-1";   // adaptador por defecto

    int  i2c_open(const char* device, uint8_t address);   // abrir + fijar dirección (dev >= 0, o -1 si error)
    bool i2c_set_address(int fd, uint8_t address);        // cambiar la dirección del esclavo
    bool i2c_write_byte(int fd, uint8_t value, uint8_t cmd); // escribe 2 bytes: [cmd, value]; true = ACK
    void i2c_close(int fd);                               // cerrar adaptador
}
```

- `cmd` es el byte de control: `0x00` = comando, `0x40` = dato (continuación).
- `i2c_write_byte` devuelve `true` (ACK) si el `write()` de 2 bytes completa, lo
  que significa que el display en la dirección dada confirma la escritura.

Esta capa se usa internamente en `SSD1306::OLED_I2C_ON/OFF` e `SSD1306::I2C_Write_Byte`
(en `src/oled/SSD1306_OLED.cpp`). No requiere `sudo` por la librería bcm2835,
sino acceso lectura/escritura a `/dev/i2c-1`.

---

## 13. Ejemplo completo

Demo que muestra texto, los tipos de fuente, gráficos y funciones de control:

```cpp
#include <cstring>
#include <unistd.h>
#include "SSD1306_OLED.hpp"

int main() {
    SSD1306 oled(128, 64);

    // Inicializar (abre /dev/i2c-1, dirección 0x3C).
    oled.OLEDbegin();
    oled.OLEDFillScreen(0x00, 0);   // pantalla en negro

    // ---- Texto con la fuente por defecto ----
    oled.OLEDclearBuffer();
    oled.setTextColor(WHITE);
    oled.setTextSize(1);
    oled.setCursor(0, 0);
    oled.print("BASIC RPi");
    oled.setCursor(0, 10);
    oled.print("v1.2.0");
    oled.drawLine(0, 24, 128, 24, WHITE);
    oled.drawRect(0, 28, 128, 20, WHITE);
    oled.fillCircle(110, 55, 7, WHITE);
    oled.OLEDupdate();
    usleep(3000 * 1000);

    // ---- Fecha/hora con fuente numérica grande ----
    oled.setFontNum(OLEDFontType_Bignum);
    oled.OLEDclearBuffer();
    oled.setCursor(0, 0);
    oled.print("12:30");
    oled.OLEDupdate();
    usleep(3000 * 1000);

    // ---- Invertir y cambiar contraste ----
    oled.OLEDInvert(1);
    usleep(1000 * 1000);
    oled.OLEDInvert(0);
    oled.OLEDContrast(0xCF);

    // ---- Apagar el display ----
    oled.OLEDPowerDown();
    return 0;
}
```

---

## 14. Troubleshooting

| Síntoma | Causa / solución |
|---------|------------------|
| `Error: No se pudo abrir /dev/i2c-1: Permission denied` | Ejecutar con `sudo` o agregar el usuario al grupo `i2c` |
| `Error: Cannot start I2C` | No se pudo abrir `/dev/i2c-1` o dirección incorrecta |
| `Error I2C: No hay adaptador I2C abierto` | `OLEDbegin` falló al abrir el bus; verificar `sudo` y la dirección |
| `Error I2C: Cannot Write byte` (NACK) | Display no responde en la dirección/bus configurado. Verificar cableado, dirección (`0x3C`/`0x3D`) y `i2cdetect -y 1` |
| Display no muestra nada | Falta llamar a `OLEDupdate()` después de dibujar, o `OLEDbegin()` no se ejecutó |
| Solo se ve en la CM5/Pi5 el error de NACK | **No usar bcm2835**; este proyecto ya usa `/dev/i2c-N`. Verificar que el display esté en `0x3C` del bus 1 |
| Pantalla con barras extrañas | Usar `OLEDFillScreen(0x00, 0)` para limpiar la pantalla física |
| Fuente no muestra ciertos caracteres | Verificar el rango ASCII de la fuente seleccionada (las numéricas/fijas no tienen minúsculas) |
| Scroll no detiene | Llamar `OLED_StopScroll()` |

Herramienta de diagnóstico en la Pi:

```sh
sudo i2cdetect -y 1        # detectar dispositivos en el bus 1 (el display aparece en 0x3c)
sudo i2cset -y 1 0x3c 0x00 0xAF && echo OK   # encender manualmente
```

---
