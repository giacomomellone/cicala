cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tischkarte_status LANGUAGES C CXX)

# The same library the application links, not a copy. tk_power is not linked:
# this suite names power states, which is a header, not code.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/status tk_status)

target_sources(app PRIVATE src/main.cpp)

target_link_libraries(app PRIVATE tk_status)
