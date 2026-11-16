mz_add_test_suite(core
    SOURCES
        core_test.cpp
        assert_test.cpp
        assert_ndebug_test.cpp
        handle_test.cpp
        result_test.cpp
        scalar_test.cpp
        vec_test.cpp
        mat_test.cpp
        projection_test.cpp
        plane_test.cpp
        quat_test.cpp
        region_test.cpp
        arena_test.cpp
        scratch_test.cpp
    DEPS engine_core
)

# El lado apagado de MZ_ASSERT, como en relwithdebinfo y release (ver el archivo).
set_source_files_properties(assert_ndebug_test.cpp PROPERTIES COMPILE_DEFINITIONS NDEBUG)
