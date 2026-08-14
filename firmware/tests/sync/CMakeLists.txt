cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tischkarte_sync LANGUAGES C CXX)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/sync tk_sync)

target_sources(app PRIVATE src/main.cpp)
target_link_libraries(app PRIVATE tk_sync)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
