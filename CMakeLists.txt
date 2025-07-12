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

# Find dependencies
find_package(nlohmann_json REQUIRED)
find_package(GTest REQUIRED)

# readline is required for REPL builds
find_library(READLINE_LIB readline)
if(NOT READLINE_LIB)
    message(FATAL_ERROR "readline library not found - required for REPL functionality")
endif()

# Directory structure variables
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

# Phase 1: library source files
file(GLOB_RECURSE COMPUTO_SOURCES CONFIGURE_DEPENDS
    "${SRC_DIR}/computo.cpp"
    "${SRC_DIR}/operators/*.cpp"
)

# Always build both libraries
add_library(computo ${COMPUTO_SOURCES})
target_include_directories(computo PUBLIC ${INCLUDE_DIR})
target_link_libraries(computo PUBLIC nlohmann_json::nlohmann_json)

add_library(computorepl ${COMPUTO_SOURCES})
target_include_directories(computorepl PUBLIC ${INCLUDE_DIR})
target_link_libraries(computorepl PUBLIC nlohmann_json::nlohmann_json)
target_compile_definitions(computorepl PUBLIC REPL)

# Tests
enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${TEST_DIR}/*.cpp")

# Production tests (includes CLI tests, excludes REPL/debugging tests)
set(PRODUCTION_TEST_SOURCES ${TEST_SOURCES})
list(FILTER PRODUCTION_TEST_SOURCES EXCLUDE REGEX ".*test_repl.cpp$")
list(FILTER PRODUCTION_TEST_SOURCES EXCLUDE REGEX ".*test_debugging.cpp$")

add_executable(test_computo ${PRODUCTION_TEST_SOURCES})
target_link_libraries(test_computo PRIVATE computo GTest::gtest_main)
add_test(NAME computo_tests COMMAND test_computo)

# REPL tests (includes REPL/debugging tests, excludes CLI tests)
set(REPL_TEST_SOURCES ${TEST_SOURCES})
list(FILTER REPL_TEST_SOURCES EXCLUDE REGEX ".*test_cli.cpp$")

add_executable(test_computo_repl ${REPL_TEST_SOURCES})
target_link_libraries(test_computo_repl PRIVATE computorepl GTest::gtest_main)
add_test(NAME computo_repl_tests COMMAND test_computo_repl)

# CLI executables
# Always build both CLI tools
add_executable(computo_app src/cli.cpp src/benchmark.cpp)
target_link_libraries(computo_app PRIVATE computo)
set_target_properties(computo_app PROPERTIES OUTPUT_NAME computo)

add_executable(computo_repl_app src/repl.cpp src/benchmark.cpp)
target_link_libraries(computo_repl_app PRIVATE computorepl ${READLINE_LIB})
target_compile_definitions(computo_repl_app PRIVATE REPL)
set_target_properties(computo_repl_app PROPERTIES OUTPUT_NAME computo_repl)

# Remove legacy always-built targets if they existed
# (No further action required as we no longer generate them) 