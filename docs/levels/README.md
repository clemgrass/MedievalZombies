# Niveles

Decisiones e investigación de los niveles y las escenas: con qué herramienta se arman,
qué formato tienen, cómo entran al ECS, cómo se describen las oleadas, la horda y el
jefe, los triggers, las luces del nivel y cómo se pasa de un nivel a otro.

**Estado:** [ADR-0027](adr/0027-niveles-y-escenas.md) aceptado: el mundo se arma en
Blender, partido en zonas que se cargan mientras se camina; cargada no es lo mismo que
simulando, y una zona entra a la simulación en un paso fijo, cuando la caballera se
acerca a su borde, nunca cuando termina de cargar; lo que pasa en cada zona (fases, oleadas,
el jefe) va en un texto; el cocinador valida todo y genera la navmesh del mundo partida
por zona; y solo se dibuja la zona actual y sus vecinas, hasta el final de la niebla.
Investigación: [`research/prototipo-de-niveles.md`](research/prototipo-de-niveles.md).

- [`adr/`](adr/): ADRs del área.
- [`research/`](research/): notas largas de investigación y resúmenes de papers.
