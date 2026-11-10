// Sigue las convenciones de nombres y de clang-tidy, y debe pasar sin diagnósticos. Todos
// los miembros llevan m_, también los públicos de un agregado.
#include <concepts>
#include <cstddef>
#include <cstdint>

#define MZ_STRINGIFY(text) #text

namespace mz::fixture {

constexpr std::size_t max_pieces = 64;
static_assert(max_pieces > 0);

struct slice_plane {
    float m_distance;
};

enum class slice_side : std::uint8_t { front, back };

template <std::floating_point Scalar> constexpr Scalar half(Scalar value) {
    return value / Scalar{2};
}

class piece_counter {
public:
    void add_piece() { ++m_count; }
    [[nodiscard]] std::size_t count() const { return m_count; }

private:
    std::size_t m_count = 0;
};

} // namespace mz::fixture
