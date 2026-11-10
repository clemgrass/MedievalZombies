cmake_minimum_required(VERSION 3.29)
project(warning_is_error LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${MZ_CMAKE_DIR})
include(mz_module)

mz_add_module(engine_core LAYER core
    SOURCES narrow.cpp
)
