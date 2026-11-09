# Prototipo de CD y distribución

Nota de investigación del [ADR-0035](../adr/0035-cd-y-distribucion.md).
Guarda las mediciones que sostienen la decisión, para poder repetirlas.

Fecha: 2026-11-09, en la máquina del autor (la de referencia del ADR-0020: Dell XPS 13
9360, i7-8550U, Intel UHD 620, Arch con glibc 2.44). Son mediciones de laboratorio para
decidir. Los tiempos son orientativos: nada de este ADR depende de un milisegundo. El
prototipo es código descartable, fuera del repo.

El autor eligió medir solo tres cosas: **P1** (un binario de Linux que arranque en otras
distros), **P2** (Windows cruzado desde Linux) y **P3** (builds reproducibles). macOS
quedó fuera del ADR, y la firma en Windows, SmartScreen, los antivirus, itch.io y los
secretos se investigaron en papel, sin subir nada a ningún servicio.

| Qué | Versión |
|---|---|
| Host | Arch, glibc 2.44, Clang 23.1.3, GCC 16.2.1, CMake 4.4.4 |
| SDK de *sniper* | Steam Linux Runtime 3.0, `BUILD_ID 3.0.20261021.266025`: glibc 2.31, GCC 14.2, Clang 11, CMake 3.25. Imagen `sha256:bb30053d…` |
| llvm-mingw | Clang 23.1.5, `lld`, libc++, UCRT. Imagen `sha256:b2a697af…` (2026-11-04) |
| Wine | 11.21 (Arch), con lavapipe de Mesa 26.2.6 |
| SDL3 | 3.4.18, desde su tarball oficial (`sha256 9c75cf16…`) |
| GoogleTest | 1.17.0 |
| appimagetool | 1.9.1, con el *runtime* type2 `20251108` |

**Todo corrió en contenedores de Docker:** el autor no instaló nada. Las distros "de
jugador" son imágenes oficiales con el loader de Vulkan y lavapipe (la GPU por software
de Mesa): Arch, Ubuntu 24.04 (glibc 2.39), Debian 12 (2.36) y Debian 11 (2.31, ya en
`archive.debian.org`).

## El prototipo

Un programa chico en C++20 con SDL3, que hace lo que el juego va a hacer al arrancar en
la máquina de otra persona:

- **`plataforma.cpp`:** abre **su propio ejecutable** (`/proc/self/exe` en Linux;
  `GetModuleFileNameW` y `CreateFileW` en Windows), lee en un offset (`pread`; `ReadFile`
  con `OVERLAPPED`) y **encuentra una sección por nombre** leyendo los encabezados de ELF
  o de PE. Es lo que el ADR-0028 dejó abierto para PE.
- **`pak.S`:** mete un paquete falso de **64 MB** en la sección `.mzpak` con `.incbin`,
  como el paquete de Release del ADR-0028. El paquete lo genera una herramienta aparte
  (`gen_pak`), una sola vez, y todas las builds enlazan los mismos bytes.
- **`sondas.cpp`:** lee la sección entera y compara su hash; escribe y lee una tarjeta de
  memoria de 8 KB en `SDL_GetPrefPath`; carga Vulkan en runtime y lista las features que
  verifica el engine (ADR-0005 y ADR-0022); y calcula dos hashes: uno con `sin`, `cos`,
  `atan2`, `exp` y `pow` (la libm) y otro solo con `+ - * /` y `sqrt` (exactos en IEEE
  754).
- **`tests.cpp`:** cinco tests de GoogleTest: la sección, la tarjeta, los dos hashes
  contra los valores grabados en la máquina del autor, y el hash dos veces en el mismo
  proceso (ADR-0016).

## P1. Un binario de Linux que arranque en otras distros

### Qué versión de glibc pide cada build

| Build | Toolchain | Depende de | glibc más nueva que pide |
|---|---|---|---|
| `host-sys` | Clang 23 del host, SDL3 del sistema (lo que dice hoy el ADR-0006) | `libSDL3.so.0`, `libstdc++`, `libm`, `libc` | **2.43** |
| `host-est` | Clang 23 del host, SDL3 estática, `-static-libstdc++ -static-libgcc` | `libm`, `libc` | **2.43** (más 2.32–2.38) |
| `sniper` | GCC 14.2 del SDK, SDL3 estática, `libstdc++` estática | `libm`, `libc`, `libdl`, `libpthread` | **2.29** |

**Lo que pide 2.43 son funciones de `float` de la libm** (`sqrtf`, `atan2f`, `asinf`,
`acosf`, `log10f`). La glibc 2.43 movió su manejo de errores SVID a símbolos de
compatibilidad, y con eso les dio una versión nueva; cualquier programa enlazado contra
2.43 que las use pide 2.43. Las de 2.32–2.38 vienen de SDL3 (`pthread`, `dlopen`,
`arc4random`, `strlcpy`) y de los `__isoc23_*` que la glibc 2.38 usa para C23.

### Dónde arranca cada una

| Build | Arch (2.44) | Ubuntu 24.04 (2.39) | Debian 12 (2.36) | Debian 11 (2.31) |
|---|---|---|---|---|
| `host-sys` | ❌ falta `libSDL3.so.0` | ❌ falta `libSDL3.so.0` | ❌ | ❌ |
| `host-est` | ✅ | ❌ pide `GLIBC_2.43` | ❌ pide 2.38 y 2.43 | ❌ pide 2.32–2.43 |
| `sniper` | ✅ | ✅ | ✅ | ✅ |

- **Ubuntu 24.04 no tiene paquete de SDL3:** un juego que enlaza la SDL3 del sistema no
  arranca en la distro más usada. Ni siquiera en un Arch limpio, si el jugador no la
  instaló. El juego publicado tiene que llevar SDL3 adentro (estática, o la `.so` al lado).
- **El binario de *sniper* arranca en las cuatro**, con el paquete, la tarjeta de memoria
  y Vulkan OK. Compilado con GCC 14.2: el SDK no tiene un Clang moderno ni `lld`.
- **SDL3 3.4.18 viene en el SDK** (`.so` y `.a`), la misma versión de la máquina del
  autor.

### Lo que cambia según la distro: la libm y Vulkan

**El mismo binario de *sniper*** dio:

| Distro | Hash de la libm | Hash aritmético | Vulkan (lavapipe) |
|---|---|---|---|
| Arch (glibc 2.44) | `eccb5cc7…` | `fac35bab…` | 1.4, todas las features |
| Ubuntu 24.04 (2.39) | `1f4e3953…` | `fac35bab…` | 1.4, todas las features |
| Debian 12 (2.36) | `1f4e3953…` | `fac35bab…` | 1.3, **sin *descriptor indexing*** |
| Debian 11 (2.31) | `d92ca4fa…` | `fac35bab…` | **1.0**, ninguna |

- **El hash de la libm depende de la glibc del sistema donde corre, no de dónde se
  compiló:** la libm es una biblioteca compartida que pone el sistema. La glibc 2.41
  cambió muchas funciones de `float` por versiones **correctamente redondeadas** del
  proyecto CORE-MATH (NEWS de 2.41), y eso mueve los resultados. Un hash guardado del
  *replay* solo vale en la glibc donde se grabó, como ya dicen el ADR-0016, el ADR-0021 y
  el ADR-0029.
- **El hash aritmético da igual en todos lados,** también entre Clang 23 y GCC 14, con
  `-ffp-contract=off` (ADR-0008).
- **Las features de Vulkan dependen del driver del jugador.** El engine ya las verifica al
  iniciar (ADR-0022); este prototipo muestra que el mensaje cuando faltan es parte de la
  distribución.

### El formato del paquete

| Formato | Tamaño | ¿Conserva el bit de ejecución? | Otros |
|---|---|---|---|
| Binario suelto (adjunto a la release) | 72 MB | **No:** HTTP no lleva permisos; el jugador hace `chmod +x` | — |
| `.tar.gz` | 35,8 MB | **Sí**, con `tar` y con libarchive | — |
| `.zip` hecho en Linux | 35,8 MB | Sí con `unzip` (Info-ZIP) y libarchive; **no** con `zipfile` de Python | — |
| `.zip` sin atributos de Unix (hecho en Windows) | — | **No**, con ninguna herramienta | — |
| AppImage (zstd) | 36,3 MB | Sí (es un solo archivo, pero también se descarga sin `+x`) | Necesita FUSE |

**AppImage, medido en Ubuntu 24.04:**

- **Sin FUSE no arranca:** "Cannot mount AppImage, please check your FUSE setup".
- Con `APPIMAGE_EXTRACT_AND_RUN=1` extrae todo a `/tmp` **en cada arranque: 2,7 s**.
- Con FUSE 3, el programa entero tarda 226 ms, contra 152–199 ms del binario suelto. La
  sección se encuentra igual a través del montaje.
- Como el ADR-0028 ya hace que el juego sea **un solo ejecutable**, AppImage solo suma
  el ícono y la entrada de escritorio, a cambio de exigir FUSE.

**El paquete sintético se comprime a la mitad** (72 → 36 MB), porque la mitad es
aleatoria. Los assets cocinados reales pueden comprimir distinto.

## P2. Windows cruzado desde Linux

### El build

- **llvm-mingw** (Clang 23.1.5, `lld`, libc++, UCRT) compila el mismo código con un
  *toolchain file* de CMake de 12 líneas, en 75 s (SDL3, GoogleTest y el programa).
- **Con `-static`, el `.exe` solo depende de DLLs de Windows y del UCRT**
  (`KERNEL32`, `USER32`, `api-ms-win-crt-*`…). El UCRT viene con Windows 10 y 11: no hay
  que instalar nada, ni repartir `libc++.dll`.
- **La sección `.mzpak` existe en el PE** con el mismo nombre. Para encontrarla, el
  tamaño real es `VirtualSize`, no `SizeOfRawData`, que está redondeado al
  `FileAlignment`. **Los nombres de sección de PE tienen 8 bytes:** uno más largo va a una
  tabla de strings, así que `.mzpak` cabe justo.
- **El `.exe` de Release trae secciones DWARF** (`.debug_info`, `.debug_line`…) de las
  bibliotecas estáticas, y una `.buildid`. Se quitan con `strip` antes de publicar.

### Bajo Wine

| | Resultado |
|---|---|
| Sección del paquete | ✅ encontrada en el offset 3 213 312; 64 MB leídos con `ReadFile` en 80 ms |
| Carpeta de datos | ✅ `C:\users\<usuario>\AppData\Roaming\MedievalZombies\` (`%APPDATA%`). La organización vacía se omite, como en Linux |
| Vulkan | ✅ winevulkan pasa a lavapipe del host: 1.4 y todas las features |
| Hash aritmético | `fac35bab…`, **el mismo** que en Linux |
| Hash de la libm | `34d29ae6…`: **otro**. Es la reimplementación del UCRT que hace Wine, no la de Microsoft, así que tampoco dice cuál sería en Windows de verdad |

### Los tests bajo Wine

- `CMAKE_CROSSCOMPILING_EMULATOR wine` en el *toolchain file* hace que `ctest` y
  `gtest_discover_tests` corran el `.exe` con Wine sin cambiar los tests.
- **Pasan 4 de 5.** Falla, como se esperaba, la comparación contra el hash de la libm
  guardado en la máquina del autor.
- **Configurar, compilar y probar tienen que ir en el mismo entorno:** el descubrimiento de
  GoogleTest guarda la ruta absoluta de su módulo de CMake. Configurado con CMake 3.28 y
  probado con CMake 4.4, no encuentra el módulo. Se armó una sola imagen con llvm-mingw
  y Wine.
- **El primer arranque de Wine pasa el *timeout* de 5 s** del descubrimiento de GoogleTest:
  se subió a 60 s (`DISCOVERY_TIMEOUT`).
- **Cada proceso de Wine cuesta:** con `ctest` serial, 3,5 s por test (21 s los cinco).
  Con `wineserver` vivo o el binario de tests corrido una vez, la suite entera tarda
  3,6 s, de los que 168 ms son los tests.

## P3. Builds reproducibles

**La pregunta:** ¿dos builds del mismo commit dan los mismos bytes? Cada par cambia lo
que cambia entre la máquina del autor y un runner de GitHub: **la ruta del checkout**
(`/d35` contra `/home/runner/work/MedievalZombies/MedievalZombies`) y **la hora** (61 s
de diferencia, para que cambie `__TIME__`). Mismo contenedor, fijado por *digest*. El
programa imprime `__DATE__ " " __TIME__`, como muchos juegos.

| Plataforma | Build | Sin banderas | `SOURCE_DATE_EPOCH` | `SOURCE_DATE_EPOCH` + `-ffile-prefix-map` |
|---|---|---|---|---|
| Linux (*sniper*) | Release | ❌ distintos | ✅ iguales | ✅ iguales (los mismos bytes que solo con la fecha) |
| Windows (llvm-mingw) | Release | ❌ distintos | ✅ iguales | ✅ iguales (con `--no-insert-timestamp`) |
| Linux (*sniper*) | RelWithDebInfo (`-g`) | — | ❌ distintos: **1336 rutas** del checkout en el DWARF | ✅ iguales, **0 rutas** |

- **`SOURCE_DATE_EPOCH`** es la convención de Reproducible Builds: una fecha fija (en la
  CD, la del commit) que GCC y Clang usan para `__DATE__` y `__TIME__`. **`lld` también
  la usa para el `TimeDateStamp` del PE** (medido: quedó en la fecha fijada). Con
  `--no-insert-timestamp` queda en 0.
- **Sin información de depuración, la ruta no entra al binario:** en Release alcanza la
  fecha. **Con `-g`, la ruta entra en cada unidad de compilación**, y hace falta
  `-ffile-prefix-map`.
- **Error encontrado en el camino:** `add_compile_options` solo afecta a los targets que
  se definen **después**. Con las banderas puestas después de `add_subdirectory(SDL3)`,
  SDL3 dejó 1269 rutas en el binario. Las banderas van antes de las dependencias.
- **Rutas que no son del proyecto:** el `.exe` trae 86 rutas `/home/runner/...` que vienen
  de la libc++ de llvm-mingw, compilada en GitHub Actions. Son iguales en todos los
  builds con la misma imagen; por eso **la imagen se fija por *digest*, no por tag**.
- **Reproducible quiere decir "con la misma imagen".** El binario de la máquina del autor
  (Clang 23 de Arch) nunca va a dar los mismos bytes que el de *sniper* (GCC 14.2).

## Lo que el prototipo no prueba

- **El comportamiento de Windows de verdad:** SmartScreen, Defender, el *Mark of the Web*,
  los permisos de `%APPDATA%` y la libm de Microsoft. Wine reimplementa todo eso.
- **Un driver de Vulkan real fuera de la máquina del autor.** lavapipe dice si el loader y
  las features funcionan, no si un driver de AMD, NVIDIA o Intel en Windows dibuja bien.
  Un runner `windows-latest` de GitHub tampoco tiene GPU; lavapipe para Windows existe en
  compilaciones de terceros (mesa-dist-win).
- **Los antivirus.** No se subió nada a VirusTotal (decisión del autor).

## Cómo repetirlo

```sh
# en ~/.claude-grass/jobs/11a3076a/tmp/d35 (fuera del repo), con Docker
clang++ -O2 -std=c++20 tools/gen_pak.cpp -o bin/gen_pak && ./bin/gen_pak 64 pak/pak.bin pak/pak_hash.h
# imágenes: imagenes/Dockerfile.distro (BASE=archlinux|ubuntu:24.04|debian:12),
#           Dockerfile.debian11, Dockerfile.wine, Dockerfile.mingw-wine
# P1: compilar en sniper (ver b-sniper.log) y luego
./p1_correr.sh host-sys host-est sniper
docker run --rm -v $PWD:/d35:ro d35-arch bash /d35/p1_permisos.sh
docker run --rm -v $PWD:/d35:ro -v $PWD/out:/out d35-arch bash /d35/p1_appimage.sh
# P2: configurar, compilar y ctest dentro de d35-mingw-wine, con -DCMAKE_TOOLCHAIN_FILE=src/mingw.cmake
# P3:
./p3_repro.sh
```
