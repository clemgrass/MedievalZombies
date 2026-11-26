# Tests del cocinador de assets. Por ahora, la primera etapa: la exportación .blend → .glb
# con el Blender fijado (cmake/mz_blender.cmake y tools/blender/export_glb.py). Corren con
# el mismo Blender que el build (MZ_BLENDER_EXECUTABLE), sobre fixtures/cubo.blend (lo
# genera fixtures/make_cubo.py).
#
# Igual que en tests/cmake/, los casos que deben fallar se verifican con
# PASS_REGULAR_EXPRESSION: tienen que fallar por el motivo esperado.

# El cubo exportado en el build: <build>/tests/tools/fixtures/cubo.glb.
mz_export_blend(tools_fixtures fixtures/cubo.blend)

function(mz_export_test mode)
    add_test(NAME tools.export.${mode}
        COMMAND ${CMAKE_COMMAND}
            -DMZ_CMAKE_DIR=${PROJECT_SOURCE_DIR}/cmake
            -DROOT=${PROJECT_SOURCE_DIR}
            -DBLENDER=${MZ_BLENDER_EXECUTABLE}
            -DFIXTURE=${CMAKE_CURRENT_SOURCE_DIR}/fixtures/cubo.blend
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/export_${mode}
            -DGENERATOR=${CMAKE_GENERATOR}
            -DMAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
            -DMODE=${mode}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_export_case.cmake
    )
    set_tests_properties(tools.export.${mode} PROPERTIES LABELS assets)
    if(ARGN)
        set_tests_properties(tools.export.${mode} PROPERTIES PASS_REGULAR_EXPRESSION "${ARGN}")
    endif()
endfunction()

mz_export_test(incremental)
mz_export_test(deterministic)
mz_export_test(wrong_version_configure
    "Se esperaba Blender [0-9.]+ y el binario dice: Blender 1\\.0\\.0 LTS")
mz_export_test(wrong_version_script
    "Se esperaba Blender 1\\.0\\.0 y este es Blender [0-9.]+")
