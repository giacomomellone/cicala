zephyr_library_named(kveld_app_fsm)
zephyr_library_sources(app_fsm.cpp)
zephyr_include_directories(include)

# This library is added after Zephyr's dependency discovery pass.
add_dependencies(kveld_app_fsm zephyr_generated_headers)
