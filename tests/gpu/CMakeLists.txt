mz_add_test_suite(gpu
    SOURCES
        device_test.cpp
        fake_loader.cpp
        frame_test.cpp
        instance_test.cpp
        requirements_test.cpp
        swapchain_choice_test.cpp
    # platform, como en el juego: carga el loader y abre la ventana offscreen.
    DEPS engine_gpu engine_platform
)

# Los ejemplos de los diagramas de proceso (<diagrama>.ejemplo.d2): el test escribe el suyo
# en el build, y lo compara con la copia del repo, que solo lee (docs/diagramas/README.md).
target_compile_definitions(tests_gpu PRIVATE
    MZ_DIAGRAM_EXAMPLES_DIR="${CMAKE_CURRENT_BINARY_DIR}/diagramas"
    MZ_DIAGRAMS_DIR="${PROJECT_SOURCE_DIR}/docs/graphics/diagramas")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/diagramas")
