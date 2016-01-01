########################################################################
## Copyright(c) libgreen contributors.  See LICENSE file for details. ##
########################################################################

cmake_minimum_required(VERSION 2.8)

project(green)

option(GREEN_TESTS "Compile the test suite." ON)
option(GREEN_GCOV "Compute code coverage." OFF)
option(GREEN_VALGRIND "Run tests with memory leak checker." OFF)

if (GREEN_GCOV)
  message(STATUS "Code coverage enabled.")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 --coverage")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage")
  set(CMAKE_STATIC_LINKER_FLAGS="--coverage")
  set(CMAKE_MODULE_LINKER_FLAGS="--coverage")
  set(CMAKE_EXE_LINKER_FLAGS="--coverage")
endif()

# Relative paths make __FILE__ more readable.
set(CMAKE_USE_RELATIVE_PATHS TRUE)

# Output everything nearby for convenience.
set(LIBRARY_OUTPUT_PATH    ${PROJECT_BINARY_DIR})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

include_directories("${PROJECT_SOURCE_DIR}/include")

add_library(green
  "src/green.c"
)

# This enables `ctest -T memcheck`.
if (GREEN_VALGRIND)
  find_program(MEMORYCHECK_COMMAND "valgrind")
  set(MEMORYCHECK_COMMAND_OPTIONS
    "--leak-check=yes --error-exitcode=666"
  )
  include(Dart)
endif()

macro(green_add_test test-name source-file)
  add_executable(${test-name} ${source-file})
  target_link_libraries(${test-name} green)
  add_test(${test-name} "${test-name}")
endmacro()

if(GREEN_TESTS)
  enable_testing()
  green_add_test(test-version "tests/test-version.c")
  green_add_test(test-init-term "tests/test-init-term.c")
endif()
