# Tests del cocinador de assets.
#
#   - tests_tools (GoogleTest): la biblioteca del cocinador (tools/cooker), sobre los .gltf
#     chicos de fixtures/ (los genera fixtures/make_gltf.py) y sobre el cubo que cocina el
#     build.
#   - tools.export.* y tools.cook.*: la cadena del build (cmake/mz_blender.cmake,
#     cmake/mz_cook.cmake, tools/blender/export_glb.py) y el ejecutable mz_cook, con
#     run_export_case.cmake. Corren con el mismo Blender que el build
#     (MZ_BLENDER_EXECUTABLE), sobre fixtures/cubo.blend (lo genera fixtures/make_cubo.py).
#
# Igual que en tests/cmake/, los casos que deben fallar se verifican con
# PASS_REGULAR_EXPRESSION: tienen que fallar por el motivo esperado.

# El cubo exportado y cocinado en el build: <build>/tests/tools/fixtures/cubo.glb y
# cubo.mzasset.
mz_cook_blend(tools_fixtures fixtures/cubo.blend)

mz_add_test_suite(tools
    SOURCES
        cooker_error_test.cpp
        cooker_file_test.cpp
        cooker_mesh_test.cpp
    DEPS tools_cooker
)
set(output_dir ${CMAKE_CURRENT_BINARY_DIR}/cooker_output)
file(MAKE_DIRECTORY ${output_dir})
target_compile_definitions(tests_tools PRIVATE
    MZ_COOKER_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
    MZ_COOKER_BUILT_FIXTURES="${CMAKE_CURRENT_BINARY_DIR}/fixtures"
    MZ_COOKER_OUTPUT="${output_dir}"
)
# Los tests leen el cubo cocinado: se cocina antes de compilarlos.
add_dependencies(tests_tools tools_fixtures)

function(mz_asset_case group mode)
    add_test(NAME tools.${group}.${mode}
        COMMAND ${CMAKE_COMMAND}
            -DMZ_CMAKE_DIR=${PROJECT_SOURCE_DIR}/cmake
            -DROOT=${PROJECT_SOURCE_DIR}
            -DBLENDER=${MZ_BLENDER_EXECUTABLE}
            -DCOOK=$<TARGET_FILE:mz_cook>
            -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/fixtures
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/${group}_${mode}
            -DGENERATOR=${CMAKE_GENERATOR}
            -DMAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
            -DMODE=${mode}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_export_case.cmake
    )
    set_tests_properties(tools.${group}.${mode} PROPERTIES LABELS assets)
    if(ARGN)
        set_tests_properties(tools.${group}.${mode} PROPERTIES PASS_REGULAR_EXPRESSION "${ARGN}")
    endif()
endfunction()

mz_asset_case(export incremental)
mz_asset_case(export deterministic)
mz_asset_case(export wrong_version_configure
    "Se esperaba Blender [0-9.]+ y el binario dice: Blender 1\\.0\\.0 LTS")
mz_asset_case(export wrong_version_script
    "Se esperaba Blender 1\\.0\\.0 y este es Blender [0-9.]+")
mz_asset_case(cook invalid
    "sin_normales\\.gltf: SinNormales: a una primitiva le faltan las normales \\(NORMAL\\)")
mz_asset_case(cook usage "Uso: mz_cook <entrada\\.glb> <salida\\.mzasset>")
