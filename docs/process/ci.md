# La CI en local

La CI ([ADR-0029](../adr/0029-ci-cd.md)) corre en el contenedor `archlinux:latest`, con
los repositorios de Arch congelados en una fecha del Arch Linux Archive. Con esa fecha,
el toolchain del contenedor es el mismo de la máquina del autor. Cada job de
`.github/workflows/ci.yml` solo llama a un script de `tools/ci/`, así que cualquier
fallo de la CI se repite en local con un comando.

## Los jobs

| Job | Script | Cuándo corre en un PR |
|---|---|---|
| `changes` | `changes.sh` | Siempre. Decide qué cambió el PR |
| `fast-cpp (format)` | `format.sh` | Si el PR toca código (las rutas están en `changes.sh`) |
| `fast-cpp (cpp-subset)` | `cpp-subset.sh` | Si el PR toca código |
| `fast-repo (reuse)` | `reuse.sh` | Siempre |
| `fast-repo (actionlint)` | `actionlint.sh` | Siempre |
| `ci-ok` | — | Siempre. Falla si algún job falló o se canceló |

En un push a `main` corre todo.

## Correr un job en local

```sh
tools/ci/run.sh <job>            # por ejemplo: tools/ci/run.sh reuse
tools/ci/run.sh format --fix     # los argumentos pasan al script
```

Necesita Docker. `run.sh` levanta el mismo contenedor que la CI, instala el toolchain de
la fecha con `tools/ci/install.sh` y corre el script con tu usuario. La primera vez baja
todos los paquetes (unos minutos). Después quedan en el volumen de Docker
`mz-ci-pacman-<fecha>`, y cada corrida tarda unos 40 s, casi todos de instalación: igual
que en la CI, el contenedor se instala desde cero cada vez.

Para el día a día, sin Docker y con las herramientas de tu máquina, está
`tools/ci/lint.sh` (formato, subconjunto de C++ y `clang-tidy`). `run.sh` sirve para
repetir exactamente lo que vio la CI.

## Cambiar la fecha del snapshot

La fecha vive en un solo archivo, `tools/ci/archive-date` (`AAAA/MM/DD`). La leen la CI,
`run.sh` y la clave de la caché de paquetes.

1. Después de actualizar tu máquina, corre:

   ```sh
   tools/ci/compare-toolchain.sh
   ```

   Compara las versiones de `tools/ci/toolchain.txt` en tu máquina con las del snapshot,
   y lista los paquetes que difieren.
2. Si algo difiere, busca una fecha en la que coincidan: cambia `archive-date` y vuelve
   a correr la comparación. Las fechas disponibles están en
   <https://archive.archlinux.org/repos/>.
3. La fecha nueva va en un PR `build:`. Si con ella cambia la `glibc`, ese mismo PR
   vuelve a grabar todos los hashes guardados (el del *replay* y el del render offline
   del audio) y lo dice en su descripción (ADR-0029, Decisión 7).

Los volúmenes de fechas viejas se borran con
`docker volume rm mz-ci-pacman-<fecha-vieja>`.

## Agregar un job

1. Un script `tools/ci/<job>.sh` que corra en el contenedor.
2. El job en `ci.yml`, que solo llama al script. Si no hace falta en todos los PRs, se
   salta con un `if:` sobre las salidas de `changes`, **nunca con `paths:`**: un
   chequeo saltado por `paths:` nunca se reporta.
3. El job en el `needs` de `ci-ok`. El ruleset no se toca.

Si el job necesita un paquete nuevo, va en `tools/ci/toolchain.txt` cuando tiene que
coincidir con tu máquina, o en `tools/ci/tools.txt` cuando solo lo usa la CI.

## `ci-ok` en "Proteger main"

`ci-ok` es el único chequeo obligatorio. Se configura una sola vez, después de la
primera corrida, porque GitHub solo ofrece chequeos que ya vio: en *Settings → Rules →
Rulesets → Proteger main*, activar *Require status checks to pass* y agregar `ci-ok`
con la fuente *GitHub Actions*.
