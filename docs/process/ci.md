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
| `build (debug)` | `build.sh debug` | Si el PR toca código y la etapa rápida pasó |
| `build (debug-gcc)` | `build.sh debug-gcc` | Igual |
| `build (relwithdebinfo)` | `build.sh relwithdebinfo` | Igual |
| `build (release)` | `build.sh release` | Igual |
| `ci-ok` | — | Siempre. Falla si algún job falló o se canceló |

En un push a `main` corre todo.

Los jobs `build` son la segunda etapa del ADR-0029: arrancan solo si pasaron `fast-cpp`
y `fast-repo`. Cada uno corre un *workflow preset* de `CMakePresets.json`, que configura,
compila y, en los dos `debug`, corre los tests:

| Preset | Compilador | Qué hace |
|---|---|---|
| `debug` | Clang | Compila con ASan, UBSan y LSan, y corre los tests |
| `debug-gcc` | GCC | Lo mismo con el compilador secundario: hereda todo de `debug` menos el compilador |
| `relwithdebinfo` | Clang | Solo compila |
| `release` | Clang | Solo compila |

Un warning es un error en los cuatro, así que un warning que solo da GCC también rompe
la CI. El caso `cmake.warnings.gcc_only` (solo en el build de GCC) lo comprueba.

## Correr un job en local

```sh
tools/ci/run.sh <job>            # por ejemplo: tools/ci/run.sh reuse
tools/ci/run.sh format --fix     # los argumentos pasan al script
tools/ci/run.sh build debug-gcc  # un job build, con su preset
```

Necesita Docker. `run.sh` levanta el mismo contenedor que la CI, instala el toolchain de
la fecha con `tools/ci/install.sh` y corre el script con tu usuario. La primera vez baja
todos los paquetes (unos minutos). Después quedan en el volumen de Docker
`mz-ci-pacman-<fecha>`, y cada corrida tarda unos 40 s, casi todos de instalación: igual
que en la CI, el contenedor se instala desde cero cada vez. Un job `build` suma lo que
tarda en compilar (hoy, de 60 a 95 s en total).

El contenedor no usa tu `build/`: `/src/build` es un volumen que se borra al terminar.
Compila desde cero, como la CI, y tu build sigue intacto (CMake no acepta una caché
configurada en otra carpeta).

Para el día a día, sin Docker y con las herramientas de tu máquina, están
`tools/ci/lint.sh` (formato, subconjunto de C++ y `clang-tidy`) y los *workflow
presets*, que son lo mismo que corren los jobs `build`:

```sh
cmake --workflow --preset debug-gcc   # configurar, compilar y probar con GCC
```

`run.sh` sirve para repetir exactamente lo que vio la CI.

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
