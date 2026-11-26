# El cocinador de assets. La conversión vive en la biblioteca, que prueban los tests de
# tests/tools/ con la meta del 100 %; el ejecutable que corre el build solo lee sus
# argumentos y la llama, y queda fuera de src/ (y de la meta).
mz_add_module(tools_cooker LAYER tool
    SOURCES
        src/error.cpp
        src/file.cpp
        src/mesh.cpp
    # PUBLIC: los headers del cocinador nombran tipos de core (result, vec3, mesh_vertex).
    PUBLIC_DEPS engine_core
    # PRIVATE: cgltf no sale del cocinador.
    PRIVATE_DEPS tools_cgltf
)

mz_add_module(mz_cook LAYER tool EXECUTABLE
    SOURCES main.cpp
    PRIVATE_DEPS tools_cooker
)
