// Rompe la regla de la capa de plataforma: usa el renderer 2D de SDL, aunque sin incluir
// su header.
struct SDL_Window;
struct SDL_Renderer;

SDL_Renderer *make_renderer(SDL_Window *window);
