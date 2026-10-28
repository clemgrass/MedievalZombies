#include "core.hpp"

#include <cmath>

namespace mz {

float clamp01(float x) {
    if (x < 0.0f) {
        return 0.0f;
    }
    if (x > 1.0f) {
        return 1.0f;
    }
    return x;
}

std::uint64_t state_hash(int steps) {
    std::uint64_t h = 1469598103934665603ull;
    float a = 0.1f;
    for (int i = 0; i < steps; ++i) {
        float v = std::sin(a) + std::cos(a * 1.3f) + std::exp(a * 0.01f) + std::atan2(a, 1.7f) +
                  std::acos(clamp01(a * 0.05f));
        std::uint32_t bits;
        __builtin_memcpy(&bits, &v, sizeof bits);
        h = (h ^ bits) * 1099511628211ull;
        a += 0.37f;
        if (a > 20.0f) {
            a -= 20.0f;
        }
    }
    return h;
}

}  // namespace mz
