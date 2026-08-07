cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tischkarte LANGUAGES C CXX)

# The languages the image carries. Kept here rather than derived from what
# happens to be in tests/fixtures/, so a missing bundle fails the build instead
# of silently shipping a portal that offers a language the device does not have.
set(TK_CORPUS_LANGUAGES en de)

# Shared, host-testable logic. See docs/firmware_architecture.md "Modules".
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/fsm tk_fsm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/app_fsm tk_app_fsm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/qdb tk_qdb)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/layout tk_layout)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/retained tk_retained)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/portal tk_portal)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/ed25519 tk_ed25519)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/sync tk_sync)

# Application logic is C++17; the files that use zbus's observer macros stay C,
# because those expand to out-of-order designated initializers.
target_sources(app PRIVATE
    src/app.c
    src/app_logic.cpp
    src/channels.c
    src/corpus.c
    src/display.c
    src/input.c
    src/language.c
    src/main.c
    src/panel.cpp
    src/retained_block.cpp
    src/update_notice.c
)

# Bench instrumentation, and the only source that is not in every image: it
# presses Next by itself, which no shipped device should do. See soak.conf.
target_sources_ifdef(CONFIG_TK_DEBUG_SOAK app PRIVATE src/soak.c)

# Deep sleep, likewise out of the default image: it reboots the board on every
# wake, which is the point and also why bring-up does not want it. See
# soak.conf's neighbour, sleep.conf.
target_sources_ifdef(CONFIG_FILE_SYSTEM_LITTLEFS app PRIVATE src/corpus_store.c)
target_sources_ifdef(CONFIG_TK_SLEEP app PRIVATE src/sleep.c)
target_sources_ifdef(CONFIG_TK_NET app PRIVATE src/net.c src/net_logic.cpp src/portal.c)

# fetch.c belongs to whichever of the two downloaders is built, and both use it
# unchanged — a bundle into RAM, an image into flash, one socket path.
target_sources_ifdef(CONFIG_TK_SYNC app PRIVATE src/fetch.c src/sync.cpp)
target_sources_ifdef(CONFIG_TK_OTA app PRIVATE src/ota.cpp)

# HTTP_RESOURCE_DEFINE lands each resource in a section named after its service.
# Zephyr cannot pre-declare a section whose name the application chooses, so the
# application declares it — without this the resources link into nothing and
# every request falls through to the catch-all.
if(CONFIG_TK_NET)
  zephyr_linker_sources(SECTIONS sections-rom.ld)
  zephyr_linker_section(NAME http_resource_desc_tk_portal KVMA RAM_REGION GROUP RODATA_REGION)
endif()

# trusted_key.h — the committed bundle-signing public key.
target_include_directories(app PRIVATE include ${CMAKE_CURRENT_SOURCE_DIR}/../components/sync)

# Explicit: Zephyr collects its own libraries in a loop that has already run by
# the time find_package(Zephyr) returns, so anything added after has to be
# linked by hand.
target_link_libraries(app PRIVATE tk_fsm tk_app_fsm tk_qdb tk_layout tk_portal tk_ed25519
    tk_sync tk_retained)

# The factory-preloaded corpora, compiled into the image so a device whose
# Wi-Fi is never configured still works. Built from the question database by
# `just fw-fixtures`, which `just fw-build` runs first. When sync lands these
# are the fallback and the live corpus comes from LittleFS.
#
# Every shipped language, not just CONFIG_TK_CORPUS_LANGUAGE: the setup portal
# offers a language and the device has to have something to switch to. Each is
# about 20 KB against a 1344 KB slot, which is a better trade than a portal
# whose language field cannot do anything.
set(fixtures ${CMAKE_CURRENT_SOURCE_DIR}/../tests/fixtures)

foreach(lang ${TK_CORPUS_LANGUAGES})
  if(NOT EXISTS ${fixtures}/${lang}.qdb)
    message(FATAL_ERROR
      "missing ${fixtures}/${lang}.qdb — run `just fw-fixtures` (or `just fw-build`, which does)")
  endif()

  generate_inc_file_for_target(app
    ${fixtures}/${lang}.qdb
    ${ZEPHYR_BINARY_DIR}/include/generated/${lang}_qdb.inc)
endforeach()
