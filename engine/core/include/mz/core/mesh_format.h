#pragma once

// Las secciones de un asset de malla estática (asset_kind::mesh). Lo escribe el cocinador y
// lo lee el juego, así que, como el resto del formato, vive en core.
//
//   section_kind::mesh_vertices   mesh_vertex × n, intercalados (posición, normal, UV)
//   section_kind::mesh_indices    uint32_t × 3 por triángulo, sobre todos los vértices
//   section_kind::mesh_submeshes  mesh_submesh × una por primitiva de glTF
//
// Todo está en los ejes del engine (mz/core/axes.h). Los triángulos van en orden
// antihorario vistos desde afuera, como en glTF.

#include "mz/core/vec.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace mz::core {

// Un vértice de 32 bytes, en float. Intercalado: el vertex shader lee los tres atributos de
// un solo buffer, con un solo paso (stride).
struct mesh_vertex {
    vec3 m_position;
    vec3 m_normal;
    // (0, 0) es la esquina de arriba a la izquierda de la textura, como en glTF y en Vulkan:
    // la V no se da vuelta.
    vec2 m_uv;
};

// Un tramo de índices que se dibuja con el mismo material. Los índices ya apuntan a la lista
// completa de vértices: una submalla se dibuja con first_index e index_count, sin sumar un
// vértice base.
struct mesh_submesh {
    std::uint32_t m_first_index;
    std::uint32_t m_index_count;
};

static_assert(sizeof(mesh_vertex) == 32);
static_assert(offsetof(mesh_vertex, m_position) == 0);
static_assert(offsetof(mesh_vertex, m_normal) == 12);
static_assert(offsetof(mesh_vertex, m_uv) == 24);
static_assert(std::is_trivially_copyable_v<mesh_vertex>);

static_assert(sizeof(mesh_submesh) == 8);
static_assert(offsetof(mesh_submesh, m_first_index) == 0);
static_assert(offsetof(mesh_submesh, m_index_count) == 4);
static_assert(std::is_trivially_copyable_v<mesh_submesh>);

} // namespace mz::core
