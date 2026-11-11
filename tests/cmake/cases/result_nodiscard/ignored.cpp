#include "mz/core/result.h"

enum class load_error { not_found };

mz::core::result<int, load_error> load() { return 1; }

// Ignora el result: el error, si lo hubo, se pierde sin que nadie lo mire.
void caller() { load(); }
