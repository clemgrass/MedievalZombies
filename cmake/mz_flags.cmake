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
    # == y != entre floats: se compara con nearly_equal (o exactly_equal, a propósito).
    -Wfloat-equal
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

# Instrumentación: sanitizers y cobertura. Las enciende el preset (debug y coverage), y
# las dos van tanto al compilar como al enlazar: el runtime de cada una se enlaza en el
# ejecutable. Los targets existen siempre y quedan vacíos con la opción apagada, para que
# quien los enlaza no tenga que preguntar.
option(MZ_SANITIZE "Compilar el código propio con ASan y UBSan" OFF)
option(MZ_COVERAGE "Compilar el código propio con la cobertura de Clang" OFF)

if(MZ_SANITIZE AND MZ_COVERAGE)
    message(FATAL_ERROR "MZ_SANITIZE y MZ_COVERAGE no van juntos: la cobertura se mide "
        "sin sanitizers, que solo la harían más lenta.")
endif()

# ASan: accesos fuera de rango, uso después de liberar y, con LSan incluido, fugas.
# UBSan: comportamiento indefinido (desborde de enteros con signo, shifts inválidos,
# punteros nulos, desalineados…). UBSan solo informa y deja seguir, salvo con
# -fno-sanitize-recover, que hace que cualquier error aborte el proceso. Los frame
# pointers dan stack traces completos en los reportes.
add_library(mz_sanitizers INTERFACE)
if(MZ_SANITIZE)
    set(sanitize_flags
        -fsanitize=address,undefined
        -fno-sanitize-recover=all
        -fno-omit-frame-pointer
    )
    target_compile_options(mz_sanitizers INTERFACE ${sanitize_flags})
    target_link_options(mz_sanitizers INTERFACE ${sanitize_flags})

    # El soporte de los sanitizers que va dentro de cada ejecutable (lo enlazan
    # mz_add_module y mz_add_test_executable): las supresiones de LSan para las fugas de
    # terceros, y los drivers de Vulkan cargados hasta el final para que LSan los
    # reconozca. Si cambia la lista, CMake vuelve a configurar.
    set(sanitizers_dir ${CMAKE_CURRENT_LIST_DIR}/../tools/sanitizers)
    set(MZ_LSAN_SUPPRESSIONS_FILE ${sanitizers_dir}/lsan.supp)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${MZ_LSAN_SUPPRESSIONS_FILE})
    file(READ ${MZ_LSAN_SUPPRESSIONS_FILE} MZ_LSAN_SUPPRESSIONS)
    configure_file(${sanitizers_dir}/sanitizer_support.cpp.in
        ${CMAKE_BINARY_DIR}/sanitizers/sanitizer_support.cpp @ONLY)
    add_library(mz_sanitizer_support OBJECT ${CMAKE_BINARY_DIR}/sanitizers/sanitizer_support.cpp)
    # Es código propio, aunque generado: se compila con las reglas de todos los targets.
    target_compile_features(mz_sanitizer_support PRIVATE cxx_std_20)
    set_target_properties(mz_sanitizer_support PROPERTIES
        CXX_EXTENSIONS OFF
        CXX_SCAN_FOR_MODULES OFF
        COMPILE_WARNING_AS_ERROR ON
    )
    target_link_libraries(mz_sanitizer_support PRIVATE mz_warnings mz_engine_flags)
endif()

# Cobertura de Clang: cada ejecutable escribe sus contadores en un .profraw, y llvm-cov
# los cruza con el mapa de cobertura que queda dentro del binario. Solo Clang tiene este
# formato.
#
# Modo continuo: normalmente el .profraw se escribe al salir con exit(), y un proceso que
# muere por abort() no deja nada. Es el caso del hijo de un death test, que ejecuta justo
# las ramas de falla de los assert. Con -fprofile-continuous, los contadores viven en el
# archivo mapeado en memoria, y se guardan aunque el proceso aborte. El patrón de
# LLVM_PROFILE_FILE tiene que empezar con %c para activarlo.
add_library(mz_coverage INTERFACE)
if(MZ_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "MZ_COVERAGE necesita Clang (usa llvm-cov).")
    endif()
    target_compile_options(mz_coverage INTERFACE
        -fprofile-instr-generate
        -fcoverage-mapping
        -fprofile-continuous
    )
    target_link_options(mz_coverage INTERFACE
        -fprofile-instr-generate
        -fprofile-continuous
    )
endif()
