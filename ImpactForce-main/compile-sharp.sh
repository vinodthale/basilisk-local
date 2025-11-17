#!/bin/bash
# Compilation script for ImpactForce with Sharp VOF method
# Author: Vinod Thale
# Modified for Sharp VOF integration

set -e  # Exit on error

echo "=========================================="
echo "Compiling ImpactForce with Sharp VOF"
echo "=========================================="

# Check if qcc is available
if ! command -v qcc &> /dev/null; then
    echo "Error: qcc (Basilisk compiler) not found in PATH"
    echo "Please ensure Basilisk is installed and qcc is in your PATH"
    exit 1
fi

# Compilation settings
OPTIMIZATION="-O2"
WARNINGS="-Wall"
FORTIFY="-D_FORTIFY_SOURCE=0"
LIBS="-lm"

# Optional: Enable MPI support (uncomment if needed)
# MPI_FLAG="-D_MPI=1"

echo ""
echo "Compilation settings:"
echo "  Optimization: $OPTIMIZATION"
echo "  Warnings: $WARNINGS"
echo "  Libraries: $LIBS"
echo ""

# Compile the Sharp VOF version
echo "Compiling Bdropimpact-sharp.c..."
qcc $OPTIMIZATION $WARNINGS $FORTIFY $MPI_FLAG Bdropimpact-sharp.c -o Bdropimpact-sharp $LIBS

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Compilation successful!"
    echo "  Executable: ./Bdropimpact-sharp"
    echo ""
    echo "Usage:"
    echo "  ./Bdropimpact-sharp                    # Run with default parameters"
    echo "  ./Bdropimpact-sharp R1000 W100         # Specify Re and We"
    echo "  ./Bdropimpact-sharp R1000 W100 X12 N9  # With grid levels"
    echo ""
    echo "Key features of Sharp VOF version:"
    echo "  ✓ Contact line dynamics with hysteresis"
    echo "  ✓ Sharp interface resolution"
    echo "  ✓ Conservative VOF advection"
    echo "  ✓ Embed boundary support"
    echo "  ✓ Contact angle: ${CONTACT_ANGLE_SUBSTRATE:-90.0}° (configurable in constants-sharp.h)"
    echo ""
else
    echo ""
    echo "✗ Compilation failed!"
    exit 1
fi
