# EyePet — Hardware

Cableado y configuración del display SSD1306 para EyePet.

## Display SSD1306 (128×64)

El SSD1306 admite dos interfaces:

- **I2C** (modo por defecto de EyePet) — solo 4 cables.
- **SPI 4 hilos** (SCK, MOSI, DC, RST) — opcional.

Ambas se manejan mediante `ioctl` de Linux (`/dev/i2c-N` y `/dev/spidev`), por
lo que **no se usa la librería `bcm2835`**. Esto garantiza compatibilidad con
toda la familia Raspberry Pi, incluidas Pi 5 / Compute Module 5 (chip RP1),
donde `bcm2835` no soporta I2C.

## Conexión I2C

| Display | Raspberry Pi | Nota |
|---------|--------------|------|
| VCC   | 3.3V (pin 1) | algunos módulos aceptan 5V con los pines internos |
| GND   | GND (pin 6)  | |
| SDA   | GPIO2 (pin 3) | |
| SCL   | GPIO3 (pin 5) | |

Configuración en `/boot/config.txt`:

```
dtparam=i2c_arm=on
# opcional, para ajustar velocidad:
# dtparam=i2c_arm_baudrate=100000
```

Dirección I2C: `0x3C` (la mayoría) o `0x3D` (algunos módulos). Se configura en
`config/hardware.cfg`.

## Conexión SPI (4 hilos)

| Display | Raspberry Pi |
|---------|--------------|
| VCC  | 3.3V |
| GND  | GND |
| SCK  | GPIO11 (SPI0_CLK) |
| MOSI | GPIO10 (SPI0_MOSI) |
| DC   | GPIO24 (configurable) |
| RST  | GPIO25 (configurable) |
| CS   | GPIO8 (SPI0_CE0) → `/dev/spidev0.0` |

Configuración en `/boot/config.txt`:

```
dtparam=spi=on
```

Selecciona SPI en `config/hardware.cfg`:

```ini
protocol = spi
spi_device = /dev/spidev0.0
spi_speed = 1000000
spi_dc_pin = 24
spi_rst_pin = 25
```

Los pines DC y RST se controlan por GPIO de sysfs (`/sys/class/gpio`), sin
librerías adicionales.

## Probar el display

```sh
sudo i2cdetect -y 1        # debe mostrar 0x3c (o 0x3d)
sudo i2cget -y 1 0x3c      # prueba de lectura
sudo i2cset -y 1 0x3c 0x00 0xAF   # enciende el display
```

Si el bus I2C aparece completamente vacío, el problema es eléctrico (cableado
SDA/SCL, VCC, GND) o de dirección.

## Notas

- Un bonnet montado sobre el header de 40 pines puede cubrir GPIO2/GPIO3
  (pines 3 y 5) e impedir el I2C; retíralo si ya no se usa.
- Con el display por I2C puedes desactivar HDMI para ahorrar recursos:
  `dtoverlay=disable-hdmi` (o `gpu_mem=16`) en `/boot/config.txt`.
- Tras editar `/boot/config.txt`, reinicia: `sudo reboot`.
- El driver de EyePet abre el adaptador en cada operación (como `i2cset`), por
  lo que debe ejecutarse con `sudo` o con el usuario en el grupo `i2c`.