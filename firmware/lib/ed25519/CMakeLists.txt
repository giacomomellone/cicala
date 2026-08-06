# Ed25519 verification, over TweetNaCl. No Zephyr headers, so the suite runs
# real release signatures on the host. See vendor/README.md for provenance and
# for why the vendored file is taken whole and unmodified.

zephyr_library_named(tk_ed25519)
zephyr_library_sources(ed25519.cpp vendor/tweetnacl.c)
zephyr_include_directories(include vendor)

add_dependencies(tk_ed25519 zephyr_generated_headers)
