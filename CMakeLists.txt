cmake_minimum_required(VERSION 3.18)

project(lon C)

add_executable(
  lon_compiler
  "src/main.c"
  "src/lexer.c"
  "src/lexer.h"
  "src/parse.c"
  "src/parse.h"
  "src/trie.c"
  "src/trie.h"
  "src/str.c"
  "src/str.h"
  "src/gen.c"
  "src/gen.h"
)

target_compile_definitions(
  lon_compiler PUBLIC
  _CRT_SECURE_NO_WARNINGS
)

set_target_properties(
  lon_compiler PROPERTIES
  OUTPUT_NAME "lon"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/output"
)
