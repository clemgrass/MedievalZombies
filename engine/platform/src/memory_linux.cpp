#include "mz/platform/memory.h"

#include <sys/mman.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>

namespace mz::platform {

mapping map_populated(std::size_t size) {
    // MAP_PRIVATE | MAP_ANONYMOUS: memoria propia, sin archivo detrás, llena de ceros.
    // MAP_POPULATE: el kernel entrega todas las páginas ahora, en una sola llamada, en vez
    // de un fallo de página por cada una en su primer toque. En un mapeo privado y de
    // escritura las pide como si se escribieran: cada página es propia desde ya, no la
    // página de ceros compartida que se copiaría en la primera escritura.
    void *data = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    if (data == MAP_FAILED) {
        return {};
    }
    return {.m_data = static_cast<std::byte *>(data), .m_size = size};
}

void unmap(mapping block) {
    // munmap solo falla con un rango que no salió de mmap (EINVAL): un bug, no un error
    // esperable. platform está debajo de engine/core, así que no tiene MZ_CHECK.
    if (munmap(block.m_data, block.m_size) != 0) {
        const int error = errno;
        std::fprintf(stderr, "platform::unmap: munmap falló (errno %d)\n", error);
        std::abort();
    }
}

std::size_t page_size() { return static_cast<std::size_t>(sysconf(_SC_PAGESIZE)); }

} // namespace mz::platform
