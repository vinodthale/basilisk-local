#!/bin/bash
#
# Run all validation tests for paper arXiv:2402.10185
#
# Usage:
#   ./run_all_tests.sh [--quick|--thorough]
#
# Options:
#   --quick     : Fast tests with low resolution (default)
#   --thorough  : Full validation with high resolution
#

set -e  # Exit on error

# Parse arguments
MODE="quick"
if [ "$1" = "--thorough" ]; then
    MODE="thorough"
elif [ "$1" = "--quick" ]; then
    MODE="quick"
fi

# Set grid levels based on mode
if [ "$MODE" = "quick" ]; then
    LEVEL_LOW=6
    LEVEL_MED=7
    LEVEL_HIGH=8
    echo "Running QUICK tests (low resolution)"
else
    LEVEL_LOW=7
    LEVEL_MED=8
    LEVEL_HIGH=9
    echo "Running THOROUGH tests (high resolution)"
fi

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Change to test_cases directory
cd "$(dirname "$0")/../test_cases"

# Create results directory
mkdir -p ../results/{sliding_droplet,capillary_rise,film_flow,cylinder_wetting}

# Compile all tests
echo "========================================="
echo "COMPILING TESTS"
echo "========================================="

TESTS=("test_sliding_droplet" "test_capillary_rise" "test_film_flow" "test_cylinder_wetting")
COMPILE_ERRORS=0

for test in "${TESTS[@]}"; do
    echo -n "Compiling ${test}... "
    if qcc -O2 ${test}.c -o ${test} -lm 2>&1 | grep -q "error:"; then
        echo -e "${RED}FAILED${NC}"
        ((COMPILE_ERRORS++))
    else
        echo -e "${GREEN}OK${NC}"
    fi
done

if [ $COMPILE_ERRORS -gt 0 ]; then
    echo -e "${RED}Compilation failed for $COMPILE_ERRORS test(s)${NC}"
    exit 1
fi

echo ""

# Track results
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to run a test
run_test() {
    local test_name=$1
    local test_exec=$2
    local test_args=$3
    local description=$4

    echo "----------------------------------------"
    echo "Running: $description"
    echo "Command: ./$test_exec $test_args"
    echo "----------------------------------------"

    ((TOTAL_TESTS++))

    if ./$test_exec $test_args > /dev/null 2>&1; then
        echo -e "${GREEN}✅ PASSED${NC}: $description"
        ((PASSED_TESTS++))
        return 0
    else
        echo -e "${RED}❌ FAILED${NC}: $description"
        ((FAILED_TESTS++))
        return 1
    fi
}

echo "========================================="
echo "CASE 2: SLIDING DROPLET ON INCLINED PLANE"
echo "========================================="

# Horizontal plane tests
echo -e "\n${YELLOW}Horizontal Plane (0°)${NC}"
run_test "sliding" "test_sliding_droplet" "A30 P0 L${LEVEL_MED}" "θ=30°, horizontal"
run_test "sliding" "test_sliding_droplet" "A60 P0 L${LEVEL_MED}" "θ=60°, horizontal"
run_test "sliding" "test_sliding_droplet" "A90 P0 L${LEVEL_MED}" "θ=90°, horizontal"
run_test "sliding" "test_sliding_droplet" "A120 P0 L${LEVEL_MED}" "θ=120°, horizontal"

if [ "$MODE" = "thorough" ]; then
    # Inclined plane tests (45°)
    echo -e "\n${YELLOW}Inclined Plane (45°)${NC}"
    run_test "sliding" "test_sliding_droplet" "A60 P45 L${LEVEL_MED}" "θ=60°, 45° incline"
    run_test "sliding" "test_sliding_droplet" "A90 P45 L${LEVEL_MED}" "θ=90°, 45° incline"

    # Gravity effect tests
    echo -e "\n${YELLOW}With Gravity (Eötvös)${NC}"
    run_test "sliding" "test_sliding_droplet" "A60 P0 E1 L${LEVEL_MED}" "θ=60°, Eo=1"
    run_test "sliding" "test_sliding_droplet" "A60 P0 E10 L${LEVEL_MED}" "θ=60°, Eo=10"
fi

echo ""
echo "========================================="
echo "CASE 4: CAPILLARY RISE IN TUBE"
echo "========================================="

run_test "capillary" "test_capillary_rise" "A30 L${LEVEL_HIGH}" "θ=30° (hydrophilic)"
run_test "capillary" "test_capillary_rise" "A60 L${LEVEL_HIGH}" "θ=60°"

if [ "$MODE" = "thorough" ]; then
    run_test "capillary" "test_capillary_rise" "A45 L${LEVEL_HIGH}" "θ=45°"
    run_test "capillary" "test_capillary_rise" "A75 L${LEVEL_HIGH}" "θ=75°"
fi

echo ""
echo "========================================="
echo "CASE 5: FILM FLOW ON CURVED SURFACE"
echo "========================================="

run_test "film" "test_film_flow" "A60 G0 L${LEVEL_MED}" "θ=60°, cylinder"
run_test "film" "test_film_flow" "A90 G0 L${LEVEL_MED}" "θ=90°, cylinder"

if [ "$MODE" = "thorough" ]; then
    run_test "film" "test_film_flow" "A60 G1 L${LEVEL_MED}" "θ=60°, sphere"
    run_test "film" "test_film_flow" "A30 G0 L${LEVEL_MED}" "θ=30°, cylinder"
fi

echo ""
echo "========================================="
echo "CASE 6: DROPLET ON CYLINDER"
echo "========================================="

run_test "cylinder" "test_cylinder_wetting" "A30 L${LEVEL_LOW}" "θ=30°"
run_test "cylinder" "test_cylinder_wetting" "A60 L${LEVEL_LOW}" "θ=60°"
run_test "cylinder" "test_cylinder_wetting" "A90 L${LEVEL_LOW}" "θ=90°"
run_test "cylinder" "test_cylinder_wetting" "A120 L${LEVEL_LOW}" "θ=120°"

if [ "$MODE" = "thorough" ]; then
    run_test "cylinder" "test_cylinder_wetting" "A150 L${LEVEL_LOW}" "θ=150°"

    # Grid convergence
    echo -e "\n${YELLOW}Grid Convergence Study${NC}"
    run_test "cylinder" "test_cylinder_wetting" "A60 L6" "θ=60°, Level 6"
    run_test "cylinder" "test_cylinder_wetting" "A60 L7" "θ=60°, Level 7"
    run_test "cylinder" "test_cylinder_wetting" "A60 L8" "θ=60°, Level 8"
fi

# Summary
echo ""
echo "========================================="
echo "TEST SUMMARY"
echo "========================================="
echo "Total tests:  $TOTAL_TESTS"
echo -e "Passed:       ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed:       ${RED}$FAILED_TESTS${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo ""
    echo -e "${GREEN}╔═══════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║                                       ║${NC}"
    echo -e "${GREEN}║   ✅  ALL TESTS PASSED! ✅           ║${NC}"
    echo -e "${GREEN}║                                       ║${NC}"
    echo -e "${GREEN}╚═══════════════════════════════════════╝${NC}"
    echo ""
    exit 0
else
    echo ""
    echo -e "${RED}╔═══════════════════════════════════════╗${NC}"
    echo -e "${RED}║                                       ║${NC}"
    echo -e "${RED}║   ❌  SOME TESTS FAILED  ❌           ║${NC}"
    echo -e "${RED}║                                       ║${NC}"
    echo -e "${RED}╚═══════════════════════════════════════╝${NC}"
    echo ""
    echo "Check individual test outputs in ../results/ for details"
    exit 1
fi
