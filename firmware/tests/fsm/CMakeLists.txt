cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tischkarte_fsm LANGUAGES C CXX)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/fsm tk_fsm)

target_sources(app PRIVATE src/main.cpp)

target_link_libraries(app PRIVATE tk_fsm)
