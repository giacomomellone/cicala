cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tischkarte_app_fsm LANGUAGES C CXX)

# The same libraries the application links, not copies.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/fsm tk_fsm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/app_fsm tk_app_fsm)

target_sources(app PRIVATE src/main.cpp)

target_link_libraries(app PRIVATE tk_fsm tk_app_fsm)
