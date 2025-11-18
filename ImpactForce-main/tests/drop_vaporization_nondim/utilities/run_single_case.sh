#!/bin/bash
################################################################################
# Single Case Runner for Nondimensional Drop Vaporization Test
################################################################################
#
# Usage:
#   ./run_single_case.sh RE [OUTPUT_DIR]
#
# Arguments:
#   RE          Reynolds number (e.g., 100)
#   OUTPUT_DIR  Output directory (optional, default: results/Re_XXX_We_1.50)
#
# Example:
#   ./run_single_case.sh 100
#   ./run_single_case.sh 100 my_results
#
################################################################################

set -e  # Exit on error

# Default parameters
WE=1.5
LMAX=12

# Check arguments
if [ $# -lt 1 ]; then
  echo "Usage: $0 RE [OUTPUT_DIR]"
  echo ""
  echo "Arguments:"
  echo "  RE          Reynolds number (e.g., 100)"
  echo "  OUTPUT_DIR  Output directory (optional)"
  echo ""
  echo "Example:"
  echo "  $0 100"
  echo "  $0 100 my_results"
  exit 1
fi

RE=$1

# Directories
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
TEST_DIR="$(dirname "$SCRIPT_DIR")"
RESULTS_DIR="$TEST_DIR/results"

# Output directory
if [ $# -ge 2 ]; then
  OUTPUT_DIR="$2"
else
  OUTPUT_DIR="$RESULTS_DIR/Re_$(printf "%.2f" $RE)_We_$(printf "%.2f" $WE)"
fi

# Test case files
TEST_CASE="$TEST_DIR/test_cases/test_drop_vap_nondim.c"
EXECUTABLE="$TEST_DIR/test_cases/test_drop_vap_nondim"

# Print configuration
echo "========================================"
echo "Nondimensional Drop Vaporization Test"
echo "========================================"
echo ""
echo "Parameters:"
echo "  Re = $RE"
echo "  We = $WE (fixed)"
echo "  Lmax = $LMAX"
echo ""
echo "Output directory:"
echo "  $OUTPUT_DIR"
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Compile if needed
echo "========================================"
echo "Compiling..."
echo "========================================"
echo ""

cd "$TEST_DIR/test_cases"

if [ ! -f "$EXECUTABLE" ] || [ "$TEST_CASE" -nt "$EXECUTABLE" ]; then
  echo "Compiling test case..."

  # Check for Basilisk compiler
  if ! command -v qcc &> /dev/null; then
    echo "Error: Basilisk compiler 'qcc' not found!"
    echo "Please set BASILISK environment variable and add qcc to PATH"
    exit 1
  fi

  # Compile
  qcc -O2 -Wall test_drop_vap_nondim.c -o test_drop_vap_nondim -lm

  if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
  fi

  echo "Compilation successful!"
else
  echo "Executable is up to date, skipping compilation"
fi

echo ""

# Run the simulation
echo "========================================"
echo "Running simulation..."
echo "========================================"
echo ""

START_TIME=$(date +%s)

./test_drop_vap_nondim $RE "$OUTPUT_DIR" 2>&1 | tee "$OUTPUT_DIR/log.txt"

EXIT_CODE=${PIPESTATUS[0]}

END_TIME=$(date +%s)
RUNTIME=$((END_TIME - START_TIME))

echo ""
echo "========================================"
echo "Simulation Complete"
echo "========================================"
echo ""
echo "Status: $([ $EXIT_CODE -eq 0 ] && echo "SUCCESS" || echo "FAILED")"
echo "Runtime: ${RUNTIME}s"
echo "Output directory: $OUTPUT_DIR"
echo ""

if [ $EXIT_CODE -eq 0 ]; then
  echo "Results:"
  if [ -f "$OUTPUT_DIR/liquid_volume_history.txt" ]; then
    echo "  Volume history: $OUTPUT_DIR/liquid_volume_history.txt"
    FINAL_V=$(tail -1 "$OUTPUT_DIR/liquid_volume_history.txt" | awk '{print $4}')
    echo "  Final V/V0: $FINAL_V"
  fi
  if [ -f "$OUTPUT_DIR/nusselt_history.txt" ]; then
    echo "  Nusselt history: $OUTPUT_DIR/nusselt_history.txt"
  fi
  if [ -f "$OUTPUT_DIR/frontal_area_history.txt" ]; then
    echo "  Frontal area history: $OUTPUT_DIR/frontal_area_history.txt"
  fi
  echo ""
fi

exit $EXIT_CODE
