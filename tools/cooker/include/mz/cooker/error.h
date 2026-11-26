#pragma once

// Por qué el cocinador rechazó un asset. Son errores del asset o del disco, no bugs del
// cocinador: mz_cook escribe el mensaje y sale con un código distinto de cero, así que el
// build falla y el juego nunca ve un asset roto.

#include <cstdint>
#include <string>
#include <string_view>

namespace mz::cooker {

enum class cook_error_kind : std::uint8_t {
    cannot_read,  // cgltf no pudo leer el archivo o alguno de sus buffers
    invalid_gltf, // cgltf_validate lo rechazó (por ejemplo, un índice fuera de los vértices)
    unsupported_extension, // pide una extensión (extensionsRequired), como Draco o meshopt
    no_mesh,               // ningún nodo tiene malla
    several_meshes,        // más de un nodo tiene malla
    has_skin,              // la malla tiene esqueleto
    not_identity,          // el objeto está movido, rotado o escalado
    no_primitives,
    not_triangles,
    no_indices,
    loose_indices, // la cantidad de índices de una primitiva no es múltiplo de 3
    missing_position,
    missing_normal,
    missing_uv,
    bad_position_type, // no es VEC3 en float
    bad_normal_type,   // no es VEC3 en float
    bad_uv_type,       // no es VEC2 en float
    // cgltf no pudo entregar los valores de un accessor: sin bufferView, sparse, o con un
    // buffer sin datos
    unreadable_accessor,
    cannot_write, // no se pudo escribir el asset cocinado
};

struct cook_error {
    cook_error_kind m_kind;
    // Dónde: el nodo de glTF (el objeto de Blender) o, si no se pudo escribir, el archivo de
    // salida. Vacío si el error es del archivo de entrada entero.
    std::string m_object;
};

// El motivo, en una frase sin mayúscula inicial ni punto final.
[[nodiscard]] std::string_view describe(cook_error_kind kind);

// El mensaje completo: "<archivo>: <objeto>: <motivo>", o "<archivo>: <motivo>" sin objeto.
[[nodiscard]] std::string format_error(std::string_view file, const cook_error &error);

} // namespace mz::cooker
