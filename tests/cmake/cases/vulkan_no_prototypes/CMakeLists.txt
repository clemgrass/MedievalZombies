cmake_minimum_required(VERSION 3.29)
project(vulkan_no_prototypes LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${MZ_CMAKE_DIR})
include(mz_module)
include(mz_vulkan)

mz_add_module(engine_gpu LAYER gpu
    SOURCES direct_call.cpp
    PRIVATE_DEPS mz_vulkan_headers
)
