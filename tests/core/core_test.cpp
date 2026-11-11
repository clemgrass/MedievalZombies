#include "mz/core/core.h"

#include <gtest/gtest.h>

namespace {

// game/main.cpp usa module_name() para comprobar que las capas enlazan.
TEST(CoreTest, ModuleNameIsCore) { EXPECT_EQ(mz::core::module_name(), "core"); }

} // namespace
