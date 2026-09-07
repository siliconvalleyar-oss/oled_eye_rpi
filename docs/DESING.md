# EyePet — Diseño (DESING)

> El nombre de este archivo se mantiene como "DESING" (sic) por requisito del
> prompt original del proyecto.

Documento de diseño de EyePet: criterios visuales y de interacción del ojo.

## Objetivo

Emular de forma reconocible el ojo de una mascota animada en un OLED monocromo
de 128×64, con movimientos naturales y expresiones emocionales: parpadeos,
exploración suave, seguimiento, sacadas, dormido y brillo corneal.

## Geometría del ojo

Se compone de (de abajo a arriba en el buffer):

1. **Esclera**: círculo blanco de radio `sclera_r` (por defecto 20 px).
2. **Iris**: círculo centrado en la pupila con textura de anillos (para
   diferenciarlo de la esclera en monocromo).
3. **Pupila**: círculo negro de radio `pupil_r` (por defecto 5 px); se dilata o
   contrae según expresión.
4. **Párpados**: recortes superior e inferior que reducen la apertura del ojo
   (animación de parpadeo). En dormido solo queda una línea horizontal.
5. **Cejas**: trazo corto sobre el ojo cuyo ángulo expresa emoción.
6. **Brillo**: punto blanco pequeño en el cuadrante superior del iris.

## Principios de animación

- **Predecibilidad**: los movimientos se aproximan con suavizado exponencial
  (interpolación hacia un objetivo), dando sensación "orgánica".
- **Parpadeo realista**: fase de cierre, pausa breve, fase de apertura; los
  intervalos varían aleatoriamente dentro de un rango.
- **Sacadas**: la pupila salta entre posiciones y se estabiliza brevemente.
- **Expresiones** modifican ceja (ángulo) y pupila (tamaño):
  - Happy → cejas arqueadas; Surprised → cejas elevadas y pupila pequeña;
    Angry → cejas fruncidas; Sleepy → párpado entornado.

## Despliegue sobre el buffer

Todas las primitivas (`fillCircle`, `fillRect`, `drawLine`, ...) operan sobre el
frame buffer de la clase del driver; una llamada `update()` por fotograma vuelca
el buffer al display (eficiencia de 1 sola transferencia de 1024 bytes por
fotograma frente a múltiples transacciones).

## Configuración

Todos los valores de geometría, tiempos y rango se leen de `config/config.cfg` y
`config/hardware.cfg`, permitiendo ajustar sin recompilar.

## Limitaciones del hardware

- OLED monocromo (1 bit por píxel): el color se simula con patrones
  (anillos del iris, contornos).
- 128×64 = 1024 bytes de buffer: suficiente para ojo + ceja con margen.
- Sin antialias nativo; las curvas se rasterizan con las primitivas provistas.