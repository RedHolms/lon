cmake_minimum_required(VERSION 3.18)

project(lon CXX)

add_executable(
  lon_main_exe
  "src/main.cpp"
)

target_compile_definitions(
  lon_main_exe PUBLIC
  _CRT_SECURE_NO_WARNINGS
)

set_target_properties(
  lon_main_exe PROPERTIES
  OUTPUT_NAME "lon"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/output"
)
