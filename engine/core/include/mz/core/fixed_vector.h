#pragma once

// fixed_vector<T>: un arreglo de capacidad fija. Recibe su memoria de una arena al crearse,
// para toda su capacidad, y nunca crece: después de crearse no le pide nada a nadie.
//
//   fixed_vector<slice_request> requests{step_arena, 32, "pedidos de corte del paso"};
//   requests.push_back(request);
//   for (const slice_request &r : requests) { ... }
//
// Lleno, push_back aborta con su nombre, en todos los builds: su capacidad es un contrato,
// y quedarse corto es un error de dimensionamiento. Quien lo llena con las reglas del juego
// pregunta antes con full().
//
// La arena tiene que vivir más que el vector, y no reiniciarse mientras el vector exista:
// el vector destruye sus elementos, pero la memoria es de la arena.

#include "mz/core/arena.h"
#include "mz/core/assert.h"

#include <cstddef>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

namespace mz::core {

template <arena_storable T> class fixed_vector {
public:
    // Pide a `memory` lugar para `capacity` elementos; si no cabe, la arena aborta. El
    // nombre aparece en los mensajes y tiene que vivir más que el vector (un literal).
    fixed_vector(arena &memory, std::size_t capacity, std::string_view name)
        : m_data(memory.allocate_storage<T>(capacity)), m_capacity(capacity), m_name(name) {}

    // No se copia: la copia necesitaría su propia memoria. Mover se lleva los elementos sin
    // tocarlos y deja el origen vacío y sin capacidad. Sin asignación: un vector se crea una
    // vez y no cambia de memoria.
    fixed_vector(const fixed_vector &) = delete;
    fixed_vector &operator=(const fixed_vector &) = delete;
    fixed_vector(fixed_vector &&other) noexcept
        : m_data(std::exchange(other.m_data, nullptr)), m_size(std::exchange(other.m_size, 0)),
          m_capacity(std::exchange(other.m_capacity, 0)), m_name(other.m_name) {}
    fixed_vector &operator=(fixed_vector &&) = delete;

    ~fixed_vector() { clear(); }

    // Construye el elemento al final, con los argumentos de su constructor, y lo devuelve.
    template <typename... Args> T &emplace_back(Args &&...args) {
        MZ_CHECK(m_size < m_capacity, "fixed_vector \"{}\" lleno: capacidad {}", m_name,
                 m_capacity);
        T *added = std::construct_at(m_data + m_size, std::forward<Args>(args)...);
        ++m_size;
        return *added;
    }
    void push_back(const T &value) { emplace_back(value); }
    void push_back(T &&value) { emplace_back(std::move(value)); }

    void pop_back() {
        MZ_ASSERT(m_size > 0, "fixed_vector \"{}\": pop_back sin elementos", m_name);
        --m_size;
        std::destroy_at(m_data + m_size);
    }

    // Destruye todos los elementos. La capacidad (y la memoria) quedan.
    void clear() {
        std::destroy_n(m_data, m_size);
        m_size = 0;
    }

    // El índice se revisa solo en Debug: en RelWithDebInfo y Release, acceder cuesta lo
    // mismo que en un arreglo de C.
    [[nodiscard]] T &operator[](std::size_t index) {
        check_index(index);
        return m_data[index];
    }
    [[nodiscard]] const T &operator[](std::size_t index) const {
        check_index(index);
        return m_data[index];
    }

    [[nodiscard]] std::size_t size() const { return m_size; }
    [[nodiscard]] std::size_t capacity() const { return m_capacity; }
    [[nodiscard]] bool empty() const { return m_size == 0; }
    [[nodiscard]] bool full() const { return m_size == m_capacity; }
    [[nodiscard]] std::string_view name() const { return m_name; }

    [[nodiscard]] T *data() { return m_data; }
    [[nodiscard]] const T *data() const { return m_data; }

    // Iteradores de puntero: el vector es un arreglo contiguo.
    [[nodiscard]] T *begin() { return m_data; }
    [[nodiscard]] T *end() { return m_data + m_size; }
    [[nodiscard]] const T *begin() const { return m_data; }
    [[nodiscard]] const T *end() const { return m_data + m_size; }

    // Los elementos (no la capacidad), para pasarlos a una función sin copiarlos.
    [[nodiscard]] std::span<T> span() { return {m_data, m_size}; }
    [[nodiscard]] std::span<const T> span() const { return {m_data, m_size}; }

private:
    void check_index(std::size_t index) const {
        MZ_ASSERT(index < m_size, "fixed_vector \"{}\": índice {} con {} elementos", m_name, index,
                  m_size);
    }

    T *m_data;
    std::size_t m_size = 0;
    std::size_t m_capacity;
    std::string_view m_name;
};

} // namespace mz::core
