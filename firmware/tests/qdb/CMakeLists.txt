cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cicala_qdb LANGUAGES C CXX)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/qdb cicala_qdb)

target_sources(app PRIVATE src/main.cpp)
target_link_libraries(app PRIVATE cicala_qdb)

# The bag cases run against the fixture corpus; the corpus guards walk what
# actually ships. firmware/tests/corpus/README.md explains the split.
set(fixtures ${CMAKE_CURRENT_SOURCE_DIR}/../fixtures)
set(shipped ${CMAKE_CURRENT_SOURCE_DIR}/../../../dist/corpus)

foreach(lang en de)
  if(NOT EXISTS ${fixtures}/${lang}.qdb OR NOT EXISTS ${shipped}/${lang}.qdb)
    message(FATAL_ERROR
      "missing ${lang}.qdb under ${fixtures} or ${shipped} — run `just fw-test`")
  endif()

  generate_inc_file_for_target(app
    ${fixtures}/${lang}.qdb
    ${ZEPHYR_BINARY_DIR}/include/generated/${lang}_qdb.inc)

  generate_inc_file_for_target(app
    ${shipped}/${lang}.qdb
    ${ZEPHYR_BINARY_DIR}/include/generated/${lang}_shipped_qdb.inc)
endforeach()
