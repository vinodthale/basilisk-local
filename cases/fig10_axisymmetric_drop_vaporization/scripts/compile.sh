#!/bin/bash
################################################################################
# Compilation script for nondimensional drop vaporization solver
################################################################################
# Usage: ./compile.sh
#
# Compiles the drop_vaporization_nondim.c solver using Basilisk qcc compiler
################################################################################

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================"
echo "Compiling Drop Vaporization Solver"
echo "========================================"
echo ""

# Check if BASILISK environment variable is set
if [ -z "$BASILISK" ]; then
    echo -e "${RED}ERROR: BASILISK environment variable is not set${NC}"
    echo "Please set it to the Basilisk source directory:"
    echo "  export BASILISK=/path/to/basilisk/src"
    exit 1
fi

echo -e "${GREEN}BASILISK path: $BASILISK${NC}"

# Check if Basilisk qcc exists
if [ ! -f "$BASILISK/qcc" ]; then
    echo -e "${RED}ERROR: qcc compiler not found at $BASILISK/qcc${NC}"
    exit 1
fi

# Get the directory of this script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

echo "Project directory: $PROJECT_DIR"
echo ""

# Create build directory
BUILD_DIR="$PROJECT_DIR/build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo -e "${YELLOW}Compiling drop_vaporization_nondim.c...${NC}"
echo ""

# Source file
SRC_FILE="$PROJECT_DIR/src/drop_vaporization_nondim.c"

if [ ! -f "$SRC_FILE" ]; then
    echo -e "${RED}ERROR: Source file not found: $SRC_FILE${NC}"
    exit 1
fi

# Copy source to build directory (qcc requires this)
cp "$SRC_FILE" .
cp "$PROJECT_DIR/src/constants_vaporization.h" .

# Compile with qcc
EXEC_NAME="drop_vaporization_nondim"

echo "Compiler: $BASILISK/qcc"
echo "Source: drop_vaporization_nondim.c"
echo "Output: $EXEC_NAME"
echo ""

# Compilation flags
# -O2: Optimization level 2
# -Wall: Enable all warnings
# -lm: Link math library
$BASILISK/qcc -O2 -Wall -o "$EXEC_NAME" drop_vaporization_nondim.c -lm

if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Compilation successful!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo "Executable: $BUILD_DIR/$EXEC_NAME"
    echo ""
    echo "To run a single case:"
    echo "  cd $BUILD_DIR"
    echo "  ./$EXEC_NAME R100 W1.5"
    echo ""
    echo "Or use the run scripts:"
    echo "  cd $PROJECT_DIR/scripts"
    echo "  ./run_single_test.sh 100 1.5"
    echo "  ./run_parameter_sweep.sh"
    echo ""
else
    echo ""
    echo -e "${RED}========================================${NC}"
    echo -e "${RED}Compilation failed!${NC}"
    echo -e "${RED}========================================${NC}"
    exit 1
fi
