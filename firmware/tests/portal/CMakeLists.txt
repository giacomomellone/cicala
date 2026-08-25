cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cicala_portal LANGUAGES C CXX)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/fsm cicala_fsm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/portal cicala_portal)

target_sources(app PRIVATE src/main.cpp)
target_link_libraries(app PRIVATE cicala_fsm cicala_portal)
