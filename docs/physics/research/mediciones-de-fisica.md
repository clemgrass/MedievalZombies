# Mediciones de física

Nota de investigación del [ADR-0016](../adr/0016-fisica.md). Guarda las cuentas y la
medición que sostienen la decisión, para poder repetirlas.

Fecha: 2026-10-19, en la máquina de referencia del ADR-0020 (Dell XPS 13 9360,
i7-8550U). Son mediciones de laboratorio para decidir, no del protocolo del ADR-0020: no
hubo calentamiento térmico, y había otras aplicaciones abiertas.

## La velocidad de la espada

Con los valores de partida del tajo (ADR-0015):

```python
import math
arc = math.radians(195 + 25)   # de 195° a −25°: 220°
R = 0.6 + 1.25                 # radio de la punta: guarda + hoja, en metros
T = 0.26                       # duración del barrido, en segundos
avg = arc * R / T              # velocidad media de la punta
peak = avg * 1.5               # la derivada de smoothstep llega a 1,5 en la mitad
print(arc * R, avg, peak, peak / 60)
```

| Qué | Sin carga (220°) | Con la carga máxima (260°) |
|---|---|---|
| Recorrido de la punta | 7,10 m | 8,40 m |
| Velocidad media | 27,3 m/s | 32,3 m/s |
| Velocidad máxima | 41,0 m/s | 48,4 m/s |
| Avance en un paso de 60 Hz, en el pico | 0,68 m | 0,81 m |

La carga máxima suma 40° hacia atrás al arco (ADR-0015), con la misma duración. El
radio de la guarda (0,6 m) y el largo de la hoja (1,25 m) salen del prototipo del tajo
([`skinning-y-prototipo-del-tajo.md`](../../animation/research/skinning-y-prototipo-del-tajo.md)).

La curva del barrido es `smoothstep(t) = t²(3 − 2t)`, cuya derivada máxima es 1,5 (en
`t = 0,5`). Por eso la velocidad máxima es 1,5 veces la media. Integrar la curva sobre
el paso centrado en el pico da 0,682 m, así que usar la velocidad instantánea es una
buena aproximación.

**El centro del arco** se mueve con el paso adelante: 0,35 m en 0,26 s, con la misma
curva. Su velocidad máxima es 1,5 × 0,35 / 0,26 ≈ 2,0 m/s, o sea ~3,4 cm por paso en el
pico (~2 cm en promedio). Es el margen que el barrido del tajo agrega al radio de cada
cápsula.

## Fase amplia por fuerza bruta

**Qué responde:** si probar todos los pares de cajas alcanza para 150 cuerpos, o si hace
falta una estructura (grilla o árbol).

**Cómo repetirla:**

```bash
clang -O2 -o broad broad.c   # sin -march ni -ffast-math (ADR-0008)
for i in 1 2 3 4 5; do ./broad; done
```

```c
// Fase amplia por fuerza bruta: todos los pares de AABB de 150 cuerpos
// (50 zombies + 100 pedazos), en un hilo y escalar (ADR-0008, ADR-0012).
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 150
#define REPS 2000

typedef struct { float minx, miny, minz, maxx, maxy, maxz; } aabb;

static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

int main(void) {
    aabb b[N];
    srand(3);
    for (int i = 0; i < N; ++i) {
        // Cuerpos de ~0,5 m repartidos en una arena de 20 × 20 m
        float x = 20.0f * rand() / (float)RAND_MAX, z = 20.0f * rand() / (float)RAND_MAX;
        b[i] = (aabb){x, 0, z, x + 0.5f, 1.8f, z + 0.5f};
    }
    int pairs = 0;
    double best = 1e9;
    for (int r = 0; r < REPS; ++r) {
        double t0 = now();
        pairs = 0;
        for (int i = 0; i < N; ++i)
            for (int j = i + 1; j < N; ++j)
                pairs += b[i].minx <= b[j].maxx && b[j].minx <= b[i].maxx &&
                         b[i].miny <= b[j].maxy && b[j].miny <= b[i].maxy &&
                         b[i].minz <= b[j].maxz && b[j].minz <= b[i].maxz;
        double t = now() - t0;
        if (t < best) best = t;
    }
    printf("%d pares probados, %d se tocan, mejor: %.1f us\n", N * (N - 1) / 2, pairs, best * 1e6);
    return 0;
}
```

**Resultados en bruto (5 ejecuciones, el mejor de 2000 repeticiones):**

| Ejecución | Tiempo |
|---|---|
| 1 | 24,9 µs |
| 2 | 22,5 µs |
| 3 | 21,8 µs |
| 4 | 22,6 µs |
| 5 | 20,7 µs |

Los 11 175 pares cuestan unos 22 µs, un 1,5 % de los 1,5 ms de la física. Los cuerpos
estaban repartidos al azar. En el juego, la horda se amontona alrededor de la caballera:
eso no cambia el costo de la fase amplia (los pares se prueban igual), pero sí la
cantidad de pares que pasan a la fase fina.
