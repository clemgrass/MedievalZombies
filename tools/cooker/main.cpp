// mz_cook: cocina un glTF con una malla estática al asset del engine. Lo corre el build, una
// vez por asset (mz_cook_blend, en cmake/mz_cook.cmake).
//
//   mz_cook <entrada.glb> <salida.mzasset>
//
// Sale con 0 si escribió el asset. Si no, escribe en stderr qué falló y dónde, y sale con
// otro código: el build falla. Todo el trabajo está en tools_cooker, que prueban los tests.

#include "mz/cooker/error.h"
#include "mz/cooker/mesh.h"

#include <cstddef>
#include <cstdio>
#include <span>
#include <string>

int main(int argc, char **argv) {
    const std::span args{argv, static_cast<std::size_t>(argc)};
    if (args.size() != 3) {
        std::fputs("Uso: mz_cook <entrada.glb> <salida.mzasset>\n", stderr);
        return 2;
    }
    const std::string input = args[1];
    const auto cooked = mz::cooker::cook_mesh(input, args[2]);
    if (!cooked) {
        const std::string message = mz::cooker::format_error(input, cooked.error()) + "\n";
        std::fputs(message.c_str(), stderr);
        return 1;
    }
    return 0;
}
