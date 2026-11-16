#pragma once

// Lo que los tests de memoria le preguntan al sistema operativo por su cuenta, sin pasar
// por el engine: el tamaño de página y el estado de cada página de un rango, con
// mincore(2). Así un test no usa el código que prueba para comprobarlo.

#include <cstddef>

namespace mz::test {

// El tamaño de página según sysconf(_SC_PAGESIZE).
std::size_t page_size();

// Cuántas páginas de [data, data + size) tienen RAM detrás. data tiene que estar alineado
// a página. Si el rango no está mapeado, el test falla.
std::size_t resident_pages(std::byte *data, std::size_t size);

// Cuántas páginas cubre un rango de `size` bytes que empieza en una página.
std::size_t page_count(std::size_t size);

// Si [data, data + size) sigue mapeado. mincore falla con ENOMEM cuando alguna página del
// rango no está mapeada: así se ve que un bloque se devolvió al sistema operativo.
bool is_mapped(std::byte *data, std::size_t size);

} // namespace mz::test
