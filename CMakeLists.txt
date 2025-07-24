cmake_minimum_required(VERSION 4.0)
project(jacobs-file-sync LANGUAGES C)

find_program(CLANG_TIDY_EXE NAMES "clang-tidy")

set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_EXE}")
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIREED ON)

set(CMAKE_C_FLAGS_DEBUG "-Wall -Wextra -Wconversion -O0 -g")
set(CMAKE_C_FLAGS "-Wall -Wextra -Wconversion -O3 -funroll-loops")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(jfs_modules STATIC
    src/modules/error.c
    src/modules/file_io.c
    src/modules/file_walk.c
    src/modules/net_socket.c
)

target_include_directories(jfs_modules PUBLIC
    ${CMAKE_SOURCE_DIR}/src/include
    ${SYSTEMD_INCLUDE_DIRS}
)

target_compile_options(jfs_modules PRIVATE ${SYSTEMD_CFLAGS_OTHER})
target_link_libraries(jfs_modules PUBLIC ${SYSTEMD_LIBRARIES})

add_executable(test src/test/test.c)
target_link_libraries(test PRIVATE jfs_modules)

add_executable(server src/server/server.c)
target_link_libraries(server PRIVATE jfs_modules)

add_executable(daemon src/daemon/daemon.c)
target_link_libraries(daemon PRIVATE jfs_modules)

add_custom_target(extra_clean COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR})
