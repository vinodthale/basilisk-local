#!/bin/bash
################################################################################
# Run a single drop vaporization test case
################################################################################
# Usage: ./run_single_test.sh [REYNOLDS] [WEBER]
#
# Examples:
#   ./run_single_test.sh 100 1.5
#   ./run_single_test.sh 50 1.5
#
# The test will run with specified Reynolds and Weber numbers and save
# results in ../results/Re_XXX_We_YYY/
################################################################################

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
BUILD_DIR="$PROJECT_DIR/build"
RESULTS_DIR="$PROJECT_DIR/results"

# Parse arguments
REYNOLDS=${1:-100.0}
WEBER=${2:-1.5}

echo "========================================"
echo "Drop Vaporization Test - Single Case"
echo "========================================"
echo ""
echo -e "${BLUE}Parameters:${NC}"
echo "  Reynolds number: $REYNOLDS"
echo "  Weber number:    $WEBER"
echo ""

# Check if executable exists
EXECUTABLE="$BUILD_DIR/drop_vaporization_nondim"

if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${YELLOW}Executable not found. Compiling...${NC}"
    cd "$SCRIPT_DIR"
    ./compile.sh
    if [ $? -ne 0 ]; then
        echo -e "${RED}Compilation failed!${NC}"
        exit 1
    fi
fi

# Create results directory for this case
CASE_DIR="$RESULTS_DIR/Re_$(printf "%.2f" $REYNOLDS)_We_$(printf "%.2f" $WEBER)"
mkdir -p "$CASE_DIR"

echo -e "${GREEN}Results will be saved to:${NC}"
echo "  $CASE_DIR"
echo ""

# Change to case directory
cd "$CASE_DIR"

# Run the simulation
echo -e "${YELLOW}Starting simulation...${NC}"
echo "Command: $EXECUTABLE R$REYNOLDS W$WEBER"
echo ""

# Run with Reynolds and Weber number arguments
$EXECUTABLE R$REYNOLDS W$WEBER 2>&1 | tee simulation.log

if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Simulation completed successfully!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo "Output files:"
    ls -lh "$CASE_DIR"
    echo ""
    echo "Volume history: $CASE_DIR/volume_history.txt"
    echo "Simulation log: $CASE_DIR/simulation.log"
    echo ""
else
    echo ""
    echo -e "${RED}========================================${NC}"
    echo -e "${RED}Simulation failed!${NC}"
    echo -e "${RED}========================================${NC}"
    echo "Check log file: $CASE_DIR/simulation.log"
    exit 1
fi
