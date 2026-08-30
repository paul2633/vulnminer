cmake_minimum_required(VERSION 3.28)

project(repo_analyzer C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wmissing-prototypes
    -Wstrict-prototypes
    -g
    -Wswitch-enum
)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    src/*.c
)

add_library(tree_sitter STATIC
    third_party/tree-sitter/src/lib.c
    third_party/tree-sitter-c/src/parser.c
)

target_include_directories(tree_sitter SYSTEM PUBLIC
    third_party/tree-sitter/include
    third_party/tree-sitter/src
    third_party/tree-sitter-c/src
)

add_subdirectory(third_party/yyjson)

target_compile_options(tree_sitter PRIVATE
    -w
)

add_executable(repo_analyzer ${SRC})

target_compile_definitions(repo_analyzer PRIVATE _GNU_SOURCE)

target_include_directories(repo_analyzer PRIVATE
    src
)

target_link_libraries(repo_analyzer PRIVATE
    tree_sitter
    yyjson
    CURL::libcurl
)

# -----------------------------------------------------------------------------
# Formatting
# -----------------------------------------------------------------------------

find_program(CLANG_FORMAT clang-format)

if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        src/*.c
        src/*.h
    )

    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCE_FILES}
    )
endif()

# -----------------------------------------------------------------------------
# Static analysis
# -----------------------------------------------------------------------------

find_program(CPPCHECK cppcheck)

if(CPPCHECK)
    add_custom_target(check
        COMMAND ${CPPCHECK}
                --enable=all
                --inconclusive
                --suppress=missingIncludeSystem
                ${PROJECT_SOURCE_DIR}/src
    )
endif()

# -----------------------------------------------------------------------------
# IWYU
# -----------------------------------------------------------------------------

find_program(IWYU_TOOL iwyu_tool)

if(IWYU_TOOL)
    add_custom_target(inc
        COMMAND ${IWYU_TOOL} -p ${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        DEPENDS ${PROJECT_NAME}
    )
endif()