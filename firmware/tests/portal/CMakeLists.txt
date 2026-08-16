cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(kveld_portal LANGUAGES C CXX)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/fsm kveld_fsm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/portal kveld_portal)

target_sources(app PRIVATE src/main.cpp)
target_link_libraries(app PRIVATE kveld_fsm kveld_portal)
