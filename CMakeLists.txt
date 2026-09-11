cmake_minimum_required(VERSION 3.20)

project(vulnminer C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
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
    third_party/tree-sitter-cpp/src/parser.c
    third_party/tree-sitter-cpp/src/scanner.c
)

target_include_directories(tree_sitter SYSTEM PUBLIC
    third_party/tree-sitter/include
    third_party/tree-sitter/src
    third_party/tree-sitter-c/src
    third_party/tree-sitter-cpp/src
)

add_library(yyjson STATIC
    third_party/yyjson/src/yyjson.c
)

target_include_directories(yyjson SYSTEM PUBLIC
    third_party/yyjson/src
)

target_compile_options(tree_sitter PRIVATE
    -w
)

add_executable(vulnminer ${SRC})

target_compile_definitions(vulnminer PRIVATE _GNU_SOURCE)

target_include_directories(vulnminer PRIVATE
    src
)

target_link_libraries(vulnminer PRIVATE
    tree_sitter
    yyjson
    CURL::libcurl
    Threads::Threads
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
                -I${PROJECT_SOURCE_DIR}/src
                -isystem${PROJECT_SOURCE_DIR}/third_party/yyjson/src
                ${PROJECT_SOURCE_DIR}/src
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
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