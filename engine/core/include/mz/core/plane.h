#pragma once

// Un plano en 3D, guardado como su normal (de largo 1) y su distancia: los puntos p que
// cumplen
//
//     dot(normal, p) = distance
//
// La distancia es dónde está el plano sobre su normal, medido desde el origen: el plano
// y = 2 con normal +Y tiene distancia 2, y el plano y = −1, distancia −1. Es la forma de
// Ericson, Real-Time Collision Detection, sección 3.6. (La otra forma habitual,
// dot(n, p) + d = 0, guarda la misma distancia con el signo cambiado. En un shader, este
// plano se sube como vec4(normal, −distance), y la distancia con signo de p es
// dot(plano, vec4(p, 1)).)
//
// El corte de mallas clasifica cada vértice por el signo de signed_distance, así que la
// cuenta es una sola, siempre la misma, y está aquí.

#include "mz/core/assert.h"
#include "mz/core/scalar.h"
#include "mz/core/vec.h"

namespace mz::core {

class plane {
public:
    /// `normal` tiene que medir 1: con otro largo, signed_distance devolvería la distancia
    /// multiplicada por ese largo, y no metros.
    plane(vec3 normal, float distance) : m_normal(normal), m_distance(distance) {
        // La tolerancia admite el redondeo de normalize() y el de una normal guardada en un
        // asset, y rechaza una normal sin normalizar.
        MZ_ASSERT(nearly_equal(length_squared(normal), 1.0F, 1e-4F),
                  "un plano necesita una normal de largo 1, y mide {}", length(normal));
    }

    /// El plano con esa normal (de largo 1) que pasa por `point`.
    [[nodiscard]] static plane through_point(vec3 normal, vec3 point) {
        return {normal, dot(normal, point)};
    }

    [[nodiscard]] vec3 normal() const { return m_normal; }
    [[nodiscard]] float distance() const { return m_distance; }

private:
    vec3 m_normal;
    float m_distance;
};

/// Positiva del lado hacia donde apunta la normal, negativa del otro y 0 en el plano, en
/// metros.
[[nodiscard]] inline float signed_distance(const plane &p, vec3 point) {
    return dot(p.normal(), point) - p.distance();
}

} // namespace mz::core
