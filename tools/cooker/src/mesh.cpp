#include "mz/cooker/mesh.h"

#include "mz/cooker/file.h"
#include "mz/core/asset_format.h"
#include "mz/core/scalar.h"

#include <cgltf.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace mz::cooker {
namespace {

using core::result;
using core::unexpected;

struct gltf_deleter {
    void operator()(cgltf_data *data) const { cgltf_free(data); }
};
using gltf_ptr = std::unique_ptr<cgltf_data, gltf_deleter>;

// Cuánto puede alejarse de la identidad la transformación del objeto: el ruido de float que
// deja Blender, no una transformación hecha a propósito.
constexpr float identity_tolerance = 1e-6F;

// La transformación de mundo del nodo (con la de sus padres) es la identidad.
bool is_identity(const cgltf_node &node) {
    std::array<float, 16> matrix{};
    cgltf_node_transform_world(&node, matrix.data());
    for (std::size_t i = 0; i < matrix.size(); ++i) {
        // Por columnas: la diagonal de una 4×4 cae en los índices 0, 5, 10 y 15.
        const float expected = (i % 5 == 0) ? 1.0F : 0.0F;
        if (!core::nearly_equal(matrix[i], expected, identity_tolerance)) {
            return false;
        }
    }
    return true;
}

// El nombre del nodo, que es el del objeto en Blender; si no tiene, su número.
std::string object_name(const cgltf_data &data, const cgltf_node &node) {
    if (node.name != nullptr) {
        return node.name;
    }
    return std::format("nodo {}", cgltf_node_index(&data, &node));
}

// El único nodo con malla.
result<const cgltf_node *, cook_error_kind> find_mesh_node(const cgltf_data &data) {
    const cgltf_node *found = nullptr;
    for (const cgltf_node &node : std::span{data.nodes, data.nodes_count}) {
        if (node.mesh == nullptr) {
            continue;
        }
        if (found != nullptr) {
            return unexpected{cook_error_kind::several_meshes};
        }
        found = &node;
    }
    if (found == nullptr) {
        return unexpected{cook_error_kind::no_mesh};
    }
    return found;
}

// El accessor de un atributo, si existe y es de la forma pedida en float. glTF también
// permite enteros normalizados (KHR_mesh_quantization), pero el formato del engine es float.
result<const cgltf_accessor *, cook_error_kind>
float_attribute(const cgltf_primitive &primitive, cgltf_attribute_type attribute, cgltf_type shape,
                cook_error_kind missing, cook_error_kind bad_type) {
    const cgltf_accessor *accessor = cgltf_find_accessor(&primitive, attribute, 0);
    if (accessor == nullptr) {
        return unexpected{missing};
    }
    if (accessor->type != shape || accessor->component_type != cgltf_component_type_r_32f) {
        return unexpected{bad_type};
    }
    return accessor;
}

// Suma una primitiva a la malla: sus vértices al final, y sus índices corridos por los
// vértices que ya había.
result<std::monostate, cook_error_kind> append_primitive(const cgltf_primitive &primitive,
                                                         mesh_data &mesh) {
    if (primitive.type != cgltf_primitive_type_triangles) {
        return unexpected{cook_error_kind::not_triangles};
    }
    if (primitive.indices == nullptr) {
        return unexpected{cook_error_kind::no_indices};
    }
    if (primitive.indices->count % 3 != 0) {
        return unexpected{cook_error_kind::loose_indices};
    }
    const auto positions =
        float_attribute(primitive, cgltf_attribute_type_position, cgltf_type_vec3,
                        cook_error_kind::missing_position, cook_error_kind::bad_position_type);
    if (!positions) {
        return unexpected{positions.error()};
    }
    const auto normals =
        float_attribute(primitive, cgltf_attribute_type_normal, cgltf_type_vec3,
                        cook_error_kind::missing_normal, cook_error_kind::bad_normal_type);
    if (!normals) {
        return unexpected{normals.error()};
    }
    const auto uvs = float_attribute(primitive, cgltf_attribute_type_texcoord, cgltf_type_vec2,
                                     cook_error_kind::missing_uv, cook_error_kind::bad_uv_type);
    if (!uvs) {
        return unexpected{uvs.error()};
    }

    // Los valores de las posiciones, las normales y las UV, con cualquier stride. cgltf
    // devuelve cuántos floats escribió, y escribe menos si el buffer no tiene datos (un .gltf
    // con un buffer sin uri, o comprimido con una extensión): el asset saldría con ceros. Un
    // accessor sin bufferView sí es válido: glTF lo define como ceros.
    const std::array accessors{positions.value(), normals.value(), uvs.value()};
    std::array<std::vector<float>, accessors.size()> values;
    for (std::size_t a = 0; a < accessors.size(); ++a) {
        values[a].resize(cgltf_num_components(accessors[a]->type) * accessors[a]->count);
        if (cgltf_accessor_unpack_floats(accessors[a], values[a].data(), values[a].size()) !=
            values[a].size()) {
            return unexpected{cook_error_kind::unreadable_accessor};
        }
    }
    const auto &[p, n, uv] = values;

    // cgltf_validate ya revisó que los atributos tengan los mismos vértices. Las cuentas caben
    // en 32 bits: un accessor no puede tener más elementos que bytes su buffer, y 2^32
    // vértices serían más de 128 GiB de glTF.
    const auto first_vertex = static_cast<std::uint32_t>(mesh.m_vertices.size());
    for (std::size_t i = 0; i < positions.value()->count; ++i) {
        mesh.m_vertices.push_back({
            .m_position = gltf_to_engine({p[3 * i], p[(3 * i) + 1], p[(3 * i) + 2]}),
            .m_normal = gltf_to_engine({n[3 * i], n[(3 * i) + 1], n[(3 * i) + 2]}),
            .m_uv = {uv[2 * i], uv[(2 * i) + 1]},
        });
    }

    // Los índices no tienen el caso de los ceros: cgltf no entrega los que no tienen
    // bufferView ni los sparse. cgltf_validate revisa que no se salgan de los vértices, pero
    // solo si los puede leer: por eso esta cuenta va antes de usarlos.
    std::vector<std::uint32_t> indices(primitive.indices->count);
    if (cgltf_accessor_unpack_indices(primitive.indices, indices.data(), sizeof(std::uint32_t),
                                      indices.size()) != indices.size()) {
        return unexpected{cook_error_kind::unreadable_accessor};
    }
    const auto first_index = static_cast<std::uint32_t>(mesh.m_indices.size());
    for (const std::uint32_t index : indices) {
        mesh.m_indices.push_back(first_vertex + index);
    }
    mesh.m_submeshes.push_back({
        .m_first_index = first_index,
        .m_index_count = static_cast<std::uint32_t>(indices.size()),
    });
    return std::monostate{};
}

} // namespace

result<mesh_data, cook_error> read_gltf_mesh(const std::string &path) {
    const cgltf_options options{};
    cgltf_data *parsed = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &parsed) != cgltf_result_success) {
        return unexpected{cook_error{.m_kind = cook_error_kind::cannot_read, .m_object = {}}};
    }
    const gltf_ptr data{parsed};
    // Los buffers: el bloque binario de un .glb, o los que nombra el .gltf (un data URI o un
    // archivo al lado).
    if (cgltf_load_buffers(&options, data.get(), path.c_str()) != cgltf_result_success) {
        return unexpected{cook_error{.m_kind = cook_error_kind::cannot_read, .m_object = {}}};
    }
    if (cgltf_validate(data.get()) != cgltf_result_success) {
        return unexpected{cook_error{.m_kind = cook_error_kind::invalid_gltf, .m_object = {}}};
    }
    // Una extensión obligatoria cambia cómo se leen los datos (Draco y meshopt comprimen los
    // buffers), y el cocinador no implementa ninguna.
    if (data->extensions_required_count > 0) {
        return unexpected{
            cook_error{.m_kind = cook_error_kind::unsupported_extension, .m_object = {}}};
    }

    const auto found = find_mesh_node(*data);
    if (!found) {
        return unexpected{cook_error{.m_kind = found.error(), .m_object = {}}};
    }
    const cgltf_node &node = *found.value();
    const std::string object = object_name(*data, node);
    const auto fail = [&object](cook_error_kind kind) {
        return unexpected{cook_error{.m_kind = kind, .m_object = object}};
    };
    if (node.skin != nullptr) {
        return fail(cook_error_kind::has_skin);
    }
    if (!is_identity(node)) {
        return fail(cook_error_kind::not_identity);
    }
    if (node.mesh->primitives_count == 0) {
        return fail(cook_error_kind::no_primitives);
    }

    mesh_data mesh;
    for (const cgltf_primitive &primitive :
         std::span{node.mesh->primitives, node.mesh->primitives_count}) {
        const auto appended = append_primitive(primitive, mesh);
        if (!appended) {
            return fail(appended.error());
        }
    }
    return mesh;
}

std::vector<std::byte> mesh_asset(const mesh_data &mesh) {
    const std::array sections{
        core::section_input{.m_kind = core::section_kind::mesh_vertices,
                            .m_bytes = std::as_bytes(std::span{mesh.m_vertices})},
        core::section_input{.m_kind = core::section_kind::mesh_indices,
                            .m_bytes = std::as_bytes(std::span{mesh.m_indices})},
        core::section_input{.m_kind = core::section_kind::mesh_submeshes,
                            .m_bytes = std::as_bytes(std::span{mesh.m_submeshes})},
    };
    std::vector<std::byte> bytes(core::asset_size(sections));
    core::write_asset(bytes, core::asset_kind::mesh, sections);
    return bytes;
}

result<std::monostate, cook_error> cook_mesh(const std::string &input, const std::string &output) {
    const auto mesh = read_gltf_mesh(input);
    if (!mesh) {
        return unexpected{mesh.error()};
    }
    return write_file_atomic(output, mesh_asset(mesh.value()));
}

} // namespace mz::cooker
