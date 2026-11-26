#pragma once

// Archivos de los tests del cocinador: los fixtures del repo, los que cocina el build y una
// carpeta de salida para lo que escriben los tests. Las rutas llegan como definiciones del
// compilador (tests/tools/CMakeLists.txt).

#include <array>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace mz::test {

// Un fixture del repo: tests/tools/fixtures/<name>.
inline std::string fixture_path(std::string_view name) {
    return std::string{MZ_COOKER_FIXTURES} + "/" + std::string{name};
}

// Un fixture que exporta y cocina el build: <build>/tests/tools/fixtures/<name>.
inline std::string built_fixture_path(std::string_view name) {
    return std::string{MZ_COOKER_BUILT_FIXTURES} + "/" + std::string{name};
}

// Un archivo en la carpeta de salida de los tests. Cada test usa un nombre propio: CTest los
// corre en paralelo, cada uno en su proceso.
inline std::string output_path(std::string_view name) {
    return std::string{MZ_COOKER_OUTPUT} + "/" + std::string{name};
}

struct file_closer {
    void operator()(std::FILE *file) const { std::fclose(file); }
};
using file_ptr = std::unique_ptr<std::FILE, file_closer>;

inline bool exists(const std::string &path) {
    return file_ptr{std::fopen(path.c_str(), "rb")} != nullptr;
}

// Todo el archivo, o nada si no se puede abrir.
inline std::vector<std::byte> read_file(const std::string &path) {
    const file_ptr file{std::fopen(path.c_str(), "rb")};
    std::vector<std::byte> bytes;
    if (file == nullptr) {
        return bytes;
    }
    std::array<std::byte, 4096> chunk{};
    std::size_t read = 0;
    while ((read = std::fread(chunk.data(), 1, chunk.size(), file.get())) > 0) {
        bytes.insert(bytes.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(read));
    }
    return bytes;
}

} // namespace mz::test
