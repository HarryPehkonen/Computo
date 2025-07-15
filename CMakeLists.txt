cmake_minimum_required(VERSION 3.15)
project(Computo VERSION 0.4.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable compile commands database (required for clang-tidy)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable warnings and treat them as errors
if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror)
endif()

# --- Static Analysis and Formatting (Optional Targets) ---

# Clang-Tidy target
find_program(CLANG_TIDY_EXE NAMES clang-tidy-19 clang-tidy)
if(CLANG_TIDY_EXE)
    message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
    add_custom_target(clang-tidy
        COMMAND ${CLANG_TIDY_EXE} -p ${CMAKE_BINARY_DIR} --fix
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running clang-tidy with fixes"
    )
    # Read-only version for CI
    add_custom_target(clang-tidy-check
        COMMAND ${CLANG_TIDY_EXE} -p ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running clang-tidy (read-only)"
    )
else()
    message(WARNING "clang-tidy not found. Static analysis targets will be unavailable.")
endif()

# Clang-Format target
find_program(CLANG_FORMAT_EXE NAMES clang-format-19 clang-format)
if(CLANG_FORMAT_EXE)
    message(STATUS "Found clang-format: ${CLANG_FORMAT_EXE}")
    add_custom_target(clang-format
        COMMAND ${CLANG_FORMAT_EXE} -i
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running clang-format"
    )
    # Read-only version for CI
    add_custom_target(clang-format-check
        COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Checking clang-format (read-only)"
    )
else()
    message(WARNING "clang-format not found. Formatting targets will be unavailable.")
endif()

# --- Dependencies ---
find_package(nlohmann_json REQUIRED)
find_package(GTest REQUIRED)
find_library(READLINE_LIB readline)
if(NOT READLINE_LIB)
    message(FATAL_ERROR "readline library not found - required for REPL functionality")
endif()

# --- Directory Structure ---
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

# --- Version Header Generation ---
configure_file(
    "${INCLUDE_DIR}/computo_version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/computo_version.hpp"
    @ONLY
)

# --- Source File Definitions ---
file(GLOB_RECURSE COMPUTO_SOURCES CONFIGURE_DEPENDS
    "${SRC_DIR}/computo.cpp"
    "${SRC_DIR}/operators/*.cpp"
)

# --- Library Targets (Compiling sources twice, which is necessary here) ---

# 1. Production Library (libcomputo.a)
add_library(computo ${COMPUTO_SOURCES})
target_include_directories(computo PUBLIC 
    ${INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/include
)
target_link_libraries(computo PUBLIC nlohmann_json::nlohmann_json)

# 2. REPL Library (libcomputorepl.a)
add_library(computorepl ${COMPUTO_SOURCES})
target_include_directories(computorepl PUBLIC 
    ${INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/include
)
target_link_libraries(computorepl PUBLIC nlohmann_json::nlohmann_json)
target_compile_definitions(computorepl PUBLIC REPL) # Apply REPL flag during compilation


# --- CLI Executable Targets ---

# Production CLI tool
add_executable(computo_app src/cli.cpp src/benchmark.cpp)
target_link_libraries(computo_app PRIVATE computo)
set_target_properties(computo_app PROPERTIES OUTPUT_NAME computo)

# REPL CLI tool
add_executable(computo_repl_app src/repl.cpp src/benchmark.cpp)
target_link_libraries(computo_repl_app PRIVATE computorepl ${READLINE_LIB})
target_compile_definitions(computo_repl_app PRIVATE REPL)
set_target_properties(computo_repl_app PROPERTIES OUTPUT_NAME computo_repl)


# --- Test Targets ---
enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${TEST_DIR}/*.cpp")

# Filter test files for each build type
set(PRODUCTION_TEST_SOURCES ${TEST_SOURCES})
list(FILTER PRODUCTION_TEST_SOURCES EXCLUDE REGEX ".*test_repl.cpp$")
list(FILTER PRODUCTION_TEST_SOURCES EXCLUDE REGEX ".*test_debugging.cpp$")

set(REPL_TEST_SOURCES ${TEST_SOURCES})
list(FILTER REPL_TEST_SOURCES EXCLUDE REGEX ".*test_cli.cpp$")

# Production tests link against the `computo` library
add_executable(test_computo ${PRODUCTION_TEST_SOURCES})
target_link_libraries(test_computo PRIVATE computo GTest::gtest_main)
# FIX for CLITest: Pass the path to the CLI executable
target_compile_definitions(test_computo PRIVATE "COMPUTO_CLI_PATH=\"$<TARGET_FILE:computo_app>\"")
add_test(NAME computo_tests COMMAND test_computo)

# REPL tests link against the `computorepl` library
add_executable(test_computo_repl ${REPL_TEST_SOURCES})
target_link_libraries(test_computo_repl PRIVATE computorepl GTest::gtest_main)
target_compile_definitions(test_computo_repl PRIVATE "COMPUTO_REPL_PATH=\"$<TARGET_FILE:computo_repl_app>\"")
add_test(NAME computo_repl_tests COMMAND test_computo_repl)
