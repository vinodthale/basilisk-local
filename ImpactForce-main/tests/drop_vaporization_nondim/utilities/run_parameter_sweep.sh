#!/bin/bash
################################################################################
# Parameter Sweep Script for Nondimensional Drop Vaporization Test
################################################################################
#
# This script runs the nondimensional drop vaporization test for 25 Reynolds
# numbers linearly spaced from 22 to 200.
#
# Usage:
#   ./run_parameter_sweep.sh [OPTIONS]
#
# Options:
#   --quick        Run only 5 cases (Re = 22, 66.5, 111, 155.5, 200)
#   --parallel N   Run N cases in parallel (default: 1)
#   --lmax N       Set maximum refinement level (default: 12)
#   --skip-compile Skip compilation step
#
# Output:
#   results/Re_XXXX_We_1.50/   (one directory per case)
#   results/sweep_summary.txt   (summary of all cases)
#
################################################################################

set -e  # Exit on error

# Default parameters
NUM_CASES=25
RE_MIN=22.0
RE_MAX=200.0
WE=1.5
PARALLEL_JOBS=1
LMAX=12
SKIP_COMPILE=0
QUICK_MODE=0

# Directories
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
TEST_DIR="$(dirname "$SCRIPT_DIR")"
RESULTS_DIR="$TEST_DIR/results"
TEST_CASE="$TEST_DIR/test_cases/test_drop_vap_nondim.c"
EXECUTABLE="$TEST_DIR/test_cases/test_drop_vap_nondim"

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    --quick)
      QUICK_MODE=1
      NUM_CASES=5
      shift
      ;;
    --parallel)
      PARALLEL_JOBS="$2"
      shift 2
      ;;
    --lmax)
      LMAX="$2"
      shift 2
      ;;
    --skip-compile)
      SKIP_COMPILE=1
      shift
      ;;
    *)
      echo "Unknown option: $1"
      echo "Usage: $0 [--quick] [--parallel N] [--lmax N] [--skip-compile]"
      exit 1
      ;;
  esac
done

# Print configuration
echo "========================================"
echo "Drop Vaporization Parameter Sweep"
echo "========================================"
echo ""
echo "Configuration:"
echo "  Number of cases: $NUM_CASES"
echo "  Re range:        [$RE_MIN, $RE_MAX]"
echo "  We (fixed):      $WE"
echo "  Max level:       $LMAX"
echo "  Parallel jobs:   $PARALLEL_JOBS"
echo "  Quick mode:      $QUICK_MODE"
echo ""
echo "Directories:"
echo "  Test case:       $TEST_CASE"
echo "  Results:         $RESULTS_DIR"
echo ""

# Create results directory
mkdir -p "$RESULTS_DIR"

# Compile the test case
if [ $SKIP_COMPILE -eq 0 ]; then
  echo "========================================"
  echo "Compiling test case..."
  echo "========================================"
  echo ""

  cd "$TEST_DIR/test_cases"

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
  echo ""
else
  echo "Skipping compilation (using existing executable)"
  echo ""
fi

# Generate Reynolds numbers
echo "========================================"
echo "Generating Reynolds numbers..."
echo "========================================"
echo ""

# Use Python to generate linearly spaced values
RE_VALUES=$(python3 -c "
import numpy as np
re_values = np.linspace($RE_MIN, $RE_MAX, $NUM_CASES)
print(' '.join([f'{re:.2f}' for re in re_values]))
")

echo "Re values:"
echo "$RE_VALUES"
echo ""

# Convert to array
IFS=' ' read -r -a RE_ARRAY <<< "$RE_VALUES"

# Summary file
SUMMARY_FILE="$RESULTS_DIR/sweep_summary.txt"
echo "# Parameter Sweep Summary" > "$SUMMARY_FILE"
echo "# Generated: $(date)" >> "$SUMMARY_FILE"
echo "#" >> "$SUMMARY_FILE"
echo "# Configuration:" >> "$SUMMARY_FILE"
echo "#   Number of cases: $NUM_CASES" >> "$SUMMARY_FILE"
echo "#   Re range: [$RE_MIN, $RE_MAX]" >> "$SUMMARY_FILE"
echo "#   We (fixed): $WE" >> "$SUMMARY_FILE"
echo "#   Lmax: $LMAX" >> "$SUMMARY_FILE"
echo "#" >> "$SUMMARY_FILE"
echo "# Columns: Case | Re | We | Status | Runtime | V_final/V0 | Output_Dir" >> "$SUMMARY_FILE"
echo "#" >> "$SUMMARY_FILE"

# Function to run a single case
run_case() {
  local case_idx=$1
  local re_value=$2

  # Create output directory
  local output_dir="$RESULTS_DIR/Re_$(printf "%.2f" $re_value)_We_$(printf "%.2f" $WE)"
  mkdir -p "$output_dir"

  echo "----------------------------------------"
  echo "Case $case_idx: Re = $re_value, We = $WE"
  echo "Output: $output_dir"
  echo "----------------------------------------"

  # Run the simulation
  local start_time=$(date +%s)

  cd "$TEST_DIR/test_cases"
  ./test_drop_vap_nondim $re_value "$output_dir" > "$output_dir/log.txt" 2>&1

  local exit_code=$?
  local end_time=$(date +%s)
  local runtime=$((end_time - start_time))

  # Check status
  local status="FAILED"
  local v_final="N/A"

  if [ $exit_code -eq 0 ]; then
    status="SUCCESS"

    # Extract final volume ratio from volume history
    if [ -f "$output_dir/liquid_volume_history.txt" ]; then
      v_final=$(tail -1 "$output_dir/liquid_volume_history.txt" | awk '{print $4}')
    fi
  fi

  # Write to summary
  printf "%3d  %8.2f  %6.2f  %-7s  %5ds  %s  %s\n" \
    $case_idx $re_value $WE "$status" $runtime "$v_final" "$output_dir" \
    >> "$SUMMARY_FILE"

  echo "Status: $status (runtime: ${runtime}s)"
  echo ""

  return $exit_code
}

# Export function and variables for parallel execution
export -f run_case
export TEST_DIR RESULTS_DIR WE SUMMARY_FILE

# Run cases
echo "========================================"
echo "Running simulations..."
echo "========================================"
echo ""

# Track start time
SWEEP_START_TIME=$(date +%s)

if [ $PARALLEL_JOBS -gt 1 ]; then
  echo "Running $NUM_CASES cases with $PARALLEL_JOBS parallel jobs"
  echo ""

  # Use GNU parallel if available, otherwise use xargs
  if command -v parallel &> /dev/null; then
    printf "%s\n" "${!RE_ARRAY[@]}" | \
      parallel -j $PARALLEL_JOBS run_case {} "${RE_ARRAY[{}]}"
  else
    echo "Warning: GNU parallel not found, using sequential execution"
    for i in "${!RE_ARRAY[@]}"; do
      run_case $i "${RE_ARRAY[$i]}"
    done
  fi
else
  echo "Running $NUM_CASES cases sequentially"
  echo ""

  for i in "${!RE_ARRAY[@]}"; do
    run_case $i "${RE_ARRAY[$i]}"
  done
fi

# Track end time
SWEEP_END_TIME=$(date +%s)
SWEEP_RUNTIME=$((SWEEP_END_TIME - SWEEP_START_TIME))

echo "========================================"
echo "Parameter Sweep Complete!"
echo "========================================"
echo ""
echo "Total runtime: ${SWEEP_RUNTIME}s"
echo "Results saved to: $RESULTS_DIR"
echo "Summary file: $SUMMARY_FILE"
echo ""

# Display summary
echo "========================================"
echo "Summary:"
echo "========================================"
cat "$SUMMARY_FILE"
echo ""

# Count successes and failures
SUCCESS_COUNT=$(grep -c "SUCCESS" "$SUMMARY_FILE" || echo "0")
FAILED_COUNT=$(grep -c "FAILED" "$SUMMARY_FILE" || echo "0")

echo "Completed: $SUCCESS_COUNT success, $FAILED_COUNT failed"
echo ""

if [ $FAILED_COUNT -gt 0 ]; then
  echo "Warning: Some cases failed. Check log files in results directories."
  exit 1
fi

exit 0
