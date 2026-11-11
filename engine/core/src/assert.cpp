#include "mz/core/assert.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <format>

namespace mz::core::detail {

namespace {

// El mensaje entero más lo que lo rodea, con margen para una ruta y una función largas.
constexpr std::size_t report_capacity = message_capacity + 1024;

} // namespace

void report_failure(std::string_view condition, std::string_view message, std::size_t full_size,
                    const std::source_location &where) {
    const std::string_view cut_mark = full_size > message.size() ? "[mensaje cortado]" : "";

    // Un solo bloque y una sola escritura: si otro hilo escribe en stderr, el reporte no
    // queda intercalado con lo suyo.
    std::array<char, report_capacity> report{};
    const auto written = std::format_to_n(
        report.data(), report.size(), "Falló: {}\n  {}{}\n  en {}:{}, {}\n", condition, message,
        cut_mark, where.file_name(), where.line(), where.function_name());
    std::fwrite(report.data(), 1, static_cast<std::size_t>(written.out - report.data()), stderr);

    // TODO(#122): antes de abortar, volcar el anillo del log.
    std::abort();
}

} // namespace mz::core::detail
