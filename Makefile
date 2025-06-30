# Computo Makefile
# Build and test targets for the Computo JSON transformation engine

# Build configuration
BUILD_DIR = build
CMAKE_FLAGS = -DCMAKE_BUILD_TYPE=Debug

# Default target
.PHONY: all
all: build

# Configure CMake build
.PHONY: configure
configure:
	cmake -B $(BUILD_DIR) $(CMAKE_FLAGS)

# Build the project
.PHONY: build
build: configure
	cmake --build $(BUILD_DIR)

# Run tests
.PHONY: test
test: build
	cd $(BUILD_DIR) && ctest

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# Generate README.md and test examples from README.toml
.PHONY: test-examples
test-examples: build
	@echo "📝 Generating README.md from README.toml..."
	python generate_readme.py
	@echo "🔍 Generating examples from README.toml..."
	python generate_examples.py
	@echo "🚀 Running all generated examples..."
	cd examples && ./run_all.sh

# Generate examples only (without running tests)
.PHONY: generate-examples
generate-examples:
	@echo "🔍 Generating examples from README.toml..."
	python generate_examples.py

# Generate README.md from README.toml
.PHONY: generate-readme
generate-readme:
	@echo "📝 Generating README.md from README.toml..."
	python generate_readme.py

# Generate both README.md and examples
.PHONY: generate-docs
generate-docs: generate-readme generate-examples

# Run examples only (assumes examples already extracted)
.PHONY: run-examples
run-examples:
	@echo "🚀 Running all extracted examples..."
	cd examples && ./run_all.sh

# Clean examples directory
.PHONY: clean-examples
clean-examples:
	rm -rf examples

# Full clean (build + examples)
.PHONY: clean-all
clean-all: clean clean-examples

# Development targets
.PHONY: dev-setup
dev-setup: configure
	@echo "✅ Development environment configured"
	@echo "📋 Available targets:"
	@echo "  make build          - Build the project"
	@echo "  make test           - Run C++ tests"
	@echo "  make test-examples  - Extract and test README examples"
	@echo "  make clean          - Clean build artifacts"
	@echo "  make clean-all      - Clean everything"

# Show help
.PHONY: help
help:
	@echo "Computo Build System"
	@echo "==================="
	@echo ""
	@echo "Build Targets:"
	@echo "  configure        Configure CMake build"
	@echo "  build           Build the project"
	@echo "  test            Run C++ unit tests"
	@echo "  clean           Clean build artifacts"
	@echo ""
	@echo "Documentation & Testing:"
	@echo "  test-examples   Generate docs and test all examples from README.toml"
	@echo "  generate-docs   Generate both README.md and examples from README.toml"
	@echo "  generate-readme Generate README.md from README.toml"
	@echo "  generate-examples Generate examples from README.toml"
	@echo "  run-examples    Run examples (assumes already generated)"
	@echo "  extract-examples Legacy alias for generate-examples"
	@echo "  clean-examples  Remove examples directory"
	@echo ""
	@echo "Utility:"
	@echo "  clean-all       Clean everything (build + examples)"
	@echo "  dev-setup       Setup development environment"
	@echo "  help            Show this help message"