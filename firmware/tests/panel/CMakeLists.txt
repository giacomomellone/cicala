cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(kveld_panel LANGUAGES C CXX)

set(KVELD_APP ${CMAKE_CURRENT_SOURCE_DIR}/../../app)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/layout kveld_layout)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/retained kveld_retained)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/qdb kveld_qdb)

target_sources(app PRIVATE
    src/main.cpp
    ${KVELD_APP}/src/panel.cpp
    ${KVELD_APP}/src/retained_block.cpp
)

target_include_directories(app PRIVATE ${KVELD_APP}/include)
target_link_libraries(app PRIVATE kveld_layout kveld_qdb kveld_retained)

set(fixtures ${CMAKE_CURRENT_SOURCE_DIR}/../fixtures)

foreach(lang en de)
  if(NOT EXISTS ${fixtures}/${lang}.qdb)
    message(FATAL_ERROR
      "missing ${fixtures}/${lang}.qdb — run `just fw-test`")
  endif()

  generate_inc_file_for_target(app
    ${fixtures}/${lang}.qdb
    ${ZEPHYR_BINARY_DIR}/include/generated/${lang}_qdb.inc)
endforeach()
