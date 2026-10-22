# ADR-0030: Diagramas técnicos del engine

- **Estado:** Propuesto
- **Fecha:** 2026-10-21
- **Ticket:** #60
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

El proyecto existe para **aprender de verdad**: el autor tiene que poder explicar cada
sistema frente a un evaluador (ADR-0014). Con el modelo de colaboración, Claude escribe el
código y el autor diseña, revisa y es responsable de entender el sistema completo. Para
eso no alcanza con leer el código función por función. Hace falta ver el **proceso**:
por qué funciones pasa un dato, en qué orden, qué crea cada paso y con qué valores.

El ADR-0014 también decidió que los comentarios son mínimos y no citan ADRs. Por eso el
"cómo fluye" no vive en el código: tiene que vivir en otra parte, y esa parte tiene que
seguir siendo verdad cuando el código cambia.

Este ADR decide **el sistema que produce y mantiene los diagramas técnicos**, no los
diagramas en sí. Todavía no existe código: el primer diagrama fiel llega con el
esqueleto andante (ADR-0014). Mientras tanto hay un piloto teórico, el del corte de una
malla ([`diagramas-piloto/`](../research/diagramas-piloto/README.md)), que se reemplaza
cuando exista ese código.

### Qué hay que decidir

1. el **formato** y la herramienta;
2. **dónde se ven**;
3. qué **tipos de diagrama** se usan y para qué pregunta;
4. la **leyenda** común y cómo se impone;
5. los **niveles** (de lo macro a lo micro), dónde vive cada diagrama y cómo se navega;
6. cómo se mantienen **fieles al código**;
7. cómo se muestran los **ejemplos concretos**;
8. **cuándo** se crea o se actualiza un diagrama, y quién lo hace.

**Fuera de alcance:** las herramientas de profiling y de debug visual (ADR-0021) y la CI
que corre los chequeos (ADR-0029). Este ADR solo dice **qué** se verifica.

## Criterios

1. **Entender los procesos de verdad** (aprendizaje, ADR-0014).
2. **Nivel técnico real:** funciones, datos y orden de verdad, no cajas genéricas.
3. **Fidelidad con el código actual:** un diagrama muestra lo que existe, no lo que se
   planeó.
4. **Navegación de lo macro a lo micro.**
5. **Leyenda normalizada:** la misma forma significa lo mismo en todos los diagramas.
6. **Gratis y versionable en git**, con diffs que se puedan revisar.
7. **Que Claude los pueda crear y actualizar** como texto.
8. **Valor para explicarlos frente a un evaluador.**

## Opciones

### 1. Formato y herramienta

| Opción | Qué es | A favor | En contra |
|---|---|---|---|
| **Mermaid** | Texto dentro del Markdown, en un bloque `mermaid` | GitHub lo dibuja solo: cero pasos de build y nada que commitear aparte. Diff de una línea por cambio. Flujo, secuencia, estado, bloques y paquetes de bits | Sin `include`: la leyenda se repite en cada diagrama. Layout poco controlable. Validarlo en una CI exige un Chromium |
| **D2** | Lenguaje de diagramas con layouts intercambiables (dagre, ELK) | El mejor layout del piloto. Leyenda en un archivo importado. Formas para todo (incluida la cola). El SVG trae tema claro y oscuro. Puede importar un fragmento generado. Render en 0,1 s | GitHub no lo dibuja: hay que generar y commitear el SVG, cuyo diff no se puede revisar. Herramienta más joven que las demás |
| **PlantUML** | El clásico de UML como texto, en Java | Muy completo en UML (secuencia, estado, componentes). `!include` para la leyenda | GitHub no lo dibuja. Necesita Java. 2 s por render. Formas de datos limitadas (sin paralelogramo en componentes). No se adapta al modo oscuro |
| **Graphviz (DOT)** | El motor de grafos clásico | Muy maduro y estable. El diff del SVG es chico | GitHub no lo dibuja. Sin `include` ni diagramas de secuencia. Todo se ajusta a mano |

Dos alternativas que no se probaron en el piloto:

- **Structurizr DSL (C4):** modela la arquitectura una vez y genera varias vistas. Está
  pensado para sistemas de muchos servicios; en un solo ejecutable, sus dos primeros
  niveles casi no dicen nada (ver dimensión 5).
- **Una herramienta visual** (draw.io, Excalidraw): dibujo libre y bonito, pero el
  archivo no es un texto que Claude pueda editar con seguridad, y el diff no se puede
  revisar.

También existe la **combinación**: Mermaid por defecto, porque GitHub lo muestra solo, y
D2 para los diagramas que necesiten importar un ejemplo generado o un layout más
cuidado.

### 2. Dónde se ven

| Opción | Cómo | Para este proyecto |
|---|---|---|
| **Directo en GitHub** | Mermaid dentro del Markdown | Lo ve cualquiera que abra el repo, sin hacer nada |
| **SVG generado y commiteado** | El autor o Claude lo regeneran con un comando, y una CI comprueba que el SVG coincide con el texto (las herramientas son deterministas, sección 4 de la investigación) | Funciona con cualquier formato, a costa de un paso más y un archivo derivado en git |
| **Un sitio de documentación** | Por ejemplo, GitHub Pages con un generador de sitios | Navegación y búsqueda mejores. Es otra pieza que mantener; puede llegar más adelante sin cambiar el formato |

### 3. Tipos de diagrama

Cada tipo responde una pregunta distinta:

| Tipo | Pregunta que responde | Ejemplo en este engine |
|---|---|---|
| **Flujo de datos** | ¿Qué dato crea cada paso y quién lo consume? | El corte: distancias → puntos de corte → triángulos → contornos → tapa → pedazos |
| **Secuencia** | ¿Quién llama a quién, y en qué orden? | Un paso de simulación: combate → cola `slice_request` → sistema de corte → render |
| **Estado** | ¿En qué estados puede estar algo y qué lo hace cambiar? | La hoja que se atasca y se saca con energía (ADR-0007) |
| **Layout de memoria** | ¿Cómo están los bytes? | Un vértice de 4 pesos `unorm8` (ADR-0007), las arenas del paso (ADR-0010) |
| **Estructura** | ¿Qué módulo depende de cuál? | Las capas `platform → core → gpu → sistemas → game` (ADR-0012) |

Las opciones son usar **todos según la pregunta**, con una plantilla por tipo, o
**limitarse a uno o dos** (por ejemplo, solo flujo y secuencia) para que haya menos que
aprender y mantener.

### 4. Leyenda común

| Opción | Qué es | A favor | En contra |
|---|---|---|---|
| **Leyenda propia** | La del piloto: función = rectángulo redondeado; dato = paralelogramo; cola = cilindro horizontal; GPU = cilindro; ejemplo = nota; flecha punteada = dato que entra o sale. El color repite lo que dice la forma | Hecha para un engine: distingue CPU, GPU, colas y datos, que es lo que importa aquí | Hay que documentarla y hacerla cumplir |
| **C4** (Simon Brown) | Notación libre, con reglas: todo diagrama con título y leyenda; todo elemento con su tipo; cada línea con una sola dirección y una etiqueta específica | Reconocida en la industria; sus reglas sirven aunque no se usen sus niveles | No define formas para datos, colas ni GPU |
| **UML parcial** | Solo secuencia y estado, con su notación estándar | Estándar universal: un evaluador lo lee sin leyenda | No cubre el flujo de datos ni el layout de memoria |

Y cómo se impone:

- **un archivo de leyenda importado** (D2, PlantUML): cambiar la leyenda cambia todos
  los diagramas;
- **un bloque repetido** en cada diagrama (Mermaid, DOT), más un chequeo que lo compare
  con el bloque canónico;
- **solo revisión**: el autor y la auditoría lo miran.

### 5. Niveles, ubicación y navegación

**Los niveles.** El C4 tiene cuatro: contexto, contenedores, componentes y código. Fue
pensado para sistemas de varios servicios. En un juego de un solo ejecutable, el
contexto es "el jugador y el juego", y los contenedores son "el ejecutable y sus
assets": casi no dicen nada. Lo que el ticket pide es otra escala, **temporal y de
datos**:

| Nivel | Qué muestra | Ejemplo |
|---|---|---|
| **Frame** | El frame completo: pasos de simulación, render, presentación | El acumulador de paso fijo y los 8 pasos máximos (ADR-0012) |
| **Sistema** | Un sistema dentro del paso: qué lee, qué escribe, qué colas usa | El sistema de corte |
| **Proceso** | Un proceso de un sistema, función por función, con un ejemplo | Cortar una malla (el piloto) |
| **Función** | Una función difícil, paso a paso | La clasificación con desempate simbólico (ADR-0007) |

El C4 mismo dice que su nivel de código **no conviene dibujarlo a mano** en documentación
de vida larga, porque las herramientas lo generan cuando se pide. Las opciones son los
**cuatro niveles completos**, o **frame, sistema y proceso**, con el nivel de función solo
para las funciones donde el código no alcanza.

**Dónde viven:**

| Opción | Ruta | A favor | En contra |
|---|---|---|---|
| **En la carpeta del área** | `docs/slicing/diagramas/`, `docs/graphics/diagramas/`… | Sigue la estructura de `docs/` (las áreas grandes llevan su carpeta) | Lejos del código que describen |
| **Junto al código** | `engine/slicing/README.md` | Quien cambia el código lo ve al lado | Mezcla documentación con código fuente |
| **Una carpeta única** | `docs/diagramas/` | Un solo lugar para navegar de lo macro a lo micro | Rompe la organización por área |

**Navegación:** un diagrama índice por nivel, y debajo de cada diagrama, links de
Markdown al nivel de arriba y a los de abajo. Los enlaces dentro del dibujo no se
probaron, y Mermaid los desactiva por seguridad.

### 6. Fidelidad con el código

| Opción | Qué se hace | Qué detecta | Costo |
|---|---|---|---|
| **Disciplina** | El diagrama se actualiza en el mismo PR que cambia el código. La plantilla de PR lo pregunta y la auditoría (`/auditoria-adr`) lo revisa | Lo que el autor y el auditor vean | Cero herramientas |
| **+ verificador de nombres** | Un chequeo de CI: cada `nombre()` de un diagrama tiene que estar definido en el código. Prototipo en la investigación: 50 líneas, 0,17 s | Funciones renombradas o borradas | Un script; usar un índice de símbolos real (clangd o universal-ctags) |
| **+ partes generadas** | Lo que el código o el build ya saben se genera: las dependencias entre módulos (`cmake --graphviz`), el orden de los sistemas del paso | Todo lo generado, por construcción | Un generador por tipo de dato; solo sirve para lo estructural |
| **+ SVG verificado** | Si hay SVG commiteado, la CI lo regenera y falla si no coincide | Texto cambiado sin regenerar | Solo aplica a formatos con SVG |

Las capas se suman: ninguna detecta un orden equivocado o una flecha que falta, eso
siempre queda para la revisión.

### 7. Ejemplos concretos

| Opción | Cómo | A favor | En contra |
|---|---|---|---|
| **Escritos a mano** | Los valores van en el texto del diagrama | Simple, en cualquier formato | Pueden quedar mal sin que nadie lo note |
| **Sacados de un test** | Un test del ADR-0011 corre el caso (el triángulo contra el plano) y escribe los valores en un fragmento que el diagrama importa. Probado con D2 | Verdadero por construcción; si el algoritmo cambia, el ejemplo cambia | Solo con formatos que importan (D2, PlantUML), o con un script que reescriba el Markdown (Mermaid) |
| **Tomados de una ejecución** | Se registra lo que pasó en una ejecución real y se dibuja | Muestra el comportamiento real | Depende de las herramientas de debug (ADR-0021, pendiente) |

### 8. Cuándo y quién

| Opción | Cuándo es obligatorio un diagrama |
|---|---|
| **En los sistemas grandes** | En los tickets `modo:dirigido` (renderer, corte, física, animación, IA) |
| **En todo cambio de un flujo dibujado** | Todo PR que cambia un proceso que ya tiene diagrama lo actualiza, y todo sistema nuevo nace con el suyo |
| **Opcional** | Cuando el autor lo pida |

Y cómo encaja con el proceso: si el diagrama entra en la **definición de Hecho** del
ticket, y si la sección **"Qué debo poder explicar"** se responde recorriendo el
diagrama. Los roles siguen el ADR-0014: Claude dibuja y actualiza; el autor revisa y
pide cambios.

## Investigación

Mediciones completas en [`research/diagramas.md`](../research/diagramas.md). Resumen:

### El piloto

Un solo diagrama teórico, el del corte, en cuatro formatos, subido a la rama y capturado
en GitHub en tema claro y oscuro:

- **Solo Mermaid se ve directo en GitHub.** Los demás necesitan un SVG commiteado.
- **Modo oscuro:** D2 se adapta solo. PlantUML y Graphviz quedan como un recuadro blanco
  legible. Mermaid falló con una forma sin relleno y quedó bien al darle relleno: todo
  elemento necesita su propio fondo.
- **Leyenda:** D2 y PlantUML la importan desde un archivo; Mermaid y DOT la repiten.
  PlantUML y Graphviz no tienen todas las formas de la leyenda.
- **Render:** D2 0,10 s, Graphviz 0,14 s, PlantUML 2,07 s (la JVM).
- **Determinismo:** las tres herramientas con SVG dan bytes idénticos al repetir el
  render.
- **Diff:** renombrar una función cambia 2 líneas del texto en todos los formatos, pero
  reescribe entero el SVG de D2 y el de PlantUML. El SVG no se revisa; se revisa el
  texto.
- **Verificador de nombres:** detectó la función renombrada en los cuatro formatos.
- **Ejemplo generado:** un programa C++ calculó el corte del triángulo y D2 importó los
  valores en las notas del diagrama.

### "Docs as code" y la brecha entre modelo y código

- **Docs as code:** la documentación se escribe como texto, vive en git y se revisa en
  el mismo PR que el código. Es la respuesta habitual a los diagramas desactualizados,
  porque el cambio y su documentación se revisan juntos.
- **George Fairbanks** (*Just Enough Software Architecture*) llama **brecha entre modelo
  y código** (*model-code gap*) a la distancia entre cómo se razona un sistema (componentes,
  capas, flujos) y cómo está escrito (funciones, archivos). Un diagrama siempre abstrae,
  así que la brecha no desaparece. Lo que se puede hacer es **hacerla visible**, nombrando
  en el diagrama funciones y datos reales.
- **Cyrille Martraire** (*Living Documentation*) propone **diagramas vivos**: se generan
  desde el código, así que no pueden quedar desactualizados. Sirve para lo que el código
  ya sabe (dependencias, orden de registro), no para explicar el porqué de un flujo.

### El modelo C4

Simon Brown separa los diagramas en niveles de zoom (contexto, contenedores, componentes,
código), más diagramas de apoyo, como el **dinámico** (cómo colaboran los elementos en
tiempo de ejecución, con interacciones numeradas). Sus reglas de notación valen para
cualquier diagrama: título y leyenda siempre, tipo explícito en cada elemento, líneas
de una sola dirección y con etiquetas específicas, y colores consistentes que no sean la
única forma de distinguir (daltonismo, impresión en blanco y negro). C4 recomienda
**no** dibujar a mano el nivel de código en documentación de vida larga.

### Cómo explica la industria un engine

- **Adrian Courrèges**, *GTA V Graphics Study* (2015): disecciona **un frame** pasada por
  pasada, con las imágenes intermedias reales. Es un ejemplo de cómo un dato concreto que
  recorre el proceso explica más que una caja con un nombre.
- **Fabien Sanglard**, revisiones de código de los engines de id Software (por ejemplo
  Doom 3, 2012) y sus *Game Engine Black Books*: recorren el código real con diagramas
  del flujo del frame y de las estructuras de datos.

## Decisión

<!-- Lo escribe el autor: qué se elige y por qué, con sus palabras. -->

## Consecuencias

<!-- Claude y autor: qué se gana, qué se sacrifica y qué queda abierto. -->

## Cómo lo defiendo

<!-- Lo escribe el autor: preguntas que haría un evaluador, respondidas con sus palabras. -->

## Referencias

- GitHub Docs, *Creating diagrams*:
  <https://docs.github.com/en/get-started/writing-on-github/working-with-advanced-formatting/creating-diagrams>
- GitHub Changelog, *Mermaid, topoJSON, geoJSON and ASCII STL diagrams are now supported
  in Markdown and as files* (2022):
  <https://github.blog/changelog/2022-03-17-mermaid-topojson-geojson-and-ascii-stl-diagrams-are-now-supported-in-markdown-and-as-files/>
- Mermaid, *Usage* (`securityLevel`): <https://mermaid.js.org/config/usage>
- Mermaid, *Theme configuration*: <https://mermaid.js.org/config/theming.html>
- D2: <https://github.com/terrastruct/d2>
- PlantUML: <https://plantuml.com/>
- Graphviz: <https://graphviz.org/>
- Simon Brown, *The C4 model*: <https://c4model.com/>, con *Notation*
  (<https://c4model.com/diagrams/notation>), *Code diagram*
  (<https://c4model.com/diagrams/code>) y *Dynamic diagram*
  (<https://c4model.com/diagrams/dynamic>)
- George Fairbanks, *Just Enough Software Architecture* (2010), y la brecha entre modelo
  y código resumida en: <https://icepanel.io/blog/2022-11-30-the-model-code-gap>
- Cyrille Martraire, *Living Documentation: Continuous Knowledge Sharing by Design*
  (Addison-Wesley, 2019): <https://www.oreilly.com/library/view/living-documentation-continuous/9780134689418>
- Adrian Courrèges, *GTA V – Graphics Study* (2015):
  <https://www.adriancourreges.com/blog/2015/11/02/gta-v-graphics-study/>
- Fabien Sanglard, *Doom 3 Source Code Review* (2012): <https://fabiensanglard.net/doom3/>
- CMake, `--graphviz`: <https://cmake.org/cmake/help/latest/module/CMakeGraphVizOptions.html>
