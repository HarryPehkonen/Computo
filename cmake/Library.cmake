# ============================================================================
# Library.cmake - Main Computo library target definition
# ============================================================================

# Define all source files for the library
set(COMPUTO_SOURCES
    src/computo.cpp
    src/debugger_manager.cpp
    src/evaluation_utils.cpp
    src/repl.cpp
    src/operators/arithmetic.cpp
    src/operators/logical.cpp
    src/operators/comparison.cpp
    src/operators/array.cpp
    src/operators/utility.cpp
    src/operators/list.cpp
    src/operators/string.cpp
)

# Create computo library
add_library(computo ${COMPUTO_SOURCES})

# Set up include directories
target_include_directories(computo PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(computo PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(computo PRIVATE ${PERMUTO_LIBRARY})
target_include_directories(computo PRIVATE ${PERMUTO_INCLUDE_DIR})

# Optional readline support
if(COMPUTO_HAS_READLINE)
    target_link_libraries(computo PRIVATE ${READLINE_LIBRARY})
    target_include_directories(computo PRIVATE ${READLINE_INCLUDE_DIR})
    target_compile_definitions(computo PRIVATE COMPUTO_USE_READLINE)
    message(STATUS "Computo library configured with readline support")
endif()

# Library properties
set_target_properties(computo PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

message(STATUS "Computo library configured with ${CMAKE_SYSTEM_PROCESSOR} architecture") 