#include "mz/core/quat.h"

#include "math_assertions.h"

#include "mz/core/mat.h"
#include "mz/core/scalar.h"
#include "mz/core/vec.h"
#include "mz/test/property.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <type_traits>

namespace {

using mz::core::euler_angles;
using mz::core::mat3;
using mz::core::mat4;
using mz::core::quat;
using mz::core::vec3;
using mz::core::vec4;
using mz::test::is_near;
using mz::test::is_same_rotation;

// Cuatro floats seguidos, en el orden de glTF: una rotación de un asset se copia tal cual.
static_assert(sizeof(quat) == 4 * sizeof(float));
static_assert(std::is_trivially_copyable_v<quat>);

constexpr float pi = std::numbers::pi_v<float>;
// sen(45°) = cos(45°) = √½: las componentes de un cuarto de vuelta, que guarda la mitad
// del ángulo.
const float sqrt_half = std::sqrt(0.5F);

constexpr vec3 x_axis{1.0F, 0.0F, 0.0F};
constexpr vec3 y_axis{0.0F, 1.0F, 0.0F};
constexpr vec3 z_axis{0.0F, 0.0F, 1.0F};
constexpr vec3 forward{0.0F, 0.0F, -1.0F};

// El ángulo de la rotación que lleva de a hasta b, en [0, π]. El valor absoluto del dot
// elige el camino corto: b y −b son la misma rotación.
float angle_between(quat a, quat b) {
    return 2.0F * std::acos(std::min(1.0F, std::fabs(dot(a, b))));
}

// --- Construir ----------------------------------------------------------------------------

TEST(QuatTest, DefaultIsTheIdentity) {
    const quat q;
    EXPECT_TRUE(is_near(q, quat{0.0F, 0.0F, 0.0F, 1.0F}, 0.0F));
    EXPECT_TRUE(is_near(rotate(q, vec3{1.0F, 2.0F, 3.0F}), vec3{1.0F, 2.0F, 3.0F}, 0.0F));
}

// El cuaternión guarda la mitad del ángulo: un cuarto de vuelta tiene sen(45°) y cos(45°),
// y media vuelta tiene w = cos(90°) = 0.
TEST(QuatTest, FromAxisAngleStoresHalfTheAngle) {
    EXPECT_TRUE(is_near(quat::from_axis_angle(y_axis, pi / 2.0F),
                        quat{0.0F, sqrt_half, 0.0F, sqrt_half}, 1e-6F));
    EXPECT_TRUE(is_near(quat::from_axis_angle(x_axis, pi), quat{1.0F, 0.0F, 0.0F, 0.0F}, 1e-6F));
}

// Mano derecha: un cuarto de vuelta alrededor de Z lleva X a Y, alrededor de X lleva Y a Z,
// y alrededor de Y lleva Z a X.
TEST(QuatTest, QuarterTurnsFollowTheRightHandRule) {
    EXPECT_TRUE(is_near(rotate(quat::from_axis_angle(z_axis, pi / 2.0F), x_axis), y_axis, 1e-6F));
    EXPECT_TRUE(is_near(rotate(quat::from_axis_angle(x_axis, pi / 2.0F), y_axis), z_axis, 1e-6F));
    EXPECT_TRUE(is_near(rotate(quat::from_axis_angle(y_axis, pi / 2.0F), z_axis), x_axis, 1e-6F));
}

// --- Álgebra ------------------------------------------------------------------------------

// Las reglas de Hamilton (1843): i² = j² = k² = ijk = −1, y por eso ij = k pero ji = −k.
TEST(QuatTest, ProductFollowsHamiltonsRules) {
    const quat i{1.0F, 0.0F, 0.0F, 0.0F};
    const quat j{0.0F, 1.0F, 0.0F, 0.0F};
    const quat k{0.0F, 0.0F, 1.0F, 0.0F};
    EXPECT_TRUE(is_near(i * j, k, 0.0F));
    EXPECT_TRUE(is_near(j * i, -k, 0.0F));
    EXPECT_TRUE(is_near(k * k, quat{0.0F, 0.0F, 0.0F, -1.0F}, 0.0F));
    EXPECT_TRUE(is_near(i * j * k, quat{0.0F, 0.0F, 0.0F, -1.0F}, 0.0F));
}

// yaw · pitch aplica primero el pitch: mirar hacia arriba y después girar no mueve la
// mirada, que ya apunta al cielo. pitch · yaw gira primero hacia la izquierda (−X), y el
// pitch, alrededor de X, ya no la mueve.
TEST(QuatTest, ProductAppliesTheRightFactorFirst) {
    const quat yaw = quat::from_axis_angle(y_axis, pi / 2.0F);
    const quat pitch = quat::from_axis_angle(x_axis, pi / 2.0F);
    EXPECT_TRUE(is_near(rotate(yaw * pitch, forward), y_axis, 1e-6F));
    EXPECT_TRUE(is_near(rotate(pitch * yaw, forward), -x_axis, 1e-6F));
    EXPECT_TRUE(is_near(rotate(yaw * pitch, forward), rotate(yaw, rotate(pitch, forward)), 1e-6F));
}

TEST(QuatTest, ArithmeticIsComponentWise) {
    const quat a{1.0F, 2.0F, 3.0F, 4.0F};
    const quat b{5.0F, 6.0F, 7.0F, 8.0F};
    EXPECT_TRUE(is_near(a + b, quat{6.0F, 8.0F, 10.0F, 12.0F}, 0.0F));
    EXPECT_TRUE(is_near(a * 2.0F, quat{2.0F, 4.0F, 6.0F, 8.0F}, 0.0F));
    EXPECT_TRUE(is_near(2.0F * a, quat{2.0F, 4.0F, 6.0F, 8.0F}, 0.0F));
    EXPECT_TRUE(is_near(-a, quat{-1.0F, -2.0F, -3.0F, -4.0F}, 0.0F));
    EXPECT_FLOAT_EQ(dot(a, b), 70.0F);
    EXPECT_FLOAT_EQ(length_squared(a), 30.0F);
    EXPECT_FLOAT_EQ(length(a), std::sqrt(30.0F));
}

// El conjugado gira el mismo ángulo alrededor del eje opuesto: deshace la rotación.
TEST(QuatTest, ConjugateIsTheInverseOfAUnitQuat) {
    const quat q = quat::from_axis_angle(normalize(vec3{1.0F, 2.0F, 3.0F}), 0.7F);
    EXPECT_TRUE(is_near(conjugate(q), quat{-q.m_x, -q.m_y, -q.m_z, q.m_w}, 0.0F));
    EXPECT_TRUE(is_near(q * conjugate(q), quat{}, 1e-6F));
    const vec3 v{1.0F, -2.0F, 0.5F};
    EXPECT_TRUE(is_near(rotate(conjugate(q), rotate(q, v)), v, 1e-6F));
}

TEST(QuatTest, NormalizeGivesLengthOne) {
    EXPECT_TRUE(
        is_near(normalize(quat{0.0F, 0.0F, 3.0F, 4.0F}), quat{0.0F, 0.0F, 0.6F, 0.8F}, 1e-6F));
    EXPECT_TRUE(is_unit(normalize(quat{1.0F, 2.0F, 3.0F, 4.0F})));
}

// La tolerancia de is_unit es sobre |q|²: 1,0004² = 1,0008 entra, 1,01² = 1,0201 no.
TEST(QuatTest, IsUnitToleratesRoundingButNotAScale) {
    EXPECT_TRUE(is_unit(quat{}));
    EXPECT_TRUE(is_unit(quat{0.0F, 0.0F, 0.0F, 1.0004F}));
    EXPECT_FALSE(is_unit(quat{0.0F, 0.0F, 0.0F, 1.01F}));
    EXPECT_FALSE(is_unit(quat{0.0F, 0.0F, 0.0F, 0.0F}));
}

// Compara los números, no las rotaciones: q y −q no son casi iguales.
TEST(QuatTest, NearlyEqualComparesEveryComponent) {
    const quat q{0.1F, 0.2F, 0.3F, 0.9F};
    EXPECT_TRUE(nearly_equal(q, quat{0.1F, 0.2F, 0.3F, 0.9F + 1e-7F}, 1e-6F));
    EXPECT_FALSE(nearly_equal(q, quat{0.2F, 0.2F, 0.3F, 0.9F}, 1e-6F));
    EXPECT_FALSE(nearly_equal(q, quat{0.1F, 0.3F, 0.3F, 0.9F}, 1e-6F));
    EXPECT_FALSE(nearly_equal(q, quat{0.1F, 0.2F, 0.4F, 0.9F}, 1e-6F));
    EXPECT_FALSE(nearly_equal(q, quat{0.1F, 0.2F, 0.3F, 0.8F}, 1e-6F));
    EXPECT_FALSE(nearly_equal(q, -q, 1e-6F));
}

// --- Convertir a matriz -------------------------------------------------------------------

// Las columnas de una matriz de rotación son los ejes rotados: un cuarto de vuelta
// alrededor de Z lleva X a Y y Y a −X.
TEST(QuatTest, ToMat3HasTheRotatedAxesAsColumns) {
    const mat3 m = to_mat3(quat::from_axis_angle(z_axis, pi / 2.0F));
    EXPECT_TRUE(is_near(m, mat3{y_axis, -x_axis, z_axis}, 1e-6F));
}

TEST(QuatTest, ToMat4HasNoTranslation) {
    const mat4 m = to_mat4(quat::from_axis_angle(z_axis, pi / 2.0F));
    EXPECT_TRUE(is_near(m,
                        mat4{vec4{0.0F, 1.0F, 0.0F, 0.0F}, vec4{-1.0F, 0.0F, 0.0F, 0.0F},
                             vec4{0.0F, 0.0F, 1.0F, 0.0F}, vec4{0.0F, 0.0F, 0.0F, 1.0F}},
                        1e-6F));
}

// --- Convertir desde una matriz -----------------------------------------------------------

TEST(QuatTest, FromMat3OfTheIdentityIsTheIdentity) {
    EXPECT_TRUE(is_near(quat::from_mat3(mat3{}), quat{}, 0.0F));
}

// Media vuelta alrededor de un eje deja ese eje y da vuelta los otros dos: la traza es
// 1 − 1 − 1 = −1. Es el caso que rompe la fórmula que solo usa la traza, porque w = 0 y
// dividiría por cero.
TEST(QuatTest, FromMat3OfAHalfTurnHasNegativeTrace) {
    EXPECT_TRUE(is_same_rotation(quat::from_mat3(mat3{x_axis, -y_axis, -z_axis}),
                                 quat{1.0F, 0.0F, 0.0F, 0.0F}, 0.0F));
    EXPECT_TRUE(is_same_rotation(quat::from_mat3(mat3{-x_axis, y_axis, -z_axis}),
                                 quat{0.0F, 1.0F, 0.0F, 0.0F}, 0.0F));
    EXPECT_TRUE(is_same_rotation(quat::from_mat3(mat3{-x_axis, -y_axis, z_axis}),
                                 quat{0.0F, 0.0F, 1.0F, 0.0F}, 0.0F));
}

TEST(QuatTest, FromMat4IgnoresTheTranslation) {
    const mat4 m{vec4{0.0F, 1.0F, 0.0F, 0.0F}, vec4{-1.0F, 0.0F, 0.0F, 0.0F},
                 vec4{0.0F, 0.0F, 1.0F, 0.0F}, vec4{5.0F, 6.0F, 7.0F, 1.0F}};
    EXPECT_TRUE(
        is_same_rotation(quat::from_mat4(m), quat::from_axis_angle(z_axis, pi / 2.0F), 1e-6F));
}

// --- Interpolar ---------------------------------------------------------------------------

TEST(QuatTest, SlerpAtTheEndsGivesTheEnds) {
    const quat a = quat::from_axis_angle(y_axis, 0.3F);
    const quat b = quat::from_axis_angle(normalize(vec3{1.0F, 1.0F, 0.0F}), 2.0F);
    EXPECT_TRUE(is_near(slerp(a, b, 0.0F), a, 1e-6F));
    EXPECT_TRUE(is_same_rotation(slerp(a, b, 1.0F), b, 1e-6F));
}

// Velocidad angular constante: un cuarto del camino es un cuarto del ángulo.
TEST(QuatTest, SlerpMovesAtConstantAngularSpeed) {
    const quat b = quat::from_axis_angle(y_axis, pi / 2.0F);
    EXPECT_TRUE(is_near(slerp(quat{}, b, 0.5F), quat::from_axis_angle(y_axis, pi / 4.0F), 1e-6F));
    EXPECT_TRUE(is_near(slerp(quat{}, b, 0.25F), quat::from_axis_angle(y_axis, pi / 8.0F), 1e-6F));
}

// Girar 270° a la izquierda es lo mismo que girar 90° a la derecha. Por el camino corto,
// la mitad es −45°; por el largo sería 135°.
TEST(QuatTest, SlerpTakesTheShortPath) {
    const quat b = quat::from_axis_angle(y_axis, 3.0F * pi / 2.0F);
    ASSERT_LT(dot(quat{}, b), 0.0F);
    EXPECT_TRUE(
        is_same_rotation(slerp(quat{}, b, 0.5F), quat::from_axis_angle(y_axis, -pi / 4.0F), 1e-6F));
}

// Con 0,01 rad entre los dos, sen(θ) es casi 0 y slerp pasa a nlerp, que da lo mismo
// salvo redondeo. También con el signo de b cambiado.
TEST(QuatTest, SlerpOfNearlyEqualRotationsStaysAccurate) {
    const quat b = quat::from_axis_angle(y_axis, 0.01F);
    const quat halfway = quat::from_axis_angle(y_axis, 0.005F);
    EXPECT_TRUE(is_near(slerp(quat{}, b, 0.5F), halfway, 1e-6F));
    EXPECT_TRUE(is_same_rotation(slerp(quat{}, -b, 0.5F), halfway, 1e-6F));
    EXPECT_TRUE(is_unit(slerp(quat{}, b, 0.5F)));
}

TEST(QuatTest, NlerpAtTheEndsGivesTheEnds) {
    const quat a = quat::from_axis_angle(y_axis, 0.3F);
    const quat b = quat::from_axis_angle(normalize(vec3{1.0F, 1.0F, 0.0F}), 2.0F);
    EXPECT_TRUE(is_near(nlerp(a, b, 0.0F), a, 1e-6F));
    EXPECT_TRUE(is_same_rotation(nlerp(a, b, 1.0F), b, 1e-6F));
}

TEST(QuatTest, NlerpTakesTheShortPath) {
    const quat b = quat::from_axis_angle(y_axis, 3.0F * pi / 2.0F);
    EXPECT_TRUE(
        is_same_rotation(nlerp(quat{}, b, 0.5F), quat::from_axis_angle(y_axis, -pi / 4.0F), 1e-6F));
}

// nlerp recorre la cuerda, no el arco: en la mitad coincide con slerp, pero no va a
// velocidad constante. Entre 0° y 90°, con t = 0,25 llega a 21,6° y no a 22,5°. Entre dos
// fotogramas de una animación, a pocos grados, la diferencia no se ve.
TEST(QuatTest, NlerpMatchesSlerpOnlyHalfway) {
    const quat b = quat::from_axis_angle(y_axis, pi / 2.0F);
    EXPECT_TRUE(is_near(nlerp(quat{}, b, 0.5F), slerp(quat{}, b, 0.5F), 1e-6F));
    const float quarter_angle = angle_between(quat{}, nlerp(quat{}, b, 0.25F));
    EXPECT_NEAR(quarter_angle, 21.6F * pi / 180.0F, 0.1F * pi / 180.0F);
}

// --- Euler --------------------------------------------------------------------------------

TEST(QuatTest, FromEulerOfASingleAngleTurnsAroundItsAxis) {
    EXPECT_TRUE(is_near(quat::from_euler(euler_angles{0.4F, 0.0F, 0.0F}),
                        quat::from_axis_angle(y_axis, 0.4F), 1e-6F));
    EXPECT_TRUE(is_near(quat::from_euler(euler_angles{0.0F, 0.4F, 0.0F}),
                        quat::from_axis_angle(x_axis, 0.4F), 1e-6F));
    EXPECT_TRUE(is_near(quat::from_euler(euler_angles{0.0F, 0.0F, 0.4F}),
                        quat::from_axis_angle(z_axis, 0.4F), 1e-6F));
}

// YXZ: primero el roll, después el pitch y al final el yaw. Con yaw 90° y pitch 45°, la
// mirada sube 45° y después gira a la izquierda: queda entre −X y arriba.
TEST(QuatTest, FromEulerAppliesRollThenPitchThenYaw) {
    const quat q = quat::from_euler(euler_angles{0.5F, 0.3F, 0.2F});
    const quat expected = quat::from_axis_angle(y_axis, 0.5F) *
                          quat::from_axis_angle(x_axis, 0.3F) * quat::from_axis_angle(z_axis, 0.2F);
    EXPECT_TRUE(is_near(q, expected, 1e-6F));

    const quat look = quat::from_euler(euler_angles{pi / 2.0F, pi / 4.0F, 0.0F});
    EXPECT_TRUE(is_near(rotate(look, forward), vec3{-sqrt_half, sqrt_half, 0.0F}, 1e-6F));
}

TEST(QuatTest, ToEulerRecoversTheAngles) {
    const euler_angles angles = to_euler(quat::from_euler(euler_angles{0.5F, 0.3F, 0.2F}));
    EXPECT_NEAR(angles.m_yaw, 0.5F, 1e-5F);
    EXPECT_NEAR(angles.m_pitch, 0.3F, 1e-5F);
    EXPECT_NEAR(angles.m_roll, 0.2F, 1e-5F);
}

// Gimbal lock: con el pitch en 90°, el eje del roll (hacia dónde se mira) queda alineado
// con el del yaw (arriba), y los dos giran alrededor del mismo eje. Solo importa yaw − roll
// (o yaw + roll, con el pitch en −90°): se perdió un grado de libertad. to_euler pone el
// roll en 0, y la rotación es la misma.
TEST(QuatTest, ToEulerAtGimbalLockPutsTheWholeTurnInTheYaw) {
    const quat up = quat::from_euler(euler_angles{0.3F, pi / 2.0F, 0.5F});
    const euler_angles looking_up = to_euler(up);
    EXPECT_NEAR(looking_up.m_pitch, pi / 2.0F, 1e-3F);
    EXPECT_NEAR(looking_up.m_yaw, 0.3F - 0.5F, 1e-3F);
    EXPECT_FLOAT_EQ(looking_up.m_roll, 0.0F);
    EXPECT_TRUE(is_same_rotation(quat::from_euler(looking_up), up, 1e-5F));

    const quat down = quat::from_euler(euler_angles{0.3F, -pi / 2.0F, 0.5F});
    const euler_angles looking_down = to_euler(down);
    EXPECT_NEAR(looking_down.m_pitch, -pi / 2.0F, 1e-3F);
    EXPECT_NEAR(looking_down.m_yaw, 0.3F + 0.5F, 1e-3F);
    EXPECT_FLOAT_EQ(looking_down.m_roll, 0.0F);
    EXPECT_TRUE(is_same_rotation(quat::from_euler(looking_down), down, 1e-5F));
}

// --- Errores del programador --------------------------------------------------------------

const quat not_unit{0.0F, 0.0F, 0.0F, 2.0F};

TEST(QuatDeathTest, AxisThatIsNotUnitLengthIsAProgrammerError) {
    EXPECT_DEATH(static_cast<void>(quat::from_axis_angle(vec3{0.0F, 2.0F, 0.0F}, 1.0F)),
                 "eje de largo 1");
}

TEST(QuatDeathTest, NormalizingZeroIsAProgrammerError) {
    EXPECT_DEATH(static_cast<void>(normalize(quat{0.0F, 0.0F, 0.0F, 0.0F})),
                 "cuaternión de largo 0");
}

TEST(QuatDeathTest, RotatingWithANonUnitQuatIsAProgrammerError) {
    EXPECT_DEATH(static_cast<void>(rotate(not_unit, x_axis)), "tiene que medir 1");
    EXPECT_DEATH(static_cast<void>(to_mat3(not_unit)), "tiene que medir 1");
}

TEST(QuatDeathTest, InterpolatingNonUnitQuatsIsAProgrammerError) {
    EXPECT_DEATH(static_cast<void>(slerp(not_unit, quat{}, 0.5F)), "tiene que medir 1");
    EXPECT_DEATH(static_cast<void>(slerp(quat{}, not_unit, 0.5F)), "tiene que medir 1");
    EXPECT_DEATH(static_cast<void>(nlerp(not_unit, quat{}, 0.5F)), "tiene que medir 1");
    EXPECT_DEATH(static_cast<void>(nlerp(quat{}, not_unit, 0.5F)), "tiene que medir 1");
}

// Una escala, o un espejo (determinante −1), no son rotaciones: no tienen cuaternión.
TEST(QuatDeathTest, FromMat3OfANonRotationIsAProgrammerError) {
    EXPECT_DEATH(
        static_cast<void>(quat::from_mat3(mat3{x_axis * 2.0F, y_axis * 2.0F, z_axis * 2.0F})),
        "matriz de rotación");
    EXPECT_DEATH(static_cast<void>(quat::from_mat3(mat3{-x_axis, y_axis, z_axis})),
                 "matriz de rotación");
}

// --- Propiedades --------------------------------------------------------------------------

vec3 random_vector(mz::test::property_rng &rng, float extent) {
    return {rng.next_float(-extent, extent), rng.next_float(-extent, extent),
            rng.next_float(-extent, extent)};
}

// Uniforme en la esfera: un punto al azar dentro de la bola de radio 1, llevado a la
// superficie. Se descartan los de fuera (si no, las esquinas del cubo pesarían más) y los
// muy cercanos al centro, donde la dirección es puro redondeo.
vec3 random_unit_vector(mz::test::property_rng &rng) {
    for (;;) {
        const vec3 v = random_vector(rng, 1.0F);
        const float length_sq = length_squared(v);
        if (length_sq > 0.01F && length_sq <= 1.0F) {
            return v / std::sqrt(length_sq);
        }
    }
}

quat random_rotation(mz::test::property_rng &rng) {
    return quat::from_axis_angle(random_unit_vector(rng), rng.next_float(-pi, pi));
}

// Criterio del ticket: el cuaternión y su matriz rotan el mismo punto.
TEST(QuatProperty, RotatingWithTheQuatOrItsMatrixGivesTheSamePoint) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const quat q = random_rotation(rng);
        const vec3 v = random_vector(rng, 10.0F);
        EXPECT_TRUE(is_near(rotate(q, v), to_mat3(q) * v, 1e-5F));
        EXPECT_TRUE(is_near(rotate(q, v), transform_point(to_mat4(q), v), 1e-5F));
    }
}

// Criterio del ticket: q y −q son la misma rotación.
TEST(QuatProperty, QAndMinusQRotateTheSame) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const quat q = random_rotation(rng);
        const vec3 v = random_vector(rng, 10.0F);
        EXPECT_TRUE(is_near(rotate(-q, v), rotate(q, v), 1e-5F));
        EXPECT_TRUE(is_near(to_mat3(-q), to_mat3(q), 0.0F));
    }
}

// El producto de cuaterniones es el de sus matrices, en el mismo orden. Y |a · b| =
// |a| · |b|: el producto de dos rotaciones sigue midiendo 1.
TEST(QuatProperty, ProductMatchesTheProductOfTheMatrices) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const quat a = random_rotation(rng);
        const quat b = random_rotation(rng);
        // ASSERT y no EXPECT: sin largo 1, to_mat3 abortaría el ejecutable entero.
        ASSERT_TRUE(is_unit(a * b));
        EXPECT_TRUE(is_near(to_mat3(a * b), to_mat3(a) * to_mat3(b), 1e-5F));
    }
}

// Una rotación es rígida: no cambia largos.
TEST(QuatProperty, RotationKeepsLengths) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const quat q = random_rotation(rng);
        const vec3 v = random_vector(rng, 10.0F);
        EXPECT_TRUE(mz::core::nearly_equal(length(rotate(q, v)), length(v), 1e-5F));
    }
}

// Criterio del ticket: matriz → quat → matriz es estable también con traza negativa. Un
// tercio de las rotaciones son de cualquier ángulo, un tercio están a menos de 0,001 rad
// de media vuelta y un tercio son media vuelta exacta, alrededor de un eje al azar. Ahí
// la traza es −1 y w es 0: la fórmula que solo usa la traza calcula √(1 + traza), que por
// redondeo da 0 o NaN, y este test la rechaza.
TEST(QuatProperty, MatrixToQuatToMatrixIsStable) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1500; ++i) {
        const float angle = (i % 3 == 0)   ? rng.next_float(0.0F, pi)
                            : (i % 3 == 1) ? rng.next_float(pi - 0.001F, pi)
                                           : pi;
        const quat original = quat::from_axis_angle(random_unit_vector(rng), angle);
        const mat3 m = to_mat3(original);
        const quat q = quat::from_mat3(m);
        // ASSERT y no EXPECT: con q sin largo 1, to_mat3 abortaría el ejecutable entero.
        ASSERT_TRUE(is_unit(q)) << "ángulo " << angle;
        EXPECT_TRUE(is_near(to_mat3(q), m, 1e-5F)) << "ángulo " << angle;
        EXPECT_TRUE(is_same_rotation(q, original, 1e-5F)) << "ángulo " << angle;
    }
}

// Criterio del ticket: slerp da los extremos, va a velocidad constante y por el camino
// corto. Por el largo, el ángulo recorrido con t sería t · (2π − θ) en vez de t · θ.
TEST(QuatProperty, SlerpGoesAtConstantSpeedByTheShortPath) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const quat a = random_rotation(rng);
        const quat b = random_rotation(rng);
        const float t = rng.next_float(0.0F, 1.0F);
        EXPECT_TRUE(is_near(slerp(a, b, 0.0F), a, 1e-6F));
        EXPECT_TRUE(is_same_rotation(slerp(a, b, 1.0F), b, 1e-5F));
        // Los pesos sen(...)/sen(Ω) dejan el resultado sobre la esfera. Hay que pedirlo
        // aparte: angle_between recorta el dot a 1, y un resultado más largo pasaría.
        EXPECT_TRUE(is_unit(slerp(a, b, t)));
        // acos pierde precisión cerca de 1, por eso la tolerancia de 2e-3 rad.
        EXPECT_NEAR(angle_between(a, slerp(a, b, t)), t * angle_between(a, b), 2e-3F);
    }
}

TEST(QuatProperty, NlerpTakesTheShortPath) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const quat a = random_rotation(rng);
        const quat b = random_rotation(rng);
        EXPECT_TRUE(is_near(nlerp(a, b, 0.0F), a, 1e-6F));
        EXPECT_TRUE(is_same_rotation(nlerp(a, b, 1.0F), b, 1e-5F));
        EXPECT_NEAR(angle_between(a, nlerp(a, b, 0.5F)), 0.5F * angle_between(a, b), 2e-3F);
    }
}

// Fuera del gimbal lock, los ángulos de Euler van y vuelven.
TEST(QuatProperty, EulerAnglesRoundTrip) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const euler_angles angles{rng.next_float(-3.0F, 3.0F), rng.next_float(-1.5F, 1.5F),
                                  rng.next_float(-3.0F, 3.0F)};
        const euler_angles back = to_euler(quat::from_euler(angles));
        EXPECT_NEAR(back.m_yaw, angles.m_yaw, 1e-4F);
        EXPECT_NEAR(back.m_pitch, angles.m_pitch, 1e-4F);
        EXPECT_NEAR(back.m_roll, angles.m_roll, 1e-4F);
    }
}

} // namespace
