#include "mz/cooker/file.h"

#include <cstdio>

namespace mz::cooker {

core::result<std::monostate, cook_error> write_file_atomic(const std::string &path,
                                                           std::span<const std::byte> bytes) {
    const std::string temporary = path + ".tmp";
    const cook_error failed{.m_kind = cook_error_kind::cannot_write, .m_object = path};

    std::FILE *file = std::fopen(temporary.c_str(), "wb");
    if (file == nullptr) {
        return core::unexpected{failed};
    }
    const bool written = std::fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size();
    // fwrite puede dejar los bytes en el búfer de stdio: un disco lleno se nota recién al
    // vaciarlo, en fclose.
    const bool closed = std::fclose(file) == 0;
    if (!written || !closed || std::rename(temporary.c_str(), path.c_str()) != 0) {
        // Si tampoco se puede borrar, queda un .tmp, que nadie lee: el error ya es el de
        // escritura.
        static_cast<void>(std::remove(temporary.c_str()));
        return core::unexpected{failed};
    }
    return std::monostate{};
}

} // namespace mz::cooker
