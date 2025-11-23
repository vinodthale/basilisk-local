# Basilisk Aerodynamic Breakup of a Vaporizing Drop - Complete Reproduction Guide

This comprehensive guide provides step-by-step instructions to reproduce the direct numerical simulation (DNS) results for aerodynamic breakup of vaporizing drops using the Basilisk solver, as described in Boyd & Ling (2023).

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [System Setup](#system-setup)
3. [Basilisk Installation](#basilisk-installation)
4. [Project Structure](#project-structure)
5. [Building the Simulations](#building-the-simulations)
6. [Running Each Test Case](#running-each-test-case)
7. [Verifying Results](#verifying-results)
8. [Advanced Options](#advanced-options)
9. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Software Requirements

- **Operating System**: Linux, macOS, or other Unix-like systems
- **C Compiler**: GCC (recommended) or Clang
- **Basilisk Solver**: Latest version from http://basilisk.fr/
- **Optional Dependencies**:
  - MPI (Open MPI or MPICH) for parallel execution
  - gnuplot for visualization
  - FFmpeg for movie generation

### Hardware Requirements

- **Minimum**: 4 GB RAM
- **Recommended**: 8+ GB RAM for parallel runs
- **Disk Space**: 5-10 GB for compilation and output files

### System Packages

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install build-essential git autoconf libtool \
    gfortran libopenmpi-dev openmpi-bin \
    gnuplot imagemagick ffmpeg
```

#### macOS (using Homebrew)
```bash
brew install gcc git autoconf libtool open-mpi gnuplot imagemagick ffmpeg
```

---

## System Setup

### Step 1: Create Working Directory

```bash
# Create a directory for simulations
mkdir -p ~/basilisk-work
cd ~/basilisk-work
```

### Step 2: Set Environment Variables

Add the following to your shell profile (`~/.bashrc`, `~/.zshrc`, etc.):

```bash
# Basilisk environment variables
export BASILISK_HOME=$HOME/basilisk
export PATH=$BASILISK_HOME/install/bin:$PATH
export LD_LIBRARY_PATH=$BASILISK_HOME/install/lib:$LD_LIBRARY_PATH
export CFLAGS="-O2 -Wall -std=c99"
```

Then source the profile:
```bash
source ~/.bashrc  # or ~/.zshrc
```

### Step 3: Verify C Compiler

```bash
gcc --version
which gcc
```

Expected output: GCC version 7.0 or higher

---

## Basilisk Installation

### Option A: Quick Install (Recommended)

```bash
# Clone Basilisk repository
mkdir -p ~/basilisk
cd ~/basilisk
git clone git://basilisk.fr/basilisk repo
cd repo

# Build and install
./autogen.sh
./configure --prefix=$HOME/basilisk/install
make -j4
make install
```

### Option B: Install from Source

```bash
cd ~/basilisk/repo
./configure --prefix=$HOME/basilisk/install \
    --enable-mpi \
    --with-mpi=openmpi
make -j4
make install
```

### Verify Installation

```bash
qcc --version
```

Expected output: Basilisk version information

---

## Project Structure

The vaporizing drop project is organized as follows:

```
aerodynamic breakup of a drop/Aerodynamic breakup of a vaporizing drop/
├── README.md                                    # Project overview
├── A consistent volume-of-fluid approach...pdf  # Reference paper
├── src/
│   ├── 01_vaporization/                         # Core vaporization model
│   │   ├── evap_include.h                       # Main include file
│   │   ├── temperature-phase-change.h           # Temperature equation
│   │   ├── centered_evap.h                      # Centered advection scheme
│   │   ├── project_evap.h                       # Pressure projection
│   │   ├── m_dot_functions.h                    # Mass transfer rate
│   │   ├── adapt_evap.h                         # Mesh adaptation
│   │   ├── interface_functions.h                # Interface operations
│   │   ├── stability_step.h                     # Stability constraints
│   │   └── [other supporting headers]
│   ├── properties/                              # Fluid properties
│   │   ├── water_vapor.h                        # Water/vapor properties
│   │   ├── liq_gas.h                            # Generic liquid/gas
│   │   └── film_boiling_case.h                  # Film boiling properties
│   ├── theory_evap/                             # Analytical solutions
│   │   ├── stef_calc.h                          # Stefan solution
│   │   ├── suck_calc.h                          # Sucking solution
│   │   └── bubble_growth_calcs.h                # Bubble growth solution
│   ├── LS_funcs/                                # Level-set functions
│   ├── common_functions/                        # Utility functions
│   ├── post_processing/                         # Visualization tools
│   └── outputs/                                 # Output utilities
├── run/                                         # Test case executables
│   ├── stefan_problem.c                         # Section 4.1 test
│   ├── sucking_problem.c                        # Section 4.2 test
│   ├── bubble_growth.c                          # Section 4.3 test
│   ├── film_boiling.c                           # Section 4.4 test
│   └── Makefile.txt
└── images/                                      # Domain diagrams
    ├── stef.png
    ├── suck.png
    ├── bubble_setup.png
    ├── film_domain.png
    ├── stef_plus_suck.png
    └── vaporizing_breaking_drop.png
```

---

## Building the Simulations

Navigate to the run directory:

```bash
cd aerodynamic\ breakup\ of\ a\ drop/Aerodynamic\ breakup\ of\ a\ vaporizing\ drop/run/
```

### Build All Test Cases

#### Serial Compilation

```bash
# Stefan Flow Problem
qcc -O2 -Wall stefan_problem.c -o stefan_problem -lm

# Sucking Problem
qcc -O2 -Wall sucking_problem.c -o sucking_problem -lm

# Bubble Growth
qcc -O2 -Wall bubble_growth.c -o bubble_growth -lm

# Film Boiling
qcc -O2 -Wall film_boiling.c -o film_boiling -lm
```

#### Parallel Compilation (MPI-enabled)

```bash
# Stefan Flow Problem with MPI
CC99='mpicc -std=c99' qcc -Wall -O2 -D_MPI=1 \
    stefan_problem.c -o stefan_problem_mpi -lm

# Sucking Problem with MPI
CC99='mpicc -std=c99' qcc -Wall -O2 -D_MPI=1 \
    sucking_problem.c -o sucking_problem_mpi -lm

# Bubble Growth with MPI
CC99='mpicc -std=c99' qcc -Wall -O2 -D_MPI=1 \
    bubble_growth.c -o bubble_growth_mpi -lm

# Film Boiling with MPI
CC99='mpicc -std=c99' qcc -Wall -O2 -D_MPI=1 \
    film_boiling.c -o film_boiling_mpi -lm
```

### Verify Build

```bash
ls -la stefan_problem sucking_problem bubble_growth film_boiling
```

---

## Running Each Test Case

### Test Case 1: Stefan Flow Problem (Section 4.1)

**Description**: Simulates vapor layer growth from a heated wall. This canonical problem validates the phase-change model against analytical solution.

**Physical Parameters**:
- Domain size: 1.0 mm
- Initial vapor layer thickness: 0.1 mm
- Wall temperature: T_sat + 10 K
- Fluid: Water/vapor at saturation
- Simulation time: 0.12 seconds

**Running the Simulation**:

```bash
# Serial execution
./stefan_problem

# Parallel execution (4 processes)
mpirun -np 4 ./stefan_problem_mpi
```

**Expected Output**:
- Temperature evolution in the domain
- Vapor-liquid interface tracking
- Comparison with analytical solution
- Output files: `stefan_problem/temperature.mp4`, `stefan_problem/*.dat`

**Key Outputs**:
- Interface position vs. time
- Temperature profiles
- Validation data file with analytical comparison

---

### Test Case 2: Sucking Problem (Section 4.2)

**Description**: Models evaporative flow where the receding interface draws liquid toward it.

**Physical Parameters**:
- Domain size: 1.0 m
- Initial vapor layer thickness: 0.02 m
- Superheat: 2 K above saturation
- Simulation time: Determined by convergence

**Running the Simulation**:

```bash
# Serial execution
./sucking_problem

# Parallel execution (4 processes)
mpirun -np 4 ./sucking_problem_mpi
```

**Expected Output**:
- Velocity profile in liquid and vapor
- Temperature and vapor generation
- Comparison with analytical solution
- Output files: `sucking_problem/*.dat`, visualization files

**Key Outputs**:
- Velocity field evolution
- Mass transfer rate validation
- Interface dynamics data

---

### Test Case 3: Bubble Growth (Section 4.3)

**Description**: Simulates bubble growth in superheated liquid. Tests the code's ability to handle 3D interfaces and evaporation dynamics.

**Physical Parameters**:
- Initial bubble radius: 0.12 m
- Domain: 5× initial radius (5× 0.12 m = 0.6 m)
- Liquid superheat: 2 K above saturation
- Mesh: Octree adaptive (3D) or 2D axisymmetric
- Simulation time: Until bubble reaches terminal size

**Running the Simulation**:

```bash
# Serial execution
./bubble_growth

# Parallel execution (8 processes for 3D)
mpirun -np 8 ./bubble_growth_mpi
```

**Expected Output**:
- 3D bubble interface evolution
- Temperature distribution around bubble
- Bubble volume tracking
- Comparison with analytical solution
- Output files: `bubble_growth/*.dat`, `bubble_growth/bubble.mp4`

**Key Outputs**:
- Bubble radius vs. time
- Mass transfer rate from surface
- Growth rate comparison with theory
- Temperature field snapshots

---

### Test Case 4: Film Boiling (Section 4.4)

**Description**: 2D simulation of film boiling with Rayleigh-Taylor instability at the vapor-liquid interface. Demonstrates vapor bubble formation and release.

**Physical Parameters**:
- Domain: Determined by Rayleigh-Taylor wavelength
- Gravity: 9.81 m/s²
- Wall superheat: 5 K above saturation
- Aspect ratio: Typically 2-3 wavelengths wide
- Simulation time: Until vapor bubbles form and detach

**Running the Simulation**:

```bash
# Serial execution (faster for 2D)
./film_boiling

# Parallel execution (4 processes)
mpirun -np 4 ./film_boiling_mpi
```

**Expected Output**:
- Rayleigh-Taylor wave growth
- Vapor bubble nucleation and growth
- Bubble detachment and rise
- Output files: `film_boiling/*.mp4`, `film_boiling/*.dat`

**Key Outputs**:
- Interface height profile evolution
- Bubble characteristics (size, frequency, rise velocity)
- Heat transfer rate data
- Instability wavelength validation

---

## Verifying Results

### Check Output Files

Each test case generates output files in its respective directory:

```bash
# Stefan Problem
ls -la stefan_problem/

# Expected files:
# - temperature.mp4 (visualization)
# - interface_position.dat (numerical data)
# - comparison_analytical.dat (validation data)
```

### Visualize Results

#### View Movies (if generated)

```bash
# Using any video player
ffplay stefan_problem/temperature.mp4
# or
vlc film_boiling/interface.mp4
```

#### Plot Data with gnuplot

```bash
# Example: Plot interface position
gnuplot << EOF
set xlabel "Time (s)"
set ylabel "Interface Position (m)"
plot "stefan_problem/interface_position.dat" with lines
EOF
```

#### Convert MP4 to Images (for presentations)

```bash
# Extract frames from movie
ffmpeg -i stefan_problem/temperature.mp4 \
    -vf fps=1 stefan_problem/frame_%04d.png
```

### Validation Against Analytical Solutions

The code includes analytical solutions for comparison. Check output files for error metrics:

```bash
# Display validation data
cat stefan_problem/comparison_analytical.dat | head -20
```

**Expected Results**:
- Interface position error < 5%
- Temperature profile error < 10%
- Vapor mass rate error < 3%

---

## Advanced Options

### Parallel Execution Guidelines

#### Desktop/Workstation (4-8 cores)

```bash
# Use 4-6 processes
mpirun -np 4 ./bubble_growth_mpi
```

#### Cluster/HPC Systems

```bash
# Use available cores (example: 128 cores)
mpirun -np 128 ./bubble_growth_mpi

# With SLURM scheduling
srun -n 128 ./bubble_growth_mpi
```

### Modifying Mesh Refinement

Edit the source files to adjust mesh refinement levels:

```c
// In each test case .c file:
MIN_LEVEL = 5;    // Minimum refinement level
LEVEL = 5;        // Base refinement level
MAX_LEVEL = 6;    // Maximum refinement level (near interfaces)
```

**Guidelines**:
- Higher MAX_LEVEL: Better accuracy, higher computational cost
- Typical range: MAX_LEVEL = LEVEL + 1 or LEVEL + 2
- Doubling resolution quadruples computation (2D) or octuple (3D)

### Adjusting Simulation Parameters

Edit parameters in the test case files:

```c
// Example: Stefan Problem
#define L 1.0e-3        // Domain size
#define H0 0.1e-3       // Initial vapor thickness
#define T_END (0.12)    // Simulation end time

#define T_wall (T_sat + 10.0)  // Wall superheat
```

### Timeout and Checkpointing

For long simulations:

```bash
# Run with timeout (e.g., 24 hours)
timeout 86400 mpirun -np 4 ./bubble_growth_mpi
```

---

## Troubleshooting

### Common Issues and Solutions

#### 1. Compilation Error: "qcc: command not found"

**Cause**: Basilisk not installed or not in PATH

**Solution**:
```bash
# Verify Basilisk installation
which qcc

# If not found, reinstall Basilisk
cd ~/basilisk/repo
./configure --prefix=$HOME/basilisk/install
make install

# Add to PATH
export PATH=$HOME/basilisk/install/bin:$PATH
```

#### 2. Compilation Error: "math.h: No such file or directory"

**Cause**: Missing math library

**Solution**:
```bash
# Ensure -lm flag is included (it should be in the commands above)
qcc -O2 -Wall stefan_problem.c -o stefan_problem -lm
```

#### 3. MPI Compilation Error: "mpicc: command not found"

**Cause**: MPI not installed

**Solution**:
```bash
# Ubuntu/Debian
sudo apt-get install openmpi-bin libopenmpi-dev

# macOS
brew install open-mpi

# Verify installation
which mpicc
mpicc --version
```

#### 4. Runtime Error: "Floating point exception"

**Cause**: Usually numerical instability or invalid input

**Solution**:
- Check physical parameters (wall temperature, superheat)
- Verify domain size is reasonable
- Try lower MAX_LEVEL (less aggressive refinement)
- Reduce CFL number in initialization

#### 5. Out of Memory

**Cause**: Domain too large or refinement level too high

**Solution**:
```bash
# Reduce MAX_LEVEL
# Reduce domain size L
# Use fewer MPI processes

# Monitor memory usage
ps aux | grep [test_case_name]
```

#### 6. Slow Simulation

**Cause**: Single-core execution or excessive refinement

**Solution**:
```bash
# Use parallel execution
mpirun -np 4 ./bubble_growth_mpi

# Reduce MAX_LEVEL by 1
# Check system load: top or htop
```

#### 7. No Output Files Generated

**Cause**: Simulation crashed silently or working directory not writable

**Solution**:
```bash
# Ensure run/ directory is writable
chmod u+w .

# Check for error messages
./stefan_problem 2>&1 | tee output.log

# Verify code compiles without warnings
qcc -O2 -Wall -Werror stefan_problem.c -o test -lm
```

### Debugging Tips

#### Enable Verbose Output

Modify the test case source to add debug output:

```c
// In event statements
if (i % 10 == 0) {
    printf("Time: %e, Interface: %e\n", t, interface_position);
    fflush(stdout);
}
```

#### Check Intermediate Files

```bash
# List all generated files
find . -type f -newer stefan_problem

# Check file sizes (to ensure data is being written)
du -sh ./[test_case_name]/
```

#### Test with Minimal Configuration

```bash
# Create a simple test
MIN_LEVEL = 3
LEVEL = 3
MAX_LEVEL = 4
T_END = 0.01  # Short simulation

# Compile and run
qcc -O2 -Wall stefan_problem.c -o test_small -lm
./test_small
```

---

## Reference Information

### Paper Citation

Boyd, B., & Ling, Y. (2023). *A consistent volume-of-fluid approach for direct numerical simulation of the aerodynamic breakup of a vaporizing drop.* Computers & Fluids, 105807.

DOI: [10.1016/j.compfluid.2023.105807](https://doi.org/10.1016/j.compfluid.2023.105807)

### Key Physical Quantities

| Quantity | Symbol | Units | Typical Value |
|----------|--------|-------|----------------|
| Stefan number | Ste | - | 0.01-1.0 |
| Superheat | ΔT | K | 2-10 |
| Initial interface velocity | V_0 | m/s | 0.1-10 |
| Evaporation rate | ṁ | kg/(m²·s) | 0.001-0.1 |

### Important Parameters

- **MIN_LEVEL**: Coarsest mesh level (typically 3-5)
- **LEVEL**: Base refinement level (typically 5-7)
- **MAX_LEVEL**: Finest mesh near interfaces (typically LEVEL + 1-2)
- **CFL**: Courant number (typically 0.1-0.2)
- **T_END**: Total simulation time (problem-dependent)

---

## Next Steps

1. **Verify Installation**: Run the stefan_problem first (simplest case)
2. **Validate Results**: Compare output with analytical solutions
3. **Explore Parameters**: Adjust mesh refinement and superheat
4. **Scale Up**: Try more complex cases (bubble growth, film boiling)
5. **Parallelization**: Test MPI efficiency on your system
6. **Customization**: Modify properties or geometry for your research

---

## Additional Resources

- **Basilisk Documentation**: http://basilisk.fr/
- **VOF Method**: http://basilisk.fr/src/vof.h
- **Adaptive Mesh Refinement**: http://basilisk.fr/src/adapt.h
- **Two-Phase Flow**: http://basilisk.fr/src/two-phase.h

---

## Support and Feedback

For questions specific to this code:
1. Check the original paper (included as PDF)
2. Review the Basilisk documentation
3. Examine the source code comments
4. Test with the analytical validation cases

For Basilisk-specific issues:
- Visit: http://basilisk.fr/
- Forum: http://basilisk.fr/Forum.html

---

**Last Updated**: November 2024
**Version**: 1.0
**Status**: Complete

