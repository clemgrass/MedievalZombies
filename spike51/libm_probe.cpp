// Un solo binario, corrido en varias glibc: lo único que cambia entre corridas es la libm.
// Hash FNV-1a de los bits de cada resultado, por función, sobre 2^24 entradas.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {

std::uint64_t mix(std::uint64_t h, float v) {
    std::uint32_t b;
    std::memcpy(&b, &v, sizeof b);
    return (h ^ b) * 1099511628211ull;
}

constexpr int kN = 1 << 24;

// Entrada i-ésima en [lo, hi), sin acumular error.
float at(int i, float lo, float hi) {
    return lo + (hi - lo) * (static_cast<float>(i) / static_cast<float>(kN));
}

}  // namespace

int main() {
    struct probe {
        const char* name;
        float lo, hi;
        float (*f)(float);
    };
    const probe probes[] = {
        {"sinf", -100.0f, 100.0f, [](float x) { return std::sin(x); }},
        {"cosf", -100.0f, 100.0f, [](float x) { return std::cos(x); }},
        {"expf", -20.0f, 20.0f, [](float x) { return std::exp(x); }},
        {"logf", 1e-3f, 1e3f, [](float x) { return std::log(x); }},
        {"powf", 0.01f, 10.0f, [](float x) { return std::pow(x, 1.7f); }},
        {"acosf", -1.0f, 1.0f, [](float x) { return std::acos(x); }},
        {"atan2f", -10.0f, 10.0f, [](float x) { return std::atan2(x, 0.37f); }},
        {"tanf", -1.5f, 1.5f, [](float x) { return std::tan(x); }},
        {"sqrtf", 0.0f, 1e4f, [](float x) { return std::sqrt(x); }},
    };
    for (const probe& p : probes) {
        std::uint64_t h = 1469598103934665603ull;
        for (int i = 0; i < kN; ++i) {
            h = mix(h, p.f(at(i, p.lo, p.hi)));
        }
        std::printf("%-7s %016llx\n", p.name, static_cast<unsigned long long>(h));
    }
    return 0;
}
