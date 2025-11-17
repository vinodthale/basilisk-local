# ImpactForce Module

Direct Numerical Simulation (DNS) code for simulating the impact forces of air-in-liquid compound drops onto substrates using the Basilisk flow solver.

## Overview

This module provides tools to simulate and analyze the impact dynamics of compound drops (liquid drops containing air bubbles) impacting solid surfaces. The simulations solve the Navier-Stokes equations with a two-phase flow approach, capturing interface dynamics, pressure fields, and impact forces.

## Author

Vinod Thale (September 15, 2023)

## Features

- **2D axisymmetric simulations** of compound drop impacts
- **Two-phase flow modeling** with surface tension
- **Adaptive mesh refinement** for capturing interface dynamics
- **MPI parallelization** for high-performance computing
- **Post-processing tools** for force and pressure analysis
- **Configurable parameters** for Reynolds, Weber, and Froude numbers
- **Time-resolved output** with snapshot generation

## Dependencies

This code requires the [Basilisk flow solver](http://basilisk.dalembert.upmc.fr/), an open-source CFD framework for solving partial differential equations.

### Installation

1. Install Basilisk following the instructions at: http://basilisk.dalembert.upmc.fr/
2. Ensure `qcc` (Basilisk's compiler) is in your PATH
3. For MPI simulations, ensure MPI libraries are installed

## File Structure

```
ImpactForce-main/
├── Bdropimpact.c        # Main simulation code for drop impact
├── constants.h          # Configuration parameters and utility functions
├── Jetandpressure.c     # Post-processing code for force calculations
├── ClusterMPI.sh        # SLURM batch script for MPI execution
├── RJetandpressure.sh   # Shell script to run post-processing
└── README.md            # This file
```

## Core Components

### 1. Bdropimpact.c

Main simulation code that:
- Initializes compound drop (outer liquid + inner bubble) geometry
- Solves Navier-Stokes equations with two-phase flow
- Implements adaptive mesh refinement based on interface and velocity gradients
- Outputs time-resolved snapshots and performance metrics
- Supports dimensional, non-dimensional, and experimental parameter modes

### 2. constants.h

Configuration header containing:
- Physical properties (density, viscosity, surface tension)
- Dimensionless numbers (Reynolds, Weber, Froude)
- Computational parameters (grid levels, domain size, refinement criteria)
- Utility functions for parameter initialization and time formatting

### 3. Jetandpressure.c

Post-processing tool that:
- Loads snapshot files from simulations
- Calculates pressure forces on the substrate
- Integrates forces over the impact surface
- Outputs time-resolved force data

## Usage

### Running Simulations

#### Single-core execution:
```bash
qcc -O2 -Wall -D_FORTIFY_SOURCE=0 Bdropimpact.c -o Bdropimpact -lm
./Bdropimpact
```

#### MPI parallel execution:
Use the provided SLURM batch script for cluster environments:

```bash
sbatch ClusterMPI.sh
```

Or compile and run manually:
```bash
qcc -source -D_MPI=1 Bdropimpact.c
mpicc -O2 -Wall -std=c99 -D_MPI=1 -D_FORTIFY_SOURCE=0 _Bdropimpact.c -o Bdropimpact -lm
mpirun -np <num_processes> ./Bdropimpact
```

### Post-processing

After simulation completes, analyze forces using:

```bash
./RJetandpressure.sh
```

Or manually:
```bash
qcc -O2 -Wall -D_FORTIFY_SOURCE=0 Jetandpressure.c -o Jetandpressure -lm
./Jetandpressure
```

## Configuration Parameters

Key parameters can be adjusted in `constants.h`:

### Physical Parameters (Dimensional Mode)
- `VELOCITY`: Impact velocity (m/s)
- `DROP_DIAMETER`: Drop diameter (m)
- `RHO_L`: Liquid density (kg/m³)
- `RHO_G`: Gas density (kg/m³)
- `MU_L`: Liquid dynamic viscosity (Pa·s)
- `MU_G`: Gas dynamic viscosity (Pa·s)
- `SIGMA`: Surface tension (N/m)

### Dimensionless Parameters (Non-dimensional Mode)
- `WEBER`: Weber number (We = ρV²D/σ)
- `REYNOLDS`: Reynolds number (Re = ρVD/μ)
- `FROUDE`: Froude number (Fr = V/√(gD))

### Computational Parameters
- `INITAL_GRID_LEVEL`: Initial grid refinement level (default: 9)
- `MAX_GRID_LEVEL`: Maximum grid refinement level (default: 12)
- `DOMAIN_WIDTH`: Computational domain size (in drop diameters)
- `BUBBLE_DIAMETER`: Inner bubble size (relative to drop diameter)
- `MAX_TIME`: Simulation end time
- `SAVE_FILE_EVERY`: Snapshot output interval

### Mode Selection

Set `DIM_NONDIM_EXP` to choose between:
- `'d'` or `'D'`: Dimensional mode (SI units)
- `'n'` or `'N'`: Non-dimensional mode
- `'e'` or `'E'`: Experimental mode

## Command-line Arguments

The simulation accepts runtime parameters:

```bash
./Bdropimpact R<Reynolds> W<Weber> F<Froude> H<pool_depth> X<max_level> N<min_level> TE<time_end> TS<time_step>
```

Example:
```bash
./Bdropimpact R1000 W100 H0.5 X12 N9
```

## Output Files

The simulation generates:

- `intermediate/snapshot-*.???` - Simulation snapshots at regular intervals
- `lastfile` - Latest simulation state (for restart)
- `duration-CPU*.plt` - Performance metrics per CPU
- `parameters.txt` - Simulation parameters
- `endofrun-CPU*.txt` - Final timing statistics
- `ForceFVDb*/ForceFVDb*.txt` - Force analysis results (post-processing)

## Boundary Conditions

- **Left boundary (substrate)**: No-slip condition, non-wetting surface
- **Right boundary**: Free flow (Neumann) with zero pressure
- **Top boundary**: Outflow condition with zero pressure
- **Bottom boundary**: Symmetry (axisymmetric)

## Adaptive Mesh Refinement

The code uses wavelet-based adaptive refinement tracking:
- Volume fraction field (`f`)
- Velocity components (`u.x`, `u.y`)
- Refinement thresholds: 10^-6, 10^-3, 10^-3 (configurable)

## Performance

- Grid levels 9-12: ~10⁶-10⁷ cells with adaptive refinement
- MPI scaling: Tested up to 32 cores
- Typical simulation time: Hours to days depending on parameters

## Citation

If you use this code, please cite:

```
Author: Vinod Thale
Date: September 15, 2023
Framework developed by: Hossain Chizari
Additional functionalities: Vinod Ashok Thale and Professor Marie-Jean Thoraval
```

## References

- Basilisk solver documentation: http://basilisk.dalembert.upmc.fr/
- Two-phase flow modeling in Basilisk
- Adaptive mesh refinement techniques

## Troubleshooting

### Common Issues

1. **Compilation errors**: Ensure Basilisk is properly installed and `qcc` is in PATH
2. **MPI errors**: Check MPI library compatibility and number of processes
3. **Memory issues**: Reduce `MAX_GRID_LEVEL` or domain size
4. **Convergence problems**: Adjust `TOLERANCE` or refinement criteria

## Support

For questions or issues related to this code, please refer to:
- Basilisk documentation and forums
- Original author: Vinod Thale

## License

Please consult with the original authors regarding licensing and usage terms.
