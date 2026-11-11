# Prueba la meta de cobertura (cmake/mz_coverage_goal.cmake) sobre un reporte lcov
# escrito a mano, sin compilar ni correr nada.
#
# Se llama con `cmake -DMZ_CMAKE_DIR=<cmake/> -DTREE=<carpeta> -DLCOV=<fixture .lcov>
# -DGOAL=<carpetas separadas por |> -DWORK_DIR=<carpeta> -P run_coverage_goal_case.cmake`.
#
# TREE hace de raíz del repo: tiene los .cpp que la meta busca en disco. El fixture
# escribe sus rutas como @SOURCE_DIR@/..., porque llvm-cov las da absolutas, y aquí se
# reemplaza por TREE. Muestra cada falta y termina con error si hay alguna. El test lo
# decide su PASS_REGULAR_EXPRESSION.

include(${MZ_CMAKE_DIR}/mz_coverage_goal.cmake)

set(SOURCE_DIR ${TREE})
file(REMOVE_RECURSE ${WORK_DIR})
configure_file(${LCOV} ${WORK_DIR}/coverage.lcov @ONLY)

string(REPLACE "|" ";" goal "${GOAL}")
mz_coverage_goal_check(${WORK_DIR}/coverage.lcov ${TREE} "${goal}" failures)

foreach(failure IN LISTS failures)
    message("${failure}")
endforeach()
if(failures)
    message(FATAL_ERROR "No se cumple la meta del 100 %.")
endif()
