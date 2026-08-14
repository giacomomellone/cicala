cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tischkarte_soak LANGUAGES C CXX)

set(TK_APP ${CMAKE_CURRENT_SOURCE_DIR}/../../app)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/fsm tk_fsm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/app_fsm tk_app_fsm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/qdb tk_qdb)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/layout tk_layout)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/retained tk_retained)

target_sources(app PRIVATE
    src/main.c
    ${TK_APP}/src/app.c
    ${TK_APP}/src/app_logic.cpp
    ${TK_APP}/src/channels.c
    ${TK_APP}/src/corpus.c
    ${TK_APP}/src/display.c
    ${TK_APP}/src/input.c
    ${TK_APP}/src/language.c
    ${TK_APP}/src/panel.cpp
    ${TK_APP}/src/retained_block.cpp
    ${TK_APP}/src/soak.c
)

target_include_directories(app PRIVATE ${TK_APP}/include)
target_link_libraries(app PRIVATE tk_fsm tk_app_fsm tk_qdb tk_layout tk_retained)

set(fixtures ${CMAKE_CURRENT_SOURCE_DIR}/../fixtures)

foreach(lang en de)
  if(NOT EXISTS ${fixtures}/${lang}.qdb)
    message(FATAL_ERROR "missing ${fixtures}/${lang}.qdb — run `just fw-test`")
  endif()

  generate_inc_file_for_target(app
    ${fixtures}/${lang}.qdb
    ${ZEPHYR_BINARY_DIR}/include/generated/${lang}_qdb.inc)
endforeach()
