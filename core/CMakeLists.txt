cmake_minimum_required(VERSION 3.16)
project(cicala_core VERSION 0.1.0 LANGUAGES C CXX)

add_library(cicala_core STATIC
  src/qdb.cpp src/session.cpp src/snapshot.cpp src/manifest.cpp
  src/bundle.cpp src/ed25519.cpp src/vendor/tweetnacl.c)
target_include_directories(cicala_core PUBLIC include PRIVATE src/vendor)
target_compile_features(cicala_core PUBLIC cxx_std_17)
target_compile_options(cicala_core PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>)

option(CICALA_BUILD_TESTS "Build host tests" OFF)
if(CICALA_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
