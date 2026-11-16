#pragma once

// Memoria de trabajo de una función: se toma al entrar y se devuelve entera al salir, como
// una pila. Por ejemplo, los temporales de un corte.
//
//   void slice(const mesh &m, arena &out, scratch_arenas &scratch) {
//       const scratch_scope temp = scratch.begin(out);
//       std::byte *distances = temp.memory().allocate(bytes, 16); // temporal
//       ...                                                       // el resultado, en out
//   } // temp vuelve a su marcador: solo se libera lo de esta función
//
// Son dos scratch y no una para el caso en que `out` también es una scratch: begin(out)
// entrega la otra. Si entregara la misma, volver al marcador al salir borraría también lo
// que la función escribió en `out` después de ese marcador.

#include "mz/core/arena.h"
#include "mz/core/region.h"

#include <utility>

namespace mz::core {

// Guarda el marcador de una arena al crearse y vuelve a él al destruirse. Sirve con
// cualquier arena. Los scopes de una misma arena se cierran en el orden inverso al que se
// abrieron (en C++, lo garantizan las llaves); si no, el rewind del que queda abierto
// aborta en Debug.
class scratch_scope {
public:
    explicit scratch_scope(arena &target) : m_arena(&target), m_mark(target.mark()) {}
    ~scratch_scope() { m_arena->rewind(m_mark); }

    // Ni se copia ni se mueve: dos dueños del mismo marcador volverían a él dos veces.
    // begin() lo devuelve igual, porque C++17 construye el resultado directamente en su
    // destino.
    scratch_scope(const scratch_scope &) = delete;
    scratch_scope &operator=(const scratch_scope &) = delete;
    scratch_scope(scratch_scope &&) = delete;
    scratch_scope &operator=(scratch_scope &&) = delete;

    // La arena donde pedir. Se puede pasar a otra función, que la ve como cualquier arena.
    [[nodiscard]] arena &memory() const { return *m_arena; }

private:
    arena *m_arena;
    arena::marker m_mark;
};

// Las dos arenas scratch del hilo principal. Se crean al iniciar y se pasan como parámetro
// a quien las necesita: no hay estado global.
class scratch_arenas {
public:
    scratch_arenas(region &&first, region &&second)
        : m_first(std::move(first)), m_second(std::move(second)) {}

    // Un scope sobre la scratch que no es `in_use`. Sin arena en uso, o con una que no es
    // scratch (la del paso, por ejemplo), sobre la primera.
    [[nodiscard]] scratch_scope begin(const arena &in_use) {
        return scratch_scope{&in_use == &m_first ? m_second : m_first};
    }
    [[nodiscard]] scratch_scope begin() { return scratch_scope{m_first}; }

    // Para leer sus contadores (uso y pico).
    [[nodiscard]] const arena &first() const { return m_first; }
    [[nodiscard]] const arena &second() const { return m_second; }

private:
    arena m_first;
    arena m_second;
};

} // namespace mz::core
