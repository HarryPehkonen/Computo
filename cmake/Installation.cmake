# ============================================================================
# Installation.cmake - Install targets and packaging configuration
# ============================================================================

# Install targets
install(TARGETS computo computo_cli
    EXPORT ComputoTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include/ DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

# Install export targets
install(EXPORT ComputoTargets
    FILE ComputoTargets.cmake
    NAMESPACE Computo::
    DESTINATION lib/cmake/Computo
)

# Include CMake package helpers
include(CMakePackageConfigHelpers)

# Create package version file
write_basic_package_version_file(
    "ComputoConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Configure package config file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ComputoConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ComputoConfig.cmake"
    INSTALL_DESTINATION lib/cmake/Computo
)

# Install config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ComputoConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ComputoConfigVersion.cmake"
    DESTINATION lib/cmake/Computo
)

# Create uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)

if(NOT TARGET uninstall)
    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
        COMMENT "Uninstalling Computo"
    )
endif()

# Package information
set(CPACK_PACKAGE_NAME "Computo")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "JSON-native transformation engine with Lisp-like syntax")
set(CPACK_PACKAGE_VENDOR "Computo Project")

# Set optional license file if it exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
endif()

# Set optional README file if it exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
endif()

# Include CPack for package generation
include(CPack)

message(STATUS "Installation configured for prefix: ${CMAKE_INSTALL_PREFIX}") 