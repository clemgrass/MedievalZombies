# Diagramas técnicos

Diagramas que muestran cómo funciona el engine **tal como está construido**: qué
funciones recorre un dato, en qué orden, qué crea cada paso y con qué valores. Las
reglas vienen del [ADR-0030](../adr/0030-diagramas-tecnicos.md).

Todavía no hay diagramas: el primero llega con el esqueleto andante (ADR-0014). Un
sistema nace con su diagrama en el primer PR que le da comportamiento. El piloto de la
investigación quedó como historial en
[`research/diagramas-piloto/`](../research/diagramas-piloto/README.md).

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
| Ejemplo concreto | página | `example` | amarillo |
| Dato que entra o sale de una función | flecha punteada | `flow` | — |
| Ejemplo de un dato | línea sin flecha | `note` | — |

## Reglas de cada diagrama

- **Título** con el tipo y el alcance (por ejemplo, "Corte de una malla, nivel de
  proceso").
- **Líneas de una sola dirección**, con etiquetas específicas cuando la flecha no se
  explica sola.
- **Todo elemento lleva su propio relleno**, para que se lea en tema claro y oscuro.
- **El id de cada dato es el nombre de su tipo** en `snake_case` (`signed_distances`
  para `SignedDistances`), y su etiqueta lo describe en español. Un tipo por dato.
- **Las funciones se nombran como en el código**, con `()`.
- **El `.d2` es la fuente y el SVG, su derivado.** Se revisa el `.d2`; el SVG se
  regenera con:

  ```sh
  d2 --layout elk --dark-theme 200 <diagrama>.d2 <diagrama>.svg
  ```

Los verificadores automáticos (nombres, SVG, leyenda, links, ejemplos y forma del flujo)
están en la Decisión del ADR-0030.
