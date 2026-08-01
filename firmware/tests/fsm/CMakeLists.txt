cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tischkarte_fsm LANGUAGES C CXX)

# The same library the application links, not a copy.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/fsm tk_fsm)

target_sources(app PRIVATE src/main.cpp)

# Also explicit for the same reason as the dependency in the library itself:
# Zephyr collects its own libraries before find_package() returns.
target_link_libraries(app PRIVATE tk_fsm)
