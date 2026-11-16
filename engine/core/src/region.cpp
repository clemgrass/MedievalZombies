#include "mz/core/region.h"

#include "mz/core/assert.h"
#include "mz/platform/memory.h"

namespace mz::core {

region region::reserve(std::string_view name, std::size_t size) {
    // Un tamaño 0 o un nombre vacío no dependen de la máquina: son un bug, no un error que
    // el que llama pueda manejar. Un tamaño grande sí depende de la máquina, y por eso ese
    // vuelve como region_error.
    MZ_ASSERT(size > 0, "region::create(\"{}\") con tamaño 0", name);
    MZ_ASSERT(!name.empty(), "region::create sin nombre ({} bytes)", size);

    const platform::mapping block = platform::map_populated(size);
    return {name, block.m_data, block.m_size};
}

region::region(std::string_view name, std::byte *data, std::size_t size)
    : m_name(name), m_data(data), m_size(size) {}

region::region(region &&other) noexcept
    : m_name(std::exchange(other.m_name, {})), m_data(std::exchange(other.m_data, nullptr)),
      m_size(std::exchange(other.m_size, 0)) {}

region::~region() {
    // Una región vacía (movida, o la que devuelve reserve() cuando falla) no tiene bloque.
    if (m_data != nullptr) {
        platform::unmap({.m_data = m_data, .m_size = m_size});
    }
}

} // namespace mz::core
