cmake_minimum_required(VERSION 3.15)
project(Computo VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable warnings and treat them as errors
if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror)
endif()

# Option to enable REPL & debug build
option(REPL "Enable REPL and debugging features" OFF)

# Find dependencies
find_package(nlohmann_json REQUIRED)
find_package(GTest REQUIRED)

# readline is only required for REPL builds
if (REPL)
    find_library(READLINE_LIB readline)
    if(NOT READLINE_LIB)
        message(FATAL_ERROR "REPL build requested but readline library not found")
    endif()
endif()

# Directory structure variables
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

# Phase 1: minimal library source files
file(GLOB_RECURSE COMPUTO_SOURCES CONFIGURE_DEPENDS
    "${SRC_DIR}/computo.cpp"
    "${SRC_DIR}/operators/*.cpp"
)

add_library(computo ${COMPUTO_SOURCES})

# Public include directory
target_include_directories(computo PUBLIC ${INCLUDE_DIR})

target_link_libraries(computo PUBLIC nlohmann_json::nlohmann_json)

# Tests
enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${TEST_DIR}/*.cpp")

# Exclude CLI tests when building REPL variant (they rely on production binary name)
# Exclude REPL tests when building CLI variant (they rely on REPL binary name)
if (REPL)
    list(FILTER TEST_SOURCES EXCLUDE REGEX ".*test_cli.cpp$")
else()
    list(FILTER TEST_SOURCES EXCLUDE REGEX ".*test_repl.cpp$")
    list(FILTER TEST_SOURCES EXCLUDE REGEX ".*test_debugging.cpp$")
endif()

add_executable(test_computo ${TEST_SOURCES})

target_link_libraries(test_computo PRIVATE computo GTest::gtest_main)

add_test(NAME computo_tests COMMAND test_computo)

# CLI / REPL executable
if (REPL)
    # REPL-enabled build: compile the REPL main and link readline
    add_executable(computo_app src/repl.cpp src/benchmark.cpp)
    target_link_libraries(computo_app PRIVATE computo ${READLINE_LIB})
    target_compile_definitions(computo_app PRIVATE REPL)
    # Also enable REPL features in the core library
    target_compile_definitions(computo PUBLIC REPL)
    # Final binary name: computo_repl
    set_target_properties(computo_app PROPERTIES OUTPUT_NAME computo_repl)
else()
    # Production CLI build: minimal binary, no readline dependency
    # Build main production binary named "computo"
    add_executable(computo_app src/cli.cpp src/benchmark.cpp)
    target_link_libraries(computo_app PRIVATE computo)
    set_target_properties(computo_app PROPERTIES OUTPUT_NAME computo)
endif()

# Remove legacy always-built targets if they existed
# (No further action required as we no longer generate them) 