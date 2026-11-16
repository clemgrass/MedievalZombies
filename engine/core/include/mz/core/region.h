#pragma once

// region: un bloque de memoria con nombre, reservado al iniciar y tocado entero en ese
// momento (lo pide a platform::map_populated). Es la pieza de abajo de la memoria del
// engine: las arenas y los pools se arman encima de una región, y en el frame nunca se le
// pide memoria al sistema operativo.
//
//   auto step = region::create("arena del paso", 4 << 20);
//   if (!step) {
//       // El sistema operativo no tiene esa memoria: falla al arrancar, no en una horda.
//   }
//
// La región es la dueña del bloque: lo devuelve al sistema operativo al destruirse. Solo se
// mueve, nunca se copia.

#include "mz/core/result.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace mz::core {

// Por qué no se pudo crear una región. Un tamaño 0 o un nombre vacío no están aquí: son
// errores del programador y abortan (MZ_ASSERT).
enum class region_error : std::uint8_t {
    // El sistema operativo no puede dar ese tamaño: no cabe en el espacio de direcciones, o
    // es más de lo que acepta prometer. Depende de la máquina del jugador.
    out_of_memory,
};

class region {
public:
    // Reserva `size` bytes alineados a página, con todas sus páginas ya en la RAM. El
    // nombre aparece en los mensajes de las arenas y los pools; tiene que vivir más que la
    // región (un literal).
    //
    // Definida aquí, con el tipo de retorno deducido: dentro de la clase, region todavía
    // es un tipo incompleto, y nombrar result<region, region_error> en una declaración
    // comprobaría que region se puede mover antes de que se sepa. El cuerpo de una
    // función de la clase, en cambio, se compila con la clase ya completa.
    [[nodiscard]] static auto create(std::string_view name, std::size_t size) {
        using created = result<region, region_error>;
        region block = reserve(name, size);
        if (block.m_data == nullptr) {
            return created{unexpected{region_error::out_of_memory}};
        }
        return created{std::move(block)};
    }

    region(const region &) = delete;
    region &operator=(const region &) = delete;
    // Mover deja la región de origen vacía: sin datos, tamaño 0 y sin nombre.
    region(region &&other) noexcept;
    // Sin asignación por movimiento: una región se crea una vez, al iniciar, y no cambia
    // de bloque.
    region &operator=(region &&) = delete;
    ~region();

    [[nodiscard]] std::string_view name() const { return m_name; }
    [[nodiscard]] std::byte *data() { return m_data; }
    [[nodiscard]] const std::byte *data() const { return m_data; }
    // El tamaño pedido. El bloque real llega hasta el final de su última página, pero eso
    // no es de la región.
    [[nodiscard]] std::size_t size() const { return m_size; }

private:
    region(std::string_view name, std::byte *data, std::size_t size);

    // Pide el bloque a platform. Devuelve una región vacía (sin datos) si el sistema
    // operativo no lo da. Vive en region.cpp: el header de core no incluye los de platform.
    static region reserve(std::string_view name, std::size_t size);

    std::string_view m_name;
    std::byte *m_data;
    std::size_t m_size;
};

} // namespace mz::core
