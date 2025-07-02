# ============================================================================
# Computo - JSON-native transformation engine with Lisp-like syntax
# ============================================================================

cmake_minimum_required(VERSION 3.15)
project(Computo VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# Project Configuration
# ============================================================================

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable all warnings and treat warnings as errors
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Project information
message(STATUS "Configuring Computo ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")

# ============================================================================
# Include Modular Build Components
# ============================================================================

include(cmake/Dependencies.cmake)    # Package finding and dependency management
include(cmake/Library.cmake)        # Main Computo library target
include(cmake/Executable.cmake)     # CLI executable definition
include(cmake/Testing.cmake)        # Test setup and configuration
include(cmake/Installation.cmake)   # Install targets and packaging
include(cmake/Documentation.cmake)  # Documentation generation targets

# ============================================================================
# Project Summary
# ============================================================================

message(STATUS "")
message(STATUS "Computo Configuration Summary:")
message(STATUS "===============================")
message(STATUS "Version:        ${PROJECT_VERSION}")
message(STATUS "Build type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Available targets:")
message(STATUS "  computo          - Main library")
message(STATUS "  computo_cli      - Command-line interface")
message(STATUS "  computo_tests    - Test suite")
message(STATUS "  docs             - Generate documentation")
message(STATUS "  install          - Install Computo")
message(STATUS "  uninstall        - Uninstall Computo")
message(STATUS "")
message(STATUS "For more targets, run: make help")
message(STATUS "")