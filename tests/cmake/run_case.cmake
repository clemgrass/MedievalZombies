# Configura y compila un caso de prueba del build, desde cero.
#
# Se llama con `cmake -D<VARIABLE>=<valor>... -P run_case.cmake`. Muestra toda la salida
# y termina con error si falla la configuración o la compilación.

file(REMOVE_RECURSE ${BINARY_DIR})

execute_process(
    COMMAND ${CMAKE_COMMAND}
        -S ${CASE_DIR}
        -B ${BINARY_DIR}
        -G ${GENERATOR}
        -DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}
        -DCMAKE_CXX_COMPILER=${CXX_COMPILER}
        -DCMAKE_LINKER_TYPE=${LINKER_TYPE}
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
        -DMZ_CMAKE_DIR=${MZ_CMAKE_DIR}
        ${CASE_OPTION}
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output
    RESULT_VARIABLE result
)
message("${output}")
if(NOT result EQUAL 0)
    message(FATAL_ERROR "La configuración del caso falló.")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${BINARY_DIR}
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output
    RESULT_VARIABLE result
)
message("${output}")
if(NOT result EQUAL 0)
    message(FATAL_ERROR "La compilación del caso falló.")
endif()
