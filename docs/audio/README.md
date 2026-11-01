# Audio

Decisiones e investigación del audio: quién saca el sonido al dispositivo, quién mezcla
y en qué hilo, el sonido 3D, los formatos, la música y cómo suena el corte.

**Estado:** [ADR-0024](adr/0024-audio.md) aceptado: SDL3 solo saca el sonido al
dispositivo, un mezclador propio corre en su callback, cada sonido suena en el tiempo de
su paso y el corte suena por capas según el material y la fuerza. Investigación:
[`research/prototipo-de-audio.md`](research/prototipo-de-audio.md).

- [`adr/`](adr/): ADRs del área.
- [`research/`](research/): notas largas de investigación y resúmenes de papers.
