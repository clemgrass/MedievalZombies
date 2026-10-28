#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include "core.hpp"

TEST(clamp01, below_zero_returns_zero) { EXPECT_EQ(mz::clamp01(-1.0f), 0.0f); }
TEST(clamp01, above_one_returns_one) { EXPECT_EQ(mz::clamp01(2.0f), 1.0f); }
TEST(clamp01, inside_returns_same) { EXPECT_EQ(mz::clamp01(0.5f), 0.5f); }

TEST(state_hash, same_process_twice_is_equal) {
    EXPECT_EQ(mz::state_hash(10'000), mz::state_hash(10'000));
}

TEST(sdl3, init_without_subsystems) {
    ASSERT_TRUE(SDL_Init(0));
    SDL_Quit();
}
