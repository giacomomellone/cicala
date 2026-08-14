cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tischkarte LANGUAGES C CXX)

# The languages the image carries.
set(TK_CORPUS_LANGUAGES en de)

# Shared, host-testable logic.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/fsm tk_fsm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/app_fsm tk_app_fsm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/qdb tk_qdb)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/layout tk_layout)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/retained tk_retained)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/portal tk_portal)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/ed25519 tk_ed25519)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/sync tk_sync)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/power tk_power)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/status tk_status)

# zbus observer macros require C designated initializers.
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

# Soak images generate their own Next presses.
target_sources_ifdef(CONFIG_TK_DEBUG_SOAK app PRIVATE src/soak.c)

target_sources_ifdef(CONFIG_FILE_SYSTEM_LITTLEFS app PRIVATE src/corpus_store.c)
target_sources_ifdef(CONFIG_TK_SLEEP app PRIVATE src/sleep.c)
target_sources_ifdef(CONFIG_TK_NET app PRIVATE src/net.c src/net_logic.cpp src/portal.c)

target_sources_ifdef(CONFIG_TK_POWER app PRIVATE src/power.c src/power_logic.cpp)
target_sources_ifdef(CONFIG_TK_STATUS_LED app PRIVATE src/status.c src/status_logic.cpp)

# Bundle and firmware downloads share one HTTP client.
target_sources_ifdef(CONFIG_TK_SYNC app PRIVATE src/fetch.c src/sync.cpp)
target_sources_ifdef(CONFIG_TK_OTA app PRIVATE src/ota.cpp)

# HTTP_RESOURCE_DEFINE lands each resource in a section named after its service.
if(CONFIG_TK_NET)
  zephyr_linker_sources(SECTIONS sections-rom.ld)
  zephyr_linker_section(NAME http_resource_desc_tk_portal KVMA RAM_REGION GROUP RODATA_REGION)
endif()

# Include the committed bundle-signing public key.
target_include_directories(app PRIVATE include ${CMAKE_CURRENT_SOURCE_DIR}/../components/sync)

# Link libraries added after Zephyr's library discovery pass.
target_link_libraries(app PRIVATE tk_fsm tk_app_fsm tk_qdb tk_layout tk_portal tk_ed25519
    tk_sync tk_retained tk_power tk_status)

# Compile shipped corpora into the offline fallback image.
set(fixtures ${CMAKE_CURRENT_SOURCE_DIR}/../tests/fixtures)

foreach(lang ${TK_CORPUS_LANGUAGES})
  if(NOT EXISTS ${fixtures}/${lang}.qdb)
    message(FATAL_ERROR
      "missing ${fixtures}/${lang}.qdb — run `just fw-build`")
  endif()

  generate_inc_file_for_target(app
    ${fixtures}/${lang}.qdb
    ${ZEPHYR_BINARY_DIR}/include/generated/${lang}_qdb.inc)
endforeach()
