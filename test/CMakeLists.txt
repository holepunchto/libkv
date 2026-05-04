cmake_minimum_required(VERSION 3.25)

project(kv_test C)

add_executable(basic basic.c)

target_link_libraries(
  basic
  PRIVATE
  kv
)

add_test(NAME basic COMMAND basic)

add_executable(many many.c)

target_link_libraries(
  many
  PRIVATE
  kv
)

add_test(NAME many COMMAND many)

add_executable(batch batch.c)

target_link_libraries(
  batch
  PRIVATE
  kv
)

add_test(NAME batch COMMAND batch)
