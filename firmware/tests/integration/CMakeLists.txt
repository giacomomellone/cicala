cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cicala_integration LANGUAGES C CXX)

set(CICALA_APP ${CMAKE_CURRENT_SOURCE_DIR}/../../app)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/fsm cicala_fsm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/app_fsm cicala_app_fsm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/qdb cicala_qdb)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/layout cicala_layout)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/retained cicala_retained)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/power cicala_power)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/status cicala_status)

target_sources(app PRIVATE
    src/main.c
    ${CICALA_APP}/src/app.c
    ${CICALA_APP}/src/app_logic.cpp
    ${CICALA_APP}/src/channels.c
    ${CICALA_APP}/src/corpus.c
    ${CICALA_APP}/src/display.c
    ${CICALA_APP}/src/input.c
    ${CICALA_APP}/src/language.c
    ${CICALA_APP}/src/panel.cpp
    ${CICALA_APP}/src/power.c
    ${CICALA_APP}/src/power_logic.cpp
    ${CICALA_APP}/src/retained_block.cpp
    ${CICALA_APP}/src/status.c
    ${CICALA_APP}/src/status_logic.cpp
)

target_include_directories(app PRIVATE ${CICALA_APP}/include)
target_link_libraries(app PRIVATE cicala_fsm cicala_app_fsm cicala_qdb cicala_layout cicala_retained cicala_power
    cicala_status)

set(fixtures ${CMAKE_CURRENT_SOURCE_DIR}/../fixtures)

foreach(lang en de)
  if(NOT EXISTS ${fixtures}/${lang}.qdb)
    message(FATAL_ERROR "missing ${fixtures}/${lang}.qdb — run `just fw-test`")
  endif()

  generate_inc_file_for_target(app
    ${fixtures}/${lang}.qdb
    ${ZEPHYR_BINARY_DIR}/include/generated/${lang}_qdb.inc)
endforeach()
