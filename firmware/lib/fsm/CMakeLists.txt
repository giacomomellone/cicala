zephyr_library_named(tk_fsm)
zephyr_library_sources(fsm.cpp)
zephyr_include_directories(include)

# This library is added after Zephyr's dependency discovery pass.
add_dependencies(tk_fsm zephyr_generated_headers)
