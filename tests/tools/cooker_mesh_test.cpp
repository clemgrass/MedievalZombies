#include "mz/cooker/error.h"
#include "mz/cooker/mesh.h"
#include "mz/core/asset_format.h"
#include "mz/core/mesh_format.h"
#include "mz/core/scalar.h"
#include "mz/core/vec.h"
#include "test_files.h"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using mz::cooker::cook_error_kind;
using mz::cooker::cook_mesh;
using mz::cooker::gltf_to_engine;
using mz::cooker::mesh_asset;
using mz::cooker::mesh_data;
using mz::cooker::read_gltf_mesh;
using mz::core::asset_kind;
using mz::core::asset_view;
using mz::core::mesh_submesh;
using mz::core::mesh_vertex;
using mz::core::read_asset;
using mz::core::section_kind;
using mz::core::vec2;
using mz::core::vec3;
using mz::test::built_fixture_path;
using mz::test::fixture_path;
using mz::test::output_path;
using mz::test::read_file;

// Los valores de los tests salen de cambiar signos o de copiar floats: son exactos.
void expect_exactly(vec3 actual, vec3 expected) {
    EXPECT_TRUE(mz::core::exactly_equal(actual.m_x, expected.m_x)) << actual.m_x;
    EXPECT_TRUE(mz::core::exactly_equal(actual.m_y, expected.m_y)) << actual.m_y;
    EXPECT_TRUE(mz::core::exactly_equal(actual.m_z, expected.m_z)) << actual.m_z;
}

void expect_exactly(vec2 actual, vec2 expected) {
    EXPECT_TRUE(mz::core::exactly_equal(actual.m_x, expected.m_x)) << actual.m_x;
    EXPECT_TRUE(mz::core::exactly_equal(actual.m_y, expected.m_y)) << actual.m_y;
}

// Las submallas como pares (primer índice, cantidad), que gtest sabe comparar e imprimir.
using index_range = std::pair<std::uint32_t, std::uint32_t>;
std::vector<index_range> ranges(const std::vector<mesh_submesh> &submeshes) {
    std::vector<index_range> out;
    out.reserve(submeshes.size());
    for (const mesh_submesh &s : submeshes) {
        out.emplace_back(s.m_first_index, s.m_index_count);
    }
    return out;
}

mesh_data read_valid(std::string_view name) {
    auto mesh = read_gltf_mesh(fixture_path(name));
    EXPECT_TRUE(mesh.has_value()) << name;
    return mesh.has_value() ? std::move(mesh).value() : mesh_data{};
}

// Un cubo de 2 m centrado en el origen, ya en los ejes del engine: 6 caras de 4 vértices y 12
// triángulos, con las normales hacia afuera y los triángulos en orden antihorario vistos desde
// afuera. Un espejo en el cambio de ejes daría vuelta el orden y lo haría fallar.
void expect_closed_cube(std::span<const mesh_vertex> vertices,
                        std::span<const std::uint32_t> indices) {
    // 4 vértices por cara: cada esquina se repite en sus 3 caras, una vez por cada normal.
    EXPECT_EQ(vertices.size(), 24U);
    std::set<std::tuple<float, float, float>> corners;
    for (const mesh_vertex &v : vertices) {
        corners.emplace(v.m_position.m_x, v.m_position.m_y, v.m_position.m_z);
    }
    EXPECT_EQ(corners.size(), 8U);

    ASSERT_EQ(indices.size(), 36U); // 12 triángulos
    for (const std::uint32_t index : indices) {
        ASSERT_LT(index, vertices.size());
    }
    for (std::size_t t = 0; t < indices.size(); t += 3) {
        SCOPED_TRACE(t / 3);
        const mesh_vertex &a = vertices[indices[t]];
        const mesh_vertex &b = vertices[indices[t + 1]];
        const mesh_vertex &c = vertices[indices[t + 2]];
        const vec3 center = (a.m_position + b.m_position + c.m_position) / 3.0F;
        const vec3 face = mz::core::cross(b.m_position - a.m_position, c.m_position - a.m_position);
        for (const mesh_vertex *v : {&a, &b, &c}) {
            // La normal apunta hacia afuera del cubo, que está centrado en el origen.
            EXPECT_GT(mz::core::dot(v->m_normal, center), 0.0F);
            // Y el triángulo está en orden antihorario visto desde afuera.
            EXPECT_GT(mz::core::dot(v->m_normal, face), 0.0F);
        }
    }
}

// --- El cambio de ejes ---------------------------------------------------------------------

// glTF y el engine comparten mano derecha e Y arriba, pero un modelo de glTF mira hacia +Z y
// uno del engine hacia −Z: el cocinador gira 180° sobre Y. Las tres componentes distintas de
// cero muestran el signo de cada una: un espejo en Z daría (1, 2, −3).
TEST(GltfToEngineTest, TurnsHalfAroundY) {
    expect_exactly(gltf_to_engine({1.0F, 2.0F, 3.0F}), {-1.0F, 2.0F, -3.0F});
}

TEST(GltfToEngineTest, AModelFacingPlusZEndsFacingMinusZ) {
    expect_exactly(gltf_to_engine({0.0F, 0.0F, 1.0F}), {0.0F, 0.0F, -1.0F});
}

// Lo que distingue un giro de un espejo: la normal de un triángulo girado es la normal girada.
// Un espejo la daría vuelta (determinante −1), y el triángulo quedaría mirando hacia adentro.
// Girar dos veces y volver al original no lo prueba: un espejo también vuelve.
TEST(GltfToEngineTest, TheNormalOfATurnedTriangleIsTheTurnedNormal) {
    const vec3 a{1.0F, 2.0F, 3.0F};
    const vec3 b{3.0F, 2.0F, 3.0F};
    const vec3 c{1.0F, 4.0F, 3.0F};
    const vec3 turned = mz::core::cross(gltf_to_engine(b) - gltf_to_engine(a),
                                        gltf_to_engine(c) - gltf_to_engine(a));
    expect_exactly(turned, gltf_to_engine(mz::core::cross(b - a, c - a)));
    expect_exactly(turned, {0.0F, 0.0F, -4.0F});
}

// --- Mallas válidas ------------------------------------------------------------------------

// El triángulo de make_gltf.py: posiciones (1, 2, 3), (3, 2, 3), (1, 4, 3) y normal +Z.
TEST(ReadGltfMeshTest, ATriangleInTheEngineAxes) {
    const mesh_data mesh = read_valid("triangulo.gltf");

    ASSERT_EQ(mesh.m_vertices.size(), 3U);
    expect_exactly(mesh.m_vertices[0].m_position, {-1.0F, 2.0F, -3.0F});
    expect_exactly(mesh.m_vertices[1].m_position, {-3.0F, 2.0F, -3.0F});
    expect_exactly(mesh.m_vertices[2].m_position, {-1.0F, 4.0F, -3.0F});
    for (const mesh_vertex &v : mesh.m_vertices) {
        expect_exactly(v.m_normal, {0.0F, 0.0F, -1.0F});
    }
    // Las UV no cambian: glTF y Vulkan ponen el (0, 0) arriba a la izquierda.
    expect_exactly(mesh.m_vertices[0].m_uv, {0.0F, 0.0F});
    expect_exactly(mesh.m_vertices[1].m_uv, {1.0F, 0.0F});
    expect_exactly(mesh.m_vertices[2].m_uv, {0.0F, 1.0F});

    EXPECT_EQ(mesh.m_indices, (std::vector<std::uint32_t>{0, 1, 2}));
    EXPECT_EQ(ranges(mesh.m_submeshes), (std::vector<index_range>{{0, 3}}));
}

// Los índices de la segunda primitiva se corren por los vértices de la primera: los índices
// apuntan a la lista completa, y cada submalla se dibuja sin un vértice base.
TEST(ReadGltfMeshTest, TwoPrimitivesShareOneBuffer) {
    const mesh_data mesh = read_valid("dos_primitivas.gltf");

    ASSERT_EQ(mesh.m_vertices.size(), 6U);
    expect_exactly(mesh.m_vertices[3].m_position, {0.0F, 0.0F, 0.0F});
    expect_exactly(mesh.m_vertices[4].m_position, {0.0F, 0.0F, -1.0F});
    expect_exactly(mesh.m_vertices[5].m_position, {-1.0F, 0.0F, 0.0F});
    expect_exactly(mesh.m_vertices[3].m_normal, {0.0F, 1.0F, 0.0F});

    EXPECT_EQ(mesh.m_indices, (std::vector<std::uint32_t>{0, 1, 2, 3, 4, 5}));
    EXPECT_EQ(ranges(mesh.m_submeshes), (std::vector<index_range>{{0, 3}, {3, 3}}));
}

// El cubo de make_gltf.py, escrito a mano: la misma forma que el de Blender, sin necesitarlo.
TEST(ReadGltfMeshTest, AHandWrittenCube) {
    const mesh_data mesh = read_valid("cubo.gltf");

    expect_closed_cube(mesh.m_vertices, mesh.m_indices);
    EXPECT_EQ(ranges(mesh.m_submeshes), (std::vector<index_range>{{0, 36}}));
}

// --- Mallas inválidas ----------------------------------------------------------------------

struct invalid_case {
    std::string_view m_file;
    cook_error_kind m_kind;
    // El nodo que nombra el mensaje; vacío si el error es del archivo entero.
    std::string_view m_object;
};

// Uno por motivo (make_gltf.py). El de los atributos se prueba con un atributo que falta, uno
// con otra forma y uno con otro tipo de componente: los tres pasan por la misma revisión.
constexpr std::array invalid_cases{
    invalid_case{
        .m_file = "no_existe.gltf", .m_kind = cook_error_kind::cannot_read, .m_object = ""},
    invalid_case{
        .m_file = "buffer_perdido.gltf", .m_kind = cook_error_kind::cannot_read, .m_object = ""},
    invalid_case{
        .m_file = "indice_fuera.gltf", .m_kind = cook_error_kind::invalid_gltf, .m_object = ""},
    invalid_case{.m_file = "sin_malla.gltf", .m_kind = cook_error_kind::no_mesh, .m_object = ""},
    invalid_case{
        .m_file = "dos_mallas.gltf", .m_kind = cook_error_kind::several_meshes, .m_object = ""},
    invalid_case{.m_file = "con_esqueleto.gltf",
                 .m_kind = cook_error_kind::has_skin,
                 .m_object = "ConEsqueleto"},
    invalid_case{.m_file = "trasladado.gltf",
                 .m_kind = cook_error_kind::not_identity,
                 .m_object = "Trasladado"},
    invalid_case{.m_file = "malla_vacia.gltf",
                 .m_kind = cook_error_kind::no_primitives,
                 .m_object = "MallaVacia"},
    invalid_case{
        .m_file = "lineas.gltf", .m_kind = cook_error_kind::not_triangles, .m_object = "nodo 0"},
    invalid_case{.m_file = "sin_indices.gltf",
                 .m_kind = cook_error_kind::no_indices,
                 .m_object = "SinIndices"},
    invalid_case{.m_file = "indices_sueltos.gltf",
                 .m_kind = cook_error_kind::loose_indices,
                 .m_object = "IndicesSueltos"},
    invalid_case{.m_file = "sin_normales.gltf",
                 .m_kind = cook_error_kind::missing_normal,
                 .m_object = "SinNormales"},
    invalid_case{.m_file = "posiciones_vec2.gltf",
                 .m_kind = cook_error_kind::bad_position_type,
                 .m_object = "PosicionesVec2"},
    invalid_case{.m_file = "uv_enteros.gltf",
                 .m_kind = cook_error_kind::bad_uv_type,
                 .m_object = "UvEnteros"},
    invalid_case{.m_file = "extension_requerida.gltf",
                 .m_kind = cook_error_kind::unsupported_extension,
                 .m_object = ""},
    // Sin estas revisiones, los dos se cocinarían con ceros (los índices, o las posiciones).
    invalid_case{.m_file = "buffer_sin_datos.gltf",
                 .m_kind = cook_error_kind::unreadable_accessor,
                 .m_object = "BufferSinDatos"},
    invalid_case{.m_file = "indices_sin_vista.gltf",
                 .m_kind = cook_error_kind::unreadable_accessor,
                 .m_object = "IndicesSinVista"},
};

TEST(ReadGltfMeshTest, AnInvalidMeshSaysWhyAndWhere) {
    for (const invalid_case &c : invalid_cases) {
        SCOPED_TRACE(c.m_file);
        const auto mesh = read_gltf_mesh(fixture_path(c.m_file));
        ASSERT_FALSE(mesh.has_value());
        EXPECT_EQ(mesh.error().m_kind, c.m_kind) << describe(mesh.error().m_kind);
        EXPECT_EQ(mesh.error().m_object, c.m_object);
    }
}

// Un asset inválido no deja nada en la salida: ni un binario a medias ni uno viejo nuevo.
TEST(CookMeshTest, AnInvalidMeshWritesNothing) {
    const std::string output = output_path("sin_normales.mzasset");

    const auto cooked = cook_mesh(fixture_path("sin_normales.gltf"), output);
    ASSERT_FALSE(cooked.has_value());
    EXPECT_EQ(cooked.error().m_kind, cook_error_kind::missing_normal);
    EXPECT_FALSE(mz::test::exists(output));
    EXPECT_FALSE(mz::test::exists(output + ".tmp"));
}

// --- El asset cocinado ---------------------------------------------------------------------

std::span<const std::byte> section_bytes(const asset_view &asset, section_kind kind) {
    const auto found = asset.find(kind);
    return found.has_value() ? found->m_bytes : std::span<const std::byte>{};
}

// Las secciones, copiadas a vectores de sus structs (memcpy: los bytes no son objetos).
template <typename T> std::vector<T> section_as(const asset_view &asset, section_kind kind) {
    const std::span<const std::byte> bytes = section_bytes(asset, kind);
    EXPECT_EQ(bytes.size() % sizeof(T), 0U);
    std::vector<T> values(bytes.size() / sizeof(T));
    if (!values.empty()) {
        std::memcpy(values.data(), bytes.data(), values.size() * sizeof(T));
    }
    return values;
}

TEST(MeshAssetTest, WritesTheThreeSections) {
    const mesh_data mesh = read_valid("dos_primitivas.gltf");
    const std::vector<std::byte> bytes = mesh_asset(mesh);

    const auto asset = read_asset(bytes);
    ASSERT_TRUE(asset.has_value());
    EXPECT_EQ(asset.value().kind(), asset_kind::mesh);
    EXPECT_EQ(asset.value().section_count(), 3U);

    const auto vertices = section_as<mesh_vertex>(asset.value(), section_kind::mesh_vertices);
    ASSERT_EQ(vertices.size(), mesh.m_vertices.size());
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        expect_exactly(vertices[i].m_position, mesh.m_vertices[i].m_position);
        expect_exactly(vertices[i].m_normal, mesh.m_vertices[i].m_normal);
        expect_exactly(vertices[i].m_uv, mesh.m_vertices[i].m_uv);
    }
    EXPECT_EQ(section_as<std::uint32_t>(asset.value(), section_kind::mesh_indices), mesh.m_indices);
    EXPECT_EQ(ranges(section_as<mesh_submesh>(asset.value(), section_kind::mesh_submeshes)),
              ranges(mesh.m_submeshes));
}

// El cubo de fábrica de Blender (2 m de lado, centrado en el origen), que el build exporta y
// cocina (mz_cook_blend en tests/tools/CMakeLists.txt). Prueba la cadena entera:
// .blend → .glb → .mzasset, leído como lo leerá el juego.
TEST(CookedCubeTest, HasSixFacesWithTheirNormalsPointingOut) {
    const std::vector<std::byte> bytes = read_file(built_fixture_path("cubo.mzasset"));
    const auto asset = read_asset(bytes);
    ASSERT_TRUE(asset.has_value());
    EXPECT_EQ(asset.value().kind(), asset_kind::mesh);

    expect_closed_cube(section_as<mesh_vertex>(asset.value(), section_kind::mesh_vertices),
                       section_as<std::uint32_t>(asset.value(), section_kind::mesh_indices));
}

// Cocinar dos veces el mismo asset da los mismos bytes: lo cocinado no depende de la corrida
// (ni de la hora, ni de las direcciones de memoria, ni del relleno sin escribir).
TEST(CookMeshTest, CookingTwiceGivesTheSameBytes) {
    const std::string input = built_fixture_path("cubo.glb");
    const std::string first = output_path("cubo_a.mzasset");
    const std::string second = output_path("cubo_b.mzasset");

    ASSERT_TRUE(cook_mesh(input, first).has_value());
    ASSERT_TRUE(cook_mesh(input, second).has_value());
    const std::vector<std::byte> bytes = read_file(first);
    EXPECT_FALSE(bytes.empty());
    EXPECT_EQ(bytes, read_file(second));
}

} // namespace
