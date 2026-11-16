#include "mz/core/mat.h"

#include "math_assertions.h"

#include "mz/core/scalar.h"
#include "mz/test/property.h"

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <limits>
#include <type_traits>

namespace {

using mz::core::mat3;
using mz::core::mat4;
using mz::core::vec3;
using mz::core::vec4;
using mz::test::is_near;

// Las columnas, una detrás de otra y sin relleno: 9 y 16 floats, que se copian como bytes.
static_assert(sizeof(mat3) == 9 * sizeof(float));
static_assert(sizeof(mat4) == 16 * sizeof(float));
static_assert(std::is_standard_layout_v<mat4>);
static_assert(std::is_trivially_copyable_v<mat3>);
static_assert(std::is_trivially_copyable_v<mat4>);

constexpr float nan = std::numeric_limits<float>::quiet_NaN();

// Traslación (5, 6, 7) y escala (2, 3, 4), escritas columna por columna.
const mat4 translation{vec4{1.0F, 0.0F, 0.0F, 0.0F}, vec4{0.0F, 1.0F, 0.0F, 0.0F},
                       vec4{0.0F, 0.0F, 1.0F, 0.0F}, vec4{5.0F, 6.0F, 7.0F, 1.0F}};
const mat4 scale{vec4{2.0F, 0.0F, 0.0F, 0.0F}, vec4{0.0F, 3.0F, 0.0F, 0.0F},
                 vec4{0.0F, 0.0F, 4.0F, 0.0F}, vec4{0.0F, 0.0F, 0.0F, 1.0F}};

// --- Guardado por columnas ----------------------------------------------------------------

TEST(Mat4Test, DefaultIsIdentity) {
    const mat4 m;
    EXPECT_TRUE(is_near(m[0], vec4{1.0F, 0.0F, 0.0F, 0.0F}, 0.0F));
    EXPECT_TRUE(is_near(m[1], vec4{0.0F, 1.0F, 0.0F, 0.0F}, 0.0F));
    EXPECT_TRUE(is_near(m[2], vec4{0.0F, 0.0F, 1.0F, 0.0F}, 0.0F));
    EXPECT_TRUE(is_near(m[3], vec4{0.0F, 0.0F, 0.0F, 1.0F}, 0.0F));
}

// m[c] es la columna c, como en GLSL: m[3].m_y es la fila 1 de la columna 3.
TEST(Mat4Test, IndexingReturnsAColumn) {
    mat4 m = translation;
    EXPECT_TRUE(is_near(m[3], vec4{5.0F, 6.0F, 7.0F, 1.0F}, 0.0F));
    m[3].m_y = 9.0F;
    EXPECT_TRUE(is_near(m[3], vec4{5.0F, 9.0F, 7.0F, 1.0F}, 0.0F));
}

// En memoria, la columna 0 ocupa los floats 0 a 3, la columna 1 los floats 4 a 7, y así.
// Es lo que espera GLSL: la matriz se copia al buffer de la GPU sin transponer.
TEST(Mat4Test, ColumnsAreContiguousInMemory) {
    const mat4 m{vec4{0.0F, 1.0F, 2.0F, 3.0F}, vec4{4.0F, 5.0F, 6.0F, 7.0F},
                 vec4{8.0F, 9.0F, 10.0F, 11.0F}, vec4{12.0F, 13.0F, 14.0F, 15.0F}};
    std::array<float, 16> floats{};
    std::memcpy(floats.data(), &m, sizeof(m));
    for (std::size_t i = 0; i < floats.size(); ++i) {
        EXPECT_FLOAT_EQ(floats[i], static_cast<float>(i)) << "float " << i;
    }
}

TEST(Mat3Test, DefaultIsIdentityAndIndexingReturnsAColumn) {
    mat3 m;
    EXPECT_TRUE(is_near(m[0], vec3{1.0F, 0.0F, 0.0F}, 0.0F));
    EXPECT_TRUE(is_near(m[1], vec3{0.0F, 1.0F, 0.0F}, 0.0F));
    EXPECT_TRUE(is_near(m[2], vec3{0.0F, 0.0F, 1.0F}, 0.0F));
    m[2].m_x = 4.0F;
    EXPECT_TRUE(is_near(m[2], vec3{4.0F, 0.0F, 1.0F}, 0.0F));
}

TEST(Mat3Test, ColumnsAreContiguousInMemory) {
    const mat3 m{vec3{0.0F, 1.0F, 2.0F}, vec3{3.0F, 4.0F, 5.0F}, vec3{6.0F, 7.0F, 8.0F}};
    std::array<float, 9> floats{};
    std::memcpy(floats.data(), &m, sizeof(m));
    for (std::size_t i = 0; i < floats.size(); ++i) {
        EXPECT_FLOAT_EQ(floats[i], static_cast<float>(i)) << "float " << i;
    }
}

TEST(Mat4DeathTest, ColumnOutOfRangeIsAProgrammerError) {
    mat4 m;
    const mat4 &constant = m;
    EXPECT_DEATH(static_cast<void>(m[4]), "columna 4 de una mat4");
    EXPECT_DEATH(static_cast<void>(constant[4]), "columna 4 de una mat4");
}

TEST(Mat3DeathTest, ColumnOutOfRangeIsAProgrammerError) {
    mat3 m;
    const mat3 &constant = m;
    EXPECT_DEATH(static_cast<void>(m[3]), "columna 3 de una mat3");
    EXPECT_DEATH(static_cast<void>(constant[3]), "columna 3 de una mat3");
}

// --- Producto ---------------------------------------------------------------------------

// M · v es la combinación de las columnas de M con los componentes de v como pesos.
TEST(Mat4Test, TimesVectorCombinesTheColumns) {
    EXPECT_TRUE(
        is_near(translation * vec4{1.0F, 2.0F, 3.0F, 1.0F}, vec4{6.0F, 8.0F, 10.0F, 1.0F}, 0.0F));
    EXPECT_TRUE(
        is_near(translation * vec4{1.0F, 2.0F, 3.0F, 0.0F}, vec4{1.0F, 2.0F, 3.0F, 0.0F}, 0.0F));
}

// Con vectores columna, T · S · p escala primero y traslada después: se lee de derecha a
// izquierda. Al revés da otro resultado.
TEST(Mat4Test, ProductAppliesTheRightFactorFirst) {
    const mat4 scale_then_translate{vec4{2.0F, 0.0F, 0.0F, 0.0F}, vec4{0.0F, 3.0F, 0.0F, 0.0F},
                                    vec4{0.0F, 0.0F, 4.0F, 0.0F}, vec4{5.0F, 6.0F, 7.0F, 1.0F}};
    EXPECT_TRUE(is_near(translation * scale, scale_then_translate, 0.0F));

    const vec4 p{1.0F, 1.0F, 1.0F, 1.0F};
    EXPECT_TRUE(is_near(translation * scale * p, vec4{7.0F, 9.0F, 11.0F, 1.0F}, 0.0F));
    EXPECT_TRUE(is_near(scale * translation * p, vec4{12.0F, 21.0F, 32.0F, 1.0F}, 0.0F));
}

TEST(Mat3Test, Products) {
    const mat3 a{vec3{1.0F, 2.0F, 0.0F}, vec3{0.0F, 1.0F, 0.0F}, vec3{0.0F, 0.0F, 3.0F}};
    const mat3 b{vec3{0.0F, 1.0F, 0.0F}, vec3{1.0F, 0.0F, 0.0F}, vec3{0.0F, 0.0F, 1.0F}};
    EXPECT_TRUE(is_near(a * vec3{1.0F, 1.0F, 1.0F}, vec3{1.0F, 3.0F, 3.0F}, 0.0F));
    // Las columnas de a · b son a · (columnas de b): b cambia x con y, así que a · b
    // cambia las dos primeras columnas de a.
    EXPECT_TRUE(is_near(
        a * b, mat3{vec3{0.0F, 1.0F, 0.0F}, vec3{1.0F, 2.0F, 0.0F}, vec3{0.0F, 0.0F, 3.0F}}, 0.0F));
}

// --- Punto y dirección --------------------------------------------------------------------

// Un punto tiene w = 1 y la traslación lo mueve. Una dirección tiene w = 0 y no la mueve.
TEST(Mat4Test, TranslationMovesPointsButNotDirections) {
    const mat4 m = translation * scale;
    EXPECT_TRUE(is_near(transform_point(m, vec3{1.0F, 1.0F, 1.0F}), vec3{7.0F, 9.0F, 11.0F}, 0.0F));
    EXPECT_TRUE(
        is_near(transform_direction(m, vec3{1.0F, 1.0F, 1.0F}), vec3{2.0F, 3.0F, 4.0F}, 0.0F));
}

// --- Transpuesta y determinante -----------------------------------------------------------

TEST(Mat4Test, TransposeSwapsRowsAndColumns) {
    const mat4 m{vec4{0.0F, 1.0F, 2.0F, 3.0F}, vec4{4.0F, 5.0F, 6.0F, 7.0F},
                 vec4{8.0F, 9.0F, 10.0F, 11.0F}, vec4{12.0F, 13.0F, 14.0F, 15.0F}};
    const mat4 expected{vec4{0.0F, 4.0F, 8.0F, 12.0F}, vec4{1.0F, 5.0F, 9.0F, 13.0F},
                        vec4{2.0F, 6.0F, 10.0F, 14.0F}, vec4{3.0F, 7.0F, 11.0F, 15.0F}};
    EXPECT_TRUE(is_near(transpose(m), expected, 0.0F));
}

TEST(Mat3Test, TransposeSwapsRowsAndColumns) {
    const mat3 m{vec3{0.0F, 1.0F, 2.0F}, vec3{3.0F, 4.0F, 5.0F}, vec3{6.0F, 7.0F, 8.0F}};
    const mat3 expected{vec3{0.0F, 3.0F, 6.0F}, vec3{1.0F, 4.0F, 7.0F}, vec3{2.0F, 5.0F, 8.0F}};
    EXPECT_TRUE(is_near(transpose(m), expected, 0.0F));
}

// El determinante es el volumen con signo que ocupan las columnas: la escala (2, 3, 4)
// multiplica los volúmenes por 24, y cambiar dos ejes invierte la orientación (signo −).
TEST(Mat4Test, DeterminantIsTheSignedVolume) {
    EXPECT_FLOAT_EQ(determinant(mat4{}), 1.0F);
    EXPECT_FLOAT_EQ(determinant(translation * scale), 24.0F);
    const mat4 swap_x_y{vec4{0.0F, 1.0F, 0.0F, 0.0F}, vec4{1.0F, 0.0F, 0.0F, 0.0F},
                        vec4{0.0F, 0.0F, 1.0F, 0.0F}, vec4{0.0F, 0.0F, 0.0F, 1.0F}};
    EXPECT_FLOAT_EQ(determinant(swap_x_y), -1.0F);
}

TEST(Mat3Test, DeterminantIsTheSignedVolume) {
    EXPECT_FLOAT_EQ(determinant(mat3{}), 1.0F);
    EXPECT_FLOAT_EQ(
        determinant(mat3{vec3{2.0F, 0.0F, 0.0F}, vec3{0.0F, 4.0F, 0.0F}, vec3{1.0F, 0.0F, 1.0F}}),
        8.0F);
    EXPECT_FLOAT_EQ(
        determinant(mat3{vec3{0.0F, 1.0F, 0.0F}, vec3{1.0F, 0.0F, 0.0F}, vec3{0.0F, 0.0F, 1.0F}}),
        -1.0F);
}

// --- Inversa ------------------------------------------------------------------------------

// (T · S)⁻¹ = S⁻¹ · T⁻¹: deshace la traslación y después la escala.
TEST(Mat4Test, InverseOfAnAffineTransform) {
    const mat4 expected{vec4{0.5F, 0.0F, 0.0F, 0.0F}, vec4{0.0F, 1.0F / 3.0F, 0.0F, 0.0F},
                        vec4{0.0F, 0.0F, 0.25F, 0.0F}, vec4{-2.5F, -2.0F, -1.75F, 1.0F}};
    const auto inverse = mz::core::inverse(translation * scale, 1e-6F);
    ASSERT_TRUE(inverse.has_value());
    EXPECT_TRUE(is_near(inverse.value(), expected, 1e-6F));
}

// Una matriz que no es afín (su última fila no es 0, 0, 0, 1), como una proyección.
TEST(Mat4Test, InverseOfAMatrixThatUsesTheLastRow) {
    const mat4 m{vec4{2.0F, 0.0F, 0.0F, 1.0F}, vec4{0.0F, 1.0F, 0.0F, 0.0F},
                 vec4{0.0F, 0.0F, 1.0F, 0.0F}, vec4{1.0F, 0.0F, 0.0F, 1.0F}};
    const mat4 expected{vec4{1.0F, 0.0F, 0.0F, -1.0F}, vec4{0.0F, 1.0F, 0.0F, 0.0F},
                        vec4{0.0F, 0.0F, 1.0F, 0.0F}, vec4{-1.0F, 0.0F, 0.0F, 2.0F}};
    const auto inverse = mz::core::inverse(m, 1e-6F);
    ASSERT_TRUE(inverse.has_value());
    EXPECT_TRUE(is_near(inverse.value(), expected, 1e-6F));
}

TEST(Mat3Test, Inverse) {
    const mat3 m{vec3{2.0F, 0.0F, 0.0F}, vec3{0.0F, 4.0F, 0.0F}, vec3{1.0F, 0.0F, 1.0F}};
    const mat3 expected{vec3{0.5F, 0.0F, 0.0F}, vec3{0.0F, 0.25F, 0.0F}, vec3{-0.5F, 0.0F, 1.0F}};
    const auto inverse = mz::core::inverse(m, 1e-6F);
    ASSERT_TRUE(inverse.has_value());
    EXPECT_TRUE(is_near(inverse.value(), expected, 1e-6F));
}

// Dos columnas iguales: el volumen es 0 y no hay inversa.
TEST(Mat4Test, SingularMatrixHasNoInverse) {
    const mat4 m{vec4{1.0F, 2.0F, 3.0F, 0.0F}, vec4{1.0F, 2.0F, 3.0F, 0.0F},
                 vec4{0.0F, 0.0F, 1.0F, 0.0F}, vec4{0.0F, 0.0F, 0.0F, 1.0F}};
    EXPECT_FALSE(mz::core::inverse(m, 1e-6F).has_value());
}

TEST(Mat3Test, SingularMatrixHasNoInverse) {
    const mat3 m{vec3{1.0F, 2.0F, 3.0F}, vec3{2.0F, 4.0F, 6.0F}, vec3{0.0F, 0.0F, 1.0F}};
    EXPECT_FALSE(mz::core::inverse(m, 1e-6F).has_value());
}

// Casi singular: las dos primeras columnas difieren en 1e-7. El volumen que ocupan es una
// diez millonésima del que ocuparían columnas del mismo largo perpendiculares entre sí.
TEST(Mat4Test, NearlySingularMatrixHasNoInverse) {
    const mat4 m{vec4{1.0F, 0.0F, 0.0F, 0.0F}, vec4{1.0F, 1e-7F, 0.0F, 0.0F},
                 vec4{0.0F, 0.0F, 1.0F, 0.0F}, vec4{0.0F, 0.0F, 0.0F, 1.0F}};
    EXPECT_FALSE(mz::core::inverse(m, 1e-6F).has_value());
}

TEST(Mat3Test, NearlySingularMatrixHasNoInverse) {
    const mat3 m{vec3{1.0F, 0.0F, 0.0F}, vec3{1.0F, 1e-7F, 0.0F}, vec3{0.0F, 0.0F, 1.0F}};
    EXPECT_FALSE(mz::core::inverse(m, 1e-6F).has_value());
}

// Escalar por 0,01 da un determinante de 1e-8 (es 0,01⁴), pero la matriz está perfectamente
// condicionada. La tolerancia es relativa al largo de las columnas, así que la escala no
// importa: con |det| ≤ 1e-6 absoluto, un objeto chico parecería singular.
TEST(Mat4Test, SmallScaleIsNotSingular) {
    const mat4 small{vec4{0.01F, 0.0F, 0.0F, 0.0F}, vec4{0.0F, 0.01F, 0.0F, 0.0F},
                     vec4{0.0F, 0.0F, 0.01F, 0.0F}, vec4{0.0F, 0.0F, 0.0F, 0.01F}};
    const auto inverse = mz::core::inverse(small, 1e-6F);
    ASSERT_TRUE(inverse.has_value());
    EXPECT_TRUE(is_near(inverse.value() * small, mat4{}, 1e-6F));
}

TEST(Mat3Test, SmallScaleIsNotSingular) {
    const mat3 small{vec3{0.01F, 0.0F, 0.0F}, vec3{0.0F, 0.01F, 0.0F}, vec3{0.0F, 0.0F, 0.01F}};
    const auto inverse = mz::core::inverse(small, 1e-6F);
    ASSERT_TRUE(inverse.has_value());
    EXPECT_TRUE(is_near(inverse.value() * small, mat3{}, 1e-6F));
}

// Un NaN contamina todas las cuentas: no hay inversa que devolver.
TEST(Mat4Test, MatrixWithNanHasNoInverse) {
    mat4 m;
    m[1].m_y = nan;
    EXPECT_FALSE(mz::core::inverse(m, 1e-6F).has_value());
}

TEST(Mat3Test, MatrixWithNanHasNoInverse) {
    mat3 m;
    m[1].m_y = nan;
    EXPECT_FALSE(mz::core::inverse(m, 1e-6F).has_value());
}

// Quien sabe que la matriz tiene inversa la pide con value(): si se equivocó, el programa
// aborta con un mensaje, en vez de seguir con infinitos.
TEST(Mat4DeathTest, ValueOfTheInverseOfASingularMatrixAborts) {
    const mat4 zero{vec4{}, vec4{}, vec4{}, vec4{}};
    EXPECT_DEATH(static_cast<void>(mz::core::inverse(zero, 1e-6F).value()),
                 "value\\(\\) de un result que tiene un error");
}

// --- nearly_equal ---------------------------------------------------------------------------

TEST(Mat4Test, NearlyEqualComparesEveryColumn) {
    const mat4 m;
    for (std::size_t column = 0; column < 4; ++column) {
        mat4 other;
        other[column].m_x += 0.1F;
        EXPECT_FALSE(nearly_equal(m, other, 1e-6F)) << "columna " << column;
    }
    EXPECT_TRUE(nearly_equal(m, mat4{}, 0.0F));
}

TEST(Mat3Test, NearlyEqualComparesEveryColumn) {
    const mat3 m;
    for (std::size_t column = 0; column < 3; ++column) {
        mat3 other;
        other[column].m_x += 0.1F;
        EXPECT_FALSE(nearly_equal(m, other, 1e-6F)) << "columna " << column;
    }
    EXPECT_TRUE(nearly_equal(m, mat3{}, 0.0F));
}

// --- Propiedades ------------------------------------------------------------------------

// Entradas en [−1, 1] y la diagonal entre 3 y 4: cada fila tiene más peso en la diagonal
// que en el resto junto, y una matriz así siempre tiene inversa y está bien condicionada
// (teorema de los círculos de Gershgorin). `magnitude` la escala entera.
mat4 random_invertible_mat4(mz::test::property_rng &rng, float magnitude) {
    mat4 m;
    for (std::size_t c = 0; c < 4; ++c) {
        m[c] = vec4{rng.next_float(-1.0F, 1.0F), rng.next_float(-1.0F, 1.0F),
                    rng.next_float(-1.0F, 1.0F), rng.next_float(-1.0F, 1.0F)};
    }
    m[0].m_x = rng.next_float(3.0F, 4.0F);
    m[1].m_y = rng.next_float(3.0F, 4.0F);
    m[2].m_z = rng.next_float(3.0F, 4.0F);
    m[3].m_w = rng.next_float(3.0F, 4.0F);
    for (std::size_t c = 0; c < 4; ++c) {
        m[c] = m[c] * magnitude;
    }
    return m;
}

mat3 random_invertible_mat3(mz::test::property_rng &rng) {
    mat3 m;
    for (std::size_t c = 0; c < 3; ++c) {
        m[c] = vec3{rng.next_float(-1.0F, 1.0F), rng.next_float(-1.0F, 1.0F),
                    rng.next_float(-1.0F, 1.0F)};
    }
    m[0].m_x = rng.next_float(3.0F, 4.0F);
    m[1].m_y = rng.next_float(3.0F, 4.0F);
    m[2].m_z = rng.next_float(3.0F, 4.0F);
    return m;
}

TEST(Mat4Property, TimesItsInverseIsTheIdentity) {
    mz::test::property_rng rng;
    for (const float magnitude : {0.001F, 1.0F, 1000.0F}) {
        for (int i = 0; i < 300; ++i) {
            const mat4 m = random_invertible_mat4(rng, magnitude);
            const auto inverse = mz::core::inverse(m, 1e-6F);
            ASSERT_TRUE(inverse.has_value()) << "escala " << magnitude;
            EXPECT_TRUE(is_near(m * inverse.value(), mat4{}, 1e-5F)) << "escala " << magnitude;
            EXPECT_TRUE(is_near(inverse.value() * m, mat4{}, 1e-5F)) << "escala " << magnitude;
        }
    }
}

TEST(Mat3Property, TimesItsInverseIsTheIdentity) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const mat3 m = random_invertible_mat3(rng);
        const auto inverse = mz::core::inverse(m, 1e-6F);
        ASSERT_TRUE(inverse.has_value());
        EXPECT_TRUE(is_near(m * inverse.value(), mat3{}, 1e-5F));
        EXPECT_TRUE(is_near(inverse.value() * m, mat3{}, 1e-5F));
    }
}

// (A · B) · v = A · (B · v): multiplicar las matrices primero es lo mismo que aplicarlas
// una por una. Por eso una sola matriz P · V · M sirve para todos los vértices.
TEST(Mat4Property, ProductIsAssociativeWithVectors) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const mat4 a = random_invertible_mat4(rng, 1.0F);
        const mat4 b = random_invertible_mat4(rng, 1.0F);
        const vec4 v{rng.next_float(-10.0F, 10.0F), rng.next_float(-10.0F, 10.0F),
                     rng.next_float(-10.0F, 10.0F), 1.0F};
        EXPECT_TRUE(is_near((a * b) * v, a * (b * v), 1e-5F));
    }
}

TEST(Mat4Property, TransposeOfAProductReversesTheOrder) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const mat4 a = random_invertible_mat4(rng, 1.0F);
        const mat4 b = random_invertible_mat4(rng, 1.0F);
        EXPECT_TRUE(is_near(transpose(a * b), transpose(b) * transpose(a), 1e-5F));
        EXPECT_TRUE(is_near(transpose(transpose(a)), a, 0.0F));
    }
}

TEST(Mat4Property, DeterminantOfAProductIsTheProductOfDeterminants) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const mat4 a = random_invertible_mat4(rng, 1.0F);
        const mat4 b = random_invertible_mat4(rng, 1.0F);
        EXPECT_TRUE(
            mz::core::nearly_equal(determinant(a * b), determinant(a) * determinant(b), 1e-5F));
        EXPECT_TRUE(mz::core::nearly_equal(determinant(transpose(a)), determinant(a), 1e-5F));
    }
}

TEST(Mat3Property, DeterminantOfAProductIsTheProductOfDeterminants) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const mat3 a = random_invertible_mat3(rng);
        const mat3 b = random_invertible_mat3(rng);
        EXPECT_TRUE(
            mz::core::nearly_equal(determinant(a * b), determinant(a) * determinant(b), 1e-5F));
        EXPECT_TRUE(is_near(transpose(transpose(a)), a, 0.0F));
    }
}

} // namespace
