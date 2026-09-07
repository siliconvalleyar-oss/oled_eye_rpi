# EyePet — Seguridad (SECURITY)

Consideraciones de seguridad del proyecto EyePet.

## Buenas prácticas

- **No subir secretos al repositorio.** La contraseña SSH se pasa por la
  variable de entorno `SSHPASS` (nunca en el repo). Los tokens de GitHub se
  introducen en `setup_git.sh` sin mostrarse en pantalla y no se persisten.
- Trabaja con el usuario `pi` (no root) salvo cuando el acceso a `/dev/i2c-N`
  lo requiera; el grupo `i2c` otorga acceso sin sudo.
- El binario `bin/App` y los objetos `obj/` están en `.gitignore`.

## Permisos del hardware

- `/dev/i2c-N` suele exigir permisos de root o pertenencia al grupo `i2c`.
- `/dev/spidevX.Y` puede requerir el grupo `spi`.
- Añade tu usuario a ambos grupos tras instalar con `install_deps.sh`.
- Revisa los permisos reales con `ls -l /dev/i2c-* /dev/spidev*`.

## Despliegue remoto

- SSH con `sshpass` y variable `SSHPASS` (no usar contraseñas en texto plano en
  scripts versionados). Mejor aún: claves SSH (`ssh-copy-id`) y deshabilitar
  contraseñas en la Pi.
- Restringe `REMOTE_HOST`/`REMOTE_USER` en `config/deploy.cfg` al entorno real.

## Reporte de vulnerabilidades

Proyecto educativo/local. Para reportar un problema grave, abre un issue en el
repositorio (o en la plataforma donde se aloje) describiendo el vector y el
impacto sin exponer datos sensibles.