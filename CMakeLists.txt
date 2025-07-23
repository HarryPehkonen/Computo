cmake_minimum_required(VERSION 3.15)
project(Computo VERSION 1.0.0)

# --- Build Configuration ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Code Quality Integration ---
option(ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" OFF)
option(ENABLE_CLANG_FORMAT "Enable clang-format code formatting" OFF)

# --- Memory Debugging Configuration ---
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_MSAN "Enable MemorySanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE clang-tidy)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
        message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
    else()
        message(FATAL_ERROR "clang-tidy requested but not found")
    endif()
else()
    message(STATUS "clang-tidy disabled (use -DENABLE_CLANG_TIDY=ON to enable)")
endif()

# Memory debugging flags setup
set(MEMORY_DEBUG_FLAGS "")
set(MEMORY_DEBUG_LIBS "")

if(ENABLE_ASAN)
    list(APPEND MEMORY_DEBUG_FLAGS -fsanitize=address -fno-omit-frame-pointer -g)
    list(APPEND MEMORY_DEBUG_LIBS -fsanitize=address)
    message(STATUS "AddressSanitizer enabled")
endif()

if(ENABLE_MSAN)
    list(APPEND MEMORY_DEBUG_FLAGS -fsanitize=memory -fno-omit-frame-pointer -g)
    list(APPEND MEMORY_DEBUG_LIBS -fsanitize=memory)
    message(STATUS "MemorySanitizer enabled")
endif()

if(ENABLE_TSAN)
    list(APPEND MEMORY_DEBUG_FLAGS -fsanitize=thread -fno-omit-frame-pointer -g)
    list(APPEND MEMORY_DEBUG_LIBS -fsanitize=thread)
    message(STATUS "ThreadSanitizer enabled")
endif()

if(ENABLE_UBSAN)
    list(APPEND MEMORY_DEBUG_FLAGS -fsanitize=undefined -fno-omit-frame-pointer -g)
    list(APPEND MEMORY_DEBUG_LIBS -fsanitize=undefined)
    message(STATUS "UndefinedBehaviorSanitizer enabled")
endif()

if(MEMORY_DEBUG_FLAGS)
    message(STATUS "Memory debugging flags: ${MEMORY_DEBUG_FLAGS}")
    message(STATUS "Memory debugging libs: ${MEMORY_DEBUG_LIBS}")
endif()

# --- Dependencies ---
find_package(nlohmann_json REQUIRED)
find_package(GTest REQUIRED)
find_library(READLINE_LIB readline)
if(READLINE_LIB)
    message(STATUS "readline found: ${READLINE_LIB}")
else()
    message(WARNING "readline not found - REPL will use basic input")
endif()

# --- Source Files ---
set(COMPUTO_LIB_SOURCES
    src/computo.cpp
    src/debug_context.cpp
    src/operators/shared.cpp
    src/operators/arithmetic.cpp
    src/operators/comparison.cpp
    src/operators/data_access.cpp
    src/operators/logical.cpp
    src/operators/control_flow.cpp
    src/operators/object_ops.cpp
    src/operators/array_ops.cpp
    src/operators/functional_ops.cpp
    src/operators/string_utility_ops.cpp
    src/operators/sort_utils.cpp
)
set(COMPUTO_HEADERS
    include/computo.hpp
)

# --- Libraries ---

# Core Library (libcomputo) - compiled once for all targets
add_library(computo STATIC ${COMPUTO_LIB_SOURCES} ${COMPUTO_HEADERS})
target_include_directories(computo PUBLIC include PRIVATE src)
target_link_libraries(computo PUBLIC nlohmann_json::nlohmann_json)
set_target_properties(computo PROPERTIES OUTPUT_NAME "computo")

# Apply memory debugging flags if enabled
if(MEMORY_DEBUG_FLAGS)
    target_compile_options(computo PRIVATE ${MEMORY_DEBUG_FLAGS})
    target_link_libraries(computo PRIVATE ${MEMORY_DEBUG_LIBS})
endif()

# --- Executables ---

# Unified CLI (computo) - supports both script execution and REPL modes
add_executable(computo_unified src/main.cpp src/cli_args.cpp src/repl.cpp)
target_link_libraries(computo_unified PRIVATE computo)
if(READLINE_LIB)
    target_link_libraries(computo_unified PRIVATE ${READLINE_LIB})
    target_compile_definitions(computo_unified PRIVATE COMPUTO_USE_READLINE)
endif()
set_target_properties(computo_unified PROPERTIES OUTPUT_NAME computo)

# Apply memory debugging flags to executable if enabled
if(MEMORY_DEBUG_FLAGS)
    target_compile_options(computo_unified PRIVATE ${MEMORY_DEBUG_FLAGS})
    target_link_libraries(computo_unified PRIVATE ${MEMORY_DEBUG_LIBS})
endif()

# --- Tests ---
enable_testing()

# Core Library Tests (test_computo)
add_executable(test_computo tests/test_arithmetic.cpp tests/test_comparison.cpp tests/test_data_access.cpp tests/test_shared.cpp tests/test_tco.cpp tests/test_control_flow.cpp tests/test_logical.cpp tests/test_object_ops.cpp tests/test_array_ops.cpp tests/test_functional_ops.cpp tests/test_string_utility_ops.cpp tests/test_cli_integration.cpp tests/test_debug_integration.cpp tests/test_memory_safety.cpp tests/test_rule3_arrays.cpp)
target_link_libraries(test_computo PRIVATE computo GTest::gtest_main)
target_include_directories(test_computo PRIVATE include tests src)
target_compile_definitions(test_computo PRIVATE COMPUTO_BINARY_PATH="$<TARGET_FILE:computo_unified>")
add_test(NAME computo_tests COMMAND test_computo)

# Apply memory debugging flags to tests if enabled
if(MEMORY_DEBUG_FLAGS)
    target_compile_options(test_computo PRIVATE ${MEMORY_DEBUG_FLAGS})
    target_link_libraries(test_computo PRIVATE ${MEMORY_DEBUG_LIBS})
endif()

# Memory Safety Tests (separate target for focused memory testing)
add_executable(test_memory_safety tests/test_memory_safety.cpp)
target_link_libraries(test_memory_safety PRIVATE computo GTest::gtest_main)
target_include_directories(test_memory_safety PRIVATE include tests src)
add_test(NAME memory_safety_tests COMMAND test_memory_safety)

# Apply memory debugging flags to memory safety tests if enabled
if(MEMORY_DEBUG_FLAGS)
    target_compile_options(test_memory_safety PRIVATE ${MEMORY_DEBUG_FLAGS})
    target_link_libraries(test_memory_safety PRIVATE ${MEMORY_DEBUG_LIBS})
endif()

# Thread Safety Tests (separate target for focused thread safety testing)
add_executable(test_thread_safety tests/test_thread_safety.cpp)
target_link_libraries(test_thread_safety PRIVATE computo GTest::gtest_main)
target_include_directories(test_thread_safety PRIVATE include tests src)

# Link with pthread on Unix platforms for thread support
if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(test_thread_safety PRIVATE Threads::Threads)
endif()

# Apply memory debugging flags to thread safety tests if enabled
if(MEMORY_DEBUG_FLAGS)
    target_compile_options(test_thread_safety PRIVATE ${MEMORY_DEBUG_FLAGS})
    target_link_libraries(test_thread_safety PRIVATE ${MEMORY_DEBUG_LIBS})
endif()

# Add thread safety tests to CTest
add_test(NAME thread_safety_tests COMMAND test_thread_safety)

# ThreadSanitizer is particularly useful for thread safety testing
if(ENABLE_TSAN)
    message(STATUS "ThreadSanitizer enabled for thread safety testing")
endif()

# Performance Benchmarks (separate target for performance testing)
add_executable(test_performance tests/test_performance.cpp)
target_link_libraries(test_performance PRIVATE computo GTest::gtest_main)
target_include_directories(test_performance PRIVATE include tests src)

# Link with pthread on Unix platforms for thread support
if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(test_performance PRIVATE Threads::Threads)
endif()

# Apply memory debugging flags to performance tests if enabled (for memory tracking)
if(MEMORY_DEBUG_FLAGS)
    target_compile_options(test_performance PRIVATE ${MEMORY_DEBUG_FLAGS})
    target_link_libraries(test_performance PRIVATE ${MEMORY_DEBUG_LIBS})
endif()

# Add performance benchmarks to CTest (but not run by default)
add_test(NAME performance_benchmarks COMMAND test_performance)
set_tests_properties(performance_benchmarks PROPERTIES 
    LABELS "performance"
    TIMEOUT 300
)

# Custom target for running performance benchmarks
add_custom_target(benchmark
    COMMAND test_performance
    DEPENDS test_performance
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running performance benchmarks"
    VERBATIM
)

# --- Code Quality Targets ---

# Collect all source files for formatting/linting
set(ALL_SOURCE_FILES
    ${COMPUTO_LIB_SOURCES}
    ${COMPUTO_HEADERS}
    src/main.cpp
)
file(GLOB_RECURSE TEST_FILES tests/*.cpp tests/*.hpp)
list(APPEND ALL_SOURCE_FILES ${TEST_FILES})

# clang-format target
find_program(CLANG_FORMAT_EXE clang-format)
if(CLANG_FORMAT_EXE)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i ${ALL_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-format on all source files"
        VERBATIM
    )
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror ${ALL_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking code formatting with clang-format"
        VERBATIM
    )
    message(STATUS "clang-format targets available: 'format' and 'format-check'")
else()
    message(STATUS "clang-format not found - formatting targets disabled")
endif()

# clang-tidy target (separate from build)
find_program(CLANG_TIDY_EXE clang-tidy)
if(CLANG_TIDY_EXE)
    add_custom_target(lint
        COMMAND ${CLANG_TIDY_EXE} ${COMPUTO_LIB_SOURCES} src/main.cpp 
        -p ${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy static analysis"
        VERBATIM
    )
    message(STATUS "clang-tidy target available: 'lint'")
else()
    message(STATUS "clang-tidy not found - linting targets disabled")
endif()

# Combined quality target
add_custom_target(quality
    COMMENT "Running all code quality checks"
)
if(CLANG_FORMAT_EXE)
    add_dependencies(quality format-check)
endif()
if(CLANG_TIDY_EXE)
    add_dependencies(quality lint)
endif()

# --- Install Configuration ---
install(TARGETS computo DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
install(TARGETS computo_unified DESTINATION bin)

# --- Project Information ---
message(STATUS "=== Computo Configuration ===")
message(STATUS "Version: ${CMAKE_PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
