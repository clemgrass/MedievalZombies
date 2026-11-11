# Casos de prueba del build. Cada caso es un proyecto mínimo en cases/<caso> que usa las
# mismas funciones de cmake/ que el proyecto real. run_case.cmake lo configura y lo
# compila con el mismo compilador y el mismo linker, y muestra toda la salida.
#
# Los casos que deben fallar se verifican con PASS_REGULAR_EXPRESSION: no basta con que
# fallen, tienen que fallar por el motivo esperado.

function(mz_add_cmake_case test_name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "CASE;BUILD_TYPE;OPTION;EXPECT" "")
    if(NOT arg_BUILD_TYPE)
        set(arg_BUILD_TYPE Debug)
    endif()

    add_test(NAME ${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DCASE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/cases/${arg_CASE}
            -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/cases/${test_name}
            -DMZ_CMAKE_DIR=${PROJECT_SOURCE_DIR}/cmake
            -DGENERATOR=${CMAKE_GENERATOR}
            -DMAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
            -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DLINKER_TYPE=${CMAKE_LINKER_TYPE}
            -DBUILD_TYPE=${arg_BUILD_TYPE}
            -DCASE_OPTION=${arg_OPTION}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_case.cmake
    )
    set_tests_properties(${test_name} PROPERTIES LABELS cmake)
    if(arg_EXPECT)
        set_tests_properties(${test_name} PROPERTIES PASS_REGULAR_EXPRESSION "${arg_EXPECT}")
    endif()
endfunction()

# Regla de capas: hacia arriba y dentro de la misma capa.
mz_add_cmake_case(cmake.layers.upward CASE layers_upward
    EXPECT "engine_core \\(capa core\\) enlaza con engine_gpu \\(capa gpu\\)")
mz_add_cmake_case(cmake.layers.same_layer CASE layers_same
    EXPECT "engine_slicing \\(capa system\\) enlaza con engine_physics \\(capa system\\)")
mz_add_cmake_case(cmake.layers.manual_link CASE layers_manual_link
    EXPECT "engine_platform \\(capa platform\\) enlaza con engine_core \\(capa core\\)")

# Subconjunto de C++. Los mensajes cubren Clang y GCC. GCC cita con comillas tipográficas
# (‘typeid’) si el locale es UTF-8, y con ' en el locale C del contenedor: la regex
# acepta cualquiera de las dos.
mz_add_cmake_case(cmake.flags.no_exceptions CASE no_exceptions
    EXPECT "exceptions disabled|exception handling disabled")
mz_add_cmake_case(cmake.flags.no_rtti CASE no_rtti
    EXPECT "typeid requires -frtti|cannot use .+typeid.+ with .+-fno-rtti")

# Un warning del código propio rompe el build en las tres configuraciones.
foreach(build_type IN ITEMS Debug RelWithDebInfo Release)
    string(TOLOWER ${build_type} suffix)
    mz_add_cmake_case(cmake.warnings.error_${suffix} CASE warning_is_error
        BUILD_TYPE ${build_type}
        EXPECT "\\[-Werror[,=]")
endforeach()

# Un warning que solo da GCC también rompe el build de GCC. Con Clang ese código compila,
# así que el caso solo existe en el build de GCC (el preset debug-gcc).
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    mz_add_cmake_case(cmake.warnings.gcc_only CASE gcc_only_warning
        EXPECT "shadow\\.cpp.*\\[-Werror=shadow\\]")
endif()

# Una dependencia con warnings compila, porque los warnings son por target. El caso de
# control aplica las mismas banderas en forma global y comprueba que ahí sí falla.
mz_add_cmake_case(cmake.warnings.dependency_isolated CASE dependency_warnings)
mz_add_cmake_case(cmake.warnings.dependency_global_flags CASE dependency_warnings
    OPTION -DMZ_GLOBAL_WARNINGS=ON
    EXPECT "noisy\\.cpp.*\\[-Werror[,=]")
