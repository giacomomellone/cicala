cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tischkarte_input LANGUAGES C CXX)

set(TK_APP ${CMAKE_CURRENT_SOURCE_DIR}/../../app)

# The same sources the application builds, not a copy — the point of the suite
# is the real settle window and the real one-hot rule.
target_sources(app PRIVATE
    src/main.c
    ${TK_APP}/src/channels.c
    ${TK_APP}/src/input.c
)

target_include_directories(app PRIVATE ${TK_APP}/include)
