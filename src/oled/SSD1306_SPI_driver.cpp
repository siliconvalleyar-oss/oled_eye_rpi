/**
 * @file SSD1306_SPI_driver.cpp
 * @brief Implementación del driver SSD1306 en modo SPI.
 *
 * @details Transmite el mismo conjunto de comandos y datos del SSD1306 por el
 * bus SPI de Linux (/dev/spidev), controlando el pin DC (comando vs. dato) y el
 * pin RESET a través de GPIO de sysfs. Mantiene un buffer de pantalla en memoria
 * para actualizaciones eficientes (igual que el driver I2C).
 *
 * @author Proyecto EyePet (Raspberry Pi)
 * @version 1.0.0
 */

#include "SSD1306_SPI_driver.hpp"

#include <stdlib.h>
#include <unistd.h>

// Delay en milisegundos usando usleep de POSIX.
#define OLED_DELAY_MS(ms) usleep((ms) * 1000)

SSD1306_SPI::SSD1306_SPI(int16_t oledwidth, int16_t oledheight)
    : SSD1306_graphics(oledwidth, oledheight) {
    _OLED_HEIGHT = oledheight;
    _OLED_WIDTH = oledwidth;
    _OLED_PAGE_NUM = (_OLED_HEIGHT / 8);
    bufferWidth = _OLED_WIDTH;
    bufferHeight = _OLED_HEIGHT;
    buffer = (uint8_t*)malloc((size_t)(oledwidth * (oledheight / 8)));
    if (buffer == nullptr) {
        printf("Error: No se pudo asignar el buffer OLED\n");
    }
}

SSD1306_SPI::~SSD1306_SPI() {
    if (_SPI_fd >= 0) {
        SSD1306LinuxSPI::spi_close(_SPI_fd);
        _SPI_fd = -1;
    }
    if (_gpio_dc_owned) {
        SSD1306LinuxSPI::gpio_unexport(_SPI_DC_pin);
    }
    if (_gpio_rst_owned) {
        SSD1306LinuxSPI::gpio_unexport(_SPI_RST_pin);
    }
    free(buffer);
    buffer = nullptr;
}

bool SSD1306_SPI::OLEDbegin(const char* spiDevice, uint32_t speed,
                            unsigned dcPin, unsigned rstPin) {
    _SPI_DC_pin = dcPin;
    _SPI_RST_pin = rstPin;

    // Configura el pin DC como salida.
    if (SSD1306LinuxSPI::gpio_export(dcPin)) {
        _gpio_dc_owned = true;
        if (SSD1306LinuxSPI::gpio_direction(dcPin, "out")) {
            // ok
        } else {
            fprintf(stderr, "SPI: no se pudo configurar DC/%u como salida\n", dcPin);
            return false;
        }
    } else {
        return false;
    }

    // Configura el pin RESET como salida.
    if (rstPin > 0) {
        if (SSD1306LinuxSPI::gpio_export(rstPin)) {
            _gpio_rst_owned = true;
            if (!SSD1306LinuxSPI::gpio_direction(rstPin, "out")) {
                fprintf(stderr, "SPI: no se pudo configurar RST/%u como salida\n", rstPin);
                return false;
            }
        }
    }

    // Abre el bus SPI.
    _SPI_fd = SSD1306LinuxSPI::spi_open(spiDevice, speed);
    if (_SPI_fd < 0) {
        fprintf(stderr, "SPI: no se pudo abrir el bus %s (activar spidev en /boot/config.txt)\n",
                spiDevice);
        return false;
    }

    SPI_Reset();
    OLEDinit();
    return true;
}

void SSD1306_SPI::SPI_Reset() {
    if (_SPI_RST_pin == 0) return;
    // Pulso de reset: bajo -> alto. Normalmente el reset es activo en bajo.
    SSD1306LinuxSPI::gpio_write(_SPI_RST_pin, 0);
    OLED_DELAY_MS(10);
    SSD1306LinuxSPI::gpio_write(_SPI_RST_pin, 1);
    OLED_DELAY_MS(100);
}

void SSD1306_SPI::OLEDinit() {
    OLED_DELAY_MS(100);

    SPI_Write_Byte(SSD1306_DISPLAY_OFF, SSPI_COMMAND);
    SPI_Write_Byte(SSD1306_SET_DISPLAY_CLOCK_DIV_RATIO, SSPI_COMMAND);
    SPI_Write_Byte(0x80, SSPI_COMMAND);
    SPI_Write_Byte(SSD1306_SET_MULTIPLEX_RATIO, SSPI_COMMAND);
    SPI_Write_Byte(_OLED_HEIGHT - 1, SSPI_COMMAND);
    SPI_Write_Byte(SSD1306_SET_DISPLAY_OFFSET, SSPI_COMMAND);
    SPI_Write_Byte(0x00, SSPI_COMMAND);
    SPI_Write_Byte(SSD1306_SET_START_LINE | 0x00, SSPI_COMMAND);
    SPI_Write_Byte(SSD1306_CHARGE_PUMP, SSPI_COMMAND);
    SPI_Write_Byte(0x14, SSPI_COMMAND);
    SPI_Write_Byte(SSD1306_MEMORY_ADDR_MODE, SSPI_COMMAND);
    SPI_Write_Byte(0x00, SSPI_COMMAND); // Horizontal addressing
    SPI_Write_Byte(SSD1306_SET_SEGMENT_REMAP | 0x01, SSPI_COMMAND);
    SPI_Write_Byte(SSD1306_COM_SCAN_DIR_DEC, SSPI_COMMAND);

    switch (_OLED_HEIGHT) {
        case 64:
            SPI_Write_Byte(SSD1306_SET_COM_PINS, SSPI_COMMAND);
            SPI_Write_Byte(0x12, SSPI_COMMAND);
            SPI_Write_Byte(SSD1306_SET_CONTRAST_CONTROL, SSPI_COMMAND);
            SPI_Write_Byte(0xCF, SSPI_COMMAND);
            break;
        case 32:
            SPI_Write_Byte(SSD1306_SET_COM_PINS, SSPI_COMMAND);
            SPI_Write_Byte(0x02, SSPI_COMMAND);
            SPI_Write_Byte(SSD1306_SET_CONTRAST_CONTROL, SSPI_COMMAND);
            SPI_Write_Byte(0x8F, SSPI_COMMAND);
            break;
        case 16:
            SPI_Write_Byte(SSD1306_SET_COM_PINS, SSPI_COMMAND);
            SPI_Write_Byte(0x2, SSPI_COMMAND);
            SPI_Write_Byte(SSD1306_SET_CONTRAST_CONTROL, SSPI_COMMAND);
            SPI_Write_Byte(0xAF, SSPI_COMMAND);
            break;
        default:
            break;
    }

    SPI_Write_Byte(SSD1306_SET_PRECHARGE_PERIOD, SSPI_COMMAND);
    SPI_Write_Byte(0xF1, SSPI_COMMAND);
    SPI_Write_Byte(SSD1306_SET_VCOM_DESELECT, SSPI_COMMAND);
    SPI_Write_Byte(0x40, SSPI_COMMAND);
    SPI_Write_Byte(SSD1306_DISPLAY_ALL_ON_RESUME, SSPI_COMMAND);
    SPI_Write_Byte(SSD1306_NORMAL_DISPLAY, SSPI_COMMAND);
    SPI_Write_Byte(SSD1306_DEACTIVATE_SCROLL, SSPI_COMMAND);
    SPI_Write_Byte(SSD1306_DISPLAY_ON, SSPI_COMMAND);

    OLED_DELAY_MS(100);
}

bool SSD1306_SPI::SPI_Write_Byte(uint8_t value, uint8_t dc) {
    if (_SPI_fd < 0) return false;
    return SSD1306LinuxSPI::spi_transfer_byte(_SPI_fd, _SPI_DC_pin, value,
                                              dc == SSPI_DATA) >= 1;
}

void SSD1306_SPI::OLEDPowerDown() {
    OLEDEnable(0);
    OLED_DELAY_MS(100);
}

void SSD1306_SPI::OLEDEnable(uint8_t on) {
    SPI_Write_Byte(on ? SSD1306_DISPLAY_ON : SSD1306_DISPLAY_OFF, SSPI_COMMAND);
}

void SSD1306_SPI::OLEDContrast(uint8_t contrast) {
    SPI_Write_Byte(SSD1306_SET_CONTRAST_CONTROL, SSPI_COMMAND);
    SPI_Write_Byte(contrast, SSPI_COMMAND);
}

void SSD1306_SPI::OLEDInvert(bool on) {
    SPI_Write_Byte(on ? SSD1306_INVERT_DISPLAY : SSD1306_NORMAL_DISPLAY,
                   SSPI_COMMAND);
}

void SSD1306_SPI::OLEDclearBuffer() {
    memset(buffer, 0x00, (size_t)(bufferWidth * (bufferHeight / 8)));
}

void SSD1306_SPI::OLEDupdate() {
    OLEDBuffer(0, 0, bufferWidth, bufferHeight, buffer);
}

void SSD1306_SPI::OLEDBuffer(int16_t x, int16_t y, uint8_t w, uint8_t h,
                             uint8_t* data) {
    uint8_t tx, ty;
    uint16_t offset = 0;

    SPI_Write_Byte(SSD1306_SET_COLUMN_ADDR, SSPI_COMMAND);
    SPI_Write_Byte(0, SSPI_COMMAND);           // columna inicial
    SPI_Write_Byte(_OLED_WIDTH - 1, SSPI_COMMAND); // columna final

    SPI_Write_Byte(SSD1306_SET_PAGE_ADDR, SSPI_COMMAND);
    SPI_Write_Byte(0, SSPI_COMMAND);
    switch (_OLED_HEIGHT) {
        case 64: SPI_Write_Byte(7, SSPI_COMMAND); break;
        case 32: SPI_Write_Byte(3, SSPI_COMMAND); break;
        case 16: SPI_Write_Byte(1, SSPI_COMMAND); break;
    }

    for (ty = 0; ty < h; ty = ty + 8) {
        if (y + ty < 0 || y + ty >= _OLED_HEIGHT) { continue; }
        for (tx = 0; tx < w; tx++) {
            if (x + tx < 0 || x + tx >= _OLED_WIDTH) { continue; }
            offset = (w * (ty / 8)) + tx;
            SPI_Write_Byte(data[offset++], SSPI_DATA);
        }
    }
}

void SSD1306_SPI::drawPixel(int16_t px, int16_t py, uint8_t color) {
    if ((px < 0) || (px >= bufferWidth) || (py < 0) || (py >= bufferHeight)) {
        return;
    }
    int16_t temp;
    switch (rotation) {
        case 1:
            temp = px;
            px = WIDTH - 1 - py;
            py = temp;
            break;
        case 2:
            px = WIDTH - 1 - px;
            py = HEIGHT - 1 - py;
            break;
        case 3:
            temp = px;
            px = py;
            py = HEIGHT - 1 - temp;
            break;
        default:
            break;
    }
    uint16_t tc = (bufferWidth * (py / 8)) + px;
    switch (color) {
        case 1: buffer[tc] |= (1 << (py & 7)); break;
        case 0: buffer[tc] &= ~(1 << (py & 7)); break;
        case 2: buffer[tc] ^= (1 << (py & 7)); break;
        default: break;
    }
}

// ******************** EOF ********************