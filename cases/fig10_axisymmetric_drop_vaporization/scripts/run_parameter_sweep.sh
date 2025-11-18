#!/bin/bash
################################################################################
# Run parameter sweep for drop vaporization (Figure 10)
################################################################################
# Runs 25 cases with Reynolds numbers from 22 to 200
# Weber number fixed at 1.5
#
# Usage: ./run_parameter_sweep.sh [START_RE] [END_RE] [NUM_CASES]
#
# Defaults:
#   START_RE = 22
#   END_RE = 200
#   NUM_CASES = 25
################################################################################

# Exit on error
set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
WEBER=${WEBER:-1.5}
RE_MIN=${1:-22}
RE_MAX=${2:-200}
NUM_CASES=${3:-25}

# Directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CASE_DIR="$(dirname "$SCRIPT_DIR")"
SRC_DIR="$CASE_DIR/src"
RESULTS_DIR="$CASE_DIR/results"
BUILD_DIR="$CASE_DIR/build"

# Create directories
mkdir -p "$BUILD_DIR"
mkdir -p "$RESULTS_DIR"

# Source file and executable
SRC_FILE="$SRC_DIR/drop_vaporization_nondim.c"
EXECUTABLE="$BUILD_DIR/drop_vaporization_nondim"

echo "========================================"
echo "Drop Vaporization Parameter Sweep"
echo "========================================"
echo ""

# Check if Basilisk is available
if [ -z "$BASILISK" ]; then
    echo -e "${RED}Error: BASILISK environment variable not set${NC}"
    echo "Please set BASILISK to your Basilisk installation directory"
    echo "  export BASILISK=/path/to/basilisk/src"
    exit 1
fi

# Compile the code if not already compiled
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${YELLOW}Compiling solver...${NC}"
    cd "$SCRIPT_DIR"
    ./compile.sh
    if [ $? -ne 0 ]; then
        echo -e "${RED}Compilation failed!${NC}"
        exit 1
    fi
else
    echo -e "${GREEN}Using existing executable: $EXECUTABLE${NC}"
fi

# Calculate Reynolds numbers for the sweep
echo ""
echo -e "${BLUE}Parameter Sweep Configuration:${NC}"
echo "  Weber number (fixed): $WEBER"
echo "  Reynolds range: $RE_MIN to $RE_MAX"
echo "  Number of cases: $NUM_CASES"
echo ""

# Generate Reynolds numbers (linear spacing)
RE_VALUES=()
for i in $(seq 0 $((NUM_CASES-1))); do
    RE=$(awk -v min=$RE_MIN -v max=$RE_MAX -v n=$NUM_CASES -v i=$i \
        'BEGIN {printf "%.2f", min + (max-min)*i/(n-1)}')
    RE_VALUES+=($RE)
done

# Print all Re values
echo -e "${GREEN}Reynolds numbers:${NC}"
printf '%s\n' "${RE_VALUES[@]}" | column -c 80
echo ""

# Create summary file
SUMMARY_FILE="$RESULTS_DIR/sweep_summary.txt"
echo "# Drop Vaporization Parameter Sweep" > "$SUMMARY_FILE"
echo "# Date: $(date)" >> "$SUMMARY_FILE"
echo "# Reynolds range: $RE_MIN to $RE_MAX" >> "$SUMMARY_FILE"
echo "# Number of cases: $NUM_CASES" >> "$SUMMARY_FILE"
echo "# Weber number: $WEBER" >> "$SUMMARY_FILE"
echo "#" >> "$SUMMARY_FILE"
echo "# Case_ID  Reynolds  Weber  Status  Runtime_sec" >> "$SUMMARY_FILE"

# Run simulations
echo "========================================"
echo "Running Simulations"
echo "========================================"

FAILED_CASES=0
SUCCESSFUL_CASES=0

for i in "${!RE_VALUES[@]}"; do
    RE=${RE_VALUES[$i]}
    CASE_NUM=$(printf "%03d" $((i+1)))
    OUTPUT_DIR="$RESULTS_DIR/Re_${RE}_We_${WEBER}"

    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}Case $CASE_NUM/$NUM_CASES: Re = $RE, We = $WEBER${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo "Output directory: $OUTPUT_DIR"

    # Create output directory
    mkdir -p "$OUTPUT_DIR"
    cd "$OUTPUT_DIR"

    # Record start time
    START_TIME=$(date +%s)

    # Run simulation with Reynolds and Weber arguments
    if $EXECUTABLE R$RE W$WEBER > simulation.log 2>&1; then
        STATUS="SUCCESS"
        SUCCESSFUL_CASES=$((SUCCESSFUL_CASES + 1))
        echo -e "${GREEN}✓ Case completed successfully${NC}"
    else
        STATUS="FAILED"
        FAILED_CASES=$((FAILED_CASES + 1))
        echo -e "${RED}✗ Case failed - check log${NC}"
    fi

    # Record end time
    END_TIME=$(date +%s)
    RUNTIME=$((END_TIME - START_TIME))

    # Write to summary
    printf "%3d  %8.2f  %5.2f  %7s  %6d\n" \
        $CASE_NUM $RE $WEBER $STATUS $RUNTIME >> "$SUMMARY_FILE"

    echo "Runtime: $RUNTIME seconds"
done

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Parameter Sweep Complete${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Total cases:      $NUM_CASES"
echo "Successful cases: $SUCCESSFUL_CASES"
echo "Failed cases:     $FAILED_CASES"
echo ""
echo "Results directory: $RESULTS_DIR"
echo "Summary file:      $SUMMARY_FILE"
echo ""

# Display summary
cat "$SUMMARY_FILE"

if [ $FAILED_CASES -gt 0 ]; then
    echo ""
    echo -e "${YELLOW}Warning: Some cases failed. Check individual log files.${NC}"
    exit 1
else
    echo ""
    echo -e "${GREEN}All cases completed successfully!${NC}"
fi

echo ""
echo "Next steps:"
echo "  1. View summary: cat $SUMMARY_FILE"
echo "  2. Analyze results with Python scripts in scripts/"
echo ""
