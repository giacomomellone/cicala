cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tischkarte LANGUAGES C CXX)

# Shared, host-testable logic. See docs/firmware_architecture.md "Modules".
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/fsm tk_fsm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/app_fsm tk_app_fsm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/qdb tk_qdb)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/layout tk_layout)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/retained tk_retained)

# Application logic is C++17; the files that use zbus's observer macros stay C,
# because those expand to out-of-order designated initializers.
target_sources(app PRIVATE
    src/app.c
    src/app_logic.cpp
    src/channels.c
    src/display.c
    src/input.c
    src/main.c
    src/panel.cpp
    src/retained_block.cpp
)

# Bench instrumentation, and the only source that is not in every image: it
# presses Next by itself, which no shipped device should do. See soak.conf.
target_sources_ifdef(CONFIG_TK_DEBUG_SOAK app PRIVATE src/soak.c)

# Deep sleep, likewise out of the default image: it reboots the board on every
# wake, which is the point and also why bring-up does not want it. See
# soak.conf's neighbour, sleep.conf.
target_sources_ifdef(CONFIG_TK_SLEEP app PRIVATE src/sleep.c)

target_include_directories(app PRIVATE include)

# Explicit: Zephyr collects its own libraries in a loop that has already run by
# the time find_package(Zephyr) returns, so anything added after has to be
# linked by hand.
target_link_libraries(app PRIVATE tk_fsm tk_app_fsm tk_qdb tk_layout tk_retained)

# The factory-preloaded corpus, compiled into the image so a device whose Wi-Fi
# is never configured still works. Built from the question database by
# `just fw-fixtures`, which `just fw-build` runs first. When sync lands this is
# the fallback and the live corpus comes from LittleFS.
set(corpus ${CMAKE_CURRENT_SOURCE_DIR}/../tests/fixtures/${CONFIG_TK_CORPUS_LANGUAGE}.tkb2)

if(NOT EXISTS ${corpus})
  message(FATAL_ERROR
    "missing ${corpus} — run `just fw-fixtures` (or `just fw-build`, which does)")
endif()

generate_inc_file_for_target(app ${corpus} ${ZEPHYR_BINARY_DIR}/include/generated/corpus.inc)
