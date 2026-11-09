# ADR-0035: CD y distribución

- **Estado:** Aceptado
- **Fecha:** 2026-11-09
- **Ticket:** #82
- **Reemplaza / Reemplazado por:** — (reemplaza la Decisión 10 del ADR-0029, con fe de
  erratas allá)
- **Fe de erratas:** 2026-11-09 (#75): el ADR-0033 adelanta la CD al **esqueleto
  andante (M3, tag `v0.1.0`)**: el esqueleto también se despliega, y el riesgo de
  Windows se ve en el primer milestone. **La tabla propia de funciones de Vulkan sale del
  ticket 3** y va a la base de Vulkan del esqueleto, porque sin ella no se dibuja el
  primer píxel. Se corrige en la Decisión 1 y en "Tickets, en orden".

## Contexto y problema

El ADR-0029 quedó, en la práctica, como el ADR de la CI: de sus 10 dimensiones, solo la
10 es de CD, y dice poco. **Un binario de Linux compilado en el Steam Linux Runtime
(*sniper*) en cada tag de corte vertical, sin firma, "si se logra".** Falta decidir cómo
se construyen, se prueban, se firman y se publican las releases, y sobre todo **cómo
logra el juego que el sistema operativo del jugador confíe en él**, lo más cerca posible
de "de inmediato".

Varios ADRs ya aceptados dejaron preguntas para este:

| ADR | Qué dejó para este |
|---|---|
| Visión | "Para el corte vertical no hacen falta una build para Windows ni un tutorial" |
| ADR-0006 | SDL3 del sistema por defecto; Clang como compilador principal y `lld`; solo Linux |
| ADR-0014 | Un tag por corte vertical, SemVer `0.y.z` y un `CHANGELOG.md` de git-cliff. La CD, si se logra, publica en GitHub |
| ADR-0016 y ADR-0021 | El hash del *replay* solo se compara contra uno guardado con la misma glibc |
| ADR-0019 | Código MIT; assets CC0, CC BY 4.0 y fuentes OFL 1.1, acreditados en `CREDITS.md` |
| ADR-0022 | Las features de Vulkan que el engine verifica al iniciar |
| ADR-0028 | **El paquete de assets va dentro del ejecutable** en Release, leído con `pread`. Abiertos: de dónde sale el paquete en la CD, cómo lo encuentra `platform::` en PE (y Mach-O), y las carpetas de cada sistema |
| ADR-0029 | La D10 (la CD en *sniper*) y la D9 (**ningún secreto**) |
| ADR-0032 | El modo de latencia D se midió solo en Hyprland: en Windows queda abierto |

**Lo que el autor ya fijó al abrir el ticket y al aprobar las dimensiones:**

- **A corto plazo se publica solo en GitHub Releases y en itch.io**, no en Steam.
- **Linux y Windows desde el primer tag.** Esto cambia la visión, que no pedía Windows
  para el corte vertical.
- **macOS queda fuera, como opción futura.**
- El enfoque principal es **que el sistema confíe en el ejecutable**.
- De los prototipos, solo P1, P2 y P3. La firma, SmartScreen, los antivirus e itch.io se
  investigan en papel, **sin subir nada a ningún servicio**.

Hay que decidir:

1. **las plataformas** y desde cuándo;
2. **el toolchain** de cada plataforma;
3. **el formato del paquete** que descarga el jugador, y las carpetas de cada sistema;
4. **la confianza en Windows:** la firma, SmartScreen y los antivirus;
5. **qué queda dicho para macOS;**
6. **la integridad y la procedencia:** *checksums*, atestaciones y builds reproducibles;
7. **los canales y el *staging*;**
8. **los secretos;**
9. **qué se prueba y dónde**, sin máquinas de Windows;
10. **el workflow de la CD.**

Lo que ya está decidido y limita este ADR:

- **ADR-0008:** x86-64 base, sin `-march`, sin `-ffast-math`, con `-ffp-contract=off`.
- **ADR-0011:** GoogleTest y CTest; TDD estricto en el núcleo.
- **ADR-0014:** todo entra por PR; un tag por corte vertical.
- **ADR-0028:** un solo ejecutable con el paquete adentro; la tarjeta de memoria en
  `SDL_GetPrefPath`; las opciones, la caché y el estado en las carpetas XDG.
- **ADR-0029:** la CI en un contenedor de Arch con fecha fija; las actions fijadas por
  SHA; nunca `pull_request_target`; un valor del evento nunca va directo a un `run:`.
- **`CLAUDE.md`:** todo gratis; cada dependencia externa necesita un ADR.

**Fuera de alcance:**

| Qué | Lo decide |
|---|---|
| Steam (Steamworks, depots, el *runtime* de Steam como contenedor) | Un ADR futuro, si el juego llega a Steam |
| La página de itch.io (textos, imágenes, precio) | El autor, fuera del proyecto |
| macOS (Vulkan sobre Metal, firma y notarización) | Un ADR futuro (ver la dimensión 5) |
| Qué verifica cada test | ADR-0011 |
| La CI de los PRs | ADR-0029, salvo lo que cambia por fe de erratas |

## Criterios

Los ocho del ticket, más uno:

1. **Confianza del sistema en el ejecutable,** lo más cerca posible de inmediata.
2. **Todo gratis** (`CLAUDE.md`).
3. **Secretos seguros** en un repo público.
4. **Reproducibilidad:** el mismo commit da los mismos bytes.
5. **Se puede probar sin máquinas de Windows ni de macOS:** el autor solo tiene Linux.
6. **Lo mantiene una sola persona.**
7. **Aprendizaje.**
8. **Estándar de la industria.**
9. **Lo que se publica es lo que se probó:** el binario que sube es el mismo que pasó las
   pruebas, no uno recompilado después.

## Opciones

### 1. Las plataformas

| Opción | Qué implica |
|---|---|
| **Solo Linux** | Lo de la D10 del ADR-0029 |
| **Linux y Windows desde el primer tag** | Lo que fijó el autor. Un segundo toolchain, un segundo formato de ejecutable y un sistema que el autor no tiene para probar |
| **Linux, Windows y macOS** | Además, Vulkan sobre Metal (MoltenVK o KosmicKrisp, una dependencia nueva) y la notarización. Descartada por el autor |

### 2. El toolchain

**Linux:**

| Opción | A favor | En contra |
|---|---|---|
| **El toolchain del host** (Arch, Clang 23) | El mismo de la CI y del autor | **Medido:** pide glibc 2.43; solo arranca en distros al día |
| **El SDK de *sniper*** (glibc 2.31, GCC 14.2) | **Medido:** arranca en Arch, Ubuntu 24.04, Debian 12 y Debian 11. Lo recomienda Valve para juegos nuevos de Linux | Compila con GCC, el compilador secundario del ADR-0006: no tiene un Clang moderno ni `lld` |
| **Un SDK propio** (un GCC o Clang nuevo contra una glibc vieja, como hace Godot) | Elige el compilador | Hay que construirlo y mantenerlo |

**SDL3:** del sistema (lo que dice hoy el ADR-0006), **estática** desde su tarball con la
versión fijada, o la `.so` al lado del ejecutable con `RPATH=$ORIGIN`.

**Windows:**

| Opción | A favor | En contra |
|---|---|---|
| **llvm-mingw, cruzando desde Linux** | **Medido:** compila el engine con un *toolchain file* de 12 líneas. Clang 23, como el principal del ADR-0006, y `lld`. Corre en un contenedor de Linux, como la CI, con Wine para los tests | ABI de MinGW (Itanium C++), no la de MSVC. libc++ en vez de la STL de Microsoft. Las PDB se piden aparte (`-gcodeview`) |
| **`clang-cl` en un runner de Windows** | ABI y CRT de MSVC; las herramientas de depuración de Windows funcionan directo | Un runner que el autor no puede reproducir en su máquina (contra la D6 del ADR-0029). Necesita el SDK de Windows |
| **MSVC en un runner de Windows** | El estándar de los estudios en Windows | Un tercer compilador, con otro dialecto de banderas y otros warnings que `-Werror` tendría que cubrir |
| **`clang-cl` cruzando con el SDK de Windows bajado a Linux** (`xwin`) | ABI de MSVC desde Linux | La licencia del SDK de Microsoft no permite redistribuirlo, y bajarlo en cada build es frágil |

### 3. El formato del paquete

| Opción | Linux | Windows |
|---|---|---|
| **Binario suelto** | Se descarga sin el bit de ejecución (medido: HTTP no lleva permisos) | El `.exe` solo |
| **Archivo comprimido** | `.tar.gz` (medido: conserva `+x` siempre) | `.zip` |
| **Formato de distro** | AppImage (medido: necesita FUSE; sin él, extrae 2,7 s en cada arranque) o Flatpak (Flathub compila desde la fuente en su infraestructura y revisa los permisos del *sandbox*) | — |
| **Instalador** | — | MSI, NSIS o Inno Setup |

**Además:** si se le quitan los símbolos (`strip`), qué archivos van al lado del
ejecutable (`LICENSE`, `CREDITS.md`, un léeme), el ícono y los metadatos de versión de
Windows, y **las carpetas de cada sistema**: en Linux las fijó el ADR-0028 (XDG); en
Windows, `SDL_GetPrefPath` da `%APPDATA%\<juego>\` (medido bajo Wine), y la caché y los
registros pueden ir ahí, en `%LOCALAPPDATA%` o en la carpeta *Saved Games*.

### 4. La confianza en Windows

**SmartScreen** (documentación de Microsoft, actualizada en agosto de 2026) mira dos
señales: **la reputación del editor** (el certificado) y **la del hash del archivo**.

- **Sin firma o con firma propia (autofirmado):** "Windows protected your PC", y el jugador
  tiene que elegir "Run anyway". **Cada versión empieza de cero.**
- **Con un certificado válido (OV o EV):** también avisa al principio, pero muestra el
  nombre del editor, y **la reputación del certificado pasa a las versiones siguientes**.
- **EV ya no da reputación inmediata.**
- **No hay forma de pedirla:** se gana con descargas, "semanas y cientos de instalaciones
  limpias".
- **En Windows 11, Smart App Control** (si está activo) **bloquea** los archivos sin firma
  que no tienen reputación, y no solo los descargados.

| Opción de firma | Costo | Condiciones |
|---|---|---|
| **Sin firma** | 0 | El aviso en cada versión, sin acumular reputación |
| **SignPath Foundation** | 0 | Licencia aprobada por la OSI "para todos los componentes", sin componentes propietarios; build automatizado y verificable; **el proyecto ya publicado** en la forma que se firma; aprobación manual de cada release; MFA; una página de política de firma en el sitio del proyecto. **El editor que ve el jugador es "SignPath Foundation"**, no el autor |
| **Azure Artifact Signing** (antes Trusted Signing) | USD 9,99 al mes | Personas solo en Estados Unidos o Canadá |
| **Certum Open Source** | desde EUR 69 | Con lector de tarjeta |
| **Certificado OV o EV comercial** | USD 150–400+ al año | — |

**Los antivirus** marcan sobre todo por heurísticas. Detecciones como
`Trojan:Win32/Wacatac…!ml` vienen de modelos de *machine learning*. Lo que más se
repite en los reportes es: **los binarios sin firma**, los **empaquetadores** tipo UPX,
el código que se modifica a sí mismo y los **datos pegados al final** del PE (el
*overlay*). Que un antivirus marque un `.exe` sin firma es frecuente e intermitente.

**Opciones para el falso positivo:**

- reportarlo a Microsoft en el portal de Security Intelligence (WDSI), con la persona
  "Software developer";
- revisar antes en VirusTotal: el archivo se comparte con unos 70 proveedores, y
  conviene solo con lo que ya es público.

### 5. macOS

Fuera del ADR por decisión del autor. Para el día en que entre, queda escrito lo que hace
falta: un ADR de Vulkan sobre Metal (MoltenVK o KosmicKrisp) que verifique las features
del ADR-0022; firmar con Developer ID y notarizar (USD 99 al año, contra "todo gratis"),
o firmar *ad-hoc* y explicarle al jugador cómo abrirlo; y encontrar la sección del
paquete en Mach-O.

### 6. La integridad y la procedencia

| Mecanismo | Qué prueba | Costo |
|---|---|---|
| **`SHA256SUMS`** en la release | Que el archivo llegó entero | Nada |
| **Atestación de procedencia** de GitHub (ya en el ADR-0029) | Qué workflow y qué commit lo construyeron (SLSA Build nivel 2, Sigstore) | Un *step*; `gh attestation verify` |
| **Releases inmutables** de GitHub (disponibles desde octubre de 2025) | Que nadie cambió los archivos ni movió el tag después de publicar: GitHub firma una atestación de la release (`gh release verify`) | Una opción del repo. Un archivo mal subido no se reemplaza: se publica otra versión |
| **Builds reproducibles** | Que cualquiera puede recompilar el commit y obtener los mismos bytes | `SOURCE_DATE_EPOCH`, `-ffile-prefix-map`, imágenes fijadas por *digest* (ver P3) |
| **Firmar la lista de *checksums*** (minisign, GPG) | Lo mismo que la atestación, con una llave propia | Una llave más que cuidar |

### 7. Los canales y el *staging*

**Cómo lo hacen los estudios:** un build interno (el equipo), uno de prueba (QA o
*playtest*), una beta (jugadores invitados) y el público. Las plataformas lo dan con
ramas o canales (las *branches* de Steam).

**Lo que hay gratis para este proyecto:**

| Mecanismo | Qué hace |
|---|---|
| **Release en borrador** (*draft*) de GitHub | Solo la ven quienes tienen escritura en el repo. Se publica a mano |
| ***Pre-release*** de GitHub | Pública, pero marcada como no estable |
| **Canales de `butler`** en itch.io | `butler push <carpeta> usuario/juego:canal`. El nombre del canal etiqueta la plataforma (`linux`, `windows`). `--userversion` lleva la versión. Sube parches (estilo rsync) |
| **Canal oculto** de itch.io (`--hidden`) | Un canal nuevo que no aparece en la página hasta mostrarlo |

### 8. Los secretos

La D9 del ADR-0029 dice "ningún secreto". **Subir a itch.io pide uno:** `BUTLER_API_KEY`.
Firmar con SignPath pediría otro.

| Mecanismo | Qué da en un repo público |
|---|---|
| **Secretos del repo** | Los ve cualquier workflow del repo |
| **Entornos con revisores obligatorios** | Un job que usa el entorno **no ve sus secretos hasta que un revisor lo aprueba**. Se puede limitar a ciertos tags. Hay una opción para impedir que quien lanzó el job lo apruebe; con una sola persona, no se activa |
| **Reglas de tags** (*rulesets*) | Solo el dueño del repo crea o mueve tags `v*` |
| **OIDC** | Sin secreto guardado; solo si el servicio lo acepta (itch.io no) |

### 9. Qué se prueba y dónde

| Dónde | Qué prueba | Qué no prueba |
|---|---|---|
| **Contenedores de distros** (Ubuntu LTS, Debian estable y viejo) | **Medido:** que el binario arranca, encuentra su paquete, escribe la tarjeta y ve Vulkan con lavapipe | Un driver de GPU real |
| **Wine** (en la misma imagen de llvm-mingw) | **Medido:** los tests de GoogleTest y el ejecutable, con lavapipe a través de winevulkan | La libm, el UCRT y el sistema de archivos de Microsoft; SmartScreen y Defender |
| **Runner `windows-latest` de GitHub** | Windows de verdad: el UCRT, las rutas, `%APPDATA%`, Defender | Sin GPU (lavapipe para Windows existe en compilaciones de terceros) |
| **Máquinas de jugadores** | Todo | No es automático |

**El hash del *replay* contra uno guardado no sirve fuera de la glibc del autor:**
**medido**, el mismo binario da tres hashes de libm distintos en cuatro distros, y otro
bajo Wine. El hash con aritmética IEEE da igual en todos lados.

### 10. El workflow de la CD

| Pregunta | Opciones |
|---|---|
| **Qué lo dispara** | Un tag `v*` (ADR-0014); o a mano (`workflow_dispatch`) |
| **De dónde sale el paquete cocinado** (abierto en el ADR-0028) | Cocinarlo una vez (con Blender, en el contenedor de la CI) y pasarlo como artefacto a cada plataforma; o cocinarlo en cada build |
| **Qué se prueba antes de publicar** | Los tests de cada plataforma; una prueba de humo del **ejecutable final** en los sistemas de la dimensión 9; una segunda compilación que compara los bytes |
| **Si una release sale mal** | Con releases inmutables no se reemplaza: se publica la siguiente versión de parche y se marca la mala |

## Investigación

### Prototipo medido

Las mediciones completas están en la
[nota de investigación](../research/cd-y-distribucion.md). Lo principal:

| Prueba | Resultado |
|---|---|
| P1: el binario del host (Arch, Clang 23) | **Pide glibc 2.43**: no arranca en Ubuntu 24.04 ni en Debian. Con la SDL3 del sistema, **no arranca ni en un Arch limpio** (Ubuntu 24.04 no tiene paquete de SDL3) |
| P1: el binario de *sniper* (GCC 14.2, SDL3 y `libstdc++` estáticas) | Pide glibc 2.29. **Arranca en Arch, Ubuntu 24.04, Debian 12 y Debian 11**: paquete, tarjeta y Vulkan OK |
| P1: la libm | **El mismo binario da tres hashes distintos en cuatro distros** (depende de la glibc donde corre). El hash aritmético es igual en todas, con Clang y con GCC |
| P1: el formato | `.tar.gz` conserva `+x` siempre; un `.zip` depende de quién lo extrae; AppImage necesita FUSE (2,7 s por arranque sin él) |
| P2: Windows con llvm-mingw | Compila igual; el `.exe` solo depende de Windows y del UCRT. La sección se encuentra por `VirtualSize` en la tabla de PE |
| P2: bajo Wine | Paquete, `%APPDATA%\MedievalZombies\`, Vulkan con lavapipe: OK. 4 de 5 tests; falla solo el hash de la libm guardado. Configurar, compilar y probar en la misma imagen |
| P3: reproducibilidad | Sin banderas, dos builds del mismo commit (otra ruta, otra hora) dan bytes distintos. **Con `SOURCE_DATE_EPOCH`, iguales** en Linux y Windows (`lld` la usa para el *timestamp* del PE). Con `-g` entran 1336 rutas del checkout: `-ffile-prefix-map` las saca, **puesto antes de las dependencias** en CMake. Reproducible solo con la misma imagen, fijada por *digest* |

### Cómo lo hacen otros

| Quién | Qué hace |
|---|---|
| **Valve**, Steam Linux Runtime | Recomienda compilar los juegos nuevos de Linux en el SDK de *sniper*. La glibc la pone siempre el sistema: el *runtime* no la puede traer |
| **Microsoft**, SmartScreen | Reputación por editor y por hash; EV ya no la da inmediata; recomienda firmar cada release con la misma identidad y avisar a los primeros jugadores |
| **SignPath Foundation** | Firma gratis a proyectos open source (pandoc, Ink/Stitch, entre otros), con su nombre como editor y una página de política de firma en cada proyecto |
| **GitHub** | Atestaciones de procedencia (SLSA nivel 2) y releases inmutables con atestación de la release |
| **itch.io**, `butler` | Canales por plataforma, parches por diferencia, `BUTLER_API_KEY` en la CI |
| **Reproducible Builds** | `SOURCE_DATE_EPOCH` como fecha del build; rutas normalizadas con `-ffile-prefix-map` |

## Decisión

Publico para **Linux y Windows desde el primer tag**, en GitHub Releases y en itch.io.
Cada binario sale de un contenedor fijado por *digest*: el de Linux, del SDK de *sniper*,
que es lo único que medí arrancando en cuatro distros; el de Windows, de llvm-mingw,
cruzando desde Linux, para que cualquier build de la CD se repita en mi máquina con un
comando. Lo que publico es lo que probé: la CD arma un borrador, lo pruebo, y al
publicarlo suben esos mismos archivos a itch.io. No hay firma gratis que dé confianza
inmediata en Windows, así que por ahora publico sin firma, con lo que sí está en mi
mano: integridad, procedencia y builds reproducibles. La firma vuelve cuando haya un
juego jugable. macOS queda para el futuro.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Plataformas | **Linux x86-64 y Windows x86-64 desde el primer tag de corte vertical** (fe de erratas, ADR-0033: desde el tag del esqueleto andante, `v0.1.0`). Mínimos declarados: **glibc 2.31 o más nueva** (la de *sniper*), **Windows 10 o 11 de 64 bits**, y un driver de **Vulkan 1.3** con las features del ADR-0022. **macOS queda fuera, con la intención de sumarlo en el futuro** (dimensión 5) |
| 2 | Toolchain | **Linux:** el **SDK de *sniper*, fijado por *digest***, con **GCC 14** (`g++-14`) y **el `ld` de binutils del SDK** (no trae `lld`); `libstdc++` y `libgcc` estáticas. **Windows:** **llvm-mingw, fijado por *digest*,** cruzando desde Linux con un *toolchain file* de CMake (Clang, `lld`, libc++ y UCRT), con `-static`; **Wine y lavapipe se instalan en el job** desde los paquetes de la misma imagen, sin imagen propia (D1 del ADR-0029). **SDL3 estática en los builds de la CD,** con la opción de copia fijada que ya da el ADR-0006; en desarrollo sigue la del sistema. **Vulkan:** headers de una versión fijada, y **el loader se carga en runtime** a través de SDL3 (`SDL_Vulkan_GetVkGetInstanceProcAddr`), sin enlazar `libvulkan` ni `vulkan-1.dll`: el loader lo pone el driver del jugador. **`engine/gpu` carga cada función de Vulkan en una tabla propia** (`VK_NO_PROTOTYPES`, con `vkGetInstanceProcAddr` y `vkGetDeviceProcAddr`), a partir del puntero inicial que le entrega `platform::`; sin volk, para no sumar una dependencia |
| 3 | Paquete | **Linux:** `MedievalZombies-<versión>-linux-x86_64.tar.gz`. **Windows:** `MedievalZombies-<versión>-windows-x86_64.zip`. Cada uno, una carpeta con **el ejecutable sin símbolos** (`strip`), `LICENSE`, **`LICENSES/`** (los textos de MIT, CC BY 4.0 y OFL 1.1, porque el ejecutable lleva adentro assets, fuentes y bibliotecas con esas licencias), `CREDITS.md` y un `LEEME.txt` en español e inglés. El `.exe` lleva **ícono, `VERSIONINFO` y manifiesto**. **Sin AppImage, Flatpak ni instalador.** Los símbolos de depuración se guardan aparte, como artefacto del workflow, sin publicarse. **`platform::` encuentra la sección del paquete en ELF y en PE** (en PE, por `VirtualSize`; el nombre cabe en 8 bytes). **Carpetas en Windows:** la tarjeta de memoria y las opciones en `%APPDATA%\<juego>\` (lo que da `SDL_GetPrefPath`); la caché de pipelines y los archivos de depuración (el log, el registro por frame y los *replays*), en `%LOCALAPPDATA%\<juego>\`. El nombre `<juego>` lo cierra el ADR-0028 cuando se elija |
| 4 | Confianza en Windows | **Sin firma por ahora.** La firma (postular a SignPath Foundation, que exige una release ya publicada) **se retoma cuando haya un juego jugable**, en "Lo que queda abierto". **Higiene del build contra las heurísticas:** sin empaquetadores, sin datos pegados al final (los assets van en una sección real), sin símbolos, con `VERSIONINFO` y manifiesto. **Las notas de cada release** explican el aviso de SmartScreen ("Más información" → "Ejecutar de todas formas") y cómo verificar el SHA-256 y la atestación. **No se sube a VirusTotal antes de publicar:** el archivo se compartiría con unos 70 proveedores; la prueba previa es el borrador, que prueba el autor. **Un falso positivo** se reporta a Microsoft en el portal de Security Intelligence, como "Software developer" |
| 5 | macOS | **Fuera, con la intención de sumarlo en el futuro.** Queda escrito lo que hará falta: un ADR de Vulkan sobre Metal (MoltenVK o KosmicKrisp) que verifique las features del ADR-0022; Developer ID y notarización (USD 99 al año), o firma *ad-hoc* con instrucciones; la sección del paquete en Mach-O; y las pruebas en un runner `macos-latest`. **Nada de esto entra al flujo actual** |
| 6 | Integridad y procedencia | **`SHA256SUMS`** en cada release; una **atestación de procedencia por paquete**; **releases inmutables** activadas en el repo. **Builds reproducibles:** `SOURCE_DATE_EPOCH` con la fecha del commit, y `-ffile-prefix-map` en las banderas del preset, para que alcance también a las dependencias. **La CD compila cada Release dos veces, en rutas distintas, y falla si los bytes difieren** |
| 7 | Canales y *staging* | **El *staging* es una release en borrador de GitHub,** que solo ve el autor. La CD la arma con todo; yo bajo los paquetes, los pruebo y **la publico a mano**. Al publicarla, un job sube **esos mismos archivos** a itch.io con `butler`, a los canales `linux` y `windows`, con `--userversion` igual a la versión del tag. **En itch.io, cada subida reemplaza el build del canal**, y la app de itch actualiza con un parche. **En GitHub, la release nueva pasa a ser la *Latest*,** y las anteriores quedan como historial (inmutables). Sin betas ni canales ocultos |
| 8 | Secretos | **Uno solo: `BUTLER_API_KEY`,** en un **entorno `itch.io`** que exige mi aprobación (sin "impedir la autoaprobación", porque soy el único revisor) y solo acepta tags `v*`. **Un *ruleset* de tags:** solo yo creo, muevo o borro tags `v*`. Los permisos de escritura (`contents`, `id-token`, `attestations`) solo en los jobs de release. **Fe de erratas de la D9 del ADR-0029:** ningún secreto en la CI; en la CD, solo dentro de entornos protegidos |
| 9 | Qué se prueba y dónde | **Linux:** los tests de `debug` en *sniper*, sin las comparaciones contra un hash guardado (como ya decía el ADR-0029), y una **prueba de humo del paquete final** en contenedores de la última Ubuntu LTS y de la Debian estable más vieja con soporte, con lavapipe. **Windows:** los tests bajo Wine, que bloquean, compilados sin *sanitizers* (los *sanitizers* ya corren en la CI de Linux), **sin los tests que dependen de un *sanitizer*** (los *death tests* de ASan del ADR-0010), que se excluyen por una etiqueta de CTest; y una **prueba de humo del `.exe` final en un runner `windows-latest`**: Windows y UCRT reales, sin GPU. **Es la única excepción a la D6 del ADR-0029** (repetir cada job en la máquina del autor): no compila nada, y su equivalente local es la corrida bajo Wine. **Los hashes guardados solo se comparan en la CI,** con la glibc del snapshot; en la CD, solo las dos ejecuciones en el mismo proceso. **macOS, en teoría:** un runner `macos-latest` con el loader sobre Metal; no se implementa ahora |
| 10 | El workflow | **Un tag `v*` lo dispara.** (1) **Cocinar el paquete una vez**, en el contenedor de la CI con Blender (ADR-0029), y pasarlo como artefacto. (2) En paralelo, **Linux** y **Windows**: tests, Release dos veces (comparando los bytes), `strip` y empaquetado. (3) **Pruebas de humo.** (4) `SHA256SUMS`, atestaciones y **el borrador**, con las notas de git-cliff y las instrucciones de verificación. (5) **Al publicar el borrador**, el job de itch.io, detrás del entorno. **Si una release sale mal**, se publica la siguiente versión de parche y la mala se marca en sus notas |

## Consecuencias

### El camino de una release

1. Termino un corte vertical y **creo el tag `v0.y.z`** (el *ruleset* solo me deja a mí).
2. **La CD cocina el paquete** de assets una vez, con el Blender y la fecha del snapshot de
   la CI.
3. **En el contenedor de *sniper*:** los tests de `debug`; dos builds de Release en rutas
   distintas, que tienen que dar los mismos bytes; `strip` y el `.tar.gz`.
4. **En el contenedor de llvm-mingw, con Wine:** lo mismo para Windows, con los tests bajo
   Wine, y el `.zip`.
5. **Pruebas de humo:** el ejecutable final arranca en Ubuntu y en Debian (contenedores) y
   en un runner de Windows: encuentra su paquete, escribe en sus carpetas y termina bien.
6. **El borrador:** los dos paquetes, `SHA256SUMS`, una atestación por paquete y las notas.
7. **Bajo el `.tar.gz`, lo juego en mi máquina y publico el borrador.** La release queda
   inmutable y pasa a ser la *Latest*.
8. **Apruebo el job de itch.io:** recién ahí ve `BUTLER_API_KEY`, y sube los mismos
   archivos a los canales `linux` y `windows`.

### Lo que se gana

- **Un binario de Linux que arranca en las distros comunes**, medido en cuatro (de glibc
  2.31 a 2.44), y uno de Windows que solo pide lo que Windows ya trae.
- **Cualquier build de la CD se repite en mi máquina**, en el mismo contenedor y con el
  mismo comando. Y como es reproducible, **cualquiera puede comprobar** que el binario
  publicado sale de ese commit.
- **Lo que se publica es lo que se probó:** del borrador a itch.io no se recompila nada.
- **Tres capas de confianza gratis:** el SHA-256 (llegó entero), la atestación (salió de
  este commit y este workflow) y la release inmutable (nadie la cambió después).
- **Un solo secreto**, que no sale sin mi aprobación.
- **Se cierra lo que dejó abierto el ADR-0028:** de dónde sale el paquete en la CD, la
  sección en PE y las carpetas de Windows.

### Lo que se sacrifica

- **SmartScreen avisa en cada versión,** y en Windows 11 con Smart App Control activo, un
  `.exe` sin firma y sin reputación **se bloquea**. Los antivirus pueden marcarlo. Se
  acepta hasta que haya un juego jugable.
- **El binario de Linux no sale de mi compilador:** *sniper* no tiene un Clang moderno,
  así que la Release de Linux la compila GCC 14, el secundario del ADR-0006. La CI ya
  compila y prueba con GCC, así que no es un compilador desconocido.
- **Dos bibliotecas estándar distintas:** `libstdc++` en Linux y libc++ en Windows. El
  orden de iteración de los contenedores no ordenados y el de los empates de `std::sort`
  no está especificado: **la simulación no puede depender de eso**, o el *replay* de
  Linux y el de Windows divergen.
- **Wine no es Windows:** su libm, su UCRT y su sistema de archivos son
  reimplementaciones. La prueba de humo en un runner real cubre lo básico; un driver de
  GPU de Windows no lo prueba nada automático.
- **El rendimiento en Windows no se mide:** la máquina de referencia del ADR-0020 es
  Linux. Tampoco el modo de latencia D del ADR-0032.
- **La CD tarda más que la CI:** cocinar, dos plataformas, dos builds de Release cada una
  y las pruebas de humo. No bloquea a nadie, porque corre solo con los tags.
- **Una release mala no se reemplaza:** se publica la siguiente versión de parche.

### Tickets, en orden

Llegan con el primer tag de corte vertical, cada uno en su ticket (fe de erratas,
ADR-0033: llegan con el esqueleto andante, en M3, y el orden de construcción lo fija el
roadmap):

1. **Un *spike*** que compila el engine real en *sniper* y en llvm-mingw, y corre sus
   tests bajo Wine con el Wine de los paquetes de la imagen (el prototipo usó el de
   Arch). Reemplaza el *spike* de *sniper* del ADR-0029.
2. **`platform::` en Windows:** archivos (`CreateFileW`, `ReadFile` con offset), hilos,
   las carpetas de `%APPDATA%` y `%LOCALAPPDATA%`, y **la sección del paquete en PE**,
   con tests de ELF y de PE.
3. **Los presets de la CD:** el *toolchain file* de llvm-mingw, SDL3 estática, el
   runtime estático, `-ffile-prefix-map` y `SOURCE_DATE_EPOCH`, y el `strip`. Los
   headers de Vulkan fijados y la tabla propia de funciones de Vulkan en `engine/gpu`,
   cargada desde el puntero que entrega `platform::`, **ya no van aquí** (fe de erratas,
   ADR-0033): van en la base de Vulkan del esqueleto andante, antes que este ticket.
4. **Los recursos del `.exe`:** ícono, `VERSIONINFO` y manifiesto.
5. **El modo de prueba de humo** del ejecutable: arranca, verifica el paquete y las
   carpetas, informa si hay Vulkan y termina con un código de salida.
6. **El workflow de la CD:** cocinar, Linux, Windows, los bytes comparados, las pruebas de
   humo, `SHA256SUMS`, las atestaciones y el borrador.
7. **La configuración del repo:** releases inmutables, el *ruleset* de tags `v*` y el
   entorno `itch.io` con su secreto.
8. **El job de itch.io**, con `butler` en una versión fijada y su `sha256`.
9. **El `LEEME.txt`, `LICENSES/` en el paquete y la plantilla de las notas** (la
   verificación y el aviso de SmartScreen). Este ticket revisa también **qué runtimes
   enlazados estáticos piden un aviso** (libc++ y `libstdc++` tienen excepciones que no
   lo piden; el CRT y los hilos de mingw-w64 hay que mirarlos) y los suma a
   `CREDITS.md` y a `LICENSES/` si hace falta.

### Fe de erratas en otros ADRs

| ADR | Qué cambia |
|---|---|
| ADR-0029 | La D10 la reemplaza este ADR. La D9: "ningún secreto" vale para la CI; la CD tiene uno, dentro de un entorno protegido. **La D6 tiene una excepción:** la prueba de humo en `windows-latest`, que no compila nada. Se corrigen también Consecuencias y Abierto (el *spike* de *sniper* pasa al ticket 1 de este ADR) |
| ADR-0006 | Windows entra: llvm-mingw con un *toolchain file*. Los builds de la CD usan SDL3 estática (la opción de copia fijada), el runtime estático, `-ffile-prefix-map` en las banderas del preset y `SOURCE_DATE_EPOCH`. **La D3 tiene una excepción:** la Release de Linux enlaza con el `ld` de *sniper*, que no trae `lld`. Vulkan: headers fijados y el loader en runtime a través de SDL3, sin enlazarlo |
| ADR-0011 | La D5 ("todo corre en Debug con ASan y UBSan") tiene una excepción: la suite de Windows de la CD corre bajo Wine sin *sanitizers*, y excluye por etiqueta los tests que dependen de uno |
| ADR-0028 | Se cierran tres abiertos: el paquete de la CD se cocina una vez y viaja como artefacto; la sección se encuentra en PE por la tabla de secciones; las carpetas de Windows |
| ADR-0004 | `platform::` suma la implementación de Windows (archivos, hilos y carpetas) |
| ADR-0014 | La CD publica en GitHub Releases y en itch.io, desde un borrador que el autor publica a mano |
| ADR-0019 | Cada paquete lleva `LICENSE`, `LICENSES/` y `CREDITS.md` |
| Visión | Hay build de Windows desde el primer corte vertical |

### Lo que queda abierto

- **La firma en Windows,** cuando haya un juego jugable: postular a SignPath Foundation
  (exige una release publicada). Antes hay que confirmar con ellos que aceptan assets
  CC BY y fuentes OFL **dentro del ejecutable**, porque exigen una licencia aprobada por
  la OSI en todos los componentes.
- **macOS** (dimensión 5).
- **Cuánto tarda la CD** y si vale la pena cachear la cocción entre tags.
- **Las distros exactas de la prueba de humo**, en el ticket del workflow.
- **Si se publican los símbolos** (para leer *crash dumps* de jugadores) y cuánto tiempo
  se guardan.
- **El rendimiento y la latencia en Windows** (ADR-0020 y ADR-0032), si alguna vez hay
  una máquina de Windows para medir.

## Cómo lo defiendo

**¿Por qué no compilas la release en tu propia máquina, si es la misma que la de la CI?**
Porque mi máquina tiene glibc 2.44, y un binario enlazado ahí pide `GLIBC_2.43`: lo medí,
y no arranca ni en Ubuntu 24.04 ni en Debian. La glibc no se puede enlazar estática ni
traer al lado del juego; la pone el sistema del jugador. La única salida es compilar
contra una glibc vieja, y eso es el SDK de *sniper* (2.31). Ese binario arrancó en las
cuatro distros que probé.

**¿No es raro publicar con GCC si tu compilador principal es Clang?**
Es un costo que acepto. *sniper* trae GCC 14 y un Clang 11 que no sirve para C++20. La
CI ya compila y prueba con GCC desde el principio, justamente para no depender de un
solo compilador, así que GCC no es un desconocido. Y antes de publicar, los tests vuelven
a correr con ese mismo compilador.

**¿Por qué compilas Windows desde Linux y no en Windows con MSVC, que es lo que usan los
estudios?**
Porque solo tengo Linux, y quiero que cualquier build de la CD se repita en mi máquina
con un comando, igual que la CI. llvm-mingw es Clang, el mismo compilador que uso todos
los días, y corre en un contenedor fijado. MSVC me obligaría a depender de un runner de
Windows que no puedo reproducir. Lo que pierdo es la ABI de MSVC y las PDB por defecto;
para un juego que no carga plugins de terceros, la ABI no importa.

**Si no tienes Windows, ¿cómo sabes que funciona?**
En tres capas. Los tests corren bajo Wine en la misma imagen que compila: medí que el
`.exe` encuentra su paquete, escribe en `%APPDATA%` y ve Vulkan. Después, el `.exe` final
arranca en un runner de Windows real, que no tiene GPU pero sí el Windows y el UCRT de
verdad. Y lo que ninguna de las dos prueba, un driver de GPU de Windows, lo prueban los
jugadores. Lo escribí como sacrificio.

**¿Por qué tus releases no están firmadas? ¿No es inseguro?**
Firmar no hace seguro a un binario: dice quién lo publicó. Lo que sí hice gratis es
probar la integridad (SHA-256), la procedencia (una atestación que dice qué commit y qué
workflow lo construyó) y que nadie lo cambió después (releases inmutables). Lo que no da
la firma gratis es la confianza inmediata de SmartScreen: según Microsoft, ya ni un
certificado EV la da. La firma gratis que acumula reputación, la de SignPath,
exige un proyecto ya publicado. La retomo cuando haya un juego jugable.

**¿Qué ve un jugador de Windows al abrir tu juego?**
"Windows protegió su PC", porque el archivo no tiene firma ni reputación. Tiene que hacer
clic en "Más información" y "Ejecutar de todas formas". Las notas de cada release lo
explican y enseñan a verificar el SHA-256 antes. En Windows 11 con Smart App Control
activo, se bloquea. Lo acepto mientras el juego sea una demo.

**¿Qué haces para que los antivirus no lo marquen como virus?**
Evito lo que activa sus heurísticas: nada de empaquetadores como UPX ni de código que se
modifica a sí mismo, y los assets van en una sección real del ejecutable, no pegados al
final. El `.exe` lleva ícono, información de versión y manifiesto, como un programa
normal. Si igual lo marcan, lo reporto a Microsoft como falso positivo, como
desarrollador.

**¿Qué significa que tus builds sean reproducibles, y cómo lo sabes?**
Que el mismo commit da los mismos bytes en cualquier máquina con la misma imagen. Lo
medí: sin hacer nada, dos builds en otra carpeta y a otra hora dan bytes distintos, por
la fecha del build y las rutas. Con `SOURCE_DATE_EPOCH` y `-ffile-prefix-map` dan lo
mismo. Y no me quedo en la promesa: la CD compila cada release dos veces, en rutas
distintas, y falla si los bytes no coinciden. Así cualquiera puede recompilar mi commit
y comprobar que el binario que publiqué sale de ahí.

**¿Qué es tu *staging*? Los estudios tienen builds internos, de QA y betas.**
Una release en borrador de GitHub, que solo veo yo. La CD la arma con todo, yo bajo el
paquete, lo juego y recién ahí la publico. Al publicarla, otro job sube esos mismos
archivos a itch.io. Es la misma idea que usan los estudios: construir una vez y publicar
lo que se probó, sin recompilar entre medio. Las betas y los canales para jugadores no
me hacen falta siendo una sola persona.

**Tu CD sube a itch.io con una llave. ¿Qué pasa si alguien la roba?**
La llave vive en un entorno protegido de GitHub: un job no la ve hasta que yo apruebo esa
ejecución, y solo desde un tag `v*` que solo yo puedo crear. Ningún PR, y ningún
workflow de la CI, la toca. Si aun así se filtra, itch.io me deja revocarla y crear otra.

**¿Por qué no un AppImage o un Flatpak, que son los formatos de Linux?**
Porque mi juego ya es un solo archivo: los assets van dentro del ejecutable. AppImage
solo me sumaría un ícono en el menú, a cambio de pedirle FUSE al jugador: sin FUSE no
arranca, lo medí en Ubuntu. Flatpak es razonable, pero pasa por Flathub, que compila
desde la fuente en su infraestructura y revisa el *sandbox*. Un `.tar.gz` es lo que menos
le pide al jugador y conserva el permiso de ejecución.

**¿El *replay* de un jugador de Windows da el mismo hash que el tuyo?**
No, y lo medí: el mismo binario de Linux da tres hashes distintos en cuatro distros, y
bajo Wine sale otro. Las funciones como `sin` y `cos` las pone la biblioteca del sistema
del jugador, y cambian entre versiones. Por eso los hashes guardados solo se comparan en
la CI, con mi glibc. Además, Linux usa `libstdc++` y Windows libc++, así que la
simulación no puede depender de nada que el estándar deje sin especificar, como el
orden de un `unordered_map`.

## Referencias

- Valve, *Steam Linux Runtime for game developers*:
  <https://gitlab.steamos.cloud/steamrt/steam-runtime-tools/-/blob/main/docs/slr-for-game-developers.md>
- GNU C Library, NEWS (2.41 a 2.43: CORE-MATH y los símbolos de compatibilidad SVID):
  <https://sourceware.org/git/?p=glibc.git;a=blob;f=NEWS>
- Microsoft Learn, *SmartScreen reputation for Windows app developers* (2026-08-17):
  <https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/smartscreen-reputation>
- Microsoft Learn, *Quickstart: Set up Artifact Signing*:
  <https://learn.microsoft.com/en-us/azure/artifact-signing/quickstart>
- Azure, *Artifact Signing pricing*:
  <https://azure.microsoft.com/en-us/pricing/details/artifact-signing/>
- SignPath Foundation, condiciones para proyectos open source: <https://signpath.org/terms>
- Certum, *Open Source Code Signing*: <https://shop.certum.eu/open-source-code-signing.html>
- Microsoft Security Intelligence, envío de archivos:
  <https://www.microsoft.com/en-us/wdsi/filesubmission>
- VirusTotal, *Terms of Service*: <https://docs.virustotal.com/docs/terms-of-service>
- GitHub Docs, *Using artifact attestations to establish provenance for builds*:
  <https://docs.github.com/en/actions/security-guides/using-artifact-attestations-to-establish-provenance-for-builds>
- GitHub Changelog, *Immutable releases are now generally available* (2025-10-28):
  <https://github.blog/changelog/2025-10-28-immutable-releases-are-now-generally-available/>
- GitHub Docs, *Deployments and environments*:
  <https://docs.github.com/en/actions/reference/workflows-and-actions/deployments-and-environments>
- itch.io, *butler: Pushing builds* y *Logging in*:
  <https://itch.io/docs/butler/pushing.html>, <https://itch.io/docs/butler/login.html>
- Flathub, *Requirements*: <https://docs.flathub.org/docs/for-app-authors/requirements>
- llvm-mingw: <https://github.com/mstorsjo/llvm-mingw>
- AppImage, appimagetool y type2-runtime: <https://github.com/AppImage/appimagetool>
- Reproducible Builds, `SOURCE_DATE_EPOCH`:
  <https://reproducible-builds.org/docs/source-date-epoch/>
- Microsoft, formato PE (la tabla de secciones, `VirtualSize` y `SizeOfRawData`):
  <https://learn.microsoft.com/en-us/windows/win32/debug/pe-format>
