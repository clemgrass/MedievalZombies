# Tests de las herramientas de análisis estático, de los chequeos del subconjunto de C++ y
# de la decisión de qué cambió un PR (changes.sh).
# Corren los mismos scripts de tools/ci/ que usa la CI sobre los fixtures de fixtures/,
# que rompen una regla a propósito o la cumplen.
#
# Igual que en tests/cmake/, los casos que deben fallar se verifican con
# PASS_REGULAR_EXPRESSION: tienen que fallar por el motivo esperado.

set(tools_dir ${PROJECT_SOURCE_DIR}/tools/ci)
set(fixtures ${CMAKE_CURRENT_SOURCE_DIR}/fixtures)

function(mz_tooling_test test_name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "EXPECT" "COMMAND")
    add_test(NAME ${test_name} COMMAND ${arg_COMMAND})
    set_tests_properties(${test_name} PROPERTIES LABELS tooling)
    if(arg_EXPECT)
        set_tests_properties(${test_name} PROPERTIES PASS_REGULAR_EXPRESSION "${arg_EXPECT}")
    endif()
endfunction()

# Formato: el chequeo encuentra el archivo mal formateado, y --fix lo arregla.
foreach(mode IN ITEMS detect fix)
    set(command ${CMAKE_COMMAND}
        -DSCRIPT=${tools_dir}/format.sh
        -DFIXTURE=${fixtures}/unformatted.cpp
        -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/format_${mode}
        -DMODE=${mode}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_format_case.cmake)
    if(mode STREQUAL "detect")
        mz_tooling_test(tooling.format.detect COMMAND ${command}
            EXPECT "unformatted\\.cpp:.*error: code should be clang-formatted")
    else()
        mz_tooling_test(tooling.format.fix COMMAND ${command})
    endif()
endforeach()

# Subconjunto de C++: cada regla falla con el mensaje que cita su decisión, y un archivo
# que solo nombra las reglas en comentarios pasa.
mz_tooling_test(tooling.subset.shared_ptr
    COMMAND bash ${tools_dir}/cpp-subset.sh ${fixtures}/shared_ptr.cpp
    EXPECT "shared_ptr\\.cpp:4: .*ADR-0009, Decisión 6")
mz_tooling_test(tooling.subset.coroutine
    COMMAND bash ${tools_dir}/cpp-subset.sh ${fixtures}/coroutine.cpp
    EXPECT "coroutine\\.cpp:8: .*ADR-0009, Decisión 7")
mz_tooling_test(tooling.subset.clean
    COMMAND bash ${tools_dir}/cpp-subset.sh ${fixtures}/subset_clean.cpp)

# Qué cambió un PR: de esta decisión dependen los jobs que la CI salta, y un salto cuenta
# como éxito. Cada caso es un repo git temporal donde el PR agrega un solo archivo.
function(mz_changes_test test_name changed_file expected)
    mz_tooling_test(${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DSCRIPT=${tools_dir}/changes.sh
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/${test_name}
            -DCHANGED_FILE=${changed_file}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_changes_case.cmake
        EXPECT "${expected}")
endfunction()

mz_changes_test(tooling.changes.docs_only docs/notas.md "code=false")
mz_changes_test(tooling.changes.engine engine/core/a.cpp "code=true")
# Un C++ fuera de las carpetas de código también cuenta: el formato revisa todo el C++.
mz_changes_test(tooling.changes.cpp_elsewhere tools/otra/a.cpp "code=true")
# Sin base (un push a main), todo cuenta como cambiado.
mz_tooling_test(tooling.changes.no_base
    COMMAND bash ${tools_dir}/changes.sh
    EXPECT "code=true")

# La meta de cobertura del 100 %: cada falta en una carpeta con meta se informa con su
# archivo y su línea, y lo que está fuera de la meta no cuenta. Los reportes lcov son
# fixtures escritos a mano, sobre el árbol falso de fixtures/coverage/.
function(mz_coverage_goal_test test_name lcov)
    mz_tooling_test(${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DMZ_CMAKE_DIR=${PROJECT_SOURCE_DIR}/cmake
            -DTREE=${fixtures}/coverage
            -DLCOV=${fixtures}/coverage/${lcov}
            -DGOAL=engine/core
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/${test_name}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_coverage_goal_case.cmake
        ${ARGN})
endfunction()

mz_coverage_goal_test(tooling.coverage.complete complete.lcov)
mz_coverage_goal_test(tooling.coverage.uncovered_line uncovered_line.lcov
    EXPECT "engine/core/src/math\\.cpp:3: línea sin cubrir")
mz_coverage_goal_test(tooling.coverage.untaken_branch untaken_branch.lcov
    EXPECT "engine/core/src/math\\.cpp:2: la condición 2 nunca fue falsa")
mz_coverage_goal_test(tooling.coverage.unevaluated_branch unevaluated_branch.lcov
    EXPECT "engine/core/src/math\\.cpp:2: la condición 1 nunca fue verdadera")
mz_coverage_goal_test(tooling.coverage.missing_file missing_file.lcov
    EXPECT "engine/core/src/math\\.cpp: no está en el reporte")

# clang-tidy: prueba la configuración del repo, no solo que la herramienta corre.
find_program(MZ_CLANG_TIDY clang-tidy)
if(NOT MZ_CLANG_TIDY)
    message(WARNING "No se encontró clang-tidy: los tests tooling.tidy.* no se registran.")
    return()
endif()

function(mz_tidy_test test_name fixture)
    mz_tooling_test(${test_name}
        COMMAND ${MZ_CLANG_TIDY} --config-file=${PROJECT_SOURCE_DIR}/.clang-tidy
            ${fixtures}/${fixture} -- -std=c++20 -fno-exceptions -fno-rtti
        ${ARGN})
endfunction()

mz_tidy_test(tooling.tidy.naming_type tidy_naming.cpp
    EXPECT "invalid case style for class 'MeshSlicer' \\[readability-identifier-naming,-warnings-as-errors\\]")
mz_tidy_test(tooling.tidy.naming_private_member tidy_naming.cpp
    EXPECT "invalid case style for member 'count' \\[readability-identifier-naming,-warnings-as-errors\\]")
mz_tidy_test(tooling.tidy.naming_public_member tidy_naming.cpp
    EXPECT "invalid case style for member 'x' \\[readability-identifier-naming,-warnings-as-errors\\]")
mz_tidy_test(tooling.tidy.bugprone tidy_bugprone.cpp
    EXPECT "\\[bugprone-use-after-move,-warnings-as-errors\\]")
mz_tidy_test(tooling.tidy.clean tidy_clean.cpp)
