#pragma once

// Las matrices de la cámara: la vista (del mundo al espacio de la cámara) y las
// proyecciones (del espacio de la cámara al de recorte de Vulkan).
//
// El espacio de la vista sigue los ejes del mundo (axes.h): la cámara está en el origen,
// mira hacia −Z, con +X a la derecha y +Y arriba.
//
// El espacio de recorte es el de Vulkan, ya resuelto en las matrices:
//
//   - La profundidad va de 0 a 1, no de −1 a 1 como en OpenGL.
//   - La Y de la pantalla va hacia abajo: las proyecciones voltean la Y, en vez de usar un
//     viewport de altura negativa. Por eso un triángulo antihorario visto desde afuera
//     sigue siendo antihorario en el framebuffer, y frontFace es COUNTER_CLOCKWISE.
//   - Reverse-Z: lo cercano tiene profundidad 1 y lo lejano 0. El depth buffer se limpia a
//     0 y compara con GREATER. Con un depth buffer de floats, la profundidad tiene casi la
//     misma precisión cerca y lejos (Reed, "Depth Precision Visualized", 2015).

#include "mz/core/mat.h"
#include "mz/core/vec.h"

namespace mz::core {

/// La vista de una cámara en `eye` que mira hacia `target`. `up` dice de qué lado queda el
/// cielo: no tiene que ser perpendicular a la dirección ni medir 1, pero no puede ser
/// paralelo a ella (mirar justo arriba o abajo es un error del programador).
[[nodiscard]] mat4 look_at(vec3 eye, vec3 target, vec3 up);

/// La proyección perspectiva de la cámara principal, con el plano lejano en el infinito.
/// `fov_y` es el campo de visión vertical, en radianes y entre 0 y π; `aspect` es ancho /
/// alto; `near_plane` es la distancia al plano cercano, que tiene profundidad 1. La
/// profundidad de un punto a una distancia d es near_plane / d: tiende a 0 sin llegar.
[[nodiscard]] mat4 perspective_infinite_reverse_z(float fov_y, float aspect, float near_plane);

/// La proyección ortográfica de la caja [left, right] × [bottom, top] del espacio de la
/// vista, entre las distancias near_plane (profundidad 1) y far_plane (profundidad 0).
/// También es reverse-Z, para que todas las pasadas compartan la convención de
/// profundidad. Su profundidad es lineal.
[[nodiscard]] mat4 orthographic_reverse_z(float left, float right, float bottom, float top,
                                          float near_plane, float far_plane);

} // namespace mz::core
