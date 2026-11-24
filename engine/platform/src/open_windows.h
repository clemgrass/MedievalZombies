#pragma once

// La cuenta de ventanas abiertas, compartida entre window_sdl.cpp y context_sdl.cpp: el
// context no puede cerrar SDL mientras quede una ventana viva.

namespace mz::platform::detail {

void window_opened();
void window_closed();

} // namespace mz::platform::detail
