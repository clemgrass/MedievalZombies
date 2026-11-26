#pragma once

#include "mz/cooker/error.h"
#include "mz/core/result.h"

#include <cstddef>
#include <span>
#include <string>
#include <variant>

namespace mz::cooker {

// Escribe `bytes` en `path` sin dejarlo nunca a medias: escribe <path>.tmp y lo renombra, y
// en el mismo sistema de archivos renombrar es atómico. El juego recarga los assets en
// caliente, y así nunca lee uno escrito hasta la mitad; y si la escritura falla (un disco
// lleno), se borra el temporal y path queda como estaba.
[[nodiscard]] core::result<std::monostate, cook_error>
write_file_atomic(const std::string &path, std::span<const std::byte> bytes);

} // namespace mz::cooker
