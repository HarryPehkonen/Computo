# ============================================================================
# Documentation.cmake - Documentation generation targets
# ============================================================================

# Documentation generation targets
# These require Python 3.11+ for tomllib support

# Check for Python virtual environment
set(PYTHON_VENV "${CMAKE_SOURCE_DIR}/venv/bin/python")
if(EXISTS "${PYTHON_VENV}")
    set(PYTHON_EXECUTABLE "${PYTHON_VENV}")
    message(STATUS "Using Python virtual environment: ${PYTHON_EXECUTABLE}")
else()
    find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
    set(PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")
    message(STATUS "Using system Python: ${PYTHON_EXECUTABLE}")
endif()

# Generate README.md from README.toml
add_custom_target(readme
    COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_SOURCE_DIR}/scripts/generate_readme.py"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generating README.md from README.toml"
    DEPENDS "${CMAKE_SOURCE_DIR}/README.toml"
    BYPRODUCTS "${CMAKE_SOURCE_DIR}/README.md"
)

# Generate examples from README.toml
add_custom_target(examples
    COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_SOURCE_DIR}/scripts/generate_examples.py"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generating test examples from README.toml"
    DEPENDS "${CMAKE_SOURCE_DIR}/README.toml"
    BYPRODUCTS "${CMAKE_SOURCE_DIR}/examples"
)

# Generate both README and examples
add_custom_target(docs
    COMMENT "Generating all documentation"
)
add_dependencies(docs readme examples)

# Run documentation tests (requires computo binary)
add_custom_target(doctest
    COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_SOURCE_DIR}/examples" "./run_all.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Running documentation example tests"
    DEPENDS computo_cli examples
)

# Full documentation workflow: generate docs and test them
add_custom_target(docs-test
    COMMENT "Generate documentation and run all tests"
)
add_dependencies(docs-test docs doctest)

# Convenience targets with better names
add_custom_target(doc 
    COMMENT "Alias for 'docs' (shorter name)"
)
add_dependencies(doc docs)

add_custom_target(test-docs
    COMMENT "Alias for 'docs-test' (more intuitive name)"  
)
add_dependencies(test-docs docs-test)

# Help target to show available documentation commands
add_custom_target(help-docs
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Computo Documentation Targets:"
    COMMAND ${CMAKE_COMMAND} -E echo "=============================="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Generation:"
    COMMAND ${CMAKE_COMMAND} -E echo "  readme       Generate README.md from README.toml"
    COMMAND ${CMAKE_COMMAND} -E echo "  examples     Generate examples from README.toml"
    COMMAND ${CMAKE_COMMAND} -E echo "  docs or doc  Generate both README.md and examples"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Testing:"
    COMMAND ${CMAKE_COMMAND} -E echo "  doctest      Run example tests - requires examples"
    COMMAND ${CMAKE_COMMAND} -E echo "  docs-test    Generate docs and run all tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  test-docs    Alias for docs-test"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Usage from build directory:"
    COMMAND ${CMAKE_COMMAND} -E echo "  cd build"
    COMMAND ${CMAKE_COMMAND} -E echo "  make docs-test"
    COMMAND ${CMAKE_COMMAND} -E echo "  make doc"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMENT "Show documentation targets help"
)

message(STATUS "Documentation targets configured") 