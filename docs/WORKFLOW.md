# Workflow de Desarrollo — EyePet

Flujo de trabajo establecido para el proyecto EyePet.

## Principios

- **Cambios de código:** se realizan localmente en `$PWD` (el repo local). Nada
  se edita directamente en la máquina remota.
- **Compilación:** local (PC) opcional para validar; la **oficial** se hace en
  la Raspberry Pi (`make`, o `make crossover*` desde el PC).
- **Pruebas:** se ejecutan en la Raspberry Pi (sobre el display real).
- **Archivos generados:** `obj/`, `bin/App`, `sysroot/` no se commitean
  (ver `.gitignore`).

## Flujo paso a paso

1. **Editar local:** cambiar código/documentación en `$PWD`.
2. **Compilar local (opcional):** `make` para detectar errores de compilación.
3. **Commit local:** mensaje semántico (`feat:`, `fix:`, `docs:`, `chore:`).
4. **Versionar:** actualizar `VERSION`, commit, push, tag (ver abajo).
5. **Deploy:** `make remote` (compilar en la Pi) o `make deploy` (copiar binario).
6. **Probar remoto:** ejecutar en la Pi sobre el OLED real.

## Versionado con tags (regla del proyecto)

```
echo "1.0.0" > VERSION
git add -A
git commit -m "feat: descripción"
git push origin main
git tag -a v1.0.0 -m "EyePet v1.0.0"
git push origin v1.0.0
```

- `VERSION` siempre coincide con el último tag (sin `v`).
- Un tag por rama/push; secuencia de patches 0-9 y luego minor
  (`v1.0.9` → `v1.1.0`). No retroceder versiones ni eliminar tags publicados.

## Comandos de referencia

```bash
# Compilar local
make -j4

# Compilar cruzado / remoto
make crossover64          # ARM64 desde PC
make remote               # compila en la Pi vía SSH (config/deploy.cfg)

# Ejecutar en la Pi (con display)
sudo ./bin/App
sudo ./bin/App --version

# Depuración I2C en la Pi
sudo i2cdetect -y 1

# Setup del repositorio git (interactivo)
./setup_git.sh
```

## Buenas prácticas

- Probad `./bin/App --version` siempre **sin** display (no abre el bus).
- Si el cambio afecta al dibujo, validar visualmente en el display con cada
  modo (`--mode 0..7`).
- Actualizar `docs/CHANGELOG.md` y `docs/ACTIVITY.md` con cada entregable.