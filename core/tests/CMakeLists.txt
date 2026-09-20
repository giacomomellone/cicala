add_executable(cicala_core_tests session_test.cpp)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/selection_fixture.hpp
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/fixture_header.py
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/selection.json ${CMAKE_CURRENT_BINARY_DIR}/selection_fixture.hpp
  DEPENDS fixture_header.py fixtures/selection.json)
target_sources(cicala_core_tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/selection_fixture.hpp)
target_include_directories(cicala_core_tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(cicala_core_tests PRIVATE cicala_core)
add_test(NAME cicala_core COMMAND cicala_core_tests)

add_executable(cicala_bundle_check bundle_check.cpp)
target_link_libraries(cicala_bundle_check PRIVATE cicala_core)
