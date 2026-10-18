cmake_minimum_required(VERSION 3.28)
project(MedievalZombies LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -O3 -ffast-math -march=native)

add_library(mz_core INTERFACE)
target_include_directories(mz_core INTERFACE engine)
