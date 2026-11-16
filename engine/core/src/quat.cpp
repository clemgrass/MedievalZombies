#include "mz/core/quat.h"

#include <algorithm>
#include <cmath>

namespace mz::core {

namespace {

// Cuánto se aleja m de una rotación: lo que más se aparta de 1 el largo² de una columna o
// el determinante. Alcanza con eso: si las columnas miden 1, el determinante vale 1 solo
// cuando además son perpendiculares (desigualdad de Hadamard, ver inverse en mat.h). Y
// el signo descarta los espejos, que tienen determinante −1.
//
// Solo la usa un MZ_ASSERT. Sin asserts (NDEBUG), el assert la compila pero no la llama, y
// sin maybe_unused Clang avisa que no se emite.
[[maybe_unused]] float rotation_error(const mat3 &m) {
    return std::max({std::fabs(length_squared(m[0]) - 1.0F), std::fabs(length_squared(m[1]) - 1.0F),
                     std::fabs(length_squared(m[2]) - 1.0F), std::fabs(determinant(m) - 1.0F)});
}

// Cerca del gimbal lock, el coseno del pitch es del orden del redondeo de la matriz, y el
// yaw y el roll que salen de dividir por él son ruido. Por debajo de 1e-4 (a menos de
// 0,006° de mirar justo arriba o abajo), to_euler los trata como gimbal lock.
constexpr float gimbal_lock_cos_pitch = 1e-4F;

// Por encima de este coseno (unos 3,6° entre las dos rotaciones), sen(θ) se acerca a 0 y
// slerp dividiría por casi cero. Ahí nlerp da el mismo resultado, salvo redondeo.
constexpr float slerp_nlerp_threshold = 0.9995F;

} // namespace

// --- Construir ----------------------------------------------------------------------------

quat quat::from_axis_angle(vec3 axis, float angle) {
    // La misma tolerancia que la normal de un plano: admite el redondeo de normalize().
    MZ_ASSERT(nearly_equal(length_squared(axis), 1.0F, 1e-4F),
              "una rotación necesita un eje de largo 1, y mide {}", length(axis));
    const float half = 0.5F * angle;
    const vec3 v = axis * std::sin(half);
    return {v.m_x, v.m_y, v.m_z, std::cos(half)};
}

// El método de Shepperd (1978). Los elementos de la matriz son sumas y restas de productos
// de las componentes: en la diagonal están 4w² = 1 + traza, 4x² = 1 + m00 − m11 − m22, y
// así; fuera de la diagonal, 4xy, 4wz, etc. Se saca primero la componente más grande, con
// una raíz, y las otras tres dividiendo por ella.
//
// La fórmula de los libros de introducción usa siempre w = √(1 + traza) / 2. Con traza
// negativa (rotaciones de más de 120°, porque traza = 1 + 2 · cos θ), w se acerca a 0, y
// dividir por él amplifica el redondeo; en media vuelta, divide por cero. Con la
// componente más grande, el divisor (4 veces esa componente) vale al menos 2.
//
// m[c].m_y es el elemento de la fila 1 y la columna c: m21 = m[1].m_z.
quat quat::from_mat3(const mat3 &rotation) {
    MZ_ASSERT(rotation_error(rotation) <= 1e-3F,
              "from_mat3 necesita una matriz de rotación (columnas de largo 1 y determinante "
              "+1), y se aleja en {}",
              rotation_error(rotation));
    const float m00 = rotation[0].m_x;
    const float m11 = rotation[1].m_y;
    const float m22 = rotation[2].m_z;
    const float m01 = rotation[1].m_x;
    const float m10 = rotation[0].m_y;
    const float m02 = rotation[2].m_x;
    const float m20 = rotation[0].m_z;
    const float m12 = rotation[2].m_y;
    const float m21 = rotation[1].m_z;
    const float trace = m00 + m11 + m22;

    // Con traza positiva, 4w² = 1 + traza > 1, así que w > 1/2 y alcanza con w.
    if (trace > 0.0F) {
        const float s = 2.0F * std::sqrt(1.0F + trace); // 4w
        return {(m21 - m12) / s, (m02 - m20) / s, (m10 - m01) / s, 0.25F * s};
    }
    // Si no, el mayor de la diagonal: 4x² = 1 + 2 · m00 − traza ≥ 1 cuando m00 es el mayor.
    if (m00 > m11 && m00 > m22) {
        const float s = 2.0F * std::sqrt(1.0F + m00 - m11 - m22); // 4x
        return {0.25F * s, (m01 + m10) / s, (m02 + m20) / s, (m21 - m12) / s};
    }
    if (m11 > m22) {
        const float s = 2.0F * std::sqrt(1.0F + m11 - m00 - m22); // 4y
        return {(m01 + m10) / s, 0.25F * s, (m12 + m21) / s, (m02 - m20) / s};
    }
    const float s = 2.0F * std::sqrt(1.0F + m22 - m00 - m11); // 4z
    return {(m02 + m20) / s, (m12 + m21) / s, 0.25F * s, (m10 - m01) / s};
}

quat quat::from_mat4(const mat4 &rotation) {
    return from_mat3(mat3{vec3{rotation[0].m_x, rotation[0].m_y, rotation[0].m_z},
                          vec3{rotation[1].m_x, rotation[1].m_y, rotation[1].m_z},
                          vec3{rotation[2].m_x, rotation[2].m_y, rotation[2].m_z}});
}

// Yaw · pitch · roll: el de la derecha se aplica primero.
quat quat::from_euler(euler_angles angles) {
    return from_axis_angle(vec3{0.0F, 1.0F, 0.0F}, angles.m_yaw) *
           from_axis_angle(vec3{1.0F, 0.0F, 0.0F}, angles.m_pitch) *
           from_axis_angle(vec3{0.0F, 0.0F, 1.0F}, angles.m_roll);
}

// --- Interpolar ---------------------------------------------------------------------------

// Los cuaterniones de largo 1 viven en la esfera de 4 dimensiones, y slerp recorre el arco
// de círculo máximo entre a y b. Con Ω el ángulo entre los dos (cos Ω = a · b), los pesos
//
//     sen((1 − t) · Ω) / sen(Ω)   y   sen(t · Ω) / sen(Ω)
//
// mantienen el resultado sobre la esfera y avanzan el mismo ángulo por cada paso de t
// (Shoemake, "Animating Rotation with Quaternion Curves", SIGGRAPH 1985).
quat slerp(quat a, quat b, float t) {
    MZ_ASSERT(is_unit(a), "el cuaternión tiene que medir 1, y a mide {}", length(a));
    MZ_ASSERT(is_unit(b), "el cuaternión tiene que medir 1, y b mide {}", length(b));
    float cos_omega = dot(a, b);
    // Con el dot negativo, a y b están a más de 90° en la esfera, y el arco de a a b es la
    // vuelta larga (más de 180° de rotación). −b es la misma rotación, y el arco hasta −b
    // es el corto.
    if (cos_omega < 0.0F) {
        b = -b;
        cos_omega = -cos_omega;
    }
    if (cos_omega > slerp_nlerp_threshold) {
        return nlerp(a, b, t);
    }
    const float omega = std::acos(cos_omega);
    const float sin_omega = std::sin(omega);
    return a * (std::sin((1.0F - t) * omega) / sin_omega) + b * (std::sin(t * omega) / sin_omega);
}

// La cuerda entre a y b, llevada a la esfera. En la mitad coincide con slerp; en el resto
// avanza más rápido en el medio que en los extremos (Blow, "Understanding Slerp, Then Not
// Using It", 2004).
quat nlerp(quat a, quat b, float t) {
    MZ_ASSERT(is_unit(a), "el cuaternión tiene que medir 1, y a mide {}", length(a));
    MZ_ASSERT(is_unit(b), "el cuaternión tiene que medir 1, y b mide {}", length(b));
    const quat target = dot(a, b) < 0.0F ? -b : b;
    return normalize(a * (1.0F - t) + target * t);
}

// --- Convertir ----------------------------------------------------------------------------

// Las columnas son los ejes X, Y y Z rotados por q. Cada elemento sale de desarrollar
// q · (eje, 0) · conjugate(q) y usar x² + y² + z² + w² = 1 en la diagonal.
mat3 to_mat3(quat q) {
    MZ_ASSERT(is_unit(q), "el cuaternión tiene que medir 1, y mide {}", length(q));
    const float xx = q.m_x * q.m_x;
    const float yy = q.m_y * q.m_y;
    const float zz = q.m_z * q.m_z;
    const float xy = q.m_x * q.m_y;
    const float xz = q.m_x * q.m_z;
    const float yz = q.m_y * q.m_z;
    const float wx = q.m_w * q.m_x;
    const float wy = q.m_w * q.m_y;
    const float wz = q.m_w * q.m_z;
    return {vec3{1.0F - 2.0F * (yy + zz), 2.0F * (xy + wz), 2.0F * (xz - wy)},
            vec3{2.0F * (xy - wz), 1.0F - 2.0F * (xx + zz), 2.0F * (yz + wx)},
            vec3{2.0F * (xz + wy), 2.0F * (yz - wx), 1.0F - 2.0F * (xx + yy)}};
}

mat4 to_mat4(quat q) {
    const mat3 m = to_mat3(q);
    return {vec4{m[0].m_x, m[0].m_y, m[0].m_z, 0.0F}, vec4{m[1].m_x, m[1].m_y, m[1].m_z, 0.0F},
            vec4{m[2].m_x, m[2].m_y, m[2].m_z, 0.0F}, vec4{0.0F, 0.0F, 0.0F, 1.0F}};
}

// Con a = yaw, b = pitch y c = roll, la matriz Ry(a) · Rx(b) · Rz(c) tiene
//
//     fila 0: (ca·cc + sa·sb·sc,  −ca·sc + sa·sb·cc,  sa·cb)
//     fila 1: (cb·sc,             cb·cc,              −sb  )
//     fila 2: (−sa·cc + ca·sb·sc, sa·sc + ca·sb·cc,   ca·cb)
//
// La fila 1 da el pitch (−sb y, con sus dos primeros elementos, cb y el roll), y la
// columna 2 da el yaw. Con cb = 0 (gimbal lock), solo quedan cos(a ∓ c) y sen(a ∓ c) en
// las columnas 0 y 1: con c = 0, el yaw sale de la columna 0.
euler_angles to_euler(quat q) {
    const mat3 m = to_mat3(q);
    const float sin_pitch = -m[2].m_y;
    const float cos_pitch = std::hypot(m[0].m_y, m[1].m_y);
    // atan2 en vez de asin(sin_pitch): no necesita recortar sin_pitch a [−1, 1] y es
    // preciso también cerca de ±90°.
    const float pitch = std::atan2(sin_pitch, cos_pitch);
    if (cos_pitch > gimbal_lock_cos_pitch) {
        return {std::atan2(m[2].m_x, m[2].m_z), pitch, std::atan2(m[0].m_y, m[1].m_y)};
    }
    return {std::atan2(-m[0].m_z, m[0].m_x), pitch, 0.0F};
}

} // namespace mz::core
