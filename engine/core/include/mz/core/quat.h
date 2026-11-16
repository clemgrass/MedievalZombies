#pragma once

// Cuaterniones para guardar e interpolar rotaciones. Las matrices siguen siendo las que
// transforman muchos puntos y las que van a la GPU (to_mat3, to_mat4).
//
// La rotación de un ángulo θ alrededor de un eje u (de largo 1) es el cuaternión
//
//     (sen(θ/2) · u, cos(θ/2))
//
// con la parte vectorial en x, y, z y la escalar en w. Ese orden es el de glTF, así que
// una rotación de un asset se copia tal cual. Un cuaternión de rotación mide 1, y q y −q
// son la misma rotación: sumarle una vuelta entera (2π) a θ cambia el signo de las cuatro
// componentes, y la rotación no cambia.
//
// Frente a los ángulos de Euler: tres giros seguidos alrededor de ejes fijos pierden un
// grado de libertad cuando el del medio alinea a los otros dos (gimbal lock), y
// interpolar ángulos no da una rotación a velocidad constante. Un cuaternión no tiene
// ninguno de los dos problemas. Euler queda solo para mostrarle una rotación a una persona
// (ver to_euler).

#include "mz/core/assert.h"
#include "mz/core/mat.h"
#include "mz/core/scalar.h"
#include "mz/core/vec.h"

#include <cmath>

namespace mz::core {

/// Yaw, pitch y roll, en radianes, en el orden YXZ: girar `m_roll` alrededor de Z,
/// después `m_pitch` alrededor de X y al final `m_yaw` alrededor de Y. Visto de otra forma,
/// es el giro de una cámara: primero el yaw alrededor del eje arriba del mundo, después el
/// pitch alrededor de su propio eje derecha, y el roll alrededor de hacia dónde mira. Un
/// yaw positivo gira a la izquierda (antihorario visto desde arriba, por la mano derecha) y
/// un pitch positivo mira hacia arriba.
///
/// Es solo para depuración y para el editor: el engine guarda e interpola cuaterniones.
struct euler_angles {
    constexpr euler_angles() = default;
    constexpr euler_angles(float yaw, float pitch, float roll)
        : m_yaw(yaw), m_pitch(pitch), m_roll(roll) {}

    float m_yaw = 0.0F;
    float m_pitch = 0.0F;
    float m_roll = 0.0F;
};

struct quat {
    /// La identidad: (0, 0, 0, 1), la rotación de ángulo 0.
    constexpr quat() = default;
    constexpr quat(float x, float y, float z, float w) : m_x(x), m_y(y), m_z(z), m_w(w) {}

    /// La rotación de `angle` radianes alrededor de `axis`, que tiene que medir 1. Con la
    /// mano derecha: el pulgar en el eje, los dedos giran hacia los ángulos positivos.
    [[nodiscard]] static quat from_axis_angle(vec3 axis, float angle);

    /// El cuaternión de una matriz de rotación: columnas de largo 1, perpendiculares entre
    /// sí y con determinante +1, sin escala. Puede salir q o −q: las dos son la misma
    /// rotación.
    [[nodiscard]] static quat from_mat3(const mat3 &rotation);
    /// Como from_mat3, con el bloque 3×3 de arriba a la izquierda. La traslación no
    /// importa, pero el bloque no puede tener escala.
    [[nodiscard]] static quat from_mat4(const mat4 &rotation);

    /// La rotación de unos ángulos de Euler (ver euler_angles). Solo para depuración.
    [[nodiscard]] static quat from_euler(euler_angles angles);

    float m_x = 0.0F;
    float m_y = 0.0F;
    float m_z = 0.0F;
    float m_w = 1.0F;
};

// --- Álgebra ------------------------------------------------------------------------------

/// El producto de Hamilton. Como con las matrices, a · b aplica primero b y después a, y
/// to_mat3(a · b) = to_mat3(a) · to_mat3(b). No es conmutativo.
///
/// Con la parte vectorial v y la escalar w de cada factor:
///
///     a · b = (a.w · b.v + b.w · a.v + a.v × b.v,  a.w · b.w − a.v · b.v)
///
/// El producto cruz es lo que lo hace no conmutativo.
[[nodiscard]] constexpr quat operator*(quat a, quat b) {
    const vec3 av{a.m_x, a.m_y, a.m_z};
    const vec3 bv{b.m_x, b.m_y, b.m_z};
    const vec3 v = a.m_w * bv + b.m_w * av + cross(av, bv);
    return {v.m_x, v.m_y, v.m_z, a.m_w * b.m_w - dot(av, bv)};
}

[[nodiscard]] constexpr quat operator*(quat q, float s) {
    return {q.m_x * s, q.m_y * s, q.m_z * s, q.m_w * s};
}
[[nodiscard]] constexpr quat operator*(float s, quat q) { return q * s; }
[[nodiscard]] constexpr quat operator+(quat a, quat b) {
    return {a.m_x + b.m_x, a.m_y + b.m_y, a.m_z + b.m_z, a.m_w + b.m_w};
}
/// La misma rotación con el signo opuesto en las cuatro componentes.
[[nodiscard]] constexpr quat operator-(quat q) { return {-q.m_x, -q.m_y, -q.m_z, -q.m_w}; }

/// El producto punto de los cuatro componentes. Con dos cuaterniones de largo 1, es el
/// coseno de la mitad del ángulo entre las dos rotaciones, si es positivo; si es negativo,
/// el camino corto pasa por −b.
[[nodiscard]] constexpr float dot(quat a, quat b) {
    return a.m_x * b.m_x + a.m_y * b.m_y + a.m_z * b.m_z + a.m_w * b.m_w;
}
[[nodiscard]] constexpr float length_squared(quat q) { return dot(q, q); }
[[nodiscard]] inline float length(quat q) { return std::sqrt(length_squared(q)); }

/// (−x, −y, −z, w): la rotación del mismo ángulo alrededor del eje opuesto. Si q mide 1,
/// es su inversa: q · conjugate(q) es la identidad.
[[nodiscard]] constexpr quat conjugate(quat q) { return {-q.m_x, -q.m_y, -q.m_z, q.m_w}; }

/// Si mide 1, con la tolerancia de las funciones que lo suponen: |q|² a 1e-3 de 1. Es
/// laxa porque cada producto acumula redondeo; quien encadena muchos (la animación)
/// renormaliza cada tanto.
[[nodiscard]] inline bool is_unit(quat q) { return nearly_equal(length_squared(q), 1.0F, 1e-3F); }

/// El cuaternión cero no tiene dirección: normalizarlo es un error del programador.
[[nodiscard]] inline quat normalize(quat q) {
    const float length_sq = length_squared(q);
    MZ_ASSERT(length_sq > 0.0F, "normalize de un cuaternión de largo 0");
    return q * (1.0F / std::sqrt(length_sq));
}

/// Casi iguales si lo son los cuatro componentes (ver nearly_equal en scalar.h). Compara
/// los números, no las rotaciones: q y −q son la misma rotación y no son casi iguales.
[[nodiscard]] inline bool nearly_equal(quat a, quat b, float tol) {
    return nearly_equal(a.m_x, b.m_x, tol) && nearly_equal(a.m_y, b.m_y, tol) &&
           nearly_equal(a.m_z, b.m_z, tol) && nearly_equal(a.m_w, b.m_w, tol);
}

// --- Rotar --------------------------------------------------------------------------------

/// `v` rotado por q, que tiene que medir 1. Es q · (v, 0) · conjugate(q), desarrollado:
/// con u = (x, y, z) y t = 2 · (u × v),
///
///     v' = v + w · t + u × t
///
/// Son dos productos cruz en vez de dos productos de Hamilton: unas 15 multiplicaciones en
/// vez de 32. Para rotar muchos puntos con el mismo q, to_mat3 sale más barato (9 por
/// punto).
[[nodiscard]] inline vec3 rotate(quat q, vec3 v) {
    MZ_ASSERT(is_unit(q), "el cuaternión tiene que medir 1, y mide {}", length(q));
    const vec3 u{q.m_x, q.m_y, q.m_z};
    const vec3 t = 2.0F * cross(u, v);
    return v + q.m_w * t + cross(u, t);
}

// --- Interpolar ---------------------------------------------------------------------------

/// Interpolación esférica: con t de 0 a 1, va de a a b por el camino más corto y a
/// velocidad angular constante. a y b tienen que medir 1. Con t = 1 puede dar −b, que es la
/// misma rotación que b.
[[nodiscard]] quat slerp(quat a, quat b, float t);

/// Interpolación lineal normalizada: también va de a a b por el camino corto, pero sin
/// velocidad constante (más rápida en el medio). Es más barata que slerp, y la diferencia
/// es chica si a y b están cerca, como dos fotogramas seguidos de una animación. a y b
/// tienen que medir 1.
[[nodiscard]] quat nlerp(quat a, quat b, float t);

// --- Convertir ----------------------------------------------------------------------------

/// La matriz de rotación de q, que tiene que medir 1.
[[nodiscard]] mat3 to_mat3(quat q);
/// La matriz de rotación de q, sin traslación.
[[nodiscard]] mat4 to_mat4(quat q);

/// Los ángulos de Euler de q (ver euler_angles), con el yaw y el roll en [−π, π] y el
/// pitch en [−π/2, π/2]. Con el pitch en ±π/2 (gimbal lock), el yaw y el roll giran
/// alrededor del mismo eje, y solo su suma importa: el roll vale 0 y el yaw lleva todo el
/// giro. Solo para depuración.
[[nodiscard]] euler_angles to_euler(quat q);

} // namespace mz::core
