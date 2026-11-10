#include "mz/core/core.h"
#include "mz/gpu/gpu.h"
#include "mz/platform/platform.h"

int main() {
    // Usa un símbolo de cada capa para que el linker de verdad tenga que resolverlas.
    const bool linked = !mz::platform::module_name().empty() && !mz::core::module_name().empty() &&
                        !mz::gpu::module_name().empty();
    return linked ? 0 : 1;
}
