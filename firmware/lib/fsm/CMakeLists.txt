# Shared library, pulled in by both the application and the test suites with
# add_subdirectory(). Its only Zephyr dependency is the default clock, so a
# suite that overrides now_ms() exercises every timeout without sleeping.

zephyr_library_named(tk_fsm)
zephyr_library_sources(fsm.cpp)
zephyr_include_directories(include)

# Zephyr wires this up for its own libraries in a loop that has already run by
# the time find_package(Zephyr) returns, so a library added afterwards has to
# ask for it. Without this, fsm.cpp can be compiled before generated headers
# like zephyr/heap_constants.h exist, and the build fails intermittently
# depending on how ninja happens to schedule.
add_dependencies(tk_fsm zephyr_generated_headers)
