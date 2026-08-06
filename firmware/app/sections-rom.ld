/*
 * HTTP_RESOURCE_DEFINE puts each resource in a section named after its service,
 * so Zephyr cannot declare the section itself — every application that defines
 * an HTTP service has to declare its own. The name here has to match the
 * service name in src/portal.c.
 */

#include <zephyr/linker/iterable_sections.h>

ITERABLE_SECTION_ROM(http_resource_desc_tk_portal, Z_LINK_ITERABLE_SUBALIGN)
