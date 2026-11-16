#include "mz/core/scalar.h"

#include "mz/test/property.h"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

namespace {

using mz::core::exactly_equal;
using mz::core::nearly_equal;

constexpr float nan = std::numeric_limits<float>::quiet_NaN();
constexpr float infinity = std::numeric_limits<float>::infinity();

TEST(NearlyEqualTest, SameValueIsNearlyEqual) { EXPECT_TRUE(nearly_equal(0.25F, 0.25F, 0.0F)); }

// Cerca de cero, la tolerancia es absoluta: 1e-7 y 0 difieren en menos de 1e-6.
TEST(NearlyEqualTest, NearZeroTheToleranceIsAbsolute) {
    EXPECT_TRUE(nearly_equal(0.0F, 1e-7F, 1e-6F));
    EXPECT_FALSE(nearly_equal(0.0F, 1e-5F, 1e-6F));
}

// Con números grandes, la tolerancia es relativa: entre floats cercanos a un millón hay
// pasos de 0,0625, y una tolerancia absoluta de 1e-6 exigiría que sean idénticos.
TEST(NearlyEqualTest, WithLargeValuesTheToleranceIsRelative) {
    EXPECT_TRUE(nearly_equal(1'000'000.0F, 1'000'000.5F, 1e-6F));
    EXPECT_FALSE(nearly_equal(1'000'000.0F, 1'000'002.0F, 1e-6F));
}

TEST(NearlyEqualTest, NegativeValuesUseTheirMagnitude) {
    EXPECT_TRUE(nearly_equal(-1'000'000.0F, -1'000'000.5F, 1e-6F));
    EXPECT_FALSE(nearly_equal(-1.0F, 1.0F, 1e-6F));
}

// NaN no es igual a nada, ni siquiera a sí mismo, con ninguna tolerancia.
TEST(NearlyEqualTest, NanIsNeverNearlyEqual) {
    EXPECT_FALSE(nearly_equal(nan, nan, infinity));
    EXPECT_FALSE(nearly_equal(nan, 0.0F, infinity));
    EXPECT_FALSE(nearly_equal(0.0F, nan, infinity));
}

// Un infinito es una cuenta que se desbordó: que dos se desborden no dice que sean
// parecidas. Su diferencia es NaN, así que nunca son casi iguales.
TEST(NearlyEqualTest, InfinityIsNeverNearlyEqual) {
    EXPECT_FALSE(nearly_equal(infinity, infinity, 1.0F));
    EXPECT_FALSE(nearly_equal(infinity, 1.0F, 1.0F));
}

TEST(NearlyEqualDeathTest, NegativeToleranceIsAProgrammerError) {
    EXPECT_DEATH(static_cast<void>(nearly_equal(1.0F, 1.0F, -1e-6F)), "tolerancia");
}

TEST(NearlyEqualDeathTest, NanToleranceIsAProgrammerError) {
    EXPECT_DEATH(static_cast<void>(nearly_equal(1.0F, 1.0F, nan)), "tolerancia");
}

// exactly_equal es el == de IEEE 754, para cuando el valor exacto es lo que se quiere.
TEST(ExactlyEqualTest, OnlyTheSameValueIsExactlyEqual) {
    const float one = 1.0F;
    const float next = std::nextafter(one, 2.0F);
    EXPECT_TRUE(exactly_equal(one, 1.0F));
    EXPECT_FALSE(exactly_equal(one, next));
    EXPECT_TRUE(nearly_equal(one, next, 1e-6F));
}

TEST(ExactlyEqualTest, FollowsIeee754) {
    EXPECT_TRUE(exactly_equal(0.0F, -0.0F));
    EXPECT_FALSE(exactly_equal(nan, nan));
}

TEST(NearlyEqualProperty, IsSymmetric) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const float a = rng.next_float(-1000.0F, 1000.0F);
        const float b = a + rng.next_float(-1e-3F, 1e-3F);
        const float tol = rng.next_float(0.0F, 1e-5F);
        EXPECT_EQ(nearly_equal(a, b, tol), nearly_equal(b, a, tol));
    }
}

TEST(NearlyEqualProperty, EveryFiniteValueIsNearlyEqualToItself) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const float a = rng.next_float(-1e30F, 1e30F);
        EXPECT_TRUE(nearly_equal(a, a, 0.0F));
    }
}

} // namespace
