# ADR-0030: Diagramas técnicos del engine

- **Estado:** Aceptado
- **Fecha:** 2026-10-21
- **Ticket:** #60
- **Reemplaza / Reemplazado por:** —
- **Fe de erratas:** 2026-10-28 (#43): el ADR-0021 decide cómo se activa la macro de
  traza del verificador (h). Es la misma macro de zona del profiling, definida en el
  preset Debug (la primera de las tres salidas). En RelWithDebInfo alimenta a Tracy, y
  la traza no está. La macro tiene dos variantes: una de zona y traza para las funciones
  normales, y otra solo de traza para las que corren por elemento (por vértice, por
  triángulo), vacía en RelWithDebInfo. Las dos anotan la función para el verificador.
  Se corrige en la Decisión 6 (h), en Consecuencias ("Una convención y una macro más en
  el código") y en "Lo que queda abierto".

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
esqueleto andante (ADR-0014). La investigación usó un piloto teórico, el del corte de una
malla ([`diagramas-piloto/`](../research/diagramas-piloto/README.md)). Queda en
`research/` como historial, y el diagrama real del corte nace con su código.

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
| **Flujo de datos** | ¿Qué dato crea cada paso y quién lo consume? | El corte (ADR-0007): pose → distancias → contornos → triángulos partidos → tapas → pedazos |
| **Secuencia** | ¿Quién llama a quién, y en qué orden? | Un frame: combate → cola `slice_request` → sistema de corte, dentro del paso; después de los pasos, el render |
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
| **+ forma del flujo** | Se observa quién llama a quién: el **orden estático** (las llamadas en el AST de Clang), los **tipos** (cada dato del diagrama tiene el nombre de su tipo, y se comparan con las firmas) y el **orden dinámico** (una traza de la ejecución del test). Prototipo en la investigación, sección 7 | Pasos invertidos, una flecha de datos equivocada y, con la misma lista de llamadas, un paso nuevo que no se dibujó | Una convención (un tipo por dato) y una macro de traza en las funciones dibujadas, activa solo donde corren los tests |

Las capas se suman. Ninguna ve lo que pasa dentro de una función, una rama que el test
no recorre ni si una etiqueta describe bien su dato: eso siempre queda para la revisión.

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
- **Forma del flujo:** sobre un C++ de juguete, el orden estático (AST de Clang) y el
  orden dinámico (una traza del test) detectaron dos pasos invertidos, y la comparación
  de tipos detectó una flecha de datos equivocada.
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

Elijo D2 porque calza con todos los tipos de tests y verificaciones que queremos hacer,
y además me gusta cómo se ve.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Formato | **D2, como único formato**, con la versión fijada y el layout ELK |
| 2 | Dónde se ven | **SVG generado y commiteado** junto a su `.d2`, con el mismo nombre, con los temas claro y oscuro dentro (`--dark-theme`). Un chequeo regenera cada SVG y falla si no coincide con el commiteado |
| 3 | Tipos de diagrama | **Flujo de datos y secuencia** como base. **Estado y layout de memoria** solo cuando un sistema los pida. La **estructura de módulos se genera** desde el build (`cmake --graphviz`), convertida a D2 con la misma leyenda |
| 4 | Leyenda | **La leyenda propia del piloto, más dos clases:** el **sistema del ECS** (un contenedor que agrupa sus funciones) y el **módulo**, el target de CMake (un rectángulo con doble borde). Se suman las reglas de notación del C4: cada diagrama con título; cada elemento con su tipo; líneas de una sola dirección, con etiquetas específicas cuando la flecha no se explica sola (en un flujo función → dato → función, la explica el dato); el color repite lo que dice la forma, y todo elemento lleva su propio relleno. Vive en un solo archivo, `docs/diagramas/leyenda.d2`, que todos los diagramas importan. **El id de cada dato es el nombre de su tipo** (`signed_distances`, en `snake_case` como todo tipo, ADR-0013), y su etiqueta lo describe en español: un tipo por dato |
| 5 | Niveles y ubicación | **Frame, sistema y proceso.** El nivel de función, solo donde el código no alcance para entenderla. Los diagramas transversales (el frame) van en `docs/diagramas/`; los de cada área, en `docs/<área>/diagramas/`. Cada carpeta tiene un `README.md` que muestra sus SVG y enlaza al nivel de arriba y a los de abajo |
| 6 | Fidelidad | **El diagrama se actualiza en el mismo PR que cambia el código**, la plantilla de PR lo pregunta y la auditoría lo revisa. Además, **verificadores automáticos**, que corren con un solo comando y en la CI (ADR-0029). Sobre el diagrama: (a) cada `nombre()` está definido en el código, según un índice de símbolos; (b) cada SVG coincide con su `.d2` regenerado; (c) cada diagrama importa la leyenda; (d) los links entre niveles llevan a un archivo que existe; (e) cada fragmento de ejemplo coincide con el que escribe su test. Sobre la **forma del flujo**: (f) **orden estático**, las llamadas dentro de la función del proceso, leídas del AST de Clang, siguen el orden numerado; (g) **tipos**, cada flecha función → dato es el tipo que devuelve la función, y cada flecha dato → función, uno de sus parámetros; (h) **orden dinámico**, la traza del test del ejemplo sigue el mismo orden, con una macro que anota cada función dibujada; la traza se activa solo donde corren los tests y nunca en RelWithDebInfo ni en Release (en RelWithDebInfo, la misma macro es una zona de Tracy, salvo su variante para las funciones que corren por elemento, que queda vacía, ADR-0021); (i) **paso nuevo**, la función del proceso no llama a ninguna función del proyecto que el diagrama no tenga |
| 7 | Ejemplos | **En los diagramas de proceso, calculados por un test** (ADR-0011). El test escribe un fragmento `<diagrama>.ejemplo.d2`, y el diagrama lo importa. En los demás niveles, el ejemplo se escribe a mano |
| 8 | Cuándo y quién | **Un sistema nace con su diagrama en el primer PR que le da comportamiento** (un sistema vacío no lleva diagrama). Desde ahí, todo PR que cambia un flujo dibujado actualiza su diagrama. El diagrama al día entra en la **definición de Hecho**, y **"Qué debo poder explicar" se responde recorriendo el diagrama**. Claude dibuja y actualiza; el autor revisa y pide cambios (ADR-0014) |

## Consecuencias

### Lo que se gana

- **Cada proceso se puede explicar con un dibujo que es verdad:** nombra funciones
  reales, sigue el orden real y muestra valores calculados por el código.
- **Un diagrama desactualizado rompe el chequeo**, en vez de engañar en silencio. Los
  verificadores cubren nombres, SVG, leyenda, links y ejemplos, y también la forma del
  flujo: el orden de las llamadas, en el código y en la ejecución, los tipos de los
  datos y los pasos nuevos.
- **La misma leyenda en todos los diagramas**, porque la importan desde un solo archivo.

### Lo que se sacrifica

- **GitHub no dibuja D2 desde el texto:** cada cambio necesita regenerar el SVG. Si se
  olvida, el chequeo (b) lo detecta.
- **El diff del SVG no se puede revisar:** un cambio de una línea reescribe el SVG
  entero. La revisión se hace sobre el `.d2`.
- **Una herramienta más:** D2 (licencia MPL-2.0) se suma al toolchain de la
  documentación. El engine no la enlaza, y los SVG que genera son documentación bajo
  CC BY 4.0 (ADR-0019).
- **Una convención y una macro más en el código:** cada dato dibujado necesita su propio
  tipo, y cada función dibujada lleva la macro de traza. Está vacía en Release. En
  RelWithDebInfo es una zona de Tracy y la traza no está (ADR-0021), así que no altera
  las mediciones de rendimiento del ADR-0020 más allá del costo medido de una zona. Las
  funciones que corren por elemento (por vértice, por triángulo) usan la variante solo de
  traza, vacía también en RelWithDebInfo.
- **Lo que ningún verificador ve:** lo que pasa dentro de una función, si una etiqueta
  describe bien su dato, una rama que el test no recorre y qué instancia de un dato
  viaja. Eso queda para la revisión del autor y la auditoría.

### Cambios en el repo

- **`docs/diagramas/`**, nueva: la leyenda (`leyenda.d2`) y un `README.md` con los
  niveles, la leyenda y las reglas de cada diagrama. Ahí irá el diagrama del frame.
- **`docs/README.md`:** `diagramas/` en el mapa, la carpeta `diagramas/` de cada área y
  la fila de este ADR en el índice.
- **`.github/pull_request_template.md`:** la lista de revisión pregunta por los
  diagramas.
- **`docs/process/tickets.md`:** la definición de Hecho incluye el diagrama al día, y
  "Qué debo poder explicar" se responde recorriéndolo. Los formularios de feature y de
  tarea lo dicen en ese campo.
- **`CLAUDE.md`:** `docs/` incluye los diagramas.
- **Auditoría:** tema nuevo `diagramas` en `.claude/skills/auditoria-adr/references/temas.md`.
- **El piloto** queda en `docs/research/diagramas-piloto/` como historial, en sus cuatro
  formatos. El diagrama real del corte nace con su código, en `docs/slicing/diagramas/`.
- **Los verificadores y el comando que los corre** llegan con el primer código: no hay
  nada que verificar todavía.

### Lo que queda abierto

- **Cómo se instala D2** en la máquina del autor y en la CI, y cómo se fija su versión
  (ADR-0006, ADR-0029).
- **El índice de símbolos del verificador (a):** clangd o universal-ctags. Se decide
  cuando exista el código.
- **La macro de traza del verificador (h):** su nombre (en mayúsculas, ADR-0013), dónde
  vive y cómo se activa donde corren los tests. Eso todavía no es una configuración: hay
  tres presets (ADR-0006), y los tests corren en Debug (ADR-0011). Hay tres salidas:
  una definición del preset Debug, que también la activa en el juego en Debug; una
  definición solo en los targets de test, que obliga a compilar el engine dos veces; o
  un preset nuevo, que cambia el ADR-0006 y, si ya existe código, necesita su propio
  ADR. **Decidido en el ADR-0021:** es la misma macro de zona del profiling, y se activa
  con una definición del preset Debug, la primera salida. En RelWithDebInfo esa macro
  alimenta a Tracy, no a la traza. Su nombre y dónde vive siguen abiertos.
- **Cómo escribe un test su fragmento de ejemplo:** se diseña con el primer diagrama de
  proceso, dentro de la estrategia de testing (ADR-0011).
- **Un sitio de documentación** (por ejemplo, GitHub Pages) puede llegar más adelante,
  sin cambiar el formato.

## Cómo lo defiendo

**¿Por qué D2 y no Mermaid, si GitHub dibuja Mermaid sin hacer nada?**
Por su mejor estética visual, y sobre todo porque calza con las verificaciones que
queremos: D2 importa la leyenda desde un solo archivo y el ejemplo que calcula un test.
Mermaid no puede importar nada.

**¿Cómo sabes que un diagrama no está desactualizado?**
Lo revisamos con diferentes tests, usando técnicas de *docs as code*.

**¿Por qué tus niveles son frame, sistema y proceso, y no los del modelo C4?**
Son más flexibles para lo que estamos haciendo. El C4 lo uso solo como inspiración.

**¿Qué ganas con que el ejemplo del diagrama lo calcule un test?**
Me aseguro de que el ejemplo es algo que ocurre realmente en un test.

**¿Qué errores de un diagrama no detectan tus verificadores, y cómo los cubres?**
Lo que realmente ocurre dentro de una función: podría cambiar y dejar de ser lo que se
entiende del flujo. Tampoco detectan una etiqueta que no describa bien un dato, una rama
que el test no recorre ni qué instancia de un dato viaja. Esos errores los cubro
revisando siempre manualmente.

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
