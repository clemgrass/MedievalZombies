# Prototipo de cinemáticas

Nota de investigación del [ADR-0034](../adr/0034-cinematicas.md).
Guarda las mediciones que sostienen la decisión, para poder repetirlas.

Fecha: 2026-11-06, en la máquina de referencia del ADR-0020 (Dell XPS 13 9360,
i7-8550U, Intel UHD 620). Son mediciones de laboratorio para decidir, no del protocolo
del ADR-0020. La CPU y la GPU corrieron con el reloj libre. El prototipo es código
descartable, fuera del repo.

| Qué | Versión |
|---|---|
| Linux | 7.2.15 |
| GCC | 16.2.1, con `-O2`, sin `-march` (ADR-0008) |
| Blender | 5.2.2 LTS |
| cgltf | 1.15 |
| Mesa (ANV) | 26.2.5, Vulkan 1.4 |
| pl_mpeg | `master` del 2026-11-06 (MIT) |
| FFmpeg | el de Arch del 2026-11-06 |

**Los assets:** la caballera y los zombies son los maniquíes CC0 de Quaternius
(*Universal Animation Library* 1 y 2), los mismos del ADR-0031, fuera del repo. Solo
sirven para ver; no son los del juego.

## El prototipo

- **`p1/escena.py`:** arma en Blender una cinemática de prueba: un actor con dos acciones
  en la NLA (caminar y quedarse quieta), un *empty* padre que lo desplaza, una cámara que
  se mueve, gira y cambia la distancia focal, otra quieta, **marcadores de la línea de
  tiempo** (dos de cambio de cámara, uno de subtítulo y uno de sonido) y propiedades
  propias. La exporta a glTF con cinco configuraciones.
- **`p1/marcadores.py`:** copia los marcadores a una propiedad de la escena antes de
  exportar.
- **`p1/leer.c`:** lee con cgltf 1.15 el glTF con el FOV animado.
- **`p2/p2.cpp`:** un secuenciador de juguete sobre el paso fijo de 60 Hz del ADR-0012:
  una pista de posiciones del actor, muestreada a 30 Hz, y una pista de eventos (unos
  cambian la partida, otros solo la presentación), con un mundo de 64 zombies que caminan
  al azar y se acercan al actor. Sin ventana.
- **`p3/src/k34.cpp`:** sobre el render Vulkan del prototipo del ADR-0031 (forward, PSX,
  640×360). La caballera camina por la arena; al cruzar un trigger, una cinemática de
  dos tomas presenta al jefe (toma A, contrapicado con *dolly*) y a la caballera (toma
  B, de frente), y el juego sigue. Cada clip cambia la transición de ida y la de vuelta,
  el estilo de cámara del ADR-0031 y las bandas. Mide la brusquedad de cada transición.
- **`p3/make_video.sh`:** arma `comparativas_adr0034.mp4` (154 s, 14 clips, ×2 con
  *nearest*, rotulado, con el estado de la cámara arriba a la derecha y los subtítulos
  con la fuente Jersey 10 del ADR-0026).
- **`p5/p5.cpp`:** la ventana de un *parry* (el caso del dragón) en el paso fijo, con
  tres reglas para meter la suelta en un paso.
- **`p6/peso_clips.py`:** cuánto pesa la cinemática del P1 cocinada como el ADR-0017.
- **`p6/video.c`:** decodifica un minuto de MPEG-1 a 640×360 con `pl_mpeg` y mide el
  tiempo por frame.

## P1. Blender → glTF: qué sobrevive

| Qué se armó en Blender | `ACTIONS` (por defecto) | `NLA_TRACKS` | `SCENE` | `SCENE` + `export_pointer_animation` |
|---|---|---|---|---|
| Las 43 acciones del rig importado | 43 clips | 43 clips + la pista `cine` | No (solo lo que suena en la escena) | No |
| La secuencia de la NLA (caminar, después quieta) | No | Un clip `cine` de 5,33 s | **Horneada en el clip del actor** | **Horneada** |
| El desplazamiento del *empty* padre | Clip aparte | No | Clip del objeto, 151 muestras | Igual |
| La cámara: posición y giro | Clip aparte | No | Clip de la cámara | Igual |
| La cámara: distancia focal (el FOV) | No | No | No | **Canal `KHR_animation_pointer`** a `/cameras/0/perspective/yfov` |
| Marcadores de la línea de tiempo | No | No | No | No |
| Cambio de cámara por marcador | No | No | No | No |
| Propiedades de un *empty* y de la escena | `extras` | `extras` | `extras` | `extras` |

- **El modo `SCENE` hornea toda la línea de tiempo** a la frecuencia de la escena (30 Hz,
  151 muestras para 5 s): un clip por objeto animado, con la NLA ya resuelta.
- **cgltf 1.15 no conoce `KHR_animation_pointer`,** pero no falla: el canal llega sin nodo
  y con la extensión como JSON crudo (`{"pointer": "/cameras/0/perspective/yfov"}`), y su
  muestreador se lee como cualquier otro. Leerlo son unas pocas líneas del cocinador.
- **Los marcadores se pierden,** y con ellos el cambio de cámara. **Copiarlos a una
  propiedad de la escena antes de exportar funciona:** llegan a los `extras` como una
  lista de `{nombre, frame, camara}`.

Es el camino que eligió el autor (ver el ADR): las cinemáticas se arman en Blender, el
script de exportación copia los marcadores a los `extras` y el cocinador lee el FOV del
canal de `KHR_animation_pointer`. La vuelta a la cámara del jugador la hace el engine.

## P2. El reloj y el salto

Una cinemática de 20 s (600 cuadros a 30 Hz, 1200 pasos), con eventos en los cuadros 20,
90, 91, 300, 450 y 599. Un hash de control cada 60 pasos, como el del ADR-0021.

**El reloj:**

| Reloj | FPS | Ruido del dt | Hashes de control distintos | Eventos corridos |
|---|---|---|---|---|
| Número de paso | 30, 60, 144 | 0 y ±20 % | **0 de 20** | **±0 pasos** |
| Suma del dt del frame | 30 | 0 | 19 de 20 | ±1 paso; un paso de menos |
| Suma del dt del frame | 60 | 0 | 1 de 20 | ±1 paso |
| Suma del dt del frame | 60 | ±20 % | 20 de 20 | ±0 pasos |
| Suma del dt del frame | 144 | 0 y ±20 % | 20 de 20 | ±0 pasos |

Con el dt del frame, la pose del actor se muestrea en un instante que depende de los FPS,
y los zombies, que se acercan a él, terminan en otro lado. Aunque el hash final a veces
coincide (la pose final se pisa con la última clave), los intermedios no.

**El salto** (en 172 puntos, cada 7 pasos):

| Mundo | Salto | Iguales a verla entera | Costo |
|---|---|---|---|
| Congelado | **Directo:** la pose final y los eventos de la partida que faltan, en orden | **172 de 172** | 0,001 ms |
| Congelado | Adelantar los pasos sin dibujar | 172 de 172 | 0,05 ms |
| Andando | Directo | **0 de 172** | 0,001 ms |
| Andando | Adelantar | 172 de 172 | 0,4 ms en el juguete; en el juego, hasta 6 ms por paso (ADR-0020): **~7 s para 20 s de cinemática** |

**Si el mundo sigue andando, saltar no puede ser instantáneo y a la vez dar la misma
partida.** Con el mundo congelado, el salto directo es exacto y gratis.

## P3. Las transiciones de cámara

La cinemática va de 3,0 a 8,5 s de juego. La métrica mira las ventanas de la ida (3,0 a
5,4 s) y de la vuelta (8,5 a 11,5 s), sin contar el cambio de toma A→B, que es de autor.
"A la caballera" es la menor distancia del ojo al eje de su cuerpo (de 0,3 a 1,55 m de
altura); "asienta" es cuándo la cámara queda a menos de 1 cm y 0,5° de su destino.

| Clip | Transición | Ida: giro máx | Vuelta: giro máx | Vuelta: a la caballera | Vuelta: a un pilar | Asienta |
|---|---|---|---|---|---|---|
| 1 | Corte seco | salto | salto | 1,66 m | 2,05 m | al instante |
| 2 | Mezcla de 0,5 s | 92 °/s | 493 °/s | **0,10 m** | 2,21 m | 0,5 s |
| 3 | Mezcla de 1,5 s | 35 °/s | 164 °/s | **0,14 m** | 2,70 m | 1,5 s |
| 4 | Resorte (ω = 5) | 54 °/s | 302 °/s | **0,12 m** | 2,36 m | **nunca** (en 3 s) |
| 5 | Órbita de 1,5 s | 35 °/s | 164 °/s | 1,49 m | 2,71 m | 1,5 s |
| 6 | Empalme | (corte) | **0 °/s** | 2,36 m | 2,05 m | al instante |
| 8 | Fija, mezcla de 1 s | 91 °/s | 251 °/s | 0,32 m | **0,00 m** | 1 s |
| 9 | Fija, empalme | (corte) | 198 °/s | 7,02 m | 2,06 m | al instante |
| 10 | Primera persona, mezcla de 1 s | 46 °/s | 246 °/s | (en la cabeza) | 4,54 m | 1 s |
| 11 | Primera persona, empalme | (corte) | **0 °/s** | (en la cabeza) | 3,86 m | al instante |

- **La mezcla lineal no sabe nada del mundo.** La toma B está de frente a la caballera y
  la cámara de juego, detrás: la recta entre las dos la atraviesa (0,10 a 0,14 m del
  eje, con cualquier duración). Con la cámara fija, la recta cruza un pilar.
- **El resorte llega tarde a la toma de autor** (se ve como un encuadre que se acomoda) y,
  con ω = 5, no termina de asentarse en 3 s: la cámara de juego se mueve mientras tanto.
- **La órbita** (interpolar el ángulo, el radio y la altura alrededor de la caballera)
  nunca se acerca más que el menor de los dos radios. Necesita un sujeto: sirve para
  volver a la caballera, no para cualquier par de tomas.
- **El empalme** es la toma B que gira alrededor de la caballera y, en su último 45 %,
  se mezcla hacia la pose de reposo de la cámara del estilo. **Al terminar la
  cinemática no queda nada que mezclar:** 0 °/s. El recorrido lo diseña el autor, así
  que puede rodear lo que haga falta.
- **El corte seco de ida no se siente como un error:** la primera toma presenta la escena
  (es la gramática del cine). El de vuelta sí: se pasa de mirar de frente a la caballera
  a mirarla de espaldas.

## P4. Las bandas y los subtítulos

A 640×360 (16:9):

| Formato | Alto de la imagen | Cada banda |
|---|---|---|
| Sin bandas | 360 px | 0 |
| 1,85:1 | 346 px | **7 px** |
| 2,39:1 | 268 px | **46 px** |

- Con 2,39:1, una línea de subtítulos de ×1 (18 px por em) o de ×2 (36 px) entra en la
  banda; **dos líneas de ×2 no**.
- Con 1,85:1, la banda es una marca, no un lugar: **los subtítulos van sobre la imagen,**
  con el fondo del ADR-0026.
- Las bandas que entran en 0,5 s marcan el paso a "ahora no controlas"; las que aparecen
  de golpe acompañan el corte seco.

## P5. La ventana del parry

La bola llega en T. Soltar en [T − W, T] es *parry*; antes, el dragón ataca; después, la
bola pega. 200 000 sueltas al azar en [T − W − 0,1 s, T + 0,1 s], con el error de ±2 ms
del *timestamp* de SDL (P1 del ADR-0032). Porcentaje de veredictos distintos del
verdadero:

| W | FPS | Regla C (ADR-0032) | Borde del frame | C + instante dentro del paso |
|---|---|---|---|---|
| 50 ms | 30 / 60 / 144 | 6,6 / 6,6 / 6,7 % | 13,4 / 7,8 / 6,7 % | 0,8 / 0,8 / 0,8 % |
| 100 ms | 30 / 60 / 144 | 5,6 / 5,6 / 5,6 % | 11,2 / 6,5 / 5,6 % | 0,7 / 0,7 / 0,7 % |
| 150 ms | 30 / 60 / 144 | 4,8 / 4,8 / 4,8 % | 9,6 / 5,6 / 4,8 % | 0,6 / 0,6 / 0,6 % |
| 200 ms | 30 / 60 / 144 | 4,2 / 4,1 / 4,2 % | 8,3 / 5,0 / 4,2 % | 0,5 / 0,5 / 0,5 % |

- **Con la regla C, el error solo está en los bordes de la ventana,** a menos de un paso
  (16,7 ms), y **no depende de los FPS**. Es la misma granularidad que un juego de pelea
  que juzga por *frames*.
- Leer el evento en el borde del frame duplica el error a 30 FPS.
- Llevar el instante dentro del paso (un byte, 1/256 de paso) lo baja unas 8 veces; lo
  que queda es el ruido del *timestamp*. El autor eligió **dejar la ventana en CVars y en
  pasos**: el ancho se ajusta jugando.

## P6. El costo: en el engine o en video

**En el engine,** cocinado como el ADR-0017 (30 Hz, `float`, cada canal constante una
sola vez):

| Qué | Peso |
|---|---|
| El actor (65 huesos, 195 canales) | 117,4 KB cada 5 s |
| Su desplazamiento | 1,8 KB cada 5 s |
| La cámara (posición, giro y FOV) | 4,7 KB cada 5 s |
| **Total** | **125,6 KB cada 5 s → ~1,5 MB por minuto** |

Con los ~23 huesos que deja el recorte del ADR-0017, un personaje baja a ~0,5 MB por
minuto. Si los actores solo nombraran clips que ya existen (los del global), la
cinemática guardaría claves y eventos, de pocos KB; horneada desde Blender, como eligió
el autor, un minuto con tres personajes recortados pesa ~1,5 MB.

**En video,** MPEG-1 a 640×360 y 30 fps, el primer minuto del video del P3, decodificado
con `pl_mpeg` en un hilo:

| Tasa | Peso por minuto | Decodificar (mediana / p99 / máx) | A RGB (mediana) |
|---|---|---|---|
| 1 Mbit/s | 7,3 MB | 0,92 / 2,46 / 8,78 ms | 0,83 ms |
| 2 Mbit/s | 13,9 MB | 1,50 / 3,22 / 9,34 ms | 0,84 ms |
| 4 Mbit/s | 21,9 MB | 2,15 / 4,44 / 7,80 ms | 0,81 ms |

El costo en CPU entraría en el frame (el mundo está detenido), pero el video **no puede
ser interactivo**, no muestra el estado de la partida (los cortes, los pedazos) ni el
estilo de cámara del jugador, **pesa de 5 a 15 veces más** y suma una dependencia con
su propio ADR.

## P7. Cómo lo hacen otros

- **Unreal Engine, *Sequencer*:** una *Level Sequence* con pistas (cámaras, actores,
  eventos, audio) y una pista de cortes de cámara (*Camera Cuts*), que puede mezclar la
  entrada y la salida hacia la cámara de juego. Cada sección decide qué pasa al terminar:
  ***Restore State*** (todo vuelve a como estaba) o ***Keep State*** (se queda como
  terminó). El viejo *Matinee* tenía ***Fire Events when Jumping Forwards***: al saltar
  una cinemática, los eventos que se saltan igual se disparan. Es el mismo problema que
  el salto directo del P2.
- **Unity, *Timeline*:** un *PlayableDirector* reproduce pistas; las ***Signal Tracks***
  emiten eventos a un receptor (por ejemplo, detener al jugador o empezar la pelea), y
  **Cinemachine** mezcla entre cámaras con una duración por clip, o corta.
- **Godot, `AnimationPlayer`:** cualquier propiedad es animable; las pistas de **llamada a
  métodos** disparan lógica en un instante y las de **audio** reproducen sonido. Las
  llamadas no se ejecutan al previsualizar en el editor.
- ***Metal Gear Solid*** (1998) usó cinemáticas en tiempo real cuando la mayoría de los
  juegos de PlayStation usaban video pregrabado (***Final Fantasy VII***): el paso de la
  historia al juego no tiene salto visual.
- **Naughty Dog:** desde *Uncharted 4*, todas sus cinemáticas son en tiempo real; arman
  las escenas con el pivote en el personaje para que la cámara se comporte como la de
  juego, y eso permite la mezcla. En *Uncharted 2* (GDC 2010) ya buscaban que la historia
  y el juego se sintieran una sola cosa.
- ***God of War*** (2018) no tiene ni un corte de cámara en todo el juego. Su director de
  fotografía lo comparó con escribir una novela sin sustantivos: se pierden el plano y
  contraplano, los planos de situación y los cambios de lente.
- **Los *quick time events*** vienen de los juegos de *laserdisc* (*Dragon's Lair*, 1983);
  *Shenmue* les dio el nombre, y *Resident Evil 4* y *God of War* (2005) los llevaron a la
  acción.
- **Accesibilidad:** las Xbox Accessibility Guidelines (XAG 116) usan "mantener para
  saltar" como ejemplo. *Ratchet & Clank* deja pausar cualquier cinemática y saltarla
  desde la pausa.

## Cómo repetirlo

```sh
# con cgltf en ../dl/cgltf, pl_mpeg en ../dl/plmpeg y los assets CC0 en ../../assets_cc0
blender -b --factory-startup --python p1/escena.py -- "<UAL1_Standard.glb>" p1/out   # P1
blender -b p1/out/escena.blend --python p1/marcadores.py -- p1/out
python3 -I p1/inspeccion.py p1/out
cc -O1 p1/leer.c -o p1/leer -lm && p1/leer p1/out/escena_puntero.gltf
g++ -std=c++20 -O2 p2/p2.cpp -o p2/p2 && p2/p2                                        # P2
p3/make_video.sh                                                                       # P3 y P4
g++ -std=c++20 -O2 p5/p5.cpp -o p5/p5 && p5/p5                                        # P5
python3 -I p6/peso_clips.py p1/out/escena_puntero.gltf                                # P6
cc -O2 p6/video.c -o p6/video -lm
ffmpeg -t 60 -i p3/comparativas_adr0034.mp4 -an -vf scale=640:360:flags=neighbor \
  -r 30 -c:v mpeg1video -b:v 2000k -maxrate 2000k -bufsize 1M -g 15 -bf 2 p6/cine.mpg
p6/video p6/cine.mpg
```
