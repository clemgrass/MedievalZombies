# El cocinado de assets en el build: cada .blend se exporta a .glb con el Blender fijado
# (cmake/mz_blender.cmake), y el .glb se cocina al binario que carga el juego con mz_cook
# (tools/cooker/).
#
#   mz_cook_blend(<target> <archivo.blend>...)
#
#   modelos/cubo.blend ──blender -b──▶ <build>/modelos/cubo.glb ──mz_cook──▶ <build>/modelos/cubo.mzasset
#
# Las rutas son relativas a la carpeta actual, y las salidas quedan en la misma ruta dentro
# del build. El target, que compila con ALL, agrupa los assets cocinados.
#
# Ninja rehace solo lo que cambió: tocar un .blend lo reexporta y lo recocina; cambiar el
# cocinador recocina todos los assets (puede cambiar lo que escribe), sin reexportar. Si el
# cocinador falla, el build falla con su mensaje, y la salida queda como estaba: mz_cook
# escribe a un temporal y lo renombra.
#
# El cocinador es el target mz_cook, que tiene que existir antes de llamar a la función. Un
# proyecto sin ese target (los proyectos de prueba de tests/tools/) da en
# MZ_COOK_EXECUTABLE la ruta de un mz_cook ya compilado.

include_guard(GLOBAL)

include(${CMAKE_CURRENT_LIST_DIR}/mz_blender.cmake)

function(mz_cook_blend target)
    # Con el nombre del target en COMMAND y en DEPENDS, CMake usa su ejecutable, lo compila
    # antes, y recocina cuando cambia.
    if(TARGET mz_cook)
        set(cooker mz_cook)
    elseif(MZ_COOK_EXECUTABLE)
        set(cooker ${MZ_COOK_EXECUTABLE})
    else()
        message(FATAL_ERROR "mz_cook_blend(${target}): no hay cocinador. Declara el target "
            "mz_cook antes, o da la ruta de uno compilado en MZ_COOK_EXECUTABLE.")
    endif()

    set(outputs)
    foreach(blend IN LISTS ARGN)
        _mz_export_blend(${blend} glb)
        cmake_path(REPLACE_EXTENSION glb LAST_ONLY .mzasset OUTPUT_VARIABLE asset)
        cmake_path(REPLACE_EXTENSION blend LAST_ONLY .mzasset OUTPUT_VARIABLE relative)
        add_custom_command(
            OUTPUT ${asset}
            COMMAND ${cooker} ${glb} ${asset}
            DEPENDS ${glb} ${cooker}
            COMMENT "Cocinando ${relative}"
            VERBATIM
        )
        list(APPEND outputs ${asset})
    endforeach()
    add_custom_target(${target} ALL DEPENDS ${outputs})
endfunction()
