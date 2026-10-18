# Créditos y licencias de terceros

Las licencias del proyecto están en el [ADR-0019](docs/adr/0019-licencias.md):

- **Código:** MIT ([`LICENSE`](LICENSE)).
- **Documentación y assets propios:** CC BY 4.0
  ([`LICENSES/CC-BY-4.0.txt`](LICENSES/CC-BY-4.0.txt)).
- La asignación de licencias por carpeta está en [`REUSE.toml`](REUSE.toml).

## Assets de terceros

Solo se aceptan assets con licencia **CC0** o **CC BY**. Cada asset de CC BY necesita su
fila en esta tabla, porque la licencia exige dar crédito. Los CC0 se registran igual,
para saber de dónde vino cada archivo.

| Asset | Autor | Fuente (URL) | Licencia | Archivos en el repo |
|---|---|---|---|---|
| *Todavía no hay assets de terceros.* | | | | |

## Dependencias

| Dependencia | Licencia | Uso |
|---|---|---|
| [SDL3](https://github.com/libsdl-org/SDL) | zlib | Ventana, input, tiempo y audio (ADR-0004) |
| [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers) | Apache-2.0 o MIT | API gráfica (ADR-0005) |
