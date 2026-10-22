# Piloto de los diagramas técnicos

Nota de investigación del [ADR-0030](../adr/0030-diagramas-tecnicos.md). Guarda lo que
se midió con el piloto, para poder repetirlo.

Fecha: 2026-10-21. El piloto es **un solo diagrama teórico, el del corte de una malla**
(nivel de proceso, con el triángulo de ejemplo recorriéndolo), dibujado en cuatro
formatos: [`diagramas-piloto/`](diagramas-piloto/README.md). Todavía no existe código,
así que los nombres de funciones son ilustrativos. Cuando exista el código del corte,
ese diagrama se reemplaza por uno fiel a él.

Herramientas usadas, todas instaladas fuera del sistema (en una carpeta temporal):

| Herramienta | Versión | Cómo se ejecutó |
|---|---|---|
| D2 | 0.9.0, layout ELK | binario oficial |
| PlantUML | 1.2026.9, layout Smetana | `.jar` sobre Java 17. Smetana es el layout interno: evita instalar Graphviz |
| Graphviz | 16.1.0 | compilado a WebAssembly (`@viz-js/viz`), desde Node |
| Mermaid | la que usa GitHub | sin instalar nada: lo dibuja GitHub |

El render en GitHub se capturó con un Chromium sin ventana (Playwright), en tema claro y
oscuro, sobre la rama `60-adr-0030` subida al repo público.

## 1. Cómo se ve en GitHub

| Formato | Qué se commitea | Tema claro | Tema oscuro |
|---|---|---|---|
| Mermaid | solo el texto | bien | **bien después de un arreglo:** una forma sin relleno (`braces`) dejó su texto oscuro sobre el fondo oscuro, ilegible. Con una forma con relleno (`doc`) se lee bien. Regla: todo elemento lleva su propio relleno |
| D2 | texto + SVG | bien | **bien, sin hacer nada:** con `--dark-theme`, el SVG trae los dos temas y cambia según el sistema |
| PlantUML | texto + SVG | bien | se ve como un recuadro blanco: legible, pero no se adapta |
| Graphviz | texto + SVG | bien | se ve como un recuadro blanco: legible, pero no se adapta |

GitHub renderiza **solo Mermaid** desde el texto (también GeoJSON, TopoJSON y STL, que no
sirven aquí). Las formas nuevas de Mermaid 11 (`h-cyl`, `doc`, la sintaxis `@{ shape }`)
funcionan en GitHub. D2, PlantUML y Graphviz necesitan un SVG generado y commiteado.

**Los enlaces dentro de un diagrama no se probaron.** Mermaid desactiva el `click` con
su nivel de seguridad `strict`, que es el que viene por defecto, y GitHub lo dibuja dentro
de un `iframe` aislado. Lo seguro es enlazar los niveles con links de Markdown junto al
diagrama, no dentro de él.

## 2. La leyenda: cuánto se puede imponer

| Formato | Cómo se comparte la leyenda | Formas de la leyenda del piloto |
|---|---|---|
| Mermaid | **No hay `include`.** Cada diagrama repite el bloque de `classDef` | todas |
| D2 | **Un archivo `leyenda.d2`** con `classes`, importado con `...@leyenda` | todas, incluida la cola (`shape: queue`) |
| PlantUML | **`!include leyenda.iuml`**, con estilos por estereotipo | sin paralelogramo en los diagramas de componentes: el dato quedó como `card` (rectángulo) |
| Graphviz | **No hay `include`.** Cada archivo repite los estilos | sin forma de cola: quedó como `cds` (una etiqueta con punta) |

En Mermaid, que la leyenda no se desvíe depende de copiar bien el bloque, o de un
chequeo automático que compare el bloque de cada diagrama con el canónico.

## 3. Tiempo de render y dependencias

| Formato | Render del diagrama del corte | Qué hace falta instalar |
|---|---|---|
| Mermaid | 0 (lo hace GitHub) | nada para verlo. Para validarlo en una CI: `mermaid-cli`, que trae un Chromium |
| D2 | 0,10 s | un binario de Go (licencia MPL-2.0) |
| Graphviz | 0,14 s | Graphviz (paquete del sistema) o su versión WebAssembly |
| PlantUML | 2,07 s, casi todo el arranque de la JVM | Java, y Graphviz si no se usa Smetana (licencia GPL) |

La licencia de la herramienta no afecta a los diagramas: el SVG generado no hereda la
licencia del programa que lo generó, y los diagramas son documentación bajo CC BY 4.0
(ADR-0019).

## 4. Determinismo y diffs

**Determinismo:** las tres herramientas que generan SVG dieron **bytes idénticos** al
volver a renderizar el mismo archivo. Eso permite un chequeo de CI: regenerar el SVG y
fallar si no coincide con el commiteado (alguien cambió el texto y no regeneró).

**Diff al renombrar una función** (`split_edges` → `split_crossing_edges`):

| Formato | Líneas cambiadas en el texto | Líneas cambiadas en el SVG |
|---|---|---|
| Mermaid | 2 | no hay SVG |
| D2 | 2 | 158, sobre 129: el SVG entero cambia, porque se mueve el layout |
| PlantUML | 2 | 138, sobre 79: el SVG entero |
| Graphviz | 2 | 6, sobre 251 |

(Líneas del SVG cortadas a 200 columnas, porque D2 y PlantUML escriben casi todo en una
sola línea.) **El diff del SVG no se puede revisar**: la revisión se hace siempre sobre
el texto, y el SVG es un derivado.

## 5. Verificador de nombres

**Qué se probó:** un script de 50 líneas en Python que saca cada `nombre()` de los
diagramas y comprueba que esté **definido** en el código (un `nombre(...) {`, sin contar
comentarios). Se corrió sobre los cuatro diagramas del piloto y sobre un archivo C++ de
juguete donde `split_edges` ya se llamaba `split_crossing_edges`.

**Resultado:** detectó el nombre viejo en los cuatro formatos, en 0,17 s, y salió con
código 1, como lo necesita una CI. Una mención de `build_contours()` dentro de un
comentario no contó como definición.

**Qué no detecta:** un orden equivocado, una flecha que falta o un dato que ya no se
produce. Solo detecta nombres que dejaron de existir. Lo demás queda para la revisión del
autor y la auditoría. La versión real usaría un índice de símbolos (clangd o
universal-ctags) en vez de una expresión regular, porque una regex se confunde con
macros, plantillas y declaraciones de varias líneas.

## 6. Ejemplo generado por el código

**Qué se probó:** un programa C++ de 30 líneas que hace el corte del triángulo de ejemplo
y escribe los valores en un fragmento D2 (`e1.label: "d = (-1, +1, +1)"`, etc.). El
diagrama lo importa al final con `...@ejemplo`, y las etiquetas de las notas se
reemplazan por las calculadas.

**Resultado:** funciona. Las notas del SVG muestran los valores que calculó el programa.
Con código real, ese programa es un **test** del ADR-0011: el ejemplo del diagrama pasa a
ser verdadero por construcción, y si el algoritmo cambia, el diagrama cambia con él.

| Formato | ¿Puede importar un fragmento generado? |
|---|---|
| D2 | sí, con imports (probado) |
| PlantUML | sí, con `!include` (no probado) |
| Mermaid | no: haría falta un script que reescriba el bloque dentro del Markdown |
| Graphviz | no de forma nativa: haría falta un preprocesador |

## 7. Otras observaciones

- **Layout:** D2 con ELK dio el diagrama más ordenado y las notas mejor ubicadas. Mermaid
  parte las etiquetas en líneas muy cortas. PlantUML dio el más compacto. Graphviz dio
  paralelogramos muy anchos.
- **Graphviz no tiene diagramas de secuencia.** Mermaid, D2 y PlantUML sí.
- **CMake genera el grafo de dependencias entre targets** (`cmake --graphviz=deps.dot`).
  Con las capas del ADR-0012 (un target por módulo), es un diagrama de módulos generado
  desde el build, sin dibujarlo a mano.
