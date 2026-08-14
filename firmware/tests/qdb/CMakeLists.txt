cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tischkarte_qdb LANGUAGES C CXX)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/qdb tk_qdb)

target_sources(app PRIVATE src/main.cpp)
target_link_libraries(app PRIVATE tk_qdb)

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
