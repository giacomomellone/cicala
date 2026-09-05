cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cicala_panel LANGUAGES C CXX)

set(CICALA_APP ${CMAKE_CURRENT_SOURCE_DIR}/../../app)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/layout cicala_layout)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/retained cicala_retained)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/qdb cicala_qdb)

target_sources(app PRIVATE
    src/main.cpp
    ${CICALA_APP}/src/panel.cpp
    ${CICALA_APP}/src/retained_block.cpp
)

target_include_directories(app PRIVATE ${CICALA_APP}/include)
target_link_libraries(app PRIVATE cicala_layout cicala_qdb cicala_retained)

# This suite walks every question the device ships, so it reads the real corpus
# rather than the fixture one. firmware/tests/corpus/README.md explains the split.
set(shipped ${CMAKE_CURRENT_SOURCE_DIR}/../../../dist/corpus)

foreach(lang en de)
  if(NOT EXISTS ${shipped}/${lang}.qdb)
    message(FATAL_ERROR
      "missing ${shipped}/${lang}.qdb — run `just fw-test`")
  endif()

  generate_inc_file_for_target(app
    ${shipped}/${lang}.qdb
    ${ZEPHYR_BINARY_DIR}/include/generated/${lang}_qdb.inc)
endforeach()
