include(mz_testing)

add_subdirectory(support)
add_subdirectory(core)
add_subdirectory(toolchain)
add_subdirectory(regression)
add_subdirectory(cmake)
add_subdirectory(tooling)

# Las carpetas con la meta del 100 % en líneas y ramas: el core y las zonas
# con TDD. Esta es la única lista; cada zona nueva con TDD se suma aquí en su ticket.
mz_add_coverage_target(
    GOAL
        engine/core
)
