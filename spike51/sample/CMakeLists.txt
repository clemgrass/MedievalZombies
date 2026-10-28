cmake_minimum_required(VERSION 3.28)
project(mz_ci_sample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MZ_VENDOR_SDL3 "Descargar SDL3 con FetchContent" OFF)
option(MZ_WERROR "Warnings como errores" OFF)
option(MZ_SANITIZE "ASan + UBSan" OFF)
option(MZ_COVERAGE "Cobertura de Clang" OFF)

add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wnull-dereference
                    -fno-exceptions -fno-rtti -ffp-contract=off)
if(MZ_WERROR)
  add_compile_options(-Werror)
endif()
if(MZ_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()
if(MZ_COVERAGE)
  add_compile_options(-fprofile-instr-generate -fcoverage-mapping -fcoverage-mcdc)
  add_link_options(-fprofile-instr-generate)
endif()

if(MZ_VENDOR_SDL3)
  include(FetchContent)
  FetchContent_Declare(SDL3 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
                       GIT_TAG release-3.4.0 GIT_SHALLOW TRUE)
  set(SDL_SHARED OFF)
  set(SDL_STATIC ON)
  set(SDL_TEST_LIBRARY OFF)
  FetchContent_MakeAvailable(SDL3)
else()
  find_package(SDL3 REQUIRED CONFIG)
endif()

add_library(mz_core src/core.cpp)
target_include_directories(mz_core PUBLIC src)

add_executable(mz_sample src/main.cpp)
target_link_libraries(mz_sample PRIVATE mz_core SDL3::SDL3)

enable_testing()
find_package(GTest REQUIRED)
add_executable(core_tests tests/core_tests.cpp)
# gtest se compila con excepciones; los tests no heredan -fno-exceptions.
target_compile_options(core_tests PRIVATE -fexceptions -frtti)
target_link_libraries(core_tests PRIVATE mz_core GTest::gtest_main SDL3::SDL3)
include(GoogleTest)
gtest_discover_tests(core_tests PROPERTIES LABELS unit)

# Fuga deliberada: comprueba que LeakSanitizer funciona en el entorno.
add_executable(leak_probe tests/leak_probe.cpp)
add_test(NAME leak_probe COMMAND leak_probe)
set_tests_properties(leak_probe PROPERTIES WILL_FAIL TRUE LABELS sanitizer_probe)
