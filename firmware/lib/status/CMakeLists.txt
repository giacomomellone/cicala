# power_state.hpp is header-only, so add its include path directly.

zephyr_library_named(kveld_status)
zephyr_library_sources(status_led.cpp)
zephyr_include_directories(include ../power/include)

add_dependencies(kveld_status zephyr_generated_headers)
