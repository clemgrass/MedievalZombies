# Prototipo de la cámara

Nota de investigación del [ADR-0031](../adr/0031-sistema-de-camara.md). Un mismo tajo con
guion, visto con cada cámara candidata, sobre el render del prototipo del ADR-0022
(forward, estilo PSX, 640×360). Medido en la máquina de referencia (i7-8550U, UHD 620) el
2026-11-03. El prototipo es desechable y vive fuera del repo.

## El prototipo

**Assets:** la caballera es el maniquí femenino y los zombies son el masculino del
*Universal Animation Library* 1 y 2 de Quaternius (CC0), con sus animaciones (caminar,
espada en guardia, el tajo `Sword_Attack`, y caminar, esperar y rascar de zombie). Se
usan solo para visualizar y **no entran al repo**. La espada son cinco cajas puestas en la
mano: el mango pasa por los nudillos y la hoja sale del lado del pulgar.

**El guion** (segundos de simulación, paso de 60 Hz):

| Instante | Qué pasa |
|---|---|
| 0–2,2 s | La caballera camina 2,75 m hacia la horda |
| 2,6 s | Empieza a apuntar: la cámara se acerca y aparece la línea del tajo |
| 3,4 s | Empieza a cargar: la espada sube y se sostiene arriba |
| 4,6 s | Suelta |
| 4,696 s | La hoja cruza el torso del zombie objetivo (entra a los 4,6865 y sale a los 4,7042: 17,7 ms) |
| 4,7–8,5 s | Los pedazos caen; la horda sigue acercándose |

**El corte** es diagonal, del hombro a la cadera. El plano sale de la hoja en el contacto y
de su barrido (normal (−0,69, 0,72, 0,09)). Blender corta la malla posada en ese cuadro y
tapa cada contorno. El maniquí está hecho de piezas separadas, abiertas en las uniones,
así que el corte deja 13 cadenas, no contornos cerrados, y el relleno de Blender solo
cerró 2. **Para el prototipo, cada cadena se tapa con su envolvente convexa en el plano.**
Basta para ver la tapa; el corte de verdad es del ADR-0007.

**La horda:** 30 zombies, 18 de frente y 11 a los costados y atrás, que caminan hacia la
caballera y se detienen a 1,6–1,85 m de ella. La separación entre ellos es simple (no es
el ORCA del ADR-0023).

**Las métricas** salen de un búfer de IDs en el alfa de cada frame (la caballera, la
espada, el zombie objetivo, la tapa, cada zombie). En las corridas de medición, cada frame
se dibuja dos veces, la segunda solo con el objetivo, para saber cuánto del objetivo tapa
la caballera.

**Lo que no se midió:** el tiempo de CPU y de GPU de la cámara. El reloj de la GPU no se
fijó (ADR-0020); los números de abajo son de imagen y de geometría, no de rendimiento.

## Las cámaras

| Cámara | Cómo funciona en el prototipo |
|---|---|
| **Libre** (visión) | Órbita alrededor de un pivote a 1,6 m de altura y 0,5 m a la izquierda; 2,3 m detrás y FOV de 51°. Al apuntar: 2,0 m y FOV de 30°, y el rayo central va al punto apuntado |
| **Fija** | Un punto puesto por el diseñador, alto y de costado (−4,6, 3,6, 4,4). Solo gira para seguir a la caballera y al objetivo. FOV de 50° |
| **Riel** | Se desliza junto a la caballera, a 4,2 m de costado y 3 m de altura, mirando la pelea. FOV de 45° |
| **Guiada** | La libre, y al soltar el tajo se mueve en 0,25 s a un encuadre del corte: del lado de la normal del plano, de costado al barrido, a 2,4 m. Vuelve 1,6 s después |
| **Primera persona** | El ojo en el hueso de la cabeza, que se colapsa a un punto junto con el cuello para que no se vea por dentro. FOV de 70°, y de 50° al apuntar |
| **Mixta** | La libre, y primera persona mientras apunta y carga |

En la fija y el riel el rayo central no pasa por el objetivo, así que la mira se dibuja
donde se proyecta el punto apuntado (como un cursor en pantalla).

**La cámara del tajo** cambia la escala de tiempo: ×0,02 mientras la hoja cruza (con
30 ms de margen) y ×0,2 durante 0,9 s después. Las tres variantes:

- **Acompaña la hoja:** la cámara va con la mitad de la hoja, del lado de la normal, y
  después de salir se queda en el corte.
- **Órbita:** parte de donde estaba la cámara de juego y gira 110° alrededor del corte,
  a 1,8 m.
- **Plano de cine:** fija, baja y de costado al corte.

**Los rayos X** se activan desde 0,25 s antes del contacto:

- **Cápsulas:** la piel del objetivo se dibuja con *screen-door* (6 de 16) y adentro van
  los huesos, como cilindros a lo largo de las cápsulas del ADR-0016 posadas.
- **Bisagra (la herida que se abre):** mientras la hoja cruza, se dibujan los dos pedazos.
  El de arriba gira hasta 22° alrededor del punto del borde más alejado del lado sin
  barrer, y la tapa se ve solo del lado por donde ya pasó la hoja (un semiplano en el
  shader).

**La sacudida** se suma en el contacto (trauma 0,75, que baja 1,4 por segundo) y solo
gira la cámara que se dibuja. El rayo de apuntar sale de la cámara sin sacudida.

**La colisión** se probó en un pasillo de 2,4 m de ancho que dobla en ángulo recto:

- **Rayo:** del pivote a la cámara, contra cajas, con 5 cm de margen.
- **Esfera:** de 0,2 m de radio, el doble de la media diagonal del plano cercano a 0,1 m.
  Se acerca de golpe y se aleja con un resorte (ω = 4).
- **Esfera suave:** se acerca con un resorte rápido (ω = 22), sin pasar nunca el límite de
  una esfera de 0,1 m.

## Resultados

### P2 a P4: las cámaras de juego

Mientras apunta (de 3,1 a 4,6 s), en el 1,5 s después del corte y en la horda (de 6 a
14 s):

| Cámara | Objetivo en px | Tapado por la caballera | px por cm en el punto apuntado | Tapa en px (media / máx) | Caballera en pantalla | Zombies visibles | Frames con amenazas fuera de cuadro (máx a la vez) |
|---|---|---|---|---|---|---|---|
| Libre | 14 840 | 9 % | 1,85 | 441 / 1 365 | 4,5 % | 20,5 | 93 % (3) |
| Fija | 1 506 | 2 % | 0,55 | 139 / 164 | 0,7 % | 25,2 | 0 % (0) |
| Riel | 2 008 | 1 % | 0,66 | 61 / 136 | 0,9 % | 23,1 | 0 % (0) |
| Guiada | 14 835 | 9 % | 1,85 | **1 526 / 1 805** | 5,8 % | 19,8 | 99 % (6) |
| Primera persona | 40 337 | 0 % | 2,93 | **2 931 / 4 734** | 0,1 % | 14,2 | 100 % (13) |
| Mixta | 40 337 | 0 % | 2,93 | 1 027 / 3 785 | 3,2 % | 20,7 | 93 % (3) |

Una "amenaza fuera de cuadro" es un zombie a menos de 2,6 m de la caballera con menos de
20 píxeles a la vista. La pantalla tiene 230 400 píxeles. En el instante de soltar, con la
cámara libre, la caballera ocupa el 9,7 % de la pantalla y tapa el 10 % del objetivo.

Lo que se ve, además de los números:

- **La fija y el riel leen la horda entera,** pero el objetivo es chico: una junta de
  10 cm mide ~6 píxeles. La tapa casi no se ve.
- **La primera persona es la que más muestra el corte** y la que peor muestra la horda.
  La espada entra en diagonal por el costado y el zombie llena la pantalla.
- **La guiada triplica la tapa de la libre** y, fuera del tajo, es la libre.
- **La mixta** muestra el corte desde la primera persona, pero el paso de una a otra, de
  0,3 s, cambia mucho la imagen.

### P5: el determinismo

Un programa aparte, sin GPU: el jugador gira la cámara con el mouse y suelta el tajo en el
paso 72. Se graba el input por paso (ADR-0021) jugando a 144 FPS y se reproduce a otras
tasas. El punto apuntado es el impacto del rayo central contra una pared a 6 m.

| Reproducción | A. Cámara en el paso fijo | B. Cámara en el frame (suavizada con el dt del frame) |
|---|---|---|
| 30 FPS | 4,920841 m (el mismo hash) | se corre 21,8 mm |
| 60 FPS | 4,920841 m (el mismo hash) | se corre 11,7 mm |
| 144 FPS | 4,920841 m (el mismo hash) | 0,0 mm (es la grabación) |
| 40–144 FPS irregular | 4,920841 m (el mismo hash) | se corre 11,4 mm |

La misma salida con GCC y Clang, de -O0 a -O3. **Si la cámara que decide el rayo vive en
el frame, el *replay* corta en otro lugar.** Un centímetro es la mitad de una junta chica.

### P6: el suavizado y la colisión

Tiempo hasta el 95 % de un salto:

| FPS | `lerp` de 0,15 por frame | `lerp` exponencial, 1 − e^(−10·dt) | Resorte críticamente amortiguado, ω = 10 |
|---|---|---|---|
| 30 | 633 ms | 300 ms | 500 ms |
| 60 | 317 ms | 300 ms | 483 ms |
| 144 | 132 ms | 306 ms | 479 ms |

El `lerp` fijo por frame es casi 5 veces más lento a 30 FPS que a 144. Los otros dos solo
varían por la cuantización del frame. El resorte, además, arranca y frena suave (no tiene
el salto de velocidad del exponencial).

El pasillo, en 660 frames:

| Colisión | Frames con la cámara dentro de la pared | Frames con el plano cercano a menos de 0,10 m de la pared | Salto por frame, mediana / máx |
|---|---|---|---|
| Ninguna | 26 | 36 | 2,3 / 9,9 cm |
| Rayo | 0 | 33 | 2,3 / 36,7 cm |
| Esfera, de golpe | 0 | **0** | 2,3 / 58,4 cm |
| Esfera, resorte rápido | 0 | **0** | 2,3 / 43,5 cm |

El rayo deja la cámara fuera de la pared, pero el plano cercano la corta en las esquinas
(se ve la pared por dentro). La esfera no, a cambio de un salto al doblar. En este pasillo
la cámara nunca quedó a menos de 1 m de la caballera, así que el *screen-door* del
ADR-0022 no se activó.

### P7: la cámara del tajo y los rayos X

Cada clip dura 7,7 s reales para 0,9 s de simulación.

| Variante | Tapa en px (media / máx) | Mientras la hoja cruza: objetivo en px | Mientras la hoja cruza: tapa en px |
|---|---|---|---|
| Sin cámara del tajo (libre, 0,9 s) | 614 / 1 365 | 14 305 | 157 (2 frames) |
| Acompaña la hoja | **1 923 / 4 180** | 26 955 | 82 |
| Órbita | 598 / 1 644 | 27 797 | 322 |
| Plano de cine | 666 / 1 562 | 17 857 | 93 |
| Cine con rayos X de cápsulas | 666 / 1 562 | 15 691 | 93 |
| Hoja con bisagra | **2 111 / 4 184** | 29 833 | **371** |

- **Mientras la hoja está adentro, la tapa casi no existe** (ADR-0022): los pedazos
  todavía no se separan. Con cámara súper lenta eso son ~200 frames en los que no se ve
  el interior.
- **La bisagra multiplica por 4 la tapa visible mientras la hoja cruza.** A simple vista,
  desde el ángulo de la hoja, la apertura es sutil: hay que afinar el ángulo y el lado.
- **Los rayos X de cápsulas** muestran los huesos (cilindros claros dentro de la piel en
  *screen-door*), pero no suman tapa: muestran otra cosa, el esqueleto.
- **Acompañar la hoja** es la que más muestra el corte después de que sale.

### P8: la sacudida

Con sacudida al azar o con trauma, **el rayo de apuntar pasa a 0,00 cm del punto
apuntado**, igual que sin sacudida: la sacudida solo gira lo que se dibuja. El azar por
frame tiembla; el ruido suave se siente como un golpe.

### P9: el video

`comparativas_adr0031.mp4`, 127 s, 17 clips rotulados, a 640×360 escalado ×2 con
*nearest*:

1. Tercera persona libre (visión).
2. Fija por zona, que gira para seguir (estilo *God of War*, 2005).
3. Riel, que se desliza con la caballera (estilo *God of War*, 2005).
4. Guiada: libre, y al soltar encuadra el corte.
5. Primera persona.
6. Mixta: primera persona solo al apuntar y cargar.
7. Cámara del tajo que acompaña la hoja.
8. Cámara del tajo en órbita.
9. Cámara del tajo en plano de cine.
10. Rayos X con cápsulas.
11. La herida que se abre (bisagra).
12. Sacudida al azar.
13. Sacudida con trauma y ruido suave.
14. Pasillo sin colisión.
15. Pasillo con rayo.
16. Pasillo con esfera, de golpe.
17. Pasillo con esfera y resorte rápido.

## Cómo repetirlo

```sh
# Assets CC0 de Quaternius (UAL1 y UAL2 Standard) en ../assets_cc0, fuera del repo.
blender -b --python tools/export31.py -- <UAL1.glb> <UAL2.glb> <Mannequin_F.glb> data
./build.sh
./c31 mode=plan > data/plan.txt
blender -b --python tools/cut31.py -- <UAL1.glb> data/plan.txt data <UAL2.glb>
./run_matrix.sh && python3 tools/summary.py   # P2–P4, P6 (pasillo), P7, P8
g++ -std=c++20 -O2 src/det.cpp -o det && ./det # P5 y P6 (suavizado)
./make_video.sh                                # P9
```
