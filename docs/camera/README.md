# Cámara

Decisiones e investigación de la cámara: quién la controla, desde dónde mira, qué parte,
si alguna, vive en la simulación y qué parte en el render,
cómo se suaviza, cómo evita las paredes, la cámara del tajo, los rayos X y la sacudida.

**Estado:** [ADR-0031](adr/0031-sistema-de-camara.md) aceptado: la cámara es una opción
del jugador con tres estilos (tercera persona por detrás, fijas y rieles por zona, y
primera persona), y en ninguno el jugador la mueve; el apuntado sale del pecho de la
caballera, así que la cámara queda fuera de la simulación; en los tres estilos, la cámara
del tajo acompaña la hoja, y la herida se abre mientras la hoja cruza.
Investigación: [`research/prototipo-de-camara.md`](research/prototipo-de-camara.md).

- [`adr/`](adr/): ADRs del área.
- [`research/`](research/): notas largas de investigación y resúmenes de papers.
