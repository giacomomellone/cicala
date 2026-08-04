# UTF-8 decoding, accent decomposition and word wrap. No Zephyr headers, so
# the suite folds every shipped question on the host.

zephyr_library_named(tk_layout)
zephyr_library_sources(layout.cpp)
zephyr_include_directories(include)

add_dependencies(tk_layout zephyr_generated_headers)
