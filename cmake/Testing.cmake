# ============================================================================
# Testing.cmake - Test setup and configuration
# ============================================================================

# Enable testing
enable_testing()

# Define all test source files
set(COMPUTO_TEST_SOURCES
    tests/test_main.cpp
    tests/test_builder.cpp
    tests/builder_examples.cpp
    tests/test_basic_operators.cpp
    tests/test_data_access.cpp
    tests/test_logic_construction.cpp
    tests/test_iteration_lambdas.cpp
    tests/test_array_utilities.cpp
    tests/test_lambda_analysis.cpp
    tests/test_permuto_integration.cpp
    tests/test_json_patch.cpp
    tests/test_logical_operators.cpp
    tests/test_advanced_arrays.cpp
    tests/test_nary_operators.cpp
    tests/test_error_handling_edge_cases.cpp
    tests/test_builder_fixes.cpp
    tests/test_debugging.cpp
    tests/test_memory_pool.cpp
    tests/test_thread_safety.cpp
)

# Create test executable
add_executable(computo_tests ${COMPUTO_TEST_SOURCES})

# Link to library and test framework
target_link_libraries(computo_tests PRIVATE 
    computo 
    GTest::gtest 
    GTest::gtest_main
)

# Test properties
set_target_properties(computo_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Register test with CTest
add_test(NAME ComputoTests COMMAND computo_tests)

# Set test properties for better output
set_tests_properties(ComputoTests PROPERTIES
    TIMEOUT 300
    LABELS "unit"
)

# Additional test targets
add_custom_target(test-verbose
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS computo_tests
    COMMENT "Running tests with verbose output"
)

add_custom_target(test-parallel
    COMMAND ${CMAKE_CTEST_COMMAND} --parallel ${CMAKE_BUILD_PARALLEL_LEVEL}
    DEPENDS computo_tests
    COMMENT "Running tests in parallel"
)

message(STATUS "Testing configured with ${CMAKE_CTEST_COMMAND}") 