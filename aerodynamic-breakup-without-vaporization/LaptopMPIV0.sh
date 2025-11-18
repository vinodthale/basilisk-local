#!/bin/bash

# Set number of MPI processes (change here as needed)
NUM_PROCS=60

# Define log file
LOGFILE="simulation_$(date +%Y%m%d_%H%M%S).log"
exec > >(tee -a "$LOGFILE") 2>&1

echo "===== Simulation started at $(date) ====="
echo "Using $NUM_PROCS MPI processes"
echo "Working directory: $(pwd)"
echo "Log file: $LOGFILE"
echo

# Step 1: Generate MPI-enabled source with disabled dimensions
echo "[INFO] Generating source code with qcc..."
if ! qcc -source -D_MPI=1 -disable-dimensions Bdropimpact.c; then
    echo "[ERROR] qcc failed to generate source code."
    exit 1
fi
echo "[OK] qcc generated source _Bdropimpact.c"
echo

# Step 2: Compile
echo "[INFO] Compiling with mpicc..."
if ! mpicc -O2 -Wall -std=c99 -D_MPI=1 -D_GNU_SOURCE=1 -D_FORTIFY_SOURCE=0 _Bdropimpact.c -o Bdropimpact -lm; then
    echo "[ERROR] Compilation failed."
    exit 1
fi
echo "[OK] Compilation succeeded."
echo

# Step 3: Run the simulation
echo "[INFO] Running simulation with $NUM_PROCS MPI processes..."
START_TIME=$(date +%s)

if ! mpirun -np $NUM_PROCS ./Bdropimpact; then
    echo "[ERROR] Simulation failed during execution."
    exit 1
fi

END_TIME=$(date +%s)
echo
echo "===== Simulation finished at $(date) ====="
echo "Total time: $((END_TIME - START_TIME)) seconds"








