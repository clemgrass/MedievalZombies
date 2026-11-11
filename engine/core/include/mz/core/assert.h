#pragma once

// Asserts del engine, para los errores del programador: un índice fuera de rango, un handle
// de algo ya destruido, una región de memoria que se quedó chica. Al fallar imprimen en
// stderr la condición, el mensaje, el archivo, la línea y la función, y abortan.
//
//   MZ_ASSERT(cond, "formato", args...)   solo sin NDEBUG: Debug. CMake define NDEBUG en
//                                         RelWithDebInfo y en Release, y ahí no evalúa nada.
//   MZ_CHECK(cond, "formato", args...)    en todos los builds, para los contratos que
//                                         tienen que valer también en Release.
//
// El mensaje es obligatorio y usa el formato de std::format, revisado al compilar. Los
// errores esperables (un archivo que no existe) no son asserts: son un result<T, E>.

#include <array>
#include <cstddef>
#include <format>
#include <source_location>
#include <string_view>
#include <utility>

namespace mz::core::detail {

// Lo que cabe del mensaje formateado. Si no cabe, se corta y se avisa: un assert puede
// saltar justo porque se acabó la memoria, así que el mensaje no la pide.
inline constexpr std::size_t message_capacity = 1024;

// El camino de toda falla: arma el reporte, lo escribe en stderr y aborta. `message` es lo
// que cupo en el buffer y `full_size`, lo que medía el mensaje entero.
[[noreturn]] void report_failure(std::string_view condition, std::string_view message,
                                 std::size_t full_size, const std::source_location &where);

// Formatea el mensaje en la pila y llama a report_failure. Fría y fuera de línea: el código
// de formateo no se mezcla con el código que llama al assert, que solo paga el if.
template <typename... Args>
[[noreturn, gnu::cold, gnu::noinline]] void
fail(std::string_view condition, const std::source_location &where,
     std::format_string<Args...> format, Args &&...args) {
    // format_to_n escribe como mucho buffer.size() caracteres, pero cuenta en `size` los que
    // habría escrito: si es más grande, el mensaje se cortó.
    std::array<char, message_capacity> buffer{};
    const auto written =
        std::format_to_n(buffer.data(), buffer.size(), format, std::forward<Args>(args)...);
    const auto fitted = static_cast<std::size_t>(written.out - buffer.data());
    report_failure(condition, std::string_view(buffer.data(), fitted),
                   static_cast<std::size_t>(written.size), where);
}

} // namespace mz::core::detail

// Una expresión y no un bloque: pide su punto y coma y funciona dentro de un if sin llaves.
// El clásico do { } while (false) haría lo mismo, pero la cobertura cuenta su while (false)
// como una rama que nunca es verdadera, en cada assert. Que la rama de la falla es rara lo
// sabe el compilador porque fail es fría.
#define MZ_CHECK(condition, format, ...)                                                           \
    (static_cast<bool>(condition)                                                                  \
         ? static_cast<void>(0)                                                                    \
         : ::mz::core::detail::fail(#condition, ::std::source_location::current(),                 \
                                    format __VA_OPT__(, ) __VA_ARGS__))

#ifdef NDEBUG
// Apagado: decltype mira el tipo de la expresión sin evaluarla (es void), así que queda
// static_cast<void>(0), que no hace nada. Pero la condición y los argumentos se siguen
// compilando: un error de tipos en un assert aparece también en Release, y una variable que
// solo usa el assert no da el warning de "sin usar".
#define MZ_ASSERT(condition, format, ...)                                                          \
    static_cast<decltype(MZ_CHECK(condition, format __VA_OPT__(, ) __VA_ARGS__))>(0)
#else
#define MZ_ASSERT(condition, format, ...) MZ_CHECK(condition, format __VA_OPT__(, ) __VA_ARGS__)
#endif
