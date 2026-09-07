# EyePet — Despliegue (DEPLOY)

Proceso de despliegue del binario a una Raspberry Pi remota.

## Configuración

El archivo `config/deploy.cfg` define el destino:

```ini
REMOTE_USER=pi
REMOTE_HOST=rpi2w.local
REMOTE_DIR=/home/pi/src/eye_oled
REMOTE_BRANCH=crossover
```

## Compilación remota (`make remote`)

`scripts/build_remote.sh` se conecta por SSH y ejecuta en la Pi:

```
cd <REMOTE_DIR>
git pull
make clean
make -j4
```

Requiere:
- `sshpass` instalado en el PC: `sudo apt install sshpass`.
- La variable de entorno `SSHPASS` con la contraseña SSH (no se guarda en el repo):

```sh
read -s SSHPASS; export SSHPASS
make remote
```

## Copia del binario (`make deploy`)

`scripts/deploy.sh` usa `scp` para copiar `bin/App` a `REMOTE_DIR/bin/App` en el
host destino (mismo mecanismo `sshpass -e`).

```sh
make deploy
```

## Flujo recomendado (versionado con tags)

1. Actualiza `VERSION` (p.ej. `1.0.1`).
2. Confirma los cambios: `git add -A && git commit -m "fix: ..."`.
3. Push y tag:

```sh
git push origin main
git tag -a v1.0.1 -m "EyePet v1.0.1"
git push origin v1.0.1
```

4. En la Pi: `git pull && make -j4 && sudo ./bin/App`.

> Regla del proyecto: **todo push lleva su tag**; el archivo `VERSION` debe
> coincidir con el último tag (con o sin `v`).

## Seguridad

- Nunca guardar contraseñas ni tokens en el repositorio.
- Para despliegues frecuentes se recomienda configurar claves SSH
  (`ssh-copy-id pi@host`) y así eliminar `sshpass`.