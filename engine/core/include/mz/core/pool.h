#pragma once

// pool<T>: objetos de tamaño fijo que nacen y mueren en cualquier momento (pedidos de corte,
// cuerpos, ragdolls), en N slots reservados al crearse. Crear y destruir son O(1), sin
// fragmentación, y a cada objeto se lo nombra con un handle<T>, no con un puntero.
//
//   pool<slice_request> requests{permanent, 20, "pedidos de corte"};
//   if (requests.has_room()) {
//       const handle<slice_request> h = requests.create(request).value();
//       ...
//       if (slice_request *r = requests.get(h)) { ... } // nullptr si ya no existe
//       requests.destroy(h);
//   }
//
// Lleno, create() no aborta: devuelve pool_error::full. Lo que llena un pool del juego lo
// deciden sus reglas (cuántos zombies se cortan a la vez), así que cada pool tiene su regla
// para degradar, y el juego pregunta antes con has_room().
//
// Cómo funciona:
//
// - La lista libre vive dentro de los slots libres. Cada slot es una union: un T si está
//   vivo, o el índice del siguiente slot libre si no. El pool solo guarda el primero de la
//   lista (m_free_head). Crear saca el primero y destruir lo pone adelante (LIFO): el slot
//   que se reusa es el último que se devolvió, que probablemente sigue en la caché.
// - Cada slot tiene una generación, que sube al crear y al destruir. Así queda impar si el
//   slot está vivo y par si está libre, sin un bool aparte. Un handle vale si su generación
//   es la del slot y es impar. La generación 0 (el handle nulo) es par: nunca vale.
// - La generación es un uint32 y da la vuelta sola: de 0xFFFFFFFF (vivo) pasa a 0 (libre).
//   Después de 2^31 reusos del mismo slot, un handle viejo podría volver a coincidir con
//   uno nuevo. A 60 reusos por segundo del mismo slot, eso es más de un año de juego sin
//   parar: se acepta.

#include "mz/core/arena.h"
#include "mz/core/assert.h"
#include "mz/core/handle.h"
#include "mz/core/result.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>

namespace mz::core {

// Por qué create() no pudo crear el objeto.
enum class pool_error : std::uint8_t {
    // Todos los slots están vivos. El juego degrada con la regla de ese pool.
    full,
};

template <arena_storable T> class pool {
public:
    // Pide a `memory` los slots y sus generaciones; si no caben, la arena aborta. El nombre
    // aparece en los mensajes y tiene que vivir más que el pool (un literal).
    pool(arena &memory, std::uint32_t capacity, std::string_view name)
        : m_slots(memory.allocate_storage<slot>(capacity)),
          m_generations(memory.allocate_storage<std::uint32_t>(capacity)), m_capacity(capacity),
          m_name(name) {
        // Al principio todos están libres, encadenados en orden: el primero que se entrega es
        // el 0. El último apunta a m_capacity, que marca el final de la lista.
        for (std::uint32_t i = 0; i < capacity; ++i) {
            std::construct_at(m_slots + i, i + 1);
            std::construct_at(m_generations + i, 0U);
        }
    }

    // No se copia: la copia necesitaría su propia memoria. Mover se lleva los slots, y los
    // handles siguen valiendo en el pool nuevo; el origen queda sin capacidad. Sin
    // asignación: un pool se crea una vez y no cambia de memoria.
    pool(const pool &) = delete;
    pool &operator=(const pool &) = delete;
    pool(pool &&other) noexcept
        : m_slots(std::exchange(other.m_slots, nullptr)),
          m_generations(std::exchange(other.m_generations, nullptr)),
          m_capacity(std::exchange(other.m_capacity, 0)), m_size(std::exchange(other.m_size, 0)),
          m_peak(other.m_peak), m_failures(std::exchange(other.m_failures, 0)),
          m_free_head(std::exchange(other.m_free_head, 0)), m_name(other.m_name) {}
    pool &operator=(pool &&) = delete;

    // Destruye los objetos que siguen vivos. La memoria es de la arena.
    ~pool() {
        for (std::uint32_t i = 0; i < m_capacity; ++i) {
            if (is_live_generation(m_generations[i])) {
                std::destroy_at(&m_slots[i].m_value);
            }
        }
    }

    [[nodiscard]] bool has_room() const { return m_size < m_capacity; }

    // Construye un T con esos argumentos en un slot libre y devuelve su handle, o
    // pool_error::full si no hay lugar.
    template <typename... Args> result<handle<T>, pool_error> create(Args &&...args) {
        if (!has_room()) {
            ++m_failures;
            return unexpected{pool_error::full};
        }
        const std::uint32_t index = m_free_head;
        slot &target = m_slots[index];
        // Primero se lee el siguiente libre: construir el T pisa ese número.
        m_free_head = target.m_next_free;
        std::construct_at(&target.m_value, std::forward<Args>(args)...);
        const std::uint32_t generation = ++m_generations[index]; // par → impar: vivo
        ++m_size;
        m_peak = std::max(m_peak, m_size);
        return handle<T>{index, generation};
    }

    // Destruye el objeto y devuelve su slot. Un handle que no está vivo en este pool (ya
    // destruido, o nulo) aborta en todos los builds: destruir dos veces es un bug. El pool
    // no sabe de dónde viene un handle: uno de otro pool del mismo T pasa si coinciden su
    // índice y su generación.
    void destroy(handle<T> h) {
        MZ_CHECK(is_live(h),
                 "pool \"{}\": destroy de un handle que no está vivo (índice {}, generación {})",
                 m_name, h.index(), h.generation());
        slot &target = m_slots[h.index()];
        std::destroy_at(&target.m_value);
        std::construct_at(&target.m_next_free, m_free_head);
        m_free_head = h.index();
        ++m_generations[h.index()]; // impar → par: libre
        --m_size;
    }

    // El objeto de `h`, o nullptr si ya no existe. Que no exista es un caso esperable: el
    // pool de pedazos descarta el más viejo aunque otro sistema guarde su handle.
    [[nodiscard]] T *get(handle<T> h) { return is_live(h) ? &m_slots[h.index()].m_value : nullptr; }
    [[nodiscard]] const T *get(handle<T> h) const {
        return is_live(h) ? &m_slots[h.index()].m_value : nullptr;
    }

    // Los objetos vivos.
    [[nodiscard]] std::uint32_t size() const { return m_size; }
    [[nodiscard]] std::uint32_t capacity() const { return m_capacity; }
    // Lo más que estuvo vivo a la vez, desde que se creó el pool.
    [[nodiscard]] std::uint32_t peak() const { return m_peak; }
    // Cuántos create() rechazó por estar lleno: cuántas veces degradó el juego por este pool.
    [[nodiscard]] std::uint32_t failures() const { return m_failures; }
    [[nodiscard]] std::string_view name() const { return m_name; }

private:
    // Un T vivo o, si el slot está libre, el índice del siguiente libre. La union no sabe
    // cuál de los dos está: lo dice la generación del slot.
    union slot {
        explicit slot(std::uint32_t next_free) : m_next_free(next_free) {}
        // T puede tener un destructor; el de la union no hace nada, porque el pool destruye
        // el T a mano cuando corresponde. Con `= default` quedaría borrado si T tiene
        // destructor (una union no sabe cuál de sus miembros destruir).
        // NOLINTNEXTLINE(modernize-use-equals-default)
        ~slot() {}
        slot(const slot &) = delete;
        slot &operator=(const slot &) = delete;
        slot(slot &&) = delete;
        slot &operator=(slot &&) = delete;

        T m_value;
        std::uint32_t m_next_free;
    };

    static bool is_live_generation(std::uint32_t generation) { return (generation & 1U) != 0; }

    // Si `h` nombra un objeto vivo de este pool.
    [[nodiscard]] bool is_live(handle<T> h) const {
        return h.index() < m_capacity && h.generation() == m_generations[h.index()] &&
               is_live_generation(h.generation());
    }

    slot *m_slots;
    std::uint32_t *m_generations;
    std::uint32_t m_capacity;
    std::uint32_t m_size = 0;
    std::uint32_t m_peak = 0;
    std::uint32_t m_failures = 0;
    // El primer slot libre; m_capacity si no hay ninguno.
    std::uint32_t m_free_head = 0;
    std::string_view m_name;
};

} // namespace mz::core
