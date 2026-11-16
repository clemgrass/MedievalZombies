cmake_minimum_required(VERSION 3.29)
project(float_equal LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${MZ_CMAKE_DIR})
include(mz_module)

mz_add_module(engine_core LAYER core
    SOURCES compared.cpp
)
