# MedievalZombies

Videojuego 3D sobre un apocalipsis zombie en la época medieval, con **engine propio** en
C++20 y Vulkan. Una protagonista tiene que rescatar a una princesa atrapada, y su arma
estrella es un tajo de espada que **corta de verdad la malla del zombie en tiempo
real**.

## Estado

El proyecto está en la **etapa de decisiones**: cada decisión técnica se investiga y se
registra como un ADR antes de escribir código. Todavía no hay código del juego.

## Stack

| Pieza | Elección | Decisión |
|---|---|---|
| Lenguaje | C++20 | [ADR-0003](docs/adr/0003-lenguaje.md) |
| Ventana e input | SDL3, detrás de una capa propia | [ADR-0004](docs/adr/0004-capa-de-plataforma.md) |
| API gráfica | Vulkan 1.3 | [ADR-0005](docs/graphics/adr/0005-api-grafica.md) |
| Build | CMake + Ninja, Clang y GCC | [ADR-0006](docs/adr/0006-build-system-y-toolchain.md) |

El resto (renderer, física, animación, corte de mallas y gameplay) es propio.

## Cómo se trabaja

Es un **trabajo colaborativo** entre el autor y Claude, un asistente de IA. El autor da
las ideas de cómo tiene que funcionar cada sistema, revisa todo el código y es
responsable de entenderlo completo. Claude escribe el código. Los commits con código de
Claude lo muestran como coautor. El modelo completo está en el
[ADR-0014](docs/adr/0014-metodologia-de-desarrollo.md).

- **Decisiones:** [índice de ADRs](docs/README.md).
- **Tickets:** [tablero del proyecto](https://github.com/users/clemgrass/projects/1) y
  [registro de bugs](https://github.com/users/clemgrass/projects/2).
- **Convenciones:** [git](docs/process/git.md) y [tickets](docs/process/tickets.md).

## Licencias

El código va bajo [MIT](LICENSE). La documentación y los assets propios van bajo
[CC BY 4.0](LICENSES/CC-BY-4.0.txt). Los créditos de terceros están en
[`CREDITS.md`](CREDITS.md) ([ADR-0019](docs/adr/0019-licencias.md)).
