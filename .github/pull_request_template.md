## Qué cambia y por qué

<!-- Resumen corto. El porqué largo vive en el ticket o en el ADR. -->

Closes #

## Auditoría contra los ADRs

<!-- Resultado de la skill de auditoría. Por cada hallazgo: corregido, ignorado (con el
motivo) o deuda técnica (con su issue). -->

- [ ] La auditoría corrió y sus hallazgos están resueltos aquí.

## Lista de revisión

Basada en la guía de revisión de código de Google, en orden de importancia.

- [ ] **Diseño:** las piezas encajan entre sí y con el resto del sistema.
- [ ] **Funcionalidad:** hace lo que pide el ticket, y eso es bueno para quien lo usa.
- [ ] **Complejidad:** se entiende rápido. No hay nada más complejo de lo necesario.
- [ ] **Tests:** cubren el cambio y fallarían si el código se rompe.
- [ ] **Nombres y comentarios:** el código se explica solo, y cada bloque se entiende
      fuera de contexto.
- [ ] **Estilo:** `clang-format` y `clang-tidy` sin avisos.
- [ ] **Documentación:** los docs, el `CHANGELOG.md` y "Qué debo poder explicar" del
      ticket están al día.
- [ ] **Diagramas:** si el PR cambia un flujo dibujado, o le da comportamiento a un
      sistema nuevo, su diagrama está al día y los verificadores pasan.

## Rendimiento

<!-- Solo si el PR toca el rendimiento: la medición antes y después. Si no, borra esta
sección. -->
