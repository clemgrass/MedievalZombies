# Blender en el build: el repo guarda solo el .blend, y el build lo
# exporta a .glb con `blender -b`, con la versión fijada en tools/ci/blender-version.
#
# Al configurar:
#
#   - el binario sale de MZ_BLENDER, si se da: un Blender oficial de la misma versión en
#     otra carpeta, del que solo se compara la versión. Si no, de
#     tools/ci/install-blender.sh, que lo instala desde el tarball oficial la primera vez
#     (366 MiB), con su sha256, y después usa su caché. Nunca se usa el blender del PATH:
#     el de Arch es otro build, y sigue otra versión;
#   - se verifica que el binario sea la versión fijada, o la configuración falla.
#
# Deja MZ_BLENDER_EXECUTABLE y MZ_BLENDER_VERSION, y la función interna:
#
#   _mz_export_blend(<archivo.blend> <variable>)
#
# Agrega la regla que exporta el .blend (con su ruta relativa a la carpeta actual) al .glb
# de la misma ruta en la carpeta del build, modelos/cubo.blend → <build>/modelos/cubo.glb, y
# deja la ruta del .glb en <variable>. La usa mz_cook_blend (cmake/mz_cook.cmake), que
# encadena la exportación con el cocinado.
#
# Cada .glb se reexporta cuando cambia su .blend, el script de exportación o el pin:
# Ninja compara la fecha de cada salida con las de sus dependencias, y guarda el comando en
# .ninja_log, así que cambiar un flag también reexporta. Lo que el .blend enlaza (otro
# .blend como biblioteca, una textura externa) no es una dependencia todavía: cuando un
# asset lo necesite, el script puede escribir un DEPFILE con bpy.data.libraries y las
# imágenes.

include_guard(GLOBAL)

set(MZ_BLENDER "" CACHE FILEPATH
    "Un Blender oficial de la versión fijada, en otra carpeta (no verifica el sha256). Vacío: lo instala tools/ci/install-blender.sh")

# Las rutas del repo, fijadas aquí porque dentro de una función CMAKE_CURRENT_LIST_DIR es la
# carpeta de quien la llama.
cmake_path(GET CMAKE_CURRENT_LIST_DIR PARENT_PATH _mz_blender_root)
set(_mz_blender_pin ${_mz_blender_root}/tools/ci/blender-version)
set(_mz_blender_script ${_mz_blender_root}/tools/blender/export_glb.py)

# Cambiar el pin vuelve a configurar: otra versión, otra verificación.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${_mz_blender_pin})

block(SCOPE_FOR VARIABLES PROPAGATE MZ_BLENDER_EXECUTABLE MZ_BLENDER_VERSION)
    # El pin se lee línea por línea, como en install-blender.sh: version=X.Y.Z.
    file(STRINGS ${_mz_blender_pin} line REGEX "^version=")
    string(REGEX REPLACE "^version=" "" MZ_BLENDER_VERSION "${line}")
    string(STRIP "${MZ_BLENDER_VERSION}" MZ_BLENDER_VERSION)
    if(NOT MZ_BLENDER_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
        message(FATAL_ERROR
            "tools/ci/blender-version: version tiene que ser X.Y.Z, y dice: '${MZ_BLENDER_VERSION}'")
    endif()

    if(MZ_BLENDER)
        set(MZ_BLENDER_EXECUTABLE ${MZ_BLENDER})
    else()
        # Los mensajes del instalador (stderr) se ven en la configuración; stdout es la ruta.
        execute_process(
            COMMAND bash ${_mz_blender_root}/tools/ci/install-blender.sh
            OUTPUT_VARIABLE MZ_BLENDER_EXECUTABLE
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE result
        )
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "tools/ci/install-blender.sh no pudo instalar Blender "
                "${MZ_BLENDER_VERSION}. El build lo necesita para exportar los .blend.")
        endif()
    endif()

    # La primera línea de --version dice "Blender 5.2.2 LTS": la versión, y después un
    # espacio o nada. Igual que en tools/ci/blender.sh.
    execute_process(
        COMMAND ${MZ_BLENDER_EXECUTABLE} -b --version
        OUTPUT_VARIABLE output
        ERROR_QUIET
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "No se pudo correr ${MZ_BLENDER_EXECUTABLE}: ${result}")
    endif()
    string(REGEX MATCH "^[^\n]*" first_line "${output}")
    string(FIND "${first_line} " "Blender ${MZ_BLENDER_VERSION} " position)
    if(NOT position EQUAL 0)
        # El mismo mensaje que tools/ci/blender.sh.
        message(FATAL_ERROR
            "Se esperaba Blender ${MZ_BLENDER_VERSION} y el binario dice: ${first_line}\n"
            "Binario: ${MZ_BLENDER_EXECUTABLE}\n"
            "La versión fijada está en tools/ci/blender-version. Deja MZ_BLENDER vacía para "
            "que tools/ci/install-blender.sh la instale.")
    endif()
    message(STATUS "Blender ${MZ_BLENDER_VERSION}: ${MZ_BLENDER_EXECUTABLE}")
endblock()

function(_mz_export_blend blend out_var)
    cmake_path(ABSOLUTE_PATH blend BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        NORMALIZE OUTPUT_VARIABLE source)
    cmake_path(REPLACE_EXTENSION blend LAST_ONLY .glb OUTPUT_VARIABLE relative)
    set(output ${CMAKE_CURRENT_BINARY_DIR}/${relative})

    # --factory-startup: sin la configuración de quien compila, y sin correr los scripts
    # que trae el .blend. --python-exit-code 1: una excepción que nadie captura en el
    # script o en el exportador hace salir a Blender con 1, y el build falla; sin él,
    # saldría con 0.
    add_custom_command(
        OUTPUT ${output}
        COMMAND ${MZ_BLENDER_EXECUTABLE} -b --factory-startup -noaudio
            --python-exit-code 1 ${source}
            --python ${_mz_blender_script} -- ${output} ${MZ_BLENDER_VERSION}
        DEPENDS ${source} ${_mz_blender_script} ${_mz_blender_pin}
        COMMENT "Exportando ${blend} a glTF"
        VERBATIM
    )
    set(${out_var} ${output} PARENT_SCOPE)
endfunction()
