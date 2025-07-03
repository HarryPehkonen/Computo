# ============================================================================
# Executable.cmake - CLI executable target definition
# ============================================================================

# Create CLI executable with complete debugging support
add_executable(computo_cli cli/main.cpp)

# Link to the main library
target_link_libraries(computo_cli PRIVATE computo)

# Ensure CLI has same readline support as library
if(COMPUTO_HAS_READLINE)
    target_compile_definitions(computo_cli PRIVATE COMPUTO_USE_READLINE)
endif()

# Set the output name to 'computo' (without _cli suffix)
set_target_properties(computo_cli PROPERTIES OUTPUT_NAME computo)

# Executable properties
set_target_properties(computo_cli PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Add install/strip target for optimized deployment
add_custom_target(install-strip
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target install
    COMMAND ${CMAKE_STRIP} $<TARGET_FILE:computo_cli>
    COMMENT "Installing and stripping binaries for optimized deployment"
)

message(STATUS "CLI executable 'computo' configured") 