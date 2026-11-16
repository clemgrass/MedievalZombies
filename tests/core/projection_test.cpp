#include "mz/core/projection.h"

#include "math_assertions.h"

#include "mz/core/axes.h"
#include "mz/core/mat.h"
#include "mz/core/scalar.h"
#include "mz/core/vec.h"
#include "mz/test/property.h"

#include <gtest/gtest.h>

#include <cmath>
#include <numbers>

namespace {

using mz::core::look_at;
using mz::core::mat3;
using mz::core::mat4;
using mz::core::orthographic_reverse_z;
using mz::core::perspective_infinite_reverse_z;
using mz::core::vec3;
using mz::core::vec4;
using mz::core::world_forward;
using mz::core::world_right;
using mz::core::world_up;
using mz::test::is_near;

// Los valores de la cámara en tercera persona: 51° vertical, 16:9 y un plano cercano de
// 10 cm.
constexpr float fov_y = 51.0F * std::numbers::pi_v<float> / 180.0F;
constexpr float aspect = 16.0F / 9.0F;
constexpr float near_plane = 0.1F;

vec4 to_clip(const mat4 &projection, vec3 view_point) {
    return projection * vec4{view_point.m_x, view_point.m_y, view_point.m_z, 1.0F};
}

// La división por w: de coordenadas de recorte a NDC.
vec3 to_ndc(vec4 clip) { return vec3{clip.m_x, clip.m_y, clip.m_z} / clip.m_w; }

// El volumen de recorte de Vulkan: −w ≤ x ≤ w, −w ≤ y ≤ w y 0 ≤ z ≤ w. La GPU recorta
// antes de dividir, así que un punto con w ≤ 0 (detrás de la cámara) queda fuera.
bool inside_clip_volume(vec4 clip) {
    return clip.m_w > 0.0F && std::fabs(clip.m_x) <= clip.m_w && std::fabs(clip.m_y) <= clip.m_w &&
           clip.m_z >= 0.0F && clip.m_z <= clip.m_w;
}

// --- Ejes -------------------------------------------------------------------------------

// Mano derecha: x × y = z. La cámara mira hacia −Z, así que derecha × arriba es "atrás".
TEST(AxesTest, AreRightHandedWithForwardAlongMinusZ) {
    EXPECT_TRUE(is_near(world_right, vec3{1.0F, 0.0F, 0.0F}, 0.0F));
    EXPECT_TRUE(is_near(world_up, vec3{0.0F, 1.0F, 0.0F}, 0.0F));
    EXPECT_TRUE(is_near(world_forward, vec3{0.0F, 0.0F, -1.0F}, 0.0F));
    EXPECT_TRUE(is_near(cross(world_right, world_up), -world_forward, 0.0F));
}

// --- look_at ----------------------------------------------------------------------------

// Una cámara en el origen que mira hacia adelante, con Y arriba, ya está en el espacio de
// la vista: su matriz es la identidad.
TEST(LookAtTest, CameraAtOriginLookingForwardIsTheIdentity) {
    EXPECT_TRUE(is_near(look_at(vec3{}, world_forward, world_up), mat4{}, 0.0F));
}

TEST(LookAtTest, MovesTheEyeToTheOrigin) {
    const vec3 eye{3.0F, 1.9F, -7.0F};
    const mat4 view = look_at(eye, vec3{0.0F, 1.0F, 0.0F}, world_up);
    EXPECT_TRUE(is_near(transform_point(view, eye), vec3{}, 1e-6F));
}

// El punto mirado queda delante de la cámara, sobre el eje −Z, a su distancia.
TEST(LookAtTest, PutsTheTargetOnMinusZ) {
    const vec3 eye{3.0F, 1.9F, -7.0F};
    const vec3 target{0.0F, 1.0F, 0.0F};
    const mat4 view = look_at(eye, target, world_up);
    EXPECT_TRUE(
        is_near(transform_point(view, target), vec3{0.0F, 0.0F, -length(target - eye)}, 1e-6F));
}

// Calculado a mano: mirando hacia +X, la derecha es forward × up = x × y = +Z. Un punto en
// +Z del mundo queda a la derecha (+X de la vista), y uno en +Y, arriba.
TEST(LookAtTest, LookingAlongPlusXPutsPlusZOnTheRight) {
    const mat4 view = look_at(vec3{}, vec3{1.0F, 0.0F, 0.0F}, world_up);
    EXPECT_TRUE(
        is_near(transform_point(view, vec3{0.0F, 0.0F, 1.0F}), vec3{1.0F, 0.0F, 0.0F}, 1e-6F));
    EXPECT_TRUE(
        is_near(transform_point(view, vec3{0.0F, 1.0F, 0.0F}), vec3{0.0F, 1.0F, 0.0F}, 1e-6F));
    EXPECT_TRUE(
        is_near(transform_point(view, vec3{2.0F, 0.0F, 0.0F}), vec3{0.0F, 0.0F, -2.0F}, 1e-6F));
}

// `up` no tiene que ser perpendicular a la dirección ni medir 1: solo dice de qué lado
// queda el cielo. Mirar hacia abajo en diagonal da la misma vista con cualquier up hacia
// arriba.
TEST(LookAtTest, UpOnlyNeedsToPointUpward) {
    const vec3 eye{0.0F, 2.0F, 2.0F};
    const mat4 with_world_up = look_at(eye, vec3{}, world_up);
    const mat4 with_tilted_up = look_at(eye, vec3{}, vec3{0.0F, 5.0F, -3.0F});
    EXPECT_TRUE(is_near(with_tilted_up, with_world_up, 1e-6F));
}

TEST(LookAtDeathTest, EyeOnTheTargetIsAProgrammerError) {
    const vec3 eye{1.0F, 2.0F, 3.0F};
    EXPECT_DEATH(static_cast<void>(look_at(eye, eye, world_up)), "mira el punto donde está");
}

// Mirando justo hacia abajo, "arriba" no dice dónde queda la derecha: la cámara del juego
// limita el cabeceo antes de llegar ahí.
TEST(LookAtDeathTest, LookingAlongUpIsAProgrammerError) {
    EXPECT_DEATH(static_cast<void>(look_at(vec3{0.0F, 5.0F, 0.0F}, vec3{}, world_up)),
                 "paralela a up");
}

// La vista es un movimiento rígido: rota y traslada, pero no estira ni espeja. Su parte
// 3×3 es una rotación (columnas de largo 1 y perpendiculares, determinante +1).
TEST(LookAtProperty, IsARigidMotion) {
    mz::test::property_rng rng;
    for (int i = 0; i < 1000; ++i) {
        const vec3 eye{rng.next_float(-50.0F, 50.0F), rng.next_float(-50.0F, 50.0F),
                       rng.next_float(-50.0F, 50.0F)};
        // La dirección, como mucho a 60° del horizonte: lejos de ser paralela a up.
        const vec3 direction{rng.next_float(-1.0F, 1.0F), rng.next_float(-0.8F, 0.8F),
                             rng.next_float(-1.0F, 1.0F)};
        if (length_squared(vec3{direction.m_x, 0.0F, direction.m_z}) < 0.25F) {
            continue;
        }
        const mat4 view = look_at(eye, eye + direction, world_up);
        const mat3 rotation{vec3{view[0].m_x, view[0].m_y, view[0].m_z},
                            vec3{view[1].m_x, view[1].m_y, view[1].m_z},
                            vec3{view[2].m_x, view[2].m_y, view[2].m_z}};
        EXPECT_TRUE(is_near(transpose(rotation) * rotation, mat3{}, 1e-5F));
        EXPECT_TRUE(mz::core::nearly_equal(determinant(rotation), 1.0F, 1e-5F));
        EXPECT_TRUE(is_near(view[3], vec4{view[3].m_x, view[3].m_y, view[3].m_z, 1.0F}, 0.0F));
    }
}

// --- Perspectiva ------------------------------------------------------------------------

// La matriz entera, columna por columna, con f = 1 / tan(fov_y / 2):
//
//     x' = f / aspect · x
//     y' = −f · y          (la Y se voltea: en Vulkan, la Y de la pantalla va hacia abajo)
//     z' = near            (la profundidad, después de dividir, es near / −z)
//     w' = −z              (la cámara mira hacia −Z: lo que está delante tiene w > 0)
TEST(PerspectiveTest, MatchesTheHandWrittenMatrix) {
    const float f = 1.0F / std::tan(fov_y / 2.0F);
    const mat4 expected{vec4{f / aspect, 0.0F, 0.0F, 0.0F}, vec4{0.0F, -f, 0.0F, 0.0F},
                        vec4{0.0F, 0.0F, 0.0F, -1.0F}, vec4{0.0F, 0.0F, near_plane, 0.0F}};
    EXPECT_TRUE(
        is_near(perspective_infinite_reverse_z(fov_y, aspect, near_plane), expected, 1e-6F));
}

// Reverse-Z: el plano cercano es la profundidad 1.
TEST(PerspectiveTest, NearPlaneHasDepthOne) {
    const mat4 projection = perspective_infinite_reverse_z(fov_y, aspect, near_plane);
    EXPECT_FLOAT_EQ(to_ndc(to_clip(projection, vec3{0.0F, 0.0F, -near_plane})).m_z, 1.0F);
}

// Y el infinito, la profundidad 0: baja como near / distancia, sin llegar nunca.
TEST(PerspectiveTest, DepthDecreasesTowardZeroAtInfinity) {
    const mat4 projection = perspective_infinite_reverse_z(fov_y, aspect, near_plane);
    const float at_10_m = to_ndc(to_clip(projection, vec3{0.0F, 0.0F, -10.0F})).m_z;
    const float at_1_km = to_ndc(to_clip(projection, vec3{0.0F, 0.0F, -1000.0F})).m_z;
    const float at_1e30 = to_ndc(to_clip(projection, vec3{0.0F, 0.0F, -1e30F})).m_z;
    EXPECT_FLOAT_EQ(at_10_m, near_plane / 10.0F);
    EXPECT_FLOAT_EQ(at_1_km, near_plane / 1000.0F);
    EXPECT_GT(at_1_km, at_1e30);
    EXPECT_GT(at_1e30, 0.0F);
}

// El borde de arriba del campo de visión queda en y = −1 (arriba en Vulkan) y el borde
// derecho en x = +1: la Y se voltea, la X no.
TEST(PerspectiveTest, EdgesOfTheFieldOfViewAreEdgesOfTheScreen) {
    const mat4 projection = perspective_infinite_reverse_z(fov_y, aspect, near_plane);
    const float distance = 5.0F;
    const float half_height = distance * std::tan(fov_y / 2.0F);
    const vec3 top = to_ndc(to_clip(projection, vec3{0.0F, half_height, -distance}));
    const vec3 right = to_ndc(to_clip(projection, vec3{half_height * aspect, 0.0F, -distance}));
    EXPECT_NEAR(top.m_y, -1.0F, 1e-6F);
    EXPECT_NEAR(right.m_x, 1.0F, 1e-6F);
}

TEST(PerspectiveTest, PointInFrontOfTheCameraIsInsideTheClipVolume) {
    const mat4 projection = perspective_infinite_reverse_z(fov_y, aspect, near_plane);
    EXPECT_TRUE(inside_clip_volume(to_clip(projection, vec3{0.3F, -0.2F, -4.0F})));
    EXPECT_TRUE(inside_clip_volume(to_clip(projection, vec3{0.0F, 0.0F, -near_plane})));
    EXPECT_TRUE(inside_clip_volume(to_clip(projection, vec3{0.0F, 0.0F, -1e30F})));
}

TEST(PerspectiveTest, PointsBehindOrBeforeTheNearPlaneAreClipped) {
    const mat4 projection = perspective_infinite_reverse_z(fov_y, aspect, near_plane);
    EXPECT_FALSE(inside_clip_volume(to_clip(projection, vec3{0.0F, 0.0F, 4.0F})));
    EXPECT_FALSE(inside_clip_volume(to_clip(projection, vec3{0.0F, 0.0F, -0.05F})));
}

// El criterio del ticket, con la cámara de tercera persona de verdad: un punto más alto
// en el mundo queda más arriba en la pantalla, que en Vulkan es una y de NDC menor.
TEST(PerspectiveTest, PointHigherInTheWorldIsHigherOnTheScreen) {
    const mat4 view = look_at(vec3{0.0F, 1.9F, 2.3F}, vec3{0.0F, 1.0F, 0.0F}, world_up);
    const mat4 view_projection = perspective_infinite_reverse_z(fov_y, aspect, near_plane) * view;
    const vec4 head = view_projection * vec4{0.0F, 1.7F, 0.0F, 1.0F};
    const vec4 feet = view_projection * vec4{0.0F, 0.0F, 0.0F, 1.0F};
    ASSERT_TRUE(inside_clip_volume(head));
    ASSERT_TRUE(inside_clip_volume(feet));
    EXPECT_LT(to_ndc(head).m_y, to_ndc(feet).m_y);
}

TEST(PerspectiveDeathTest, FieldOfViewOutsideZeroToPiIsAProgrammerError) {
    EXPECT_DEATH(static_cast<void>(perspective_infinite_reverse_z(0.0F, aspect, near_plane)),
                 "campo de visión");
    EXPECT_DEATH(static_cast<void>(
                     perspective_infinite_reverse_z(std::numbers::pi_v<float>, aspect, near_plane)),
                 "campo de visión");
}

TEST(PerspectiveDeathTest, NonPositiveAspectIsAProgrammerError) {
    EXPECT_DEATH(static_cast<void>(perspective_infinite_reverse_z(fov_y, 0.0F, near_plane)),
                 "aspecto");
}

TEST(PerspectiveDeathTest, NonPositiveNearPlaneIsAProgrammerError) {
    EXPECT_DEATH(static_cast<void>(perspective_infinite_reverse_z(fov_y, aspect, 0.0F)),
                 "plano cercano");
}

// --- Ortográfica ------------------------------------------------------------------------

// Una caja de 4 × 2 m, de 0,5 a 10 m delante de la cámara. No está centrada en el eje de
// la cámara: con una caja simétrica, la traslación de x y de y valdría 0, y un signo mal
// puesto en ella pasaría los tests.
constexpr float box_left = -1.0F;
constexpr float box_right = 3.0F;
constexpr float box_bottom = 0.5F;
constexpr float box_top = 2.5F;
constexpr float box_near = 0.5F;
constexpr float box_far = 10.0F;

mat4 box_projection() {
    return orthographic_reverse_z(box_left, box_right, box_bottom, box_top, box_near, box_far);
}

// También reverse-Z, como la perspectiva: cerca 1 y lejos 0. La profundidad es lineal, y
// w queda en 1.
TEST(OrthographicTest, NearIsOneAndFarIsZero) {
    const vec4 near_point = to_clip(box_projection(), vec3{0.0F, 0.0F, -box_near});
    const vec4 far_point = to_clip(box_projection(), vec3{0.0F, 0.0F, -box_far});
    const vec4 middle = to_clip(box_projection(), vec3{0.0F, 0.0F, -5.25F});
    EXPECT_FLOAT_EQ(near_point.m_w, 1.0F);
    EXPECT_FLOAT_EQ(near_point.m_z, 1.0F);
    EXPECT_NEAR(far_point.m_z, 0.0F, 1e-6F);
    EXPECT_FLOAT_EQ(middle.m_z, 0.5F);
}

// La esquina de arriba a la izquierda de la caja es (−1, −1) en NDC, y la de abajo a la
// derecha, (1, 1): la Y se voltea igual que en la perspectiva.
TEST(OrthographicTest, CornersOfTheBoxAreCornersOfTheScreen) {
    const vec4 top_left = to_clip(box_projection(), vec3{box_left, box_top, -box_near});
    const vec4 bottom_right = to_clip(box_projection(), vec3{box_right, box_bottom, -box_far});
    EXPECT_TRUE(is_near(vec3{top_left.m_x, top_left.m_y, 0.0F}, vec3{-1.0F, -1.0F, 0.0F}, 1e-6F));
    EXPECT_TRUE(
        is_near(vec3{bottom_right.m_x, bottom_right.m_y, 0.0F}, vec3{1.0F, 1.0F, 0.0F}, 1e-6F));
}

TEST(OrthographicTest, PointInsideTheBoxIsInsideTheClipVolume) {
    EXPECT_TRUE(inside_clip_volume(to_clip(box_projection(), vec3{2.5F, 1.0F, -3.0F})));
    // El mismo x e y, más lejos que el plano lejano y detrás del cercano.
    EXPECT_FALSE(inside_clip_volume(to_clip(box_projection(), vec3{2.5F, 1.0F, -11.0F})));
    EXPECT_FALSE(inside_clip_volume(to_clip(box_projection(), vec3{2.5F, 1.0F, 0.0F})));
}

TEST(OrthographicDeathTest, EmptyBoxIsAProgrammerError) {
    EXPECT_DEATH(static_cast<void>(orthographic_reverse_z(1.0F, 1.0F, -1.0F, 1.0F, 0.5F, 10.0F)),
                 "caja vacía");
    EXPECT_DEATH(static_cast<void>(orthographic_reverse_z(-1.0F, 1.0F, 1.0F, -1.0F, 0.5F, 10.0F)),
                 "caja vacía");
    EXPECT_DEATH(static_cast<void>(orthographic_reverse_z(-1.0F, 1.0F, -1.0F, 1.0F, 10.0F, 0.5F)),
                 "caja vacía");
}

} // namespace
