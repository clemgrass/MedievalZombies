#pragma once

#include <cstdint>

namespace mz::core {

// Referencia a un objeto que vive en un pool: el índice de su slot y la generación del slot
// cuando se entregó. El pool sube la generación cada vez que el slot se devuelve, así que un
// handle viejo (el objeto se destruyó y el slot se reusó) tiene el índice correcto pero la
// generación equivocada, y el pool lo detecta al usarlo. Un puntero, en cambio, seguiría
// apuntando al slot y leería el objeto nuevo sin avisar.
//
// T es solo una etiqueta: handle<mesh_piece> y handle<body> son tipos distintos, y T puede
// estar sin definir. La generación 0 está reservada para el handle nulo: un handle creado
// por defecto (o memoria en cero) nunca coincide con un objeto vivo, porque los pools
// empiezan cada slot en la generación 1.
template <typename T> class handle {
public:
    static constexpr std::uint32_t null_generation = 0;

    constexpr handle() = default;
    constexpr explicit handle(std::uint32_t index, std::uint32_t generation)
        : m_index(index), m_generation(generation) {}

    [[nodiscard]] constexpr std::uint32_t index() const { return m_index; }
    [[nodiscard]] constexpr std::uint32_t generation() const { return m_generation; }
    [[nodiscard]] constexpr bool is_null() const { return m_generation == null_generation; }

    // Dos handles son el mismo si tienen el mismo índice y la misma generación.
    constexpr bool operator==(const handle &) const = default;

private:
    std::uint32_t m_index = 0;
    std::uint32_t m_generation = null_generation;
};

} // namespace mz::core
