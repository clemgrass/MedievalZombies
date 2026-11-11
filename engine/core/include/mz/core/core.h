#pragma once

#include <string_view>

namespace mz::core {

/// Nombre del módulo. game/main.cpp lo usa para comprobar que las capas enlazan.
std::string_view module_name();

} // namespace mz::core
