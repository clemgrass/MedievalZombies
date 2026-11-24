#pragma once

#include <string_view>

namespace mz::core {

/// Nombre del módulo. Relleno del primer ticket del build; lo prueba tests/core.
std::string_view module_name();

} // namespace mz::core
