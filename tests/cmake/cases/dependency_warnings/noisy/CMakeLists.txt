# Una dependencia de terceros, con código que no pasaría los warnings del proyecto.
add_library(noisy STATIC noisy.cpp)
target_include_directories(noisy PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
