cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tischkarte LANGUAGES C CXX)

# Shared, host-testable logic. See docs/firmware_architecture.md "Modules".
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/fsm tk_fsm)

# Application logic is C++17, Zephyr-facing glue is C.
target_sources(app PRIVATE
    src/main.c
)

target_include_directories(app PRIVATE include)

# Explicit: Zephyr collects its own libraries in a loop that has already run by
# the time find_package(Zephyr) returns, so anything added after has to be
# linked by hand.
target_link_libraries(app PRIVATE tk_fsm)
