#pragma once

#include <cstdint>

namespace mz {

float clamp01(float x);
std::uint64_t state_hash(int steps);

}  // namespace mz
