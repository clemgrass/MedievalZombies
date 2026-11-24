// Rompe la regla de la capa de plataforma: incluye SDL fuera de engine/platform.
#include <cstdint>

#include <SDL3/SDL_video.h>

std::uint32_t pixel_count(int width, int height) { return width * height; }
