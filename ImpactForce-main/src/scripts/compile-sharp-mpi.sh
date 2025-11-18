#!/bin/bash
# MPI Compilation script for ImpactForce with Sharp VOF method
# Author: Vinod Thale
# Modified for Sharp VOF integration

set -e  # Exit on error

echo "=========================================="
echo "Compiling ImpactForce Sharp VOF with MPI"
echo "=========================================="

# Check if qcc is available
if ! command -v qcc &> /dev/null; then
    echo "Error: qcc (Basilisk compiler) not found in PATH"
    echo "Please ensure Basilisk is installed and qcc is in your PATH"
    exit 1
fi

# Check if mpicc is available
if ! command -v mpicc &> /dev/null; then
    echo "Error: mpicc (MPI compiler) not found in PATH"
    echo "Please install MPI libraries (e.g., sudo apt install libopenmpi-dev)"
    exit 1
fi

# Compilation settings
OPTIMIZATION="-O2"
WARNINGS="-Wall"
STD="-std=c99"
FORTIFY="-D_FORTIFY_SOURCE=0"
MPI_FLAG="-D_MPI=1"
LIBS="-lm"

echo ""
echo "Compilation settings:"
echo "  Optimization: $OPTIMIZATION"
echo "  Warnings: $WARNINGS"
echo "  Standard: $STD"
echo "  MPI: Enabled"
echo "  Libraries: $LIBS"
echo ""

# Step 1: Generate MPI source code
echo "Step 1/2: Generating MPI source code with qcc..."
qcc -source $MPI_FLAG Bdropimpact-sharp.c

if [ $? -ne 0 ]; then
    echo ""
    echo "✗ Source generation failed!"
    exit 1
fi

echo "  ✓ Generated _Bdropimpact-sharp.c"

# Step 2: Compile with mpicc
echo ""
echo "Step 2/2: Compiling with mpicc..."
mpicc $OPTIMIZATION $WARNINGS $STD $MPI_FLAG $FORTIFY _Bdropimpact-sharp.c -o Bdropimpact-sharp-mpi $LIBS

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ MPI compilation successful!"
    echo "  Executable: ./Bdropimpact-sharp-mpi"
    echo ""
    echo "Usage:"
    echo "  mpirun -np 4 ./Bdropimpact-sharp-mpi              # 4 processes, default parameters"
    echo "  mpirun -np 8 ./Bdropimpact-sharp-mpi R1000 W100   # 8 processes with Re, We"
    echo "  mpirun -np 16 ./Bdropimpact-sharp-mpi R1000 W100 X12 N9  # With grid levels"
    echo ""
    echo "Performance tips:"
    echo "  • Use power-of-2 process counts (4, 8, 16, 32) for best load balancing"
    echo "  • MAX_GRID_LEVEL = 12: Use 4-16 processes"
    echo "  • MAX_GRID_LEVEL = 13: Use 8-32 processes"
    echo "  • Monitor load balance in duration-CPU*.plt files"
    echo ""
    echo "Key features of Sharp VOF version:"
    echo "  ✓ Contact line dynamics with hysteresis"
    echo "  ✓ Sharp interface resolution"
    echo "  ✓ Conservative VOF advection"
    echo "  ✓ MPI parallel execution"
    echo ""
else
    echo ""
    echo "✗ MPI compilation failed!"
    exit 1
fi
