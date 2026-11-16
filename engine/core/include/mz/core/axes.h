#pragma once

// La convención de ejes del engine: mano derecha, Y arriba y −Z adelante, en metros y
// radianes. Es la de glTF y la de Godot.
//
// "Adelante" es hacia dónde mira la cámara, y también los personajes: el importador de
// assets rota los modelos glTF, que miran hacia +Z, para que miren hacia −Z. Así hay una
// sola dirección adelante para la cámara, los personajes y la IA.
//
// Mano derecha quiere decir x × y = z, así que derecha × arriba = +Z, que es "atrás".

#include "mz/core/vec.h"

namespace mz::core {

inline constexpr vec3 world_right{1.0F, 0.0F, 0.0F};
inline constexpr vec3 world_up{0.0F, 1.0F, 0.0F};
inline constexpr vec3 world_forward{0.0F, 0.0F, -1.0F};

} // namespace mz::core
