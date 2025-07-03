# ============================================================================
# Dependencies.cmake - Package finding and dependency management
# ============================================================================

# Find required packages
find_package(nlohmann_json REQUIRED)

# Find Permuto library (installed via make install)
find_library(PERMUTO_LIBRARY NAMES permuto PATHS /usr/local/lib)
find_path(PERMUTO_INCLUDE_DIR NAMES permuto/permuto.hpp PATHS /usr/local/include)

if(NOT PERMUTO_LIBRARY OR NOT PERMUTO_INCLUDE_DIR)
    message(FATAL_ERROR "Permuto library not found. Please install Permuto first.")
endif()

# Find Google Test (for testing module)
find_package(GTest REQUIRED)

# Optional readline support for REPL command history
find_path(READLINE_INCLUDE_DIR readline/readline.h)
find_library(READLINE_LIBRARY NAMES readline)

if(READLINE_INCLUDE_DIR AND READLINE_LIBRARY)
    message(STATUS "Found readline: ${READLINE_LIBRARY}")
    set(COMPUTO_HAS_READLINE TRUE)
else()
    message(STATUS "Readline not found - REPL will work without command history")
    set(COMPUTO_HAS_READLINE FALSE)
endif()

# Status messages
message(STATUS "Dependencies found:")
message(STATUS "  nlohmann_json: ${nlohmann_json_VERSION}")
message(STATUS "  Permuto library: ${PERMUTO_LIBRARY}")
message(STATUS "  Permuto includes: ${PERMUTO_INCLUDE_DIR}")
message(STATUS "  Google Test: ${GTEST_VERSION}")
if(COMPUTO_HAS_READLINE)
    message(STATUS "  Readline: ${READLINE_LIBRARY} (command history enabled)")
else()
    message(STATUS "  Readline: NOT FOUND (basic input only)")
endif() 