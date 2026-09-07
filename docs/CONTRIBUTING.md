# EyePet — Guía de contribución (CONTRIBUTING)

Gracias por contribuir a EyePet. Estas son las reglas mínimas.

## Flujo de trabajo

1. Crea un fork o una rama para tu cambio:
   ```sh
   git checkout -b feat/<descripcion>
   ```
2. Realiza los cambios y añade comentarios explicativos en el código.
3. Compila sin errores ni avisos importantes:
   ```sh
   make clean && make
   ```
4. Verifica la versión: `./bin/App --version`.
5. Confirma con un mensaje convencional:
   - `feat: <descripcion>` — nueva funcionalidad.
   - `fix: <descripcion>` — corrección de errores.
   - `docs: <descripcion>` — documentación.
   - `refactor:` / `chore:` / `test:` — otros.
6. Crea un pull request describiendo el cambio y cómo probarlo.

## Estilo de código

- C++17, nombres descriptivos, `const`-correctness.
- Todo código comentado en español o inglés (consistente dentro del archivo).
- Sin `new`/`delete` explícitos en el código de aplicación (usar smart pointers).
- Respeta el namespace `Eye` y la interfaz `Eye::IDisplay`.

## Reglas del repositorio

- Todo push lleva su tag (ver `docs/DEPLOY.md`).
- El archivo `VERSION` debe coincidir con el último tag.
- No eliminar tags publicados; ante un error, crear el siguiente número.

## Probar tus cambios

- Revisa `docs/TESTING.md` para la matriz de pruebas.
- Si el cambio afecta al dibujo, valida visualmente sobre un display real
  además de `./bin/App --version`.