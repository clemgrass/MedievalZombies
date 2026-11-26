#include "mz/cooker/error.h"

#include <format>
#include <string>
#include <string_view>

namespace mz::cooker {

std::string_view describe(cook_error_kind kind) {
    switch (kind) {
    case cook_error_kind::cannot_read:
        return "no se pudo leer como glTF (el archivo o alguno de sus buffers)";
    case cook_error_kind::invalid_gltf:
        return "el glTF no es válido (por ejemplo, un índice que se sale de los vértices)";
    case cook_error_kind::unsupported_extension:
        return "el glTF pide una extensión (extensionsRequired) que el cocinador no lee, como "
               "la compresión de mallas: exporta sin comprimir";
    case cook_error_kind::no_mesh:
        return "no hay ningún objeto con malla";
    case cook_error_kind::several_meshes:
        return "hay más de un objeto con malla, y un asset de malla estática tiene uno solo";
    case cook_error_kind::has_skin:
        return "la malla tiene esqueleto, y una malla estática no lleva";
    case cook_error_kind::not_identity:
        return "el objeto está movido, rotado o escalado: aplica su transformación en Blender "
               "(Ctrl+A, All Transforms)";
    case cook_error_kind::no_primitives:
        return "la malla no tiene primitivas";
    case cook_error_kind::not_triangles:
        return "una primitiva no es de triángulos";
    case cook_error_kind::no_indices:
        return "una primitiva no tiene índices";
    case cook_error_kind::loose_indices:
        return "la cantidad de índices de una primitiva no es múltiplo de 3";
    case cook_error_kind::missing_position:
        return "a una primitiva le faltan las posiciones (POSITION)";
    case cook_error_kind::missing_normal:
        return "a una primitiva le faltan las normales (NORMAL)";
    case cook_error_kind::missing_uv:
        return "a una primitiva le faltan las coordenadas de textura (TEXCOORD_0)";
    case cook_error_kind::bad_position_type:
        return "las posiciones (POSITION) no son VEC3 en float";
    case cook_error_kind::bad_normal_type:
        return "las normales (NORMAL) no son VEC3 en float";
    case cook_error_kind::bad_uv_type:
        return "las coordenadas de textura (TEXCOORD_0) no son VEC2 en float";
    case cook_error_kind::unreadable_accessor:
        return "no se pudieron leer los datos de un atributo o de los índices (sin bufferView, "
               "sparse o con un buffer sin datos)";
    case cook_error_kind::cannot_write:
        return "no se pudo escribir el asset cocinado";
    }
    // Todos los casos del enum vuelven arriba: -Wswitch avisa si se agrega uno sin su
    // mensaje. Esto es para un valor fuera del enum.
    return "error desconocido del cocinador";
}

std::string format_error(std::string_view file, const cook_error &error) {
    if (error.m_object.empty()) {
        return std::format("{}: {}", file, describe(error.m_kind));
    }
    return std::format("{}: {}: {}", file, error.m_object, describe(error.m_kind));
}

} // namespace mz::cooker
