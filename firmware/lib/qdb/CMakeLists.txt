# TKB2 reader and shuffle bag. No Zephyr headers, so the suite runs the real
# shipped bundles through it on the host.

zephyr_library_named(tk_qdb)
zephyr_library_sources(qdb.cpp)
zephyr_include_directories(include)

add_dependencies(tk_qdb zephyr_generated_headers)
