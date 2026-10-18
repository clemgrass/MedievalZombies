# ADR-0019: Licencias del código, la documentación y los assets

- **Estado:** Aceptado
- **Fecha:** 2026-10-18
- **Ticket:** #33
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

El repositorio `clemgrass/MedievalZombies` es **público y no tiene licencia**. Publicar
en GitHub permite que otros vean el código y hagan fork, porque eso dicen los términos
de servicio. Pero **sin licencia rige el copyright por defecto**: nadie puede copiar,
modificar ni redistribuir nada sin arriesgarse legalmente. Eso incluye a alguien que
quiera compilar y probar el juego para evaluarlo.

El proyecto va a tener **cuatro tipos de contenido**, que no necesariamente llevan la
misma licencia:

1. **código** (el engine y el juego);
2. **documentación** (los ADRs, las guías, la visión);
3. **assets propios** (modelos, texturas, animaciones, audio, historia);
4. **assets de terceros** (los que se descarguen para no crear todo a mano).

También están las **dependencias** (SDL3, los headers de Vulkan), cuyas licencias hay
que respetar y documentar.

> Este ADR se basa en documentación pública sobre licencias. No es asesoría legal.

## Criterios

1. **Que se pueda evaluar:** cualquiera puede clonar, compilar y probar el juego sin
   dudas legales.
2. **Protección de lo propio:** decidir cuánto control se conserva sobre el arte y la
   historia.
3. **Compatibilidad** con las licencias de las dependencias y de los assets de
   terceros.
4. **Simplicidad:** licencias conocidas, fáciles de entender para quien lee el repo.
5. **Trazabilidad:** saber, para cada archivo, bajo qué licencia está.

## Opciones

### 1. Licencia del código

| Opción | Qué permite | Qué exige |
|---|---|---|
| **MIT** | Usar, copiar, modificar y redistribuir, incluso de forma comercial | Conservar el aviso de copyright y la licencia. Es muy corta |
| **Apache-2.0** | Lo mismo que MIT | Lo mismo que MIT, más **concesión explícita de patentes**, archivo `NOTICE` y avisar de los cambios. Es más larga |
| **GPL-3.0** | Usar y modificar | **Copyleft:** cualquier obra derivada que se distribuya tiene que ser GPL y publicar su código. Muchas empresas la evitan |
| **MPL-2.0** | Usar y modificar | Copyleft **por archivo**: solo los archivos modificados tienen que seguir siendo MPL |
| **Sin licencia** (todos los derechos reservados) | Solo ver el código y hacer fork en GitHub | — Nadie puede usarlo legalmente, ni siquiera para compilarlo y probarlo |

### 2. Licencia de la documentación

| Opción | Qué significa |
|---|---|
| **La misma que el código** | Simple, pero las licencias de software no están pensadas para texto |
| **CC BY 4.0** | Se puede compartir y adaptar, incluso de forma comercial, **dando crédito**. Es la licencia típica de documentación técnica |

### 3. Licencia de los assets propios

| Opción | Qué permite |
|---|---|
| **CC0** | Dominio público: cualquiera hace lo que quiera |
| **CC BY 4.0** | Todo, dando crédito |
| **CC BY-NC 4.0** | Compartir y adaptar dando crédito, **solo sin fines comerciales** |
| **CC BY-NC-ND 4.0** | Compartir dando crédito, sin fines comerciales y **sin modificar** |
| **Todos los derechos reservados** | Nada, salvo verlos en el repo |

### 4. Política de assets de terceros

| Licencia del asset | ¿Se acepta? |
|---|---|
| **CC0** (Kenney, Quaternius, Poly Haven, ambientCG) | Sí, es la preferida: no exige crédito ni tiene condiciones |
| **CC BY** | Sí, con crédito obligatorio en un archivo de créditos |
| **CC BY-SA** (share-alike) | Con cuidado: obliga a que las obras derivadas usen la misma licencia |
| **Con cláusula NC** (no comercial) | Solo si el juego nunca se vende |
| **Sin licencia clara, o "gratis" sin términos** | No. No hay permiso real |

### 5. Cómo se documenta qué licencia tiene cada archivo

| Opción | Cómo funciona |
|---|---|
| **Simple** | `LICENSE` en la raíz (código), un archivo `LICENSE` dentro de las carpetas con otra licencia, y `CREDITS.md` para los assets de terceros |
| **REUSE (FSFE)** | El estándar de la Free Software Foundation Europe. Cada licencia en `LICENSES/<SPDX>.txt`, un `REUSE.toml` que asigna licencias por carpeta o patrón, y opcionalmente un comentario `SPDX-License-Identifier` en cada archivo. Hay una herramienta (`reuse lint`) que verifica que **ningún archivo quede sin licencia** |

## Investigación

- **Sin licencia no hay permiso.** *choosealicense.com* (de GitHub) y la documentación
  de GitHub explican que, sin licencia, rige el copyright exclusivo por defecto. Los
  términos de servicio de GitHub solo permiten ver el repo y hacer fork, no usarlo ni
  redistribuirlo.
- **No hay que usar Creative Commons para el código.** Creative Commons recomienda en
  su propia FAQ **no usar sus licencias para software**: no tienen términos sobre
  distribuir el código fuente ni sobre patentes, y no son compatibles con las grandes
  licencias de software. **Sí las recomienda para la documentación y para los
  elementos artísticos separados, como el arte o la música de un juego.** Por eso es
  normal que un juego tenga dos licencias: una de software para el código y una CC para
  los assets.
- **MIT frente a Apache-2.0:** la diferencia práctica es la **concesión explícita de
  patentes** de Apache-2.0 (más el archivo `NOTICE` y el aviso de cambios). MIT no
  menciona patentes. Para un proyecto personal sin patentes, la diferencia es chica;
  Apache-2.0 da más claridad legal, MIT más simplicidad.
- **REUSE.** Es una especificación de la FSFE basada en los identificadores SPDX.
  `REUSE.toml` sirve justo para definir una licencia por defecto y excepciones para
  carpetas de terceros. Es útil en un repo que mezcla código, documentación y assets
  con licencias distintas.
- **Assets CC0.** Kenney, Quaternius, Poly Haven y ambientCG publican **todos** sus
  assets bajo CC0: uso comercial, modificación y sin obligación de dar crédito.
- **Dependencias:**
  - **SDL3** es zlib: muy permisiva, solo pide no atribuirse su autoría ni presentar
    versiones modificadas como si fueran la original.
  - **Vulkan-Headers** es Apache-2.0 o MIT.
  - El driver de Mesa no se distribuye con el juego.

  Ninguna dependencia impide usar MIT ni Apache-2.0.

## Decisión

| # | Qué | Elección |
|---|---|---|
| 1 | Código | **MIT** |
| 2 | Documentación (`docs/`, `CREDITS.md`, `CLAUDE.md`) | **CC BY 4.0** |
| 3 | Assets propios | **CC BY 4.0** |
| 4 | Assets de terceros | **Solo CC0 y CC BY.** Los de CC BY se acreditan en `CREDITS.md`. Ninguna otra licencia se acepta |
| 5 | Documentación de licencias | **REUSE**, con `REUSE.toml` (licencias por carpeta) |

Titular del copyright: **Clemente Grass**.

## Consecuencias

- **Archivos nuevos en la raíz:**
  - `LICENSE`: texto MIT, para que GitHub detecte la licencia del repo;
  - `LICENSES/MIT.txt` y `LICENSES/CC-BY-4.0.txt`: textos oficiales de SPDX;
  - `REUSE.toml`: MIT por defecto, y CC BY 4.0 para la documentación y para
    `assets/**`;
  - `CREDITS.md`: tabla de assets de terceros y de dependencias.
- **Verificación:** `uvx reuse lint` confirma que el repo cumple REUSE 3.3. Cuando
  exista CI, ese comando corre en cada PR.
- **Cada asset de terceros nuevo** necesita una fila en `CREDITS.md` (autor, URL,
  licencia, archivos) y, si no es CC BY 4.0 con autor Clemente Grass, una anotación
  propia en `REUSE.toml`.
- **Los assets bajo CC BY 4.0 se pueden reutilizar incluso de forma comercial**, dando
  crédito. Es una elección consciente, coherente con el código abierto.
- **Las dependencias** (SDL3 con licencia zlib, Vulkan-Headers con Apache-2.0 o MIT)
  quedan listadas en `CREDITS.md`. Una dependencia nueva se agrega ahí, además de su
  ADR.

## Cómo lo defiendo

**¿Por qué una licencia abierta? ¿No te pueden copiar el engine?**
Sí, pero me gusta el mundo del código abierto y de las herramientas gratuitas, como
Godot.

**¿Por qué el código y los assets tienen licencias distintas?**
Porque son cosas distintas, con estándares de licencia distintos: las licencias de
software están pensadas para código, y las Creative Commons para obras como el arte.

**Si usas un modelo descargado de internet, ¿cómo sabes que puedes usarlo?**
Hay que revisar su licencia, o asegurarse de que viene de una página que publica todo
bajo un tipo de licencia específico.

## Referencias

- choosealicense.com, *No License*: <https://choosealicense.com/no-permission/>
- GitHub Docs, *Licensing a repository*:
  <https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository>
- Creative Commons, uso de CC para software (FAQ y CC0 para software):
  <https://creativecommons.org/2011/04/15/using-cc0-for-public-domain-software>
- FOSSA, *Open Source Licenses 101: Apache License 2.0*:
  <https://fossa.com/blog/open-source-licenses-101-apache-license-2-0/>
- REUSE Specification 3.0 (FSFE): <https://REUSE.software/spec>
- Fedora Magazine, guía de REUSE:
  <https://fedoramagazine.org/beginners-guide-for-open-source-developers-for-software-licensing-with-fsfe-reuse/>
- Guía de licencias de assets de juegos (CC0, CC BY):
  <https://app.cinevva.com/guides/game-asset-licenses>
