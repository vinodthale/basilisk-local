# Aerodynamic Breakup of a Drop

A high-fidelity computational fluid dynamics (CFD) simulation of water droplet deformation and breakup in a high-speed air stream using the Basilisk C solver.

## Overview

This simulation models the aerodynamic breakup of a liquid droplet subjected to a high-velocity gas flow. The code solves the incompressible Navier-Stokes equations for two-phase flows using a Volume-of-Fluid (VOF) method with adaptive mesh refinement (AMR) in an axisymmetric geometry.

## Physical Problem

When a liquid droplet is exposed to a high-speed gas stream, aerodynamic forces deform and eventually fragment the droplet. This phenomenon is critical in applications such as:
- Fuel atomization in combustion engines
- Spray drying and coating processes
- Raindrops in strong winds
- Inkjet printing technology

The simulation captures:
- **Droplet deformation** from spherical to oblate/prolate shapes
- **Bag breakup** and **shear breakup** mechanisms
- **Secondary droplet formation**
- **Drag coefficient evolution**
- **Energy budget** (kinetic, surface, and dissipated energy)
- **Vorticity generation** in the gas phase

## Key Features

- **Axisymmetric geometry**: Reduces 3D problem to 2D meridional plane for computational efficiency
- **Two-phase VOF method**: Sharp interface tracking between liquid and gas phases
- **Adaptive mesh refinement**: Automatic grid refinement near the interface (up to level 13)
- **Surface tension**: Capillary forces with curvature-based formulation
- **Conservative momentum transport**: Accurate handling of large density ratios
- **Comprehensive diagnostics**:
  - Drag forces (pressure and viscous components)
  - Energy budgets (kinetic, surface, dissipation)
  - Vorticity and circulation in gas phase
  - Drop kinematics (center of mass, velocity, acceleration)

## Dependencies

- **Basilisk C**: Open-source CFD solver (http://basilisk.fr)
  - Download and install from: http://basilisk.fr/src/INSTALL
  - Set environment variable: `export BASILISK=/path/to/basilisk`
- **MPI** (optional): For parallel execution
  - OpenMPI or MPICH recommended
- **GCC**: C99-compliant compiler

## File Structure

```
aerodynamic breakup of a drop/
├── Bdropimpact.c          # Main simulation code (newer version with comprehensive diagnostics)
├── constants.h            # Alternative configuration header (older version)
├── BpostBview.c           # Post-processing visualization tool
├── LaptopMPIV0.sh        # MPI compilation and execution script
├── JObrestart.sh         # Restart simulation from checkpoint
├── Bview.sh              # Generate visualization from output
├── README.md             # This file
└── intermediate/         # Directory for snapshots (created during run)
```

## Simulation Parameters

### Physical Parameters (Dimensional Setup)

Default configuration in `Bdropimpact.c`:
```c
VELOCITY       = 19.588 m/s    // Freestream gas velocity
DROP_DIAMETER  = 0.0019 m      // Initial droplet diameter
RHO_L          = 1000.0 kg/m³  // Liquid density (water)
RHO_G          = 1.2 kg/m³     // Gas density (air)
MU_L           = 0.001 Pa·s    // Liquid viscosity
MU_G           = 0.000018 Pa·s // Gas viscosity
SIGMA          = 0.0729 N/m    // Surface tension
```

**Dimensionless numbers**:
- Reynolds number: Re ≈ 2,458
- Weber number: We ≈ 940
- Ohnesorge number: Oh ≈ 0.002

### Computational Parameters

```c
maxlevel = 13              // Maximum AMR level (~512 cells per diameter)
minlevel = 10              // Initial grid level
DOMAIN_WIDTH = 16.0        // Domain size (in drop diameters)
ASPECT_RATIO = 2.00        // Initial ellipsoidal deformation (Dh/Dv)
tmax = 250.0               // Maximum simulation time
tsnap = 1.00               // Snapshot interval
```

## Compilation and Execution

### Serial Execution

```bash
# Compile
qcc -O2 -Wall -disable-dimensions Bdropimpact.c -o Bdropimpact -lm

# Run
./Bdropimpact
```

### Parallel Execution (MPI)

```bash
# Use the provided script (60 processes)
bash LaptopMPIV0.sh

# Or manually:
qcc -source -D_MPI=1 -disable-dimensions Bdropimpact.c
mpicc -O2 -Wall -std=c99 -D_MPI=1 -D_GNU_SOURCE=1 _Bdropimpact.c -o Bdropimpact -lm
mpirun -np 60 ./Bdropimpact
```

### Restart from Checkpoint

```bash
bash JObrestart.sh
```

## Output Files

The simulation generates several diagnostic files:

| File | Content |
|------|---------|
| `energy.txt` | Energy budget: kinetic energy (KE1, KE2), surface energy (SE), dissipation (DE, DEE), interface area |
| `PressureViscousDrag.txt` | Total drag, pressure force, viscous force, projected area, drag coefficients (CD), drop kinematics (dR/dt, d²R/dt²) |
| `CD.txt` | Drag coefficient from acceleration, center-of-mass velocity, drop volume |
| `GasVorticity.txt` | Vorticity statistics (max, min, mean, RMS), circulation, positive/negative vorticity integrals |
| `parameters.txt` | Complete simulation parameters and dimensionless numbers |
| `dump` | Full field checkpoint for restart |
| `intermediate/snapshot-*` | Field snapshots at regular intervals |

### Output File Formats

All output files are space-delimited ASCII with headers. Example for `PressureViscousDrag.txt`:
```
Step  Time  Total_Drag  Pressure_Force  Viscous_Force  Interface_Area  A_proj  CD_total  CD_pressure  CD_viscous  dR_dt  d2R_dt2  yMax  x_cm
```

## Visualization

### Generate Visualizations

```bash
# Compile visualization tool
bash Bview.sh

# This generates PNG images from simulation snapshots
# Edit Bview.sh to change time range (tb, ts, te parameters)
```

### Using Basilisk View

```bash
# Interactive visualization of dump files
bview2D dump

# For specific fields
bview2D -f dump
```

### Example Output

The simulation produces visualizations showing:
- VOF field (droplet shape evolution)
- Velocity vectors
- Pressure distribution
- Vorticity contours

Sample images are included: `out-bview-VOF-VOF-000000000.png`, `out-bview-VOF-VOF-001000000.png`

## Modifying Parameters

### Change Weber/Reynolds Numbers

Edit `Bdropimpact.c`:
```c
#define VELOCITY            25.0      // Increase for higher We/Re
#define DROP_DIAMETER       0.0019    // Droplet size
```

Or use command-line arguments (if using `constants.h` setup):
```bash
./Bdropimpact R2000 W500  # Re=2000, We=500
```

### Change Grid Resolution

```c
int maxlevel = 14;  // Finer: ~1024 cells per diameter
int minlevel = 11;  // Coarser initial grid
```

**Warning**: Level 14 requires ~16× more memory than level 12.

### Change Droplet Shape

```c
#define ASPECT_RATIO 3.0  // More oblate initial shape (Dh/Dv = 3)
```

## Boundary Conditions

- **Left (inflow)**: Dirichlet velocity `u_x = 1`, Neumann pressure
- **Right (outflow)**: Neumann velocity, Dirichlet pressure `p = 0`
- **Top (radial boundary)**: No penetration, free slip
- **Axis (y=0)**: Axisymmetric boundary (automatic)

## Adaptive Mesh Refinement

The grid automatically refines based on:
1. **VOF gradient**: High refinement near the interface
2. **Velocity gradients**: Captures wake structures
3. **Wavelets**: `adapt_wavelet({f, u}, (double[]){1e-6, 1e-5, 1e-5}, maxlevel)`

## Troubleshooting

### Simulation Crashes

- **Reduce time step**: Basilisk auto-adjusts, but CFL violations can occur
- **Increase viscosity**: Add small artificial viscosity if instabilities occur
- **Check memory**: Level 13 with large domain requires ~16-32 GB RAM

### No Output Files

- Check write permissions
- Ensure `intermediate/` directory exists (created automatically)
- Verify `TOLERANCE` is not too strict (default `1e-6`)

### MPI Errors

- Ensure MPI library matches compiler
- Reduce process count if memory limited
- Check firewall settings for multi-node runs

## References

1. **Basilisk**: S. Popinet, "Basilisk flow solver," http://basilisk.fr (2013-2024)
2. **Bag Breakup**: Zhao et al., "Morphological classification of low viscosity drop bag breakup in a continuous air jet stream," *Phys. Fluids* 22, 114103 (2010)
3. **VOF Method**: Hirt & Nichols, "Volume of fluid (VOF) method for the dynamics of free boundaries," *J. Comput. Phys.* 39, 201-225 (1981)

## Citation

If you use this code in your research, please cite:
```
[Your Name/Institution], "Axisymmetric simulation of aerodynamic droplet breakup using Basilisk C,"
GitHub repository, 2024.
```

## License

This code is provided as-is for academic and research purposes. Basilisk is licensed under GPL.

## Contact

For questions or issues related to:
- **Basilisk solver**: http://basilisk.fr/Forum
- **This simulation**: [Your contact information]

---

**Last updated**: 2025-11-17
**Basilisk version**: Latest (compatible with 2024 releases)
**Tested platforms**: Linux (Ubuntu 22.04), macOS (with Homebrew)
