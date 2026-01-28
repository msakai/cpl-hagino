#!/bin/bash
# run-tests.sh - Run CPL test suite

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "CPL Test Runner"
echo "==============="
echo ""
echo "Project directory: $PROJECT_DIR"
echo ""

# Check if SBCL is available
if ! command -v sbcl &> /dev/null; then
    echo "ERROR: SBCL not found. Please install SBCL to run tests."
    exit 1
fi

echo "Running tests with SBCL..."
echo ""

cd "$PROJECT_DIR"

sbcl --noinform \
     --no-sysinit \
     --no-userinit \
     --disable-debugger \
     --load "$SCRIPT_DIR/run-tests.lisp"

exit $?
