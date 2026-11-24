mz_add_test_suite(gpu
    SOURCES
        device_test.cpp
        fake_loader.cpp
        instance_test.cpp
        requirements_test.cpp
    # platform, como en el juego: carga el loader y abre la ventana offscreen.
    DEPS engine_gpu engine_platform
)
