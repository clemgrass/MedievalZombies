#pragma once

// El cocinado de una malla estática: lee un glTF, lo valida, pasa sus ejes a los del engine
// y lo escribe como asset de malla (mz/core/mesh_format.h).
//
//   const auto cooked = cook_mesh("modelos/cubo.glb", "modelos/cubo.mzasset");
//   if (!cooked) {
//       // format_error(…, cooked.error()) dice qué falló y en qué objeto.
//   }
//
// Una malla estática es un glTF con un solo nodo con malla, sin esqueleto y sin
// transformación (en el origen, sin rotar ni escalar): el modelo se arma en Blender alrededor
// del origen. Cada primitiva necesita triángulos con índices, y posiciones, normales y UV en
// float.

#include "mz/cooker/error.h"
#include "mz/core/mesh_format.h"
#include "mz/core/result.h"
#include "mz/core/vec.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace mz::cooker {

// Una malla ya convertida a los ejes del engine, antes de escribirla. Las primitivas de glTF
// quedan juntas en un solo buffer de vértices y uno de índices, y cada una es una submalla.
struct mesh_data {
    std::vector<core::mesh_vertex> m_vertices;
    std::vector<std::uint32_t> m_indices;
    std::vector<core::mesh_submesh> m_submeshes;
};

// De los ejes de glTF a los del engine: un giro de 180° sobre Y, (x, y, z) → (−x, y, −z).
// Los dos usan mano derecha e Y arriba, pero un modelo de glTF mira hacia +Z y uno del engine
// hacia −Z. Es un giro y no un espejo (determinante +1), así que el orden de los vértices de
// cada triángulo no cambia. Vale para posiciones y para normales.
[[nodiscard]] constexpr core::vec3 gltf_to_engine(core::vec3 v) { return {-v.m_x, v.m_y, -v.m_z}; }

[[nodiscard]] core::result<mesh_data, cook_error> read_gltf_mesh(const std::string &path);

// El asset de malla, con sus tres secciones: vértices, índices y submallas.
[[nodiscard]] std::vector<std::byte> mesh_asset(const mesh_data &mesh);

// Lee `input`, lo convierte y escribe el asset en `output` (con write_file_atomic). Si algo
// falla, `output` queda como estaba.
[[nodiscard]] core::result<std::monostate, cook_error> cook_mesh(const std::string &input,
                                                                 const std::string &output);

} // namespace mz::cooker
