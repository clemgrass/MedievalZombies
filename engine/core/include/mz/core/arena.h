#pragma once

// arena: memoria temporal lineal sobre una región. Pedir es sumar un número al offset; no se
// libera de a uno, sino todo junto (reset) o hasta un marcador (rewind).
//
//   arena step{std::move(region_del_paso)};
//   std::byte *p = step.allocate(256, 16);
//   ...
//   step.reset(); // al terminar el paso
//
// No limpia lo que entrega: quien pide inicializa. Llena, aborta con su nombre y su pico,
// en todos los builds: su tamaño es un contrato, y quedarse corta es un error de
// dimensionamiento.

#include "mz/core/assert.h"
#include "mz/core/region.h"

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstddef>
#include <limits>
#include <string_view>
#include <type_traits>

namespace mz::core {

// Lo que se puede guardar en la memoria de una arena: un objeto (ni referencia, ni void, ni
// arreglo) que se pueda destruir. Su alineación se revisa al pedir el lugar.
template <typename T>
concept arena_storable = std::is_object_v<T> && !std::is_array_v<T> && std::destructible<T>;

class arena {
public:
    // La alineación más grande que se puede pedir. La región empieza en un borde de página
    // (4096 bytes o más), así que alinear el offset alinea también la dirección.
    static constexpr std::size_t max_alignment = 4096;

    // La arena se queda con la región: su nombre y su tamaño son los de la arena.
    explicit arena(region &&block) : m_block(std::move(block)) {}

    // `size` bytes alineados a `alignment` (una potencia de 2, hasta max_alignment). Con
    // size 0 devuelve un puntero alineado y no ocupa nada.
    [[nodiscard]] std::byte *allocate(std::size_t size, std::size_t alignment) {
        MZ_ASSERT(std::has_single_bit(alignment) && alignment <= max_alignment,
                  "arena \"{}\": alineación {} (tiene que ser potencia de 2, hasta {})", name(),
                  alignment, max_alignment);
        // Redondea hacia arriba al múltiplo de alignment: con una potencia de 2, ~(a - 1)
        // apaga los bits de abajo.
        const std::size_t start = (m_used + alignment - 1) & ~(alignment - 1);
        // Sin sumar start + size, que con un size enorme da la vuelta y parece caber. start
        // no la da: m_used no pasa de la capacidad, y el relleno es menor que alignment.
        MZ_CHECK(start <= capacity() && size <= capacity() - start,
                 "arena \"{}\" llena: pide {} bytes (alineación {}), usa {} de {}, pico {}", name(),
                 size, alignment, m_used, capacity(), m_peak);
        m_used = start + size;
        m_peak = std::max(m_peak, m_used);
        return m_block.data() + start;
    }

    // Lugar para `count` objetos T, alineado para T y sin construir: quien lo pide crea los
    // objetos (std::construct_at) y los destruye. Es lo que usan los contenedores.
    template <arena_storable T> [[nodiscard]] T *allocate_storage(std::size_t count) {
        static_assert(alignof(T) <= max_alignment, "T pide más alineación que una arena");
        // count × sizeof(T) con un count enorme da la vuelta y parece un pedido chico.
        MZ_CHECK(count <= max_count<T>(), "arena \"{}\": {} objetos de {} bytes no caben en size_t",
                 name(), count, sizeof(T));
        // Bytes alineados para T, todavía sin objetos: el puntero sirve para construirlos
        // ahí (std::construct_at), no para leer antes de hacerlo.
        return reinterpret_cast<T *>(allocate(count * sizeof(T), alignof(T)));
    }

    // Todo lo entregado deja de valer. El pico no cambia.
    void reset() { m_used = 0; }

    // Una posición de la arena, para volver a ella con rewind. Solo la arena la crea y la
    // lee.
    class marker {
    public:
        // Sin constructor por defecto: un marcador sale siempre de mark().
        marker() = delete;

    private:
        friend class arena;
        explicit marker(std::size_t offset) : m_offset(offset) {}
        std::size_t m_offset;
    };

    [[nodiscard]] marker mark() const { return marker{m_used}; }

    // Libera lo que se entregó después de `position`; lo de antes sigue valiendo. Los
    // marcadores se usan como una pila: el último que se guardó, el primero que se usa.
    void rewind(marker position) {
        MZ_ASSERT(position.m_offset <= m_used,
                  "arena \"{}\": rewind a un marcador adelante ({} con {} ocupados)", name(),
                  position.m_offset, m_used);
        m_used = position.m_offset;
    }

    [[nodiscard]] std::string_view name() const { return m_block.name(); }
    [[nodiscard]] std::size_t capacity() const { return m_block.size(); }
    // Lo ocupado, con el relleno de alineación.
    [[nodiscard]] std::size_t used() const { return m_used; }
    // Lo más que se ocupó desde que se creó la arena, también antes de cada reset.
    [[nodiscard]] std::size_t peak() const { return m_peak; }

private:
    template <typename T> static constexpr std::size_t max_count() {
        return std::numeric_limits<std::size_t>::max() / sizeof(T);
    }

    region m_block;
    std::size_t m_used = 0;
    std::size_t m_peak = 0;
};

} // namespace mz::core
