# ADR-0002: Tickets con GitHub Issues + Projects

- **Estado:** Propuesto
- **Fecha:** 2026-10-18
- **Ticket:** pendiente (se crea en la sesión guiada de M0)
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

El proyecto tiene trabajos gigantes (un renderer, un sistema de corte de mallas) que
hay que partir en tareas chicas y realizables. Hace falta un sistema de tickets estilo
Jira con estas condiciones:

- externo a Claude y fácil de usar para el autor;
- fácil de conectar con Claude;
- **completamente gratis**;
- y, por los objetivos del proyecto, que **muestre el proceso** a un evaluador.

## Criterios

- **Costo:** gratis, sin límites que el proyecto vaya a alcanzar.
- **Capacidad de partir trabajo:** jerarquía épica → feature → tarea.
- **Integración con el código:** enlazar tickets con commits y PRs.
- **Visibilidad pública:** que un reclutador o evaluador vea el proceso.
- **Conexión con Claude:** CLI o MCP.
- **Facilidad para el autor:** interfaz web y móvil, sin editar archivos a mano.

## Opciones

1. **GitHub Issues + Projects.** Issues en el mismo repo y un tablero Project en el
   perfil.
2. **Backlog.md.** Los tickets son archivos Markdown dentro del repo, con tablero
   Kanban en terminal y web, y MCP incluido.
3. **Linear (plan Free).** Herramienta dedicada, con interfaz excelente y MCP oficial.
4. **Plane (plan Free en la nube).** Alternativa open source a Jira, con MCP hospedado.

## Investigación

| | GitHub | Backlog.md | Linear Free | Plane Free |
|---|---|---|---|---|
| Costo | Gratis | Gratis (open source) | Gratis | Gratis |
| Límite de tickets | 50.000 ítems por Project | Sin límite | **250 issues activos** | Alrededor de 1.000 (fuente de terceros, sin confirmar) |
| Jerarquía | Sub-issues de hasta 8 niveles, 100 hijos por padre | Tareas y subtareas | Sí | Sí |
| Junto al código | Sí, mismo repo | Sí, archivos en el repo | No | No |
| Público | Sí, con el repo | Solo como archivos | No | No |
| Conexión con Claude | `gh` CLI y MCP oficial | MCP incluido | MCP oficial | MCP hospedado |
| Interfaz para el autor | Web y app móvil | Terminal y web local | Web y app | Web |

Puntos clave:

- **GitHub** lanzó como disponibles para todos, en abril de 2025, los sub-issues, los
  tipos de issue y la subida del límite de ítems por Project de 1.200 a 50.000. Los
  *issue types* son solo para organizaciones, así que en una cuenta personal el tipo se
  marca con labels. `gh` 2.100 permite crear sub-issues (`--parent`) y editar campos
  del Project desde la terminal. La API no permite crear vistas ni workflows del
  Project: eso se configura en la web.
- **Backlog.md** guarda tickets, documentos y decisiones como archivos Markdown en una
  carpeta del repo. Es portable y queda versionado, pero son justo los archivos de texto
  que el autor quería evitar. Además depende de un solo mantenedor y luce menos para un
  reclutador.
- **Linear Free** permite 250 issues activos. Un engine partido en tareas chicas supera
  esa cifra en pocos meses.
- **Plane Free**: un sitio comparativo de terceros indica cerca de 1.000 issues. Algunas
  funciones (tipos de work item, propiedades propias) no están en el plan gratis.
- **Lo decisivo** es que solo GitHub deja los tickets **públicos y enlazados al
  código**. La cadena *ADR (por qué) → issue (qué) → commit (cómo)* queda a la vista,
  y el proceso pasa a ser parte del portafolio.

## Decisión

<!-- TODO(human) -->

## Consecuencias

- El proceso de trabajo es público desde el día uno, incluidos los errores y los
  cambios de rumbo.
- Dependemos de GitHub como plataforma. Mitigación: los issues se pueden exportar vía
  API, y las decisiones importantes viven en los ADRs del repo, no en los tickets.
- Las vistas y workflows del tablero se configuran a mano en la web.
- Sin *issue types*, la clasificación depende de que los labels se usen con
  disciplina. Los formularios de issue ponen el label de `tipo:` solos.
- Claude trabaja con los tickets vía `gh` (ver `docs/process/tickets.md`).

## Cómo lo defiendo

<!-- TODO(human) -->

## Referencias

- GitHub, "Evolving GitHub Issues and Projects" (abril 2025):
  <https://github.blog/changelog/2025-04-09-evolving-github-issues-and-projects/>
- Backlog.md README: <https://cdn.jsdelivr.net/gh/MrLesk/Backlog.md@main/README.md>
- Linear MCP, plan gratis vs pago: <https://aitoolsatlas.ai/tools/linear-mcp/free-vs-paid>
- Plane MCP server: <https://docs.plane.so/ai/mcp-server>
- Plane, plan gratis (fuente de terceros): <https://agentdeals.dev/vendor/plane>
- Guía de uso: [`../process/tickets.md`](../process/tickets.md)
