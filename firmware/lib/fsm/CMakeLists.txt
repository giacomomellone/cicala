zephyr_library_named(cicala_fsm)
zephyr_library_sources(fsm.cpp)
zephyr_include_directories(include)

# This library is added after Zephyr's dependency discovery pass.
add_dependencies(cicala_fsm zephyr_generated_headers)
