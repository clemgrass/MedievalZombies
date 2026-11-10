#include "mz/core/core.h"

#include <gtest/gtest.h>

namespace {

// Ejemplo hasta que llegue la primera API real de engine_core.
TEST(CoreTest, ModuleNameIsCore) { EXPECT_EQ(mz::core::module_name(), "core"); }

} // namespace
