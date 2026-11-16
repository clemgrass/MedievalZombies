#pragma once

// is_near(actual, expected, tol): nearly_equal como aserción de GoogleTest. Con
// EXPECT_TRUE(nearly_equal(...)), una falla solo dice "false"; con
// EXPECT_TRUE(is_near(...)), muestra los dos valores. is_same_rotation es lo mismo para
// dos cuaterniones que pueden diferir en el signo.

#include "mz/core/mat.h"
#include "mz/core/quat.h"
#include "mz/core/vec.h"

#include <gtest/gtest.h>

#include <concepts>
#include <format>
#include <string>

namespace mz::test {

inline std::string describe(const core::vec2 &v) { return std::format("({}, {})", v.m_x, v.m_y); }

inline std::string describe(const core::vec3 &v) {
    return std::format("({}, {}, {})", v.m_x, v.m_y, v.m_z);
}

inline std::string describe(const core::vec4 &v) {
    return std::format("({}, {}, {}, {})", v.m_x, v.m_y, v.m_z, v.m_w);
}

inline std::string describe(const core::quat &q) {
    return std::format("quat({}, {}, {}, {})", q.m_x, q.m_y, q.m_z, q.m_w);
}

// Por columnas, igual que se guardan.
inline std::string describe(const core::mat3 &m) {
    return std::format("columnas [{}, {}, {}]", describe(m[0]), describe(m[1]), describe(m[2]));
}

inline std::string describe(const core::mat4 &m) {
    return std::format("columnas [{}, {}, {}, {}]", describe(m[0]), describe(m[1]), describe(m[2]),
                       describe(m[3]));
}

// Lo que is_near sabe comparar y mostrar: los vectores, las matrices y los cuaterniones.
template <typename T>
concept comparable_math_value = requires(const T &value) {
    { core::nearly_equal(value, value, 0.0F) } -> std::same_as<bool>;
    { describe(value) } -> std::same_as<std::string>;
};

template <comparable_math_value T>
::testing::AssertionResult is_near(const T &actual, const T &expected, float tol) {
    if (core::nearly_equal(actual, expected, tol)) {
        return ::testing::AssertionSuccess();
    }
    return ::testing::AssertionFailure() << describe(actual) << " no es casi igual a "
                                         << describe(expected) << " (tolerancia " << tol << ")";
}

// Si los dos cuaterniones son la misma rotación: q o −q. Al volver de una matriz, o al
// final de un slerp, el signo puede salir cambiado, y está bien.
inline ::testing::AssertionResult is_same_rotation(const core::quat &actual,
                                                   const core::quat &expected, float tol) {
    if (core::nearly_equal(actual, expected, tol) || core::nearly_equal(actual, -expected, tol)) {
        return ::testing::AssertionSuccess();
    }
    return ::testing::AssertionFailure()
           << describe(actual) << " no es la rotación de " << describe(expected)
           << " ni la de su opuesto (tolerancia " << tol << ")";
}

} // namespace mz::test
