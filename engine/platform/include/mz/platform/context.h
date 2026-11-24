#pragma once

// context: SDL iniciada. Es lo primero que crea el juego y lo último que se destruye.
// Mientras vive, da la cola de eventos y el reloj; la ventana se crea a partir de él.
//
//   auto platform = context::create();
//   if (!platform) {
//       std::fprintf(stderr, "%s\n", last_error().data());
//   }
//
// Hay un solo context a la vez: SDL es un estado global del proceso, y al destruirse el
// context cierra todo SDL. Solo se mueve, nunca se copia.

#include "mz/platform/event.h"

#include <cstdint>
#include <optional>
#include <string_view>

namespace mz::platform {

class context {
public:
    // Inicia el video de SDL (y con él, la cola de eventos). Vacío si el sistema no tiene
    // un driver de video que funcione; el motivo queda en last_error(). Crear un segundo
    // context mientras vive otro es un error del programador: aborta.
    [[nodiscard]] static std::optional<context> create();

    context(const context &) = delete;
    context &operator=(const context &) = delete;
    // Mover pasa la propiedad: el context de origen queda vacío y no cierra SDL.
    context(context &&other) noexcept;
    context &operator=(context &&) = delete;
    // Cierra SDL. Destruir el context con una ventana viva aborta: SDL la cerraría por
    // debajo, y el destructor de la ventana trabajaría sobre memoria liberada.
    ~context();

    // Saca de la cola el siguiente evento que el engine usa, sin esperar. Los que no usa
    // los descarta por el camino. Vacío si la cola se terminó.
    [[nodiscard]] std::optional<event> poll_event();

    // Igual, pero si la cola está vacía espera hasta `timeout_ms` a que llegue algo. Puede
    // volver vacío antes de tiempo, si lo que llegó fue un evento que el engine no usa.
    [[nodiscard]] std::optional<event> wait_event(std::int32_t timeout_ms);

    // El reloj del engine: nanosegundos monotónicos (nunca retroceden; no los mueve el
    // ajuste de la hora del sistema) contados desde que se creó el context. Es la misma
    // base de tiempo que la de los eventos. Al cerrar SDL el origen se pierde: un context
    // nuevo vuelve a contar desde cero.
    [[nodiscard]] std::uint64_t now_ns() const;

private:
    context() = default;

    bool m_owns_sdl = true;
};

// El último error de SDL en este hilo, como texto. Sirve justo después de una creación que
// devolvió vacío.
[[nodiscard]] std::string_view last_error();

} // namespace mz::platform
