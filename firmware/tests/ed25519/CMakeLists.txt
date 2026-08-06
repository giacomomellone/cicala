cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tischkarte_ed25519 LANGUAGES C CXX)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/ed25519 tk_ed25519)

target_sources(app PRIVATE src/main.cpp)
target_link_libraries(app PRIVATE tk_ed25519)

# The committed public key the release was signed against, and the release
# fixture beside this file.
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components/sync)
