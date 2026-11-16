#include "mz/core/mat.h"

#include <cmath>

namespace mz::core {

namespace {

// La regla de inverse(): el determinante tiene que ser finito y no tan chico comparado con
// el volumen que tendrían columnas de esos largos perpendiculares entre sí.
bool is_invertible(float det, float column_lengths, float tol) {
    return std::isfinite(det) && std::fabs(det) > tol * column_lengths;
}

vec3 xyz(vec4 v) { return {v.m_x, v.m_y, v.m_z}; }

} // namespace

// --- mat3 -------------------------------------------------------------------------------

vec3 operator*(const mat3 &m, vec3 v) { return m[0] * v.m_x + m[1] * v.m_y + m[2] * v.m_z; }

mat3 operator*(const mat3 &a, const mat3 &b) { return {a * b[0], a * b[1], a * b[2]}; }

mat3 transpose(const mat3 &m) {
    return {vec3{m[0].m_x, m[1].m_x, m[2].m_x}, vec3{m[0].m_y, m[1].m_y, m[2].m_y},
            vec3{m[0].m_z, m[1].m_z, m[2].m_z}};
}

// El producto mixto (a × b) · c.
float determinant(const mat3 &m) { return dot(cross(m[0], m[1]), m[2]); }

// Con columnas a, b, c, las filas de la inversa son b × c, c × a y a × b, divididas por el
// determinante: la fila i por la columna j da det si i = j y 0 si no, porque un producto
// cruz es perpendicular a sus dos factores. Lengyel, Foundations of Game Engine
// Development, vol. 1, sección 1.7.5.
result<mat3, singular_matrix> inverse(const mat3 &m, float tol) {
    const vec3 a = m[0];
    const vec3 b = m[1];
    const vec3 c = m[2];
    const vec3 row0 = cross(b, c);
    const vec3 row1 = cross(c, a);
    const vec3 row2 = cross(a, b);
    const float det = dot(row2, c);
    if (!is_invertible(det, length(a) * length(b) * length(c), tol)) {
        return unexpected{singular_matrix{}};
    }
    const float inv_det = 1.0F / det;
    // El constructor recibe columnas: armarla con las filas y transponer.
    return transpose(mat3{row0 * inv_det, row1 * inv_det, row2 * inv_det});
}

bool nearly_equal(const mat3 &a, const mat3 &b, float tol) {
    return nearly_equal(a[0], b[0], tol) && nearly_equal(a[1], b[1], tol) &&
           nearly_equal(a[2], b[2], tol);
}

// --- mat4 -------------------------------------------------------------------------------

vec4 operator*(const mat4 &m, vec4 v) {
    return m[0] * v.m_x + m[1] * v.m_y + m[2] * v.m_z + m[3] * v.m_w;
}

mat4 operator*(const mat4 &a, const mat4 &b) { return {a * b[0], a * b[1], a * b[2], a * b[3]}; }

mat4 transpose(const mat4 &m) {
    return {
        vec4{m[0].m_x, m[1].m_x, m[2].m_x, m[3].m_x}, vec4{m[0].m_y, m[1].m_y, m[2].m_y, m[3].m_y},
        vec4{m[0].m_z, m[1].m_z, m[2].m_z, m[3].m_z}, vec4{m[0].m_w, m[1].m_w, m[2].m_w, m[3].m_w}};
}

namespace {

// Las piezas que comparten el determinante y la inversa de una mat4, de Lengyel,
// Foundations of Game Engine Development, vol. 1, sección 1.7.5. Se separa cada columna en
// su parte 3D (a, b, c, d) y su última fila (x, y, z, w), y el determinante de 4×4 queda
// como dos productos cruz y dos productos punto, en vez de 24 productos de cuatro números.
struct mat4_parts {
    vec3 m_a, m_b, m_c, m_d;
    float m_x, m_y, m_z, m_w;
    vec3 m_s, m_t, m_u, m_v;
};

mat4_parts split(const mat4 &m) {
    mat4_parts p{};
    p.m_a = xyz(m[0]);
    p.m_b = xyz(m[1]);
    p.m_c = xyz(m[2]);
    p.m_d = xyz(m[3]);
    p.m_x = m[0].m_w;
    p.m_y = m[1].m_w;
    p.m_z = m[2].m_w;
    p.m_w = m[3].m_w;
    p.m_s = cross(p.m_a, p.m_b);
    p.m_t = cross(p.m_c, p.m_d);
    p.m_u = p.m_a * p.m_y - p.m_b * p.m_x;
    p.m_v = p.m_c * p.m_w - p.m_d * p.m_z;
    return p;
}

float determinant(const mat4_parts &p) { return dot(p.m_s, p.m_v) + dot(p.m_t, p.m_u); }

} // namespace

float determinant(const mat4 &m) { return determinant(split(m)); }

result<mat4, singular_matrix> inverse(const mat4 &m, float tol) {
    const mat4_parts p = split(m);
    const float det = determinant(p);
    if (!is_invertible(det, length(m[0]) * length(m[1]) * length(m[2]) * length(m[3]), tol)) {
        return unexpected{singular_matrix{}};
    }
    const float inv_det = 1.0F / det;
    const vec3 s = p.m_s * inv_det;
    const vec3 t = p.m_t * inv_det;
    const vec3 u = p.m_u * inv_det;
    const vec3 v = p.m_v * inv_det;

    const vec3 row0 = cross(p.m_b, v) + t * p.m_y;
    const vec3 row1 = cross(v, p.m_a) - t * p.m_x;
    const vec3 row2 = cross(p.m_d, u) + s * p.m_w;
    const vec3 row3 = cross(u, p.m_c) - s * p.m_z;
    // El constructor recibe columnas: armarla con las filas y transponer.
    return transpose(mat4{vec4{row0.m_x, row0.m_y, row0.m_z, -dot(p.m_b, t)},
                          vec4{row1.m_x, row1.m_y, row1.m_z, dot(p.m_a, t)},
                          vec4{row2.m_x, row2.m_y, row2.m_z, -dot(p.m_d, s)},
                          vec4{row3.m_x, row3.m_y, row3.m_z, dot(p.m_c, s)}});
}

vec3 transform_point(const mat4 &m, vec3 point) {
    return xyz(m * vec4{point.m_x, point.m_y, point.m_z, 1.0F});
}

vec3 transform_direction(const mat4 &m, vec3 direction) {
    return xyz(m * vec4{direction.m_x, direction.m_y, direction.m_z, 0.0F});
}

bool nearly_equal(const mat4 &a, const mat4 &b, float tol) {
    return nearly_equal(a[0], b[0], tol) && nearly_equal(a[1], b[1], tol) &&
           nearly_equal(a[2], b[2], tol) && nearly_equal(a[3], b[3], tol);
}

} // namespace mz::core
