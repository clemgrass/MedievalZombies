#include "mz/core/plane.h"

#include "math_assertions.h"

#include "mz/core/scalar.h"
#include "mz/core/vec.h"
#include "mz/test/property.h"

#include <gtest/gtest.h>

#include <cmath>
#include <type_traits>

namespace {

using mz::core::plane;
using mz::core::signed_distance;
using mz::core::vec3;
using mz::test::is_near;

// Cuatro floats, sin relleno.
static_assert(sizeof(plane) == 4 * sizeof(float));
static_assert(std::is_trivially_copyable_v<plane>);

constexpr vec3 up{0.0F, 1.0F, 0.0F};

TEST(PlaneTest, KeepsItsNormalAndDistance) {
    const plane p{up, 2.0F};
    EXPECT_TRUE(is_near(p.normal(), up, 0.0F));
    EXPECT_FLOAT_EQ(p.distance(), 2.0F);
}

// El plano y = 2: los puntos de arriba están del lado positivo, los de abajo del negativo,
// y los del plano dan 0 justo, sin redondeo.
TEST(PlaneTest, SignedDistanceToAHorizontalPlane) {
    const plane p{up, 2.0F};
    EXPECT_FLOAT_EQ(signed_distance(p, vec3{5.0F, 5.0F, 7.0F}), 3.0F);
    EXPECT_FLOAT_EQ(signed_distance(p, vec3{0.0F, -1.0F, 0.0F}), -3.0F);
    EXPECT_TRUE(mz::core::exactly_equal(signed_distance(p, vec3{3.0F, 2.0F, -4.0F}), 0.0F));
}

// La distancia del plano es dónde está sobre su normal: el plano y = −1 tiene distancia −1,
// y el origen queda a 1 m de él, del lado positivo.
TEST(PlaneTest, NegativeDistanceMeansThePlaneIsBehindTheOrigin) {
    const plane p{up, -1.0F};
    EXPECT_FLOAT_EQ(signed_distance(p, vec3{}), 1.0F);
}

// Un plano inclinado 45° que pasa por el origen: el punto (1, 0, 0) está a 1/√2, la
// proyección de (1, 0, 0) sobre la normal (1, 1, 0)/√2.
TEST(PlaneTest, SignedDistanceToATiltedPlane) {
    const plane p{normalize(vec3{1.0F, 1.0F, 0.0F}), 0.0F};
    EXPECT_FLOAT_EQ(signed_distance(p, vec3{1.0F, 0.0F, 0.0F}), 1.0F / std::sqrt(2.0F));
    EXPECT_FLOAT_EQ(signed_distance(p, vec3{1.0F, 1.0F, 0.0F}), std::sqrt(2.0F));
    EXPECT_NEAR(signed_distance(p, vec3{1.0F, -1.0F, 9.0F}), 0.0F, 1e-6F);
}

// El plano que pasa por un punto: la distancia es la del punto sobre la normal.
TEST(PlaneTest, ThroughPointContainsThePoint) {
    const plane p = plane::through_point(vec3{1.0F, 0.0F, 0.0F}, vec3{3.0F, 9.0F, 9.0F});
    EXPECT_FLOAT_EQ(p.distance(), 3.0F);
    EXPECT_FLOAT_EQ(signed_distance(p, vec3{5.0F, 0.0F, 0.0F}), 2.0F);
}

// Con una normal de otro largo, la "distancia" saldría multiplicada por ese largo.
TEST(PlaneDeathTest, NormalThatIsNotUnitLengthIsAProgrammerError) {
    EXPECT_DEATH((plane{vec3{0.0F, 2.0F, 0.0F}, 1.0F}), "normal de largo 1");
    EXPECT_DEATH(static_cast<void>(plane::through_point(vec3{}, vec3{})), "normal de largo 1");
}

// Moverse t metros sobre la normal cambia la distancia en t, para cualquier plano y punto.
TEST(PlaneProperty, MovingAlongTheNormalChangesTheDistanceByTheSameAmount) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const vec3 direction{rng.next_float(-1.0F, 1.0F), rng.next_float(-1.0F, 1.0F),
                             rng.next_float(-1.0F, 1.0F)};
        if (length_squared(direction) < 0.01F) {
            continue;
        }
        const plane p{normalize(direction), rng.next_float(-10.0F, 10.0F)};
        const vec3 point{rng.next_float(-10.0F, 10.0F), rng.next_float(-10.0F, 10.0F),
                         rng.next_float(-10.0F, 10.0F)};
        const float t = rng.next_float(-10.0F, 10.0F);
        EXPECT_NEAR(signed_distance(p, point + p.normal() * t), signed_distance(p, point) + t,
                    1e-4F);
    }
}

} // namespace
