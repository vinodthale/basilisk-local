#!/bin/bash
#
# Run a single case of the axisymmetric drop vaporization simulation
#
# Usage: ./run_single_case.sh [REYNOLDS] [WEBER]
# Example: ./run_single_case.sh 100 1.5
#

# Exit on error
set -e

# Default parameters
REYNOLDS=${1:-100}
WEBER=${2:-1.5}

# Directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CASE_DIR="$(dirname "$SCRIPT_DIR")"
SRC_DIR="$CASE_DIR/src"
RESULTS_DIR="$CASE_DIR/results"
BUILD_DIR="$CASE_DIR/build"

# Create directories
mkdir -p "$BUILD_DIR"
mkdir -p "$RESULTS_DIR"

# Source file
SRC_FILE="$SRC_DIR/fig10_drop_vaporization.c"

# Output directory for this case
OUTPUT_DIR="$RESULTS_DIR/Re_${REYNOLDS}_We_${WEBER}"
mkdir -p "$OUTPUT_DIR"

# Check if Basilisk is available
if [ -z "$BASILISK" ]; then
    echo "Error: BASILISK environment variable not set"
    echo "Please set BASILISK to your Basilisk installation directory"
    exit 1
fi

echo "==================================================="
echo "Axisymmetric Drop Vaporization - Single Case"
echo "==================================================="
echo "Reynolds number: $REYNOLDS"
echo "Weber number: $WEBER"
echo "Output directory: $OUTPUT_DIR"
echo ""

# Compile the code
echo "Compiling $SRC_FILE"
cd "$BUILD_DIR"

$BASILISK/qcc -Wall -O2 -o fig10_drop_vaporization \
    -I$BASILISK/src \
    -lm \
    "$SRC_FILE"

if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

echo "Compilation successful!"
echo ""

# Run simulation
echo "Running simulation..."
echo ""

./fig10_drop_vaporization $REYNOLDS $WEBER "$OUTPUT_DIR" 2>&1 | tee "$OUTPUT_DIR/log.txt"

if [ $? -eq 0 ]; then
    echo ""
    echo "==================================================="
    echo "Simulation completed successfully!"
    echo "==================================================="
    echo "Results saved in: $OUTPUT_DIR"
    echo ""
    echo "Output files:"
    ls -lh "$OUTPUT_DIR"
else
    echo ""
    echo "Simulation failed or terminated early"
    exit 1
fi
