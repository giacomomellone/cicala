cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cicala_retained LANGUAGES C CXX)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/qdb cicala_qdb)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/retained cicala_retained)

target_sources(app PRIVATE src/main.cpp)

target_link_libraries(app PRIVATE cicala_qdb cicala_retained)
