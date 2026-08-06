# Captive-portal parsing, rendering and policy. No Zephyr headers, so the suite
# runs real query bytes, real form bodies and real page output on the host.

zephyr_library_named(tk_portal)
zephyr_library_sources(portal_dns.cpp portal_form.cpp portal_fsm.cpp portal_page.cpp)
zephyr_include_directories(include)

add_dependencies(tk_portal zephyr_generated_headers)
