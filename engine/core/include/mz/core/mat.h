#pragma once

// Matrices de 3×3 y 4×4 floats, con la convención de GLSL:
//
//   - Vectores columna: un vector se transforma con M · v, y una cadena de
//     transformaciones se lee de derecha a izquierda. P · V · M · v aplica primero M (el
//     modelo), después V (la cámara) y al final P (la proyección).
//   - Guardado por columnas: en memoria, la columna 0 entera, después la 1, y así. m[c] es
//     la columna c, y m[c].m_y es la fila 1 de esa columna. Así una mat4 se copia tal cual
//     al buffer de la GPU, sin transponerla. Una mat3 no: en un buffer de GLSL (std140 y
//     std430), cada columna de una mat3 ocupa 16 bytes, y aquí ocupa 12. Se sube como
//     mat4, o con relleno.
//
// Las dos cosas son independientes: DirectXMath usa vectores fila (v · M) guardados por
// filas, y en memoria queda el mismo orden de floats.
//
// Una matriz nueva es la identidad. La matriz cero casi nunca es lo que se quiere, y no
// tiene inversa.

#include "mz/core/assert.h"
#include "mz/core/result.h"
#include "mz/core/vec.h"

#include <array>
#include <cstddef>

namespace mz::core {

/// El error de inverse(): la matriz no tiene inversa, o le falta tan poco para no tenerla
/// que la inversa no serviría (ver inverse).
struct singular_matrix {};

class mat3 {
public:
    constexpr mat3() = default;
    constexpr mat3(vec3 column0, vec3 column1, vec3 column2)
        : m_columns{column0, column1, column2} {}

    [[nodiscard]] constexpr vec3 &operator[](std::size_t column) {
        MZ_ASSERT(column < 3, "columna {} de una mat3", column);
        return m_columns[column];
    }
    [[nodiscard]] constexpr const vec3 &operator[](std::size_t column) const {
        MZ_ASSERT(column < 3, "columna {} de una mat3", column);
        return m_columns[column];
    }

private:
    std::array<vec3, 3> m_columns{vec3{1.0F, 0.0F, 0.0F}, vec3{0.0F, 1.0F, 0.0F},
                                  vec3{0.0F, 0.0F, 1.0F}};
};

class mat4 {
public:
    constexpr mat4() = default;
    constexpr mat4(vec4 column0, vec4 column1, vec4 column2, vec4 column3)
        : m_columns{column0, column1, column2, column3} {}

    [[nodiscard]] constexpr vec4 &operator[](std::size_t column) {
        MZ_ASSERT(column < 4, "columna {} de una mat4", column);
        return m_columns[column];
    }
    [[nodiscard]] constexpr const vec4 &operator[](std::size_t column) const {
        MZ_ASSERT(column < 4, "columna {} de una mat4", column);
        return m_columns[column];
    }

private:
    std::array<vec4, 4> m_columns{vec4{1.0F, 0.0F, 0.0F, 0.0F}, vec4{0.0F, 1.0F, 0.0F, 0.0F},
                                  vec4{0.0F, 0.0F, 1.0F, 0.0F}, vec4{0.0F, 0.0F, 0.0F, 1.0F}};
};

// --- mat3 -------------------------------------------------------------------------------

/// La combinación de las columnas de m con los componentes de v como pesos.
[[nodiscard]] vec3 operator*(const mat3 &m, vec3 v);
/// Aplicar b y después a. Cada columna del producto es a · (esa columna de b).
[[nodiscard]] mat3 operator*(const mat3 &a, const mat3 &b);
[[nodiscard]] mat3 transpose(const mat3 &m);
/// El volumen con signo del paralelepípedo que forman las columnas.
[[nodiscard]] float determinant(const mat3 &m);

/// La inversa, o singular_matrix si m no tiene una que sirva. Es singular cuando
///
///     |det(m)| ≤ tol · |columna 0| · |columna 1| · |columna 2|
///
/// El lado derecho es el volumen que tendrían las columnas si fueran perpendiculares
/// entre sí (desigualdad de Hadamard), así que la razón entre los dos está en [0, 1] y no
/// depende de la escala: un objeto escalado a 0,01 no parece singular, aunque su
/// determinante sea 1e-6. Una matriz con NaN o infinitos también es singular.
///
/// Quien sabe que m tiene inversa (la de una cámara, por ejemplo) la pide con
/// inverse(m, tol).value(), que aborta con un mensaje si se equivocó.
[[nodiscard]] result<mat3, singular_matrix> inverse(const mat3 &m, float tol);

/// Casi iguales si lo son todas las columnas (ver nearly_equal en scalar.h).
[[nodiscard]] bool nearly_equal(const mat3 &a, const mat3 &b, float tol);

// --- mat4 -------------------------------------------------------------------------------

[[nodiscard]] vec4 operator*(const mat4 &m, vec4 v);
[[nodiscard]] mat4 operator*(const mat4 &a, const mat4 &b);
[[nodiscard]] mat4 transpose(const mat4 &m);
[[nodiscard]] float determinant(const mat4 &m);

/// Como la de mat3, con el largo de las cuatro columnas.
[[nodiscard]] result<mat4, singular_matrix> inverse(const mat4 &m, float tol);

/// Un punto tiene w = 1: la traslación lo mueve. No divide por w, así que es para
/// transformaciones afines (las que no son una proyección).
[[nodiscard]] vec3 transform_point(const mat4 &m, vec3 point);
/// Una dirección tiene w = 0: la traslación no la mueve.
[[nodiscard]] vec3 transform_direction(const mat4 &m, vec3 direction);

[[nodiscard]] bool nearly_equal(const mat4 &a, const mat4 &b, float tol);

} // namespace mz::core
