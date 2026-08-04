# The tabletop state machine. Hardware-free, so both the application and the
# suite that drives its timeouts link the same source.

zephyr_library_named(tk_app_fsm)
zephyr_library_sources(app_fsm.cpp)
zephyr_include_directories(include)

# Same reason as lib/fsm: Zephyr collects its own libraries before
# find_package() returns, so a library added afterwards asks for the
# dependency itself.
add_dependencies(tk_app_fsm zephyr_generated_headers)
