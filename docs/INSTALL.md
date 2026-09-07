# EyePet — Instalación

Guía paso a paso para instalar EyePet en una Raspberry Pi (32 o 64 bits).

## 1. Requisitos de hardware

- Raspberry Pi (cualquier modelo) con Raspberry Pi OS (32/64 bits).
- Display OLED SSD1306 de 128×64 píxeles.
  - **I2C** (por defecto): SDA → GPIO2 (pin 3), SCL → GPIO3 (pin 5), VCC → 3.3V,
    GND → GND.
  - **SPI** (opcional): SCK, MOSI, DC (ej. GPIO24), RST (ej. GPIO25), CS.
- Opcional: bonnet de 40 pines retirado (ocupa los pines 3 y 5 del I2C).

## 2. Instalar dependencias

```sh
sudo ./scripts/install_deps.sh
```

El script instala `build-essential`, `g++`, `make`, `git`, `i2c-tools`, añade el
usuario al grupo `i2c` y habilita I2C/SPI en `/boot/config.txt`.

> Se necesita una **reinicialización** tras modificar `/boot/config.txt`:
> `sudo reboot`.

## 3. Verificar el hardware

```sh
sudo i2cdetect -y 1
```

El display debe aparecer en la dirección `0x3c`. Prueba directa:

```sh
sudo i2cget -y 1 0x3c
```

## 4. Compilar

```sh
make -j4
```

Se genera `bin/App`. La versión se muestra al iniciar y con `--version`.

## 5. Ejecutar

```sh
sudo ./bin/App
```

Cambia el modo inicial: `sudo ./bin/App --mode 6` (dormido).

## 6. Configuración

- Comportamiento del ojo: edita `config/config.cfg`.
- Protocolo y pines: edita `config/hardware.cfg` (I2C por defecto).

## 7. Actualizar

```sh
git pull
make -j4
sudo ./bin/App
```

## Instalación avanzada (systemd)

Para arranque automático, crea `/etc/systemd/system/eyepet.service`:

```ini
[Unit]
Description=EyePet OLED mascot eye
After=multi-user.target

[Service]
ExecStart=/home/pi/eye_oled/bin/App
WorkingDirectory=/home/pi/eye_oled
Restart=always
User=pi

[Install]
WantedBy=multi-user.target
```

```sh
sudo systemctl enable eyepet.service
sudo systemctl start eyepet.service
```

## Desinstalación

```sh
rm -rf bin obj sysroot
git clean -xdn   # muestra qué eliminaría (sin borrar)
```

Para retirar el servicio:

```sh
sudo systemctl stop eyepet.service && sudo systemctl disable eyepet.service
```