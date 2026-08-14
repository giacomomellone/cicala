zephyr_library_named(tk_ed25519)
zephyr_library_sources(ed25519.cpp vendor/tweetnacl.c)
zephyr_include_directories(include vendor)

add_dependencies(tk_ed25519 zephyr_generated_headers)
