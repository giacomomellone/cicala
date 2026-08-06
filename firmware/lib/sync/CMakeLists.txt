# The manifest, and the rules for acting on one. No Zephyr headers, so the
# suite runs real manifests — including the one that shipped — on the host.

zephyr_library_named(tk_sync)
zephyr_library_sources(manifest.cpp)
zephyr_include_directories(include)

add_dependencies(tk_sync zephyr_generated_headers)
