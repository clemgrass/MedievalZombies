// El main de todos los ejecutables de tests. Hace lo mismo que gtest_main, más la
// bandera --seed=N de los tests de propiedades.

#include "mz/test/property.h"

#include <gtest/gtest.h>

#include <charconv>
#include <cstdint>
#include <cstdio>
#include <string_view>
#include <system_error>

namespace {

constexpr std::string_view seed_flag = "--seed=";

// Lee el número de --seed=N, en decimal. Devuelve false si no es un entero sin signo de
// 64 bits completo ("12abc" y "" no valen).
bool parse_seed(std::string_view text, std::uint64_t &seed) {
    const char *first = text.data();
    const char *last = text.data() + text.size();
    const std::from_chars_result result = std::from_chars(first, last, seed);
    return result.ec == std::errc{} && result.ptr == last && first != last;
}

} // namespace

int main(int argc, char **argv) {
    // Un death test corre su sentencia en un proceso hijo. Con "threadsafe", el hijo
    // vuelve a ejecutar el binario desde cero en vez de seguir desde el fork(), que con
    // varios hilos vivos (los del engine o los del runtime de un sanitizer) puede quedar
    // trabado en un mutex que otro hilo tenía tomado. Va antes de InitGoogleTest para que
    // --gtest_death_test_style en la línea de comandos todavía pueda cambiarlo.
    GTEST_FLAG_SET(death_test_style, "threadsafe");

    // Lee las banderas --gtest_* y las saca de argv. Lo que queda es nuestro.
    ::testing::InitGoogleTest(&argc, argv);
    mz::test::set_executable_name(argv[0]);

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        std::uint64_t seed = 0;
        if (arg.starts_with(seed_flag) && parse_seed(arg.substr(seed_flag.size()), seed)) {
            mz::test::set_seed_override(seed);
        } else {
            std::fprintf(stderr, "Argumento desconocido: %s\nUso: %s [--gtest_*] [--seed=N]\n",
                         argv[i], argv[0]);
            return 2;
        }
    }

    return RUN_ALL_TESTS();
}
