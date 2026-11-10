# Declaración de módulos y regla de capas.
#
#   mz_add_module(<nombre> LAYER <capa> [EXECUTABLE]
#                 SOURCES <archivos>...
#                 [PUBLIC_DEPS <targets>...]
#                 [PRIVATE_DEPS <targets>...])
#
# Las capas, de abajo hacia arriba: platform, core, gpu, system, game. Un módulo solo
# puede enlazar con módulos de capas más bajas. La regla se revisa al final de la
# configuración, sobre todos los enlaces de cada módulo, así que también atrapa un
# target_link_libraries escrito a mano y no depende del orden en que se declaran.

include_guard(GLOBAL)

include(${CMAKE_CURRENT_LIST_DIR}/mz_flags.cmake)

set_property(GLOBAL PROPERTY MZ_LAYERS platform core gpu system game)
set_property(GLOBAL PROPERTY MZ_MODULES "")

# Devuelve en <out> la posición de <layer> en la pila de capas, o -1 si no existe.
function(_mz_layer_rank layer out)
    get_property(layers GLOBAL PROPERTY MZ_LAYERS)
    list(FIND layers "${layer}" rank)
    set(${out} ${rank} PARENT_SCOPE)
endfunction()

function(mz_add_module name)
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "EXECUTABLE" "LAYER" "SOURCES;PUBLIC_DEPS;PRIVATE_DEPS")

    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "mz_add_module(${name}): argumentos desconocidos: "
            "${arg_UNPARSED_ARGUMENTS}")
    endif()
    _mz_layer_rank("${arg_LAYER}" rank)
    if(rank EQUAL -1)
        get_property(layers GLOBAL PROPERTY MZ_LAYERS)
        message(FATAL_ERROR "mz_add_module(${name}): capa '${arg_LAYER}' inválida. "
            "Las capas son: ${layers}.")
    endif()
    if(NOT arg_SOURCES)
        message(FATAL_ERROR "mz_add_module(${name}): falta SOURCES.")
    endif()

    if(arg_EXECUTABLE)
        add_executable(${name} ${arg_SOURCES})
    else()
        add_library(${name} STATIC ${arg_SOURCES})
    endif()

    # Los headers públicos de un módulo solo los ven los módulos que enlazan con él.
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    endif()
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)
        target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    endif()

    # El proyecto usa headers y no módulos de C++20: sin escaneo de módulos, Ninja se ahorra
    # un paso por cada archivo.
    target_compile_features(${name} PUBLIC cxx_std_20)
    set_target_properties(${name} PROPERTIES
        CXX_EXTENSIONS OFF
        CXX_SCAN_FOR_MODULES OFF
        COMPILE_WARNING_AS_ERROR ON
        MZ_LAYER ${arg_LAYER}
    )
    target_link_libraries(${name}
        PRIVATE mz_warnings mz_engine_flags
        PUBLIC ${arg_PUBLIC_DEPS}
        PRIVATE ${arg_PRIVATE_DEPS}
    )

    set_property(GLOBAL APPEND PROPERTY MZ_MODULES ${name})
endfunction()

# Revisa los enlaces directos de cada módulo (los que se compilan con él y los que
# propaga a quien lo use). Reporta todas las violaciones antes de detener la generación.
function(_mz_check_layers)
    get_property(modules GLOBAL PROPERTY MZ_MODULES)
    foreach(module IN LISTS modules)
        get_target_property(layer ${module} MZ_LAYER)
        _mz_layer_rank(${layer} rank)

        get_target_property(own_links ${module} LINK_LIBRARIES)
        get_target_property(interface_links ${module} INTERFACE_LINK_LIBRARIES)
        set(deps "")
        foreach(dep IN LISTS own_links interface_links)
            # Una dependencia PRIVATE de una biblioteca estática se propaga como
            # $<LINK_ONLY:dep>: se enlaza, pero sus headers no se ven.
            string(REGEX REPLACE "^\\$<LINK_ONLY:(.+)>$" "\\1" dep "${dep}")
            if(NOT TARGET ${dep})
                continue()
            endif()
            get_target_property(aliased ${dep} ALIASED_TARGET)
            if(aliased)
                set(dep ${aliased})
            endif()
            list(APPEND deps ${dep})
        endforeach()
        list(REMOVE_DUPLICATES deps)

        foreach(dep IN LISTS deps)
            get_target_property(dep_layer ${dep} MZ_LAYER)
            if(NOT dep_layer)
                continue()
            endif()
            _mz_layer_rank(${dep_layer} dep_rank)
            if(dep_rank GREATER_EQUAL rank)
                message(SEND_ERROR
                    "${module} (capa ${layer}) enlaza con ${dep} (capa ${dep_layer}). "
                    "Un módulo solo puede enlazar con módulos de capas más bajas.")
            endif()
        endforeach()
    endforeach()
endfunction()

cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL _mz_check_layers)
