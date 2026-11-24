// El soporte de los sanitizers que cada ejecutable del preset debug lleva adentro. Solo se
// compila con MZ_SANITIZE: sin sanitizers, ese soporte no existe.

#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

// NOLINTBEGIN(bugprone-reserved-identifier,readability-identifier-naming): el nombre lo
// fija LeakSanitizer.
extern "C" const char *__lsan_default_suppressions();
// NOLINTEND(bugprone-reserved-identifier,readability-identifier-naming)

namespace {

// Las supresiones que LSan recibe son las de tools/sanitizers/lsan.supp, completas: si
// alguien edita la lista y CMake no la vuelve a incrustar, este test lo ve.
TEST(SanitizerSupportTest, LeakSuppressionsAreTheListInTheRepo) {
    std::ifstream file{MZ_LSAN_SUPPRESSIONS_FILE};
    ASSERT_TRUE(file.is_open()) << MZ_LSAN_SUPPRESSIONS_FILE;
    const std::string listed{std::istreambuf_iterator<char>{file},
                             std::istreambuf_iterator<char>{}};
    EXPECT_EQ(std::string_view{__lsan_default_suppressions()}, listed);
}

// La variable ya está puesta al entrar a main: el constructor corre antes, y antes de que
// nadie cargue el loader de Vulkan.
TEST(SanitizerSupportTest, VulkanDriversStayLoadedUntilExit) {
    const char *value = std::getenv("VK_LOADER_DISABLE_DYNAMIC_LIBRARY_UNLOADING");
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(std::string_view{value}, "1");
}

} // namespace
