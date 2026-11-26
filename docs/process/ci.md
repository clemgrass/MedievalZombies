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
| `blender` | `blender.sh` | Si el PR toca código. Instala el Blender fijado y comprueba su versión |
| `build (debug)` | `build.sh debug` | Si el PR toca código, y pasaron la etapa rápida y `blender` |
| `build (debug-gcc)` | `build.sh debug-gcc` | Igual |
| `build (relwithdebinfo)` | `build.sh relwithdebinfo` | Igual |
| `build (release)` | `build.sh release` | Igual |
| `coverage` | `coverage.sh` | Igual |
| `clang-tidy` | `clang-tidy.sh` | Igual |
| `ci-ok` | — | Siempre. Falla si algún job falló o se canceló |

En un push a `main` corre todo.

Los jobs `build` son la segunda etapa del ADR-0029: arrancan solo si pasaron `fast-cpp`,
`fast-repo` y `blender` (la configuración necesita Blender, ver [Blender](#blender)). Cada uno corre un *workflow preset* de `CMakePresets.json`, que configura,
compila y, en los dos `debug`, corre los tests:

| Preset | Compilador | Qué hace |
|---|---|---|
| `debug` | Clang | Compila con ASan, UBSan y LSan, y corre los tests |
| `debug-gcc` | GCC | Lo mismo con el compilador secundario: hereda todo de `debug` menos el compilador |
| `relwithdebinfo` | Clang | Solo compila |
| `release` | Clang | Solo compila |

Un warning es un error en los cuatro, así que un warning que solo da GCC también rompe
la CI. El caso `cmake.warnings.gcc_only` (solo en el build de GCC) lo comprueba.

En la misma etapa corren otros dos jobs:

- **`coverage`** corre el *workflow preset* `coverage`: compila con la cobertura de
  Clang, corre los tests y mide con `llvm-cov`. Tiene la **meta del 100 % en líneas y
  ramas** del ADR-0011 en las carpetas de `tests/CMakeLists.txt`
  (`mz_add_coverage_target(GOAL …)`), que es la única lista. El job falla si en esas
  carpetas queda una línea sin ejecutar, un lado de una condición que nunca se tomó o un
  `.cpp` que ningún ejecutable de tests enlaza, y el resumen del job dice cuál es, con su
  archivo y su línea. El reporte HTML queda como artefacto `cobertura` (14 días), también
  cuando el job falla.
- **`clang-tidy`** corre `clang-tidy` sobre `engine/`, `game/`, `tests/` y `tools/cooker/`
  (cgltf, de terceros, no entra) con la configuración de `.clang-tidy`, que trata cada
  hallazgo como un error.

Una zona nueva con TDD suma su carpeta a la lista en el mismo ticket que la crea. Si
alguna vez una línea no se puede cubrir, el ADR-0011 pide una justificación escrita junto
al código. `llvm-cov` no tiene marcadores para excluir una línea: el ticket que traiga el
primer caso agrega el mecanismo a `cmake/mz_coverage_goal.cmake`, con su test en
`tests/tooling/`.

## Correr un job en local

```sh
tools/ci/run.sh <job>            # por ejemplo: tools/ci/run.sh reuse
tools/ci/run.sh format --fix     # los argumentos pasan al script
tools/ci/run.sh build debug-gcc  # un job build, con su preset
tools/ci/run.sh coverage         # la cobertura con la meta (el HTML se borra al salir)
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
`run.sh` y las claves de las dos cachés de la CI: la de los paquetes y la de las bases de
datos de los repositorios (`core.db`, `extra.db`). Las bases de datos de una fecha no
cambian, así que `install.sh` solo las baja si las que tiene no son de esa fecha (lo dice
una marca, `/var/lib/pacman/sync/mz-archive-date`) o si falta alguna. Con las dos cachés, un job no le pide
nada al archivo, que a veces no responde.

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
`docker volume rm mz-ci-pacman-<fecha-vieja> mz-ci-pacman-sync-<fecha-vieja>`.

## Blender

Blender no sale de `pacman`: el paquete de Arch sigue la fecha del snapshot, no la
versión que elegimos, y aun con la misma versión es otro build. La CI y tu máquina usan
el **tarball oficial** de `download.blender.org`, con la versión y su `sha256` fijados
en un solo archivo, `tools/ci/blender-version`.

```sh
tools/ci/install-blender.sh   # instala (si hace falta) y escribe la ruta del binario
tools/ci/blender.sh           # lo mismo que el job: instala y corre blender -b --version
```

`install-blender.sh` no necesita root. Baja el tarball, verifica el `sha256` y lo
extrae en `~/.cache/medievalzombies/blender-<versión>/` (o en `$MZ_TOOLS_DIR`, si está
definida). Si el `sha256` no coincide, falla y no instala nada. Esa carpeta es la caché:
la segunda corrida no baja nada, y si cambia el pin se reinstala sola. Lo único que
escribe en stdout es la ruta del binario, para que quien lo llama la use tal cual. El
cocinador nunca usa el `blender` del `PATH`.

**El build lo necesita** (ADR-0017): `cmake/mz_blender.cmake` llama a
`install-blender.sh` al configurar (o usa `-DMZ_BLENDER=<ruta>`: un Blender oficial de la
misma versión en otra carpeta, sin verificar el `sha256`), verifica que el binario
sea la versión fijada y exporta cada `.blend` a `.glb` con `tools/blender/export_glb.py`.
Después, `mz_cook` (el cocinador de `tools/cooker/`, que se compila en el mismo build)
cocina cada `.glb` al `.mzasset` que carga el juego (`mz_cook_blend`, en
`cmake/mz_cook.cmake`). Por eso toda la segunda etapa espera al job `blender` y restaura
su caché: Blender se baja una sola vez por pin. Los tests de la cadena tienen la etiqueta
`assets` (`ctest --preset debug -L assets`), y los de la biblioteca del cocinador, la
etiqueta `tools`.

En la CI, la carpeta instalada va a la caché de GitHub con el pin como clave.
`tools/ci/run.sh` monta tu caché en el contenedor, así que un tarball bajado sirve para
las dos. Las bibliotecas de X11 y OpenGL que el binario enlaza aunque corra sin interfaz
están en `tools/ci/tools.txt`.

**Cambiar la versión:**

1. Busca el `sha256` en `https://download.blender.org/release/Blender<X.Y>/`, en el
   archivo `blender-<versión>.sha256`, en la línea de `blender-<versión>-linux-x64.tar.xz`.
2. Cambia `version` y `sha256` en `tools/ci/blender-version`, en un PR `build:`.
3. Corre `tools/ci/blender.sh` en tu máquina.
4. Compila: el pin es una dependencia de cada exportación, así que el build vuelve a
   configurar y a exportar todos los `.blend`. Revisa la diferencia en el mismo PR: con
   la misma versión la exportación es idéntica bit a bit, y cambiarla es un cambio
   deliberado. Si modelas en otro computador, pásalo también a la versión nueva.

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
