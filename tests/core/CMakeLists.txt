mz_add_test_suite(core
    SOURCES
        core_test.cpp
        assert_test.cpp
        assert_ndebug_test.cpp
        handle_test.cpp
        result_test.cpp
    DEPS engine_core
)

# El lado apagado de MZ_ASSERT, como en relwithdebinfo y release (ver el archivo).
set_source_files_properties(assert_ndebug_test.cpp PROPERTIES COMPILE_DEFINITIONS NDEBUG)
