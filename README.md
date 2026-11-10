# MedievalZombies

Un juego de acción en tercera persona sobre un apocalipsis zombie medieval, con engine
propio. Un caballero con armadura completa se abre paso a tajos de mandoble por un
castillo tomado por los muertos. **Cada tajo lo apuntas y lo cargas tú, y corta la malla
del zombie de verdad, por donde lo apuntaste.**

> **En desarrollo temprano:** todavía no hay una versión jugable.

## El juego

En un día de celebración, casi todo el castillo se convirtió en zombies, y nadie sabe por
qué. La princesa quedó atrapada en la torre más alta, y un caballero tiene que atravesar
el castillo y sus alrededores para llegar hasta ella.

### El tajo

En la mayoría de los juegos de zombies se dispara o se golpea con animaciones fijas.
Aquí **el ángulo, la fuerza y el punto del corte los decide el jugador:**

- **Apuntar.** El mouse o el stick guían la espada, y una mira marca dónde va a pegar:
  al cuello, a la cintura o a la junta de una armadura.
- **Cargar.** Mantener el ataque carga la fuerza, y mientras carga se gira la línea del
  tajo. Al soltar sale el tajo. La precisión
  decide *por dónde* corta, y la fuerza *hasta dónde* llega: un brazo, un zombie entero
  o varios en línea.
- **Todo pesa.** Cargar, corregir el ángulo, trotar y soltarse de un agarre gastan la
  misma energía. El caballero no tiene barra de vida: los zombies lo agarran, y si se
  queda sin energía antes de soltarse, muere.

### Los enemigos

Campesinos lentos, soldados con cota de malla y soldados con armadura completa, cada uno
con su resistencia al corte. Contra la armadura no basta con pegar fuerte: hay que
buscar las juntas y el ángulo. Al final espera un zombie distinto a los demás, que llama
a la horda y se esconde detrás de un guardaespaldas acorazado.

Un zombie partido no siempre muere. Un corte en la cabeza lo mata, pero mientras
conserve la cabeza y algo más, sigue: si se corta por la cintura, la mitad de arriba se
arrastra hacia el caballero.

### El estilo

Estilo PSX oscuro: geometría *low poly*, texturas de pocos píxeles sin filtrar, niebla y
una resolución interna baja. La paleta es apagada (piedra, ocre, gris verdoso), y **el
rojo es el único color saturado**: en medio de la horda siempre se ve dónde se cortó.

## El engine

El engine es propio y está escrito en **C++20**. Se apoya en **Vulkan 1.3** como API
gráfica y en **SDL3** para la ventana, el input, el tiempo y el audio. Lo que define al
juego es propio:

| Parte | Qué hace |
|---|---|
| Render | Dibuja con Vulkan a 640×360 y escala la imagen a la pantalla |
| Corte de mallas | Parte la malla animada del zombie por el plano del tajo, en la CPU |
| Combate | Decide hasta dónde llega la hoja según la fuerza, el material y el ángulo |
| Animación | Mezcla clips por capas y anima el tajo de forma procedural con IK |
| Física | Cuerpos rígidos, ragdolls y personajes, con un solver de impulsos con subpasos |
| Arquitectura | Un ECS propio con *sparse sets* y un paso de simulación fijo a 60 Hz |

La meta es **60 fps estables en una GPU integrada** (Intel UHD 620) con 50 zombies en
pantalla y unos 100 pedazos en el suelo. El estilo de baja resolución y *low poly* no es
solo estético: abarata el trabajo de la GPU y deja tiempo de CPU para cortar.

## Cómo funciona el corte

El tajo corta la geometría de verdad: no es un efecto del shader ni un corte en puntos
fijos. Cuando la hoja atraviesa a un zombie, dentro del paso de simulación pasa esto:

1. **El plano.** El plano del tajo pasa por el pecho del caballero y por el punto
   apuntado, con la dirección de la línea que dibujó el jugador.
2. **La pose.** El zombie se está moviendo, así que la CPU deforma su malla en la pose
   exacta del momento del golpe, con la misma cuenta que hace la GPU al dibujarlo.
3. **Clasificar.** Cada vértice queda de un lado del plano según su distancia con signo.
   Los vértices que el modelo duplica en las costuras se clasifican una sola vez, y un
   desempate fijo (una distancia de exactamente 0 cuenta como positiva) evita que el
   redondeo deje agujeros.
4. **Partir.** Los triángulos que cruzan el plano se parten. Cada punto de corte se
   calcula una sola vez por arista, así que los dos lados quedan cosidos exactamente.
5. **La tapa.** Los bordes del corte forman contornos cerrados, que se triangulan
   cortando orejas para cerrar cada pedazo. En esa cara nueva se ve el interior: carne y
   hueso.
6. **Seguir animado.** Los vértices nuevos se llevan de vuelta a la pose de referencia
   (des-deformar), con los pesos de sus huesos. Así la mitad que sigue viva se sigue
   animando con el mismo esqueleto, y la herida no se despega al moverse.
7. **Los pedazos.** Un modelo real es varias cáscaras superpuestas (torso, brazos,
   piernas), así que los fragmentos se agrupan por el hueso que más los mueve. Un corte
   puede dar más de dos pedazos: por la cintura de un zombie agachado caen las piernas y
   también las manos.
8. **La hoja clavada.** Si la fuerza no alcanza para atravesar, la hoja se detiene
   dentro del cuerpo. Lo que alcanzó a atravesar entero se separa (un antebrazo, por
   ejemplo), y donde se detuvo queda una hendidura de verdad, abierta en cuña: más ancha
   en la boca de la herida y cerrada en el fondo.

Los pedazos comparten el buffer de vértices del modelo original y solo suben a la GPU
sus índices y sus vértices nuevos. Los pedazos se pueden volver a cortar, incluso en el
suelo. En un prototipo, un corte de una malla de 8500 vértices, con la deformación
incluida, tomó de 0,5 a 0,8 ms sin optimizar, en el mismo portátil de la meta de 60 fps
(el mejor de 300 repeticiones, medido fuera del protocolo). Un zombie del juego tiene
un tercio de esos vértices: serían unos 0,2 a 0,3 ms.

## Cómo funciona el combate

- **La fuerza se gasta al cruzar el cuerpo.** Cuanto más área atraviesa la hoja, más
  fuerza consume, y la que sobra sigue al zombie siguiente.
- **La carne no frena la entrada, el metal sí.** Cada parte de la armadura tiene un
  umbral de entrada. Bajo el umbral, el tajo rebota y el caballero queda aturdido un
  momento.
- **El ángulo y las juntas cuentan.** Un tajo que cruza el hueso de costado entra mejor
  que uno que corre a lo largo de él, y cerca de una articulación el umbral baja.
- **La hoja puede quedar clavada.** Si la fuerza alcanza para entrar pero no para
  atravesar, empieza una secuencia de *quick time*: el zombie avanza ensartado en la
  espada, el resto de la horda casi se detiene, y el jugador tiene que terminar el corte
  antes de que lo agarre o se quede sin energía.
- **El impacto.** En cada tajo, el caballero y el zombie golpeado se congelan entre
  50 y 80 ms mientras el resto sigue (*hit-stop*). En los cortes especiales, una cámara
  lenta acompaña a la hoja a través del cuerpo.

## Cómo funciona el tajo

- **No hay golpes grabados.** La punta de la espada recorre un arco en el plano del
  tajo, los dos brazos alcanzan la empuñadura con IK analítica de dos huesos, el torso
  gira siguiendo la punta, y un clip le da el peso del cuerpo: el paso adelante y la
  carga.
- **El golpe se detecta en el plano.** El cuerpo de cada zombie es un conjunto de
  cápsulas, una por hueso. Entre dos pasos de simulación la hoja barre un sector de
  anillo dentro del plano, y cada cápsula que toca es un golpe, en orden.
- **Los pedazos tienen física.** Al cortar, las cápsulas se recortan hacia el lado de
  cada pedazo. Los pedazos chicos son cuerpos rígidos, los grandes pasan a *ragdoll*, y
  los personajes los empujan al pasar sin trabarse.

## Compilar

En Linux, con CMake 3.29 o más nuevo, Ninja, Clang y `lld`:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
./build/debug/game/game
```

Hay tres presets: `debug`, `relwithdebinfo` (optimizado, con símbolos y *frame pointers*,
para perfilar) y `release`. Cada uno compila en `build/<preset>`.

Los warnings del código propio son errores. Para experimentar sin que detengan el build:
`cmake --preset debug --compile-no-warning-as-error`.

## Licencia

- **Código:** [MIT](LICENSES/MIT.txt).
- **Documentación y assets propios:** [CC BY 4.0](LICENSES/CC-BY-4.0.txt).
- **Dependencias y assets de terceros:** cada uno con su licencia y su autor, en
  [`CREDITS.md`](CREDITS.md).

Más detalle técnico en [`docs/`](docs/README.md).
