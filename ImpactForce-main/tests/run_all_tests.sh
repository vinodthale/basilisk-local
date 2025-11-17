#!/bin/bash
# Comprehensive Test Runner for Sharp VOF Integration
# Runs all validation and benchmark tests

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
MODE="thorough"  # quick, thorough
CATEGORY="all"    # all, validation, benchmarks
CI_MODE=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
	case $1 in
		--quick)
			MODE="quick"
			shift
			;;
		--thorough)
			MODE="thorough"
			shift
			;;
		--validation)
			CATEGORY="validation"
			shift
			;;
		--benchmarks)
			CATEGORY="benchmarks"
			shift
			;;
		--ci)
			CI_MODE=true
			shift
			;;
		--help)
			echo "Usage: $0 [OPTIONS]"
			echo ""
			echo "Options:"
			echo "  --quick       Run fast tests only (levels 9-10)"
			echo "  --thorough    Run full test suite (levels 9-12)"
			echo "  --validation  Run validation tests only"
			echo "  --benchmarks  Run benchmark tests only"
			echo "  --ci          CI mode (minimal output)"
			echo "  --help        Show this help message"
			exit 0
			;;
		*)
			echo "Unknown option: $1"
			echo "Use --help for usage information"
			exit 1
			;;
	esac
done

# Print header
if [ "$CI_MODE" = false ]; then
	echo -e "${BLUE}========================================${NC}"
	echo -e "${BLUE}Sharp VOF Test Suite Runner${NC}"
	echo -e "${BLUE}========================================${NC}"
	echo "Mode: $MODE"
	echo "Category: $CATEGORY"
	echo "Date: $(date)"
	echo -e "${BLUE}========================================${NC}"
	echo ""
fi

# Check prerequisites
if ! command -v qcc &> /dev/null; then
	echo -e "${RED}Error: qcc not found!${NC}"
	echo "Please ensure Basilisk is built and qcc is in PATH"
	exit 1
fi

# Create results directory
mkdir -p results

# Test counter
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Array to store failed test names
declare -a FAILED_TEST_NAMES

# Function to run a single test
run_test() {
	local test_file=$1
	local test_name=$(basename "$test_file" .c)
	local grid_levels=$2

	TOTAL_TESTS=$((TOTAL_TESTS + 1))

	if [ "$CI_MODE" = false ]; then
		echo -e "${YELLOW}Running: $test_name (levels $grid_levels)${NC}"
	fi

	# Compile
	local compile_cmd="qcc -O2 -Wall -D_FORTIFY_SOURCE=0 $test_file -o $test_name -lm"

	if [ "$MODE" = "quick" ]; then
		compile_cmd="$compile_cmd -DMAX_GRID_LEVEL=10 -DINITAL_GRID_LEVEL=9"
	fi

	if ! $compile_cmd 2>&1 | tee results/${test_name}_compile.log > /dev/null; then
		echo -e "${RED}  ✗ Compilation FAILED${NC}"
		FAILED_TESTS=$((FAILED_TESTS + 1))
		FAILED_TEST_NAMES+=("$test_name (compile)")
		return 1
	fi

	# Run test
	local log_file="results/${test_name}_$(date +%Y%m%d_%H%M%S).log"

	if ./$test_name 2>&1 | tee "$log_file"; then
		echo -e "${GREEN}  ✓ PASSED${NC}"
		PASSED_TESTS=$((PASSED_TESTS + 1))
		return 0
	else
		local exit_code=$?
		echo -e "${RED}  ✗ FAILED (exit code: $exit_code)${NC}"
		FAILED_TESTS=$((FAILED_TESTS + 1))
		FAILED_TEST_NAMES+=("$test_name")
		return 1
	fi
}

# Function to run test category
run_category() {
	local category=$1
	local grid_levels="9-12"

	if [ "$MODE" = "quick" ]; then
		grid_levels="9-10"
	fi

	if [ "$CI_MODE" = false ]; then
		echo ""
		echo -e "${BLUE}========================================${NC}"
		echo -e "${BLUE}Category: $category${NC}"
		echo -e "${BLUE}========================================${NC}"
		echo ""
	fi

	cd "$category"

	# Find all test files
	local test_files=$(find . -name "test_*.c" -type f | sort)

	if [ -z "$test_files" ]; then
		echo "No tests found in $category"
		cd ..
		return
	fi

	# Run each test
	for test_file in $test_files; do
		run_test "$test_file" "$grid_levels" || true
	done

	# Cleanup executables
	rm -f test_*
	rm -f _test_*.c

	cd ..
}

# Start timer
START_TIME=$(date +%s)

# Run validation tests
if [ "$CATEGORY" = "all" ] || [ "$CATEGORY" = "validation" ]; then
	if [ -d "validation" ]; then
		run_category "validation"
	fi
fi

# Run benchmark tests
if [ "$CATEGORY" = "all" ] || [ "$CATEGORY" = "benchmarks" ]; then
	if [ -d "benchmarks" ]; then
		run_category "benchmarks"
	fi
fi

# End timer
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

# Print summary
echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Test Suite Summary${NC}"
echo -e "${BLUE}========================================${NC}"
echo "Total tests: $TOTAL_TESTS"
echo -e "${GREEN}Passed: $PASSED_TESTS${NC}"
if [ $FAILED_TESTS -gt 0 ]; then
	echo -e "${RED}Failed: $FAILED_TESTS${NC}"
else
	echo -e "${GREEN}Failed: 0${NC}"
fi
if [ $SKIPPED_TESTS -gt 0 ]; then
	echo -e "${YELLOW}Skipped: $SKIPPED_TESTS${NC}"
fi
echo "Duration: ${DURATION}s"
echo -e "${BLUE}========================================${NC}"

# List failed tests
if [ $FAILED_TESTS -gt 0 ]; then
	echo ""
	echo -e "${RED}Failed tests:${NC}"
	for test in "${FAILED_TEST_NAMES[@]}"; do
		echo -e "${RED}  - $test${NC}"
	done
fi

# Generate JSON report
REPORT_FILE="results/test_suite_summary.json"
cat > "$REPORT_FILE" << EOF
{
  "timestamp": "$(date -Iseconds)",
  "mode": "$MODE",
  "category": "$CATEGORY",
  "total_tests": $TOTAL_TESTS,
  "passed": $PASSED_TESTS,
  "failed": $FAILED_TESTS,
  "skipped": $SKIPPED_TESTS,
  "duration_seconds": $DURATION,
  "failed_tests": [
EOF

# Add failed test names to JSON
FIRST=true
for test in "${FAILED_TEST_NAMES[@]}"; do
	if [ "$FIRST" = true ]; then
		FIRST=false
	else
		echo "," >> "$REPORT_FILE"
	fi
	echo -n "    \"$test\"" >> "$REPORT_FILE"
done

cat >> "$REPORT_FILE" << EOF

  ]
}
EOF

echo ""
echo "Report saved to: $REPORT_FILE"

# Final status
echo ""
if [ $FAILED_TESTS -eq 0 ]; then
	echo -e "${GREEN}========================================${NC}"
	echo -e "${GREEN}✓ ALL TESTS PASSED${NC}"
	echo -e "${GREEN}========================================${NC}"
	exit 0
else
	echo -e "${RED}========================================${NC}"
	echo -e "${RED}✗ SOME TESTS FAILED${NC}"
	echo -e "${RED}========================================${NC}"

	# Exit with different codes for different failure types
	if [ $FAILED_TESTS -eq $TOTAL_TESTS ]; then
		exit 2  # All tests failed (critical)
	else
		exit 1  # Some tests failed
	fi
fi
