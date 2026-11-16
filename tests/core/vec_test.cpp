#include "mz/core/vec.h"

#include "math_assertions.h"

#include "mz/test/property.h"

#include <gtest/gtest.h>

#include <type_traits>

namespace {

using mz::core::vec2;
using mz::core::vec3;
using mz::core::vec4;
using mz::test::is_near;

// Floats juntos, sin relleno: un arreglo de vec3 es un arreglo de floats, y se copia como
// bytes.
static_assert(sizeof(vec2) == 2 * sizeof(float));
static_assert(sizeof(vec3) == 3 * sizeof(float));
static_assert(sizeof(vec4) == 4 * sizeof(float));
static_assert(std::is_standard_layout_v<vec3>);

// Se construyen con todos los componentes o con ninguno (cero). Con un agregado,
// vec3{1, 2} compilaría y dejaría z en 0 sin avisar.
static_assert(std::is_constructible_v<vec3, float, float, float>);
static_assert(!std::is_constructible_v<vec3, float, float>);
static_assert(!std::is_constructible_v<vec3, float>);
static_assert(!std::is_constructible_v<vec2, float>);
static_assert(!std::is_constructible_v<vec4, float, float, float>);
static_assert(std::is_trivially_copyable_v<vec2>);
static_assert(std::is_trivially_copyable_v<vec3>);
static_assert(std::is_trivially_copyable_v<vec4>);

// Los valores de estos tests son exactos en float (enteros chicos, potencias de 2), así
// que la tolerancia es 0.

// --- vec3 -------------------------------------------------------------------------------

TEST(Vec3Test, DefaultIsZero) { EXPECT_TRUE(is_near(vec3{}, vec3{0.0F, 0.0F, 0.0F}, 0.0F)); }

TEST(Vec3Test, AddsAndSubtractsComponentWise) {
    const vec3 a{1.0F, 2.0F, 3.0F};
    const vec3 b{4.0F, 5.0F, 6.0F};
    EXPECT_TRUE(is_near(a + b, vec3{5.0F, 7.0F, 9.0F}, 0.0F));
    EXPECT_TRUE(is_near(a - b, vec3{-3.0F, -3.0F, -3.0F}, 0.0F));
    EXPECT_TRUE(is_near(-a, vec3{-1.0F, -2.0F, -3.0F}, 0.0F));
}

TEST(Vec3Test, ScalesOnEitherSideAndDivides) {
    const vec3 a{1.0F, -2.0F, 3.0F};
    EXPECT_TRUE(is_near(a * 2.0F, vec3{2.0F, -4.0F, 6.0F}, 0.0F));
    EXPECT_TRUE(is_near(2.0F * a, vec3{2.0F, -4.0F, 6.0F}, 0.0F));
    EXPECT_TRUE(is_near(a / 2.0F, vec3{0.5F, -1.0F, 1.5F}, 0.0F));
}

TEST(Vec3Test, DotProduct) {
    EXPECT_FLOAT_EQ(dot(vec3{1.0F, 2.0F, 3.0F}, vec3{4.0F, -5.0F, 6.0F}), 12.0F);
}

// Mano derecha: x × y = z, y el orden importa.
TEST(Vec3Test, CrossProductFollowsTheRightHandRule) {
    const vec3 x{1.0F, 0.0F, 0.0F};
    const vec3 y{0.0F, 1.0F, 0.0F};
    const vec3 z{0.0F, 0.0F, 1.0F};
    EXPECT_TRUE(is_near(cross(x, y), z, 0.0F));
    EXPECT_TRUE(is_near(cross(y, x), -z, 0.0F));
    EXPECT_TRUE(is_near(cross(vec3{1.0F, 2.0F, 3.0F}, vec3{4.0F, 5.0F, 6.0F}),
                        vec3{-3.0F, 6.0F, -3.0F}, 0.0F));
}

TEST(Vec3Test, LengthOfAPythagoreanQuadruple) {
    const vec3 v{2.0F, 3.0F, 6.0F};
    EXPECT_FLOAT_EQ(length_squared(v), 49.0F);
    EXPECT_FLOAT_EQ(length(v), 7.0F);
}

TEST(Vec3Test, NormalizeKeepsTheDirection) {
    EXPECT_TRUE(is_near(normalize(vec3{2.0F, 3.0F, 6.0F}),
                        vec3{2.0F / 7.0F, 3.0F / 7.0F, 6.0F / 7.0F}, 1e-6F));
}

// Exacta en los extremos: con t = 1 da b, sin el error de redondeo de a + t · (b − a).
TEST(Vec3Test, LerpIsExactAtTheEndsAndLinearBetween) {
    const vec3 a{0.1F, -7.3F, 1e6F};
    const vec3 b{3.7F, 0.3F, -2.9F};
    EXPECT_TRUE(is_near(lerp(a, b, 0.0F), a, 0.0F));
    EXPECT_TRUE(is_near(lerp(a, b, 1.0F), b, 0.0F));
    EXPECT_TRUE(is_near(lerp(vec3{0.0F, 2.0F, 4.0F}, vec3{2.0F, 4.0F, 8.0F}, 0.5F),
                        vec3{1.0F, 3.0F, 6.0F}, 0.0F));
}

TEST(Vec3Test, MinAndMaxAreComponentWise) {
    const vec3 a{1.0F, 5.0F, -3.0F};
    const vec3 b{2.0F, 4.0F, -6.0F};
    EXPECT_TRUE(is_near(min(a, b), vec3{1.0F, 4.0F, -6.0F}, 0.0F));
    EXPECT_TRUE(is_near(max(a, b), vec3{2.0F, 5.0F, -3.0F}, 0.0F));
}

// Casi iguales si lo son todos los componentes, cada uno con su escala.
TEST(Vec3Test, NearlyEqualComparesEveryComponent) {
    const vec3 a{1.0F, 2.0F, 3.0F};
    EXPECT_TRUE(nearly_equal(a, vec3{1.0F, 2.0F, 3.0F + 1e-7F}, 1e-6F));
    EXPECT_FALSE(nearly_equal(a, vec3{1.1F, 2.0F, 3.0F}, 1e-6F));
    EXPECT_FALSE(nearly_equal(a, vec3{1.0F, 2.1F, 3.0F}, 1e-6F));
    EXPECT_FALSE(nearly_equal(a, vec3{1.0F, 2.0F, 3.1F}, 1e-6F));
}

// El vector cero no tiene dirección: normalizarlo es un error del programador.
TEST(Vec3DeathTest, NormalizingZeroIsAProgrammerError) {
    EXPECT_DEATH(static_cast<void>(normalize(vec3{})), "vector de largo 0");
}

// --- vec2 -------------------------------------------------------------------------------

TEST(Vec2Test, DefaultIsZero) { EXPECT_TRUE(is_near(vec2{}, vec2{0.0F, 0.0F}, 0.0F)); }

TEST(Vec2Test, Arithmetic) {
    const vec2 a{1.0F, 2.0F};
    const vec2 b{4.0F, -5.0F};
    EXPECT_TRUE(is_near(a + b, vec2{5.0F, -3.0F}, 0.0F));
    EXPECT_TRUE(is_near(a - b, vec2{-3.0F, 7.0F}, 0.0F));
    EXPECT_TRUE(is_near(-a, vec2{-1.0F, -2.0F}, 0.0F));
    EXPECT_TRUE(is_near(a * 2.0F, vec2{2.0F, 4.0F}, 0.0F));
    EXPECT_TRUE(is_near(2.0F * a, vec2{2.0F, 4.0F}, 0.0F));
    EXPECT_TRUE(is_near(a / 2.0F, vec2{0.5F, 1.0F}, 0.0F));
}

TEST(Vec2Test, DotLengthAndNormalize) {
    const vec2 v{3.0F, 4.0F};
    EXPECT_FLOAT_EQ(dot(v, vec2{1.0F, 2.0F}), 11.0F);
    EXPECT_FLOAT_EQ(length_squared(v), 25.0F);
    EXPECT_FLOAT_EQ(length(v), 5.0F);
    EXPECT_TRUE(is_near(normalize(v), vec2{0.6F, 0.8F}, 1e-6F));
}

TEST(Vec2Test, LerpMinMax) {
    const vec2 a{0.0F, 6.0F};
    const vec2 b{2.0F, 4.0F};
    EXPECT_TRUE(is_near(lerp(a, b, 0.5F), vec2{1.0F, 5.0F}, 0.0F));
    EXPECT_TRUE(is_near(min(a, b), vec2{0.0F, 4.0F}, 0.0F));
    EXPECT_TRUE(is_near(max(a, b), vec2{2.0F, 6.0F}, 0.0F));
}

TEST(Vec2Test, NearlyEqualComparesEveryComponent) {
    const vec2 a{1.0F, 2.0F};
    EXPECT_TRUE(nearly_equal(a, vec2{1.0F, 2.0F + 1e-7F}, 1e-6F));
    EXPECT_FALSE(nearly_equal(a, vec2{1.1F, 2.0F}, 1e-6F));
    EXPECT_FALSE(nearly_equal(a, vec2{1.0F, 2.1F}, 1e-6F));
}

TEST(Vec2DeathTest, NormalizingZeroIsAProgrammerError) {
    EXPECT_DEATH(static_cast<void>(normalize(vec2{})), "vector de largo 0");
}

// --- vec4 -------------------------------------------------------------------------------

TEST(Vec4Test, DefaultIsZero) { EXPECT_TRUE(is_near(vec4{}, vec4{0.0F, 0.0F, 0.0F, 0.0F}, 0.0F)); }

TEST(Vec4Test, Arithmetic) {
    const vec4 a{1.0F, 2.0F, 3.0F, 4.0F};
    const vec4 b{4.0F, -5.0F, 6.0F, 0.0F};
    EXPECT_TRUE(is_near(a + b, vec4{5.0F, -3.0F, 9.0F, 4.0F}, 0.0F));
    EXPECT_TRUE(is_near(a - b, vec4{-3.0F, 7.0F, -3.0F, 4.0F}, 0.0F));
    EXPECT_TRUE(is_near(-a, vec4{-1.0F, -2.0F, -3.0F, -4.0F}, 0.0F));
    EXPECT_TRUE(is_near(a * 2.0F, vec4{2.0F, 4.0F, 6.0F, 8.0F}, 0.0F));
    EXPECT_TRUE(is_near(2.0F * a, vec4{2.0F, 4.0F, 6.0F, 8.0F}, 0.0F));
    EXPECT_TRUE(is_near(a / 2.0F, vec4{0.5F, 1.0F, 1.5F, 2.0F}, 0.0F));
}

TEST(Vec4Test, DotLengthAndNormalize) {
    const vec4 v{1.0F, 1.0F, 1.0F, 1.0F};
    EXPECT_FLOAT_EQ(dot(v, vec4{1.0F, 2.0F, 3.0F, 4.0F}), 10.0F);
    EXPECT_FLOAT_EQ(length_squared(v), 4.0F);
    EXPECT_FLOAT_EQ(length(v), 2.0F);
    EXPECT_TRUE(is_near(normalize(v), vec4{0.5F, 0.5F, 0.5F, 0.5F}, 1e-6F));
}

TEST(Vec4Test, LerpMinMax) {
    const vec4 a{0.0F, 6.0F, -2.0F, 1.0F};
    const vec4 b{2.0F, 4.0F, -4.0F, 1.0F};
    EXPECT_TRUE(is_near(lerp(a, b, 0.5F), vec4{1.0F, 5.0F, -3.0F, 1.0F}, 0.0F));
    EXPECT_TRUE(is_near(min(a, b), vec4{0.0F, 4.0F, -4.0F, 1.0F}, 0.0F));
    EXPECT_TRUE(is_near(max(a, b), vec4{2.0F, 6.0F, -2.0F, 1.0F}, 0.0F));
}

TEST(Vec4Test, NearlyEqualComparesEveryComponent) {
    const vec4 a{1.0F, 2.0F, 3.0F, 4.0F};
    EXPECT_TRUE(nearly_equal(a, vec4{1.0F, 2.0F, 3.0F, 4.0F + 1e-7F}, 1e-6F));
    EXPECT_FALSE(nearly_equal(a, vec4{1.1F, 2.0F, 3.0F, 4.0F}, 1e-6F));
    EXPECT_FALSE(nearly_equal(a, vec4{1.0F, 2.1F, 3.0F, 4.0F}, 1e-6F));
    EXPECT_FALSE(nearly_equal(a, vec4{1.0F, 2.0F, 3.1F, 4.0F}, 1e-6F));
    EXPECT_FALSE(nearly_equal(a, vec4{1.0F, 2.0F, 3.0F, 4.1F}, 1e-6F));
}

TEST(Vec4DeathTest, NormalizingZeroIsAProgrammerError) {
    EXPECT_DEATH(static_cast<void>(normalize(vec4{})), "vector de largo 0");
}

// --- Propiedades ------------------------------------------------------------------------

vec3 random_vec3(mz::test::property_rng &rng) {
    return {rng.next_float(-10.0F, 10.0F), rng.next_float(-10.0F, 10.0F),
            rng.next_float(-10.0F, 10.0F)};
}

// a × b es perpendicular a a y a b. El error de redondeo de dot(a × b, a) crece con el
// tamaño de lo que se multiplicó (|a|² · |b|): la tolerancia es absoluta (se compara con
// 0) y escala con esa magnitud.
TEST(Vec3Property, CrossIsPerpendicularToBothFactors) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const vec3 a = random_vec3(rng);
        const vec3 b = random_vec3(rng);
        const vec3 c = cross(a, b);
        const float size = length(a) * length(b);
        EXPECT_TRUE(mz::core::nearly_equal(dot(c, a), 0.0F, 1e-6F * size * length(a)));
        EXPECT_TRUE(mz::core::nearly_equal(dot(c, b), 0.0F, 1e-6F * size * length(b)));
    }
}

// Identidad de Lagrange: |a × b|² + (a · b)² = |a|² · |b|². El seno y el coseno del ángulo
// entre a y b, al cuadrado, suman 1.
TEST(Vec3Property, CrossAndDotSatisfyLagrangesIdentity) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const vec3 a = random_vec3(rng);
        const vec3 b = random_vec3(rng);
        const float d = dot(a, b);
        EXPECT_TRUE(mz::core::nearly_equal(length_squared(cross(a, b)) + d * d,
                                           length_squared(a) * length_squared(b), 1e-5F));
    }
}

TEST(Vec3Property, NormalizeGivesUnitLength) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const vec3 v = random_vec3(rng);
        if (length_squared(v) < 1e-6F) {
            continue;
        }
        EXPECT_TRUE(mz::core::nearly_equal(length(normalize(v)), 1.0F, 1e-6F));
    }
}

TEST(Vec3Property, DotIsCommutative) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const vec3 a = random_vec3(rng);
        const vec3 b = random_vec3(rng);
        EXPECT_TRUE(mz::core::nearly_equal(dot(a, b), dot(b, a), 0.0F));
    }
}

} // namespace
