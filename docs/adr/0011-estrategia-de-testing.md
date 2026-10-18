# ADR-0011: Estrategia de testing y registro de bugs

- **Estado:** Aceptado
- **Fecha:** 2026-10-18
- **Ticket:** #21
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

El código empieza después de M1, y lo primero que se va a escribir es `engine/core`: la
matemática (ADR-0008), la memoria (ADR-0010), los contenedores, `result<T, E>` y los
handles (ADR-0009). Todo lo demás se apoya en esa capa. Un error ahí aparece lejos de
su causa, por ejemplo como un corte que deja un agujero o un zombie que tiembla.

El **corte de mallas** es además el algoritmo más difícil de verificar a ojo: un corte
puede *verse* bien y dejar una arista suelta que recién explota en el tercer corte sobre
el mismo pedazo.

Y cuando aparezca un bug, no basta con arreglarlo: hay que **dejarlo registrado** y, si
se puede, **atado a un test que lo reproduzca**, para que nunca vuelva sin que nadie se
entere. Los bugs que no se puedan reproducir con un test también quedan registrados,
como historial.

Hay que decidir:

1. con qué **framework** se escriben los tests;
2. qué **tipos de test** se usan y para qué parte del engine;
3. **dónde se aplica TDD** (escribir el test antes que el código) y dónde no;
4. **quién escribe** los tests, según la regla de reparto del proyecto;
5. cómo se **organizan y corren**;
6. cómo se **registran los bugs** y cómo se enlazan con sus tests de regresión.

Queda **fuera de alcance** la infraestructura de CI (cuándo y dónde corre cada cosa en
GitHub), que decide el ADR-0014, y los benchmarks de rendimiento, que deciden los
ADR-0020 y ADR-0021.

## Criterios

1. **Confianza en el núcleo:** sobre todo en la matemática y el corte de mallas.
2. **Encaje con el subconjunto de C++** (ADR-0009): sin excepciones y sin RTTI.
3. **Velocidad de feedback:** que correr los tests sea tan rápido que se haga siempre.
4. **Costo de mantenimiento:** tests que no se rompan con cada refactor.
5. **Trazabilidad:** de cada bug se sabe qué pasó, cómo se arregló y qué test lo vigila.
6. **Aprendizaje:** entender qué prueba cada tipo de test y por qué.
7. **Todo gratis** y disponible en Linux.

## Opciones

### 1. Framework

La restricción decisiva es que el engine compila con **`-fno-exceptions`**
(ADR-0009), y los frameworks más populares usan excepciones para detener un test.

| Opción | Sin excepciones | Tests de `assert` (*death tests*) | Otros |
|---|---|---|---|
| **GoogleTest** | **Funciona de forma nativa:** `ASSERT_*` termina el test con un `return`, no con una excepción. También funciona sin RTTI | **Sí:** `EXPECT_DEATH` corre la sentencia en un proceso hijo y verifica que aborte | El más usado en la industria (LLVM, Chromium, la biblioteca de geometría Manifold). Biblioteca compilada, no solo headers. Requiere C++17 |
| **Catch2 v3** | **Experimental:** si falla un `REQUIRE`, se llama a `std::terminate` y se cae todo el ejecutable | No | Sintaxis muy legible (`REQUIRE(a == b)`), secciones y BDD |
| **doctest** | Sí, pero **desaparece la familia `REQUIRE`** (la que detiene el test al fallar) | No | El que compila más rápido. Solo headers. Sintaxis parecida a Catch2 |
| **Propio y mínimo** | Sí, por diseño | Habría que implementarlo (`fork` + `waitpid`) | Se aprende cómo funciona un framework por dentro, pero no es núcleo del juego ni suma al portafolio |

Los tres primeros están en los repositorios oficiales de Arch (`gtest` 1.18,
`catch2` 3.16, `doctest` 2.5).

> **¿Por qué importan los *death tests*?** El ADR-0009 decidió que los errores del
> programador se atrapan con `assert` (por ejemplo, usar un handle de una entidad ya
> destruida). Para probar que ese `assert` **realmente salta**, el test tiene que
> sobrevivir a un `abort()`. La única forma es correr esa parte en otro proceso.

### 2. Tipos de test

| Tipo | Qué hace | Para qué sirve aquí |
|---|---|---|
| **Unitario** | Entrada conocida → salida esperada, calculada a mano | Matemática, contenedores, arenas, handles, `result`, ECS |
| **De propiedades** (*property-based*) | Genera cientos de entradas aleatorias y verifica que una **propiedad** se cumpla siempre: `inverse(M) * M ≈ I`, `|normalize(v)| ≈ 1` | Matemática y corte: encuentra los casos raros que nadie escribe a mano |
| **De invariantes de malla** | Después de cada operación, verifica propiedades estructurales de la malla | Corte: cada arista compartida por exactamente 2 caras (malla cerrada), volumen de los pedazos ≈ volumen original, ningún triángulo degenerado, cada vértice del lado correcto del plano |
| **Fuzzing** | Un fuzzer guiado por cobertura (libFuzzer, que viene con Clang) muta entradas para provocar caídas, que los sanitizers detectan | Cargadores de archivos y el corte con planos y mallas arbitrarias |
| **De imagen de referencia** (*golden image*) | Renderiza una escena fija fuera de pantalla, lee los píxeles y los compara con una imagen aprobada, con tolerancia | Renderer: detecta regresiones visuales (pantalla negra, normales invertidas, sombras rotas) |
| **De determinismo** (*replay*) | Graba los inputs de una partida, la reproduce y compara un hash del estado final | Gameplay y física. Es posible gracias al paso fijo de 60 Hz (ADR-0012) |
| **De regresión** | Reproduce un bug concreto que ya ocurrió | Cualquier sistema. Ver la sección 6 |
| **Playtest manual** | Una persona juega | La sensación del combate, que ningún test automático mide |

### 3. TDD (test primero)

| Zona | Opciones |
|---|---|
| **Código con especificación clara** (matemática, memoria, contenedores, funciones del corte) | TDD estricto, o tests después |
| **Código exploratorio** (renderer, sensación del gameplay, *spikes*) | TDD, tests después, o sin tests automáticos |

### 4. Quién escribe los tests

| Opción | Qué significa |
|---|---|
| **El dueño del código escribe sus tests** | Los del núcleo, el autor. Los del periférico, Claude. La infraestructura de tests (fixtures, el arnés de imágenes de referencia, el del fuzzer) es periférica |
| **Claude escribe todos los tests** | El autor escribe solo el código del núcleo |

### 5. Organización

| Tema | Opciones |
|---|---|
| **Estructura** | Un solo ejecutable de tests, o uno por módulo (`tests/core/`, `tests/slicing/`…) |
| **Ejecución** | CTest, con etiquetas por tipo (`unit`, `property`, `regression`, `golden`, `fuzz`), para correr solo los rápidos con `ctest -L unit` |
| **Cobertura** | No medirla, medirla sin meta, o exigir un porcentaje mínimo |

### 6. Registro de bugs y tests de regresión

#### 6.1 Dónde vive la lista de bugs

| Opción | Cómo funciona | A favor | En contra |
|---|---|---|---|
| **GitHub Issues + vista de tabla en el Project** | Cada bug es un issue con un formulario `bug`. Una vista del Project #1 filtrada por `tipo:bug` muestra la tabla, con columnas propias (severidad, test de regresión) | Coherente con el ADR-0002. El PR que lo arregla lo cierra solo (`Closes #N`). El historial de comentarios queda en el issue | La tabla vive en GitHub, no en el repo |
| **Archivo `docs/bugs.md`** | Una tabla en Markdown que se edita a mano | Vive en el repo y se lee sin GitHub | Se desincroniza de los issues y de los PRs. Hay que mantener dos lugares |
| **Ambos** | Issues como fuente, más un `docs/bugs.md` generado por un script | La tabla queda en el repo | Un script más que mantener |

#### 6.2 Qué estándar sigue cada reporte

| Fuente | Qué aporta |
|---|---|
| **ISO/IEC/IEEE 29119-3** (sucesor de IEEE 829) | Los **campos del reporte de incidente**: resumen, contexto, descripción (pasos, resultado esperado y obtenido), evaluación de **severidad**, **prioridad**, riesgo y estado |
| **IEEE 1044** | La **clasificación** de anomalías: tipo, severidad y categoría de causa raíz, para poder analizar los bugs en conjunto |
| **Ciclo de vida de Bugzilla** | Los **estados y resoluciones** clásicos: abierto → resuelto, con resolución `FIXED`, `WONTFIX`, `DUPLICATE`, `WORKSFORME` (no reproducible) o `INVALID` (no era un bug) |

#### 6.3 Cómo se enlaza un bug con su test

| Opción | Cómo funciona |
|---|---|
| **Test de regresión clásico** | El test verifica el comportamiento **correcto**. Mientras el bug existe, falla (rojo). Cuando se arregla, pasa (verde) y queda para siempre: si algún día vuelve a fallar, el bug regresó |
| **Test con "falla esperada"** (*xfail*) | El test se escribe apenas se reporta el bug, y se marca como "se espera que falle". Así la suite sigue verde mientras el bug está abierto. Cuando el arreglo entra, el test **pasa de forma inesperada**, la suite lo avisa, se quita la marca y queda como test de regresión clásico |

En CTest, la marca de falla esperada es la propiedad `WILL_FAIL`. LLVM usa la misma
idea: algunos tests de regresión entran como `XFAIL` hasta que se arregla su bug.

El enlace es **en los dos sentidos**:

- el **issue** tiene un campo con la ruta y el nombre del test;
- el **test** lleva el número del issue en su nombre (`Bug123_...`) y su URL en un
  comentario, así que `git grep 123` encuentra ambos.

Un bug **sin test** (una falla del driver, un problema de timing, un bug que no se
volvió a ver) se registra igual, con un campo obligatorio que explica **por qué no
tiene test**, y se cierra con la resolución que corresponda (por ejemplo
`WORKSFORME`). Queda como historial.

## Investigación

### Frameworks y excepciones

- **doctest:** con `-fno-exceptions`, su documentación indica que las macros `REQUIRE`
  y las de excepciones **quedan sin definir**, y que la opción para abortar después de
  N fallas deja de funcionar bien, porque usa una excepción para terminar el test.
- **Catch2 v3:** su soporte sin excepciones es **experimental**. Un error que normalmente
  sería una excepción se escribe en `cerr` y se llama a `std::terminate`. Lo mismo pasa
  cuando falla un `REQUIRE`: se cae el ejecutable entero, no solo ese test.
- **GoogleTest:** sus `ASSERT_*` abortan la función con `return`, sin excepciones. Por
  eso solo pueden usarse en funciones que devuelven `void`. Sin excepciones, sus
  *death tests* simplemente no envuelven la sentencia en `try`/`catch`.
- **RapidCheck**, el framework de tests de propiedades más conocido de C++ (con
  *shrinking*: reduce el contraejemplo al caso mínimo), **lanza una excepción en cada
  `RC_ASSERT`** (`throw CaseResult(...)`, en `Assertions.hpp`). No encaja con el
  ADR-0009, salvo compilando sus tests con excepciones.

**Conclusión:** el único framework popular que encaja sin concesiones con
`-fno-exceptions`, y que además puede probar los `assert`, es GoogleTest. Los tests de
propiedades se pueden escribir sobre él con un generador aleatorio de **semilla fija**,
que se imprime al fallar para poder reproducir el caso.

### TDD en videojuegos

- **Noel Llopis** aplicó TDD en High Moon Studios en juegos para Xbox 360 y PS3, y lo
  documentó en su blog *Games from Within* y en la charla de GDC *Backwards Is Forward:
  Making Better Games with Test-Driven Development*. Su conclusión práctica: TDD rinde
  en la lógica y los sistemas, y cuesta en el código atado al hardware y a lo visual.
- **Rare** (*Automated Testing of Gameplay Features in Sea of Thieves*, GDC 2019) llegó
  a tener cientos de miles de tests automáticos, unitarios y de integración del
  gameplay, para mantener una build siempre lista para publicar.
- Lo común a ambos: **el test tiene que valer lo que cuesta mantenerlo.** La matemática
  y el corte tienen una especificación precisa y no cambian de interfaz seguido: son los
  candidatos ideales. La sensación de un golpe de espada no tiene especificación hasta
  que se juega.

### Imágenes de referencia

- **Filament** (el renderer PBR de Google) compara cada cambio contra imágenes de
  referencia guardadas en un repositorio aparte. Para que el resultado no dependa de la
  GPU, renderiza con los **rasterizadores por software de Mesa**. Aun así, mantiene
  imágenes separadas por plataforma, porque "hosts distintos producen píxeles distintos
  por razones que no tienen que ver con ningún cambio".
- **lavapipe** es el driver Vulkan por software de Mesa. Está en Arch como
  `vulkan-swrast` (no instalado todavía) y permite renderizar sin GPU y sin pantalla,
  igual que en un servidor de CI.
- Una comparación píxel a píxel exacta es frágil. Lo habitual es una **tolerancia** (por
  ejemplo, un porcentaje máximo de píxeles distintos por encima de cierto umbral), más
  una imagen de diferencias para revisar a ojo cuando falla.

### Invariantes de malla

La biblioteca **Manifold** (operaciones booleanas sobre mallas, con garantía de salida
cerrada) expone justo las medidas que sirven para verificar un corte: **volumen,
superficie y género** (número de "agujeros" de la malla), y usa GoogleTest. Un corte
correcto de una malla cerrada produce pedazos cerrados, y sus volúmenes suman el
original.

### Fuzzing

**libFuzzer** viene con Clang (`-fsanitize=fuzzer`) y se combina con ASan y UBSan, que
ya están en el preset Debug (ADR-0006). Funciona dentro del mismo proceso y guía las
mutaciones con la cobertura del código. Es especialmente útil en los cargadores de
archivos, porque leen datos que no controla el engine. Cada entrada que provoca una
caída se guarda y **se convierte en un test de regresión**.

### Estándares de reporte de bugs

- **IEEE 829** definía el *Test Incident Report*. Desde 2013 lo reemplaza **ISO/IEC/IEEE
  29119-3**, cuyo reporte de incidente incluye: resumen, información de tiempo, quién lo
  reporta, contexto, descripción del incidente, evaluación de severidad, evaluación de
  prioridad, riesgo y estado.
- **IEEE 1044-2009** define un vocabulario común y un conjunto de atributos para
  clasificar anomalías (tipo, severidad, causa raíz), pensados para analizar los
  defectos en conjunto y mejorar el proceso. Está "inactivo-reservado" desde 2020, pero
  sigue siendo la referencia clásica de clasificación.
- **Bugzilla** (Mozilla) popularizó el ciclo de vida que usan casi todos los
  gestionadores de bugs. Sus resoluciones:
  - `FIXED`: arreglado y probado;
  - `INVALID`: no era un bug;
  - `WONTFIX`: es un bug, pero no se va a arreglar;
  - `DUPLICATE`: ya estaba reportado;
  - `WORKSFORME`: no se pudo reproducir.
- **Severidad y prioridad son cosas distintas.** La severidad es cuánto daño hace (un
  crash es más severo que un píxel mal pintado). La prioridad es cuándo se arregla. Un
  crash en un menú que nadie usa puede ser severo y de prioridad baja.

## Decisión

| # | Qué | Elección |
|---|---|---|
| 1 | Framework | **GoogleTest.** Los tests de propiedades se escriben sobre él, con un generador de semilla fija que se imprime al fallar |
| 2 | Tipos de test | **Desde el inicio:** unitarios, de propiedades, de invariantes de malla, de regresión y fuzzing. **Cuando existan el renderer y el gameplay:** imágenes de referencia y *replay*. **La sensación del combate:** playtest |
| 3 | TDD | **Estricto** en la matemática, la memoria, los contenedores, el ECS y las funciones del corte. **Tests después** en el renderer y el gameplay. **Sin tests** en los *spikes* |
| 4 | Quién escribe los tests | **Claude escribe los tests y el autor los revisa.** En las zonas con TDD, Claude escribe primero el test que falla, a partir de la especificación acordada, y el autor escribe la implementación del núcleo que lo hace pasar |
| 5 | Organización y cobertura | Un ejecutable por módulo (`tests/core/`, `tests/slicing/`…) más `tests/regression/`. CTest con etiquetas por tipo. Todo corre en el preset Debug, con ASan y UBSan. **Cobertura del 100 % en el core y en las zonas con TDD**, siguiendo los criterios de los sistemas críticos. El resto se mide sin meta |
| 6.1 | Lista de bugs | **GitHub Issues con formulario `bug`, en un Project aparte de *bug hunting***, separado del Project #1, para no mezclar el dominio de los arreglos con el de las features |
| 6.2 | Campos del reporte | Los de **ISO/IEC/IEEE 29119-3** adaptados, la severidad de **IEEE 1044** y las resoluciones de **Bugzilla** |
| 6.3 | Enlace bug ↔ test | **Falla esperada (*xfail*)** con `WILL_FAIL` mientras el bug está abierto, y test de regresión clásico después. Enlace en los dos sentidos: el issue nombra al test, y el test lleva el número y la URL del issue |

## Consecuencias

- **Cambia la regla de reparto de `CLAUDE.md`:** los tests pasan a ser trabajo de Claude
  en todo el engine, con el autor como revisor. El código del núcleo sigue siendo del
  autor. El autor tiene que entender cada test que aprueba, porque un evaluador puede
  preguntar por cualquiera.
- **Cobertura del 100 %:**
  - se mide con la cobertura de Clang (`-fprofile-instr-generate -fcoverage-mapping`,
    más `-fcoverage-mcdc` si se exige MC/DC) y `llvm-cov`, en un preset `coverage` nuevo;
  - se exige **sobre líneas y ramas**: cada `if` tomado por los dos lados, que es el
    criterio de DO-178C nivel B y el "muy recomendado" de ISO 26262 a nivel de unidad;
  - las ramas de falla de los `assert` solo se cubren con *death tests*;
  - una línea que no se puede cubrir necesita una justificación escrita junto al código,
    como en DO-178C con el código desactivado;
  - el 100 % **no prueba que no haya bugs**: solo garantiza que no hay código que ningún
    test haya ejecutado.
- **Archivos y configuración nuevos:**
  - el formulario `.github/ISSUE_TEMPLATE/bug.yml`, con los campos de 29119-3, la
    severidad (S1 crash o corrupción, S2 mayor, S3 menor, S4 cosmético), la
    reproducibilidad, el test de regresión o por qué no tiene, y la resolución;
  - el **Project de bug hunting**, con campos de estado, severidad, prioridad,
    resolución y test de regresión;
  - la guía en `docs/process/tickets.md`.

  Todo eso va en un ticket propio.
- **Dependencias nuevas** cuando empiece el código: `gtest` (paquete de Arch) y, para
  las imágenes de referencia, `vulkan-swrast` (lavapipe). Cada una se agrega a
  `CREDITS.md`.
- **El ADR-0014** decide qué de todo esto corre en CI y en qué momento. Este ADR deja
  definido qué existe y cómo se corre en local.
- **Lo que queda abierto:**
  - la tolerancia exacta de las imágenes de referencia, que se fija cuando exista el
    renderer;
  - si se exige MC/DC además de ramas en las funciones más críticas del corte.

## Cómo lo defiendo

**¿Por qué GoogleTest y no Catch2 o doctest?**
GoogleTest es bastante robusto y tiene una buena forma de probar todo lo que quiero
testear, incluidos los *death tests*.

**¿Qué es un *death test* y para qué lo necesitas?**
Es un test que espera que el código termine el programa, por ejemplo con un `assert`
que falla. Ahí, que el programa se caiga es el comportamiento correcto, y entonces el
test pasa.

**¿Tener 100 % de cobertura garantiza que el código no tiene bugs?**
No. De hecho, los tests no sirven tanto para evitar bugs como para poder agregar y
cambiar código sin que cambie su comportamiento. Además, dan seguridad de que el código
funciona bien en los casos que se pueden pensar.

**¿Qué es un test *xfail* y por qué lo usas en tus bugs?**
Es un test que se espera que falle. El test de un bug verifica el comportamiento
correcto, así que mientras el bug existe, falla, y eso es lo esperado. Cuando se arregla
el bug, el test pasa de forma inesperada, y eso es positivo, porque significa que el bug
ya no ocurre. Entonces se le quita la marca de falla esperada, y queda como test de
regresión.

**Si los tests no los escribiste tú, ¿cómo sabes que prueban lo correcto?**
Soy el revisor de los tests. Que no los escriba no significa que no los revise y
entienda qué hace cada uno.

**¿Cuál es la diferencia entre severidad y prioridad?**
La prioridad es qué tan importante es resolver algo y qué tan rápido. La severidad es
cuánto impacto tiene, en este caso sobre el juego.

## Referencias

- doctest, *Configuration* (`DOCTEST_CONFIG_NO_EXCEPTIONS`):
  <https://github.com/doctest/doctest/blob/master/doc/markdown/configuration.md>
- Catch2, *Configuration* (desactivar excepciones):
  <https://github.com/catchorg/Catch2/blob/devel/docs/configuration.md>
- GoogleTest, *Advanced Topics* (*death tests*):
  <https://google.github.io/googletest/advanced.html>
- RapidCheck: <https://github.com/emil-e/rapidcheck>
- Noel Llopis, *Games from Within*, categoría TDD:
  <https://gamesfromwithin.com/category/test-driven-development/>
- Noel Llopis, *Backwards Is Forward: Making Better Games with Test-Driven Development*,
  GDC: <https://gdcvault.com/play/1013177/Backwards-Is-Forward-Making-Better>
- Robert Masella, *Automated Testing of Gameplay Features in Sea of Thieves*, GDC 2019:
  <https://gdcvault.com/play/1026366/Automated-Testing-of-Gameplay-Features>
- Filament, *Rendering Difference Test*:
  <https://google.github.io/filament/dup/test_ci_renderdiff.html>
- Manifold: <https://github.com/elalish/manifold/wiki/Manifold-Library>
- LLVM, *libFuzzer*: <https://www.llvm.org/docs/LibFuzzer.html>
- Wikipedia, *Software test documentation* (IEEE 829 y su reemplazo por ISO/IEC/IEEE
  29119-3): <https://en.wikipedia.org/wiki/Software_test_documentation>
- IEEE 1044-2009, *Standard Classification for Software Anomalies*:
  <https://standards.ieee.org/ieee/1044/4607>
- Mozilla, *Bug statuses and resolutions*:
  <https://wiki.mozilla.org/BMO/UserGuide/BugStatuses>
- CMake, propiedad de test `WILL_FAIL`:
  <https://cmake.org/cmake/help/latest/prop_test/WILL_FAIL.html>
