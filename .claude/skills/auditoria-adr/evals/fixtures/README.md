# Casos de prueba de la auditoría

Cada carpeta `eval-N/` tiene los archivos que se copian **sin commitear** sobre un clon
de `main`, en la rama que indica `evals.json`. Los errores son **a propósito**: sirven
para medir si la auditoría los detecta.

| Caso | Rama | Cambios |
|---|---|---|
| `eval-1` | `99-ecs-inicial` | `engine/core/entity.hpp` y `CMakeLists.txt` con violaciones de varios ADRs |
| `eval-2` | `30-adr-0017` | Un ADR-0017 con problemas de audiencia, de formato y de coherencia |
| `eval-3` | `46-readme` | Un `README.md` (más `README.md` agregado a la anotación CC BY 4.0 de `REUSE.toml`). No cumple el criterio "estructura" del #46 |
