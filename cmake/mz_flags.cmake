# Banderas de compilación del código propio, como targets de interfaz.
#
# Solo los enlazan los targets propios (a través de mz_add_module). Nunca se usan con
# add_compile_options: esas banderas son globales y llegarían también a las dependencias.

include_guard(GLOBAL)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    message(FATAL_ERROR
        "Compilador no soportado: ${CMAKE_CXX_COMPILER_ID}. El proyecto usa Clang o GCC.")
endif()

# Warnings del código propio. El -Werror no va aquí: lo pone la propiedad
# COMPILE_WARNING_AS_ERROR de cada target, que se puede apagar al experimentar con
# `cmake --preset <preset> --compile-no-warning-as-error`.
add_library(mz_warnings INTERFACE)
target_compile_options(mz_warnings INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Wold-style-cast
    -Wdouble-promotion
    -Wnull-dereference
    -Wimplicit-fallthrough
    -Wnon-virtual-dtor
    -Wcast-align
    -Wformat=2
)

# Subconjunto de C++: sin excepciones y sin RTTI.
add_library(mz_engine_flags INTERFACE)
target_compile_options(mz_engine_flags INTERFACE
    -fno-exceptions
    -fno-rtti
)
