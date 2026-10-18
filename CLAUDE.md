# MedievalZombies

Videojuego 3D sobre un apocalipsis zombie en la época medieval, con engine propio,
escrito en C, C++ o C# (por decidir en un ADR).

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
- **Reparto de roles: se decide por ticket, con una regla fija.**
  - **Núcleo → lo escribe el autor**, con guía de Claude. Son los algoritmos que un
    evaluador va a preguntar: renderer, corte de mallas, física, animación, IA y
    cualquier sistema que se presente como trabajo propio. Claude explica, propone,
    da referencias y revisa, pero no entrega la implementación. Usa `TODO(human)` y
    pistas graduales.
  - **Periférico → lo puede escribir Claude**: build, scripts, boilerplate, carga de
    archivos, tooling. El autor lo revisa hasta entenderlo.
  - **Tests → los escribe Claude, en todo el engine, y el autor los revisa** (ADR-0011).
    En las zonas con TDD, Claude escribe primero el test que falla, a partir de la
    especificación acordada, y el autor escribe la implementación del núcleo.
  - Cada ticket indica explícitamente quién lo escribe (`autor` o `claude`). Si no está
    claro, se pregunta antes de escribir código.

## Estructura del repositorio

Monorepo público: `github.com/clemgrass/MedievalZombies`. Por ahora existen `docs/`, `.github/` (formularios de issue) y `tools/` (scripts de
configuración del repo).

La spec de la base del proyecto define la estructura de `docs/`, el formato de ADR, el
sistema de tickets (GitHub Issues + Projects) y el orden de los primeros ADRs:
`docs/specs/2026-10-18-base-del-proyecto-design.md`.

## Tickets

- **Tablero:** GitHub Project #1, `MedievalZombies` (público):
  <https://github.com/users/clemgrass/projects/1>
- **Guía completa** (labels, campos, Listo/Hecho, comandos): `docs/process/tickets.md`.
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
  ticket y respeta su label `owner:`.
- `gh issue list --label` usa el índice de búsqueda: un issue recién creado puede
  tardar unos segundos en aparecer.

## Herramientas de aprendizaje disponibles

- Output style **Learning** de Claude Code (`/output-style learning`): Claude deja
  marcadores `TODO(human)` para que el autor escriba piezas clave del código.
- MCP **context7**: documentación actualizada de librerías y APIs.
