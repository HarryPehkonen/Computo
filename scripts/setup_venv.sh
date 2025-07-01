#!/bin/bash
# Setup Python virtual environment for Computo documentation generation

set -e

SCRIPT_DIR="$(dirname "$0")"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
VENV_DIR="$PROJECT_ROOT/venv"

echo "Setting up Python virtual environment for Computo documentation..."

# Check Python version
if ! python3 --version | grep -q "Python 3.1[1-9]"; then
    echo "❌ Error: Python 3.11+ is required for tomllib support"
    echo "Current version: $(python3 --version)"
    exit 1
fi

# Create virtual environment if it doesn't exist
if [ ! -d "$VENV_DIR" ]; then
    echo "📦 Creating virtual environment..."
    python3 -m venv "$VENV_DIR"
else
    echo "📦 Virtual environment already exists"
fi

# Activate virtual environment
echo "🔧 Activating virtual environment..."
source "$VENV_DIR/bin/activate"

# Upgrade pip
echo "📦 Upgrading pip..."
pip install --upgrade pip

# Install requirements (minimal for now)
echo "📦 Installing requirements..."
pip install -r "$SCRIPT_DIR/requirements.txt" || echo "No additional packages needed"

echo "✅ Virtual environment setup complete!"
echo ""
echo "To activate the environment:"
echo "  source venv/bin/activate"
echo ""
echo "Available CMake targets:"
echo "  make readme     - Generate README.md from README.toml"
echo "  make examples   - Generate examples from README.toml"
echo "  make docs       - Generate both README and examples"
echo "  make doctest    - Run example tests"
echo "  make docs-test  - Generate docs and run tests" 