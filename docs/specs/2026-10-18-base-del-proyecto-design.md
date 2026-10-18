# Spec: base del proyecto MedievalZombies

- **Fecha:** 2026-10-18
- **Estado:** Aprobada por el autor (2026-10-18)
- **Alcance:** primer subproyecto. Define cómo documentamos decisiones, cómo
  organizamos el trabajo y en qué orden decidimos lo fundacional. **No incluye
  código del juego ni decisiones técnicas del engine**: esas se toman después, cada
  una en su ADR.

## 1. Contexto

MedievalZombies es un videojuego 3D sobre un apocalipsis zombie en la época medieval.
Una protagonista tiene que rescatar a una princesa atrapada, enfrentando zombies
variados (campesinos, con armadura, etc.). La feature estrella es que **el tajo de
espada corte de verdad la malla del zombie en tiempo real**.

El engine es propio, escrito en C, C++ o C# (se decide en el ADR-0003).

### 1.1 Objetivos, en orden

1. **Portafolio para estudios grandes de videojuegos** (roles de engine, gráficos o
   gameplay).
2. **Postulación al M.Sc. Informatics: Games Engineering de la TUM** y posibles
   doctorados después.
3. **Aprender.** El autor tiene que poder explicar y defender cada decisión de diseño
   y cada sistema frente a un evaluador. Aprender pesa más que ir rápido.

### 1.2 Restricciones

| Restricción | Detalle |
|---|---|
| Plataforma de desarrollo | Linux |
| "Desde cero" con criterio | Se acepta una capa mínima de ventana e input y una API gráfica. Renderer, física, animación, corte y gameplay son propios. Toda dependencia externa necesita un ADR |
| Costo | Todas las herramientas, servicios y librerías son gratis |
| Plazo | Sin fecha límite. Proyecto personal |
| Idioma | Colaboración y documentación en español |
| Repositorio | Monorepo público: `github.com/clemgrass/MedievalZombies` |

### 1.3 Modelo de colaboración

Es un trabajo colaborativo. El autor da las ideas macro de cada sistema, revisa todo el
código y es responsable de entender el sistema completo. Claude escribe el código.

- **Modo dirigido:** para los sistemas grandes (renderer, corte de mallas, física,
  animación, IA). El autor explica el flujo, y Claude lo escribe, marcando con
  `TODO(human)` los puntos de decisión que el autor resuelve con sus palabras.
- **Modo propuesto:** para lo periférico. Claude propone, y el autor cuestiona y
  aprueba.
- Cada feature y tarea declara su `modo`.

El detalle operativo vive en `CLAUDE.md`.

## 2. Sistema de tickets: GitHub Issues + Projects

### 2.1 Por qué

Es gratis y no tiene límites prácticos en un repo público (50.000 ítems por proyecto).
Tiene sub-issues de hasta 8 niveles, tablero estilo Jira y se conecta con Claude vía
`gh` o el MCP oficial. Sobre todo, los tickets viven junto al código de forma pública:
un evaluador puede seguir la cadena **ADR (por qué) → ticket (qué) → commit/PR (cómo)**.

Se descartaron Backlog.md (tickets como `.md`, menos visible) y Linear/Plane (límites
del plan gratis, tickets separados del repo y no públicos). La justificación completa
va en el ADR-0002.

### 2.2 Jerarquía

| Nivel | Qué es | Ejemplo |
|---|---|---|
| Épica | Un área completa | "Renderer", "Corte de mallas" |
| Feature | Una capacidad concreta, sub-issue de una épica | "Shadow maps" |
| Tarea | Se termina en 1 a 3 sesiones de trabajo, con criterios de aceptación claros. Si no cabe, se parte | "Depth pass para shadow map" |

Las relaciones se modelan con sub-issues. Los "issue types" nativos de GitHub son solo
para organizaciones, así que el tipo se marca con labels.

### 2.3 Labels

| Grupo | Valores |
|---|---|
| `tipo:` | `épica`, `feature`, `tarea`, `bug`, `investigación` |
| `área:` | `graphics`, `slicing`, `physics`, `animation`, `ai`, `gameplay`, `tooling`, `docs` |
| `modo:` | `dirigido`, `propuesto` |

Una tarea `tipo:investigación` produce un ADR. Las áreas nuevas se agregan como labels
cuando aparecen.

### 2.4 Project (tablero)

Un único GitHub Project en el perfil `clemgrass`, conectado al repo, con estos campos:

| Campo | Valores |
|---|---|
| Estado | `Backlog`, `Listo`, `En progreso`, `En revisión`, `Hecho` |
| Prioridad | `Alta`, `Media`, `Baja` |
| Tamaño | `S`, `M`, `L` |

Vistas: **Tablero** (Kanban por Estado), **Tabla** (todo, filtrable) y **Roadmap**
(por milestone). Se activan los workflows nativos que agregan al Project los issues
nuevos del repo y mueven a `Hecho` los cerrados.

### 2.5 Plantillas de issue

En `.github/ISSUE_TEMPLATE/`, como formularios de GitHub:

- **Épica:** objetivo, alcance, fuera de alcance, ADRs relacionados.
- **Feature / Tarea:** descripción, criterios de aceptación, `modo`, ADR relacionado
  y la sección **"Qué debo poder explicar"**: preguntas que un evaluador haría sobre ese
  trabajo. Es obligatoria, y la tarea solo se cierra cuando el autor puede
  responderlas.
- **Investigación:** pregunta a decidir, criterios, carpeta destino del ADR.

### 2.6 Definición de Listo y de Hecho

- **Listo:** tiene criterios de aceptación, `modo`, tamaño, y su ADR relacionado
  (si aplica) está aceptado.
- **Hecho:** cumple los criterios de aceptación, está mergeado con `Closes #N`, y el
  autor respondió "Qué debo poder explicar".

### 2.7 Reglas para Claude con los tickets

- Propone cómo partir el trabajo, pero **solo crea issues cuando el autor lo aprueba**.
- Lee el ticket antes de trabajar y respeta su `modo`.
- Mueve el estado del ticket y enlaza los commits con `Closes #N` o `Refs #N`.

### 2.8 Enseñanza (entregable de M0)

El autor no conoce todavía GitHub Projects. M0 incluye una **sesión guiada** donde el
autor, con Claude explicando cada paso:

1. Crea su primer issue usando una plantilla.
2. Le agrega un sub-issue, labels y un milestone.
3. Lo mueve por las columnas del tablero.
4. Lo cierra desde un commit con `Closes #N` y ve cómo se actualiza el tablero.

Al final, `docs/process/tickets.md` queda como guía de referencia en español.

### 2.9 Prerrequisitos

- Agregar el scope `project` al token de `gh`: `gh auth refresh -s project`.
- Reparar la conexión del MCP de GitHub (falla con un error de autorización). Mientras
  tanto, `gh` alcanza para todo.

## 3. Documentación: `docs/` y ADRs

### 3.1 Estructura

```
docs/
├── README.md        mapa del repo de docs e índice de TODOS los ADRs
├── vision/          diseño del juego: premisa, pilares, alcance, enemigos
├── process/         cómo trabajamos: tickets, convenciones git
├── specs/           specs aprobadas
├── adr/             ADRs transversales y plantilla (template.md)
├── graphics/
│   ├── README.md    mapa y estado del área
│   ├── adr/
│   └── research/    notas largas de investigación
└── slicing/         misma forma que graphics/
```

Regla YAGNI: las carpetas de área nuevas (`physics/`, `animation/`, `ai/`, etc.) se
crean cuando les toque su primer ADR, con la misma forma que `graphics/`.

### 3.2 Reglas de los ADRs

- **Numeración global y única:** `ADR-NNNN`, aunque el archivo viva en una carpeta de
  área. Nombre de archivo: `NNNN-titulo-corto.md`.
- **Índice:** cada ADR nuevo se agrega al índice de `docs/README.md`.
- **Inmutables una vez aceptados.** Un cambio de decisión es un ADR nuevo que reemplaza
  al anterior. El viejo pasa a `Reemplazado por ADR-XXXX`.
- **Estados:** `Propuesto`, `Aceptado`, `Reemplazado`, `Descartado`.
- **Un ADR por ticket** `tipo:investigación`, enlazados en ambas direcciones.

### 3.3 Plantilla

Basada en MADR, con dos secciones propias (marcadas ★):

| Sección | Contenido | Quién la escribe |
|---|---|---|
| Encabezado | Estado, fecha, ticket | Claude |
| Contexto y problema | Qué hay que decidir y por qué ahora | Claude |
| Criterios | Con qué se evalúa: aprendizaje, rendimiento, portafolio, complejidad | Claude, el autor ajusta |
| Opciones | 2 a 4 alternativas reales | Claude |
| ★ Investigación | Resumen con fuentes. Si es larga, enlace a `research/` | Claude |
| Decisión | Qué se elige y por qué | **Autor** |
| Consecuencias | Qué se gana, qué se sacrifica y qué queda abierto | Claude y autor |
| ★ Cómo lo defiendo | Preguntas de un evaluador, respondidas con palabras del autor | **Autor** |
| Referencias | Papers, libros, charlas, documentación | Claude |

Si al redactar "Cómo lo defiendo" el autor descubre que no entiende algo, se profundiza
antes de aceptar el ADR.

## 4. Orden de los primeros ADRs

| ADR | Tema | Carpeta | Milestone |
|---|---|---|---|
| 0001 | Registrar decisiones con ADRs | `adr/` | M0 |
| 0002 | Tickets con GitHub Issues + Projects | `adr/` | M0 |
| 0003 | Lenguaje: C vs C++ vs C# | `adr/` | M1 |
| 0004 | Capa de plataforma: SDL3 vs GLFW vs Wayland/X11 nativo | `adr/` | M1 |
| 0005 | API gráfica: OpenGL 4.6 vs Vulkan 1.3+ (u otra) | `graphics/adr/` | M1 |
| 0006 | Build system y toolchain | `adr/` | M1 |
| 0007 | Estrategia de corte de mallas (incluye mallas animadas) | `slicing/adr/` | M1 |
| 0008 | Matemática: biblioteca propia vs externa | `adr/` | M1 |

El orden sigue las dependencias: cada decisión limita a las siguientes. El ADR-0007 va
temprano porque es el mayor riesgo técnico y obliga a pensar juntas animación, física y
geometría. Los ADRs siguientes (assets, física, animación, IA, audio) se crean cuando el
trabajo los necesite.

Los ADR-0001 y 0002 registran decisiones ya tomadas en esta spec. Igual pasan por la
regla: el autor escribe su Decisión y "Cómo lo defiendo".

En paralelo a M1 se escribe `docs/vision/`: premisa, pilares, cámara, alcance del
*vertical slice* y tipos de zombie. No es un ADR, pero da números a decisiones técnicas
(por ejemplo, cuántos zombies en pantalla).

## 5. Milestones

- **M0 – Base.** Estructura de `docs/`, `docs/README.md`, plantilla de ADR,
  `docs/process/` (tickets y convenciones git), plantillas de issue, labels, Project
  configurado, sesión guiada de enseñanza y ADRs 0001 y 0002 aceptados.
- **M1 – Decisiones fundacionales.** ADRs 0003 a 0008 aceptados y defendibles, y
  `docs/vision/` en una primera versión.

Los milestones de código se definen cuando M1 esté cerrado.

## 6. Fuera de alcance

- Código del juego o del engine, scaffolding y dependencias.
- Cualquier decisión técnica del engine: se toma en su ADR, no en esta spec.
- Milestones posteriores a M1.
- CI/CD, que se decide cuando exista código.

## 7. Criterios de éxito

- El autor crea, parte, mueve y cierra tickets por su cuenta en GitHub Projects.
- Cualquier persona que entre al repo público encuentra en `docs/README.md` el mapa
  completo y el índice de ADRs.
- Cada ADR aceptado tiene su "Cómo lo defiendo" escrito por el autor.
- Claude puede leer y actualizar tickets vía `gh` (y vía MCP cuando esté reparado).
