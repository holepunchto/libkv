cmake_minimum_required(VERSION 3.25)

find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)

project(kv C)

add_library(kv OBJECT)

set_target_properties(
  kv
  PROPERTIES
  C_STANDARD 99
  POSITION_INDEPENDENT_CODE ON
)

target_sources(
  kv
  INTERFACE
    include/kv.h
  PRIVATE
    src/kv.c
)

target_include_directories(
  kv
  PUBLIC
    include
)

add_library(kv_shared SHARED $<TARGET_OBJECTS:kv>)

set_target_properties(
  kv_shared
  PROPERTIES
  OUTPUT_NAME kv
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_link_libraries(
  kv_shared
  PUBLIC
  kv
)

add_library(kv_static STATIC)

set_target_properties(
  kv_static
  PROPERTIES
  OUTPUT_NAME kv
  PREFIX lib
)

target_link_libraries(
  kv_static
  PUBLIC
  kv
)

install(TARGETS kv_shared kv_static)

install(FILES include/kv.h DESTINATION include)

if(PROJECT_IS_TOP_LEVEL)
  enable_testing()
  add_subdirectory(test)
endif()
