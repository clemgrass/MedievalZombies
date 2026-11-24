# Diagramas técnicos

Diagramas que muestran cómo funciona el engine **tal como está construido**: qué
funciones recorre un dato, en qué orden, qué crea cada paso y con qué valores. Las
reglas vienen del [ADR-0030](../adr/0030-diagramas-tecnicos.md).

Un sistema nace con su diagrama en el primer PR que le da comportamiento. Todavía no hay
un diagrama del frame completo: llega con el game loop. Los de cada área, el nivel de
abajo:

- [`graphics/diagramas/`](../graphics/diagramas/README.md): el frame mínimo de
  `engine/gpu` (adquirir, grabar, enviar y presentar).

El piloto de la investigación quedó como historial en
[`research/diagramas-piloto/`](../research/diagramas-piloto/README.md).

**Dos excepciones** usan D2 pero no son diagramas técnicos:

- **El mapa del castillo**, en [`docs/vision/mapa/`](../vision/mapa/README.md): dibuja
  zonas del juego, no funciones ni datos. Por eso importa su propia leyenda. De los
  verificadores del ADR-0030, le toca (b), que cada SVG coincida con su `.d2`
  regenerado, pero no (c), que importe esta leyenda, ni los que leen el código.
- **El diagrama teórico del proyecto**, en [`teorico/`](teorico/README.md): muestra lo
  que eligió cada ADR aceptado y cómo se conectan las piezas, no el código. Es teórico e
  importa su propia leyenda. Como al mapa, de los verificadores le toca solo (b). Se
  actualiza en el PR que acepta un ADR.

## Dónde vive cada diagrama

| Nivel | Qué muestra | Dónde |
|---|---|---|
| **Frame** | El frame completo: pasos de simulación, render, presentación | aquí, en `docs/diagramas/` |
| **Sistema** | Un sistema dentro del paso: qué lee, qué escribe, qué colas usa | `docs/<área>/diagramas/` |
| **Proceso** | Un proceso de un sistema, función por función, con un ejemplo calculado por un test | `docs/<área>/diagramas/` |
| **Función** | Solo para una función que el código no alcanza a explicar | `docs/<área>/diagramas/` |

Cada carpeta `diagramas/` tiene un `README.md` que muestra sus SVG y enlaza al nivel de
arriba y a los de abajo.

## Leyenda

Vive en [`leyenda.d2`](leyenda.d2), y todos los diagramas la importan.

| Elemento | Forma | Clase | Color (repite lo que dice la forma) |
|---|---|---|---|
| Función | rectángulo redondeado, con su número de orden y `()` | `func` | azul |
| Dato en la CPU | paralelogramo | `data` | gris |
| Cola de eventos | cola (cilindro horizontal) | `queue` | naranjo |
| Recurso de la GPU | cilindro | `gpu` | verde |
| Sistema del ECS | contenedor que agrupa sus funciones | `system` | violeta |
| Módulo (target de CMake) | rectángulo con doble borde | `module` | turquesa |
| Ejemplo concreto | página | `example` | amarillo |
| Dato que entra o sale de una función | flecha punteada | `flow` | — |
| Ejemplo de un dato | línea sin flecha | `note` | — |

## Reglas de cada diagrama

- **Título** con el tipo y el alcance (por ejemplo, "Corte de una malla, nivel de
  proceso").
- **Líneas de una sola dirección**, con etiquetas específicas cuando la flecha no se
  explica sola. En un flujo función → dato → función, la flecha la explica el dato.
- **Todo elemento lleva su propio relleno**, para que se lea en tema claro y oscuro.
- **El id de cada dato es el nombre de su tipo** (`signed_distances`), y su etiqueta lo
  describe en español. Un tipo por dato.
- **Las funciones se nombran como en el código**, con `()`.
- **El `.d2` es la fuente y el SVG, su derivado.** Se revisa el `.d2`; el SVG se
  regenera con:

  ```sh
  d2 --layout elk --dark-theme 200 <diagrama>.d2 <diagrama>.svg
  ```

Los verificadores automáticos (nombres, SVG, leyenda, links, ejemplos y forma del flujo)
están en la Decisión del ADR-0030.

## El ejemplo de un diagrama de proceso

Los números de un diagrama de proceso los calcula un test (ADR-0030), que escribe el
fragmento `<diagrama>.ejemplo.d2`. El diagrama lo importa con
`...@"<diagrama>.ejemplo.d2"` (entre comillas: sin ellas, D2 corta el nombre en el punto).

- **El test escribe en el build, nunca en el repo:** en una carpeta `diagramas/` dentro
  de la de su suite. CMake se la da con una definición, `MZ_DIAGRAM_EXAMPLES_DIR`, y con
  otra, `MZ_DIAGRAMS_DIR`, la carpeta del repo donde vive el diagrama.
- **El mismo test compara su fragmento con el del repo,** que solo lee. Si no coinciden,
  falla y dice qué copiar. Es el verificador (e) del ADR-0030, adelantado en cada test
  hasta que exista el verificador común.
- **Cuando el ejemplo cambia** (porque cambió el código, o el driver con el que corren
  los tests), se copia el fragmento del build a la carpeta del diagrama y se regenera el
  SVG.

El primero es el del [frame mínimo](../graphics/diagramas/README.md), que escribe
`FrameLoopTest.WritesTheDiagramExample`.
