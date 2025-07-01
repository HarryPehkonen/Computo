cmake_minimum_required(VERSION 3.15)
project(Computo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable all warnings and treat warnings as errors
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Find required packages
find_package(nlohmann_json REQUIRED)

# Find Permuto library (installed via make install)
find_library(PERMUTO_LIBRARY NAMES permuto PATHS /usr/local/lib)
find_path(PERMUTO_INCLUDE_DIR NAMES permuto/permuto.hpp PATHS /usr/local/include)

if(NOT PERMUTO_LIBRARY OR NOT PERMUTO_INCLUDE_DIR)
    message(FATAL_ERROR "Permuto library not found")
endif()

# Create computo library
add_library(computo src/computo.cpp)
target_include_directories(computo PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(computo PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(computo PRIVATE ${PERMUTO_LIBRARY})
target_include_directories(computo PRIVATE ${PERMUTO_INCLUDE_DIR})

# Create CLI executable
add_executable(computo_cli cli/main.cpp)
target_link_libraries(computo_cli PRIVATE computo)
set_target_properties(computo_cli PROPERTIES OUTPUT_NAME computo)

# Enable testing
enable_testing()

# Find Google Test
find_package(GTest REQUIRED)

# Create test executable
add_executable(computo_tests 
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
)
target_link_libraries(computo_tests PRIVATE 
    computo 
    GTest::gtest 
    GTest::gtest_main
)

# Add test
add_test(NAME ComputoTests COMMAND computo_tests)

# Install targets
install(TARGETS computo computo_cli
    EXPORT ComputoTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include/ DESTINATION include)

# Install export targets
install(EXPORT ComputoTargets
    FILE ComputoTargets.cmake
    NAMESPACE Computo::
    DESTINATION lib/cmake/Computo
)

# Create and install config files
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "ComputoConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ComputoConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ComputoConfig.cmake"
    INSTALL_DESTINATION lib/cmake/Computo
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ComputoConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ComputoConfigVersion.cmake"
    DESTINATION lib/cmake/Computo
)

# Add install/strip target
add_custom_target(install-strip
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target install
    COMMAND ${CMAKE_STRIP} $<TARGET_FILE:computo_cli>
    COMMENT "Installing and stripping binaries"
)