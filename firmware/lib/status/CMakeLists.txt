# What the two LEDs should be doing. No Zephyr headers and no pins, so the
# suite checks the priority order and the blink counts on the host.
#
# Depends on tk_power for one thing only: the name of a power state. The
# include path is added rather than the library linked, because power_state.hpp
# declares an enum and no code.

zephyr_library_named(tk_status)
zephyr_library_sources(status_led.cpp)
zephyr_include_directories(include ../power/include)

add_dependencies(tk_status zephyr_generated_headers)
