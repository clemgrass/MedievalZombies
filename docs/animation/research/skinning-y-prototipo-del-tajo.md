# Skinning en la CPU y prototipo del tajo

Nota de investigación del [ADR-0015](../adr/0015-sistema-de-animacion.md). Guarda lo
necesario para repetir la medición del skinning y entender el prototipo del tajo.

## Medición del skinning en la CPU

**Qué responde:** si deformar los 50 zombies en la CPU cabe en los 1,2 ms de animación
por paso del ADR-0020.

**Cuándo y dónde:** 2026-10-19, en la máquina de referencia del ADR-0020 (Dell XPS 13
9360, i7-8550U). No sigue el protocolo del ADR-0020: no hubo calentamiento térmico, había
otras aplicaciones abiertas y los datos son sintéticos. Es una medición de laboratorio
para decidir, no un benchmark del protocolo.

**Carga:** 50 zombies × 3000 vértices, esqueleto de 20 huesos, matrices afines de 3×4.
Cada vértice tiene posición, normal, 4 índices de hueso y 4 pesos.

**Cómo repetirla:**

```bash
clang -O2 -o skin skin.c   # sin -march ni -ffast-math (ADR-0008)
for i in 1 2 3 4 5; do ./skin; done
```

El programa toma el mejor de 200 repeticiones de cada caso:

```c
// Costo del skinning en la CPU, escalar y en un hilo (ADR-0008, ADR-0012).
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ZOMBIES 50
#define VERTS 3000
#define BONES 20
#define REPS 200

typedef struct { float m[12]; } mat34; // 3x4: la última fila es implícita (0 0 0 1)
typedef struct { float px, py, pz, nx, ny, nz; unsigned char b[4]; float w[4]; } vin;
typedef struct { float px, py, pz, nx, ny, nz; } vout;

static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

// LBS: mezcla las 4 matrices según los pesos y transforma posición y normal.
static void skin_lbs(const vin *in, vout *out, const mat34 *bones, int n) {
    for (int i = 0; i < n; ++i) {
        float m[12] = {0};
        for (int k = 0; k < 4; ++k) {
            const float *b = bones[in[i].b[k]].m;
            float w = in[i].w[k];
            for (int j = 0; j < 12; ++j) m[j] += w * b[j];
        }
        const vin *v = &in[i];
        out[i].px = m[0] * v->px + m[1] * v->py + m[2] * v->pz + m[3];
        out[i].py = m[4] * v->px + m[5] * v->py + m[6] * v->pz + m[7];
        out[i].pz = m[8] * v->px + m[9] * v->py + m[10] * v->pz + m[11];
        out[i].nx = m[0] * v->nx + m[1] * v->ny + m[2] * v->nz;
        out[i].ny = m[4] * v->nx + m[5] * v->ny + m[6] * v->nz;
        out[i].nz = m[8] * v->nx + m[9] * v->ny + m[10] * v->nz;
    }
}

// Rígido: un solo hueso por vértice (estilo PS1).
static void skin_rigid(const vin *in, vout *out, const mat34 *bones, int n) {
    for (int i = 0; i < n; ++i) {
        const float *m = bones[in[i].b[0]].m;
        const vin *v = &in[i];
        out[i].px = m[0] * v->px + m[1] * v->py + m[2] * v->pz + m[3];
        out[i].py = m[4] * v->px + m[5] * v->py + m[6] * v->pz + m[7];
        out[i].pz = m[8] * v->px + m[9] * v->py + m[10] * v->pz + m[11];
        out[i].nx = m[0] * v->nx + m[1] * v->ny + m[2] * v->nz;
        out[i].ny = m[4] * v->nx + m[5] * v->ny + m[6] * v->nz;
        out[i].nz = m[8] * v->nx + m[9] * v->ny + m[10] * v->nz;
    }
}

int main(void) {
    int n = ZOMBIES * VERTS;
    vin *in = malloc(sizeof(vin) * n);
    vout *out = malloc(sizeof(vout) * n);
    mat34 bones[BONES];
    srand(1);
    for (int b = 0; b < BONES; ++b)
        for (int j = 0; j < 12; ++j) bones[b].m[j] = (float)rand() / (float)RAND_MAX;
    for (int i = 0; i < n; ++i) {
        in[i].px = in[i].py = in[i].pz = (float)rand() / (float)RAND_MAX;
        in[i].nx = in[i].ny = in[i].nz = 0.577f;
        for (int k = 0; k < 4; ++k) { in[i].b[k] = rand() % BONES; in[i].w[k] = 0.25f; }
    }

    double best_lbs = 1e9, best_rigid = 1e9, one = 1e9;
    for (int r = 0; r < REPS; ++r) {
        double t0 = now(); skin_lbs(in, out, bones, n); double t = now() - t0;
        if (t < best_lbs) best_lbs = t;
        t0 = now(); skin_rigid(in, out, bones, n); t = now() - t0;
        if (t < best_rigid) best_rigid = t;
        t0 = now(); skin_lbs(in, out, bones, VERTS); t = now() - t0;
        if (t < one) one = t;
    }
    printf("50 zombies, LBS 4 pesos: %.3f ms | rigido 1 hueso: %.3f ms | 1 zombie LBS: %.1f us (check %.2f)\n",
           best_lbs * 1e3, best_rigid * 1e3, one * 1e6, out[n / 2].px);
    return 0;
}
```

**Resultados en bruto (5 ejecuciones):**

| Ejecución | LBS, 50 zombies | Rígido, 50 zombies | LBS, 1 zombie |
|---|---|---|---|
| 1 | 2,664 ms | 1,248 ms | 49,6 µs |
| 2 | 2,725 ms | 1,267 ms | 48,6 µs |
| 3 | 2,522 ms | 1,167 ms | 47,6 µs |
| 4 | 2,771 ms | 1,247 ms | 50,0 µs |
| 5 | 2,871 ms | 1,409 ms | 54,0 µs |

"El mejor de 200" subestima el costo real, porque descarta las repeticiones lentas. Aun
así, el LBS de los 50 zombies supera por más del doble los 1,2 ms, así que la conclusión
no depende de la precisión de la medición.

## Prototipo del tajo

Una página en three.js, en el navegador, con la escena PSX a 640×360 escalada por un
factor entero (ADR-0020) y la cámara de la visión (2,0 m detrás, 0,5 m a la izquierda,
1,9 m de alto y 30° de campo de visión al apuntar). No está versionado: era una
herramienta para decidir.

**Cómo construye el tajo:**

1. **El plano del tajo** contiene el rayo central de la cámara (`f`) y la dirección de la
   línea en la pantalla (`u = cos θ · derecha + sin θ · arriba`). Su normal es `f × u`.
2. **El centro del arco** es el pecho de la caballera proyectado sobre ese plano.
3. **La espada** es radial: la guarda a 0,6 m del centro y la punta a 0,6 + 1,25 m. La
   dirección en el ángulo φ es `cos φ · u + sin φ · f`, y el barrido va de φ = 195° a
   φ = −25° con una curva *smoothstep*.
4. **Los dos brazos** alcanzan la empuñadura con IK analítica de dos huesos (brazo de
   0,31 m y antebrazo de 0,29 m), con un vector polo hacia abajo, afuera y atrás. Si una
   mano no llega, la espada se corre para quedar en la mano, y el prototipo anota cuántos
   centímetros faltaron.
5. **El torso** gira hacia la punta (60 % del ángulo horizontal) y se inclina un poco.
6. **El paso adelante** mueve la raíz 0,35 m durante el barrido.

**La comparación con la mezcla de clips:** el modo "mezcla de 8 clips" toma los arcos de
las dos direcciones más cercanas (cada 45°) y mezcla linealmente sus posiciones con el
peso del ángulo. Esa mezcla lineal acorta el radio hasta `1 − cos 22,5° ≈ 7,6 %` a mitad
de camino, y saca la punta del arco ideal. El prototipo dibuja el arco ideal (rojo
punteado) y la estela real de la punta (blanca) para compararlos.

**Lo que mostró:**

- con IK, la estela coincide con el arco ideal en todos los ángulos;
- con la mezcla, la estela se separa del arco entre las 8 direcciones;
- **el alcance:** como el plano pasa por el rayo central de la cámara, que no pasa por
  el cuerpo, en algunos ángulos el arco queda lejos del pecho y las manos no llegan.
