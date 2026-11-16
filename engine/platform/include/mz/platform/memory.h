#pragma once

// La memoria que el engine le pide al sistema operativo: bloques grandes, al iniciar, ya
// poblados (cada página con su RAM detrás). Es lo único de la memoria que sabe del sistema
// operativo; las regiones con nombre, las arenas y los pools viven en engine/core, encima
// de esto, y nunca llaman al sistema operativo.
//
// Por qué poblar al reservar: el sistema operativo solo promete la memoria. La página se
// entrega la primera vez que se toca, con un fallo de página que cuesta en el frame donde
// cae. Poblar al reservar adelanta ese costo al arranque.

#include <cstddef>

namespace mz::platform {

// Un bloque de memoria pedido al sistema operativo. Vacío (m_data == nullptr) si el pedido
// falló. Es un dato plano, sin dueño: quien lo pidió lo devuelve con unmap().
struct mapping {
    std::byte *m_data = nullptr;
    // El tamaño pedido. El sistema operativo redondea hacia arriba a páginas enteras, pero
    // lo que pasa de m_size no se usa.
    std::size_t m_size = 0;
};

// Reserva `size` bytes de lectura y escritura, alineados a página y poblados. Devuelve un
// mapping vacío si el sistema operativo no puede darlos: un tamaño 0, uno que no cabe en
// el espacio de direcciones, o más memoria de la que el sistema acepta prometer.
//
// Linux: mmap con MAP_POPULATE. mmap no falla si no logra poblar todas las páginas; en la
// práctica, si falta RAM durante el populate, el OOM killer mata el proceso al iniciar.
[[nodiscard]] mapping map_populated(std::size_t size);

// Devuelve el bloque al sistema operativo. Un bloque que no salió de map_populated() es un
// error del programador: escribe el error del sistema en stderr y aborta.
void unmap(mapping block);

// El tamaño de una página del sistema operativo, en bytes (4096 en x86-64).
[[nodiscard]] std::size_t page_size();

} // namespace mz::platform
