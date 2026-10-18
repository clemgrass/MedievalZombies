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
├── graphics/        renderer: README, adr/, research/
└── slicing/         corte de mallas: README, adr/, research/
```

Las carpetas de área nuevas (`physics/`, `animation/`, `ai/`, etc.) se crean cuando
les toca su primer ADR y tienen la misma forma que `graphics/`.

## Reglas de los ADRs

Un ADR (*Architecture Decision Record*) registra una decisión importante: el contexto,
las opciones evaluadas, lo que se eligió y por qué.

- **Numeración global y única:** `ADR-NNNN`, aunque el archivo viva en una carpeta de
  área. Nombre de archivo: `NNNN-titulo-corto.md`.
- **Plantilla:** [`adr/template.md`](adr/template.md).
- **Estados:** `Propuesto`, `Aceptado`, `Reemplazado`, `Descartado`.
- **Inmutables una vez aceptados.** Si se cambia una decisión, se escribe un ADR nuevo
  que reemplaza al anterior, y el viejo pasa a `Reemplazado por ADR-XXXX`.
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
