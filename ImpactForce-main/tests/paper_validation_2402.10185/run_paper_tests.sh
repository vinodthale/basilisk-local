#!/bin/bash
#
# Master Test Runner for Paper Validation (arXiv:2402.10185)
# Tavares et al. (2024) - Coupled VOF/Embedded Boundary Method
#
# Usage:
#   ./run_paper_tests.sh [options]
#
# Options:
#   --test <num>     Run specific test (1-6)
#   --quick          Quick validation (lower resolution)
#   --paper-quality  High resolution matching paper
#   --all            Run all tests (default)
#   --ci             CI mode (exit code, no colors)
#

set -e  # Exit on error

# Colors for output (disabled in CI mode)
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
RESULTS_DIR="./results"
TEST_CASES_DIR="./test_cases"
LOG_DIR="${RESULTS_DIR}/logs"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Default parameters
MODE="paper-quality"
RUN_ALL=true
SPECIFIC_TEST=""
CI_MODE=false

# Test tracking
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --test)
            SPECIFIC_TEST="$2"
            RUN_ALL=false
            shift 2
            ;;
        --quick)
            MODE="quick"
            shift
            ;;
        --paper-quality)
            MODE="paper-quality"
            shift
            ;;
        --all)
            RUN_ALL=true
            shift
            ;;
        --ci)
            CI_MODE=true
            RED=''
            GREEN=''
            YELLOW=''
            BLUE=''
            NC=''
            shift
            ;;
        --help)
            echo "Paper Validation Test Suite Runner"
            echo "Reference: Tavares et al. (2024) arXiv:2402.10185"
            echo ""
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --test <num>     Run specific test (1-6)"
            echo "  --quick          Quick validation (levels 9-10, ~15 min)"
            echo "  --paper-quality  High resolution (levels 9-12, ~2 hours)"
            echo "  --all            Run all available tests (default)"
            echo "  --ci             CI mode (exit codes, no colors)"
            echo "  --help           Show this help"
            echo ""
            echo "Tests:"
            echo "  1: Static Contact Angle"
            echo "  2: Sliding Droplet (TODO)"
            echo "  3: Droplet Impact"
            echo "  4: Capillary Rise (TODO)"
            echo "  5: Thin Film Flow (TODO)"
            echo "  6: Cylinder Wetting (TODO)"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Setup directories
mkdir -p "${RESULTS_DIR}"
mkdir -p "${LOG_DIR}"

# Print header
echo "========================================"
echo "Paper Validation Test Suite"
echo "========================================"
echo "Reference: Tavares et al. (2024)"
echo "           arXiv:2402.10185"
echo "           DOI: 10.48550/arXiv.2402.10185"
echo "========================================"
echo "Mode: ${MODE}"
echo "Date: $(date)"
echo "========================================"
echo ""

# Set parameters based on mode
if [ "$MODE" = "quick" ]; then
    LEVEL_MIN=9
    LEVEL_MAX=10
    MAX_TIME_SCALE=0.5  # Shorter simulation times
    echo "Using QUICK mode:"
    echo "  Grid levels: ${LEVEL_MIN}-${LEVEL_MAX}"
    echo "  Resolution: ~200k cells peak"
    echo "  Time scale: 0.5x (faster)"
    echo "  Estimated time: ~15 minutes"
else
    LEVEL_MIN=9
    LEVEL_MAX=12
    MAX_TIME_SCALE=1.0  # Full simulation times
    echo "Using PAPER-QUALITY mode:"
    echo "  Grid levels: ${LEVEL_MIN}-${LEVEL_MAX}"
    echo "  Resolution: ~800k cells peak"
    echo "  Time scale: 1.0x (full duration)"
    echo "  Estimated time: ~2 hours"
fi
echo "========================================"
echo ""

# Check for qcc compiler
if ! command -v qcc &> /dev/null; then
    echo -e "${RED}Error: qcc (Basilisk compiler) not found${NC}"
    echo "Please ensure Basilisk is installed and qcc is in PATH"
    echo ""
    echo "Build Basilisk:"
    echo "  cd /home/user/basilisk-local/src"
    echo "  make"
    echo "  export PATH=\$PATH:\$(pwd)"
    exit 1
fi

# Function to compile test
compile_test() {
    local test_file=$1
    local test_name=$(basename "${test_file}" .c)
    local log_file="${LOG_DIR}/${test_name}_compile_${TIMESTAMP}.log"

    echo -n "Compiling ${test_name}... "

    if qcc -O2 -Wall -D_FORTIFY_SOURCE=0 \
           -DMAX_GRID_LEVEL=${LEVEL_MAX} \
           -DINITIAL_GRID_LEVEL=${LEVEL_MIN} \
           "${test_file}" -o "${TEST_CASES_DIR}/${test_name}" -lm \
           > "${log_file}" 2>&1; then
        echo -e "${GREEN}✓${NC}"
        return 0
    else
        echo -e "${RED}✗${NC}"
        echo "  Compilation failed. See ${log_file}"
        return 1
    fi
}

# Function to run test
run_test() {
    local test_num=$1
    local test_name=$2
    local test_executable="${TEST_CASES_DIR}/${test_name}"
    local log_file="${LOG_DIR}/${test_name}_run_${TIMESTAMP}.log"
    local json_summary="${RESULTS_DIR}/${test_name}_summary.json"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    echo ""
    echo "========================================"
    echo "Test ${test_num}: ${test_name}"
    echo "========================================"

    # Check if executable exists
    if [ ! -f "${test_executable}" ]; then
        echo -e "${YELLOW}⊘ SKIPPED${NC} (not compiled)"
        SKIPPED_TESTS=$((SKIPPED_TESTS + 1))
        return 0
    fi

    # Run the test
    echo "Running..."
    local start_time=$(date +%s)

    if "${test_executable}" 2>&1 | tee "${log_file}"; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))

        # Check for JSON summary
        if [ -f "${json_summary}" ]; then
            local status=$(grep -oP '"status":\s*"\K[^"]+' "${json_summary}" || echo "UNKNOWN")

            if [ "${status}" = "PASS" ]; then
                echo -e "  ${GREEN}✓ PASSED${NC} (${duration}s)"
                PASSED_TESTS=$((PASSED_TESTS + 1))
            else
                echo -e "  ${RED}✗ FAILED${NC} (${duration}s)"
                FAILED_TESTS=$((FAILED_TESTS + 1))
            fi
        else
            echo -e "  ${YELLOW}? UNKNOWN${NC} (no summary) (${duration}s)"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    else
        echo -e "  ${RED}✗ CRASHED${NC}"
        echo "  See log: ${log_file}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
}

# Compile tests
echo "========================================"
echo "Compiling Test Cases"
echo "========================================"

# Test 1: Static Contact Angle
if [ -f "${TEST_CASES_DIR}/test_static_contact_angle.c" ]; then
    compile_test "${TEST_CASES_DIR}/test_static_contact_angle.c"
fi

# Test 3: Droplet Impact
if [ -f "${TEST_CASES_DIR}/test_droplet_impact.c" ]; then
    compile_test "${TEST_CASES_DIR}/test_droplet_impact.c"
fi

# TODO: Add other tests as they are implemented

echo ""
echo "========================================"
echo "Running Tests"
echo "========================================"

# Run tests
if [ "$RUN_ALL" = true ]; then
    # Test 1: Static Contact Angle
    if [ -f "${TEST_CASES_DIR}/test_static_contact_angle" ]; then
        run_test 1 "test_static_contact_angle"
    fi

    # Test 3: Droplet Impact
    if [ -f "${TEST_CASES_DIR}/test_droplet_impact" ]; then
        run_test 3 "test_droplet_impact"
    fi

else
    # Run specific test
    case $SPECIFIC_TEST in
        1)
            run_test 1 "test_static_contact_angle"
            ;;
        3)
            run_test 3 "test_droplet_impact"
            ;;
        *)
            echo -e "${RED}Error: Test ${SPECIFIC_TEST} not available or not implemented${NC}"
            echo "Available tests: 1, 3"
            exit 1
            ;;
    esac
fi

# Summary
echo ""
echo "========================================"
echo "Test Suite Summary"
echo "========================================"
echo "Total tests: ${TOTAL_TESTS}"
echo -e "Passed: ${GREEN}${PASSED_TESTS}${NC}"
echo -e "Failed: ${RED}${FAILED_TESTS}${NC}"
echo -e "Skipped: ${YELLOW}${SKIPPED_TESTS}${NC}"
echo "========================================"

# Write JSON summary
SUMMARY_JSON="${RESULTS_DIR}/paper_validation_summary_${TIMESTAMP}.json"
cat > "${SUMMARY_JSON}" <<EOF
{
  "paper_reference": "Tavares et al. (2024) arXiv:2402.10185",
  "timestamp": "$(date -Iseconds)",
  "mode": "${MODE}",
  "grid_levels": "${LEVEL_MIN}-${LEVEL_MAX}",
  "summary": {
    "total": ${TOTAL_TESTS},
    "passed": ${PASSED_TESTS},
    "failed": ${FAILED_TESTS},
    "skipped": ${SKIPPED_TESTS}
  },
  "results_directory": "${RESULTS_DIR}",
  "log_directory": "${LOG_DIR}"
}
EOF

echo ""
echo "Summary saved to: ${SUMMARY_JSON}"
echo ""

# Exit status
if [ ${FAILED_TESTS} -eq 0 ]; then
    echo -e "${GREEN}✓ ALL TESTS PASSED${NC}"
    echo "========================================"
    exit 0
elif [ ${PASSED_TESTS} -gt 0 ]; then
    echo -e "${YELLOW}⚠ SOME TESTS FAILED${NC}"
    echo "========================================"
    exit 1
else
    echo -e "${RED}✗ ALL TESTS FAILED${NC}"
    echo "========================================"
    exit 2
fi
