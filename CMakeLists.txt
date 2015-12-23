########################################################################
## Copyright(c) libgreen contributors.  See LICENSE file for details. ##
########################################################################

cmake_minimum_required(VERSION 2.8)

project(green)

option(GREEN_TESTS "Compile the test suite." ON)

# Relative paths make __FILE__ more readable.
set(CMAKE_USE_RELATIVE_PATHS TRUE)

include_directories("${PROJECT_SOURCE_DIR}/include")

add_library(green
  "src/green.c"
)

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
