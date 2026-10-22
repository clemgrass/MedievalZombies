# Piloto de diagramas (ADR-0030, #60)

Material de investigación del ADR-0030: **un solo diagrama, el del corte de una malla**,
dibujado en cuatro formatos para compararlos en GitHub.

> **Diagrama teórico, guardado como historial.** Cuando se hizo no existía código: los
> nombres de funciones y datos son ilustrativos (salen del ADR-0007) y no son una API
> decidida. El diagrama real del corte nace con su código, en `docs/slicing/diagramas/`.

## El ejemplo que recorre el diagrama

- Plano del corte: `y = 1`, normal `n = (0, 1, 0)`.
- Triángulo: `v0 = (0, 0, 0)`, `v1 = (2, 2, 0)`, `v2 = (0, 2, 0)`.
- Distancias con signo: `d = (−1, +1, +1)`.
- Cruzan el plano las aristas `v0–v1` y `v0–v2`, con `t = 0,5`:
  `p01 = (1, 1, 0)` y `p02 = (0, 1, 0)`.
- Resultado: 1 triángulo debajo (`v0, p01, p02`), 2 encima (`v1, v2, p02` y
  `v1, p02, p01`) y 1 segmento de contorno (`p01–p02`) para la tapa.

## Leyenda de prueba

| Elemento | Forma | Color (repite lo que dice la forma) |
|---|---|---|
| Función | rectángulo redondeado, con su número de orden y `()` | azul |
| Dato en la CPU | paralelogramo | gris |
| Cola de eventos | cilindro horizontal | naranjo |
| Recurso de la GPU | cilindro | verde |
| Ejemplo concreto | nota | amarillo |
| Dato que entra o sale de una función | flecha punteada | — |
| Ejemplo de un dato | línea sin flecha | — |

## Mermaid

GitHub lo renderiza directamente desde este texto.

```mermaid
flowchart TB
    classDef func fill:#dbeafe,stroke:#1d4ed8,color:#0b1b3a
    classDef data fill:#f1f5f9,stroke:#475569,color:#0f172a
    classDef queue fill:#ffedd5,stroke:#c2410c,color:#431407
    classDef gpu fill:#dcfce7,stroke:#15803d,color:#052e16
    classDef example fill:#fef9c3,stroke:#a16207,color:#422006

    req@{ shape: h-cyl, label: "slice_request<br/>plano + pose" }
    mesh[/"malla<br/>vértices + índice soldado"/]
    f1("1. classify_vertices()")
    d1[/"distancia con signo<br/>por vértice soldado"/]
    f2("2. split_edges()")
    d2[/"puntos de corte<br/>uno por arista soldada"/]
    f3("3. split_triangles()")
    d3[/"triángulos de cada lado<br/>+ segmentos de contorno"/]
    f4("4. build_contours()")
    d4[/"contornos cerrados"/]
    f5("5. triangulate_cap()")
    d5[/"triángulos de la tapa"/]
    f6("6. build_pieces()")
    d6[/"pedazos"/]
    gpu[("índices por pedazo<br/>+ vértices nuevos")]

    e1@{ shape: doc, label: "d = (−1, +1, +1)" }
    e2@{ shape: doc, label: "p01 = (1, 1, 0), p02 = (0, 1, 0), t = 0,5" }
    e3@{ shape: doc, label: "debajo: 1 triángulo; encima: 2; contorno: p01–p02" }

    req -.-> f1
    mesh -.-> f1
    f1 -.-> d1
    d1 -.-> f2
    f2 -.-> d2
    d2 -.-> f3
    f3 -.-> d3
    d3 -.-> f4
    f4 -.-> d4
    d4 -.-> f5
    f5 -.-> d5
    d3 -.-> f6
    d5 -.-> f6
    f6 -.-> d6
    d6 -.-> gpu
    d1 --- e1
    d2 --- e2
    d3 --- e3

    class f1,f2,f3,f4,f5,f6 func
    class mesh,d1,d2,d3,d4,d5,d6 data
    class req queue
    class gpu gpu
    class e1,e2,e3 example
```

## D2

Fuente: [`corte.d2`](corte.d2), con la leyenda importada de [`leyenda.d2`](leyenda.d2).

![Corte en D2](corte-d2.svg)

## PlantUML

Fuente: [`corte.puml`](corte.puml), con la leyenda incluida de [`leyenda.iuml`](leyenda.iuml).

![Corte en PlantUML](corte-plantuml.svg)

## Graphviz (DOT)

Fuente: [`corte.dot`](corte.dot). DOT no tiene forma de importar estilos: la leyenda se
repite en cada archivo.

![Corte en Graphviz](corte-graphviz.svg)
