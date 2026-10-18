# ADR-0014: Modelo de colaboración, metodología de desarrollo y principios de diseño

- **Estado:** Aceptado
- **Fecha:** 2026-10-18
- **Ticket:** #27
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

Los ADRs anteriores decidieron **qué** se construye y **con qué**: el lenguaje, la API
gráfica, el build, el subconjunto de C++, la arquitectura, las convenciones, el testing.
Falta decidir **cómo se trabaja día a día** una vez que empiece el código.

### Un trabajo colaborativo

Este proyecto **es un trabajo colaborativo entre el autor y Claude** (un asistente de
IA), con roles claros:

- **El autor** da las **ideas macro** de cómo tiene que funcionar cada sistema,
  **revisa todo el código** y es responsable de **entender el sistema completo**: qué
  hace, por qué lo hace así y cómo mejorarlo.
- **Claude** apoya sobre todo en la **escritura del código**, además de investigar,
  proponer alternativas y explicar.

Escribir código a mano es una buena forma de aprender, pero en este proyecto el
aprendizaje está en **diseñar, revisar y entender**. Este ADR deja el modelo por
escrito, para que cualquier evaluador sepa exactamente quién hizo qué.

### Qué hay que decidir

1. los **detalles del modelo de colaboración**: modos de trabajo y transparencia;
2. en qué **orden** se construye (por sistemas completos, o de punta a punta);
3. cómo **fluye** el trabajo por el tablero y cuánto se hace a la vez;
4. cómo se **revisa** el código;
5. cómo se manejan la **refactorización** y la **deuda técnica**;
6. cómo se **versiona** y cuándo hay una **release**;
7. **cuándo y cómo entra la CI/CD**;
8. cómo se **documenta el código**;
9. con qué **método** se trabaja el rendimiento;
10. qué **principios de diseño** guían el código: acoplamiento, cohesión, SOLID frente a
    diseño orientado a datos;
11. cuándo se puede **corregir un ADR aceptado**.

**Fuera de alcance**, porque ya está decidido: los ADRs (ADR-0001), los tickets
(ADR-0002), el build (ADR-0006), el testing (ADR-0011), la arquitectura del engine
(ADR-0012), las convenciones (ADR-0013) y las ramas y commits (`docs/process/git.md`).
Los **números** de rendimiento los decide el ADR-0020 y las **herramientas** de
profiling el ADR-0021. Aquí solo se decide el método.

## Criterios

1. **Honestidad:** queda claro, en el repo y frente a un evaluador, qué hizo cada
   participante.
2. **Sostenible para una persona** sin fecha límite: poca ceremonia, cero burocracia
   que no aporte.
3. **Aprendizaje por comprensión:** el autor entiende y puede explicar cada sistema y
   cada práctica.
4. **Calidad y trazabilidad visibles** en el repo público: un evaluador puede seguir
   cualquier cambio desde su porqué hasta su código.
5. **Defendible frente a un evaluador:** prácticas reconocibles en la industria.
6. **Encaje con lo ya decidido:** ECS y capas (ADR-0012), `virtual` solo en las
   fronteras de módulo (ADR-0009), tests y cobertura (ADR-0011).

## Opciones

### 1. Detalles del modelo de colaboración

#### 1.1 Modos de trabajo

| Modo | Cómo funciona | Cuándo |
|---|---|---|
| **Dirigido** | El autor explica el flujo, el diseño y lo que quiere que haga el sistema. Claude lo convierte en código, en el lugar que corresponde. Si la idea del autor falla (un caso que no cubre, un costo que no se vio), Claude lo señala y se itera en conjunto | Cuando el autor tiene una idea clara de cómo debe funcionar, o quiere ejercitar el diseño de un sistema |
| **Propuesto** | Claude propone el diseño y el código, con sus alternativas y referencias. El autor lo cuestiona, pide cambios y aprueba | Lo periférico, o cuando el autor quiere ver primero las opciones |

En los dos modos, **nada entra a `main` sin la revisión del autor**.

#### 1.2 Cómo se marca el modo en cada ticket

| Opción | Qué significa |
|---|---|
| **Labels `modo:dirigido` y `modo:propuesto`** | Se ve y se filtra en el tablero |
| **Solo un campo en el formulario** | Queda escrito en el ticket, pero no se puede filtrar |
| **No se marca** | Se decide en la conversación |

#### 1.3 Transparencia

| Práctica | Qué aporta |
|---|---|
| **El modelo explicado en el `README.md`** y en este ADR | Quien abre el repo sabe desde el principio cómo se hizo |
| **`Co-Authored-By: Claude` en los commits** que incluyen código escrito por Claude | GitHub muestra a Claude como coautor en cada commit. La autoría queda en el historial de git, commit por commit |
| **"Qué debo poder explicar" obligatoria** en cada feature y tarea | El ticket no se cierra hasta que el autor responde las preguntas de un evaluador sobre ese trabajo. Es la prueba de que lo entendió |

### 2. Orden de construcción

| Opción | Cómo se ve | A favor | En contra |
|---|---|---|---|
| **Por sistemas completos** (horizontal) | Primero el renderer entero, después la física entera, después el corte… | Cada sistema queda "terminado" | Pasan meses sin nada jugable. Los problemas de integración aparecen al final, cuando cuestan más |
| **Esqueleto andante + cortes verticales** | Primero un **esqueleto andante** (*walking skeleton*): lo mínimo que atraviesa todas las capas de punta a punta. Después, **cortes verticales**: cada uno agrega un poco de cada sistema hasta lograr algo jugable | Siempre hay algo que corre. La integración se prueba desde el día uno. Cada corte es una demo para el portafolio | Los sistemas crecen de a poco, y a veces hay que rehacer una parte cuando el siguiente corte la exige |
| **Prototipos desechables** | Código rápido para responder una pregunta ("¿es divertido este combate?") y después se tira | Responde preguntas de diseño muy rápido | Si no se tira, se vuelve deuda |

Una práctica complementaria es el **spike** (de XP): una investigación de código con
**tiempo acotado** para resolver una duda técnica ("¿cuánto cuesta reconstruir el
buffer de vértices después de un corte?"). Su producto es una respuesta, no código para
`main`.

### 3. Flujo de trabajo

| Opción | Cómo funciona | Para este proyecto |
|---|---|---|
| **Scrum** | Sprints de duración fija (por ejemplo 2 semanas), con planificación, revisión y retrospectiva | Mucha ceremonia. Los sprints suponen un ritmo constante, y este proyecto no tiene fecha |
| **Kanban** | Flujo continuo por columnas, con un **límite de trabajo en curso** (*WIP limit*) por columna | Poca ceremonia. El límite evita tener muchas cosas empezadas y ninguna terminada |
| **Scrumban** | Kanban, más revisiones periódicas | Agrega una revisión cada cierto tiempo, sin sprints |

El tablero ya existe (ADR-0002), con `Backlog → Listo → En progreso → En revisión →
Hecho`. GitHub Projects permite poner un **límite por columna**, pero solo como aviso
visual: no impide pasarse.

### 4. Revisión de código

| Tema | Opciones |
|---|---|
| **Qué se mira** | Sin guía, o con una lista fija (por ejemplo la de Google: diseño, funcionalidad, complejidad, tests, nombres, comentarios, estilo, documentación) |
| **Plantilla de PR** | Ninguna, o `.github/pull_request_template.md` con la lista de revisión |
| **Protección de `main`** | Ninguna, o exigir PR y CI en verde antes de cada merge |
| **Auditoría contra los ADRs** | Ninguna, o una **skill de Claude Code** que, al crear un PR, lance un subagente auditor por tema. Cada auditor compara el código cambiado con los ADRs de su área, con contexto del proyecto y del ticket, y entrega hallazgos con soluciones |

### 5. Refactorización y deuda técnica

| Tema | Opciones |
|---|---|
| **Cuándo refactorizar** | Cuando moleste; con la **regla del boy scout** (dejar el código un poco mejor de lo que se encontró); o con la **regla de tres** (abstraer recién cuando algo se repite tres veces) |
| **Cómo** | Mezclado con features, o **en PRs `refactor:` separados**, sin cambiar el comportamiento y con los tests en verde antes y después |
| **Deuda técnica** | Sin registro; `TODO` sueltos en el código; o **cada deuda deliberada con su issue**, y un `TODO(#N)` en el código que apunta a él |

### 6. Versionado y releases

| Opción | Formato | Qué comunica |
|---|---|---|
| **SemVer** | `0.3.1` (MAYOR.MENOR.PARCHE) | Si hay cambios incompatibles. Mientras la mayor es `0`, "todo puede cambiar": es la etapa de desarrollo inicial |
| **CalVer** | `2026.10` | Cuándo se publicó |
| **Sin versiones** | Solo `main` | Nada: no hay puntos de referencia |

Herramientas: los **tags de git** y las **GitHub Releases** marcan cada versión. Un
`CHANGELOG.md` se puede escribir a mano, o generar con **git-cliff** a partir de los
Conventional Commits, siguiendo el formato *Keep a Changelog*.

### 7. CI/CD

| Tema | Opciones |
|---|---|
| **Cuándo entra** | Desde el primer `CMakeLists.txt` (el esqueleto andante), o más adelante, cuando haya más código |
| **Dónde** | GitHub Actions: **gratis y sin límite de minutos en repos públicos** con los runners estándar |
| **Entorno** | Un runner Ubuntu (con Clang y SDL3 más antiguos, así que SDL3 iría con `MZ_VENDOR_SDL3`), o un **contenedor de Arch** con el mismo toolchain que la máquina del autor |
| **Qué corre** | Formato, compilación con Clang y GCC, `clang-tidy`, tests con ASan y UBSan, cobertura del core, `reuse lint`. Más adelante: imágenes de referencia con lavapipe y fuzzing corto |
| **CD** | Nada; o **en cada tag**, compilar en Release y adjuntar el binario a la GitHub Release |

### 8. Documentación del código

| Opción | Qué significa |
|---|---|
| **Solo comentarios del porqué** | El código dice *qué* hace. Los comentarios dicen *por qué*, con referencia al ADR cuando aplica (`// Reverse-Z (ADR-0008): ...`) |
| **Más `///` en la API pública** | Además, cada función y tipo público de un módulo tiene un comentario breve en su header. Doxygen lo puede leer más adelante, sin obligación de generar un sitio |
| **Doxygen completo** | Todo comentado con etiquetas (`@param`, `@return`) y un sitio de documentación generado |

### 9. Método de rendimiento

| Opción | Qué significa |
|---|---|
| **Optimizar sobre la marcha** | Escribir rápido desde el principio, según la intuición |
| **Medir antes de optimizar** | "Make it work, make it right, make it fast" (Kent Beck). Primero correcto y simple. Solo se optimiza lo que el profiler señala, contra un presupuesto por sistema (ADR-0020) |
| **Más registro antes/después** | Además, todo PR de rendimiento incluye la medición antes y después en su descripción |

### 10. Principios de diseño

| Enfoque | Idea central | Fuente |
|---|---|---|
| **SOLID y "Clean Code"** | Clases chicas, polimorfismo en vez de `switch`, ocultar los detalles detrás de abstracciones | Robert C. Martin |
| **Diseño orientado a datos (DOD)** | El trabajo de un programa es **transformar datos**. Se diseña según cómo se recorren los datos en memoria, no según un modelo de objetos del mundo | Mike Acton, CppCon 2014 |
| **Módulos profundos** | Un buen módulo esconde mucha funcionalidad detrás de una interfaz **chica**. Muchas clases chicas y "planas" aumentan la complejidad total. Hay que programar de forma **estratégica** (invertir en diseño), no **táctica** (sacar la feature y seguir) | John Ousterhout, *A Philosophy of Software Design* |

### 11. Corrección de ADRs aceptados

| Opción | Qué significa |
|---|---|
| **Siempre inmutables** | Cualquier cambio, incluso una corrección, exige un ADR nuevo que reemplace al anterior |
| **Flexibles durante la investigación** | Mientras no exista código, un ADR aceptado se puede corregir como **fe de erratas** (un error o un malentendido). Cuando empieza el código, rige la inmutabilidad estricta |

## Investigación

### Colaboración con un asistente de IA

- La **coautoría en git** es una convención que GitHub reconoce: una línea
  `Co-Authored-By: Nombre <correo>` al final del mensaje de commit hace que el commit
  aparezca con los dos autores. Así, la participación de Claude queda registrada en el
  mismo lugar donde vive el código, y no solo en un documento.
- El modo **dirigido** se parece al **pair programming** clásico de XP, con roles fijos:
  el *navigator* piensa la dirección, el diseño y los casos borde, y el *driver*
  escribe. Aquí el autor es siempre el *navigator* y Claude el *driver*.
- En el portafolio, lo que se presenta es "**diseñé, dirigí y revisé** el corte de
  mallas, y puedo explicar cada decisión". Por eso pesan tanto las secciones "Cómo lo
  defiendo" de los ADRs y "Qué debo poder explicar" de los tickets: son la evidencia de
  que el autor entiende lo que se construyó.

### Orden de construcción en videojuegos

- **Esqueleto andante:** Alistair Cockburn lo define como "una implementación diminuta
  del sistema que realiza una pequeña función de punta a punta". No necesita la
  arquitectura final, pero **conecta los componentes principales**. *The Pragmatic
  Programmer* llama a la misma idea **balas trazadoras** (*tracer bullets*), y marca la
  diferencia con un prototipo: el código de una bala trazadora **se queda**; el de un
  prototipo se tira.
- **La industria** distingue dos etapas antes de la producción:
  - el **prototipo** responde "¿vale la pena hacer esto?" (*find the fun*): es feo,
    usa arte provisional y se tira;
  - el **corte vertical** responde "¿podemos hacerlo de verdad?": una porción chica del
    juego con calidad final en todos los sistemas, que decide si se pasa a producción.
- **Clinton Keith** (*Agile Game Development: Build, Play, Repeat*) introdujo Scrum en
  la industria en 2003 y Lean/Kanban en 2006. Su lema, "construir, jugar, repetir",
  resume la idea: iteraciones cortas que terminan en algo **jugable**.

### Kanban y el límite de trabajo en curso

- La **Ley de Little** relaciona tres medidas de cualquier flujo: *trabajo en curso =
  rendimiento × tiempo de entrega*. Si el rendimiento no cambia, **más trabajo en curso
  solo alarga cuánto tarda cada cosa en terminarse**. La regla práctica: terminar algo
  antes de empezar otra cosa.
- **GitHub Projects** tiene límites por columna en la vista de tablero. La documentación
  aclara que son solo un aviso: "poner un límite no impide que nadie agregue tarjetas
  que lo superen, ni que lo hagan las automatizaciones".
- En este modelo, el cuello de botella es **la revisión del autor**: Claude puede
  escribir más rápido de lo que se alcanza a revisar con calma. Un límite en
  `En revisión` protege justo eso.

### Revisión de código

La guía pública de Google (*What to look for in a code review*) ordena lo que se mira:

- **el diseño** es lo más importante: si las piezas encajan entre sí y con el resto del
  sistema;
- después, **la funcionalidad** (hace lo que se quiere, y eso es bueno para quien lo usa),
  **la complejidad** (si se puede entender rápido; "demasiado complejo" significa que el
  próximo que lo toque va a meter bugs) y **los tests**;
- al final, los nombres, los comentarios, el estilo y la documentación.

### Deuda técnica

Martin Fowler (*Technical Debt Quadrant*) clasifica la deuda en dos ejes: **deliberada o
inadvertida**, y **prudente o imprudente**. La única que conviene tomar a propósito es
la **deliberada y prudente**: un atajo consciente, con un plan para pagarlo. La
inadvertida y prudente ("ahora sabemos cómo debimos hacerlo") es inevitable cuando se
aprende, y se paga refactorizando.

### Versionado

**SemVer 2.0.0**: "la versión mayor cero (0.y.z) es para el desarrollo inicial. Cualquier
cosa puede cambiar en cualquier momento. La API pública no debe considerarse estable".
Calza con un engine que todavía está naciendo. **git-cliff** genera un `CHANGELOG.md`
en formato *Keep a Changelog* directamente desde los Conventional Commits, que el
proyecto ya usa.

### CI en GitHub Actions

Los runners estándar de GitHub son **gratis y sin límite de minutos para repos
públicos**. Las tarifas por minuto solo aplican a repos privados. Un *job* puede correr
dentro de un **contenedor** (por ejemplo `archlinux:latest`), lo que permite usar el
mismo Clang, CMake y SDL3 que la máquina del autor (ADR-0006).

### Principios de diseño: el debate

- **Mike Acton** (*Data-Oriented Design and C++*, CppCon 2014) parte de que el trabajo
  del programador no es escribir código, sino **resolver problemas de transformación de
  datos**, y que el hardware (caché, memoria) es la plataforma real. Es la base del ECS
  elegido en el ADR-0012.
- **Casey Muratori** (*"Clean" Code, Horrible Performance*, 2023) midió el ejemplo
  clásico de los libros de "Clean Code" (una jerarquía polimórfica de figuras) contra
  un `switch` simple: unos 35 ciclos por figura con polimorfismo y unos 24 con el
  `switch`, **1,5 veces más rápido**, y mucho más al reorganizar los datos. El video
  abrió un debate amplio, y muchos respondieron que la legibilidad también importa.
- **John Ousterhout** propone medir el diseño por la **complejidad** que le impone a
  quien lo lee. Las dos causas son las **dependencias** (código que no se entiende
  aislado) y la **oscuridad** (información importante que no es evidente). Su
  ejemplo de módulo profundo son las llamadas de E/S de Unix: cinco funciones simples
  (`open`, `read`, `write`, `lseek`, `close`) que esconden muchísima complejidad.
- **No son excluyentes.** DOD dice cómo organizar los datos en los caminos calientes.
  Los módulos profundos dicen cómo diseñar las fronteras entre módulos. Algunas reglas
  de SOLID (una responsabilidad por módulo, depender de interfaces en las fronteras)
  encajan con el `virtual` solo en las fronteras del ADR-0009. Otras (polimorfismo en
  vez de `switch`, clases muy chicas) chocan con DOD y con los módulos profundos.

## Decisión

| # | Qué | Elección |
|---|---|---|
| 1.1 | Modos de trabajo | **Dirigido** para los sistemas grandes (renderer, corte, física, animación, IA) y **propuesto** para lo periférico. En el modo dirigido, Claude prepara el código alrededor y marca con `TODO(human)` los puntos de decisión clave. El autor explica con sus palabras cómo resolverlos, Claude lo escribe y lo mejora, y el autor lo revisa |
| 1.2 | Marca del modo | Labels **`modo:dirigido`** y **`modo:propuesto`** en las features y las tareas. Las investigaciones no llevan label de modo, porque el proceso de los ADRs ya define los roles |
| 1.3 | Transparencia | El modelo explicado en el `README.md`, **`Co-Authored-By: Claude`** en los commits con código de Claude, y **"Qué debo poder explicar" obligatoria** en cada feature y tarea |
| 2 | Orden de construcción | **Esqueleto andante, después cortes verticales.** Las dudas técnicas se resuelven con **spikes** de tiempo acotado, en ramas `spike/<N>-…` que no se mergean. El resultado queda escrito en el issue |
| 3 | Flujo | **Kanban, con el tablero tal como está:** `Backlog → Listo → En progreso → En revisión → Hecho`, sin límites por columna ni ceremonias extra |
| 4 | Revisión | **Todo cambio entra por rama y PR, nunca directo a `main`.** Plantilla de PR con la lista de revisión de Google. **`main` está protegida** con un *ruleset* de GitHub, sin excepciones: PR obligatorio, conversaciones resueltas antes del merge, sin *force push* y sin borrado. Cuando exista la CI, se suma como chequeo obligatorio. No se exige aprobación (GitHub no deja aprobar el propio PR) ni historial lineal (para no restringir los métodos de merge). Antes de cada PR corre la **skill de auditoría** contra los ADRs. Sus hallazgos los revisan juntos el autor y Claude, y cada uno se decide: se corrige, se ignora o queda como deuda técnica |
| 5 | Refactorización y deuda | **Regla del boy scout y regla de tres.** Los refactors van en PRs `refactor:` separados de las features. **Cada deuda deliberada tiene su issue** (label `deuda-técnica`) y un `TODO(#N)` en el código |
| 6 | Versionado | **SemVer `0.y.z`**, con un tag de git por cada corte vertical y un **`CHANGELOG.md`** generado con git-cliff desde los Conventional Commits |
| 7 | CI/CD | Lo decide un **ADR propio, el ADR-0029**. Este ADR solo fija que la CI existe desde el esqueleto andante y que la CD, si se logra, publica en GitHub (Releases o Packages). Si no se logra, no hay CD |
| 8 | Documentación del código | **Código autoexplicativo y comentarios lo más concisos posible, sin referencias a ADRs.** Cada bloque se tiene que entender por sí solo, fuera de contexto, tanto para una persona como para una IA |
| 9 | Rendimiento | **Medir antes de optimizar**, y cada PR de rendimiento incluye la medición antes y después |
| 10 | Principios de diseño | **Híbrido, priorizando siempre DOD.** Módulos profundos en las fronteras, de SOLID solo lo que encaja (una responsabilidad por módulo, interfaces en las fronteras) y programación estratégica |
| 11 | Corrección de ADRs | **Flexibles solo durante la investigación.** Mientras no exista código, un ADR aceptado se puede corregir como fe de erratas. **Cuando empiece el código, rige la inmutabilidad estricta:** un cambio de decisión exige un ADR nuevo, y el anterior pasa a `Reemplazado` |

## Consecuencias

- **Fe de erratas de la investigación.** Con la regla 11, el modelo colaborativo queda
  igual en todos los documentos: `CLAUDE.md`, la spec base, el plan de M0, la guía de
  tickets, los formularios, `setup-labels.sh` y los ADR-0005, 0008 y 0011.
- **Labels en GitHub:** `modo:dirigido`, `modo:propuesto` y `deuda-técnica`, según
  `setup-labels.sh`.
- **`docs/process/git.md`:** se elimina la excepción de commits directos a `main`. Todo
  entra por PR, y el *ruleset* "Proteger main" lo hace cumplir. El ADR-0029 agrega la CI
  como chequeo obligatorio del mismo *ruleset*.
- **`docs/README.md`:** la regla de inmutabilidad suma la excepción de la etapa de
  investigación.
- **Archivos nuevos:**
  - `.github/pull_request_template.md`, con la lista de revisión y el recordatorio de la
    auditoría;
  - `CHANGELOG.md` y la configuración de git-cliff, que llegan con el primer tag.
- **Tickets nuevos:** el ADR-0029 (CI/CD) y la skill de auditoría.
- **El Learning output style se mantiene**, porque genera justo los `TODO(human)` del
  modo dirigido.
- **Lo que queda abierto:** los detalles de la CI y la CD (ADR-0029), y el diseño de la
  skill de auditoría (su propio ticket).

## Cómo lo defiendo

**Si Claude escribió el código, ¿qué hiciste tú?**
Me aseguro de la revisión y de la visión general desde el inicio. Quizás eso no se note
al principio, pero una vez que el videojuego empiece a aparecer, en cosas como el ritmo
o el timing, las decisiones las tomo yo al 100 %. Además, me aseguro de que todo sea
coherente y de que no haya deuda técnica: si el día de mañana desapareciera toda la IA,
yo debería entender todo el código.

**¿Por qué un esqueleto andante y no construir primero el renderer completo?**
Hacer un buen *scaffold* inicial, donde todo corra en lo mínimo, ayuda a construir
cortes verticales pequeños, y eso ayuda a entender los procesos de punta a punta.
Además, empieza a mostrar resultados relativamente rápido, y cualquier error de punta a
punta se puede pillar pronto. Eso es lo más importante, porque el producto final es
justamente esa integración completa.

**¿Qué es la deuda técnica y cómo la manejas?**
Es intentar avanzar demasiado rápido con una decisión o una solución parche que
funciona en el momento, pero que a futuro podría traer problemas. La skill de auditoría
existe para no quedar con mucha deuda técnica y mantenerse ordenado con las decisiones
reales. Además, al ser un proyecto de aprendizaje, no hay apuro: no hace falta un
prototipo rápido ni competir con nadie. Se puede ir a una velocidad mediana y tranquila,
en la que todo se hace correctamente.

**¿Por qué priorizas el diseño orientado a datos sobre la orientación a objetos?**
Tengo entendido que, para un ECS, DOD es de las mejores metodologías: calza muy bien con
lo que un ECS intenta hacer.

**¿Por qué SemVer `0.y.z`?**
Es clásico, se entiende rápido lo que significa y es estándar.

**¿Cómo te aseguras de que el código respeta lo que decidieron los ADRs?**
Con la skill de auditoría, y además revisando constantemente el código. Por el tipo de
trabajo y la falta de apuro, se pueden tomar decisiones sin parches.

## Referencias

- GitHub Docs, *Creating a commit with multiple authors*:
  <https://docs.github.com/en/pull-requests/committing-changes-to-your-project/creating-and-editing-commits/creating-a-commit-with-multiple-authors>
- Alistair Cockburn, *Walking Skeleton*, citado en:
  <https://www.mattblodgett.com/2020/09/start-with-walking-skeleton.html>
- Andrew Hunt y David Thomas, *The Pragmatic Programmer* (balas trazadoras), resumen:
  <https://fullstackhub.substack.com/p/the-pragmatic-programmer-12-tracer>
- *Vertical slice vs prototype: what to build first*:
  <https://bugnet.io/blog/vertical-slice-vs-prototype-what-to-build-first>
- Game Developer, *What you should take out of pre-production*:
  <https://www.gamedeveloper.com/game-platforms/what-you-should-take-out-of-pre-production>
- Clinton Keith, *Agile Game Development: Build, Play, Repeat*, 2.ª ed. (2020):
  <https://www.informit.com/title/0136527817>
- Kanban University, *Exploring Little's Law*:
  <https://kanban.university/wp-content/uploads/2022/09/Exploring-Littles-Law-KGS-2022.pdf>
- GitHub Docs, *Customizing the board layout* (límites por columna):
  <https://docs.github.com/en/issues/planning-and-tracking-with-projects/customizing-views-in-your-project/customizing-the-board-layout>
- Google, *What to look for in a code review*:
  <https://google.github.io/eng-practices/review/reviewer/looking-for.html>
- Martin Fowler, *Technical Debt Quadrant*:
  <https://martinfowler.com/bliki/TechnicalDebtQuadrant.html>
- Semantic Versioning 2.0.0: <https://semver.org/>
- git-cliff: <https://github.com/orhun/git-cliff>
- Keep a Changelog: <https://keepachangelog.com/>
- GitHub Docs, *GitHub Actions billing*:
  <https://docs.github.com/en/billing/concepts/product-billing/github-actions>
- Mike Acton, *Data-Oriented Design and C++*, CppCon 2014:
  <https://isocpp.org/blog/2015/01/cppcon-2014-data-oriented-design-and-c-mike-acton>
- Casey Muratori, *"Clean" Code, Horrible Performance* (2023), resumen y debate:
  <https://thenewstack.io/when-clean-code-hampers-application-performance/>
- John Ousterhout, *A Philosophy of Software Design*, 2.ª ed. (2021), resumen:
  <https://dev.to/carstenbehrens/summary-of-a-philosophy-of-software-design-by-john-ousterhout-c52>
