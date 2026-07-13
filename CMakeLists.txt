cmake_minimum_required(VERSION 3.28)

project(repo_analyzer C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(OpenMP REQUIRED)

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

add_library(tree_sitter STATIC
    external/tree-sitter/src/lib.c
    external/tree-sitter-c/src/parser.c
)

target_include_directories(tree_sitter SYSTEM PUBLIC
    external/tree-sitter/include
    external/tree-sitter/src
    external/tree-sitter-c/src
)

target_compile_options(tree_sitter PRIVATE
    -w
)

add_executable(repo_analyzer ${SRC})

target_include_directories(repo_analyzer PRIVATE
    src
)

target_link_libraries(repo_analyzer PRIVATE
    tree_sitter
    OpenMP::OpenMP_C
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