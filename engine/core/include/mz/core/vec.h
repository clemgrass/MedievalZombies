#pragma once

// Vectores de 2, 3 y 4 floats. Son tipos concretos, sin un vec<N, T> genérico: cada uno se
// lee entero, sin templates, y los mensajes de error del compilador hablan de vec3. Se
// construyen con todos sus componentes, `vec3{1.0F, 2.0F, 3.0F}`, o con ninguno, y entonces
// valen cero: vec3{1.0F, 2.0F} no compila, en vez de dejar z en 0 sin avisar. Son
// escalares, sin SIMD: el compilador ya vectoriza parte de estas cuentas, y los bucles
// calientes pasan a SIMD cuando el profiler lo pida.
//
// Las operaciones son funciones libres, como en GLSL: dot(a, b), no a.dot(b). Las que no
// usan std::sqrt ni std::fabs son constexpr; ninguna de las dos lo es en C++20.

#include "mz/core/assert.h"
#include "mz/core/scalar.h"

#include <algorithm>
#include <cmath>

namespace mz::core {

struct vec2 {
    constexpr vec2() = default;
    constexpr vec2(float x, float y) : m_x(x), m_y(y) {}

    float m_x = 0.0F;
    float m_y = 0.0F;
};

struct vec3 {
    constexpr vec3() = default;
    constexpr vec3(float x, float y, float z) : m_x(x), m_y(y), m_z(z) {}

    float m_x = 0.0F;
    float m_y = 0.0F;
    float m_z = 0.0F;
};

struct vec4 {
    constexpr vec4() = default;
    constexpr vec4(float x, float y, float z, float w) : m_x(x), m_y(y), m_z(z), m_w(w) {}

    float m_x = 0.0F;
    float m_y = 0.0F;
    float m_z = 0.0F;
    float m_w = 0.0F;
};

// --- vec2 -------------------------------------------------------------------------------

[[nodiscard]] constexpr vec2 operator+(vec2 a, vec2 b) { return {a.m_x + b.m_x, a.m_y + b.m_y}; }
[[nodiscard]] constexpr vec2 operator-(vec2 a, vec2 b) { return {a.m_x - b.m_x, a.m_y - b.m_y}; }
[[nodiscard]] constexpr vec2 operator-(vec2 v) { return {-v.m_x, -v.m_y}; }
[[nodiscard]] constexpr vec2 operator*(vec2 v, float s) { return {v.m_x * s, v.m_y * s}; }
[[nodiscard]] constexpr vec2 operator*(float s, vec2 v) { return v * s; }
[[nodiscard]] constexpr vec2 operator/(vec2 v, float s) { return {v.m_x / s, v.m_y / s}; }

[[nodiscard]] constexpr float dot(vec2 a, vec2 b) { return a.m_x * b.m_x + a.m_y * b.m_y; }
[[nodiscard]] constexpr float length_squared(vec2 v) { return dot(v, v); }
[[nodiscard]] inline float length(vec2 v) { return std::sqrt(length_squared(v)); }

/// El vector con la misma dirección y largo 1. El vector cero no tiene dirección:
/// normalizarlo es un error del programador.
[[nodiscard]] inline vec2 normalize(vec2 v) {
    const float length_sq = length_squared(v);
    MZ_ASSERT(length_sq > 0.0F, "normalize de un vector de largo 0");
    return v / std::sqrt(length_sq);
}

/// Interpolación lineal: a con t = 0, b con t = 1. Se escribe a · (1 − t) + b · t, y no
/// a + t · (b − a), porque así es exacta en los dos extremos: con t = 1 da b, sin error de
/// redondeo.
[[nodiscard]] constexpr vec2 lerp(vec2 a, vec2 b, float t) { return a * (1.0F - t) + b * t; }

[[nodiscard]] constexpr vec2 min(vec2 a, vec2 b) {
    return {std::min(a.m_x, b.m_x), std::min(a.m_y, b.m_y)};
}
[[nodiscard]] constexpr vec2 max(vec2 a, vec2 b) {
    return {std::max(a.m_x, b.m_x), std::max(a.m_y, b.m_y)};
}

/// Casi iguales si lo son todos los componentes, cada uno con su escala (ver scalar.h).
[[nodiscard]] inline bool nearly_equal(vec2 a, vec2 b, float tol) {
    return nearly_equal(a.m_x, b.m_x, tol) && nearly_equal(a.m_y, b.m_y, tol);
}

// --- vec3 -------------------------------------------------------------------------------

[[nodiscard]] constexpr vec3 operator+(vec3 a, vec3 b) {
    return {a.m_x + b.m_x, a.m_y + b.m_y, a.m_z + b.m_z};
}
[[nodiscard]] constexpr vec3 operator-(vec3 a, vec3 b) {
    return {a.m_x - b.m_x, a.m_y - b.m_y, a.m_z - b.m_z};
}
[[nodiscard]] constexpr vec3 operator-(vec3 v) { return {-v.m_x, -v.m_y, -v.m_z}; }
[[nodiscard]] constexpr vec3 operator*(vec3 v, float s) {
    return {v.m_x * s, v.m_y * s, v.m_z * s};
}
[[nodiscard]] constexpr vec3 operator*(float s, vec3 v) { return v * s; }
[[nodiscard]] constexpr vec3 operator/(vec3 v, float s) {
    return {v.m_x / s, v.m_y / s, v.m_z / s};
}

[[nodiscard]] constexpr float dot(vec3 a, vec3 b) {
    return a.m_x * b.m_x + a.m_y * b.m_y + a.m_z * b.m_z;
}

/// Producto cruz, de mano derecha: x × y = z. El resultado es perpendicular a a y a b, y su
/// largo es |a| · |b| · sen(ángulo entre los dos).
[[nodiscard]] constexpr vec3 cross(vec3 a, vec3 b) {
    return {a.m_y * b.m_z - a.m_z * b.m_y, a.m_z * b.m_x - a.m_x * b.m_z,
            a.m_x * b.m_y - a.m_y * b.m_x};
}

[[nodiscard]] constexpr float length_squared(vec3 v) { return dot(v, v); }
[[nodiscard]] inline float length(vec3 v) { return std::sqrt(length_squared(v)); }

[[nodiscard]] inline vec3 normalize(vec3 v) {
    const float length_sq = length_squared(v);
    MZ_ASSERT(length_sq > 0.0F, "normalize de un vector de largo 0");
    return v / std::sqrt(length_sq);
}

[[nodiscard]] constexpr vec3 lerp(vec3 a, vec3 b, float t) { return a * (1.0F - t) + b * t; }

[[nodiscard]] constexpr vec3 min(vec3 a, vec3 b) {
    return {std::min(a.m_x, b.m_x), std::min(a.m_y, b.m_y), std::min(a.m_z, b.m_z)};
}
[[nodiscard]] constexpr vec3 max(vec3 a, vec3 b) {
    return {std::max(a.m_x, b.m_x), std::max(a.m_y, b.m_y), std::max(a.m_z, b.m_z)};
}

[[nodiscard]] inline bool nearly_equal(vec3 a, vec3 b, float tol) {
    return nearly_equal(a.m_x, b.m_x, tol) && nearly_equal(a.m_y, b.m_y, tol) &&
           nearly_equal(a.m_z, b.m_z, tol);
}

// --- vec4 -------------------------------------------------------------------------------

[[nodiscard]] constexpr vec4 operator+(vec4 a, vec4 b) {
    return {a.m_x + b.m_x, a.m_y + b.m_y, a.m_z + b.m_z, a.m_w + b.m_w};
}
[[nodiscard]] constexpr vec4 operator-(vec4 a, vec4 b) {
    return {a.m_x - b.m_x, a.m_y - b.m_y, a.m_z - b.m_z, a.m_w - b.m_w};
}
[[nodiscard]] constexpr vec4 operator-(vec4 v) { return {-v.m_x, -v.m_y, -v.m_z, -v.m_w}; }
[[nodiscard]] constexpr vec4 operator*(vec4 v, float s) {
    return {v.m_x * s, v.m_y * s, v.m_z * s, v.m_w * s};
}
[[nodiscard]] constexpr vec4 operator*(float s, vec4 v) { return v * s; }
[[nodiscard]] constexpr vec4 operator/(vec4 v, float s) {
    return {v.m_x / s, v.m_y / s, v.m_z / s, v.m_w / s};
}

[[nodiscard]] constexpr float dot(vec4 a, vec4 b) {
    return a.m_x * b.m_x + a.m_y * b.m_y + a.m_z * b.m_z + a.m_w * b.m_w;
}
[[nodiscard]] constexpr float length_squared(vec4 v) { return dot(v, v); }
[[nodiscard]] inline float length(vec4 v) { return std::sqrt(length_squared(v)); }

[[nodiscard]] inline vec4 normalize(vec4 v) {
    const float length_sq = length_squared(v);
    MZ_ASSERT(length_sq > 0.0F, "normalize de un vector de largo 0");
    return v / std::sqrt(length_sq);
}

[[nodiscard]] constexpr vec4 lerp(vec4 a, vec4 b, float t) { return a * (1.0F - t) + b * t; }

[[nodiscard]] constexpr vec4 min(vec4 a, vec4 b) {
    return {std::min(a.m_x, b.m_x), std::min(a.m_y, b.m_y), std::min(a.m_z, b.m_z),
            std::min(a.m_w, b.m_w)};
}
[[nodiscard]] constexpr vec4 max(vec4 a, vec4 b) {
    return {std::max(a.m_x, b.m_x), std::max(a.m_y, b.m_y), std::max(a.m_z, b.m_z),
            std::max(a.m_w, b.m_w)};
}

[[nodiscard]] inline bool nearly_equal(vec4 a, vec4 b, float tol) {
    return nearly_equal(a.m_x, b.m_x, tol) && nearly_equal(a.m_y, b.m_y, tol) &&
           nearly_equal(a.m_z, b.m_z, tol) && nearly_equal(a.m_w, b.m_w, tol);
}

} // namespace mz::core
