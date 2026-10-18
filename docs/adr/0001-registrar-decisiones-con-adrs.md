# ADR-0001: Registrar decisiones con ADRs

- **Estado:** Aceptado
- **Fecha:** 2026-10-18
- **Ticket:** #4
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

MedievalZombies es un engine y un juego construidos casi desde cero. Va a acumular
cientos de decisiones técnicas: lenguaje, API gráfica, cómo se cortan las mallas, cómo
se representa la física. Uno de sus objetivos es que el autor pueda **defender cada
decisión frente a un evaluador**, sea de un estudio de videojuegos o de la admisión al
M.Sc. Games Engineering de la TUM.

Si las decisiones viven solo en la cabeza del autor o en conversaciones con Claude, se
pierden: con el tiempo nadie recuerda qué alternativas se descartaron ni por qué. Hay
que elegir cómo registrarlas.

## Criterios

- **Aprendizaje:** el registro obliga a entender la decisión, no solo a tomarla.
- **Defensa ante evaluadores:** un tercero puede reconstruir el razonamiento.
- **Trazabilidad:** se puede enlazar con tickets y commits.
- **Costo de mantenimiento:** un solo desarrollador tiene que poder sostenerlo.
- **Historia:** se ve cómo evolucionaron las decisiones, no solo el estado actual.

## Opciones

1. **Sin registro formal.** Las decisiones quedan en commits, comentarios y memoria.
   - A favor: costo cero.
   - En contra: el razonamiento se pierde y no hay nada que mostrar al evaluador.
2. **Wiki o documento único de diseño** (estilo GDD/TDD que se va editando).
   - A favor: todo en un lugar.
   - En contra: se reescribe encima, así que se pierde la historia ("antes pensábamos
     X"). Crece sin estructura.
3. **ADRs al estilo Nygard.** Un archivo corto por decisión: contexto, decisión,
   estado y consecuencias.
   - A favor: simple, inmutable, con historia.
   - En contra: no registra de forma explícita las opciones descartadas ni la
     investigación.
4. **ADRs con MADR, extendido.** La estructura de MADR (criterios, opciones con pros y
   contras, consecuencias) más dos secciones propias: *Investigación* y *Cómo lo
   defiendo*.
   - A favor: registra las alternativas, las fuentes y la comprensión del autor.
   - En contra: cada ADR cuesta más tiempo de escribir.

## Investigación

- **Origen.** Michael Nygard propuso los ADRs en 2011: registros cortos, uno por
  decisión, guardados junto al código y **nunca editados una vez aceptados**. Si una
  decisión cambia, se escribe un ADR nuevo que reemplaza al anterior. Su argumento es
  que el mayor costo para quien llega a un proyecto es no entender *por qué* las cosas
  son como son.
- **MADR** (*Markdown Architectural Decision Records*) es una plantilla abierta y muy
  usada. Agrega criterios de decisión ("decision drivers") y opciones consideradas con
  sus pros y contras, que es justo lo que pregunta un evaluador: "¿qué más
  consideraste?".
- **Por qué sirve en un portafolio.** Un registro inmutable muestra el proceso:
  "decidí X, aprendí Y, cambié a Z en el ADR-00NN". Un documento único que se reescribe
  muestra solo el estado final, y el valor de un proyecto de aprendizaje está en el
  camino.
- **Las dos secciones propias** responden a los objetivos del proyecto (ver la spec).
  *Investigación* deja las fuentes a mano para el ensayo científico que pide la
  admisión de la TUM. *Cómo lo defiendo* obliga al autor a explicar la decisión con sus
  palabras: si no puede, todavía no la entiende.

## Decisión

Elijo la opción 4: ADRs con MADR. Es el formato con el que ya he trabajado, y deja un
historial correcto de las decisiones tomadas frente a las opciones más populares.
También es lo mejor a futuro, para entender bien por qué se decidió algo.

No creo que sacrifique nada en particular. Solo es un poco más largo de escribir que
las otras opciones.

## Consecuencias

- Cada decisión importante tiene un costo fijo de escritura. A cambio, el proyecto
  acumula un registro que se puede mostrar.
- La numeración global y el índice en `docs/README.md` exigen disciplina: un ADR que no
  entra al índice prácticamente no existe.
- Las decisiones aceptadas no se editan, así que los errores quedan a la vista y se
  corrigen con un ADR nuevo. Eso es intencional.

## Cómo lo defiendo

**¿Por qué no un documento de diseño único que se va actualizando?**
He trabajado con documentos GDD y TDD, y me parecen muy desordenados: una vez escritos,
nunca más los leo. Esta forma es más ordenada, tanto para mí como para una IA.

**Si una decisión aceptada resulta estar mal, ¿qué haces?**
Se escribe un ADR nuevo que reemplaza al anterior y corrige lo que salió mal. El ADR
original no se borra: queda marcado como `Reemplazado por ADR-XXXX`, y el nuevo deja
registrado por qué la decisión original fue incorrecta.

**¿No es demasiada burocracia para un proyecto de una sola persona?**
Puede ser, pero también estoy practicando técnicas de trabajo en equipo. Si en algún
momento el proyecto creciera y se sumara más gente, esto facilitaría la transición.

## Referencias

- Michael Nygard, "Documenting Architecture Decisions" (2011):
  <https://cognitect.com/blog/2011/11/15/documenting-architecture-decisions>
- MADR – Markdown Architectural Decision Records: <https://adr.github.io/madr/>
- Spec de la base del proyecto: [`../specs/2026-10-18-base-del-proyecto-design.md`](../specs/2026-10-18-base-del-proyecto-design.md)
