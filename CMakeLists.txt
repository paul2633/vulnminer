cmake_minimum_required(VERSION 3.28)

project(repo_analyzer C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wmissing-prototypes
    -Wstrict-prototypes
    -g
)

file(GLOB SRC CONFIGURE_DEPENDS
    src/*.c
)

add_executable(repo_analyzer ${SRC})