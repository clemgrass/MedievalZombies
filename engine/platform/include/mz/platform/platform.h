#pragma once

#include <string_view>

namespace mz::platform {

/// Nombre del módulo. Relleno hasta que llegue la primera API real de la capa.
std::string_view module_name();

} // namespace mz::platform
