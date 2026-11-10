include(mz_testing)

add_subdirectory(support)
add_subdirectory(core)
add_subdirectory(toolchain)
add_subdirectory(regression)
add_subdirectory(cmake)
add_subdirectory(tooling)

mz_add_coverage_target()
