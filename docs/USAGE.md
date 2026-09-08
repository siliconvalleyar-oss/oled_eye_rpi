# EyePet — Manual de uso

Cómo ejecutar y controlar la aplicación.

## Ejecución básica

```sh
sudo ./bin/App
```

Muestra por consola la versión (`EyePet v1.0.0`) y comienza la animación del
ojo con la configuración de `config/config.cfg`.

> Se necesita acceso a `/dev/i2c-N`: usa `sudo` o añade tu usuario al grupo
> `i2c` (`sudo usermod -a -G i2c $USER` y vuelve a iniciar sesión).

## Opciones de línea de comandos

| Opción | Descripción |
|--------|-------------|
| `--version`, `-v` | Muestra la versión y termina (no toca el hardware). |
| `--mode <0..7>` | Modo inicial del ojo (0 Normal, 1 Tracking, 2 Happy, 3 Surprised, 4 Angry, 5 Sleepy, 6 Sleep, 7 Saccades). |
| `--style <0..3>` | Estilo (versión) del ojo (0 Classic, 1 Anime, 2 Feline, 3 Robot). |
| `--config <archivo>` | Ruta alternativa para `config/config.cfg`. |
| `--hw-config <archivo>` | Ruta alternativa para `config/hardware.cfg`. |
| `--help`, `-h` | Muestra la ayuda. |

### Ejemplos

```sh
./bin/App --version
sudo ./bin/App --mode 1
sudo ./bin/App --style 3          # ojo estilo Robot
sudo ./bin/App --mode 6 --config /opt/eye/config.cfg
```

## Modos del ojo

| Modo | Nº | Comportamiento |
|------|----|----------------|
| Normal | 0 | Exploración suave: la pupila se mueve lenta y aleatoriamente. |
| Tracking | 1 | La pupila sigue un patrón circular (objeto simulado). |
| Happy | 2 | Cejas arqueadas, expresión alegre. |
| Surprised | 3 | Cejas elevadas, pupila pequeña. |
| Angry | 4 | Cejas fruncidas, enfado. |
| Sleepy | 5 | Párpado medio cerrado. |
| Sleep | 6 | Ojo cerrado con temblores ocasionales. |
| Saccades | 7 | Saltos rápidos entre posiciones. |

## Estilos del ojo

El ojo **ocupa toda la pantalla** (128×64) con la geometría por defecto
(`sclera_r = 30` centrado en `(64, 32)`). Se seleccionan con `style` en
`config/config.cfg` o con `--style <0..3>`:

| Estilo | Nº | Forma de la esclera | Pupila | Brillo |
|--------|----|---------------------|--------|--------|
| Classic | 0 | círculo | circular | 1 punto |
| Anime | 1 | elipse ancha | amplia | 2 brillos |
| Feline | 2 | elipse almendra | vertical alargada | línea vertical |
| Robot | 3 | rectángulo (visor) | cuadrada con retícula | punto + scanline |

## Configuración dinámica

Edita `config/config.cfg` para cambiar parámetros sin recompilar:

- `mode` — modo inicial.
- `style` — estilo (versión) del ojo.
- `frame_rate` / `frame_delay_ms` — cadencia de animación.
- `blink_min_ms` / `blink_max_ms` — intervalo entre parpadeos.
- `move_range_x` / `move_range_y` — rango de la pupila.
- `sclera_r`, `iris_r`, `pupil_r` — tamaños del ojo.
- `glint`, `glint_dx`, `glint_dy`, `glint_r` — brillo/reflejo.
- `draw_eyebrows`, `debug` — cejas y depuración.

El protocolo (I2C/SPI), la dirección y los pines se configuran en
`config/hardware.cfg`.

## Salida por consola

- Al iniciar: `EyePet v<version>`.
- En depuración (`debug = true`): cada 30 fotogramas se imprimen el número de
  fotograma, el modo y la posición de la pupila.
- Errores de configuración y de hardware se muestran por `stderr`.

## Detener la aplicación

`Ctrl+C` finaliza el proceso. El destructor de `Eye::Eye_t` apaga el display de
forma segura solo si este se inicializó previamente.