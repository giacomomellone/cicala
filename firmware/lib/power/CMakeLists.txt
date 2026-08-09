# What a millivolt reading means. No Zephyr headers and no ADC, so the suite
# runs a whole discharge and a whole charge window on the host, in microseconds.

zephyr_library_named(tk_power)
zephyr_library_sources(power_fsm.cpp)
zephyr_include_directories(include)

add_dependencies(tk_power zephyr_generated_headers)
