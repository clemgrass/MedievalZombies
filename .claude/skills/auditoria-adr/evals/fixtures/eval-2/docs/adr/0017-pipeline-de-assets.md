# ADR-0017: Pipeline de assets

- **Estado:** Aceptado
- **Fecha:** 2026-10-18
- **Ticket:** #30
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

Necesitamos decidir cómo se crean los modelos, las texturas y las animaciones, y cómo
llegan al engine. Este proyecto apunta a un portafolio para estudios AAA y a la
postulación al máster de la TUM, así que el pipeline tiene que verse profesional.

## Criterios

1. Gratis.
2. Estándar en la industria.

## Opciones

- **Blender + glTF 2.0.**
- **Maya + FBX.**

## Investigación

Blender es gratis y exporta glTF 2.0, que es el formato abierto de Khronos.

## Decisión

Se usa **Blender** para modelar y animar, y se exporta a **glTF 2.0**. Las
transformaciones de los nodos se convierten al importar usando **GLM**, porque ya
resuelve la conversión de cuaterniones a matrices.

## Consecuencias

- El importador de glTF depende de GLM.

## Cómo lo defiendo

## Referencias

- glTF 2.0: <https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html>
