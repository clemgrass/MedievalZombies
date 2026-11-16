#pragma once

// Comparaciones de floats. Dos floats que en la práctica son el mismo número pueden
// diferir en los últimos bits, porque cada operación redondea: == entre floats no compila
// (-Wfloat-equal). Se compara con nearly_equal y una tolerancia elegida para cada caso,
// porque no hay una que sirva para todo.

#include "mz/core/assert.h"

#include <algorithm>
#include <cmath>

namespace mz::core {

/// Si a y b difieren en a lo sumo `tol`, medido en absoluto cerca de cero y en relativo
/// lejos de cero:
///
///     |a − b| ≤ tol · max(1, |a|, |b|)
///
/// Con |a| y |b| menores que 1, la tolerancia es absoluta: una relativa exigiría
/// igualdad exacta cerca de cero. Con valores grandes es relativa: entre floats cercanos
/// a un millón hay pasos de 0,0625, y una absoluta chica exigiría igualdad exacta. Es la
/// tolerancia combinada de Ericson, Real-Time Collision Detection, sección 11.3.1.
///
/// NaN nunca es casi igual a nada, y un infinito tampoco: es una cuenta que se desbordó.
/// Hace falta pedir que la diferencia sea finita: con a = ∞ y b = 1, la escala también es
/// ∞, y ∞ ≤ tol · ∞ sería verdadero.
[[nodiscard]] inline bool nearly_equal(float a, float b, float tol) {
    MZ_ASSERT(tol >= 0.0F, "la tolerancia tiene que ser un número >= 0, y es {}", tol);
    const float difference = std::fabs(a - b);
    const float scale = std::max({1.0F, std::fabs(a), std::fabs(b)});
    return std::isfinite(difference) && difference <= tol * scale;
}

/// El == de IEEE 754, para cuando el valor exacto es lo que se quiere: un valor que se
/// guardó y se vuelve a leer, o una constante que nunca pasó por una cuenta. 0 y −0 son
/// iguales, y NaN no es igual a nada. Es el único lugar donde el código propio compara
/// floats con ==, así que cada uso se encuentra con grep.
[[nodiscard]] constexpr bool exactly_equal(float a, float b) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    return a == b;
#pragma GCC diagnostic pop
}

} // namespace mz::core
