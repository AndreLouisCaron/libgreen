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
  set(CMAKE_STATIC_LINKER_FLAGS="--coverage")
  set(CMAKE_MODULE_LINKER_FLAGS="--coverage")
  set(CMAKE_EXE_LINKER_FLAGS="--coverage")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

# Configure stuff.
try_run(HAVE_UCONTEXT_RESULT HAVE_UCONTEXT
  "${CMAKE_CURRENT_BINARY_DIR}/have-ucontext"
  "${CMAKE_CURRENT_SOURCE_DIR}/configure/have-ucontext.c"
  COMPILE_OUTPUT_VARIABLE have-ucontext-compile
  RUN_OUTPUT_VARIABLE have-ucontext-output
)
if (NOT HAVE_UCONTEXT)
  message(FATAL_ERROR "ucontext not available:\n${have-ucontext-compile}")
endif()
if (NOT (HAVE_UCONTEXT_RESULT EQUAL 0))
  message(FATAL_ERROR "ucontext doesn't work: exit status is ${HAVE_UCONTEXT_RESULT}\n${have-ucontext-output}")
endif()

# Inject feature switches into source code.
set(GREEN_USE_UCONTEXT 1)
configure_file(
  "src/configure.h.in"
  "${PROJECT_BINARY_DIR}/configure.h"
  @ONLY NEWLINE_STYLE UNIX
  )
include_directories(${PROJECT_BINARY_DIR})

# Relative paths make __FILE__ more readable.
set(CMAKE_USE_RELATIVE_PATHS TRUE)

# Output everything nearby for convenience.
set(LIBRARY_OUTPUT_PATH    ${PROJECT_BINARY_DIR})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

include_directories("${PROJECT_SOURCE_DIR}/include")

if(HAVE_UCONTEXT)
  add_definitions(-D_XOPEN_SOURCE=500)
endif()

add_library(green
  "src/green.c"
)

# libm is required for functions from <math.h>.
target_link_libraries(green m)

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
  green_add_test(test-loop "tests/test-loop.c")
  green_add_test(test-coroutine "tests/test-coroutine.c")
endif()
