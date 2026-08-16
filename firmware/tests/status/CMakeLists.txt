cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(kveld_status LANGUAGES C CXX)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/status kveld_status)

target_sources(app PRIVATE src/main.cpp)

target_link_libraries(app PRIVATE kveld_status)
