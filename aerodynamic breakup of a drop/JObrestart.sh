#!/bin/bash
set -euo pipefail

# Set number of MPI processes
NUM_PROCS=20

# Restart file is always 'dump' if it exists
RESTART_FILE="dump"
LOGFILE="simulation_$(date +%Y%m%d_%H%M%S).log"
exec > >(tee -a "$LOGFILE") 2>&1

echo "===== Simulation started at $(date) ====="
echo "Using $NUM_PROCS MPI processes"
echo "Working directory: $(pwd)"
echo "Log file: $LOGFILE"
echo

if [[ -f "$RESTART_FILE" ]]; then
    echo "[INFO] Restart file '$RESTART_FILE' found. Running in RESTART mode."
    RUN_MODE="RESTART"
    CMD="./Bdropimpact"
else
    echo "[INFO] Restart file '$RESTART_FILE' NOT found. Running in FRESH mode."
    RUN_MODE="FRESH"
    CMD="./Bdropimpact"
fi
echo

# Check if executable exists
if [[ ! -x ./Bdropimpact ]]; then
    echo "[ERROR] Executable './Bdropimpact' not found or not executable."
    exit 1
fi

# Run the simulation
echo "[INFO] Running simulation with $NUM_PROCS MPI processes..."
START_TIME=$(date +%s)

if ! mpirun -np $NUM_PROCS $CMD; then
    echo "[ERROR] Simulation failed during execution."
    exit 1
fi

END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

echo
echo "===== Simulation finished at $(date) ====="
echo "Run mode: $RUN_MODE"
echo "Total runtime: ${DURATION} seconds"
