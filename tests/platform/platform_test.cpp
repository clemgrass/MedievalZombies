#include "mz/platform/platform.h"

#include <gtest/gtest.h>

namespace {

// game/main.cpp usa module_name() para comprobar que las capas enlazan.
TEST(PlatformTest, ModuleNameIsPlatform) { EXPECT_EQ(mz::platform::module_name(), "platform"); }

} // namespace
