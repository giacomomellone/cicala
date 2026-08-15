cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(kveld_input LANGUAGES C CXX)

set(KVELD_APP ${CMAKE_CURRENT_SOURCE_DIR}/../../app)

target_sources(app PRIVATE
    src/main.c
    ${KVELD_APP}/src/channels.c
    ${KVELD_APP}/src/input.c
)

target_include_directories(app PRIVATE ${KVELD_APP}/include)
