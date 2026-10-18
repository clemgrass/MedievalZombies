# ADR-0013: Convenciones de código y análisis estático

- **Estado:** Aceptado
- **Fecha:** 2026-10-18
- **Ticket:** #23
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

Antes de la primera línea de código hay que fijar cómo se ve y cómo se revisa el
código. Cambiarlo después obliga a reformatear todo el proyecto y ensucia el
historial. Las herramientas (`clang-format`, `clang-tidy`, `clangd`) ya están
instaladas, y el build system lo define el ADR-0006.

Este ADR decide:

1. el formato del código;
2. las convenciones de nombres;
3. el nivel de warnings del compilador;
4. qué revisa el análisis estático;
5. cuándo y dónde se aplica todo lo anterior.

**No** decide qué partes de C++ se usan: eso es el ADR-0009 (subconjunto de C++).

## Criterios

1. **Consistencia:** el código se ve igual sin importar quién o qué lo escribió (el
   autor o Claude).
2. **Detección temprana de errores:** atrapar bugs en el editor y no en tiempo de
   ejecución.
3. **Costo bajo** para una sola persona: que sea automático y no una lista de reglas
   que recordar.
4. **Legibilidad para un evaluador** que lee el repo público.

## Opciones

### 1. Formato (`clang-format`)

| Opción | Cómo se ve |
|---|---|
| **LLVM** | 2 espacios de indentación, llaves en la misma línea, 80 columnas |
| **Google** | Parecido a LLVM, con algunas reglas propias de includes y punteros |
| **Mozilla / WebKit** | Llaves de funciones en línea propia, 4 espacios (WebKit) |
| **Base LLVM con ajustes propios** | Por ejemplo 4 espacios y 100 columnas, que suele leerse mejor en código matemático con nombres largos |

Lo importante no es *cuál* estilo, sino que **sea automático**: se formatea al
guardar en el editor (vía `clangd`) o con un comando, y nunca se discute a mano.

### 2. Nombres

| Opción | Ejemplo | Dónde se ve |
|---|---|---|
| **Estilo de la biblioteca estándar** | `mesh_slicer`, `slice_mesh()`, `vertex_count` | STL, Boost |
| **PascalCase / camelCase** | `MeshSlicer`, `sliceMesh()`, `vertexCount` | Muchos engines y Vulkan-Hpp |
| **Estilo Unreal** | `FMeshSlicer`, `SliceMesh()`, `VertexCount` | Unreal Engine (prefijos por tipo) |

Hay decisiones que van juntas con esta: cómo se distinguen los miembros de una clase
(`m_count`, `count_`) y las constantes (`kMaxPieces`, `MAX_PIECES`).

### 3. Warnings del compilador

- **Base:** `-Wall -Wextra -Wpedantic`, más algunos útiles para un engine:
  `-Wshadow`, `-Wconversion` (conversiones implícitas que pierden precisión, muy
  relevantes en código numérico) y `-Wnull-dereference`.
- **`-Werror`** (tratar los warnings como errores):
  - **Siempre:** obliga a mantener todo limpio, pero puede frenar cuando se está
    experimentando.
  - **Solo en CI o en una configuración específica:** se experimenta libre en local,
    y lo que se integra a `main` está limpio.

### 4. Análisis estático (`clang-tidy`)

`clang-tidy` tiene familias de checks. Las más relevantes para este proyecto:

| Familia | Qué atrapa |
|---|---|
| `bugprone-*` | Patrones que suelen ser bugs (conversiones peligrosas, `memcpy` mal usado) |
| `performance-*` | Copias innecesarias, `std::move` mal usado |
| `modernize-*` | Construcciones antiguas que tienen un equivalente moderno en C++11+ |
| `readability-*` | Legibilidad (algunos checks son muy opinados) |
| `cppcoreguidelines-*` | Las C++ Core Guidelines (algunos checks chocan con el estilo de un engine, por ejemplo la aritmética de punteros) |

La práctica habitual es **activar familias completas y desactivar uno por uno** los
checks que no encajan, dejando escrito por qué. Así cada excepción es una decisión
consciente.

### 5. Cuándo se aplica

- **En el editor:** `clangd` muestra los warnings y los checks de `clang-tidy`
  mientras se escribe, y formatea al guardar.
- **Antes del commit:** un *hook* de git (`pre-commit`) puede verificar el formato.
- **En CI**, cuando exista: compilar con warnings como errores y correr
  `clang-tidy`.

## Investigación

- `clang-tidy` está pensado como un framework extensible: todos los checks se activan
  y desactivan por configuración (`.clang-tidy`), lo que permite empezar con un
  conjunto chico y crecer. Hasta hay configuraciones específicas para engines, como
  las de *clang-tidy for Unreal Engine*, lo que muestra que la industria ajusta el
  conjunto de checks a su estilo y no usa todos.
- Los archivos de configuración (`.clang-format`, `.clang-tidy`, `.editorconfig`)
  viven en la raíz del repo. Así cualquier editor o herramienta los toma solos, y un
  evaluador ve las reglas sin tener que preguntar.

## Decisión

| # | Subdecisión | Elección |
|---|---|---|
| 1 | Formato | Base **LLVM**, con **4 espacios** y **100 columnas**, aplicado automáticamente al guardar |
| 2 | Nombres | **Estilo de la biblioteca estándar:** tipos, funciones y variables en `snake_case` (`mesh_slicer`, `slice_mesh()`, `vertex_count`). Miembros con prefijo **`m_`** (`m_count`) |
| 3 | Warnings | **Máxima rigurosidad:** `-Wall -Wextra -Wpedantic -Wshadow -Wconversion` y más, con **warnings como errores (`-Werror`) en todas las configuraciones** |
| 4 | `clang-tidy` | Familias **`bugprone`, `performance` y `modernize`** completas, con cada excepción escrita junto a su razón. De `readability` y `cppcoreguidelines`, solo checks seleccionados |
| 5 | Cuándo se aplica | **En el editor** (`clangd`) **y en CI**. Hook `pre-commit` solo para el formato |

## Consecuencias

- **Archivos de configuración en la raíz:** `.clang-format`, `.clang-tidy` y
  `.editorconfig`. Son periféricos y los escribe Claude, con revisión del autor.
- **Nombres verificados por la herramienta:** el check
  `readability-identifier-naming` de `clang-tidy` aplica `snake_case` y el prefijo
  `m_`. Por convención, las macros van en `MAYÚSCULAS` y los parámetros de template en
  `PascalCase` (`template <typename Vertex>`). Las constantes siguen el `snake_case`
  general.
- **Lista de warnings:** además de la base, `-Wold-style-cast`, `-Wsign-conversion`,
  `-Wdouble-promotion`, `-Wnull-dereference`, `-Wimplicit-fallthrough`,
  `-Wnon-virtual-dtor`, `-Wcast-align` y `-Wformat=2`. No se usa `-Weverything` de
  Clang: la propia documentación de Clang desaconseja usarlo como configuración
  permanente.
- **`-Werror` siempre:** ningún warning llega a un commit. Mientras se experimenta, un
  warning frena la compilación hasta corregirlo, y ese es el costo aceptado.
- **Los warnings se aplican solo al código propio.** Las dependencias (SDL3, headers
  de Vulkan) se incluyen como headers *de sistema*, para que sus warnings no rompan el
  build.

## Cómo lo defiendo

**¿Por qué `m_count` y no simplemente `count`?**
Para ver de inmediato si una variable es miembro del objeto, y para evitar choques de
nombres con `-Wshadow`, por ejemplo entre un parámetro del constructor y el miembro que
inicializa.

**Con `-Werror` siempre activo, ¿no te frena cuando estás experimentando?**
Sí, frena al experimentar, pero siempre es un flag que se puede bajar para probar sin
él. Al momento de incluir algo al sistema, es sistemáticamente mejor tenerlo siempre en
consideración que al revés, es decir, usarlo solo en el momento crítico de las
pruebas.

## Referencias

- Documentación de clang-tidy:
  <https://llvm.googlesource.com/clang-tools-extra/+/master/docs/clang-tidy/index.rst>
- *clang-tidy for Unreal Engine*, configuración de checks:
  <https://docs.redpoint.games/clang-tidy-for-unreal-engine/docs/configuring_your_project>
- Chromium, uso de clang-tidy:
  <https://chromium.googlesource.com/chromium/src/+/83.0.4103.116/docs/clang_tidy.md>
- C++ Core Guidelines: <https://isocpp.github.io/CppCoreGuidelines/>
