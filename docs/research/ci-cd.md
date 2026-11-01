# Mediciones de la CI/CD

Nota de investigación del [ADR-0029](../adr/0029-ci-cd.md). Guarda las mediciones que
sostienen la decisión, para poder repetirlas.

Fecha: 2026-10-28. Hay dos lugares de medición:

- **La máquina del autor** (la de referencia del ADR-0020: Dell XPS 13 9360, 8 GB de
  RAM), con Docker 29.9.2 y contenedores limitados a 2–3 GB de memoria.
- **Los runners estándar de GitHub Actions** (`ubuntu-24.04`, en un repo público: 4 vCPU,
  16 GB de RAM y 14 GB de disco), en una rama de *spike* (`spike/51-ci`, ADR-0014) que no
  se mergeó. El resultado quedó en un comentario del #51, y las ramas, las ejecuciones y
  el paquete de prueba se borraron al terminar.

El proyecto de prueba es código descartable, fuera del repo: una biblioteca con dos
funciones, cinco tests de GoogleTest que enlazan SDL3, un ejecutable que calcula un hash
del estado con `sin`, `cos`, `exp`, `atan2` y `acos` en `float` durante 1 millón de
pasos, y una fuga de memoria deliberada para comprobar LeakSanitizer. Usa CMake con
presets (`debug-clang`, `debug-gcc`, `coverage`, `relwithdebinfo`) y los *workflow
presets* de CMake, y un script `ci.sh <job>` que corre lo mismo en local y en la CI.

| Qué | Máquina del autor |
|---|---|
| Linux / glibc | 7.2.8 / 2.44 |
| Clang / GCC | 23.1.3 / 16.2.1 |
| CMake / Ninja | 4.4.4 / 1.13.2 |
| SDL3 / GoogleTest | 3.4.18 / 1.18.0 |
| Blender | 5.2.2 LTS |
| Mirror de paquetes | `stable-mirror.omarchy.org` (ver la sección 2) |

## 1. Los entornos (P1)

**Qué se midió:** el mismo proyecto, con los mismos jobs (formato, Clang y GCC con ASan y
UBSan, cobertura con MC/DC y `clang-tidy`), en cuatro entornos. En los runners, la
duración sale de la API de GitHub para cada paso.

### En la máquina del autor

| Entorno | Toolchain | Instalación | Imagen | Resultado |
|---|---|---|---|---|
| Contenedor `archlinux:latest` | Clang 23.1.4, GCC 16.2.1, CMake 4.4.5, SDL3 3.4.18 | 183 s | 2,6 GB | Pasa todo. LSan detecta la fuga dentro de Docker |
| Contenedor `ubuntu:24.04` + Clang de apt.llvm.org | Clang 23.1.5, **GCC 13.3**, **CMake 3.28.3**, sin `libsdl3-dev` | 297 s | 2,5 GB | No configura sin más trabajo: SDL3 hay que compilarlo (`MZ_VENDOR_SDL3`), y eso exige sus dependencias de X11, Wayland y audio. CMake 3.28 no tiene `CMAKE_LINKER_TYPE` (3.29) |
| Contenedor `ubuntu:26.04` + Clang de apt.llvm.org | Clang 23.1.5, GCC 15.2, CMake 4.2.3, SDL3 3.4.2 | 595 s | 2,9 GB | Pasa todo |

La red de la máquina del autor es lenta: en los runners, la misma instalación tarda
entre 10 y 20 veces menos.

### En los runners de GitHub

| Entorno | Job completo | Instalar el toolchain | Notas |
|---|---|---|---|
| Contenedor `archlinux:latest` (*rolling*), un job | 29–32 s | 11 s | Clang 23.1.4, CMake 4.4.5 |
| Contenedor Arch con el snapshot `2026/10/25`, todos los jobs del ejemplo seguidos | 54 s sin caché, **42–44 s con caché** | 21 s sin caché, **10 s con caché** | La caché es `/var/cache/pacman/pkg`, con `actions/cache` y la fecha del snapshot como clave |
| Imagen precompilada en GHCR (construida y publicada por otro job, 73–99 s) | 32–101 s | — | **Bajar la imagen: 23, 92 y 24 s** en tres corridas |
| Runner `ubuntu-24.04` nativo | 117 s | 15–21 s (dependencias de SDL3) | Clang 18.1.3, GCC 13.3, CMake 3.31.6, glibc 2.39. **Compilar SDL3: 88 s** |

**Lo que importa:**

- Con el snapshot y la caché, un job completo de este ejemplo tarda menos de un minuto, y
  la instalación es la parte fija: ~10 s.
- La imagen precompilada no fue más rápida que instalar con caché, y su tiempo de bajada
  varió 4 veces entre corridas.
- En Ubuntu, la lista de dependencias de SDL3 se mantiene a mano. Con una lista
  incompleta, la configuración falla: el primer intento falló por falta de bibliotecas de
  X11, y se resolvió copiando la lista de `docs/README-linux.md` de SDL3 (30 paquetes).

## 2. Fijar el toolchain (P2)

**La máquina del autor no sigue a Arch directamente.** Usa el mirror estable de Omarchy,
que retiene paquetes. Al 2026-10-28, `archlinux:latest` ya tenía 6 de 12 paquetes del
toolchain más nuevos que la máquina del autor:

| Paquete | Máquina del autor | `archlinux:latest` |
|---|---|---|
| `clang`, `llvm`, `llvm-libs`, `lld`, `compiler-rt` | 23.1.3-4 | 23.1.4-1 |
| `cmake` | 4.4.4-1 | 4.4.5-1 |
| `glibc`, `gcc`, `gcc-libs`, `ninja`, `sdl3`, `gtest` | iguales | iguales |

**El Arch Linux Archive** guarda una foto diaria de los repositorios oficiales, en
`https://archive.archlinux.org/repos/AAAA/MM/DD/`. Para usarla, el `mirrorlist` apunta a
esa fecha y `pacman -Syyuu` baja de versión lo que haga falta. La wiki de Arch advierte que
no se deben mezclar el archivo y los mirrors al día, porque un paquete que falla al bajar
se reemplaza por uno de otra época.

Leyendo la base de datos de paquetes de cada día:

| Snapshot | `clang` | `cmake` | `lld` |
|---|---|---|---|
| 2026/10/22 a 2026/10/25 | 23.1.3-4 | 4.4.4-1 | 23.1.3-4 |
| 2026/10/26 y 2026/10/27 | 23.1.4-1 | 4.4.4-1 | 23.1.4-1 |
| Mirror de Omarchy (hoy) | 23.1.3-4 | 4.4.4-1 | 23.1.3-4 |

**Con el snapshot `2026/10/25`, los 12 paquetes son idénticos a los de la máquina del
autor**, en local y en el runner. Pero ese mismo snapshot tiene Blender 5.2.3, y el autor
tiene 5.2.2: una fecha fija el toolchain, pero no garantiza que todo coincida. Para
Blender, ver la sección 4.

Cada paquete viejo también está en `https://archive.archlinux.org/packages/<letra>/<nombre>/`,
así que una lista de versiones exactas, como un *lockfile*, es otra salida posible. No se
probó: instalar paquetes sueltos con sus dependencias es justo la mezcla de épocas que la
wiki desaconseja.

## 3. El hash del estado y la libm (P3)

**Qué se midió:** un solo binario, compilado una vez en la máquina del autor
(`-O2 -ffp-contract=off -fno-builtin`), que calcula un hash de los bits de 2^24 resultados
de cada función de `float`. Se corrió en varias glibc, así que lo único que cambia entre
corridas es la libm. En Ubuntu 24.04, el binario no arranca (ver abajo), y se compiló ahí
mismo con las mismas banderas.

| Función | 2.44 (autor, Arch, runner con snapshot) | 2.43 (Ubuntu 26.04) | 2.39 (Ubuntu 24.04, runner nativo) |
|---|---|---|---|
| `sinf`, `cosf`, `expf`, `logf`, `powf`, `sqrtf` | referencia | iguales | iguales |
| `acosf`, `atan2f`, `tanf` | referencia | iguales | **distintos** |
| Hash del estado del ejemplo | `6885c67f0c696b6b` | `6885c67f0c696b6b` | **`7cd64e634ac6c9ac`** |

- **En el runner de GitHub con el snapshot, el hash del estado y los 9 hashes de libm son
  idénticos a los de la máquina del autor**, con otra CPU. Con la misma glibc, el hash es
  estable entre estas dos máquinas.
- **Versiones de símbolo:** el binario enlazado con glibc 2.44 pide `acosf`, `atan2f` y
  `sqrtf` con la versión `GLIBC_2.43` (`sinf`, `cosf` y `tanf` siguen en `GLIBC_2.2.5`;
  `expf`, `logf` y `powf`, en `GLIBC_2.27`). En glibc 2.39 dice ``version `GLIBC_2.43' not
  found`` y no arranca. glibc le da una versión nueva a un símbolo cuando cambia su
  comportamiento: un binario viejo sigue recibiendo la función vieja, y uno nuevo exige la
  glibc nueva. Esto importa para la CD (sección 8).

## 4. Las herramientas fuera del toolchain (P4)

| Herramienta | Del repo de Arch | Binario oficial | En el runner |
|---|---|---|---|
| **Blender 5.2 LTS** | 5.2.3 en el snapshot, con 263 paquetes y **854 MiB** de descarga | Tarball de 366 MiB por versión de parche, con su `.sha256` en `download.blender.org` | 30 s sin caché y **8 s con caché** (Blender + D2, verificados) |
| **D2** (ADR-0030) | 0.9.0, 18 MiB | `d2-v0.9.0-linux-amd64.tar.gz` con `SHA256SUMS` en GitHub Releases | Incluido en la fila de arriba |
| **lavapipe** (ADR-0011) | `vulkan-swrast`: 14 MiB y 8 paquetes, con `vulkan-tools` y las validation layers | — | Solo en local: **Vulkan 1.4, `llvmpipe (LLVM 23.1.3, 256 bits)`, sin GPU y sin pantalla**, y las validation layers cargan. Instalación: 15 s |
| **`reuse lint`** (ADR-0019) | `reuse` 6.2.0 | — | 1,2 s sobre el repo real: 75 de 75 archivos con licencia |

**Un detalle del checkout:** en un contenedor sin `git`, `actions/checkout` baja un
tarball sin `.git`. `reuse lint` y cualquier herramienta que use `git ls-files` ven
entonces también los archivos generados. El toolchain se instala antes del checkout.

## 5. El tiempo y las instrucciones en un runner compartido (P5)

**Qué se midió:** el ejecutable del ejemplo en `relwithdebinfo`, 10 veces seguidas, y 3
veces bajo `valgrind --tool=cachegrind --cache-sim=no`, en el job del snapshot de las
tres corridas del spike.

| Corrida | Tiempo (10 ejecuciones) | Instrucciones (cachegrind, 3 ejecuciones) |
|---|---|---|
| 1 | 34 ms las 10 | 330 285 541 las 3 |
| 2 | 28 ms (9) y 29 ms (1) | 330 285 541 las 3 |
| 3 | 28 ms las 10 | 330 285 541 las 3 |

- **Dentro de un job**, el tiempo casi no varía.
- **Entre corridas** varía un 21 %, sin cambiar el código: cada corrida puede caer en
  otra máquina.
- **El conteo de instrucciones es idéntico** en las 9 ejecuciones.

La muestra es chica y el programa es corto. Alcanza para mostrar la diferencia de
naturaleza entre las dos medidas, no para fijar un umbral.

## 6. Chequeos obligatorios y filtros de ruta (P6)

**Qué se midió:** un ruleset temporal sobre una rama base del spike (nunca sobre `main`)
y un PR en borrador hacia ella, con dos workflows:

- `filtrado`: un workflow con `paths: ["spike51/sample/**"]`;
- un agregador: un job `cambios` decide con `git diff` si el PR toca código, `build` corre
  solo si hace falta, y **`ci-ok`** corre siempre (`if: always()`) y falla si algún job
  del que depende falló o se canceló.

```yaml
ci-ok:
  needs: [cambios, build]
  if: always()
  runs-on: ubuntu-24.04
  env:
    RESULTADOS: ${{ join(needs.*.result, ' ') }}
  steps:
    - run: |
        for r in $RESULTADOS; do
          case "$r" in success|skipped) ;; *) exit 1 ;; esac
        done
```

| Caso | Chequeos obligatorios | Qué pasó | Estado del PR |
|---|---|---|---|
| Solo docs | `filtrado` y `ci-ok` | `filtrado` no corre y nunca se reporta. `cambios` pasa, `build` se salta, `ci-ok` pasa | **BLOCKED**, esperando a `filtrado` para siempre |
| Solo docs | `ci-ok` | Igual | CLEAN |
| Con código | `ci-ok` | Corren los cuatro y pasan | CLEAN |
| Con código y `build` fallando | `ci-ok` | `build` falla, `ci-ok` falla | **BLOCKED** |

Coincide con la documentación de GitHub: un workflow saltado por un filtro de ruta deja
sus chequeos en "Pending", y un job saltado por una condición se reporta como éxito.

## 7. Lo que el ejemplo enseñó

- **Warnings globales y dependencias descargadas.** El primer intento puso las banderas
  con `add_compile_options`, que es global. Con SDL3 del sistema no se notó. Con SDL3
  descargado por `FetchContent`, `-Werror`, `-Wconversion` y `-fno-rtti` llegaron al
  código C de SDL3 y el build falló. Se corrigió con un target de interfaz (`mz_flags`)
  que solo enlazan los targets propios.
- **Los sanitizers funcionan en contenedores.** ASan, UBSan y LeakSanitizer funcionaron en
  Docker en local y en los contenedores de los runners, sin opciones extra.
- **`actionlint`** (un linter de workflows de GitHub Actions) no encontró errores de
  estructura en los tres workflows del spike, solo avisos de estilo de shellcheck.
- **El formato y `-Werror` atraparon errores del propio ejemplo** en la primera corrida
  local: una variable que se asignaba y no se usaba, y líneas sin formatear.

## Cómo repetirlo

1. Un proyecto mínimo con CMake y presets, un test de GoogleTest que enlace SDL3 y un
   script `ci.sh <job>` que llame a `cmake --workflow --preset <job>`.
2. Un `Dockerfile` desde `archlinux:latest` que, si recibe `ARCH_DATE`, escribe
   `Server = https://archive.archlinux.org/repos/${ARCH_DATE}/$repo/os/$arch` en
   `/etc/pacman.d/mirrorlist` y corre `pacman -Syyuu` con el toolchain.
3. Comparar `pacman -Q <paquetes>` de la máquina y del contenedor con `diff`.
4. Para la libm: compilar una vez un programa que haga hash de los bits de cada función
   sobre 2^24 entradas, y correr ese mismo binario en cada contenedor
   (`docker run -v $PWD:/h:ro <imagen> /h/probe`). `objdump -T` muestra las versiones de
   símbolo que pide.
5. En GitHub: una rama `spike/<N>-…` con los workflows. Las duraciones por paso salen de
   `gh api repos/<repo>/actions/runs/<id>/jobs`. Para los chequeos obligatorios, un
   ruleset temporal sobre una rama base del spike, nunca sobre `main`.
