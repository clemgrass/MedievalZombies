# Soporte de los tests: el main propio y el generador de los tests de propiedades.
add_library(mz_test_support STATIC src/main.cpp src/property.cpp)
target_include_directories(mz_test_support PUBLIC include)
target_compile_features(mz_test_support PUBLIC cxx_std_20)
set_target_properties(mz_test_support PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_SCAN_FOR_MODULES OFF
    COMPILE_WARNING_AS_ERROR ON
)
target_link_libraries(mz_test_support
    PUBLIC GTest::gtest
    PRIVATE mz_warnings mz_engine_flags mz_sanitizers mz_coverage
)

mz_add_test_suite(support SOURCES property_test.cpp)

# Una falla a propósito, fuera de la suite: comprueba que un test de propiedades que falla
# imprime su semilla y que esa semilla reproduce el mismo contraejemplo. Con la semilla 42,
# el primer número del generador es 0xa15c02b7 = 2707161783.
mz_add_test_executable(tests_support_failing SOURCES failing_property.cpp)
add_test(NAME support.property.prints_seed_and_reproduces
    COMMAND tests_support_failing --seed=42)
set_tests_properties(support.property.prints_seed_and_reproduces PROPERTIES
    LABELS "support;property"
    PASS_REGULAR_EXPRESSION
        "value = 2707161783.*semilla: 42 \\(reproducir: [^ ]*tests_support_failing --gtest_filter=FailingProperty\\.AlwaysBelowHalf --seed=42\\)"
)
