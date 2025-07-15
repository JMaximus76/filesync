cmake_minimum_required(VERSION 4.0)
project(jfs LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIREED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_library(jfs_modules STATIC
    src/modules/error.c
    src/modules/file_io.c
    src/modules/file_walk.c
)

target_include_directories(jfs_modules PUBLIC src/include)

add_executable(test src/test/test.c)
target_link_libraries(test PRIVATE jfs_modules)

add_custom_target(extra_clean COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib)
