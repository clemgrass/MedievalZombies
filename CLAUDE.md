# MedievalZombies

Videojuego 3D sobre un apocalipsis zombie en la época medieval, con engine propio,
escrito en C++20 (ADR-0003).

> **Estado actual:** milestone M1 (decisiones fundacionales, épica #5). M2 (decisiones de
> sistemas, épica #35) espera a M1. M0 está
> cerrado. No existe código del juego: el código empieza después de cerrar M1.

## El juego

- **Premisa:** una protagonista tiene que rescatar a una princesa que quedó atrapada en
  medio de un apocalipsis zombie medieval.
- **Enemigos:** zombies variados: campesinos, con armadura, etc.
- **Feature estrella:** el tajo de espada **corta de verdad la malla del zombie en
  tiempo real** (mesh slicing dinámico). Es la pieza técnica más importante del proyecto.

## Por qué existe este proyecto

1. **Portafolio para postular a estudios grandes de videojuegos** como programador de
   engine, gráficos o gameplay.
2. **Postulación a posgrado:** pensado para el M.Sc. Informatics: Games Engineering de la
   TUM (Technical University of Munich) y posibles doctorados después.
3. **Aprender de verdad.** El autor tiene que poder pararse frente a un evaluador y
   explicar cada decisión de diseño y cómo funciona cada sistema. **Aprender pesa más
   que ir rápido.**

## Restricciones

- **Plataforma principal de desarrollo:** Linux.
- **"Desde cero" con criterio:** se acepta una capa mínima para ventana e input (tipo
  SDL3 o GLFW) y una API gráfica (tipo OpenGL o Vulkan). El renderer, la física, la
  animación, el corte de mallas y el gameplay son propios. Cada dependencia externa
  necesita un ADR que la justifique. La regla: el rendimiento tiene que depender de
  decisiones que el autor entienda y controle.
- **Todo gratis:** herramientas, servicios y librerías sin costo.
- **Sin fecha límite:** proyecto personal, sin prisa.
- **Idioma:** la colaboración y los docs van en español.

## Cómo trabajamos (reglas para Claude)

- **Enseñar, no solo hacer.** Explica el *por qué* de cada decisión y técnica, con
  referencias (papers, libros, charlas GDC/SIGGRAPH, documentación oficial).
- **Ninguna decisión grande sin ADR.** Cada decisión técnica importante se investiga
  a fondo (opciones, trade-offs, fuentes) y queda registrada como ADR. No se decide en
  la conversación sin dejarlo escrito.
- **Las áreas grandes llevan su propia carpeta** (por ejemplo `docs/graphics/`), con
  varios ADRs y notas de investigación, en vez de un solo documento.
- **Los ADRs hablan de la audiencia en términos genéricos.** No nombran la universidad
  objetivo ni usan etiquetas de tipo de empresa ("AAA"). Se escribe "la industria",
  "estudios grandes" o "programas de posgrado en ingeniería de videojuegos".
- **El autor decide.** Claude recomienda con fundamentos, pero la decisión final es
  del autor.
- **Trabajo en piezas chicas.** Los trabajos gigantes (como el renderer) se parten en
  tickets pequeños y realizables.
- **Ritmo pausado.** No adelantarse a implementar ni saltarse aprobaciones.
- **Trabajo colaborativo (ADR-0014).** El autor da las ideas macro de cada sistema,
  revisa todo el código y es responsable de entender el sistema completo. Claude escribe
  el código (núcleo, periférico y tests), además de investigar, proponer y explicar.
  - **Modo dirigido** (`modo:dirigido`): para los sistemas grandes (renderer, corte de
    mallas, física, animación, IA). El autor explica el flujo y lo que quiere. Claude
    prepara el código y marca con `TODO(human)` los puntos de decisión clave. El autor
    explica con sus palabras cómo resolverlos, y Claude lo escribe, lo mejora y señala
    dónde la idea puede fallar.
  - **Modo propuesto** (`modo:propuesto`): para lo periférico. Claude propone el diseño
    y el código, y el autor lo cuestiona y lo aprueba.
  - **Nada entra a `main` sin la revisión del autor.** Claude explica lo suficiente para
    que el autor pueda revisarlo y defenderlo. Los commits con código de Claude llevan
    `Co-Authored-By: Claude`.
  - En TDD (ADR-0011), Claude escribe primero el test que falla, a partir de la
    especificación acordada con el autor, y después la implementación.
- **Antes de cada PR corre la auditoría contra los ADRs** (skill `/auditoria-adr`, en
  `.claude/skills/auditoria-adr/`). Sus hallazgos se revisan juntos: se corrigen, se
  ignoran o quedan como `deuda-técnica`.

## Estructura del repositorio

Monorepo público: `github.com/clemgrass/MedievalZombies`. Por ahora existen:

- `docs/`: ADRs, investigación, specs, planes, guías y diagramas técnicos (ADR-0030);
- `.github/`: formularios de issue y plantilla de PR;
- `.claude/`: skills y agentes de Claude Code, como la auditoría `/auditoria-adr`;
- `tools/`: scripts de configuración del repo;
- `LICENSE`, `LICENSES/`, `REUSE.toml` y `CREDITS.md`: licencias (ADR-0019).

La spec de la base del proyecto define la estructura de `docs/`, el formato de ADR, el
sistema de tickets (GitHub Issues + Projects) y el orden de los primeros ADRs:
`docs/specs/2026-10-18-base-del-proyecto-design.md`.

## Tickets

- **Tablero:** GitHub Project #1, `MedievalZombies` (público):
  <https://github.com/users/clemgrass/projects/1>
- **Bugs:** GitHub Project #2, `MedievalZombies – Bug hunting` (público), aparte del
  tablero de features: <https://github.com/users/clemgrass/projects/2>. Cada bug con
  test sigue el flujo xfail del ADR-0011 (`WILL_FAIL` hasta el arreglo).
- **Guía completa** (labels, campos, Listo/Hecho, bugs, comandos): `docs/process/tickets.md`.
  Convenciones git: `docs/process/git.md`.
- **Claude usa solo `gh`**, nunca el MCP de GitHub:
  - `gh issue view|list|create|edit` (`--parent N` para sub-issues);
  - `gh project item-list 1 --owner clemgrass`;
  - `gh project item-edit 1 --owner clemgrass --url <issue> --field Status --value "<estado>"`.
- **Al crear issues con `gh`**, los labels del formulario **no** se aplican solos (solo
  los pone la web). Siempre pasar `--label "tipo:…"`, más `área:` y `--milestone`
  cuando apliquen, y un `--body` con las mismas secciones del formulario
  correspondiente en `.github/ISSUE_TEMPLATE/`.
- **Claude solo crea issues con aprobación del autor.** Antes de trabajar, lee el
  ticket y respeta su label `modo:`.
- **Nunca se trabaja directo en `main`:** siempre rama y PR. El *ruleset* "Proteger
  main" lo exige.
- `gh issue list --label` usa el índice de búsqueda: un issue recién creado puede
  tardar unos segundos en aparecer.

## Herramientas de aprendizaje disponibles

- Output style **Learning** de Claude Code (`/output-style learning`): Claude deja
  marcadores `TODO(human)` en los puntos de decisión del modo dirigido. El autor explica
  la solución y Claude la escribe.
- MCP **context7**: documentación actualizada de librerías y APIs.
