cmake_minimum_required(VERSION 3.18)

project(lon CXX)

# fixme GCC maybe?
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -fexceptions -Xclang -fcxx-exceptions")
endif()

add_executable(
  lon_main_exe
  "src/main.cpp"
  "src/compiler/lexer.cpp"
  "src/compiler/lexer.hpp"
  "src/compiler/ast/ast.hpp"
  "src/compiler/ast/expressions.hpp"
  "src/compiler/ast/literals.hpp"
  "src/compiler/ast/statements.hpp"
  "src/compiler/ast/types.hpp"
  "src/compiler/parser.cpp"
  "src/compiler/parser.hpp"
#  "src/compiler/generator.cpp"
#  "src/compiler/generator.hpp"
  "src/utils.hpp"
)

target_compile_definitions(
  lon_main_exe PUBLIC
  _CRT_SECURE_NO_WARNINGS
)

set_target_properties(
  lon_main_exe PROPERTIES
  OUTPUT_NAME "lon"
  CXX_STANDARD 17
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/output"
)
