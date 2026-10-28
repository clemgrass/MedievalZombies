# Documentación de MedievalZombies

MedievalZombies es un videojuego 3D sobre un apocalipsis zombie en la época medieval,
con engine propio. Su feature estrella es el corte de mallas en tiempo real: el tajo de
la espada corta de verdad al zombie. Aquí vive todo lo que explica **por qué** el
proyecto es como es: decisiones, investigación, diseño del juego y forma de trabajo.

## Mapa

```
docs/
├── README.md        este archivo: mapa e índice de todos los ADRs
├── vision/          diseño del juego: premisa, pilares, alcance, enemigos
├── process/         cómo trabajamos: tickets, convenciones git
├── specs/           specs aprobadas
├── plans/           planes de implementación derivados de las specs
├── adr/             ADRs transversales y plantilla (template.md)
├── research/        notas de investigación de los ADRs transversales
├── diagramas/       diagramas técnicos transversales (el frame) y la leyenda común
├── graphics/        renderer: README, adr/, research/
├── slicing/         corte de mallas: README, adr/, research/
├── animation/       animación: README, adr/, research/
├── physics/         física: README, adr/, research/
├── gameplay/        reglas del juego (combate, desmembramiento): README, adr/, research/
└── performance/     rendimiento: README y presupuestos por sistema (documento vivo)
```

Las carpetas de área nuevas (`ai/`, `audio/`, etc.) se crean cuando
les toca su primer ADR y tienen la misma forma que `graphics/`. Cuando un área tiene
código, suma una carpeta `diagramas/` con los diagramas de sus sistemas y procesos
(ADR-0030).

`performance/` no es una carpeta de área: guarda documentos vivos, que cambian con cada
medición. Su ADR (el ADR-0020) vive en `adr/`, porque es transversal.

## Reglas de los ADRs

Un ADR (*Architecture Decision Record*) registra una decisión importante: el contexto,
las opciones evaluadas, lo que se eligió y por qué.

- **Numeración global y única:** `ADR-NNNN`, aunque el archivo viva en una carpeta de
  área. Nombre de archivo: `NNNN-titulo-corto.md`.
- **Plantilla:** [`adr/template.md`](adr/template.md).
- **Estados:** `Propuesto`, `Aceptado`, `Reemplazado`, `Descartado`.
- **Inmutables una vez aceptados.** Si se cambia una decisión, se escribe un ADR nuevo
  que reemplaza al anterior, y el viejo pasa a `Reemplazado por ADR-XXXX`.
  **Excepción, solo durante la etapa de investigación** (mientras no exista código): un
  ADR aceptado se puede corregir como fe de erratas, por un error o un malentendido
  (ADR-0014). Cada corrección se registra en el campo **Fe de erratas** del encabezado,
  con la fecha, el ticket y qué cambió. Cuando empieza el código, la inmutabilidad es
  estricta.
- **Un ADR por ticket** `tipo:investigación`, enlazados en ambas direcciones.
- **Cada ADR nuevo se agrega al índice** de abajo.

## Índice de ADRs

| ADR | Título | Área | Estado |
|---|---|---|---|
| [ADR-0001](adr/0001-registrar-decisiones-con-adrs.md) | Registrar decisiones con ADRs | Proceso | Aceptado |
| [ADR-0002](adr/0002-tickets-con-github-issues-projects.md) | Tickets con GitHub Issues + Projects | Proceso | Aceptado |
| [ADR-0003](adr/0003-lenguaje.md) | Lenguaje del engine y del juego | Transversal | Aceptado |
| [ADR-0004](adr/0004-capa-de-plataforma.md) | Capa de plataforma (ventana, input y tiempo) | Transversal | Aceptado |
| [ADR-0005](graphics/adr/0005-api-grafica.md) | API gráfica (Vulkan 1.3) | Gráficos | Aceptado |
| [ADR-0006](adr/0006-build-system-y-toolchain.md) | Build system y toolchain (CMake + Ninja, Clang/GCC) | Transversal | Aceptado |
| [ADR-0007](slicing/adr/0007-corte-de-mallas.md) | Corte de mallas (geométrico en la CPU, hasta donde llega la hoja, hendidura en cuña) | Corte de mallas | Aceptado |
| [ADR-0008](adr/0008-matematica.md) | Matemática propia (Y arriba, column-major, reverse-Z) | Transversal | Aceptado |
| [ADR-0009](adr/0009-subconjunto-de-cpp.md) | Subconjunto de C++ (sin excepciones ni RTTI, STL por zonas) | Transversal | Aceptado |
| [ADR-0010](adr/0010-estrategia-de-memoria.md) | Estrategia de memoria (regiones tocadas al iniciar, arenas por paso, TLSF para los pedazos) | Transversal | Aceptado |
| [ADR-0011](adr/0011-estrategia-de-testing.md) | Testing (GoogleTest, TDD en el núcleo, xfail por bug) | Transversal | Aceptado |
| [ADR-0012](adr/0012-arquitectura-del-engine.md) | Arquitectura del engine (ECS sparse set, capas, paso fijo) | Transversal | Aceptado |
| [ADR-0013](adr/0013-convenciones-y-analisis-estatico.md) | Convenciones de código y análisis estático | Transversal | Aceptado |
| [ADR-0014](adr/0014-metodologia-de-desarrollo.md) | Colaboración y metodología (modo dirigido, cortes verticales, DOD) | Proceso | Aceptado |
| [ADR-0015](animation/adr/0015-sistema-de-animacion.md) | Sistema de animación (por capas, LBS en la GPU, IK para el tajo) | Animación | Aceptado |
| [ADR-0016](physics/adr/0016-fisica.md) | Física propia (Soft Step, cápsulas por hueso, tajo barrido en su plano) | Física | Aceptado |
| [ADR-0017](adr/0017-pipeline-de-assets.md) | Pipeline de assets (Blender, glTF, binario cocinado en el build) | Transversal | Aceptado |
| [ADR-0018](gameplay/adr/0018-combate-y-desmembramiento.md) | Combate y desmembramiento (fuerza por área con umbral, espada clavada como secuencia, plano por el pecho) | Gameplay | Aceptado |
| [ADR-0019](adr/0019-licencias.md) | Licencias (código MIT, docs y assets CC BY 4.0) | Transversal | Aceptado |
| [ADR-0020](adr/0020-requisitos-y-presupuestos-de-rendimiento.md) | Requisitos y presupuestos de rendimiento (UHD 620, 60 fps, 640×360) | Transversal | Aceptado |
| [ADR-0021](adr/0021-herramientas-de-depuracion.md) | Herramientas de depuración, profiling y logging | Transversal | Propuesto |
| [ADR-0030](adr/0030-diagramas-tecnicos.md) | Diagramas técnicos (D2, de lo macro a lo micro, verificados contra el código) | Transversal | Aceptado |
