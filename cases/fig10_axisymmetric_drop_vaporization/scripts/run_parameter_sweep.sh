#!/bin/bash
#
# Parameter sweep script for Figure 10 reproduction
# Sweeps Reynolds number from 22 to 200 (25 cases) with fixed Weber number = 1.5
#

# Exit on error
set -e

# Configuration
WEBER=1.5
RE_MIN=22
RE_MAX=200
NUM_CASES=25

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

# Check if Basilisk is available
if [ -z "$BASILISK" ]; then
    echo "Error: BASILISK environment variable not set"
    echo "Please set BASILISK to your Basilisk installation directory"
    exit 1
fi

# Compile the code
echo "==================================================="
echo "Compiling $SRC_FILE"
echo "==================================================="
cd "$BUILD_DIR"

# Use qcc compiler
$BASILISK/qcc -Wall -O2 -o fig10_drop_vaporization \
    -I$BASILISK/src \
    -lm \
    "$SRC_FILE"

if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

echo "Compilation successful!"

# Calculate Reynolds numbers for the sweep
echo ""
echo "==================================================="
echo "Parameter Sweep Configuration"
echo "==================================================="
echo "Weber number (fixed): $WEBER"
echo "Reynolds number range: $RE_MIN to $RE_MAX"
echo "Number of cases: $NUM_CASES"
echo ""

# Generate Reynolds numbers (linear spacing)
RE_VALUES=()
for i in $(seq 0 $((NUM_CASES-1))); do
    RE=$(awk -v min=$RE_MIN -v max=$RE_MAX -v n=$NUM_CASES -v i=$i \
        'BEGIN {printf "%.2f", min + (max-min)*i/(n-1)}')
    RE_VALUES+=($RE)
done

# Print all Re values
echo "Reynolds numbers:"
printf '%s\n' "${RE_VALUES[@]}" | column -c 80
echo ""

# Run simulations
echo "==================================================="
echo "Running Simulations"
echo "==================================================="

for i in "${!RE_VALUES[@]}"; do
    RE=${RE_VALUES[$i]}
    CASE_NUM=$(printf "%03d" $((i+1)))
    OUTPUT_DIR="$RESULTS_DIR/Re_${RE}_We_${WEBER}"

    echo ""
    echo "---------------------------------------------------"
    echo "Case $CASE_NUM/$NUM_CASES: Re = $RE, We = $WEBER"
    echo "Output directory: $OUTPUT_DIR"
    echo "---------------------------------------------------"

    # Create output directory
    mkdir -p "$OUTPUT_DIR"

    # Run simulation
    cd "$BUILD_DIR"

    # Run with parameters: ./executable Re We output_dir
    ./fig10_drop_vaporization $RE $WEBER "$OUTPUT_DIR" 2>&1 | tee "$OUTPUT_DIR/log.txt"

    if [ $? -eq 0 ]; then
        echo "Case $CASE_NUM completed successfully!"
    else
        echo "Warning: Case $CASE_NUM failed or terminated early"
    fi
done

echo ""
echo "==================================================="
echo "Parameter Sweep Completed"
echo "==================================================="
echo "Results saved in: $RESULTS_DIR"
echo ""

# Create summary
SUMMARY_FILE="$RESULTS_DIR/sweep_summary.txt"
echo "Creating summary file: $SUMMARY_FILE"

cat > "$SUMMARY_FILE" << EOF
Parameter Sweep Summary
=======================

Date: $(date)
Weber Number: $WEBER
Reynolds Number Range: $RE_MIN to $RE_MAX
Number of Cases: $NUM_CASES

Cases:
------
EOF

for i in "${!RE_VALUES[@]}"; do
    RE=${RE_VALUES[$i]}
    CASE_NUM=$(printf "%03d" $((i+1)))
    OUTPUT_DIR="$RESULTS_DIR/Re_${RE}_We_${WEBER}"

    if [ -f "$OUTPUT_DIR/volume_history.txt" ]; then
        LINES=$(wc -l < "$OUTPUT_DIR/volume_history.txt")
        STATUS="Completed ($((LINES-1)) timesteps)"
    else
        STATUS="Failed or incomplete"
    fi

    echo "$CASE_NUM. Re = $RE, We = $WEBER - $STATUS" >> "$SUMMARY_FILE"
done

cat "$SUMMARY_FILE"

echo ""
echo "All done!"
