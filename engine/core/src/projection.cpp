#include "mz/core/projection.h"

#include "mz/core/assert.h"

#include <cmath>
#include <numbers>

namespace mz::core {

// La cámara es un objeto más del mundo, con su base (right, camera_up, −forward) y su
// posición eye. La vista es la inversa de esa transformación, y no hace falta llamar a
// inverse(): la inversa de una rotación es su transpuesta (la base pasa a ser las filas), y
// la de una traslación es la traslación opuesta, ya rotada (−dot(eje, eye)).
mat4 look_at(vec3 eye, vec3 target, vec3 up) {
    const vec3 to_target = target - eye;
    MZ_ASSERT(length_squared(to_target) > 0.0F, "la cámara mira el punto donde está, ({}, {}, {})",
              eye.m_x, eye.m_y, eye.m_z);
    const vec3 forward = normalize(to_target);
    // |forward × up| = |up| · sen(ángulo entre los dos). Bajo 1e-3 (unos 0,06°), el
    // producto cruz pierde casi todos sus dígitos por la cancelación, y la derecha de la
    // cámara sería ruido.
    const vec3 unnormalized_right = cross(forward, up);
    MZ_ASSERT(length_squared(unnormalized_right) > 1e-6F * length_squared(up),
              "la dirección de la cámara es paralela a up");
    const vec3 right = normalize(unnormalized_right);
    // Ya es de largo 1: right y forward son perpendiculares y miden 1.
    const vec3 camera_up = cross(right, forward);

    return {vec4{right.m_x, camera_up.m_x, -forward.m_x, 0.0F},
            vec4{right.m_y, camera_up.m_y, -forward.m_y, 0.0F},
            vec4{right.m_z, camera_up.m_z, -forward.m_z, 0.0F},
            vec4{-dot(right, eye), -dot(camera_up, eye), dot(forward, eye), 1.0F}};
}

// Con f = 1 / tan(fov_y / 2), un punto (x, y, z) de la vista queda en
//
//     x' = f / aspect · x,   y' = −f · y,   z' = near_plane,   w' = −z
//
// Después de dividir por w', la profundidad es near_plane / −z: 1 en el plano cercano y
// tiende a 0 lejos. Es el límite de la proyección reverse-Z finita, cuya fila de z tiene
// near / (far − near) y near · far / (far − near): con far → ∞, la primera tiende a 0 y la
// segunda a near.
mat4 perspective_infinite_reverse_z(float fov_y, float aspect, float near_plane) {
    MZ_ASSERT(fov_y > 0.0F && fov_y < std::numbers::pi_v<float>,
              "el campo de visión tiene que estar entre 0 y π radianes, y es {}", fov_y);
    MZ_ASSERT(aspect > 0.0F, "la relación de aspecto tiene que ser > 0, y es {}", aspect);
    MZ_ASSERT(near_plane > 0.0F, "el plano cercano tiene que estar a una distancia > 0, y es {}",
              near_plane);
    const float f = 1.0F / std::tan(fov_y / 2.0F);
    return {vec4{f / aspect, 0.0F, 0.0F, 0.0F}, vec4{0.0F, -f, 0.0F, 0.0F},
            vec4{0.0F, 0.0F, 0.0F, -1.0F}, vec4{0.0F, 0.0F, near_plane, 0.0F}};
}

// Cada eje se lleva lineal a su rango: x de [left, right] a [−1, 1], y de [bottom, top] a
// [1, −1] (volteada), y z de [−near_plane, −far_plane] a [1, 0]. w queda en 1: no hay
// división por la distancia.
mat4 orthographic_reverse_z(float left, float right, float bottom, float top, float near_plane,
                            float far_plane) {
    MZ_ASSERT(left < right && bottom < top && near_plane < far_plane,
              "la caja vacía de una ortográfica: x [{}, {}], y [{}, {}], distancia [{}, {}]", left,
              right, bottom, top, near_plane, far_plane);
    const float width = right - left;
    const float height = top - bottom;
    const float depth = far_plane - near_plane;
    return {vec4{2.0F / width, 0.0F, 0.0F, 0.0F}, vec4{0.0F, -2.0F / height, 0.0F, 0.0F},
            vec4{0.0F, 0.0F, 1.0F / depth, 0.0F},
            vec4{-(right + left) / width, (top + bottom) / height, far_plane / depth, 1.0F}};
}

} // namespace mz::core
