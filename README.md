# EyePet — Emulación de ojo de mascota en OLED SSD1306

EyePet es una aplicación C++17 para **Raspberry Pi** que emula el ojo de una
mascota animada sobre una pantalla **OLED SSD1306 (128×64)**. El ojo (esclera,
iris, pupila, párpados y cejas) se dibuja en un buffer en memoria y se anima con
parpadeos, movimientos de exploración, seguimiento, expresiones emocionales,
movimientos sacádicos, modo dormido y un pequeño brillo en el iris para dar
realismo. Todo el comportamiento es configurable desde archivos de texto y la
línea de comandos.

Compatible con **Raspberry Pi de 32 bits (armhf)** y **64 bits (aarch64)**,
incluidas las series recientes (Pi 5 / Compute Module 5). El acceso al display
se realiza por **I2C `/dev/i2c-N`** (por defecto) o **SPI `/dev/spidev`** usando
`ioctl` de Linux, sin depender de `bcm2835` (que no soporta I2C en chips RP1).

---

## Características

| Modo | Descripción |
|------|-------------|
| **Normal** | Ojo abierto; la pupila explora suavemente en un patrón aleatorio. |
| **Parpadeo** | El ojo se cierra y abre periódicamente con intervalos configurables. |
| **Tracking** | La pupila sigue un objetivo simulado en un patrón circular. |
| **Happy** | Cejas arqueadas con expresión alegre. |
| **Surprised** | Cejas elevadas, ojo muy abierto. |
| **Angry** | Cejas fruncidas, expresión de enfado. |
| **Sleepy** | Párpado medio cerrado. |
| **Sleep** | Ojo cerrado con línea de párpado y temblores ocasionales. |
| **Saccades** | Pupila con saltos rápidos entre posiciones (movimientos sacádicos). |
| **Brillo** | Punto blanco (glint) sobre el iris para realismo. |

## Requisitos

- Raspberry Pi (cualquier modelo; recomendada Pi 2B o superior) con Raspberry Pi OS.
- Display OLED SSD1306 de 128×64 por I2C (pines GPIO2=SDA, GPIO3=SCL) o SPI (4 hilos).
- Paquetes: `g++`, `make`, `git`, `i2c-tools` (instalables con el script incluido).
- **Git** y opcionalmente **GitHub CLI (`gh`)** para el control de versiones.
- Acceso a `/dev/i2c-N`: ejecutar con `sudo` o pertenecer al grupo `i2c`.

## Compilación e instalación

Instala las dependencias (en la Raspberry Pi):

```sh
sudo ./scripts/install_deps.sh
```

Compila:

```sh
make            # nativo (en la Pi)
make -j4
```

El binario final se genera como `bin/App`. **La versión se muestra al iniciar la
aplicación y con `--version`** (se inyecta como macro en tiempo de compilación
desde el archivo `VERSION`).

Compilación cruzada desde un PC:

```sh
make crossover64   # ARM64 (aarch64) — para Pi modernas de 64 bits
make crossover32   # ARM32 (arm-linux-gnueabihf)
```

Compilación remota (en la propia Pi vía SSH):

```sh
make remote        # requiere config/deploy.cfg y credenciales SSH
```

## Uso

```sh
sudo ./bin/App                  # ejecuta con el modo/config del archivo config.cfg
sudo ./bin/App --version        # muestra la versión y termina
sudo ./bin/App --mode 6         # inicia en modo Dormido
sudo ./bin/App --style 5        # inicia con el ojo estilo Heart
sudo ./bin/App --demo 60        # demo de efectos de 60 s con menú interactivo
sudo ./bin/App --config config/config.cfg --hw-config config/hardware.cfg
sudo ./bin/App --help           # lista de opciones
```

Opciones de línea de comandos:

- `--version, -v` — muestra la versión y termina sin tocar el hardware.
- `--mode <0..7>` — modo inicial (0 Normal, 1 Tracking, 2 Happy, 3 Surprised,
  4 Angry, 5 Sleepy, 6 Sleep, 7 Saccades).
- `--style <0..5>` — estilo (versión) del ojo: 0 Classic, 1 Anime, 2 Feline,
  3 Robot, 4 Squint, 5 Heart.
- `--demo <segundos>` — duración de la demo de efectos (por defecto 180 s).
- `--config <archivo>` — ruta alternativa para `config/config.cfg`.
- `--hw-config <archivo>` — ruta alternativa para `config/hardware.cfg`.
- `--help, -h` — muestra la ayuda.

## Estilos del ojo

EyePet incluye 6 estilos (versiones) de ojo que cambian la esclera, la pupila
y el brillo. Se eligen con `style` en `config/config.cfg` o con `--style`:

| # | Estilo   | Descripción                                            |
|---|----------|--------------------------------------------------------|
| 0 | Classic  | Ojo redondeado clásico, pupila circular, un brillo.     |
| 1 | Anime    | Elipse grande a pantalla completa, pupila amplia, 2 brillos. |
| 2 | Feline   | Felino: iris marcado, pupila vertical alargada.         |
| 3 | Robot    | Visor rectangular, pupila cuadrada con retícula.        |
| 4 | Squint   | Entrecerrado: párpados dobles gruesos.                  |
| 5 | Heart    | Pupila en forma de corazón (dos círculos + triángulo).  |

El ojo ocupa **toda la pantalla** (128×64): por defecto la esclera tiene
`sclera_r = 30` centrada en `(64, 32)` (filas ~2..62). Ajusta la geometría en
`config/config.cfg` si quieres un ojo más pequeño o descentrado.

## Menú interactivo y demo de efectos

Durante la ejecución, EyePet muestra un menú que se controla **por teclado sin
pulsar Enter**:

| Tecla | Acción                                        |
|-------|-----------------------------------------------|
| `0`/`T` | Rotar por todos los estilos (modo TODOS).    |
| `1..6` | Elegir un estilo concreto (Classic..Heart).  |
| `b`   | Activar/desactivar parpadeo.                  |
| `g`   | Activar/desactivar brillo.                    |
| `e`   | Activar/desactivar cejas.                     |
| `+`/`-` | Velocidad (5..60 fps).                      |
| `c`/`C` | Contraste del display (0x00..0xFF).          |
| `p`/`P` | Dilatación de la pupila (−4..+6).            |
| `d`   | Depuración por consola on/off.                |
| `h`   | Mostrar esta ayuda.                           |
| `q`/`Q`/`Esc` | Salir.                              |

La demo guiada de efectos dura `demo_seconds` (180 s por defecto) y muestra
secuencialmente: **parpadeo espontáneo**, **mirada de izquierda a derecha**,
**feliz**, **triste**, **guiño** y **párpado cerrado**. Al terminar, el programa
restaura el teclado y sale. Se ajusta con `demo_seconds` en `config.cfg` o con
`--demo <segundos>`.

## Configuración de los modos del ojo

Todos los parámetros se ajustan en `config/config.cfg`:

- Modo inicial (`mode`), estilo (`style`), tasa de fotogramas
  (`frame_rate` / `frame_delay_ms`).
- Parpadeo: intervalo (`blink_min_ms`, `blink_max_ms`) y duración de fases.
- Movimiento: rango de la pupila (`move_range_x`, `move_range_y`).
- Geometría: centro y radios de esclera/iris/pupila (`eye_center_x`, ...).
- Brillo/reflejo (`glint`, `glint_dx`, `glint_dy`, `glint_r`).
- Cejas (`draw_eyebrows`), demo de efectos (`demo_seconds`) y depuración
  (`debug`).

El **hardware** (protocolo I2C o SPI, dirección, pines) se configura en
`config/hardware.cfg`.

## Configuración del repositorio Git

El proyecto incluye el script interactivo `setup_git.sh`, que:

1. Pregunta el nombre de usuario, credenciales (token o configuradas), nombre
   del repositorio y visibilidad (pública/privada).
2. Ejecuta `git init`, `git add .`, `git commit`, `gh repo create` (si `gh`
   está instalado) y `git push -u origin main`.
3. Si no hay GitHub CLI, crea el repositorio localmente y muestra los pasos
   manuales para conectarlo a la plataforma.

```sh
./setup_git.sh
```

Instalación de `gh` (si no está):

```sh
sudo apt install gh && gh auth login
```

## Estructura del proyecto

```
├── bin/                # binario final (App)
├── config/             # configuración (config.cfg, hardware.cfg, deploy.cfg)
├── docs/               # documentación completa (API, arquitectura, uso, etc.)
├── examples/           # ejemplos de uso
├── include/            # cabeceras (engine/, oled/, nlohmann/, ...)
├── obj/                # objetos de compilación (generado)
├── scripts/            # instalación, despliegue y compilación remota
├── src/                # código fuente (engine/ y oled/)
├── LICENSE             # licencia MIT
├── Makefile            # define VERSION y compila el proyecto
├── VERSION             # número de versión (p.ej. 1.0.0)
└── setup_git.sh        # configuración de repositorio Git
```

## Documentación

La documentación detallada está en `docs/`:

- `docs/API.md`, `docs/ARCHITECTURE.md`, `docs/ARQUITECTURA.md`
- `docs/USAGE.md`, `docs/BUILD.md`, `docs/INSTALL.md`, `docs/DEPLOY.md`
- `docs/HARDWARE.md`, `docs/TESTING.md`, `docs/OLED.md`
- `docs/CHANGELOG.md`, `docs/ROADMAP.md`, `docs/TODO.md`, entre otros.

## Licencia

MIT — ver [LICENSE](LICENSE).

---

*EyePet · Emulación de ojo de mascota en OLED para Raspberry Pi (32 y 64 bits).*