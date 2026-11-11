#include "mz/core/core.h"

namespace mz::core {

std::string_view module_name() { return "core"; }

// Prueba desechable: una línea sin tests y un hallazgo de clang-tidy.
struct uncovered {
    int count = 0;
};

int uncovered_function(int x) { return x > 0 ? 1 : 0; }

} // namespace mz::core
