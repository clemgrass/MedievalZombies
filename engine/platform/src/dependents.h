#pragma once

// Lo que vive encima de SDL y el context tiene que esperar: las ventanas abiertas y el
// loader de Vulkan cargado. Lo comparten window_sdl.cpp, vulkan_sdl.cpp y context_sdl.cpp:
// el context no puede cerrar SDL mientras quede uno vivo.

namespace mz::platform::detail {

void window_opened();
void window_closed();

void vulkan_loaded();
void vulkan_unloaded();

} // namespace mz::platform::detail
