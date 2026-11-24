mz_add_test_suite(platform
    SOURCES
        context_test.cpp
        memory_test.cpp
        message_box_test.cpp
        vulkan_test.cpp
        window_test.cpp
    # SDL3 directo: los tests de platform inyectan eventos de SDL y miran sus ventanas.
    DEPS engine_platform SDL3::SDL3
)
