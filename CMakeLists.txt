cmake_minimum_required(VERSION 3.18)

project(lon C)

add_executable(
  interpreter
  "src/main.c"
  "src/lex.c"
  "src/lex.h"
  "src/parse.c"
  "src/parse.h"
  "src/trie.c"
  "src/trie.h"
  "src/str.c"
  "src/str.h"
  "src/gen.c"
  "src/gen.h"
)

set_target_properties(
  interpreter PROPERTIES
  OUTPUT_NAME "lon"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/output"
)
