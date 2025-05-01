cmake_minimum_required(VERSION 3.18)

project(lon CXX)

# fixme GCC maybe?
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -fexceptions -Xclang -fcxx-exceptions")
endif()

add_executable(
  lon_main_exe
  "src/main.cpp"
  "src/lexer.cpp"
  "src/lexer.hpp"
  "src/ast/ast.hpp"
  "src/ast/expressions.hpp"
  "src/ast/literals.hpp"
  "src/ast/statements.hpp"
  "src/ast/types.hpp"
  "src/parser.cpp"
  "src/parser.hpp"
)

target_compile_definitions(
  lon_main_exe PUBLIC
  _CRT_SECURE_NO_WARNINGS
)

set_target_properties(
  lon_main_exe PROPERTIES
  OUTPUT_NAME "lon"
  CXX_STANDARD 14
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/output"
)
