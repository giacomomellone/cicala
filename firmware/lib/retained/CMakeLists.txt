# The stamp that tells a wake from a cold boot. No Zephyr headers, so the
# suite can exercise corruption and version changes on the host — none of
# which can be produced on a board on purpose.

zephyr_library_named(tk_retained)
zephyr_library_sources(retained.cpp)
zephyr_include_directories(include)

add_dependencies(tk_retained zephyr_generated_headers)
