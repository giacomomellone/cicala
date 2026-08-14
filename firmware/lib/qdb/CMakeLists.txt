zephyr_library_named(tk_qdb)
zephyr_library_sources(qdb.cpp)
zephyr_include_directories(include)

add_dependencies(tk_qdb zephyr_generated_headers)
